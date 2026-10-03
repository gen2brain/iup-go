/** \file
 * \brief WebView2 Web Browser Control for Windows 10/11 with built-in loader
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <string.h>

#include <windows.h>
#include <string>
#include <vector>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_childtree.h"
#include "iup_str.h"
#include "iup_webbrowser.h"
#include "iup_drvfont.h"

#include "iupwin_webbrowser.h"

#if defined(IUPWEB_HOSTED)
#include "iupweb_host.h"
#elif defined(IUP_USE_WINUI)
extern "C" IUP_DRV_API void iupwinuiHwndHostRemove(Ihandle* ih);
#endif


static WCHAR* iupwinStrChar2Wide(const char* str)
{
  if (str)
  {
    int len = static_cast<int>(strlen(str));
    auto* wstr = static_cast<WCHAR*>(malloc((len + 1) * sizeof(WCHAR)));
    int wlen = MultiByteToWideChar(CP_UTF8, 0, str, len, wstr, len);
    if (wlen < 0)
      wlen = 0;
    wstr[wlen] = 0;
    return wstr;
  }

  return nullptr;
}

static char* iupwinStrWide2Char(const WCHAR* wstr)
{
  if (wstr)
  {
    int len = static_cast<int>(wcslen(wstr));
    char* str = static_cast<char*>(malloc((3 * len + 1) * sizeof(char)));
    int clen = WideCharToMultiByte(CP_UTF8, 0, wstr, len, str, 3 * len, nullptr, nullptr);
    if (clen < 0)
      clen = 0;
    str[clen] = 0;
    return str;
  }

  return nullptr;
}

#ifndef __IID_DEFINED__
#define __IID_DEFINED__
typedef struct _GUID IID;
#endif

/* ========================================================================== */
/* WebView2 Built-in Loader Implementation                                   */
/* ========================================================================== */

static HMODULE g_embeddedBrowserModule = nullptr;
static std::wstring g_runtimePath;
static CreateWebViewEnvironmentWithOptionsInternalFunc g_createEnvInternalFunc = nullptr;
static WebView2RuntimeType g_runtimeType = WEBVIEW2_RUNTIME_TYPE_INSTALLED;

static std::wstring FindLatestWebView2Version(const std::wstring& basePath)
{
  std::wstring runtimePath = basePath + L"\\Microsoft\\EdgeWebView\\Application";

  if (GetFileAttributesW(runtimePath.c_str()) == INVALID_FILE_ATTRIBUTES)
    return {};

  WIN32_FIND_DATAW findData;
  std::wstring searchPath = runtimePath + L"\\*";
  HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);

  if (hFind == INVALID_HANDLE_VALUE)
    return {};

  std::wstring latestVersion;
  ULONGLONG latestVersionNum = 0;

  do
  {
    if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
        wcscmp(findData.cFileName, L".") != 0 &&
        wcscmp(findData.cFileName, L"..") != 0)
    {
      if (findData.cFileName[0] >= L'0' && findData.cFileName[0] <= L'9')
      {
        std::wstring versionStr = findData.cFileName;
        ULONGLONG versionNum = 0;
        int parts[4] = { 0 };
        int partIndex = 0;

        size_t start = 0;
        size_t end = versionStr.find(L'.');
        while (end != std::wstring::npos && partIndex < 4)
        {
          parts[partIndex++] = _wtoi(versionStr.substr(start, end - start).c_str());
          start = end + 1;
          end = versionStr.find(L'.', start);
        }
        if (partIndex < 4 && start < versionStr.length())
        {
          parts[partIndex] = _wtoi(versionStr.substr(start).c_str());
        }

        versionNum = (static_cast<ULONGLONG>(parts[0]) << 48) | (static_cast<ULONGLONG>(parts[1]) << 32) |
          (static_cast<ULONGLONG>(parts[2]) << 16) | static_cast<ULONGLONG>(parts[3]);

        if (versionNum > latestVersionNum)
        {
          latestVersionNum = versionNum;
          latestVersion = versionStr;
        }
      }
    }
  } while (FindNextFileW(hFind, &findData));

  FindClose(hFind);

  if (!latestVersion.empty())
  {
    std::wstring fullPath = runtimePath + L"\\" + latestVersion;
    return fullPath;
  }

  return {};
}

static std::wstring GetWebView2RuntimePath()
{
  wchar_t path[MAX_PATH];
  std::wstring runtimePath;

  if (GetEnvironmentVariableW(L"ProgramFiles(x86)", path, MAX_PATH) > 0)
  {
    runtimePath = FindLatestWebView2Version(path);
    if (!runtimePath.empty())
      return runtimePath;
  }

  if (GetEnvironmentVariableW(L"ProgramFiles", path, MAX_PATH) > 0)
  {
    runtimePath = FindLatestWebView2Version(path);
    if (!runtimePath.empty())
      return runtimePath;
  }

  if (GetEnvironmentVariableW(L"LOCALAPPDATA", path, MAX_PATH) > 0)
  {
    runtimePath = FindLatestWebView2Version(path);
    if (!runtimePath.empty())
      return runtimePath;
  }

  return {};
}

static std::wstring GetWebView2UserDataFolder()
{
  wchar_t localAppData[MAX_PATH];
  wchar_t exePath[MAX_PATH];

  if (GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH) == 0)
    return {};

  if (GetModuleFileNameW(nullptr, exePath, MAX_PATH) == 0)
    return {};

  std::wstring exePathStr = exePath;
  size_t lastSlash = exePathStr.find_last_of(L"\\/");
  std::wstring exeName = (lastSlash != std::wstring::npos) ?
    exePathStr.substr(lastSlash + 1) : exePathStr;

  size_t lastDot = exeName.find_last_of(L'.');
  if (lastDot != std::wstring::npos)
    exeName = exeName.substr(0, lastDot);

  std::wstring userDataPath = localAppData;
  userDataPath += L"\\";
  userDataPath += exeName;
  userDataPath += L"\\WebView2";

  return userDataPath;
}

STDAPI CreateCoreWebView2EnvironmentWithOptions(
    PCWSTR browserExecutableFolder,
    PCWSTR userDataFolder,
    void* environmentOptions,
    ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* environmentCreatedHandler)
{
  if (g_createEnvInternalFunc == nullptr)
    return E_FAIL;

  HRESULT hr = g_createEnvInternalFunc(
      true,
      g_runtimeType,
      userDataFolder,
      (IUnknown*)environmentOptions,
      environmentCreatedHandler);

  return hr;
}

