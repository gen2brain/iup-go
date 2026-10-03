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
#define IUPHAIKU_JS_TIMEOUT  5000000
#define IUPHAIKU_EDIT_MSG    'IuED'  /* editor action hop to the WebPage looper */
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
  explicit IupHaikuWebHandler(Ihandle* ih, BWebView* popup = nullptr)
    : BHandler("iup_webhandler"), fIhandle(ih), fPopup(popup), fStatus(IUP_WEB_LOAD_COMPLETED),
      fCanBack(false), fCanForward(false)
  {
  }

  void SetIhandle(Ihandle* ih) { fIhandle = ih; }
  IupHaikuWebLoadStatus Status() const { return fStatus; }
  bool CanGoBack() const { return fCanBack; }
  bool CanGoForward() const { return fCanForward; }

  void MessageReceived(BMessage* msg) override;

private:
  void PopupReceived(BMessage* msg);

  Ihandle* fIhandle;
  BWebView* fPopup;
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

static IupHaikuWebHandler* haikuWebBrowserHandler(Ihandle* ih)
{
  return reinterpret_cast<IupHaikuWebHandler*>(iupAttribGet(ih, "_IUPWEB_HANDLER"));
}

static BWebView* haikuWebBrowserView(Ihandle* ih)
{
  if (!iupAttribGet(ih, "_IUPWEB_READY")) return nullptr;
  return reinterpret_cast<BWebView*>(ih->handle);
}

class IupHaikuJSExecHandler : public BHandler
{
public:
  IupHaikuJSExecHandler() : BHandler("iup_web_jsexec") {}

  void MessageReceived(BMessage* msg) override
  {
    if (msg->what != IUPHAIKU_JS_EXEC_MSG && msg->what != IUPHAIKU_EDIT_MSG) { BHandler::MessageReceived(msg); return; }

    void* page_ptr = nullptr;
    msg->FindPointer("page", &page_ptr);

    BMessage reply(msg->what);

    auto* page = static_cast<BWebPage*>(page_ptr);
    BWebFrame* frame = page ? page->MainFrame() : nullptr;

    if (msg->what == IUPHAIKU_EDIT_MSG)
    {
      Edit(frame, msg->GetString("action", ""), reply);
      msg->SendReply(&reply);
      return;
    }

    const char* script = nullptr;
    msg->FindString("script", &script);
    JSGlobalContextRef ctx = frame ? frame->GlobalContext() : nullptr;

    if (ctx && script)
    {
      JSStringRef js_script = JSStringCreateWithUTF8CString(script);
      JSValueRef exception = nullptr;
      JSValueRef result = JSEvaluateScript(ctx, js_script, nullptr, nullptr, 1, &exception);
      JSStringRelease(js_script);

      if (!exception && result
          && !JSValueIsUndefined(ctx, result) && !JSValueIsNull(ctx, result))
      {
        JSStringRef str_ref = JSValueToStringCopy(ctx, result, nullptr);
        if (str_ref)
        {
          size_t maxlen = JSStringGetMaximumUTF8CStringSize(str_ref);
          char* buf = static_cast<char*>(malloc(maxlen));
          if (buf)
          {
            JSStringGetUTF8CString(str_ref, buf, maxlen);
            reply.AddString("result", buf);
            free(buf);
          }
          JSStringRelease(str_ref);
        }
      }
    }
    msg->SendReply(&reply);
  }

private:
  static void Edit(BWebFrame* frame, const char* action, BMessage& reply)
  {
    if (!frame) return;

    if (strcmp(action, "Copy") == 0) frame->Copy();
    else if (strcmp(action, "Cut") == 0) frame->Cut();
    else if (strcmp(action, "Paste") == 0) frame->Paste();
    else if (strcmp(action, "Undo") == 0) frame->Undo();
    else if (strcmp(action, "Redo") == 0) frame->Redo();
    else if (strcmp(action, "CanPaste") == 0) reply.AddString("result", frame->CanPaste() ? "1" : "0");
    else if (strcmp(action, "AsMarkup") == 0) reply.AddString("result", frame->AsMarkup());
  }
};

