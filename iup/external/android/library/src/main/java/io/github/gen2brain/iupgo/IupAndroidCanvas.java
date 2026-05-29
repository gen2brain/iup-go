package io.github.gen2brain.iupgo;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;

import androidx.annotation.NonNull;


/* Canvas back-buffer blit in onDraw; ViewGroup base so TYPECANVAS containers (FlatTabs/FlatFrame) can parent children. */
public class IupAndroidCanvas extends IupAndroidFixed
{
    /* mirror the IUP_GESTURE_* enums in iup.h */
    private static final int GESTURE_PINCH = 0, GESTURE_ROTATE = 1, GESTURE_PAN = 2, GESTURE_SWIPE = 3, GESTURE_TAP = 4, GESTURE_LONGPRESS = 5;
    private static final int GESTURE_BEGIN = 0, GESTURE_CHANGED = 1, GESTURE_END = 2;
    private static final int SWIPE_RIGHT = 0, SWIPE_LEFT = 1, SWIPE_UP = 2, SWIPE_DOWN = 3;

    private final long ihandlePtr;
    private Bitmap back;
    private Canvas backCanvas;
    private GestureDetector gestureDetector;
    private ScaleGestureDetector scaleDetector;
    private float pinchScale = 1f;
    private boolean panActive;
    private float panDx, panDy;
    private boolean rotateActive;
    private float rotateLast, rotateAngle;
    /* Tracks an outstanding clip save() for setClipRect's restore-then-replace contract. */
    boolean clipSaved;

    public IupAndroidCanvas(Context ctx, long ihandlePtr)
    {
        super(ctx);
        this.ihandlePtr = ihandlePtr;
        setFocusable(true);
        setFocusableInTouchMode(true);
        setWillNotDraw(false);
        initGestureDetectors(ctx);
    }

    public IupAndroidCanvas(Context ctx, AttributeSet attrs)
    {
        super(ctx, attrs);
        this.ihandlePtr = 0;
        setWillNotDraw(false);
        initGestureDetectors(ctx);
    }

    private void initGestureDetectors(Context ctx)
    {
        this.gestureDetector = new GestureDetector(ctx,
            new GestureDetector.SimpleOnGestureListener()
            {
                @Override public void onLongPress(@NonNull MotionEvent e)
                {
                    if (ihandlePtr == 0) return;
                    int x = (int) e.getX();
                    int y = (int) e.getY();
                    IupCanvasHelper.dispatchGesture(ihandlePtr, GESTURE_LONGPRESS, GESTURE_END, x, y, 0, 0);
                    IupCanvasHelper.dispatchButton(ihandlePtr, 3, 1, x, y, e.getMetaState());
                    IupCanvasHelper.dispatchButton(ihandlePtr, 3, 0, x, y, e.getMetaState());
                }

                /* two-finger drag is a pan; one finger stays BUTTON_CB+MOTION_CB */
                @Override public boolean onScroll(MotionEvent e1, @NonNull MotionEvent e2, float dX, float dY)
                {
                    if (ihandlePtr == 0 || e2.getPointerCount() < 2) return false;
                    int x = (int) e2.getX(), y = (int) e2.getY();
                    if (!panActive)
                    {
                        panActive = true; panDx = 0; panDy = 0;
                        IupCanvasHelper.dispatchGesture(ihandlePtr, GESTURE_PAN, GESTURE_BEGIN, x, y, 0, 0);
                    }
                    panDx -= dX; panDy -= dY;
                    IupCanvasHelper.dispatchGesture(ihandlePtr, GESTURE_PAN, GESTURE_CHANGED, x, y, panDx, panDy);
                    return true;
                }

                @Override public boolean onFling(MotionEvent e1, @NonNull MotionEvent e2, float vX, float vY)
                {
                    if (ihandlePtr == 0) return false;
                    int dir;
                    if (Math.abs(vX) > Math.abs(vY)) dir = vX > 0 ? SWIPE_RIGHT : SWIPE_LEFT;
                    else                             dir = vY > 0 ? SWIPE_DOWN : SWIPE_UP;
                    IupCanvasHelper.dispatchGesture(ihandlePtr, GESTURE_SWIPE, GESTURE_END, (int) e2.getX(), (int) e2.getY(), dir, 0);
                    return true;
                }

                @Override public boolean onSingleTapConfirmed(@NonNull MotionEvent e)
                {
                    if (ihandlePtr == 0) return false;
                    IupCanvasHelper.dispatchGesture(ihandlePtr, GESTURE_TAP, GESTURE_END, (int) e.getX(), (int) e.getY(), 1, 0);
                    return true;
                }

                @Override public boolean onDoubleTap(@NonNull MotionEvent e)
                {
                    if (ihandlePtr == 0) return false;
                    IupCanvasHelper.dispatchGesture(ihandlePtr, GESTURE_TAP, GESTURE_END, (int) e.getX(), (int) e.getY(), 2, 0);
                    return true;
                }
            });

        this.scaleDetector = new ScaleGestureDetector(ctx,
            new ScaleGestureDetector.SimpleOnScaleGestureListener()
            {
                @Override public boolean onScaleBegin(@NonNull ScaleGestureDetector d)
                {
                    if (ihandlePtr == 0) return false;
                    pinchScale = 1f;
                    IupCanvasHelper.dispatchGesture(ihandlePtr, GESTURE_PINCH, GESTURE_BEGIN, (int) d.getFocusX(), (int) d.getFocusY(), 1.0, 0);
                    return true;
                }

                @Override public boolean onScale(@NonNull ScaleGestureDetector d)
                {
                    if (ihandlePtr == 0) return false;
                    pinchScale *= d.getScaleFactor();
                    IupCanvasHelper.dispatchGesture(ihandlePtr, GESTURE_PINCH, GESTURE_CHANGED, (int) d.getFocusX(), (int) d.getFocusY(), pinchScale, 0);
                    return true;
                }

                @Override public void onScaleEnd(@NonNull ScaleGestureDetector d)
                {
                    if (ihandlePtr == 0) return;
                    IupCanvasHelper.dispatchGesture(ihandlePtr, GESTURE_PINCH, GESTURE_END, (int) d.getFocusX(), (int) d.getFocusY(), pinchScale, 0);
                }
            });
    }

