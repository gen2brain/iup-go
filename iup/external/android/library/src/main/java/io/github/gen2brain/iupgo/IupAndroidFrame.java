package io.github.gen2brain.iupgo;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.text.TextPaint;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;


/* IupFrame: optional title above a stroked border; children in inner Fixed. */
public class IupAndroidFrame extends ViewGroup
{
    private String title;
    private int strokeColor;
    private int titleColor;
    private int fillColor = 0;
    private boolean sunken;
    private final float cornerRadiusPx;
    private final float strokeWidthPx;
    private final Paint strokePaint;
    private final Paint fillPaint;
    private final TextPaint titlePaint;
    private final IupAndroidFixed inner;

    public IupAndroidFrame(Context ctx)
    {
        super(ctx);
        setWillNotDraw(false);

        strokeWidthPx = IupCommon.getDisplayDensity();
        cornerRadiusPx = 8.0f * IupCommon.getDisplayDensity();  /* 8dp Material radius */

        titleColor = IupCommon.paletteDlgFg;
        strokeColor = (IupCommon.paletteDlgFg & 0x00FFFFFF) | 0x40000000;

        strokePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        strokePaint.setStyle(Paint.Style.STROKE);
        strokePaint.setStrokeWidth(strokeWidthPx);
        strokePaint.setColor(strokeColor);

        fillPaint = new Paint();
        fillPaint.setStyle(Paint.Style.FILL);

        titlePaint = new TextPaint(new TextView(ctx).getPaint());
        titlePaint.setColor(titleColor);

        inner = new IupAndroidFixed(ctx);
        addView(inner);
    }

    /* Re-read title/stroke from palette. Called from IupActivity on theme toggle. */
    @Keep
    public void refreshTheme()
    {
        titleColor = IupCommon.paletteDlgFg;
        strokeColor = (IupCommon.paletteDlgFg & 0x00FFFFFF) | 0x40000000;
        titlePaint.setColor(titleColor);
        strokePaint.setColor(strokeColor);
        invalidate();
    }

    public IupAndroidFixed getInner() { return inner; }

    public void setFrameTitle(String title)
    {
        this.title = title;
        requestLayout();
        invalidate();
    }

    public void setFrameTitleColor(int r, int g, int b)
    {
        titleColor = Color.rgb(r, g, b);
        titlePaint.setColor(titleColor);
        invalidate();
    }

    public void setFrameStrokeColor(int r, int g, int b)
    {
        strokeColor = Color.rgb(r, g, b);
        strokePaint.setColor(strokeColor);
        invalidate();
    }

    public void setFrameBgColor(int r, int g, int b)
    {
        fillColor = Color.rgb(r, g, b);
        invalidate();
    }

    public int getTitleHeight()
    {
        if (title == null || title.isEmpty()) return 0;
        Paint.FontMetrics fm = titlePaint.getFontMetrics();
        return (int)Math.ceil(fm.bottom - fm.top);
    }

    /* sizeUnit is a TypedValue.COMPLEX_UNIT_* (0 = leave size unchanged). */
    public void setTitleFont(android.graphics.Typeface face, int sizeUnit, float size)
    {
        titlePaint.setTypeface(face);
        if (sizeUnit != 0 && size > 0)
        {
            float px = android.util.TypedValue.applyDimension(sizeUnit, size,
                getResources().getDisplayMetrics());
            titlePaint.setTextSize(px);
        }
        requestLayout();
        invalidate();
    }

    public int getFrameStrokePx()
    {
        return (int)Math.ceil(sunken ? strokeWidthPx * 2f : strokeWidthPx);
    }

    public void setSunken(boolean sunken)
    {
        this.sunken = sunken;
        invalidate();
    }

    @Override
    protected void onMeasure(int widthSpec, int heightSpec)
    {
        int w = resolveSize(Integer.MAX_VALUE, widthSpec);
        int h = resolveSize(Integer.MAX_VALUE, heightSpec);
        int stroke = getFrameStrokePx();
        int titleH = getTitleHeight();
        inner.measure(
            MeasureSpec.makeMeasureSpec(Math.max(w - 2 * stroke, 0), MeasureSpec.EXACTLY),
            MeasureSpec.makeMeasureSpec(Math.max(h - 2 * stroke - titleH, 0), MeasureSpec.EXACTLY));
        setMeasuredDimension(w, h);
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b)
    {
        int stroke = getFrameStrokePx();
        int titleH = getTitleHeight();
        int iy = titleH + stroke;
        inner.layout(stroke, iy, stroke + inner.getMeasuredWidth(), iy + inner.getMeasuredHeight());
    }

    @Override
    protected void onDraw(@NonNull Canvas c)
    {
        super.onDraw(c);
        int titleH = getTitleHeight();
        boolean hasTitle = title != null && !title.isEmpty();
        boolean drawSunken = sunken && !hasTitle;  /* spec: SUNKEN only without title */

        float effStroke = drawSunken ? strokeWidthPx * 2f : strokeWidthPx;
        float halfStroke = effStroke / 2f;
        float radius = drawSunken ? 0f : cornerRadiusPx;

        if (Color.alpha(fillColor) != 0)
        {
            fillPaint.setColor(fillColor);
            c.drawRoundRect(0, titleH, getWidth(), getHeight(), radius, radius, fillPaint);
        }

        if (drawSunken)
        {
            int prevColor = strokePaint.getColor();
            float prevWidth = strokePaint.getStrokeWidth();
            /* Inset look: full-opacity neutral grey, double thickness, sharp corners. */
            strokePaint.setColor((IupCommon.paletteDlgFg & 0x00FFFFFF) | 0x90000000);
            strokePaint.setStrokeWidth(effStroke);
            c.drawRect(halfStroke, titleH + halfStroke, getWidth() - halfStroke, getHeight() - halfStroke, strokePaint);
            strokePaint.setColor(prevColor);
            strokePaint.setStrokeWidth(prevWidth);
        }
        else
        {
            c.drawRoundRect(halfStroke, titleH + halfStroke, getWidth() - halfStroke, getHeight() - halfStroke, radius, radius, strokePaint);
        }

        if (hasTitle)
        {
            Paint.FontMetrics fm = titlePaint.getFontMetrics();
            c.drawText(title, 0, -fm.top, titlePaint);
        }
    }
}
