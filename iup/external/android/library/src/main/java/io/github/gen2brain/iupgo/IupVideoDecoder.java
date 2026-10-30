package io.github.gen2brain.iupgo;

import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTimestamp;
import android.media.AudioTrack;
import android.media.MediaCodec;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.net.Uri;
import android.view.Surface;
import android.view.View;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import androidx.annotation.WorkerThread;

final class IupVideoDecoder extends IupVideoHelper.Player
{
    private static final long TIMEOUT_US = 10000;
    private static final long LATE_US = 80000;
    private static final long STALL_NS = 500000000L;

    private final String audioSource;
    private final Object lock = new Object();
    private final AudioTimestamp stamp = new AudioTimestamp();
    private final ExecutorService control = Executors.newSingleThreadExecutor(task -> new Thread(task, "IupVideoControl"));

    private MediaExtractor videoExtractor, audioExtractor;
    private MediaFormat videoFormat;
    private volatile MediaCodec videoCodec;
    private MediaCodec audioCodec;
    private AudioTrack track;
    private Thread videoThread, audioThread, subtitleThread;
    private final ArrayList<MediaFormat> textFormats = new ArrayList<>();
    private final ArrayList<Integer> textTracks = new ArrayList<>();
    private int selectedText = -1;
    private volatile int subtitleGeneration;
    private volatile boolean running;
    private long durationUs;
    private int deviceId = -1;
    private int sampleRate, frameSize, rotation, seekFast;

    private boolean playingNow, audioClock, needFrame, videoEnd, audioEnd, surfaceLost, seekPending;
    private long clockBaseUs, clockStartNs, audioBaseUs, dropUntilUs, seekUs, lastClockUs = -1, lastAdvanceNs, pausedFrames = -1;

    IupVideoDecoder(View host, long session, String source, boolean url, String audioSource)
    {
        super(host, session, source, url);
        this.audioSource = audioSource;
    }

    private void post(Runnable task)
    {
        host.post(() -> {
            if (!closed)
                task.run();
        });
    }

    private void postError(Exception error)
    {
        post(() -> fail(error.toString()));
    }

    private void postWorkerError(Exception error)
    {
        if (running)
            postError(error);
    }

    private MediaExtractor newExtractor(String source, boolean url) throws Exception
    {
        MediaExtractor extractor = new MediaExtractor();
        if (url)
            extractor.setDataSource(context, Uri.parse(source), null);
        else
            extractor.setDataSource(source);
        return extractor;
    }

    private static int findTrack(MediaExtractor extractor, String prefix)
    {
        for (int i = 0; i < extractor.getTrackCount(); i++)
        {
            String mime = extractor.getTrackFormat(i).getString(MediaFormat.KEY_MIME);
            if (mime != null && mime.startsWith(prefix))
                return i;
        }
        return -1;
    }

    private boolean rotated()
    {
        return rotation == 90 || rotation == 270;
    }

    private static long formatDuration(MediaFormat format)
    {
        return format != null && format.containsKey(MediaFormat.KEY_DURATION) ? format.getLong(MediaFormat.KEY_DURATION) : 0;
    }

    @Override
    void open()
    {
        control.execute(this::prepare);
    }