HRESULT IupWebView2LoaderInit(void)
{
  if (g_createEnvInternalFunc)
    return S_OK;

  g_runtimePath = GetWebView2RuntimePath();
  if (g_runtimePath.empty())
  {
    IupSetGlobal("IUP_WEBBROWSER_MISSING_LIB", "Microsoft Edge WebView2 Runtime");
    return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
  }

  g_runtimeType = WEBVIEW2_RUNTIME_TYPE_INSTALLED;

#if defined(_M_X64) || defined(__x86_64__)
  std::wstring archFolder = L"\\EBWebView\\x64\\";
#elif defined(_M_IX86) || defined(__i386__)
  std::wstring archFolder = L"\\EBWebView\\x86\\";
#elif defined(_M_ARM64) || defined(__aarch64__)
  std::wstring archFolder = L"\\EBWebView\\arm64\\";
#else
  #error Unsupported architecture
#endif

  std::wstring loaderDllPath = g_runtimePath + archFolder + L"EmbeddedBrowserWebView.dll";

  if (GetFileAttributesW(loaderDllPath.c_str()) == INVALID_FILE_ATTRIBUTES)
  {
    loaderDllPath = g_runtimePath + L"\\EmbeddedBrowserWebView.dll";
    if (GetFileAttributesW(loaderDllPath.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
      IupSetGlobal("IUP_WEBBROWSER_MISSING_LIB", "EmbeddedBrowserWebView.dll");
      return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }
  }

  g_embeddedBrowserModule = LoadLibraryExW(loaderDllPath.c_str(), nullptr,
      LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR);

  if (!g_embeddedBrowserModule)
  {
    DWORD error = GetLastError();
    IupSetGlobal("IUP_WEBBROWSER_MISSING_LIB", "EmbeddedBrowserWebView.dll");
    return HRESULT_FROM_WIN32(error);
  }

  g_createEnvInternalFunc = reinterpret_cast<CreateWebViewEnvironmentWithOptionsInternalFunc>(GetProcAddress(g_embeddedBrowserModule, "CreateWebViewEnvironmentWithOptionsInternal"));

  if (!g_createEnvInternalFunc)
  {
    DWORD error = GetLastError();
    FreeLibrary(g_embeddedBrowserModule);
    g_embeddedBrowserModule = nullptr;
    IupSetGlobal("IUP_WEBBROWSER_MISSING_LIB", "EmbeddedBrowserWebView.dll (Invalid Version?)");
    return HRESULT_FROM_WIN32(error);
  }

  return S_OK;
}

void IupWebView2LoaderCleanup(void)
{
  if (g_embeddedBrowserModule)
  {
    FreeLibrary(g_embeddedBrowserModule);
    g_embeddedBrowserModule = nullptr;
  }

  g_createEnvInternalFunc = nullptr;
  g_runtimePath.clear();
}

CreateCoreWebView2EnvironmentWithOptionsFunc IupWebView2LoaderGetCreateEnvironmentFunc(void)
{
  return CreateCoreWebView2EnvironmentWithOptions;
}

const wchar_t* IupWebView2LoaderGetRuntimePath(void)
{
  return g_runtimePath.empty() ? nullptr : g_runtimePath.c_str();
}

/* ========================================================================== */
/* WebBrowser Control Implementation                                         */
/* ========================================================================== */

static int g_comInitialized = 0;

#define IUPWIN_WEBVIEW_INIT_TIMEOUT 3000  /* 10ms slices */

typedef enum
{
  WEBVIEW_STATUS_COMPLETED = 0,
  WEBVIEW_STATUS_FAILED = 1,
  WEBVIEW_STATUS_LOADING = 2
} WebViewLoadStatus;

struct _IcontrolData
{
  ICoreWebView2Controller* webviewController;
  ICoreWebView2* webviewWindow;
  EventRegistrationToken navigationStartingToken;
  EventRegistrationToken navigationCompletedToken;
  EventRegistrationToken newWindowRequestedToken;
  EventRegistrationToken historyChangedToken;
  EventRegistrationToken webMessageReceivedToken;
  WebViewLoadStatus loadStatus;
  HWND hwnd;
  RECT viewBounds;
  int hasViewBounds;
  WNDPROC oldWndProc;
};

static void winWebBrowserApplyBounds(Ihandle* ih)
{
  RECT bounds;

  if (!ih->data->webviewController || !ih->data->hwnd)
    return;

  if (ih->data->hasViewBounds)
    bounds = ih->data->viewBounds;
  else
    GetClientRect(ih->data->hwnd, &bounds);

  ih->data->webviewController->put_Bounds(bounds);
}

static LRESULT CALLBACK WebBrowserWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  auto* ih = static_cast<Ihandle*>(GetProp(hwnd, TEXT("IUP_WEBBROWSER_IH")));

  if (msg == WM_SIZE && ih && ih->data)
    winWebBrowserApplyBounds(ih);

  if (ih && ih->data && ih->data->oldWndProc)
    return CallWindowProc(ih->data->oldWndProc, hwnd, msg, wParam, lParam);
  else
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

template <typename TInterface, typename TDerived>
class EventHandler : public TInterface
{
protected:
  ULONG refCount;
  Ihandle* ih;

  ~EventHandler() {}

public:
  EventHandler(Ihandle* handle) : refCount(1), ih(handle) {}

  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) noexcept override
  {
    if (riid == IID_IUnknown || riid == iup_uuidof<TInterface>())
    {
      *ppvObject = static_cast<TInterface*>(this);
      AddRef();
      return S_OK;
    }
    *ppvObject = nullptr;
    return E_NOINTERFACE;
  }

  ULONG STDMETHODCALLTYPE AddRef() noexcept override
  {
    return InterlockedIncrement(&refCount);
  }

  ULONG STDMETHODCALLTYPE Release() noexcept override
  {
    ULONG count = InterlockedDecrement(&refCount);
    if (count == 0)
      delete static_cast<TDerived*>(this);
    return count;
  }
};

struct WinInitState
{
  int complete;
  int abandoned;

  WinInitState() : complete(0), abandoned(0) {}
};

struct WinScriptResult
{
  char* result;
  int completed;
  int abandoned;

  WinScriptResult() : result(nullptr), completed(0), abandoned(0) {}
};

static void winWebBrowserWaitScript(WinScriptResult* async)
{
  MSG msg;
  int loopCount = 0;

  while (async->completed == 0 && loopCount < 1000)
  {
    if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    else
    {
      Sleep(10);
    }
    loopCount++;
  }
}

static char* winWebBrowserTakeScript(WinScriptResult* async)
{
  char* result;

  if (!async->completed)
  {
    async->abandoned = 1;
    return nullptr;
  }

  result = async->result;
  delete async;
  return result;
}

static std::string winWebBrowserEscapeJavaScript(const char* str)
{
  if (!str)
    return "null";

  std::string result = "\"";
  for (const char* p = str; *p; p++)
  {
    if (static_cast<unsigned char>(*p) == 0xE2 && static_cast<unsigned char>(*(p+1)) == 0x80 &&
        (static_cast<unsigned char>(*(p+2)) == 0xA8 || static_cast<unsigned char>(*(p+2)) == 0xA9))
    {
      result += (static_cast<unsigned char>(*(p+2)) == 0xA8) ? "\\u2028" : "\\u2029";
      p += 2;
      continue;
    }

    switch (*p)
    {
      case '"':  result += "\\\""; break;
      case '\\': result += "\\\\"; break;
      case '\b': result += "\\b"; break;
      case '\f': result += "\\f"; break;
      case '\n': result += "\\n"; break;
      case '\r': result += "\\r"; break;
      case '\t': result += "\\t"; break;
      default:
        if (static_cast<unsigned char>(*p) < 32)
        {
          char buf[7];
          snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(*p));
          result += buf;
        }
        else
        {
          result += *p;
        }
        break;
    }
  }
  result += "\"";
  return result;
}

class NavigationStartingHandler : public EventHandler<ICoreWebView2NavigationStartingEventHandler, NavigationStartingHandler>
{
public:
  NavigationStartingHandler(Ihandle* handle) : EventHandler(handle) {}

  HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args) noexcept override
  {
    iupAttribSet(ih, "_IUPWEB_FAILED", nullptr);
    ih->data->loadStatus = WEBVIEW_STATUS_LOADING;

    if (iupAttribGet(ih, "_IUPWEB_IGNORE_NAVIGATE"))
      return S_OK;

    IFns cb = reinterpret_cast<IFns>(IupGetCallback(ih, "NAVIGATE_CB"));
    if (cb)
    {
      LPWSTR uri = nullptr;
      args->get_Uri(&uri);
      if (uri)
      {
        char* urlString = iupwinStrWide2Char(uri);
        int ret = cb(ih, urlString);
        free(urlString);
        CoTaskMemFree(uri);

        if (ret == IUP_IGNORE)
          args->put_Cancel(TRUE);
      }
    }
    return S_OK;
  }
};

class NavigationCompletedHandler : public EventHandler<ICoreWebView2NavigationCompletedEventHandler, NavigationCompletedHandler>
{
public:
  NavigationCompletedHandler(Ihandle* handle) : EventHandler(handle) {}

  HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) noexcept override;
};

class NewWindowHandler : public EventHandler<ICoreWebView2NewWindowRequestedEventHandler, NewWindowHandler>
{
public:
  NewWindowHandler(Ihandle* handle) : EventHandler(handle) {}

  HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2* sender, ICoreWebView2NewWindowRequestedEventArgs* args) noexcept override
  {
    IFns cb = reinterpret_cast<IFns>(IupGetCallback(ih, "NEWWINDOW_CB"));
    if (cb)
    {
      LPWSTR uri = nullptr;
      args->get_Uri(&uri);
      if (uri)
      {
        char* urlString = iupwinStrWide2Char(uri);
        cb(ih, urlString);
        free(urlString);
        CoTaskMemFree(uri);
      }
    }
    args->put_Handled(TRUE);
    return S_OK;
  }
};

class HistoryChangedHandler : public EventHandler<ICoreWebView2HistoryChangedEventHandler, HistoryChangedHandler>
{
public:
  HistoryChangedHandler(Ihandle* handle) : EventHandler(handle) {}

  HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2* sender, IUnknown* args) noexcept override;
};

class WebMessageReceivedHandler : public EventHandler<ICoreWebView2WebMessageReceivedEventHandler, WebMessageReceivedHandler>
{
public:
  WebMessageReceivedHandler(Ihandle* handle) : EventHandler(handle) {}

  HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args) noexcept override
  {
    LPWSTR message = nullptr;
    args->TryGetWebMessageAsString(&message);

    if (message)
    {
      if (wcscmp(message, L"dirty") == 0)
      {
        iupAttribSet(ih, "_IUPWEB_DIRTY", "1");
      }
      else if (wcscmp(message, L"update") == 0)
      {
        IFn update_cb = static_cast<IFn>(IupGetCallback(ih, "UPDATE_CB"));
        if (update_cb)
          update_cb(ih);
      }

      CoTaskMemFree(message);
    }

    return S_OK;
  }
};

class CreateWebViewHandler : public EventHandler<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler, CreateWebViewHandler>
{
private:
  WinInitState* state;

public:
  CreateWebViewHandler(Ihandle* handle, WinInitState* init) : EventHandler(handle), state(init) {}

  HRESULT STDMETHODCALLTYPE Invoke(HRESULT result, ICoreWebView2Controller* controller) noexcept override;
};

class CreateEnvironmentHandler : public EventHandler<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler, CreateEnvironmentHandler>
{
private:
  HWND hwnd;
  WinInitState* state;

public:
  CreateEnvironmentHandler(Ihandle* handle, HWND window, WinInitState* init) : EventHandler(handle), hwnd(window), state(init) {}

