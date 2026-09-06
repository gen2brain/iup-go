/** \file
 * \brief WinUI Driver Core - Initialization and Setup using XAML Islands
 *
 * See Copyright Notice in "iup.h"
 */

#include <clocale>
#include "pch.h"
#include <shobjidl.h>
#include <shellapi.h>
#include <appmodel.h>

using namespace winrt;
using namespace Microsoft::UI::Dispatching;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Hosting;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::XamlTypeInfo;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml::Interop;


/* appmodel.h hides the OS ones under the driver's NTDDI_VERSION; the runtime DLL exports them as Mdd* */
typedef HRESULT (WINAPI *TryCreatePackageDependencyFunc)(PSID, PCWSTR, PACKAGE_VERSION, INT32, INT32, PCWSTR, INT32, PWSTR*);
typedef HRESULT (WINAPI *AddPackageDependencyFunc)(PCWSTR, INT32, INT32, void**, PWSTR*);
typedef HRESULT (WINAPI *RemovePackageDependencyFunc)(void*);
typedef HRESULT (WINAPI *DeletePackageDependencyFunc)(PCWSTR);
typedef void (WINAPI *MddDeletePackageDependencyFunc)(PCWSTR);

#if defined(_M_X64) || defined(__x86_64__)
#define WINUI_ARCH PROCESSOR_ARCHITECTURE_AMD64
#elif defined(_M_ARM64) || defined(__aarch64__)
#define WINUI_ARCH PROCESSOR_ARCHITECTURE_ARM64
#else
#define WINUI_ARCH PROCESSOR_ARCHITECTURE_INTEL
#endif

static PWSTR winui_dependency_id = NULL;
static void* winui_dependency_context = NULL;
static HMODULE winui_runtime_module = NULL;

extern "C" {
#include "iup.h"
#include "iup_drv.h"
#include "iup_globalattrib.h"
}

#include "iupwinui_drv.h"

static bool winui_initialized = false;
static bool winui_bootstrap_initialized = false;
static DispatcherQueueController winui_dispatcher_controller = nullptr;
static WindowsXamlManager winui_xaml_manager = nullptr;

/****************************************************************************
 * Application Class for XAML Islands
 *
 * WinUI 3 controls get their templates and styles from XamlControlsResources,
 * which needs an Application with IXamlMetadataProvider.
 ****************************************************************************/

class IupWinUIApp : public ApplicationT<IupWinUIApp, IXamlMetadataProvider>
{
public:
  IXamlType GetXamlType(TypeName const& type)
  {
    return m_provider.GetXamlType(type);
  }

  IXamlType GetXamlType(hstring const& fullName)
  {
    return m_provider.GetXamlType(fullName);
  }

  com_array<XmlnsDefinition> GetXmlnsDefinitions()
  {
    return m_provider.GetXmlnsDefinitions();
  }

private:
  XamlControlsXamlMetaDataProvider m_provider;
};

static Application winui_app = nullptr;

/* the default ResourceManager fails without a resources.pri before Windows App SDK 1.8.11, the path constructor never did */
static void winuiResourceManagerRequested(IInspectable const&, ResourceManagerRequestedEventArgs const& args)
{
  using namespace winrt::Microsoft::Windows::ApplicationModel::Resources;

  wchar_t path[MAX_PATH];
  DWORD len = GetModuleFileNameW(NULL, path, MAX_PATH);
  wchar_t* sep = (len > 0 && len < MAX_PATH) ? wcsrchr(path, L'\\') : NULL;
  if (!sep)
    return;
  wcscpy_s(sep + 1, MAX_PATH - (sep + 1 - path), L"resources.pri");

  IResourceManagerFactory factory = get_activation_factory<ResourceManager, IResourceManagerFactory>();
  hstring file(path);
  IResourceManager manager{ nullptr };
  check_hresult(static_cast<impl::abi_t<IResourceManagerFactory>*>(get_abi(factory))->CreateInstance(get_abi(file), put_abi(manager)));
  args.CustomResourceManager(manager);
}

/****************************************************************************
 * System Theme Detection
 ****************************************************************************/

