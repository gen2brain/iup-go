/** \file
 * \brief WebAssembly Driver Core
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <emscripten.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_object.h"
#include "iup_dlglist.h"
#include "iup_globalattrib.h"

#include "iupwasm_drv.h"


IUP_SDK_API void* iupdrvGetDisplay(void)
{
  return NULL;
}

IUP_SDK_API int iupdrvSetGlobalAppIDAttrib(const char* value)
{
  static int appid_set = 0;
  if (appid_set || !value || !value[0])
    return 0;

  IupStoreGlobal("_IUP_APPID_INTERNAL", value);
  appid_set = 1;
  return 1;
}

IUP_SDK_API int iupdrvSetGlobalAppNameAttrib(const char* value)
{
  static int appname_set = 0;
  if (appname_set || !value || !value[0])
    return 0;

  IupStoreGlobal("_IUP_APPNAME_INTERNAL", value);
  appname_set = 1;
  return 1;
}

EM_JS(int, iupwasmJsIsDarkMode, (void), {
  if (typeof window === 'undefined') return globalThis.__iupReadSync({ op: 'darkmode' });
  return (window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches) ? 1 : 0;
})

/* system-default colors switch with prefers-color-scheme; user-set globals still win */
static void wasmUpdateGlobalColors(int dark)
{
  if (dark)
  {
    iupGlobalSetDefaultColorAttrib("DLGBGCOLOR", 50, 50, 50);
    iupGlobalSetDefaultColorAttrib("DLGFGCOLOR", 230, 230, 230);
    iupGlobalSetDefaultColorAttrib("TXTBGCOLOR", 35, 35, 35);
    iupGlobalSetDefaultColorAttrib("TXTFGCOLOR", 230, 230, 230);
    iupGlobalSetDefaultColorAttrib("MENUBGCOLOR", 50, 50, 50);
    iupGlobalSetDefaultColorAttrib("MENUFGCOLOR", 230, 230, 230);
    iupGlobalSetDefaultColorAttrib("LINKFGCOLOR", 90, 160, 255);
    iupGlobalSetDefaultColorAttrib("ACCENTCOLOR", 0, 120, 215);
    iupGlobalSetDefaultColorAttrib("TXTHLCOLOR", 0, 120, 215);
  }
  else
  {
    iupGlobalSetDefaultColorAttrib("DLGBGCOLOR", 237, 237, 237);
    iupGlobalSetDefaultColorAttrib("DLGFGCOLOR", 0, 0, 0);
    iupGlobalSetDefaultColorAttrib("TXTBGCOLOR", 255, 255, 255);
    iupGlobalSetDefaultColorAttrib("TXTFGCOLOR", 0, 0, 0);
    iupGlobalSetDefaultColorAttrib("MENUBGCOLOR", 237, 237, 237);
    iupGlobalSetDefaultColorAttrib("MENUFGCOLOR", 0, 0, 0);
    iupGlobalSetDefaultColorAttrib("LINKFGCOLOR", 0, 0, 238);
    iupGlobalSetDefaultColorAttrib("ACCENTCOLOR", 0, 120, 215);
    iupGlobalSetDefaultColorAttrib("TXTHLCOLOR", 0, 120, 215);
  }
}

EM_JS(void, iupwasmJsInstallTheme, (int bgr, int bgg, int bgb, int fgr, int fgg, int fgb,
  int tbr, int tbg, int tbb, int tfr, int tfg, int tfb, int lr, int lg, int lb,
  int mbr, int mbg, int mbb, int mfr, int mfg, int mfb, int ar, int ag, int ab), {
  globalThis.__iupApply({ op: 'theme',
    bg: [bgr, bgg, bgb], fg: [fgr, fgg, fgb], txtbg: [tbr, tbg, tbb], txtfg: [tfr, tfg, tfb],
    link: [lr, lg, lb], menubg: [mbr, mbg, mbb], menufg: [mfr, mfg, mfb], accent: [ar, ag, ab] });
})

