package io.github.gen2brain.iupgo;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import androidx.annotation.Keep;


/* IupHelp/IupExecute: ACTION_VIEW for URLs, getLaunchIntentForPackage for package names */
public final class IupHelpHelper
{
    private IupHelpHelper() {}

    private static Context resolveContext()
    {
        Activity a = IupActivity.currentActivity();
        if (a != null) return a;
        return IupApplication.getIupApplication();
    }

    @Keep
    public static int openUrl(String url)
    {
        if (url == null || url.isEmpty()) return -1;

        Context ctx = resolveContext();
        if (ctx == null) return -1;

        Uri uri = Uri.parse(url);
        /* keep ACTION_VIEW to common user-facing schemes; reject intent://, javascript:, etc. */
        String scheme = uri.getScheme();
        if (scheme == null) return -1;
        scheme = scheme.toLowerCase(java.util.Locale.ROOT);
        if (!(scheme.equals("http") || scheme.equals("https") || scheme.equals("mailto")
              || scheme.equals("tel") || scheme.equals("sms") || scheme.equals("geo")
              || scheme.equals("content")))
            return -1;

        Intent intent = new Intent(Intent.ACTION_VIEW, uri);
        if (!(ctx instanceof Activity))
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

        try
        {
            ctx.startActivity(intent);
            return 1;
        }
        catch (ActivityNotFoundException ex)
        {
            return -2;
        }
        catch (Exception ex)
        {
            return -1;
        }
    }

    @Keep
    public static int launchPackage(String packageName)
    {
        if (packageName == null || packageName.isEmpty()) return -1;

        Context ctx = resolveContext();
        if (ctx == null) return -1;

        PackageManager pm = ctx.getPackageManager();
        Intent intent = pm.getLaunchIntentForPackage(packageName);
        if (intent == null) return -2;

        if (!(ctx instanceof Activity))
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

        try
        {
            ctx.startActivity(intent);
            return 1;
        }
        catch (ActivityNotFoundException ex)
        {
            return -2;
        }
        catch (Exception ex)
        {
            return -1;
        }
    }
}
