package io.github.gen2brain.iupgo;

import android.graphics.Typeface;

import androidx.annotation.Keep;
import androidx.appcompat.view.ContextThemeWrapper;


public final class IupFrameHelper
{
    private IupFrameHelper() {}


    @Keep
    public static IupAndroidFrame createFrame(final long ihandlePtr)
    {
        ContextThemeWrapper themeContext = IupCommon.getContextThemeWrapper();
        return new IupAndroidFrame(themeContext);
    }

    @Keep
    public static IupAndroidFixed getInner(IupAndroidFrame frame)
    {
        return frame.getInner();
    }

    @Keep
    public static void setTitle(IupAndroidFrame frame, String title)
    {
        frame.setFrameTitle(title);
    }

    @Keep
    public static void setFgColor(IupAndroidFrame frame, int r, int g, int b)
    {
        frame.setFrameTitleColor(r, g, b);
    }

    @Keep
    public static void setBgColor(IupAndroidFrame frame, int r, int g, int b)
    {
        frame.setFrameBgColor(r, g, b);
    }

    @Keep
    public static void setStrokeColor(IupAndroidFrame frame, int r, int g, int b)
    {
        frame.setFrameStrokeColor(r, g, b);
    }

    @Keep
    public static int getTitleHeight(IupAndroidFrame frame)
    {
        return frame.getTitleHeight();
    }

    @Keep
    public static int getStrokePx(IupAndroidFrame frame)
    {
        return frame.getFrameStrokePx();
    }

    /* style: 0=normal 1=bold 2=italic 3=bold-italic; sizeUnit: TypedValue.COMPLEX_UNIT_*; size 0 = leave unchanged. */
    @Keep
    public static void setTitleFont(IupAndroidFrame frame, String family, int style, int sizeUnit, float size)
    {
        Typeface base = (family != null && !family.isEmpty())
            ? Typeface.create(family, style)
            : Typeface.create((Typeface) null, style);
        frame.setTitleFont(base, sizeUnit, size);
    }

    @Keep
    public static void setSunken(IupAndroidFrame frame, boolean sunken)
    {
        frame.setSunken(sunken);
    }
}
