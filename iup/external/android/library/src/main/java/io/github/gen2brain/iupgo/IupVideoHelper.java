package io.github.gen2brain.iupgo;

import android.graphics.Matrix;
import android.graphics.Rect;
import android.graphics.SurfaceTexture;
import android.content.Context;
import android.media.AudioDeviceInfo;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.net.Uri;
import android.os.Build;
import android.view.Surface;
import android.view.TextureView;
import android.view.View;
import android.view.ViewGroup;
import android.view.Gravity;
import android.widget.TextView;
import android.graphics.Color;
import android.graphics.Typeface;
import java.io.ByteArrayOutputStream;
import java.io.FileInputStream;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.nio.ByteBuffer;
import java.util.ArrayList;

import androidx.annotation.Keep;

public final class IupVideoHelper
{
    static final int LOADING = 1;
    static final int PLAYING = 3;
    static final int PAUSED = 4;
    static final int BUFFERING = 5;
    static final int ERROR = 7;

    private IupVideoHelper() {}

    @Keep
    public static Object create(View host, long session, String source, boolean url, String audio)
    {
        Player player = audio != null ? new IupVideoDecoder(host, session, source, url, audio) : new NativePlayer(host, session, source, url);
        player.open();
        player.layout();
        return player;
    }

    @Keep
    public static void close(Object object)
    {
        ((Player)object).close();
    }

    @Keep
    public static void play(Object object, boolean play)
    {
        ((Player)object).play(play);
    }

    @Keep
    public static void seek(Object object, double position, int fast)
    {
        ((Player)object).seek(position, fast);
    }

    @Keep
    public static void volume(Object object, int volume, boolean mute)
    {
        ((Player)object).volume(volume, mute);
    }

    @Keep
    public static String[] audioDevices()
    {
        if (Build.VERSION.SDK_INT < 28)
            return new String[0];
        ArrayList<String> list = new ArrayList<>();
        AudioManager manager = (AudioManager)IupCommon.getContextThemeWrapper().getSystemService(Context.AUDIO_SERVICE);
        for (AudioDeviceInfo device : manager.getDevices(AudioManager.GET_DEVICES_OUTPUTS))
        {
            String name = audioDeviceName(device);
            if (name == null)
                continue;
            list.add(String.valueOf(device.getId()));
            list.add(name);
        }
        return list.toArray(new String[0]);
    }

    private static String audioDeviceName(AudioDeviceInfo device)
    {
        String type;
        switch (device.getType())
        {
            case AudioDeviceInfo.TYPE_BUILTIN_SPEAKER: return "Speaker";
            case AudioDeviceInfo.TYPE_BUILTIN_EARPIECE: return "Earpiece";
            case AudioDeviceInfo.TYPE_WIRED_HEADSET: return "Wired headset";
            case AudioDeviceInfo.TYPE_WIRED_HEADPHONES: return "Wired headphones";
            case AudioDeviceInfo.TYPE_LINE_ANALOG:
            case AudioDeviceInfo.TYPE_LINE_DIGITAL: return "Line out";
            case AudioDeviceInfo.TYPE_TELEPHONY:
            case AudioDeviceInfo.TYPE_REMOTE_SUBMIX:
            case AudioDeviceInfo.TYPE_BUILTIN_SPEAKER_SAFE: return null;
            case AudioDeviceInfo.TYPE_BLUETOOTH_A2DP:
            case AudioDeviceInfo.TYPE_BLUETOOTH_SCO: type = "Bluetooth"; break;
            case AudioDeviceInfo.TYPE_HDMI:
            case AudioDeviceInfo.TYPE_HDMI_ARC: type = "HDMI"; break;
            case AudioDeviceInfo.TYPE_USB_DEVICE:
            case AudioDeviceInfo.TYPE_USB_ACCESSORY:
            case AudioDeviceInfo.TYPE_USB_HEADSET: type = "USB"; break;
            default: type = "Audio output"; break;
        }
        CharSequence product = device.getProductName();
        return product == null || product.length() == 0 ? type : product + " (" + type + ")";
    }

