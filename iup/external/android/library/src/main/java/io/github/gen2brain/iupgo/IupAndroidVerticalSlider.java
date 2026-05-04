package io.github.gen2brain.iupgo;

import android.content.Context;
import android.view.MotionEvent;
import android.view.ViewGroup;

import com.google.android.material.slider.Slider;


/* no native Material3 vertical slider; rotate a horizontal one 90 and remap touch */
public class IupAndroidVerticalSlider extends ViewGroup
{
    private final Slider slider;

    public IupAndroidVerticalSlider(Context ctx)
    {
        super(ctx);
        slider = new Slider(ctx);
        slider.setRotation(-90f);
        addView(slider);
    }

    public Slider getSlider() { return slider; }

    @Override
    protected void onMeasure(int widthSpec, int heightSpec)
    {
        int w = resolveSize(100, widthSpec);
        int h = resolveSize(400, heightSpec);
        /* slider is rotated -90, so its pre-rotation layout swaps w/h */
        slider.measure(
            MeasureSpec.makeMeasureSpec(h, MeasureSpec.EXACTLY),
            MeasureSpec.makeMeasureSpec(w, MeasureSpec.EXACTLY));
        setMeasuredDimension(w, h);
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b)
    {
        int w = getMeasuredWidth();
        int h = getMeasuredHeight();
        int sW = slider.getMeasuredWidth();
        int sH = slider.getMeasuredHeight();
        /* lay out un-rotated at center; rotation pivots on midpoint to fit wrapper bounds */
        int left = (w - sW) / 2;
        int top = (h - sH) / 2;
        slider.layout(left, top, left + sW, top + sH);
        slider.setPivotX(sW / 2f);
        slider.setPivotY(sH / 2f);
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent ev)
    {
        /* invert the -90 draw rotation: getHeight()-y, x land in slider local coords */
        MotionEvent mapped = MotionEvent.obtain(ev);
        mapped.setLocation(getHeight() - ev.getY(), ev.getX());
        boolean handled = slider.dispatchTouchEvent(mapped);
        mapped.recycle();
        return handled;
    }
}
