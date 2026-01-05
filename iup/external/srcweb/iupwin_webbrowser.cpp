/** \file
 * \brief WebView2 Web Browser Control for Windows 10/11 with built-in loader
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>
#include <string>
#include <vector>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_register.h"
#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_childtree.h"
#include "iup_stdcontrols.h"
#include "iup_str.h"
#include "iup_layout.h"
#include "iup_dialog.h"
#include "iup_webbrowser.h"
#include "iup_drv.h"
#include "iup_drvfont.h"

#include "iupwin_webbrowser.h"

extern "C" {
  IUP_DRV_API WCHAR* iupwinStrChar2Wide(const char* str);
  IUP_DRV_API char* iupwinStrWide2Char(const WCHAR* wstr);
}

#ifndef __IID_DEFINED__
#define __IID_DEFINED__
typedef struct _GUID IID;
#endif

/* ========================================================================== */
/* WebView2 Built-in Loader Implementation                                   */
/* ========================================================================== */

static HMODULE g_embeddedBrowserModule = NULL;
static std::wstring g_runtimePath;
static CreateWebViewEnvironmentWithOptionsInternalFunc g_createEnvInternalFunc = NULL;
static WebView2RuntimeType g_runtimeType = WEBVIEW2_RUNTIME_TYPE_INSTALLED;

static std::wstring FindLatestWebView2Version(const std::wstring& basePath)
{
  std::wstring runtimePath = basePath + L"\\Microsoft\\EdgeWebView\\Application";

  if (GetFileAttributesW(runtimePath.c_str()) == INVALID_FILE_ATTRIBUTES)
    return L"";

  WIN32_FIND_DATAW findData;
  std::wstring searchPath = runtimePath + L"\\*";
  HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);

  if (hFind == INVALID_HANDLE_VALUE)
    return L"";

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

        versionNum = ((ULONGLONG)parts[0] << 48) | ((ULONGLONG)parts[1] << 32) |
          ((ULONGLONG)parts[2] << 16) | (ULONGLONG)parts[3];

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

  return L"";
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

  return L"";
}

static std::wstring GetWebView2UserDataFolder()
{
  wchar_t localAppData[MAX_PATH];
  wchar_t exePath[MAX_PATH];

  if (GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH) == 0)
    return L"";

  if (GetModuleFileNameW(NULL, exePath, MAX_PATH) == 0)
    return L"";

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
  if (g_createEnvInternalFunc == NULL)
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

  SetDllDirectoryW(g_runtimePath.c_str());

  g_embeddedBrowserModule = LoadLibraryW(loaderDllPath.c_str());

  if (!g_embeddedBrowserModule)
  {
    DWORD error = GetLastError();
    SetDllDirectoryW(NULL);
    IupSetGlobal("IUP_WEBBROWSER_MISSING_LIB", "EmbeddedBrowserWebView.dll");
    return HRESULT_FROM_WIN32(error);
  }

  g_createEnvInternalFunc = (CreateWebViewEnvironmentWithOptionsInternalFunc)
      GetProcAddress(g_embeddedBrowserModule, "CreateWebViewEnvironmentWithOptionsInternal");

  if (!g_createEnvInternalFunc)
  {
    DWORD error = GetLastError();
    FreeLibrary(g_embeddedBrowserModule);
    g_embeddedBrowserModule = NULL;
    SetDllDirectoryW(NULL);
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
    g_embeddedBrowserModule = NULL;
    SetDllDirectoryW(NULL);
  }

  g_createEnvInternalFunc = NULL;
  g_runtimePath.clear();
}

CreateCoreWebView2EnvironmentWithOptionsFunc IupWebView2LoaderGetCreateEnvironmentFunc(void)
{
  return CreateCoreWebView2EnvironmentWithOptions;
}

const wchar_t* IupWebView2LoaderGetRuntimePath(void)
{
  return g_runtimePath.empty() ? NULL : g_runtimePath.c_str();
}

/* ========================================================================== */
/* WebBrowser Control Implementation                                         */
/* ========================================================================== */

static int g_comInitialized = 0;

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
  int initComplete;
  WNDPROC oldWndProc;
};

