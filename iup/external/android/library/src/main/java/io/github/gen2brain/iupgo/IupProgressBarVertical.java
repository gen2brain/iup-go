package io.github.gen2brain.iupgo;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.Canvas;
import android.util.AttributeSet;
import android.widget.ProgressBar;


/* no native vertical ProgressBar; render a horizontal one rotated 90 deg via swapped measure/size/draw */
@SuppressLint("ViewConstructor")
public class IupProgressBarVertical extends ProgressBar
{
    public IupProgressBarVertical(Context ctx, AttributeSet attrs, int defStyle)
    {
        super(ctx, attrs, defStyle);
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh)
    {
        super.onSizeChanged(h, w, oldh, oldw);
        invalidate();
    }

    @Override
    @SuppressWarnings("SuspiciousNameCombination")
    protected synchronized void onMeasure(int widthMeasureSpec, int heightMeasureSpec)
    {
        /* swap: super is still horizontal; our long axis is height */
        super.onMeasure(heightMeasureSpec, widthMeasureSpec);
        setMeasuredDimension(getMeasuredHeight(), getMeasuredWidth());
    }

    @Override
    protected void onDraw(Canvas c)
    {
        c.rotate(-90);
        c.translate(-getHeight(), 0);
        super.onDraw(c);
    }
}