    private void prepare()
    {
        try
        {
            videoExtractor = newExtractor(source, sourceURL);
            audioExtractor = newExtractor(audioSource, true);
            int videoTrack = findTrack(videoExtractor, "video/");
            int audioTrack = findTrack(audioExtractor, "audio/");
            if (videoTrack < 0 && audioTrack < 0)
                throw new IllegalArgumentException("No video or audio track");
            MediaFormat audioFormat = null;
            if (videoTrack >= 0)
            {
                videoExtractor.selectTrack(videoTrack);
                videoFormat = videoExtractor.getTrackFormat(videoTrack);
                if (videoFormat.containsKey("rotation-degrees"))
                    rotation = videoFormat.getInteger("rotation-degrees");
                for (int i = 0; i < videoExtractor.getTrackCount(); i++)
                {
                    MediaFormat format = videoExtractor.getTrackFormat(i);
                    if (IupVideoHelper.isTimedText(format.getString(MediaFormat.KEY_MIME)))
                    {
                        textTracks.add(i);
                        textFormats.add(format);
                    }
                }
            }
            else
            {
                videoExtractor.release();
                videoExtractor = null;
            }
            if (audioTrack >= 0)
            {
                audioExtractor.selectTrack(audioTrack);
                audioFormat = audioExtractor.getTrackFormat(audioTrack);
                audioCodec = MediaCodec.createDecoderByType(audioFormat.getString(MediaFormat.KEY_MIME));
                audioCodec.configure(audioFormat, null, null, 0);
                audioCodec.start();
            }
            else
            {
                audioExtractor.release();
                audioExtractor = null;
            }
            durationUs = Math.max(formatDuration(videoFormat), formatDuration(audioFormat));
            int fw = videoFormat != null ? videoFormat.getInteger(MediaFormat.KEY_WIDTH) : 0;
            int fh = videoFormat != null ? videoFormat.getInteger(MediaFormat.KEY_HEIGHT) : 0;
            final int w = rotated() ? fh : fw, h = rotated() ? fw : fh;
            final boolean hasVideo = videoTrack >= 0, hasAudio = audioTrack >= 0;
            post(() -> {
                prepared = true;
                video = hasVideo;
                audio = hasAudio;
                width = w;
                height = h;
                volume(level, mute);
                layout();
                if (!video)
                {
                    control.execute(() -> {
                        try
                        {
                            restart();
                        }
                        catch (Exception error)
                        {
                            postError(error);
                        }
                    });
                }
                else if (surface != null)
                    attachSurface(surface);
                play(requested);
            });
        }
        catch (Exception error)
        {
            postError(error);
        }
    }

    private void attachSurface(Surface target)
    {
        control.execute(() -> {
            if (closed)
                return;
            try
            {
                stopWorkers();
                releaseVideoCodec();
                MediaCodec codec = MediaCodec.createDecoderByType(videoFormat.getString(MediaFormat.KEY_MIME));
                codec.configure(videoFormat, target, null, 0);
                codec.start();
                videoCodec = codec;
                restart();
            }
            catch (Exception error)
            {
                postError(error);
            }
        });
    }

    private void restart()
    {
        long at;
        synchronized (lock)
        {
            at = clockUs();
        }
        seekExtractors(at, 0);
        startWorkers();
    }

    private void releaseVideoCodec()
    {
        MediaCodec codec = videoCodec;
        videoCodec = null;
        if (codec != null)
        {
            try
            {
                codec.stop();
            }
            catch (IllegalStateException ignored)
            {
            }
            codec.release();
        }
    }

    private void startWorkers()
    {
        if (running || closed || (videoFormat != null && videoCodec == null))
            return;
        running = true;
        if (videoCodec != null)
        {
            videoThread = new Thread(this::videoLoop, "IupVideoDecode");
            videoThread.start();
        }
        if (audioCodec != null)
        {
            audioThread = new Thread(this::audioLoop, "IupAudioDecode");
            audioThread.start();
        }
    }

    private void stopWorkers()
    {
        running = false;
        synchronized (lock)
        {
            lock.notifyAll();
        }
        join(videoThread);
        join(audioThread);
        videoThread = null;
        audioThread = null;
    }

    private static void join(Thread thread)
    {
        if (thread == null)
            return;
        boolean interrupted = false;
        while (thread.isAlive())
        {
            try
            {
                thread.join();
            }
            catch (InterruptedException error)
            {
                interrupted = true;
            }
        }
        if (interrupted)
            Thread.currentThread().interrupt();
    }

    private void seekExtractors(long us, int fast)
    {
        long target = us;
        if (videoExtractor != null)
        {
            videoExtractor.seekTo(us, fast > 0 ? MediaExtractor.SEEK_TO_NEXT_SYNC : MediaExtractor.SEEK_TO_PREVIOUS_SYNC);
            long sample = videoExtractor.getSampleTime();
            if (fast != 0 && sample >= 0)
                target = sample;
        }
        if (audioExtractor != null)
            audioExtractor.seekTo(target, MediaExtractor.SEEK_TO_PREVIOUS_SYNC);
        if (videoCodec != null)
            videoCodec.flush();
        if (audioCodec != null)
            audioCodec.flush();
        synchronized (lock)
        {
            dropUntilUs = target;
            clockBaseUs = target;
            audioBaseUs = target;
            audioClock = false;
            videoEnd = videoExtractor == null;
            audioEnd = audioExtractor == null;
            clockStartNs = playingNow && systemClock() ? System.nanoTime() : 0;
            needFrame = true;
            lastClockUs = -1;
            pausedFrames = -1;
            subtitleGeneration++;
            lock.notifyAll();
            if (track != null)
            {
                track.pause();
                track.flush();
                if (playingNow)
                    track.play();
            }
        }
    }

