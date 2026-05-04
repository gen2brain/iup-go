package io.github.gen2brain.iupgo;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import androidx.annotation.Keep;

import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.atomic.AtomicBoolean;


/* drains under a per-tick time budget, re-posting if work remains; keeps UI > 20fps under floods */
public final class IupPostMessage
{
    public native static void OnMainThreadCallback(long ihandlePtr, long messageDataPtr, String usrStr, long usrInt, double usrDouble);

    /* Primitive-field message: avoids Object[]+Long/Double autoboxing on the hot path. */
    private static final class Msg
    {
        long ih;
        long ptr;
        String s;
        long i;
        double d;
    }

    private static final Handler sMainHandler = new Handler(Looper.getMainLooper());
    private static final ConcurrentLinkedQueue<Msg> sQueue = new ConcurrentLinkedQueue<>();
    private static final AtomicBoolean sDrainScheduled = new AtomicBoolean(false);
    private static final long MAX_NANOS_PER_TICK = 50_000_000L;

    private static final Runnable sDrain = new Runnable() {
        @Override public void run()
        {
            long deadline = System.nanoTime() + MAX_NANOS_PER_TICK;
            while (System.nanoTime() < deadline)
            {
                Msg m = sQueue.poll();
                if (m == null) break;
                OnMainThreadCallback(m.ih, m.ptr, m.s, m.i, m.d);
            }

            if (sQueue.peek() != null)
            {
                sMainHandler.post(this);
                return;
            }

            /* Race-check producers that enqueued between the last poll and the flag release. */
            sDrainScheduled.set(false);
            if (sQueue.peek() == null) return;
            if (sDrainScheduled.compareAndSet(false, true))
                sMainHandler.post(this);
        }
    };

    @Keep
    public static void postMessage(Context appContext, final long ih, final long messageDataPtr, final String usrStr, final long usrInt, final double usrDouble)
    {
        Msg m = new Msg();
        m.ih = ih; m.ptr = messageDataPtr; m.s = usrStr; m.i = usrInt; m.d = usrDouble;
        sQueue.offer(m);
        if (sDrainScheduled.compareAndSet(false, true))
            sMainHandler.post(sDrain);
    }
}