static void wasmRGB(const char* name, unsigned char rgb[3])
{
  char* v = IupGetGlobal(name);
  rgb[0] = rgb[1] = rgb[2] = 0;
  if (v)
    iupStrToRGB(v, &rgb[0], &rgb[1], &rgb[2]);
}

void iupwasmInstallTheme(void)
{
  unsigned char bg[3], fg[3], tb[3], tf[3], lk[3], mb[3], mf[3], ac[3];
  wasmRGB("DLGBGCOLOR", bg);
  wasmRGB("DLGFGCOLOR", fg);
  wasmRGB("TXTBGCOLOR", tb);
  wasmRGB("TXTFGCOLOR", tf);
  wasmRGB("LINKFGCOLOR", lk);
  wasmRGB("MENUBGCOLOR", mb);
  wasmRGB("MENUFGCOLOR", mf);
  wasmRGB("ACCENTCOLOR", ac);
  iupwasmJsInstallTheme(bg[0], bg[1], bg[2], fg[0], fg[1], fg[2], tb[0], tb[1], tb[2], tf[0], tf[1], tf[2],
    lk[0], lk[1], lk[2], mb[0], mb[1], mb[2], mf[0], mf[1], mf[2], ac[0], ac[1], ac[2]);
}

/* iup_globalattrib.c short-circuits iupdrvSetGlobal for the registered default colors, so the CSS
   variables are refreshed from whoever asks for a color next */
void iupwasmRefreshTheme(void)
{
  static const char* names[8] = { "DLGBGCOLOR", "DLGFGCOLOR", "TXTBGCOLOR", "TXTFGCOLOR",
                                  "LINKFGCOLOR", "MENUBGCOLOR", "MENUFGCOLOR", "ACCENTCOLOR" };
  static unsigned char last[8][3];
  static int has_last = 0;
  unsigned char cur[8][3];
  int i, changed = 0;

  for (i = 0; i < 8; i++)
  {
    wasmRGB(names[i], cur[i]);
    if (!has_last || cur[i][0] != last[i][0] || cur[i][1] != last[i][1] || cur[i][2] != last[i][2])
      changed = 1;
  }

  if (!changed)
    return;

  memcpy(last, cur, sizeof(last));
  has_last = 1;
  iupwasmInstallTheme();
}

EMSCRIPTEN_KEEPALIVE void iupwasmThemeChanged(void)
{
  int dark = iupwasmJsIsDarkMode();
  Ihandle* ih;
  wasmUpdateGlobalColors(dark);
  iupwasmInstallTheme();
  for (ih = iupDlgListFirst(); ih; ih = iupDlgListNext())
  {
    IFni cb = (IFni)IupGetCallback(ih, "THEMECHANGED_CB");
    if (cb)
      cb(ih, dark);
  }
}

EM_JS(int, iupwasmJsLanguage, (void), {
  var s = (typeof navigator !== 'undefined' && navigator.language) ? navigator.language : "";
  var len = lengthBytesUTF8(s) + 1;
  var ptr = _malloc(len);
  stringToUTF8(s, ptr, len);
  return ptr;
})

IUP_SDK_API int iupdrvOpen(int *argc, char ***argv)
{
  char* lang;
  (void)argc;
  (void)argv;

  IupSetGlobal("DRIVER", "WASM");

  lang = (char*)(intptr_t)iupwasmJsLanguage();
  if (lang)
  {
    if (lang[0])
      IupStoreGlobal("SYSTEMLANGUAGE", lang);
    free(lang);
  }

  iupwasmJsInstallProxy();
  iupwasmJsInstallKeyHandler();

  wasmUpdateGlobalColors(iupwasmJsIsDarkMode());
  IupSetGlobal("_IUP_RESET_GLOBALCOLORS", "YES");
  iupwasmInstallTheme();

  return IUP_NOERROR;
}

IUP_SDK_API void iupdrvClose(void)
{
}
