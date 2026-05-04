package io.github.gen2brain.iupgo;

import android.app.Activity;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.util.Log;
import android.view.View;
import android.view.Window;

import androidx.annotation.Keep;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.view.WindowInsetsControllerCompat;


/* dialog actions from C; handle is IupAndroidFixed pre-Activity, IupActivity after */
public final class IupDialogHelper
{
    private static final String TAG = "Iup";

    private IupDialogHelper() {}


    @Keep
    public static void setTitle(Object handle, String title)
    {
        if (handle instanceof Activity)
        {
            ((Activity)handle).setTitle(title != null ? title : "");
        }
    }

    @Keep
    public static void setIcon(Object handle, Bitmap bmp)
    {
        if (!(handle instanceof AppCompatActivity)) return;
        ActionBar bar = ((AppCompatActivity)handle).getSupportActionBar();
        if (bar == null) return;
        if (bmp != null)
        {
            Drawable d = new BitmapDrawable(((Activity)handle).getResources(), bmp);
            bar.setLogo(d);
            bar.setDisplayUseLogoEnabled(true);
            bar.setDisplayShowHomeEnabled(true);
        }
        else
        {
            bar.setLogo(null);
            bar.setDisplayUseLogoEnabled(false);
        }
    }

    @Keep
    public static void setBgColor(Object handle, int r, int g, int b)
    {
        IupAndroidFixed rootFixed = resolveRootFixed(handle);
        if (rootFixed != null)
        {
            rootFixed.setBackgroundColor(Color.rgb(r, g, b));
        }
    }

    @Keep
    public static void setFullscreen(Object handle, boolean enable)
    {
        if (!(handle instanceof Activity activity))
        {
            return;
        }
        Window window = activity.getWindow();
        View decor = window.getDecorView();

        WindowInsetsControllerCompat controller = WindowCompat.getInsetsController(window, decor);

        if (enable)
        {
            controller.setSystemBarsBehavior(WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
            controller.hide(WindowInsetsCompat.Type.systemBars());
        }
        else
        {
            controller.show(WindowInsetsCompat.Type.systemBars());
        }
    }

    @Keep
    public static void setHideTitleBar(Object handle, boolean hide)
    {
        if (!(handle instanceof AppCompatActivity))
        {
            return;
        }
        ActionBar bar = ((AppCompatActivity)handle).getSupportActionBar();
        if (bar == null)
        {
            /* no ActionBar in this theme (e.g. NoActionBar variant) */
            return;
        }
        if (hide)
        {
            bar.hide();
        }
        else
        {
            bar.show();
        }
    }

    @Keep
    public static void setTitleCentered(Object handle, boolean centered)
    {
        if (handle instanceof IupActivity ia) ia.setTitleCentered(centered);
    }

    @Keep
    public static void setOpacity(Object handle, float alpha)
    {
        if (!(handle instanceof Activity activity)) return;
        if (alpha < 0f) alpha = 0f;
        if (alpha > 1f) alpha = 1f;
        Window window = activity.getWindow();
        android.view.WindowManager.LayoutParams lp = window.getAttributes();
        lp.alpha = alpha;
        window.setAttributes(lp);
    }

    @Keep
    public static void bringToFront(Object handle)
    {
        if (!(handle instanceof Activity activity)) return;
        /* Reorder existing instance to top of task; no REORDER_TASKS permission needed. */
        Intent intent = new Intent(activity, activity.getClass());
        intent.addFlags(Intent.FLAG_ACTIVITY_REORDER_TO_FRONT);
        activity.startActivity(intent);
    }

    @Keep
    public static void moveToBack(Object handle)
    {
        if (handle instanceof Activity)
        {
            ((Activity)handle).moveTaskToBack(false);
        }
    }

    /** Finishes the hosting Activity. Counterpart of IupExitLoop on Android. */
    @Keep
    public static void finish(Object handle)
    {
        if (handle instanceof Activity)
        {
            ((Activity)handle).finish();
        }
    }

    @Keep
    public static void setTitleBarStyle(Object handle, String style)
    {
        if (handle instanceof IupActivity activity)
            activity.setTitleBarStyle(style);
    }

    @Keep
    public static void setOrientation(Object handle, String orientation)
    {
        if (!(handle instanceof Activity activity) || orientation == null)
        {
            return;
        }
        int mode;
        if ("LANDSCAPE".equalsIgnoreCase(orientation))
        {
            mode = ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE;
        }
        else if ("PORTRAIT".equalsIgnoreCase(orientation))
        {
            mode = ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;
        }
        else if ("SENSOR".equalsIgnoreCase(orientation))
        {
            mode = ActivityInfo.SCREEN_ORIENTATION_SENSOR;
        }
        else if ("UNSPECIFIED".equalsIgnoreCase(orientation))
        {
            mode = ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED;
        }
        else
        {
            Log.w(TAG, "IupDialogHelper.setOrientation: unknown value '" + orientation + "'");
            return;
        }
        activity.setRequestedOrientation(mode);
    }

    private static IupAndroidFixed resolveRootFixed(Object handle)
    {
        if (handle instanceof IupAndroidFixed)
        {
            return (IupAndroidFixed)handle;
        }
        if (handle instanceof IupActivity)
        {
            return ((IupActivity)handle).getRootFixed();
        }
        return null;
    }
}
