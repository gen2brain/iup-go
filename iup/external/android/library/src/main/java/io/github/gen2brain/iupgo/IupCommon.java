package io.github.gen2brain.iupgo;

import android.app.Activity;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.MessageQueue;
import android.os.SystemClock;
import android.view.Choreographer;
import android.util.Log;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.Keep;
import androidx.appcompat.view.ContextThemeWrapper;

import java.lang.reflect.Field;
import java.lang.reflect.Method;


/* Common JNI bridge. Non-widget-specific C <-> Java calls. */
public final class IupCommon
{
    private static final String TAG = "Iup";

    private IupCommon() {}

    public native static void RetainIhandle(Object widget, long ihandlePtr);
    public native static void ReleaseIhandle(long ihandlePtr);
    public native static Object GetObjectFromIhandle(long ihandlePtr);

    public static void retainIhandle(Object widget, long ihandlePtr)
    {
        RetainIhandle(widget, ihandlePtr);
    }

    /* Html.fromHtml ignores Pango span attrs (foreground, font_family, etc.); walk + span ourselves */
    public static CharSequence parseMarkup(String text)
    {
        if (text == null) return "";
        return IupMarkupHelper.parse(text);
    }

    public static void releaseIhandle(long ihandlePtr)
    {
        ReleaseIhandle(ihandlePtr);
    }

    public static Object getObjectFromIhandle(long ihandlePtr)
    {
        return GetObjectFromIhandle(ihandlePtr);
    }

    public native static String nativeIupAttribGet(long ihandlePtr, String key);
    public native static void nativeIupAttribSet(long ihandlePtr, String key, String value);
    public native static int nativeIupAttribGetInt(long ihandlePtr, String key);
    public native static void nativeIupAttribSetInt(long ihandlePtr, String key, int value);

    public static String iupAttribGet(long ihandlePtr, String key)
    {
        return nativeIupAttribGet(ihandlePtr, key);
    }

    public static void iupAttribSet(long ihandlePtr, String key, String value)
    {
        nativeIupAttribSet(ihandlePtr, key, value);
    }

    public static int iupAttribGetInt(long ihandlePtr, String key)
    {
        return nativeIupAttribGetInt(ihandlePtr, key);
    }

    public static void iupAttribSetInt(long ihandlePtr, String key, int value)
    {
        nativeIupAttribSetInt(ihandlePtr, key, value);
    }

    /** IUP callbacks return -1...-4; we return -15 to signal "no callback registered". */
    public native static int HandleIupCallback(long ihandlePtr, String key);
    public native static int HandleIupCallbackInt(long ihandlePtr, String key, int arg);
    public native static int DoResize(long ihandlePtr, int x, int y, int width, int height);

    public static int handleIupCallback(long ihandlePtr, String key)
    {
        return HandleIupCallback(ihandlePtr, key);
    }

    /** Dispatches a callback with an {@code IFni} signature (e.g. SHOW_CB, FOCUS_CB). */
    public static int handleIupCallbackInt(long ihandlePtr, String key, int arg)
    {
        return HandleIupCallbackInt(ihandlePtr, key, arg);
    }

    public static int doResize(long ihandlePtr, int x, int y, int width, int height)
    {
        return DoResize(ihandlePtr, x, y, width, height);
    }

    @Keep
    public static void addWidgetToParent(Object parentWidget, Object childWidget)
    {
        if (!(childWidget instanceof View childView))
        {
            Log.e(TAG, "addWidgetToParent: childWidget is not a View");
            return;
        }

        ViewGroup parentView = resolveParentViewGroup(parentWidget);
        if (parentView == null)
        {
            Log.e(TAG, "addWidgetToParent: parentWidget is not a supported type");
            return;
        }

        /* layout pass calls setWidgetPosition with real coords before first paint */
        parentView.addView(childView, new IupAndroidFixed.LayoutParams(0, 0, 0, 0));
    }

