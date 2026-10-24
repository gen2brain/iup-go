package io.github.gen2brain.iupgo;

import android.app.Activity;
import android.content.ClipData;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.provider.OpenableColumns;
import android.util.Log;
import android.webkit.MimeTypeMap;
import androidx.annotation.Keep;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.LinkedHashSet;
import java.util.Locale;


/* SAF gives content:// URIs, IUP wants fopen paths; stage through getCacheDir(). DIR unsupported. */
public final class IupFileDlgHelper
{
    private static final String TAG = "Iup";

    private IupFileDlgHelper() {}


    /* Reserved request-code range; IupActivity dispatches here. */
    public static final int REQ_OPEN       = 0x7100;
    public static final int REQ_OPEN_MULTI = 0x7101;
    public static final int REQ_SAVE       = 0x7102;

    public static boolean isFileDlgRequest(int rc)
    {
        return rc >= REQ_OPEN && rc <= REQ_SAVE;
    }


    /* Modal, one FileDlg in flight at a time. */
    private static final boolean[] sDone = new boolean[1];
    private static int   sStatus;      /* 0 ok, -1 cancel */
    private static Uri   sResultUri;
    private static Uri[] sResultUris;

    private static Uri    sPendingSaveUri;
    private static String sPendingCachePath;

    /* Raw URIs from the last pick; C reads these into VALUE_URI for recent-files. */
    private static String sLastUri;
    private static String[] sLastUris;

    @Keep public static String   getLastUri()  { return sLastUri; }
    @Keep public static String[] getLastUris() { return sLastUris; }


    /* Returns newline-joined cache paths, or "" on cancel. */
    @Keep
    public static String pickOpen(String patterns, boolean allowMultiple)
    {
        Activity a = IupActivity.currentActivity();
        if (a == null) return "";

        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        String[] types = mimeTypes(patterns);
        if (types != null && types.length == 1)
            intent.setType(types[0]);
        else
        {
            intent.setType("*/*");
            if (types != null) intent.putExtra(Intent.EXTRA_MIME_TYPES, types);
        }
        if (allowMultiple) intent.putExtra(Intent.EXTRA_ALLOW_MULTIPLE, true);

        resetState();
        try
        {
            a.startActivityForResult(intent, allowMultiple ? REQ_OPEN_MULTI : REQ_OPEN);
        }
        catch (Exception e)
        {
            Log.e(TAG, "FileDlg pickOpen startActivityForResult failed: " + e);
            return "";
        }
        IupCommon.pumpUntilDone(sDone);
        if (sStatus != 0) return "";

        File dir = stageDir(a);
        if (allowMultiple && sResultUris != null)
        {
            StringBuilder sb = new StringBuilder();
            for (Uri u : sResultUris)
            {
                String p = copyUriToCache(a, u, dir);
                if (p == null) continue;
                if (sb.length() > 0) sb.append('\n');
                sb.append(p);
            }
            return sb.toString();
        }
        if (sResultUri != null)
        {
            String p = copyUriToCache(a, sResultUri, dir);
            return p != null ? p : "";
        }
        return "";
    }

    /* Returns a writable cache path. commitSave() flushes to the SAF URI. */
    @Keep
    public static String pickSave(String patterns, String defaultName)
    {
        Activity a = IupActivity.currentActivity();
        if (a == null) return "";

        String mimeType = null;
        if (defaultName != null)
        {
            int dot = defaultName.lastIndexOf('.');
            if (dot >= 0) mimeType = mimeFromExtension(defaultName.substring(dot + 1));
        }
        if (mimeType == null)
        {
            String[] types = mimeTypes(patterns);
            mimeType = types != null ? types[0] : "application/octet-stream";
        }

        Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType(mimeType);
        if (defaultName != null && !defaultName.isEmpty())
            intent.putExtra(Intent.EXTRA_TITLE, defaultName);

        resetState();
        try
        {
            a.startActivityForResult(intent, REQ_SAVE);
        }
        catch (Exception e)
        {
            Log.e(TAG, "FileDlg pickSave startActivityForResult failed: " + e);
            return "";
        }
        IupCommon.pumpUntilDone(sDone);
        if (sStatus != 0 || sResultUri == null) return "";

        String displayName = queryDisplayName(a, sResultUri);
        if (displayName == null || displayName.isEmpty()) displayName = "file";
        File cache = stageFile(a, displayName);
        try
        {
            cache.createNewFile();
        }
        catch (IOException e)
        {
            Log.e(TAG, "FileDlg pickSave cache createNewFile failed: " + e);
            return "";
        }
        sPendingSaveUri   = sResultUri;
        sPendingCachePath = cache.getAbsolutePath();
        return sPendingCachePath;
    }

