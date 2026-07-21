package io.github.gen2brain.iupgo;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.DashPathEffect;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RadialGradient;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Shader;
import android.graphics.Typeface;
import android.text.TextPaint;

import androidx.annotation.Keep;
import androidx.appcompat.view.ContextThemeWrapper;


public final class IupCanvasHelper
{
    /* Style codes mirror iup_drvdraw.h's IUP_DRAW_* enum. */
    public static final int STYLE_FILL = 0;
    public static final int STYLE_STROKE = 1;
    public static final int STYLE_STROKE_DASH = 2;
    public static final int STYLE_STROKE_DOT = 3;
    public static final int STYLE_STROKE_DASH_DOT = 4;
    public static final int STYLE_STROKE_DASH_DOT_DOT = 5;

    /* Text flag bits from iup_drvdraw.h. */
    public static final int TEXT_LEFT    = 0x0000;
    public static final int TEXT_CENTER  = 0x0001;
    public static final int TEXT_RIGHT   = 0x0002;
    public static final int TEXT_WRAP    = 0x0004;
    public static final int TEXT_ELLIPSIS = 0x0008;
    public static final int TEXT_CLIP    = 0x0010;
    public static final int TEXT_LAYOUTCENTER = 0x0020;


    private IupCanvasHelper() {}


    @Keep
    public static IupAndroidCanvas createCanvas(final long ihandlePtr)
    {
        ContextThemeWrapper themeContext = IupCommon.getContextThemeWrapper();
        return new IupAndroidCanvas(themeContext, ihandlePtr);
    }

    @Keep
    public static void ensureBackBuffer(IupAndroidCanvas view)
    {
        view.ensureBackBuffer();
    }

    @Keep
    public static void flush(IupAndroidCanvas view)
    {
        view.postInvalidate();
    }

    /* HW-pixel back buffer downscaled to logical size; always an ARGB_8888 copy. */
    @Keep
    public static Bitmap getBackBufferSnapshot(IupAndroidCanvas view, int width, int height)
    {
        if (view == null || width <= 0 || height <= 0) return null;
        Bitmap src = view.getBackBuffer();
        if (src == null) return null;
        if (src.getWidth() == width && src.getHeight() == height)
            return src.copy(Bitmap.Config.ARGB_8888, false);
        return Bitmap.createScaledBitmap(src, width, height, true);
    }


    @Keep
    public static void drawLine(IupAndroidCanvas view, int x1, int y1, int x2, int y2, int color, int style, int width)
    {
        Canvas c = view.getBackCanvas(); if (c == null) return;
        Paint p = strokePaint(color, style, width);
        c.drawLine(x1, y1, x2, y2, p);
    }

    @Keep
    public static void drawRectangle(IupAndroidCanvas view, int x1, int y1, int x2, int y2, int color, int style, int width)
    {
        Canvas c = view.getBackCanvas(); if (c == null) return;
        Paint p = stylePaint(color, style, width);
        normalize(tmpRect, x1, y1, x2, y2, style == STYLE_FILL);
        c.drawRect(tmpRect.left, tmpRect.top, tmpRect.right, tmpRect.bottom, p);
    }

    @Keep
    public static void drawRoundedRectangle(IupAndroidCanvas view, int x1, int y1, int x2, int y2, int corner, int color, int style, int width)
    {
        Canvas c = view.getBackCanvas(); if (c == null) return;
        Paint p = stylePaint(color, style, width);
        normalize(tmpRect, x1, y1, x2, y2, style == STYLE_FILL);
        c.drawRoundRect(tmpRect.left, tmpRect.top, tmpRect.right, tmpRect.bottom, corner, corner, p);
    }

    @Keep
    public static void drawPixel(IupAndroidCanvas view, int x, int y, int color)
    {
        Canvas c = view.getBackCanvas(); if (c == null) return;
        Paint p = new Paint();
        p.setColor(color);
        p.setStyle(Paint.Style.FILL);
        c.drawRect(x, y, x + 1, y + 1, p);
    }

    @Keep
    public static void drawArc(IupAndroidCanvas view, int x1, int y1, int x2, int y2, float startDeg, float sweepDeg, int color, int style, int width)
    {
        Canvas c = view.getBackCanvas(); if (c == null) return;
        Paint p = stylePaint(color, style, width);
        normalize(tmpRect, x1, y1, x2, y2, style == STYLE_FILL);
        /* IUP angles: 0 at 3 o'clock, CCW. Android arc: 0 at 3 o'clock, CW. Flip both start and sweep. */
        c.drawArc(tmpRect.left, tmpRect.top, tmpRect.right, tmpRect.bottom, -startDeg, -sweepDeg, style == STYLE_FILL, p);
    }

    @Keep
    public static void drawEllipse(IupAndroidCanvas view, int x1, int y1, int x2, int y2, int color, int style, int width)
    {
        Canvas c = view.getBackCanvas(); if (c == null) return;
        Paint p = stylePaint(color, style, width);
        normalize(tmpRect, x1, y1, x2, y2, style == STYLE_FILL);
        c.drawOval(tmpRect.left, tmpRect.top, tmpRect.right, tmpRect.bottom, p);
    }

