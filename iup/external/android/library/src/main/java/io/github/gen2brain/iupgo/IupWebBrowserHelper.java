package io.github.gen2brain.iupgo;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Message;
import android.print.PrintAttributes;
import android.print.PrintDocumentAdapter;
import android.print.PrintManager;
import android.util.Base64;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.webkit.JavascriptInterface;
import android.webkit.WebBackForwardList;
import android.webkit.WebChromeClient;
import android.webkit.WebHistoryItem;
import android.webkit.WebResourceError;
import android.webkit.WebResourceRequest;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;

import androidx.annotation.Keep;
import androidx.appcompat.view.ContextThemeWrapper;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;


/* android.webkit.WebView + WebViewClient/WebChromeClient wiring. */
public final class IupWebBrowserHelper
{
    private static final String TAG = "Iup";

    public static final String STATUS_LOADING   = "LOADING";
    public static final String STATUS_COMPLETED = "COMPLETED";
    public static final String STATUS_FAILED    = "FAILED";

    /* Re-injected at every onPageFinished to survive navigation. */
    private static final String EDIT_BRIDGE_JS =
        "(function(){" +
        "  if (window.__iupEditInit) return;" +
        "  window.__iupEditInit = true;" +
        "  window.__iupDirty = false;" +
        "  window.__iupSavedRange = null;" +
        "  document.addEventListener('selectionchange', function() {" +
        "    if (document.body && document.body.isContentEditable) {" +
        "      var sel = window.getSelection();" +
        "      if (sel && sel.rangeCount > 0)" +
        "        window.__iupSavedRange = sel.getRangeAt(0).cloneRange();" +
        "      if (window.IupBridge) window.IupBridge.onEditUpdate();" +
        "    }" +
        "  });" +
        "  document.addEventListener('input', function() {" +
        "    if (document.body && document.body.isContentEditable && !window.__iupDirty) {" +
        "      window.__iupDirty = true;" +
        "      if (window.IupBridge) window.IupBridge.onEditUpdate();" +
        "    }" +
        "  });" +
        "  window.iupRestoreSelection = function() {" +
        "    if (window.__iupSavedRange) {" +
        "      var sel = window.getSelection();" +
        "      sel.removeAllRanges();" +
        "      try { sel.addRange(window.__iupSavedRange); } catch(e) {}" +
        "    }" +
        "  };" +
        "})();";

    private IupWebBrowserHelper() {}

