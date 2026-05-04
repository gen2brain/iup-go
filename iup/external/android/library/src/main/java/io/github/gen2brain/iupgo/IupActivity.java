package io.github.gen2brain.iupgo;

import android.app.Activity;
import android.content.Intent;
import android.content.res.Configuration;
import android.graphics.drawable.ColorDrawable;
import android.os.Bundle;
import android.util.Log;
import android.view.MotionEvent;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;

import androidx.annotation.Keep;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.ActionBarDrawerToggle;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.widget.NestedScrollView;
import androidx.drawerlayout.widget.DrawerLayout;

import com.google.android.material.appbar.MaterialToolbar;
import com.google.android.material.navigation.NavigationView;


/* host Activity for an IupDialog; wraps IupAndroidFixed in a NestedScrollView */
public class IupActivity extends AppCompatActivity
{
    private static final String TAG = "Iup";

    /* IUP show-state codes from iup.h */
    private static final int IUP_SHOW = 0;
    private static final int IUP_RESTORE = 1;
    private static final int IUP_HIDE = 4;

    private Configuration previousConfig;
    private IupAndroidFixed rootFixed;
    private NestedScrollView scrollView;
    /* Toolbar + scrollView; rebuilt on Day/Night flips so the overflow popup follows the theme. */
    private android.widget.LinearLayout content;
    private MaterialToolbar toolbar;
    private boolean titleCentered;
    private boolean firstStart = true;
    private long menuIhandle = 0;

    private DrawerLayout drawerLayout;
    private NavigationView navigationView;
    private ActionBarDrawerToggle drawerToggle;
    private long drawerIhandle = 0;

    /* Latest RESUMED IupActivity; weak to avoid static Context leak. */
    private static java.lang.ref.WeakReference<Activity> sCurrentActivityRef;

    /* tracked on DOWN/MOVE so iupdrvGetCursorPos stays fresh during drag (sbox reads CURSORPOS) */
    private static int sLastTouchX;
    private static int sLastTouchY;

    public static Activity currentActivity()
    {
        return sCurrentActivityRef != null ? sCurrentActivityRef.get() : null;
    }

    @Keep
    public static int getLastTouchX() { return sLastTouchX; }
    @Keep
    public static int getLastTouchY() { return sLastTouchY; }

    @Override
    public boolean dispatchTouchEvent(MotionEvent ev)
    {
        int action = ev.getActionMasked();
        if (action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_MOVE)
        {
            sLastTouchX = (int) ev.getRawX();
            sLastTouchY = (int) ev.getRawY();
        }
        return super.dispatchTouchEvent(ev);
    }


    /** the absolute-positioning content view; IUP child widgets live here */
    public IupAndroidFixed getRootFixed()
    {
        return rootFixed;
    }

    private String titleBarStyle = "FLAT";

    private MaterialToolbar buildToolbar()
    {
        MaterialToolbar tb = new MaterialToolbar(this);
        tb.setTitleCentered(titleCentered);
        applyTitleBarStyleTo(tb);
        return tb;
    }

    /* FLAT = colorSurface (Material3 default), LIFTED = colorSurfaceContainer, PRIMARY = colorPrimary + colorOnPrimary text. */
    private void applyTitleBarStyleTo(MaterialToolbar tb)
    {
        int bg, fg;
        switch (titleBarStyle == null ? "FLAT" : titleBarStyle.toUpperCase(java.util.Locale.ROOT))
        {
            case "LIFTED":
                bg = IupCommon.resolveThemeColorByName("colorSurfaceContainer", IupCommon.paletteDlgBg);
                fg = IupCommon.paletteDlgFg;
                break;
            case "PRIMARY":
                bg = IupCommon.resolveThemeColorByName("colorPrimary",   IupCommon.paletteAccent);
                fg = IupCommon.resolveThemeColorByName("colorOnPrimary", 0xFFFFFFFF);
                break;
            case "FLAT":
            default:
                bg = IupCommon.paletteDlgBg;
                fg = IupCommon.paletteDlgFg;
                break;
        }
        tb.setBackgroundColor(bg);
        tb.setTitleTextColor(fg);
        tb.setSubtitleTextColor(fg);
        tb.setNavigationIconTint(fg);
    }

