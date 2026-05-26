package io.github.gen2brain.iupgo;

import android.view.View;

import androidx.annotation.Keep;
import androidx.appcompat.view.ContextThemeWrapper;

import com.google.android.material.slider.Slider;


public final class IupValHelper
{
    private IupValHelper() {}

    /* Slider caches colorPrimary into track/thumb/halo tints at construction; rebuild on theme flip. */
    private static final java.util.WeakHashMap<Slider, Boolean> sThemableSliders = new java.util.WeakHashMap<>();

    static {
        IupTheme.register(IupValHelper::refreshAllSliders);
    }

    private static void refreshAllSliders()
    {
        for (Slider s : new java.util.ArrayList<>(sThemableSliders.keySet()))
        {
            if (s != null) applySliderPalette(s);
        }
    }

    private static void applySliderPalette(Slider s)
    {
        android.content.Context ctx = IupCommon.getThemeContext();
        int primary  = com.google.android.material.color.MaterialColors.getColor(ctx,
            com.google.android.material.R.attr.colorPrimary, android.graphics.Color.BLUE);
        int surface  = com.google.android.material.color.MaterialColors.getColor(ctx,
            com.google.android.material.R.attr.colorSurfaceContainerHighest, android.graphics.Color.LTGRAY);
        android.content.res.ColorStateList primaryCsl = android.content.res.ColorStateList.valueOf(primary);
        android.content.res.ColorStateList surfaceCsl = android.content.res.ColorStateList.valueOf(surface);
        s.setThumbTintList(primaryCsl);
        s.setTrackActiveTintList(primaryCsl);
        s.setTrackInactiveTintList(surfaceCsl);
        s.setHaloTintList(primaryCsl);
        s.setTickActiveTintList(surfaceCsl);
        s.setTickInactiveTintList(primaryCsl);
    }


    @Keep
    public static Slider createHorizontal(final long ihandlePtr)
    {
        ContextThemeWrapper themeContext = IupCommon.getContextThemeWrapper();
        Slider slider = new Slider(themeContext);
        applySliderPalette(slider);
        sThemableSliders.put(slider, Boolean.TRUE);
        wireCallbacks(slider, ihandlePtr);
        return slider;
    }

    /** Creates a vertical slider (Slider rotated -90 inside a sizing wrapper). */
    @Keep
    public static IupAndroidVerticalSlider createVertical(final long ihandlePtr)
    {
        ContextThemeWrapper themeContext = IupCommon.getContextThemeWrapper();
        IupAndroidVerticalSlider wrapper = new IupAndroidVerticalSlider(themeContext);
        Slider s = wrapper.getSlider();
        applySliderPalette(s);
        sThemableSliders.put(s, Boolean.TRUE);
        wireCallbacks(s, ihandlePtr);
        return wrapper;
    }

    public static Slider unwrap(View widget)
    {
        if (widget instanceof Slider) return (Slider)widget;
        if (widget instanceof IupAndroidVerticalSlider) return ((IupAndroidVerticalSlider)widget).getSlider();
        return null;
    }

    @Keep
    public static void setValue(View widget, float value)
    {
        Slider s = unwrap(widget);
        if (s == null) return;
        /* Slider throws on out-of-range or off-step values; clamp defensively */
        float clamped = Math.max(s.getValueFrom(), Math.min(s.getValueTo(), value));
        s.setValue(clamped);
    }

    @Keep
    public static void setRange(View widget, float min, float max)
    {
        Slider s = unwrap(widget);
        if (s == null || min >= max) return;
        /* Order matters: Slider rejects setValueFrom > current valueTo. */
        float current = s.getValue();
        if (max < current) { s.setValueFrom(min); s.setValueTo(max); }
        else { s.setValueTo(max); s.setValueFrom(min); }
    }

    @Keep
    public static void setStep(View widget, float step)
    {
        Slider s = unwrap(widget);
        if (s == null) return;
        s.setStepSize(Math.max(0f, step));
    }


    private static void wireCallbacks(final Slider slider, final long ihandlePtr)
    {
        slider.addOnChangeListener((s, value, fromUser) -> {
            if (!fromUser) return;
            dispatchValueChanged(ihandlePtr, value);
        });
    }

    public static native void dispatchValueChanged(long ihandlePtr, float value);
}