  HRESULT STDMETHODCALLTYPE Invoke(HRESULT result, ICoreWebView2Environment* env) noexcept override
  {
    if (state->abandoned)
    {
      delete state;
      return S_OK;
    }

    if (FAILED(result))
    {
      state->complete = -1;
      return result;
    }

    auto* handler = new CreateWebViewHandler(ih, state);
    HRESULT hr = env->CreateCoreWebView2Controller(hwnd, handler);
    handler->Release();

    if (FAILED(hr))
      state->complete = -1;

    return hr;
  }
};

static void winWebBrowserUpdateHistory(Ihandle* ih)
{
  if (!ih->data->webviewWindow)
    return;

  BOOL canGoBack = FALSE;
  BOOL canGoForward = FALSE;

  ih->data->webviewWindow->get_CanGoBack(&canGoBack);
  ih->data->webviewWindow->get_CanGoForward(&canGoForward);

  iupAttribSet(ih, "CANGOBACK", canGoBack ? "YES" : "NO");
  iupAttribSet(ih, "CANGOFORWARD", canGoForward ? "YES" : "NO");
}

HRESULT NavigationCompletedHandler::Invoke(ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) noexcept
{
  BOOL success = FALSE;
  args->get_IsSuccess(&success);

  if (!success)
  {
    iupAttribSet(ih, "_IUPWEB_FAILED", "1");
    ih->data->loadStatus = WEBVIEW_STATUS_FAILED;

    IFns cb = reinterpret_cast<IFns>(IupGetCallback(ih, "ERROR_CB"));
    if (cb)
    {
      LPWSTR uri = nullptr;
      sender->get_Source(&uri);
      if (uri)
      {
        char* urlString = iupwinStrWide2Char(uri);
        cb(ih, urlString);
        free(urlString);
        CoTaskMemFree(uri);
      }
      else
      {
        cb(ih, const_cast<char*>("Unknown URI"));
      }
    }
  }
  else
  {
    ih->data->loadStatus = WEBVIEW_STATUS_COMPLETED;

    if (iupAttribGet(ih, "_IUPWEB_EDITABLE"))
    {
      sender->ExecuteScript(L"document.body.contentEditable = 'true';", nullptr);
    }
    else
    {
      sender->ExecuteScript(L"document.body.contentEditable = 'false';", nullptr);
    }

    IFns cb = reinterpret_cast<IFns>(IupGetCallback(ih, "COMPLETED_CB"));
    if (cb)
    {
      LPWSTR uri = nullptr;
      sender->get_Source(&uri);
      if (uri)
      {
        char* urlString = iupwinStrWide2Char(uri);
        cb(ih, urlString);
        free(urlString);
        CoTaskMemFree(uri);
      }
      else
      {
        cb(ih, const_cast<char*>("Unknown URI"));
      }
    }
  }

  winWebBrowserUpdateHistory(ih);

  IFn update_cb = static_cast<IFn>(IupGetCallback(ih, "UPDATE_CB"));
  if (update_cb)
    update_cb(ih);

  return S_OK;
}

HRESULT HistoryChangedHandler::Invoke(ICoreWebView2* sender, IUnknown* args) noexcept
{
  (void)sender;
  (void)args;
  winWebBrowserUpdateHistory(ih);
  return S_OK;
}

HRESULT CreateWebViewHandler::Invoke(HRESULT result, ICoreWebView2Controller* controller) noexcept
{
  if (state->abandoned)
  {
    delete state;
    return S_OK;
  }

  if (FAILED(result))
  {
    state->complete = -1;
    return result;
  }

  ih->data->webviewController = controller;
  controller->AddRef();

  ICoreWebView2Controller2* controller2 = nullptr;
  HRESULT hr = controller->QueryInterface(iup_uuidof<ICoreWebView2Controller2>(), reinterpret_cast<void**>(&controller2));
  if (SUCCEEDED(hr) && controller2)
  {
    controller2->put_DefaultBackgroundColor(RGB(255, 255, 255));
    controller2->Release();
  }

  controller->get_CoreWebView2(&ih->data->webviewWindow);
  if (ih->data->webviewWindow)
    ih->data->webviewWindow->AddRef();

  winWebBrowserApplyBounds(ih);
  controller->put_IsVisible(TRUE);

  ICoreWebView2* webview = ih->data->webviewWindow;

  ICoreWebView2Settings* settings = nullptr;
  hr = webview->get_Settings(&settings);
  if (SUCCEEDED(hr) && settings)
  {
    settings->put_IsScriptEnabled(TRUE);
    settings->put_IsWebMessageEnabled(TRUE);
    settings->put_AreDefaultScriptDialogsEnabled(TRUE);
    settings->put_IsStatusBarEnabled(FALSE);
    settings->put_AreDevToolsEnabled(TRUE);
    settings->put_AreDefaultContextMenusEnabled(TRUE);
    settings->put_AreHostObjectsAllowed(FALSE);
    settings->put_IsZoomControlEnabled(TRUE);
    settings->put_IsBuiltInErrorPageEnabled(TRUE);
    settings->Release();
  }

  auto* navStartHandler = new NavigationStartingHandler(ih);
  webview->add_NavigationStarting(navStartHandler, &ih->data->navigationStartingToken);
  navStartHandler->Release();

  auto* navCompletedHandler = new NavigationCompletedHandler(ih);
  webview->add_NavigationCompleted(navCompletedHandler, &ih->data->navigationCompletedToken);
  navCompletedHandler->Release();

  auto* newWindowHandler = new NewWindowHandler(ih);
  webview->add_NewWindowRequested(newWindowHandler, &ih->data->newWindowRequestedToken);
  newWindowHandler->Release();

  auto* historyHandler = new HistoryChangedHandler(ih);
  webview->add_HistoryChanged(historyHandler, &ih->data->historyChangedToken);
  historyHandler->Release();

  auto* messageHandler = new WebMessageReceivedHandler(ih);
  webview->add_WebMessageReceived(messageHandler, &ih->data->webMessageReceivedToken);
  messageHandler->Release();

  const wchar_t* updateScript =
    L"(function() {"
    L"  var iupDirtyFlag = false;"
    L"  var iupSavedRange = null;"
    L"  document.addEventListener('selectionchange', function() {"
    L"    if (document.body.contentEditable == 'true') {"
    L"      window.chrome.webview.postMessage('update');"
    L"      var sel = window.getSelection();"
    L"      if (sel.rangeCount > 0) {"
    L"        iupSavedRange = sel.getRangeAt(0);"
    L"      }"
    L"    }"
    L"  });"
    L"  document.addEventListener('input', function() {"
    L"    if (document.body.contentEditable == 'true' && !iupDirtyFlag) {"
    L"      iupDirtyFlag = true;"
    L"      window.chrome.webview.postMessage('dirty');"
    L"    }"
    L"  });"
    L"  window.iupRestoreSelection = function() {"
    L"    if (document.body.contentEditable == 'true' && iupSavedRange) {"
    L"      var sel = window.getSelection();"
    L"      if (!sel.isCollapsed && document.body.contains(sel.anchorNode)) return;"
    L"      sel.removeAllRanges();"
    L"      sel.addRange(iupSavedRange);"
    L"    }"
    L"  };"
    L"  window.iupResetDirtyFlag = function() {"
    L"    iupDirtyFlag = false;"
    L"  };"
    L"  window.iupGetDirtyFlag = function() {"
    L"    return iupDirtyFlag;"
    L"  };"
    L"})();";

  webview->AddScriptToExecuteOnDocumentCreated(updateScript, nullptr);

  const wchar_t* scrollbarStyleScript =
    L"(function() {"
    L"  var style = document.createElement('style');"
    L"  style.textContent = '"
    L"    ::-webkit-scrollbar { width: 16px; height: 16px; }"
    L"    ::-webkit-scrollbar-track { background: #f1f1f1; }"
    L"    ::-webkit-scrollbar-thumb { background: #888; border-radius: 8px; }"
    L"    ::-webkit-scrollbar-thumb:hover { background: #555; }"
    L"    ::-webkit-scrollbar-corner { background: #f1f1f1; }"
    L"    * { scrollbar-width: thin; scrollbar-color: #888 #f1f1f1; }"
    L"  ';"
    L"  if (document.head) {"
    L"    document.head.appendChild(style);"
    L"  } else {"
    L"    document.addEventListener('DOMContentLoaded', function() {"
    L"      document.head.appendChild(style);"
    L"    });"
    L"  }"
    L"})();";

  webview->AddScriptToExecuteOnDocumentCreated(scrollbarStyleScript, nullptr);

  state->complete = 1;

  return S_OK;
}

static int winWebBrowserSetValueAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->webviewWindow || !value)
    return 0;

  iupAttribSet(ih, "_IUPWEB_DIRTY", nullptr);
  iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", "1");

  if (iupStrEqualPartial(value, "http://") || iupStrEqualPartial(value, "https://") ||
      iupStrEqualPartial(value, "file://") || iupStrEqualPartial(value, "ftp://"))
  {
    WCHAR* wurl = iupwinStrChar2Wide(value);
    ih->data->webviewWindow->Navigate(wurl);
    free(wurl);
  }
  else
  {
    char* url = iupStrFileMakeURL(value);
    if (url)
    {
      WCHAR* wurl = iupwinStrChar2Wide(url);
      ih->data->webviewWindow->Navigate(wurl);
      free(wurl);
      free(url);
    }
  }

  iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", nullptr);

  return 0;
}

