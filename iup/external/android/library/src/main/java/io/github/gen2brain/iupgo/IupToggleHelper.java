package io.github.gen2brain.iupgo;

import android.content.res.ColorStateList;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.StateListDrawable;
import android.view.Gravity;
import android.view.View;
import android.widget.CompoundButton;

import androidx.annotation.Keep;
import androidx.appcompat.view.ContextThemeWrapper;

import com.google.android.material.button.MaterialButton;
import com.google.android.material.checkbox.MaterialCheckBox;
import com.google.android.material.color.MaterialColors;
import com.google.android.material.materialswitch.MaterialSwitch;
import com.google.android.material.radiobutton.MaterialRadioButton;


/* IupToggle: checkbox/radio/switch for TEXT, checkable MaterialButton for IMAGE. */
public final class IupToggleHelper
{
    private IupToggleHelper() {}

    /* Image-toggle outlined stroke captures colorPrimary at construction; rebuild it on theme flip. Value = FLAT. */
    private static final java.util.WeakHashMap<MaterialButton, Boolean> sThemableImageToggles = new java.util.WeakHashMap<>();
    /* CompoundButton (checkbox/switch/radio) tint state-lists also resolve at construction. */
    private static final java.util.WeakHashMap<CompoundButton, Boolean> sThemableTextToggles = new java.util.WeakHashMap<>();
    /* no native 3-state click cycle; track tri-state (0/1/-1) per widget */
    private static final java.util.WeakHashMap<MaterialCheckBox, Integer> sThreeStateValue = new java.util.WeakHashMap<>();

    static {
        IupTheme.register(IupToggleHelper::refreshAllImageToggles);
        IupTheme.register(IupToggleHelper::refreshAllTextToggles);
    }

    private static void refreshAllImageToggles()
    {
        for (MaterialButton btn : new java.util.ArrayList<>(sThemableImageToggles.keySet()))
        {
            if (btn == null) continue;
            applyImageToggleStroke(btn);
        }
    }

    private static void refreshAllTextToggles()
    {
        android.content.Context ctx = IupCommon.getThemeContext();
        for (CompoundButton cb : new java.util.ArrayList<>(sThemableTextToggles.keySet()))
        {
            if (cb == null) continue;
            applyTextTogglePalette(cb, ctx);
        }
    }

    private static void applyTextTogglePalette(CompoundButton cb, android.content.Context ctx)
    {
        if (cb instanceof MaterialCheckBox mcb)
        {
            mcb.setButtonTintList(androidx.appcompat.content.res.AppCompatResources.getColorStateList(
                ctx, com.google.android.material.R.color.m3_checkbox_button_tint));
            mcb.setButtonIconTintList(androidx.appcompat.content.res.AppCompatResources.getColorStateList(
                ctx, com.google.android.material.R.color.m3_checkbox_button_icon_tint));
        }
        else if (cb instanceof MaterialSwitch ms)
        {
            /* MaterialSwitch's style references mtrl_* (not m3_*) and pulls four tints. */
            ms.setThumbTintList(androidx.appcompat.content.res.AppCompatResources.getColorStateList(
                ctx, com.google.android.material.R.color.mtrl_switch_thumb_tint));
            ms.setThumbIconTintList(androidx.appcompat.content.res.AppCompatResources.getColorStateList(
                ctx, com.google.android.material.R.color.mtrl_switch_thumb_icon_tint));
            ms.setTrackTintList(androidx.appcompat.content.res.AppCompatResources.getColorStateList(
                ctx, com.google.android.material.R.color.mtrl_switch_track_tint));
            ms.setTrackDecorationTintList(androidx.appcompat.content.res.AppCompatResources.getColorStateList(
                ctx, com.google.android.material.R.color.mtrl_switch_track_decoration_tint));
        }
        else if (cb instanceof MaterialRadioButton mrb)
        {
            mrb.setButtonTintList(androidx.appcompat.content.res.AppCompatResources.getColorStateList(
                ctx, com.google.android.material.R.color.m3_radiobutton_button_tint));
        }
    }

