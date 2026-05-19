/** \file
 * \brief Haiku WebKit Web Browser Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <Application.h>
#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <Handler.h>
#include <Looper.h>
#include <Message.h>
#include <Messenger.h>
#include <Path.h>
#include <String.h>
#include <View.h>
#include <Window.h>

#include <WebPage.h>
#include <WebSettings.h>
#include <WebView.h>
#include <WebViewConstants.h>

#include <JavaScriptCore/JSStringRef.h>
#include <JavaScriptCore/JSValueRef.h>

class __attribute__ ((visibility ("default"))) BWebFrame {
public:
  bool CanPaste() const;
  void Copy();
  void Cut();
  void Paste();
  void Undo();
  void Redo();
  BString AsMarkup() const;
  JSGlobalContextRef GlobalContext() const;
};

extern "C" {
#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_classbase.h"
#include "iup_str.h"
#include "iup_drvfont.h"
#include "iup_webbrowser.h"
}

#include "iuphaiku_drv.h"


#define IUPHAIKU_JS_EXEC_MSG 'IuJX'  /* sync JS eval hop to the WebPage looper */
#define IUPHAIKU_WEB_MAP_MSG 'IuWM'  /* deferred bind of BWebView once the dialog window is up */

typedef enum {
  IUP_WEB_LOAD_LOADING,
  IUP_WEB_LOAD_COMPLETED,
  IUP_WEB_LOAD_FAILED
} IupHaikuWebLoadStatus;

struct _IcontrolData
{
  int sb;
};


class IupHaikuWebHandler : public BHandler
{
public:
  explicit IupHaikuWebHandler(Ihandle* ih)
    : BHandler("iup_webhandler"), fIhandle(ih), fStatus(IUP_WEB_LOAD_COMPLETED),
      fCanBack(false), fCanForward(false)
  {
  }

  void SetIhandle(Ihandle* ih) { fIhandle = ih; }
  IupHaikuWebLoadStatus Status() const { return fStatus; }
  bool CanGoBack() const { return fCanBack; }
  bool CanGoForward() const { return fCanForward; }

  void MessageReceived(BMessage* msg) override;

private:
  Ihandle* fIhandle;
  IupHaikuWebLoadStatus fStatus;
  bool fCanBack;
  bool fCanForward;
};

static bool sWebKitInitialized = false;

static void iuphaikuWebBrowserInit()
{
  sWebKitInitialized = true;

  BWebPage::InitializeOnce();
  BWebPage::SetCacheModel(B_WEBKIT_CACHE_MODEL_WEB_BROWSER);

  BPath path;
  if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK
      && path.Append("iup-webkit") == B_OK
      && create_directory(path.Path(), 0777) == B_OK)
  {
    BWebSettings::SetPersistentStoragePath(path.Path());

    BPath cookies(path);
    if (cookies.Append("cookie.jar.db") == B_OK)
      setenv("CURL_COOKIE_JAR_PATH", cookies.Path(), 0);
  }
}

static void haikuWebBrowserBindView(Ihandle* ih, BWebView* view)
{
  ih->handle = (InativeHandle*)view;

  IupHaikuWebHandler* handler = new IupHaikuWebHandler(ih);
  BWindow* win = view->Window();
  {
    LooperLockGuard guard(win);
    if (win) win->AddHandler(handler);
  }
  iupAttribSet(ih, "_IUPWEB_HANDLER", (char*)handler);
  view->WebPage()->SetListener(BMessenger(handler));

  iupAttribSet(ih, "_IUPWEB_READY", "1");
  iupAttribUpdate(ih);
}

static IupHaikuWebHandler* haikuWebBrowserHandler(Ihandle* ih)
{
  return (IupHaikuWebHandler*)iupAttribGet(ih, "_IUPWEB_HANDLER");
}

static BWebView* haikuWebBrowserView(Ihandle* ih)
{
  if (!iupAttribGet(ih, "_IUPWEB_READY")) return NULL;
  return (BWebView*)ih->handle;
}

static BWebFrame* haikuWebBrowserFrame(Ihandle* ih)
{
  BWebView* view = haikuWebBrowserView(ih);
  if (!view) return NULL;
  BWebPage* page = view->WebPage();
  if (!page) return NULL;
  return page->MainFrame();
}

class IupHaikuJSExecHandler : public BHandler
{
public:
  IupHaikuJSExecHandler() : BHandler("iup_web_jsexec") {}

