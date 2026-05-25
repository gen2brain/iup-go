package io.github.gen2brain.iupgo;

import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.text.InputType;
import android.util.SparseArray;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.appcompat.view.ContextThemeWrapper;

import com.google.android.material.color.MaterialColors;
import com.google.android.material.textfield.MaterialAutoCompleteTextView;
import com.google.android.material.textfield.TextInputLayout;

import java.util.ArrayList;


public final class IupListHelper
{
    private IupListHelper() {}


    /* Fixed row height so VISIBLELINES*row renders exactly, no partial tail. */
    private static final float ROW_VERT_PAD_DP = 6.0f;
    private static final float ROW_HORIZ_PAD_DP = 16.0f;
    private static int s_rowHeightPx = -1;

    @Keep
    public static int getPreferredRowHeightPx()
    {
        if (s_rowHeightPx >= 0) return s_rowHeightPx;
        android.content.Context ctx = IupCommon.getContextThemeWrapper();
        float density = IupCommon.getDisplayDensity();
        TextView tv = new TextView(ctx);
        android.graphics.Paint.FontMetrics fm = tv.getPaint().getFontMetrics();
        int textH = (int)Math.ceil(fm.descent - fm.ascent);
        int padV = (int)(ROW_VERT_PAD_DP * density);
        s_rowHeightPx = textH + 2 * padV;
        if (s_rowHeightPx <= 0)
            s_rowHeightPx = (int)(32 * density);
        return s_rowHeightPx;
    }

    private static int s_editBoxHeightPx = -1;