    /* checked: primary stroke; unchecked: colorOutline, or transparent when FLAT */
    private static void applyImageToggleStroke(MaterialButton btn)
    {
        android.content.Context ctx = IupCommon.getThemeContext();
        int primary = MaterialColors.getColor(ctx, com.google.android.material.R.attr.colorPrimary, Color.BLUE);
        Boolean flat = sThemableImageToggles.get(btn);
        int unchecked = (flat != null && flat) ? Color.TRANSPARENT
            : MaterialColors.getColor(ctx, com.google.android.material.R.attr.colorOutline, Color.GRAY);
        ColorStateList stroke = new ColorStateList(
            new int[][] {
                new int[] { android.R.attr.state_checked },
                new int[0]
            },
            new int[] { primary, unchecked });
        btn.setStrokeColor(stroke);
        btn.setRippleColor(androidx.appcompat.content.res.AppCompatResources.getColorStateList(
            ctx, com.google.android.material.R.color.m3_text_button_ripple_color_selector));
    }


    /* setChecked fires the listener even for programmatic changes; toggled around setValue/setValueState so attribute replay does not dispatch ACTION. */
    private static boolean sSuppressDispatch = false;

    @Keep
    public static View createCheckbox(final long ihandlePtr)
    {
        ContextThemeWrapper ctx = IupCommon.getContextThemeWrapper();
        MaterialCheckBox cb = new MaterialCheckBox(ctx);
        /* Tri-state-aware listener: STATE_INDETERMINATE -> IUP -1 (NOTDEF). */
        cb.addOnCheckedStateChangedListener((cbox, state) -> {
            if (sSuppressDispatch) return;
            if (sThreeStateValue.containsKey(cbox))
            {
                /* drive OFF -> ON -> NOTDEF -> OFF */
                Integer prevObj = sThreeStateValue.get(cbox);
                int prev = (prevObj == null) ? 0 : prevObj;
                int next = (prev == 0) ? 1 : (prev == 1) ? -1 : 0;
                sThreeStateValue.put(cbox, next);
                int target = (next < 0) ? MaterialCheckBox.STATE_INDETERMINATE
                           : (next > 0) ? MaterialCheckBox.STATE_CHECKED
                           : MaterialCheckBox.STATE_UNCHECKED;
                sSuppressDispatch = true;
                try { if (cbox.getCheckedState() != target) cbox.setCheckedState(target); }
                finally { sSuppressDispatch = false; }
                cbox.post(() -> dispatchAction(ihandlePtr, next));
                return;
            }
            int iupState = (state == MaterialCheckBox.STATE_INDETERMINATE) ? -1
                         : (state == MaterialCheckBox.STATE_CHECKED) ? 1 : 0;
            cbox.post(() -> dispatchAction(ihandlePtr, iupState));
        });
        sThemableTextToggles.put(cb, Boolean.TRUE);
        return cb;
    }

    @Keep
    public static View createRadio(final long ihandlePtr)
    {
        ContextThemeWrapper ctx = IupCommon.getContextThemeWrapper();
        MaterialRadioButton rb = new MaterialRadioButton(ctx);
        wireCheckedChange(rb, ihandlePtr);
        sThemableTextToggles.put(rb, Boolean.TRUE);
        return rb;
    }

    @Keep
    public static View createSwitch(final long ihandlePtr)
    {
        ContextThemeWrapper ctx = IupCommon.getContextThemeWrapper();
        MaterialSwitch sw = new MaterialSwitch(ctx);
        wireCheckedChange(sw, ihandlePtr);
        sThemableTextToggles.put(sw, Boolean.TRUE);
        return sw;
    }