static char* winWebBrowserGetValueAttrib(Ihandle* ih)
{
  if (!ih->data->webviewWindow)
    return nullptr;

  LPWSTR uri = nullptr;
  ih->data->webviewWindow->get_Source(&uri);

  if (uri)
  {
    char* urlString = iupwinStrWide2Char(uri);
    char* retStr = iupStrReturnStr(urlString);
    free(urlString);
    CoTaskMemFree(uri);
    return retStr;
  }

  return nullptr;
}

static int winWebBrowserSetBackForwardAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->webviewWindow)
    return 0;

  int val = 0;
  if (iupStrToInt(value, &val))
  {
    if (val > 0)
    {
      for (int i = 0; i < val; i++)
        ih->data->webviewWindow->GoForward();
    }
    else if (val < 0)
    {
      for (int i = 0; i > val; i--)
        ih->data->webviewWindow->GoBack();
    }
  }

  return 0;
}

static int winWebBrowserSetGoBackAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->webviewWindow)
    return 0;

  (void)value;
  ih->data->webviewWindow->GoBack();
  return 0;
}

static int winWebBrowserSetGoForwardAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->webviewWindow)
    return 0;

  (void)value;
  ih->data->webviewWindow->GoForward();
  return 0;
}

static int winWebBrowserSetStopAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->webviewWindow)
    return 0;

  (void)value;
  ih->data->webviewWindow->Stop();
  return 0;
}

static int winWebBrowserSetReloadAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->webviewWindow)
    return 0;

  (void)value;
  ih->data->webviewWindow->Reload();
  return 0;
}

static int winWebBrowserSetHTMLAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->webviewWindow)
    return 0;

  if (!value)
    value = "";

  iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", "1");
  WCHAR* whtml = iupwinStrChar2Wide(value);
  ih->data->webviewWindow->NavigateToString(whtml);
  free(whtml);
  iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", nullptr);
  iupAttribSet(ih, "_IUPWEB_DIRTY", nullptr);

  return 0;
}

static char* winWebBrowserUnescapeJSON(const char* json_str)
{
  size_t len = strlen(json_str);
  char* result = static_cast<char*>(malloc(len + 1));
  if (!result)
    return nullptr;

  size_t j = 0;
  for (size_t i = 0; i < len; i++)
  {
    if (json_str[i] == '\\' && i + 1 < len)
    {
      i++;
      switch (json_str[i])
      {
        case '"':  result[j++] = '"';  break;
        case '\\': result[j++] = '\\'; break;
        case '/':  result[j++] = '/';  break;
        case 'b':  result[j++] = '\b'; break;
        case 'f':  result[j++] = '\f'; break;
        case 'n':  result[j++] = '\n'; break;
        case 'r':  result[j++] = '\r'; break;
        case 't':  result[j++] = '\t'; break;
        case 'u':
          if (i + 4 < len)
          {
            char hex[5] = {0};
            hex[0] = json_str[++i];
            hex[1] = json_str[++i];
            hex[2] = json_str[++i];
            hex[3] = json_str[++i];
            unsigned int code;
            if (sscanf(hex, "%x", &code) == 1)
            {
              if (code >= 0xD800 && code <= 0xDBFF && i + 6 < len &&
                  json_str[i+1] == '\\' && json_str[i+2] == 'u')
              {
                char lohex[5] = {0};
                unsigned int low;
                lohex[0] = json_str[i+3];
                lohex[1] = json_str[i+4];
                lohex[2] = json_str[i+5];
                lohex[3] = json_str[i+6];
                if (sscanf(lohex, "%x", &low) == 1 && low >= 0xDC00 && low <= 0xDFFF)
                {
                  code = 0x10000 + ((code - 0xD800) << 10) + (low - 0xDC00);
                  i += 6;
                }
              }

              if (code < 0x80)
              {
                result[j++] = static_cast<char>(code);
              }
              else if (code < 0x800)
              {
                result[j++] = static_cast<char>(0xC0 | (code >> 6));
                result[j++] = static_cast<char>(0x80 | (code & 0x3F));
              }
              else if (code < 0x10000)
              {
                result[j++] = static_cast<char>(0xE0 | (code >> 12));
                result[j++] = static_cast<char>(0x80 | ((code >> 6) & 0x3F));
                result[j++] = static_cast<char>(0x80 | (code & 0x3F));
              }
              else
              {
                result[j++] = static_cast<char>(0xF0 | (code >> 18));
                result[j++] = static_cast<char>(0x80 | ((code >> 12) & 0x3F));
                result[j++] = static_cast<char>(0x80 | ((code >> 6) & 0x3F));
                result[j++] = static_cast<char>(0x80 | (code & 0x3F));
              }
            }
          }
          break;
        default:
          result[j++] = json_str[i];
          break;
      }
    }
    else
    {
      result[j++] = json_str[i];
    }
  }
  result[j] = '\0';
  return result;
}

static char* winWebBrowserGetHTMLAttrib(Ihandle* ih)
{
  if (!ih->data->webviewWindow)
    return nullptr;

  const char* script = "document.documentElement.outerHTML;";
  WCHAR* wscript = iupwinStrChar2Wide(script);

  class GetHTMLHandler : public EventHandler<ICoreWebView2ExecuteScriptCompletedHandler, GetHTMLHandler>
  {
  private:
    WinScriptResult* async;

  public:
    GetHTMLHandler(Ihandle* handle, WinScriptResult* res) : EventHandler(handle), async(res) {}

    HRESULT STDMETHODCALLTYPE Invoke(HRESULT errorCode, LPCWSTR resultObjectAsJson) noexcept override
    {
      if (async->abandoned)
      {
        delete async;
        return S_OK;
      }

      if (SUCCEEDED(errorCode) && resultObjectAsJson)
      {
        char* str = iupwinStrWide2Char(resultObjectAsJson);
        if (str)
        {
          size_t len = strlen(str);
          if (len >= 2 && str[0] == '"' && str[len-1] == '"')
          {
            str[len-1] = '\0';
            memmove(str, str+1, len-1);
          }
          char* unescaped = winWebBrowserUnescapeJSON(str);
          free(str);
          if (unescaped)
          {
            async->result = iupStrDup(unescaped);
            free(unescaped);
          }
        }
      }
      async->completed = 1;
      return S_OK;
    }
  };

  auto* async = new WinScriptResult();
  auto* handler = new GetHTMLHandler(ih, async);
  HRESULT hr = ih->data->webviewWindow->ExecuteScript(wscript, handler);
  handler->Release();
  free(wscript);

  if (FAILED(hr))
  {
    delete async;
    return nullptr;
  }

  winWebBrowserWaitScript(async);

  char* result = winWebBrowserTakeScript(async);
  if (result)
  {
    char* ret = iupStrReturnStr(result);
    free(result);
    return ret;
  }

  return nullptr;
}

static char* winWebBrowserGetStatusAttrib(Ihandle* ih)
{
  if (!ih->data->webviewWindow)
    return const_cast<char*>("COMPLETED");

  switch (ih->data->loadStatus)
  {
    case WEBVIEW_STATUS_LOADING:
      return const_cast<char*>("LOADING");
    case WEBVIEW_STATUS_FAILED:
      return const_cast<char*>("FAILED");
    case WEBVIEW_STATUS_COMPLETED:
    default:
      return const_cast<char*>("COMPLETED");
  }
}

static int winWebBrowserSetZoomAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->webviewController)
    return 0;

  int zoom = 100;
  if (iupStrToInt(value, &zoom))
  {
    double zoomFactor = static_cast<double>(zoom) / 100.0;
    ih->data->webviewController->put_ZoomFactor(zoomFactor);
  }

  return 0;
}

static char* winWebBrowserGetZoomAttrib(Ihandle* ih)
{
  if (!ih->data->webviewController)
    return nullptr;

  double zoomFactor = 1.0;
  ih->data->webviewController->get_ZoomFactor(&zoomFactor);

  int zoom = static_cast<int>(zoomFactor * 100);
  return iupStrReturnInt(zoom);
}

static int winWebBrowserSetPrintAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->webviewWindow)
    return 0;

  (void)value;
  ih->data->webviewWindow->ExecuteScript(L"window.print();", nullptr);
  return 0;
}

static char* winWebBrowserGetCanGoBackAttrib(Ihandle* ih)
{
  if (!ih->data->webviewWindow)
    return nullptr;

  BOOL canGoBack = FALSE;
  ih->data->webviewWindow->get_CanGoBack(&canGoBack);
  return iupStrReturnBoolean(canGoBack);
}

static char* winWebBrowserGetCanGoForwardAttrib(Ihandle* ih)
{
  if (!ih->data->webviewWindow)
    return nullptr;

  BOOL canGoForward = FALSE;
  ih->data->webviewWindow->get_CanGoForward(&canGoForward);
  return iupStrReturnBoolean(canGoForward);
}

static char* winWebBrowserGetBackCountAttrib(Ihandle* ih)
{
  if (!ih->data->webviewWindow)
    return iupStrReturnInt(0);

  /* WebView2 API limitation: cannot get actual history count */
  BOOL canGoBack = FALSE;
  ih->data->webviewWindow->get_CanGoBack(&canGoBack);
  return iupStrReturnInt(canGoBack ? 1 : 0);
}

