/** \file
 * \brief WinUI Driver Core - Initialization and Setup using XAML Islands
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdlib>
#include <cstring>
#include <clocale>
#include "pch.h"
#include <shobjidl.h>

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


/* PACKAGE_VERSION structure for Windows App SDK Bootstrap */
typedef struct {
  union {
    UINT64 Version;
    struct {
      USHORT Revision;
      USHORT Build;
      USHORT Minor;
      USHORT Major;
    };
  };
} PACKAGE_VERSION_T;

/* Dynamic loading of Windows App SDK Bootstrap */
typedef HRESULT (WINAPI *MddBootstrapInitializeFunc)(UINT32, PCWSTR, PACKAGE_VERSION_T);
typedef void (WINAPI *MddBootstrapShutdownFunc)(void);

static HMODULE winui_bootstrap_module = NULL;
static MddBootstrapShutdownFunc winui_bootstrap_shutdown = NULL;

extern "C" {
#include "iup.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_object.h"
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
 * WinUI 3 controls require an Application with IXamlMetadataProvider
 * to provide their visual templates and styles via XamlControlsResources.
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

/****************************************************************************
 * System Theme Detection
 ****************************************************************************/

extern "C" int iupwinuiIsSystemDarkMode(void)
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

extern "C" void iupwinuiSetGlobalColors(void)
{
  using namespace Windows::UI::ViewManagement;

  UISettings settings;

  auto bg = settings.GetColorValue(UIColorType::Background);
  auto fg = settings.GetColorValue(UIColorType::Foreground);
  auto accent = settings.GetColorValue(UIColorType::Accent);

  int dark_mode = iupwinuiIsSystemDarkMode();

  iupGlobalSetDefaultColorAttrib("DLGBGCOLOR", bg.R, bg.G, bg.B);
  iupGlobalSetDefaultColorAttrib("DLGFGCOLOR", fg.R, fg.G, fg.B);

  if (dark_mode)
    iupGlobalSetDefaultColorAttrib("TXTBGCOLOR", 32, 32, 32);
  else
    iupGlobalSetDefaultColorAttrib("TXTBGCOLOR", 255, 255, 255);

  iupGlobalSetDefaultColorAttrib("TXTFGCOLOR", fg.R, fg.G, fg.B);
  iupGlobalSetDefaultColorAttrib("TXTHLCOLOR", accent.R, accent.G, accent.B);
  iupGlobalSetDefaultColorAttrib("MENUBGCOLOR", bg.R, bg.G, bg.B);
  iupGlobalSetDefaultColorAttrib("MENUFGCOLOR", fg.R, fg.G, fg.B);
  iupGlobalSetDefaultColorAttrib("LINKFGCOLOR", accent.R, accent.G, accent.B);
}

/****************************************************************************
 * Dispatcher Queue Access
 ****************************************************************************/

extern "C" void* iupwinuiGetDispatcherQueue(void)
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

static int iupwinuiInitBootstrap(void)
{
  /* Try to load the Windows App SDK Bootstrap DLL */
  winui_bootstrap_module = LoadLibraryW(L"Microsoft.WindowsAppRuntime.Bootstrap.dll");
  if (!winui_bootstrap_module)
  {
    return 0;
  }

  MddBootstrapInitializeFunc initFunc = (MddBootstrapInitializeFunc)
    GetProcAddress(winui_bootstrap_module, "MddBootstrapInitialize2");
  if (!initFunc)
  {
    initFunc = (MddBootstrapInitializeFunc)
      GetProcAddress(winui_bootstrap_module, "MddBootstrapInitialize");
  }
  winui_bootstrap_shutdown = (MddBootstrapShutdownFunc)
    GetProcAddress(winui_bootstrap_module, "MddBootstrapShutdown");

  if (!initFunc)
  {
    FreeLibrary(winui_bootstrap_module);
    winui_bootstrap_module = NULL;
    return 0;
  }

  PACKAGE_VERSION_T minVersion = {};
  minVersion.Major = 1;
  minVersion.Minor = 0;

  /* Try Windows App SDK versions from newest to oldest */
  UINT32 versions[] = { 0x00010008, 0x00010007, 0x00010006, 0x00010005, 0x00010004, 0x00010003, 0x00010002, 0x00010001 };
  HRESULT lastHr = S_OK;
  for (int i = 0; i < 8; i++)
  {
    HRESULT hr = initFunc(versions[i], NULL, minVersion);
    if (SUCCEEDED(hr))
    {
      return 1;
    }
    lastHr = hr;
  }

  FreeLibrary(winui_bootstrap_module);
  winui_bootstrap_module = NULL;
  return 0;
}

typedef BOOL (__stdcall *ContentPreTranslateMessageFunc)(const MSG*);
static ContentPreTranslateMessageFunc winui_content_pretranslate = NULL;

static void iupwinuiFindContentPreTranslateMessage(void)
{
  HMODULE hModule = GetModuleHandleW(L"Microsoft.UI.Xaml.dll");
  if (!hModule)
    hModule = GetModuleHandleW(L"Microsoft.ui.xaml.dll");

  if (hModule)
  {
    winui_content_pretranslate = (ContentPreTranslateMessageFunc)
      GetProcAddress(hModule, "ContentPreTranslateMessage");
  }
}

extern "C" BOOL iupwinuiContentPreTranslateMessage(const MSG* msg)
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

void iupwinuiProcessPendingMessages(void)
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

extern "C" int iupdrvOpen(int *argc, char ***argv)
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

  winui_xaml_manager = WindowsXamlManager::InitializeForCurrentThread();

  iupwinuiFindContentPreTranslateMessage();

  winui_app.Resources().MergedDictionaries().Append(XamlControlsResources());

  setlocale(LC_NUMERIC, "C");

  IupSetGlobal("DRIVER", "WinUI");
  IupSetGlobal("WINDOWING", "WINUI");
  IupSetGlobal("WINUIVERSION", "3");

  if (argv && *argv && (*argv)[0] && (*argv)[0][0] != 0)
  {
    IupStoreGlobal("ARGV0", (*argv)[0]);
  }

  iupwinuiSetGlobalColors();

  IupSetGlobal("SHOWMENUIMAGES", "YES");

  if (iupwinuiIsSystemDarkMode())
    IupSetGlobal("DARKMODE", "YES");

  iupwinuiProcessPendingMessages();

  winui_initialized = true;

  return IUP_NOERROR;
}