  void MessageReceived(BMessage* msg) override
  {
    if (msg->what != IUPHAIKU_JS_EXEC_MSG) { BHandler::MessageReceived(msg); return; }

    void* ctx_ptr = NULL;
    const char* script = NULL;
    msg->FindPointer("ctx", &ctx_ptr);
    msg->FindString("script", &script);

    BMessage reply(IUPHAIKU_JS_EXEC_MSG);
    JSGlobalContextRef ctx = (JSGlobalContextRef)ctx_ptr;
    if (ctx && script)
    {
      JSStringRef js_script = JSStringCreateWithUTF8CString(script);
      JSValueRef exception = NULL;
      JSValueRef result = JSEvaluateScript(ctx, js_script, NULL, NULL, 1, &exception);
      JSStringRelease(js_script);

      if (!exception && result
          && !JSValueIsUndefined(ctx, result) && !JSValueIsNull(ctx, result))
      {
        JSStringRef str_ref = JSValueToStringCopy(ctx, result, NULL);
        if (str_ref)
        {
          size_t maxlen = JSStringGetMaximumUTF8CStringSize(str_ref);
          char* buf = (char*)malloc(maxlen);
          JSStringGetUTF8CString(str_ref, buf, maxlen);
          JSStringRelease(str_ref);
          reply.AddString("result", buf);
          free(buf);
        }
      }
    }
    msg->SendReply(&reply);
  }
};

static IupHaikuJSExecHandler* sJSExecHandler = NULL;

static BMessenger haikuWebBrowserJSExecMessenger(BWebPage* page)
{
  if (!sJSExecHandler)
  {
    BHandler* page_handler = reinterpret_cast<BHandler*>(page);
    BLooper* page_looper = page_handler->Looper();
    if (!page_looper) return BMessenger();

    sJSExecHandler = new IupHaikuJSExecHandler();
    LooperLockGuard guard(page_looper);
    page_looper->AddHandler(sJSExecHandler);
  }
  return BMessenger(sJSExecHandler);
}

static char* haikuWebBrowserRunJSSync(Ihandle* ih, const char* script)
{
  BWebView* view = haikuWebBrowserView(ih);
  if (!view || !script) return NULL;

  BWebPage* page = view->WebPage();
  if (!page) return NULL;
  BWebFrame* frame = page->MainFrame();
  if (!frame) return NULL;
  JSGlobalContextRef ctx = frame->GlobalContext();
  if (!ctx) return NULL;

  BMessenger msgr = haikuWebBrowserJSExecMessenger(page);
  if (!msgr.IsValid()) return NULL;

  BMessage msg(IUPHAIKU_JS_EXEC_MSG);
  msg.AddPointer("ctx", ctx);
  msg.AddString("script", script);
  BMessage reply;
  if (msgr.SendMessage(&msg, &reply) != B_OK) return NULL;

  const char* result = NULL;
  if (reply.FindString("result", &result) != B_OK || !result) return NULL;
  return iupStrReturnStr(result);
}

static void haikuWebBrowserRunJSAsync(Ihandle* ih, const char* script)
{
  BWebView* view = haikuWebBrowserView(ih);
  if (!view || !script) return;

  BWebPage* page = view->WebPage();
  if (!page) return;
  BWebFrame* frame = page->MainFrame();
  if (!frame) return;
  JSGlobalContextRef ctx = frame->GlobalContext();
  if (!ctx) return;

  BMessenger msgr = haikuWebBrowserJSExecMessenger(page);
  if (!msgr.IsValid()) return;

  BMessage msg(IUPHAIKU_JS_EXEC_MSG);
  msg.AddPointer("ctx", ctx);
  msg.AddString("script", script);
  msgr.SendMessage(&msg);
}

static void haikuWebBrowserJSEscape(BString& out, const char* s)
{
  out << '"';
  for (const char* p = s; *p; ++p)
  {
    unsigned char c = (unsigned char)*p;
    switch (c)
    {
      case '"':  out << "\\\""; break;
      case '\\': out << "\\\\"; break;
      case '\n': out << "\\n";  break;
      case '\r': out << "\\r";  break;
      case '\t': out << "\\t";  break;
      case '\b': out << "\\b";  break;
      case '\f': out << "\\f";  break;
      default:
        if (c < 0x20)
        {
          char esc[8];
          snprintf(esc, sizeof(esc), "\\u%04x", c);
          out << esc;
        }
        else
        {
          out << *p;
        }
    }
  }
  out << '"';
}

static char* haikuWebBrowserJSQueryCommand(Ihandle* ih, const char* fn, const char* command)
{
  if (!command) return NULL;
  BString js;
  js << "document." << fn << "(";
  haikuWebBrowserJSEscape(js, command);
  js << ");";
  return haikuWebBrowserRunJSSync(ih, js.String());
}

