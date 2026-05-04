package io.github.gen2brain.iupgo;

import android.graphics.Rect;
import android.graphics.Typeface;
import android.text.Layout;
import android.text.StaticLayout;
import android.text.TextPaint;
import android.view.View;
import android.widget.TextView;

import androidx.annotation.Keep;

import com.google.android.material.button.MaterialButton;

import java.util.Collections;


public final class IupFontHelper
{
    private IupFontHelper() {}


    /* objectType hint codes passed from C (iupandroid_font.c). */
    public static final int KIND_DEFAULT = 0;
    public static final int KIND_BUTTON  = 1;

    @Keep
    public static int getStringWidth(final long ihandlePtr, Object nativeObject, int objectType, String str)
    {
        CharSequence seq = isMarkup(ihandlePtr) ? IupCommon.parseMarkup(str) : str;
        if (objectType == KIND_BUTTON)
            return measureButtonText(nativeObject, seq).width();
        TextPaint p = resolveTextPaint(nativeObject, null);
        return (int)Math.ceil(Layout.getDesiredWidth(seq, p));
    }

    @Keep
    public static Rect getMultiLineStringSize(final long ihandlePtr, Object nativeObject, int objectType, String str)
    {
        boolean markup = isMarkup(ihandlePtr);
        CharSequence seq = markup ? IupCommon.parseMarkup(str) : str;
        if (objectType == KIND_BUTTON)
            return measureButtonText(nativeObject, seq);
        TextPaint p = resolveTextPaint(nativeObject, null);
        if (markup)
            return measureSpanned(p, seq);
        return measureMultiLine(p, str);
    }

    private static boolean isMarkup(long ihandlePtr)
    {
        if (ihandlePtr == 0) return false;
        String v = IupCommon.iupAttribGet(ihandlePtr, "MARKUP");
        return v != null && (v.equalsIgnoreCase("YES") || v.equals("1"));
    }

    /* StaticLayout honours size/typeface spans that the base TextPaint ignores; needed for MARKUP=YES */
    private static Rect measureSpanned(TextPaint paint, CharSequence seq)
    {
        if (seq == null || seq.length() == 0)
            return measureMultiLine(paint, "");
        StaticLayout layout = StaticLayout.Builder.obtain(seq, 0, seq.length(), paint, 0x3fffffff).build();
        int maxW = 0;
        for (int i = 0; i < layout.getLineCount(); i++) {
            int lw = (int)Math.ceil(layout.getLineWidth(i));
            if (lw > maxW) maxW = lw;
        }
        return new Rect(0, 0, maxW, layout.getHeight());
    }

    @Keep
    public static Rect getCharSize(final long ihandlePtr, Object nativeObject, int objectType)
    {
        TextPaint paint = resolveTextPaint(nativeObject, null);
        if (objectType == KIND_BUTTON && nativeObject instanceof TextView)
            paint = new TextPaint(((TextView)nativeObject).getPaint());
        int charWidth = (int)Math.ceil(Layout.getDesiredWidth("x", paint));
        int charHeight = (int)Math.ceil(paint.getFontSpacing());
        return new Rect(0, 0, charWidth, charHeight);
    }

    @Keep
    public static Rect getTextSize(String family, int style, int sizeUnit, float size, String str)
    {
        return measureMultiLine(paintForFont(family, style, sizeUnit, size), str);
    }

    /** Returns [max_width, line_height, ascent, descent] in HW px. */
    @Keep
    public static int[] getFontDim(String family, int style, int sizeUnit, float size)
    {
        TextPaint p = paintForFont(family, style, sizeUnit, size);
        android.graphics.Paint.FontMetrics fm = p.getFontMetrics();
        int maxWidth   = (int)Math.ceil(p.measureText("W"));
        int lineHeight = (int)Math.ceil(fm.bottom - fm.top);
        int ascent     = (int)Math.ceil(-fm.ascent);
        int descent    = (int)Math.ceil(fm.descent);
        return new int[]{ maxWidth, lineHeight, ascent, descent };
    }

