package io.github.gen2brain.iupgo;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.Typeface;
import android.text.TextUtils;
import android.util.TypedValue;
import android.view.GestureDetector;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.view.ContextThemeWrapper;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;


/* notifyDataSetChanged on purpose: bulk ADD/DEL/EXPAND/MARK across the flat id cache */
@android.annotation.SuppressLint("NotifyDataSetChanged")
public final class IupTreeHelper
{
    private IupTreeHelper() {}


    public static final class TreeNode
    {
        int kind;
        String title = "";
        TreeNode parent;
        final ArrayList<TreeNode> children = new ArrayList<>();
        boolean expanded = true;
    }


    public static final class NodeFont
    {
        final String family;
        final int style;
        final int sizeUnit;
        final float size;
        NodeFont(String family, int style, int sizeUnit, float size)
        {
            this.family = family;
            this.style = style;
            this.sizeUnit = sizeUnit;
            this.size = size;
        }
    }


    /* Programmatic-only; requires an Ihandle so the XML constructors are absent. */
    @android.annotation.SuppressLint("ViewConstructor")
    public static final class IupTreeView extends RecyclerView
    {
        final long ihandlePtr;

        @Override
        public void setEnabled(boolean enabled)
        {
            super.setEnabled(enabled);
            setAlpha(enabled ? 1.0f : 0.5f);
        }

        @Override
        public boolean dispatchTouchEvent(MotionEvent ev)
        {
            if (!isEnabled()) return true;
            return super.dispatchTouchEvent(ev);
        }

        final ArrayList<TreeNode> roots = new ArrayList<>();
        final ArrayList<TreeNode> visible = new ArrayList<>();
        final HashSet<TreeNode> marked = new HashSet<>();
        final HashMap<TreeNode, Integer> nodeColor = new HashMap<>();
        final HashMap<TreeNode, NodeFont> nodeFont = new HashMap<>();
        final HashMap<TreeNode, Bitmap> nodeImage = new HashMap<>();
        final HashMap<TreeNode, Bitmap> nodeImageExpanded = new HashMap<>();
        Bitmap defaultImageLeaf;
        Bitmap defaultImageBranchCollapsed;
        Bitmap defaultImageBranchExpanded;
        float defaultTitleTextSizePx;
        NodeAdapter adapter;
        int focusIdx = -1;
        TreeNode markStart;

        int rowHeightPx;
        int indentPx;
        int chevronSizePx;
        int iconSizePx;
        int iconPaddingPx;

        int selectBg;
        int selectFg;
        int textFg;
        int rowBg;
        int chevronFg;
        int markedBg;

        boolean addExpanded = true;
        boolean markMultiple;
        boolean showRename;
        boolean showDragDrop;

        /* Adapter index of the row over which the drag is hovering, -1 = no drop indicator. */
        int dropIndicatorIdx = -1;

        public IupTreeView(Context ctx, long ih)
        {
            super(ctx);
            this.ihandlePtr = ih;
            float d = IupCommon.getDisplayDensity();
            this.rowHeightPx = Math.round(32 * d);
            this.indentPx = Math.round(24 * d);
            this.chevronSizePx = Math.round(24 * d);
            this.iconSizePx = Math.round(20 * d);
            this.iconPaddingPx = Math.round(4 * d);
            applyThemeColors();
            setLayoutManager(new LinearLayoutManager(ctx));
            this.adapter = new NodeAdapter(this);
            setAdapter(adapter);
            addItemDecoration(new DropIndicatorDecoration());
            installDropListener(this);
        }

        void applyThemeColors()
        {
            this.textFg    = IupCommon.paletteTxtFg;
            this.rowBg     = IupCommon.paletteTxtBg;
            this.selectBg  = IupCommon.paletteTxtHlBg;
            this.selectFg  = IupCommon.paletteTxtHlFg;
            this.chevronFg = IupCommon.blendColor(IupCommon.paletteTxtBg, IupCommon.paletteTxtFg, 0.55f);
            this.markedBg  = IupCommon.blendColor(IupCommon.paletteTxtBg, selectBg, 0.30f);
            setBackgroundColor(rowBg);
        }
    }

    @Keep
    public static void setBgColor(View widget, int r, int g, int b)
    {
        if (!(widget instanceof IupTreeView t)) return;
        int color = Color.rgb(r, g, b);
        t.rowBg     = color;
        t.chevronFg = IupCommon.blendColor(color, t.textFg, 0.55f);
        t.markedBg  = IupCommon.blendColor(color, t.selectBg, 0.30f);
        t.setBackgroundColor(color);
        if (t.adapter != null) t.adapter.notifyDataSetChanged();
    }


    static final class NodeHolder extends RecyclerView.ViewHolder
    {
        final LinearLayout row;
        final View spacer;
        final TextView chevron;
        final TextView iconEmoji;
        final ImageView iconImage;
        final TextView title;

        NodeHolder(LinearLayout row, View spacer, TextView chev, TextView iconEmoji, ImageView iconImage, TextView tv)
        {
            super(row);
            this.row = row;
            this.spacer = spacer;
            this.chevron = chev;
            this.iconEmoji = iconEmoji;
            this.iconImage = iconImage;
            this.title = tv;
        }
    }


    static final class NodeAdapter extends RecyclerView.Adapter<NodeHolder>
    {
        final IupTreeView tree;

        NodeAdapter(IupTreeView tree) { this.tree = tree; }

        @Override public int getItemCount() { return tree.visible.size(); }

        @NonNull
        @Override
        public NodeHolder onCreateViewHolder(ViewGroup parent, int viewType)
        {
            Context ctx = parent.getContext();
            LinearLayout row = new LinearLayout(ctx);
            row.setOrientation(LinearLayout.HORIZONTAL);
            row.setGravity(Gravity.CENTER_VERTICAL);
            row.setLayoutParams(new RecyclerView.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, tree.rowHeightPx));

            View spacer = new View(ctx);
            spacer.setLayoutParams(new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT));
            row.addView(spacer);

            TextView chev = new TextView(ctx);
            chev.setGravity(Gravity.CENTER);
            chev.setTextColor(tree.chevronFg);
            chev.setLayoutParams(new LinearLayout.LayoutParams(tree.chevronSizePx, ViewGroup.LayoutParams.MATCH_PARENT));
            row.addView(chev);

