package io.github.gen2brain.iupgo;

import android.content.Context;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.util.Log;
import androidx.annotation.Keep;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Locale;


public final class IupImageHelper
{
    private static final String TAG = "Iup";

    /* tagged 160dpi so BitmapDrawable treats IUP px as dp (32 px -> 96 phys px on 3x) */
    @Keep
    public static Bitmap createBitmap(int width, int height, int bitsPerPixel)
    {
        Bitmap bmp = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        bmp.setDensity(android.util.DisplayMetrics.DENSITY_DEFAULT);
        return bmp;
    }

    /* try filesystem first, fall back to APK assets; null if neither yields a bitmap */
    @Keep
    public static Bitmap loadBitmap(String fileName)
    {
        if (null == fileName)
        {
            return null;
        }

        File file = new File(fileName);
        if (file.exists())
        {
            return BitmapFactory.decodeFile(fileName);
        }

        Context context = IupApplication.getIupApplication();
        AssetManager assetManager = context.getAssets();

        try (InputStream inputStream = assetManager.open(fileName)) {
            return BitmapFactory.decodeStream(inputStream);
        } catch (IOException ex) {
            Log.w(TAG, "loadBitmap: asset open failed for " + fileName + ": " + ex.getMessage());
            return null;
        }
        /* Close-failure on read path is non-actionable; leak is bounded. */
    }

    @Keep
    public static boolean saveBitmap(Bitmap bitmap, String fileName, String format, int quality)
    {
        Bitmap.CompressFormat fmt = pickCompressFormat(format);
        if (fmt == null || bitmap == null || fileName == null) return false;
        try (FileOutputStream fos = new FileOutputStream(fileName)) {
            return bitmap.compress(fmt, quality, fos);
        } catch (IOException ex) {
            Log.w(TAG, "saveBitmap: " + fileName + ": " + ex.getMessage());
            return false;
        }
    }

    @Keep
    public static byte[] saveBitmapToBuffer(Bitmap bitmap, String format, int quality)
    {
        Bitmap.CompressFormat fmt = pickCompressFormat(format);
        if (fmt == null || bitmap == null) return null;
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        if (!bitmap.compress(fmt, quality, baos)) return null;
        return baos.toByteArray();
    }

    private static Bitmap.CompressFormat pickCompressFormat(String format)
    {
        if (format == null) return null;
        String up = format.toUpperCase(Locale.ROOT);
        switch (up)
        {
            case "PNG":  return Bitmap.CompressFormat.PNG;
            case "JPG":
            case "JPEG": return Bitmap.CompressFormat.JPEG;
        }
        return null;
    }
}
