package io.github.gen2brain.iupgo;

import android.content.ClipData;
import android.content.ClipDescription;
import android.content.ClipboardManager;
import android.content.Context;
import androidx.annotation.Keep;


/* plain text only; images/typed formats need ContentProvider plumbing */
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
}
