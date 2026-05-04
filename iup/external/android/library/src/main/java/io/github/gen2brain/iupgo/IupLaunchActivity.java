package io.github.gen2brain.iupgo;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.util.Log;


/* loads native libs and calls the C entry point; subclass to override library/entry name */
public class IupLaunchActivity extends Activity
{
    private static final String TAG = "Iup";

    public native void IupEntry(IupLaunchActivity thisActivity, String entryFunctionName, String entryLibraryName);


    private boolean loadLibraryFailed = false;

    /* Override to add libraries (no lib prefix, no .so suffix). Must include "iup". */
    protected String[] getLibraries()
    {
        return new String[] { "iup" };
    }

    /** Override to change the library whose IupEntryPoint is called. */
    public String getEntryPointLibraryName()
    {
        return "libMyIupProgram.so";
    }

    private void loadLibraries()
    {
        String[] libs = getLibraries();
        for (String lib : libs)
        {
            System.loadLibrary(lib);
        }
        Log.i(TAG, "IupLaunchActivity.loadLibraries loaded " + java.util.Arrays.toString(libs));
    }

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        handleNotificationIntent(getIntent());

        /* IUP already running: notification just routed through here, finish */
        if (IupActivity.currentActivity() != null)
        {
            finish();
            return;
        }

        String errorMsg = "";
        try
        {
            loadLibraries();
        }
        catch (UnsatisfiedLinkError | Exception e)
        {
            Log.e(TAG, "IupLaunchActivity.loadLibraries failed: " + e.getMessage(), e);
            loadLibraryFailed = true;
            errorMsg = e.getMessage();
        }

        if (loadLibraryFailed)
        {
            showLoadLibraryFailedDialog(errorMsg);
        }
    }

    @Override
    protected void onStart()
    {
        super.onStart();

        String entryFunctionName = getManifestMetaString("ENTRY_POINT");
        String entryLibraryName = getManifestMetaString("ENTRY_LIBRARY");
        if (null == entryLibraryName)
        {
            entryLibraryName = getEntryPointLibraryName();
        }

        IupEntry(this, entryFunctionName, entryLibraryName);
    }

    @Override
    protected void onNewIntent(Intent intent)
    {
        super.onNewIntent(intent);
        handleNotificationIntent(intent);
        if (IupActivity.currentActivity() != null)
        {
            finish();
        }
    }

    private void handleNotificationIntent(Intent intent)
    {
        if (intent == null) return;
        if (IupNotifyHelper.ACTION_NOTIFY.equals(intent.getAction()))
        {
            long ih = intent.getLongExtra(IupNotifyHelper.EXTRA_IHANDLE, 0);
            int action = intent.getIntExtra(IupNotifyHelper.EXTRA_ACTION, 0);
            if (ih != 0)
            {
                IupNotifyHelper.dispatchAction(ih, action);
            }
        }
    }

    private String getManifestMetaString(String key)
    {
        try
        {
            ApplicationInfo appInfo = getPackageManager().getApplicationInfo(getPackageName(), PackageManager.GET_META_DATA);
            Bundle bundle = appInfo.metaData;
            if (bundle == null)
            {
                return null;
            }
            return bundle.getString(key);
        }
        catch (PackageManager.NameNotFoundException e)
        {
            Log.e(TAG, "IupLaunchActivity: meta-data lookup failed for " + key + ": " + e.getMessage());
            return null;
        }
    }

    private void showLoadLibraryFailedDialog(String errorMsg)
    {
        AlertDialog.Builder dialog = new AlertDialog.Builder(this);
        dialog.setMessage("An error occurred while trying to start the application after calling System.loadLibrary()."
            + System.lineSeparator()
            + System.lineSeparator()
            + "Error: " + errorMsg);
        dialog.setTitle("IUP Error");
        dialog.setPositiveButton("Exit", (d, id) -> IupLaunchActivity.this.finish());
        dialog.setCancelable(false);
        dialog.create().show();
    }
}