    /* Flushes cache file -> SAF URI. Called at FileDlg destroy. */
    @Keep
    public static void commitSave()
    {
        if (sPendingSaveUri == null || sPendingCachePath == null) return;
        Activity a = IupActivity.currentActivity();
        if (a == null) return;

        File f = new File(sPendingCachePath);
        try (InputStream in = new FileInputStream(f);
             OutputStream out = a.getContentResolver().openOutputStream(sPendingSaveUri, "w"))
        {
            if (out != null)
            {
                byte[] buf = new byte[8192];
                int n;
                while ((n = in.read(buf)) > 0) out.write(buf, 0, n);
            }
        }
        catch (IOException e)
        {
            Log.e(TAG, "FileDlg commitSave failed: " + e);
        }
        f.delete();
        f.getParentFile().delete();
        sPendingSaveUri   = null;
        sPendingCachePath = null;
    }


    public static void deliverResult(int requestCode, int resultCode, Intent data)
    {
        if (resultCode == Activity.RESULT_OK && data != null)
        {
            ClipData cd = data.getClipData();
            if (cd != null)
            {
                sResultUris = new Uri[cd.getItemCount()];
                for (int i = 0; i < cd.getItemCount(); i++)
                    sResultUris[i] = cd.getItemAt(i).getUri();
                if (sResultUris.length > 0) sResultUri = sResultUris[0];
            }
            else
            {
                sResultUri = data.getData();
            }
            persistUriPermissions(data, requestCode);
            captureLastUris();
            sStatus = 0;
        }
        else
        {
            sStatus = -1;
            sLastUri = null;
            sLastUris = null;
        }
        sDone[0] = true;
    }

