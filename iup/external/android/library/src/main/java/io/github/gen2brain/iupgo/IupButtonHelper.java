package io.github.gen2brain.iupgo;

import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.StateListDrawable;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import androidx.annotation.Keep;

import com.google.android.material.button.MaterialButton;
import com.google.android.material.shape.RelativeCornerSize;
import com.google.android.material.shape.ShapeAppearanceModel;


public final class IupButtonHelper
{
    private IupButtonHelper() {}

    /* MaterialButton -> SHOWASDEFAULT state; weak so dismissed-dialog buttons drop out automatically. */
    private static final java.util.WeakHashMap<MaterialButton, Boolean> sThemableButtons = new java.util.WeakHashMap<>();

    static {
        IupTheme.register(IupButtonHelper::refreshAllButtonThemes);
    }

    private static void refreshAllButtonThemes()
    {
        for (java.util.Map.Entry<MaterialButton, Boolean> e :
             new java.util.ArrayList<>(sThemableButtons.entrySet()))
        {
            MaterialButton b = e.getKey();
            if (b == null) continue;
            if (b instanceof IupMaterialButton iup)
            {
                if (iup.isBorderless) continue;
                if (iup.buttonStyle != null) { applyButtonStyle(iup, iup.buttonStyle); continue; }
            }
            applyShowAsDefault(b, Boolean.TRUE.equals(e.getValue()));
        }
    }

    /* per-instance state for IMAGE/IMPRESS/IMINACTIVE StateListDrawable rebuild; programmatic-only */
    @android.annotation.SuppressLint("ViewConstructor")
    public static class IupMaterialButton extends MaterialButton
    {
        long ihandlePtr;
        Bitmap imgNormal;
        Bitmap imgPressed;
        Bitmap imgInactive;

        ColorStateList defaultBackgroundTint;
        ColorStateList defaultTextColors;
        int defaultPaddingL, defaultPaddingT, defaultPaddingR, defaultPaddingB;
        int materialPaddingL, materialPaddingR;
        boolean isBorderless;
        String buttonStyle;

        public IupMaterialButton(Context ctx, long ih)
        {
            super(ctx);
            this.ihandlePtr = ih;
        }

        @Override
        public boolean performClick()
        {
            /* Accessibility hook; IUP's BUTTON_CB is dispatched via the touch listener. */
            return super.performClick();
        }
    }


    @Keep
    public static Button createButton(final long ihandlePtr)
    {
        IupMaterialButton button = new IupMaterialButton(IupCommon.getContextThemeWrapper(), ihandlePtr);

        /* IUP owns sizing; strip Material3 minimums/insets. */
        button.setInsetTop(0);
        button.setInsetBottom(0);
        button.setMinHeight(0);
        button.setMinimumHeight(0);
        button.setMinWidth(0);
        button.setMinimumWidth(0);

        /* Material3 MaterialButton inherits textAlignment=viewStart; force gravity. */
        button.setGravity(Gravity.CENTER);
        button.setTextAlignment(View.TEXT_ALIGNMENT_GRAVITY);
        /* Drop TextView font-metric padding so gravity=CENTER centers the visible glyphs. */
        button.setIncludeFontPadding(false);

        applyShowAsDefault(button, false);

        button.defaultPaddingT = button.getPaddingTop();
        button.defaultPaddingB = button.getPaddingBottom();
        button.materialPaddingL = button.getPaddingLeft();
        button.materialPaddingR = button.getPaddingRight();
        /* Strip horizontal padding for text-only; rebuildIcon restores it when an icon is set. */
        button.defaultPaddingL = 0;
        button.defaultPaddingR = 0;
        button.setPadding(0, button.defaultPaddingT, 0, button.defaultPaddingB);

        String title = IupCommon.iupAttribGet(ihandlePtr, "TITLE");
        if (title != null)
        {
            boolean markup = "YES".equalsIgnoreCase(IupCommon.iupAttribGet(ihandlePtr, "MARKUP"));
            button.setText(markup ? IupCommon.parseMarkup(title) : title);
        }

        /* Defer so a user CB that opens a modal cannot race the libutils Looper teardown. */
        button.setOnClickListener(v -> v.post(() -> IupCommon.handleIupCallback(ihandlePtr, "ACTION")));

        /* BUTTON_CB; return false so ACTION click still fires via OnClickListener. */
        button.setOnTouchListener(new View.OnTouchListener()
        {
            @Override
            @android.annotation.SuppressLint("ClickableViewAccessibility")
            public boolean onTouch(View v, MotionEvent ev)
            {
                int a = ev.getActionMasked();
                final int x = (int)ev.getX();
                final int y = (int)ev.getY();
                if (a == MotionEvent.ACTION_DOWN)
                    v.post(() -> dispatchButton(ihandlePtr, 1, 1, x, y));
                else if (a == MotionEvent.ACTION_UP || a == MotionEvent.ACTION_CANCEL)
                    v.post(() -> dispatchButton(ihandlePtr, 1, 0, x, y));
                return false;
            }
        });

        return button;
    }


