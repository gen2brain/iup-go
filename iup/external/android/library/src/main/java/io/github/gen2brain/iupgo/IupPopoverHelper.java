package io.github.gen2brain.iupgo;

import android.graphics.drawable.ColorDrawable;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.PopupWindow;
import androidx.annotation.Keep;


/* IupPopover via PopupWindow; content is an IupAndroidFixed for absolute child positioning */
public final class IupPopoverHelper
{
    private IupPopoverHelper() {}


    @Keep
    public static PopupWindow create(final long ihandlePtr)
    {
        IupAndroidFixed inner = new IupAndroidFixed(IupCommon.getContextThemeWrapper());

        PopupWindow pw = new PopupWindow(inner,
            ViewGroup.LayoutParams.WRAP_CONTENT,
            ViewGroup.LayoutParams.WRAP_CONTENT,
            false /* focusable toggled by show() */);

        /* setBackgroundDrawable is also required for outside-touch dismiss. */
        pw.setBackgroundDrawable(new ColorDrawable(IupCommon.paletteDlgBg));
        pw.setElevation(8f);

        pw.setOnDismissListener(() -> {
            /* IUP_HIDE = 4 */
            IupCommon.handleIupCallbackInt(ihandlePtr, "SHOW_CB", 4);
        });

        return pw;
    }

    /** Returns the inner IUP container so C can parent IUP widgets into it. */
    @Keep
    public static IupAndroidFixed getInner(PopupWindow pw)
    {
        return (IupAndroidFixed) pw.getContentView();
    }

    /* x/y are screen coords (from iupPopoverCalcPosition). */
    @Keep
    public static void show(PopupWindow pw, View anchor, int x, int y, int width, int height, boolean autohide)
    {
        pw.setOutsideTouchable(autohide);
        pw.setFocusable(autohide);
        if (width > 0)  pw.setWidth(width);
        if (height > 0) pw.setHeight(height);
        pw.showAtLocation(anchor, Gravity.NO_GRAVITY, x, y);
    }

    @Keep
    public static void dismiss(PopupWindow pw)
    {
        pw.dismiss();
    }

    @Keep
    public static boolean isShowing(PopupWindow pw)
    {
        return pw.isShowing();
    }
}