    @Keep
    public static void drawPolygon(IupAndroidCanvas view, int[] points, int color, int style, int width)
    {
        Canvas c = view.getBackCanvas(); if (c == null) return;
        if (points == null || points.length < 4) return;
        Paint p = stylePaint(color, style, width);
        Path path = new Path();
        path.moveTo(points[0], points[1]);
        for (int i = 2; i < points.length; i += 2) path.lineTo(points[i], points[i + 1]);
        path.close();
        c.drawPath(path, p);
    }

    @Keep
    public static void drawBezier(IupAndroidCanvas view, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, int color, int style, int width)
    {
        Canvas c = view.getBackCanvas(); if (c == null) return;
        Paint p = strokePaint(color, style, width);
        Path path = new Path();
        path.moveTo(x1, y1);
        path.cubicTo(x2, y2, x3, y3, x4, y4);
        c.drawPath(path, p);
    }

    @Keep
    public static void drawQuadraticBezier(IupAndroidCanvas view, int x1, int y1, int x2, int y2, int x3, int y3, int color, int style, int width)
    {
        Canvas c = view.getBackCanvas(); if (c == null) return;
        Paint p = strokePaint(color, style, width);
        Path path = new Path();
        path.moveTo(x1, y1);
        path.quadTo(x2, y2, x3, y3);
        c.drawPath(path, p);
    }

    @Keep
    public static void drawLinearGradient(IupAndroidCanvas view, int x1, int y1, int x2, int y2, int[] colors, float[] offsets)
    {
        Canvas c = view.getBackCanvas(); if (c == null) return;
        Paint p = new Paint(Paint.ANTI_ALIAS_FLAG);
        p.setStyle(Paint.Style.FILL);
        p.setShader(new LinearGradient(x1, y1, x2, y2, colors, offsets, Shader.TileMode.CLAMP));
        normalize(tmpRect, x1, y1, x2, y2, true);
        c.drawRect(tmpRect.left, tmpRect.top, tmpRect.right, tmpRect.bottom, p);
    }

    @Keep
    public static void drawRadialGradient(IupAndroidCanvas view, int cx, int cy, int radius, int[] colors, float[] offsets)
    {
        Canvas c = view.getBackCanvas(); if (c == null) return;
        Paint p = new Paint(Paint.ANTI_ALIAS_FLAG);
        p.setStyle(Paint.Style.FILL);
        p.setShader(new RadialGradient(cx, cy, radius, colors, offsets, Shader.TileMode.CLAMP));
        c.drawCircle(cx, cy, radius, p);
    }

    @Keep
    public static void drawText(IupAndroidCanvas view, String text, int x, int y, int w, int h, int color, int flags, float orientation,
        String fontFamily, int fontStyle, int fontSize, boolean underline, boolean strikethrough)
    {
        Canvas c = view.getBackCanvas(); if (c == null || text == null) return;

        TextPaint tp = new TextPaint(Paint.ANTI_ALIAS_FLAG);
        tp.setColor(color);

        /* fontStyle: 0=normal, 1=bold, 2=italic, 3=both. */
        int tfStyle = switch (fontStyle) {
            case 1 -> Typeface.BOLD;
            case 2 -> Typeface.ITALIC;
            case 3 -> Typeface.BOLD_ITALIC;
            default -> Typeface.NORMAL;
        };
        Typeface tf = (fontFamily != null && !fontFamily.isEmpty())
            ? Typeface.create(fontFamily, tfStyle)
            : Typeface.create((Typeface)null, tfStyle);
        tp.setTypeface(tf);
        tp.setUnderlineText(underline);
        tp.setStrikeThruText(strikethrough);

        /* textSize compensates canvas scale(density); sp for size>0, abs px for <0. */
        float density = IupCommon.getDisplayDensity();
        if (density < 1.0f) density = 1.0f;
        android.util.DisplayMetrics dm = IupCommon.getContextThemeWrapper().getResources().getDisplayMetrics();
        float hwPx;
        if (fontSize < 0)
            hwPx = -fontSize;
        else if (fontSize > 0)
            hwPx = android.util.TypedValue.applyDimension(android.util.TypedValue.COMPLEX_UNIT_SP, fontSize, dm);
        else
            hwPx = new android.widget.TextView(IupCommon.getContextThemeWrapper()).getTextSize();
        tp.setTextSize(hwPx / density);

        int saved = c.save();
        if ((flags & TEXT_CLIP) != 0)
        {
            c.clipRect(x, y, x + w, y + h);
        }
        if (orientation != 0.0f)
        {
            c.rotate(-(float)orientation, x, y);  /* IUP CCW, Canvas CW */
        }

        Paint.FontMetrics fm = tp.getFontMetrics();
        float lineHeight = fm.bottom - fm.top;
        float baselineY = y - fm.top;
        float anchorX;
        if ((flags & TEXT_CENTER) != 0) { tp.setTextAlign(Paint.Align.CENTER); anchorX = x + w / 2f; }
        else if ((flags & TEXT_RIGHT) != 0) { tp.setTextAlign(Paint.Align.RIGHT); anchorX = x + w; }
        else { tp.setTextAlign(Paint.Align.LEFT); anchorX = x; }

        /* Canvas.drawText is single-line; honour explicit '\n' by drawing each segment at its own baseline. */
        int start = 0;
        while (start <= text.length())
        {
            int nl = text.indexOf('\n', start);
            int end = nl < 0 ? text.length() : nl;
            c.drawText(text, start, end, anchorX, baselineY, tp);
            if (nl < 0) break;
            baselineY += lineHeight;
            start = nl + 1;
        }

        c.restoreToCount(saved);
    }