static void haikuWebBrowserJSExecCommand(Ihandle* ih, const char* command, const char* param)
{
  BWebView* view = haikuWebBrowserView(ih);
  if (view)
  {
    LooperLockGuard guard(view->Looper());
    view->MakeFocus(true);
  }

  BString js("document.body.focus(); document.execCommand(");
  haikuWebBrowserJSEscape(js, command);
  js << ", false, ";
  if (param) haikuWebBrowserJSEscape(js, param);
  else       js << "null";
  js << ");";
  haikuWebBrowserRunJSAsync(ih, js.String());
}

void IupHaikuWebHandler::MessageReceived(BMessage* msg)
{
  if (!fIhandle || !iupObjectCheck(fIhandle))
  {
    BHandler::MessageReceived(msg);
    return;
  }

  switch (msg->what)
  {
    case LOAD_NEGOTIATING:
    case LOAD_STARTED:
    {
      fStatus = IUP_WEB_LOAD_LOADING;
      BString url;
      msg->FindString("url", &url);
      if (!iupAttribGet(fIhandle, "_IUPWEB_IGNORE_NAVIGATE"))
      {
        IFns cb = (IFns)IupGetCallback(fIhandle, "NAVIGATE_CB");
        if (cb) cb(fIhandle, (char*)url.String());
      }
      break;
    }
    case LOAD_FAILED:
    {
      fStatus = IUP_WEB_LOAD_FAILED;
      BString url;
      msg->FindString("url", &url);
      IFns cb = (IFns)IupGetCallback(fIhandle, "ERROR_CB");
      if (cb) cb(fIhandle, (char*)url.String());
      break;
    }
    case LOAD_FINISHED:
    {
      fStatus = IUP_WEB_LOAD_COMPLETED;
      BString url;
      msg->FindString("url", &url);
      if (iupAttribGet(fIhandle, "_IUPWEB_EDITABLE"))
        haikuWebBrowserRunJSAsync(fIhandle, "document.body.contentEditable = 'true';");
      IFns cb = (IFns)IupGetCallback(fIhandle, "COMPLETED_CB");
      if (cb) cb(fIhandle, (char*)url.String());
      break;
    }
    case NEW_WINDOW_REQUESTED:
    {
      BString url;
      msg->FindString("url", &url);
      IFns cb = (IFns)IupGetCallback(fIhandle, "NEWWINDOW_CB");
      if (cb) cb(fIhandle, (char*)url.String());
      break;
    }
    case UPDATE_NAVIGATION_INTERFACE:
    {
      bool canBack = false, canForward = false;
      msg->FindBool("can go backward", &canBack);
      msg->FindBool("can go forward", &canForward);
      fCanBack = canBack;
      fCanForward = canForward;
      break;
    }
    default:
      BHandler::MessageReceived(msg);
      break;
  }
}

static int haikuWebBrowserSetValueAttrib(Ihandle* ih, const char* value)
{
  if (!value) return 0;
  BWebView* view = haikuWebBrowserView(ih);
  if (!view) return 1;

  iupAttribSet(ih, "_IUPWEB_DIRTY", NULL);

  BString url(value);
  if (!iupStrEqualPartial(value, "http://") && !iupStrEqualPartial(value, "https://")
      && !iupStrEqualPartial(value, "ftp://") && !iupStrEqualPartial(value, "file://")
      && !iupStrEqualPartial(value, "about:") && !iupStrEqualPartial(value, "data:"))
  {
    char* file_url = iupStrFileMakeURL(value);
    url = file_url;
    free(file_url);
  }

  LooperLockGuard guard(view->Looper());
  iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", "1");
  view->LoadURL(url.String());
  iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", NULL);
  return 0;
}

static char* haikuWebBrowserGetValueAttrib(Ihandle* ih)
{
  BWebView* view = haikuWebBrowserView(ih);
  if (!view) return NULL;
  LooperLockGuard guard(view->Looper());
  BString url = view->MainFrameURL();
  if (url.Length() == 0) return NULL;
  return iupStrReturnStr(url.String());
}

static void haikuWebBrowserBase64Encode(BString& out, const char* in, size_t len)
{
  static const char kTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  size_t i = 0;
  while (i + 3 <= len)
  {
    unsigned a = (unsigned char)in[i], b = (unsigned char)in[i + 1], c = (unsigned char)in[i + 2];
    out << kTable[a >> 2] << kTable[((a & 3) << 4) | (b >> 4)]
        << kTable[((b & 15) << 2) | (c >> 6)] << kTable[c & 63];
    i += 3;
  }
  if (i < len)
  {
    unsigned a = (unsigned char)in[i], b = (i + 1 < len) ? (unsigned char)in[i + 1] : 0;
    out << kTable[a >> 2] << kTable[((a & 3) << 4) | (b >> 4)];
    out << ((i + 1 < len) ? kTable[(b & 15) << 2] : '=');
    out << '=';
  }
}

