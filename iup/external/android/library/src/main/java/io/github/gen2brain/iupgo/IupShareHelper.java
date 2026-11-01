package io.github.gen2brain.iupgo;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.ClipData;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.Uri;
import android.util.Log;
import android.webkit.MimeTypeMap;
import androidx.annotation.Keep;
import androidx.core.content.ContextCompat;
import androidx.core.content.FileProvider;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Locale;


public final class IupShareHelper
{
    private static final String TAG = "IupShareHelper";
    private static final String ACTION_CHOSEN = "io.github.gen2brain.iupgo.SHARE_CHOSEN";

    public static final int REQ_SHARE = 0x7110;

    private static final boolean[] sDone = new boolean[1];
    private static boolean sChosen;

    private IupShareHelper() {}

    public static boolean isShareRequest(int requestCode)
    {
        return requestCode == REQ_SHARE;
    }

    public static void deliverResult()
    {
        sDone[0] = true;
    }

    private static Uri stageFile(Context ctx, String path) throws Exception
    {
        File src = new File(path);
        File dir = new File(ctx.getCacheDir(), "share");
        if (!dir.isDirectory() && !dir.mkdirs()) return null;

        File[] old = dir.listFiles();
        if (old != null)
        {
            for (File f : old) f.delete();
        }

        File dst = new File(dir, src.getName());
        try (InputStream in = new FileInputStream(src); OutputStream out = new FileOutputStream(dst))
        {
            byte[] buffer = new byte[65536];
            int n;
            while ((n = in.read(buffer)) > 0) out.write(buffer, 0, n);
        }
        return FileProvider.getUriForFile(ctx, ctx.getPackageName() + ".iupclipboard", dst);
    }

    private static String mimeType(String path)
    {
        int dot = path.lastIndexOf('.');
        if (dot >= 0)
        {
            String type = MimeTypeMap.getSingleton().getMimeTypeFromExtension(path.substring(dot + 1).toLowerCase(Locale.ROOT));
            if (type != null) return type;
        }
        return "application/octet-stream";
    }

    @Keep
    public static int share(String title, String text, String url, String file)
    {
        Activity a = IupActivity.currentActivity();
        if (a == null) return -1;

        Intent intent = new Intent(Intent.ACTION_SEND);
        String body = text;
        if (url != null)
            body = (body != null) ? body + "\n" + url : url;
        if (body != null) intent.putExtra(Intent.EXTRA_TEXT, body);
        if (title != null) intent.putExtra(Intent.EXTRA_SUBJECT, title);

        if (file != null)
        {
            Uri uri;
            try
            {
                uri = stageFile(a, file);
            }
            catch (Exception e)
            {
                Log.e(TAG, "share file failed: " + e);
                return -1;
            }
            if (uri == null) return -1;
            intent.setType(mimeType(file));
            intent.putExtra(Intent.EXTRA_STREAM, uri);
            intent.setClipData(ClipData.newRawUri(null, uri));
            intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        }
        else
            intent.setType("text/plain");

        BroadcastReceiver receiver = new BroadcastReceiver()
        {
            @Override
            public void onReceive(Context context, Intent data)
            {
                sChosen = true;
            }
        };
        ContextCompat.registerReceiver(a, receiver, new IntentFilter(ACTION_CHOSEN), ContextCompat.RECEIVER_NOT_EXPORTED);

        Intent chosen = new Intent(ACTION_CHOSEN).setPackage(a.getPackageName());
        PendingIntent pending = PendingIntent.getBroadcast(a, REQ_SHARE, chosen, PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_MUTABLE);

        sDone[0] = false;
        sChosen = false;

        try
        {
            a.startActivityForResult(Intent.createChooser(intent, title, pending.getIntentSender()), REQ_SHARE);
        }
        catch (Exception e)
        {
            Log.e(TAG, "share failed: " + e);
            a.unregisterReceiver(receiver);
            return -1;
        }

        IupCommon.pumpUntilDone(sDone);
        a.unregisterReceiver(receiver);
        return sChosen ? 1 : 0;
    }
}