    static AudioDeviceInfo findAudioDevice(Context context, int id)
    {
        if (Build.VERSION.SDK_INT < 28)
            return null;
        AudioManager manager = (AudioManager)context.getSystemService(Context.AUDIO_SERVICE);
        for (AudioDeviceInfo device : manager.getDevices(AudioManager.GET_DEVICES_OUTPUTS))
        {
            if (device.getId() == id)
                return device;
        }
        return null;
    }

    @Keep
    public static void audioDevice(Object object, int id)
    {
        ((Player)object).audioDevice(id);
    }

    @Keep
    public static void layout(Object object)
    {
        ((Player)object).layout();
    }

    @Keep
    public static double[] poll(Object object)
    {
        return ((Player)object).poll();
    }

    @Keep
    private static native void error(long session, String message);

    @Keep
    static native void subtitleCue(long session, byte[] text);

    @Keep
    public static void subtitleText(Object object, String text, int size, int color)
    {
        Player player = (Player)object;
        player.caption.setText(text == null ? "" : text);
        player.caption.setTextSize(size);
        player.caption.setTextColor(color);
        player.placed = false;
        player.layout();
    }

    @Keep
    public static int subtitleCount(Object object)
    {
        return ((Player)object).subtitleCount();
    }

    @Keep
    public static String subtitleName(Object object, int index, boolean language)
    {
        return ((Player)object).subtitleName(index, language);
    }

    @Keep
    public static boolean subtitleSelect(Object object, int index, double delay)
    {
        return ((Player)object).subtitleSelect(index, delay);
    }

    @Keep
    public static byte[] readSubtitle(String source)
    {
        android.app.Activity activity = IupActivity.currentActivity();
        try (InputStream input = source.startsWith("content:") && activity != null ?
             activity.getContentResolver().openInputStream(Uri.parse(source)) : new FileInputStream(source))
        {
            if (input == null)
                return null;
            ByteArrayOutputStream output = new ByteArrayOutputStream();
            byte[] bytes = new byte[8192];
            int count;
            while ((count = input.read(bytes)) != -1)
            {
                if (output.size() + count > 16 * 1024 * 1024)
                    return null;
                output.write(bytes, 0, count);
            }
            return output.toByteArray();
        }
        catch (Exception error)
        {
            return null;
        }
    }

    abstract static class Player implements TextureView.SurfaceTextureListener
    {
        final View host;
        final TextureView view;
        final TextView caption;
        final String source;
        final boolean sourceURL;
        final double[] snapshot = new double[10];
        final Rect clip = new Rect();
        final Rect placedClip = new Rect();
        final Matrix matrix = new Matrix();
        ViewGroup parent;
        Surface surface;
        long session;
        boolean prepared, failed, requested, buffering, ended, video, audio;
        volatile boolean closed;
        boolean placed, placedVisible;
        int width, height, level = 100;
        int placedX, placedY, placedW, placedH, placedWidth, placedHeight;
        boolean mute;

        Player(View host, long session, String source, boolean url)
        {
            this.host = host;
            this.session = session;
            this.source = source;
            sourceURL = url;
            view = new TextureView(host.getContext());
            view.setOpaque(false);
            view.setSurfaceTextureListener(this);
            caption = new TextView(host.getContext());
            caption.setGravity(Gravity.CENTER_HORIZONTAL | Gravity.BOTTOM);
            caption.setTypeface(Typeface.DEFAULT, Typeface.BOLD);
            int padding = Math.round(8 * host.getResources().getDisplayMetrics().density);
            caption.setPadding(padding, padding, padding, padding);
            caption.setShadowLayer(2, 0, 1, Color.BLACK);
            caption.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        }