    @Keep
    public static View createImageToggle(final long ihandlePtr)
    {
        ContextThemeWrapper ctx = IupCommon.getContextThemeWrapper();
        /* outlined style: clear unchecked/checked/disabled distinction without burying the image */
        MaterialButton btn = new MaterialButton(ctx, null,
            com.google.android.material.R.attr.materialButtonOutlinedStyle);
        btn.setCheckable(true);
        btn.setText("");
        /* preserve bitmap colors; default tint is theme primary */
        btn.setIconTint(null);
        btn.setIconGravity(MaterialButton.ICON_GRAVITY_TEXT_START);
        btn.setIconPadding(0);
        btn.setGravity(android.view.Gravity.CENTER);
        /* Material3 default radius would round square icons into circles; pin to small rounded-square */
        btn.setCornerRadius((int)(6 * IupCommon.getDisplayDensity()));
        /* drive stroke visibility off checked state; outlined-style is always-on otherwise */
        applyImageToggleStroke(btn);
        sThemableImageToggles.put(btn, Boolean.FALSE);
        btn.setInsetTop(0);
        btn.setInsetBottom(0);
        btn.setMinHeight(0);
        btn.setMinimumHeight(0);
        btn.setMinWidth(0);
        btn.setMinimumWidth(0);
        /* Defer; sync ACTION CB that opens a modal would race the touch dispatcher Surface teardown. */
        btn.addOnCheckedChangeListener((b, isChecked) -> {
            if (sSuppressDispatch) return;
            b.post(() -> dispatchAction(ihandlePtr, isChecked ? 1 : 0));
        });
        return btn;
    }


    @Keep
    public static void setTitle(View widget, String title, boolean markup)
    {
        if (title == null) title = "";
        CharSequence cs = markup ? IupCommon.parseMarkup(title) : title;
        if (widget instanceof CompoundButton)
            ((CompoundButton)widget).setText(cs);
        else if (widget instanceof MaterialButton)
            ((MaterialButton)widget).setText(cs);
    }

    @Keep
    public static void setValue(View widget, boolean checked)
    {
        sSuppressDispatch = true;
        try
        {
            if (widget instanceof CompoundButton cb)
            {
                if (cb.isChecked() != checked) cb.setChecked(checked);
            }
            else if (widget instanceof MaterialButton b)
            {
                if (b.isChecked() != checked) b.setChecked(checked);
            }
        }
        finally { sSuppressDispatch = false; }
    }

    @Keep
    public static boolean getValue(View widget)
    {
        if (widget instanceof CompoundButton) return ((CompoundButton)widget).isChecked();
        if (widget instanceof MaterialButton) return ((MaterialButton)widget).isChecked();
        return false;
    }

    @Keep
    public static void enableThreeState(View widget)
    {
        if (widget instanceof MaterialCheckBox mcb)
            sThreeStateValue.put(mcb, 0);
    }

    @Keep
    public static void setImageFlat(View widget, boolean flat)
    {
        if (!(widget instanceof MaterialButton b)) return;
        sThemableImageToggles.put(b, flat);
        applyImageToggleStroke(b);
    }

    @Keep
    public static void setImageAlignment(View widget, String horiz, String vert)
    {
        if (!(widget instanceof MaterialButton b)) return;
        int gravity;
        if ("ARIGHT".equalsIgnoreCase(horiz)) gravity = Gravity.END;
        else if ("ALEFT".equalsIgnoreCase(horiz)) gravity = Gravity.START;
        else gravity = Gravity.CENTER_HORIZONTAL;
        if ("ATOP".equalsIgnoreCase(vert)) gravity |= Gravity.TOP;
        else if ("ABOTTOM".equalsIgnoreCase(vert)) gravity |= Gravity.BOTTOM;
        else gravity |= Gravity.CENTER_VERTICAL;
        b.setGravity(gravity);
    }

    /* MaterialCheckBox tri-state 0/1/-1; other widgets use boolean checked */
    @Keep
    public static void setValueState(View widget, int state)
    {
        if (widget instanceof MaterialCheckBox mcb)
        {
            int target = (state < 0) ? MaterialCheckBox.STATE_INDETERMINATE
                       : (state > 0) ? MaterialCheckBox.STATE_CHECKED
                       : MaterialCheckBox.STATE_UNCHECKED;
            sSuppressDispatch = true;
            try { if (mcb.getCheckedState() != target) mcb.setCheckedState(target); }
            finally { sSuppressDispatch = false; }
            if (sThreeStateValue.containsKey(mcb))
                sThreeStateValue.put(mcb, (state < 0) ? -1 : (state > 0) ? 1 : 0);
            return;
        }
        setValue(widget, state > 0);
    }