static int haikuWebBrowserSetHTMLAttrib(Ihandle* ih, const char* value)
{
  if (!value) return 0;
  BWebView* view = haikuWebBrowserView(ih);
  if (!view) return 1;

  BString url("data:text/html;charset=utf-8;base64,");
  haikuWebBrowserBase64Encode(url, value, strlen(value));

  LooperLockGuard guard(view->Looper());
  iupAttribSet(ih, "_IUPWEB_DIRTY", NULL);
  iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", "1");
  view->LoadURL(url.String());
  iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", NULL);
  return 0;
}

static char* haikuWebBrowserGetHTMLAttrib(Ihandle* ih)
{
  BWebFrame* frame = haikuWebBrowserFrame(ih);
  if (!frame) return NULL;

  BWebView* view = haikuWebBrowserView(ih);
  LooperLockGuard guard(view->Looper());
  BString markup = frame->AsMarkup();
  if (markup.Length() == 0) return NULL;
  return iupStrReturnStr(markup.String());
}

static int haikuWebBrowserSetReloadAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  BWebView* view = haikuWebBrowserView(ih);
  if (!view) return 1;
  LooperLockGuard guard(view->Looper());
  view->Reload();
  return 0;
}

static int haikuWebBrowserSetStopAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  BWebView* view = haikuWebBrowserView(ih);
  if (!view) return 1;
  LooperLockGuard guard(view->Looper());
  view->StopLoading();
  return 0;
}

static int haikuWebBrowserSetGoBackAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  BWebView* view = haikuWebBrowserView(ih);
  if (!view) return 1;
  LooperLockGuard guard(view->Looper());
  view->GoBack();
  return 0;
}

static int haikuWebBrowserSetGoForwardAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  BWebView* view = haikuWebBrowserView(ih);
  if (!view) return 1;
  LooperLockGuard guard(view->Looper());
  view->GoForward();
  return 0;
}

static int haikuWebBrowserSetBackForwardAttrib(Ihandle* ih, const char* value)
{
  int n;
  if (!iupStrToInt(value, &n) || n == 0) return 0;
  BWebView* view = haikuWebBrowserView(ih);
  if (!view) return 1;

  LooperLockGuard guard(view->Looper());
  if (n < 0) for (int i = 0; i < -n; ++i) view->GoBack();
  else       for (int i = 0; i < n;  ++i) view->GoForward();
  return 0;
}

static char* haikuWebBrowserGetCanGoBackAttrib(Ihandle* ih)
{
  IupHaikuWebHandler* h = haikuWebBrowserHandler(ih);
  return iupStrReturnBoolean(h && h->CanGoBack());
}

static char* haikuWebBrowserGetCanGoForwardAttrib(Ihandle* ih)
{
  IupHaikuWebHandler* h = haikuWebBrowserHandler(ih);
  return iupStrReturnBoolean(h && h->CanGoForward());
}

static char* haikuWebBrowserGetStatusAttrib(Ihandle* ih)
{
  IupHaikuWebHandler* h = haikuWebBrowserHandler(ih);
  if (!h) return (char*)"COMPLETED";
  switch (h->Status())
  {
    case IUP_WEB_LOAD_LOADING:   return (char*)"LOADING";
    case IUP_WEB_LOAD_FAILED:    return (char*)"FAILED";
    case IUP_WEB_LOAD_COMPLETED: return (char*)"COMPLETED";
  }
  return (char*)"COMPLETED";
}

static int haikuWebBrowserSetZoomAttrib(Ihandle* ih, const char* value)
{
  int target;
  if (!iupStrToInt(value, &target)) return 0;
  if (target < 25) target = 25;
  if (target > 500) target = 500;

  BWebView* view = haikuWebBrowserView(ih);
  if (!view) return 1;
  LooperLockGuard guard(view->Looper());

  view->ResetZoomFactor();
  int current = 100;
  while (current < target) { view->IncreaseZoomFactor(false); current += 10; }
  while (current > target) { view->DecreaseZoomFactor(false); current -= 10; }

  iupAttribSetInt(ih, "_IUPWEB_ZOOM", current);
  return 0;
}

static char* haikuWebBrowserGetZoomAttrib(Ihandle* ih)
{
  int zoom = iupAttribGetInt(ih, "_IUPWEB_ZOOM");
  if (zoom == 0) zoom = 100;
  return iupStrReturnInt(zoom);
}

static int haikuWebBrowserSetFindAttrib(Ihandle* ih, const char* value)
{
  if (!value) return 0;
  BWebView* view = haikuWebBrowserView(ih);
  if (!view) return 1;
  LooperLockGuard guard(view->Looper());
  view->FindString(value, true, false, true, false);
  return 0;
}

static int haikuWebBrowserSetCopyAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  BWebFrame* frame = haikuWebBrowserFrame(ih);
  if (!frame) return 0;
  BWebView* view = haikuWebBrowserView(ih);
  LooperLockGuard guard(view->Looper());
  frame->Copy();
  return 0;
}

static int haikuWebBrowserSetCutAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  BWebFrame* frame = haikuWebBrowserFrame(ih);
  if (!frame) return 0;
  BWebView* view = haikuWebBrowserView(ih);
  LooperLockGuard guard(view->Looper());
  frame->Cut();
  return 0;
}

static int haikuWebBrowserSetPasteAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  BWebFrame* frame = haikuWebBrowserFrame(ih);
  if (!frame) return 0;
  BWebView* view = haikuWebBrowserView(ih);
  LooperLockGuard guard(view->Looper());
  frame->Paste();
  return 0;
}

static char* haikuWebBrowserGetPasteAttrib(Ihandle* ih)
{
  BWebFrame* frame = haikuWebBrowserFrame(ih);
  if (!frame) return iupStrReturnBoolean(0);
  BWebView* view = haikuWebBrowserView(ih);
  LooperLockGuard guard(view->Looper());
  return iupStrReturnBoolean(frame->CanPaste());
}

static int haikuWebBrowserSetUndoAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  BWebFrame* frame = haikuWebBrowserFrame(ih);
  if (!frame) return 0;
  BWebView* view = haikuWebBrowserView(ih);
  LooperLockGuard guard(view->Looper());
  frame->Undo();
  return 0;
}

static int haikuWebBrowserSetRedoAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  BWebFrame* frame = haikuWebBrowserFrame(ih);
  if (!frame) return 0;
  BWebView* view = haikuWebBrowserView(ih);
  LooperLockGuard guard(view->Looper());
  frame->Redo();
  return 0;
}

static int haikuWebBrowserSetSelectAllAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  haikuWebBrowserJSExecCommand(ih, "selectAll", NULL);
  return 0;
}

static int haikuWebBrowserSetEditableAttrib(Ihandle* ih, const char* value)
{
  bool editable = iupStrBoolean(value);
  iupAttribSet(ih, "_IUPWEB_EDITABLE", editable ? "1" : NULL);

  if (!haikuWebBrowserView(ih)) return 1;

  haikuWebBrowserRunJSAsync(ih,
    editable ? "document.body.contentEditable = 'true';"
             : "document.body.contentEditable = 'false';");
  return 0;
}

static char* haikuWebBrowserGetEditableAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(iupAttribGet(ih, "_IUPWEB_EDITABLE") != NULL);
}

static int haikuWebBrowserSetNewAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  iupAttribSet(ih, "_IUPWEB_DIRTY", NULL);
  haikuWebBrowserSetHTMLAttrib(ih, "<html><body></body></html>");
  haikuWebBrowserSetEditableAttrib(ih, "Yes");
  return 0;
}

static int haikuWebBrowserSetOpenFileAttrib(Ihandle* ih, const char* value)
{
  if (!value) return 0;
  char* url = iupStrFileMakeURL(value);
  if (!url) return 0;
  iupAttribSet(ih, "_IUPWEB_DIRTY", NULL);
  haikuWebBrowserSetValueAttrib(ih, url);
  free(url);
  return 0;
}

static int haikuWebBrowserSetSaveFileAttrib(Ihandle* ih, const char* value)
{
  if (!value) return 0;
  char* html = haikuWebBrowserGetHTMLAttrib(ih);
  if (!html) return 0;

  BFile file(value, B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);
  if (file.InitCheck() != B_OK) return 0;
  file.Write(html, strlen(html));
  return 0;
}

static int haikuWebBrowserSetExecCommandAttrib(Ihandle* ih, const char* value)
{
  if (value) haikuWebBrowserJSExecCommand(ih, value, NULL);
  return 0;
}

static int haikuWebBrowserSetInsertImageAttrib(Ihandle* ih, const char* value)
{
  if (value) haikuWebBrowserJSExecCommand(ih, "insertImage", value);
  return 0;
}

static int haikuWebBrowserSetCreateLinkAttrib(Ihandle* ih, const char* value)
{
  if (value) haikuWebBrowserJSExecCommand(ih, "createLink", value);
  return 0;
}

static int haikuWebBrowserSetInsertTextAttrib(Ihandle* ih, const char* value)
{
  if (value) haikuWebBrowserJSExecCommand(ih, "insertText", value);
  return 0;
}

static int haikuWebBrowserSetInsertHtmlAttrib(Ihandle* ih, const char* value)
{
  if (value) haikuWebBrowserJSExecCommand(ih, "insertHTML", value);
  return 0;
}

