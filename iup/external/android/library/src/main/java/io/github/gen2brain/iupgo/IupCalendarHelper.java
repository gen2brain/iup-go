package io.github.gen2brain.iupgo;

import android.content.Context;
import android.view.ContextThemeWrapper;
import android.widget.CalendarView;
import androidx.annotation.Keep;

import java.util.Calendar;
import java.util.GregorianCalendar;


public final class IupCalendarHelper
{
    private IupCalendarHelper() {}


    @Keep
    public static CalendarView createCalendar(final long ihandlePtr)
    {
        /* CalendarView's internal DatePicker needs legacy Material attrs Material3 doesn't ship */
        Context ctx = new ContextThemeWrapper(IupCommon.getContextThemeWrapper(),
            android.R.style.Theme_DeviceDefault_Light);
        CalendarView cv = new CalendarView(ctx);
        /* Defer; sync VALUECHANGED_CB that opens a modal would race the touch dispatcher Surface teardown. */
        cv.setOnDateChangeListener((view, year, month, dayOfMonth) -> view.post(() -> dispatchValueChanged(ihandlePtr)));
        return cv;
    }

    @Keep
    public static void setDate(CalendarView cv, int year, int month, int day)
    {
        /* Calendar.MONTH is 0-based; IUP VALUE is 1-based. */
        Calendar c = new GregorianCalendar(year, month - 1, day);
        cv.setDate(c.getTimeInMillis(), false, true);
    }

    @Keep
    public static String getDate(CalendarView cv)
    {
        Calendar c = Calendar.getInstance();
        c.setTimeInMillis(cv.getDate());
        return String.format(java.util.Locale.ROOT, "%d/%d/%d", c.get(Calendar.YEAR), c.get(Calendar.MONTH) + 1, c.get(Calendar.DAY_OF_MONTH));
    }

    public static native void dispatchValueChanged(long ihandlePtr);
}