static BMessenger haikuWebBrowserJSExecMessenger(Ihandle* ih, BWebPage* page)
{
  auto* handler = reinterpret_cast<IupHaikuJSExecHandler*>(iupAttribGet(ih, "_IUPWEB_JSHANDLER"));

  if (!handler)
  {
    auto* page_handler = reinterpret_cast<BHandler*>(page);
    BLooper* page_looper = page_handler->Looper();
    if (!page_looper) return {};

    handler = new IupHaikuJSExecHandler();
    {
      LooperLockGuard guard(page_looper);
      page_looper->AddHandler(handler);
    }
    iupAttribSet(ih, "_IUPWEB_JSHANDLER", reinterpret_cast<char*>(handler));
  }

  return {handler};
}

static void haikuWebBrowserBindView(Ihandle* ih, BWebView* view)
{
  ih->handle = reinterpret_cast<InativeHandle*>(view);

  auto* handler = new IupHaikuWebHandler(ih);
  BWindow* win = view->Window();
  {
    LooperLockGuard guard(win);
    if (win) win->AddHandler(handler);
  }
  iupAttribSet(ih, "_IUPWEB_HANDLER", reinterpret_cast<char*>(handler));
  view->WebPage()->SetListener(BMessenger(handler));
  haikuWebBrowserJSExecMessenger(ih, view->WebPage());

  iupAttribSet(ih, "_IUPWEB_READY", "1");
  iupAttribUpdate(ih);
}

static void haikuWebBrowserDestroyJSExecHandler(Ihandle* ih)
{
  auto* handler = reinterpret_cast<IupHaikuJSExecHandler*>(iupAttribGet(ih, "_IUPWEB_JSHANDLER"));
  if (!handler) return;

  BLooper* looper = handler->Looper();
  if (looper)
  {
    LooperLockGuard guard(looper);
    looper->RemoveHandler(handler);
  }
  delete handler;
  iupAttribSet(ih, "_IUPWEB_JSHANDLER", nullptr);
}

static char* haikuWebBrowserPageCall(Ihandle* ih, BMessage& msg)
{
  BWebView* view = haikuWebBrowserView(ih);
  if (!view) return nullptr;

  BWebPage* page = view->WebPage();
  if (!page) return nullptr;

  BMessenger msgr = haikuWebBrowserJSExecMessenger(ih, page);
  if (!msgr.IsValid()) return nullptr;

  msg.AddPointer("page", page);
  BMessage reply;

  /* the page thread needs this looper to reach the view, and we are about to
     block it */
  BLooper* self_looper = BLooper::LooperForThread(find_thread(nullptr));
  auto* self_window = dynamic_cast<BWindow*>(self_looper);
  int relock = 0;

  if (self_window) self_window->UpdateIfNeeded();
  if (self_looper) while (self_looper->IsLocked()) { self_looper->Unlock(); relock++; }

  status_t sent = msgr.SendMessage(&msg, &reply, IUPHAIKU_JS_TIMEOUT, IUPHAIKU_JS_TIMEOUT);

  while (relock--) self_looper->Lock();

  if (sent != B_OK) return nullptr;

  const char* result = nullptr;
  if (reply.FindString("result", &result) != B_OK || !result) return nullptr;
  return iupStrReturnStr(result);
}

static char* haikuWebBrowserRunJSSync(Ihandle* ih, const char* script)
{
  if (!script) return nullptr;
  BMessage msg(IUPHAIKU_JS_EXEC_MSG);
  msg.AddString("script", script);
  return haikuWebBrowserPageCall(ih, msg);
}

static char* haikuWebBrowserEdit(Ihandle* ih, const char* action)
{
  BMessage msg(IUPHAIKU_EDIT_MSG);
  msg.AddString("action", action);
  return haikuWebBrowserPageCall(ih, msg);
}

static void haikuWebBrowserRunJSAsync(Ihandle* ih, const char* script)
{
  BWebView* view = haikuWebBrowserView(ih);
  if (!view || !script) return;

  BWebPage* page = view->WebPage();
  if (!page) return;

  BMessenger msgr = haikuWebBrowserJSExecMessenger(ih, page);
  if (!msgr.IsValid()) return;

  BMessage msg(IUPHAIKU_JS_EXEC_MSG);
  msg.AddPointer("page", page);
  msg.AddString("script", script);
  msgr.SendMessage(&msg);
}

