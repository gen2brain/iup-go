package io.github.gen2brain.iupgo;

import android.app.Activity;
import android.app.Application;
import android.os.Bundle;
import android.util.Log;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

import java.lang.ref.WeakReference;


/* Tracks the current foreground Activity for the C driver. */
public class IupApplication extends Application
{
    private static final String TAG = "Iup";

    private static IupApplication sSharedInstance;
    private WeakReference<Activity> currentActivity = new WeakReference<>(null);

    @Keep
    public static IupApplication getIupApplication()
    {
        return sSharedInstance;
    }

    @Keep
    public Activity getCurrentActivity()
    {
        return currentActivity.get();
    }

    /* fires user IUP exit cb when the last Activity is destroyed */
    public native void IupExitCallback();

    @Override
    public void onCreate()
    {
        super.onCreate();
        sSharedInstance = this;
        registerActivityLifecycleCallbacks(new IupActivityLifecycleHandler());
        Log.i(TAG, "IupApplication.onCreate pkg=" + getPackageName());
    }

    /* Fires IUP exit callback when the last Activity is destroyed. */
    private final class IupActivityLifecycleHandler implements ActivityLifecycleCallbacks
    {
        private int resumedCount;
        private int pausedCount;
        private int startedCount;
        private int stoppedCount;
        private int activityCount;

        /* suppresses redundant fg/bg transitions when activities cycle */
        private boolean savedIsInBackground;

        @Override
        public void onActivityCreated(@NonNull Activity activity, Bundle savedInstanceState)
        {
            activityCount += 1;
        }

        @Override
        public void onActivityDestroyed(@NonNull Activity activity)
        {
            activityCount -= 1;
            if (activityCount == 0)
            {
                IupExitCallback();
            }
        }

        @Override
        public void onActivityResumed(@NonNull Activity activity)
        {
            currentActivity = new WeakReference<>(activity);
            resumedCount += 1;
        }

        @Override
        public void onActivityPaused(@NonNull Activity activity)
        {
            pausedCount += 1;
        }

        @Override
        public void onActivitySaveInstanceState(@NonNull Activity activity, @NonNull Bundle outState)
        {
        }

        @Override
        public void onActivityStarted(@NonNull Activity activity)
        {
            currentActivity = new WeakReference<>(activity);
            startedCount += 1;
            /* earliest hook where isApplicationInBackground flips to false on resume */
            checkForBackgroundTransition();
        }

        @Override
        public void onActivityStopped(@NonNull Activity activity)
        {
            stoppedCount += 1;
            /* earliest hook where isApplicationInBackground flips to true after last Activity goes away */
            checkForBackgroundTransition();
        }

        public boolean isApplicationVisible()
        {
            return startedCount > stoppedCount;
        }

        public boolean isApplicationInForeground()
        {
            return resumedCount > pausedCount;
        }

        public boolean isApplicationInBackground()
        {
            return !isApplicationVisible() && !isApplicationInForeground();
        }

        /** @return true if a foreground/background transition was detected. */
        private boolean checkForBackgroundTransition()
        {
            boolean inBackground = isApplicationInBackground();
            if (inBackground != savedIsInBackground)
            {
                savedIsInBackground = inBackground;
                return true;
            }
            return false;
        }
    }
}