static char* winWebBrowserGetForwardCountAttrib(Ihandle* ih)
{
  if (!ih->data->webviewWindow)
    return iupStrReturnInt(0);

  /* WebView2 API limitation: cannot get actual history count */
  BOOL canGoForward = FALSE;
  ih->data->webviewWindow->get_CanGoForward(&canGoForward);
  return iupStrReturnInt(canGoForward ? 1 : 0);
}

static char* winWebBrowserGetItemHistoryAttrib(Ihandle* ih, int id)
{
  (void)ih;
  (void)id;
  /* WebView2 API limitation: navigation history list is not accessible. */
  return nullptr;
}

static void winWebBrowserExecuteJavascript(Ihandle* ih, const char* script)
{
  if (!ih->data->webviewWindow || !script)
    return;

  WCHAR* wscript = iupwinStrChar2Wide(script);
  ih->data->webviewWindow->ExecuteScript(wscript, nullptr);
  free(wscript);
}

class ExecuteScriptHandler : public EventHandler<ICoreWebView2ExecuteScriptCompletedHandler, ExecuteScriptHandler>
{
private:
  WinScriptResult* async;

public:
  ExecuteScriptHandler(Ihandle* handle, WinScriptResult* res) : EventHandler(handle), async(res) {}

  HRESULT STDMETHODCALLTYPE Invoke(HRESULT errorCode, LPCWSTR resultObjectAsJson) noexcept override
  {
    if (async->abandoned)
    {
      delete async;
      return S_OK;
    }

    if (SUCCEEDED(errorCode) && resultObjectAsJson)
    {
      char* str = iupwinStrWide2Char(resultObjectAsJson);
      if (str)
      {
        size_t len = strlen(str);
        if (len >= 2 && str[0] == '"' && str[len-1] == '"')
        {
          str[len-1] = '\0';
          memmove(str, str+1, len-1);
        }
        char* unescaped = winWebBrowserUnescapeJSON(str);
        free(str);
        if (unescaped)
        {
          async->result = iupStrDup(unescaped);
          free(unescaped);
        }
      }
    }
    async->completed = 1;
    return S_OK;
  }
};

static char* winWebBrowserRunJavaScriptSync(Ihandle* ih, const char* script)
{
  if (!ih->data->webviewWindow || !script)
    return nullptr;

  auto* async = new WinScriptResult();

  WCHAR* wscript = iupwinStrChar2Wide(script);
  auto* handler = new ExecuteScriptHandler(ih, async);
  HRESULT hr = ih->data->webviewWindow->ExecuteScript(wscript, handler);
  handler->Release();
  free(wscript);

  if (FAILED(hr))
  {
    delete async;
    return nullptr;
  }

  winWebBrowserWaitScript(async);

  return winWebBrowserTakeScript(async);
}

static void winWebBrowserExecCommand(Ihandle* ih, const char* cmd)
{
  if (!ih->data->webviewWindow || !cmd)
    return;

  std::string escaped_cmd = winWebBrowserEscapeJavaScript(cmd);
  std::string script = "iupRestoreSelection(); document.execCommand(" + escaped_cmd + ", false, null);";
  winWebBrowserExecuteJavascript(ih, script.c_str());
}

static void winWebBrowserExecCommandParam(Ihandle* ih, const char* cmd, const char* param)
{
  if (!ih->data->webviewWindow || !cmd)
    return;

  std::string escaped_cmd = winWebBrowserEscapeJavaScript(cmd);
  std::string escaped_param = winWebBrowserEscapeJavaScript(param);
  std::string script = "iupRestoreSelection(); document.execCommand(" + escaped_cmd + ", false, " + escaped_param + ");";
  winWebBrowserExecuteJavascript(ih, script.c_str());
}

static char* winWebBrowserQueryCommandValue(Ihandle* ih, const char* cmd)
{
  if (!ih->data->webviewWindow || !cmd)
    return nullptr;

  std::string escaped_cmd = winWebBrowserEscapeJavaScript(cmd);
  std::string script = "document.queryCommandValue(" + escaped_cmd + ");";
  return winWebBrowserRunJavaScriptSync(ih, script.c_str());
}

static char* winWebBrowserQueryCommandState(Ihandle* ih, const char* cmd)
{
  if (!ih->data->webviewWindow || !cmd)
    return nullptr;

  std::string escaped_cmd = winWebBrowserEscapeJavaScript(cmd);
  std::string script = "document.queryCommandState(" + escaped_cmd + ");";
  return winWebBrowserRunJavaScriptSync(ih, script.c_str());
}

static char* winWebBrowserQueryCommandEnabled(Ihandle* ih, const char* cmd)
{
  if (!ih->data->webviewWindow || !cmd)
    return nullptr;

  std::string escaped_cmd = winWebBrowserEscapeJavaScript(cmd);
  std::string script = "document.queryCommandEnabled(" + escaped_cmd + ");";
  return winWebBrowserRunJavaScriptSync(ih, script.c_str());
}

static int winWebBrowserSetCopyAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  winWebBrowserExecCommand(ih, "copy");
  return 0;
}

static int winWebBrowserSetCutAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  winWebBrowserExecCommand(ih, "cut");
  return 0;
}

static int winWebBrowserSetPasteAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  winWebBrowserExecCommand(ih, "paste");
  return 0;
}

static char* winWebBrowserGetPasteAttrib(Ihandle* ih)
{
  if (!ih->data->webviewWindow)
    return nullptr;

  char* result = winWebBrowserQueryCommandEnabled(ih, "paste");
  if (result)
  {
    int enabled = strcmp(result, "true") == 0;
    free(result);
    return iupStrReturnBoolean(enabled);
  }
  return iupStrReturnBoolean(0);
}

static int winWebBrowserSetSelectAllAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  winWebBrowserExecCommand(ih, "selectAll");
  return 0;
}

static int winWebBrowserSetUndoAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  winWebBrowserExecCommand(ih, "undo");
  return 0;
}

static int winWebBrowserSetRedoAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  winWebBrowserExecCommand(ih, "redo");
  return 0;
}

static int winWebBrowserSetNewAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  winWebBrowserSetHTMLAttrib(ih, "<HTML><BODY><P></P></BODY></HTML>");
  char* editable = iupAttribGet(ih, "_IUPWEB_EDITABLE");
  if (editable)
    winWebBrowserExecuteJavascript(ih, "document.body.contentEditable = 'true';");
  return 0;
}

static char* winWebBrowserGetEditableAttrib(Ihandle* ih)
{
  if (!ih->data->webviewWindow)
    return nullptr;

  char* result = winWebBrowserRunJavaScriptSync(ih, "document.body.contentEditable == 'true';");
  if (result)
  {
    int val = strcmp(result, "true") == 0;
    free(result);
    return iupStrReturnBoolean(val);
  }
  return iupStrReturnBoolean(0);
}

static int winWebBrowserSetEditableAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->webviewWindow)
    return 0;

  if (iupStrBoolean(value))
  {
    winWebBrowserExecuteJavascript(ih, "document.body.contentEditable = 'true';");
    iupAttribSet(ih, "_IUPWEB_EDITABLE", "1");
  }
  else
  {
    winWebBrowserExecuteJavascript(ih, "document.body.contentEditable = 'false';");
    iupAttribSet(ih, "_IUPWEB_EDITABLE", nullptr);
  }
  return 0;
}

static int winWebBrowserSetOpenAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  FILE* file = fopen(value, "rb");
  if (!file)
    return 0;

  fseek(file, 0, SEEK_END);
  long fileSize = ftell(file);
  fseek(file, 0, SEEK_SET);

  if (fileSize <= 0 || fileSize > 64 * 1024 * 1024)
  {
    fclose(file);
    return 0;
  }

  char* buffer = static_cast<char*>(malloc(fileSize + 1));
  if (buffer)
  {
    size_t bytesRead = fread(buffer, 1, static_cast<size_t>(fileSize), file);
    buffer[bytesRead] = '\0';
    winWebBrowserSetHTMLAttrib(ih, buffer);
    free(buffer);
  }

  fclose(file);
  return 0;
}

static int winWebBrowserSetSaveAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  char* html = winWebBrowserGetHTMLAttrib(ih);
  if (html)
  {
    FILE* file = fopen(value, "wb");
    if (file)
    {
      fwrite(html, 1, strlen(html), file);
      fclose(file);
    }
  }

  return 0;
}

static int winWebBrowserSetExecCommandAttrib(Ihandle* ih, const char* value)
{
  if (value)
    winWebBrowserExecCommand(ih, value);
  return 0;
}

static int winWebBrowserSetInsertImageAttrib(Ihandle* ih, const char* value)
{
  if (value)
    winWebBrowserExecCommandParam(ih, "insertImage", value);
  return 0;
}

static int winWebBrowserSetInsertImageFileAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  FILE* file = fopen(value, "rb");
  if (!file)
    return 0;

  fseek(file, 0, SEEK_END);
  long fileSize = ftell(file);
  fseek(file, 0, SEEK_SET);

  if (fileSize <= 0 || fileSize > 10 * 1024 * 1024)
  {
    fclose(file);
    return 0;
  }

  auto* buffer = static_cast<unsigned char*>(malloc(fileSize));
  if (buffer)
  {
    size_t bytesRead = fread(buffer, 1, static_cast<size_t>(fileSize), file);

    static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t base64_len = 4 * ((bytesRead + 2) / 3);
    char* base64 = static_cast<char*>(malloc(base64_len + 1));

    if (base64)
    {
      size_t i = 0, j = 0;
      for (; i + 2 < bytesRead; i += 3)
      {
        base64[j++] = base64_chars[(buffer[i] >> 2) & 0x3F];
        base64[j++] = base64_chars[((buffer[i] & 0x3) << 4) | ((buffer[i+1] & 0xF0) >> 4)];
        base64[j++] = base64_chars[((buffer[i+1] & 0xF) << 2) | ((buffer[i+2] & 0xC0) >> 6)];
        base64[j++] = base64_chars[buffer[i+2] & 0x3F];
      }

      if (i < bytesRead)
      {
        base64[j++] = base64_chars[(buffer[i] >> 2) & 0x3F];
        if (i + 1 < bytesRead)
        {
          base64[j++] = base64_chars[((buffer[i] & 0x3) << 4) | ((buffer[i+1] & 0xF0) >> 4)];
          base64[j++] = base64_chars[((buffer[i+1] & 0xF) << 2)];
        }
        else
        {
          base64[j++] = base64_chars[((buffer[i] & 0x3) << 4)];
          base64[j++] = '=';
        }
        base64[j++] = '=';
      }
      base64[j] = '\0';

      const char* ext = strrchr(value, '.');
      const char* mime = "image/png";
      if (ext)
      {
        if (iupStrEqualNoCase(ext, ".jpg") || iupStrEqualNoCase(ext, ".jpeg"))
          mime = "image/jpeg";
        else if (iupStrEqualNoCase(ext, ".gif"))
          mime = "image/gif";
        else if (iupStrEqualNoCase(ext, ".webp"))
          mime = "image/webp";
      }

      size_t dataUrlSize = strlen(mime) + base64_len + 20;
      char* dataUrl = static_cast<char*>(malloc(dataUrlSize));
      if (dataUrl)
      {
        snprintf(dataUrl, dataUrlSize, "data:%s;base64,%s", mime, base64);
        winWebBrowserSetInsertImageAttrib(ih, dataUrl);
        free(dataUrl);
      }

      free(base64);
    }

    free(buffer);
  }

  fclose(file);
  return 0;
}

static int winWebBrowserSetCreateLinkAttrib(Ihandle* ih, const char* value)
{
  if (value)
    winWebBrowserExecCommandParam(ih, "createLink", value);
  return 0;
}

static int winWebBrowserSetInsertTextAttrib(Ihandle* ih, const char* value)
{
  if (value)
    winWebBrowserExecCommandParam(ih, "insertText", value);
  return 0;
}

static int winWebBrowserSetInsertHtmlAttrib(Ihandle* ih, const char* value)
{
  if (value)
    winWebBrowserExecCommandParam(ih, "insertHTML", value);
  return 0;
}

static char* winWebBrowserGetCommandStateAttrib(Ihandle* ih)
{
  char* cmd = iupAttribGet(ih, "COMMAND");
  if (cmd)
  {
    const char* js_cmd = cmd;

    if (iupStrEqualNoCase(cmd, "FONTNAME"))
      js_cmd = "fontName";
    else if (iupStrEqualNoCase(cmd, "FONTSIZE"))
      js_cmd = "fontSize";
    else if (iupStrEqualNoCase(cmd, "FORMATBLOCK"))
      js_cmd = "formatBlock";
    else if (iupStrEqualNoCase(cmd, "FORECOLOR"))
      js_cmd = "foreColor";
    else if (iupStrEqualNoCase(cmd, "BACKCOLOR"))
      js_cmd = "backColor";

    char* result = winWebBrowserQueryCommandState(ih, js_cmd);
    if (result)
    {
      int val = strcmp(result, "true") == 0;
      free(result);
      return iupStrReturnBoolean(val);
    }
  }
  return iupStrReturnBoolean(0);
}

static char* winWebBrowserGetCommandEnabledAttrib(Ihandle* ih)
{
  char* cmd = iupAttribGet(ih, "COMMAND");
  if (cmd)
  {
    const char* js_cmd = cmd;

    if (iupStrEqualNoCase(cmd, "FONTNAME"))
      js_cmd = "fontName";
    else if (iupStrEqualNoCase(cmd, "FONTSIZE"))
      js_cmd = "fontSize";
    else if (iupStrEqualNoCase(cmd, "FORMATBLOCK"))
      js_cmd = "formatBlock";
    else if (iupStrEqualNoCase(cmd, "FORECOLOR"))
      js_cmd = "foreColor";
    else if (iupStrEqualNoCase(cmd, "BACKCOLOR"))
      js_cmd = "backColor";

    char* result = winWebBrowserQueryCommandEnabled(ih, js_cmd);
    if (result)
    {
      int val = strcmp(result, "true") == 0;
      free(result);
      return iupStrReturnBoolean(val);
    }
  }
  return iupStrReturnBoolean(0);
}

static char* winWebBrowserGetCommandValueAttrib(Ihandle* ih)
{
  char* cmd = iupAttribGet(ih, "COMMAND");
  if (cmd)
  {
    const char* js_cmd = cmd;

    if (iupStrEqualNoCase(cmd, "FONTNAME"))
      js_cmd = "fontName";
    else if (iupStrEqualNoCase(cmd, "FONTSIZE"))
      js_cmd = "fontSize";
    else if (iupStrEqualNoCase(cmd, "FORMATBLOCK"))
      js_cmd = "formatBlock";
    else if (iupStrEqualNoCase(cmd, "FORECOLOR"))
      js_cmd = "foreColor";
    else if (iupStrEqualNoCase(cmd, "BACKCOLOR"))
      js_cmd = "backColor";

    char* result = winWebBrowserQueryCommandValue(ih, js_cmd);
    if (result)
    {
      char* ret = iupStrReturnStr(result);
      free(result);
      return ret;
    }
  }
  return iupStrReturnStr("");
}

static char* winWebBrowserGetCommandTextAttrib(Ihandle* ih)
{
  char* cmd = iupAttribGet(ih, "COMMAND");
  if (cmd)
  {
    const char* js_cmd = cmd;

    if (iupStrEqualNoCase(cmd, "FONTNAME"))
      js_cmd = "fontName";
    else if (iupStrEqualNoCase(cmd, "FONTSIZE"))
      js_cmd = "fontSize";
    else if (iupStrEqualNoCase(cmd, "FORMATBLOCK"))
      js_cmd = "formatBlock";
    else if (iupStrEqualNoCase(cmd, "FORECOLOR"))
      js_cmd = "foreColor";
    else if (iupStrEqualNoCase(cmd, "BACKCOLOR"))
      js_cmd = "backColor";

    char* result = winWebBrowserQueryCommandValue(ih, js_cmd);
    if (result)
    {
      char* ret = iupStrReturnStr(result);
      free(result);
      return ret;
    }
  }
  return iupStrReturnStr("");
}

static char* winWebBrowserGetFontNameAttrib(Ihandle* ih)
{
  char* result = winWebBrowserQueryCommandValue(ih, "fontName");
  if (result)
  {
    char* ret = iupStrReturnStr(result);
    free(result);
    return ret;
  }
  return nullptr;
}

static int winWebBrowserSetFontNameAttrib(Ihandle* ih, const char* value)
{
  if (value)
    winWebBrowserExecCommandParam(ih, "fontName", value);
  return 0;
}

static char* winWebBrowserGetFontSizeAttrib(Ihandle* ih)
{
  char* result = winWebBrowserQueryCommandValue(ih, "fontSize");
  if (result)
  {
    char* ret = iupStrReturnStr(result);
    free(result);
    return ret;
  }
  return nullptr;
}

static int winWebBrowserSetFontSizeAttrib(Ihandle* ih, const char* value)
{
  if (value)
    winWebBrowserExecCommandParam(ih, "fontSize", value);
  return 0;
}

static char* winWebBrowserGetFormatBlockAttrib(Ihandle* ih)
{
  char* result = winWebBrowserQueryCommandValue(ih, "formatBlock");
  if (result)
  {
    char* ret = iupStrReturnStr(result);
    free(result);
    return ret;
  }
  return nullptr;
}

static int winWebBrowserSetFormatBlockAttrib(Ihandle* ih, const char* value)
{
  if (value)
    winWebBrowserExecCommandParam(ih, "formatBlock", value);
  return 0;
}

static char* winWebBrowserGetForeColorAttrib(Ihandle* ih)
{
  char* result = winWebBrowserQueryCommandValue(ih, "foreColor");
  if (result)
  {
    char* ret = iupStrReturnStr(result);
    free(result);
    return ret;
  }
  return nullptr;
}

static int winWebBrowserSetForeColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
  {
    char rgb_color[32];
    snprintf(rgb_color, sizeof(rgb_color), "rgb(%d,%d,%d)", r, g, b);
    winWebBrowserExecCommandParam(ih, "foreColor", rgb_color);
  }
  return 0;
}

