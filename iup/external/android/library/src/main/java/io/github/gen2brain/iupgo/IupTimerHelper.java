package io.github.gen2brain.iupgo;

import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import androidx.annotation.Keep;


public final class IupTimerHelper
{
    private IupTimerHelper() {}


    @Keep
    public static IupTimer createTimer(final long ihandlePtr)
    {
        return new IupTimer();
    }

    @Keep
    public static void startTimer(final long ihandlePtr, IupTimer timer, final long intervalPeriod)
    {
        timer.start(ihandlePtr, intervalPeriod);
    }

    @Keep
    public static void stopTimer(final long ihandlePtr, IupTimer timer)
    {
        timer.stop();
    }


    /* uptimeMillis is monotonic and pauses in deep sleep, right for animation timers. */
    public static final class IupTimer extends Handler
    {
        private boolean isStarted;
        private long startTime;
        private Runnable runnableCode;

        public IupTimer() { super(Looper.getMainLooper()); }

        public long getElapsedTime()
        {
            return SystemClock.uptimeMillis() - startTime;
        }

        public void start(final long ihandlePtr, final long intervalPeriod)
        {
            if (isStarted) return;

            Runnable runnable = new Runnable()
            {
                @Override
                public void run()
                {
                    IupTimer.this.postDelayed(this, intervalPeriod);
                    IupCommon.iupAttribSetInt(ihandlePtr, "ELAPSEDTIME", (int) getElapsedTime());
                    IupCommon.handleIupCallback(ihandlePtr, "ACTION_CB");
                }
            };

            runnableCode = runnable;
            startTime = SystemClock.uptimeMillis();
            postDelayed(runnable, intervalPeriod);
            isStarted = true;
        }

        public void stop()
        {
            if (runnableCode != null) removeCallbacks(runnableCode);
            runnableCode = null;
            isStarted = false;
        }
    }
}
