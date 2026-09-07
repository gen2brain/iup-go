package io.github.gen2brain.iupgo;

import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;

import androidx.annotation.Keep;

import java.util.HashSet;
import java.util.Set;

public final class IupCameraHelper
{
    private static final int REQUEST_CODE = 0x43414D;

    private IupCameraHelper() {}

    private static final Set<Long> sPending = new HashSet<>();
    private static boolean sDenied = false;

    private static boolean hasPermission()
    {
        Context ctx = IupCommon.getContextThemeWrapper();
        if (ctx == null) return false;
        return ctx.checkSelfPermission(Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED;
    }

    /** 0 prompt, 1 granted, 2 denied. */
    @Keep
    public static int permissionState()
    {
        if (hasPermission()) return 1;
        return sDenied ? 2 : 0;
    }

    @Keep
    public static void requestPermission(long ih)
    {
        Activity act = IupActivity.currentActivity();
        if (act == null)
        {
            dispatchPermission(ih, false);
            return;
        }
        sPending.add(ih);
        act.requestPermissions(new String[]{Manifest.permission.CAMERA}, REQUEST_CODE);
    }

    @Keep
    public static void onPermissionResult(int requestCode, int[] grantResults)
    {
        if (requestCode != REQUEST_CODE) return;

        boolean granted = false;
        for (int r : grantResults) if (r == PackageManager.PERMISSION_GRANTED) granted = true;
        sDenied = !granted;

        Set<Long> pending = new HashSet<>(sPending);
        sPending.clear();
        for (Long ih : pending)
            dispatchPermission(ih, granted);
    }

    public static native void dispatchPermission(long ihandlePtr, boolean granted);
}
