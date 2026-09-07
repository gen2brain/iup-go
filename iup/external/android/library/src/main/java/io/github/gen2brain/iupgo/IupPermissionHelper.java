package io.github.gen2brain.iupgo;

import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;

import androidx.annotation.Keep;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

public final class IupPermissionHelper
{
    private static final int REQUEST_CODE = 0x50524D;

    private IupPermissionHelper() {}

    private static final Map<String, Set<Long>> sPending = new HashMap<>();
    private static final Set<String> sDenied = new HashSet<>();

    private static boolean hasPermission(String permission)
    {
        Context ctx = IupCommon.getContextThemeWrapper();
        if (ctx == null) return false;
        return ctx.checkSelfPermission(permission) == PackageManager.PERMISSION_GRANTED;
    }

    /** 0 prompt, 1 granted, 2 denied. */
    @Keep
    public static int permissionState(String permission)
    {
        if (hasPermission(permission)) return 1;
        return sDenied.contains(permission) ? 2 : 0;
    }

    @Keep
    public static void requestPermission(String permission, long ih)
    {
        Activity act = IupActivity.currentActivity();
        if (act == null)
        {
            dispatchPermission(permission, ih, false);
            return;
        }
        Set<Long> pending = sPending.get(permission);
        if (pending == null)
        {
            pending = new HashSet<>();
            sPending.put(permission, pending);
        }
        pending.add(ih);
        act.requestPermissions(new String[]{permission}, REQUEST_CODE);
    }

    @Keep
    public static void onPermissionResult(int requestCode, String[] permissions, int[] grantResults)
    {
        if (requestCode != REQUEST_CODE || permissions.length == 0) return;

        String permission = permissions[0];
        boolean granted = false;
        for (int r : grantResults) if (r == PackageManager.PERMISSION_GRANTED) granted = true;
        if (granted) sDenied.remove(permission); else sDenied.add(permission);

        Set<Long> pending = sPending.remove(permission);
        if (pending == null) return;
        for (Long ih : pending)
            dispatchPermission(permission, ih, granted);
    }

    public static native void dispatchPermission(String permission, long ihandlePtr, boolean granted);
}