static LRESULT CALLBACK WebBrowserWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  Ihandle* ih = (Ihandle*)GetProp(hwnd, "IUP_WEBBROWSER_IH");

  if (msg == WM_SIZE)
  {
    WORD width = LOWORD(lParam);
    WORD height = HIWORD(lParam);

    if (ih && ih->data && ih->data->webviewController)
    {
      RECT bounds;
      bounds.left = 0;
      bounds.top = 0;
      bounds.right = width;
      bounds.bottom = height;

      ih->data->webviewController->put_Bounds(bounds);
    }
  }

  if (ih && ih->data && ih->data->oldWndProc)
    return CallWindowProc(ih->data->oldWndProc, hwnd, msg, wParam, lParam);
  else
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

template <typename T>
class EventHandler : public T
{
protected:
  ULONG refCount;
  Ihandle* ih;

public:
  EventHandler(Ihandle* handle) : refCount(1), ih(handle) {}
  virtual ~EventHandler() {}

  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override
  {
    if (riid == IID_IUnknown || riid == __mingw_uuidof<T>())
    {
      *ppvObject = static_cast<T*>(this);
      AddRef();
      return S_OK;
    }
    *ppvObject = nullptr;
    return E_NOINTERFACE;
  }

  ULONG STDMETHODCALLTYPE AddRef() override
  {
    return InterlockedIncrement(&refCount);
  }

  ULONG STDMETHODCALLTYPE Release() override
  {
    ULONG count = InterlockedDecrement(&refCount);
    if (count == 0)
      delete this;
    return count;
  }
};