static int haikuWebBrowserSetFontNameAttrib(Ihandle* ih, const char* value)
{
  if (value) haikuWebBrowserJSExecCommand(ih, "fontName", value);
  return 0;
}

static int haikuWebBrowserSetFontSizeAttrib(Ihandle* ih, const char* value)
{
  if (value) haikuWebBrowserJSExecCommand(ih, "fontSize", value);
  return 0;
}

static int haikuWebBrowserSetFormatBlockAttrib(Ihandle* ih, const char* value)
{
  if (value) haikuWebBrowserJSExecCommand(ih, "formatBlock", value);
  return 0;
}

static int haikuWebBrowserSetForeColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b)) return 0;
  char rgb[32];
  snprintf(rgb, sizeof(rgb), "rgb(%d,%d,%d)", r, g, b);
  haikuWebBrowserJSExecCommand(ih, "foreColor", rgb);
  return 0;
}

static int haikuWebBrowserSetBackColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b)) return 0;
  char rgb[32];
  snprintf(rgb, sizeof(rgb), "rgb(%d,%d,%d)", r, g, b);
  haikuWebBrowserJSExecCommand(ih, "backColor", rgb);
  return 0;
}

static char* haikuWebBrowserGetFontNameAttrib(Ihandle* ih)
{
  return haikuWebBrowserJSQueryCommand(ih, "queryCommandValue", "fontName");
}

static char* haikuWebBrowserGetFontSizeAttrib(Ihandle* ih)
{
  return haikuWebBrowserJSQueryCommand(ih, "queryCommandValue", "fontSize");
}

static char* haikuWebBrowserGetFormatBlockAttrib(Ihandle* ih)
{
  return haikuWebBrowserJSQueryCommand(ih, "queryCommandValue", "formatBlock");
}

static char* haikuWebBrowserGetForeColorAttrib(Ihandle* ih)
{
  return haikuWebBrowserJSQueryCommand(ih, "queryCommandValue", "foreColor");
}

static char* haikuWebBrowserGetBackColorAttrib(Ihandle* ih)
{
  return haikuWebBrowserJSQueryCommand(ih, "queryCommandValue", "backColor");
}

static char* haikuWebBrowserGetCommandStateAttrib(Ihandle* ih)
{
  return haikuWebBrowserJSQueryCommand(ih, "queryCommandState", iupAttribGet(ih, "COMMAND"));
}

static char* haikuWebBrowserGetCommandEnabledAttrib(Ihandle* ih)
{
  return haikuWebBrowserJSQueryCommand(ih, "queryCommandEnabled", iupAttribGet(ih, "COMMAND"));
}

static char* haikuWebBrowserGetCommandValueAttrib(Ihandle* ih)
{
  return haikuWebBrowserJSQueryCommand(ih, "queryCommandValue", iupAttribGet(ih, "COMMAND"));
}

static int haikuWebBrowserSetInnerTextAttrib(Ihandle* ih, const char* value)
{
  char* elem = iupAttribGet(ih, "ELEMENT_ID");
  if (!elem || !value) return 0;

  BString js("(function(){var e=document.getElementById(");
  haikuWebBrowserJSEscape(js, elem);
  js << "); if(e) e.innerText=";
  haikuWebBrowserJSEscape(js, value);
  js << ";})();";
  haikuWebBrowserRunJSSync(ih, js.String());
  return 0;
}

static char* haikuWebBrowserGetInnerTextAttrib(Ihandle* ih)
{
  char* elem = iupAttribGet(ih, "ELEMENT_ID");
  if (!elem) return NULL;
  BString js("(function(){var e=document.getElementById(");
  haikuWebBrowserJSEscape(js, elem);
  js << "); return e ? e.innerText : null;})()";
  return haikuWebBrowserRunJSSync(ih, js.String());
}

static int haikuWebBrowserSetAttributeAttrib(Ihandle* ih, const char* value)
{
  char* elem = iupAttribGet(ih, "ELEMENT_ID");
  char* name = iupAttribGet(ih, "ATTRIBUTE_NAME");
  if (!elem || !name || !value) return 0;
  BString js("(function(){var e=document.getElementById(");
  haikuWebBrowserJSEscape(js, elem);
  js << "); if(e) e.setAttribute(";
  haikuWebBrowserJSEscape(js, name);
  js << ",";
  haikuWebBrowserJSEscape(js, value);
  js << ");})();";
  haikuWebBrowserRunJSSync(ih, js.String());
  return 0;
}

static char* haikuWebBrowserGetAttributeAttrib(Ihandle* ih)
{
  char* elem = iupAttribGet(ih, "ELEMENT_ID");
  char* name = iupAttribGet(ih, "ATTRIBUTE_NAME");
  if (!elem || !name) return NULL;
  BString js("(function(){var e=document.getElementById(");
  haikuWebBrowserJSEscape(js, elem);
  js << "); return e ? e.getAttribute(";
  haikuWebBrowserJSEscape(js, name);
  js << ") : null;})()";
  return haikuWebBrowserRunJSSync(ih, js.String());
}

