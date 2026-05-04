package io.github.gen2brain.iupgo;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.text.TextUtils;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.SparseArray;
import android.util.SparseBooleanArray;
import android.util.SparseIntArray;
import android.view.GestureDetector;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.widget.EditText;
import android.widget.HorizontalScrollView;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.view.ContextThemeWrapper;
import androidx.recyclerview.widget.DividerItemDecoration;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import java.util.ArrayList;
import java.util.HashMap;


/* notifyDataSetChanged on purpose: bulk C-side mutations make per-item notifications more expensive */
@android.annotation.SuppressLint("NotifyDataSetChanged")
public final class IupTableHelper
{
    private IupTableHelper() {}


    /* Programmatic-only; requires an Ihandle so the XML constructors are absent. */
    @android.annotation.SuppressLint("ViewConstructor")
    public static final class IupTableView extends LinearLayout
    {
        final long ihandlePtr;

        int numCol;
        int numLin;
        int[] colWidths;
        String[] colTitles;
        final ArrayList<String[]> rows = new ArrayList<>();

        int defaultColWidth;
        int rowHeightPx;
        int headerHeightPx;
        boolean showGrid = true;
        int gridColor;
        boolean[] colExplicit = new boolean[0];
        boolean stretchLast = true;
        int lastColAutoWidth;

        boolean showImage;
        boolean fitImage = true;
        boolean alternateColor;
        boolean hasEvenBg, hasOddBg;
        int evenBg, oddBg;
        int defaultFg;
        int defaultBg;
        int headerBg;
        int headerFg;
        int iconPaddingPx;
        float defaultCellTextSizePx;

        int focusLin = 1;
        int focusCol = 1;
        int rowSelectBg;
        int rowSelectFg;
        int focusCellBg;
        GradientDrawable focusCellDrawable;
        boolean focusRect = true;

        boolean editableAll;
        final SparseBooleanArray editableCol = new SparseBooleanArray();

        boolean sortable;
        int sortCol;
        boolean sortAsc = true;
        boolean userResize;
        boolean allowReorder;
        boolean virtualMode;

        final HashMap<Long, Integer> cellBg = new HashMap<>();
        final HashMap<Long, Integer> cellFg = new HashMap<>();
        final SparseIntArray colBg = new SparseIntArray();
        final SparseIntArray colFg = new SparseIntArray();
        final SparseIntArray rowBg = new SparseIntArray();
        final SparseIntArray rowFg = new SparseIntArray();
        final SparseArray<String> colAlign = new SparseArray<>();
        final HashMap<Long, Drawable> cellImage = new HashMap<>();
        final HashMap<Long, CellFont> cellFont = new HashMap<>();
        final SparseArray<CellFont> colFont = new SparseArray<>();
        final SparseArray<CellFont> rowFont = new SparseArray<>();

        SyncScrollView headerScroll;
        SyncScrollView bodyScroll;
        LinearLayout headerRow;
        RecyclerView recyclerView;
        RowAdapter adapter;

        boolean syncing;

        public IupTableView(Context ctx, long ih)
        {
            super(ctx);
            this.ihandlePtr = ih;
            setOrientation(VERTICAL);

            float density = IupCommon.getDisplayDensity();
            this.defaultColWidth = Math.round(100 * density);
            this.rowHeightPx = Math.round(32 * density);
            this.headerHeightPx = Math.round(36 * density);
            this.iconPaddingPx = Math.round(6 * density);
            applyThemeColors();
        }

        /* Read cell/header/grid/highlight defaults from the IUP palette. */
        void applyThemeColors()
        {
            this.defaultFg = IupCommon.paletteTxtFg;
            this.defaultBg = IupCommon.paletteTxtBg;
            this.gridColor = IupCommon.blendColor(IupCommon.paletteDlgBg, IupCommon.paletteDlgFg, 0.20f);
            this.headerBg  = IupCommon.blendColor(IupCommon.paletteDlgBg, IupCommon.paletteDlgFg, 0.10f);
            this.headerFg  = IupCommon.paletteDlgFg;
            this.rowSelectBg = IupCommon.paletteTxtHlBg;
            this.rowSelectFg = IupCommon.paletteTxtHlFg;
            this.focusCellBg = IupCommon.blendColor(IupCommon.paletteTxtHlBg, IupCommon.paletteDlgFg, 0.45f);
            this.focusCellDrawable = null;
            setBackgroundColor(defaultBg);
        }
    }

    /* DARKMODE transition: refresh header/grid/body so palette-derived colors land on existing views */
    @Keep
    public static void refreshTheme(View v)
    {
        if (!(v instanceof IupTableView t)) return;
        t.applyThemeColors();
        rebuildHeader(t);
        applyGridDecoration(t);
        t.adapter.notifyDataSetChanged();
    }


    static long cellKey(int lin, int col) { return ((long)lin << 32) | ((long)col & 0xFFFFFFFFL); }

    record CellFont(String family, int style, int sizeUnit, float size) {}

    static CellFont resolveFont(IupTableView t, int lin, int col)
    {
        CellFont v = t.cellFont.get(cellKey(lin, col));
        if (v != null) return v;
        v = t.colFont.get(col);
        if (v != null) return v;
        v = t.rowFont.get(lin);
        return v;
    }

    static void applyCellFont(IupTableView t, TextView tv, CellFont cf)
    {
        if (cf == null)
        {
            tv.setTypeface(Typeface.DEFAULT);
            if (t.defaultCellTextSizePx > 0)
                tv.setTextSize(android.util.TypedValue.COMPLEX_UNIT_PX, t.defaultCellTextSizePx);
            return;
        }
        Typeface base = (cf.family() != null && !cf.family().isEmpty())
            ? Typeface.create(cf.family(), cf.style())
            : Typeface.create((Typeface)null, cf.style());
        tv.setTypeface(base);
        if (cf.sizeUnit() != 0 && cf.size() > 0) tv.setTextSize(cf.sizeUnit(), cf.size());
    }

    static int resolveBg(IupTableView t, int lin, int col)
    {
        Integer v = t.cellBg.get(cellKey(lin, col));
        if (v != null) return v;
        int idx = t.colBg.indexOfKey(col);
        if (idx >= 0) return t.colBg.valueAt(idx);
        idx = t.rowBg.indexOfKey(lin);
        if (idx >= 0) return t.rowBg.valueAt(idx);
        if (t.alternateColor)
            return (lin % 2 == 0) ? (t.hasEvenBg ? t.evenBg : 0) : (t.hasOddBg ? t.oddBg : 0);
        return 0;
    }

    static int resolveFg(IupTableView t, int lin, int col)
    {
        Integer v = t.cellFg.get(cellKey(lin, col));
        if (v != null) return v;
        int idx = t.colFg.indexOfKey(col);
        if (idx >= 0) return t.colFg.valueAt(idx);
        idx = t.rowFg.indexOfKey(lin);
        if (idx >= 0) return t.rowFg.valueAt(idx);
        return t.defaultFg;
    }