        abstract void open();
        abstract void play(boolean play);
        abstract void seek(double position, int fast);
        abstract void volume(int volume, boolean mute);
        abstract void audioDevice(int id);
        abstract boolean playing();
        abstract double position();
        abstract double duration();
        abstract int subtitleCount();
        abstract String subtitleName(int index, boolean language);
        abstract boolean subtitleSelect(int index, double delay);
        abstract void surfaceAvailable();
        abstract void surfaceDestroyed();
        abstract void release();

        void fail(String message)
        {
            failed = true;
            host.setKeepScreenOn(false);
            if (session != 0)
                error(session, message);
        }

        void layout()
        {
            if (closed || !(host.getParent() instanceof ViewGroup group))
                return;
            if (parent != group)
            {
                if (parent != null)
                {
                    parent.getOverlay().remove(view);
                    parent.getOverlay().remove(caption);
                }
                parent = group;
                parent.getOverlay().add(view);
                parent.getOverlay().add(caption);
                placed = false;
            }
            int x = Math.round(host.getX()), y = Math.round(host.getY());
            int w = host.getWidth(), h = host.getHeight();
            boolean visible = host.isShown() && host.getLocalVisibleRect(clip);
            if (placed && x == placedX && y == placedY && w == placedW && h == placedH && width == placedWidth &&
                height == placedHeight && visible == placedVisible && clip.equals(placedClip))
                return;
            view.layout(x, y, x + w, y + h);
            view.setClipBounds(clip);
            view.setVisibility(visible ? View.VISIBLE : View.INVISIBLE);
            int vx = 0, vy = 0, vw = w, vh = h;
            if (w > 0 && h > 0 && width > 0 && height > 0)
            {
                float aspect = (float)width / height;
                float current = (float)w / h;
                matrix.setScale(aspect < current ? aspect / current : 1, aspect > current ? current / aspect : 1, w / 2.0f, h / 2.0f);
                view.setTransform(matrix);
                if (aspect > current)
                {
                    vh = Math.round(w / aspect);
                    vy = (h - vh) / 2;
                }
                else
                {
                    vw = Math.round(h * aspect);
                    vx = (w - vw) / 2;
                }
            }
            caption.measure(View.MeasureSpec.makeMeasureSpec(Math.max(0, vw), View.MeasureSpec.EXACTLY),
                            View.MeasureSpec.makeMeasureSpec(Math.max(0, vh), View.MeasureSpec.EXACTLY));
            caption.layout(x + vx, y + vy, x + vx + vw, y + vy + vh);
            Rect captionClip = new Rect(clip);
            captionClip.offset(-vx, -vy);
            caption.setClipBounds(captionClip);
            caption.setVisibility(visible ? View.VISIBLE : View.INVISIBLE);
            placed = true;
            placedX = x;
            placedY = y;
            placedW = w;
            placedH = h;
            placedWidth = width;
            placedHeight = height;
            placedVisible = visible;
            placedClip.set(clip);
        }

        double[] poll()
        {
            int state;
            layout();
            if (failed)
                state = ERROR;
            else if (!prepared)
                state = LOADING;
            else if (buffering || (video && surface == null))
                state = BUFFERING;
            else
                state = playing() ? PLAYING : PAUSED;
            snapshot[0] = state;
            snapshot[1] = prepared ? 1 : 0;
            snapshot[2] = ended ? 1 : 0;
            snapshot[3] = prepared && !failed ? position() : 0;
            snapshot[4] = prepared && !failed ? duration() : 0;
            snapshot[5] = snapshot[4] > 0 ? 1 : 0;
            snapshot[6] = video ? 1 : 0;
            snapshot[7] = audio ? 1 : 0;
            snapshot[8] = width;
            snapshot[9] = height;
            ended = false;
            return snapshot;
        }

        void close()
        {
            session = 0;
            closed = true;
            host.setKeepScreenOn(false);
            view.setSurfaceTextureListener(null);
            release();
            if (parent != null)
            {
                parent.getOverlay().remove(view);
                parent.getOverlay().remove(caption);
                parent = null;
            }
        }

