/** \file
 * \brief Web Browser Control for Windows
 *
 * WebView2 interface declarations and built-in loader API.
 * This file contains the minimal WebView2 interfaces needed for IUP WebBrowser
 * and the built-in loader that eliminates the need for WebView2Loader.dll.
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUP_WEBBROWSER_WIN_H
#define __IUP_WEBBROWSER_WIN_H

#include <windows.h>
#include <objbase.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ICoreWebView2 ICoreWebView2;
typedef struct ICoreWebView2Controller ICoreWebView2Controller;
typedef struct ICoreWebView2Controller2 ICoreWebView2Controller2;
typedef struct ICoreWebView2Environment ICoreWebView2Environment;
typedef struct ICoreWebView2Settings ICoreWebView2Settings;
typedef struct ICoreWebView2NavigationStartingEventArgs ICoreWebView2NavigationStartingEventArgs;
typedef struct ICoreWebView2NavigationCompletedEventArgs ICoreWebView2NavigationCompletedEventArgs;
typedef struct ICoreWebView2NewWindowRequestedEventArgs ICoreWebView2NewWindowRequestedEventArgs;
typedef struct ICoreWebView2WebMessageReceivedEventArgs ICoreWebView2WebMessageReceivedEventArgs;

typedef struct ICoreWebView2NavigationStartingEventHandler ICoreWebView2NavigationStartingEventHandler;
typedef struct ICoreWebView2NavigationCompletedEventHandler ICoreWebView2NavigationCompletedEventHandler;
typedef struct ICoreWebView2NewWindowRequestedEventHandler ICoreWebView2NewWindowRequestedEventHandler;
typedef struct ICoreWebView2HistoryChangedEventHandler ICoreWebView2HistoryChangedEventHandler;
typedef struct ICoreWebView2WebMessageReceivedEventHandler ICoreWebView2WebMessageReceivedEventHandler;
typedef struct ICoreWebView2CreateCoreWebView2ControllerCompletedHandler ICoreWebView2CreateCoreWebView2ControllerCompletedHandler;
typedef struct ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler;
typedef struct ICoreWebView2ExecuteScriptCompletedHandler ICoreWebView2ExecuteScriptCompletedHandler;

typedef UINT64 EventRegistrationToken;

typedef enum {
  WEBVIEW2_RUNTIME_TYPE_INSTALLED = 0,
  WEBVIEW2_RUNTIME_TYPE_EMBEDDED = 1
} WebView2RuntimeType;

typedef HRESULT (STDAPICALLTYPE *CreateWebViewEnvironmentWithOptionsInternalFunc)(
    bool useCustomLoader,
    WebView2RuntimeType runtimeType,
    PCWSTR userDataFolder,
    IUnknown* environmentOptions,
    ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* environmentCreatedHandler);

typedef HRESULT (STDAPICALLTYPE *CreateCoreWebView2EnvironmentWithOptionsFunc)(
    PCWSTR browserExecutableFolder,
    PCWSTR userDataFolder,
    void* environmentOptions,
    ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* environmentCreatedHandler);

HRESULT IupWebView2LoaderInit(void);
void IupWebView2LoaderCleanup(void);
CreateCoreWebView2EnvironmentWithOptionsFunc IupWebView2LoaderGetCreateEnvironmentFunc(void);
const wchar_t* IupWebView2LoaderGetRuntimePath(void);

#ifdef __cplusplus
}

#undef INTERFACE
#define INTERFACE ICoreWebView2NavigationStartingEventHandler
DECLARE_INTERFACE_(ICoreWebView2NavigationStartingEventHandler, IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;
  STDMETHOD(Invoke)(THIS_ ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args) PURE;
};

#undef INTERFACE
#define INTERFACE ICoreWebView2NavigationCompletedEventHandler
DECLARE_INTERFACE_(ICoreWebView2NavigationCompletedEventHandler, IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;
  STDMETHOD(Invoke)(THIS_ ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) PURE;
};

#undef INTERFACE
#define INTERFACE ICoreWebView2NewWindowRequestedEventHandler
DECLARE_INTERFACE_(ICoreWebView2NewWindowRequestedEventHandler, IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;
  STDMETHOD(Invoke)(THIS_ ICoreWebView2* sender, ICoreWebView2NewWindowRequestedEventArgs* args) PURE;
};

#undef INTERFACE
#define INTERFACE ICoreWebView2HistoryChangedEventHandler
DECLARE_INTERFACE_(ICoreWebView2HistoryChangedEventHandler, IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;
  STDMETHOD(Invoke)(THIS_ ICoreWebView2* sender, IUnknown* args) PURE;
};

#undef INTERFACE
#define INTERFACE ICoreWebView2WebMessageReceivedEventHandler
DECLARE_INTERFACE_(ICoreWebView2WebMessageReceivedEventHandler, IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;
  STDMETHOD(Invoke)(THIS_ ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args) PURE;
};

#undef INTERFACE
#define INTERFACE ICoreWebView2CreateCoreWebView2ControllerCompletedHandler
DECLARE_INTERFACE_(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler, IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;
  STDMETHOD(Invoke)(THIS_ HRESULT errorCode, ICoreWebView2Controller* createdController) PURE;
};

#undef INTERFACE
#define INTERFACE ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler
DECLARE_INTERFACE_(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler, IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;
  STDMETHOD(Invoke)(THIS_ HRESULT errorCode, ICoreWebView2Environment* createdEnvironment) PURE;
};

#undef INTERFACE
#define INTERFACE ICoreWebView2ExecuteScriptCompletedHandler
DECLARE_INTERFACE_(ICoreWebView2ExecuteScriptCompletedHandler, IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;
  STDMETHOD(Invoke)(THIS_ HRESULT errorCode, LPCWSTR resultObjectAsJson) PURE;
};

#undef INTERFACE
#define INTERFACE ICoreWebView2NavigationStartingEventArgs
DECLARE_INTERFACE_(ICoreWebView2NavigationStartingEventArgs, IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;
  STDMETHOD(get_Uri)(THIS_ LPWSTR* uri) PURE;
  STDMETHOD(get_IsUserInitiated)(THIS_ BOOL* isUserInitiated) PURE;
  STDMETHOD(get_IsRedirected)(THIS_ BOOL* isRedirected) PURE;
  STDMETHOD(get_RequestHeaders)(THIS_ void** requestHeaders) PURE;
  STDMETHOD(get_Cancel)(THIS_ BOOL* cancel) PURE;
  STDMETHOD(put_Cancel)(THIS_ BOOL cancel) PURE;
  STDMETHOD(get_NavigationId)(THIS_ UINT64* navigationId) PURE;
};

#undef INTERFACE
#define INTERFACE ICoreWebView2NavigationCompletedEventArgs
DECLARE_INTERFACE_(ICoreWebView2NavigationCompletedEventArgs, IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;
  STDMETHOD(get_IsSuccess)(THIS_ BOOL* isSuccess) PURE;
  STDMETHOD(get_WebErrorStatus)(THIS_ int* webErrorStatus) PURE;
  STDMETHOD(get_NavigationId)(THIS_ UINT64* navigationId) PURE;
};

#undef INTERFACE
#define INTERFACE ICoreWebView2NewWindowRequestedEventArgs
DECLARE_INTERFACE_(ICoreWebView2NewWindowRequestedEventArgs, IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;
  STDMETHOD(get_Uri)(THIS_ LPWSTR* uri) PURE;
  STDMETHOD(put_NewWindow)(THIS_ ICoreWebView2* newWindow) PURE;
  STDMETHOD(get_NewWindow)(THIS_ ICoreWebView2** newWindow) PURE;
  STDMETHOD(put_Handled)(THIS_ BOOL handled) PURE;
  STDMETHOD(get_Handled)(THIS_ BOOL* handled) PURE;
  STDMETHOD(get_IsUserInitiated)(THIS_ BOOL* isUserInitiated) PURE;
  STDMETHOD(GetDeferral)(THIS_ void** deferral) PURE;
  STDMETHOD(get_WindowFeatures)(THIS_ void** windowFeatures) PURE;
};

#undef INTERFACE
#define INTERFACE ICoreWebView2WebMessageReceivedEventArgs
DECLARE_INTERFACE_(ICoreWebView2WebMessageReceivedEventArgs, IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;
  STDMETHOD(get_Source)(THIS_ LPWSTR* source) PURE;
  STDMETHOD(get_WebMessageAsJson)(THIS_ LPWSTR* webMessageAsJson) PURE;
  STDMETHOD(TryGetWebMessageAsString)(THIS_ LPWSTR* webMessageAsString) PURE;
};

#undef INTERFACE
#define INTERFACE ICoreWebView2Settings
DECLARE_INTERFACE_(ICoreWebView2Settings, IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;
  STDMETHOD(get_IsScriptEnabled)(THIS_ BOOL* isScriptEnabled) PURE;
  STDMETHOD(put_IsScriptEnabled)(THIS_ BOOL isScriptEnabled) PURE;
  STDMETHOD(get_IsWebMessageEnabled)(THIS_ BOOL* isWebMessageEnabled) PURE;
  STDMETHOD(put_IsWebMessageEnabled)(THIS_ BOOL isWebMessageEnabled) PURE;
  STDMETHOD(get_AreDefaultScriptDialogsEnabled)(THIS_ BOOL* areDefaultScriptDialogsEnabled) PURE;
  STDMETHOD(put_AreDefaultScriptDialogsEnabled)(THIS_ BOOL areDefaultScriptDialogsEnabled) PURE;
  STDMETHOD(get_IsStatusBarEnabled)(THIS_ BOOL* isStatusBarEnabled) PURE;
  STDMETHOD(put_IsStatusBarEnabled)(THIS_ BOOL isStatusBarEnabled) PURE;
  STDMETHOD(get_AreDevToolsEnabled)(THIS_ BOOL* areDevToolsEnabled) PURE;
  STDMETHOD(put_AreDevToolsEnabled)(THIS_ BOOL areDevToolsEnabled) PURE;
  STDMETHOD(get_AreDefaultContextMenusEnabled)(THIS_ BOOL* enabled) PURE;
  STDMETHOD(put_AreDefaultContextMenusEnabled)(THIS_ BOOL enabled) PURE;
  STDMETHOD(get_AreHostObjectsAllowed)(THIS_ BOOL* allowed) PURE;
  STDMETHOD(put_AreHostObjectsAllowed)(THIS_ BOOL allowed) PURE;
  STDMETHOD(get_IsZoomControlEnabled)(THIS_ BOOL* enabled) PURE;
  STDMETHOD(put_IsZoomControlEnabled)(THIS_ BOOL enabled) PURE;
  STDMETHOD(get_IsBuiltInErrorPageEnabled)(THIS_ BOOL* enabled) PURE;
  STDMETHOD(put_IsBuiltInErrorPageEnabled)(THIS_ BOOL enabled) PURE;
};

#undef INTERFACE
#define INTERFACE ICoreWebView2Controller
DECLARE_INTERFACE_(ICoreWebView2Controller, IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;
  STDMETHOD(get_IsVisible)(THIS_ BOOL* isVisible) PURE;
  STDMETHOD(put_IsVisible)(THIS_ BOOL isVisible) PURE;
  STDMETHOD(get_Bounds)(THIS_ RECT* bounds) PURE;
  STDMETHOD(put_Bounds)(THIS_ RECT bounds) PURE;
  STDMETHOD(get_ZoomFactor)(THIS_ double* zoomFactor) PURE;
  STDMETHOD(put_ZoomFactor)(THIS_ double zoomFactor) PURE;
  STDMETHOD(add_ZoomFactorChanged)(THIS_ void* eventHandler, EventRegistrationToken* token) PURE;
  STDMETHOD(remove_ZoomFactorChanged)(THIS_ EventRegistrationToken token) PURE;
  STDMETHOD(SetBoundsAndZoomFactor)(THIS_ RECT bounds, double zoomFactor) PURE;
  STDMETHOD(MoveFocus)(THIS_ int reason) PURE;
  STDMETHOD(add_MoveFocusRequested)(THIS_ void* eventHandler, EventRegistrationToken* token) PURE;
  STDMETHOD(remove_MoveFocusRequested)(THIS_ EventRegistrationToken token) PURE;
  STDMETHOD(add_GotFocus)(THIS_ void* eventHandler, EventRegistrationToken* token) PURE;
  STDMETHOD(remove_GotFocus)(THIS_ EventRegistrationToken token) PURE;
  STDMETHOD(add_LostFocus)(THIS_ void* eventHandler, EventRegistrationToken* token) PURE;
  STDMETHOD(remove_LostFocus)(THIS_ EventRegistrationToken token) PURE;
  STDMETHOD(add_AcceleratorKeyPressed)(THIS_ void* eventHandler, EventRegistrationToken* token) PURE;
  STDMETHOD(remove_AcceleratorKeyPressed)(THIS_ EventRegistrationToken token) PURE;
  STDMETHOD(get_ParentWindow)(THIS_ HWND* parentWindow) PURE;
  STDMETHOD(put_ParentWindow)(THIS_ HWND parentWindow) PURE;
  STDMETHOD(NotifyParentWindowPositionChanged)(THIS) PURE;
  STDMETHOD(Close)(THIS) PURE;
  STDMETHOD(get_CoreWebView2)(THIS_ ICoreWebView2** coreWebView2) PURE;
};

#undef INTERFACE
#define INTERFACE ICoreWebView2Controller2
DECLARE_INTERFACE_(ICoreWebView2Controller2, ICoreWebView2Controller)
{
  STDMETHOD(get_DefaultBackgroundColor)(THIS_ COLORREF* backgroundColor) PURE;
  STDMETHOD(put_DefaultBackgroundColor)(THIS_ COLORREF backgroundColor) PURE;
};

#undef INTERFACE
#define INTERFACE ICoreWebView2
DECLARE_INTERFACE_(ICoreWebView2, IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;
  STDMETHOD(get_Settings)(THIS_ ICoreWebView2Settings** settings) PURE;
  STDMETHOD(get_Source)(THIS_ LPWSTR* uri) PURE;
  STDMETHOD(Navigate)(THIS_ LPCWSTR uri) PURE;
  STDMETHOD(NavigateToString)(THIS_ LPCWSTR htmlContent) PURE;
  STDMETHOD(add_NavigationStarting)(THIS_ ICoreWebView2NavigationStartingEventHandler* eventHandler, EventRegistrationToken* token) PURE;
  STDMETHOD(remove_NavigationStarting)(THIS_ EventRegistrationToken token) PURE;
  STDMETHOD(add_ContentLoading)(THIS_ void* eventHandler, EventRegistrationToken* token) PURE;
  STDMETHOD(remove_ContentLoading)(THIS_ EventRegistrationToken token) PURE;
  STDMETHOD(add_SourceChanged)(THIS_ void* eventHandler, EventRegistrationToken* token) PURE;
  STDMETHOD(remove_SourceChanged)(THIS_ EventRegistrationToken token) PURE;
  STDMETHOD(add_HistoryChanged)(THIS_ ICoreWebView2HistoryChangedEventHandler* eventHandler, EventRegistrationToken* token) PURE;
  STDMETHOD(remove_HistoryChanged)(THIS_ EventRegistrationToken token) PURE;
  STDMETHOD(add_NavigationCompleted)(THIS_ ICoreWebView2NavigationCompletedEventHandler* eventHandler, EventRegistrationToken* token) PURE;
  STDMETHOD(remove_NavigationCompleted)(THIS_ EventRegistrationToken token) PURE;
  STDMETHOD(add_FrameNavigationStarting)(THIS_ void* eventHandler, EventRegistrationToken* token) PURE;
  STDMETHOD(remove_FrameNavigationStarting)(THIS_ EventRegistrationToken token) PURE;
  STDMETHOD(add_FrameNavigationCompleted)(THIS_ void* eventHandler, EventRegistrationToken* token) PURE;
  STDMETHOD(remove_FrameNavigationCompleted)(THIS_ EventRegistrationToken token) PURE;
  STDMETHOD(add_ScriptDialogOpening)(THIS_ void* eventHandler, EventRegistrationToken* token) PURE;
  STDMETHOD(remove_ScriptDialogOpening)(THIS_ EventRegistrationToken token) PURE;
  STDMETHOD(add_PermissionRequested)(THIS_ void* eventHandler, EventRegistrationToken* token) PURE;
  STDMETHOD(remove_PermissionRequested)(THIS_ EventRegistrationToken token) PURE;
  STDMETHOD(add_ProcessFailed)(THIS_ void* eventHandler, EventRegistrationToken* token) PURE;
  STDMETHOD(remove_ProcessFailed)(THIS_ EventRegistrationToken token) PURE;
  STDMETHOD(AddScriptToExecuteOnDocumentCreated)(THIS_ LPCWSTR javaScript, void* handler) PURE;
  STDMETHOD(RemoveScriptToExecuteOnDocumentCreated)(THIS_ LPCWSTR id) PURE;
  STDMETHOD(ExecuteScript)(THIS_ LPCWSTR javaScript, ICoreWebView2ExecuteScriptCompletedHandler* handler) PURE;
  STDMETHOD(CapturePreview)(THIS_ int imageFormat, IStream* imageStream, void* handler) PURE;
  STDMETHOD(Reload)(THIS) PURE;
  STDMETHOD(PostWebMessageAsJson)(THIS_ LPCWSTR webMessageAsJson) PURE;
  STDMETHOD(PostWebMessageAsString)(THIS_ LPCWSTR webMessageAsString) PURE;
  STDMETHOD(add_WebMessageReceived)(THIS_ ICoreWebView2WebMessageReceivedEventHandler* handler, EventRegistrationToken* token) PURE;
  STDMETHOD(remove_WebMessageReceived)(THIS_ EventRegistrationToken token) PURE;
  STDMETHOD(CallDevToolsProtocolMethod)(THIS_ LPCWSTR methodName, LPCWSTR parametersAsJson, void* handler) PURE;
  STDMETHOD(get_BrowserProcessId)(THIS_ DWORD* value) PURE;
  STDMETHOD(get_CanGoBack)(THIS_ BOOL* canGoBack) PURE;
  STDMETHOD(get_CanGoForward)(THIS_ BOOL* canGoForward) PURE;
  STDMETHOD(GoBack)(THIS) PURE;
  STDMETHOD(GoForward)(THIS) PURE;
  STDMETHOD(GetDevToolsProtocolEventReceiver)(THIS_ LPCWSTR eventName, void** receiver) PURE;
  STDMETHOD(Stop)(THIS) PURE;
  STDMETHOD(add_NewWindowRequested)(THIS_ ICoreWebView2NewWindowRequestedEventHandler* eventHandler, EventRegistrationToken* token) PURE;
  STDMETHOD(remove_NewWindowRequested)(THIS_ EventRegistrationToken token) PURE;
  STDMETHOD(add_DocumentTitleChanged)(THIS_ void* eventHandler, EventRegistrationToken* token) PURE;
  STDMETHOD(remove_DocumentTitleChanged)(THIS_ EventRegistrationToken token) PURE;
  STDMETHOD(get_DocumentTitle)(THIS_ LPWSTR* title) PURE;
  STDMETHOD(AddHostObjectToScript)(THIS_ LPCWSTR name, VARIANT* object) PURE;
  STDMETHOD(RemoveHostObjectFromScript)(THIS_ LPCWSTR name) PURE;
  STDMETHOD(OpenDevToolsWindow)(THIS) PURE;
  STDMETHOD(add_ContainsFullScreenElementChanged)(THIS_ void* eventHandler, EventRegistrationToken* token) PURE;
  STDMETHOD(remove_ContainsFullScreenElementChanged)(THIS_ EventRegistrationToken token) PURE;
  STDMETHOD(get_ContainsFullScreenElement)(THIS_ BOOL* containsFullScreenElement) PURE;
  STDMETHOD(add_WebResourceRequested)(THIS_ void* eventHandler, EventRegistrationToken* token) PURE;
  STDMETHOD(remove_WebResourceRequested)(THIS_ EventRegistrationToken token) PURE;
  STDMETHOD(AddWebResourceRequestedFilter)(THIS_ LPCWSTR uri, int resourceContext) PURE;
  STDMETHOD(RemoveWebResourceRequestedFilter)(THIS_ LPCWSTR uri, int resourceContext) PURE;
  STDMETHOD(add_WindowCloseRequested)(THIS_ void* eventHandler, EventRegistrationToken* token) PURE;
  STDMETHOD(remove_WindowCloseRequested)(THIS_ EventRegistrationToken token) PURE;
};

#undef INTERFACE
#define INTERFACE ICoreWebView2Environment
DECLARE_INTERFACE_(ICoreWebView2Environment, IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;
  STDMETHOD(CreateCoreWebView2Controller)(THIS_ HWND parentWindow, ICoreWebView2CreateCoreWebView2ControllerCompletedHandler* handler) PURE;
  STDMETHOD(CreateWebResourceResponse)(THIS_ IStream* content, int statusCode, LPCWSTR reasonPhrase, LPCWSTR headers, void** response) PURE;
  STDMETHOD(get_BrowserVersionString)(THIS_ LPWSTR* versionInfo) PURE;
  STDMETHOD(add_NewBrowserVersionAvailable)(THIS_ void* eventHandler, EventRegistrationToken* token) PURE;
  STDMETHOD(remove_NewBrowserVersionAvailable)(THIS_ EventRegistrationToken token) PURE;
};

#endif

#ifdef __cplusplus
extern "C++" {

template<typename T>
const GUID& __mingw_uuidof();

template<>
inline const GUID& __mingw_uuidof<ICoreWebView2NavigationStartingEventHandler>() {
  static const GUID guid = {0x9adbe429, 0xf36d, 0x432b, {0x9d, 0xdc, 0xf8, 0x88, 0x1f, 0xbd, 0x76, 0xe3}};
  return guid;
}

template<>
inline const GUID& __mingw_uuidof<ICoreWebView2NavigationCompletedEventHandler>() {
  static const GUID guid = {0xd33a35bf, 0x1c49, 0x4f98, {0x93, 0xab, 0x00, 0x6e, 0x05, 0x33, 0xfe, 0x1c}};
  return guid;
}

template<>
inline const GUID& __mingw_uuidof<ICoreWebView2NewWindowRequestedEventHandler>() {
  static const GUID guid = {0xd4c185fe, 0xc81c, 0x4989, {0x97, 0xaf, 0x2d, 0x3f, 0xa7, 0xab, 0x56, 0x51}};
  return guid;
}

template<>
inline const GUID& __mingw_uuidof<ICoreWebView2HistoryChangedEventHandler>() {
  static const GUID guid = {0xc79a0bba, 0x933a, 0x4319, {0x98, 0x47, 0x3b, 0x55, 0x30, 0x7d, 0x47, 0x96}};
  return guid;
}

template<>
inline const GUID& __mingw_uuidof<ICoreWebView2WebMessageReceivedEventHandler>() {
  static const GUID guid = {0x57213f19, 0x00e6, 0x49fa, {0x8e, 0x07, 0x89, 0x8e, 0xa0, 0x1e, 0xcb, 0xd2}};
  return guid;
}

template<>
inline const GUID& __mingw_uuidof<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>() {
  static const GUID guid = {0x6c4819f3, 0xc9b7, 0x4260, {0x81, 0x27, 0xc9, 0xf5, 0xbd, 0xe7, 0xf6, 0x8c}};
  return guid;
}

template<>
inline const GUID& __mingw_uuidof<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>() {
  static const GUID guid = {0x4e8a3389, 0xc9d8, 0x4bd2, {0xb6, 0xb5, 0x12, 0x4f, 0xee, 0x6c, 0xc1, 0x4d}};
  return guid;
}

template<>
inline const GUID& __mingw_uuidof<ICoreWebView2ExecuteScriptCompletedHandler>() {
  static const GUID guid = {0x49511172, 0xcc67, 0x4bca, {0x99, 0x23, 0x13, 0x71, 0x12, 0xf4, 0xc4, 0xcc}};
  return guid;
}

template<>
inline const GUID& __mingw_uuidof<ICoreWebView2Settings>() {
  static const GUID guid = {0x3C067F9F, 0x5388, 0x4772, {0x8B, 0x48, 0xED, 0xC4, 0x49, 0xB8, 0x67, 0x34}};
  return guid;
}

template<>
inline const GUID& __mingw_uuidof<ICoreWebView2Environment>() {
  static const GUID guid = {0xB96D755E, 0x0319, 0x4E92, {0xA2, 0x96, 0x23, 0x43, 0xA3, 0x15, 0x63, 0x10}};
  return guid;
}

template<>
inline const GUID& __mingw_uuidof<ICoreWebView2Controller2>() {
  static const GUID guid = {0xc979903e, 0xd4ca, 0x4228, {0x92, 0xeb, 0x47, 0xee, 0x3f, 0xa9, 0x6c, 0xab}};
  return guid;
}

}
#endif

#endif
