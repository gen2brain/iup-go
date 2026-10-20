package io.github.gen2brain.iupgo;

import android.graphics.Matrix;
import android.graphics.Rect;
import android.graphics.SurfaceTexture;
import android.media.MediaPlayer;
import android.net.Uri;
import android.os.Build;
import android.view.Surface;
import android.view.TextureView;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.Keep;

public final class IupVideoHelper
{
    private static final int LOADING = 1;
    private static final int PLAYING = 3;
    private static final int PAUSED = 4;
    private static final int BUFFERING = 5;
    private static final int ERROR = 7;

    private IupVideoHelper() {}

    @Keep
    public static Object create(View host, long session, String source, boolean url)
    {
        Player player = new Player(host, session);
        try
        {
            if (url)
                player.media.setDataSource(host.getContext(), Uri.parse(source));
            else
                player.media.setDataSource(source);
            player.media.prepareAsync();
        }
        catch (Exception error)
        {
            player.fail(error.toString());
        }
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
    public static void seek(Object object, double position)
    {
        ((Player)object).seek(position);
    }

    @Keep
    public static void volume(Object object, int volume, boolean mute)
    {
        ((Player)object).volume(volume, mute);
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

    private static final class Player implements TextureView.SurfaceTextureListener
    {
        final View host;
        final TextureView view;
        final MediaPlayer media;
        final double[] snapshot = new double[10];
        final Rect clip = new Rect();
        final Rect placedClip = new Rect();
        final Matrix matrix = new Matrix();
        ViewGroup parent;
        Surface surface;
        long session;
        boolean prepared, closed, failed, requested, buffering, ended, video, audio, seeking;
        boolean placed, placedVisible;
        int width, height, level = 100;
        int placedX, placedY, placedW, placedH, placedWidth, placedHeight;
        boolean mute;
        double pendingSeek = -1;

        Player(View host, long session)
        {
            this.host = host;
            this.session = session;
            view = new TextureView(host.getContext());
            view.setOpaque(false);
            view.setSurfaceTextureListener(this);
            media = new MediaPlayer();
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
                    seek(position);
                }
            });
        }

        void fail(String message)
        {
            failed = true;
            host.setKeepScreenOn(false);
            if (session != 0)
                error(session, message);
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

        void play(boolean play)
        {
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

        void seek(double position)
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

        void layout()
        {
            if (closed || !(host.getParent() instanceof ViewGroup group))
                return;
            if (parent != group)
            {
                if (parent != null)
                    parent.getOverlay().remove(view);
                parent = group;
                parent.getOverlay().add(view);
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
            if (w > 0 && h > 0 && width > 0 && height > 0)
            {
                float aspect = (float)width / height;
                float current = (float)w / h;
                matrix.setScale(aspect < current ? aspect / current : 1, aspect > current ? current / aspect : 1, w / 2.0f, h / 2.0f);
                view.setTransform(matrix);
            }
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
                state = media.isPlaying() ? PLAYING : PAUSED;
            snapshot[0] = state;
            snapshot[1] = prepared ? 1 : 0;
            snapshot[2] = ended ? 1 : 0;
            snapshot[3] = prepared && !failed ? media.getCurrentPosition() / 1000.0 : 0;
            snapshot[4] = prepared && !failed ? Math.max(0, media.getDuration()) / 1000.0 : 0;
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
            media.setOnPreparedListener(null);
            media.setOnVideoSizeChangedListener(null);
            media.setOnCompletionListener(null);
            media.setOnErrorListener(null);
            media.setOnInfoListener(null);
            media.setOnSeekCompleteListener(null);
            media.release();
            if (surface != null)
            {
                surface.release();
                surface = null;
            }
            if (parent != null)
            {
                parent.getOverlay().remove(view);
                parent = null;
            }
        }

        @Override
        public void onSurfaceTextureAvailable(SurfaceTexture texture, int width, int height)
        {
            if (closed || failed)
                return;
            surface = new Surface(texture);
            media.setSurface(surface);
            showFrame();
            play(requested);
        }

        @Override
        public boolean onSurfaceTextureDestroyed(SurfaceTexture texture)
        {
            if (!closed)
            {
                if (prepared && !failed && media.isPlaying())
                    media.pause();
                media.setSurface(null);
            }
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
}