static std::string winWebBrowserEscapeJavaScript(const char* str)
{
  if (!str)
    return "null";

  std::string result = "\"";
  for (const char* p = str; *p; p++)
  {
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
        if ((unsigned char)*p < 32)
        {
          char buf[7];
          snprintf(buf, sizeof(buf), "\\u%04x", (unsigned char)*p);
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

class NavigationStartingHandler : public EventHandler<ICoreWebView2NavigationStartingEventHandler>
{
public:
  NavigationStartingHandler(Ihandle* handle) : EventHandler(handle) {}

  HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args) override
  {
    iupAttribSet(ih, "_IUPWEB_FAILED", NULL);
    ih->data->loadStatus = WEBVIEW_STATUS_LOADING;

    if (iupAttribGet(ih, "_IUPWEB_IGNORE_NAVIGATE"))
      return S_OK;

    IFns cb = (IFns)IupGetCallback(ih, "NAVIGATE_CB");
    if (cb)
    {
      LPWSTR uri = NULL;
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

class NavigationCompletedHandler : public EventHandler<ICoreWebView2NavigationCompletedEventHandler>
{
public:
  NavigationCompletedHandler(Ihandle* handle) : EventHandler(handle) {}

  HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) override;
};

class NewWindowHandler : public EventHandler<ICoreWebView2NewWindowRequestedEventHandler>
{
public:
  NewWindowHandler(Ihandle* handle) : EventHandler(handle) {}

  HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2* sender, ICoreWebView2NewWindowRequestedEventArgs* args) override
  {
    IFns cb = (IFns)IupGetCallback(ih, "NEWWINDOW_CB");
    if (cb)
    {
      LPWSTR uri = NULL;
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

class HistoryChangedHandler : public EventHandler<ICoreWebView2HistoryChangedEventHandler>
{
public:
  HistoryChangedHandler(Ihandle* handle) : EventHandler(handle) {}

  HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2* sender, IUnknown* args) override;
};

class WebMessageReceivedHandler : public EventHandler<ICoreWebView2WebMessageReceivedEventHandler>
{
public:
  WebMessageReceivedHandler(Ihandle* handle) : EventHandler(handle) {}

  HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args) override
  {
    LPWSTR message = NULL;
    args->TryGetWebMessageAsString(&message);

    if (message)
    {
      if (wcscmp(message, L"dirty") == 0)
      {
        iupAttribSet(ih, "_IUPWEB_DIRTY", "1");
      }
      else if (wcscmp(message, L"update") == 0)
      {
        IFn update_cb = (IFn)IupGetCallback(ih, "UPDATE_CB");
        if (update_cb)
          update_cb(ih);
      }

      CoTaskMemFree(message);
    }

    return S_OK;
  }
};

class CreateWebViewHandler : public EventHandler<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>
{
private:
  HWND hwnd;

public:
  CreateWebViewHandler(Ihandle* handle, HWND window) : EventHandler(handle), hwnd(window) {}

  HRESULT STDMETHODCALLTYPE Invoke(HRESULT result, ICoreWebView2Controller* controller) override;
};

class CreateEnvironmentHandler : public EventHandler<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>
{
private:
  HWND hwnd;

public:
  CreateEnvironmentHandler(Ihandle* handle, HWND window) : EventHandler(handle), hwnd(window) {}

  HRESULT STDMETHODCALLTYPE Invoke(HRESULT result, ICoreWebView2Environment* env) override
  {
    if (FAILED(result))
    {
      ih->data->initComplete = -1;
      return result;
    }

    CreateWebViewHandler* handler = new CreateWebViewHandler(ih, hwnd);
    HRESULT hr = env->CreateCoreWebView2Controller(hwnd, handler);
    handler->Release();

    if (FAILED(hr))
      ih->data->initComplete = -1;

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

HRESULT NavigationCompletedHandler::Invoke(ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args)
{
  BOOL success = FALSE;
  args->get_IsSuccess(&success);

  if (!success)
  {
    iupAttribSet(ih, "_IUPWEB_FAILED", "1");
    ih->data->loadStatus = WEBVIEW_STATUS_FAILED;

    IFns cb = (IFns)IupGetCallback(ih, "ERROR_CB");
    if (cb)
    {
      LPWSTR uri = NULL;
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
        cb(ih, (char*)"Unknown URI");
      }
    }
  }
  else
  {
    ih->data->loadStatus = WEBVIEW_STATUS_COMPLETED;

    if (iupAttribGet(ih, "_IUPWEB_EDITABLE"))
    {
      sender->ExecuteScript(L"document.body.contentEditable = 'true';", NULL);
    }
    else
    {
      sender->ExecuteScript(L"document.body.contentEditable = 'false';", NULL);
    }

    IFns cb = (IFns)IupGetCallback(ih, "COMPLETED_CB");
    if (cb)
    {
      LPWSTR uri = NULL;
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
        cb(ih, (char*)"Unknown URI");
      }
    }
  }

  winWebBrowserUpdateHistory(ih);

  IFn update_cb = (IFn)IupGetCallback(ih, "UPDATE_CB");
  if (update_cb)
    update_cb(ih);

  return S_OK;
}

HRESULT HistoryChangedHandler::Invoke(ICoreWebView2* sender, IUnknown* args)
{
  (void)sender;
  (void)args;
  winWebBrowserUpdateHistory(ih);
  return S_OK;
}

HRESULT CreateWebViewHandler::Invoke(HRESULT result, ICoreWebView2Controller* controller)
{
  if (FAILED(result))
  {
    ih->data->initComplete = -1;
    return result;
  }

  ih->data->webviewController = controller;
  controller->AddRef();

  ICoreWebView2Controller2* controller2 = NULL;
  HRESULT hr = controller->QueryInterface(__mingw_uuidof<ICoreWebView2Controller2>(), (void**)&controller2);
  if (SUCCEEDED(hr) && controller2)
  {
    controller2->put_DefaultBackgroundColor(RGB(255, 255, 255));
    controller2->Release();
  }

  controller->get_CoreWebView2(&ih->data->webviewWindow);
  if (ih->data->webviewWindow)
    ih->data->webviewWindow->AddRef();

  RECT bounds;
  GetClientRect(hwnd, &bounds);
  controller->put_Bounds(bounds);
  controller->put_IsVisible(TRUE);

  ICoreWebView2* webview = ih->data->webviewWindow;

  ICoreWebView2Settings* settings = NULL;
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

  NavigationStartingHandler* navStartHandler = new NavigationStartingHandler(ih);
  webview->add_NavigationStarting(navStartHandler, &ih->data->navigationStartingToken);
  navStartHandler->Release();

  NavigationCompletedHandler* navCompletedHandler = new NavigationCompletedHandler(ih);
  webview->add_NavigationCompleted(navCompletedHandler, &ih->data->navigationCompletedToken);
  navCompletedHandler->Release();

  NewWindowHandler* newWindowHandler = new NewWindowHandler(ih);
  webview->add_NewWindowRequested(newWindowHandler, &ih->data->newWindowRequestedToken);
  newWindowHandler->Release();

  HistoryChangedHandler* historyHandler = new HistoryChangedHandler(ih);
  webview->add_HistoryChanged(historyHandler, &ih->data->historyChangedToken);
  historyHandler->Release();

  WebMessageReceivedHandler* messageHandler = new WebMessageReceivedHandler(ih);
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

  webview->AddScriptToExecuteOnDocumentCreated(updateScript, NULL);

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

  webview->AddScriptToExecuteOnDocumentCreated(scrollbarStyleScript, NULL);

  ih->data->initComplete = 1;

  return S_OK;
}

static int winWebBrowserSetValueAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->webviewWindow || !value)
    return 0;

  iupAttribSet(ih, "_IUPWEB_DIRTY", NULL);
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

  iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", NULL);

  return 0;
}

static char* winWebBrowserGetValueAttrib(Ihandle* ih)
{
  if (!ih->data->webviewWindow)
    return NULL;

  LPWSTR uri = NULL;
  ih->data->webviewWindow->get_Source(&uri);

  if (uri)
  {
    char* urlString = iupwinStrWide2Char(uri);
    char* retStr = iupStrReturnStr(urlString);
    free(urlString);
    CoTaskMemFree(uri);
    return retStr;
  }

  return NULL;
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
  iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", NULL);
  iupAttribSet(ih, "_IUPWEB_DIRTY", NULL);

  return 0;
}

static char* winWebBrowserUnescapeJSON(const char* json_str)
{
  if (!json_str)
    return NULL;

  size_t len = strlen(json_str);
  char* result = (char*)malloc(len + 1);
  if (!result)
    return NULL;

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
              if (code < 0x80)
              {
                result[j++] = (char)code;
              }
              else if (code < 0x800)
              {
                result[j++] = (char)(0xC0 | (code >> 6));
                result[j++] = (char)(0x80 | (code & 0x3F));
              }
              else
              {
                result[j++] = (char)(0xE0 | (code >> 12));
                result[j++] = (char)(0x80 | ((code >> 6) & 0x3F));
                result[j++] = (char)(0x80 | (code & 0x3F));
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
    return NULL;

  const char* script = "document.documentElement.outerHTML;";
  WCHAR* wscript = iupwinStrChar2Wide(script);

  char* result = NULL;
  int complete = 0;

  class GetHTMLHandler : public EventHandler<ICoreWebView2ExecuteScriptCompletedHandler>
  {
  private:
    char** result;
    int* complete;

  public:
    GetHTMLHandler(Ihandle* handle, char** res, int* comp) : EventHandler(handle), result(res), complete(comp) {}

    HRESULT STDMETHODCALLTYPE Invoke(HRESULT errorCode, LPCWSTR resultObjectAsJson) override
    {
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
            *result = iupStrDup(unescaped);
            free(unescaped);
          }
        }
      }
      *complete = 1;
      return S_OK;
    }
  };

  GetHTMLHandler* handler = new GetHTMLHandler(ih, &result, &complete);
  HRESULT hr = ih->data->webviewWindow->ExecuteScript(wscript, handler);
  handler->Release();
  free(wscript);

  if (FAILED(hr))
    return NULL;

  MSG msg;
  int loopCount = 0;
  while (complete == 0 && loopCount < 1000)
  {
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
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

  if (result)
  {
    char* ret = iupStrReturnStr(result);
    free(result);
    return ret;
  }

  return NULL;
}

static char* winWebBrowserGetStatusAttrib(Ihandle* ih)
{
  if (!ih->data->webviewWindow)
    return (char*)"COMPLETED";

  switch (ih->data->loadStatus)
  {
    case WEBVIEW_STATUS_LOADING:
      return (char*)"LOADING";
    case WEBVIEW_STATUS_FAILED:
      return (char*)"FAILED";
    case WEBVIEW_STATUS_COMPLETED:
    default:
      return (char*)"COMPLETED";
  }
}

static int winWebBrowserSetZoomAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->webviewController)
    return 0;

  int zoom = 100;
  if (iupStrToInt(value, &zoom))
  {
    double zoomFactor = (double)zoom / 100.0;
    ih->data->webviewController->put_ZoomFactor(zoomFactor);
  }

  return 0;
}

