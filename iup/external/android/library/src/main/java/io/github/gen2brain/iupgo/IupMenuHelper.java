package io.github.gen2brain.iupgo;

import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.ShapeDrawable;
import android.graphics.drawable.shapes.PathShape;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.Menu;
import android.view.MenuItem;
import android.view.SubMenu;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.PopupMenu;
import android.widget.PopupWindow;
import android.widget.TextView;

import androidx.annotation.Keep;
import androidx.recyclerview.widget.RecyclerView;


/* IupMenu backend; action-bar submenus share the PopupWindow drill-down with IupPopup. */
public final class IupMenuHelper
{
    private static final String TAG = "Iup";

    private IupMenuHelper() {}

    static {
        IupTheme.register(IupMenuHelper::onThemeChanged);
    }

    private static void onThemeChanged()
    {
        Activity a = IupApplication.getIupApplication().getCurrentActivity();
        if (a != null) a.invalidateOptionsMenu();
        repaintNavigationView();
        rebuildNavigationDrawer();
    }

    private static void repaintNavigationView()
    {
        if (sNavigationView == null) return;
        android.content.Context ctx = IupCommon.getThemeContext();
        int onSurface = com.google.android.material.color.MaterialColors.getColor(
            ctx, com.google.android.material.R.attr.colorOnSurface, android.graphics.Color.BLACK);
        int surface = com.google.android.material.color.MaterialColors.getColor(
            ctx, com.google.android.material.R.attr.colorSurface, android.graphics.Color.WHITE);
        sNavigationView.setBackgroundColor(surface);
        sNavigationView.setItemTextColor(android.content.res.ColorStateList.valueOf(onSurface));
    }

    public static final int TYPE_UNKNOWN   = 0;
    public static final int TYPE_MENU      = 1;
    public static final int TYPE_SUBMENU   = 2;
    public static final int TYPE_ITEM      = 3;
    public static final int TYPE_SEPARATOR = 4;

    /* Offset to avoid clashing with Android standard ids. */
    private static final int ID_BASE = 0x42000;


    @Keep
    public static void attachToActivity(Object handle, long menuIhandle)
    {
        Activity a = resolveActivity(handle);
        if (a instanceof IupActivity)
        {
            ((IupActivity) a).setMenuIhandle(menuIhandle);
            a.invalidateOptionsMenu();
        }
    }

    @Keep
    public static void detachFromActivity(Object handle)
    {
        Activity a = resolveActivity(handle);
        if (a instanceof IupActivity)
        {
            ((IupActivity) a).setMenuIhandle(0);
            a.invalidateOptionsMenu();
        }
    }

    @Keep
    public static void attachDrawer(Object handle, long drawerMenuIh)
    {
        Activity a = resolveActivity(handle);
        if (a instanceof IupActivity) ((IupActivity) a).setDrawerIhandle(drawerMenuIh);
    }

    @Keep
    public static void detachDrawer(Object handle)
    {
        Activity a = resolveActivity(handle);
        if (a instanceof IupActivity) ((IupActivity) a).setDrawerIhandle(0);
    }

    private static Activity resolveActivity(Object handle)
    {
        if (handle instanceof Activity) return (Activity) handle;
        return IupApplication.getIupApplication().getCurrentActivity();
    }


    public static void buildActionBarMenu(Menu androidMenu, long menuIhandle)
    {
        if (menuIhandle == 0) return;
        appendRecentItems(androidMenu, menuIhandle);
        int count = nativeGetChildCount(menuIhandle);
        for (int i = 0; i < count; i++)
        {
            long childIh = nativeGetChild(menuIhandle, i);
            if (childIh == 0) continue;
            int type = nativeGetType(childIh);
            if (type == TYPE_ITEM) addItem(androidMenu, childIh);
            else if (type == TYPE_SUBMENU) addSubmenu(androidMenu, childIh);
            /* Overflow menu has no separator primitive pre API 28. */
        }
    }

