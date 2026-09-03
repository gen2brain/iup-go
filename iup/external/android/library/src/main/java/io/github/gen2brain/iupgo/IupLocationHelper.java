package io.github.gen2brain.iupgo;

import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.os.Looper;

import androidx.annotation.Keep;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

public final class IupLocationHelper
{
    private static final int REQUEST_CODE = 0x4C4F43;

    private IupLocationHelper() {}

    private static final class Pending
    {
        long ih;
        boolean fine;
        long intervalMs;
        float distanceM;
    }

    private static final Map<Long, LocationListener> sListeners = new HashMap<>();
    private static final Map<Long, Pending> sPending = new HashMap<>();
    private static boolean sDenied = false;

    private static LocationManager manager()
    {
        Context ctx = IupCommon.getContextThemeWrapper();
        return ctx == null ? null : (LocationManager)ctx.getSystemService(Context.LOCATION_SERVICE);
    }

    private static boolean hasPermission(boolean fine)
    {
        Context ctx = IupCommon.getContextThemeWrapper();
        if (ctx == null) return false;
        String perm = fine ? Manifest.permission.ACCESS_FINE_LOCATION : Manifest.permission.ACCESS_COARSE_LOCATION;
        return ctx.checkSelfPermission(perm) == PackageManager.PERMISSION_GRANTED;
    }

    @Keep
    public static boolean isAvailable()
    {
        LocationManager lm = manager();
        if (lm == null) return false;
        List<String> providers = lm.getAllProviders();
        return providers != null && !providers.isEmpty();
    }

    /** 0 prompt, 1 granted, 2 denied, 3 unavailable. */
    @Keep
    public static int permissionState()
    {
        if (!isAvailable()) return 3;
        if (hasPermission(false)) return 1;
        return sDenied ? 2 : 0;
    }

    private static String provider(LocationManager lm, boolean fine)
    {
        String preferred = fine ? LocationManager.GPS_PROVIDER : LocationManager.NETWORK_PROVIDER;
        String other = fine ? LocationManager.NETWORK_PROVIDER : LocationManager.GPS_PROVIDER;
        if (lm.isProviderEnabled(preferred)) return preferred;
        if (lm.isProviderEnabled(other)) return other;
        if (lm.isProviderEnabled(LocationManager.PASSIVE_PROVIDER)) return LocationManager.PASSIVE_PROVIDER;
        return null;
    }

    private static void deliver(long ih, Location loc)
    {
        dispatchFix(ih, loc.getLatitude(), loc.getLongitude(),
                    loc.getAltitude(), loc.hasAltitude(),
                    loc.getAccuracy(),
                    loc.getSpeed(), loc.hasSpeed(),
                    loc.getBearing(), loc.hasBearing(),
                    loc.getTime());
    }

    private static boolean subscribe(final long ih, boolean fine, long intervalMs, float distanceM)
    {
        LocationManager lm = manager();
        if (lm == null) { dispatchError(ih, "Location service not available"); return false; }

        String provider = provider(lm, fine);
        if (provider == null) { dispatchError(ih, "No location provider enabled"); return false; }

        LocationListener listener = new LocationListener() {
            @Override public void onLocationChanged(Location loc) { deliver(ih, loc); }
            @Override public void onProviderDisabled(String p) { dispatchError(ih, "Location provider disabled"); }
            @Override public void onProviderEnabled(String p) {}
        };

        stop(ih);
        try
        {
            lm.requestLocationUpdates(provider, intervalMs, distanceM, listener, Looper.getMainLooper());
            Location last = lm.getLastKnownLocation(provider);
            if (last != null) deliver(ih, last);
        }
        catch (SecurityException e)
        {
            dispatchError(ih, "Location access denied");
            return false;
        }

        sListeners.put(ih, listener);
        return true;
    }

    @Keep
    public static boolean start(long ih, boolean fine, long intervalMs, float distanceM)
    {
        if (hasPermission(fine))
            return subscribe(ih, fine, intervalMs, distanceM);

        Activity act = IupActivity.currentActivity();
        if (act == null) { dispatchError(ih, "No current activity"); return false; }

        Pending p = new Pending();
        p.ih = ih; p.fine = fine; p.intervalMs = intervalMs; p.distanceM = distanceM;
        sPending.put(ih, p);

        String[] perms = fine
            ? new String[]{Manifest.permission.ACCESS_FINE_LOCATION, Manifest.permission.ACCESS_COARSE_LOCATION}
            : new String[]{Manifest.permission.ACCESS_COARSE_LOCATION};
        act.requestPermissions(perms, REQUEST_CODE);
        return true;
    }

    @Keep
    public static void onPermissionResult(int requestCode, int[] grantResults)
    {
        if (requestCode != REQUEST_CODE) return;

        boolean granted = false;
        for (int r : grantResults) if (r == PackageManager.PERMISSION_GRANTED) granted = true;
        sDenied = !granted;

        Map<Long, Pending> pending = new HashMap<>(sPending);
        sPending.clear();
        for (Pending p : pending.values())
        {
            dispatchPermission(p.ih, granted);
            if (granted) subscribe(p.ih, p.fine && hasPermission(true), p.intervalMs, p.distanceM);
            else dispatchError(p.ih, "Location access denied");
        }
    }

    @Keep
    public static void stop(long ih)
    {
        sPending.remove(ih);
        LocationListener listener = sListeners.remove(ih);
        if (listener == null) return;
        LocationManager lm = manager();
        if (lm != null) lm.removeUpdates(listener);
    }

    public static native void dispatchFix(long ihandlePtr, double latitude, double longitude,
                                          double altitude, boolean hasAltitude, float accuracy,
                                          float speed, boolean hasSpeed, float bearing, boolean hasBearing,
                                          long timeMs);
    public static native void dispatchPermission(long ihandlePtr, boolean granted);
    public static native void dispatchError(long ihandlePtr, String message);
}
