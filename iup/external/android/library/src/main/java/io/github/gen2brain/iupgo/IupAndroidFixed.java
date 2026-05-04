package io.github.gen2brain.iupgo;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;


/* Absolute-position ViewGroup (GtkFixed analog); sized to max(child, parent). */
public class IupAndroidFixed extends ViewGroup
{
    public IupAndroidFixed(Context context)
    {
        super(context);
    }

    public IupAndroidFixed(Context context, AttributeSet attrs)
    {
        super(context, attrs);
    }

    public IupAndroidFixed(Context context, AttributeSet attrs, int defStyleAttr)
    {
        super(context, attrs, defStyleAttr);
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec)
    {
        int desiredWidth = 0;
        int desiredHeight = 0;

        int count = getChildCount();
        for (int i = 0; i < count; i++)
        {
            View child = getChildAt(i);
            if (child.getVisibility() == GONE)
            {
                continue;
            }
            LayoutParams lp = (LayoutParams)child.getLayoutParams();
            /* Non-positive = not yet laid out. */
            int w = Math.max(lp.width, 0);
            int h = Math.max(lp.height, 0);
            child.measure(
                MeasureSpec.makeMeasureSpec(w, MeasureSpec.EXACTLY),
                MeasureSpec.makeMeasureSpec(h, MeasureSpec.EXACTLY));

            int right = lp.x + w;
            int bottom = lp.y + h;
            if (right > desiredWidth) desiredWidth = right;
            if (bottom > desiredHeight) desiredHeight = bottom;
        }

        setMeasuredDimension(
            resolveSize(desiredWidth, widthMeasureSpec),
            resolveSize(desiredHeight, heightMeasureSpec));
    }

    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom)
    {
        int count = getChildCount();
        for (int i = 0; i < count; i++)
        {
            View child = getChildAt(i);
            if (child.getVisibility() == GONE)
            {
                continue;
            }
            LayoutParams lp = (LayoutParams)child.getLayoutParams();
            int cw = child.getMeasuredWidth();
            int ch = child.getMeasuredHeight();
            child.layout(lp.x, lp.y, lp.x + cw, lp.y + ch);
        }
    }

    /* Called from IupCommon. */
    public void setChildBounds(View child, int x, int y, int width, int height)
    {
        ViewGroup.LayoutParams raw = child.getLayoutParams();
        LayoutParams lp;
        if (raw instanceof LayoutParams)
        {
            lp = (LayoutParams)raw;
            lp.x = x;
            lp.y = y;
            lp.width = width;
            lp.height = height;
        }
        else
        {
            lp = new LayoutParams(width, height, x, y);
        }
        child.setLayoutParams(lp);
    }

    @Override
    protected ViewGroup.LayoutParams generateDefaultLayoutParams()
    {
        return new LayoutParams(0, 0, 0, 0);
    }

    @Override
    public ViewGroup.LayoutParams generateLayoutParams(AttributeSet attrs)
    {
        return new LayoutParams(getContext(), attrs);
    }

    @Override
    protected ViewGroup.LayoutParams generateLayoutParams(ViewGroup.LayoutParams p)
    {
        if (p instanceof LayoutParams)
        {
            return new LayoutParams((LayoutParams)p);
        }
        return new LayoutParams(p.width, p.height, 0, 0);
    }

    @Override
    protected boolean checkLayoutParams(ViewGroup.LayoutParams p)
    {
        return p instanceof LayoutParams;
    }


    /* (x, y) + (width, height). */
    public static class LayoutParams extends ViewGroup.LayoutParams
    {
        public int x;
        public int y;

        public LayoutParams(int width, int height, int x, int y)
        {
            super(width, height);
            this.x = x;
            this.y = y;
        }

        public LayoutParams(Context c, AttributeSet attrs)
        {
            super(c, attrs);
        }

        public LayoutParams(LayoutParams src)
        {
            super(src);
            this.x = src.x;
            this.y = src.y;
        }
    }
}