extern "C" IUP_SDK_API int iupdrvIsSystemDarkMode(void)
{
  using namespace Windows::UI::ViewManagement;

  UISettings settings;
  auto bg = settings.GetColorValue(UIColorType::Background);

  int luminance = (bg.R * 299 + bg.G * 587 + bg.B * 114) / 1000;
  return luminance < 128 ? 1 : 0;
}

/****************************************************************************
 * Global Colors
 ****************************************************************************/

IUP_DRV_API void iupwinuiSetGlobalColors(void)
{
  using namespace Windows::UI::ViewManagement;

  UISettings settings;

  auto accent = settings.GetColorValue(UIColorType::Accent);

  int appearance = iupGlobalGetAppearance();
  int system_dark = iupdrvIsSystemDarkMode();
  int dark_mode = (appearance == IUP_APPEARANCE_DARK) ||
                  (appearance == IUP_APPEARANCE_SYSTEM && system_dark);

  if (dark_mode != system_dark)  /* UISettings only reports the system appearance */
    iupGlobalSetAppearanceColors(dark_mode);
  else
  {
    auto bg = settings.GetColorValue(UIColorType::Background);
    auto fg = settings.GetColorValue(UIColorType::Foreground);
    int txt_bg = dark_mode? 32: 255;

    iupGlobalSetDefaultColorAttrib("DLGBGCOLOR", bg.R, bg.G, bg.B);
    iupGlobalSetDefaultColorAttrib("DLGFGCOLOR", fg.R, fg.G, fg.B);
    iupGlobalSetDefaultColorAttrib("TXTBGCOLOR", txt_bg, txt_bg, txt_bg);
    iupGlobalSetDefaultColorAttrib("TXTFGCOLOR", fg.R, fg.G, fg.B);
    iupGlobalSetDefaultColorAttrib("MENUBGCOLOR", bg.R, bg.G, bg.B);
    iupGlobalSetDefaultColorAttrib("MENUFGCOLOR", fg.R, fg.G, fg.B);
  }

  iupGlobalSetDefaultColorAttrib("TXTHLCOLOR", accent.R, accent.G, accent.B);
  iupGlobalSetDefaultColorAttrib("ACCENTCOLOR", accent.R, accent.G, accent.B);
  iupGlobalSetDefaultColorAttrib("LINKFGCOLOR", accent.R, accent.G, accent.B);
}

/****************************************************************************
 * Dispatcher Queue Access
 ****************************************************************************/

IUP_DRV_API void* iupwinuiGetDispatcherQueue(void)
{
  if (!winui_dispatcher_controller)
    return NULL;

  DispatcherQueue dq = winui_dispatcher_controller.DispatcherQueue();
  void* ptr = nullptr;
  winrt::copy_to_abi(dq, ptr);
  return ptr;
}

/****************************************************************************
 * Driver Initialization
 ****************************************************************************/

/* the driver is built against the 1.6 headers; 2.x runs it too and has no minor in its family name */
static const UINT32 winui_runtime_versions[] = { 0x00010008, 0x00010007, 0x00010006, 0x00010005, 0x00020000 };
#define WINUI_RUNTIME_VERSIONS 5

static void winuiFrameworkFamily(UINT32 version, wchar_t* family, size_t length)
{
  if (version >= 0x00020000)
    swprintf(family, length, L"Microsoft.WindowsAppRuntime.%u_8wekyb3d8bbwe", version >> 16);
  else
    swprintf(family, length, L"Microsoft.WindowsAppRuntime.%u.%u_8wekyb3d8bbwe", version >> 16, version & 0xFFFF);
}

static int winuiAddDependency(TryCreatePackageDependencyFunc tryCreate, AddPackageDependencyFunc add, PCWSTR family)
{
  PACKAGE_VERSION minVersion = {};
  PWSTR id = NULL;
  if (FAILED(tryCreate(NULL, family, minVersion, 0, 0, NULL, 0, &id)))
    return 0;

  void* context = NULL;
  if (FAILED(add(id, 0, 0, &context, NULL)))
  {
    HeapFree(GetProcessHeap(), 0, id);
    return 0;
  }

  winui_dependency_id = id;
  winui_dependency_context = context;
  return 1;
}