    @Keep
    public static void removeWidgetFromParent(long ihandlePtr)
    {
        Object childWidget = getObjectFromIhandle(ihandlePtr);
        if (!(childWidget instanceof View childView))
        {
            Log.e(TAG, "removeWidgetFromParent: widget is not a View");
            return;
        }

        ViewGroup parentViewGroup = (ViewGroup)childView.getParent();
        if (parentViewGroup != null)
        {
            parentViewGroup.removeView(childView);
        }
    }

    /* applies IUP's computed bounds; IupAndroidFixed parent stores them on LayoutParams directly */
    @Keep
    public static void setWidgetPosition(Object widget, int x, int y, int width, int height)
    {
        if (!(widget instanceof View view))
        {
            Log.e(TAG, "setWidgetPosition: widget is not a View");
            return;
        }
        ViewGroup parent = (ViewGroup)view.getParent();
        if (parent instanceof IupAndroidFixed)
        {
            ((IupAndroidFixed)parent).setChildBounds(view, x, y, width, height);
            return;
        }
        Log.e(TAG, "setWidgetPosition: parent is not an IupAndroidFixed (" + (parent != null ? parent.getClass().getName() : "null") + ")");
    }

    @Keep
    public static void setActive(Object widget, boolean active)
    {
        if (widget instanceof View)
        {
            ((View)widget).setEnabled(active);
            return;
        }
        /* Dialog ih->handle becomes an Activity after onCreate swaps it. */
        if (widget instanceof Activity)
            return;
        Log.e(TAG, "setActive: widget is not a View (" + (widget != null ? widget.getClass().getName() : "null") + ")");
    }

    @Keep
    public static void setFocus(Object widget)
    {
        if (widget instanceof View view)
        {
            if (!view.isFocusable()) view.setFocusableInTouchMode(true);
            view.requestFocus();
        }
    }

    @Keep
    public static boolean isActive(Object widget)
    {
        if (widget == null) return true;
        if (widget instanceof View) return ((View)widget).isEnabled();
        if (widget instanceof Activity) return true;
        Log.e(TAG, "isActive: widget is not a View (" + widget.getClass().getName() + ")");
        return true;
    }

    @Keep
    public static void postInvalidate(Object widget)
    {
        if (widget instanceof View) ((View)widget).postInvalidate();
    }

    /* View.invalidate() is UI-thread only; post when called from a worker. */
    @Keep
    public static void invalidateNow(Object widget)
    {
        if (!(widget instanceof View v)) return;
        if (android.os.Looper.myLooper() == android.os.Looper.getMainLooper())
            v.invalidate();
        else
            v.post(new Runnable() { @Override public void run() { v.invalidate(); } });
    }

    @Keep
    public static void performClick(Object widget)
    {
        if (widget instanceof View) ((View)widget).performClick();
    }

    @Keep
    public static void setAccessibleTitle(Object widget, String title)
    {
        if (widget instanceof View) ((View)widget).setContentDescription(title);
    }

    @Keep
    public static void reparentTo(long childIhandlePtr, Object newParentWidget)
    {
        Object childWidget = getObjectFromIhandle(childIhandlePtr);
        if (!(childWidget instanceof View child)) return;
        ViewGroup oldParent = (ViewGroup)child.getParent();
        if (oldParent != null) oldParent.removeView(child);
        ViewGroup newParent = resolveParentViewGroup(newParentWidget);
        if (newParent == null)
        {
            Log.e(TAG, "reparentTo: new parent is not a supported type");
            return;
        }
        newParent.addView(child, new IupAndroidFixed.LayoutParams(0, 0, 0, 0));
    }


    /* APPID/APPNAME are manifest-driven; read-only on Android */

    @Keep
    public static String getAppId()
    {
        IupApplication app = IupApplication.getIupApplication();
        return (app != null) ? app.getPackageName() : null;
    }