    /* JS is required for IupWebBrowser: evalJS, edit-tracking IIFE, NEWWINDOW probe. */
    @Keep
    @android.annotation.SuppressLint("SetJavaScriptEnabled")
    public static IupWebView create(final long ihandlePtr)
    {
        ContextThemeWrapper ctx = IupCommon.getContextThemeWrapper();
        IupWebView wv = new IupWebView(ctx, ihandlePtr);
        WebSettings s = wv.getSettings();
        s.setJavaScriptEnabled(true);
        s.setDomStorageEnabled(true);
        s.setBuiltInZoomControls(false);
        s.setDisplayZoomControls(false);
        s.setSupportZoom(true);
        s.setJavaScriptCanOpenWindowsAutomatically(true);
        s.setSupportMultipleWindows(true);
        s.setAllowFileAccess(false);
        s.setAllowContentAccess(false);
        s.setMixedContentMode(WebSettings.MIXED_CONTENT_NEVER_ALLOW);
        s.setMediaPlaybackRequiresUserGesture(true);
        if (android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.R)
            applyLegacyFileUrlLockdown(s);
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O)
            s.setSafeBrowsingEnabled(true);
        wv.setWebViewClient(new IupWebViewClient(ihandlePtr));
        wv.setWebChromeClient(new IupWebChromeClient(ihandlePtr));
        wv.addJavascriptInterface(new IupBridge(ihandlePtr), "IupBridge");
        return wv;
    }

    /* default true on API < 30, default false + deprecated on API >= 30 (R) */
    @SuppressWarnings("deprecation")
    private static void applyLegacyFileUrlLockdown(WebSettings s)
    {
        s.setAllowFileAccessFromFileURLs(false);
        s.setAllowUniversalAccessFromFileURLs(false);
    }

    @Keep
    public static void loadUrl(View view, String url)
    {
        if (!(view instanceof IupWebView wv) || url == null) return;
        wv.setIgnoreNavigate(true);
        wv.setStatus(STATUS_LOADING);
        wv.loadUrl(url);
    }

    @Keep
    public static String getUrl(View view)
    {
        if (!(view instanceof IupWebView)) return null;
        return ((IupWebView)view).getUrl();
    }

    @Keep
    public static String getStatus(View view)
    {
        if (!(view instanceof IupWebView)) return STATUS_FAILED;
        return ((IupWebView)view).getStatus();
    }

    @Keep
    public static void reload(View view)
    {
        if (!(view instanceof IupWebView wv)) return;
        wv.setIgnoreNavigate(true);
        wv.setStatus(STATUS_LOADING);
        wv.reload();
    }

    @Keep
    public static void stopLoading(View view)
    {
        if (view instanceof IupWebView) ((IupWebView)view).stopLoading();
    }

    @Keep
    public static void goBack(View view)
    {
        if (!(view instanceof IupWebView wv)) return;
        if (wv.canGoBack())
        {
            wv.setIgnoreNavigate(true);
            wv.goBack();
        }
    }

    @Keep
    public static void goForward(View view)
    {
        if (!(view instanceof IupWebView wv)) return;
        if (wv.canGoForward())
        {
            wv.setIgnoreNavigate(true);
            wv.goForward();
        }
    }

    @Keep
    public static void goBackOrForward(View view, int steps)
    {
        if (!(view instanceof IupWebView wv)) return;
        if (wv.canGoBackOrForward(steps))
        {
            wv.setIgnoreNavigate(true);
            wv.goBackOrForward(steps);
        }
    }

    @Keep
    public static boolean canGoBack(View view)
    {
        return (view instanceof IupWebView) && ((IupWebView)view).canGoBack();
    }

    @Keep
    public static boolean canGoForward(View view)
    {
        return (view instanceof IupWebView) && ((IupWebView)view).canGoForward();
    }


    @Keep
    public static int backCount(View view)
    {
        if (!(view instanceof IupWebView)) return 0;
        WebBackForwardList list = ((IupWebView)view).copyBackForwardList();
        return list.getCurrentIndex();
    }

    @Keep
    public static int forwardCount(View view)
    {
        if (!(view instanceof IupWebView)) return 0;
        WebBackForwardList list = ((IupWebView)view).copyBackForwardList();
        return list.getSize() - list.getCurrentIndex() - 1;
    }

    @Keep
    public static String itemHistory(View view, int id)
    {
        if (!(view instanceof IupWebView)) return null;
        WebBackForwardList list = ((IupWebView)view).copyBackForwardList();
        int index = list.getCurrentIndex() + id;
        if (index < 0 || index >= list.getSize()) return null;
        WebHistoryItem item = list.getItemAtIndex(index);
        return item != null ? item.getUrl() : null;
    }


    @Keep
    public static void loadHtml(View view, String html)
    {
        if (!(view instanceof IupWebView wv) || html == null) return;
        wv.setIgnoreNavigate(true);
        wv.setStatus(STATUS_LOADING);
        /* Base "" so relative URLs resolve against about:blank, not assets/. */
        wv.loadDataWithBaseURL("", html, "text/html", "UTF-8", null);
    }


    @Keep
    public static void setZoom(View view, int percent)
    {
        if (!(view instanceof IupWebView)) return;
        ((IupWebView)view).getSettings().setTextZoom(percent);
    }

    @Keep
    public static int getZoom(View view)
    {
        if (!(view instanceof IupWebView)) return 100;
        return ((IupWebView)view).getSettings().getTextZoom();
    }


    @Keep
    public static void setEditable(View view, boolean editable)
    {
        if (!(view instanceof IupWebView wv)) return;
        wv.setEditable(editable);
        wv.evaluateJavascript(
            "(function(){if(document.body)document.body.contentEditable='"
                + (editable ? "true" : "false") + "';})();",
            null);
    }

    @Keep
    public static boolean getEditable(View view)
    {
        return (view instanceof IupWebView) && ((IupWebView)view).getEditable();
    }

    @Keep
    public static void newDoc(View view)
    {
        if (!(view instanceof IupWebView wv)) return;
        wv.setIgnoreNavigate(true);
        wv.loadDataWithBaseURL("", "<html><body></body></html>", "text/html", "UTF-8", null);
    }

    @Keep
    public static void openFile(View view, String path)
    {
        if (!(view instanceof IupWebView wv) || path == null) return;
        wv.setIgnoreNavigate(true);
        wv.loadUrl("file://" + path);
    }

    @Keep
    public static boolean saveFile(View view, String path)
    {
        if (!(view instanceof IupWebView wv) || path == null) return false;
        final String[] slot = new String[1];
        final boolean[] done = new boolean[1];
        wv.evaluateJavascript("document.documentElement.outerHTML", value -> { slot[0] = value; done[0] = true; });
        IupCommon.pumpUntilDone(done);
        String html = unquoteJsResult(slot[0]);
        if (html == null) return false;
        try (OutputStream out = new FileOutputStream(path))
        {
            out.write(html.getBytes(StandardCharsets.UTF_8));
            wv.evaluateJavascript("window.__iupDirty=false;", null);
            return true;
        }
        catch (IOException e)
        {
            Log.e(TAG, "WebBrowser saveFile failed: " + e);
            return false;
        }
    }

    @Keep
    public static String imageFileToDataUri(String path)
    {
        if (path == null) return null;
        String mime = "image/png";
        int dot = path.lastIndexOf('.');
        if (dot >= 0)
        {
            String ext = path.substring(dot + 1).toLowerCase();
            mime = switch (ext) {
                case "jpg", "jpeg" -> "image/jpeg";
                case "gif" -> "image/gif";
                case "webp" -> "image/webp";
                case "svg" -> "image/svg+xml";
                default -> mime;
            };
        }
        File f = new File(path);
        try (InputStream in = new FileInputStream(f);
             ByteArrayOutputStream bos = new ByteArrayOutputStream())
        {
            byte[] buf = new byte[8192];
            int n;
            while ((n = in.read(buf)) > 0) bos.write(buf, 0, n);
            return "data:" + mime + ";base64," + Base64.encodeToString(bos.toByteArray(), Base64.NO_WRAP);
        }
        catch (IOException e)
        {
            Log.e(TAG, "WebBrowser imageFileToDataUri failed: " + e);
            return null;
        }
    }

    /* Sync via nested Looper pump; IUP loop == UI thread on Android. */
    @Keep
    public static String evalJs(View view, String js)
    {
        if (!(view instanceof IupWebView wv) || js == null) return null;
        final String[] slot = new String[1];
        final boolean[] done = new boolean[1];
        wv.evaluateJavascript(js, value -> {
            slot[0] = value;
            done[0] = true;
        });
        IupCommon.pumpUntilDone(done);
        return unquoteJsResult(slot[0]);
    }

    @Keep
    public static void evalJsVoid(View view, String js)
    {
        if (!(view instanceof IupWebView) || js == null) return;
        ((IupWebView)view).evaluateJavascript(js, null);
    }

    private static String unquoteJsResult(String v)
    {
        if (v == null || "null".equals(v)) return null;
        int len = v.length();
        if (len >= 2 && v.charAt(0) == '"' && v.charAt(len - 1) == '"')
        {
            StringBuilder sb = new StringBuilder(len - 2);
            for (int i = 1; i < len - 1; i++)
            {
                char c = v.charAt(i);
                if (c == '\\' && i + 1 < len - 1)
                {
                    char n = v.charAt(++i);
                    switch (n)
                    {
                        case 'n':  sb.append('\n'); break;
                        case 'r':  sb.append('\r'); break;
                        case 't':  sb.append('\t'); break;
                        case 'b':  sb.append('\b'); break;
                        case 'f':  sb.append('\f'); break;
                        case '"':  sb.append('"'); break;
                        case '\\': sb.append('\\'); break;
                        case '/':  sb.append('/'); break;
                        case 'u':
                            if (i + 4 < len - 1)
                            {
                                try
                                {
                                    sb.append((char)Integer.parseInt(v.substring(i + 1, i + 5), 16));
                                    i += 4;
                                }
                                catch (NumberFormatException ex) { sb.append(n); }
                            }
                            break;
                        default:   sb.append(n); break;
                    }
                }
                else sb.append(c);
            }
            return sb.toString();
        }
        return v;
    }

    @Keep
    public static void find(View view, String text)
    {
        if (!(view instanceof IupWebView wv)) return;
        if (text == null || text.isEmpty())
            wv.clearMatches();
        else
            wv.findAllAsync(text);
    }

    /* PrintManager opens native preview; PRINTPREVIEW aliases here. */
    @Keep
    public static void print(View view, String jobName)
    {
        if (!(view instanceof IupWebView wv)) return;
        Context ctx = wv.getContext();
        Activity act = IupActivity.currentActivity();
        Context printCtx = act != null ? act : ctx;
        PrintManager pm = (PrintManager)printCtx.getSystemService(Context.PRINT_SERVICE);
        if (pm == null) return;
        String name = (jobName != null && !jobName.isEmpty()) ? jobName : "IupWebBrowser";
        PrintDocumentAdapter adapter = wv.createPrintDocumentAdapter(name);
        pm.print(name, adapter, new PrintAttributes.Builder().build());
    }

    /* dispatchNavigate returns int so shouldOverrideUrlLoading can obey IUP_IGNORE. */
    public static native int  dispatchNavigate(long ihandlePtr, String url);
    public static native void dispatchCompleted(long ihandlePtr, String url);
    public static native void dispatchError(long ihandlePtr, String url);
    public static native void dispatchNewWindow(long ihandlePtr, String url);
    public static native void dispatchUpdate(long ihandlePtr);

    /* @JavascriptInterface calls arrive on a worker thread; hop to UI for UPDATE_CB. */
    private record IupBridge(long ihandlePtr)
    {
        /* Called from injected JS via window.IupBridge.onEditUpdate(). */
        @JavascriptInterface
        @SuppressWarnings("unused")
        public void onEditUpdate()
        {
            final long ptr = ihandlePtr;
            new android.os.Handler(android.os.Looper.getMainLooper()).post(() -> dispatchUpdate(ptr));
        }
    }

    /* Programmatic-only; requires an Ihandle so the XML constructors are absent. */
    @android.annotation.SuppressLint("ViewConstructor")
    public static final class IupWebView extends WebView
    {
        private final long ihandlePtr;
        private String status = STATUS_COMPLETED;
        private boolean ignoreNavigate;
        private boolean editable;

        public IupWebView(ContextThemeWrapper ctx, long ihandlePtr)
        {
            super(ctx);
            this.ihandlePtr = ihandlePtr;
        }

        public long getIhandlePtr() { return ihandlePtr; }
        public String getStatus() { return status; }
        public void setStatus(String s) { this.status = s; }

        /* One-shot flag consumed by shouldOverrideUrlLoading. */
        public boolean consumeIgnoreNavigate()
        {
            boolean v = ignoreNavigate;
            ignoreNavigate = false;
            return v;
        }
        public void setIgnoreNavigate(boolean v) { this.ignoreNavigate = v; }

        public boolean getEditable() { return editable; }
        public void setEditable(boolean v) { this.editable = v; }

        /* claim touch so vertical drags reach the WebView, not the wrapping NestedScrollView */
        @Override
        @android.annotation.SuppressLint("ClickableViewAccessibility")
        public boolean onTouchEvent(MotionEvent ev)
        {
            if (ev.getActionMasked() == MotionEvent.ACTION_DOWN && getParent() != null)
                getParent().requestDisallowInterceptTouchEvent(true);
            return super.onTouchEvent(ev);
        }

        /* Accessibility hook; real clicks are handled by super.onTouchEvent. */
        @Override public boolean performClick() { return super.performClick(); }
    }

    private static final class IupWebViewClient extends WebViewClient
    {
        private final long ihandlePtr;

        IupWebViewClient(long ihandlePtr) { this.ihandlePtr = ihandlePtr; }

        @Override
        public boolean shouldOverrideUrlLoading(WebView view, WebResourceRequest req)
        {
            Uri uri = (req != null) ? req.getUrl() : null;
            return dispatchOverride(view, uri != null ? uri.toString() : "");
        }

        @Override
        @SuppressWarnings("deprecation")
        public boolean shouldOverrideUrlLoading(WebView view, String url)
        {
            return dispatchOverride(view, url != null ? url : "");
        }

        private boolean dispatchOverride(WebView view, String url)
        {
            if (!(view instanceof IupWebView wv)) return false;
            if (wv.consumeIgnoreNavigate()) return false;
            return dispatchNavigate(ihandlePtr, url) == -1;
        }

        @Override
        public void onPageStarted(WebView view, String url, Bitmap favicon)
        {
            if (view instanceof IupWebView) ((IupWebView)view).setStatus(STATUS_LOADING);
        }

        @Override
        public void onPageFinished(WebView view, String url)
        {
            if (view instanceof IupWebView wv)
            {
                wv.setStatus(STATUS_COMPLETED);
                wv.evaluateJavascript(EDIT_BRIDGE_JS, null);
                if (wv.getEditable())
                    wv.evaluateJavascript("document.body&&(document.body.contentEditable='true');", null);
            }
            dispatchCompleted(ihandlePtr, url != null ? url : "");
        }

        @Override
        public void onReceivedError(WebView view, WebResourceRequest req, WebResourceError err)
        {
            if (req == null || !req.isForMainFrame()) return;
            if (view instanceof IupWebView) ((IupWebView)view).setStatus(STATUS_FAILED);
            Uri uri = req.getUrl();
            dispatchError(ihandlePtr, uri != null ? uri.toString() : "");
        }

        @Override
        @SuppressWarnings("deprecation")
        public void onReceivedError(WebView view, int errorCode, String description, String failingUrl)
        {
            if (view instanceof IupWebView) ((IupWebView)view).setStatus(STATUS_FAILED);
            dispatchError(ihandlePtr, failingUrl != null ? failingUrl : "");
        }
    }

    /* onCreateWindow gets a Message, not a URL; sniff via a probe WebView. */
    private static final class IupWebChromeClient extends WebChromeClient
    {
        private final long ihandlePtr;

        IupWebChromeClient(long ihandlePtr) { this.ihandlePtr = ihandlePtr; }

        @Override
        @android.annotation.SuppressLint("SetJavaScriptEnabled")
        public boolean onCreateWindow(WebView view, boolean isDialog, boolean isUserGesture, Message resultMsg)
        {
            Context ctx = view.getContext();
            /* Probe mirrors the parent's JS flag so target URL resolution matches. */
            WebView probe = new WebView(ctx);
            WebSettings ps = probe.getSettings();
            ps.setJavaScriptEnabled(true);
            ps.setAllowFileAccess(false);
            ps.setAllowContentAccess(false);
            ps.setMixedContentMode(WebSettings.MIXED_CONTENT_NEVER_ALLOW);
            if (android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.R)
                applyLegacyFileUrlLockdown(ps);
            probe.setWebViewClient(new WebViewClient()
            {
                @Override
                public boolean shouldOverrideUrlLoading(WebView v, WebResourceRequest req)
                {
                    Uri uri = req != null ? req.getUrl() : null;
                    dispatchNewWindow(ihandlePtr, uri != null ? uri.toString() : "");
                    v.destroy();
                    return true;
                }

                @Override
                @SuppressWarnings("deprecation")
                public boolean shouldOverrideUrlLoading(WebView v, String url)
                {
                    dispatchNewWindow(ihandlePtr, url != null ? url : "");
                    v.destroy();
                    return true;
                }
            });
            WebView.WebViewTransport transport = (WebView.WebViewTransport)resultMsg.obj;
            transport.setWebView(probe);
            resultMsg.sendToTarget();
            return true;
        }
    }
}