    public void setTitleBarStyle(String style)
    {
        titleBarStyle = (style == null || style.isEmpty()) ? "FLAT" : style;
        if (toolbar != null) applyTitleBarStyleTo(toolbar);
    }

    /** TITLECENTERED on IupDialog. */
    public void setTitleCentered(boolean centered)
    {
        titleCentered = centered;
        if (toolbar != null) toolbar.setTitleCentered(centered);
    }

    /* drive light/dark status bar from live uiMode; Material3 DayNight skips windowLightStatusBar */
    private void applySystemBarAppearance()
    {
        androidx.core.view.WindowInsetsControllerCompat ctrl =
            new androidx.core.view.WindowInsetsControllerCompat(getWindow(), getWindow().getDecorView());
        boolean lightBars = !IupCommon.isDarkMode();
        ctrl.setAppearanceLightStatusBars(lightBars);
        ctrl.setAppearanceLightNavigationBars(lightBars);
    }

    public void setMenuIhandle(long ih)
    {
        menuIhandle = ih;
    }

    /** Installs/updates the NavigationDrawer. ih=0 removes it. */
    public void setDrawerIhandle(long ih)
    {
        drawerIhandle = ih;
        if (scrollView == null) return;
        if (ih != 0) ensureDrawerInstalled();
        else removeDrawer();
    }

