package io.github.gen2brain.iupgo;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
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
import android.view.View;

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
    public static void setBgColor(View view, int r, int g, int b)
    {
        if (view != null) view.setBackgroundColor(Color.rgb(r, g, b));
    }

    @Keep
    public static void ensureBackBuffer(IupAndroidCanvas view)
    {
        view.ensureBackBuffer();
        while (!view.layers.isEmpty())
            endLayer(view);
        resetClip(view);
    }

    @Keep
    public static void flush(IupAndroidCanvas view)
    {
        if (!view.inDraw)
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
    public static void drawLinearGradient(IupAndroidCanvas view, int x1, int y1, int x2, int y2,
                                          int sx, int sy, int ex, int ey, int[] colors, float[] offsets)
    {
        Canvas c = view.getBackCanvas(); if (c == null) return;
        Paint p = new Paint(Paint.ANTI_ALIAS_FLAG);
        p.setStyle(Paint.Style.FILL);
        p.setShader(new LinearGradient(sx, sy, ex, ey, colors, offsets, Shader.TileMode.CLAMP));
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
        Paint.FontMetrics fm = tp.getFontMetrics();
        float lineHeight = fm.bottom - fm.top;
        float fx = x, fy = y, fw = w;
        if (orientation != 0.0f)
        {
            float px = x, py = y;
            if ((flags & TEXT_LAYOUTCENTER) != 0)
            {
                String[] lines = text.split("\n", -1);
                float layoutW = 0;
                for (String line : lines)
                    layoutW = Math.max(layoutW, tp.measureText(line));
                float layoutH = lines.length * lineHeight;
                px = x + w / 2f;
                py = y + h / 2f;
                fx = px - layoutW / 2f;
                fy = py - layoutH / 2f;
                fw = layoutW;
            }
            c.rotate(-(float)orientation, px, py);  /* IUP CCW, Canvas CW */
        }

        float baselineY = fy - fm.top;
        float anchorX;
        if ((flags & TEXT_CENTER) != 0) { tp.setTextAlign(Paint.Align.CENTER); anchorX = fx + fw / 2f; }
        else if ((flags & TEXT_RIGHT) != 0) { tp.setTextAlign(Paint.Align.RIGHT); anchorX = fx + fw; }
        else { tp.setTextAlign(Paint.Align.LEFT); anchorX = fx; }

        /* Canvas.drawText is single-line; honour explicit '\n' by drawing each segment at its own baseline. */
        int start = 0;
        while (start <= text.length())
        {
            int nl = text.indexOf('\n', start);
            int end = nl < 0 ? text.length() : nl;
            CharSequence line = text.subSequence(start, end);
            if ((flags & TEXT_ELLIPSIS) != 0 && w > 0)
                line = android.text.TextUtils.ellipsize(line, tp, w, android.text.TextUtils.TruncateAt.END);
            c.drawText(line, 0, line.length(), anchorX, baselineY, tp);
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
        Paint p = new Paint();
        p.setFilterBitmap(filter);
        p.setAlpha(alpha);
        c.drawBitmap(bmp, new Rect(sx, sy, sx + sw, sy + sh), new Rect(x, y, x + w, y + h), p);
    }

    @Keep
    public static void setTransform(IupAndroidCanvas view, float a, float b, float c, float d, float e, float f)
    {
        view.setDrawTransform(a, b, c, d, e, f);
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
        if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0) return;
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
        view.applyDrawTransform();
    }

    @Keep
    public static void beginLayer(IupAndroidCanvas view, int alpha)
    {
        Canvas c = view.getBackCanvas(); if (c == null) return;
        view.layers.add(new int[]{c.saveLayerAlpha(null, alpha), view.clipSaved ? 1 : 0});
        view.clipSaved = false;
    }

    @Keep
    public static void endLayer(IupAndroidCanvas view)
    {
        if (view.layers.isEmpty()) return;
        int[] layer = view.layers.remove(view.layers.size() - 1);
        Canvas c = view.getBackCanvas();
        if (c != null)
        {
            resetClip(view);
            try { c.restoreToCount(layer[0]); } catch (IllegalArgumentException | IllegalStateException ignored) {}
        }
        view.clipSaved = layer[1] != 0;
        view.applyDrawTransform();
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
    public static native void dispatchTextInput(long ihandlePtr, String text);
    public static native boolean dispatchKey(long ihandlePtr, int keyCode, int unicode, int metaState);
    public static native boolean wantsTextInput(long ihandlePtr);


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

    private static int strokeCap = 0;
    private static int strokeJoin = 0;
    private static float[] strokeDashes = null;
    private static float strokeDashOffset = 0;

    @Keep
    public static void setStroke(int cap, int join, float[] dashes, float offset)
    {
        strokeCap = cap;
        strokeJoin = join;
        strokeDashes = dashes;
        strokeDashOffset = offset;
    }

    private static Paint strokePaint(int color, int style, int width)
    {
        Paint p = new Paint(Paint.ANTI_ALIAS_FLAG);
        p.setStyle(Paint.Style.STROKE);
        p.setColor(color);
        p.setStrokeWidth(Math.max(width, 1));
        p.setStrokeCap(strokeCap == 1 ? Paint.Cap.ROUND : strokeCap == 2 ? Paint.Cap.SQUARE : Paint.Cap.BUTT);
        p.setStrokeJoin(strokeJoin == 1 ? Paint.Join.ROUND : strokeJoin == 2 ? Paint.Join.BEVEL : Paint.Join.MITER);
        p.setStrokeMiter(10.0f);
        if (strokeDashes != null)
            p.setPathEffect(new DashPathEffect(strokeDashes, strokeDashOffset));
        return p;
    }

    @Keep
    public static void drawPathFill(IupAndroidCanvas view, float[] segs, int color, int count, int rule)
    {
        Canvas c = view.getBackCanvas(); if (c == null) return;
        Paint p = fillPaint(color);
        Path path = pathFromSegments(segs, count);
        path.setFillType(rule == 1 ? Path.FillType.EVEN_ODD : Path.FillType.WINDING);
        c.drawPath(path, p);
    }

    @Keep
    public static void drawPathFillGradient(IupAndroidCanvas view, float[] segs, int count, int rule,
                                            int sx, int sy, int ex, int ey, int cx, int cy, int radius, int type,
                                            int[] colors, float[] offsets)
    {
        Canvas c = view.getBackCanvas(); if (c == null) return;
        Paint p = new Paint(Paint.ANTI_ALIAS_FLAG);
        p.setStyle(Paint.Style.FILL);
        Path path = pathFromSegments(segs, count);
        path.setFillType(rule == 1 ? Path.FillType.EVEN_ODD : Path.FillType.WINDING);
        if (type == SOURCE_LINEAR_GRADIENT)
            p.setShader(new LinearGradient(sx, sy, ex, ey, colors, offsets, Shader.TileMode.CLAMP));
        else
            p.setShader(new RadialGradient(cx, cy, radius, colors, offsets, Shader.TileMode.CLAMP));
        c.drawPath(path, p);
    }

    @Keep
    public static void drawPathStroke(IupAndroidCanvas view, float[] segs, int color, int count, int style, int width)
    {
        Canvas c = view.getBackCanvas(); if (c == null) return;
        c.drawPath(pathFromSegments(segs, count), strokePaint(color, style, width));
    }

    @Keep
    public static void drawPathStrokeGradient(IupAndroidCanvas view, float[] segs, int color, int count, int style, int width,
                                              int sx, int sy, int ex, int ey, int cx, int cy, int radius, int type,
                                              int[] colors, float[] offsets)
    {
        Canvas c = view.getBackCanvas(); if (c == null) return;
        Paint p = strokePaint(color, style, width);
        if (type == SOURCE_LINEAR_GRADIENT)
            p.setShader(new LinearGradient(sx, sy, ex, ey, colors, offsets, Shader.TileMode.CLAMP));
        else
            p.setShader(new RadialGradient(cx, cy, radius, colors, offsets, Shader.TileMode.CLAMP));
        c.drawPath(pathFromSegments(segs, count), p);
    }

    @Keep
    public static void setClipPath(IupAndroidCanvas view, float[] segs, int count, int rule)
    {
        Canvas c = view.getBackCanvas(); if (c == null) return;
        resetClip(view);
        Path path = pathFromSegments(segs, count);
        path.setFillType(rule == 1 ? Path.FillType.EVEN_ODD : Path.FillType.WINDING);
        c.save();
        c.clipPath(path);
        view.clipSaved = true;
    }

    private static Path pathFromSegments(float[] segs, int count)
    {
        Path path = new Path();
        if (segs == null) return path;
        int n = Math.min(count, segs.length / 9);
        boolean current = false;
        for (int s = 0; s < n; s++)
        {
            int base = s * 9;
            int op = (int)segs[base + 0];
            float x1 = segs[base + 1];
            float y1 = segs[base + 2];
            float x2 = segs[base + 3];
            float y2 = segs[base + 4];
            float x3 = segs[base + 5];
            float y3 = segs[base + 6];
            double a1 = segs[base + 7];
            double a2 = segs[base + 8];

            switch (op)
            {
                case 0:
                    path.moveTo(x1, y1);
                    current = true;
                    break;
                case 1:
                    if (current) path.lineTo(x1, y1);
                    else path.moveTo(x1, y1);
                    current = true;
                    break;
                case 2:
                    path.cubicTo(x1, y1, x2, y2, x3, y3);
                    current = true;
                    break;
                case 3:
                    path.quadTo(x1, y1, x2, y2);
                    current = true;
                    break;
                case 4:
                {
                    float cx = x1, cy = y1, rx = x2, ry = y2;
                    float sweep = (float)(a2 - a1);
                    while (sweep < 0) sweep += 360;
                    while (sweep > 360) sweep -= 360;
                    RectF oval = new RectF(cx - rx, cy - ry, cx + rx, cy + ry);
                    if (sweep >= 360f)
                    {
                        path.arcTo(oval, (float)-a1, -180f);
                        path.arcTo(oval, (float)-a1 - 180f, -180f);
                    }
                    else if (sweep >= 0.01f)
                        path.arcTo(oval, (float)-a1, -sweep);
                    current = true;
                    break;
                }
                case 5:
                    path.close();
                    break;
            }
        }
        return path;
    }

    private static final int SOURCE_LINEAR_GRADIENT = 1;
}