        @Override
        public void onSurfaceTextureAvailable(SurfaceTexture texture, int width, int height)
        {
            if (closed || failed)
                return;
            surface = new Surface(texture);
            surfaceAvailable();
        }

        @Override
        public boolean onSurfaceTextureDestroyed(SurfaceTexture texture)
        {
            if (!closed)
                surfaceDestroyed();
            if (surface != null)
            {
                surface.release();
                surface = null;
            }
            return true;
        }

        @Override
        public void onSurfaceTextureSizeChanged(SurfaceTexture texture, int width, int height)
        {
            layout();
        }

        @Override
        public void onSurfaceTextureUpdated(SurfaceTexture texture)
        {
        }
    }

    static boolean isTimedText(String mime)
    {
        return "application/x-3gpp-tt".equals(mime) || "text/3gpp".equals(mime) || "text/3gpp-tt".equals(mime);
    }

    static boolean subtitleDefault(MediaFormat format)
    {
        return subtitleFlag(format, MediaFormat.KEY_IS_DEFAULT) || subtitleFlag(format, MediaFormat.KEY_IS_FORCED_SUBTITLE);
    }

    static byte[] timedText(ByteBuffer buffer, int bytes)
    {
        if (bytes < 2)
            return null;
        int length = buffer.getShort(0) & 0xffff;
        if (length > bytes - 2)
            return null;
        byte[] encoded = new byte[length];
        buffer.position(2);
        buffer.get(encoded);
        if (length >= 2 && ((encoded[0] & 255) == 254 || (encoded[0] & 255) == 255))
            return new String(encoded, StandardCharsets.UTF_16).getBytes(StandardCharsets.UTF_8);
        return encoded;
    }

    private static boolean subtitleFlag(MediaFormat format, String key)
    {
        try
        {
            return format != null && format.containsKey(key) && format.getInteger(key) != 0;
        }
        catch (ClassCastException | NullPointerException error)
        {
            return false;
        }
    }

    private static final class NativePlayer extends Player
    {
        final MediaPlayer media;
        boolean seeking;
        volatile int subtitleGeneration;
        Thread subtitleThread;
        int previewTrack;
        long previewTime;
        boolean previewPending;
        double pendingSeek = -1;
        Thread keyframeThread;
        MediaExtractor keyframeExtractor;
        volatile int keyframeGeneration;
        double keyframeTarget;
        int keyframeDirection;
        boolean keyframePending, keyframeTrack;

        NativePlayer(View host, long session, String source, boolean url)
        {
            super(host, session, source, url);
            media = new MediaPlayer();
            media.setOnTimedTextListener((player, text) -> {
                if (!closed && this.session != 0)
                    subtitleCue(this.session, text == null || text.getText() == null ? null : text.getText().getBytes(StandardCharsets.UTF_8));
            });
            media.setOnPreparedListener(player -> {
                if (closed)
                    return;
                prepared = true;
                width = player.getVideoWidth();
                height = player.getVideoHeight();
                MediaPlayer.TrackInfo[] tracks = player.getTrackInfo();
                if (tracks != null)
                {
                    for (MediaPlayer.TrackInfo track : tracks)
                    {
                        if (track.getTrackType() == MediaPlayer.TrackInfo.MEDIA_TRACK_TYPE_VIDEO)
                            video = true;
                        if (track.getTrackType() == MediaPlayer.TrackInfo.MEDIA_TRACK_TYPE_AUDIO)
                            audio = true;
                    }
                }
                video |= width > 0 && height > 0;
                volume(level, mute);
                showFrame();
                play(requested);
                layout();
            });
            media.setOnVideoSizeChangedListener((player, w, h) -> {
                width = w;
                height = h;
                video = w > 0 && h > 0;
                layout();
            });
            media.setOnCompletionListener(player -> {
                if (closed)
                    return;
                ended = true;
                host.setKeepScreenOn(false);
            });
            media.setOnErrorListener((player, what, extra) -> {
                fail("Android MediaPlayer error " + what + ":" + extra);
                return true;
            });
            media.setOnInfoListener((player, what, extra) -> {
                if (what == MediaPlayer.MEDIA_INFO_BUFFERING_START)
                    buffering = true;
                if (what == MediaPlayer.MEDIA_INFO_BUFFERING_END)
                    buffering = false;
                return false;
            });
            media.setOnSeekCompleteListener(player -> {
                seeking = false;
                if (!closed && pendingSeek >= 0)
                {
                    double position = pendingSeek;
                    pendingSeek = -1;
                    seekTo(position);
                }
                else if (!closed && !requested)
                {
                    int selected = media.getSelectedTrack(MediaPlayer.TrackInfo.MEDIA_TRACK_TYPE_TIMEDTEXT);
                    if (selected >= 0)
                        previewSubtitle(selected);
                }
            });
        }