    private void ensureDrawerInstalled()
    {
        if (drawerLayout == null)
        {
            drawerLayout = new DrawerLayout(this);
            int idx = content.indexOfChild(scrollView);
            android.widget.LinearLayout.LayoutParams svLp = (android.widget.LinearLayout.LayoutParams) scrollView.getLayoutParams();
            content.removeView(scrollView);
            drawerLayout.addView(scrollView, new DrawerLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
            content.addView(drawerLayout, idx, svLp);

            navigationView = new NavigationView(this);
            DrawerLayout.LayoutParams nvParams = new DrawerLayout.LayoutParams(
                (int)(280 * getResources().getDisplayMetrics().density),
                ViewGroup.LayoutParams.MATCH_PARENT);
            nvParams.gravity = android.view.Gravity.START;
            navigationView.setLayoutParams(nvParams);
            /* Material tints menu-item icons with colorControlNormal by default; null preserves IupImage colors. */
            navigationView.setItemIconTintList(null);
            drawerLayout.addView(navigationView);

            navigationView.setNavigationItemSelectedListener(item -> {
                IupMenuHelper.handleDrawerClick(drawerLayout, item.getItemId());
                return true;
            });

            drawerToggle = new ActionBarDrawerToggle(this, drawerLayout, toolbar,
                androidx.appcompat.R.string.abc_action_bar_home_description,
                androidx.appcompat.R.string.abc_action_bar_home_description);
            drawerLayout.addDrawerListener(drawerToggle);
            drawerToggle.syncState();
        }
        IupMenuHelper.buildNavigationDrawer(navigationView, drawerIhandle);
    }

    private void removeDrawer()
    {
        if (drawerLayout == null) return;
        if (drawerToggle != null) drawerLayout.removeDrawerListener(drawerToggle);
        int idx = content.indexOfChild(drawerLayout);
        android.widget.LinearLayout.LayoutParams dlLp = (android.widget.LinearLayout.LayoutParams) drawerLayout.getLayoutParams();
        drawerLayout.removeAllViews();
        content.removeView(drawerLayout);
        content.addView(scrollView, idx, dlLp);
        drawerLayout = null;
        navigationView = null;
        drawerToggle = null;
        if (toolbar != null) toolbar.setNavigationIcon(null);
    }

    @Override
    public boolean onOptionsItemSelected(android.view.MenuItem item)
    {
        if (drawerToggle != null && drawerToggle.onOptionsItemSelected(item)) return true;
        return super.onOptionsItemSelected(item);
    }

    @Override
    public boolean onCreateOptionsMenu(android.view.Menu menu)
    {
        if (menuIhandle != 0)
        {
            IupMenuHelper.buildActionBarMenu(menu, menuIhandle);
            return true;
        }
        return super.onCreateOptionsMenu(menu);
    }

    native protected void OnActivityResult(long ihandlePtr, int requestCode, int resultCode, Intent intentData);

    /** Replays Android-only Dialog attributes on the C side once the Activity exists. */
    native protected void DialogActivityCreated(long ihandlePtr);

    native protected void FinalizeDialogDestroy(long ihandlePtr);

    /** refreshes DLG/TXT palette on the C side and fires THEMECHANGED_CB. */
    native protected void NotifyThemeChanged(long ihandlePtr, int darkMode);

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        final Intent intent = getIntent();
        final long ihandlePtr = intent.getLongExtra("Ihandle", 0);

        Object viewGroupObject = IupCommon.getObjectFromIhandle(ihandlePtr);
        if (viewGroupObject instanceof IupAndroidFixed)
        {
            rootFixed = (IupAndroidFixed)viewGroupObject;

            scrollView = new NestedScrollView(this);
            scrollView.setFillViewport(true);
            scrollView.addView(rootFixed, new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT));

            content = new android.widget.LinearLayout(this);
            content.setOrientation(android.widget.LinearLayout.VERTICAL);
            toolbar = buildToolbar();
            content.addView(toolbar, new android.widget.LinearLayout.LayoutParams(
                android.widget.LinearLayout.LayoutParams.MATCH_PARENT,
                android.widget.LinearLayout.LayoutParams.WRAP_CONTENT));
            content.addView(scrollView, new android.widget.LinearLayout.LayoutParams(
                android.widget.LinearLayout.LayoutParams.MATCH_PARENT, 0, 1f));

            /* Insets on the LinearLayout: toolbar sits below status bar, scrollView stops above nav bar / IME. */
            installInsetHandler(content);
            content.setBackgroundColor(IupCommon.paletteDlgBg);
            setContentView(content);
            setSupportActionBar(toolbar);
            applySystemBarAppearance();

            /* swap the Java ref: placeholder ViewGroup -> real Activity, so ih->handle now finds it */
            IupCommon.releaseIhandle(ihandlePtr);
            IupCommon.retainIhandle(this, ihandlePtr);

            /* re-apply setters that no-op'd during IupMap (Activity didn't exist yet) */
            DialogActivityCreated(ihandlePtr);

            /* ScrollView is still 0x0 here; wait for first layout before reporting size to IUP */
            observeLayout(scrollView, ihandlePtr, "onCreate");
        }
        else
        {
            Log.e(TAG, "IupActivity.onCreate: expected an IupAndroidFixed in Ihandle but did not get one");
            IupCommon.retainIhandle(this, ihandlePtr);
        }
    }

    @Override
    protected void onStart()
    {
        super.onStart();
        long ihandlePtr = getIntent().getLongExtra("Ihandle", 0);
        if (ihandlePtr != 0)
        {
            int state = firstStart ? IUP_SHOW : IUP_RESTORE;
            firstStart = false;
            IupCommon.handleIupCallbackInt(ihandlePtr, "SHOW_CB", state);
        }
    }

    @Override
    protected void onStop()
    {
        long ihandlePtr = getIntent().getLongExtra("Ihandle", 0);
        if (ihandlePtr != 0)
        {
            IupCommon.handleIupCallbackInt(ihandlePtr, "SHOW_CB", IUP_HIDE);
        }
        super.onStop();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus)
    {
        super.onWindowFocusChanged(hasFocus);
        long ihandlePtr = getIntent().getLongExtra("Ihandle", 0);
        if (ihandlePtr != 0)
        {
            IupCommon.handleIupCallbackInt(ihandlePtr, "FOCUS_CB", hasFocus ? 1 : 0);
        }
    }

    @Override
    protected void onResume()
    {
        super.onResume();
        sCurrentActivityRef = new java.lang.ref.WeakReference<>(this);
        previousConfig = new Configuration(getResources().getConfiguration());
    }

    @Override
    protected void onPause()
    {
        if (sCurrentActivityRef != null && sCurrentActivityRef.get() == this)
            sCurrentActivityRef = null;
        super.onPause();
    }

    @Override
    protected void onDestroy()
    {
        /* extra cleared = IUP already disposed; else treat back/OOM as a forced close */
        Intent intent = getIntent();
        long ihandlePtr = intent.getLongExtra("Ihandle", 0);
        if (ihandlePtr != 0)
        {
            IupCommon.handleIupCallback(ihandlePtr, "CLOSE_CB");
            IupCommon.releaseIhandle(ihandlePtr);
            FinalizeDialogDestroy(ihandlePtr);
        }

        /* Releases the IupMainLoop nested pump from iup.Popup, if one is active. */
        IupCommon.modalPumpExitTopmost();

        super.onDestroy();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig)
    {
        int oldNight = (previousConfig != null)
            ? (previousConfig.uiMode & Configuration.UI_MODE_NIGHT_MASK)
            : (getResources().getConfiguration().uiMode & Configuration.UI_MODE_NIGHT_MASK);
        int newNight = newConfig.uiMode & Configuration.UI_MODE_NIGHT_MASK;

        super.onConfigurationChanged(newConfig);
        previousConfig = new Configuration(newConfig);
        if (drawerToggle != null) drawerToggle.onConfigurationChanged(newConfig);

        /* configChanges absorbs rotation; force a layout pass so insets + observer re-read */
        if (scrollView != null)
        {
            scrollView.requestApplyInsets();
            scrollView.requestLayout();
            scrollView.invalidate();
        }

        /* dark-mode flip: applyDayNight, refresh palette + THEMECHANGED_CB, then invalidate */
        if (oldNight != newNight)
        {
            getDelegate().applyDayNight();

            long ihandlePtr = getIntent().getLongExtra("Ihandle", 0);
            if (ihandlePtr != 0)
            {
                int darkMode = (newNight == Configuration.UI_MODE_NIGHT_YES) ? 1 : 0;
                NotifyThemeChanged(ihandlePtr, darkMode);
            }

            IupTheme.notifyThemeChanged();

            swapToolbar();
            refreshActionBarAppearance();
            applySystemBarAppearance();
            if (content != null) content.setBackgroundColor(IupCommon.paletteDlgBg);
            getWindow().setBackgroundDrawable(new ColorDrawable(IupCommon.paletteDlgBg));
            if (scrollView != null) scrollView.setBackgroundColor(IupCommon.paletteDlgBg);
            if (rootFixed != null) invalidateSubtree(rootFixed);
        }
    }

    /* fresh Toolbar instance so mPopupContext/ActionMenuPresenter rebuild against the post-flip context */
    private void swapToolbar()
    {
        if (content == null || toolbar == null) return;
        CharSequence title = (getSupportActionBar() != null) ? getSupportActionBar().getTitle() : null;
        android.graphics.drawable.Drawable logo = toolbar.getLogo();

        int idx = content.indexOfChild(toolbar);
        ViewGroup.LayoutParams lp = toolbar.getLayoutParams();
        content.removeView(toolbar);

        toolbar = buildToolbar();
        content.addView(toolbar, idx, lp);
        setSupportActionBar(toolbar);
        if (title != null && getSupportActionBar() != null) getSupportActionBar().setTitle(title);
        if (logo != null) toolbar.setLogo(logo);

        if (drawerLayout != null)
        {
            if (drawerToggle != null) drawerLayout.removeDrawerListener(drawerToggle);
            drawerToggle = new ActionBarDrawerToggle(this, drawerLayout, toolbar,
                androidx.appcompat.R.string.abc_action_bar_home_description,
                androidx.appcompat.R.string.abc_action_bar_home_description);
            drawerLayout.addDrawerListener(drawerToggle);
            drawerToggle.syncState();
        }
    }

    /* uiMode in configChanges skips Activity recreate; ActionBar/Toolbar still hold inflation-time colors */
    private void refreshActionBarAppearance()
    {
        ActionBar bar = getSupportActionBar();
        if (bar == null) return;
        if (toolbar != null) applyTitleBarStyleTo(toolbar);
        CharSequence current = bar.getTitle();
        int fg = ("PRIMARY".equalsIgnoreCase(titleBarStyle))
            ? IupCommon.resolveThemeColorByName("colorOnPrimary", 0xFFFFFFFF)
            : IupCommon.paletteDlgFg;
        bar.setTitle(htmlColoredTitle(current != null ? current.toString() : "", fg));
    }

    private static CharSequence htmlColoredTitle(String text, int argb)
    {
        int rgb = argb & 0x00FFFFFF;
        String hex = String.format("#%06X", rgb);
        String escaped = text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;");
        return androidx.core.text.HtmlCompat.fromHtml("<font color='" + hex + "'>" + escaped + "</font>", androidx.core.text.HtmlCompat.FROM_HTML_MODE_LEGACY);
    }

    private static void invalidateSubtree(android.view.View v)
    {
        v.invalidate();
        if (v instanceof ViewGroup g)
        {
            for (int i = 0; i < g.getChildCount(); i++) invalidateSubtree(g.getChildAt(i));
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent intentData)
    {
        super.onActivityResult(requestCode, resultCode, intentData);

        /* FileDlg owns its reserved request-code range. */
        if (IupFileDlgHelper.isFileDlgRequest(requestCode))
        {
            IupFileDlgHelper.deliverResult(requestCode, resultCode, intentData);
            return;
        }

        /* forward so app code can observe results of externally-launched activities */
        long ihandlePtr = getIntent().getLongExtra("Ihandle", 0);
        OnActivityResult(ihandlePtr, requestCode, resultCode, intentData);
    }


    /** starts IupActivity and returns a detached placeholder Fixed; the Activity wraps it later. */
    @Keep
    public static ViewGroup createActivity(final Activity parentActivity, long ihandlePtr)
    {
        final Intent intent = new Intent(parentActivity, IupActivity.class);
        intent.putExtra("Ihandle", ihandlePtr);
        parentActivity.startActivity(intent);
        return new IupAndroidFixed(parentActivity);
    }

    /** Called from C (IupDestroy path). */
    @Keep
    public static void unMapActivity(Object activityOrViewGroup, long ihandlePtr)
    {
        if (!(activityOrViewGroup instanceof Activity activity))
        {
            return;
        }

        activity.finish();

        /* release here, not in onDestroy: we can't tell explicit Destroy from OS kill there */
        IupCommon.releaseIhandle(ihandlePtr);

        /* clear the extra so onDestroy doesn't fire CLOSE_CB for a disposed dialog */
        activity.getIntent().removeExtra("Ihandle");
    }


    /** consumes system-bar insets as ScrollView padding; required for edge-to-edge on API 30+ */
    private static void installInsetHandler(final android.view.View host)
    {
        host.setFitsSystemWindows(true);
        ViewCompat.setOnApplyWindowInsetsListener(host, (view, insets) -> {
            Insets bars = insets.getInsets(
                WindowInsetsCompat.Type.systemBars() |
                WindowInsetsCompat.Type.displayCutout());
            view.setPadding(bars.left, bars.top, bars.right, bars.bottom);
            return WindowInsetsCompat.CONSUMED;
        });
    }

    /* persistent listener; reports viewport (not fixed) size to IUP on layout/insets/rotate/keyboard */
    private void observeLayout(final NestedScrollView host, final long ihandlePtr, final String reason)
    {
        host.getViewTreeObserver().addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener()
        {
            int lastWidth = -1;
            int lastHeight = -1;

            @Override
            public void onGlobalLayout()
            {
                int w = host.getWidth() - host.getPaddingLeft() - host.getPaddingRight();
                int h = host.getHeight() - host.getPaddingTop() - host.getPaddingBottom();
                if (w <= 0 || h <= 0)
                {
                    return;
                }
                if (w == lastWidth && h == lastHeight)
                {
                    return;
                }
                lastWidth = w;
                lastHeight = h;
                IupCommon.doResize(ihandlePtr, 0, 0, w, h);
            }
        });
    }
}