    @Keep
    public static int getEditBoxHeightPx()
    {
        if (s_editBoxHeightPx >= 0) return s_editBoxHeightPx;
        android.widget.EditText e = new android.widget.EditText(IupCommon.getContextThemeWrapper());
        e.setSingleLine(true);
        int unspec = View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED);
        e.measure(unspec, unspec);
        s_editBoxHeightPx = e.getMeasuredHeight();
        return s_editBoxHeightPx;
    }

    @Keep public static int getDropdownBorderH() { return (int)(80f * IupCommon.getDisplayDensity()); }
    @Keep public static int getDropdownBorderV() { return (int)(20f * IupCommon.getDisplayDensity()); }

    static void installListDropTracker(final IupListView lv)
    {
        lv.setOnDragListener((v, ev) -> {
            int action = ev.getAction();
            switch (action)
            {
                case android.view.DragEvent.ACTION_DRAG_LOCATION:
                case android.view.DragEvent.ACTION_DRAG_ENTERED:
                {
                    int idx = lv.pointToPosition((int)ev.getX(), (int)ev.getY());
                    if (lv.dropIndicatorIdx != idx)
                    {
                        lv.dropIndicatorIdx = idx;
                        lv.invalidate();
                    }
                    break;
                }
                case android.view.DragEvent.ACTION_DRAG_EXITED:
                case android.view.DragEvent.ACTION_DRAG_ENDED:
                case android.view.DragEvent.ACTION_DROP:
                    if (lv.dropIndicatorIdx != -1)
                    {
                        lv.dropIndicatorIdx = -1;
                        lv.invalidate();
                    }
                    break;
            }
            return IupDragDropHelper.handleDragEvent(v, ev);
        });
    }

    public static final class IupListView extends android.widget.ListView
    {
        public int dropIndicatorIdx = -1;

        public IupListView(android.content.Context ctx) { super(ctx); }

        @Override
        @android.annotation.SuppressLint("ClickableViewAccessibility")
        public boolean onTouchEvent(android.view.MotionEvent ev)
        {
            if (ev.getActionMasked() == android.view.MotionEvent.ACTION_DOWN && getParent() != null)
                getParent().requestDisallowInterceptTouchEvent(true);
            return super.onTouchEvent(ev);
        }
        @Override public boolean performClick() { return super.performClick(); }

        @Override
        protected void dispatchDraw(android.graphics.Canvas canvas)
        {
            super.dispatchDraw(canvas);
            if (dropIndicatorIdx < 0) return;
            int firstVisible = getFirstVisiblePosition();
            int childIdx = dropIndicatorIdx - firstVisible;
            if (childIdx < 0 || childIdx >= getChildCount()) return;
            android.view.View child = getChildAt(childIdx);
            if (child == null) return;
            float density = IupCommon.getDisplayDensity();
            android.graphics.Paint p = new android.graphics.Paint();
            p.setColor(IupCommon.paletteTxtHlBg);
            p.setStrokeWidth(Math.max(2f, 2f * density));
            float y = child.getTop();
            canvas.drawLine(0, y, getWidth(), y, p);
        }
    }

    /* row-image render height; driver uses it to widen natural_w by render_w - maximg_w */
    @Keep
    public static int getRowImageHeightPx()
    {
        int rowH = getPreferredRowHeightPx();
        int padV = (int)(ROW_VERT_PAD_DP * IupCommon.getDisplayDensity());
        return Math.max(1, rowH - 2 * padV);
    }

    /* Compound-drawable padding (gap between image and text) so the driver can include it in natural_w. */
    @Keep public static int getRowIconPaddingPx() { return (int)(8 * IupCommon.getDisplayDensity()); }

    /* Mirror the selected dropdown item's image as the MACTV's leading compound drawable. */
    private static void updateDropdownLeadingImage(MaterialAutoCompleteTextView actv, int position)
    {
        Drawable d = (actv.getAdapter() instanceof ThemedAdapter ta) ? ta.images.get(position) : null;
        if (d != null) actv.setCompoundDrawablePadding((int)(8 * IupCommon.getDisplayDensity()));
        actv.setCompoundDrawablesRelative(d, null, null, null);
    }


    /* Applies FGCOLOR, FONT and per-item IMAGE; popup theme handled by Android. */
    private static final class ThemedAdapter extends ArrayAdapter<String>
    {
        int textColor;
        boolean userTextColor;
        android.graphics.Typeface typeface;
        int sizeUnit;
        float size;
        final SparseArray<Drawable> images = new SparseArray<>();
        /* Source bitmaps mirror images for IMAGENATIVEHANDLE / DRAGDROPLIST. */
        final SparseArray<Bitmap> bitmaps = new SparseArray<>();
        int iconPaddingPx;
        int spacingPx;
        /* VIRTUALMODE: getCount returns virtualCount, getItem fetches text via VALUE_CB. */
        long virtualIhandle;
        int virtualCount = -1;

        ThemedAdapter(Context ctx, int rowLayout, int textColor)
        {
            super(ctx, rowLayout, new ArrayList<>());
            this.textColor = textColor;
            this.iconPaddingPx = (int)(8 * IupCommon.getDisplayDensity());
        }

        @Override
        public int getCount()
        {
            return virtualCount >= 0 ? virtualCount : super.getCount();
        }

        @Override
        public String getItem(int pos)
        {
            if (virtualCount < 0) return super.getItem(pos);
            String s = dispatchListValueCb(virtualIhandle, pos);
            return s != null ? s : "";
        }

        private void decorate(TextView tv, int pos, boolean applyTextColor)
        {
            if (applyTextColor)
            {
                int[][] states = { new int[]{ android.R.attr.state_activated }, new int[]{} };
                int[] colors = { IupCommon.paletteTxtHlFg, textColor };
                tv.setTextColor(new android.content.res.ColorStateList(states, colors));
            }
            if (typeface != null) tv.setTypeface(typeface);
            if (sizeUnit != 0 && size > 0) tv.setTextSize(sizeUnit, size);
            /* Override framework 48dp minHeight; compact text + padding. */
            int padV = (int)(ROW_VERT_PAD_DP * IupCommon.getDisplayDensity()) + spacingPx;
            int padH = (int)(ROW_HORIZ_PAD_DP * IupCommon.getDisplayDensity()) + spacingPx;
            tv.setMinHeight(0);
            tv.setMinimumHeight(0);
            /* explicit padding so iupdrvListAddBorders matches; theme listPreferredItemPaddingStart varies */
            tv.setPadding(padH, padV, padH, padV);
            int rowH = getPreferredRowHeightPx() + 2 * spacingPx;
            tv.setLayoutParams(new android.widget.AbsListView.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, rowH));
            /* Activated bg from Material You can be pink; use IUP TXTHLCOLOR. */
            android.graphics.drawable.StateListDrawable sld = new android.graphics.drawable.StateListDrawable();
            sld.addState(new int[]{ android.R.attr.state_activated },
                new android.graphics.drawable.ColorDrawable(IupCommon.paletteTxtHlBg));
            sld.addState(new int[]{},
                new android.graphics.drawable.ColorDrawable(Color.TRANSPARENT));
            tv.setBackground(sld);
            Drawable d = images.get(pos);
            if (d == null && virtualCount >= 0)
            {
                Bitmap bmp = dispatchListImageCb(virtualIhandle, pos);
                if (bmp != null) d = sizedDrawable(tv.getResources(), bmp);
            }
            if (d != null) tv.setCompoundDrawablePadding(iconPaddingPx);
            tv.setCompoundDrawablesRelative(d, null, null, null);
        }

        @NonNull
        @Override
        public View getView(int pos, View convertView, @NonNull ViewGroup parent)
        {
            View v = super.getView(pos, convertView, parent);
            if (v instanceof TextView) decorate((TextView)v, pos, true);
            return v;
        }

        @Override
        public View getDropDownView(int pos, View convertView, @NonNull ViewGroup parent)
        {
            View v = super.getDropDownView(pos, convertView, parent);
            /* Paint explicitly; rows inherit textColorPrimary from the popup's frozen context otherwise. */
            if (v instanceof TextView) decorate((TextView)v, pos, true);
            return v;
        }
    }


    @Keep
    public static View createListView(final long ihandlePtr, final boolean multiple, final boolean editbox, final boolean scrollbar)
    {
        ContextThemeWrapper ctx = IupCommon.getContextThemeWrapper();
        /* Activated layout: plain text rows, background highlight for selection. */
        int rowLayout = android.R.layout.simple_list_item_activated_1;

        ThemedAdapter adapter = new ThemedAdapter(ctx, rowLayout, IupCommon.paletteTxtFg);
        final IupListView lv = new IupListView(ctx);
        installListDropTracker(lv);
        lv.setAdapter(adapter);
        lv.setChoiceMode(multiple ? ListView.CHOICE_MODE_MULTIPLE : ListView.CHOICE_MODE_SINGLE);
        /* Divider pushes last row past allocated height; drop it. */
        lv.setDivider(null);
        lv.setDividerHeight(0);
        lv.setVerticalScrollBarEnabled(scrollbar);
        lv.setBackgroundColor(IupCommon.paletteTxtBg);
        sThemableLists.put(lv, Boolean.TRUE);

        if (!editbox)
        {
            lv.setOnItemClickListener((parent, view, position, id) -> {
                String text = String.valueOf(parent.getItemAtPosition(position));
                int state = multiple ? (lv.isItemChecked(position) ? 1 : 0) : 1;
                dispatchAction(ihandlePtr, text, position + 1, state);
            });
            return lv;
        }

        /* EDITBOX=YES: stacked EditText + ListView, VALUE is entry text; uses IupEditText for MASK/NC/FILTER */
        final android.widget.LinearLayout box = new android.widget.LinearLayout(ctx);
        box.setOrientation(android.widget.LinearLayout.VERTICAL);

        final View edit = IupTextHelper.createSingleLineText(ihandlePtr);
        box.addView(edit, new android.widget.LinearLayout.LayoutParams(
            android.view.ViewGroup.LayoutParams.MATCH_PARENT,
            android.view.ViewGroup.LayoutParams.WRAP_CONTENT));

        android.widget.LinearLayout.LayoutParams lvLp = new android.widget.LinearLayout.LayoutParams(
            android.view.ViewGroup.LayoutParams.MATCH_PARENT, 0, 1.0f);
        box.addView(lv, lvLp);

        lv.setOnItemClickListener((parent, view, position, id) -> {
            String text = String.valueOf(parent.getItemAtPosition(position));
            /* auto-fill is a selection, not editing; suppress VALUECHANGED_CB */
            IupTextHelper.setText(0L, edit, text);
            int state = multiple ? (lv.isItemChecked(position) ? 1 : 0) : 1;
            dispatchAction(ihandlePtr, text, position + 1, state);
        });

        box.setTag(lv);
        return box;
    }

    @Keep
    public static void setEditBoxText(View widget, String text)
    {
        android.widget.EditText e = editBoxOf(widget);
        if (e == null) return;
        if (text == null) text = "";
        /* MACTV.setText filters by default; (text, false) skips it. */
        if (e instanceof MaterialAutoCompleteTextView m) { m.setText(text, false); return; }
        IupTextHelper.setText(0L, e, text);
    }

    @Keep
    public static String getEditBoxText(View widget)
    {
        android.widget.EditText e = editBoxOf(widget);
        if (e == null) return "";
        CharSequence s = e.getText();
        return s == null ? "" : s.toString();
    }

    private static final java.util.WeakHashMap<TextInputLayout, Boolean> sThemableDropdowns = new java.util.WeakHashMap<>();
    private static final java.util.WeakHashMap<ListView, Boolean> sThemableLists = new java.util.WeakHashMap<>();

    static {
        IupTheme.register(IupListHelper::refreshAllDropdowns);
        IupTheme.register(IupListHelper::refreshAllLists);
    }

    private static void refreshAllLists()
    {
        for (ListView lv : new java.util.ArrayList<>(sThemableLists.keySet()))
        {
            if (lv != null) lv.setBackgroundColor(IupCommon.paletteTxtBg);
        }
    }

    /* Theme attrs cache at construction; refresh stroke + popup bg + adapter colors on flip. */
    private static void refreshAllDropdowns()
    {
        Context ctx = IupCommon.getThemeContext();
        int outline      = MaterialColors.getColor(ctx, com.google.android.material.R.attr.colorOutline, Color.GRAY);
        int primary      = MaterialColors.getColor(ctx, androidx.appcompat.R.attr.colorPrimary, Color.BLUE);
        int onSurfaceVar = MaterialColors.getColor(ctx, com.google.android.material.R.attr.colorOnSurfaceVariant, Color.DKGRAY);
        int popupBg      = IupCommon.resolveThemeColorByName("colorSurface", IupCommon.paletteMenuBg);

        int[][] states = { { android.R.attr.state_focused }, { android.R.attr.state_hovered }, {} };
        int[]   colors = { primary, primary, outline };
        ColorStateList strokeCsl = new ColorStateList(states, colors);

        for (TextInputLayout til : new java.util.ArrayList<>(sThemableDropdowns.keySet()))
        {
            if (til == null) continue;
            til.setBoxStrokeColorStateList(strokeCsl);
            til.setDefaultHintTextColor(ColorStateList.valueOf(onSurfaceVar));
            til.setHintTextColor(ColorStateList.valueOf(primary));
            til.setBoxBackgroundColor(IupCommon.paletteTxtBg);
            Object tag = til.getTag();
            if (tag instanceof MaterialAutoCompleteTextView m)
            {
                m.setTextColor(IupCommon.paletteTxtFg);
                m.setDropDownBackgroundDrawable(new ColorDrawable(popupBg));
                if (m.getAdapter() instanceof ThemedAdapter ta)
                {
                    if (!ta.userTextColor) ta.textColor = IupCommon.paletteTxtFg;
                    ta.notifyDataSetChanged();
                }
            }
        }
    }

    /* tracks 1-based selection; AutoCompleteTextView isn't an AdapterView */
    @android.annotation.SuppressLint("ViewConstructor")
    static final class IupDropdownView extends MaterialAutoCompleteTextView
    {
        long ihandlePtr;
        int currentPosition;
        IupDropdownView(Context ctx, long ih) { super(ctx); ihandlePtr = ih; }
    }

    @Keep
    public static View createDropdownView(final long ihandlePtr, final boolean editbox)
    {
        ContextThemeWrapper ctx = IupCommon.getContextThemeWrapper();
        TextInputLayout til = new TextInputLayout(ctx, null,
            com.google.android.material.R.attr.textInputOutlinedExposedDropdownMenuStyle);
        til.setHintEnabled(false);
        til.setBoxCollapsedPaddingTop(0);

        final IupDropdownView actv = new IupDropdownView(til.getContext(), ihandlePtr);
        actv.setSingleLine(true);
        actv.setMinHeight(0);
        actv.setMinimumHeight(0);
        int padV = (int)(8f * IupCommon.getDisplayDensity());
        actv.setPadding(actv.getPaddingLeft(), padV, actv.getPaddingRight(), padV);
        if (!editbox)
        {
            actv.setKeyListener(null);
            actv.setInputType(InputType.TYPE_NULL);
        }
        else
        {
            actv.setThreshold(1);
        }

        ThemedAdapter adapter = new ThemedAdapter(til.getContext(),
            android.R.layout.simple_spinner_dropdown_item, IupCommon.paletteTxtFg);
        actv.setAdapter(adapter);

        actv.setOnItemClickListener((parent, view, position, id) -> {
            String text = String.valueOf(parent.getItemAtPosition(position));
            actv.currentPosition = position + 1;
            /* setText(.., false) bypasses the autocomplete filter so editable mode does not re-open. */
            actv.setText(text, false);
            updateDropdownLeadingImage(actv, position);
            /* Defer: synchronous Activity launch races ListPopupWindow's Surface teardown, MTE-traps libutils Looper. */
            actv.post(() -> dispatchAction(ihandlePtr, text, position + 1, 1));
        });

        til.addView(actv, new android.widget.LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        til.setTag(actv);
        sThemableDropdowns.put(til, Boolean.TRUE);
        return til;
    }


    public static void setListFont(View widget, android.graphics.Typeface face, int sizeUnit, float size)
    {
        ArrayAdapter<String> a = adapterOf(widget);
        if (!(a instanceof ThemedAdapter ta)) return;
        ta.typeface = face;
        ta.sizeUnit = sizeUnit;
        ta.size = size;
        ta.notifyDataSetChanged();
    }

    public static void setSpacing(View widget, int spacing)
    {
        ArrayAdapter<String> a = adapterOf(widget);
        if (!(a instanceof ThemedAdapter ta)) return;
        ta.spacingPx = (int)(spacing * IupCommon.getDisplayDensity());
        ta.notifyDataSetChanged();
    }

    public static void setVisibleItems(View widget, int count)
    {
        if (count <= 0) return;
        if (widget instanceof TextInputLayout til && til.getTag() instanceof MaterialAutoCompleteTextView m)
            m.setDropDownHeight(count * getPreferredRowHeightPx());
    }

    public static void setShowDropdown(View widget, boolean show)
    {
        if (widget instanceof TextInputLayout til && til.getTag() instanceof MaterialAutoCompleteTextView m)
        {
            if (show) m.showDropDown();
            else m.dismissDropDown();
        }
    }


    @SuppressWarnings("unchecked")
    private static ArrayAdapter<String> adapterOf(View widget)
    {
        if (widget instanceof ListView)
            return (ArrayAdapter<String>)((ListView)widget).getAdapter();
        if (widget instanceof TextInputLayout til)
        {
            Object tag = til.getTag();
            if (tag instanceof MaterialAutoCompleteTextView m)
                return (ArrayAdapter<String>)m.getAdapter();
        }
        /* EDITBOX wrapper: LinearLayout with ListView tagged. */
        if (widget instanceof android.widget.LinearLayout)
        {
            Object tag = widget.getTag();
            if (tag instanceof ListView)
                return (ArrayAdapter<String>)((ListView)tag).getAdapter();
        }
        return null;
    }

    @Keep
    public static int getCount(View widget)
    {
        ArrayAdapter<String> a = adapterOf(widget);
        return a == null ? 0 : a.getCount();
    }

    @Keep
    public static void appendItem(View widget, String text)
    {
        ArrayAdapter<String> a = adapterOf(widget);
        if (a != null) a.add(text);
    }

    @Keep
    public static void insertItem(View widget, int pos, String text)
    {
        ArrayAdapter<String> a = adapterOf(widget);
        if (a != null) a.insert(text, pos);
    }

    @Keep
    public static void removeItem(View widget, int pos)
    {
        ArrayAdapter<String> a = adapterOf(widget);
        if (a == null || pos < 0 || pos >= a.getCount()) return;
        a.remove(a.getItem(pos));
    }

    @Keep
    public static void removeAll(View widget)
    {
        ArrayAdapter<String> a = adapterOf(widget);
        if (a != null) a.clear();
    }

    @Keep
    public static String getItem(View widget, int pos)
    {
        ArrayAdapter<String> a = adapterOf(widget);
        if (a == null || pos < 0 || pos >= a.getCount()) return null;
        return a.getItem(pos);
    }


    /** Sets selection from a 1-based index. 0 clears selection. */
    @Keep
    public static void setSelection(View widget, int index1Based)
    {
        if (widget instanceof TextInputLayout til)
        {
            Object tag = til.getTag();
            if (tag instanceof IupDropdownView actv)
            {
                int count = actv.getAdapter() != null ? actv.getAdapter().getCount() : 0;
                if (index1Based >= 1 && index1Based <= count)
                {
                    actv.setText(String.valueOf(actv.getAdapter().getItem(index1Based - 1)), false);
                    actv.currentPosition = index1Based;
                    updateDropdownLeadingImage(actv, index1Based - 1);
                }
                else
                {
                    actv.setText("", false);
                    actv.currentPosition = 0;
                    actv.setCompoundDrawablesRelative(null, null, null, null);
                }
            }
            return;
        }
        if (widget instanceof ListView lv)
        {
            lv.clearChoices();
            if (index1Based >= 1 && index1Based <= lv.getCount())
                lv.setItemChecked(index1Based - 1, true);
        }
    }

    /** Returns the 1-based index of the first selected item, or 0. */
    @Keep
    public static int getSelection(View widget)
    {
        if (widget instanceof TextInputLayout til)
        {
            Object tag = til.getTag();
            if (tag instanceof IupDropdownView actv) return actv.currentPosition;
            return 0;
        }
        if (widget instanceof ListView)
        {
            int p = ((ListView)widget).getCheckedItemPosition();
            return p == ListView.INVALID_POSITION ? 0 : p + 1;
        }
        return 0;
    }

    /** Toggles the AbsListView fast-scroll thumb; ignored if widget has no inner ListView. */
    @Keep
    public static void setFastScrollEnabled(View widget, boolean enabled)
    {
        ListView lv = listViewOf(widget);
        if (lv == null) return;
        lv.setFastScrollEnabled(enabled);
        lv.setFastScrollAlwaysVisible(enabled);
    }

    /** Scrolls a ListView so that the given 1-based item is at (or near) the top. */
    @Keep
    public static void setTopItem(View widget, int oneBased)
    {
        ListView lv = listViewOf(widget);
        if (lv == null || oneBased < 1) return;
        int max = lv.getCount();
        if (oneBased > max) return;
        lv.setSelectionFromTop(oneBased - 1, 0);
    }

    /* px x,y -> 0-based index, -1 if none; subtracts ListView offset for EDITBOX wrappers */
    @Keep
    public static int pointToPosition(View widget, int x, int y)
    {
        ListView lv = listViewOf(widget);
        if (lv == null) return -1;
        int dx = x, dy = y;
        if (widget != lv)
        {
            dx -= lv.getLeft();
            dy -= lv.getTop();
        }
        int pos = lv.pointToPosition(dx, dy);
        return (pos == AdapterView.INVALID_POSITION) ? -1 : pos;
    }

    /* Locates the inner ListView for plain or EDITBOX wrappers. */
    private static ListView listViewOf(View widget)
    {
        if (widget instanceof ListView) return (ListView)widget;
        if (widget instanceof android.widget.LinearLayout)
        {
            Object tag = widget.getTag();
            if (tag instanceof ListView) return (ListView)tag;
        }
        return null;
    }

    /* Editable target: TIL tag (dropdown MACTV or single-line TIET) or LinearLayout child. */
    private static android.widget.EditText editBoxOf(View widget)
    {
        if (widget instanceof TextInputLayout til)
        {
            Object tag = til.getTag();
            if (tag instanceof android.widget.EditText) return (android.widget.EditText) tag;
        }
        if (widget instanceof android.widget.LinearLayout box)
        {
            for (int i = 0; i < box.getChildCount(); i++)
            {
                View c = box.getChildAt(i);
                if (c instanceof android.widget.EditText) return (android.widget.EditText) c;
                if (c instanceof TextInputLayout til)
                {
                    Object tag = til.getTag();
                    if (tag instanceof android.widget.EditText) return (android.widget.EditText) tag;
                }
            }
        }
        return null;
    }

    /* end == -1 sentinel selects to end of text. */
    @Keep
    public static void setEditSelection(View widget, int start, int end)
    {
        android.widget.EditText e = editBoxOf(widget);
        if (e == null) return;
        CharSequence cs = e.getText();
        int len = cs == null ? 0 : cs.length();
        if (end < 0) end = len;
        if (start < 0) start = 0;
        if (start > len) start = len;
        if (end > len) end = len;
        e.setSelection(start, end);
    }

    /* Packed (start << 32) | end; -1 if no selection. */
    @Keep
    public static long getEditSelection(View widget)
    {
        android.widget.EditText e = editBoxOf(widget);
        if (e == null) return -1L;
        int s = e.getSelectionStart(), en = e.getSelectionEnd();
        if (s < 0 || en < 0 || s == en) return -1L;
        if (s > en) { int t = s; s = en; en = t; }
        return ((long)s << 32) | (en & 0xFFFFFFFFL);
    }

    /** Applies a "+-" style selection mask to a multi-select ListView. */
    @Keep
    public static void setMultipleSelection(View widget, String mask)
    {
        if (!(widget instanceof ListView lv) || mask == null) return;
        lv.clearChoices();
        int n = Math.min(mask.length(), lv.getCount());
        for (int i = 0; i < n; i++)
        {
            if (mask.charAt(i) == '+') lv.setItemChecked(i, true);
        }
    }

    @Keep
    public static String getMultipleSelection(View widget)
    {
        if (!(widget instanceof ListView lv)) return "";
        int n = lv.getCount();
        StringBuilder sb = new StringBuilder(n);
        for (int i = 0; i < n; i++)
        {
            sb.append(lv.isItemChecked(i) ? '+' : '-');
        }
        return sb.toString();
    }


    public static native void dispatchAction(long ihandlePtr, String text, int item, int state);

    /* VIRTUALMODE: adapter calls this for each visible row to fetch text via VALUE_CB. */
    public static native String dispatchListValueCb(long ihandlePtr, int pos);

    /* VIRTUALMODE: adapter resolves IMAGE_CB to a Bitmap (or null) for the row. */
    public static native Bitmap dispatchListImageCb(long ihandlePtr, int pos);

    /* VIRTUALMODE: C calls this from iupdrvListSetItemCount to resize the virtual list. */
    @Keep
    public static void setItemCount(View widget, long ihandlePtr, int count)
    {
        ArrayAdapter<String> a = adapterOf(widget);
        if (!(a instanceof ThemedAdapter ta)) return;
        ta.virtualIhandle = ihandlePtr;
        ta.virtualCount = Math.max(0, count);
        ta.notifyDataSetChanged();
    }


    @Keep
    public static void setTextColor(View widget, int r, int g, int b)
    {
        ArrayAdapter<String> a = adapterOf(widget);
        if (!(a instanceof ThemedAdapter ta)) return;
        ta.textColor = Color.rgb(r, g, b);
        ta.userTextColor = true;
        a.notifyDataSetChanged();
    }

    @Keep
    public static void setBgColor(View widget, int r, int g, int b)
    {
        int color = Color.rgb(r, g, b);
        if (widget instanceof TextInputLayout til) til.setBoxBackgroundColor(color);
        else if (widget instanceof ListView lv) lv.setBackgroundColor(color);
        else if (widget instanceof android.widget.LinearLayout box)
        {
            for (int i = 0; i < box.getChildCount(); i++)
            {
                android.view.View c = box.getChildAt(i);
                if (c instanceof TextInputLayout til2) til2.setBoxBackgroundColor(color);
                else if (c instanceof ListView lv2) lv2.setBackgroundColor(color);
            }
        }
    }

    /* fitImage=true scales to 32dp; else intrinsic size. Null clears. */
    @Keep
    public static void setItemImage(View widget, int pos, Bitmap bmp, boolean fitImage)
    {
        ArrayAdapter<String> a = adapterOf(widget);
        if (!(a instanceof ThemedAdapter ta)) return;
        if (bmp == null)
        {
            ta.images.remove(pos);
            ta.bitmaps.remove(pos);
        }
        else
        {
            ta.images.put(pos, sizedDrawable(widget.getResources(), bmp, fitImage));
            ta.bitmaps.put(pos, bmp);
        }
        if (widget instanceof TextInputLayout til && til.getTag() instanceof IupDropdownView actv
            && actv.currentPosition == pos + 1)
            updateDropdownLeadingImage(actv, pos);
        ta.notifyDataSetChanged();
    }

    private static BitmapDrawable sizedDrawable(android.content.res.Resources res, Bitmap bmp)
    {
        return sizedDrawable(res, bmp, true);
    }

    /* compoundDrawables only render when bounds are explicit; intrinsic without setBounds is 0. */
    private static BitmapDrawable sizedDrawable(android.content.res.Resources res, Bitmap bmp, boolean fitImage)
    {
        BitmapDrawable d = new BitmapDrawable(res, bmp);
        int dw, dh;
        if (fitImage)
        {
            /* cap to row text-content height so the image isn't clipped by row LayoutParams */
            int rowH = getPreferredRowHeightPx();
            int padV = (int)(ROW_VERT_PAD_DP * IupCommon.getDisplayDensity());
            int targetH = Math.max(1, rowH - 2 * padV);
            int sw = bmp.getWidth(), sh = bmp.getHeight();
            int max = Math.max(sw, sh);
            dw = sw * targetH / max;
            dh = sh * targetH / max;
        }
        else
        {
            dw = d.getIntrinsicWidth();
            dh = d.getIntrinsicHeight();
        }
        d.setBounds(0, 0, dw, dh);
        return d;
    }

    @Keep
    public static Bitmap getItemBitmap(View widget, int pos)
    {
        ArrayAdapter<String> a = adapterOf(widget);
        if (!(a instanceof ThemedAdapter ta)) return null;
        return ta.bitmaps.get(pos);
    }
}
