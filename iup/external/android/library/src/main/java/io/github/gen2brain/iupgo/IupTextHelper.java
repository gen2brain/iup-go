package io.github.gen2brain.iupgo;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Typeface;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.text.Editable;
import android.text.InputFilter;
import android.text.InputType;
import android.text.Layout;
import android.text.Spanned;
import android.text.TextPaint;
import android.text.TextWatcher;
import android.text.method.KeyListener;
import android.text.method.LinkMovementMethod;
import android.text.style.ClickableSpan;
import android.text.style.AbsoluteSizeSpan;
import android.text.style.AlignmentSpan;
import android.text.style.BackgroundColorSpan;
import android.text.style.ForegroundColorSpan;
import android.text.style.ImageSpan;
import android.text.style.LeadingMarginSpan;
import android.text.style.RelativeSizeSpan;
import android.text.style.StrikethroughSpan;
import android.text.style.StyleSpan;
import android.text.style.SubscriptSpan;
import android.text.style.SuperscriptSpan;
import android.text.style.TypefaceSpan;
import android.text.style.UnderlineSpan;
import android.view.ContextThemeWrapper;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.HorizontalScrollView;
import android.widget.LinearLayout;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.appcompat.widget.AppCompatEditText;
import androidx.core.widget.NestedScrollView;

import com.google.android.material.button.MaterialButton;
import com.google.android.material.color.MaterialColors;
import com.google.android.material.textfield.TextInputEditText;
import com.google.android.material.textfield.TextInputLayout;

import android.content.res.ColorStateList;
import android.graphics.Color;


public final class IupTextHelper
{
    private IupTextHelper() {}

    private static final java.util.WeakHashMap<IupEditText, Boolean> sThemableTexts = new java.util.WeakHashMap<>();
    private static final java.util.WeakHashMap<TextInputLayout, Boolean> sThemableTils = new java.util.WeakHashMap<>();

    static {
        IupTheme.register(IupTextHelper::refreshAllTexts);
        IupTheme.register(IupTextHelper::refreshAllTils);
    }

    private static void refreshAllTexts()
    {
        for (IupEditText tv : new java.util.ArrayList<>(sThemableTexts.keySet()))
        {
            if (tv != null) applyTextPalette(tv);
        }
    }

    private static void refreshAllTils()
    {
        Context ctx = IupCommon.getThemeContext();
        int outline = MaterialColors.getColor(ctx,
            com.google.android.material.R.attr.colorOutline, Color.GRAY);
        int primary = MaterialColors.getColor(ctx,
            androidx.appcompat.R.attr.colorPrimary, Color.BLUE);
        int onSurfaceVar = MaterialColors.getColor(ctx,
            com.google.android.material.R.attr.colorOnSurfaceVariant, Color.DKGRAY);

        int[][] states = { { android.R.attr.state_focused }, { android.R.attr.state_hovered }, {} };
        int[]   colors = { primary, primary, outline };
        ColorStateList strokeCsl = new ColorStateList(states, colors);

        for (TextInputLayout til : new java.util.ArrayList<>(sThemableTils.keySet()))
        {
            if (til == null) continue;
            til.setBoxStrokeColorStateList(strokeCsl);
            til.setDefaultHintTextColor(ColorStateList.valueOf(onSurfaceVar));
            til.setHintTextColor(ColorStateList.valueOf(primary));
            til.setBoxBackgroundColor(IupCommon.paletteTxtBg);
        }
    }

    private static void applyTextPalette(IupEditText tv)
    {
        tv.setTextColor(IupCommon.paletteTxtFg);
        tv.setHintTextColor(IupCommon.blendColor(IupCommon.paletteTxtBg, IupCommon.paletteTxtFg, 0.50f));
    }

    /* TextInputEditText cooperates with TIL when wrapped; behaves like AppCompatEditText standalone. */
    @android.annotation.SuppressLint("ViewConstructor")
    public static class IupEditText extends TextInputEditText
    {
        long ihandlePtr;
        KeyListener savedKeyListener;
        int maxChars;
        /* Set during programmatic mutation; the TextWatcher skips dispatch while true. */
        boolean suppressVC;
        /* FILTER mode: 0=none, 1=LOWERCASE, 2=UPPERCASE, 3=NUMBER. */
        int filterMode;
        boolean overwrite;

        public IupEditText(Context ctx, long ih)
        {
            super(ctx);
            ihandlePtr = ih;
        }

        @Override
        public boolean dispatchKeyEvent(android.view.KeyEvent event)
        {
            if (event.getKeyCode() == android.view.KeyEvent.KEYCODE_INSERT
                && event.getAction() == android.view.KeyEvent.ACTION_DOWN
                && !event.isCtrlPressed() && !event.isAltPressed() && !event.isShiftPressed())
            {
                overwrite = !overwrite;
                return true;
            }
            return super.dispatchKeyEvent(event);
        }

        @Override
        protected void onSelectionChanged(int selStart, int selEnd)
        {
            super.onSelectionChanged(selStart, selEnd);
            if (suppressVC) return;
            dispatchCaret(ihandlePtr, selStart);
        }
    }


    private static IupEditText newEditText(long ihandlePtr, int maxLines, boolean materialContext)
    {
        Context ctx = materialContext
            ? IupCommon.getContextThemeWrapper()
            : new ContextThemeWrapper(IupCommon.getContextThemeWrapper(),
                android.R.style.Theme_DeviceDefault);
        IupEditText tv = new IupEditText(ctx, ihandlePtr);
        if (maxLines > 0) tv.setMaxLines(maxLines);
        applyTextPalette(tv);
        sThemableTexts.put(tv, Boolean.TRUE);
        tv.savedKeyListener = tv.getKeyListener();
        installCallbacks(tv);
        return tv;
    }