            FrameLayout iconBox = new FrameLayout(ctx);
            LinearLayout.LayoutParams iconBoxLp = new LinearLayout.LayoutParams(
                tree.iconSizePx + 2 * tree.iconPaddingPx, ViewGroup.LayoutParams.MATCH_PARENT);
            iconBox.setLayoutParams(iconBoxLp);

            TextView iconEmoji = new TextView(ctx);
            iconEmoji.setGravity(Gravity.CENTER);
            iconEmoji.setLayoutParams(new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
            iconBox.addView(iconEmoji);

            ImageView iconImage = new ImageView(ctx);
            FrameLayout.LayoutParams iconImgLp = new FrameLayout.LayoutParams(
                tree.iconSizePx, tree.iconSizePx, Gravity.CENTER);
            iconImage.setLayoutParams(iconImgLp);
            iconImage.setVisibility(View.GONE);
            iconBox.addView(iconImage);

            row.addView(iconBox);

            TextView tv = new TextView(ctx);
            tv.setSingleLine(true);
            tv.setEllipsize(TextUtils.TruncateAt.END);
            tv.setGravity(Gravity.CENTER_VERTICAL | Gravity.START);
            LinearLayout.LayoutParams tvLp = new LinearLayout.LayoutParams(
                0, ViewGroup.LayoutParams.MATCH_PARENT, 1f);
            tv.setLayoutParams(tvLp);
            row.addView(tv);

            final NodeHolder holder = new NodeHolder(row, spacer, chev, iconEmoji, iconImage, tv);
            attachListeners(tree, holder);
            return holder;
        }

