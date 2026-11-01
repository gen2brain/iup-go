package io.github.gen2brain.iupgo;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkRequest;
import android.util.Log;
import androidx.annotation.Keep;

public final class IupNetworkHelper
{
    private static final String TAG = "IupNetworkHelper";

    private static ConnectivityManager.NetworkCallback sCallback;
    private static long sHandle;

    private IupNetworkHelper() {}

    private static native void nativeSetHandle(long handle);

    private static synchronized void setHandle(long handle)
    {
        sHandle = handle;
        nativeSetHandle(handle);
    }

    @Keep
    public static boolean setNetwork(int transport)
    {
        Context ctx = IupCommon.getContextThemeWrapper().getApplicationContext();
        final ConnectivityManager manager = (ConnectivityManager) ctx.getSystemService(Context.CONNECTIVITY_SERVICE);
        if (manager == null) return false;

        if (sCallback != null)
        {
            try { manager.unregisterNetworkCallback(sCallback); } catch (Exception e) { Log.e(TAG, "unregisterNetworkCallback failed: " + e); }
            sCallback = null;
        }
        manager.bindProcessToNetwork(null);
        setHandle(0);

        if (transport < 0) return true;

        NetworkRequest request = new NetworkRequest.Builder()
            .addCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET)
            .addTransportType(transport)
            .build();

        ConnectivityManager.NetworkCallback callback = new ConnectivityManager.NetworkCallback()
        {
            @Override
            public void onAvailable(Network network)
            {
                if (manager.bindProcessToNetwork(network))
                    setHandle(network.getNetworkHandle());
            }

            @Override
            public void onLost(Network network)
            {
                synchronized (IupNetworkHelper.class)
                {
                    if (sHandle == network.getNetworkHandle())
                    {
                        manager.bindProcessToNetwork(null);
                        setHandle(0);
                    }
                }
            }
        };

        try
        {
            manager.requestNetwork(request, callback);
        }
        catch (Exception e)
        {
            Log.e(TAG, "requestNetwork failed: " + e);
            return false;
        }

        sCallback = callback;
        return true;
    }
}