static char* winWebBrowserGetZoomAttrib(Ihandle* ih)
{
  if (!ih->data->webviewController)
    return NULL;

  double zoomFactor = 1.0;
  ih->data->webviewController->get_ZoomFactor(&zoomFactor);

  int zoom = (int)(zoomFactor * 100);
  return iupStrReturnInt(zoom);
}

static int winWebBrowserSetPrintAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->webviewWindow)
    return 0;

  (void)value;
  ih->data->webviewWindow->ExecuteScript(L"window.print();", NULL);
  return 0;
}

static char* winWebBrowserGetCanGoBackAttrib(Ihandle* ih)
{
  if (!ih->data->webviewWindow)
    return NULL;

  BOOL canGoBack = FALSE;
  ih->data->webviewWindow->get_CanGoBack(&canGoBack);
  return iupStrReturnBoolean(canGoBack);
}

static char* winWebBrowserGetCanGoForwardAttrib(Ihandle* ih)
{
  if (!ih->data->webviewWindow)
    return NULL;

  BOOL canGoForward = FALSE;
  ih->data->webviewWindow->get_CanGoForward(&canGoForward);
  return iupStrReturnBoolean(canGoForward);
}

static char* winWebBrowserGetBackCountAttrib(Ihandle* ih)
{
  if (!ih->data->webviewWindow)
    return iupStrReturnInt(0);

  /* WebView2 API limitation: cannot get actual history count.
     Return 1 if back is possible, 0 otherwise. */
  BOOL canGoBack = FALSE;
  ih->data->webviewWindow->get_CanGoBack(&canGoBack);
  return iupStrReturnInt(canGoBack ? 1 : 0);
}

