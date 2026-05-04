package io.github.gen2brain.iupgo;

import android.view.View;

import androidx.annotation.Keep;
import androidx.appcompat.widget.TooltipCompat;


public final class IupTipHelper
{
    private IupTipHelper() {}

    @Keep
    public static void setTip(Object widget, String text)
    {
        if (!(widget instanceof View)) return;
        TooltipCompat.setTooltipText((View)widget, (text != null && !text.isEmpty()) ? text : null);
    }

    /* show via long-click (same path as hover); no public cancel API, hide is best-effort */
    @Keep
    public static void setTipVisible(Object widget, boolean visible)
    {
        if (!(widget instanceof View v)) return;
        if (visible) v.performLongClick();
        else v.cancelLongPress();
    }
}