/* what the bootstrap DLL does on Windows 11 24H1+ for runtime 1.7+ */
static int iupwinuiInitDependency(void)
{
  HMODULE kernelbase = GetModuleHandleW(L"kernelbase.dll");
  if (!kernelbase || !GetProcAddress(kernelbase, "TryCreatePackageDependency2"))
    return 0;

  TryCreatePackageDependencyFunc tryCreate = (TryCreatePackageDependencyFunc)GetProcAddress(kernelbase, "TryCreatePackageDependency");
  AddPackageDependencyFunc add = (AddPackageDependencyFunc)GetProcAddress(kernelbase, "AddPackageDependency");
  if (!tryCreate || !add)
    return 0;

  for (int i = 0; i < WINUI_RUNTIME_VERSIONS; i++)
  {
    wchar_t family[128];
    winuiFrameworkFamily(winui_runtime_versions[i], family, 128);
    if (winuiAddDependency(tryCreate, add, family))
      return 1;
  }
  return 0;
}

static int winuiFindFrameworkPath(PCWSTR family, wchar_t* path, UINT32 path_length)
{
  UINT32 count = 0, length = 0;
  if (FindPackagesByPackageFamily(family, PACKAGE_FILTER_HEAD | PACKAGE_FILTER_DIRECT, &count, NULL, &length, NULL, NULL) != ERROR_INSUFFICIENT_BUFFER || count == 0)
    return 0;

  PWSTR* names = (PWSTR*)malloc(count * sizeof(PWSTR));
  wchar_t* buffer = (wchar_t*)malloc(length * sizeof(wchar_t));
  PCWSTR best = NULL;
  UINT64 bestVersion = 0;
  if (FindPackagesByPackageFamily(family, PACKAGE_FILTER_HEAD | PACKAGE_FILTER_DIRECT, &count, names, &length, buffer, NULL) == ERROR_SUCCESS)
  {
    for (UINT32 i = 0; i < count; i++)
    {
      BYTE idBuffer[1024];
      UINT32 idLength = sizeof(idBuffer);
      if (PackageIdFromFullName(names[i], PACKAGE_INFORMATION_BASIC, &idLength, idBuffer) != ERROR_SUCCESS)
        continue;
      PACKAGE_ID* id = (PACKAGE_ID*)idBuffer;
      if (id->processorArchitecture == WINUI_ARCH && id->version.Version > bestVersion)
      {
        bestVersion = id->version.Version;
        best = names[i];
      }
    }
  }

  int ok = best && GetPackagePathByFullName(best, &path_length, path) == ERROR_SUCCESS;
  free(names);
  free(buffer);
  return ok;
}

static void winuiPathPrepend(PCWSTR dir)
{
  DWORD length = GetEnvironmentVariableW(L"PATH", NULL, 0);
  wchar_t* path = (wchar_t*)malloc((wcslen(dir) + 1 + length + 1) * sizeof(wchar_t));
  wcscpy(path, dir);
  if (length)
  {
    wcscat(path, L";");
    GetEnvironmentVariableW(L"PATH", path + wcslen(path), length);
  }
  SetEnvironmentVariableW(L"PATH", path);
  free(path);
}

/* MddAddPackageDependency prepends its own copy of the directory, so only the leading one goes */
static void winuiPathRemove(PCWSTR dir)
{
  DWORD length = GetEnvironmentVariableW(L"PATH", NULL, 0);
  if (!length)
    return;
  wchar_t* path = (wchar_t*)malloc(length * sizeof(wchar_t));
  GetEnvironmentVariableW(L"PATH", path, length);

  size_t dir_length = wcslen(dir);
  if (_wcsnicmp(path, dir, dir_length) == 0 && (path[dir_length] == L';' || path[dir_length] == 0))
    SetEnvironmentVariableW(L"PATH", path[dir_length] == L';' ? path + dir_length + 1 : NULL);
  free(path);
}

