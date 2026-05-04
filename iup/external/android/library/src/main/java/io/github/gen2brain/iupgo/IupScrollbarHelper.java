package io.github.gen2brain.iupgo;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewConfiguration;
import androidx.annotation.Keep;


public final class IupScrollbarHelper
{
    private IupScrollbarHelper() {}

    /* iup.h enum values; keep in sync. */
    private static final int IUP_SBUP      = 0;
    private static final int IUP_SBDN      = 1;
    private static final int IUP_SBPGUP    = 2;
    private static final int IUP_SBPGDN    = 3;
    private static final int IUP_SBDRAGV   = 5;
    private static final int IUP_SBLEFT    = 6;
    private static final int IUP_SBRIGHT   = 7;
    private static final int IUP_SBPGLEFT  = 8;
    private static final int IUP_SBPGRIGHT = 9;
    private static final int IUP_SBDRAGH   = 11;


    public static class IupScrollbarView extends View
    {
        private final long ihandlePtr;
        private final boolean horizontal;
        private boolean inverted;

        private double vmin = 0.0, vmax = 1.0, val = 0.0;
        private double pagesize = 0.1, pagestep = 0.1;

        private final Paint trackPaint;
        private final Paint thumbPaint;
        private final float density;

        private boolean dragging;
        private float dragAnchor;
        private final int touchSlop;

        public IupScrollbarView(Context ctx, long ih, boolean h, boolean inv)
        {
            super(ctx);
            ihandlePtr = ih;
            horizontal = h;
            inverted = inv;
            density = ctx.getResources().getDisplayMetrics().density;
            touchSlop = ViewConfiguration.get(ctx).getScaledTouchSlop();

            trackPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
            trackPaint.setColor(0xFFD9D9D9);
            thumbPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
            thumbPaint.setColor(0xFF737373);
        }

        public void setRange(double vmin_, double vmax_, double val_, double pagesize_)
        {
            vmin = vmin_; vmax = vmax_; val = val_; pagesize = pagesize_;
            invalidate();
        }

        public void setSteps(double pagestep_)
        {
            pagestep = pagestep_;
        }

        public void setVal(double v) { val = v; invalidate(); }

        public void setInverted(boolean i) { inverted = i; invalidate(); }

        private RectF thumbRect()
        {
            float w = getWidth(), h = getHeight();
            float length    = horizontal ? w : h;
            float thickness = horizontal ? h : w;

            double range = vmax - vmin;
            if (range <= 0) range = 1.0;

            double prop = pagesize / range;
            if (prop > 1.0)  prop = 1.0;
            if (prop < 0.05) prop = 0.05;

            float thumb_len = (float)(prop * length);
            float track_len = length - thumb_len;

            double span = (vmax - pagesize) - vmin;
            double norm = (span > 0) ? (val - vmin) / span : 0.0;
            if (norm < 0) norm = 0;
            if (norm > 1) norm = 1;
            if (inverted) norm = 1.0 - norm;

            float offset = (float)(norm * track_len);
            if (horizontal) return new RectF(offset, 0, offset + thumb_len, thickness);
            return new RectF(0, offset, thickness, offset + thumb_len);
        }

        @Override
        protected void onDraw(Canvas c)
        {
            float w = getWidth(), h = getHeight();
            float thick = 8f * density;
            float radius = 4f * density;

            if (horizontal) {
                float ty = (h - thick) / 2f;
                c.drawRoundRect(0, ty, w, ty + thick, radius, radius, trackPaint);
            } else {
                float tx = (w - thick) / 2f;
                c.drawRoundRect(tx, 0, tx + thick, h, radius, radius, trackPaint);
            }

            RectF tr = thumbRect();
            c.drawRoundRect(tr, radius, radius, thumbPaint);
        }

        private double clamp(double v)
        {
            double hi = vmax - pagesize;
            if (v < vmin) v = vmin;
            if (v > hi)   v = hi;
            return v;
        }

        private double valueFromOffset(float offset, float thumb_len)
        {
            float length = horizontal ? getWidth() : getHeight();
            float track_len = length - thumb_len;
            double norm = (track_len > 0) ? (offset / track_len) : 0.0;
            if (norm < 0) norm = 0;
            if (norm > 1) norm = 1;
            if (inverted) norm = 1.0 - norm;
            double span = (vmax - pagesize) - vmin;
            if (span < 0) span = 0;
            return vmin + norm * span;
        }

        @Override
        public boolean onTouchEvent(MotionEvent ev)
        {
            float x = ev.getX(), y = ev.getY();
            switch (ev.getActionMasked())
            {
                case MotionEvent.ACTION_DOWN: {
                    if (getParent() != null)
                        getParent().requestDisallowInterceptTouchEvent(true);
                    RectF tr = thumbRect();
                    if (tr.contains(x, y)) {
                        dragging = true;
                        dragAnchor = horizontal ? (x - tr.left) : (y - tr.top);
                    } else {
                        dragging = false;
                        boolean before = horizontal ? (x < tr.left) : (y < tr.top);
                        if (inverted) before = !before;
                        double newval = clamp(val + (before ? -pagestep : pagestep));
                        int op;
                        if (horizontal) op = before ? IUP_SBPGLEFT : IUP_SBPGRIGHT;
                        else            op = before ? IUP_SBPGUP   : IUP_SBPGDN;
                        val = newval;
                        invalidate();
                        dispatchScroll(ihandlePtr, op, newval);
                    }
                    return true;
                }
                case MotionEvent.ACTION_MOVE: {
                    if (!dragging) return true;
                    RectF tr = thumbRect();
                    float thumb_len = horizontal ? tr.width() : tr.height();
                    float pos = horizontal ? (x - dragAnchor) : (y - dragAnchor);
                    double v = valueFromOffset(pos, thumb_len);
                    val = v;
                    invalidate();
                    int op = horizontal ? IUP_SBDRAGH : IUP_SBDRAGV;
                    dispatchScroll(ihandlePtr, op, v);
                    return true;
                }
                case MotionEvent.ACTION_UP:
                case MotionEvent.ACTION_CANCEL:
                    dragging = false;
                    return true;
            }
            return super.onTouchEvent(ev);
        }
    }


    @Keep
    public static View createScrollbar(long ihandlePtr, boolean horizontal, boolean inverted)
    {
        return new IupScrollbarView(IupCommon.getContextThemeWrapper(), ihandlePtr, horizontal, inverted);
    }

    @Keep
    public static void setRange(View v, double vmin, double vmax, double val, double pagesize)
    {
        if (v instanceof IupScrollbarView) ((IupScrollbarView)v).setRange(vmin, vmax, val, pagesize);
    }

    @Keep
    public static void setSteps(View v, double pagestep)
    {
        if (v instanceof IupScrollbarView) ((IupScrollbarView)v).setSteps(pagestep);
    }

    @Keep
    public static void setValue(View v, double val)
    {
        if (v instanceof IupScrollbarView) ((IupScrollbarView)v).setVal(val);
    }

    @Keep
    public static void setInverted(View v, boolean inv)
    {
        if (v instanceof IupScrollbarView) ((IupScrollbarView)v).setInverted(inv);
    }


    public static native void dispatchScroll(long ihandlePtr, int op, double value);
}