    @Keep
    public static String getAppName()
    {
        IupApplication app = IupApplication.getIupApplication();
        if (app == null) return null;
        try
        {
            PackageManager pm = app.getPackageManager();
            ApplicationInfo ai = app.getApplicationInfo();
            CharSequence label = pm.getApplicationLabel(ai);
            return label.toString();
        }
        catch (Throwable t) { return null; }
    }


    /* IupSendKey/IupSendMouse: app-local only; routes via View.dispatch*Event on focused/decor view */

    @Keep
    public static void sendKey(int keyCode, int metaState, int press)
    {
        Activity a = IupApplication.getIupApplication().getCurrentActivity();
        if (a == null) return;
        View target = a.getCurrentFocus();
        if (target == null) target = a.getWindow().getDecorView();
        final View dst = target;
        final long t = SystemClock.uptimeMillis();
        final boolean doPress = (press & 0x01) != 0;
        final boolean doRelease = (press & 0x02) != 0;
        dst.post(new Runnable()
        {
            @Override public void run()
            {
                if (doPress)   dst.dispatchKeyEvent(new KeyEvent(t, t, KeyEvent.ACTION_DOWN, keyCode, 0, metaState));
                if (doRelease) dst.dispatchKeyEvent(new KeyEvent(t, t, KeyEvent.ACTION_UP,   keyCode, 0, metaState));
            }
        });
    }

    @Keep
    public static void sendMouse(int x, int y, int button, int status)
    {
        Activity a = IupApplication.getIupApplication().getCurrentActivity();
        if (a == null) return;
        final View decor = a.getWindow().getDecorView();
        final int px = x, py = y, pstatus = status, pbutton = button;
        decor.post(new Runnable()
        {
            @Override public void run() { doSendMouse(decor, px, py, pbutton, pstatus); }
        });
    }

    private static void doSendMouse(View decor, int x, int y, int button, int status)
    {
        long t = SystemClock.uptimeMillis();
        int buttonState = buttonStateFromIup(button);
        int src = (buttonState != 0) ? InputDevice.SOURCE_MOUSE : InputDevice.SOURCE_TOUCHSCREEN;

        if (status == -1)                            /* motion only */
        {
            dispatchPointer(decor, t, MotionEvent.ACTION_MOVE, x, y, buttonState, src);
        }
        else if (status == 0)                         /* release */
        {
            dispatchPointer(decor, t, MotionEvent.ACTION_UP, x, y, buttonState, src);
        }
        else if (status == 1)                         /* press */
        {
            dispatchPointer(decor, t, MotionEvent.ACTION_DOWN, x, y, buttonState, src);
        }
        else if (status == 2)                         /* double-click */
        {
            dispatchPointer(decor, t,     MotionEvent.ACTION_DOWN, x, y, buttonState, src);
            dispatchPointer(decor, t + 1, MotionEvent.ACTION_UP,   x, y, buttonState, src);
            dispatchPointer(decor, t + 2, MotionEvent.ACTION_DOWN, x, y, buttonState, src);
            dispatchPointer(decor, t + 3, MotionEvent.ACTION_UP,   x, y, buttonState, src);
        }
    }

    private static int buttonStateFromIup(int b)
    {
        switch (b)
        {
            case '1': return MotionEvent.BUTTON_PRIMARY;
            case '2': return MotionEvent.BUTTON_TERTIARY;
            case '3': return MotionEvent.BUTTON_SECONDARY;
            default:  return 0;
        }
    }

    private static void dispatchPointer(View view, long time, int action, int x, int y, int buttonState, int source)
    {
        MotionEvent.PointerProperties pp = new MotionEvent.PointerProperties();
        pp.id = 0;
        pp.toolType = (source == InputDevice.SOURCE_MOUSE) ? MotionEvent.TOOL_TYPE_MOUSE : MotionEvent.TOOL_TYPE_FINGER;
        MotionEvent.PointerCoords pc = new MotionEvent.PointerCoords();
        pc.x = x; pc.y = y; pc.pressure = 1f; pc.size = 1f;
        MotionEvent ev = MotionEvent.obtain(time, time, action, 1,
            new MotionEvent.PointerProperties[] { pp },
            new MotionEvent.PointerCoords[] { pc },
            0, buttonState, 1f, 1f, 0, 0, source, 0);
        view.dispatchTouchEvent(ev);
        ev.recycle();
    }