        @Override
        void open()
        {
            try
            {
                if (sourceURL)
                    media.setDataSource(host.getContext(), Uri.parse(source));
                else
                    media.setDataSource(source);
                media.prepareAsync();
            }
            catch (Exception error)
            {
                fail(error.toString());
            }
        }

        @Override
        int subtitleCount()
        {
            if (!prepared || failed || closed)
                return 0;
            int count = 0;
            for (MediaPlayer.TrackInfo track : media.getTrackInfo())
            {
                if (track.getTrackType() == MediaPlayer.TrackInfo.MEDIA_TRACK_TYPE_TIMEDTEXT)
                    count++;
            }
            return count;
        }

        @Override
        String subtitleName(int index, boolean language)
        {
            if (!prepared || failed || closed)
                return null;
            int count = 0;
            for (MediaPlayer.TrackInfo track : media.getTrackInfo())
            {
                if (track.getTrackType() == MediaPlayer.TrackInfo.MEDIA_TRACK_TYPE_TIMEDTEXT && ++count == index)
                    return language ? track.getLanguage() : "Track " + index;
            }
            return null;
        }

        @Override
        boolean subtitleSelect(int index, double delay)
        {
            if (!prepared || failed || closed)
                return index <= 0;
            try
            {
                int previous = media.getSelectedTrack(MediaPlayer.TrackInfo.MEDIA_TRACK_TYPE_TIMEDTEXT);
                int selected = -1;
                int count = 0;
                MediaPlayer.TrackInfo[] tracks = media.getTrackInfo();
                for (int i = 0; i < tracks.length && selected < 0; i++)
                {
                    if (tracks[i].getTrackType() != MediaPlayer.TrackInfo.MEDIA_TRACK_TYPE_TIMEDTEXT)
                        continue;
                    count++;
                    if (index == -1)
                    {
                        if (subtitleDefault(tracks[i].getFormat()))
                            selected = i;
                    }
                    else if (count == index)
                        selected = i;
                }
                if (index > 0 && selected < 0)
                    return false;
                if (previous >= 0 && previous != selected)
                    media.deselectTrack(previous);
                if (selected != previous)
                {
                    cancelSubtitlePreview();
                    if (selected >= 0)
                        media.selectTrack(selected);
                    if (session != 0)
                        subtitleCue(session, null);
                    if (selected >= 0 && !media.isPlaying() && media.getDuration() > 0)
                        previewSubtitle(selected);
                }
                return true;
            }
            catch (IllegalStateException | IllegalArgumentException error)
            {
                return false;
            }
        }