static void haikuWebBrowserJSEscape(BString& out, const char* s)
{
  out << '"';
  for (const char* p = s; *p; ++p)
  {
    auto c = static_cast<unsigned char>(*p);

    if (c == 0xE2 && static_cast<unsigned char>(*(p+1)) == 0x80 &&
        (static_cast<unsigned char>(*(p+2)) == 0xA8 || static_cast<unsigned char>(*(p+2)) == 0xA9))
    {
      out << ((static_cast<unsigned char>(*(p+2)) == 0xA8) ? "\\u2028" : "\\u2029");
      p += 2;
      continue;
    }

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
  if (!command) return nullptr;
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

static const char* haikuWebBrowserInitScript =
  "(function() {"
  "  if (window.iupGetDirtyFlag) return;"
  "  var iupDirtyFlag = false;"
  "  document.addEventListener('input', function(e) {"
  "    if (document.body.contentEditable == 'true') {"
  "      iupDirtyFlag = true;"
  "    }"
  "  });"
  "  window.iupGetDirtyFlag = function() {"
  "    return iupDirtyFlag;"
  "  };"
  "})();";

void IupHaikuWebHandler::PopupReceived(BMessage* msg)
{
  int opener = fIhandle && iupObjectCheck(fIhandle);
  BString url;

  if (opener)
  {
    if (msg->what != NAVIGATION_REQUESTED || msg->FindString("url", &url) != B_OK ||
        url.IsEmpty() || url == "about:blank")
      return;

    IFns cb = reinterpret_cast<IFns>(IupGetCallback(fIhandle, "NEWWINDOW_CB"));
    BWebView* view = haikuWebBrowserView(fIhandle);
    if ((!cb || cb(fIhandle, const_cast<char*>(url.String())) != IUP_IGNORE) && view)
      view->LoadURL(url.String());
  }

  fPopup->WebPage()->SetListener(BMessenger());
  fPopup->Shutdown();
  Looper()->RemoveHandler(this);
  delete this;
}

void IupHaikuWebHandler::MessageReceived(BMessage* msg)
{
  if (fPopup)
  {
    PopupReceived(msg);
    return;
  }

  if (!fIhandle || !iupObjectCheck(fIhandle))
  {
    BHandler::MessageReceived(msg);
    return;
  }

  switch (msg->what)
  {
    case NAVIGATION_REQUESTED:
    {
      BString url;
      msg->FindString("url", &url);
      IFns cb = reinterpret_cast<IFns>(IupGetCallback(fIhandle, "NAVIGATE_CB"));
      if (cb && cb(fIhandle, const_cast<char*>(url.String())) == IUP_IGNORE)
      {
        haikuWebBrowserRunJSAsync(fIhandle, "window.stop();");
      }
      break;
    }
    case LOAD_NEGOTIATING:
    case LOAD_STARTED:
      fStatus = IUP_WEB_LOAD_LOADING;
      break;
    case LOAD_FAILED:
    {
      fStatus = IUP_WEB_LOAD_FAILED;
      BString url;
      msg->FindString("url", &url);
      IFns cb = reinterpret_cast<IFns>(IupGetCallback(fIhandle, "ERROR_CB"));
      if (cb) cb(fIhandle, const_cast<char*>(url.String()));
      break;
    }
    case LOAD_FINISHED:
    {
      fStatus = IUP_WEB_LOAD_COMPLETED;
      BString url;
      msg->FindString("url", &url);
      haikuWebBrowserRunJSAsync(fIhandle, haikuWebBrowserInitScript);
      if (iupAttribGet(fIhandle, "_IUPWEB_EDITABLE"))
        haikuWebBrowserRunJSAsync(fIhandle, "document.body.contentEditable = 'true';");
      IFns cb = reinterpret_cast<IFns>(IupGetCallback(fIhandle, "COMPLETED_CB"));
      if (cb) cb(fIhandle, const_cast<char*>(url.String()));
      break;
    }
    case NEW_PAGE_CREATED:
    {
      BWebView* popup = nullptr;
      if (msg->FindPointer("view", reinterpret_cast<void**>(&popup)) != B_OK || !popup)
        break;
      auto* handler = new IupHaikuWebHandler(fIhandle, popup);
      Looper()->AddHandler(handler);
      popup->WebPage()->SetListener(BMessenger(handler));
      break;
    }
    case NEW_WINDOW_REQUESTED:
    {
      BString url;
      msg->FindString("url", &url);
      IFns cb = reinterpret_cast<IFns>(IupGetCallback(fIhandle, "NEWWINDOW_CB"));
      if (cb) cb(fIhandle, const_cast<char*>(url.String()));
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
  view->LoadURL(url.String());
  return 0;
}

static char* haikuWebBrowserGetValueAttrib(Ihandle* ih)
{
  BWebView* view = haikuWebBrowserView(ih);
  if (!view) return nullptr;
  LooperLockGuard guard(view->Looper());
  BString url = view->MainFrameURL();
  if (url.Length() == 0) return nullptr;
  return iupStrReturnStr(url.String());
}

static void haikuWebBrowserBase64Encode(BString& out, const char* in, size_t len)
{
  static const char kTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  size_t i = 0;
  while (i + 3 <= len)
  {
    unsigned a = static_cast<unsigned char>(in[i]), b = static_cast<unsigned char>(in[i + 1]), c = static_cast<unsigned char>(in[i + 2]);
    out << kTable[a >> 2] << kTable[((a & 3) << 4) | (b >> 4)]
        << kTable[((b & 15) << 2) | (c >> 6)] << kTable[c & 63];
    i += 3;
  }
  if (i < len)
  {
    unsigned a = static_cast<unsigned char>(in[i]), b = (i + 1 < len) ? static_cast<unsigned char>(in[i + 1]) : 0;
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
  view->LoadURL(url.String());
  return 0;
}

static char* haikuWebBrowserGetHTMLAttrib(Ihandle* ih)
{
  char* markup = haikuWebBrowserEdit(ih, "AsMarkup");
  if (!markup || !markup[0]) return nullptr;
  return markup;
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
  if (!h) return const_cast<char*>("COMPLETED");
  switch (h->Status())
  {
    case IUP_WEB_LOAD_LOADING:   return const_cast<char*>("LOADING");
    case IUP_WEB_LOAD_FAILED:    return const_cast<char*>("FAILED");
    case IUP_WEB_LOAD_COMPLETED: return const_cast<char*>("COMPLETED");
  }
  return const_cast<char*>("COMPLETED");
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
  haikuWebBrowserEdit(ih, "Copy");
  return 0;
}

static int haikuWebBrowserSetCutAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  haikuWebBrowserEdit(ih, "Cut");
  return 0;
}

static int haikuWebBrowserSetPasteAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  haikuWebBrowserEdit(ih, "Paste");
  return 0;
}

static char* haikuWebBrowserGetPasteAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(iupStrBoolean(haikuWebBrowserEdit(ih, "CanPaste")));
}

static int haikuWebBrowserSetUndoAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  haikuWebBrowserEdit(ih, "Undo");
  return 0;
}

static int haikuWebBrowserSetRedoAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  haikuWebBrowserEdit(ih, "Redo");
  return 0;
}

static int haikuWebBrowserSetSelectAllAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  haikuWebBrowserJSExecCommand(ih, "selectAll", nullptr);
  return 0;
}

static int haikuWebBrowserSetEditableAttrib(Ihandle* ih, const char* value)
{
  bool editable = iupStrBoolean(value);
  iupAttribSet(ih, "_IUPWEB_EDITABLE", editable ? "1" : nullptr);

  if (!haikuWebBrowserView(ih)) return 1;

  haikuWebBrowserRunJSAsync(ih,
    editable ? "document.body.contentEditable = 'true';"
             : "document.body.contentEditable = 'false';");
  return 0;
}

static char* haikuWebBrowserGetEditableAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(iupAttribGet(ih, "_IUPWEB_EDITABLE") != nullptr);
}

static int haikuWebBrowserSetNewAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  haikuWebBrowserSetHTMLAttrib(ih, "<html><body></body></html>");
  haikuWebBrowserSetEditableAttrib(ih, "Yes");
  return 0;
}

static int haikuWebBrowserSetOpenFileAttrib(Ihandle* ih, const char* value)
{
  if (!value) return 0;
  char* url = iupStrFileMakeURL(value);
  if (!url) return 0;
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
  if (value) haikuWebBrowserJSExecCommand(ih, value, nullptr);
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
  char* result = haikuWebBrowserJSQueryCommand(ih, "queryCommandState", iupAttribGet(ih, "COMMAND"));
  return iupStrReturnBoolean(result && strcmp(result, "true") == 0);
}