    /* Persist the SAF grant so stored URIs survive app restarts (needed for recent-files). */
    private static void persistUriPermissions(Intent data, int requestCode)
    {
        Activity a = IupActivity.currentActivity();
        if (a == null) return;
        int take = data.getFlags() & (Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
        if (take == 0) take = Intent.FLAG_GRANT_READ_URI_PERMISSION;
        if (requestCode == REQ_SAVE) take |= Intent.FLAG_GRANT_WRITE_URI_PERMISSION;
        try
        {
            if (sResultUris != null)
            {
                for (Uri u : sResultUris)
                    a.getContentResolver().takePersistableUriPermission(u, take);
            }
            else if (sResultUri != null)
            {
                a.getContentResolver().takePersistableUriPermission(sResultUri, take);
            }
        }
        catch (SecurityException e)
        {
            Log.w(TAG, "FileDlg takePersistableUriPermission failed: " + e);
        }
    }

    private static void captureLastUris()
    {
        sLastUri  = sResultUri != null ? sResultUri.toString() : null;
        if (sResultUris != null)
        {
            sLastUris = new String[sResultUris.length];
            for (int i = 0; i < sResultUris.length; i++)
                sLastUris[i] = sResultUris[i].toString();
        }
        else
        {
            sLastUris = null;
        }
    }


    /* Copies a previously-picked content:// URI to a fresh cache file for RecentMenu dispatch. */
    @Keep
    public static String stageRecentUri(String uriString)
    {
        if (uriString == null || uriString.isEmpty()) return "";
        Activity a = IupActivity.currentActivity();
        if (a == null) return "";
        Uri uri = Uri.parse(uriString);
        String name = queryDisplayName(a, uri);
        if (name == null || name.isEmpty()) name = "file";
        File cache = stageFile(a, name);
        try (InputStream in = a.getContentResolver().openInputStream(uri);
             OutputStream out = new FileOutputStream(cache))
        {
            if (in == null) return "";
            byte[] buf = new byte[8192];
            int n;
            while ((n = in.read(buf)) > 0) out.write(buf, 0, n);
        }
        catch (Exception e)
        {
            Log.e(TAG, "FileDlg stageRecentUri failed: " + e);
            return "";
        }
        return cache.getAbsolutePath();
    }


    private static void resetState()
    {
        sDone[0] = false;
        sStatus = -1;
        sResultUri = null;
        sResultUris = null;
    }

    private static String queryDisplayName(Activity a, Uri uri)
    {
        try (Cursor c = a.getContentResolver().query(uri, null, null, null, null))
        {
            if (c != null && c.moveToFirst())
            {
                int col = c.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                if (col >= 0) return safeLeaf(c.getString(col));
            }
        }
        catch (Exception e) { /* ignore */ }
        return null;
    }

    /* hostile DocumentsProvider can return display names with separators or ".."; strip to a safe leaf. */
    static String safeLeaf(String s)
    {
        if (s == null) return null;
        int slash = Math.max(s.lastIndexOf('/'), s.lastIndexOf('\\'));
        if (slash >= 0) s = s.substring(slash + 1);
        int nul = s.indexOf('\0');
        if (nul >= 0) s = s.substring(0, nul);
        while (s.startsWith(".")) s = s.substring(1);
        if (s.length() > 128) s = s.substring(0, 128);
        return s;
    }

    /* null means all files */
    private static String[] mimeTypes(String patterns)
    {
        if (patterns == null || patterns.isEmpty()) return null;
        LinkedHashSet<String> types = new LinkedHashSet<>();
        for (String p : patterns.split(";"))
        {
            p = p.trim();
            if (!p.startsWith("*.")) return null;
            String mime = mimeFromExtension(p.substring(2));
            if (mime == null) return null;
            types.add(mime);
        }
        return types.isEmpty() ? null : types.toArray(new String[0]);
    }

    private static String mimeFromExtension(String ext)
    {
        if (ext.isEmpty() || ext.contains("*") || ext.contains("?")) return null;
        return MimeTypeMap.getSingleton().getMimeTypeFromExtension(ext.toLowerCase(Locale.ROOT));
    }

    private static File stageDir(Activity a)
    {
        File dir = new File(a.getCacheDir(), "iup_filedlg_" + System.nanoTime());
        dir.mkdirs();
        return dir;
    }

    private static File stageFile(Activity a, String name)
    {
        return new File(stageDir(a), name);
    }

    private static String copyUriToCache(Activity a, Uri uri, File dir)
    {
        String name = queryDisplayName(a, uri);
        if (name == null || name.isEmpty()) name = "file";
        File cache = new File(dir, name);
        int dot = name.lastIndexOf('.');
        String stem = dot > 0 ? name.substring(0, dot) : name;
        String ext = dot > 0 ? name.substring(dot) : "";
        for (int i = 2; cache.exists(); i++)
            cache = new File(dir, stem + " (" + i + ")" + ext);
        try (InputStream in = a.getContentResolver().openInputStream(uri);
             OutputStream out = new FileOutputStream(cache))
        {
            if (in == null) return null;
            byte[] buf = new byte[8192];
            int n;
            while ((n = in.read(buf)) > 0) out.write(buf, 0, n);
        }
        catch (IOException e)
        {
            Log.e(TAG, "FileDlg copyUriToCache failed: " + e);
            return null;
        }
        return cache.getAbsolutePath();
    }


}