        void previewSubtitle(int selected)
        {
            int ordinal = 0;
            MediaPlayer.TrackInfo[] tracks = media.getTrackInfo();
            for (int i = 0; i <= selected; i++)
            {
                if (tracks[i].getTrackType() == MediaPlayer.TrackInfo.MEDIA_TRACK_TYPE_TIMEDTEXT)
                    ordinal++;
            }
            synchronized (this)
            {
                subtitleGeneration++;
                previewTrack = ordinal;
                previewTime = (long)media.getCurrentPosition() * 1000;
                previewPending = true;
                if (subtitleThread != null)
                    return;
                subtitleThread = new Thread(this::readSubtitlePreview, "IupSubtitle");
                subtitleThread.start();
            }
        }

        synchronized void cancelSubtitlePreview()
        {
            subtitleGeneration++;
            previewPending = false;
        }

        void readSubtitlePreview()
        {
            ByteBuffer buffer = ByteBuffer.allocate(1024 * 1024);
            for (;;)
            {
                final int generation, ordinal;
                final long time;
                synchronized (this)
                {
                    if (closed || !previewPending)
                    {
                        subtitleThread = null;
                        return;
                    }
                    generation = subtitleGeneration;
                    ordinal = previewTrack;
                    time = previewTime;
                    previewPending = false;
                }
                byte[] text = null;
                MediaExtractor extractor = new MediaExtractor();
                try
                {
                    if (sourceURL)
                        extractor.setDataSource(host.getContext(), Uri.parse(source), null);
                    else
                        extractor.setDataSource(source);
                    int found = 0;
                    for (int i = 0; i < extractor.getTrackCount(); i++)
                    {
                        MediaFormat format = extractor.getTrackFormat(i);
                        if (!isTimedText(format.getString(MediaFormat.KEY_MIME)))
                            continue;
                        if (++found != ordinal)
                            continue;
                        extractor.selectTrack(i);
                        extractor.seekTo(time, MediaExtractor.SEEK_TO_PREVIOUS_SYNC);
                        long start = extractor.getSampleTime();
                        if (start >= 0 && start <= time)
                        {
                            buffer.clear();
                            text = timedText(buffer, extractor.readSampleData(buffer, 0));
                        }
                        break;
                    }
                }
                catch (Exception error)
                {
                    text = null;
                }
                finally
                {
                    extractor.release();
                }
                final byte[] result = text;
                host.post(() -> {
                    if (!closed && session != 0 && subtitleGeneration == generation)
                        subtitleCue(session, result);
                });
            }
        }

        void showFrame()
        {
            if (closed || failed || !prepared || requested || seeking || surface == null)
                return;
            try
            {
                media.seekTo(media.getCurrentPosition());
            }
            catch (IllegalStateException error)
            {
                fail(error.toString());
            }
        }

        @Override
        void play(boolean play)
        {
            if (play && !requested)
                cancelSubtitlePreview();
            requested = play;
            host.setKeepScreenOn(play && !failed);
            if (closed || failed || !prepared || (video && surface == null))
                return;
            try
            {
                if (play)
                    media.start();
                else if (media.isPlaying())
                    media.pause();
            }
            catch (IllegalStateException error)
            {
                fail(error.toString());
            }
        }

        @Override
        void seek(double position, int fast)
        {
            if (closed || failed || !prepared)
                return;
            if (fast == 0)
            {
                synchronized (this)
                {
                    keyframeGeneration++;
                    keyframePending = false;
                }
                seekTo(position);
                return;
            }
            synchronized (this)
            {
                keyframeGeneration++;
                keyframeTarget = position;
                keyframeDirection = fast;
                keyframePending = true;
                if (keyframeThread != null)
                    return;
                keyframeThread = new Thread(this::findKeyframes, "IupKeyframe");
                keyframeThread.start();
            }
        }