    /* TextPaint with size in HW px so the round-trip through iupdrvScaleNaturalPx stays exact */
    private static TextPaint paintForFont(String family, int style, int sizeUnit, float size)
    {
        TextPaint p = new TextPaint(android.graphics.Paint.ANTI_ALIAS_FLAG);
        int tfStyle = switch (style) {
            case 1 -> Typeface.BOLD;
            case 2 -> Typeface.ITALIC;
            case 3 -> Typeface.BOLD_ITALIC;
            default -> Typeface.NORMAL;
        };
        Typeface tf = (family != null && !family.isEmpty())
            ? Typeface.create(family, tfStyle)
            : Typeface.create((Typeface)null, tfStyle);
        p.setTypeface(tf);

        float hwPx;
        if (sizeUnit == 0 && size == 0)
            hwPx = new TextView(IupCommon.getContextThemeWrapper()).getTextSize();
        else if (sizeUnit == android.util.TypedValue.COMPLEX_UNIT_PX || size < 0)
            hwPx = Math.abs(size);
        else
        {
            android.util.DisplayMetrics dm = IupCommon.getContextThemeWrapper().getResources().getDisplayMetrics();
            hwPx = android.util.TypedValue.applyDimension(sizeUnit, size, dm);
        }
        p.setTextSize(hwPx);
        return p;
    }

    /** sizeUnit is TypedValue.COMPLEX_UNIT_*, 0 leaves size unchanged. */
    @Keep
    public static void applyFont(Object widget, String family, int style, int sizeUnit, float size, boolean underline, boolean strikethrough)
    {
        Typeface base = (family != null && !family.isEmpty())
            ? Typeface.create(family, style)
            : Typeface.create((Typeface)null, style);
        /* NestedScrollView (multi-line) and TextInputLayout (single-line) both stash the inner EditText as tag. */
        if (widget instanceof androidx.core.widget.NestedScrollView)
        {
            Object tag = ((androidx.core.widget.NestedScrollView)widget).getTag();
            if (tag instanceof TextView) widget = tag;
        }
        else if (widget instanceof com.google.android.material.textfield.TextInputLayout)
        {
            Object tag = ((com.google.android.material.textfield.TextInputLayout)widget).getTag();
            if (tag instanceof TextView) widget = tag;
        }
        if (widget instanceof com.google.android.material.textfield.MaterialAutoCompleteTextView m)
        {
            applyTextView(m, base, sizeUnit, size, underline, strikethrough);
            IupListHelper.setListFont(m, base, sizeUnit, size);
            return;
        }
        if (widget instanceof TextView tv)
        {
            applyTextView(tv, base, sizeUnit, size, underline, strikethrough);
            return;
        }
        if (widget instanceof IupAndroidFrame)
        {
            ((IupAndroidFrame)widget).setTitleFont(base, sizeUnit, size);
            return;
        }
        if (widget instanceof android.widget.AdapterView)
        {
            IupListHelper.setListFont((android.widget.AdapterView<?>)widget, base, sizeUnit, size);
            return;
        }
        if (widget instanceof android.widget.LinearLayout box)
        {
            /* EDITBOX wrapper: tagged ListView + TextView children (EditText). */
            Object tag = box.getTag();
            if (tag instanceof android.widget.AdapterView)
                IupListHelper.setListFont((android.widget.AdapterView<?>)tag, base, sizeUnit, size);
            for (int i = 0; i < box.getChildCount(); i++)
            {
                android.view.View c = box.getChildAt(i);
                if (c instanceof TextView tv)
                    applyTextView(tv, base, sizeUnit, size, underline, strikethrough);
                else if (c instanceof com.google.android.material.textfield.TextInputLayout til)
                {
                    Object t = til.getTag();
                    if (t instanceof TextView tv)
                        applyTextView(tv, base, sizeUnit, size, underline, strikethrough);
                }
            }
        }
    }