static char* winWebBrowserGetForwardCountAttrib(Ihandle* ih)
{
  if (!ih->data->webviewWindow)
    return iupStrReturnInt(0);

  /* WebView2 API limitation: cannot get actual history count.
     Return 1 if forward is possible, 0 otherwise. */
  BOOL canGoForward = FALSE;
  ih->data->webviewWindow->get_CanGoForward(&canGoForward);
  return iupStrReturnInt(canGoForward ? 1 : 0);
}

static char* winWebBrowserGetItemHistoryAttrib(Ihandle* ih, int id)
{
  (void)ih;
  (void)id;
  /* WebView2 API limitation: navigation history list is not accessible. */
  return NULL;
}

static void winWebBrowserExecuteJavascript(Ihandle* ih, const char* script)
{
  if (!ih->data->webviewWindow || !script)
    return;

  WCHAR* wscript = iupwinStrChar2Wide(script);
  ih->data->webviewWindow->ExecuteScript(wscript, NULL);
  free(wscript);
}

class ExecuteScriptHandler : public EventHandler<ICoreWebView2ExecuteScriptCompletedHandler>
{
private:
  char** result;
  int* complete;

public:
  ExecuteScriptHandler(Ihandle* handle, char** res, int* comp) : EventHandler(handle), result(res), complete(comp) {}

  HRESULT STDMETHODCALLTYPE Invoke(HRESULT errorCode, LPCWSTR resultObjectAsJson) override
  {
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
        *result = iupStrDup(str);
        free(str);
      }
    }
    *complete = 1;
    return S_OK;
  }
};

