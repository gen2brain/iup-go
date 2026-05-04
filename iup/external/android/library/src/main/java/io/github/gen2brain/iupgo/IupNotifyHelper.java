package io.github.gen2brain.iupgo;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.app.Activity;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowInsets;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.Keep;
import androidx.core.app.NotificationCompat;
import androidx.core.app.NotificationManagerCompat;


public final class IupNotifyHelper
{
    private static final String TAG = "Iup";

    private IupNotifyHelper() {}

    public static final String CHANNEL_LOW     = "iup_low";
    public static final String CHANNEL_NORMAL  = "iup_normal";
    public static final String CHANNEL_HIGH    = "iup_high";

    public static final String ACTION_NOTIFY = "io.github.gen2brain.iupgo.NOTIFY_ACTION";
    public static final String ACTION_DELETE = "io.github.gen2brain.iupgo.NOTIFY_DELETE";
    public static final String EXTRA_IHANDLE   = "ihandle";
    public static final String EXTRA_ACTION    = "action";
    public static final String EXTRA_NOTIFY_ID = "notify_id";

    private static int sNextId = 1;


    @Keep
    public static boolean isAvailable()
    {
        Context ctx = IupCommon.getContextThemeWrapper();
        return NotificationManagerCompat.from(ctx).areNotificationsEnabled();
    }

