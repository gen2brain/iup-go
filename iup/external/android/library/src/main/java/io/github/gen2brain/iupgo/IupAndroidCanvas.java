package io.github.gen2brain.iupgo;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.util.AttributeSet;
import android.view.GestureDetector;
import android.view.MotionEvent;

import androidx.annotation.NonNull;


/* Canvas back-buffer blit in onDraw; ViewGroup base so TYPECANVAS containers (FlatTabs/FlatFrame) can parent children. */
public class IupAndroidCanvas extends IupAndroidFixed
{
    private final long ihandlePtr;
    private Bitmap back;
    private Canvas backCanvas;
    private GestureDetector rightClickDetector;
    /* Tracks an outstanding clip save() for setClipRect's restore-then-replace contract. */
    boolean clipSaved;

    public IupAndroidCanvas(Context ctx, long ihandlePtr)
    {
        super(ctx);
        this.ihandlePtr = ihandlePtr;
        setFocusable(true);
        setFocusableInTouchMode(true);
        setWillNotDraw(false);
        initRightClickDetector(ctx);
    }

    public IupAndroidCanvas(Context ctx, AttributeSet attrs)
    {
        super(ctx, attrs);
        this.ihandlePtr = 0;
        setWillNotDraw(false);
        initRightClickDetector(ctx);
    }

    private void initRightClickDetector(Context ctx)
    {
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
                }
            });
    }

    public long getIhandlePtr() { return ihandlePtr; }
    public Canvas getBackCanvas() { return backCanvas; }
    public Bitmap getBackBuffer() { return back; }

    /** allocates/realloc back buffer to current view size; called before each draw batch */
    public void ensureBackBuffer()
    {
        int w = getWidth();
        int h = getHeight();
        if (w <= 0 || h <= 0) return;
        if (back != null && back.getWidth() == w && back.getHeight() == h) return;
        back = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
        /* DENSITY_NONE: blit to view 1:1 (view's canvas would auto-scale otherwise). */
        back.setDensity(Bitmap.DENSITY_NONE);
        backCanvas = new Canvas(back);
        /* Scale so IUP draw coordinates are logical px (dp-equivalent). */
        float density = IupCommon.getDisplayDensity();
        if (density != 1.0f) backCanvas.scale(density, density);
    }

    @Override
    protected void onDraw(@NonNull Canvas canvas)
    {
        /* Let IUP repaint the back buffer via ACTION, then blit it. */
        if (ihandlePtr != 0) IupCanvasHelper.dispatchAction(ihandlePtr);
        if (back != null) canvas.drawBitmap(back, 0, 0, null);
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh)
    {
        if (back != null && (back.getWidth() != w || back.getHeight() != h))
        {
            back = null;
            backCanvas = null;
        }
        if (ihandlePtr != 0 && w > 0 && h > 0)
        {
            IupCommon.doResize(ihandlePtr, 0, 0, w, h);
            IupCanvasHelper.dispatchAction(ihandlePtr);
            invalidate();
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent ev)
    {
        if (ihandlePtr == 0 || !isEnabled()) return false;

        rightClickDetector.onTouchEvent(ev);

        int x = (int)ev.getX();
        int y = (int)ev.getY();
        int mods = ev.getMetaState();
        int action = ev.getActionMasked();

        switch (action)
        {
            case MotionEvent.ACTION_DOWN:
                /* Drag-interactive canvas keeps the gesture; parent must not intercept. */
                if (getParent() != null && IupCanvasHelper.isDragInteractive(ihandlePtr))
                    getParent().requestDisallowInterceptTouchEvent(true);
                IupCanvasHelper.dispatchButton(ihandlePtr, 1, 1, x, y, mods);
                return true;
            case MotionEvent.ACTION_MOVE:
                IupCanvasHelper.dispatchMotion(ihandlePtr, x, y, mods, 1);
                return true;
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_CANCEL:
                /* Touch has no hover; release + fire LEAVEWINDOW_CB to clear it. */
                IupCanvasHelper.dispatchButton(ihandlePtr, 1, 0, x, y, mods);
                IupCanvasHelper.dispatchLeaveWindow(ihandlePtr);
                if (action == MotionEvent.ACTION_UP)
                    performClick();
                return true;
        }
        return super.onTouchEvent(ev);
    }

    @Override
    public boolean performClick()
    {
        /* Accessibility hook; IUP callbacks are dispatched via touch events. */
        return super.performClick();
    }
}