static char* winWebBrowserGetBackColorAttrib(Ihandle* ih)
{
  char* result = winWebBrowserQueryCommandValue(ih, "backColor");
  if (result)
  {
    char* ret = iupStrReturnStr(result);
    free(result);
    return ret;
  }
  return nullptr;
}

static int winWebBrowserSetBackColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
  {
    char rgb_color[32];
    snprintf(rgb_color, sizeof(rgb_color), "rgb(%d,%d,%d)", r, g, b);
    winWebBrowserExecCommandParam(ih, "backColor", rgb_color);
  }
  return 0;
}

static char* winWebBrowserGetInnerTextAttrib(Ihandle* ih)
{
  char* element_id = iupAttribGet(ih, "ELEMENT_ID");
  if (!element_id)
    return nullptr;

  std::string escaped_id = winWebBrowserEscapeJavaScript(element_id);
  std::string script = "document.getElementById(" + escaped_id + ")?.innerText || '';";
  char* result = winWebBrowserRunJavaScriptSync(ih, script.c_str());
  if (result)
  {
    char* ret = iupStrReturnStr(result);
    free(result);
    return ret;
  }
  return nullptr;
}

static int winWebBrowserSetInnerTextAttrib(Ihandle* ih, const char* value)
{
  char* element_id = iupAttribGet(ih, "ELEMENT_ID");
  if (!element_id)
    return 0;

  std::string escaped_id = winWebBrowserEscapeJavaScript(element_id);
  std::string escaped_value = winWebBrowserEscapeJavaScript(value ? value : "");
  std::string script = "if(document.getElementById(" + escaped_id + ")) document.getElementById(" +
                       escaped_id + ").innerText = " + escaped_value + ";";
  winWebBrowserExecuteJavascript(ih, script.c_str());
  return 0;
}

static char* winWebBrowserGetJavascriptAttrib(Ihandle* ih)
{
  return iupAttribGet(ih, "_IUPWEB_JS_RESULT");
}

static int winWebBrowserSetJavascriptAttrib(Ihandle* ih, const char* value)
{
  iupAttribSet(ih, "_IUPWEB_JS_RESULT", nullptr);
  if (!value)
    return 0;

  char* result = winWebBrowserRunJavaScriptSync(ih, value);
  if (result)
  {
    iupAttribSetStr(ih, "_IUPWEB_JS_RESULT", result);
    free(result);
  }
  return 0;
}

static char* winWebBrowserGetAttributeAttrib(Ihandle* ih)
{
  char* element_id = iupAttribGet(ih, "ELEMENT_ID");
  char* attr_name = iupAttribGet(ih, "ATTRIBUTE_NAME");
  if (!element_id || !attr_name)
    return nullptr;

  std::string escaped_id = winWebBrowserEscapeJavaScript(element_id);
  std::string escaped_attr = winWebBrowserEscapeJavaScript(attr_name);
  std::string script = "document.getElementById(" + escaped_id + ")?.getAttribute(" +
                       escaped_attr + ") || '';";
  char* result = winWebBrowserRunJavaScriptSync(ih, script.c_str());
  if (result)
  {
    char* ret = iupStrReturnStr(result);
    free(result);
    return ret;
  }
  return nullptr;
}

static int winWebBrowserSetAttributeAttrib(Ihandle* ih, const char* value)
{
  char* element_id = iupAttribGet(ih, "ELEMENT_ID");
  char* attr_name = iupAttribGet(ih, "ATTRIBUTE_NAME");
  if (!element_id || !attr_name)
    return 0;

  std::string escaped_id = winWebBrowserEscapeJavaScript(element_id);
  std::string escaped_attr = winWebBrowserEscapeJavaScript(attr_name);
  std::string escaped_value = winWebBrowserEscapeJavaScript(value ? value : "");
  std::string script = "if(document.getElementById(" + escaped_id + ")) document.getElementById(" +
                       escaped_id + ").setAttribute(" + escaped_attr + ", " + escaped_value + ");";
  winWebBrowserExecuteJavascript(ih, script.c_str());
  return 0;
}

static char* winWebBrowserGetDirtyAttrib(Ihandle* ih)
{
  if (iupAttribGet(ih, "_IUPWEB_DIRTY"))
    return const_cast<char*>("YES");

  char* result = winWebBrowserRunJavaScriptSync(ih, "window.iupGetDirtyFlag ? window.iupGetDirtyFlag() : false;");
  if (result)
  {
    int dirty = strcmp(result, "true") == 0;
    free(result);
    if (dirty)
      iupAttribSet(ih, "_IUPWEB_DIRTY", "1");
    return iupStrReturnBoolean(dirty);
  }
  return const_cast<char*>("NO");
}

static int winWebBrowserSetFindAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  std::string escaped_text = winWebBrowserEscapeJavaScript(value);
  std::string script = "window.find(" + escaped_text + ");";
  winWebBrowserExecuteJavascript(ih, script.c_str());
  return 0;
}

static int winWebBrowserSetPrintPreviewAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  return winWebBrowserSetPrintAttrib(ih, nullptr);
}

static void winWebBrowserReleaseHost(Ihandle* ih)
{
  if (ih->data->hwnd)
  {
    DestroyWindow(ih->data->hwnd);
    ih->data->hwnd = nullptr;
  }

#ifdef IUPWEB_HOSTED
  iupwebHostUnMap(ih);
#else
  ih->handle = nullptr;
#endif
}

#ifdef IUPWEB_HOSTED
static HWND winWebBrowserParkingWindow(void)
{
  static HWND parking = nullptr;
  if (!parking)
    parking = CreateWindowEx(0, TEXT("STATIC"), TEXT(""), WS_POPUP, 0, 0, 0, 0,
                             nullptr, nullptr, static_cast<HINSTANCE>(GetModuleHandle(nullptr)), nullptr);
  return parking;
}

extern "C" void iupwebHostSetParent(Ihandle* ih, void* parent)
{
  if (!ih->data || !ih->data->hwnd)
    return;

  HWND hwnd = parent ? static_cast<HWND>(parent) : winWebBrowserParkingWindow();
  if (GetParent(ih->data->hwnd) == hwnd)
    return;

  if (!parent)
    ShowWindow(ih->data->hwnd, SW_HIDE);

  SetParent(ih->data->hwnd, hwnd);

  if (ih->data->webviewController)
    ih->data->webviewController->NotifyParentWindowPositionChanged();
}

extern "C" void iupwebHostSetBounds(Ihandle* ih, int x, int y, int width, int height, int clip_x, int clip_y, int clip_width, int clip_height)
{
  int visible = clip_width > 0 && clip_height > 0;

  if (!ih->data || !ih->data->hwnd)
    return;

  ih->data->viewBounds.left = x - clip_x;
  ih->data->viewBounds.top = y - clip_y;
  ih->data->viewBounds.right = x - clip_x + width;
  ih->data->viewBounds.bottom = y - clip_y + height;
  ih->data->hasViewBounds = 1;

  SetWindowPos(ih->data->hwnd, nullptr, clip_x, clip_y, clip_width, clip_height,
               SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER | (visible ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
  winWebBrowserApplyBounds(ih);
}
#endif

static int winWebBrowserMapMethod(Ihandle* ih)
{
  if (!g_comInitialized)
  {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hr))
    {
      g_comInitialized = 1;
    }
    else if (hr == RPC_E_CHANGED_MODE)
    {
      g_comInitialized = 1;
    }
    else
    {
      return IUP_ERROR;
    }
  }

#if defined(IUPWEB_HOSTED)
  HWND parent = static_cast<HWND>(iupwebHostMap(ih));
  if (!parent && ih->handle)
    parent = winWebBrowserParkingWindow();
  DWORD style = WS_CHILD;
#elif defined(IUP_USE_WINUI)
  Ihandle* dialog = IupGetDialog(ih);
  HWND parent = dialog ? static_cast<HWND>(dialog->handle) : nullptr;
  DWORD style = WS_CHILD | WS_VISIBLE;
#else
  HWND parent = static_cast<HWND>(iupChildTreeGetNativeParentHandle(ih));
  DWORD style = WS_CHILD | WS_VISIBLE;
#endif
  if (!parent)
  {
    winWebBrowserReleaseHost(ih);
    return IUP_ERROR;
  }

  HWND hwnd = CreateWindowEx(0, TEXT("STATIC"), TEXT(""), style,
                              ih->x, ih->y, ih->currentwidth, ih->currentheight,
                              parent, nullptr, static_cast<HINSTANCE>(GetModuleHandle(nullptr)), nullptr);

  if (!hwnd)
  {
    winWebBrowserReleaseHost(ih);
    return IUP_ERROR;
  }

  ih->data->hwnd = hwnd;
#ifndef IUPWEB_HOSTED
  ih->handle = hwnd;
#endif

  SetProp(hwnd, TEXT("IUP_WEBBROWSER_IH"), reinterpret_cast<HANDLE>(ih));
  ih->data->oldWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WebBrowserWndProc)));

  HRESULT hr = IupWebView2LoaderInit();
  if (FAILED(hr))
  {
    winWebBrowserReleaseHost(ih);
    return IUP_ERROR;
  }

  CreateCoreWebView2EnvironmentWithOptionsFunc createEnvFunc = IupWebView2LoaderGetCreateEnvironmentFunc();
  if (!createEnvFunc)
  {
    winWebBrowserReleaseHost(ih);
    return IUP_ERROR;
  }

  const wchar_t* runtimePath = IupWebView2LoaderGetRuntimePath();

  std::wstring userDataFolder = GetWebView2UserDataFolder();

  auto* state = new WinInitState();

  auto* envHandler = new CreateEnvironmentHandler(ih, hwnd, state);
  hr = createEnvFunc(runtimePath, userDataFolder.empty() ? nullptr : userDataFolder.c_str(), nullptr, envHandler);
  envHandler->Release();

  if (FAILED(hr))
  {
    delete state;
    winWebBrowserReleaseHost(ih);
    return IUP_ERROR;
  }

  MSG msg;
  int loopCount = 0;
  while (state->complete == 0 && loopCount < IUPWIN_WEBVIEW_INIT_TIMEOUT)
  {
    if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    else
    {
      Sleep(10);
    }
    loopCount++;
  }

  if (state->complete == 0)
  {
    state->abandoned = 1;
    winWebBrowserReleaseHost(ih);
    return IUP_ERROR;
  }

  if (state->complete < 0)
  {
    delete state;
    winWebBrowserReleaseHost(ih);
    return IUP_ERROR;
  }

  delete state;

  RECT rect;
  GetClientRect(hwnd, &rect);
  SendMessage(hwnd, WM_SIZE, SIZE_RESTORED, MAKELPARAM(rect.right, rect.bottom));

  return IUP_NOERROR;
}