static char* haikuWebBrowserGetCommandEnabledAttrib(Ihandle* ih)
{
  char* result = haikuWebBrowserJSQueryCommand(ih, "queryCommandEnabled", iupAttribGet(ih, "COMMAND"));
  return iupStrReturnBoolean(result && strcmp(result, "true") == 0);
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
  if (!elem) return nullptr;
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
  if (!elem || !name) return nullptr;
  BString js("(function(){var e=document.getElementById(");
  haikuWebBrowserJSEscape(js, elem);
  js << "); return e ? e.getAttribute(";
  haikuWebBrowserJSEscape(js, name);
  js << ") : null;})()";
  return haikuWebBrowserRunJSSync(ih, js.String());
}

static int haikuWebBrowserSetJavascriptAttrib(Ihandle* ih, const char* value)
{
  iupAttribSet(ih, "_IUPWEB_JS_RESULT", nullptr);
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
  if (!iupAttribGet(ih, "_IUPWEB_EDITABLE"))
    return iupStrReturnBoolean(0);

  char* result = haikuWebBrowserRunJSSync(ih, "window.iupGetDirtyFlag ? window.iupGetDirtyFlag() : false;");
  return iupStrReturnBoolean(result && strcmp(result, "true") == 0);
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

    Ihandle* ih = nullptr;
    msg->FindPointer("ih", reinterpret_cast<void**>(&ih));
    if (!ih || !iupObjectCheck(ih)) return;
    if (iupAttribGet(ih, "_IUPWEB_HANDLER")) return;

    auto* placeholder = reinterpret_cast<BView*>(ih->handle);
    if (!placeholder) return;
    BView* parent = placeholder->Parent();
    if (!parent) return;

    LooperLockGuard guard(parent->Looper());

    BRect frame = placeholder->Frame();
    parent->RemoveChild(placeholder);
    delete placeholder;
    ih->handle = nullptr;

    auto* view = new(std::nothrow) BWebView("iup_webview", nullptr);
    if (!view) return;
    view->SetResizingMode(B_FOLLOW_NONE);
    view->SetAutoHidePointer(false);
    parent->AddChild(view);
    view->MoveTo(frame.LeftTop());
    view->ResizeTo(frame.Width(), frame.Height());

    haikuWebBrowserBindView(ih, view);
  }
};

static IupHaikuWebDispatcher* sDispatcher = nullptr;

static BMessenger haikuWebBrowserDispatcher()
{
  if (!sDispatcher)
  {
    sDispatcher = new IupHaikuWebDispatcher();
    LooperLockGuard guard(be_app);
    be_app->AddHandler(sDispatcher);
  }
  return {sDispatcher};
}

static int haikuWebBrowserMapMethod(Ihandle* ih)
{
  auto* placeholder = new BView(BRect(0, 0, 0, 0), "iup_web_placeholder", B_FOLLOW_NONE, B_WILL_DRAW);
  placeholder->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
  ih->handle = reinterpret_cast<InativeHandle*>(placeholder);
  iuphaikuAddToParent(ih);

  BMessage msg(IUPHAIKU_WEB_MAP_MSG);
  msg.AddPointer("ih", ih);
  haikuWebBrowserDispatcher().SendMessage(&msg);

  return IUP_NOERROR;
}