    /* Recent entries stored on the menu Ihandle; prepend them and route to RECENT_CB. */
    private static void appendRecentItems(Menu androidMenu, final long menuIh)
    {
        int n = nativeGetRecentCount(menuIh);
        for (int i = 0; i < n; i++)
        {
            String path = nativeGetRecentFile(menuIh, i);
            if (path == null || path.isEmpty()) continue;
            final int index = i;
            MenuItem mi = androidMenu.add(Menu.NONE, ID_BASE, Menu.NONE, recentDisplayName(path));
            mi.setOnMenuItemClickListener(m -> {
                new Handler(Looper.getMainLooper()).post(() -> nativeDispatchRecent(menuIh, index));
                return true;
            });
        }
    }

    /* SAF display name for content:// URIs; trailing path segment otherwise. */
    private static String recentDisplayName(String filename)
    {
        if (filename.startsWith("content://"))
        {
            Activity a = IupApplication.getIupApplication().getCurrentActivity();
            if (a != null)
            {
                try (android.database.Cursor c = a.getContentResolver().query(
                    android.net.Uri.parse(filename), null, null, null, null))
                {
                    if (c != null && c.moveToFirst())
                    {
                        int col = c.getColumnIndex(android.provider.OpenableColumns.DISPLAY_NAME);
                        if (col >= 0)
                        {
                            String dn = c.getString(col);
                            if (dn != null && !dn.isEmpty()) return dn;
                        }
                    }
                }
                catch (Exception ignore) { }
            }
            int q = filename.indexOf('?');
            String s = q >= 0 ? filename.substring(0, q) : filename;
            int slash = s.lastIndexOf('/');
            return slash >= 0 ? s.substring(slash + 1) : s;
        }
        int slash = filename.lastIndexOf('/');
        return slash >= 0 ? filename.substring(slash + 1) : filename;
    }


    /* IupPopup is synchronous on other drivers; match semantics with pumpUntilDone. */
    @Keep
    public static void popup(long menuIhandle, int x, int y)
    {
        Activity a = IupApplication.getIupApplication().getCurrentActivity();
        if (a == null || menuIhandle == 0) return;

        FrameLayout content = a.findViewById(android.R.id.content);
        /* Deep nesting + missing content fall back to the hand-rolled drill-down. */
        if (content == null || hasNestedSubmenus(menuIhandle))
        {
            boolean[] fallback = { false };
            showPopupWindow(a, menuIhandle, Gravity.TOP | Gravity.START, x, y, fallback);
            IupCommon.pumpUntilDone(fallback);
            return;
        }

        int[] rootLoc = new int[2];
        content.getLocationOnScreen(rootLoc);
        int viewX = Math.max(0, x - rootLoc[0]);
        int viewY = Math.max(0, y - rootLoc[1]);

        View anchor = new View(a);
        FrameLayout.LayoutParams alp = new FrameLayout.LayoutParams(1, 1);
        alp.leftMargin = viewX;
        alp.topMargin = viewY;
        anchor.setLayoutParams(alp);
        content.addView(anchor);

        PopupMenu pm = new PopupMenu(a, anchor, Gravity.START | Gravity.TOP);
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.Q)
            pm.setForceShowIcon(true);

        long[] picked = { 0 };
        Menu menu = pm.getMenu();
        int count = nativeGetChildCount(menuIhandle);
        for (int i = 0; i < count; i++)
        {
            long childIh = nativeGetChild(menuIhandle, i);
            if (childIh == 0) continue;
            int type = nativeGetType(childIh);
            if (type == TYPE_ITEM) addItem(menu, childIh, picked);
            else if (type == TYPE_SUBMENU) addSubmenu(menu, childIh, picked);
            /* Native Menu has no separator pre API 28; ignore. */
        }

        boolean[] done = { false };
        ViewGroup parent = content;
        pm.setOnDismissListener(p -> {
            parent.removeView(anchor);
            if (picked[0] != 0) nativeDispatchAction(picked[0]);
            done[0] = true;
        });