static void winWebBrowserUnMapMethod(Ihandle* ih)
{
#ifdef IUP_USE_WINUI
  iupwinuiHwndHostRemove(ih);
#endif

  if (ih->data->webviewWindow)
  {
    ih->data->webviewWindow->remove_NavigationStarting(ih->data->navigationStartingToken);
    ih->data->webviewWindow->remove_NavigationCompleted(ih->data->navigationCompletedToken);
    ih->data->webviewWindow->remove_NewWindowRequested(ih->data->newWindowRequestedToken);
    ih->data->webviewWindow->remove_HistoryChanged(ih->data->historyChangedToken);
    ih->data->webviewWindow->remove_WebMessageReceived(ih->data->webMessageReceivedToken);

    ih->data->webviewWindow->Release();
    ih->data->webviewWindow = nullptr;
  }

  if (ih->data->webviewController)
  {
    ih->data->webviewController->Close();
    ih->data->webviewController->Release();
    ih->data->webviewController = nullptr;
  }

  if (ih->data->hwnd && ih->data->oldWndProc)
  {
    SetWindowLongPtr(ih->data->hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(ih->data->oldWndProc));
    ih->data->oldWndProc = nullptr;
  }
  if (ih->data->hwnd)
    RemoveProp(ih->data->hwnd, TEXT("IUP_WEBBROWSER_IH"));

  winWebBrowserReleaseHost(ih);
}

static void winWebBrowserComputeNaturalSizeMethod(Ihandle* ih, int* w, int* h, int* children_expand)
{
  int natural_w = 0, natural_h = 0;
  (void)children_expand;

  iupdrvFontGetCharSize(ih, &natural_w, &natural_h);

  *w = natural_w;
  *h = natural_h;
}

static void winWebBrowserLayoutUpdateMethod(Ihandle* ih)
{
#ifdef IUPWEB_HOSTED
  iupwebHostLayoutUpdate(ih);
#else
  iupdrvBaseLayoutUpdateMethod(ih);
#endif

  winWebBrowserApplyBounds(ih);
}

static int winWebBrowserCreateMethod(Ihandle* ih, void** params)
{
  (void)params;

  ih->data = iupALLOCCTRLDATA();
  ih->data->webviewController = nullptr;
  ih->data->webviewWindow = nullptr;
  ih->data->loadStatus = WEBVIEW_STATUS_COMPLETED;
  ih->data->hwnd = nullptr;
  ih->data->hasViewBounds = 0;
  ih->data->oldWndProc = nullptr;

  IupSetAttribute(ih, "BORDER", "NO");
  IupSetAttribute(ih, "CANFOCUS", "YES");

  ih->expand = IUP_EXPAND_BOTH;

  return IUP_NOERROR;
}

static void winWebBrowserDestroyMethod(Ihandle* ih)
{
  if (ih->data)
  {
    if (ih->data->hwnd)
      winWebBrowserUnMapMethod(ih);
  }
}

Iclass* iupWebBrowserNewClass(void)
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
  ic->Create = winWebBrowserCreateMethod;
  ic->Destroy = winWebBrowserDestroyMethod;
  ic->Map = winWebBrowserMapMethod;
  ic->UnMap = winWebBrowserUnMapMethod;
  ic->LayoutUpdate = winWebBrowserLayoutUpdateMethod;
  ic->ComputeNaturalSize = winWebBrowserComputeNaturalSizeMethod;

  iupClassRegisterCallback(ic, "NEWWINDOW_CB", "s");
  iupClassRegisterCallback(ic, "NAVIGATE_CB", "s");
  iupClassRegisterCallback(ic, "ERROR_CB", "s");
  iupClassRegisterCallback(ic, "COMPLETED_CB", "s");
  iupClassRegisterCallback(ic, "UPDATE_CB", "");

  iupBaseRegisterCommonAttrib(ic);

  iupBaseRegisterVisualAttrib(ic);

  iupClassRegisterAttribute(ic, "BGCOLOR", nullptr, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "VALUE", winWebBrowserGetValueAttrib, winWebBrowserSetValueAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKFORWARD", nullptr, winWebBrowserSetBackForwardAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GOBACK", nullptr, winWebBrowserSetGoBackAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GOFORWARD", nullptr, winWebBrowserSetGoForwardAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STOP", nullptr, winWebBrowserSetStopAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RELOAD", nullptr, winWebBrowserSetReloadAttrib, nullptr, nullptr, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HTML", winWebBrowserGetHTMLAttrib, winWebBrowserSetHTMLAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STATUS", winWebBrowserGetStatusAttrib, nullptr, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE|IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ZOOM", winWebBrowserGetZoomAttrib, winWebBrowserSetZoomAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PRINT", nullptr, winWebBrowserSetPrintAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CANGOBACK", winWebBrowserGetCanGoBackAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CANGOFORWARD", winWebBrowserGetCanGoForwardAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "EDITABLE", winWebBrowserGetEditableAttrib, winWebBrowserSetEditableAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NEW", nullptr, winWebBrowserSetNewAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OPENFILE", nullptr, winWebBrowserSetOpenAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SAVEFILE", nullptr, winWebBrowserSetSaveAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "UNDO", nullptr, winWebBrowserSetUndoAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "REDO", nullptr, winWebBrowserSetRedoAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COPY", nullptr, winWebBrowserSetCopyAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUT", nullptr, winWebBrowserSetCutAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PASTE", winWebBrowserGetPasteAttrib, winWebBrowserSetPasteAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTALL", nullptr, winWebBrowserSetSelectAllAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EXECCOMMAND", nullptr, winWebBrowserSetExecCommandAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTIMAGE", nullptr, winWebBrowserSetInsertImageAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTIMAGEFILE", nullptr, winWebBrowserSetInsertImageFileAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CREATELINK", nullptr, winWebBrowserSetCreateLinkAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTTEXT", nullptr, winWebBrowserSetInsertTextAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTHTML", nullptr, winWebBrowserSetInsertHtmlAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "FONTNAME", winWebBrowserGetFontNameAttrib, winWebBrowserSetFontNameAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FONTSIZE", winWebBrowserGetFontSizeAttrib, winWebBrowserSetFontSizeAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATBLOCK", winWebBrowserGetFormatBlockAttrib, winWebBrowserSetFormatBlockAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORECOLOR", winWebBrowserGetForeColorAttrib, winWebBrowserSetForeColorAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKCOLOR", winWebBrowserGetBackColorAttrib, winWebBrowserSetBackColorAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "COMMAND", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDSHOWUI", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDSTATE", winWebBrowserGetCommandStateAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDENABLED", winWebBrowserGetCommandEnabledAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDTEXT", winWebBrowserGetCommandTextAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDVALUE", winWebBrowserGetCommandValueAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ELEMENT_ID", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INNERTEXT", winWebBrowserGetInnerTextAttrib, winWebBrowserSetInnerTextAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ATTRIBUTE_NAME", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ATTRIBUTE", winWebBrowserGetAttributeAttrib, winWebBrowserSetAttributeAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "JAVASCRIPT", winWebBrowserGetJavascriptAttrib, winWebBrowserSetJavascriptAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BACKCOUNT", winWebBrowserGetBackCountAttrib, nullptr, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORWARDCOUNT", winWebBrowserGetForwardCountAttrib, nullptr, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "ITEMHISTORY", winWebBrowserGetItemHistoryAttrib, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DIRTY", winWebBrowserGetDirtyAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FIND", nullptr, winWebBrowserSetFindAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PRINTPREVIEW", nullptr, winWebBrowserSetPrintPreviewAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  return ic;
}
