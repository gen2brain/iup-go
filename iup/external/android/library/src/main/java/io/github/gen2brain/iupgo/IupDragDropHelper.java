package io.github.gen2brain.iupgo;

import android.app.Activity;
import android.content.ClipData;
import android.content.ClipDescription;
import android.content.Context;
import android.database.Cursor;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Point;
import android.net.Uri;
import android.os.Build;
import android.provider.OpenableColumns;
import android.util.Log;
import android.view.DragAndDropPermissions;
import android.view.DragEvent;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ListView;
import androidx.annotation.Keep;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.WeakHashMap;


/* long-press starts an IUP drag carrying Payload in localState; DROPFILESTARGET stages URIs through cache */
public final class IupDragDropHelper
{
    private static final String TAG = "IupDragDrop";

    private static final int IUP_IGNORE = -1;

    private IupDragDropHelper() {}


    public static final class Payload
    {
        public final long sourceIhandlePtr;
        public final String[] types;
        public final boolean wantMove;
        public Payload(long sourceIhandlePtr, String[] types, boolean wantMove)
        {
            this.sourceIhandlePtr = sourceIhandlePtr;
            this.types = (types != null) ? types : new String[0];
            this.wantMove = wantMove;
        }
    }

    /* DRAGSOURCE / DROPTARGET / DROPFILESTARGET share a single listener. */
    private static final class Facade
    {
        long ih;
        boolean dragSource;
        boolean dropTarget;
        boolean dropFiles;
    }

    private static final WeakHashMap<View, Facade> sFacades = new WeakHashMap<>();

    private static Facade ensureFacade(View v, long ih)
    {
        Facade f = sFacades.get(v);
        if (f == null)
        {
            f = new Facade();
            sFacades.put(v, f);
        }
        f.ih = ih;
        return f;
    }


    /* IupDialog hands us the Activity; attach at its root fixed instead. */
    private static View resolveView(Object widget)
    {
        if (widget instanceof View v) return v;
        if (widget instanceof IupActivity a) return a.getRootFixed();
        return null;
    }

    @Keep
    public static void setDragSource(Object widget, final long ihandlePtr, boolean enable)
    {
        View v = resolveView(widget);
        if (v == null) return;
        Facade f = ensureFacade(v, ihandlePtr);
        f.dragSource = enable;
        syncListeners(v, f);
    }

    @Keep
    public static void setDropTarget(Object widget, final long ihandlePtr, boolean enable)
    {
        View v = resolveView(widget);
        if (v == null) return;
        Facade f = ensureFacade(v, ihandlePtr);
        f.dropTarget = enable;
        syncListeners(v, f);
    }

    @Keep
    public static void setDropFilesTarget(Object widget, final long ihandlePtr, boolean enable)
    {
        View v = resolveView(widget);
        if (v == null) return;
        Facade f = ensureFacade(v, ihandlePtr);
        f.dropFiles = enable;
        syncListeners(v, f);
    }

    private static void syncListeners(View v, Facade f)
    {
        installSourceListeners(v, f);
        installDropListener(v, f);
    }

    /* Hook for Tree (RecyclerView consumes touches before OnLongClickListener). */
    public static boolean tryStartGenericDrag(View dragView, View shadowView, long sourceIhandlePtr, int x, int y)
    {
        if (dragView == null) return false;
        Facade f = sFacades.get(dragView);
        if (f == null || !f.dragSource) return false;
        Payload p = requestDragPayload(sourceIhandlePtr, x, y);
        if (p == null) return false;
        startDrag(dragView, shadowView != null ? shadowView : dragView, p, sourceIhandlePtr);
        return true;
    }


    private static void installSourceListeners(final View v, final Facade f)
    {
        if (v instanceof AdapterView<?> av)
        {
            if (f.dragSource)
            {
                av.setOnItemLongClickListener((parent, itemView, position, id) -> {
                    if (parent instanceof ListView)
                        ((ListView)parent).setItemChecked(position, true);
                    /* Row centre, parent-relative; _IUP_XY2POS_CB undoes it. */
                    int x = itemView.getLeft() + itemView.getWidth() / 2;
                    int y = itemView.getTop()  + itemView.getHeight() / 2;
                    Payload p = requestDragPayload(f.ih, x, y);
                    if (p == null) return false;
                    startDrag(parent, itemView, p, f.ih);
                    return true;
                });
            }
            else
            {
                av.setOnItemLongClickListener(null);
            }
            return;
        }

        if (f.dragSource)
        {
            v.setOnLongClickListener(view -> {
                Payload p = requestDragPayload(f.ih, 0, 0);
                if (p == null) return false;
                startDrag(view, view, p, f.ih);
                return true;
            });
        }
        else
        {
            v.setOnLongClickListener(null);
        }
    }