static void haikuWebBrowserUnMapMethod(Ihandle* ih)
{
  IupHaikuWebHandler* handler = haikuWebBrowserHandler(ih);

  haikuWebBrowserDestroyJSExecHandler(ih);

  if (handler)
  {
    auto* view = reinterpret_cast<BWebView*>(ih->handle);
    BWindow* win = view ? view->Window() : nullptr;
    {
      LooperLockGuard guard(win);
      handler->SetIhandle(nullptr);
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
    iupAttribSet(ih, "_IUPWEB_HANDLER", nullptr);
    ih->handle = nullptr;
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
  Iclass* ic = iupClassNew(nullptr);

  ic->name = "webbrowser";
  ic->cons = "WebBrowser";
  ic->format = nullptr;
  ic->nativetype = IUP_TYPECONTROL;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;
  ic->has_attrib_id = 1;

  ic->New = nullptr;
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

  iupClassRegisterAttribute(ic, "BGCOLOR", nullptr, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "VALUE", haikuWebBrowserGetValueAttrib, haikuWebBrowserSetValueAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKFORWARD", nullptr, haikuWebBrowserSetBackForwardAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GOBACK", nullptr, haikuWebBrowserSetGoBackAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GOFORWARD", nullptr, haikuWebBrowserSetGoForwardAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STOP", nullptr, haikuWebBrowserSetStopAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RELOAD", nullptr, haikuWebBrowserSetReloadAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HTML", haikuWebBrowserGetHTMLAttrib, haikuWebBrowserSetHTMLAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STATUS", haikuWebBrowserGetStatusAttrib, nullptr, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ZOOM", haikuWebBrowserGetZoomAttrib, haikuWebBrowserSetZoomAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CANGOBACK", haikuWebBrowserGetCanGoBackAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CANGOFORWARD", haikuWebBrowserGetCanGoForwardAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FIND", nullptr, haikuWebBrowserSetFindAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "EDITABLE", haikuWebBrowserGetEditableAttrib, haikuWebBrowserSetEditableAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NEW", nullptr, haikuWebBrowserSetNewAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OPENFILE", nullptr, haikuWebBrowserSetOpenFileAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SAVEFILE", nullptr, haikuWebBrowserSetSaveFileAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "UNDO", nullptr, haikuWebBrowserSetUndoAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "REDO", nullptr, haikuWebBrowserSetRedoAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COPY", nullptr, haikuWebBrowserSetCopyAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUT", nullptr, haikuWebBrowserSetCutAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PASTE", haikuWebBrowserGetPasteAttrib, haikuWebBrowserSetPasteAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTALL", nullptr, haikuWebBrowserSetSelectAllAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EXECCOMMAND", nullptr, haikuWebBrowserSetExecCommandAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTIMAGE", nullptr, haikuWebBrowserSetInsertImageAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTIMAGEFILE", nullptr, haikuWebBrowserSetInsertImageAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CREATELINK", nullptr, haikuWebBrowserSetCreateLinkAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTTEXT", nullptr, haikuWebBrowserSetInsertTextAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTHTML", nullptr, haikuWebBrowserSetInsertHtmlAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "FONTNAME", haikuWebBrowserGetFontNameAttrib, haikuWebBrowserSetFontNameAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FONTSIZE", haikuWebBrowserGetFontSizeAttrib, haikuWebBrowserSetFontSizeAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATBLOCK", haikuWebBrowserGetFormatBlockAttrib, haikuWebBrowserSetFormatBlockAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORECOLOR", haikuWebBrowserGetForeColorAttrib, haikuWebBrowserSetForeColorAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKCOLOR", haikuWebBrowserGetBackColorAttrib, haikuWebBrowserSetBackColorAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "COMMAND", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDSTATE", haikuWebBrowserGetCommandStateAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDENABLED", haikuWebBrowserGetCommandEnabledAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDVALUE", haikuWebBrowserGetCommandValueAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ELEMENT_ID", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INNERTEXT", haikuWebBrowserGetInnerTextAttrib, haikuWebBrowserSetInnerTextAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ATTRIBUTE_NAME", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ATTRIBUTE", haikuWebBrowserGetAttributeAttrib, haikuWebBrowserSetAttributeAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "JAVASCRIPT", haikuWebBrowserGetJavascriptAttrib, haikuWebBrowserSetJavascriptAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DIRTY", haikuWebBrowserGetDirtyAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "PRINT", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PRINTPREVIEW", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKCOUNT", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORWARDCOUNT", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "ITEMHISTORY", nullptr, nullptr, IUPAF_NOT_SUPPORTED | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDTEXT", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED | IUPAF_READONLY | IUPAF_NO_INHERIT);

  return ic;
}