/* the runtime's own dependency API, loaded straight from the framework package; no lifetime manager process */
static int iupwinuiInitFramework(void)
{
  for (int i = 0; i < WINUI_RUNTIME_VERSIONS; i++)
  {
    wchar_t family[128];
    winuiFrameworkFamily(winui_runtime_versions[i], family, 128);

    wchar_t dir[1024];
    if (!winuiFindFrameworkPath(family, dir, 1024))
      continue;

    DLL_DIRECTORY_COOKIE cookie = AddDllDirectory(dir);
    winuiPathPrepend(dir);

    wchar_t filename[1024];
    swprintf(filename, 1024, L"%s\\Microsoft.WindowsAppRuntime.dll", dir);
    HMODULE runtime = LoadLibraryExW(filename, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    TryCreatePackageDependencyFunc tryCreate = runtime ? (TryCreatePackageDependencyFunc)GetProcAddress(runtime, "MddTryCreatePackageDependency") : NULL;
    AddPackageDependencyFunc add = runtime ? (AddPackageDependencyFunc)GetProcAddress(runtime, "MddAddPackageDependency") : NULL;
    int ok = tryCreate && add && winuiAddDependency(tryCreate, add, family);

    winuiPathRemove(dir);
    if (cookie)
      RemoveDllDirectory(cookie);

    if (ok)
    {
      winui_runtime_module = runtime;
      return 1;
    }
    if (runtime)
      FreeLibrary(runtime);
  }
  return 0;
}

static void iupwinuiShutdownDependency(void)
{
  HMODULE module = winui_runtime_module ? winui_runtime_module : GetModuleHandleW(L"kernelbase.dll");
  const char* remove_name = winui_runtime_module ? "MddRemovePackageDependency" : "RemovePackageDependency";
  const char* delete_name = winui_runtime_module ? "MddDeletePackageDependency" : "DeletePackageDependency";
  RemovePackageDependencyFunc remove = module ? (RemovePackageDependencyFunc)GetProcAddress(module, remove_name) : NULL;
  FARPROC del = module ? GetProcAddress(module, delete_name) : NULL;

  if (winui_dependency_context && remove)
    remove(winui_dependency_context);
  winui_dependency_context = NULL;

  if (winui_dependency_id)
  {
    if (del && winui_runtime_module)
      ((MddDeletePackageDependencyFunc)del)(winui_dependency_id);
    else if (del)
      ((DeletePackageDependencyFunc)del)(winui_dependency_id);
    HeapFree(GetProcessHeap(), 0, winui_dependency_id);
    winui_dependency_id = NULL;
  }

  if (winui_runtime_module)
  {
    FreeLibrary(winui_runtime_module);
    winui_runtime_module = NULL;
  }
}

static void winuiRuntimeMissing(void)
{
  wchar_t caption[MAX_PATH + 64];
  wchar_t module[MAX_PATH];
  DWORD len = GetModuleFileNameW(NULL, module, MAX_PATH);
  wchar_t* sep = (len > 0 && len < MAX_PATH) ? wcsrchr(module, L'\\') : NULL;
  swprintf(caption, MAX_PATH + 64, L"%s - This application could not be started", sep ? sep + 1 : L"IUP");

  if (MessageBoxW(NULL, L"The Windows App Runtime is missing.\n    Version 1.5 to 1.8 or 2.x\n\nDo you want to install the Windows App Runtime now?", caption, MB_YESNO | MB_ICONERROR) == IDYES)
    ShellExecuteW(NULL, L"open", L"https://learn.microsoft.com/windows/apps/windows-app-sdk/downloads", NULL, NULL, SW_SHOWNORMAL);
}

static int iupwinuiInitBootstrap(void)
{
  if (iupwinuiInitDependency() || iupwinuiInitFramework())
    return 1;

  winuiRuntimeMissing();
  return 0;
}

typedef BOOL (__stdcall *ContentPreTranslateMessageFunc)(const MSG*);
static ContentPreTranslateMessageFunc winui_content_pretranslate = NULL;

static void iupwinuiFindContentPreTranslateMessage(void)
{
  HMODULE hModule = GetModuleHandleW(L"Microsoft.UI.Windowing.Core.dll");
  if (!hModule)
    hModule = LoadLibraryW(L"Microsoft.UI.Windowing.Core.dll");

  if (hModule)
  {
    winui_content_pretranslate = (ContentPreTranslateMessageFunc)
      GetProcAddress(hModule, "ContentPreTranslateMessage");
  }
}

IUP_DRV_API BOOL iupwinuiContentPreTranslateMessage(const MSG* msg)
{
  if (!winui_content_pretranslate)
  {
    static bool checked = false;
    if (!checked)
    {
      iupwinuiFindContentPreTranslateMessage();
      checked = true;
    }
  }

  if (winui_content_pretranslate)
    return winui_content_pretranslate(msg);
  return FALSE;
}

IUP_DRV_API void iupwinuiProcessPendingMessages(void)
{
  MSG msg;
  while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
  {
    if (!iupwinuiContentPreTranslateMessage(&msg))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
}

extern "C" IUP_SDK_API int iupdrvOpen(int *argc, char ***argv)
{
  (void)argc;
  (void)argv;

  if (winui_initialized)
    return IUP_NOERROR;

  init_apartment(apartment_type::single_threaded);

  winui_bootstrap_initialized = iupwinuiInitBootstrap();
  if (!winui_bootstrap_initialized)
  {
    uninit_apartment();
    return IUP_ERROR;
  }

  winui_dispatcher_controller = DispatcherQueueController::CreateOnCurrentThread();

  winui_app = make<IupWinUIApp>();
  winui_app.ResourceManagerRequested(winuiResourceManagerRequested);

  winui_xaml_manager = WindowsXamlManager::InitializeForCurrentThread();

  iupwinuiFindContentPreTranslateMessage();

  winui_app.Resources().MergedDictionaries().Append(XamlControlsResources());

  setlocale(LC_NUMERIC, "C");

  IupSetGlobal("DRIVER", "WinUI");
  IupSetGlobal("WINDOWING", "DWM");
  IupSetGlobal("WINUIVERSION", "3");

  if (argv && *argv && (*argv)[0] && (*argv)[0][0] != 0)
  {
    IupStoreGlobal("ARGV0", (*argv)[0]);
  }

  iupwinuiSetGlobalColors();

  IupSetGlobal("SHOWMENUIMAGES", "YES");

  iupwinuiProcessPendingMessages();

  winui_initialized = true;

  return IUP_NOERROR;
}

extern "C" IUP_SDK_API int iupdrvSetGlobalAppIDAttrib(const char* value)
{
  static int appid_set = 0;
  if (appid_set || !value || !value[0])
    return 0;

  IupStoreGlobal("_IUP_APPID_INTERNAL", value);
  appid_set = 1;
  return 1;
}

extern "C" IUP_SDK_API int iupdrvSetGlobalAppNameAttrib(const char* value)
{
  static int appname_set = 0;
  if (appname_set || !value || !value[0])
    return 0;

  WCHAR wappname[129];
  MultiByteToWideChar(CP_UTF8, 0, value, -1, wappname, 129);

  HRESULT hr = SetCurrentProcessExplicitAppUserModelID(wappname);
  if (SUCCEEDED(hr))
  {
    appname_set = 1;
    return 1;
  }
  return 0;
}

IUP_DRV_API void iupwinuiDrawCleanup(void);

extern "C" IUP_SDK_API void iupdrvClose(void)
{
  iupwinuiDrawCleanup();
  iupwinuiLoopCleanup();

  if (winui_initialized)
  {
    winui_initialized = false;

    if (winui_xaml_manager)
    {
      winui_xaml_manager.Close();
      winui_xaml_manager = nullptr;
    }

    winui_app = nullptr;

    if (winui_dispatcher_controller)
    {
      winui_dispatcher_controller.ShutdownQueue();
      winui_dispatcher_controller = nullptr;
    }

    winui_content_pretranslate = NULL;

    iupwinuiShutdownDependency();
    winui_bootstrap_initialized = false;
  }
}