    static Drawable focusCellDrawable(IupTableView t)
    {
        if (t.focusCellDrawable == null)
        {
            GradientDrawable gd = new GradientDrawable();
            gd.setColor(t.focusCellBg);
            t.focusCellDrawable = gd;
        }
        return t.focusCellDrawable;
    }

    static int gravityFromAlign(String align)
    {
        if (align == null) return Gravity.CENTER_VERTICAL | Gravity.START;
        if ("ARIGHT".equalsIgnoreCase(align)) return Gravity.CENTER_VERTICAL | Gravity.END;
        if ("ACENTER".equalsIgnoreCase(align)) return Gravity.CENTER_VERTICAL | Gravity.CENTER_HORIZONTAL;
        return Gravity.CENTER_VERTICAL | Gravity.START;
    }


    static final class SyncScrollView extends HorizontalScrollView
    {
        IupTableView owner;
        boolean isHeader;

        SyncScrollView(Context ctx) { super(ctx); }

        @Override
        protected void onScrollChanged(int l, int t, int oldl, int oldt)
        {
            super.onScrollChanged(l, t, oldl, oldt);
            if (owner == null || owner.syncing) return;
            SyncScrollView other = isHeader ? owner.bodyScroll : owner.headerScroll;
            if (other == null) return;
            owner.syncing = true;
            other.scrollTo(l, other.getScrollY());
            owner.syncing = false;
        }
    }


    static final class RowHolder extends RecyclerView.ViewHolder
    {
        final LinearLayout row;
        RowHolder(LinearLayout row) { super(row); this.row = row; }
    }

    static final class CellState { int lin, col; }


    static final class RowAdapter extends RecyclerView.Adapter<RowHolder>
    {
        final IupTableView table;

        RowAdapter(IupTableView table) { this.table = table; }

        @Override
        public int getItemCount() { return table.numLin; }

        @NonNull
        @Override
        public RowHolder onCreateViewHolder(ViewGroup parent, int viewType)
        {
            LinearLayout row = new LinearLayout(parent.getContext());
            row.setOrientation(LinearLayout.HORIZONTAL);
            row.setLayoutParams(new RecyclerView.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, table.rowHeightPx));
            for (int c = 0; c < table.numCol; c++)
                row.addView(makeEditableCell(parent.getContext(), table));
            return new RowHolder(row);
        }