    @Keep
    public static void setTitle(Button button, String title, boolean markup)
    {
        if (title == null) title = "";
        button.setText(markup ? IupCommon.parseMarkup(title) : title);
    }

    @Keep
    public static void setPadding(Button button, int horiz, int vert)
    {
        button.setPadding(horiz, vert, horiz, vert);
    }

    @Keep
    public static void setAlignment(Button button, String horiz, String vert)
    {
        int gravity;
        if ("ARIGHT".equalsIgnoreCase(horiz)) gravity = Gravity.END;
        else if ("ALEFT".equalsIgnoreCase(horiz)) gravity = Gravity.START;
        else gravity = Gravity.CENTER_HORIZONTAL;
        if ("ATOP".equalsIgnoreCase(vert)) gravity |= Gravity.TOP;
        else if ("ABOTTOM".equalsIgnoreCase(vert)) gravity |= Gravity.BOTTOM;
        else gravity |= Gravity.CENTER_VERTICAL;
        button.setGravity(gravity);
    }

    @Keep
    public static void setTextColor(Button button, int r, int g, int b)
    {
        button.setTextColor(Color.rgb(r, g, b));
    }

    @Keep
    public static void setBgColor(Button button, int r, int g, int b)
    {
        if (button instanceof MaterialButton mb)
        {
            mb.setBackgroundTintList(ColorStateList.valueOf(Color.rgb(r, g, b)));
            sThemableButtons.remove(mb);  // user-set colour, opt out of theme refresh
        }
        else
            button.setBackgroundColor(Color.rgb(r, g, b));
    }


    /* IMAGE/IMPRESS/IMINACTIVE are baked into one StateListDrawable. */
    @Keep
    public static void setImage(Button button, Bitmap bmp)
    {
        if (button instanceof IupMaterialButton)
        {
            ((IupMaterialButton)button).imgNormal = bmp;
            rebuildIcon((IupMaterialButton)button);
        }
    }

    @Keep
    public static void setPressedImage(Button button, Bitmap bmp)
    {
        if (button instanceof IupMaterialButton)
        {
            ((IupMaterialButton)button).imgPressed = bmp;
            rebuildIcon((IupMaterialButton)button);
        }
    }

    @Keep
    public static void setInactiveImage(Button button, Bitmap bmp)
    {
        if (button instanceof IupMaterialButton)
        {
            ((IupMaterialButton)button).imgInactive = bmp;
            rebuildIcon((IupMaterialButton)button);
        }
    }

    /* IMPRESS && !IMPRESSBORDER: strip chrome. */
    @Keep
    public static void setBorderless(Button button, boolean borderless)
    {
        if (!(button instanceof IupMaterialButton mb)) return;
        if (borderless == mb.isBorderless) return;
        mb.isBorderless = borderless;
        if (borderless)
        {
            mb.setBackgroundTintList(ColorStateList.valueOf(Color.TRANSPARENT));
            mb.setStrokeWidth(0);
            mb.setRippleColor(null);
            mb.setElevation(0);
            mb.setPadding(0, 0, 0, 0);
            /* Filled-button text is white; unreadable on the app background. */
            android.util.TypedValue tv = new android.util.TypedValue();
            if (mb.getContext().getTheme().resolveAttribute(android.R.attr.textColorPrimary, tv, true))
            {
                ColorStateList csl = androidx.core.content.res.ResourcesCompat.getColorStateList(
                    mb.getResources(), tv.resourceId, mb.getContext().getTheme());
                if (csl != null) mb.setTextColor(csl);
            }
        }
        else
        {
            mb.setBackgroundTintList(mb.defaultBackgroundTint);
            mb.setPadding(mb.defaultPaddingL, mb.defaultPaddingT,
                mb.defaultPaddingR, mb.defaultPaddingB);
            if (mb.defaultTextColors != null) mb.setTextColor(mb.defaultTextColors);
        }
    }