    @SuppressWarnings("deprecation")
    private static void startDrag(View source, View shadowSource, Payload p, long sourceIh)
    {
        ClipData cd = buildPlaceholderClip(p);
        View.DragShadowBuilder shadow = buildShadow(source, shadowSource, sourceIh);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N)
            source.startDragAndDrop(cd, shadow, p, 0);
        else
            source.startDrag(cd, shadow, p, 0);
    }

    private static ClipData buildPlaceholderClip(Payload p)
    {
        String label = (p.types.length > 0 && p.types[0] != null) ? p.types[0] : "iup";
        return ClipData.newPlainText(label, label);
    }

    /* DRAGCURSOR overrides the default snapshot shadow with the IUP image. */
    private static View.DragShadowBuilder buildShadow(View source, View shadowSource, long sourceIh)
    {
        Bitmap cursor = requestDragCursor(sourceIh);
        if (cursor != null) return new BitmapShadow(cursor);
        return new View.DragShadowBuilder(shadowSource);
    }

    private static final class BitmapShadow extends View.DragShadowBuilder
    {
        private final Bitmap bmp;
        BitmapShadow(Bitmap bmp) { this.bmp = bmp; }
        @Override public void onProvideShadowMetrics(Point outShadowSize, Point outShadowTouchPoint)
        {
            outShadowSize.set(bmp.getWidth(), bmp.getHeight());
            outShadowTouchPoint.set(bmp.getWidth() / 2, bmp.getHeight() / 2);
        }
        @Override public void onDrawShadow(Canvas canvas) { canvas.drawBitmap(bmp, 0f, 0f, null); }
    }


    private static void installDropListener(final View v, final Facade f)
    {
        /* Tree and List own their OnDragListener and chain back via handleDragEvent. */
        if (v instanceof IupTreeHelper.IupTreeView) return;
        if (v instanceof IupListHelper.IupListView) return;
        boolean any = f.dragSource || f.dropTarget || f.dropFiles;
        if (!any)
        {
            v.setOnDragListener(null);
            return;
        }
        v.setOnDragListener((view, event) -> dispatchDragEvent(view, event, f));
    }

    /* Delegate for widgets that own their OnDragListener. */
    public static boolean handleDragEvent(View view, DragEvent event)
    {
        Facade f = sFacades.get(view);
        if (f == null) return false;
        return dispatchDragEvent(view, event, f);
    }

    private static boolean dispatchDragEvent(View view, DragEvent event, Facade f)
    {
        int action = event.getAction();

        /* ACTION_DRAG_ENDED is broadcast; only the source fires DRAGEND_CB. */
        if (action == DragEvent.ACTION_DRAG_ENDED)
        {
            if (event.getLocalState() instanceof Payload p && p.sourceIhandlePtr == f.ih)
            {
                int act = !event.getResult() ? -1 : (p.wantMove ? 1 : 0);
                dispatchDragEnd(p.sourceIhandlePtr, act);
            }
            return true;
        }

        if (!f.dropTarget && !f.dropFiles) return false;

        switch (action)
        {
            case DragEvent.ACTION_DRAG_STARTED:
                return canHandleStarted(event, f);

            case DragEvent.ACTION_DRAG_LOCATION:
                if (f.dropTarget && event.getLocalState() instanceof Payload)
                    deliverMotion(f.ih, (int)event.getX(), (int)event.getY());
                return true;

            case DragEvent.ACTION_DROP:
                return performDrop(view, event, f);
        }
        return false;
    }

    private static boolean canHandleStarted(DragEvent event, Facade f)
    {
        if (f.dropTarget && event.getLocalState() instanceof Payload p
                && firstMatchingType(p, f.ih) != null)
            return true;
        if (f.dropFiles && hasFileClip(event))
            return true;
        return false;
    }

    private static boolean performDrop(View view, DragEvent event, Facade f)
    {
        if (f.dropFiles && hasFileClip(event))
            return performFileDrop(view, event, f);
        if (f.dropTarget && event.getLocalState() instanceof Payload p)
            return performGenericDrop(p, event, f);
        return false;
    }

    private static boolean performGenericDrop(Payload p, DragEvent event, Facade f)
    {
        String type = firstMatchingType(p, f.ih);
        if (type == null) return false;
        byte[] data = requestDragData(p.sourceIhandlePtr, type);
        if (data == null) return false;
        deliverDrop(f.ih, type, data, (int)event.getX(), (int)event.getY());
        return true;
    }

    /* Stage each URI through cache and fire DROPFILES_CB; reverse-index order. */
    private static boolean performFileDrop(View view, DragEvent event, Facade f)
    {
        Activity act = IupActivity.currentActivity();
        DragAndDropPermissions perms = null;
        if (act != null)
        {
            try { perms = act.requestDragAndDropPermissions(event); }
            catch (Exception e) { Log.w(TAG, "requestDragAndDropPermissions failed: " + e); }
        }

        try
        {
            ClipData clip = event.getClipData();
            if (clip == null) return false;
            int total = clip.getItemCount();
            int x = (int)event.getX(), y = (int)event.getY();
            Context ctx = view.getContext();

            for (int i = 0; i < total; i++)
            {
                Uri uri = clip.getItemAt(i).getUri();
                if (uri == null) continue;
                String path = stageUriToCache(ctx, uri);
                if (path == null) continue;
                if (dispatchDropFile(f.ih, path, total - i - 1, x, y) == IUP_IGNORE)
                    break;
            }
            return true;
        }
        finally
        {
            if (perms != null) perms.release();
        }
    }


    private static boolean hasFileClip(DragEvent event)
    {
        ClipDescription d = event.getClipDescription();
        if (d == null) return false;
        for (int i = 0; i < d.getMimeTypeCount(); i++)
        {
            String mt = d.getMimeType(i);
            if (mt == null) continue;
            if (mt.equals(ClipDescription.MIMETYPE_TEXT_URILIST)) return true;
            if (mt.startsWith("image/") || mt.startsWith("video/") || mt.startsWith("audio/")
                    || mt.startsWith("application/")) return true;
        }
        return false;
    }

    /* First source-published type that target accepts (target order). */
    private static String firstMatchingType(Payload p, long targetIh)
    {
        for (String t : p.types)
        {
            if (t == null) continue;
            if (acceptsType(targetIh, t)) return t;
        }
        return null;
    }

    private static String stageUriToCache(Context ctx, Uri uri)
    {
        String name = queryDisplayName(ctx, uri);
        if (name == null || name.isEmpty()) name = "file";
        File cache = new File(ctx.getCacheDir(), "iup_drop_" + System.currentTimeMillis() + "_" + name);
        try (InputStream in = ctx.getContentResolver().openInputStream(uri);
             OutputStream out = new FileOutputStream(cache))
        {
            if (in == null) return null;
            byte[] buf = new byte[8192];
            int n;
            while ((n = in.read(buf)) > 0) out.write(buf, 0, n);
        }
        catch (Exception e)
        {
            Log.e(TAG, "stageUriToCache failed: " + e);
            return null;
        }
        return cache.getAbsolutePath();
    }

    private static String queryDisplayName(Context ctx, Uri uri)
    {
        try (Cursor c = ctx.getContentResolver().query(uri, null, null, null, null))
        {
            if (c != null && c.moveToFirst())
            {
                int col = c.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                if (col >= 0) return IupFileDlgHelper.safeLeaf(c.getString(col));
            }
        }
        catch (Exception ignored) {}
        return null;
    }


    public static native Payload requestDragPayload(long sourceIhandlePtr, int x, int y);
    public static native byte[] requestDragData(long sourceIhandlePtr, String type);
    public static native Bitmap requestDragCursor(long sourceIhandlePtr);
    public static native boolean acceptsType(long targetIhandlePtr, String type);
    public static native void deliverDrop(long targetIhandlePtr, String type, byte[] data, int x, int y);
    public static native void deliverMotion(long targetIhandlePtr, int x, int y);
    public static native void dispatchDragEnd(long sourceIhandlePtr, int action);
    public static native int dispatchDropFile(long ihandlePtr, String path, int remaining, int x, int y);
}
