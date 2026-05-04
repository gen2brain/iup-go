package io.github.gen2brain.iupgo;

import android.content.Context;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.appcompat.view.ContextThemeWrapper;


public final class IupGLCanvasHelper
{
    private IupGLCanvasHelper() {}


    @Keep
    public static IupGLSurfaceView create(final long ihandlePtr)
    {
        ContextThemeWrapper ctx = IupCommon.getContextThemeWrapper();
        return new IupGLSurfaceView(ctx, ihandlePtr);
    }


    /* Programmatic-only; requires an Ihandle so the XML constructors are absent. */
    @android.annotation.SuppressLint("ViewConstructor")
    public static class IupGLSurfaceView extends SurfaceView implements SurfaceHolder.Callback
    {
        private final long ihandlePtr;
        private final GestureDetector rightClickDetector;
        private boolean rightClickFired;

        public IupGLSurfaceView(Context ctx, long ih)
        {
            super(ctx);
            this.ihandlePtr = ih;
            setFocusable(true);
            setFocusableInTouchMode(true);

            getHolder().addCallback(this);

            this.rightClickDetector = new GestureDetector(ctx,
                new GestureDetector.SimpleOnGestureListener()
                {
                    @Override public void onLongPress(@NonNull MotionEvent e)
                    {
                        if (ihandlePtr == 0) return;
                        int x = (int) e.getX();
                        int y = (int) e.getY();
                        int mods = e.getMetaState();
                        IupCanvasHelper.dispatchButton(ihandlePtr, 3, 1, x, y, mods);
                        IupCanvasHelper.dispatchButton(ihandlePtr, 3, 0, x, y, mods);
                        rightClickFired = true;
                    }
                });
        }

        public long getIhandlePtr() { return ihandlePtr; }

        @Override
        public void surfaceCreated(SurfaceHolder holder)
        {
            Surface s = holder.getSurface();
            if (s != null && ihandlePtr != 0)
                dispatchSurfaceCreated(ihandlePtr, s);
        }

        @Override
        public void surfaceChanged(@NonNull SurfaceHolder holder, int format, int w, int h)
        {
            if (ihandlePtr != 0)
                dispatchSurfaceChanged(ihandlePtr, w, h);
        }

        @Override
        public void surfaceDestroyed(@NonNull SurfaceHolder holder)
        {
            if (ihandlePtr != 0)
                dispatchSurfaceDestroyed(ihandlePtr);
        }

        @Override
        public boolean onTouchEvent(MotionEvent ev)
        {
            if (ihandlePtr == 0) return false;

            if (ev.getActionMasked() == MotionEvent.ACTION_DOWN && getParent() != null)
                getParent().requestDisallowInterceptTouchEvent(true);

            rightClickDetector.onTouchEvent(ev);

            int x = (int)ev.getX();
            int y = (int)ev.getY();
            int mods = ev.getMetaState();

            switch (ev.getActionMasked())
            {
                case MotionEvent.ACTION_DOWN:
                    rightClickFired = false;
                    IupCanvasHelper.dispatchButton(ihandlePtr, 1, 1, x, y, mods);
                    return true;
                case MotionEvent.ACTION_MOVE:
                    IupCanvasHelper.dispatchMotion(ihandlePtr, x, y, mods, 1);
                    return true;
                case MotionEvent.ACTION_UP:
                case MotionEvent.ACTION_CANCEL:
                    if (!rightClickFired)
                        IupCanvasHelper.dispatchButton(ihandlePtr, 1, 0, x, y, mods);
                    if (ev.getActionMasked() == MotionEvent.ACTION_UP)
                        performClick();
                    return true;
            }
            return super.onTouchEvent(ev);
        }

        @Override
        public boolean performClick()
        {
            /* Accessibility hook; IUP callbacks dispatch via onTouchEvent. */
            return super.performClick();
        }
    }


    public static native void dispatchSurfaceCreated(long ihandlePtr, Surface surface);
    public static native void dispatchSurfaceChanged(long ihandlePtr, int width, int height);
    public static native void dispatchSurfaceDestroyed(long ihandlePtr);
}