    /* No-op while borderless: FLAT/IMPRESS-without-border owns the paint until the user clears it. */
    @Keep
    public static void setShowAsDefault(Button button, boolean show)
    {
        if (!(button instanceof IupMaterialButton mb)) return;
        if (mb.isBorderless) return;
        applyShowAsDefault(mb, show);
    }

    @Keep
    public static void setButtonStyle(Button button, String style)
    {
        if (!(button instanceof IupMaterialButton mb)) return;
        if (mb.isBorderless) return;
        mb.buttonStyle = (style == null || style.isEmpty()) ? null
            : style.toUpperCase(java.util.Locale.ROOT);
        if (mb.buttonStyle == null) applyShowAsDefault(mb, false);
        else                        applyButtonStyle(mb, mb.buttonStyle);
    }

    @Keep
    public static void setCornerStyle(Button button, String style)
    {
        if (!(button instanceof MaterialButton mb)) return;
        String s = (style == null || style.isEmpty()) ? null : style.toUpperCase(java.util.Locale.ROOT);
        ShapeAppearanceModel.Builder builder = mb.getShapeAppearanceModel().toBuilder();
        float density = IupCommon.getDisplayDensity();

        if      ("SMALL".equals(s))   builder.setAllCornerSizes(8 * density);
        else if ("MEDIUM".equals(s))  builder.setAllCornerSizes(12 * density);
        else if ("LARGE".equals(s))   builder.setAllCornerSizes(20 * density);
        else if ("CAPSULE".equals(s)) builder.setAllCornerSizes(new RelativeCornerSize(0.5f));
        else                          return;

        mb.setShapeAppearanceModel(builder.build());
    }

    /* Material3 styles via programmatic tints (R.style refs would force view recreation) */
    static void applyButtonStyle(MaterialButton mb, String style)
    {
        Context ctx = IupCommon.getThemeContext();
        int primary    = resolveAttrColor(ctx, com.google.android.material.R.attr.colorPrimary);
        int onPrimary  = resolveAttrColor(ctx, com.google.android.material.R.attr.colorOnPrimary);
        int secCont    = resolveAttrColor(ctx, com.google.android.material.R.attr.colorSecondaryContainer);
        int onSecCont  = resolveAttrColor(ctx, com.google.android.material.R.attr.colorOnSecondaryContainer);
        int outline    = resolveAttrColor(ctx, com.google.android.material.R.attr.colorOutline);
        int surfaceLow = resolveAttrColor(ctx, com.google.android.material.R.attr.colorSurfaceContainerLow);

        float density = IupCommon.getDisplayDensity();
        int bg, fg, strokeColor = Color.TRANSPARENT;
        int strokeWidth = 0;
        float elevation = 0f;
        switch (style)
        {
            case "TONAL":    bg = secCont;          fg = onSecCont;  break;
            case "OUTLINED": bg = Color.TRANSPARENT; fg = primary;
                             strokeColor = outline;  strokeWidth = (int)(1f * density);  break;
            case "ELEVATED": bg = surfaceLow;       fg = primary;    elevation = 1f * density;  break;
            case "TEXT":     bg = Color.TRANSPARENT; fg = primary;   break;
            case "FILLED":
            default:         bg = primary;          fg = onPrimary;  break;
        }

        int[][] states = { new int[]{ -android.R.attr.state_enabled }, new int[]{} };
        ColorStateList bgTint = new ColorStateList(states,
            new int[]{ (bg & 0x00FFFFFF) | 0x80000000, bg });
        ColorStateList fgTint = new ColorStateList(states,
            new int[]{ (fg & 0x00FFFFFF) | 0x80000000, fg });
        mb.setBackgroundTintList(bgTint);
        mb.setTextColor(fgTint);
        mb.setStrokeColor(ColorStateList.valueOf(strokeColor));
        mb.setStrokeWidth(strokeWidth);
        mb.setElevation(elevation);
        if (mb instanceof IupMaterialButton iup)
        {
            iup.defaultBackgroundTint = bgTint;
            iup.defaultTextColors = fgTint;
        }
        sThemableButtons.put(mb, false);
    }