    /* Android 13+ runtime permission. Prompts the user if not yet decided. */
    private static void ensurePermission()
    {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU) return;
        Context ctx = IupCommon.getContextThemeWrapper();
        if (ctx.checkSelfPermission(android.Manifest.permission.POST_NOTIFICATIONS)
            == PackageManager.PERMISSION_GRANTED) return;
        Activity act = IupActivity.currentActivity();
        if (act != null)
        {
            Log.i(TAG, "IupNotify requesting POST_NOTIFICATIONS permission");
            act.requestPermissions(new String[]{android.Manifest.permission.POST_NOTIFICATIONS}, 0);
        }
        else
        {
            Log.w(TAG, "IupNotify cannot request POST_NOTIFICATIONS: no current Activity");
        }
    }

    @Keep
    public static int nextId()
    {
        return sNextId++;
    }

    private static void ensureChannels(Context ctx)
    {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) return;
        NotificationManager nm = ctx.getSystemService(NotificationManager.class);
        if (nm == null) return;
        if (nm.getNotificationChannel(CHANNEL_LOW) == null)
            nm.createNotificationChannel(new NotificationChannel(CHANNEL_LOW, "IUP Low", NotificationManager.IMPORTANCE_LOW));
        if (nm.getNotificationChannel(CHANNEL_NORMAL) == null)
            nm.createNotificationChannel(new NotificationChannel(CHANNEL_NORMAL, "IUP Normal", NotificationManager.IMPORTANCE_DEFAULT));
        if (nm.getNotificationChannel(CHANNEL_HIGH) == null)
            nm.createNotificationChannel(new NotificationChannel(CHANNEL_HIGH, "IUP High", NotificationManager.IMPORTANCE_HIGH));
    }

    /* "NONE/MIN/LOW/DEFAULT/HIGH/MAX" -> NotificationManager.IMPORTANCE_*. */
    private static int parseImportance(String s, int fallback)
    {
        if (s == null || s.isEmpty()) return fallback;
        if (s.equalsIgnoreCase("NONE"))    return NotificationManager.IMPORTANCE_NONE;
        if (s.equalsIgnoreCase("MIN"))     return NotificationManager.IMPORTANCE_MIN;
        if (s.equalsIgnoreCase("LOW"))     return NotificationManager.IMPORTANCE_LOW;
        if (s.equalsIgnoreCase("DEFAULT")) return NotificationManager.IMPORTANCE_DEFAULT;
        if (s.equalsIgnoreCase("HIGH"))    return NotificationManager.IMPORTANCE_HIGH;
        if (s.equalsIgnoreCase("MAX"))     return NotificationManager.IMPORTANCE_MAX;
        return fallback;
    }

    /* lazy-create channel if missing; existing channels can't change importance at runtime */
    private static void ensureCustomChannel(Context ctx, String channelId, String importance)
    {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) return;
        if (channelId == null || channelId.isEmpty()) return;
        NotificationManager nm = ctx.getSystemService(NotificationManager.class);
        if (nm == null) return;
        if (nm.getNotificationChannel(channelId) != null) return;
        int imp = parseImportance(importance, NotificationManager.IMPORTANCE_DEFAULT);
        nm.createNotificationChannel(new NotificationChannel(channelId, channelId, imp));
    }

    /* urgency 0/1/2 = low/normal/high; channelId overrides iup_* default; returns id, 0 on failure */
    @Keep
    public static int show(long ihandlePtr, int notifyId, String title, String body,
                           Bitmap iconBitmap, int timeoutMs, int urgency, boolean silent, boolean transient_,
                           String action1, String action2, String action3, String action4,
                           int progressCurrent, int progressMax, boolean progressIndeterminate,
                           Bitmap bigPicture,
                           String channelId, String importance)
    {
        ensurePermission();
        Context ctx = IupCommon.getContextThemeWrapper().getApplicationContext();
        ensureChannels(ctx);

        String channel;
        int priority;
        if (channelId != null && !channelId.isEmpty())
        {
            ensureCustomChannel(ctx, channelId, importance);
            channel = channelId;
            /* Pre-O priority hint mirrors the importance string when supplied. */
            int impFallback = (urgency == 0) ? NotificationManager.IMPORTANCE_LOW
                            : (urgency == 2) ? NotificationManager.IMPORTANCE_HIGH
                            : NotificationManager.IMPORTANCE_DEFAULT;
            int imp = parseImportance(importance, impFallback);
            priority = imp >= NotificationManager.IMPORTANCE_HIGH ? NotificationCompat.PRIORITY_HIGH
                     : imp <= NotificationManager.IMPORTANCE_LOW  ? NotificationCompat.PRIORITY_LOW
                     : NotificationCompat.PRIORITY_DEFAULT;
        }
        else
        {
            switch (urgency)
            {
                case 0:  channel = CHANNEL_LOW;    priority = NotificationCompat.PRIORITY_LOW;     break;
                case 2:  channel = CHANNEL_HIGH;   priority = NotificationCompat.PRIORITY_HIGH;    break;
                default: channel = CHANNEL_NORMAL; priority = NotificationCompat.PRIORITY_DEFAULT; break;
            }
        }

        NotificationCompat.Builder b = new NotificationCompat.Builder(ctx, channel)
            .setContentTitle(title != null ? title : "")
            .setContentText(body != null ? body : "")
            .setSmallIcon(ctx.getApplicationInfo().icon != 0 ? ctx.getApplicationInfo().icon : android.R.drawable.ic_dialog_info)
            .setPriority(priority)
            .setAutoCancel(transient_)
            .setOnlyAlertOnce(silent);

        if (iconBitmap != null) b.setLargeIcon(iconBitmap);
        if (silent) b.setSilent(true);
        if (timeoutMs > 0) b.setTimeoutAfter(timeoutMs);

        if (progressIndeterminate)
            b.setProgress(0, 0, true);
        else if (progressMax > 0)
            b.setProgress(progressMax, Math.max(0, Math.min(progressCurrent, progressMax)), false);

        if (bigPicture != null)
        {
            NotificationCompat.BigPictureStyle bps = new NotificationCompat.BigPictureStyle().bigPicture(bigPicture);
            if (title != null) bps.setBigContentTitle(title);
            if (body  != null) bps.setSummaryText(body);
            b.setStyle(bps);
        }

        /* Content tap -> Activity (fixes AS warning). */
        b.setContentIntent(makeContentIntent(ctx, ihandlePtr, notifyId));
        b.setDeleteIntent(makeDeleteIntent(ctx, ihandlePtr, notifyId));

        addAction(ctx, b, ihandlePtr, notifyId, 1, action1);
        addAction(ctx, b, ihandlePtr, notifyId, 2, action2);
        addAction(ctx, b, ihandlePtr, notifyId, 3, action3);
        addAction(ctx, b, ihandlePtr, notifyId, 4, action4);

        try
        {
            NotificationManagerCompat.from(ctx).notify(notifyId, b.build());
            return notifyId;
        }
        catch (SecurityException e)
        {
            return 0;
        }
    }

    @Keep
    public static void close(int notifyId)
    {
        NotificationManagerCompat.from(IupCommon.getContextThemeWrapper()).cancel(notifyId);
    }


    /* Per-Ihandle live custom toast so CLOSE can cancel it before auto-dismiss. */
    private static final java.util.Map<Long, View> sToasts = new java.util.HashMap<>();
    private static final java.util.Map<Long, Runnable> sToastDismissers = new java.util.HashMap<>();

    /* custom toast view; system Toast prepends launcher icon and ignores app-supplied ICON */
    @Keep
    public static void showToast(long ihandlePtr, String title, String body, Bitmap iconBitmap, int timeoutMs)
    {
        final String text = joinToastText(title, body);
        if (text.isEmpty()) return;
        final int duration = (timeoutMs > 0) ? timeoutMs : 3500;
        new Handler(Looper.getMainLooper()).post(() -> showToastUi(ihandlePtr, text, iconBitmap, duration));
    }

    private static void showToastUi(long ihandlePtr, String text, Bitmap iconBitmap, int durationMs)
    {
        Activity act = IupActivity.currentActivity();
        if (act == null) return;
        FrameLayout host = act.findViewById(android.R.id.content);
        if (host == null) return;
        Context ctx = host.getContext();

        cancelToastUi(ihandlePtr);

        boolean dark = isNightMode(ctx);
        int bgColor = dark ? 0xE6E6E6E6 : 0xE6323232;
        int fgColor = dark ? Color.BLACK : Color.WHITE;

        LinearLayout row = new LinearLayout(ctx);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setGravity(Gravity.CENTER_VERTICAL);
        int padH = dp(ctx, 16);
        int padV = dp(ctx, 12);
        row.setPadding(padH, padV, padH, padV);

        GradientDrawable bg = new GradientDrawable();
        bg.setShape(GradientDrawable.RECTANGLE);
        bg.setCornerRadius(dp(ctx, 24));
        bg.setColor(bgColor);
        row.setBackground(bg);
        row.setElevation(dp(ctx, 6));

        if (iconBitmap != null)
        {
            ImageView iv = new ImageView(ctx);
            iv.setImageBitmap(iconBitmap);
            iv.setScaleType(ImageView.ScaleType.FIT_CENTER);
            int sz = dp(ctx, 32);
            LinearLayout.LayoutParams ivLp = new LinearLayout.LayoutParams(sz, sz);
            ivLp.rightMargin = dp(ctx, 12);
            row.addView(iv, ivLp);
        }

        TextView tv = new TextView(ctx);
        tv.setText(text);
        tv.setTextColor(fgColor);
        tv.setTypeface(Typeface.DEFAULT);
        tv.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        tv.setMaxLines(3);
        row.addView(tv, new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT));

        FrameLayout.LayoutParams lp = new FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT,
            Gravity.CENTER_HORIZONTAL | Gravity.BOTTOM);
        lp.bottomMargin = dp(ctx, 80) + bottomInset(host);
        lp.leftMargin = dp(ctx, 16);
        lp.rightMargin = dp(ctx, 16);

        row.setAlpha(0f);
        host.addView(row, lp);
        sToasts.put(ihandlePtr, row);

        row.animate().alpha(1f).setDuration(180).start();

        Handler h = new Handler(Looper.getMainLooper());
        Runnable dismiss = () -> dismissToast(ihandlePtr);
        sToastDismissers.put(ihandlePtr, dismiss);
        h.postDelayed(dismiss, durationMs);

        row.setOnClickListener(v -> dismissToast(ihandlePtr));
    }

    private static void dismissToast(long ihandlePtr)
    {
        final View v = sToasts.remove(ihandlePtr);
        Runnable r = sToastDismissers.remove(ihandlePtr);
        if (r != null) new Handler(Looper.getMainLooper()).removeCallbacks(r);
        if (v == null) return;
        v.animate().alpha(0f).setDuration(180).setListener(new AnimatorListenerAdapter() {
            @Override public void onAnimationEnd(Animator a)
            {
                ViewGroup parent = (ViewGroup)v.getParent();
                if (parent != null) parent.removeView(v);
            }
        }).start();
    }

    @Keep
    public static void cancelToast(long ihandlePtr)
    {
        new Handler(Looper.getMainLooper()).post(() -> cancelToastUi(ihandlePtr));
    }

    private static void cancelToastUi(long ihandlePtr)
    {
        View v = sToasts.remove(ihandlePtr);
        Runnable r = sToastDismissers.remove(ihandlePtr);
        if (r != null) new Handler(Looper.getMainLooper()).removeCallbacks(r);
        if (v == null) return;
        ViewGroup parent = (ViewGroup)v.getParent();
        if (parent != null) parent.removeView(v);
    }

    private static int dp(Context ctx, int v)
    {
        return Math.round(v * ctx.getResources().getDisplayMetrics().density);
    }

    private static boolean isNightMode(Context ctx)
    {
        int mode = ctx.getResources().getConfiguration().uiMode & Configuration.UI_MODE_NIGHT_MASK;
        return mode == Configuration.UI_MODE_NIGHT_YES;
    }

    private static int bottomInset(View v)
    {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R) return 0;
        WindowInsets wi = v.getRootWindowInsets();
        if (wi == null) return 0;
        return wi.getInsets(WindowInsets.Type.systemBars() | WindowInsets.Type.ime()).bottom;
    }

    private static String joinToastText(String title, String body)
    {
        boolean hasT = title != null && !title.isEmpty();
        boolean hasB = body  != null && !body.isEmpty();
        if (hasT && hasB) return title + "\n" + body;
        if (hasT) return title;
        if (hasB) return body;
        return "";
    }

    private static void addAction(Context ctx, NotificationCompat.Builder b, long ihandlePtr, int notifyId,
                                  int actionIndex, String label)
    {
        if (label == null || label.isEmpty()) return;
        PendingIntent pi = makeActionIntent(ctx, ihandlePtr, notifyId, actionIndex);
        b.addAction(new NotificationCompat.Action.Builder(0, label, pi).build());
    }

    private static PendingIntent makeContentIntent(Context ctx, long ihandlePtr, int notifyId)
    {
        /* Use an Activity for the main tap to fix Android Studio trampoline warnings. */
        Intent i = new Intent(ctx, IupLaunchActivity.class)
            .setAction(ACTION_NOTIFY)
            .putExtra(EXTRA_IHANDLE, ihandlePtr)
            .putExtra(EXTRA_ACTION, 0)
            .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_SINGLE_TOP);
        int rc = (notifyId << 4);
        int flags = PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE;
        return PendingIntent.getActivity(ctx, rc, i, flags);
    }

    private static PendingIntent makeActionIntent(Context ctx, long ihandlePtr, int notifyId, int actionIndex)
    {
        Intent i = new Intent(ctx, IupNotifyReceiver.class)
            .setAction(ACTION_NOTIFY)
            .putExtra(EXTRA_IHANDLE, ihandlePtr)
            .putExtra(EXTRA_ACTION, actionIndex)
            .putExtra(EXTRA_NOTIFY_ID, notifyId);
        int rc = (notifyId << 4) | actionIndex;
        int flags = PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE;
        return PendingIntent.getBroadcast(ctx, rc, i, flags);
    }

    private static PendingIntent makeDeleteIntent(Context ctx, long ihandlePtr, int notifyId)
    {
        Intent i = new Intent(ctx, IupNotifyReceiver.class)
            .setAction(ACTION_DELETE)
            .putExtra(EXTRA_IHANDLE, ihandlePtr);
        int rc = (notifyId << 4) | 0xF;
        int flags = PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE;
        return PendingIntent.getBroadcast(ctx, rc, i, flags);
    }

    public static native void dispatchAction(long ihandlePtr, int actionIndex);
    public static native void dispatchClose(long ihandlePtr, int reason);
}