        pm.show();
        IupCommon.pumpUntilDone(done);
    }

    private static void showPopupWindow(final Activity a, final long menuIh, final int gravity, final int x, final int y, final boolean[] done)
    {
        final float density = IupCommon.getDisplayDensity();
        final int padV = Math.round(4 * density);
        final int padH = Math.round(8 * density);
        final int rowH = Math.round(44 * density);
        final int minW = Math.round(160 * density);

        /* colorSurface/textColorPrimary match the native overflow menu; MENUBGCOLOR is IUP's legacy gray. */
        final int bgColor = IupCommon.resolveThemeColorByName("colorSurface", IupCommon.paletteMenuBg);
        final int fgColor = IupCommon.resolveThemeColorByName("textColorPrimary", IupCommon.paletteMenuFg);

        LinearLayout content = new LinearLayout(a);
        content.setOrientation(LinearLayout.VERTICAL);
        content.setMinimumWidth(minW);
        content.setPadding(0, padV, 0, padV);

        GradientDrawable bg = new GradientDrawable();
        bg.setColor(bgColor);
        bg.setCornerRadius(4 * density);
        content.setBackground(bg);

        final PopupWindow pw = new PopupWindow(content,
            ViewGroup.LayoutParams.WRAP_CONTENT,
            ViewGroup.LayoutParams.WRAP_CONTENT,
            true);
        pw.setBackgroundDrawable(new ColorDrawable(0x00000000));
        pw.setOutsideTouchable(true);
        pw.setElevation(8 * density);

        final boolean[] drilled = { false };

        int recentN = nativeGetRecentCount(menuIh);
        for (int i = 0; i < recentN; i++)
        {
            String path = nativeGetRecentFile(menuIh, i);
            if (path == null || path.isEmpty()) continue;
            final int index = i;

            TextView row = new TextView(a);
            row.setText(recentDisplayName(path));
            row.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
            row.setTextColor(fgColor);
            row.setPadding(padH * 2, 0, padH * 2, 0);
            row.setGravity(Gravity.CENTER_VERTICAL | Gravity.START);
            row.setSingleLine(true);
            row.setClickable(true);
            row.setFocusable(true);
            row.setLayoutParams(new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, rowH));
            applyRowRipple(row);
            /* pw.dismiss() detaches v; main-looper Handler so the post survives. */
            row.setOnClickListener(v -> {
                pw.dismiss();
                new Handler(Looper.getMainLooper()).post(() -> nativeDispatchRecent(menuIh, index));
            });
            content.addView(row);
        }

        int count = nativeGetChildCount(menuIh);
        for (int i = 0; i < count; i++)
        {
            final long childIh = nativeGetChild(menuIh, i);
            int type = nativeGetType(childIh);
            if (type == TYPE_SEPARATOR)
            {
                View sep = new View(a);
                sep.setBackgroundColor(IupCommon.blendColor(bgColor, fgColor, 0.25f));
                LinearLayout.LayoutParams slp = new LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, Math.max(1, Math.round(density)));
                int m = Math.round(4 * density);
                slp.topMargin = m; slp.bottomMargin = m;
                sep.setLayoutParams(slp);
                content.addView(sep);
                continue;
            }
            if (type != TYPE_ITEM && type != TYPE_SUBMENU) continue;

            String title = nativeGetTitle(childIh);
            final boolean isSubmenu = (type == TYPE_SUBMENU);
            final boolean isEnabled = nativeIsActive(childIh);

            TextView row = new TextView(a);
            row.setText(title != null ? title : "");
            row.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
            row.setTextColor(isEnabled
                ? fgColor
                : IupCommon.blendColor(bgColor, fgColor, 0.45f));
            row.setPadding(padH * 2, 0, padH * 2, 0);
            row.setGravity(Gravity.CENTER_VERTICAL | Gravity.START);
            row.setSingleLine(true);
            row.setClickable(isEnabled);
            row.setFocusable(isEnabled);
            LinearLayout.LayoutParams rlp = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, rowH);
            row.setLayoutParams(rlp);
            if (isEnabled) applyRowRipple(row);

            Drawable icon = resolveItemIcon(childIh, a);
            if (icon != null)
            {
                int iconSize = Math.round(20 * density);
                icon.setBounds(0, 0, iconSize, iconSize);
            }
            Drawable chevron = null;
            if (isSubmenu)
            {
                int chevronSize = Math.round(18 * density);
                chevron = makeChevronRight(isEnabled ? fgColor : IupCommon.blendColor(bgColor, fgColor, 0.45f), chevronSize);
                chevron.setBounds(0, 0, chevronSize, chevronSize);
            }
            if (icon != null || chevron != null)
            {
                row.setCompoundDrawables(icon, null, chevron, null);
                row.setCompoundDrawablePadding(padH);
            }

            row.setOnClickListener(v -> {
                if (isSubmenu)
                {
                    drilled[0] = true;
                    pw.dismiss();
                    final long inner = submenuInnerMenu(childIh);
                    if (inner != 0)
                    {
                        a.runOnUiThread(() -> showPopupWindow(a, inner, gravity, x, y, done));
                    }
                    else
                    {
                        done[0] = true;
                    }
                }
                else
                {
                    pw.dismiss();
                    /* pw.dismiss() detaches v; main-looper Handler so the post survives. */
                    new Handler(Looper.getMainLooper()).post(() -> nativeDispatchAction(childIh));
                }
            });

            content.addView(row);
        }

        if (content.getChildCount() == 0) { done[0] = true; return; }

        pw.setOnDismissListener(() -> { if (!drilled[0]) done[0] = true; });

        View anchor = a.getWindow().getDecorView();
        int safeX = Math.max(0, x);
        int safeY = Math.max(0, y);
        pw.showAtLocation(anchor, gravity, safeX, safeY);
    }

    private static void applyRowRipple(TextView row)
    {
        android.util.TypedValue tv = new android.util.TypedValue();
        row.getContext().getTheme().resolveAttribute(
            android.R.attr.selectableItemBackground, tv, true);
        if (tv.resourceId != 0)
            row.setBackgroundResource(tv.resourceId);
    }

    private static Drawable makeChevronRight(int color, int sizePx)
    {
        Path p = new Path();
        p.moveTo(9f, 6f);
        p.lineTo(15f, 12f);
        p.lineTo(9f, 18f);
        p.close();
        ShapeDrawable sd = new ShapeDrawable(new PathShape(p, 24f, 24f));
        Paint paint = sd.getPaint();
        paint.setColor(color);
        paint.setStyle(Paint.Style.FILL);
        paint.setAntiAlias(true);
        sd.setIntrinsicWidth(sizePx);
        sd.setIntrinsicHeight(sizePx);
        return sd;
    }


    private static void addItem(Menu androidMenu, final long itemIh)
    {
        addItem(androidMenu, itemIh, null);
    }

    private static void addItem(Menu androidMenu, final long itemIh, final long[] pickedSlot)
    {
        String title = nativeGetTitle(itemIh);
        MenuItem mi = androidMenu.add(Menu.NONE, ID_BASE, Menu.NONE, title != null ? title : "");
        mi.setEnabled(nativeIsActive(itemIh));
        Activity a = IupApplication.getIupApplication().getCurrentActivity();
        if (a != null)
        {
            Drawable icon = resolveItemIcon(itemIh, a);
            if (icon != null) mi.setIcon(icon);
        }
        /* RADIO members are always checkable so all radios render the indicator from the start. */
        String value = nativeGetStringAttribute(itemIh, "VALUE");
        boolean isRadio = nativeIsParentRadio(itemIh);
        if (isRadio || (value != null && !nativeGetBoolAttribute(itemIh, "HIDEMARK")))
        {
            mi.setCheckable(true);
            mi.setChecked("ON".equalsIgnoreCase(value));
        }
        mi.setOnMenuItemClickListener(m -> {
            if (pickedSlot != null)
                pickedSlot[0] = itemIh;
            else
                new Handler(Looper.getMainLooper()).post(() -> nativeDispatchAction(itemIh));
            return true;
        });
    }

    /* IUP Submenu's first child is the inner Menu whose children are the visible items. */
    private static long submenuInnerMenu(long submenuIh)
    {
        if (nativeGetChildCount(submenuIh) == 0) return 0;
        long inner = nativeGetChild(submenuIh, 0);
        if (nativeGetType(inner) != TYPE_MENU) return 0;
        return inner;
    }

    /* Native addSubMenu for the chevron; PopupWindow fallback when IUP nests deeper than one level. */
    private static void addSubmenu(Menu androidMenu, final long submenuIh)
    {
        addSubmenu(androidMenu, submenuIh, null);
    }

    private static void addSubmenu(Menu androidMenu, final long submenuIh, final long[] pickedSlot)
    {
        final long innerMenu = submenuInnerMenu(submenuIh);
        String title = nativeGetTitle(submenuIh);
        Activity actx = IupApplication.getIupApplication().getCurrentActivity();

        if (innerMenu != 0 && !hasNestedSubmenus(innerMenu))
        {
            SubMenu sub = androidMenu.addSubMenu(Menu.NONE, ID_BASE, Menu.NONE, title != null ? title : "");
            MenuItem parent = sub.getItem();
            parent.setEnabled(nativeIsActive(submenuIh));
            if (actx != null)
            {
                Drawable icon = resolveItemIcon(submenuIh, actx);
                if (icon != null) parent.setIcon(icon);
            }

            int count = nativeGetChildCount(innerMenu);
            for (int i = 0; i < count; i++)
            {
                long childIh = nativeGetChild(innerMenu, i);
                if (childIh == 0) continue;
                int type = nativeGetType(childIh);
                if (type == TYPE_ITEM) addItem(sub, childIh, pickedSlot);
                /* SubMenu children do not support icons or further nesting per Android spec. */
            }
            return;
        }

        /* TabStopSpan pushes ▶ to the right edge; framework's submenuarrow needs a real SubMenu attached. */
        String t = title != null ? title : "";
        android.text.SpannableStringBuilder sb = new android.text.SpannableStringBuilder(t + "\t▶");
        sb.setSpan(new android.text.style.TabStopSpan.Standard(Math.round(192 * IupCommon.getDisplayDensity())),
                   0, sb.length(), android.text.Spanned.SPAN_INCLUSIVE_INCLUSIVE);
        MenuItem mi = androidMenu.add(Menu.NONE, ID_BASE, Menu.NONE, sb);
        mi.setEnabled(nativeIsActive(submenuIh));
        if (actx != null)
        {
            Drawable icon = resolveItemIcon(submenuIh, actx);
            if (icon != null) mi.setIcon(icon);
        }
        mi.setOnMenuItemClickListener(m -> {
            final Activity a = IupApplication.getIupApplication().getCurrentActivity();
            if (a == null || innerMenu == 0) return true;
            a.getWindow().getDecorView().post(() ->
                showActionBarPopup(a, innerMenu));
            return true;
        });
    }

    private static boolean hasNestedSubmenus(long menuIh)
    {
        int count = nativeGetChildCount(menuIh);
        for (int i = 0; i < count; i++)
        {
            long c = nativeGetChild(menuIh, i);
            if (c != 0 && nativeGetType(c) == TYPE_SUBMENU) return true;
        }
        return false;
    }

    private static void showActionBarPopup(Activity a, long innerMenu)
    {
        float density = IupCommon.getDisplayDensity();
        int x = Math.round(8 * density);
        int y = Math.round(64 * density);
        boolean[] done = { false };
        showPopupWindow(a, innerMenu, Gravity.TOP | Gravity.END, x, y, done);
    }


    /* Drawer rebuilds store per-item context keyed on the MenuItem id. */
    private static final class DrawerEntry
    {
        final long ihandle;      /* item / submenu Ihandle, or owning menu Ihandle for recent */
        final int recentIndex;   /* >=0 for recent entries, -1 otherwise */
        DrawerEntry(long ih, int ri) { this.ihandle = ih; this.recentIndex = ri; }
    }
    private static final java.util.Map<Integer, DrawerEntry> sDrawerEntries = new java.util.HashMap<>();
    private static int sDrawerNextId = ID_BASE;

    /* Cached expand state per submenu Ihandle, survives drawer rebuilds. */
    private static final java.util.Map<Long, Boolean> sExpandedState = new java.util.HashMap<>();

    /* Section group id allocator (children of an expandable submenu share one group). */
    private static int sNextSectionGroupId = ID_BASE + 0x10000;

    /* DrawerEntry.recentIndex sentinel for expandable section headers. */
    private static final int RECENT_INDEX_EXPAND = -2;

    private static com.google.android.material.navigation.NavigationView sNavigationView;
    private static long sDrawerMenuIh;

    public static void buildNavigationDrawer(com.google.android.material.navigation.NavigationView nv, long menuIh)
    {
        sNavigationView = nv;
        sDrawerMenuIh = menuIh;
        rebuildNavigationDrawer();
    }

    private static void rebuildNavigationDrawer()
    {
        if (sNavigationView == null) return;
        sDrawerEntries.clear();
        sDrawerNextId = ID_BASE;
        sNextSectionGroupId = ID_BASE + 0x10000;
        Menu m = sNavigationView.getMenu();
        m.clear();
        if (sDrawerMenuIh == 0) { forceNavigationRefresh(); return; }

        appendRecentItemsAsDrawer(m, sDrawerMenuIh);
        int count = nativeGetChildCount(sDrawerMenuIh);
        for (int i = 0; i < count; i++)
        {
            long childIh = nativeGetChild(sDrawerMenuIh, i);
            if (childIh == 0) continue;
            int type = nativeGetType(childIh);
            if (type == TYPE_ITEM) addDrawerLeaf(m, childIh);
            else if (type == TYPE_SUBMENU) addDrawerSection(m, childIh);
        }
        forceNavigationRefresh();
    }

    /* NavigationMenuAdapter's own item list is rebuilt by reflective update(); setAdapter swap is the fallback. */
    private static void forceNavigationRefresh()
    {
        if (sNavigationView == null) return;
        final RecyclerView rv = findInnerRecyclerView();
        if (rv == null || rv.getAdapter() == null) return;
        sNavigationView.post(() -> {
            RecyclerView.Adapter<?> adapter = rv.getAdapter();
            if (adapter == null) return;
            int before = readAdapterItemsSize(adapter);
            clearUpdateSuspendedFlag(adapter);
            boolean updated = invokeAdapterUpdate(adapter);
            int after = readAdapterItemsSize(adapter);
            if (!updated || before == after)
            {
                rv.setAdapter(null);
                rv.setAdapter(adapter);
            }
        });
    }

    private static RecyclerView findInnerRecyclerView()
    {
        if (sNavigationView == null) return null;
        for (int i = 0; i < sNavigationView.getChildCount(); i++)
        {
            View c = sNavigationView.getChildAt(i);
            if (c instanceof RecyclerView) return (RecyclerView)c;
        }
        return null;
    }

    private static void clearUpdateSuspendedFlag(RecyclerView.Adapter<?> adapter)
    {
        for (java.lang.reflect.Field f : adapter.getClass().getDeclaredFields())
        {
            if (f.getType() == boolean.class && f.getName().toLowerCase().contains("suspend"))
            {
                try { f.setAccessible(true); f.setBoolean(adapter, false); } catch (Throwable ignore) {}
            }
        }
    }

    private static int readAdapterItemsSize(RecyclerView.Adapter<?> adapter)
    {
        for (java.lang.reflect.Field f : adapter.getClass().getDeclaredFields())
        {
            if (f.getName().equals("items") || f.getName().equals("mItems"))
            {
                try {
                    f.setAccessible(true);
                    Object v = f.get(adapter);
                    if (v instanceof java.util.Collection<?>) return ((java.util.Collection<?>)v).size();
                } catch (Throwable ignore) {}
            }
        }
        return -1;
    }

    private static boolean invokeAdapterUpdate(RecyclerView.Adapter<?> adapter)
    {
        try {
            java.lang.reflect.Method update = adapter.getClass().getDeclaredMethod("update");
            update.setAccessible(true);
            update.invoke(adapter);
            return true;
        } catch (Throwable t) {
            adapter.notifyDataSetChanged();
            return false;
        }
    }

    private static void appendRecentItemsAsDrawer(Menu parent, long menuIh)
    {
        appendRecentItemsAsDrawer(parent, menuIh, Menu.NONE);
    }

    private static void appendRecentItemsAsDrawer(Menu parent, long menuIh, int groupId)
    {
        int n = nativeGetRecentCount(menuIh);
        if (n <= 0) return;
        android.view.SubMenu sub = parent.addSubMenu(groupId, Menu.NONE, Menu.NONE, "Recent");
        for (int i = 0; i < n; i++)
        {
            String path = nativeGetRecentFile(menuIh, i);
            if (path == null || path.isEmpty()) continue;
            int id = sDrawerNextId++;
            sDrawerEntries.put(id, new DrawerEntry(menuIh, i));
            sub.add(groupId, id, Menu.NONE, recentDisplayName(path));
        }
    }

    private static void addDrawerLeaf(Menu parent, long itemIh)
    {
        addDrawerLeaf(parent, itemIh, Menu.NONE);
    }

    private static void addDrawerLeaf(Menu parent, long itemIh, int groupId)
    {
        int id = sDrawerNextId++;
        sDrawerEntries.put(id, new DrawerEntry(itemIh, -1));
        String title = nativeGetTitle(itemIh);
        MenuItem mi = parent.add(groupId, id, Menu.NONE, title != null ? title : "");
        mi.setEnabled(nativeIsActive(itemIh));
        Activity a = IupApplication.getIupApplication().getCurrentActivity();
        if (a != null)
        {
            Drawable icon = resolveItemIcon(itemIh, a);
            if (icon != null) mi.setIcon(icon);
        }
    }

    /* Section header; EXPANDABLE=YES makes it a clickable collapsible header. */
    private static void addDrawerSection(Menu parent, long submenuIh)
    {
        String title = nativeGetTitle(submenuIh);
        long innerMenu = submenuInnerMenu(submenuIh);

        if (nativeGetBoolAttribute(submenuIh, "EXPANDABLE"))
        {
            addExpandableSection(parent, submenuIh, title, innerMenu);
            return;
        }

        android.view.SubMenu section = parent.addSubMenu(Menu.NONE, Menu.NONE, Menu.NONE, title != null ? title : "");
        if (innerMenu == 0) return;

        appendRecentItemsAsDrawer(section, innerMenu);

        int count = nativeGetChildCount(innerMenu);
        for (int i = 0; i < count; i++)
        {
            long childIh = nativeGetChild(innerMenu, i);
            if (childIh == 0) continue;
            int type = nativeGetType(childIh);
            if (type == TYPE_ITEM)
            {
                addDrawerLeaf(section, childIh);
            }
            else if (type == TYPE_SUBMENU)
            {
                int id = sDrawerNextId++;
                sDrawerEntries.put(id, new DrawerEntry(childIh, -1));
                String subtitle = nativeGetTitle(childIh);
                MenuItem submi = section.add(Menu.NONE, id, Menu.NONE, (subtitle != null ? subtitle : "") + "   ▸");
                Activity a = IupApplication.getIupApplication().getCurrentActivity();
                if (a != null)
                {
                    Drawable icon = resolveItemIcon(childIh, a);
                    if (icon != null) submi.setIcon(icon);
                }
            }
        }
    }

    private static void addExpandableSection(Menu parent, long submenuIh, String title, long innerMenu)
    {
        Boolean cached = sExpandedState.get(submenuIh);
        boolean expanded = (cached != null)
            ? cached
            : "OPEN".equalsIgnoreCase(nativeGetStringAttribute(submenuIh, "STATE"));
        sExpandedState.put(submenuIh, expanded);

        int groupId = sNextSectionGroupId++;

        int headerId = sDrawerNextId++;
        sDrawerEntries.put(headerId, new DrawerEntry(submenuIh, RECENT_INDEX_EXPAND));
        MenuItem hdr = parent.add(Menu.NONE, headerId, Menu.NONE, headerLabel(title, expanded));
        hdr.setEnabled(true);
        hdr.setCheckable(false);
        Activity a = IupApplication.getIupApplication().getCurrentActivity();
        if (a != null)
        {
            Drawable icon = resolveItemIcon(submenuIh, a);
            if (icon != null) hdr.setIcon(icon);
        }

        if (!expanded || innerMenu == 0) return;

        appendRecentItemsAsDrawer(parent, innerMenu, groupId);

        int count = nativeGetChildCount(innerMenu);
        for (int i = 0; i < count; i++)
        {
            long childIh = nativeGetChild(innerMenu, i);
            if (childIh == 0) continue;
            int type = nativeGetType(childIh);
            if (type == TYPE_ITEM)
            {
                addDrawerLeaf(parent, childIh, groupId);
            }
            else if (type == TYPE_SUBMENU)
            {
                int id = sDrawerNextId++;
                sDrawerEntries.put(id, new DrawerEntry(childIh, -1));
                String subtitle = nativeGetTitle(childIh);
                MenuItem submi = parent.add(groupId, id, Menu.NONE, (subtitle != null ? subtitle : "") + "   ▸");
                if (a != null)
                {
                    Drawable icon = resolveItemIcon(childIh, a);
                    if (icon != null) submi.setIcon(icon);
                }
            }
        }
    }

    private static String headerLabel(String title, boolean expanded)
    {
        return (title != null ? title : "") + (expanded ? "  ▼" : "  ▶");
    }

    public static void handleDrawerClick(androidx.drawerlayout.widget.DrawerLayout dl, int itemId)
    {
        DrawerEntry e = sDrawerEntries.get(itemId);
        if (e == null) return;
        if (e.recentIndex == RECENT_INDEX_EXPAND)
        {
            Boolean cur = sExpandedState.get(e.ihandle);
            sExpandedState.put(e.ihandle, (cur == null) || !cur);
            rebuildNavigationDrawer();
            return;
        }
        if (e.recentIndex >= 0)
        {
            nativeDispatchRecent(e.ihandle, e.recentIndex);
            dl.closeDrawers();
            return;
        }
        int type = nativeGetType(e.ihandle);
        if (type == TYPE_ITEM)
        {
            nativeDispatchAction(e.ihandle);
            dl.closeDrawers();
            return;
        }
        if (type == TYPE_SUBMENU)
        {
            final long innerMenu = submenuInnerMenu(e.ihandle);
            final Activity a = IupApplication.getIupApplication().getCurrentActivity();
            dl.closeDrawers();
            if (a != null && innerMenu != 0)
                dl.postDelayed(() -> showActionBarPopup(a, innerMenu), 250);
        }
    }


    /* iupandroid_jni_IupMenu.c */
    public static native int nativeGetType(long ih);
    public static native int nativeGetChildCount(long ih);
    public static native long nativeGetChild(long ih, int index);
    public static native String nativeGetTitle(long ih);
    public static native boolean nativeIsActive(long ih);
    public static native boolean nativeGetBoolAttribute(long ih, String name);
    public static native String nativeGetStringAttribute(long ih, String name);
    public static native boolean nativeIsParentRadio(long ih);
    public static native Bitmap nativeGetImage(long ih);
    public static native void nativeDispatchAction(long ih);
    public static native int nativeGetRecentCount(long ih);
    public static native String nativeGetRecentFile(long ih, int index);
    public static native void nativeDispatchRecent(long ih, int index);


    /* Resolves IMAGE to Drawable via the IupImage to Bitmap pipeline. */
    private static Drawable resolveItemIcon(long ih, android.content.Context ctx)
    {
        Bitmap bmp = nativeGetImage(ih);
        if (bmp == null) return null;
        return new BitmapDrawable(ctx.getResources(), bmp);
    }
}
