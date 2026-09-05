/** \file
 * \brief Haiku Driver Core. BApplication runs on the team main thread; the
 * IupHaikuApp subclass routes 'IuPM' / 'IuIT' codes to IUP-side hooks.
 *
 * See Copyright Notice in "iup.h"
 */

#include <clocale>

#include <Application.h>
#include <AppFileInfo.h>
#include <File.h>
#include <Message.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <InterfaceDefs.h>
#include <Mime.h>
#include <Roster.h>
#include <Window.h>

extern "C" {
#include "iup.h"
#include "iup_drv.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_globalattrib.h"
#include "iup_dlglist.h"
}

#include "iuphaiku_drv.h"


class IupHaikuApp : public BApplication {
public:
  explicit IupHaikuApp(const char* sig)
    : BApplication(sig), fIdleRunner(NULL) {}

  ~IupHaikuApp() override
  {
    if (fIdleRunner) { delete fIdleRunner; fIdleRunner = NULL; }
  }

  void DispatchMessage(BMessage* msg, BHandler* handler) override
  {
    switch (msg->what) {
      case IUPHAIKU_APP_DRAIN_MSG:
        iuphaikuAppDrainPosts();
        return;
      case IUPHAIKU_APP_IDLE_TICK:
        iuphaikuAppIdleTick();
        return;
      case IUPHAIKU_SI_MSG:
        iuphaikuSingleInstanceDispatch(msg);
        return;
      case B_COLORS_UPDATED:
        iupdrvSetAppearance(iupGlobalGetAppearance());
        iupGlobalUpdateThemeColors();
        for (Ihandle* dlg = iupDlgListFirst(); dlg; dlg = iupDlgListNext())
        {
          if (!dlg->handle) continue;
          BMessage tc(IUPHAIKU_THEME_CHANGED_MSG);
          BMessenger((BWindow*)dlg->handle).SendMessage(&tc);
        }
        break;
      case IUPHAIKU_APP_SHOW_WIN: {
        BWindow* win = NULL;
        if (msg->FindPointer("win", (void**)&win) == B_OK && win)
        {
          if (win->Lock())
          {
            if (win->IsHidden()) win->Show();
            win->Unlock();
          }
        }
        return;
      }
      case IUPHAIKU_MENU_ITEM_MSG: {
        Ihandle* item_ih = NULL;
        msg->FindPointer("ih", (void**)&item_ih);
        if (item_ih && iupObjectCheck(item_ih))
        {
          if (iupAttribGetBoolean(item_ih, "AUTOTOGGLE"))
          {
            int v = iupStrBoolean(iupAttribGet(item_ih, "VALUE"));
            IupSetAttribute(item_ih, "VALUE", v ? "OFF" : "ON");
          }
          Icallback cb = IupGetCallback(item_ih, "ACTION");
          if (cb && cb(item_ih) == IUP_CLOSE) IupExitLoop();
        }
        return;
      }
      case IUPHAIKU_MENU_RECENT_MSG: {
        Ihandle* menu_ih = NULL;
        int32 index = -1;
        msg->FindPointer("menu", (void**)&menu_ih);
        msg->FindInt32("index", &index);
        iuphaikuRecentDispatch(menu_ih, (int)index);
        return;
      }
    }
    BApplication::DispatchMessage(msg, handler);
  }

  /* IUP dialogs always veto BWindow::QuitRequested; CLOSE_CB / IupExitLoop drive teardown. */
  bool QuitRequested() override
  {
    iuphaikuAppStopIdleRunner();
    return true;
  }

  void StartIdleRunner()
  {
    if (fIdleRunner) return;
    BMessage tick(IUPHAIKU_APP_IDLE_TICK);
    fIdleRunner = new BMessageRunner(BMessenger(this), &tick, 1000);
  }

  void StopIdleRunner()
  {
    if (fIdleRunner) { delete fIdleRunner; fIdleRunner = NULL; }
  }

private:
  BMessageRunner* fIdleRunner;
};


static IupHaikuApp* iuphaiku_app = NULL;
static int iuphaiku_owns_app = 0;


IUP_DRV_API BApplication* iuphaikuGetApplication()
{
  return be_app;
}

