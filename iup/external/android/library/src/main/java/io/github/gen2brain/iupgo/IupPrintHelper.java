package io.github.gen2brain.iupgo;

import android.app.Activity;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.graphics.pdf.PdfDocument;
import android.os.Bundle;
import android.os.CancellationSignal;
import android.os.ParcelFileDescriptor;
import android.print.PageRange;
import android.print.PrintAttributes;
import android.print.PrintDocumentAdapter;
import android.print.PrintDocumentInfo;
import android.print.PrintManager;
import android.print.pdf.PrintedPdfDocument;
import android.util.Log;
import androidx.annotation.Keep;
import java.io.FileOutputStream;
import java.io.IOException;

public final class IupPrintHelper
{
    private static final String TAG = "IupPrintHelper";
    private static final float PRINT_SCALE = 72.0f / 96.0f;
    private static final int IUP_CLOSE = -3;

    private static final boolean[] sDone = new boolean[1];
    private static int sStatus;
    private static boolean sLandscape;

    private IupPrintHelper() {}

    private static native int nativePaginate(long ihandlePtr, int width, int height);
    private static native int nativeDrawPage(long ihandlePtr, IupAndroidCanvas view, int page, int width, int height);

    @Keep
    public static boolean lastLandscape() { return sLandscape; }

    @Keep
    public static int print(long ihandlePtr, String jobName, boolean landscape)
    {
        Activity a = IupActivity.currentActivity();
        if (a == null) return -1;

        PrintManager manager = (PrintManager) a.getSystemService(Context.PRINT_SERVICE);
        if (manager == null) return -1;

        PrintAttributes attributes = new PrintAttributes.Builder()
            .setMediaSize(landscape ? PrintAttributes.MediaSize.UNKNOWN_LANDSCAPE : PrintAttributes.MediaSize.UNKNOWN_PORTRAIT)
            .build();

        sDone[0] = false;
        sStatus = 0;
        sLandscape = landscape;

        try
        {
            manager.print(jobName, new Adapter(a, ihandlePtr, jobName), attributes);
        }
        catch (Exception e)
        {
            Log.e(TAG, "PrintManager.print failed: " + e);
            return -1;
        }

        IupCommon.pumpUntilDone(sDone);
        return sStatus;
    }

    private static boolean pageInRanges(PageRange[] ranges, int page)
    {
        for (PageRange range : ranges)
        {
            if (range.equals(PageRange.ALL_PAGES) || (page >= range.getStart() && page <= range.getEnd()))
                return true;
        }
        return false;
    }

    private static final class Adapter extends PrintDocumentAdapter
    {
        private final Activity activity;
        private final long ihandlePtr;
        private final String jobName;
        private PrintAttributes attributes;
        private int pageCount, width, height;

        Adapter(Activity activity, long ihandlePtr, String jobName)
        {
            this.activity = activity;
            this.ihandlePtr = ihandlePtr;
            this.jobName = jobName;
        }

        @Override
        public void onLayout(PrintAttributes oldAttributes, PrintAttributes newAttributes, CancellationSignal cancellationSignal,
                             LayoutResultCallback callback, Bundle extras)
        {
            if (cancellationSignal.isCanceled())
            {
                callback.onLayoutCancelled();
                return;
            }

            attributes = newAttributes;
            PrintAttributes.MediaSize media = newAttributes.getMediaSize();
            PrintAttributes.Margins margins = newAttributes.getMinMargins();
            if (media == null)
            {
                callback.onLayoutFailed("no media size");
                return;
            }

            int left = margins != null ? margins.getLeftMils() : 0, right = margins != null ? margins.getRightMils() : 0;
            int top = margins != null ? margins.getTopMils() : 0, bottom = margins != null ? margins.getBottomMils() : 0;
            width = Math.round((media.getWidthMils() - left - right) * 96.0f / 1000.0f);
            height = Math.round((media.getHeightMils() - top - bottom) * 96.0f / 1000.0f);
            sLandscape = !media.isPortrait();

            pageCount = nativePaginate(ihandlePtr, width, height);

            PrintDocumentInfo info = new PrintDocumentInfo.Builder(jobName)
                .setContentType(PrintDocumentInfo.CONTENT_TYPE_DOCUMENT)
                .setPageCount(pageCount)
                .build();
            callback.onLayoutFinished(info, true);
        }

        @Override
        public void onWrite(PageRange[] pages, ParcelFileDescriptor destination, CancellationSignal cancellationSignal,
                            WriteResultCallback callback)
        {
            PrintedPdfDocument document = new PrintedPdfDocument(activity, attributes);
            IupAndroidCanvas view = new IupAndroidCanvas(activity, 0);

            try
            {
                for (int index = 0; index < pageCount; index++)
                {
                    if (!pageInRanges(pages, index))
                        continue;

                    if (cancellationSignal.isCanceled())
                    {
                        callback.onWriteCancelled();
                        sStatus = 0;
                        return;
                    }

                    PdfDocument.Page page = document.startPage(index);
                    Canvas canvas = page.getCanvas();
                    Rect content = page.getInfo().getContentRect();
                    canvas.translate(content.left, content.top);
                    canvas.scale(PRINT_SCALE, PRINT_SCALE);

                    view.beginPrintPage(canvas);
                    int ret = nativeDrawPage(ihandlePtr, view, index + 1, width, height);
                    view.endPrintPage();
                    document.finishPage(page);

                    if (ret == IUP_CLOSE)
                    {
                        callback.onWriteCancelled();
                        sStatus = 0;
                        return;
                    }
                }

                try (FileOutputStream out = new FileOutputStream(destination.getFileDescriptor()))
                {
                    document.writeTo(out);
                }
                sStatus = 1;
                callback.onWriteFinished(pages);
            }
            catch (IOException e)
            {
                sStatus = -1;
                callback.onWriteFailed(e.toString());
            }
            finally
            {
                document.close();
            }
        }

        @Override
        public void onFinish()
        {
            sDone[0] = true;
        }
    }
}