    private boolean systemClock()
    {
        return audioCodec == null || audioEnd;
    }

    private long clockUs()
    {
        if (audioClock && track != null && sampleRate > 0)
            return audioBaseUs + audioFrames() * 1000000L / sampleRate;
        if (clockStartNs != 0)
            return clockBaseUs + (System.nanoTime() - clockStartNs) / 1000;
        return clockBaseUs;
    }

    private long audioFrames()
    {
        if (!playingNow && pausedFrames >= 0)
            return pausedFrames;
        long head = track.getPlaybackHeadPosition() & 0xffffffffL;
        if (playingNow && track.getTimestamp(stamp))
        {
            long frames = stamp.framePosition + (System.nanoTime() - stamp.nanoTime) * sampleRate / 1000000000L;
            if (frames >= 0 && frames <= head)
                return frames;
        }
        return head;
    }

    private static boolean feed(MediaCodec codec, MediaExtractor extractor)
    {
        int index = codec.dequeueInputBuffer(TIMEOUT_US);
        if (index < 0)
            return false;
        ByteBuffer buffer = codec.getInputBuffer(index);
        int size = buffer != null ? extractor.readSampleData(buffer, 0) : -1;
        if (size < 0)
        {
            codec.queueInputBuffer(index, 0, 0, 0, MediaCodec.BUFFER_FLAG_END_OF_STREAM);
            return true;
        }
        codec.queueInputBuffer(index, 0, size, extractor.getSampleTime(), 0);
        extractor.advance();
        return false;
    }