static int haikuWebBrowserSetJavascriptAttrib(Ihandle* ih, const char* value)
{
  iupAttribSet(ih, "_IUPWEB_JS_RESULT", NULL);
  if (!value) return 0;
  char* result = haikuWebBrowserRunJSSync(ih, value);
  if (result) iupAttribSetStr(ih, "_IUPWEB_JS_RESULT", result);
  return 0;
}

static char* haikuWebBrowserGetJavascriptAttrib(Ihandle* ih)
{
  return iupAttribGet(ih, "_IUPWEB_JS_RESULT");
}

static char* haikuWebBrowserGetDirtyAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(iupAttribGet(ih, "_IUPWEB_DIRTY") != NULL);
}

static void haikuWebBrowserComputeNaturalSizeMethod(Ihandle* ih, int* w, int* h, int* children_expand)
{
  (void)children_expand;
  int natural_w = 0, natural_h = 0;
  iupdrvFontGetCharSize(ih, &natural_w, &natural_h);
  *w = natural_w;
  *h = natural_h;
}

class IupHaikuWebDispatcher : public BHandler
{
public:
  IupHaikuWebDispatcher() : BHandler("iup_web_dispatcher") {}

  void MessageReceived(BMessage* msg) override
  {
    if (msg->what != IUPHAIKU_WEB_MAP_MSG)
    {
      BHandler::MessageReceived(msg);
      return;
    }

    iuphaikuWebBrowserInit();

    Ihandle* ih = NULL;
    msg->FindPointer("ih", (void**)&ih);
    if (!ih || !iupObjectCheck(ih)) return;
    if (iupAttribGet(ih, "_IUPWEB_HANDLER")) return;

    BView* placeholder = (BView*)ih->handle;
    if (!placeholder) return;
    BView* parent = placeholder->Parent();
    if (!parent) return;

    LooperLockGuard guard(parent->Looper());

    BRect frame = placeholder->Frame();
    parent->RemoveChild(placeholder);
    delete placeholder;
    ih->handle = NULL;

    BWebView* view = new(std::nothrow) BWebView("iup_webview", NULL);
    if (!view) return;
    view->SetResizingMode(B_FOLLOW_NONE);
    view->SetAutoHidePointer(false);
    parent->AddChild(view);
    view->MoveTo(frame.LeftTop());
    view->ResizeTo(frame.Width(), frame.Height());

    haikuWebBrowserBindView(ih, view);
  }
};

static IupHaikuWebDispatcher* sDispatcher = NULL;

static BMessenger haikuWebBrowserDispatcher()
{
  if (!sDispatcher)
  {
    sDispatcher = new IupHaikuWebDispatcher();
    LooperLockGuard guard(be_app);
    be_app->AddHandler(sDispatcher);
  }
  return BMessenger(sDispatcher);
}

static int haikuWebBrowserMapMethod(Ihandle* ih)
{
  BView* placeholder = new BView(BRect(0, 0, 0, 0), "iup_web_placeholder", B_FOLLOW_NONE, B_WILL_DRAW);
  placeholder->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
  ih->handle = (InativeHandle*)placeholder;
  iuphaikuAddToParent(ih);

  BMessage msg(IUPHAIKU_WEB_MAP_MSG);
  msg.AddPointer("ih", ih);
  haikuWebBrowserDispatcher().SendMessage(&msg);

  return IUP_NOERROR;
}

static void haikuWebBrowserUnMapMethod(Ihandle* ih)
{
  IupHaikuWebHandler* handler = haikuWebBrowserHandler(ih);

  if (handler)
  {
    BWebView* view = (BWebView*)ih->handle;
    BWindow* win = view ? view->Window() : NULL;
    {
      LooperLockGuard guard(win);
      handler->SetIhandle(NULL);
      if (view) view->WebPage()->SetListener(BMessenger());
      if (view) view->Shutdown();
    }
    BLooper* looper = handler->Looper();
    if (looper)
    {
      LooperLockGuard guard(looper);
      looper->RemoveHandler(handler);
    }
    delete handler;
    iupAttribSet(ih, "_IUPWEB_HANDLER", NULL);
    ih->handle = NULL;
    return;
  }

  iupdrvBaseUnMapMethod(ih);
}

static int haikuWebBrowserCreateMethod(Ihandle* ih, void** params)
{
  (void)params;
  ih->data = iupALLOCCTRLDATA();
  ih->data->sb = IUP_SB_HORIZ | IUP_SB_VERT;
  ih->expand = IUP_EXPAND_BOTH;
  return IUP_NOERROR;
}

