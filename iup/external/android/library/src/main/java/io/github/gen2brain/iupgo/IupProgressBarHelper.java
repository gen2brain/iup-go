package io.github.gen2brain.iupgo;

import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.Color;
import android.util.AttributeSet;
import android.util.TypedValue;
import android.widget.ProgressBar;

import androidx.annotation.Keep;
import androidx.appcompat.view.ContextThemeWrapper;

import com.google.android.material.color.MaterialColors;
import com.google.android.material.progressindicator.BaseProgressIndicator;
import com.google.android.material.progressindicator.CircularProgressIndicator;
import com.google.android.material.progressindicator.LinearProgressIndicator;

import java.util.WeakHashMap;


public final class IupProgressBarHelper
{
    /* Material3 default 4dp reads as a hairline; 8dp matches sibling controls. */
    private static final float TRACK_THICKNESS_DP = 8.0f;

    private static final WeakHashMap<BaseProgressIndicator<?>, Boolean> sThemableIndicators = new WeakHashMap<>();

    static {
        IupTheme.register(IupProgressBarHelper::refreshAllIndicators);
    }

    private IupProgressBarHelper() {}

    public static int getTrackThicknessPx()
    {
        return (int)TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP,
            TRACK_THICKNESS_DP,
            IupCommon.getContextThemeWrapper().getResources().getDisplayMetrics());
    }

    /* Indicator colors cache from theme attrs at construction; refresh on flip unless user pinned FG/BG. */
    private static final class IupLinearProgressIndicator extends LinearProgressIndicator
    {
        boolean userFg;
        boolean userBg;
        IupLinearProgressIndicator(Context ctx) { super(ctx); }
    }

    private static final class IupCircularProgressIndicator extends CircularProgressIndicator
    {
        boolean userFg;
        boolean userBg;
        IupCircularProgressIndicator(Context ctx) { super(ctx); }
    }

    @Keep
    public static ProgressBar createProgressBar(final long ihandlePtr, int width, int height, boolean vertical, boolean marquee, boolean circular)
    {
        ContextThemeWrapper themeContext = IupCommon.getContextThemeWrapper();

        if (circular)
        {
            IupCircularProgressIndicator cpi = new IupCircularProgressIndicator(themeContext);
            cpi.setIndeterminate(marquee);
            sThemableIndicators.put(cpi, Boolean.TRUE);
            return cpi;
        }

        /* Material has no vertical indicator; fall back to a rotated plain ProgressBar. */
        if (vertical)
            return new IupProgressBarVertical(themeContext, null, android.R.attr.progressBarStyleHorizontal);

        IupLinearProgressIndicator lpi = new IupLinearProgressIndicator(themeContext);
        lpi.setIndeterminate(marquee);
        lpi.setTrackThickness(getTrackThicknessPx());
        sThemableIndicators.put(lpi, Boolean.TRUE);
        return lpi;
    }

    @Keep
    public static void setProgressBarValues(final long ihandlePtr, ProgressBar progressBar, double min, double max, double value)
    {
        /* API < 26 wants int [0,100]. */
        double normalized = (100.0 / (max - min)) * (value - min);
        progressBar.setProgress((int)(normalized + 0.5));
    }

    @Keep
    public static void setIndeterminate(final long ihandlePtr, ProgressBar progressBar, boolean enableMarquee)
    {
        progressBar.setIndeterminate(enableMarquee);
    }

    /* Vertical fallback is a plain ProgressBar; use the generic progress-tint path there. */
    @Keep
    public static void setIndicatorColor(ProgressBar progressBar, int r, int g, int b)
    {
        int color = Color.rgb(r, g, b);
        if (progressBar instanceof IupLinearProgressIndicator lpi)
        {
            lpi.setIndicatorColor(color);
            lpi.userFg = true;
        }
        else if (progressBar instanceof IupCircularProgressIndicator cpi)
        {
            cpi.setIndicatorColor(color);
            cpi.userFg = true;
        }
        else
        {
            progressBar.setProgressTintList(ColorStateList.valueOf(color));
        }
    }

    @Keep
    public static void setTrackColor(ProgressBar progressBar, int r, int g, int b)
    {
        int color = Color.rgb(r, g, b);
        if (progressBar instanceof IupLinearProgressIndicator lpi)
        {
            lpi.setTrackColor(color);
            lpi.userBg = true;
        }
        else if (progressBar instanceof IupCircularProgressIndicator cpi)
        {
            cpi.setTrackColor(color);
            cpi.userBg = true;
        }
        else
        {
            progressBar.setProgressBackgroundTintList(ColorStateList.valueOf(color));
        }
    }

    private static void refreshAllIndicators()
    {
        for (BaseProgressIndicator<?> ind : new java.util.ArrayList<>(sThemableIndicators.keySet()))
        {
            if (ind != null) applyIndicatorPalette(ind);
        }
    }

    private static void applyIndicatorPalette(BaseProgressIndicator<?> ind)
    {
        Context ctx = IupCommon.getThemeContext();
        int primary = MaterialColors.getColor(ctx,
            androidx.appcompat.R.attr.colorPrimary, Color.BLUE);

        if (ind instanceof IupLinearProgressIndicator lpi)
        {
            if (!lpi.userFg) lpi.setIndicatorColor(primary);
            if (!lpi.userBg)
            {
                int track = MaterialColors.compositeARGBWithAlpha(primary, (int)(0.24f * 255));
                lpi.setTrackColor(track);
            }
        }
        else if (ind instanceof IupCircularProgressIndicator cpi)
        {
            if (!cpi.userFg) cpi.setIndicatorColor(primary);
        }
    }
}