    private void videoLoop()
    {
        MediaCodec codec = videoCodec;
        MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
        boolean inputDone = false;
        try
        {
            while (running)
            {
                if (!inputDone)
                    inputDone = feed(codec, videoExtractor);
                int index = codec.dequeueOutputBuffer(info, TIMEOUT_US);
                if (index == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED)
                {
                    videoSize(codec.getOutputFormat());
                    continue;
                }
                if (index < 0)
                    continue;
                if ((info.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0)
                {
                    codec.releaseOutputBuffer(index, false);
                    endOfStream(true);
                    return;
                }
                codec.releaseOutputBuffer(index, present(info.presentationTimeUs));
            }
        }
        catch (Exception error)
        {
            synchronized (lock)
            {
                if (surfaceLost)
                    return;
            }
            postWorkerError(error);
        }
    }

    private boolean present(long pts) throws InterruptedException
    {
        synchronized (lock)
        {
            if (pts < dropUntilUs)
                return false;
            if (needFrame)
            {
                needFrame = false;
                if (!playingNow)
                    return true;
            }
            while (running)
            {
                if (!playingNow)
                {
                    lock.wait(20);
                    continue;
                }
                long delay = pts - clockUs();
                if (delay <= 4000)
                    return delay > -LATE_US;
                lock.wait(Math.max(1, Math.min(delay / 1000, 20)));
            }
            return false;
        }
    }

    private void videoSize(MediaFormat format)
    {
        int w = format.getInteger(MediaFormat.KEY_WIDTH), h = format.getInteger(MediaFormat.KEY_HEIGHT);
        if (format.containsKey("crop-left") && format.containsKey("crop-right"))
            w = format.getInteger("crop-right") - format.getInteger("crop-left") + 1;
        if (format.containsKey("crop-top") && format.containsKey("crop-bottom"))
            h = format.getInteger("crop-bottom") - format.getInteger("crop-top") + 1;
        final int fw = rotated() ? h : w, fh = rotated() ? w : h;
        post(() -> {
            width = fw;
            height = fh;
            layout();
        });
    }

    @WorkerThread
    private void audioLoop()
    {
        MediaCodec codec = audioCodec;
        MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
        boolean inputDone = false;
        try
        {
            while (running)
            {
                if (!inputDone)
                    inputDone = feed(codec, audioExtractor);
                int index = codec.dequeueOutputBuffer(info, TIMEOUT_US);
                if (index == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED)
                {
                    audioOutput(codec.getOutputFormat());
                    continue;
                }
                if (index < 0)
                    continue;
                if ((info.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0)
                {
                    codec.releaseOutputBuffer(index, false);
                    endOfStream(false);
                    return;
                }
                if (track == null)
                    audioOutput(codec.getOutputFormat());
                ByteBuffer output = codec.getOutputBuffer(index);
                if (output != null && info.size > 0)
                {
                    output.position(info.offset);
                    output.limit(info.offset + info.size);
                    write(output, info.presentationTimeUs);
                }
                codec.releaseOutputBuffer(index, false);
            }
        }
        catch (Exception error)
        {
            postWorkerError(error);
        }
    }

    private void write(ByteBuffer output, long pts) throws InterruptedException
    {
        AudioTrack target;
        synchronized (lock)
        {
            if (pts < dropUntilUs)
            {
                long skip = (dropUntilUs - pts) * sampleRate / 1000000L * frameSize;
                if (skip >= output.remaining())
                    return;
                output.position(output.position() + (int)skip);
                pts = dropUntilUs;
            }
            if (!audioClock && !audioEnd)
            {
                audioBaseUs = pts;
                audioClock = true;
                lastClockUs = -1;
                pausedFrames = -1;
            }
            target = track;
        }
        while (output.hasRemaining() && running)
        {
            int written = target.write(output, output.remaining(), AudioTrack.WRITE_NON_BLOCKING);
            if (written < 0)
                throw new IllegalStateException("AudioTrack write error " + written);
            if (written == 0)
            {
                synchronized (lock)
                {
                    lock.wait(5);
                }
            }
        }
    }

    private void audioOutput(MediaFormat format)
    {
        int rate = format.getInteger(MediaFormat.KEY_SAMPLE_RATE);
        int channels = format.getInteger(MediaFormat.KEY_CHANNEL_COUNT);
        int encoding = format.containsKey(MediaFormat.KEY_PCM_ENCODING) ?
            format.getInteger(MediaFormat.KEY_PCM_ENCODING) : AudioFormat.ENCODING_PCM_16BIT;
        int mask;
        switch (channels)
        {
            case 1: mask = AudioFormat.CHANNEL_OUT_MONO; break;
            case 2: mask = AudioFormat.CHANNEL_OUT_STEREO; break;
            case 4: mask = AudioFormat.CHANNEL_OUT_QUAD; break;
            case 6: mask = AudioFormat.CHANNEL_OUT_5POINT1; break;
            case 8: mask = AudioFormat.CHANNEL_OUT_7POINT1_SURROUND; break;
            default: throw new IllegalArgumentException("Unsupported audio channel count " + channels);
        }
        int bytes = encoding == AudioFormat.ENCODING_PCM_FLOAT ? 4 : encoding == AudioFormat.ENCODING_PCM_8BIT ? 1 : 2;
        int minimum = AudioTrack.getMinBufferSize(rate, mask, encoding);
        AudioTrack created = new AudioTrack(
            new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_MEDIA).setContentType(AudioAttributes.CONTENT_TYPE_MOVIE).build(),
            new AudioFormat.Builder().setSampleRate(rate).setChannelMask(mask).setEncoding(encoding).build(),
            Math.max(minimum, 1) * 4, AudioTrack.MODE_STREAM, AudioManager.AUDIO_SESSION_ID_GENERATE);
        float value = mute ? 0 : level / 100.0f;
        created.setVolume(value);
        if (deviceId >= 0)
            created.setPreferredDevice(IupVideoHelper.findAudioDevice(context, deviceId));
        AudioTrack previous;
        synchronized (lock)
        {
            previous = track;
            track = created;
            sampleRate = rate;
            frameSize = channels * bytes;
            audioClock = false;
            pausedFrames = -1;
            if (playingNow)
                created.play();
        }
        if (previous != null)
            previous.release();
    }

    private void endOfStream(boolean isVideo)
    {
        synchronized (lock)
        {
            if (isVideo)
                videoEnd = true;
            else
            {
                clockBaseUs = clockUs();
                audioClock = false;
                audioEnd = true;
                clockStartNs = playingNow ? System.nanoTime() : 0;
            }
            if (!videoEnd || !audioEnd || !playingNow)
                return;
            clockBaseUs = clockUs();
            clockStartNs = 0;
            playingNow = false;
            if (track != null)
                track.pause();
        }
        post(() -> {
            ended = true;
            host.setKeepScreenOn(false);
        });
    }

    @Override
    void play(boolean play)
    {
        requested = play;
        host.setKeepScreenOn(play && !failed);
        if (closed || failed || !prepared || (video && surface == null))
            return;
        synchronized (lock)
        {
            if (playingNow == play)
                return;
            if (play && videoEnd && audioEnd)
            {
                post(() -> ended = true);
                return;
            }
            if (!play && systemClock())
                clockBaseUs = clockUs();
            pausedFrames = !play && audioClock && track != null ? audioFrames() : -1;
            playingNow = play;
            if (systemClock())
                clockStartNs = play ? System.nanoTime() : 0;
            if (track != null)
            {
                if (play)
                    track.play();
                else
                    track.pause();
            }
            lastClockUs = -1;
            lock.notifyAll();
        }
    }

    @Override
    void seek(double position, int fast)
    {
        if (closed || failed || !prepared)
            return;
        ended = false;
        synchronized (lock)
        {
            seekUs = (long)(position * 1000000);
            seekFast = fast;
            seekPending = true;
            videoEnd = videoFormat == null;
            audioEnd = audioCodec == null;
        }
        control.execute(() -> {
            long target;
            int snap;
            synchronized (lock)
            {
                if (!seekPending || closed)
                    return;
                target = seekUs;
                snap = seekFast;
                seekPending = false;
            }
            try
            {
                stopWorkers();
                seekExtractors(target, snap);
                startWorkers();
            }
            catch (Exception error)
            {
                postError(error);
            }
        });
    }

    @Override
    void volume(int volume, boolean mute)
    {
        level = volume;
        this.mute = mute;
        synchronized (lock)
        {
            if (track != null)
                track.setVolume(mute ? 0 : volume / 100.0f);
        }
    }

    @Override
    void audioDevice(int id)
    {
        deviceId = id;
        synchronized (lock)
        {
            if (track != null)
                track.setPreferredDevice(IupVideoHelper.findAudioDevice(context, id));
        }
    }

    @Override
    boolean playing()
    {
        synchronized (lock)
        {
            return playingNow;
        }
    }

    @Override
    double position()
    {
        long us;
        synchronized (lock)
        {
            us = Math.max(0, clockUs());
        }
        if (durationUs > 0)
            us = Math.min(us, durationUs);
        return us / 1000000.0;
    }

    @Override
    double duration()
    {
        return durationUs / 1000000.0;
    }

    @Override
    double[] poll()
    {
        synchronized (lock)
        {
            long now = System.nanoTime();
            if (!playingNow)
                buffering = false;
            else
            {
                long clock = clockUs();
                if (clock != lastClockUs)
                {
                    lastClockUs = clock;
                    lastAdvanceNs = now;
                    buffering = false;
                }
                else if (now - lastAdvanceNs > STALL_NS)
                    buffering = true;
            }
        }
        return super.poll();
    }

    @Override
    int subtitleCount()
    {
        return prepared && !failed && !closed ? textTracks.size() : 0;
    }

    @Override
    String subtitleName(int index, boolean language)
    {
        if (!prepared || failed || closed || index < 1 || index > textTracks.size())
            return null;
        if (!language)
            return "Track " + index;
        MediaFormat format = textFormats.get(index - 1);
        return format.containsKey(MediaFormat.KEY_LANGUAGE) ? format.getString(MediaFormat.KEY_LANGUAGE) : null;
    }

    @Override
    boolean subtitleSelect(int index, double delay)
    {
        if (!prepared || failed || closed)
            return index <= 0;
        int selected = -1;
        if (index == -1)
        {
            for (int i = 0; i < textTracks.size() && selected < 0; i++)
            {
                if (IupVideoHelper.subtitleDefault(textFormats.get(i)))
                    selected = textTracks.get(i);
            }
        }
        else if (index > 0)
        {
            if (index > textTracks.size())
                return false;
            selected = textTracks.get(index - 1);
        }
        synchronized (lock)
        {
            if (selected == selectedText)
                return true;
            selectedText = selected;
            subtitleGeneration++;
            lock.notifyAll();
            if (selected >= 0 && subtitleThread == null)
            {
                subtitleThread = new Thread(this::subtitleLoop, "IupSubtitle");
                subtitleThread.start();
            }
        }
        if (session != 0)
            IupVideoHelper.subtitleCue(session, null);
        return true;
    }

    private boolean subtitleCurrent(int track, int generation)
    {
        return !closed && selectedText == track && subtitleGeneration == generation;
    }

    @WorkerThread
    private void subtitleLoop()
    {
        MediaExtractor extractor = null;
        ByteBuffer buffer = ByteBuffer.allocate(64 * 1024);
        int track = -1, generation = -1;
        try
        {
            while (!closed)
            {
                int wanted, current;
                long at;
                synchronized (lock)
                {
                    wanted = selectedText;
                    current = subtitleGeneration;
                    at = clockUs();
                }
                if (wanted < 0)
                    return;
                if (extractor == null || wanted != track)
                {
                    if (extractor != null)
                        extractor.release();
                    extractor = newExtractor(source, sourceURL);
                    extractor.selectTrack(wanted);
                    track = wanted;
                    generation = -1;
                }
                if (current != generation)
                {
                    extractor.seekTo(at, MediaExtractor.SEEK_TO_PREVIOUS_SYNC);
                    generation = current;
                    final int cleared = current;
                    post(() -> {
                        if (session != 0 && subtitleGeneration == cleared)
                            IupVideoHelper.subtitleCue(session, null);
                    });
                }
                long pts = extractor.getSampleTime();
                boolean show = false;
                byte[] text = null;
                if (pts >= 0)
                {
                    buffer.clear();
                    text = IupVideoHelper.timedText(buffer, extractor.readSampleData(buffer, 0));
                }
                synchronized (lock)
                {
                    while (subtitleCurrent(track, generation))
                    {
                        long delay = pts < 0 ? Long.MAX_VALUE : pts - clockUs();
                        if (delay <= 0)
                        {
                            show = true;
                            break;
                        }
                        lock.wait(Math.max(1, Math.min(delay / 1000, 50)));
                    }
                }
                if (!show)
                    continue;
                final byte[] cue = text;
                final int shown = generation;
                post(() -> {
                    if (session != 0 && subtitleGeneration == shown)
                        IupVideoHelper.subtitleCue(session, cue);
                });
                extractor.advance();
            }
        }
        catch (Exception ignored)
        {
        }
        finally
        {
            if (extractor != null)
                extractor.release();
            synchronized (lock)
            {
                subtitleThread = null;
            }
        }
    }

    @Override
    void surfaceAvailable()
    {
        if (prepared && video)
            attachSurface(surface);
        play(requested);
    }

    @Override
    void surfaceDestroyed()
    {
        synchronized (lock)
        {
            if (playingNow)
            {
                if (systemClock())
                    clockBaseUs = clockUs();
                if (audioClock && track != null)
                    pausedFrames = audioFrames();
                playingNow = false;
                clockStartNs = 0;
                if (track != null)
                    track.pause();
            }
            surfaceLost = true;
            lock.notifyAll();
        }
        running = false;
        MediaCodec codec = videoCodec;
        if (codec != null)
        {
            try
            {
                codec.stop();
            }
            catch (IllegalStateException ignored)
            {
            }
        }
        control.execute(() -> {
            stopWorkers();
            releaseVideoCodec();
            synchronized (lock)
            {
                surfaceLost = false;
            }
        });
    }

    @Override
    void release()
    {
        running = false;
        synchronized (lock)
        {
            surfaceLost = true;
            lock.notifyAll();
        }
        MediaCodec codec = videoCodec;
        if (codec != null)
        {
            try
            {
                codec.stop();
            }
            catch (IllegalStateException ignored)
            {
            }
        }
        if (surface != null)
        {
            surface.release();
            surface = null;
        }
        control.execute(() -> {
            stopWorkers();
            releaseVideoCodec();
            if (audioCodec != null)
            {
                try
                {
                    audioCodec.stop();
                }
                catch (IllegalStateException ignored)
                {
                }
                audioCodec.release();
                audioCodec = null;
            }
            synchronized (lock)
            {
                if (track != null)
                {
                    track.release();
                    track = null;
                }
            }
            if (videoExtractor != null)
                videoExtractor.release();
            if (audioExtractor != null)
                audioExtractor.release();
            videoExtractor = null;
            audioExtractor = null;
        });
        control.shutdown();
    }
}