    @Keep
    public static int getValueState(View widget)
    {
        if (widget instanceof MaterialCheckBox mcb)
        {
            int s = mcb.getCheckedState();
            if (s == MaterialCheckBox.STATE_INDETERMINATE) return -1;
            return (s == MaterialCheckBox.STATE_CHECKED) ? 1 : 0;
        }
        return getValue(widget) ? 1 : 0;
    }

    /* indicator at trailing edge via LAYOUT_DIRECTION_RTL; pin text LTR so labels stay left-to-right */
    @Keep
    public static void setRightButton(View widget, boolean rightButton)
    {
        if (!(widget instanceof CompoundButton cb)) return;
        if (rightButton)
        {
            cb.setLayoutDirection(View.LAYOUT_DIRECTION_RTL);
            cb.setTextDirection(View.TEXT_DIRECTION_LTR);
            cb.setTextAlignment(View.TEXT_ALIGNMENT_TEXT_START);
        }
        else
        {
            cb.setLayoutDirection(View.LAYOUT_DIRECTION_INHERIT);
            cb.setTextDirection(View.TEXT_DIRECTION_INHERIT);
            cb.setTextAlignment(View.TEXT_ALIGNMENT_INHERIT);
        }
    }

    @Keep
    public static void setFgColor(View widget, int r, int g, int b)
    {
        /* ColorStateList so ACTIVE=NO fades text to Material's 38% disabled alpha. */
        int base = Color.rgb(r, g, b);
        int disabled = (base & 0x00FFFFFF) | (0x61 << 24);
        ColorStateList csl = new ColorStateList(
            new int[][] {
                new int[] { -android.R.attr.state_enabled },
                new int[0]
            },
            new int[] { disabled, base });
        if (widget instanceof CompoundButton)
            ((CompoundButton)widget).setTextColor(csl);
        else if (widget instanceof MaterialButton)
            ((MaterialButton)widget).setTextColor(csl);
    }

    @Keep
    public static void setImage(View widget, Bitmap bmp, Bitmap impress)
    {
        if (widget instanceof MaterialButton btn)
        {
            if (bmp == null) { btn.setIcon(null); return; }
            BitmapDrawable d = makeBitmapDrawable(widget, bmp);
            btn.setIconSize(d.getBounds().width());
            int pad = (int)(8 * IupCommon.getDisplayDensity());
            btn.setPadding(pad, pad, pad, pad);
            if (impress == null) { btn.setIcon(d); return; }
            BitmapDrawable imp = makeBitmapDrawable(widget, impress);
            StateListDrawable sld = new StateListDrawable();
            sld.addState(new int[]{ android.R.attr.state_pressed }, imp);
            sld.addState(new int[]{ android.R.attr.state_checked }, imp);
            sld.addState(new int[0], d);
            btn.setIcon(sld);
            return;
        }

        if (!(widget instanceof CompoundButton cb)) return;
        if (bmp == null)
        {
            cb.setCompoundDrawablesRelative(null, null, null, null);
            return;
        }
        BitmapDrawable d = makeBitmapDrawable(widget, bmp);
        cb.setCompoundDrawablePadding((int)(8 * IupCommon.getDisplayDensity()));
        cb.setCompoundDrawablesRelative(d, null, null, null);
    }


    private static BitmapDrawable makeBitmapDrawable(View widget, Bitmap bmp)
    {
        int box = (int)(24 * IupCommon.getDisplayDensity());
        int sw = bmp.getWidth(), sh = bmp.getHeight();
        int max = Math.max(sw, sh);
        Bitmap scaled = (max >= box) ? bmp
            : Bitmap.createScaledBitmap(bmp, sw * box / max, sh * box / max, true);
        BitmapDrawable d = new BitmapDrawable(widget.getResources(), scaled);
        d.setBounds(0, 0, scaled.getWidth(), scaled.getHeight());
        return d;
    }

    private static void wireCheckedChange(final CompoundButton cb, final long ihandlePtr)
    {
        /* Defer; sync ACTION CB that opens a modal would race the touch dispatcher Surface teardown. */
        cb.setOnCheckedChangeListener((buttonView, isChecked) -> {
            if (sSuppressDispatch) return;
            buttonView.post(() -> dispatchAction(ihandlePtr, isChecked ? 1 : 0));
        });
    }

    public static native void dispatchAction(long ihandlePtr, int state);
}