    private static void applyTextView(TextView tv, Typeface tf, int sizeUnit, float size, boolean underline, boolean strikethrough)
    {
        tv.setTypeface(tf);
        if (sizeUnit != 0 && size > 0) tv.setTextSize(sizeUnit, size);
        int flags = tv.getPaintFlags();
        flags = underline     ? (flags |  android.graphics.Paint.UNDERLINE_TEXT_FLAG)  : (flags & ~android.graphics.Paint.UNDERLINE_TEXT_FLAG);
        flags = strikethrough ? (flags |  android.graphics.Paint.STRIKE_THRU_TEXT_FLAG) : (flags & ~android.graphics.Paint.STRIKE_THRU_TEXT_FLAG);
        tv.setPaintFlags(flags);
    }


    /* Plain text paint for non-button widgets. */
    private static TextPaint sDefaultPaint;

    private static TextPaint resolveTextPaint(Object nativeObject, Typeface typeface)
    {
        /* NestedScrollView (multi-line) and TextInputLayout (single-line) both stash the inner EditText as tag. */
        if (nativeObject instanceof androidx.core.widget.NestedScrollView)
        {
            Object tag = ((androidx.core.widget.NestedScrollView)nativeObject).getTag();
            if (tag instanceof TextView) nativeObject = tag;
        }
        else if (nativeObject instanceof com.google.android.material.textfield.TextInputLayout)
        {
            Object tag = ((com.google.android.material.textfield.TextInputLayout)nativeObject).getTag();
            if (tag instanceof TextView) nativeObject = tag;
        }
        TextPaint paint;
        if (nativeObject instanceof TextView)
            paint = new TextPaint(((TextView)nativeObject).getPaint());
        else
        {
            if (sDefaultPaint == null)
                sDefaultPaint = new TextPaint(new TextView(IupCommon.getContextThemeWrapper()).getPaint());
            paint = new TextPaint(sDefaultPaint);
        }
        if (typeface != null) paint.setTypeface(typeface);
        return paint;
    }

    private static Rect measureMultiLine(TextPaint paint, String str)
    {
        /* fm.bottom-fm.top is full glyph box incl. leading; getFontSpacing trims it */
        android.graphics.Paint.FontMetrics fm = paint.getFontMetrics();
        int charHeight = (int)Math.ceil(fm.bottom - fm.top);
        if (str == null || str.isEmpty()) return new Rect(0, 0, 0, charHeight);
        int maxWidth = 0, runningHeight = 0;
        for (String line : str.split("\n"))
        {
            int w = (int)Math.ceil(Layout.getDesiredWidth(line, paint));
            if (w > maxWidth) maxWidth = w;
            runningHeight += charHeight;
        }
        return new Rect(0, 0, maxWidth, runningHeight);
    }


    /* Temp MaterialButton for text-only measurement; border reported separately. */
    private static MaterialButton sSampleButton;
    private static int sEmptyButtonW, sEmptyButtonH;

