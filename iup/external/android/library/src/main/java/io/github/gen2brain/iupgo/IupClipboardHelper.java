package io.github.gen2brain.iupgo;

import android.content.ClipData;
import android.content.ClipDescription;
import android.content.ClipboardManager;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;

import androidx.annotation.Keep;
import androidx.core.content.FileProvider;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;


/* plain text + custom string formats (custom MIME on a text item); images go through the FileProvider */
public final class IupClipboardHelper
{
    private IupClipboardHelper() {}

    private static ClipboardManager getClipboard()
    {
        Context ctx = IupApplication.getIupApplication();
        return (ClipboardManager) ctx.getSystemService(Context.CLIPBOARD_SERVICE);
    }

    @Keep
    public static void setText(String text)
    {
        ClipboardManager cm = getClipboard();
        if (cm == null) return;
        if (text == null)
        {
            /* API 28+ has clearPrimaryClip; earlier levels overwrite with empty. */
            if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.P)
                cm.clearPrimaryClip();
            else
                cm.setPrimaryClip(ClipData.newPlainText(null, ""));
            return;
        }
        cm.setPrimaryClip(ClipData.newPlainText(null, text));
    }

    @Keep
    public static String getText()
    {
        ClipboardManager cm = getClipboard();
        if (cm == null) return null;
        if (!cm.hasPrimaryClip()) return null;

        ClipData clip = cm.getPrimaryClip();
        if (clip == null || clip.getItemCount() == 0) return null;

        CharSequence cs = clip.getItemAt(0).coerceToText(IupApplication.getIupApplication());
        return cs != null ? cs.toString() : null;
    }

    @Keep
    public static boolean isTextAvailable()
    {
        ClipboardManager cm = getClipboard();
        if (cm == null || !cm.hasPrimaryClip()) return false;
        ClipDescription desc = cm.getPrimaryClipDescription();
        if (desc == null) return false;
        return desc.hasMimeType(ClipDescription.MIMETYPE_TEXT_PLAIN)
            || desc.hasMimeType(ClipDescription.MIMETYPE_TEXT_HTML);
    }

    @Keep
    public static boolean setImage(Bitmap bitmap)
    {
        ClipboardManager cm = getClipboard();
        Context ctx = IupApplication.getIupApplication();
        if (cm == null || ctx == null || bitmap == null) return false;
        try
        {
            File dir = new File(ctx.getCacheDir(), "clipboard");
            if (!dir.isDirectory() && !dir.mkdirs()) return false;
            File file = new File(dir, "image.png");
            try (FileOutputStream out = new FileOutputStream(file))
            {
                bitmap.compress(Bitmap.CompressFormat.PNG, 100, out);
            }
            Uri uri = FileProvider.getUriForFile(ctx, ctx.getPackageName() + ".iupclipboard", file);
            cm.setPrimaryClip(ClipData.newUri(ctx.getContentResolver(), "image", uri));
            return true;
        }
        catch (Exception e)
        {
            return false;
        }
    }

    @Keep
    public static Bitmap getImage()
    {
        ClipboardManager cm = getClipboard();
        Context ctx = IupApplication.getIupApplication();
        if (cm == null || ctx == null || !cm.hasPrimaryClip()) return null;
        ClipData clip = cm.getPrimaryClip();
        if (clip == null) return null;

        for (int i = 0; i < clip.getItemCount(); i++)
        {
            Uri uri = clip.getItemAt(i).getUri();
            if (uri == null) continue;
            try (InputStream in = ctx.getContentResolver().openInputStream(uri))
            {
                Bitmap bitmap = BitmapFactory.decodeStream(in);
                if (bitmap != null) return bitmap;
            }
            catch (Exception ignored)
            {
            }
        }
        return null;
    }

    @Keep
    public static boolean isImageAvailable()
    {
        ClipboardManager cm = getClipboard();
        if (cm == null || !cm.hasPrimaryClip()) return false;
        ClipDescription desc = cm.getPrimaryClipDescription();
        if (desc == null) return false;
        for (int i = 0; i < desc.getMimeTypeCount(); i++)
        {
            if (desc.getMimeType(i).startsWith("image/")) return true;
        }
        return false;
    }

    @Keep
    public static void setFormatData(String mime, String text)
    {
        ClipboardManager cm = getClipboard();
        if (cm == null || mime == null) return;
        if (text == null)
        {
            if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.P)
                cm.clearPrimaryClip();
            else
                cm.setPrimaryClip(ClipData.newPlainText(null, ""));
            return;
        }
        ClipData clip = new ClipData(new ClipDescription(null, new String[]{ mime }), new ClipData.Item(text));
        cm.setPrimaryClip(clip);
    }

    @Keep
    public static String getFormatData(String mime)
    {
        ClipboardManager cm = getClipboard();
        if (cm == null || mime == null || !cm.hasPrimaryClip()) return null;

        ClipDescription desc = cm.getPrimaryClipDescription();
        if (desc == null || !desc.hasMimeType(mime)) return null;

        ClipData clip = cm.getPrimaryClip();
        if (clip == null || clip.getItemCount() == 0) return null;

        CharSequence cs = clip.getItemAt(0).getText();
        return cs != null ? cs.toString() : null;
    }

    @Keep
    public static boolean isFormatAvailable(String mime)
    {
        ClipboardManager cm = getClipboard();
        if (cm == null || mime == null || !cm.hasPrimaryClip()) return false;
        ClipDescription desc = cm.getPrimaryClipDescription();
        return desc != null && desc.hasMimeType(mime);
    }
}