    @Keep
    public static void drawBitmap(IupAndroidCanvas view, Bitmap bmp, int x, int y, int w, int h, int sx, int sy, int sw, int sh, boolean filter, int alpha)
    {
        Canvas c = view.getBackCanvas(); if (c == null || bmp == null) return;
        Paint p = new Paint(filter ? Paint.FILTER_BITMAP_FLAG : 0);
        p.setAlpha(alpha);
        c.drawBitmap(bmp, new Rect(sx, sy, sx + sw, sy + sh), new Rect(x, y, x + w, y + h), p);
    }


    @Keep
    public static void setClipRect(IupAndroidCanvas view, int x1, int y1, int x2, int y2)
    {
        Canvas c = view.getBackCanvas(); if (c == null) return;
        resetClip(view);
        if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0) return;
        normalize(tmpRect, x1, y1, x2, y2, true);
        c.save();
        c.clipRect(tmpRect.left, tmpRect.top, tmpRect.right, tmpRect.bottom);
        view.clipSaved = true;
    }

    @Keep
    public static void setClipRoundedRect(IupAndroidCanvas view, int x1, int y1, int x2, int y2, int cornerRadius)
    {
        Canvas c = view.getBackCanvas(); if (c == null) return;
        resetClip(view);
        normalize(tmpRect, x1, y1, x2, y2, true);
        c.save();
        Path p = new Path();
        p.addRoundRect(tmpRect, cornerRadius, cornerRadius, Path.Direction.CW);
        c.clipPath(p);
        view.clipSaved = true;
    }

    @Keep
    public static void resetClip(IupAndroidCanvas view)
    {
        Canvas c = view.getBackCanvas(); if (c == null) return;
        if (!view.clipSaved) return;
        try { c.restore(); } catch (IllegalStateException ignored) {}
        view.clipSaved = false;
    }


    public static native void dispatchButton(long ihandlePtr, int button, int pressed, int x, int y, int metaState);
    public static native void dispatchMotion(long ihandlePtr, int x, int y, int metaState, int buttonState);
    public static native void dispatchAction(long ihandlePtr, int x1, int y1, int x2, int y2);
    public static native void dispatchLeaveWindow(long ihandlePtr);
    public static native boolean isTouchEnabled(long ihandlePtr);
    public static native boolean isGestureEnabled(long ihandlePtr);
    public static native void dispatchTouch(long ihandlePtr, int count, int[] ids, int[] xs, int[] ys, int[] states, int primaryId);
    public static native void dispatchGesture(long ihandlePtr, int gesture, int state, int x, int y, double v1, double v2);
    public static native boolean isDragInteractive(long ihandlePtr);


    /* shared scratch; Canvas draws complete before the next JNI round-trip */
    private static final RectF tmpRect = new RectF();

    private static void normalize(RectF out, int x1, int y1, int x2, int y2, boolean fill)
    {
        int l = Math.min(x1, x2), r = Math.max(x1, x2);
        int t = Math.min(y1, y2), b = Math.max(y1, y2);
        /* IUP is inclusive both ends; Canvas.drawRect excludes right/bottom, +1 px for fills only */
        if (fill) { r += 1; b += 1; }
        out.set(l, t, r, b);
    }

    private static Paint stylePaint(int color, int style, int width)
    {
        return (style == STYLE_FILL) ? fillPaint(color) : strokePaint(color, style, width);
    }

    private static Paint fillPaint(int color)
    {
        Paint p = new Paint(Paint.ANTI_ALIAS_FLAG);
        p.setStyle(Paint.Style.FILL);
        p.setColor(color);
        return p;
    }

    private static Paint strokePaint(int color, int style, int width)
    {
        Paint p = new Paint(Paint.ANTI_ALIAS_FLAG);
        p.setStyle(Paint.Style.STROKE);
        p.setColor(color);
        p.setStrokeWidth(Math.max(width, 1));
        switch (style)
        {
            case STYLE_STROKE_DASH:
                p.setPathEffect(new DashPathEffect(new float[]{9, 3}, 0)); break;
            case STYLE_STROKE_DOT:
                p.setPathEffect(new DashPathEffect(new float[]{1, 2}, 0)); break;
            case STYLE_STROKE_DASH_DOT:
                p.setPathEffect(new DashPathEffect(new float[]{7, 3, 1, 3}, 0)); break;
            case STYLE_STROKE_DASH_DOT_DOT:
                p.setPathEffect(new DashPathEffect(new float[]{7, 3, 1, 3, 1, 3}, 0)); break;
            default: break;
        }
        return p;
    }
}