    private static synchronized MaterialButton getSampleButton()
    {
        if (sSampleButton != null) return sSampleButton;
        MaterialButton b = new MaterialButton(IupCommon.getContextThemeWrapper());
        /* setText() -> requestLayout() reads LayoutParams.width; NPE without. */
        b.setLayoutParams(new android.view.ViewGroup.LayoutParams(
            android.view.ViewGroup.LayoutParams.WRAP_CONTENT,
            android.view.ViewGroup.LayoutParams.WRAP_CONTENT));
        b.setInsetTop(0);
        b.setInsetBottom(0);
        b.setMinHeight(0);
        b.setMinimumHeight(0);
        b.setMinWidth(0);
        b.setMinimumWidth(0);
        b.setIncludeFontPadding(false);
        b.setText("");
        int unspec = View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED);
        b.measure(unspec, unspec);
        /* Border = theme padding; empty measured_h would include a phantom line. */
        sEmptyButtonW = b.getMeasuredWidth();
        sEmptyButtonH = b.getPaddingTop() + b.getPaddingBottom();
        sSampleButton = b;
        return b;
    }

    /* measure against the sample MaterialButton; CharSequence input so MARKUP spans apply */
    public static synchronized Rect measureButtonText(Object nativeObject, CharSequence text)
    {
        MaterialButton b = getSampleButton();
        b.setText(text == null ? "" : text);
        int unspec = View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED);
        b.measure(unspec, unspec);
        boolean borderless = (nativeObject instanceof IupButtonHelper.IupMaterialButton)
            && ((IupButtonHelper.IupMaterialButton)nativeObject).isBorderless;
        int emptyW = borderless ? 0 : sEmptyButtonW;
        int w = b.getMeasuredWidth() - emptyW;
        int h = b.getMeasuredHeight() - sEmptyButtonH;
        b.setText("");
        if (w < 0) w = 0;
        if (h < 0) h = 0;
        return new Rect(0, 0, w, h);
    }

    @Keep
    public static synchronized Rect getButtonBorderSize()
    {
        getSampleButton();
        return new Rect(0, 0, sEmptyButtonW, sEmptyButtonH);
    }


    /* IUP format "family, [Bold][Italic] size"; sp round-trips through iupdrvSetFontAttrib. */
    @Keep
    public static String getSystemFont()
    {
        android.content.Context ctx = IupCommon.getContextThemeWrapper();
        TextView tv = new TextView(ctx);
        android.content.res.Resources res = ctx.getResources();
        float scaledDensity = res.getDisplayMetrics().density * res.getConfiguration().fontScale;
        int sizeSp = Math.max(1, Math.round(tv.getTextSize() / scaledDensity));

        Typeface tf = tv.getTypeface();
        int style = (tf != null) ? tf.getStyle() : Typeface.NORMAL;
        String styleStr = "";
        if ((style & Typeface.BOLD) != 0 && (style & Typeface.ITALIC) != 0) styleStr = "Bold Italic ";
        else if ((style & Typeface.BOLD) != 0) styleStr = "Bold ";
        else if ((style & Typeface.ITALIC) != 0) styleStr = "Italic ";

        /* Public Typeface API doesn't expose the family name; default is sans-serif. */
        return "sans-serif, " + styleStr + sizeSp;
    }

    /* Family names addressable via Typeface.create(name, style). */
    @Keep
    public static String[] getSystemFamilyList()
    {
        java.util.TreeSet<String> families = new java.util.TreeSet<>(String.CASE_INSENSITIVE_ORDER);
        try
        {
            java.io.File f = new java.io.File("/system/etc/fonts.xml");
            if (f.exists())
            {
                try (java.io.FileInputStream in = new java.io.FileInputStream(f)) {
                    org.xmlpull.v1.XmlPullParser p = android.util.Xml.newPullParser();
                    p.setInput(in, null);
                    int event = p.getEventType();
                    while (event != org.xmlpull.v1.XmlPullParser.END_DOCUMENT) {
                        if (event == org.xmlpull.v1.XmlPullParser.START_TAG && "family".equals(p.getName())) {
                            String name = p.getAttributeValue(null, "name");
                            if (name != null && !name.isEmpty())
                                families.add(name);
                        }
                        event = p.next();
                    }
                } catch (java.io.IOException ignore) {
                }
            }
        }
        catch (Throwable t)
        {
            /* fall through to fallback */
        }
        if (families.isEmpty())
        {
            String[] fallback = {
                "sans-serif", "sans-serif-condensed", "sans-serif-light",
                "sans-serif-medium", "sans-serif-black", "sans-serif-thin",
                "serif", "serif-monospace", "monospace", "cursive", "casual"
            };
            Collections.addAll(families, fallback);
        }
        return families.toArray(new String[0]);
    }
}