IUP_DRV_API void iuphaikuAppStartIdleRunner()
{
  if (iuphaiku_app) iuphaiku_app->StartIdleRunner();
}

IUP_DRV_API void iuphaikuAppStopIdleRunner()
{
  if (iuphaiku_app) iuphaiku_app->StopIdleRunner();
}

IUP_DRV_API void iuphaikuSetGlobalColors()
{
  rgb_color c;

  c = ui_color(B_PANEL_BACKGROUND_COLOR);
  iupGlobalSetDefaultColorAttrib("DLGBGCOLOR", c.red, c.green, c.blue);

  c = ui_color(B_PANEL_TEXT_COLOR);
  iupGlobalSetDefaultColorAttrib("DLGFGCOLOR", c.red, c.green, c.blue);

  c = ui_color(B_DOCUMENT_BACKGROUND_COLOR);
  iupGlobalSetDefaultColorAttrib("TXTBGCOLOR", c.red, c.green, c.blue);

  c = ui_color(B_DOCUMENT_TEXT_COLOR);
  iupGlobalSetDefaultColorAttrib("TXTFGCOLOR", c.red, c.green, c.blue);

  c = ui_color(B_CONTROL_HIGHLIGHT_COLOR);
  iupGlobalSetDefaultColorAttrib("TXTHLCOLOR", c.red, c.green, c.blue);
  iupGlobalSetDefaultColorAttrib("ACCENTCOLOR", c.red, c.green, c.blue);

  c = ui_color(B_MENU_BACKGROUND_COLOR);
  iupGlobalSetDefaultColorAttrib("MENUBGCOLOR", c.red, c.green, c.blue);

  c = ui_color(B_MENU_ITEM_TEXT_COLOR);
  iupGlobalSetDefaultColorAttrib("MENUFGCOLOR", c.red, c.green, c.blue);

  iupGlobalSetDefaultColorAttrib("LINKFGCOLOR", 0, 0, 238);
}

extern "C" IUP_SDK_API int iupdrvIsSystemDarkMode(void)
{
  rgb_color c = ui_color(B_PANEL_BACKGROUND_COLOR);
  int luminance = (int)(0.299 * c.red + 0.587 * c.green + 0.114 * c.blue);
  return luminance < 128 ? 1 : 0;
}

static int iuphaiku_forced_dark = -1;

static rgb_color haikuGlobalColor(const char* name)
{
  rgb_color c = { 0, 0, 0, 255 };
  iupStrToRGB(IupGetGlobal(name), &c.red, &c.green, &c.blue);
  return c;
}

static rgb_color haikuMixColor(rgb_color a, rgb_color b, float t)
{
  rgb_color c;
  c.red = (uint8)(a.red + (b.red - a.red) * t);
  c.green = (uint8)(a.green + (b.green - a.green) * t);
  c.blue = (uint8)(a.blue + (b.blue - a.blue) * t);
  c.alpha = 255;
  return c;
}

IUP_DRV_API int iuphaikuColorForced()
{
  return iuphaiku_forced_dark >= 0;
}

