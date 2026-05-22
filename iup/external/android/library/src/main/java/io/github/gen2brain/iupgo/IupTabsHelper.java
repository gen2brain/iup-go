package io.github.gen2brain.iupgo;

import android.content.ClipData;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.view.DragEvent;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import androidx.annotation.Keep;

import android.content.res.ColorStateList;

import com.google.android.material.color.MaterialColors;
import com.google.android.material.tabs.TabLayout;


/* TabLayout + FrameLayout pages; TABTYPE=TOP only. */
public final class IupTabsHelper
{
    private IupTabsHelper() {}

    /* TabLayout caches tab text + indicator colours from theme attrs at construction; rebuild on theme flip. */
    private static final java.util.WeakHashMap<IupAndroidTabs, Boolean> sThemableTabs = new java.util.WeakHashMap<>();

    static {
        IupTheme.register(IupTabsHelper::refreshAllTabs);
    }

    private static void refreshAllTabs()
    {
        for (IupAndroidTabs tabs : new java.util.ArrayList<>(sThemableTabs.keySet()))
        {
            if (tabs != null) applyTabsPalette(tabs);
        }
    }

    private static void applyTabsPalette(IupAndroidTabs tabs)
    {
        Context ctx = IupCommon.getThemeContext();
        int primary      = MaterialColors.getColor(ctx, com.google.android.material.R.attr.colorPrimary, android.graphics.Color.BLUE);
        int onSurfaceVar = MaterialColors.getColor(ctx, com.google.android.material.R.attr.colorOnSurfaceVariant, android.graphics.Color.GRAY);
        int surface      = MaterialColors.getColor(ctx, com.google.android.material.R.attr.colorSurface, android.graphics.Color.WHITE);

        tabs.setBackgroundColor(surface);
        tabs.tabLayout.setBackgroundColor(surface);
        tabs.tabLayout.setSelectedTabIndicatorColor(primary);
        tabs.tabLayout.setTabRippleColor(android.content.res.ColorStateList.valueOf(
            (primary & 0x00FFFFFF) | 0x33000000));

        /* setTabTextColors only paints TabLayout's auto-managed views; setCustomView TabContent needs explicit paint. */
        ColorStateList textCsl = new ColorStateList(
            new int[][] { { android.R.attr.state_selected }, {} },
            new int[]   { primary, onSurfaceVar });
        for (int i = 0, n = tabs.tabLayout.getTabCount(); i < n; i++)
        {
            TabContent tc = contentOf(tabs.tabLayout.getTabAt(i));
            if (tc == null) continue;
            tc.title.setTextColor(textCsl);
            tc.closeBtn.setTextColor(textCsl);
        }
    }

    /* Programmatic-only; requires an Ihandle so the XML constructors are absent. */
    @android.annotation.SuppressLint("ViewConstructor")
    public static class IupAndroidTabs extends LinearLayout
    {
        final long ihandlePtr;
        final TabLayout tabLayout;
        final FrameLayout content;
        boolean firing;
        boolean reorderable;
        /* Global SHOWCLOSE state propagated to each tab's close button. */
        boolean showClose;

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

        public IupAndroidTabs(Context ctx, long ih)
        {
            super(ctx);
            this.ihandlePtr = ih;
            setOrientation(VERTICAL);

            tabLayout = new TabLayout(ctx);
            tabLayout.setTabMode(TabLayout.MODE_SCROLLABLE);
            tabLayout.setInlineLabel(true);  /* icon to the left of title, single row */
            tabLayout.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT));
            addView(tabLayout);

