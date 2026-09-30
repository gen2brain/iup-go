package io.github.gen2brain.iupgo;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.drawable.ClipDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.LayerDrawable;
import android.util.AttributeSet;
import android.view.Gravity;
import android.widget.ProgressBar;


/* no native vertical ProgressBar; render a horizontal one rotated 90 deg via swapped measure/size/draw */
@SuppressLint("ViewConstructor")
public class IupProgressBarVertical extends ProgressBar
{
    private LayerDrawable layers;
    private int thickness;

    public IupProgressBarVertical(Context ctx, AttributeSet attrs, int defStyle)
    {
        super(ctx, attrs, defStyle);
    }

    void setTrack(int thicknessPx, int indicatorColor, int trackColor)
    {
        thickness = thicknessPx;
        GradientDrawable track = new GradientDrawable();
        track.setCornerRadius(thicknessPx / 2f);
        track.setColor(trackColor);
        GradientDrawable fill = new GradientDrawable();
        fill.setCornerRadius(thicknessPx / 2f);
        fill.setColor(indicatorColor);
        layers = new LayerDrawable(new Drawable[] { track, new ClipDrawable(fill, Gravity.START, ClipDrawable.HORIZONTAL) });
        layers.setId(0, android.R.id.background);
        layers.setId(1, android.R.id.progress);
        setProgressDrawable(layers);
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh)
    {
        if (layers != null)
        {
            int inset = Math.max(0, (w - thickness) / 2);
            layers.setLayerInset(0, 0, inset, 0, inset);
            layers.setLayerInset(1, 0, inset, 0, inset);
        }
        super.onSizeChanged(h, w, oldh, oldw);
        invalidate();
    }

    @Override
    @SuppressWarnings("SuspiciousNameCombination")
    protected synchronized void onMeasure(int widthMeasureSpec, int heightMeasureSpec)
    {
        /* swap: super is still horizontal; the long axis is height */
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