static char* winWebBrowserRunJavaScriptSync(Ihandle* ih, const char* script)
{
  if (!ih->data->webviewWindow || !script)
    return NULL;

  char* result = NULL;
  int complete = 0;

  WCHAR* wscript = iupwinStrChar2Wide(script);
  ExecuteScriptHandler* handler = new ExecuteScriptHandler(ih, &result, &complete);
  HRESULT hr = ih->data->webviewWindow->ExecuteScript(wscript, handler);
  handler->Release();
  free(wscript);

  if (FAILED(hr))
    return NULL;

  MSG msg;
  int loopCount = 0;
  while (complete == 0 && loopCount < 1000)
  {
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
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

  return result;
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
    return NULL;

  std::string escaped_cmd = winWebBrowserEscapeJavaScript(cmd);
  std::string script = "document.queryCommandValue(" + escaped_cmd + ");";
  return winWebBrowserRunJavaScriptSync(ih, script.c_str());
}

static char* winWebBrowserQueryCommandState(Ihandle* ih, const char* cmd)
{
  if (!ih->data->webviewWindow || !cmd)
    return NULL;

  std::string escaped_cmd = winWebBrowserEscapeJavaScript(cmd);
  std::string script = "document.queryCommandState(" + escaped_cmd + ");";
  return winWebBrowserRunJavaScriptSync(ih, script.c_str());
}

static char* winWebBrowserQueryCommandEnabled(Ihandle* ih, const char* cmd)
{
  if (!ih->data->webviewWindow || !cmd)
    return NULL;

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
    return NULL;

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
    return NULL;

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
    iupAttribSet(ih, "_IUPWEB_EDITABLE", NULL);
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

  char* buffer = (char*)malloc(fileSize + 1);
  if (buffer)
  {
    size_t bytesRead = fread(buffer, 1, fileSize, file);
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

  unsigned char* buffer = (unsigned char*)malloc(fileSize);
  if (buffer)
  {
    size_t bytesRead = fread(buffer, 1, fileSize, file);

    static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t base64_len = 4 * ((bytesRead + 2) / 3);
    char* base64 = (char*)malloc(base64_len + 1);

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

      char* dataUrl = (char*)malloc(strlen(mime) + base64_len + 20);
      if (dataUrl)
      {
        sprintf(dataUrl, "data:%s;base64,%s", mime, base64);
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
      char* ret = iupStrReturnStr(result);
      free(result);
      return ret;
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
      char* ret = iupStrReturnStr(result);
      free(result);
      return ret;
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
  return NULL;
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
  return NULL;
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
  return NULL;
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
  return NULL;
}

static int winWebBrowserSetForeColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
  {
    char rgb_color[32];
    sprintf(rgb_color, "rgb(%d,%d,%d)", r, g, b);
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
  return NULL;
}

static int winWebBrowserSetBackColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
  {
    char rgb_color[32];
    sprintf(rgb_color, "rgb(%d,%d,%d)", r, g, b);
    winWebBrowserExecCommandParam(ih, "backColor", rgb_color);
  }
  return 0;
}

static char* winWebBrowserGetInnerTextAttrib(Ihandle* ih)
{
  char* element_id = iupAttribGet(ih, "ELEMENT_ID");
  if (!element_id)
    return NULL;

  std::string escaped_id = winWebBrowserEscapeJavaScript(element_id);
  std::string script = "document.getElementById(" + escaped_id + ")?.innerText || '';";
  char* result = winWebBrowserRunJavaScriptSync(ih, script.c_str());
  if (result)
  {
    char* ret = iupStrReturnStr(result);
    free(result);
    return ret;
  }
  return NULL;
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

static char* winWebBrowserGetAttributeAttrib(Ihandle* ih)
{
  char* element_id = iupAttribGet(ih, "ELEMENT_ID");
  char* attr_name = iupAttribGet(ih, "ATTRIBUTE_NAME");
  if (!element_id || !attr_name)
    return NULL;

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
  return NULL;
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
    return (char*)"YES";

  char* result = winWebBrowserRunJavaScriptSync(ih, "window.iupGetDirtyFlag ? window.iupGetDirtyFlag() : false;");
  if (result)
  {
    int dirty = strcmp(result, "true") == 0;
    free(result);
    if (dirty)
      iupAttribSet(ih, "_IUPWEB_DIRTY", "1");
    return iupStrReturnBoolean(dirty);
  }
  return (char*)"NO";
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
  return winWebBrowserSetPrintAttrib(ih, NULL);
}

static int winWebBrowserMapMethod(Ihandle* ih)
{
  if (!g_comInitialized)
  {
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
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

  HWND parent = (HWND)iupChildTreeGetNativeParentHandle(ih);
  if (!parent)
    return IUP_ERROR;

  HWND hwnd = CreateWindowEx(0, "STATIC", "",
                              WS_CHILD | WS_VISIBLE,
                              ih->x, ih->y, ih->currentwidth, ih->currentheight,
                              parent, NULL, (HINSTANCE)GetModuleHandle(NULL), NULL);

  if (!hwnd)
    return IUP_ERROR;

  ih->handle = hwnd;

  SetProp(hwnd, "IUP_WEBBROWSER_IH", (HANDLE)ih);
  ih->data->oldWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)WebBrowserWndProc);

  HRESULT hr = IupWebView2LoaderInit();
  if (FAILED(hr))
  {
    DestroyWindow(hwnd);
    ih->handle = NULL;
    return IUP_ERROR;
  }

  CreateCoreWebView2EnvironmentWithOptionsFunc createEnvFunc = IupWebView2LoaderGetCreateEnvironmentFunc();
  if (!createEnvFunc)
  {
    DestroyWindow(hwnd);
    ih->handle = NULL;
    return IUP_ERROR;
  }

  const wchar_t* runtimePath = IupWebView2LoaderGetRuntimePath();

  std::wstring userDataFolder = GetWebView2UserDataFolder();

  ih->data->initComplete = 0;

  CreateEnvironmentHandler* envHandler = new CreateEnvironmentHandler(ih, hwnd);
  hr = createEnvFunc(runtimePath, userDataFolder.empty() ? nullptr : userDataFolder.c_str(), nullptr, envHandler);
  envHandler->Release();

  if (FAILED(hr))
  {
    DestroyWindow(hwnd);
    ih->handle = NULL;
    return IUP_ERROR;
  }

  MSG msg;
  while (ih->data->initComplete == 0)
  {
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    else
    {
      Sleep(10);
    }
  }

  if (ih->data->initComplete < 0)
  {
    DestroyWindow(hwnd);
    ih->handle = NULL;
    return IUP_ERROR;
  }

  RECT rect;
  GetClientRect(hwnd, &rect);
  SendMessage(hwnd, WM_SIZE, SIZE_RESTORED, MAKELPARAM(rect.right, rect.bottom));

  return IUP_NOERROR;
}

static void winWebBrowserUnMapMethod(Ihandle* ih)
{
  if (ih->data->webviewWindow)
  {
    ih->data->webviewWindow->remove_NavigationStarting(ih->data->navigationStartingToken);
    ih->data->webviewWindow->remove_NavigationCompleted(ih->data->navigationCompletedToken);
    ih->data->webviewWindow->remove_NewWindowRequested(ih->data->newWindowRequestedToken);
    ih->data->webviewWindow->remove_HistoryChanged(ih->data->historyChangedToken);
    ih->data->webviewWindow->remove_WebMessageReceived(ih->data->webMessageReceivedToken);

    ih->data->webviewWindow->Release();
    ih->data->webviewWindow = NULL;
  }

  if (ih->data->webviewController)
  {
    ih->data->webviewController->Close();
    ih->data->webviewController->Release();
    ih->data->webviewController = NULL;
  }

  if (ih->handle)
  {
    if (ih->data->oldWndProc)
    {
      SetWindowLongPtr((HWND)ih->handle, GWLP_WNDPROC, (LONG_PTR)ih->data->oldWndProc);
      ih->data->oldWndProc = NULL;
    }
    RemoveProp((HWND)ih->handle, "IUP_WEBBROWSER_IH");
    DestroyWindow((HWND)ih->handle);
    ih->handle = NULL;
  }
}

static void winWebBrowserComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  int natural_w = 0, natural_h = 0;
  (void)children_expand;

  iupdrvFontGetCharSize(ih, &natural_w, &natural_h);

  *w = natural_w;
  *h = natural_h;
}

static void winWebBrowserLayoutUpdateMethod(Ihandle* ih)
{
  iupdrvBaseLayoutUpdateMethod(ih);

  if (ih->data->webviewController && ih->handle)
  {
    RECT clientRect;
    GetClientRect((HWND)ih->handle, &clientRect);

    RECT bounds;
    bounds.left = 0;
    bounds.top = 0;
    bounds.right = clientRect.right;
    bounds.bottom = clientRect.bottom;

    ih->data->webviewController->put_Bounds(bounds);
  }
}

static int winWebBrowserCreateMethod(Ihandle* ih, void **params)
{
  (void)params;

  ih->data = iupALLOCCTRLDATA();
  ih->data->webviewController = NULL;
  ih->data->webviewWindow = NULL;
  ih->data->loadStatus = WEBVIEW_STATUS_COMPLETED;
  ih->data->initComplete = 0;
  ih->data->oldWndProc = NULL;

  IupSetAttribute(ih, "BORDER", "NO");
  IupSetAttribute(ih, "CANFOCUS", "YES");

  ih->expand = IUP_EXPAND_BOTH;

  return IUP_NOERROR;
}

static void winWebBrowserDestroyMethod(Ihandle* ih)
{
  if (ih->data)
  {
    if (ih->handle)
      winWebBrowserUnMapMethod(ih);
  }
}

Iclass* iupWebBrowserNewClass(void)
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

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "VALUE", winWebBrowserGetValueAttrib, winWebBrowserSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKFORWARD", NULL, winWebBrowserSetBackForwardAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GOBACK", NULL, winWebBrowserSetGoBackAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GOFORWARD", NULL, winWebBrowserSetGoForwardAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STOP", NULL, winWebBrowserSetStopAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RELOAD", NULL, winWebBrowserSetReloadAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HTML", winWebBrowserGetHTMLAttrib, winWebBrowserSetHTMLAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STATUS", winWebBrowserGetStatusAttrib, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ZOOM", winWebBrowserGetZoomAttrib, winWebBrowserSetZoomAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PRINT", NULL, winWebBrowserSetPrintAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CANGOBACK", winWebBrowserGetCanGoBackAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CANGOFORWARD", winWebBrowserGetCanGoForwardAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "EDITABLE", winWebBrowserGetEditableAttrib, winWebBrowserSetEditableAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NEW", NULL, winWebBrowserSetNewAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OPENFILE", NULL, winWebBrowserSetOpenAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SAVEFILE", NULL, winWebBrowserSetSaveAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "UNDO", NULL, winWebBrowserSetUndoAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "REDO", NULL, winWebBrowserSetRedoAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COPY", NULL, winWebBrowserSetCopyAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUT", NULL, winWebBrowserSetCutAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PASTE", winWebBrowserGetPasteAttrib, winWebBrowserSetPasteAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTALL", NULL, winWebBrowserSetSelectAllAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EXECCOMMAND", NULL, winWebBrowserSetExecCommandAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTIMAGE", NULL, winWebBrowserSetInsertImageAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTIMAGEFILE", NULL, winWebBrowserSetInsertImageFileAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CREATELINK", NULL, winWebBrowserSetCreateLinkAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTTEXT", NULL, winWebBrowserSetInsertTextAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTHTML", NULL, winWebBrowserSetInsertHtmlAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "FONTNAME", winWebBrowserGetFontNameAttrib, winWebBrowserSetFontNameAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FONTSIZE", winWebBrowserGetFontSizeAttrib, winWebBrowserSetFontSizeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATBLOCK", winWebBrowserGetFormatBlockAttrib, winWebBrowserSetFormatBlockAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORECOLOR", winWebBrowserGetForeColorAttrib, winWebBrowserSetForeColorAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKCOLOR", winWebBrowserGetBackColorAttrib, winWebBrowserSetBackColorAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "COMMAND", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDSHOWUI", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDSTATE", winWebBrowserGetCommandStateAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDENABLED", winWebBrowserGetCommandEnabledAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDTEXT", winWebBrowserGetCommandTextAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDVALUE", winWebBrowserGetCommandValueAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ELEMENT_ID", NULL, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INNERTEXT", winWebBrowserGetInnerTextAttrib, winWebBrowserSetInnerTextAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ATTRIBUTE_NAME", NULL, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ATTRIBUTE", winWebBrowserGetAttributeAttrib, winWebBrowserSetAttributeAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BACKCOUNT", winWebBrowserGetBackCountAttrib, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORWARDCOUNT", winWebBrowserGetForwardCountAttrib, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "ITEMHISTORY", winWebBrowserGetItemHistoryAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DIRTY", winWebBrowserGetDirtyAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FIND", NULL, winWebBrowserSetFindAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PRINTPREVIEW", NULL, winWebBrowserSetPrintPreviewAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  return ic;
}
