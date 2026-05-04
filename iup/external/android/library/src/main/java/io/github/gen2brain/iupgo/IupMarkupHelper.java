package io.github.gen2brain.iupgo;

import android.graphics.Color;
import android.graphics.Typeface;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.style.AbsoluteSizeSpan;
import android.text.style.BackgroundColorSpan;
import android.text.style.ForegroundColorSpan;
import android.text.style.RelativeSizeSpan;
import android.text.style.StrikethroughSpan;
import android.text.style.StyleSpan;
import android.text.style.SubscriptSpan;
import android.text.style.SuperscriptSpan;
import android.text.style.TypefaceSpan;
import android.text.style.UnderlineSpan;
import android.util.TypedValue;

import java.util.ArrayDeque;
import java.util.Deque;
import java.util.HashMap;
import java.util.Map;


/* Pango-style parser for MARKUP=YES: b/i/u/s/strike/del/tt/code/sub/sup/small/big, span with
   foreground/background/font_family/font_size/font_weight/font_style/underline/strikethrough,
   plus &lt;/&gt;/&amp;/&quot;/&apos; and numeric &#NN;/&#xHH; entities. */
final class IupMarkupHelper
{
    private IupMarkupHelper() {}

    static CharSequence parse(String src)
    {
        SpannableStringBuilder out = new SpannableStringBuilder();
        Deque<Frame> stack = new ArrayDeque<>();
        int i = 0, n = src.length();
        while (i < n)
        {
            char c = src.charAt(i);
            if (c == '<')
            {
                int gt = src.indexOf('>', i + 1);
                if (gt < 0) { out.append(c); i++; continue; }
                String tagBody = src.substring(i + 1, gt).trim();
                i = gt + 1;
                if (tagBody.isEmpty()) continue;
                if (tagBody.charAt(0) == '/')
                {
                    String name = tagBody.substring(1).trim().toLowerCase();
                    closeTag(out, stack, name);
                }
                else
                {
                    boolean selfClosing = tagBody.endsWith("/");
                    if (selfClosing) tagBody = tagBody.substring(0, tagBody.length() - 1).trim();
                    String name; Map<String,String> attrs;
                    int sp = firstWhitespace(tagBody);
                    if (sp < 0) { name = tagBody.toLowerCase(); attrs = null; }
                    else        { name = tagBody.substring(0, sp).toLowerCase(); attrs = parseAttrs(tagBody.substring(sp + 1)); }
                    Frame f = new Frame();
                    f.name = name;
                    f.attrs = attrs;
                    f.start = out.length();
                    stack.push(f);
                    if (selfClosing) closeTag(out, stack, name);
                }
            }
            else if (c == '&')
            {
                int sc = src.indexOf(';', i + 1);
                if (sc > 0 && sc - i <= 10)
                {
                    String ent = src.substring(i + 1, sc);
                    String dec = decodeEntity(ent);
                    if (dec != null) { out.append(dec); i = sc + 1; continue; }
                }
                out.append(c);
                i++;
            }
            else
            {
                out.append(c);
                i++;
            }
        }
        /* Auto-close any leftover tags so unbalanced markup still renders. */
        while (!stack.isEmpty())
            applyFrame(out, stack.pop(), out.length());
        return out;
    }
    private static int firstWhitespace(String s)
    {
        for (int i = 0; i < s.length(); i++)
            if (Character.isWhitespace(s.charAt(i))) return i;
        return -1;
    }
    private static void closeTag(SpannableStringBuilder out, Deque<Frame> stack, String name)
    {
        /* tolerate out-of-order nesting like Pango: pop until match, auto-close intermediates */
        Deque<Frame> reopen = new ArrayDeque<>();
        Frame match = null;
        while (!stack.isEmpty())
        {
            Frame f = stack.pop();
            if (f.name.equals(name)) { match = f; break; }
            reopen.push(f);
        }
        if (match != null)
            applyFrame(out, match, out.length());
        while (!reopen.isEmpty())
            stack.push(reopen.pop());
    }
    private static void applyFrame(SpannableStringBuilder out, Frame f, int end)
    {
        int start = f.start;
        if (start >= end) return;
        String n = f.name;
        if (n.equals("b") || n.equals("strong"))             out.setSpan(new StyleSpan(Typeface.BOLD), start, end, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        else if (n.equals("i") || n.equals("em"))            out.setSpan(new StyleSpan(Typeface.ITALIC), start, end, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        else if (n.equals("u"))                              out.setSpan(new UnderlineSpan(), start, end, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        else if (n.equals("s") || n.equals("strike") || n.equals("del"))
                                                             out.setSpan(new StrikethroughSpan(), start, end, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        else if (n.equals("tt") || n.equals("code"))         out.setSpan(new TypefaceSpan("monospace"), start, end, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        else if (n.equals("sub"))                          { out.setSpan(new SubscriptSpan(),  start, end, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                                                             out.setSpan(new RelativeSizeSpan(0.75f), start, end, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE); }
        else if (n.equals("sup"))                          { out.setSpan(new SuperscriptSpan(), start, end, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                                                             out.setSpan(new RelativeSizeSpan(0.75f), start, end, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE); }
        else if (n.equals("small"))                          out.setSpan(new RelativeSizeSpan(0.83f), start, end, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        else if (n.equals("big"))                            out.setSpan(new RelativeSizeSpan(1.2f),  start, end, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        else if (n.equals("span") && f.attrs != null)        applySpanAttrs(out, start, end, f.attrs);
    }
    private static void applySpanAttrs(SpannableStringBuilder out, int start, int end, Map<String,String> a)
    {
        String fg = pick(a, "foreground", "fgcolor", "color");
        if (fg != null) {
            int col = parseColor(fg);
            if (col != 0) out.setSpan(new ForegroundColorSpan(col), start, end, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        }
        String bg = pick(a, "background", "bgcolor");
        if (bg != null) {
            int col = parseColor(bg);
            if (col != 0) out.setSpan(new BackgroundColorSpan(col), start, end, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        }
        String fam = pick(a, "font_family", "face");
        if (fam != null) out.setSpan(new TypefaceSpan(fam), start, end, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);

        String sz = pick(a, "font_size", "size");
        if (sz != null) {
            Float rel = relativeSize(sz);
            if (rel != null)
                out.setSpan(new RelativeSizeSpan(rel), start, end, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
            else {
                int px = absoluteSizePx(sz);
                if (px > 0) out.setSpan(new AbsoluteSizeSpan(px), start, end, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
            }
        }

        String w = pick(a, "font_weight", "weight");
        if (w != null && isBoldWeight(w)) out.setSpan(new StyleSpan(Typeface.BOLD), start, end, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);

        String st = pick(a, "font_style", "style");
        if (st != null) {
            String s = st.toLowerCase();
            if (s.equals("italic") || s.equals("oblique"))
                out.setSpan(new StyleSpan(Typeface.ITALIC), start, end, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        }

        String u = a.get("underline");
        if (u != null) {
            String s = u.toLowerCase();
            if (!s.equals("none") && !s.equals("false") && !s.equals("0"))
                out.setSpan(new UnderlineSpan(), start, end, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        }
        String stk = a.get("strikethrough");
        if (stk != null) {
            String s = stk.toLowerCase();
            if (s.equals("true") || s.equals("yes") || s.equals("1"))
                out.setSpan(new StrikethroughSpan(), start, end, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        }
    }
    private static String pick(Map<String,String> a, String... keys)
    {
        for (String k : keys) { String v = a.get(k); if (v != null) return v; }
        return null;
    }

    /* Pango "size" values: smaller, larger, x-small..xx-large -> RelativeSizeSpan. */
    private static Float relativeSize(String s)
    {
        switch (s.toLowerCase()) {
            case "xx-small": return 0.58f;
            case "x-small":  return 0.69f;
            case "small":    return 0.83f;
            case "medium":   return 1.0f;
            case "large":    return 1.2f;
            case "x-large":  return 1.44f;
            case "xx-large": return 1.73f;
            case "smaller":  return 0.83f;
            case "larger":   return 1.2f;
            default:         return null;
        }
    }

    /* Pango font_size: 1/1024 pt or "Npt"; we also accept plain ints as pt */
    private static int absoluteSizePx(String s)
    {
        s = s.trim().toLowerCase();
        if (s.endsWith("pt")) s = s.substring(0, s.length() - 2).trim();
        if (s.endsWith("px")) {
            try { return Math.max(1, Integer.parseInt(s.substring(0, s.length() - 2).trim())); }
            catch (NumberFormatException e) { return 0; }
        }
        try {
            float pt = Float.parseFloat(s);
            if (pt > 1024) pt = pt / 1024f;  /* Pango's 1/1024 pt encoding */
            float px = TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_PT, pt,
                IupCommon.getContextThemeWrapper().getResources().getDisplayMetrics());
            return Math.max(1, (int)Math.ceil(px));
        } catch (NumberFormatException e) { return 0; }
    }
    private static boolean isBoldWeight(String w)
    {
        String s = w.toLowerCase();
        if (s.equals("bold") || s.equals("bolder") || s.equals("heavy") || s.equals("ultrabold")) return true;
        try { return Integer.parseInt(s) >= 600; } catch (NumberFormatException e) { return false; }
    }
    private static int parseColor(String s)
    {
        try { return Color.parseColor(s.trim()); } catch (Exception e) { return 0; }
    }
    private static Map<String,String> parseAttrs(String s)
    {
        Map<String,String> map = new HashMap<>();
        int i = 0, n = s.length();
        while (i < n)
        {
            while (i < n && Character.isWhitespace(s.charAt(i))) i++;
            int keyStart = i;
            while (i < n && s.charAt(i) != '=' && !Character.isWhitespace(s.charAt(i))) i++;
            if (i == keyStart) break;
            String key = s.substring(keyStart, i).toLowerCase();
            while (i < n && Character.isWhitespace(s.charAt(i))) i++;
            String val = "";
            if (i < n && s.charAt(i) == '=') {
                i++;
                while (i < n && Character.isWhitespace(s.charAt(i))) i++;
                if (i < n && (s.charAt(i) == '"' || s.charAt(i) == '\'')) {
                    char q = s.charAt(i); i++;
                    int vs = i;
                    while (i < n && s.charAt(i) != q) i++;
                    val = s.substring(vs, i);
                    if (i < n) i++;
                } else {
                    int vs = i;
                    while (i < n && !Character.isWhitespace(s.charAt(i))) i++;
                    val = s.substring(vs, i);
                }
            }
            map.put(key, val);
        }
        return map;
    }
    private static String decodeEntity(String name)
    {
        if (name.isEmpty()) return null;
        switch (name) {
            case "lt":   return "<";
            case "gt":   return ">";
            case "amp":  return "&";
            case "quot": return "\"";
            case "apos": return "'";
            case "nbsp": return " ";
        }
        if (name.charAt(0) == '#') {
            try {
                int cp = (name.length() > 1 && (name.charAt(1) == 'x' || name.charAt(1) == 'X'))
                    ? Integer.parseInt(name.substring(2), 16)
                    : Integer.parseInt(name.substring(1));
                return new String(Character.toChars(cp));
            } catch (Exception e) { return null; }
        }
        return null;
    }
    private static final class Frame
    {
        String name;
        Map<String,String> attrs;
        int start;
    }
}
