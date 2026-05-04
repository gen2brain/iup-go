package io.github.gen2brain.iupgo;

import android.app.Activity;
import android.content.Context;

import androidx.annotation.Keep;
import androidx.appcompat.view.ContextThemeWrapper;
import androidx.fragment.app.FragmentActivity;

import com.google.android.material.button.MaterialButton;
import com.google.android.material.datepicker.MaterialDatePicker;

import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.GregorianCalendar;
import java.util.Locale;
import java.util.TimeZone;


public final class IupDatePickHelper
{
    private IupDatePickHelper() {}

    /* Per-instance state on the button; currentPicker held only while on screen so SHOWDROPDOWN=NO can dismiss. */
    static final class IupDatePickButton extends MaterialButton
    {
        long ihandlePtr;
        int year, month, day;
        String pattern = "d/M/yyyy";
        MaterialDatePicker<Long> currentPicker;

        IupDatePickButton(Context ctx)
        {
            super(ctx, null, com.google.android.material.R.attr.materialButtonOutlinedStyle);
        }
    }

    @Keep
    public static MaterialButton createDatePick(final long ihandlePtr)
    {
        ContextThemeWrapper ctx = IupCommon.getContextThemeWrapper();
        IupDatePickButton btn = new IupDatePickButton(ctx);
        btn.ihandlePtr = ihandlePtr;
        btn.setAllCaps(false);
        btn.setOnClickListener(v -> showPicker(btn));
        return btn;
    }

    @Keep
    public static void setValue(MaterialButton button, int year, int month, int day)
    {
        if (!(button instanceof IupDatePickButton b)) return;
        b.year = year; b.month = month; b.day = day;
        renderText(b);
    }

    @Keep
    public static void setPattern(MaterialButton button, String pattern)
    {
        if (!(button instanceof IupDatePickButton b)) return;
        if (pattern == null || pattern.isEmpty()) pattern = "d/M/yyyy";
        b.pattern = pattern;
        renderText(b);
    }

    @Keep
    public static void showDropdown(MaterialButton button, boolean show)
    {
        if (!(button instanceof IupDatePickButton b)) return;
        if (show) showPicker(b);
        else if (b.currentPicker != null) b.currentPicker.dismiss();
    }

    private static void renderText(IupDatePickButton b)
    {
        try
        {
            Calendar c = new GregorianCalendar(b.year, b.month - 1, b.day);
            SimpleDateFormat fmt = new SimpleDateFormat(b.pattern, Locale.getDefault());
            b.setText(fmt.format(c.getTime()));
        }
        catch (Throwable t)
        {
            b.setText(String.format(Locale.ROOT, "%d/%d/%d", b.year, b.month, b.day));
        }
    }

    private static void showPicker(final IupDatePickButton b)
    {
        Activity a = IupApplication.getIupApplication().getCurrentActivity();
        if (!(a instanceof FragmentActivity fa)) return;

        /* MaterialDatePicker uses UTC midnight ms. */
        Calendar utc = new GregorianCalendar(TimeZone.getTimeZone("UTC"));
        utc.clear();
        utc.set(b.year, b.month - 1, b.day);
        long selection = utc.getTimeInMillis();

        MaterialDatePicker<Long> picker = MaterialDatePicker.Builder.datePicker()
            .setSelection(selection)
            .build();

        picker.addOnPositiveButtonClickListener(ms -> {
            Calendar c = new GregorianCalendar(TimeZone.getTimeZone("UTC"));
            c.setTimeInMillis(ms);
            int y = c.get(Calendar.YEAR);
            int m = c.get(Calendar.MONTH) + 1;
            int d = c.get(Calendar.DAY_OF_MONTH);
            b.year = y; b.month = m; b.day = d;
            renderText(b);
            dispatchValueChanged(b.ihandlePtr, y, m, d);
        });
        picker.addOnDismissListener(dlg -> { if (b.currentPicker == picker) b.currentPicker = null; });

        b.currentPicker = picker;
        picker.show(fa.getSupportFragmentManager(), "iupDatePick");
    }

    public static native void dispatchValueChanged(long ihandlePtr, int year, int month, int day);
}