    @Keep
    public static View createSingleLineText(final long ihandlePtr)
    {
        IupEditText tv = newEditText(ihandlePtr, 1, true);
        tv.setSingleLine(true);
        /* NO_SUGGESTIONS so filter sees per-key, not composition batches. */
        tv.setInputType(InputType.TYPE_CLASS_TEXT
            | InputType.TYPE_TEXT_VARIATION_NORMAL
            | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);

        /* M3 textInputStyle defaults to OutlinedBox; hint stays off until CUEBANNER is set. */
        TextInputLayout til = new TextInputLayout(IupCommon.getContextThemeWrapper());
        til.setHintEnabled(false);
        til.setBoxCollapsedPaddingTop(0);
        tv.setMinHeight(0);
        tv.setMinimumHeight(0);
        til.addView(tv, new LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.WRAP_CONTENT));
        til.setTag(tv);
        sThemableTils.put(til, Boolean.TRUE);
        return til;
    }

    @Keep
    public static View createMultiLineText(final long ihandlePtr, boolean wordWrap, boolean autoHide)
    {
        IupEditText tv = newEditText(ihandlePtr, 0, false);
        tv.setInputType(InputType.TYPE_CLASS_TEXT
            | InputType.TYPE_TEXT_VARIATION_NORMAL
            | InputType.TYPE_TEXT_FLAG_MULTI_LINE
            | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);
        tv.setSingleLine(false);
        tv.setHorizontallyScrolling(!wordWrap);
        /* TIET inherits M3 bodyLarge metrics through the appcompat editTextStyle chain; reset to classic. */
        tv.setLineSpacing(0f, 1.0f);
        tv.setIncludeFontPadding(false);
        tv.setGravity(Gravity.START | Gravity.TOP);
        /* LinkMovementMethod dispatches ClickableSpan taps; NestedScrollView owns scroll. */
        tv.setMovementMethod(LinkMovementMethod.getInstance());

        NestedScrollView sv = new NestedScrollView(tv.getContext());
        sv.setFillViewport(true);
        sv.setVerticalScrollBarEnabled(true);
        sv.setScrollbarFadingEnabled(autoHide);

        if (wordWrap)
        {
            sv.addView(tv, new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT));
        }
        else
        {
            HorizontalScrollView hsv = new HorizontalScrollView(tv.getContext());
            hsv.setFillViewport(true);
            hsv.setHorizontalScrollBarEnabled(true);
            hsv.setScrollbarFadingEnabled(autoHide);
            hsv.addView(tv, new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT));
            sv.addView(hsv, new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT));
        }
        sv.setTag(tv);
        return sv;
    }

    /* Synced with iupdrvTextAddSpin. */
    static final int SPIN_BUTTON_WIDTH_DP = 36;

    /* Horizontal wrapper: EditText + minus/plus MaterialButtons. Tag is the EditText. */
    @Keep
    public static View createSpinnerText(final long ihandlePtr)
    {
        final IupEditText tv = newEditText(ihandlePtr, 1, true);
        tv.setSingleLine(true);
        tv.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_SIGNED);
        tv.setGravity(Gravity.CENTER_VERTICAL | Gravity.START);

        Context ctx = IupCommon.getContextThemeWrapper();
        TextInputLayout til = new TextInputLayout(ctx);
        til.setHintEnabled(false);
        til.setBoxCollapsedPaddingTop(0);
        tv.setMinHeight(0);
        tv.setMinimumHeight(0);
        til.addView(tv, new LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        til.setTag(tv);
        sThemableTils.put(til, Boolean.TRUE);

        LinearLayout box = new LinearLayout(ctx);
        box.setOrientation(LinearLayout.HORIZONTAL);

        LinearLayout.LayoutParams tilLp = new LinearLayout.LayoutParams(0,
            ViewGroup.LayoutParams.MATCH_PARENT, 1.0f);
        box.addView(til, tilLp);

        int btnWidthPx = (int)(SPIN_BUTTON_WIDTH_DP * IupCommon.getDisplayDensity());
        final MaterialButton down = makeSpinButton(ctx, "−");
        final MaterialButton up   = makeSpinButton(ctx, "+");
        LinearLayout.LayoutParams btLp = new LinearLayout.LayoutParams(
            btnWidthPx, ViewGroup.LayoutParams.MATCH_PARENT);
        box.addView(down, btLp);
        box.addView(up, new LinearLayout.LayoutParams(btLp));

        down.setOnClickListener(v -> spinStep(tv, ihandlePtr, -1));
        up.setOnClickListener(v -> spinStep(tv, ihandlePtr, +1));

        box.setTag(tv);
        return box;
    }


    private static MaterialButton makeSpinButton(Context ctx, String glyph)
    {
        MaterialButton b = new MaterialButton(ctx, null,
            com.google.android.material.R.attr.materialButtonOutlinedStyle);
        b.setText(glyph);
        b.setInsetTop(0);
        b.setInsetBottom(0);
        b.setMinHeight(0);
        b.setMinimumHeight(0);
        b.setMinWidth(0);
        b.setMinimumWidth(0);
        b.setPadding(0, 0, 0, 0);
        b.setTextSize(android.util.TypedValue.COMPLEX_UNIT_SP, 16);
        b.setCornerRadius(0);
        return b;
    }


    /* Unwraps IupEditText from spin LinearLayout or multiline NestedScrollView tag. */
    private static IupEditText resolve(View v)
    {
        if (v instanceof IupEditText) return (IupEditText)v;
        if (v instanceof ViewGroup)
        {
            Object tag = v.getTag();
            if (tag instanceof IupEditText) return (IupEditText)tag;
        }
        return null;
    }


    /* BORDER=NO: drop EditText chrome so fields render inline. */
    @Keep
    public static void setBgColor(View v, int r, int g, int b)
    {
        int color = Color.rgb(r, g, b);
        if (v instanceof TextInputLayout til) til.setBoxBackgroundColor(color);
        else if (v instanceof androidx.core.widget.NestedScrollView nsv) nsv.setBackgroundColor(color);
        else if (v instanceof IupEditText tv) tv.setBackgroundColor(color);
    }

    @Keep
    public static void setBorderless(View v)
    {
        IupEditText tv = resolve(v);
        if (tv == null) return;
        if (v instanceof TextInputLayout til)
        {
            til.setBoxBackgroundMode(TextInputLayout.BOX_BACKGROUND_NONE);
            til.setHintEnabled(false);
            til.setPadding(0, 0, 0, 0);
        }
        tv.setBackground(null);
        tv.setPadding(0, 0, 0, 0);
        tv.setMinHeight(0);
        tv.setMinimumHeight(0);
    }


    @Keep
    public static void setText(final long ihandlePtr, View v, String text)
    {
        IupEditText tv = resolve(v);
        if (tv == null) return;
        /* skip filters + suppress VALUECHANGED_CB so programmatic writes don't re-enter ACTION */
        InputFilter[] filters = tv.getFilters();
        tv.setFilters(new InputFilter[0]);
        tv.suppressVC = true;
        tv.setText(text != null ? text : "");
        tv.suppressVC = false;
        tv.setFilters(filters);
    }



    @Keep
    public static String getText(final long ihandlePtr, View v)
    {
        IupEditText tv = resolve(v);
        if (tv == null) return "";
        Editable e = tv.getText();
        return e == null ? "" : e.toString();
    }


    @Keep
    public static int getCaretPos(View v)
    {
        IupEditText tv = resolve(v);
        if (tv == null) return 0;
        int p = tv.getSelectionStart();
        return Math.max(p, 0);
    }

    @Keep
    public static void setCaretPos(View v, int pos)
    {
        IupEditText tv = resolve(v);
        if (tv == null) return;
        Editable e = tv.getText();
        int len = e == null ? 0 : e.length();
        if (pos < 0) pos = 0;
        if (pos > len) pos = len;
        tv.setSelection(pos);
        if (tv.getLayout() != null)
        {
            try { tv.bringPointIntoView(pos); } catch (Throwable ignored) {}
        }
    }

    /* Packed (start << 32) | end; -1 if no selection. */
    @Keep
    public static long getSelectionRange(View v)
    {
        IupEditText tv = resolve(v);
        if (tv == null) return -1L;
        int s = tv.getSelectionStart(), e = tv.getSelectionEnd();
        if (s < 0 || e < 0 || s == e) return -1L;
        if (s > e) { int t = s; s = e; e = t; }
        return ((long)s << 32) | (e & 0xFFFFFFFFL);
    }

    /* end == -1 sentinel selects to end of text. */
    @Keep
    public static void setSelectionRange(View v, int start, int end)
    {
        IupEditText tv = resolve(v);
        if (tv == null) return;
        Editable e = tv.getText();
        int len = e == null ? 0 : e.length();
        if (end < 0) end = len;
        if (start < 0) start = 0;
        if (start > len) start = len;
        if (end > len) end = len;
        tv.setSelection(start, end);
        if (tv.getLayout() != null)
        {
            try { tv.bringPointIntoView(end); } catch (Throwable ignored) {}
        }
    }

    @Keep
    public static String getSelectedText(View v)
    {
        IupEditText tv = resolve(v);
        if (tv == null) return null;
        int s = tv.getSelectionStart(), en = tv.getSelectionEnd();
        if (s < 0 || en < 0 || s == en) return null;
        if (s > en) { int t = s; s = en; en = t; }
        Editable e = tv.getText();
        return e == null ? null : e.subSequence(s, en).toString();
    }

    /* Replace current selection; no-op if no selection (matches IUP SELECTEDTEXT). */
    @Keep
    public static void replaceSelection(View v, String s)
    {
        IupEditText tv = resolve(v);
        if (tv == null || s == null) return;
        Editable e = tv.getText();
        if (e == null) return;
        int a = tv.getSelectionStart(), b = tv.getSelectionEnd();
        if (a < 0 || b < 0 || a == b) return;
        if (a > b) { int t = a; a = b; b = t; }
        InputFilter[] saved = tv.getFilters();
        tv.setFilters(new InputFilter[0]);
        tv.suppressVC = true;
        e.replace(a, b, s);
        tv.suppressVC = false;
        tv.setFilters(saved);
    }

    /* INSERT: replace selection if any, else insert at caret. */
    @Keep
    public static void insertAtCaret(View v, String s)
    {
        IupEditText tv = resolve(v);
        if (tv == null || s == null) return;
        Editable e = tv.getText();
        if (e == null) return;
        int a = tv.getSelectionStart(), b = tv.getSelectionEnd();
        if (a < 0) a = 0;
        if (b < 0) b = a;
        if (a > b) { int t = a; a = b; b = t; }
        InputFilter[] saved = tv.getFilters();
        tv.setFilters(new InputFilter[0]);
        tv.suppressVC = true;
        if (a != b) e.replace(a, b, s);
        else e.insert(a, s);
        tv.suppressVC = false;
        tv.setFilters(saved);
    }

    /* op: 0=COPY, 1=CUT, 2=PASTE, 3=CLEAR, 4=UNDO, 5=REDO. */
    @Keep
    public static void clipboardOp(View v, int op)
    {
        IupEditText tv = resolve(v);
        if (tv == null) return;
        android.content.ClipboardManager cm = (android.content.ClipboardManager)
            tv.getContext().getSystemService(Context.CLIPBOARD_SERVICE);
        if (cm == null) return;
        Editable e = tv.getText();
        int a = tv.getSelectionStart(), b = tv.getSelectionEnd();
        if (a < 0) a = 0;
        if (b < 0) b = a;
        if (a > b) { int t = a; a = b; b = t; }
        InputFilter[] saved;
        switch (op)
        {
            case 0:
                if (a != b && e != null)
                    cm.setPrimaryClip(android.content.ClipData.newPlainText(null, e.subSequence(a, b)));
                break;
            case 1:
                if (a != b && e != null)
                {
                    cm.setPrimaryClip(android.content.ClipData.newPlainText(null, e.subSequence(a, b)));
                    saved = tv.getFilters();
                    tv.setFilters(new InputFilter[0]);
                    tv.suppressVC = true;
                    e.delete(a, b);
                    tv.suppressVC = false;
                    tv.setFilters(saved);
                }
                break;
            case 2:
                if (cm.hasPrimaryClip() && e != null)
                {
                    android.content.ClipData clip = cm.getPrimaryClip();
                    if (clip != null && clip.getItemCount() > 0)
                    {
                        CharSequence cs = clip.getItemAt(0).coerceToText(tv.getContext());
                        if (cs != null)
                        {
                            saved = tv.getFilters();
                            tv.setFilters(new InputFilter[0]);
                            tv.suppressVC = true;
                            if (a != b) e.replace(a, b, cs);
                            else e.insert(a, cs);
                            tv.suppressVC = false;
                            tv.setFilters(saved);
                        }
                    }
                }
                break;
            case 3:
                if (a != b && e != null)
                {
                    saved = tv.getFilters();
                    tv.setFilters(new InputFilter[0]);
                    tv.suppressVC = true;
                    e.delete(a, b);
                    tv.suppressVC = false;
                    tv.setFilters(saved);
                }
                break;
            case 4:
                tv.onTextContextMenuItem(android.R.id.undo);
                break;
            case 5:
                tv.onTextContextMenuItem(android.R.id.redo);
                break;
        }
    }

    @Keep
    public static int linColToPos(View v, int lin, int col)
    {
        IupEditText tv = resolve(v);
        if (tv == null) return 0;
        Editable buf = tv.getText();
        if (buf == null) return 0;
        return clamp(linColToPos(buf, lin, col), 0, buf.length());
    }

    @Keep
    public static int getCharCount(View v)
    {
        IupEditText tv = resolve(v);
        if (tv == null) return 0;
        Editable e = tv.getText();
        return e == null ? 0 : e.length();
    }

    /* Logical line count: number of '\n' + 1; matches gtk_text_buffer_get_line_count. */
    @Keep
    public static int getLineCount(View v)
    {
        IupEditText tv = resolve(v);
        if (tv == null) return 1;
        Editable e = tv.getText();
        if (e == null) return 1;
        int count = 1, len = e.length();
        for (int i = 0; i < len; i++) if (e.charAt(i) == '\n') count++;
        return count;
    }

    /* Text on the line containing the caret, no trailing newline. */
    @Keep
    public static String getLineValue(View v)
    {
        IupEditText tv = resolve(v);
        if (tv == null) return "";
        Editable e = tv.getText();
        if (e == null) return "";
        int caret = tv.getSelectionStart();
        if (caret < 0) caret = 0;
        int len = e.length();
        if (caret > len) caret = len;
        int s = caret;
        while (s > 0 && e.charAt(s - 1) != '\n') s--;
        int en = caret;
        while (en < len && e.charAt(en) != '\n') en++;
        return e.subSequence(s, en).toString();
    }

    /* Scroll without moving caret; bringPointIntoView propagates up to NestedScrollView. */
    @Keep
    public static void scrollToOffset(View v, int pos)
    {
        IupEditText tv = resolve(v);
        if (tv == null) return;
        Editable e = tv.getText();
        int len = e == null ? 0 : e.length();
        if (pos < 0) pos = 0;
        if (pos > len) pos = len;
        if (tv.getLayout() != null)
        {
            try { tv.bringPointIntoView(pos); } catch (Throwable ignored) {}
        }
    }

    /* al: 0=ALEFT, 1=ACENTER, 2=ARIGHT. */
    @Keep
    public static void setAlignment(View v, int al)
    {
        IupEditText tv = resolve(v);
        if (tv == null) return;
        int g;
        switch (al)
        {
            case 1:  g = Gravity.CENTER_HORIZONTAL; break;
            case 2:  g = Gravity.END; break;
            default: g = Gravity.START; break;
        }
        tv.setGravity(g | Gravity.CENTER_VERTICAL);
    }

    /* mode: 0=none, 1=LOWERCASE, 2=UPPERCASE, 3=NUMBER. */
    @Keep
    public static void setFilterMode(View v, int mode)
    {
        IupEditText tv = resolve(v);
        if (tv == null) return;
        tv.filterMode = mode;
    }

    @Keep
    public static void setOverwrite(View v, boolean enable)
    {
        IupEditText tv = resolve(v);
        if (tv == null) return;
        tv.overwrite = enable;
    }

    @Keep
    public static void setPadding(View v, int h, int vp)
    {
        IupEditText tv = resolve(v);
        if (tv == null) return;
        tv.setPadding(h, vp, h, vp);
    }

    @Keep
    public static boolean getOverwrite(View v)
    {
        IupEditText tv = resolve(v);
        return tv != null && tv.overwrite;
    }

    /* Bit 0 = horizontal scrollable, Bit 1 = vertical scrollable, both queried via TextView/NestedScrollView. */
    @Keep
    public static int getScrollVisibleBits(View v)
    {
        int bits = 0;
        if (v instanceof NestedScrollView nsv)
        {
            if (nsv.canScrollVertically(-1) || nsv.canScrollVertically(1)) bits |= 2;
            if (nsv.canScrollHorizontally(-1) || nsv.canScrollHorizontally(1)) bits |= 1;
        }
        IupEditText tv = resolve(v);
        if (tv != null)
        {
            if (tv.canScrollHorizontally(-1) || tv.canScrollHorizontally(1)) bits |= 1;
        }
        return bits;
    }

    private static CharSequence applyFilterMode(CharSequence s, int mode)
    {
        StringBuilder sb = new StringBuilder(s.length());
        for (int i = 0; i < s.length(); i++)
        {
            char c = s.charAt(i);
            switch (mode)
            {
                case 1: sb.append(Character.toLowerCase(c)); break;
                case 2: sb.append(Character.toUpperCase(c)); break;
                case 3: if (c >= '0' && c <= '9') sb.append(c); break;
                default: sb.append(c);
            }
        }
        return sb;
    }

    /* Packed (lin << 32) | col, both 1-based. */
    @Keep
    public static long posToLinCol(View v, int pos)
    {
        IupEditText tv = resolve(v);
        long oneOne = (1L << 32) | 1L;
        if (tv == null) return oneOne;
        Editable buf = tv.getText();
        if (buf == null) return oneOne;
        int len = buf.length();
        if (pos < 0) pos = 0;
        if (pos > len) pos = len;
        int line = 1, col = 1;
        for (int i = 0; i < pos; i++)
        {
            if (buf.charAt(i) == '\n') { line++; col = 1; }
            else col++;
        }
        return ((long)line << 32) | (col & 0xFFFFFFFFL);
    }

    @Keep
    public static void appendText(final long ihandlePtr, View v, String text, boolean multilineNewline, boolean scrollEnd)
    {
        IupEditText tv = resolve(v);
        if (tv == null || text == null) return;
        Editable buf = tv.getText();
        if (buf == null) return;
        InputFilter[] filters = tv.getFilters();
        tv.setFilters(new InputFilter[0]);
        tv.suppressVC = true;
        if (multilineNewline && buf.length() > 0) buf.append('\n');
        buf.append(text);
        tv.suppressVC = false;
        tv.setFilters(filters);
        if (scrollEnd)
        {
            tv.setSelection(buf.length());
            if (v instanceof NestedScrollView nsv)
                nsv.post(() -> nsv.fullScroll(View.FOCUS_DOWN));
        }
    }

    /* TIL hint doubles as the floating label; multi-line/spin keep the hint on the EditText. */
    @Keep
    public static void setCueBanner(final long ihandlePtr, View v, String text)
    {
        if (v instanceof TextInputLayout til)
        {
            if (text == null || text.isEmpty())
            {
                til.setHintEnabled(false);
                til.setHint(null);
            }
            else
            {
                til.setHintEnabled(true);
                til.setHint(text);
            }
            return;
        }
        IupEditText tv = resolve(v);
        if (tv != null) tv.setHint(text);
    }


    @Keep
    public static void setPassword(final long ihandlePtr, View v, boolean password)
    {
        IupEditText tv = resolve(v);
        if (tv == null) return;
        int type = tv.getInputType();
        if (password)
            type = (type & ~InputType.TYPE_MASK_VARIATION) | InputType.TYPE_TEXT_VARIATION_PASSWORD;
        else
            type = (type & ~InputType.TYPE_MASK_VARIATION) | InputType.TYPE_TEXT_VARIATION_NORMAL;
        tv.setInputType(type);
    }

    @Keep
    public static void setMaxChars(final long ihandlePtr, View v, int nc)
    {
        IupEditText tv = resolve(v);
        if (tv == null) return;
        tv.maxChars = nc;
        rebuildFilters(tv);
    }

    @Keep
    public static void setReadOnlyMultiLine(final long ihandlePtr, View v, boolean readOnly)
    {
        IupEditText tv = resolve(v);
        if (tv != null) setReadOnly(tv, readOnly, InputType.TYPE_CLASS_TEXT
            | InputType.TYPE_TEXT_VARIATION_NORMAL
            | InputType.TYPE_TEXT_FLAG_MULTI_LINE);
    }

    @Keep
    public static void setReadOnlySingleLine(final long ihandlePtr, View v, boolean readOnly)
    {
        IupEditText tv = resolve(v);
        if (tv != null) setReadOnly(tv, readOnly, InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_NORMAL);
    }

    @Keep
    public static boolean getReadOnlyMultiLine(final long ihandlePtr, View v)
    {
        IupEditText tv = resolve(v);
        return tv != null && tv.getKeyListener() == null;
    }

    @Keep
    public static boolean getReadOnlySingleLine(final long ihandlePtr, View v)
    {
        IupEditText tv = resolve(v);
        return tv != null && tv.getKeyListener() == null;
    }

    private static void setReadOnly(IupEditText tv, boolean readOnly, int editInputType)
    {
        /* setInputType(TYPE_NULL) would wipe MULTI_LINE; swap the KeyListener instead. */
        tv.setTextIsSelectable(true);
        tv.setKeyListener(readOnly ? null : tv.savedKeyListener);
        if (!readOnly) tv.setInputType(editInputType);
    }


    private static void spinStep(IupEditText tv, long ihandlePtr, int direction)
    {
        int current = 0;
        Editable e = tv.getText();
        if (e != null)
        {
            try { current = Integer.parseInt(e.toString().trim()); }
            catch (NumberFormatException ignored) {}
        }
        int next = dispatchSpin(ihandlePtr, current, direction);
        if (next == Integer.MIN_VALUE) return;  /* callback rejected */
        IupTextHelper.setText(ihandlePtr, tv, Integer.toString(next));
    }

    @Keep
    public static void setSpinValue(final long ihandlePtr, View v, int value)
    {
        IupEditText tv = resolve(v);
        if (tv == null) return;
        setText(ihandlePtr, tv, Integer.toString(value));
    }

    @Keep
    public static int getSpinValue(final long ihandlePtr, View v)
    {
        IupEditText tv = resolve(v);
        if (tv == null) return 0;
        Editable e = tv.getText();
        if (e == null) return 0;
        try { return Integer.parseInt(e.toString().trim()); }
        catch (NumberFormatException ignored) { return 0; }
    }


    private static void installCallbacks(final IupEditText tv)
    {
        tv.setFilters(new InputFilter[] { makeActionFilter(tv) });

        tv.addTextChangedListener(new TextWatcher()
        {
            int overwriteDelPos = -1;

            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after)
            {
                overwriteDelPos = -1;
                if (tv.overwrite && !tv.suppressVC && count == 0 && after == 1
                    && start < s.length() && s.charAt(start) != '\n')
                {
                    overwriteDelPos = start + 1;
                }
            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {}

            @Override
            public void afterTextChanged(Editable s)
            {
                if (tv.suppressVC) return;
                if (overwriteDelPos >= 0 && overwriteDelPos < s.length() && s.charAt(overwriteDelPos) != '\n')
                {
                    int del = overwriteDelPos;
                    overwriteDelPos = -1;
                    tv.suppressVC = true;
                    s.delete(del, del + 1);
                    tv.suppressVC = false;
                }
                dispatchValueChanged(tv.ihandlePtr);
            }
        });

        tv.setOnKeyListener((v, keyCode, ev) -> {
            if (ev.getAction() != KeyEvent.ACTION_DOWN) return false;
            return dispatchKAny(tv.ihandlePtr, keyCode, ev.getMetaState());
        });

        /* IME Done/Go/Send/Next -> AKEYCODE_ENTER so DEFAULTENTER fires; multiline uses native Enter */
        tv.setOnEditorActionListener((v, actionId, ev) -> {
            switch (actionId)
            {
                case android.view.inputmethod.EditorInfo.IME_ACTION_DONE:
                case android.view.inputmethod.EditorInfo.IME_ACTION_GO:
                case android.view.inputmethod.EditorInfo.IME_ACTION_SEND:
                case android.view.inputmethod.EditorInfo.IME_ACTION_NEXT:
                case android.view.inputmethod.EditorInfo.IME_ACTION_PREVIOUS:
                case android.view.inputmethod.EditorInfo.IME_ACTION_SEARCH:
                    return dispatchKAny(tv.ihandlePtr, KeyEvent.KEYCODE_ENTER, 0);
                default:
                    return false;
            }
        });
    }

    private static InputFilter makeActionFilter(final IupEditText tv)
    {
        return (source, start, end, dest, dstart, dend) -> {
            if (tv.filterMode != 0)
            {
                CharSequence raw = source.subSequence(start, end);
                CharSequence filtered = applyFilterMode(raw, tv.filterMode);
                if (!filtered.toString().contentEquals(raw))
                {
                    source = filtered;
                    start = 0;
                    end = filtered.length();
                }
            }
            if (tv.maxChars > 0)
            {
                int currentLen = dest.length() - (dend - dstart);
                int insertLen = end - start;
                if (currentLen + insertLen > tv.maxChars)
                {
                    int allowed = tv.maxChars - currentLen;
                    if (allowed <= 0) return "";
                    end = start + allowed;
                    source = source.subSequence(start, end);
                    start = 0;
                }
            }

            String insert = source.subSequence(start, end).toString();
            int key = 0;
            if (insert.length() == 1) key = insert.charAt(0);

            String newValue = new StringBuilder(dest)
                .replace(dstart, dend, insert)
                .toString();

            int result = dispatchAction(tv.ihandlePtr, key, newValue);
            if (result == 0) return "";
            if (result > 0 && key != 0)
                return String.valueOf((char) result);
            return null;
        };
    }

    private static void rebuildFilters(IupEditText tv)
    {
        tv.setFilters(new InputFilter[] { makeActionFilter(tv) });
    }


    public static native int dispatchAction(long ihandlePtr, int key, String newValue);
    public static native void dispatchValueChanged(long ihandlePtr);
    public static native boolean dispatchKAny(long ihandlePtr, int androidKeyCode, int metaState);
    public static native void dispatchCaret(long ihandlePtr, int pos);

    /** Applies SPININC/MIN/MAX/WRAP, fires SPIN_CB, returns new value or Integer.MIN_VALUE if rejected. */
    public static native int dispatchSpin(long ihandlePtr, int current, int direction);

    /* Cached EditText default padding so iupdrvTextAddBorders can account for Android's inherent compound padding. */
    private static int sEditTextPadH = -1;
    private static int sEditTextPadV = -1;

    @Keep
    public static int getEditTextBorderH()
    {
        if (sEditTextPadH < 0) measureEditTextPadding();
        return sEditTextPadH;
    }

    @Keep
    public static int getEditTextBorderV()
    {
        if (sEditTextPadV < 0) measureEditTextPadding();
        return sEditTextPadV;
    }

    private static synchronized void measureEditTextPadding()
    {
        if (sEditTextPadH >= 0) return;
        android.content.Context ctx = new android.view.ContextThemeWrapper(
            IupCommon.getContextThemeWrapper(), android.R.style.Theme_DeviceDefault);
        AppCompatEditText probe = new AppCompatEditText(ctx);
        sEditTextPadH = probe.getCompoundPaddingLeft() + probe.getCompoundPaddingRight();
        sEditTextPadV = probe.getCompoundPaddingTop() + probe.getCompoundPaddingBottom();
    }

    @Keep public static int getTextInputLayoutBorderH() { return (int)(32f * IupCommon.getDisplayDensity()); }
    @Keep public static int getTextInputLayoutBorderV() { return (int)(20f * IupCommon.getDisplayDensity()); }

    /** Fires LINK_CB(url); no return value (IUP LINK_CB ignores result for the span path). */
    public static native void dispatchLinkClick(long ihandlePtr, String url);

    /* ClickableSpan that routes taps to the IUP LINK_CB callback. */
    private static final class IupLinkSpan extends ClickableSpan
    {
        final long ihandlePtr;
        final String url;
        IupLinkSpan(long ih, String u) { ihandlePtr = ih; url = u; }
        @Override public void onClick(@NonNull View widget) { dispatchLinkClick(ihandlePtr, url); }
        @Override public void updateDrawState(TextPaint ds)
        {
            ds.setUnderlineText(true);
            ds.setColor(0xFF2196F3);
        }
    }


    /* Sentinel for "attribute not set" in applyFormatTag primitive params. */
    static final int FMT_UNSET = Integer.MIN_VALUE;

    /* One FormatTag, parses attrs to spans (FMT_UNSET = unset). */
    @Keep
    public static void applyFormatTag(View v,
        String selection, String selectionPos,
        int bold, int italic, int underline, int strike,
        int superscript, int subscript,
        int fgColor, int bgColor,
        int fontSize,
        float fontScale,
        String fontFamily,
        int alignment, int indent, int lineSpacing,
        long ihandlePtr, String link,
        String numbering, String numberingStyle)
    {
        IupEditText tv = resolve(v);
        if (tv == null) return;
        Editable buf = tv.getText();
        if (buf == null) return;

        int[] range = resolveRange(buf, selection, selectionPos);
        int start = range[0], end = range[1];
        if (end <= start) return;

        applyCharSpans(buf, start, end, bold, italic, underline, strike,
            superscript, subscript, fgColor, bgColor, fontSize, fontFamily);

        if (fontScale > 0f && fontScale != 1.0f)
            buf.setSpan(new RelativeSizeSpan(fontScale), start, end, Spanned.SPAN_INCLUSIVE_EXCLUSIVE);

        if (link != null && !link.isEmpty())
            buf.setSpan(new IupLinkSpan(ihandlePtr, link), start, end, Spanned.SPAN_INCLUSIVE_EXCLUSIVE);

        if (alignment != FMT_UNSET || indent != FMT_UNSET || lineSpacing != FMT_UNSET)
        {
            int pstart = start;
            while (pstart > 0 && buf.charAt(pstart - 1) != '\n') pstart--;
            int pend = end;
            int len = buf.length();
            while (pend < len && buf.charAt(pend - 1) != '\n') pend++;
            applyParaSpans(buf, pstart, pend, alignment, indent, lineSpacing);
        }

        if (numbering != null && !numbering.equalsIgnoreCase("NONE"))
        {
            int ordinal = 1;
            int pos = start;
            int finish = end;
            while (pos < finish && pos <= buf.length())
            {
                int paraStart = pos;
                while (paraStart > 0 && buf.charAt(paraStart - 1) != '\n') paraStart--;
                int paraEnd = pos;
                int len = buf.length();
                while (paraEnd < len && buf.charAt(paraEnd) != '\n') paraEnd++;
                String marker = buildNumberingMarker(numbering, numberingStyle, ordinal);
                if (!marker.isEmpty())
                {
                    buf.insert(paraStart, marker);
                    finish += marker.length();
                    pos = paraEnd + marker.length() + 1;
                }
                else pos = paraEnd + 1;
                ordinal++;
            }
        }
    }

    private static String romanNumeral(int n, boolean upper)
    {
        if (n < 1) return "";
        int[] vals = {1000,900,500,400,100,90,50,40,10,9,5,4,1};
        String[] syms = {"M","CM","D","CD","C","XC","L","XL","X","IX","V","IV","I"};
        StringBuilder s = new StringBuilder();
        for (int i = 0; i < vals.length; i++)
            while (n >= vals[i]) { s.append(syms[i]); n -= vals[i]; }
        return upper ? s.toString() : s.toString().toLowerCase();
    }

    private static String buildNumberingMarker(String numbering, String style, int ordinal)
    {
        if (numbering.equalsIgnoreCase("BULLET")) return "• ";
        String core;
        if      (numbering.equalsIgnoreCase("ARABIC"))   core = Integer.toString(ordinal);
        else if (numbering.equalsIgnoreCase("LCLETTER")) core = String.valueOf((char)('a' + (ordinal - 1) % 26));
        else if (numbering.equalsIgnoreCase("UCLETTER")) core = String.valueOf((char)('A' + (ordinal - 1) % 26));
        else if (numbering.equalsIgnoreCase("LCROMAN"))  core = romanNumeral(ordinal, false);
        else if (numbering.equalsIgnoreCase("UCROMAN"))  core = romanNumeral(ordinal, true);
        else return "";
        if (style != null)
        {
            if (style.equalsIgnoreCase("NONUMBER"))         return "";
            if (style.equalsIgnoreCase("RIGHTPARENTHESIS")) return core + ") ";
            if (style.equalsIgnoreCase("PARENTHESES"))      return "(" + core + ") ";
            if (style.equalsIgnoreCase("PERIOD"))           return core + ". ";
        }
        return core + " ";
    }

    private static int[] resolveRange(Editable buf, String selection, String selectionPos)
    {
        int len = buf.length();
        if (selectionPos != null)
        {
            int[] p = parseTwoInts(selectionPos, ':');
            if (p != null) return new int[]{clamp(p[0], 0, len), clamp(p[1], 0, len)};
        }
        if (selection != null)
        {
            int colon = selection.indexOf(':');
            if (colon < 0) return new int[]{0, 0};
            int[] a = parseTwoInts(selection.substring(0, colon), ',');
            int[] b = parseTwoInts(selection.substring(colon + 1), ',');
            if (a == null || b == null) return new int[]{0, 0};
            return new int[]{clamp(linColToPos(buf, a[0], a[1]), 0, len),
                             clamp(linColToPos(buf, b[0], b[1]), 0, len)};
        }
        return new int[]{0, 0};
    }

    private static int linColToPos(CharSequence s, int lin, int col)
    {
        int line = 1, pos = 0, len = s.length();
        while (pos < len && line < lin)
        {
            if (s.charAt(pos) == '\n') line++;
            pos++;
        }
        int c = 1;
        while (pos < len && c < col && s.charAt(pos) != '\n')
        {
            pos++;
            c++;
        }
        return pos;
    }

    private static int[] parseTwoInts(String s, char sep)
    {
        if (s == null) return null;
        int k = s.indexOf(sep);
        if (k < 0) return null;
        try
        {
            return new int[]{
                Integer.parseInt(s.substring(0, k).trim()),
                Integer.parseInt(s.substring(k + 1).trim())};
        }
        catch (NumberFormatException e) { return null; }
    }

    private static int clamp(int v, int lo, int hi)
    {
        return v < lo ? lo : (Math.min(v, hi));
    }

    private static void applyCharSpans(Editable buf, int start, int end,
        int bold, int italic, int underline, int strike,
        int superscript, int subscript,
        int fgColor, int bgColor, int fontSize, String fontFamily)
    {
        final int mask = Spanned.SPAN_INCLUSIVE_EXCLUSIVE;

        if (bold != FMT_UNSET || italic != FMT_UNSET)
        {
            int style = 0;
            if (bold == 1) style |= Typeface.BOLD;
            if (italic == 1) style |= Typeface.ITALIC;
            buf.setSpan(new StyleSpan(style), start, end, mask);
        }
        if (underline == 1) buf.setSpan(new UnderlineSpan(), start, end, mask);
        if (strike == 1) buf.setSpan(new StrikethroughSpan(), start, end, mask);
        if (superscript == 1) buf.setSpan(new SuperscriptSpan(), start, end, mask);
        if (subscript == 1) buf.setSpan(new SubscriptSpan(), start, end, mask);

        if (fgColor != FMT_UNSET) buf.setSpan(new ForegroundColorSpan(fgColor), start, end, mask);
        if (bgColor != FMT_UNSET) buf.setSpan(new BackgroundColorSpan(bgColor), start, end, mask);

        if (fontSize != 0)
        {
            float density = IupCommon.getDisplayDensity();
            int px = fontSize > 0
                ? Math.round(fontSize * (96f / 72f) * density)  /* points to hw px */
                : Math.round(-fontSize * density);               /* logical px to hw px */
            if (px > 0) buf.setSpan(new AbsoluteSizeSpan(px), start, end, mask);
        }

        if (fontFamily != null && !fontFamily.isEmpty())
            buf.setSpan(new TypefaceSpan(fontFamily), start, end, mask);
    }

    private static void applyParaSpans(Editable buf, int start, int end,
        int alignment, int indent, int lineSpacing)
    {
        final int mask = Spanned.SPAN_PARAGRAPH;

        if (alignment != FMT_UNSET)
        {
            Layout.Alignment al = switch (alignment) {
                case 1 -> Layout.Alignment.ALIGN_CENTER;
                case 2 -> Layout.Alignment.ALIGN_OPPOSITE;
                default -> Layout.Alignment.ALIGN_NORMAL;
            };
            buf.setSpan(new AlignmentSpan.Standard(al), start, end, mask);
        }
        if (indent != FMT_UNSET)
        {
            int px = Math.round(indent * IupCommon.getDisplayDensity());
            buf.setSpan(new LeadingMarginSpan.Standard(px), start, end, mask);
        }
        /* LINESPACING: LineHeightSpan.Standard is API 29+; skip for min-API compatibility. */
    }

    /* Replaces selection with inline image (width/height in IUP logical px; 0 = natural). */
    @Keep
    public static void applyImageSpan(View v, String selection, String selectionPos,
        Bitmap bitmap, int width, int height)
    {
        IupEditText tv = resolve(v);
        if (tv == null || bitmap == null) return;
        Editable buf = tv.getText();
        if (buf == null) return;

        int[] range = resolveRange(buf, selection, selectionPos);
        int start = range[0], end = range[1];

        float density = IupCommon.getDisplayDensity();
        int logicalW = width > 0 ? width : bitmap.getWidth();
        int logicalH = height > 0 ? height : bitmap.getHeight();
        int hwW = Math.round(logicalW * density);
        int hwH = Math.round(logicalH * density);

        Drawable d = new BitmapDrawable(IupCommon.getContextThemeWrapper().getResources(), bitmap);
        d.setBounds(0, 0, hwW, hwH);

        InputFilter[] saved = tv.getFilters();
        tv.setFilters(new InputFilter[0]);
        if (end > start) buf.delete(start, end);
        buf.insert(start, "￼");
        buf.setSpan(new ImageSpan(d, ImageSpan.ALIGN_BASELINE),
            start, start + 1, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        tv.setFilters(saved);
    }

    /* REMOVEFORMATTING=ALL: strip char/para/image spans. */
    @Keep
    public static void removeFormattingAll(View v)
    {
        IupEditText tv = resolve(v);
        if (tv == null) return;
        Editable buf = tv.getText();
        if (buf == null) return;
        Object[] spans = buf.getSpans(0, buf.length(), Object.class);
        for (Object s : spans)
        {
            if (s instanceof StyleSpan || s instanceof UnderlineSpan ||
                s instanceof StrikethroughSpan || s instanceof SuperscriptSpan ||
                s instanceof SubscriptSpan || s instanceof ForegroundColorSpan ||
                s instanceof BackgroundColorSpan || s instanceof AbsoluteSizeSpan ||
                s instanceof TypefaceSpan || s instanceof AlignmentSpan ||
                s instanceof LeadingMarginSpan || s instanceof ImageSpan)
                buf.removeSpan(s);
        }
    }

    @Keep
    public static Object beginFormatBulk(View v)
    {
        IupEditText tv = resolve(v);
        if (tv == null) return null;
        InputFilter[] saved = tv.getFilters();
        tv.setFilters(new InputFilter[0]);
        return saved;
    }

    @Keep
    public static void endFormatBulk(View v, Object state)
    {
        IupEditText tv = resolve(v);
        if (tv == null || !(state instanceof InputFilter[])) return;
        tv.setFilters((InputFilter[])state);
    }
}
