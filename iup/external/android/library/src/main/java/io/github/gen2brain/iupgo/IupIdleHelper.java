package io.github.gen2brain.iupgo;

import android.os.Handler;
import android.os.Looper;
import android.os.MessageQueue;
import androidx.annotation.Keep;


/* queueIdle fires once per "going to wait"; no-op post on IUP_DEFAULT re-fires next iteration */
public final class IupIdleHelper
{
    private IupIdleHelper() {}

    private static native int dispatchIdle();

    private static final Handler sMainHandler = new Handler(Looper.getMainLooper());
    private static final Runnable sNoOp = new Runnable() { @Override public void run() {} };

    private static final MessageQueue.IdleHandler sIdleHandler = new MessageQueue.IdleHandler()
    {
        @Override
        public boolean queueIdle()
        {
            if (!sActive) return false;
            int keep = dispatchIdle();
            if (keep == 0)
            {
                sActive = false;
                sInstalled = false;
                return false;
            }
            sMainHandler.post(sNoOp);
            return true;
        }
    };

    private static volatile boolean sActive;
    private static boolean sInstalled;

    @Keep
    public static void setIdle(boolean enable)
    {
        if (enable)
        {
            if (sInstalled)
            {
                sActive = true;
                return;
            }
            sActive = true;
            sInstalled = true;
            Looper.getMainLooper().getQueue().addIdleHandler(sIdleHandler);
            sMainHandler.post(sNoOp);
        }
        else
        {
            sActive = false;
            if (sInstalled)
            {
                Looper.getMainLooper().getQueue().removeIdleHandler(sIdleHandler);
                sInstalled = false;
            }
        }
    }
}