        @Override
        public void onBindViewHolder(RowHolder holder, int position)
        {
            LinearLayout row = holder.row;
            String[] cells = (!table.virtualMode && position < table.rows.size()) ? table.rows.get(position) : null;
            final int lin = position + 1;

            while (row.getChildCount() < table.numCol)
                row.addView(makeEditableCell(row.getContext(), table));
            while (row.getChildCount() > table.numCol)
                row.removeViewAt(row.getChildCount() - 1);

            int[] widths = table.colWidths;
            int gridPx = table.showGrid ? 1 : 0;
            for (int c = 0; c < table.numCol; c++)
            {
                final int col = c + 1;
                TextView tv = (TextView) row.getChildAt(c);
                int w = (widths != null && c < widths.length && widths[c] > 0)
                    ? widths[c] : table.defaultColWidth;
                LinearLayout.LayoutParams lp = (LinearLayout.LayoutParams) tv.getLayoutParams();
                lp.width = w;
                lp.height = table.rowHeightPx;
                lp.rightMargin = (c < table.numCol - 1) ? gridPx : 0;
                tv.setLayoutParams(lp);
                String text;
                if (table.virtualMode)
                {
                    text = dispatchValueRequest(table.ihandlePtr, lin, col);
                    if (text == null) text = "";
                }
                else
                {
                    text = (cells != null && c < cells.length && cells[c] != null) ? cells[c] : "";
                }
                tv.setText(text);
                tv.setGravity(gravityFromAlign(table.colAlign.get(col)));

                boolean rowSelected = (lin == table.focusLin);
                boolean focusCell = rowSelected && (col == table.focusCol) && table.focusRect;
                if (focusCell)
                {
                    tv.setBackground(focusCellDrawable(table));
                }
                else if (rowSelected)
                {
                    tv.setBackgroundColor(table.rowSelectBg);
                }
                else
                {
                    int bg = resolveBg(table, lin, col);
                    if (bg != 0) tv.setBackgroundColor(bg);
                    else tv.setBackground(null);
                }

                tv.setTextColor(rowSelected ? table.rowSelectFg : resolveFg(table, lin, col));

                ensureDefaultCellTextSize(table, tv);
                applyCellFont(table, tv, resolveFont(table, lin, col));

                Drawable img = table.showImage ? table.cellImage.get(cellKey(lin, col)) : null;
                if (img != null) tv.setCompoundDrawablePadding(table.iconPaddingPx);
                tv.setCompoundDrawablesRelative(img, null, null, null);

                CellState state = (CellState) tv.getTag();
                state.lin = lin;
                state.col = col;
            }
        }
    }


    static void handleCellTap(IupTableView t, int lin, int col)
    {
        int oldLin = t.focusLin, oldCol = t.focusCol;
        boolean changed = (oldLin != lin || oldCol != col);
        if (changed)
        {
            t.focusLin = lin;
            t.focusCol = col;
            refreshRowStyle(t, oldLin);
            refreshRowStyle(t, lin);
        }
        final boolean changedFinal = changed;
        /* Defer; sync CLICK_CB that opens a modal would race the touch dispatcher Surface teardown. */
        t.recyclerView.post(() -> dispatchClick(t.ihandlePtr, lin, col, changedFinal ? 1 : 0));
    }

    static void refreshRowStyle(IupTableView t, int lin)
    {
        if (lin < 1 || lin > t.numLin) return;
        RecyclerView.ViewHolder vh = t.recyclerView.findViewHolderForAdapterPosition(lin - 1);
        if (!(vh instanceof RowHolder)) return;
        LinearLayout row = ((RowHolder) vh).row;
        for (int c = 0; c < row.getChildCount(); c++)
        {
            TextView tv = (TextView) row.getChildAt(c);
            applyCellStyle(t, tv, lin, c + 1);
        }
    }

    static void applyCellStyle(IupTableView t, TextView tv, int lin, int col)
    {
        boolean rowSelected = (lin == t.focusLin);
        boolean focusCell = rowSelected && (col == t.focusCol) && t.focusRect;
        if (focusCell)
        {
            tv.setBackground(focusCellDrawable(t));
        }
        else if (rowSelected)
        {
            tv.setBackgroundColor(t.rowSelectBg);
        }
        else
        {
            int bg = resolveBg(t, lin, col);
            if (bg != 0) tv.setBackgroundColor(bg);
            else tv.setBackground(null);
        }
        tv.setTextColor(rowSelected ? t.rowSelectFg : resolveFg(t, lin, col));
        ensureDefaultCellTextSize(t, tv);
        applyCellFont(t, tv, resolveFont(t, lin, col));
    }

    static boolean isCellEditable(IupTableView t, int col)
    {
        int idx = t.editableCol.indexOfKey(col);
        if (idx >= 0) return t.editableCol.valueAt(idx);
        return t.editableAll;
    }

    /* Cell taps/double-taps route through GestureDetector; no native click. */
    @android.annotation.SuppressLint("ClickableViewAccessibility")
    static TextView makeEditableCell(Context ctx, final IupTableView t)
    {
        final TextView tv = makeCell(ctx);
        final CellState state = new CellState();
        tv.setTag(state);
        final GestureDetector gd = new GestureDetector(ctx,
            new GestureDetector.SimpleOnGestureListener()
            {
                @Override public boolean onDown(@NonNull MotionEvent e) { return true; }
                @Override public boolean onSingleTapUp(@NonNull MotionEvent e)
                {
                    if (state.lin > 0 && state.col > 0) handleCellTap(t, state.lin, state.col);
                    return true;
                }
                @Override public boolean onDoubleTap(@NonNull MotionEvent e)
                {
                    /* defer; startEditCell + AlertDialog handlers all fire user CBs */
                    if (state.lin > 0 && state.col > 0 && isCellEditable(t, state.col))
                        tv.post(() -> startEditCell(t, state.lin, state.col));
                    return true;
                }
            });
        tv.setOnTouchListener((v, ev) -> gd.onTouchEvent(ev));
        return tv;
    }

    static void startEditCell(final IupTableView t, final int lin, final int col)
    {
        if (dispatchEditBegin(t.ihandlePtr, lin, col) != 0) return;

        String current = "";
        if (lin - 1 < t.rows.size())
        {
            String[] row = t.rows.get(lin - 1);
            if (row != null && col - 1 < row.length && row[col - 1] != null)
                current = row[col - 1];
        }

        android.app.Activity activity = IupCommon.getActivity(t);
        if (activity == null) activity = IupApplication.getIupApplication().getCurrentActivity();
        if (activity == null) return;

        final EditText input = new EditText(activity);
        input.setText(current);
        input.setSelection(current.length());
        input.setSingleLine(true);
        input.setImeOptions(EditorInfo.IME_ACTION_DONE);

        input.addTextChangedListener(new TextWatcher()
        {
            @Override public void beforeTextChanged(CharSequence s, int start, int count, int after) {}
            @Override public void afterTextChanged(Editable s) {}
            @Override public void onTextChanged(CharSequence s, int start, int before, int count)
            {
                dispatchEdition(t.ihandlePtr, lin, col, s.toString());
            }
        });

        String title = (col - 1 < t.colTitles.length && t.colTitles[col - 1] != null)
            ? t.colTitles[col - 1] : "";

        /* Defer dialog-button user CBs; sync dispatch races the AlertDialog Surface dismiss. */
        AlertDialog dlg = new AlertDialog.Builder(activity)
            .setTitle(title)
            .setView(input)
            .setPositiveButton(android.R.string.ok, (d, which) -> {
                final String newVal = input.getText().toString();
                t.recyclerView.post(() -> {
                    int rejected = dispatchEditEnd(t.ihandlePtr, lin, col, newVal, 1);
                    if (rejected == 0)
                    {
                        setCellValue(t, lin, col, newVal);
                        dispatchValueChanged(t.ihandlePtr, lin, col);
                    }
                });
            })
            .setNegativeButton(android.R.string.cancel, (d, which) -> {
                final String cur = input.getText().toString();
                t.recyclerView.post(() -> dispatchEditEnd(t.ihandlePtr, lin, col, cur, 0));
            })
            .setOnCancelListener(d -> {
                final String cur = input.getText().toString();
                t.recyclerView.post(() -> dispatchEditEnd(t.ihandlePtr, lin, col, cur, 0));
            })
            .create();
        dlg.show();
        input.requestFocus();
    }


    static TextView makeCell(Context ctx)
    {
        TextView tv = new TextView(ctx);
        tv.setLayoutParams(new LinearLayout.LayoutParams(0, 0));
        tv.setSingleLine(true);
        tv.setEllipsize(TextUtils.TruncateAt.END);
        tv.setGravity(Gravity.CENTER_VERTICAL | Gravity.START);
        int padH = Math.round(6 * IupCommon.getDisplayDensity());
        tv.setPadding(padH, 0, padH, 0);
        return tv;
    }

    static void ensureDefaultCellTextSize(IupTableView t, TextView tv)
    {
        if (t.defaultCellTextSizePx <= 0) t.defaultCellTextSizePx = tv.getTextSize();
    }

    static final class HeaderCell extends androidx.appcompat.widget.AppCompatTextView
    {
        boolean showGrabber;
        final Paint paint = new Paint();
        HeaderCell(Context ctx)
        {
            super(ctx);
            paint.setAntiAlias(true);
        }
        @Override protected void onDraw(Canvas canvas)
        {
            super.onDraw(canvas);
            if (!showGrabber) return;
            paint.setColor(IupCommon.blendColor(IupCommon.paletteDlgBg, IupCommon.paletteDlgFg, 0.35f));
            float w = getWidth(), h = getHeight();
            float stripe = 3f * IupCommon.getDisplayDensity();
            float margin = h * 0.15f;
            canvas.drawRect(w - stripe, margin, w - 1, h - margin, paint);
        }
        /* Accessibility hook; IUP callbacks dispatch via the touch listener. */
        @Override public boolean performClick() { return super.performClick(); }
    }

    /* onTouch handles resize/reorder; clicks go through setOnClickListener for performClick */
    @android.annotation.SuppressLint("ClickableViewAccessibility")
    static TextView makeHeaderCell(Context ctx, final IupTableView t)
    {
        final HeaderCell tv = new HeaderCell(ctx);
        tv.setLayoutParams(new LinearLayout.LayoutParams(0, 0));
        tv.setSingleLine(true);
        int padH = Math.round(6 * IupCommon.getDisplayDensity());
        tv.setPadding(padH, 0, padH, 0);
        tv.setTypeface(Typeface.DEFAULT_BOLD);
        tv.setBackgroundColor(t.headerBg);
        tv.setTextColor(t.headerFg);
        tv.setGravity(Gravity.CENTER_VERTICAL | Gravity.CENTER_HORIZONTAL);
        final int[] colRef = new int[]{0};
        tv.setTag(colRef);

        tv.setOnTouchListener(new View.OnTouchListener()
        {
            int mode;
            float downRawX;
            int initialWidth;
            Runnable longPressRunnable;
            final int grabberPx = Math.max(1, Math.round(28 * IupCommon.getDisplayDensity()));
            final int slopPx = Math.max(1, Math.round(8 * IupCommon.getDisplayDensity()));

            @Override public boolean onTouch(final View v, MotionEvent ev)
            {
                int a = ev.getActionMasked();
                int c = colRef[0];

                if (a == MotionEvent.ACTION_DOWN)
                {
                    mode = 0;
                    if (c < 1) return false;

                    if (t.userResize && ev.getX() >= v.getWidth() - grabberPx)
                    {
                        mode = 1;
                        downRawX = ev.getRawX();
                        initialWidth = v.getWidth();
                        v.getParent().requestDisallowInterceptTouchEvent(true);
                        return true;
                    }

                    if (t.allowReorder)
                    {
                        downRawX = ev.getRawX();
                        longPressRunnable = () -> {
                            longPressRunnable = null;
                            mode = 2;
                            v.setAlpha(0.5f);
                            v.getParent().requestDisallowInterceptTouchEvent(true);
                        };
                        v.postDelayed(longPressRunnable, 400);
                    }
                    return false;
                }

                if (a == MotionEvent.ACTION_MOVE)
                {
                    if (mode == 1)
                    {
                        if (c < 1 || c > t.numCol) return true;
                        int dx = Math.round(ev.getRawX() - downRawX);
                        int newW = Math.max(Math.round(30 * IupCommon.getDisplayDensity()), initialWidth + dx);
                        t.colWidths[c - 1] = newW;
                        rebuildHeader(t);
                        t.adapter.notifyDataSetChanged();
                        return true;
                    }
                    if (mode == 2) return true;
                    if (longPressRunnable != null && Math.abs(ev.getRawX() - downRawX) > slopPx)
                    {
                        v.removeCallbacks(longPressRunnable);
                        longPressRunnable = null;
                    }
                    return false;
                }

                if (a == MotionEvent.ACTION_UP || a == MotionEvent.ACTION_CANCEL)
                {
                    if (longPressRunnable != null)
                    {
                        v.removeCallbacks(longPressRunnable);
                        longPressRunnable = null;
                    }
                    if (mode == 1)
                    {
                        mode = 0;
                        v.getParent().requestDisallowInterceptTouchEvent(false);
                        return true;
                    }
                    if (mode == 2)
                    {
                        v.setAlpha(1.0f);
                        v.getParent().requestDisallowInterceptTouchEvent(false);
                        int xInRow = Math.round(ev.getX()) + v.getLeft();
                        int targetCol = computeTargetCol(t, xInRow);
                        mode = 0;
                        if (a == MotionEvent.ACTION_UP && c >= 1 && targetCol != c && targetCol >= 1 && targetCol <= t.numCol)
                        {
                            reorderColumn(t, c, targetCol);
                            dispatchReorder(t.ihandlePtr, c, targetCol);
                        }
                        return true;
                    }
                    return false;
                }
                return false;
            }
        });

        /* Defer; handleSortTap calls a user CB (dispatchSort) that may open a modal. */
        tv.setOnClickListener(v -> {
            int c = colRef[0];
            if (c > 0 && t.sortable) v.post(() -> handleSortTap(t, c));
        });
        return tv;
    }

    static void handleSortTap(IupTableView t, int col)
    {
        if (t.sortCol == col) t.sortAsc = !t.sortAsc;
        else { t.sortCol = col; t.sortAsc = true; }
        sortRowsByColumn(t, col, t.sortAsc);
        rebuildHeader(t);
        t.adapter.notifyDataSetChanged();
        dispatchSort(t.ihandlePtr, col, t.sortAsc ? 1 : 0);
    }

    static void sortRowsByColumn(IupTableView t, int col, final boolean asc)
    {
        final int ci = col - 1;
        if (ci < 0 || ci >= t.numCol) return;
        java.util.Collections.sort(t.rows, (a, b) -> {
            String sa = (a != null && ci < a.length && a[ci] != null) ? a[ci] : "";
            String sb = (b != null && ci < b.length && b[ci] != null) ? b[ci] : "";
            int cmp = naturalCompare(sa, sb);
            return asc ? cmp : -cmp;
        });
    }

    static int naturalCompare(String a, String b)
    {
        try { return Double.compare(Double.parseDouble(a), Double.parseDouble(b)); }
        catch (NumberFormatException ignored) {}
        return a.compareToIgnoreCase(b);
    }

    static int computeTargetCol(IupTableView t, int xInRow)
    {
        int acc = 0;
        for (int i = 0; i < t.numCol; i++)
        {
            int cw = t.colWidths[i] > 0 ? t.colWidths[i] : t.defaultColWidth;
            if (xInRow < acc + cw) return i + 1;
            acc += cw;
        }
        return t.numCol;
    }

    static int remapCol(int col, int fromCol, int toCol)
    {
        if (col == fromCol) return toCol;
        if (fromCol < toCol)
        {
            if (col > fromCol && col <= toCol) return col - 1;
        }
        else
        {
            if (col >= toCol && col < fromCol) return col + 1;
        }
        return col;
    }

    static <V> SparseArray<V> remapSparseArray(SparseArray<V> src, int fromCol, int toCol)
    {
        SparseArray<V> dst = new SparseArray<>();
        for (int i = 0; i < src.size(); i++)
            dst.put(remapCol(src.keyAt(i), fromCol, toCol), src.valueAt(i));
        return dst;
    }

    static SparseIntArray remapSparseIntArray(SparseIntArray src, int fromCol, int toCol)
    {
        SparseIntArray dst = new SparseIntArray(src.size());
        for (int i = 0; i < src.size(); i++)
            dst.put(remapCol(src.keyAt(i), fromCol, toCol), src.valueAt(i));
        return dst;
    }

    static void reorderColumn(IupTableView t, int fromCol, int toCol)
    {
        if (fromCol == toCol) return;
        int fromIdx = fromCol - 1;
        int toIdx = toCol - 1;
        if (fromIdx < 0 || fromIdx >= t.numCol || toIdx < 0 || toIdx >= t.numCol) return;

        int movedWidth = t.colWidths[fromIdx];
        String movedTitle = t.colTitles[fromIdx];
        int[] newWidths = new int[t.numCol];
        String[] newTitles = new String[t.numCol];
        int k = 0;
        for (int i = 0; i < t.numCol; i++)
        {
            if (i == fromIdx) continue;
            newWidths[k] = t.colWidths[i];
            newTitles[k] = t.colTitles[i];
            k++;
        }
        for (int i = t.numCol - 1; i > toIdx; i--)
        {
            newWidths[i] = newWidths[i - 1];
            newTitles[i] = newTitles[i - 1];
        }
        newWidths[toIdx] = movedWidth;
        newTitles[toIdx] = movedTitle;
        t.colWidths = newWidths;
        t.colTitles = newTitles;

        for (int r = 0; r < t.rows.size(); r++)
        {
            String[] row = t.rows.get(r);
            if (row == null) continue;
            String moved = row[fromIdx];
            String[] nr = new String[t.numCol];
            int kk = 0;
            for (int i = 0; i < t.numCol; i++)
            {
                if (i == fromIdx) continue;
                nr[kk++] = row[i];
            }
            for (int i = t.numCol - 1; i > toIdx; i--) nr[i] = nr[i - 1];
            nr[toIdx] = moved;
            t.rows.set(r, nr);
        }

        SparseIntArray nb = remapSparseIntArray(t.colBg, fromCol, toCol);
        t.colBg.clear();
        for (int i = 0; i < nb.size(); i++) t.colBg.put(nb.keyAt(i), nb.valueAt(i));

        SparseIntArray nf = remapSparseIntArray(t.colFg, fromCol, toCol);
        t.colFg.clear();
        for (int i = 0; i < nf.size(); i++) t.colFg.put(nf.keyAt(i), nf.valueAt(i));

        SparseArray<String> na = remapSparseArray(t.colAlign, fromCol, toCol);
        t.colAlign.clear();
        for (int i = 0; i < na.size(); i++) t.colAlign.put(na.keyAt(i), na.valueAt(i));

        SparseBooleanArray ne = new SparseBooleanArray();
        for (int i = 0; i < t.editableCol.size(); i++)
            ne.put(remapCol(t.editableCol.keyAt(i), fromCol, toCol), t.editableCol.valueAt(i));
        t.editableCol.clear();
        for (int i = 0; i < ne.size(); i++) t.editableCol.put(ne.keyAt(i), ne.valueAt(i));

        HashMap<Long, Integer> newCellBg = new HashMap<>();
        for (HashMap.Entry<Long, Integer> e : t.cellBg.entrySet())
        {
            long key = e.getKey();
            int lin = (int)(key >> 32);
            int col = (int)(key & 0xFFFFFFFFL);
            newCellBg.put(cellKey(lin, remapCol(col, fromCol, toCol)), e.getValue());
        }
        t.cellBg.clear();
        t.cellBg.putAll(newCellBg);

        HashMap<Long, Integer> newCellFg = new HashMap<>();
        for (HashMap.Entry<Long, Integer> e : t.cellFg.entrySet())
        {
            long key = e.getKey();
            int lin = (int)(key >> 32);
            int col = (int)(key & 0xFFFFFFFFL);
            newCellFg.put(cellKey(lin, remapCol(col, fromCol, toCol)), e.getValue());
        }
        t.cellFg.clear();
        t.cellFg.putAll(newCellFg);

        HashMap<Long, Drawable> newCellImage = new HashMap<>();
        for (HashMap.Entry<Long, Drawable> e : t.cellImage.entrySet())
        {
            long key = e.getKey();
            int lin = (int)(key >> 32);
            int col = (int)(key & 0xFFFFFFFFL);
            newCellImage.put(cellKey(lin, remapCol(col, fromCol, toCol)), e.getValue());
        }
        t.cellImage.clear();
        t.cellImage.putAll(newCellImage);

        SparseArray<CellFont> nfo = remapSparseArray(t.colFont, fromCol, toCol);
        t.colFont.clear();
        for (int i = 0; i < nfo.size(); i++) t.colFont.put(nfo.keyAt(i), nfo.valueAt(i));

        HashMap<Long, CellFont> newCellFont = new HashMap<>();
        for (HashMap.Entry<Long, CellFont> e : t.cellFont.entrySet())
        {
            long key = e.getKey();
            int lin = (int)(key >> 32);
            int col = (int)(key & 0xFFFFFFFFL);
            newCellFont.put(cellKey(lin, remapCol(col, fromCol, toCol)), e.getValue());
        }
        t.cellFont.clear();
        t.cellFont.putAll(newCellFont);

        if (t.focusCol == fromCol) t.focusCol = toCol;
        else if (fromCol < t.focusCol && t.focusCol <= toCol) t.focusCol--;
        else if (toCol <= t.focusCol && t.focusCol < fromCol) t.focusCol++;

        if (t.sortCol == fromCol) t.sortCol = toCol;
        else if (fromCol < t.sortCol && t.sortCol <= toCol) t.sortCol--;
        else if (toCol <= t.sortCol && t.sortCol < fromCol) t.sortCol++;

        rebuildHeader(t);
        t.adapter.notifyDataSetChanged();
    }


    @Keep
    public static View createTable(long ihandlePtr)
    {
        ContextThemeWrapper ctx = IupCommon.getContextThemeWrapper();
        final IupTableView table = new IupTableView(ctx, ihandlePtr);

        table.headerScroll = new SyncScrollView(ctx);
        table.headerScroll.owner = table;
        table.headerScroll.isHeader = true;
        table.headerScroll.setHorizontalScrollBarEnabled(false);
        table.headerScroll.setLayoutParams(new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT));

        table.headerRow = new LinearLayout(ctx);
        table.headerRow.setOrientation(LinearLayout.HORIZONTAL);
        table.headerRow.setLayoutParams(new ViewGroup.LayoutParams(
            ViewGroup.LayoutParams.WRAP_CONTENT, table.headerHeightPx));
        table.headerScroll.addView(table.headerRow);

        table.bodyScroll = new SyncScrollView(ctx);
        table.bodyScroll.owner = table;
        table.bodyScroll.isHeader = false;
        table.bodyScroll.setFillViewport(true);
        LinearLayout.LayoutParams bodyLp = new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT, 0);
        bodyLp.weight = 1;
        table.bodyScroll.setLayoutParams(bodyLp);

        table.recyclerView = new RecyclerView(ctx);
        table.recyclerView.setLayoutManager(new LinearLayoutManager(ctx));
        table.recyclerView.setLayoutParams(new ViewGroup.LayoutParams(
            ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.MATCH_PARENT));
        table.adapter = new RowAdapter(table);
        table.recyclerView.setAdapter(table.adapter);
        table.bodyScroll.addView(table.recyclerView);

        table.addView(table.headerScroll);
        table.addView(table.bodyScroll);

        applyGridDecoration(table);

        table.bodyScroll.addOnLayoutChangeListener((v, l, t2, r, b, ol, ot, or, ob) -> {
            if ((r - l) != (or - ol)) applyStretchLast(table);
        });

        return table;
    }

    private static void applyGridDecoration(IupTableView table)
    {
        while (table.recyclerView.getItemDecorationCount() > 0)
            table.recyclerView.removeItemDecorationAt(0);
        if (!table.showGrid) return;

        DividerItemDecoration dec = new DividerItemDecoration(
            table.getContext(), DividerItemDecoration.VERTICAL);
        dec.setDrawable(new ColorDrawable(table.gridColor));
        table.recyclerView.addItemDecoration(dec);
    }


    private static void resizeArrays(IupTableView table, int numCol)
    {
        int[] widths = new int[numCol];
        String[] titles = new String[numCol];
        boolean[] explicit = new boolean[numCol];
        int keep = Math.min(numCol, table.colWidths != null ? table.colWidths.length : 0);
        for (int i = 0; i < keep; i++)
        {
            widths[i] = table.colWidths[i];
            titles[i] = table.colTitles[i];
            if (i < table.colExplicit.length) explicit[i] = table.colExplicit[i];
        }
        table.colWidths = widths;
        table.colTitles = titles;
        table.colExplicit = explicit;
    }

    private static void rebuildHeader(IupTableView table)
    {
        LinearLayout hdr = table.headerRow;
        while (hdr.getChildCount() < table.numCol)
            hdr.addView(makeHeaderCell(hdr.getContext(), table));
        while (hdr.getChildCount() > table.numCol)
            hdr.removeViewAt(hdr.getChildCount() - 1);

        int gridPx = table.showGrid ? 1 : 0;
        for (int c = 0; c < table.numCol; c++)
        {
            int col = c + 1;
            TextView tv = (TextView) hdr.getChildAt(c);
            int w = (table.colWidths[c] > 0) ? table.colWidths[c] : table.defaultColWidth;
            LinearLayout.LayoutParams lp = (LinearLayout.LayoutParams) tv.getLayoutParams();
            lp.width = w;
            lp.height = table.headerHeightPx;
            lp.rightMargin = (c < table.numCol - 1) ? gridPx : 0;
            tv.setLayoutParams(lp);
            tv.setBackgroundColor(table.headerBg);
            tv.setTextColor(table.headerFg);
            String title = table.colTitles[c] != null ? table.colTitles[c] : "";
            if (table.sortable && col == table.sortCol)
                title = title + (table.sortAsc ? "  ▲" : "  ▼");
            tv.setText(title);
            ((int[]) tv.getTag())[0] = col;
            if (tv instanceof HeaderCell) ((HeaderCell) tv).showGrabber = table.userResize;
        }
    }


    @Keep
    public static void setNumCol(View v, int numCol)
    {
        if (!(v instanceof IupTableView table)) return;
        if (numCol < 0) numCol = 0;
        if (numCol == table.numCol) return;

        table.numCol = numCol;
        resizeArrays(table, numCol);

        for (int i = 0; i < table.rows.size(); i++)
        {
            String[] old_row = table.rows.get(i);
            String[] new_row = new String[numCol];
            int keep = Math.min(numCol, old_row != null ? old_row.length : 0);
            if (keep > 0) System.arraycopy(old_row, 0, new_row, 0, keep);
            table.rows.set(i, new_row);
        }

        rebuildHeader(table);
        table.adapter.notifyDataSetChanged();
    }

    @Keep
    public static void setNumLin(View v, int numLin)
    {
        if (!(v instanceof IupTableView table)) return;
        if (numLin < 0) numLin = 0;
        if (numLin == table.numLin) return;

        int old = table.numLin;
        table.numLin = numLin;

        while (table.rows.size() < numLin)
            table.rows.add(new String[table.numCol]);
        while (table.rows.size() > numLin)
            table.rows.remove(table.rows.size() - 1);

        if (numLin > old)
            table.adapter.notifyItemRangeInserted(old, numLin - old);
        else
            table.adapter.notifyItemRangeRemoved(numLin, old - numLin);
    }

    @Keep
    public static void addLin(View v, int pos)
    {
        if (!(v instanceof IupTableView table)) return;
        int idx = Math.max(0, Math.min(pos - 1, table.numLin));
        table.rows.add(idx, new String[table.numCol]);
        table.numLin++;
        table.adapter.notifyItemInserted(idx);
    }

    @Keep
    public static void delLin(View v, int pos)
    {
        if (!(v instanceof IupTableView table)) return;
        int idx = pos - 1;
        if (idx < 0 || idx >= table.numLin) return;
        table.rows.remove(idx);
        table.numLin--;
        table.adapter.notifyItemRemoved(idx);
    }

    @Keep
    public static void addCol(View v, int pos)
    {
        if (!(v instanceof IupTableView table)) return;
        int idx = Math.max(0, Math.min(pos - 1, table.numCol));
        int newN = table.numCol + 1;
        int[] w = new int[newN];
        String[] t = new String[newN];
        for (int c = 0, d = 0; c < newN; c++)
        {
            if (c == idx) continue;
            w[c] = d < table.colWidths.length ? table.colWidths[d] : 0;
            t[c] = d < table.colTitles.length ? table.colTitles[d] : null;
            d++;
        }
        table.colWidths = w;
        table.colTitles = t;
        table.numCol = newN;

        for (int i = 0; i < table.rows.size(); i++)
        {
            String[] old_row = table.rows.get(i);
            String[] new_row = new String[newN];
            for (int c = 0, d = 0; c < newN; c++)
            {
                if (c == idx) continue;
                new_row[c] = (old_row != null && d < old_row.length) ? old_row[d] : null;
                d++;
            }
            table.rows.set(i, new_row);
        }
        rebuildHeader(table);
        table.adapter.notifyDataSetChanged();
    }

    @Keep
    public static void delCol(View v, int pos)
    {
        if (!(v instanceof IupTableView table)) return;
        int idx = pos - 1;
        int oldN = table.numCol;
        if (idx < 0 || idx >= oldN) return;

        int newN = oldN - 1;
        int[] w = new int[newN];
        String[] t = new String[newN];
        for (int c = 0, d = 0; c < oldN; c++)
        {
            if (c == idx) continue;
            w[d] = table.colWidths[c];
            t[d] = table.colTitles[c];
            d++;
        }

        for (int i = 0; i < table.rows.size(); i++)
        {
            String[] old_row = table.rows.get(i);
            String[] new_row = new String[newN];
            for (int c = 0, d = 0; c < oldN; c++)
            {
                if (c == idx) continue;
                new_row[d++] = (old_row != null && c < old_row.length) ? old_row[c] : null;
            }
            table.rows.set(i, new_row);
        }

        table.colWidths = w;
        table.colTitles = t;
        table.numCol = newN;
        rebuildHeader(table);
        table.adapter.notifyDataSetChanged();
    }


    @Keep
    public static void setCellValue(View v, int lin, int col, String value)
    {
        if (!(v instanceof IupTableView table)) return;
        int li = lin - 1, ci = col - 1;
        if (li < 0 || li >= table.numLin || ci < 0 || ci >= table.numCol) return;

        String[] row = table.rows.get(li);
        if (row == null || row.length != table.numCol)
        {
            row = new String[table.numCol];
            table.rows.set(li, row);
        }
        row[ci] = value;
        table.adapter.notifyItemChanged(li);
    }

    @Keep
    public static String getCellValue(View v, int lin, int col)
    {
        if (!(v instanceof IupTableView table)) return null;
        int li = lin - 1, ci = col - 1;
        if (li < 0 || li >= table.numLin || ci < 0 || ci >= table.numCol) return null;
        String[] row = table.rows.get(li);
        return (row != null && ci < row.length) ? row[ci] : null;
    }


    @Keep
    public static void setColTitle(View v, int col, String title)
    {
        if (!(v instanceof IupTableView table)) return;
        int ci = col - 1;
        if (ci < 0 || ci >= table.numCol) return;
        table.colTitles[ci] = title;
        rebuildHeader(table);
    }

    @Keep
    public static String getColTitle(View v, int col)
    {
        if (!(v instanceof IupTableView table)) return null;
        int ci = col - 1;
        if (ci < 0 || ci >= table.numCol) return null;
        return table.colTitles[ci];
    }


    @Keep
    public static void setColWidth(View v, int col, int widthPx)
    {
        if (!(v instanceof IupTableView table)) return;
        int ci = col - 1;
        if (ci < 0 || ci >= table.numCol) return;
        table.colWidths[ci] = widthPx;
        if (ci < table.colExplicit.length) table.colExplicit[ci] = true;
        rebuildHeader(table);
        table.adapter.notifyDataSetChanged();
    }

    @Keep
    public static void autoSizeColumns(View v)
    {
        if (!(v instanceof IupTableView t)) return;
        if (t.virtualMode) return;

        float density = IupCommon.getDisplayDensity();
        int cellPadPx = Math.round(12 * density);
        int slackPx = Math.round(16 * density);
        int drawablePadPx = t.iconPaddingPx;
        int minPx = Math.round(60 * density);
        int maxPx = Math.round(600 * density);

        TextView probe = makeCell(t.getContext());
        probe.setTypeface(Typeface.DEFAULT);
        probe.setEllipsize(null);
        probe.setMaxWidth(Integer.MAX_VALUE);
        ensureDefaultCellTextSize(t, probe);
        float defaultProbeSize = probe.getTextSize();
        Paint boldPaint = new Paint();
        boldPaint.setAntiAlias(true);

        int rowsToSample = Math.min(100, t.rows.size());
        int[] specUn = { View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED),
                         View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED) };

        for (int c = 0; c < t.numCol; c++)
        {
            if (c < t.colExplicit.length && t.colExplicit[c]) continue;

            int col = c + 1;
            int widest;

            String title = t.colTitles[c] != null ? t.colTitles[c] : "";
            boldPaint.setTypeface(Typeface.DEFAULT_BOLD);
            boldPaint.setTextSize(defaultProbeSize);
            widest = Math.round(boldPaint.measureText(title));

            for (int r = 0; r < rowsToSample; r++)
            {
                String[] row = t.rows.get(r);
                if (row == null || c >= row.length) continue;
                String s = row[c] != null ? row[c] : "";
                applyCellFont(t, probe, resolveFont(t, r + 1, col));
                probe.setText(s);
                probe.measure(specUn[0], specUn[1]);
                int w = probe.getMeasuredWidth() - cellPadPx;
                if (w > widest) widest = w;
            }

            int imageBudget = 0;
            if (t.showImage)
            {
                for (Long key : t.cellImage.keySet())
                {
                    int kcol = (int)(key & 0xFFFFFFFFL);
                    if (kcol != col) continue;
                    Drawable d = t.cellImage.get(key);
                    int dw = d != null ? d.getBounds().width() : t.rowHeightPx;
                    int budget = dw + drawablePadPx;
                    if (budget > imageBudget) imageBudget = budget;
                }
            }

            int total = widest + imageBudget + cellPadPx + slackPx;
            total = Math.max(minPx, Math.min(maxPx, total));
            t.colWidths[c] = total;
            if (c == t.numCol - 1) t.lastColAutoWidth = total;
        }

        rebuildHeader(t);
        t.adapter.notifyDataSetChanged();
        applyStretchLast(t);
    }

    static void applyStretchLast(IupTableView t)
    {
        if (!t.stretchLast || t.numCol <= 0) return;
        int lastIdx = t.numCol - 1;
        if (lastIdx < t.colExplicit.length && t.colExplicit[lastIdx]) return;

        int viewport = t.bodyScroll != null ? t.bodyScroll.getWidth() : 0;
        if (viewport <= 0) return;

        int sumOthers = 0;
        for (int c = 0; c < lastIdx; c++)
            sumOthers += (t.colWidths[c] > 0 ? t.colWidths[c] : t.defaultColWidth);

        int autoW = t.lastColAutoWidth > 0 ? t.lastColAutoWidth : t.defaultColWidth;
        int want = Math.max(autoW, viewport - sumOthers);
        if (t.colWidths[lastIdx] == want) return;
        t.colWidths[lastIdx] = want;
        rebuildHeader(t);
        t.adapter.notifyDataSetChanged();
    }

    @Keep
    public static int getColWidth(View v, int col)
    {
        if (!(v instanceof IupTableView table)) return 0;
        int ci = col - 1;
        if (ci < 0 || ci >= table.numCol) return 0;
        int w = table.colWidths[ci];
        return w > 0 ? w : table.defaultColWidth;
    }


    @Keep
    public static void setShowGrid(View v, boolean show)
    {
        if (!(v instanceof IupTableView table)) return;
        if (table.showGrid == show) return;
        table.showGrid = show;
        applyGridDecoration(table);
        rebuildHeader(table);
        table.adapter.notifyDataSetChanged();
    }


    @Keep
    public static void redraw(View v)
    {
        if (!(v instanceof IupTableView table)) return;
        rebuildHeader(table);
        table.adapter.notifyDataSetChanged();
    }


    @Keep
    public static int getRowHeight(View v)
    {
        if (!(v instanceof IupTableView)) return 0;
        return ((IupTableView) v).rowHeightPx;
    }

    @Keep
    public static int getHeaderHeight(View v)
    {
        if (!(v instanceof IupTableView)) return 0;
        return ((IupTableView) v).headerHeightPx;
    }

    @Keep
    public static int defaultRowHeightPx()
    {
        return Math.round(32 * IupCommon.getDisplayDensity());
    }

    @Keep
    public static int defaultHeaderHeightPx()
    {
        return Math.round(36 * IupCommon.getDisplayDensity());
    }


    /* lin/col: both>0 cell, lin==0 column, col==0 row; negative rgb clears */

    @Keep
    public static void setBgColor(View v, int lin, int col, int r, int g, int b)
    {
        if (!(v instanceof IupTableView t)) return;
        boolean clear = (r < 0 || g < 0 || b < 0);
        Integer rgb = clear ? null : Color.rgb(r, g, b);
        if (lin > 0 && col > 0) { if (clear) t.cellBg.remove(cellKey(lin, col)); else t.cellBg.put(cellKey(lin, col), rgb); }
        else if (lin == 0 && col > 0) { if (clear) t.colBg.delete(col); else t.colBg.put(col, rgb); }
        else if (lin > 0 && col == 0) { if (clear) t.rowBg.delete(lin); else t.rowBg.put(lin, rgb); }
        else return;
        t.adapter.notifyDataSetChanged();
    }

    @Keep
    public static void setFgColor(View v, int lin, int col, int r, int g, int b)
    {
        if (!(v instanceof IupTableView t)) return;
        boolean clear = (r < 0 || g < 0 || b < 0);
        Integer rgb = clear ? null : Color.rgb(r, g, b);
        if (lin > 0 && col > 0) { if (clear) t.cellFg.remove(cellKey(lin, col)); else t.cellFg.put(cellKey(lin, col), rgb); }
        else if (lin == 0 && col > 0) { if (clear) t.colFg.delete(col); else t.colFg.put(col, rgb); }
        else if (lin > 0 && col == 0) { if (clear) t.rowFg.delete(lin); else t.rowFg.put(lin, rgb); }
        else return;
        t.adapter.notifyDataSetChanged();
    }

    @Keep
    public static void setCellFont(View v, int lin, int col, String family, int style, int sizeUnit, float size)
    {
        if (!(v instanceof IupTableView t)) return;
        boolean clear = (family == null && size <= 0);
        CellFont cf = clear ? null : new CellFont(family, style, sizeUnit, size);
        if (lin > 0 && col > 0) { if (clear) t.cellFont.remove(cellKey(lin, col)); else t.cellFont.put(cellKey(lin, col), cf); }
        else if (lin == 0 && col > 0) { if (clear) t.colFont.remove(col); else t.colFont.put(col, cf); }
        else if (lin > 0 && col == 0) { if (clear) t.rowFont.remove(lin); else t.rowFont.put(lin, cf); }
        else return;
        t.adapter.notifyDataSetChanged();
    }

    @Keep
    public static void setDefaultFgColor(View v, int r, int g, int b)
    {
        if (!(v instanceof IupTableView t)) return;
        t.defaultFg = Color.rgb(r, g, b);
        t.adapter.notifyDataSetChanged();
    }

    @Keep
    public static void setDefaultBgColor(View v, int r, int g, int b)
    {
        if (!(v instanceof IupTableView t)) return;
        int color = Color.rgb(r, g, b);
        t.defaultBg = color;
        t.setBackgroundColor(color);
        t.adapter.notifyDataSetChanged();
    }

    @Keep
    public static void setAlternateColor(View v, boolean on)
    {
        if (!(v instanceof IupTableView t)) return;
        t.alternateColor = on;
        t.adapter.notifyDataSetChanged();
    }

    @Keep
    public static void setEvenRowColor(View v, int r, int g, int b)
    {
        if (!(v instanceof IupTableView t)) return;
        t.evenBg = Color.rgb(r, g, b);
        t.hasEvenBg = true;
        t.adapter.notifyDataSetChanged();
    }

    @Keep
    public static void setOddRowColor(View v, int r, int g, int b)
    {
        if (!(v instanceof IupTableView t)) return;
        t.oddBg = Color.rgb(r, g, b);
        t.hasOddBg = true;
        t.adapter.notifyDataSetChanged();
    }


    @Keep
    public static void setColAlignment(View v, int col, String align)
    {
        if (!(v instanceof IupTableView t)) return;
        if (align == null) t.colAlign.remove(col); else t.colAlign.put(col, align);
        t.adapter.notifyDataSetChanged();
    }


    /* Images: Bitmap or null to clear. */

    @Keep
    public static void setShowImage(View v, boolean show)
    {
        if (!(v instanceof IupTableView t)) return;
        t.showImage = show;
        t.adapter.notifyDataSetChanged();
    }

    @Keep
    public static void setFitImage(View v, boolean fit)
    {
        if (!(v instanceof IupTableView)) return;
        ((IupTableView) v).fitImage = fit;
    }

    @Keep
    public static void setCellImage(View v, int lin, int col, Bitmap bmp)
    {
        if (!(v instanceof IupTableView t)) return;
        if (bmp == null) { t.cellImage.remove(cellKey(lin, col)); }
        else
        {
            BitmapDrawable d = new BitmapDrawable(v.getResources(), bmp);
            int dw, dh;
            if (t.fitImage)
            {
                int box = Math.max(1, t.rowHeightPx - Math.round(4 * IupCommon.getDisplayDensity()));
                int sw = bmp.getWidth(), sh = bmp.getHeight();
                int max = Math.max(sw, sh);
                dw = sw * box / max;
                dh = sh * box / max;
            }
            else { dw = d.getIntrinsicWidth(); dh = d.getIntrinsicHeight(); }
            d.setBounds(0, 0, dw, dh);
            t.cellImage.put(cellKey(lin, col), d);
        }
        if (t.showImage)
        {
            int lin0 = lin - 1;
            if (lin0 >= 0 && lin0 < t.numLin) t.adapter.notifyItemChanged(lin0);
        }
    }


    /* Focus and selection. */

    @Keep
    public static void setFocusCell(View v, int lin, int col)
    {
        if (!(v instanceof IupTableView t)) return;
        if (lin < 1 || lin > t.numLin || col < 1 || col > t.numCol) return;
        int oldLin = t.focusLin, oldCol = t.focusCol;
        if (oldLin == lin && oldCol == col) return;
        t.focusLin = lin;
        t.focusCol = col;
        refreshRowStyle(t, oldLin);
        refreshRowStyle(t, lin);
    }

    @Keep
    public static int getFocusCellLin(View v)
    {
        if (!(v instanceof IupTableView)) return 1;
        return ((IupTableView) v).focusLin;
    }

    @Keep
    public static int getFocusCellCol(View v)
    {
        if (!(v instanceof IupTableView)) return 1;
        return ((IupTableView) v).focusCol;
    }

    @Keep
    public static void scrollToCell(View v, int lin, int col)
    {
        if (!(v instanceof IupTableView t)) return;
        final int lin0 = lin - 1;
        if (lin0 >= 0 && lin0 < t.numLin)
            t.recyclerView.post(() -> t.recyclerView.smoothScrollToPosition(lin0));

        if (col >= 1 && col <= t.numCol)
        {
            int x = 0;
            for (int c = 0; c < col - 1; c++)
                x += (t.colWidths[c] > 0 ? t.colWidths[c] : t.defaultColWidth);
            final int fx = x;
            t.bodyScroll.post(() -> t.bodyScroll.smoothScrollTo(fx, 0));
        }
    }


    /* Editing. */

    @Keep
    public static void setEditableAll(View v, boolean editable)
    {
        if (!(v instanceof IupTableView)) return;
        ((IupTableView) v).editableAll = editable;
    }

    @Keep
    public static void setEditableCol(View v, int col, boolean editable)
    {
        if (!(v instanceof IupTableView)) return;
        ((IupTableView) v).editableCol.put(col, editable);
    }


    /* Sort / resize / reorder. */

    @Keep
    public static void setSortable(View v, boolean on)
    {
        if (!(v instanceof IupTableView t)) return;
        t.sortable = on;
        if (!on) { t.sortCol = 0; }
        rebuildHeader(t);
    }

    @Keep
    public static void setUserResize(View v, boolean on)
    {
        if (!(v instanceof IupTableView t)) return;
        t.userResize = on;
        applyHeaderGrabberIndicator(t);
    }

    static void applyHeaderGrabberIndicator(IupTableView t)
    {
        for (int c = 0; c < t.headerRow.getChildCount(); c++)
        {
            View hv = t.headerRow.getChildAt(c);
            if (hv instanceof HeaderCell)
            {
                ((HeaderCell) hv).showGrabber = t.userResize;
                hv.invalidate();
            }
        }
    }

    @Keep
    public static void setAllowReorder(View v, boolean on)
    {
        if (!(v instanceof IupTableView)) return;
        ((IupTableView) v).allowReorder = on;
    }

    @Keep
    public static void setVirtualMode(View v, boolean on)
    {
        if (!(v instanceof IupTableView t)) return;
        t.virtualMode = on;
        t.adapter.notifyDataSetChanged();
    }

    @Keep
    public static void setFocusRect(View v, boolean on)
    {
        if (!(v instanceof IupTableView t)) return;
        t.focusRect = on;
        refreshRowStyle(t, t.focusLin);
    }

    @Keep
    public static void setStretchLast(View v, boolean on)
    {
        if (!(v instanceof IupTableView t)) return;
        t.stretchLast = on;
    }

    public static native void dispatchClick(long ihandlePtr, int lin, int col, int focusChanged);
    public static native int dispatchEditBegin(long ihandlePtr, int lin, int col);
    public static native void dispatchEdition(long ihandlePtr, int lin, int col, String text);
    public static native int dispatchEditEnd(long ihandlePtr, int lin, int col, String text, int apply);
    public static native void dispatchValueChanged(long ihandlePtr, int lin, int col);
    public static native void dispatchSort(long ihandlePtr, int col, int asc);
    public static native void dispatchReorder(long ihandlePtr, int fromCol, int toCol);
    public static native String dispatchValueRequest(long ihandlePtr, int lin, int col);
}