/* the UI colors are system wide; a forced APPEARANCE maps them onto the IUP palette per application */
IUP_DRV_API rgb_color iuphaikuColor(color_which which)
{
  if (iuphaiku_forced_dark < 0)
    return ui_color(which);

  switch (which)
  {
    case B_PANEL_BACKGROUND_COLOR:
      return haikuGlobalColor("DLGBGCOLOR");
    case B_PANEL_TEXT_COLOR:
    case B_CONTROL_TEXT_COLOR:
    case B_WINDOW_TEXT_COLOR:
      return haikuGlobalColor("DLGFGCOLOR");
    case B_CONTROL_BACKGROUND_COLOR:
      return tint_color(haikuGlobalColor("DLGBGCOLOR"), iuphaiku_forced_dark ? B_LIGHTEN_1_TINT : B_LIGHTEN_2_TINT);
    case B_CONTROL_BORDER_COLOR:
      return haikuMixColor(haikuGlobalColor("DLGBGCOLOR"), haikuGlobalColor("DLGFGCOLOR"), 0.4f);
    case B_SCROLL_BAR_THUMB_COLOR:
      return haikuMixColor(haikuGlobalColor("DLGBGCOLOR"), haikuGlobalColor("DLGFGCOLOR"), 0.25f);
    case B_DOCUMENT_BACKGROUND_COLOR:
    case B_LIST_BACKGROUND_COLOR:
      return haikuGlobalColor("TXTBGCOLOR");
    case B_DOCUMENT_TEXT_COLOR:
    case B_LIST_ITEM_TEXT_COLOR:
    case B_LIST_SELECTED_ITEM_TEXT_COLOR:
      return haikuGlobalColor("TXTFGCOLOR");
    case B_LIST_SELECTED_BACKGROUND_COLOR:
      return haikuMixColor(haikuGlobalColor("TXTBGCOLOR"), haikuGlobalColor("TXTFGCOLOR"), 0.3f);
    case B_MENU_BACKGROUND_COLOR:
      return haikuGlobalColor("MENUBGCOLOR");
    case B_MENU_ITEM_TEXT_COLOR:
    case B_MENU_SELECTED_ITEM_TEXT_COLOR:
      return haikuGlobalColor("MENUFGCOLOR");
    case B_MENU_SELECTED_BACKGROUND_COLOR:
      return haikuMixColor(haikuGlobalColor("MENUBGCOLOR"), haikuGlobalColor("MENUFGCOLOR"), 0.3f);
    case B_LINK_TEXT_COLOR:
      return haikuGlobalColor("LINKFGCOLOR");
    default:
      return ui_color(which);
  }
}

extern "C" IUP_SDK_API void iupdrvSetAppearance(int appearance)
{
  int dark = (appearance == IUP_APPEARANCE_DARK)? 1: 0;

  iuphaikuSetGlobalColors();
  iuphaiku_forced_dark = -1;

  if (appearance != IUP_APPEARANCE_SYSTEM && iupdrvIsSystemDarkMode() != dark)
  {
    iupGlobalSetAppearanceColors(dark);
    iuphaiku_forced_dark = dark;
  }

  iuphaikuControlLookUpdate();

  for (Ihandle* dlg = iupDlgListFirst(); dlg; dlg = iupDlgListNext())
  {
    Ihandle* menu = IupGetAttributeHandle(dlg, "MENU");
    if (menu && menu->handle)
      iuphaikuMenuBarUpdateColors(menu);
  }
}

extern "C" IUP_SDK_API int iupdrvOpen(int *argc, char ***argv)
{
  if (be_app == NULL)
  {
    const char* sig = "application/x-vnd.iup-Application";
    char* user_sig = IupGetGlobal("APPID");
    if (user_sig && user_sig[0]) sig = user_sig;

    iuphaiku_app = new IupHaikuApp(sig);
    iuphaiku_owns_app = 1;

    app_info info;
    if (iuphaiku_app->GetAppInfo(&info) == B_OK)
    {
      BFile file(&info.ref, B_READ_WRITE);
      if (file.InitCheck() == B_OK)
      {
        BAppFileInfo finfo(&file);
        finfo.SetSignature(sig);
        finfo.SetType("application/x-vnd.Be-elfexecutable");
      }
      BMimeType mime(sig);
      if (mime.InitCheck() == B_OK)
      {
        if (!mime.IsInstalled()) mime.Install();
        mime.SetPreferredApp(sig);
        mime.SetAppHint(&info.ref);
      }
    }
  }
  else
  {
    iuphaiku_app = (IupHaikuApp*)be_app;
    iuphaiku_owns_app = 0;
  }

  setlocale(LC_NUMERIC, "C");

  IupSetGlobal("DRIVER", "Haiku");
  IupSetGlobal("WINDOWING", "HAIKU");

  if (argc && argv && *argv && (*argv)[0] && (*argv)[0][0] != 0)
    IupStoreGlobal("ARGV0", (*argv)[0]);

  iuphaikuSetGlobalColors();
  iuphaikuControlLookInstall();

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

  appname_set = 1;
  return 1;
}

extern "C" IUP_SDK_API void iupdrvClose(void)
{
  iuphaikuLoopCleanup();

  if (iuphaiku_app && iuphaiku_owns_app)
    delete iuphaiku_app;
  iuphaiku_app = NULL;
  iuphaiku_owns_app = 0;
}