    static void applyShowAsDefault(MaterialButton mb, boolean show)
    {
        Context ctx = IupCommon.getThemeContext();
        int bgAttr = show
            ? com.google.android.material.R.attr.colorTertiary
            : com.google.android.material.R.attr.colorPrimary;
        int fgAttr = show
            ? com.google.android.material.R.attr.colorOnTertiary
            : com.google.android.material.R.attr.colorOnPrimary;

        int bg = resolveAttrColor(ctx, bgAttr);
        int fg = resolveAttrColor(ctx, fgAttr);

        /* Material3's stock disabled state is barely visible on light themes; force 50% alpha of the enabled colour. */
        int[][] states = { new int[]{ -android.R.attr.state_enabled }, new int[]{} };
        ColorStateList bgTint = new ColorStateList(states,
            new int[] { (bg & 0x00FFFFFF) | 0x80000000, bg });
        ColorStateList fgTint = new ColorStateList(states,
            new int[] { (fg & 0x00FFFFFF) | 0x80000000, fg });

        mb.setBackgroundTintList(bgTint);
        mb.setTextColor(fgTint);
        if (mb instanceof IupMaterialButton iup)
        {
            iup.defaultBackgroundTint = bgTint;
            iup.defaultTextColors = fgTint;
        }
        sThemableButtons.put(mb, show);
    }

    private static int resolveAttrColor(Context ctx, int attr)
    {
        android.util.TypedValue tv = new android.util.TypedValue();
        if (!ctx.getTheme().resolveAttribute(attr, tv, true)) return Color.GRAY;
        if (tv.resourceId != 0)
            return androidx.core.content.ContextCompat.getColor(ctx, tv.resourceId);
        return tv.data;
    }

    @Keep
    public static void setImagePosition(Button button, String pos)
    {
        if (!(button instanceof MaterialButton mb)) return;
        int g;
        /* MaterialButton has no TEXT_BOTTOM; map BOTTOM → TOP. */
        if ("RIGHT".equalsIgnoreCase(pos)) g = MaterialButton.ICON_GRAVITY_TEXT_END;
        else if ("TOP".equalsIgnoreCase(pos) || "BOTTOM".equalsIgnoreCase(pos))
            g = MaterialButton.ICON_GRAVITY_TEXT_TOP;
        else g = MaterialButton.ICON_GRAVITY_TEXT_START;
        mb.setIconGravity(g);
    }


    private static void rebuildIcon(IupMaterialButton button)
    {
        if (button.imgNormal == null && button.imgPressed == null && button.imgInactive == null)
        {
            button.setIcon(null);
            if (!button.isBorderless)
                button.setPadding(0, button.defaultPaddingT, 0, button.defaultPaddingB);
            return;
        }

        /* Image-only: center the icon (no text to anchor it). */
        CharSequence txt = button.getText();
        if (txt == null || txt.length() == 0)
        {
            button.setGravity(android.view.Gravity.CENTER);
            button.setIconGravity(MaterialButton.ICON_GRAVITY_TEXT_START);
        }

        /* Compound-drawable layout needs paddingStart to center icon+text. */
        if (!button.isBorderless)
            button.setPadding(button.materialPaddingL, button.defaultPaddingT,
                              button.materialPaddingR, button.defaultPaddingB);

        /* Keep bitmap's own colors (theme tint would recolor pixmaps). */
        button.setIconTint(null);

        StateListDrawable sld = new StateListDrawable();
        Drawable normal = toDrawable(button, button.imgNormal);
        Drawable pressed = toDrawable(button, button.imgPressed);
        Drawable inactive = toDrawable(button, button.imgInactive);

        if (pressed != null)
            sld.addState(new int[]{ android.R.attr.state_pressed }, pressed);
        if (inactive != null)
            sld.addState(new int[]{ -android.R.attr.state_enabled }, inactive);
        if (normal != null)
            sld.addState(new int[]{}, normal);

        /* Bitmap is density-tagged 160dpi; floor to 24dp Material min for chromed. */
        Bitmap probe = button.imgNormal != null ? button.imgNormal
            : (button.imgPressed != null ? button.imgPressed : button.imgInactive);
        if (probe != null)
        {
            int size = probe.getScaledWidth(button.getResources().getDisplayMetrics());
            if (!button.isBorderless)
            {
                int minDp = Math.round(24 * IupCommon.getDisplayDensity());
                if (minDp > size) size = minDp;
            }
            button.setIconSize(size);
        }
        boolean hasText = txt != null && txt.length() > 0;
        button.setIconPadding(hasText ? Math.round(8 * IupCommon.getDisplayDensity()) : 0);
        button.setIcon(sld);
    }

    private static Drawable toDrawable(View v, Bitmap bmp)
    {
        if (bmp == null) return null;
        return new BitmapDrawable(v.getResources(), bmp);
    }


    public static native void dispatchButton(long ihandlePtr, int button, int pressed, int x, int y);
}