    /** View top-left in screen coords; used by iupdrvClientToScreen. */
    @Keep
    public static int[] getScreenLocation(Object widget)
    {
        int[] loc = new int[2];
        if (widget instanceof View)
        {
            ((View)widget).getLocationOnScreen(loc);
        }
        return loc;
    }

    /** Display density (scaling factor for dp to px). */
    @Keep
    public static float getDisplayDensity()
    {
        return getContextThemeWrapper().getResources().getDisplayMetrics().density;
    }

    /** packed [widthPx, heightPx, dpi]; size is usable content area (excludes system bars on older APIs). */
    @Keep
    public static int[] getDisplayMetrics()
    {
        android.util.DisplayMetrics dm = getContextThemeWrapper().getResources().getDisplayMetrics();
        return new int[] { dm.widthPixels, dm.heightPixels, dm.densityDpi };
    }

    /** Packed real-display size: [widthPx, heightPx]. Includes system bars. */
    @Keep
    public static int[] getRealDisplayMetrics()
    {
        android.view.WindowManager wm = (android.view.WindowManager)
            getContextThemeWrapper().getSystemService(android.content.Context.WINDOW_SERVICE);
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.R)
        {
            android.graphics.Rect b = wm.getMaximumWindowMetrics().getBounds();
            return new int[] { b.width(), b.height() };
        }
        return realDisplayMetricsLegacy(wm);
    }

    @SuppressWarnings("deprecation")
    private static int[] realDisplayMetricsLegacy(android.view.WindowManager wm)
    {
        android.util.DisplayMetrics dm = new android.util.DisplayMetrics();
        wm.getDefaultDisplay().getRealMetrics(dm);
        return new int[] { dm.widthPixels, dm.heightPixels };
    }

    /** "13", "14", etc. from Build.VERSION.RELEASE. */
    @Keep
    public static String getSystemVersion()
    {
        return android.os.Build.VERSION.RELEASE;
    }

    /** BCP-47 locale tag, e.g. "en-US". */
    @Keep
    public static String getLocaleTag()
    {
        return java.util.Locale.getDefault().toLanguageTag();
    }

    /** "Manufacturer Model", suitable for COMPUTERNAME. */
    @Keep
    public static String getDeviceName()
    {
        return android.os.Build.MANUFACTURER + " " + android.os.Build.MODEL;
    }

    /** Reflects system night-mode setting (and any per-app override via AppCompatDelegate). */
    @Keep
    public static boolean isDarkMode()
    {
        try
        {
            IupApplication app = IupApplication.getIupApplication();
            if (app == null) return false;
            /* Application's config doesn't refresh on Day/Night flip; the Activity's does. */
            Activity a = app.getCurrentActivity();
            android.content.res.Resources res = (a != null) ? a.getResources() : app.getResources();
            int mode = res.getConfiguration().uiMode
                       & android.content.res.Configuration.UI_MODE_NIGHT_MASK;
            return mode == android.content.res.Configuration.UI_MODE_NIGHT_YES;
        }
        catch (Throwable t) { return false; }
    }


    /* mirrors IUP DLG/TXT/MENU globals; pushed from iupAndroidUpdateGlobalColors() */
    public static volatile int paletteDlgBg    = 0xFFEDEDED;
    public static volatile int paletteDlgFg    = 0xFF000000;
    public static volatile int paletteTxtBg    = 0xFFFFFFFF;
    public static volatile int paletteTxtFg    = 0xFF000000;
    public static volatile int paletteMenuBg   = 0xFFB7B7B7;
    public static volatile int paletteMenuFg   = 0xFF000000;
    public static volatile int paletteTxtHlBg  = 0xFF6750A4;
    public static volatile int paletteTxtHlFg  = 0xFFFFFFFF;
    public static volatile int paletteAccent   = 0xFF6750A4;
    public static volatile boolean paletteDark = false;

    /** Called from iupandroid_open.c:iupAndroidUpdateGlobalColors(). */
    @Keep
    public static void setThemePalette(int dlgBg, int dlgFg, int txtBg, int txtFg,
                                       int menuBg, int menuFg,
                                       int txtHlBg, int txtHlFg, int accent,
                                       boolean dark)
    {
        paletteDlgBg   = dlgBg;
        paletteDlgFg   = dlgFg;
        paletteTxtBg   = txtBg;
        paletteTxtFg   = txtFg;
        paletteMenuBg  = menuBg;
        paletteMenuFg  = menuFg;
        paletteTxtHlBg = txtHlBg;
        paletteTxtHlFg = txtHlFg;
        paletteAccent  = accent;
        paletteDark    = dark;
    }

    /** Material3 theme attr by name (e.g. "colorPrimary") -> ARGB; fallback if missing. */
    @Keep
    /* attrName arrives from C at runtime, not from generated R.java; getIdentifier is required. */
    @android.annotation.SuppressLint("DiscouragedApi")
    public static int resolveThemeColorByName(String attrName, int fallback)
    {
        try
        {
            IupApplication app = IupApplication.getIupApplication();
            if (app == null) return fallback;
            int attrId = app.getResources().getIdentifier(attrName, "attr", app.getPackageName());
            if (attrId == 0) attrId = app.getResources().getIdentifier(attrName, "attr", "android");
            if (attrId == 0) return fallback;

            android.content.Context ctx = getThemeContext();
            android.util.TypedValue tv = new android.util.TypedValue();
            android.content.res.Resources.Theme theme = ctx.getTheme();
            if (!theme.resolveAttribute(attrId, tv, true)) return fallback;
            if (tv.type >= android.util.TypedValue.TYPE_FIRST_COLOR_INT
                && tv.type <= android.util.TypedValue.TYPE_LAST_COLOR_INT)
                return tv.data;
            if (tv.resourceId != 0)
                return androidx.core.content.res.ResourcesCompat.getColor(
                    ctx.getResources(), tv.resourceId, theme);
            return fallback;
        }
        catch (Throwable t) { return fallback; }
    }

    /** drops the cached AppTheme wrapper so resolveThemeColor sees post-DayNight resources. */
    @Keep
    public static synchronized void resetContextThemeWrapper()
    {
        sContextThemeWrapper = null;
    }

    /** Theme-resolution context that always carries Material3 attrs, including during the launch-activity phase. */
    public static android.content.Context getThemeContext()
    {
        return getContextThemeWrapper();
    }

    /** Mixes two ARGB colors by weight (0..1 = amount of b). */
    public static int blendColor(int a, int b, float t)
    {
        if (t <= 0f) return a;
        if (t >= 1f) return b;
        int ar = (a >> 16) & 0xFF, ag = (a >> 8) & 0xFF, ab = a & 0xFF;
        int br = (b >> 16) & 0xFF, bg = (b >> 8) & 0xFF, bb = b & 0xFF;
        int rr = (int)(ar + (br - ar) * t);
        int rg = (int)(ag + (bg - ag) * t);
        int rb = (int)(ab + (bb - ab) * t);
        return 0xFF000000 | (rr << 16) | (rg << 8) | rb;
    }

    /** Android sandboxes per-user name behind system-only permissions; return a stable placeholder. */
    @Keep
    public static String getUserName()
    {
        return "user";
    }

    @Keep
    public static int getMonitorsCount()
    {
        try
        {
            IupApplication app = IupApplication.getIupApplication();
            if (app == null) return 1;
            android.hardware.display.DisplayManager dm =
                (android.hardware.display.DisplayManager) app.getSystemService(Context.DISPLAY_SERVICE);
            if (dm == null) return 1;
            android.view.Display[] displays = dm.getDisplays();
            return (displays != null && displays.length > 0) ? displays.length : 1;
        }
        catch (Throwable t) { return 1; }
    }

    /** Newline-separated "x y w h" lines, one per Display. */
    @Keep
    public static String getMonitorsInfo()
    {
        try
        {
            IupApplication app = IupApplication.getIupApplication();
            if (app == null) return "0 0 0 0\n";
            android.hardware.display.DisplayManager dm =
                (android.hardware.display.DisplayManager) app.getSystemService(Context.DISPLAY_SERVICE);
            if (dm == null) return "0 0 0 0\n";
            android.view.Display[] displays = dm.getDisplays();
            if (displays == null || displays.length == 0) return "0 0 0 0\n";

            StringBuilder sb = new StringBuilder();
            for (android.view.Display d : displays)
            {
                int[] wh = displaySize(app, d);
                sb.append("0 0 ").append(wh[0]).append(' ').append(wh[1]).append('\n');
            }
            return sb.toString();
        }
        catch (Throwable t) { return "0 0 0 0\n"; }
    }

    /** union of all Display sizes; Android exposes no screen-space origins. */
    @Keep
    public static int[] getVirtualScreen()
    {
        try
        {
            IupApplication app = IupApplication.getIupApplication();
            if (app == null) return new int[] { 0, 0, 0, 0 };
            android.hardware.display.DisplayManager dm =
                (android.hardware.display.DisplayManager) app.getSystemService(Context.DISPLAY_SERVICE);
            if (dm == null) return new int[] { 0, 0, 0, 0 };
            android.view.Display[] displays = dm.getDisplays();
            if (displays == null || displays.length == 0) return new int[] { 0, 0, 0, 0 };

            int maxW = 0, sumH = 0;
            for (android.view.Display d : displays)
            {
                int[] wh = displaySize(app, d);
                if (wh[0] > maxW) maxW = wh[0];
                sumH += wh[1];
            }
            return new int[] { 0, 0, maxW, sumH };
        }
        catch (Throwable t) { return new int[] { 0, 0, 0, 0 }; }
    }

    private static int[] displaySize(Context ctx, android.view.Display d)
    {
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.R)
        {
            Context displayCtx = ctx.createDisplayContext(d);
            android.view.WindowManager wm = displayCtx.getSystemService(android.view.WindowManager.class);
            if (wm != null)
            {
                android.graphics.Rect b = wm.getMaximumWindowMetrics().getBounds();
                return new int[] { b.width(), b.height() };
            }
        }
        return displaySizeLegacy(d);
    }

    @SuppressWarnings("deprecation")
    private static int[] displaySizeLegacy(android.view.Display d)
    {
        android.graphics.Point size = new android.graphics.Point();
        d.getRealSize(size);
        return new int[] { size.x, size.y };
    }

    /** Unwraps an Activity from a View's context chain, if present. */
    public static Activity getActivity(View view)
    {
        Context context = view.getContext();
        while (context instanceof ContextWrapper)
        {
            if (context instanceof Activity)
            {
                return (Activity)context;
            }
            context = ((ContextWrapper)context).getBaseContext();
        }
        return null;
    }

    /* Nested Looper sub-loop via reflection (no public API); reflection failure sets done[0]=true. */
    private static Method sMessageQueueNext;
    private static Method sMessageRecycleUnchecked;
    private static Field  sMessageTarget;


    /* gated on a resumed Activity; pumping mid-lifecycle reenters TransactionExecutor and crashes */
    @Keep
    public static void loopStepFlush()
    {
        if (Looper.myLooper() != Looper.getMainLooper()) return;
        if (IupActivity.currentActivity() == null) return;
        /* Fire-and-forget vsync nudge; we cannot block waiting since the frame runs on this same thread. */
        Choreographer.getInstance().postFrameCallback(t -> {});
    }

    @Keep
    public static void setViewVisible(Object widget, boolean visible)
    {
        if (!(widget instanceof android.view.View)) return;
        ((android.view.View)widget).setVisibility(visible ? android.view.View.VISIBLE : android.view.View.GONE);
    }

    @Keep
    public static boolean isViewVisible(Object widget)
    {
        if (!(widget instanceof android.view.View)) return false;
        return ((android.view.View)widget).getVisibility() == android.view.View.VISIBLE;
    }

    @android.annotation.SuppressLint({"DiscouragedPrivateApi", "PrivateApi", "BlockedPrivateApi", "SoonBlockedPrivateApi"})
    public static void pumpUntilDone(boolean[] done)
    {
        try
        {
            if (sMessageQueueNext == null)
            {
                sMessageQueueNext = MessageQueue.class.getDeclaredMethod("next");
                sMessageQueueNext.setAccessible(true);
            }
            if (sMessageRecycleUnchecked == null)
            {
                sMessageRecycleUnchecked = Message.class.getDeclaredMethod("recycleUnchecked");
                sMessageRecycleUnchecked.setAccessible(true);
            }
            if (sMessageTarget == null)
            {
                sMessageTarget = Message.class.getDeclaredField("target");
                sMessageTarget.setAccessible(true);
            }

            MessageQueue queue = Looper.myQueue();
            while (!done[0])
            {
                Message msg = (Message)sMessageQueueNext.invoke(queue);
                if (msg == null) return;

                Handler target = (Handler)sMessageTarget.get(msg);
                if (target != null) target.dispatchMessage(msg);

                /* recycleUnchecked is best-effort; failure only leaks a Message. */
                try { sMessageRecycleUnchecked.invoke(msg); }
                catch (Throwable ignored) {}
            }
        }
        catch (Throwable t)
        {
            Log.e(TAG, "IupCommon.pumpUntilDone failed", t);
            done[0] = true;
        }
    }


    /* Stack of modal-popup pump flags; IupActivity.onDestroy pops via modalPumpExitTopmost. */
    private static final java.util.ArrayDeque<boolean[]> sModalPumpStack = new java.util.ArrayDeque<>();

    @Keep
    public static void modalPumpRun()
    {
        boolean[] done = { false };
        sModalPumpStack.push(done);
        try { pumpUntilDone(done); }
        finally { sModalPumpStack.pop(); }
    }

    @Keep
    public static void modalPumpExitTopmost()
    {
        boolean[] top = sModalPumpStack.peek();
        if (top != null) top[0] = true;
    }


    /* AppTheme-bearing wrapper used when no Activity exists yet (early widget creation) */
    private static ContextThemeWrapper sContextThemeWrapper = null;
    public static synchronized ContextThemeWrapper getContextThemeWrapper()
    {
        if (sContextThemeWrapper == null)
        {
            IupApplication app = IupApplication.getIupApplication();
            /* Activity config tracks Day/Night, Application's doesn't; wrap from the Activity */
            Activity a = app.getCurrentActivity();
            android.content.Context base = (a != null)
                ? app.createConfigurationContext(a.getResources().getConfiguration())
                : app;
            sContextThemeWrapper = new ContextThemeWrapper(base, R.style.AppTheme);
        }
        return sContextThemeWrapper;
    }

    private static ViewGroup resolveParentViewGroup(Object parentWidget)
    {
        /* IUP parent = Activity: return its IupAndroidFixed content view, not the decor view */
        if (parentWidget instanceof IupActivity)
        {
            return ((IupActivity)parentWidget).getRootFixed();
        }
        if (parentWidget instanceof ViewGroup)
        {
            return (ViewGroup)parentWidget;
        }
        return null;
    }
}