        @Override
        public void onBindViewHolder(NodeHolder h, int position)
        {
            TreeNode node = tree.visible.get(position);

            int depth = depthOf(node);
            LinearLayout.LayoutParams sp = (LinearLayout.LayoutParams) h.spacer.getLayoutParams();
            sp.width = depth * tree.indentPx;
            h.spacer.setLayoutParams(sp);

            if (node.kind == 0)
            {
                h.chevron.setText(node.expanded ? "▾" : "▸");
                h.chevron.setVisibility(View.VISIBLE);
            }
            else
            {
                h.chevron.setText("");
                h.chevron.setVisibility(View.INVISIBLE);
            }

            h.title.setText(node.title != null ? node.title : "");

            if (tree.defaultTitleTextSizePx <= 0) tree.defaultTitleTextSizePx = h.title.getTextSize();
            applyNodeFont(tree, h.title, tree.nodeFont.get(node));

            boolean selected = (position == tree.focusIdx);
            boolean marked = tree.markMultiple && tree.marked.contains(node);
            drawFolderOrLeaf(tree, h, node, selected);

            Integer customFg = tree.nodeColor.get(node);
            int fg = selected ? tree.selectFg : (customFg != null ? customFg : tree.textFg);

            if (selected)
            {
                h.row.setBackgroundColor(tree.selectBg);
                h.chevron.setTextColor(tree.selectFg);
            }
            else if (marked)
            {
                h.row.setBackgroundColor(tree.markedBg);
                h.chevron.setTextColor(tree.chevronFg);
            }
            else
            {
                h.row.setBackground(null);
                h.chevron.setTextColor(tree.chevronFg);
            }
            h.title.setTextColor(fg);
        }
    }


    static void drawFolderOrLeaf(IupTreeView tree, NodeHolder h, TreeNode node, boolean selected)
    {
        Bitmap bmp = resolveNodeBitmap(tree, node);
        if (bmp != null)
        {
            h.iconImage.setImageBitmap(bmp);
            h.iconImage.setVisibility(View.VISIBLE);
            h.iconEmoji.setVisibility(View.GONE);
            return;
        }
        h.iconImage.setVisibility(View.GONE);
        h.iconEmoji.setVisibility(View.VISIBLE);
        if (node.kind == 0) h.iconEmoji.setText(node.expanded ? "📂" : "📁");
        else                h.iconEmoji.setText("📄");
        h.iconEmoji.setTextColor(selected ? tree.selectFg : tree.chevronFg);
    }

    /* Called from C on theme change: pick new defaults and redraw. */
    @Keep
    public static void refreshTheme(View v)
    {
        if (!(v instanceof IupTreeView t)) return;
        t.applyThemeColors();
        t.adapter.notifyDataSetChanged();
    }

    static Bitmap resolveNodeBitmap(IupTreeView tree, TreeNode node)
    {
        if (node.kind == 0 && node.expanded)
        {
            Bitmap b = tree.nodeImageExpanded.get(node);
            if (b != null) return b;
            if (tree.defaultImageBranchExpanded != null) return tree.defaultImageBranchExpanded;
        }
        Bitmap b = tree.nodeImage.get(node);
        if (b != null) return b;
        if (node.kind == 0) return tree.defaultImageBranchCollapsed;
        return tree.defaultImageLeaf;
    }


    /* Taps/long-presses route through GestureDetector; no native click. */
    @android.annotation.SuppressLint("ClickableViewAccessibility")
    static void attachListeners(final IupTreeView tree, final NodeHolder h)
    {
        final GestureDetector gd = new GestureDetector(h.itemView.getContext(),
            new GestureDetector.SimpleOnGestureListener()
            {
                @Override public boolean onDown(@NonNull MotionEvent e) { return true; }

                @Override public boolean onSingleTapUp(@NonNull MotionEvent e)
                {
                    int pos = h.getBindingAdapterPosition();
                    if (pos == RecyclerView.NO_POSITION) return true;
                    int old = tree.focusIdx;
                    tree.focusIdx = pos;
                    if (old != pos)
                    {
                        if (old >= 0) tree.adapter.notifyItemChanged(old);
                        tree.adapter.notifyItemChanged(pos);
                        dispatchSelection(tree.ihandlePtr, idOfNode(tree, tree.visible.get(pos)), 1);
                    }
                    return true;
                }

                @Override public boolean onDoubleTap(@NonNull MotionEvent e)
                {
                    int pos = h.getBindingAdapterPosition();
                    if (pos == RecyclerView.NO_POSITION) return true;
                    TreeNode n = tree.visible.get(pos);
                    if (n.kind == 0) toggleExpand(tree, n);
                    else dispatchExecuteLeaf(tree.ihandlePtr, idOfNode(tree, n));
                    return true;
                }

                @Override public void onLongPress(@NonNull MotionEvent e)
                {
                    int pos = h.getBindingAdapterPosition();
                    if (pos == RecyclerView.NO_POSITION) return;
                    TreeNode n = tree.visible.get(pos);
                    int id = idOfNode(tree, n);
                    if (tree.focusIdx != pos)
                    {
                        int old = tree.focusIdx;
                        tree.focusIdx = pos;
                        if (old >= 0) tree.adapter.notifyItemChanged(old);
                        tree.adapter.notifyItemChanged(pos);
                    }
                    /* SHOWDRAGDROP wins: kicks off internal node drag with id payload. */
                    if (tree.showDragDrop)
                    {
                        startTreeDrag(tree, h, id);
                        return;
                    }
                    /* RecyclerView owns gestures so OnLongClickListener never fires; pass row centre */
                    int dragX = h.itemView.getLeft() + h.itemView.getWidth() / 2;
                    int dragY = h.itemView.getTop()  + h.itemView.getHeight() / 2;
                    if (IupDragDropHelper.tryStartGenericDrag(tree, h.itemView, tree.ihandlePtr, dragX, dragY))
                        return;
                    int hadCb = dispatchRightClick(tree.ihandlePtr, id);
                    if (hadCb == 0 && tree.showRename) startRename(tree, id);
                }
            });

        h.itemView.setOnTouchListener((v, ev) -> gd.onTouchEvent(ev));

        h.chevron.setOnClickListener(v -> {
            int pos = h.getBindingAdapterPosition();
            if (pos == RecyclerView.NO_POSITION) return;
            TreeNode n = tree.visible.get(pos);
            if (n.kind == 0) toggleExpand(tree, n);
        });
    }


    /* SHOWDRAGDROP: long-press carries source node id in localState; drop walks RecyclerView children */
    @SuppressWarnings("deprecation")
    static void startTreeDrag(IupTreeView tree, NodeHolder h, int dragId)
    {
        android.content.ClipData data = android.content.ClipData.newPlainText("iuptreenode", String.valueOf(dragId));
        View.DragShadowBuilder shadow = new View.DragShadowBuilder(h.itemView);
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.N)
            h.itemView.startDragAndDrop(data, shadow, dragId, 0);
        else
            h.itemView.startDrag(data, shadow, dragId, 0);
    }

    static void installDropListener(final IupTreeView tree)
    {
        tree.setOnDragListener((v, ev) -> {
            int action = ev.getAction();
            /* Track hover position for the drop indicator (any drag, regardless of payload). */
            switch (action)
            {
                case android.view.DragEvent.ACTION_DRAG_LOCATION:
                case android.view.DragEvent.ACTION_DRAG_ENTERED:
                {
                    int idx = -1;
                    View child = tree.findChildViewUnder(ev.getX(), ev.getY());
                    if (child != null)
                    {
                        int pos = tree.getChildAdapterPosition(child);
                        if (pos != RecyclerView.NO_POSITION) idx = pos;
                    }
                    if (tree.dropIndicatorIdx != idx)
                    {
                        tree.dropIndicatorIdx = idx;
                        tree.invalidate();
                    }
                    break;
                }
                case android.view.DragEvent.ACTION_DRAG_EXITED:
                case android.view.DragEvent.ACTION_DRAG_ENDED:
                case android.view.DragEvent.ACTION_DROP:
                    if (tree.dropIndicatorIdx != -1)
                    {
                        tree.dropIndicatorIdx = -1;
                        tree.invalidate();
                    }
                    break;
            }

            Object state = ev.getLocalState();
            /* Internal SHOWDRAGDROP drag carries an Integer node id. */
            if (tree.showDragDrop && (state instanceof Integer || action == android.view.DragEvent.ACTION_DRAG_STARTED))
            {
                switch (action)
                {
                    case android.view.DragEvent.ACTION_DRAG_STARTED:
                        if (state instanceof Integer) return true;
                        break;
                    case android.view.DragEvent.ACTION_DRAG_ENTERED:
                    case android.view.DragEvent.ACTION_DRAG_LOCATION:
                    case android.view.DragEvent.ACTION_DRAG_EXITED:
                        if (state instanceof Integer) return true;
                        break;
                    case android.view.DragEvent.ACTION_DROP:
                        if (state instanceof Integer)
                        {
                            int dragId = (Integer) state;
                            View child = tree.findChildViewUnder(ev.getX(), ev.getY());
                            if (child == null) return false;
                            int pos = tree.getChildAdapterPosition(child);
                            if (pos == RecyclerView.NO_POSITION) return false;
                            int dropId = idOfNode(tree, tree.visible.get(pos));
                            if (dropId < 0) return false;
                            /* Android drag events don't carry shift/ctrl directly; use 0 for both. */
                            dispatchDragDrop(tree.ihandlePtr, dragId, dropId, 0, 0);
                            return true;
                        }
                        break;
                }
            }
            /* Generic IUP DROPTARGET / DROPFILESTARGET path for Payload or ClipData URIs. */
            return IupDragDropHelper.handleDragEvent(v, ev);
        });
    }

    /* Tint expanded branches (drop-into), line below leaves/collapsed (drop-after). */
    static final class DropIndicatorDecoration extends RecyclerView.ItemDecoration
    {
        @Override
        public void onDrawOver(@NonNull android.graphics.Canvas c, @NonNull RecyclerView parent, @NonNull RecyclerView.State state)
        {
            if (!(parent instanceof IupTreeView t)) return;
            if (t.dropIndicatorIdx < 0 || t.dropIndicatorIdx >= t.visible.size()) return;
            View child = t.getLayoutManager() != null ? t.getLayoutManager().findViewByPosition(t.dropIndicatorIdx) : null;
            if (child == null) return;

            TreeNode node = t.visible.get(t.dropIndicatorIdx);
            android.graphics.Paint p = new android.graphics.Paint();
            p.setColor(t.selectBg);
            if (node.kind == 0 && node.expanded)
            {
                p.setStyle(android.graphics.Paint.Style.FILL);
                p.setAlpha(80);
                c.drawRect(child.getLeft(), child.getTop(), child.getRight(), child.getBottom(), p);
            }
            else
            {
                float density = IupCommon.getDisplayDensity();
                p.setStrokeWidth(Math.max(2f, 2f * density));
                float y = child.getBottom();
                c.drawLine(0, y, parent.getWidth(), y, p);
            }
        }
    }

    static void toggleExpand(IupTreeView tree, TreeNode n)
    {
        n.expanded = !n.expanded;
        int id = idOfNode(tree, n);
        rebuildVisible(tree);
        tree.adapter.notifyDataSetChanged();
        if (n.expanded) dispatchBranchOpen(tree.ihandlePtr, id);
        else dispatchBranchClose(tree.ihandlePtr, id);
    }


    static int depthOf(TreeNode n)
    {
        int d = 0;
        TreeNode p = n.parent;
        while (p != null) { d++; p = p.parent; }
        return d;
    }


    static void rebuildVisible(IupTreeView t)
    {
        t.visible.clear();
        for (TreeNode r : t.roots) addVisibleRec(t, r);
    }

    static void addVisibleRec(IupTreeView t, TreeNode n)
    {
        t.visible.add(n);
        if (n.kind == 0 && n.expanded)
        {
            for (TreeNode c : n.children) addVisibleRec(t, c);
        }
    }


    /* depth-first pre-order matching IUP's id assignment in iupTreeAddToCache. */
    static int idOfNode(IupTreeView t, TreeNode target)
    {
        int[] cursor = new int[]{0};
        for (TreeNode r : t.roots)
        {
            int r2 = walkFindId(r, target, cursor);
            if (r2 >= 0) return r2;
        }
        return -1;
    }

    static int walkFindId(TreeNode n, TreeNode target, int[] cursor)
    {
        if (n == target) return cursor[0];
        cursor[0]++;
        for (TreeNode c : n.children)
        {
            int r = walkFindId(c, target, cursor);
            if (r >= 0) return r;
        }
        return -1;
    }


    static int totalDescendants(TreeNode n)
    {
        int total = 0;
        for (TreeNode c : n.children)
        {
            total++;
            total += totalDescendants(c);
        }
        return total;
    }

    static void applyNodeFont(IupTreeView tree, TextView tv, NodeFont nf)
    {
        if (nf == null)
        {
            tv.setTypeface(Typeface.DEFAULT);
            if (tree.defaultTitleTextSizePx > 0)
                tv.setTextSize(TypedValue.COMPLEX_UNIT_PX, tree.defaultTitleTextSizePx);
            return;
        }
        Typeface base = (nf.family != null && !nf.family.isEmpty())
            ? Typeface.create(nf.family, nf.style)
            : Typeface.create((Typeface) null, nf.style);
        tv.setTypeface(base);
        if (nf.sizeUnit != 0 && nf.size > 0) tv.setTextSize(nf.sizeUnit, nf.size);
    }

    static void notifyNodeChanged(IupTreeView tree, TreeNode n)
    {
        int idx = tree.visible.indexOf(n);
        if (idx >= 0) tree.adapter.notifyItemChanged(idx);
    }


    /* Public API for JNI. */

    @Keep
    public static View createTree(long ihandlePtr)
    {
        ContextThemeWrapper ctx = IupCommon.getContextThemeWrapper();
        return new IupTreeView(ctx, ihandlePtr);
    }

    @Keep
    public static Object addNode(View v, Object prevObj, int kind, String title, int add)
    {
        if (!(v instanceof IupTreeView t)) return null;

        TreeNode n = new TreeNode();
        n.kind = kind;
        n.title = title != null ? title : "";
        n.expanded = t.addExpanded;

        TreeNode prev = (prevObj instanceof TreeNode) ? (TreeNode) prevObj : null;
        if (prev == null)
        {
            t.roots.add(n);
        }
        else if (prev.kind == 0 && add == 1)
        {
            n.parent = prev;
            prev.children.add(0, n);
        }
        else
        {
            TreeNode parent = prev.parent;
            ArrayList<TreeNode> list = (parent != null) ? parent.children : t.roots;
            int idx = list.indexOf(prev);
            int insertAt;
            if (add == 1)
                insertAt = idx + 1;
            else if (prev.kind == 0)
                insertAt = idx + 1 + totalDescendants(prev);
            else
                insertAt = idx + 1;
            if (insertAt > list.size()) insertAt = list.size();
            n.parent = parent;
            list.add(insertAt, n);
        }

        rebuildVisible(t);
        t.adapter.notifyDataSetChanged();
        return n;
    }

    @Keep
    public static int getNodeKind(Object o)
    {
        return (o instanceof TreeNode) ? ((TreeNode) o).kind : 0;
    }

    @Keep
    public static int getNodeChildCount(Object o)
    {
        return (o instanceof TreeNode) ? ((TreeNode) o).children.size() : 0;
    }

    @Keep
    public static int getNodeTotalDescendants(Object o)
    {
        return (o instanceof TreeNode) ? totalDescendants((TreeNode) o) : 0;
    }

    @Keep
    public static int getNodeDepth(Object o)
    {
        return (o instanceof TreeNode) ? depthOf((TreeNode) o) : 0;
    }

    @Keep
    public static String getNodeTitle(Object o)
    {
        return (o instanceof TreeNode) ? ((TreeNode) o).title : null;
    }

    @Keep
    public static void setNodeTitle(View v, Object o, String title)
    {
        if (!(v instanceof IupTreeView t) || !(o instanceof TreeNode)) return;
        ((TreeNode) o).title = title != null ? title : "";
        int vis = t.visible.indexOf(o);
        if (vis >= 0) t.adapter.notifyItemChanged(vis);
    }

    @Keep
    public static boolean isNodeExpanded(Object o)
    {
        return (o instanceof TreeNode) && ((TreeNode) o).expanded;
    }

    @Keep
    public static void setNodeExpanded(View v, Object o, boolean exp)
    {
        if (!(v instanceof IupTreeView t) || !(o instanceof TreeNode n)) return;
        if (n.kind != 0) return;
        if (n.expanded == exp) return;
        n.expanded = exp;
        rebuildVisible(t);
        t.adapter.notifyDataSetChanged();
    }

    public static Object getFocusNode(View v)
    {
        if (!(v instanceof IupTreeView t)) return null;
        if (t.focusIdx >= 0 && t.focusIdx < t.visible.size()) return t.visible.get(t.focusIdx);
        return t.roots.isEmpty() ? null : t.roots.get(0);
    }

    @Keep
    public static void setFocusNodeById(View v, int id)
    {
        if (!(v instanceof IupTreeView t)) return;
        TreeNode target = nodeAtId(t, id);
        if (target == null) return;
        ensureVisible(target);
        rebuildVisible(t);
        int idx = t.visible.indexOf(target);
        if (idx < 0) return;
        int old = t.focusIdx;
        t.focusIdx = idx;
        t.adapter.notifyDataSetChanged();
        t.scrollToPosition(idx);
        if (old != idx)
            dispatchSelection(t.ihandlePtr, id, 1);
    }

    @Keep
    public static void scrollToId(View v, int id)
    {
        if (!(v instanceof IupTreeView t)) return;
        TreeNode target = nodeAtId(t, id);
        if (target == null) return;
        ensureVisible(target);
        rebuildVisible(t);
        t.adapter.notifyDataSetChanged();
        int idx = t.visible.indexOf(target);
        if (idx >= 0) t.scrollToPosition(idx);
    }

    /* DELNODE - scope: "SELECTED"|"MARKED"|"CHILDREN"|"ALL"|numeric id */
    @Keep
    public static void deleteNode(View v, int id, String scope)
    {
        if (!(v instanceof IupTreeView t)) return;

        if (scope != null && scope.equalsIgnoreCase("ALL"))
        {
            t.roots.clear();
            t.marked.clear();
            t.focusIdx = -1;
            t.markStart = null;
            rebuildVisible(t);
            t.adapter.notifyDataSetChanged();
            return;
        }

        if (scope != null && scope.equalsIgnoreCase("MARKED"))
        {
            ArrayList<TreeNode> victims = effectiveMarked(t);
            if (victims.isEmpty()) return;
            for (TreeNode n : victims) removeNodeTree(t, n);
            t.marked.clear();
            clampFocus(t);
            rebuildVisible(t);
            t.adapter.notifyDataSetChanged();
            return;
        }

        TreeNode target = nodeAtId(t, id);
        if (target == null) return;

        if (scope != null && scope.equalsIgnoreCase("CHILDREN"))
        {
            for (TreeNode c : new ArrayList<>(target.children)) removeNodeTree(t, c);
            target.children.clear();
        }
        else
        {
            removeNodeTree(t, target);
        }
        clampFocus(t);
        rebuildVisible(t);
        t.adapter.notifyDataSetChanged();
    }

    /* In MARK_MODE=SINGLE, MARKED falls back to the focused node (per IUP spec). */
    static ArrayList<TreeNode> effectiveMarked(IupTreeView t)
    {
        if (!t.marked.isEmpty()) return new ArrayList<>(t.marked);
        ArrayList<TreeNode> out = new ArrayList<>();
        if (!t.markMultiple && t.focusIdx >= 0 && t.focusIdx < t.visible.size())
            out.add(t.visible.get(t.focusIdx));
        return out;
    }

    @Keep
    public static int[] getMarkedTopLevelIdsDescending(View v)
    {
        if (!(v instanceof IupTreeView t)) return new int[0];
        ArrayList<TreeNode> marked = effectiveMarked(t);
        ArrayList<Integer> out = new ArrayList<>();
        for (TreeNode m : marked)
        {
            boolean ancestorMarked = false;
            for (TreeNode p = m.parent; p != null; p = p.parent)
            {
                if (marked.contains(p)) { ancestorMarked = true; break; }
            }
            if (!ancestorMarked) out.add(idOfNode(t, m));
        }
        out.sort(java.util.Collections.reverseOrder());
        int[] arr = new int[out.size()];
        for (int i = 0; i < arr.length; i++) arr[i] = out.get(i);
        return arr;
    }

    static void removeNodeTree(IupTreeView t, TreeNode n)
    {
        for (TreeNode c : new ArrayList<>(n.children)) removeNodeTree(t, c);
        n.children.clear();
        t.marked.remove(n);
        t.nodeColor.remove(n);
        t.nodeFont.remove(n);
        t.nodeImage.remove(n);
        t.nodeImageExpanded.remove(n);
        if (n.parent != null) n.parent.children.remove(n);
        else t.roots.remove(n);
        if (t.markStart == n) t.markStart = null;
    }

    static void clampFocus(IupTreeView t)
    {
        rebuildVisible(t);
        if (t.focusIdx >= t.visible.size()) t.focusIdx = t.visible.size() - 1;
        if (t.focusIdx < 0 && !t.visible.isEmpty()) t.focusIdx = 0;
    }


    /* In-place rename via AlertDialog (Android's natural rename UI). */

    @Keep
    public static void startRename(View v, int id)
    {
        if (!(v instanceof IupTreeView t)) return;
        final TreeNode n = nodeAtId(t, id);
        if (n == null) return;

        if (dispatchShowRename(t.ihandlePtr, id) == 1) return;

        Activity activity = IupCommon.getActivity(t);
        if (activity == null) activity = IupApplication.getIupApplication().getCurrentActivity();
        if (activity == null) return;

        final EditText input = new EditText(activity);
        String current = n.title != null ? n.title : "";
        input.setText(current);
        input.setSelection(current.length());
        input.setSingleLine(true);
        input.setImeOptions(EditorInfo.IME_ACTION_DONE);

        AlertDialog dlg = new AlertDialog.Builder(activity)
            .setTitle("Rename")
            .setView(input)
            .setPositiveButton(android.R.string.ok, (d, which) -> {
                String newTitle = input.getText().toString();
                int rejected = dispatchRename(t.ihandlePtr, id, newTitle);
                if (rejected == 0)
                {
                    n.title = newTitle;
                    notifyNodeChanged(t, n);
                }
            })
            .setNegativeButton(android.R.string.cancel, null)
            .create();
        dlg.show();
        input.requestFocus();
    }


    /* Per-node color / font / image. */

    @Keep
    public static void setNodeColor(View v, int id, int r, int g, int b)
    {
        if (!(v instanceof IupTreeView t)) return;
        TreeNode n = nodeAtId(t, id);
        if (n == null) return;
        if (r < 0 || g < 0 || b < 0) t.nodeColor.remove(n);
        else t.nodeColor.put(n, Color.rgb(r, g, b));
        notifyNodeChanged(t, n);
    }

    @Keep
    public static String getNodeColorStr(View v, int id)
    {
        if (!(v instanceof IupTreeView t)) return null;
        TreeNode n = nodeAtId(t, id);
        if (n == null) return null;
        Integer c = t.nodeColor.get(n);
        if (c == null) return null;
        return Color.red(c) + " " + Color.green(c) + " " + Color.blue(c);
    }

    @Keep
    public static void setNodeFont(View v, int id, String family, int style, int sizeUnit, float size)
    {
        if (!(v instanceof IupTreeView t)) return;
        TreeNode n = nodeAtId(t, id);
        if (n == null) return;
        if (family == null && size <= 0) t.nodeFont.remove(n);
        else t.nodeFont.put(n, new NodeFont(family, style, sizeUnit, size));
        notifyNodeChanged(t, n);
    }

    @Keep
    public static void setNodeImage(View v, int id, Bitmap bmp)
    {
        if (!(v instanceof IupTreeView t)) return;
        TreeNode n = nodeAtId(t, id);
        if (n == null) return;
        if (bmp == null) t.nodeImage.remove(n);
        else t.nodeImage.put(n, bmp);
        notifyNodeChanged(t, n);
    }

    @Keep
    public static void setNodeImageExpanded(View v, int id, Bitmap bmp)
    {
        if (!(v instanceof IupTreeView t)) return;
        TreeNode n = nodeAtId(t, id);
        if (n == null) return;
        if (bmp == null) t.nodeImageExpanded.remove(n);
        else t.nodeImageExpanded.put(n, bmp);
        notifyNodeChanged(t, n);
    }

    @Keep
    public static void setDefaultImageLeaf(View v, Bitmap bmp)
    {
        if (v instanceof IupTreeView) { ((IupTreeView) v).defaultImageLeaf = bmp; ((IupTreeView) v).adapter.notifyDataSetChanged(); }
    }

    @Keep
    public static void setDefaultImageBranchCollapsed(View v, Bitmap bmp)
    {
        if (v instanceof IupTreeView) { ((IupTreeView) v).defaultImageBranchCollapsed = bmp; ((IupTreeView) v).adapter.notifyDataSetChanged(); }
    }

    @Keep
    public static void setDefaultImageBranchExpanded(View v, Bitmap bmp)
    {
        if (v instanceof IupTreeView) { ((IupTreeView) v).defaultImageBranchExpanded = bmp; ((IupTreeView) v).adapter.notifyDataSetChanged(); }
    }


    /* MARKED state. */

    @Keep
    public static boolean isNodeMarked(Object o, View v)
    {
        if (!(v instanceof IupTreeView) || !(o instanceof TreeNode)) return false;
        return ((IupTreeView) v).marked.contains(o);
    }

    @Keep
    public static void setNodeMarked(View v, int id, boolean mark)
    {
        if (!(v instanceof IupTreeView t)) return;
        TreeNode n = nodeAtId(t, id);
        if (n == null) return;
        if (mark) t.marked.add(n);
        else t.marked.remove(n);
        int vis = t.visible.indexOf(n);
        if (vis >= 0) t.adapter.notifyItemChanged(vis);
    }

    @Keep
    public static void setMarkMultiple(View v, boolean multi)
    {
        if (!(v instanceof IupTreeView t)) return;
        t.markMultiple = multi;
        if (!multi) t.marked.clear();
        t.adapter.notifyDataSetChanged();
    }

    @Keep
    public static void markAll(View v, boolean mark)
    {
        if (!(v instanceof IupTreeView t)) return;
        t.marked.clear();
        if (mark)
        {
            for (TreeNode r : t.roots) markAllRec(r, t.marked);
        }
        t.adapter.notifyDataSetChanged();
    }

    static void markAllRec(TreeNode n, HashSet<TreeNode> set)
    {
        set.add(n);
        for (TreeNode c : n.children) markAllRec(c, set);
    }

    @Keep
    public static void invertMarks(View v)
    {
        if (!(v instanceof IupTreeView t)) return;
        HashSet<TreeNode> all = new HashSet<>();
        for (TreeNode r : t.roots) markAllRec(r, all);
        HashSet<TreeNode> prev = new HashSet<>(t.marked);
        t.marked.clear();
        for (TreeNode n : all) if (!prev.contains(n)) t.marked.add(n);
        t.adapter.notifyDataSetChanged();
    }

    @Keep
    public static void markBlock(View v, int fromId, int toId)
    {
        if (!(v instanceof IupTreeView t)) return;
        int a = Math.min(fromId, toId), b = Math.max(fromId, toId);
        for (int i = a; i <= b; i++)
        {
            TreeNode n = nodeAtId(t, i);
            if (n != null) t.marked.add(n);
        }
        t.adapter.notifyDataSetChanged();
    }

    @Keep
    public static void setMarkStart(View v, int id)
    {
        if (!(v instanceof IupTreeView t)) return;
        t.markStart = nodeAtId(t, id);
    }

    @Keep
    public static String getMarkedNodesStr(View v)
    {
        if (!(v instanceof IupTreeView t)) return "";
        int total = 0;
        for (TreeNode r : t.roots) total += 1 + totalDescendants(r);
        char[] buf = new char[total];
        int[] cursor = new int[]{0};
        for (TreeNode r : t.roots) markedStrRec(r, t.marked, buf, cursor);
        return new String(buf);
    }

    static void markedStrRec(TreeNode n, HashSet<TreeNode> set, char[] buf, int[] cursor)
    {
        buf[cursor[0]++] = set.contains(n) ? '+' : '-';
        for (TreeNode c : n.children) markedStrRec(c, set, buf, cursor);
    }

    @Keep
    public static void setMarkedNodesStr(View v, String str)
    {
        if (!(v instanceof IupTreeView t) || str == null) return;
        t.marked.clear();
        int[] cursor = new int[]{0};
        for (TreeNode r : t.roots) setMarkedRec(r, str, cursor, t.marked);
        t.adapter.notifyDataSetChanged();
    }

    static void setMarkedRec(TreeNode n, String str, int[] cursor, HashSet<TreeNode> set)
    {
        if (cursor[0] < str.length() && str.charAt(cursor[0]) == '+') set.add(n);
        cursor[0]++;
        for (TreeNode c : n.children) setMarkedRec(c, str, cursor, set);
    }


    /* Navigation: resolve id from keyword (ROOT/LAST/NEXT/PREV/HOME/END/PARENT/BLOCK/CLEAR). */
    @Keep
    public static int resolveNavId(View v, int currentId, String keyword)
    {
        if (!(v instanceof IupTreeView t)) return -1;
        int total = 0;
        for (TreeNode r : t.roots) total += 1 + totalDescendants(r);
        if (total == 0) return -1;
        if (keyword == null) return -1;

        if (keyword.equalsIgnoreCase("ROOT") || keyword.equalsIgnoreCase("FIRST") || keyword.equalsIgnoreCase("HOME"))
            return 0;
        if (keyword.equalsIgnoreCase("LAST") || keyword.equalsIgnoreCase("END"))
            return total - 1;
        if (keyword.equalsIgnoreCase("NEXT") || keyword.equalsIgnoreCase("DOWN"))
            return Math.min(currentId + 1, total - 1);
        if (keyword.equalsIgnoreCase("PREVIOUS") || keyword.equalsIgnoreCase("PREV") || keyword.equalsIgnoreCase("UP"))
            return Math.max(currentId - 1, 0);
        if (keyword.equalsIgnoreCase("PGUP"))
            return Math.max(currentId - 10, 0);
        if (keyword.equalsIgnoreCase("PGDN"))
            return Math.min(currentId + 10, total - 1);
        if (keyword.equalsIgnoreCase("PARENT"))
        {
            TreeNode n = nodeAtId(t, currentId);
            if (n == null || n.parent == null) return -1;
            return idOfNode(t, n.parent);
        }
        return -1;
    }

    @Keep
    public static int getFocusNodeId(View v)
    {
        if (!(v instanceof IupTreeView t)) return -1;
        Object n = getFocusNode(v);
        return (n instanceof TreeNode) ? idOfNode(t, (TreeNode) n) : -1;
    }

    @Keep
    public static void setAddExpanded(View v, boolean exp)
    {
        if (v instanceof IupTreeView) ((IupTreeView) v).addExpanded = exp;
    }

    @Keep
    public static void expandAll(View v, boolean exp)
    {
        if (!(v instanceof IupTreeView t)) return;
        for (TreeNode r : t.roots) setExpandedRec(r, exp);
        rebuildVisible(t);
        t.adapter.notifyDataSetChanged();
    }

    static void setExpandedRec(TreeNode n, boolean exp)
    {
        if (n.kind == 0) n.expanded = exp;
        for (TreeNode c : n.children) setExpandedRec(c, exp);
    }

    static void ensureVisible(TreeNode n)
    {
        TreeNode p = n.parent;
        while (p != null) { p.expanded = true; p = p.parent; }
    }

    @Keep
    public static int getRootCount(View v)
    {
        return (v instanceof IupTreeView) ? ((IupTreeView) v).roots.size() : 0;
    }


    /* PARENT/FIRST/LAST/NEXT/PREVIOUS: same-depth siblings, -1 at boundaries. */
    private static ArrayList<TreeNode> siblingsOf(IupTreeView t, TreeNode n)
    {
        return n.parent != null ? n.parent.children : t.roots;
    }

    @Keep
    public static int getParentNodeId(View v, int id)
    {
        if (!(v instanceof IupTreeView t)) return -1;
        TreeNode n = nodeAtId(t, id);
        if (n == null || n.parent == null) return -1;
        return idOfNode(t, n.parent);
    }

    @Keep
    public static int getFirstNodeId(View v, int id)
    {
        if (!(v instanceof IupTreeView t)) return -1;
        TreeNode n = nodeAtId(t, id);
        if (n == null) return -1;
        ArrayList<TreeNode> sibs = siblingsOf(t, n);
        if (sibs.isEmpty()) return -1;
        return idOfNode(t, sibs.get(0));
    }

    @Keep
    public static int getLastNodeId(View v, int id)
    {
        if (!(v instanceof IupTreeView t)) return -1;
        TreeNode n = nodeAtId(t, id);
        if (n == null) return -1;
        ArrayList<TreeNode> sibs = siblingsOf(t, n);
        if (sibs.isEmpty()) return -1;
        return idOfNode(t, sibs.get(sibs.size() - 1));
    }

    @Keep
    public static int getNextNodeId(View v, int id)
    {
        if (!(v instanceof IupTreeView t)) return -1;
        TreeNode n = nodeAtId(t, id);
        if (n == null) return -1;
        ArrayList<TreeNode> sibs = siblingsOf(t, n);
        int idx = sibs.indexOf(n);
        if (idx < 0 || idx + 1 >= sibs.size()) return -1;
        return idOfNode(t, sibs.get(idx + 1));
    }

    @Keep
    public static int getPreviousNodeId(View v, int id)
    {
        if (!(v instanceof IupTreeView t)) return -1;
        TreeNode n = nodeAtId(t, id);
        if (n == null) return -1;
        ArrayList<TreeNode> sibs = siblingsOf(t, n);
        int idx = sibs.indexOf(n);
        if (idx <= 0) return -1;
        return idOfNode(t, sibs.get(idx - 1));
    }


    /* Total node count (depth-first), used by C cache rebuilds after MOVENODE/COPYNODE. */
    @Keep
    public static int getTotalNodeCount(View v)
    {
        if (!(v instanceof IupTreeView t)) return 0;
        int total = 0;
        for (TreeNode r : t.roots) total += 1 + totalDescendants(r);
        return total;
    }

    /* Returns the TreeNode at depth-first id, or null. */
    @Keep
    public static Object getNodeAtIdForCache(View v, int id)
    {
        if (!(v instanceof IupTreeView t)) return null;
        return nodeAtId(t, id);
    }


    /* MOVENODE/COPYNODE: returns depth-first id of result, or -1 on rejection */
    @Keep
    public static int moveNode(View v, int srcId, int dstId)
    {
        if (!(v instanceof IupTreeView t)) return -1;
        TreeNode src = nodeAtId(t, srcId);
        TreeNode dst = nodeAtId(t, dstId);
        if (src == null || dst == null || src == dst) return -1;
        /* Reject moving a node into its own subtree. */
        for (TreeNode p = dst; p != null; p = p.parent)
            if (p == src) return -1;

        /* Detach src. */
        ArrayList<TreeNode> srcSibs = siblingsOf(t, src);
        srcSibs.remove(src);

        insertAtDestination(t, src, dst);
        clampFocus(t);
        rebuildVisible(t);
        t.adapter.notifyDataSetChanged();
        return idOfNode(t, src);
    }

    @Keep
    public static int copyNode(View v, int srcId, int dstId)
    {
        if (!(v instanceof IupTreeView t)) return -1;
        TreeNode src = nodeAtId(t, srcId);
        TreeNode dst = nodeAtId(t, dstId);
        if (src == null || dst == null || src == dst) return -1;
        for (TreeNode p = dst; p != null; p = p.parent)
            if (p == src) return -1;

        TreeNode clone = deepCloneNode(src);
        insertAtDestination(t, clone, dst);
        rebuildVisible(t);
        t.adapter.notifyDataSetChanged();
        return idOfNode(t, clone);
    }

    /* Cross-tree subtree clone for iupdrvTreeDragDropCopyNode. */
    @Keep
    public static int copyNodeAcross(View srcView, int srcId, View dstView, int dstId)
    {
        if (!(srcView instanceof IupTreeView srcTree)) return -1;
        if (!(dstView instanceof IupTreeView dstTree)) return -1;
        TreeNode src = nodeAtId(srcTree, srcId);
        TreeNode dst = nodeAtId(dstTree, dstId);
        if (src == null || dst == null) return -1;

        TreeNode clone = deepCloneNode(src);
        copyNodeSideTable(srcTree, src, dstTree, clone);
        insertAtDestination(dstTree, clone, dst);
        rebuildVisible(dstTree);
        dstTree.adapter.notifyDataSetChanged();
        return idOfNode(dstTree, clone);
    }

    /* per-node color/font/image live in side maps; re-key onto the cloned dst node */
    static void copyNodeSideTable(IupTreeView srcTree, TreeNode src, IupTreeView dstTree, TreeNode clone)
    {
        Integer color = srcTree.nodeColor.get(src);
        if (color != null) dstTree.nodeColor.put(clone, color);
        NodeFont font = srcTree.nodeFont.get(src);
        if (font != null) dstTree.nodeFont.put(clone, font);
        Bitmap img = srcTree.nodeImage.get(src);
        if (img != null) dstTree.nodeImage.put(clone, img);
        Bitmap imgExp = srcTree.nodeImageExpanded.get(src);
        if (imgExp != null) dstTree.nodeImageExpanded.put(clone, imgExp);
        for (int i = 0; i < src.children.size() && i < clone.children.size(); i++)
            copyNodeSideTable(srcTree, src.children.get(i), dstTree, clone.children.get(i));
    }

    /* Pixel x,y -> depth-first node id, -1 if none. */
    @Keep
    public static int nodeIdAt(View v, int x, int y)
    {
        if (!(v instanceof IupTreeView t)) return -1;
        View child = t.findChildViewUnder(x, y);
        if (child == null) return -1;
        int pos = t.getChildAdapterPosition(child);
        if (pos == RecyclerView.NO_POSITION || pos < 0 || pos >= t.visible.size()) return -1;
        return idOfNode(t, t.visible.get(pos));
    }

    /* expanded branch -> first child, else next sibling of dst */
    static void insertAtDestination(IupTreeView t, TreeNode node, TreeNode dst)
    {
        if (dst.kind == 0 && dst.expanded)
        {
            node.parent = dst;
            dst.children.add(0, node);
            return;
        }
        ArrayList<TreeNode> dstSibs = siblingsOf(t, dst);
        int idx = dstSibs.indexOf(dst);
        node.parent = dst.parent;
        dstSibs.add(idx + 1, node);
    }

    static TreeNode deepCloneNode(TreeNode src)
    {
        TreeNode c = new TreeNode();
        c.kind = src.kind;
        c.title = src.title;
        c.expanded = src.expanded;
        for (TreeNode child : src.children)
        {
            TreeNode cc = deepCloneNode(child);
            cc.parent = c;
            c.children.add(cc);
        }
        return c;
    }


    /* id-based walk: depth-first pre-order. */
    static TreeNode nodeAtId(IupTreeView t, int id)
    {
        if (id < 0) return null;
        int[] cursor = new int[]{0};
        for (TreeNode r : t.roots)
        {
            TreeNode n = walkAt(r, id, cursor);
            if (n != null) return n;
        }
        return null;
    }

    static TreeNode walkAt(TreeNode n, int id, int[] cursor)
    {
        if (cursor[0] == id) return n;
        cursor[0]++;
        for (TreeNode c : n.children)
        {
            TreeNode r = walkAt(c, id, cursor);
            if (r != null) return r;
        }
        return null;
    }


    @Keep
    public static void setShowRename(View v, boolean enable)
    {
        if (v instanceof IupTreeView) ((IupTreeView) v).showRename = enable;
    }

    @Keep
    public static void setShowDragDrop(View v, boolean enable)
    {
        if (v instanceof IupTreeView) ((IupTreeView) v).showDragDrop = enable;
    }


    /* ---- JNI bridge ---- */
    public static native void dispatchExecuteLeaf(long ihandlePtr, int id);
    public static native void dispatchBranchOpen(long ihandlePtr, int id);
    public static native void dispatchBranchClose(long ihandlePtr, int id);
    public static native void dispatchSelection(long ihandlePtr, int id, int status);
    public static native int  dispatchRightClick(long ihandlePtr, int id);
    public static native int  dispatchShowRename(long ihandlePtr, int id);
    public static native int  dispatchRename(long ihandlePtr, int id, String title);
    public static native void dispatchDragDrop(long ihandlePtr, int dragId, int dropId, int isShift, int isControl);
}