            content = new FrameLayout(ctx);
            content.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                0, 1.0f));
            addView(content);

            tabLayout.addOnTabSelectedListener(new TabLayout.OnTabSelectedListener()
            {
                @Override
                public void onTabSelected(TabLayout.Tab tab)
                {
                    int pos = tab.getPosition();
                    showPage(pos);
                    if (!firing) dispatchTabChange(ihandlePtr, pos);
                }
                @Override public void onTabUnselected(TabLayout.Tab tab) {}
                @Override public void onTabReselected(TabLayout.Tab tab) {}
            });

            tabLayout.setOnDragListener((v, event) -> {
                if (!reorderable) return false;
                if (event.getAction() == DragEvent.ACTION_DROP)
                {
                    int source = (Integer) event.getLocalState();
                    int target = findTabAtX(event.getX());
                    if (target >= 0 && target != source)
                        dispatchReorder(ihandlePtr, source, target);
                }
                return true;
            });
        }

        void showPage(int pos)
        {
            int count = content.getChildCount();
            for (int i = 0; i < count; i++)
                content.getChildAt(i).setVisibility(i == pos ? View.VISIBLE : View.GONE);
        }

        int findTabAtX(float x)
        {
            View strip = tabLayout.getChildAt(0);
            if (!(strip instanceof ViewGroup vg)) return -1;
            float relX = x - strip.getLeft() + tabLayout.getScrollX();
            for (int i = 0; i < vg.getChildCount(); i++)
            {
                View tv = vg.getChildAt(i);
                if (relX >= tv.getLeft() && relX < tv.getRight()) return i;
            }
            return -1;
        }

        @SuppressWarnings("deprecation")
        void installDragOnTab(final TabLayout.Tab tab)
        {
            View tabView = tab.view;
            tabView.setOnLongClickListener(v -> {
                if (!reorderable) return false;
                ClipData data = ClipData.newPlainText("iuptab", String.valueOf(tab.getPosition()));
                DragShadowBuilder shadow = new DragShadowBuilder(v);
                if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.N)
                    v.startDragAndDrop(data, shadow, tab.getPosition(), 0);
                else
                    v.startDrag(data, shadow, tab.getPosition(), 0);
                return true;
            });
        }
    }


    @Keep
    public static View createTabs(long ihandlePtr)
    {
        IupAndroidTabs tabs = new IupAndroidTabs(IupCommon.getContextThemeWrapper(), ihandlePtr);
        applyTabsPalette(tabs);
        sThemableTabs.put(tabs, Boolean.TRUE);
        return tabs;
    }

    /* TabLayout has no close affordance; custom view holds icon + title + close at fixed indices */
    private static final class TabContent extends LinearLayout
    {
        final ImageView icon;
        final TextView title;
        final TextView closeBtn;
        String titleText;

        TabContent(Context ctx)
        {
            super(ctx);
            setOrientation(HORIZONTAL);
            setGravity(Gravity.CENTER_VERTICAL);
            float density = IupCommon.getDisplayDensity();
            int gap = (int)(8 * density);

            icon = new ImageView(ctx);
            int iconSz = (int)(20 * density);
            LayoutParams iconLp = new LayoutParams(iconSz, iconSz);
            iconLp.setMarginEnd(gap);
            addView(icon, iconLp);
            icon.setVisibility(GONE);

            title = new TextView(ctx);
            title.setMaxLines(1);
            addView(title);

            closeBtn = new TextView(ctx);
            closeBtn.setText("✕");  /* ✕ */
            closeBtn.setGravity(Gravity.CENTER);
            closeBtn.setClickable(true);
            int closeSz = (int)(20 * density);
            LayoutParams closeLp = new LayoutParams(closeSz, closeSz);
            closeLp.setMarginStart(gap);
            addView(closeBtn, closeLp);
            closeBtn.setVisibility(GONE);
        }

        void setTitleText(String s)
        {
            titleText = s;
            title.setText(s != null ? s : "");
        }

        void setIconBitmap(Bitmap bmp)
        {
            if (bmp == null)
            {
                icon.setImageDrawable(null);
                icon.setVisibility(GONE);
            }
            else
            {
                icon.setImageBitmap(bmp);
                icon.setVisibility(VISIBLE);
            }
        }

        void setIconSizePx(int sizePx)
        {
            if (sizePx <= 0) return;
            LayoutParams lp = (LayoutParams)icon.getLayoutParams();
            if (lp == null) return;
            lp.width = sizePx;
            lp.height = sizePx;
            icon.setLayoutParams(lp);
            icon.setScaleType(ImageView.ScaleType.FIT_CENTER);
        }

        void setShowClose(boolean show)
        {
            closeBtn.setVisibility(show ? VISIBLE : GONE);
        }
    }

    private static TabContent contentOf(TabLayout.Tab tab)
    {
        if (tab == null) return null;
        View v = tab.getCustomView();
        return (v instanceof TabContent) ? (TabContent)v : null;
    }

    /* Returns the IupAndroidFixed page IUP children get parented into. */
    @Keep
    public static View addTab(View tabs, int pos, String title, Bitmap icon, int iconSizePx)
    {
        if (!(tabs instanceof IupAndroidTabs t)) return null;

        IupAndroidFixed page = new IupAndroidFixed(t.getContext());
        page.setLayoutParams(new FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.MATCH_PARENT,
            FrameLayout.LayoutParams.MATCH_PARENT));
        if (pos < 0 || pos > t.content.getChildCount()) pos = t.content.getChildCount();
        t.content.addView(page, pos);

        final TabContent tc = new TabContent(t.getContext());
        tc.setTitleText(title);
        tc.setIconSizePx(iconSizePx);
        tc.setIconBitmap(icon);
        if (t.showClose) tc.setShowClose(true);
        tc.closeBtn.setOnClickListener(v -> {
            int p = -1;
            for (int i = 0; i < t.tabLayout.getTabCount(); i++)
            {
                TabLayout.Tab cand = t.tabLayout.getTabAt(i);
                if (cand != null && cand.getCustomView() == tc) { p = i; break; }
            }
            if (p >= 0) dispatchTabClose(t.ihandlePtr, p);
        });

        TabLayout.Tab tab = t.tabLayout.newTab();
        tab.setCustomView(tc);
        t.firing = true;
        t.tabLayout.addTab(tab, pos, t.tabLayout.getTabCount() == 0);
        t.firing = false;
        t.installDragOnTab(tab);
        t.showPage(t.tabLayout.getSelectedTabPosition());
        applyTabsPalette(t);
        return page;
    }

    @Keep
    public static void setReorderable(View tabs, boolean enable)
    {
        if (!(tabs instanceof IupAndroidTabs)) return;
        ((IupAndroidTabs) tabs).reorderable = enable;
    }


    @Keep
    public static void removeTab(View tabs, int pos)
    {
        if (!(tabs instanceof IupAndroidTabs t)) return;
        if (pos < 0 || pos >= t.tabLayout.getTabCount()) return;
        t.firing = true;
        t.tabLayout.removeTabAt(pos);
        t.firing = false;
        if (pos < t.content.getChildCount()) t.content.removeViewAt(pos);
    }

    /* Native-only reorder: rebuilds the tab row so each tab's content view + page move with it. */
    @Keep
    public static void moveTab(View tabs, int source, int target)
    {
        if (!(tabs instanceof IupAndroidTabs t)) return;
        int count = t.tabLayout.getTabCount();
        if (source < 0 || source >= count || target < 0 || target >= count || source == target) return;

        View[] customs = new View[count];
        for (int i = 0; i < count; i++)
        {
            TabLayout.Tab tab = t.tabLayout.getTabAt(i);
            customs[i] = tab != null ? tab.getCustomView() : null;
        }

        View page = t.content.getChildAt(source);
        t.content.removeViewAt(source);
        t.content.addView(page, target);

        int selected = t.tabLayout.getSelectedTabPosition();
        int newSelected = selected;
        if (selected == source) newSelected = target;
        else if (source < selected && selected <= target) newSelected = selected - 1;
        else if (target <= selected && selected < source) newSelected = selected + 1;

        View movedCustom = customs[source];
        int step = source < target ? 1 : -1;
        for (int i = source; i != target; i += step)
            customs[i] = customs[i + step];
        customs[target] = movedCustom;

        t.firing = true;
        t.tabLayout.removeAllTabs();
        for (int i = 0; i < count; i++)
        {
            TabLayout.Tab tab = t.tabLayout.newTab();
            if (customs[i] != null)
            {
                ViewGroup parent = (ViewGroup)customs[i].getParent();
                if (parent != null) parent.removeView(customs[i]);
                tab.setCustomView(customs[i]);
            }
            t.tabLayout.addTab(tab, i, i == newSelected);
            t.installDragOnTab(tab);
        }
        t.firing = false;
        t.showPage(newSelected);
    }

    @Keep
    public static void setCurrentTab(View tabs, int pos)
    {
        if (!(tabs instanceof IupAndroidTabs t)) return;
        if (pos < 0 || pos >= t.tabLayout.getTabCount()) return;
        TabLayout.Tab tab = t.tabLayout.getTabAt(pos);
        if (tab != null)
        {
            t.firing = true;
            tab.select();
            t.firing = false;
            t.showPage(pos);
        }
    }

    @Keep
    public static int getCurrentTab(View tabs)
    {
        if (!(tabs instanceof IupAndroidTabs)) return 0;
        return ((IupAndroidTabs) tabs).tabLayout.getSelectedTabPosition();
    }

    @Keep
    public static void setTabTitle(View tabs, int pos, String title)
    {
        if (!(tabs instanceof IupAndroidTabs t)) return;
        TabContent tc = contentOf(t.tabLayout.getTabAt(pos));
        if (tc != null) tc.setTitleText(title);
    }

    @Keep
    public static void setTabIcon(View tabs, int pos, Bitmap bmp, int iconSizePx)
    {
        if (!(tabs instanceof IupAndroidTabs t)) return;
        TabContent tc = contentOf(t.tabLayout.getTabAt(pos));
        if (tc != null)
        {
            tc.setIconSizePx(iconSizePx);
            tc.setIconBitmap(bmp);
        }
    }

    /* Hides the tab cell (and its page); indices stay stable per IUP semantics. */
    @Keep
    public static void setTabVisible(View tabs, int pos, boolean visible)
    {
        if (!(tabs instanceof IupAndroidTabs t)) return;
        TabLayout.Tab tab = t.tabLayout.getTabAt(pos);
        if (tab == null || tab.view == null) return;
        tab.view.setVisibility(visible ? View.VISIBLE : View.GONE);
        if (pos < t.content.getChildCount())
        {
            View page = t.content.getChildAt(pos);
            if (!visible) page.setVisibility(View.GONE);
            else if (pos == t.tabLayout.getSelectedTabPosition()) page.setVisibility(View.VISIBLE);
        }
    }

    @Keep
    public static boolean getTabVisible(View tabs, int pos)
    {
        if (!(tabs instanceof IupAndroidTabs t)) return false;
        TabLayout.Tab tab = t.tabLayout.getTabAt(pos);
        if (tab == null || tab.view == null) return false;
        return tab.view.getVisibility() == View.VISIBLE;
    }

    /* Global SHOWCLOSE: toggles the close button in every tab's content view. */
    @Keep
    public static void setShowClose(View tabs, boolean enable)
    {
        if (!(tabs instanceof IupAndroidTabs t)) return;
        t.showClose = enable;
        for (int i = 0; i < t.tabLayout.getTabCount(); i++)
        {
            TabContent tc = contentOf(t.tabLayout.getTabAt(i));
            if (tc != null) tc.setShowClose(enable);
        }
    }

    /* Per-tab SHOWCLOSE override; null/empty resets to the global flag. */
    @Keep
    public static void setShowCloseAt(View tabs, int pos, boolean enable)
    {
        if (!(tabs instanceof IupAndroidTabs t)) return;
        TabContent tc = contentOf(t.tabLayout.getTabAt(pos));
        if (tc != null) tc.setShowClose(enable);
    }


    public static native void dispatchTabChange(long ihandlePtr, int newPos);

    /* Fires REORDER_CB; if not IUP_IGNORE, C calls IupReparent and routes back to moveTab. */
    public static native void dispatchReorder(long ihandlePtr, int sourcePos, int targetPos);

    /* Fires TABCLOSE_CB on the IUP tabs; default behaviour (hide / destroy) is in C. */
    public static native void dispatchTabClose(long ihandlePtr, int pos);
}