    /* angle of the two-finger axis in degrees; screen y-down makes atan2 grow clockwise */
    private static float twoFingerAngle(MotionEvent ev)
    {
        double dx = ev.getX(1) - ev.getX(0);
        double dy = ev.getY(1) - ev.getY(0);
        return (float) Math.toDegrees(Math.atan2(dy, dx));
    }

    /* Android has no rotation detector; accumulate the two-finger axis delta ourselves */
    private void handleRotation(MotionEvent ev)
    {
        int action = ev.getActionMasked();
        boolean twoDown = ev.getPointerCount() >= 2 &&
            (action == MotionEvent.ACTION_MOVE || action == MotionEvent.ACTION_POINTER_DOWN);

        if (twoDown)
        {
            float a = twoFingerAngle(ev);
            int fx = (int) ((ev.getX(0) + ev.getX(1)) / 2);
            int fy = (int) ((ev.getY(0) + ev.getY(1)) / 2);
            if (!rotateActive)
            {
                rotateActive = true; rotateLast = a; rotateAngle = 0;
                IupCanvasHelper.dispatchGesture(ihandlePtr, GESTURE_ROTATE, GESTURE_BEGIN, fx, fy, 0, 0);
            }
            else
            {
                float delta = a - rotateLast; rotateLast = a;
                while (delta > 180) delta -= 360;
                while (delta < -180) delta += 360;
                rotateAngle += delta;
                IupCanvasHelper.dispatchGesture(ihandlePtr, GESTURE_ROTATE, GESTURE_CHANGED, fx, fy, rotateAngle, 0);
            }
        }
        else if (rotateActive)
        {
            rotateActive = false;
            IupCanvasHelper.dispatchGesture(ihandlePtr, GESTURE_ROTATE, GESTURE_END, (int) ev.getX(), (int) ev.getY(), rotateAngle, 0);
        }
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
        if (ihandlePtr != 0)
        {
            Rect clip = canvas.getClipBounds();
            IupCanvasHelper.dispatchAction(ihandlePtr, clip.left, clip.top, clip.right - 1, clip.bottom - 1);
        }
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
            IupCanvasHelper.dispatchAction(ihandlePtr, 0, 0, w - 1, h - 1);
            invalidate();
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent ev)
    {
        if (ihandlePtr == 0 || !isEnabled()) return false;

        gestureDetector.onTouchEvent(ev);
        scaleDetector.onTouchEvent(ev);
        handleRotation(ev);

        int x = (int)ev.getX();
        int y = (int)ev.getY();
        int mods = ev.getMetaState();
        int action = ev.getActionMasked();

        /* onScroll has no end notification; close the pan when a second finger lifts */
        if (panActive && (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_CANCEL ||
            (action == MotionEvent.ACTION_POINTER_UP && ev.getPointerCount() <= 2)))
        {
            panActive = false;
            IupCanvasHelper.dispatchGesture(ihandlePtr, GESTURE_PAN, GESTURE_END, x, y, panDx, panDy);
        }

        /* multi-touch: fire TOUCH_CB per pointer + one MULTITOUCH_CB for the batch */
        if (IupCanvasHelper.isTouchEnabled(ihandlePtr))
        {
            int n = ev.getPointerCount();
            int ai = ev.getActionIndex();
            int[] ids = new int[n];
            int[] xs = new int[n];
            int[] ys = new int[n];
            int[] states = new int[n];
            for (int i = 0; i < n; i++)
            {
                ids[i] = ev.getPointerId(i);
                xs[i] = (int)ev.getX(i);
                ys[i] = (int)ev.getY(i);
                if ((action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN) && i == ai)
                    states[i] = 'D';
                else if ((action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_POINTER_UP || action == MotionEvent.ACTION_CANCEL) && i == ai)
                    states[i] = 'U';
                else
                    states[i] = 'M';
            }
            IupCanvasHelper.dispatchTouch(ihandlePtr, n, ids, xs, ys, states, ev.getPointerId(0));
        }

        switch (action)
        {
            case MotionEvent.ACTION_DOWN:
                /* Drag-interactive or gesture canvas owns the sequence; parent must not intercept. */
                if (getParent() != null && (IupCanvasHelper.isDragInteractive(ihandlePtr) || IupCanvasHelper.isGestureEnabled(ihandlePtr)))
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
