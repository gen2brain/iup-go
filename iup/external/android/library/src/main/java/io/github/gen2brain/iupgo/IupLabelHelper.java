package io.github.gen2brain.iupgo;

import android.content.res.ColorStateList;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.view.GestureDetector;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;

import androidx.annotation.Keep;
import androidx.appcompat.view.ContextThemeWrapper;
import androidx.appcompat.widget.AppCompatTextView;

import com.google.android.material.color.MaterialColors;
import com.google.android.material.divider.MaterialDivider;


public final class IupLabelHelper
{
    private IupLabelHelper() {}

    /* M3 divider color (colorOutlineVariant) is cached at construction; rebuild on theme flip. */
    private static final java.util.WeakHashMap<View, Boolean> sThemableSeparators = new java.util.WeakHashMap<>();

    static {
        IupTheme.register(IupLabelHelper::refreshAllSeparators);
    }

    private static int resolveDividerColor()
    {
        return MaterialColors.getColor(IupCommon.getThemeContext(),
            com.google.android.material.R.attr.colorOutlineVariant,
            IupCommon.blendColor(IupCommon.paletteDlgBg, IupCommon.paletteDlgFg, 0.25f));
    }

    private static void refreshAllSeparators()
    {
        int color = resolveDividerColor();
        for (View v : new java.util.ArrayList<>(sThemableSeparators.keySet()))
        {
            if (v == null) continue;
            if (v instanceof MaterialDivider md) md.setDividerColor(color);
            else v.setBackgroundColor(color);
        }
    }


    @Keep
    public static TextView createLabelText(final long ihandlePtr)
    {
        ContextThemeWrapper themeContext = IupCommon.getContextThemeWrapper();
        TextView tv = new AppCompatTextView(themeContext);
        attachButtonDispatch(tv, ihandlePtr);
        return tv;
    }

    /* MaterialDivider is horizontal-only; vertical falls back to a thin colored View. */
    @Keep
    public static View createLabelSeparator(final long ihandlePtr, boolean horizontal)
    {
        ContextThemeWrapper ctx = IupCommon.getContextThemeWrapper();
        View v;
        if (horizontal)
        {
            v = new MaterialDivider(ctx);
        }
        else
        {
            v = new View(ctx);
            v.setBackgroundColor(resolveDividerColor());
        }
        sThemableSeparators.put(v, Boolean.TRUE);
        return v;
    }

    /* Defer dispatch so a user CB that opens a modal cannot race the libutils Looper teardown of the touch dispatcher. */
    private static void attachButtonDispatch(View view, final long ihandlePtr)
    {
        view.setClickable(true);
        /* Detect long-press separately so the TIP tooltip (via View.OnLongClickListener) still fires. */
        final GestureDetector longPress = new GestureDetector(view.getContext(),
            new GestureDetector.SimpleOnGestureListener()
            {
                @Override public void onLongPress(@NonNull MotionEvent e) { view.performLongClick(); }
            });
        view.setOnTouchListener((v, ev) -> {
            longPress.onTouchEvent(ev);
            int a = ev.getActionMasked();
            final int x = (int)ev.getX();
            final int y = (int)ev.getY();
            if (a == MotionEvent.ACTION_DOWN)
                v.post(() -> IupButtonHelper.dispatchButton(ihandlePtr, 1, 1, x, y));
            else if (a == MotionEvent.ACTION_UP || a == MotionEvent.ACTION_CANCEL)
            {
                final boolean wasUp = (a == MotionEvent.ACTION_UP);
                v.post(() -> {
                    IupButtonHelper.dispatchButton(ihandlePtr, 1, 0, x, y);
                    if (wasUp) v.performClick();
                });
            }
            return true;
        });
    }

    @Keep
    public static void setText(final long ihandlePtr, TextView textView, String text, boolean markup)
    {
        if (text == null) text = "";
        textView.setText(markup ? IupCommon.parseMarkup(text) : text);
    }

    @Keep
    public static void setEllipsis(TextView textView, boolean enable)
    {
        textView.setEllipsize(enable ? android.text.TextUtils.TruncateAt.END : null);
    }

    /* WORDWRAP=NO disables auto-wrap; '\n' still inserts newlines */
    @Keep
    public static void setWordWrap(TextView textView, boolean enable)
    {
        textView.setHorizontallyScrolling(!enable);
    }

    @Keep
    public static void setSelectable(TextView textView, boolean enable)
    {
        if (enable)
            textView.setOnTouchListener(null);
        textView.setTextIsSelectable(enable);
    }

    /* h and v are device pixels (already scaled by iupdrvScaleNaturalPx). */
    @Keep
    public static void setPadding(View view, int h, int v)
    {
        view.setPadding(h, v, h, v);
    }

    /** {@code horiz} is ALEFT|ACENTER|ARIGHT, {@code vert} is ATOP|ACENTER|ABOTTOM. */
    @Keep
    public static void setAlignment(TextView textView, String horiz, String vert)
    {
        int gravity;
        if ("ARIGHT".equalsIgnoreCase(horiz)) gravity = Gravity.END;
        else if ("ALEFT".equalsIgnoreCase(horiz)) gravity = Gravity.START;
        else gravity = Gravity.CENTER_HORIZONTAL;
        if ("ATOP".equalsIgnoreCase(vert)) gravity |= Gravity.TOP;
        else if ("ABOTTOM".equalsIgnoreCase(vert)) gravity |= Gravity.BOTTOM;
        else gravity |= Gravity.CENTER_VERTICAL;
        textView.setGravity(gravity);
    }

    @Keep
    public static void setTextColor(TextView textView, int r, int g, int b)
    {
        ColorStateList csl = new ColorStateList(
            new int[][] { new int[]{ -android.R.attr.state_enabled }, new int[0] },
            new int[] { Color.argb(97, r, g, b), Color.rgb(r, g, b) });
        textView.setTextColor(csl);
    }

    @Keep
    public static void setBgColor(TextView textView, int r, int g, int b)
    {
        textView.setBackgroundColor(Color.rgb(r, g, b));
    }

    @Keep
    public static ImageView createLabelImage(final long ihandlePtr)
    {
        ContextThemeWrapper themeContext = IupCommon.getContextThemeWrapper();
        ImageView imageView = new ImageView(themeContext);
        imageView.setScaleType(ImageView.ScaleType.CENTER_INSIDE);
        imageView.setAdjustViewBounds(true);
        attachButtonDispatch(imageView, ihandlePtr);
        return imageView;
    }

    @Keep
    public static void setImage(final long ihandlePtr, ImageView imageView, Bitmap bitmap)
    {
        imageView.setImageBitmap(bitmap);
        if (null == bitmap)
        {
            return;
        }
        android.util.DisplayMetrics dm = imageView.getResources().getDisplayMetrics();
        imageView.getLayoutParams().width = bitmap.getScaledWidth(dm);
        imageView.getLayoutParams().height = bitmap.getScaledHeight(dm);
        imageView.requestLayout();
    }
}