Iclass* iupWebBrowserNewClass()
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "webbrowser";
  ic->cons = "WebBrowser";
  ic->format = NULL;
  ic->nativetype = IUP_TYPECONTROL;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;
  ic->has_attrib_id = 1;

  ic->New = NULL;
  ic->Create = haikuWebBrowserCreateMethod;
  ic->Map = haikuWebBrowserMapMethod;
  ic->UnMap = haikuWebBrowserUnMapMethod;
  ic->ComputeNaturalSize = haikuWebBrowserComputeNaturalSizeMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;

  iupClassRegisterCallback(ic, "NEWWINDOW_CB", "s");
  iupClassRegisterCallback(ic, "NAVIGATE_CB", "s");
  iupClassRegisterCallback(ic, "ERROR_CB", "s");
  iupClassRegisterCallback(ic, "COMPLETED_CB", "s");
  iupClassRegisterCallback(ic, "UPDATE_CB", "");

  iupBaseRegisterCommonAttrib(ic);
  iupBaseRegisterVisualAttrib(ic);

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "VALUE", haikuWebBrowserGetValueAttrib, haikuWebBrowserSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKFORWARD", NULL, haikuWebBrowserSetBackForwardAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GOBACK", NULL, haikuWebBrowserSetGoBackAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GOFORWARD", NULL, haikuWebBrowserSetGoForwardAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STOP", NULL, haikuWebBrowserSetStopAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RELOAD", NULL, haikuWebBrowserSetReloadAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HTML", haikuWebBrowserGetHTMLAttrib, haikuWebBrowserSetHTMLAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STATUS", haikuWebBrowserGetStatusAttrib, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ZOOM", haikuWebBrowserGetZoomAttrib, haikuWebBrowserSetZoomAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CANGOBACK", haikuWebBrowserGetCanGoBackAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CANGOFORWARD", haikuWebBrowserGetCanGoForwardAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FIND", NULL, haikuWebBrowserSetFindAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "EDITABLE", haikuWebBrowserGetEditableAttrib, haikuWebBrowserSetEditableAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NEW", NULL, haikuWebBrowserSetNewAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OPENFILE", NULL, haikuWebBrowserSetOpenFileAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SAVEFILE", NULL, haikuWebBrowserSetSaveFileAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "UNDO", NULL, haikuWebBrowserSetUndoAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "REDO", NULL, haikuWebBrowserSetRedoAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COPY", NULL, haikuWebBrowserSetCopyAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUT", NULL, haikuWebBrowserSetCutAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PASTE", haikuWebBrowserGetPasteAttrib, haikuWebBrowserSetPasteAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTALL", NULL, haikuWebBrowserSetSelectAllAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EXECCOMMAND", NULL, haikuWebBrowserSetExecCommandAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTIMAGE", NULL, haikuWebBrowserSetInsertImageAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTIMAGEFILE", NULL, haikuWebBrowserSetInsertImageAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CREATELINK", NULL, haikuWebBrowserSetCreateLinkAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTTEXT", NULL, haikuWebBrowserSetInsertTextAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTHTML", NULL, haikuWebBrowserSetInsertHtmlAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "FONTNAME", haikuWebBrowserGetFontNameAttrib, haikuWebBrowserSetFontNameAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FONTSIZE", haikuWebBrowserGetFontSizeAttrib, haikuWebBrowserSetFontSizeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATBLOCK", haikuWebBrowserGetFormatBlockAttrib, haikuWebBrowserSetFormatBlockAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORECOLOR", haikuWebBrowserGetForeColorAttrib, haikuWebBrowserSetForeColorAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKCOLOR", haikuWebBrowserGetBackColorAttrib, haikuWebBrowserSetBackColorAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "COMMAND", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDSTATE", haikuWebBrowserGetCommandStateAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDENABLED", haikuWebBrowserGetCommandEnabledAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDVALUE", haikuWebBrowserGetCommandValueAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ELEMENT_ID", NULL, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INNERTEXT", haikuWebBrowserGetInnerTextAttrib, haikuWebBrowserSetInnerTextAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ATTRIBUTE_NAME", NULL, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ATTRIBUTE", haikuWebBrowserGetAttributeAttrib, haikuWebBrowserSetAttributeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "JAVASCRIPT", haikuWebBrowserGetJavascriptAttrib, haikuWebBrowserSetJavascriptAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DIRTY", haikuWebBrowserGetDirtyAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "PRINT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PRINTPREVIEW", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKCOUNT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORWARDCOUNT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "ITEMHISTORY", NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDTEXT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_READONLY | IUPAF_NO_INHERIT);

  return ic;
}