        void findKeyframes()
        {
            for (;;)
            {
                final int generation, direction;
                final double target;
                synchronized (this)
                {
                    if (closed || !keyframePending)
                    {
                        keyframeThread = null;
                        if (closed && keyframeExtractor != null)
                        {
                            keyframeExtractor.release();
                            keyframeExtractor = null;
                        }
                        return;
                    }
                    generation = keyframeGeneration;
                    target = keyframeTarget;
                    direction = keyframeDirection;
                    keyframePending = false;
                }
                double keyframe = target;
                try
                {
                    if (keyframeExtractor == null)
                    {
                        keyframeExtractor = new MediaExtractor();
                        if (sourceURL)
                            keyframeExtractor.setDataSource(host.getContext(), Uri.parse(source), null);
                        else
                            keyframeExtractor.setDataSource(source);
                        for (int i = 0; i < keyframeExtractor.getTrackCount(); i++)
                        {
                            String mime = keyframeExtractor.getTrackFormat(i).getString(MediaFormat.KEY_MIME);
                            if (mime != null && mime.startsWith("video/"))
                            {
                                keyframeExtractor.selectTrack(i);
                                keyframeTrack = true;
                                break;
                            }
                        }
                    }
                    if (keyframeTrack)
                    {
                        keyframeExtractor.seekTo((long)(target * 1000000),
                            direction > 0 ? MediaExtractor.SEEK_TO_NEXT_SYNC : MediaExtractor.SEEK_TO_PREVIOUS_SYNC);
                        long sample = keyframeExtractor.getSampleTime();
                        if (sample >= 0)
                            keyframe = sample / 1000000.0;
                    }
                }
                catch (Exception error)
                {
                    keyframe = target;
                }
                final double position = keyframe;
                host.post(() -> {
                    if (!closed && keyframeGeneration == generation)
                        seekTo(position);
                });
            }
        }

        void seekTo(double position)
        {
            if (closed || failed || !prepared)
                return;
            if (seeking)
            {
                pendingSeek = position;
                return;
            }
            if (position > Integer.MAX_VALUE / 1000.0)
                return;
            try
            {
                seeking = true;
                cancelSubtitlePreview();
                ended = false;
                if (Build.VERSION.SDK_INT >= 26)
                    media.seekTo((long)(position * 1000), MediaPlayer.SEEK_CLOSEST);
                else
                    media.seekTo((int)(position * 1000));
            }
            catch (IllegalStateException error)
            {
                seeking = false;
                fail(error.toString());
            }
        }

        @Override
        void audioDevice(int id)
        {
            if (closed || Build.VERSION.SDK_INT < 28)
                return;
            media.setPreferredDevice(findAudioDevice(host.getContext(), id));
        }

        @Override
        void volume(int volume, boolean mute)
        {
            level = volume;
            this.mute = mute;
            if (!closed && !failed && prepared)
            {
                float value = mute ? 0 : volume / 100.0f;
                media.setVolume(value, value);
            }
        }

        @Override
        boolean playing()
        {
            return media.isPlaying();
        }

        @Override
        double position()
        {
            return media.getCurrentPosition() / 1000.0;
        }

        @Override
        double duration()
        {
            return Math.max(0, media.getDuration()) / 1000.0;
        }

        @Override
        void release()
        {
            cancelSubtitlePreview();
            synchronized (this)
            {
                keyframePending = false;
                if (keyframeThread == null && keyframeExtractor != null)
                {
                    keyframeExtractor.release();
                    keyframeExtractor = null;
                }
            }
            media.setOnPreparedListener(null);
            media.setOnVideoSizeChangedListener(null);
            media.setOnCompletionListener(null);
            media.setOnErrorListener(null);
            media.setOnInfoListener(null);
            media.setOnSeekCompleteListener(null);
            media.setOnTimedTextListener(null);
            media.release();
            if (surface != null)
            {
                surface.release();
                surface = null;
            }
        }

        @Override
        void surfaceAvailable()
        {
            media.setSurface(surface);
            showFrame();
            play(requested);
        }

        @Override
        void surfaceDestroyed()
        {
            if (prepared && !failed && media.isPlaying())
                media.pause();
            media.setSurface(null);
        }
    }
}