extern "C" int iupdrvSetGlobalAppIDAttrib(const char* value)
{
  static int appid_set = 0;
  if (appid_set || !value || !value[0])
    return 0;

  IupStoreGlobal("_IUP_APPID_INTERNAL", value);
  appid_set = 1;
  return 1;
}

extern "C" int iupdrvSetGlobalAppNameAttrib(const char* value)
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

extern "C" void iupwinuiDrawCleanup(void);

extern "C" void iupdrvClose(void)
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
      auto shutdownOp = winui_dispatcher_controller.ShutdownQueueAsync();
      HANDLE event = CreateEvent(NULL, TRUE, FALSE, NULL);
      if (event)
      {
        shutdownOp.Completed([event](auto&&, auto&&) { SetEvent(event); });
        while (true)
        {
          DWORD result = MsgWaitForMultipleObjectsEx(1, &event, 5000, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
          if (result == WAIT_OBJECT_0)
            break;
          if (result == WAIT_OBJECT_0 + 1)
          {
            MSG msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
              TranslateMessage(&msg);
              DispatchMessage(&msg);
            }
          }
          else
            break;
        }
        CloseHandle(event);
      }
      winui_dispatcher_controller = nullptr;
    }

    winui_content_pretranslate = NULL;

    if (winui_bootstrap_initialized && winui_bootstrap_shutdown)
    {
      winui_bootstrap_shutdown();
      winui_bootstrap_initialized = false;
    }

    if (winui_bootstrap_module)
    {
      FreeLibrary(winui_bootstrap_module);
      winui_bootstrap_module = NULL;
    }
  }
}
