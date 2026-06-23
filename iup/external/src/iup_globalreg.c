/** \file
 * \brief Registry of known IUP global attributes
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <string.h>

#include "iup.h"

#include "iup_globalreg.h"
#include "iup_globalattrib.h"
#include "iup_str.h"
#include "iup_assert.h"


#define W   IUPDRV_WIN
#define M   IUPDRV_MOTIF
#define G   IUPDRV_GTK
#define C   IUPDRV_COCOA
#define Q   IUPDRV_QT
#define G4  IUPDRV_GTK4
#define E   IUPDRV_EFL
#define WU  IUPDRV_WINUI
#define F   IUPDRV_FLTK
#define A   IUPDRV_ANDROID
#define CT  IUPDRV_COCOATOUCH
#define HK  IUPDRV_HAIKU
#define WS  IUPDRV_WASM
#define ALL  (W|M|G|C|Q|G4|E|WU|F|A|CT|HK|WS)
#define UNIX (M|G|C|Q|G4|E|F|HK)
#define MOB  (A|CT)
#define R  IUPGF_READONLY
#define P  IUPGF_POINTER

/* Sorted alphabetically; keep in sync when a driver adds a global. */
static const iGlobalRegEntry registry[] = {
  { "ACCENTCOLOR",            0,    ALL },
  { "APPID",                  0,    G|Q|G4|E|WU|A|CT|HK },
  { "APPNAME",                0,    W|C|Q|E|WU|F|A|CT },
  { "APPSHELL",               R|P,  M|G|G4 },
  { "ARGV0",                  0,    G|Q|G4|E|WU|F|HK },
  { "AUTODARKMODE",           0,    W },
  { "AUTOREPEAT",             0,    M|E|F },
  { "CACHEDIR",               R,    ALL },
  { "COMCTL32VER6",           R,    W },
  { "COMPUTERNAME",           R,    ALL },
  { "CONFIGDIR",              R,    ALL },
  { "CONTROLKEY",             R,    ALL },
  { "COPYRIGHT",              R,    ALL },
  { "CURSORPOS",              0,    ALL },
  { "DARKMODE",               R,    ALL },
  { "DATADIR",                R,    ALL },
  { "DEFAULTBUTTONPADDING",   0,    ALL },
  { "DEFAULTDECIMALSYMBOL",   0,    ALL },
  { "DEFAULTFONT",            0,    ALL },
  { "DEFAULTFONTFACE",        0,    ALL },
  { "DEFAULTFONTSIZE",        0,    ALL },
  { "DEFAULTFONTSTYLE",       0,    ALL },
  { "DEFAULTPRECISION",       0,    ALL },
  { "DEFAULTTHEME",           0,    ALL },
  { "DLGBGCOLOR",             0,    ALL },
  { "DLGFGCOLOR",             0,    ALL },
  { "DLL_HINSTANCE",          R|P,  W },
  { "DRIVER",                 R,    ALL },
  { "DWM_COMPOSITION",        R,    W|E },
  { "EFLACCEL",               0,    E },
  { "EFLENGINE",              R,    E },
  { "EFLTHEME",               0,    E },
  { "EFLTHEMEDATA",           0,    E },
  { "EFLTHEMEDATALEN",        0,    E },
  { "EFLVERSION",             R,    E },
  { "EXEFILENAME",            R,    ALL },
  { "EXITLOOP",               0,    ALL },
  { "FLTKTHEME",              0,    F },
  { "FLTKVERSION",            R,    F },
  { "FULLSIZE",               R,    ALL },
  { "GLOBALLAYOUTDLGKEY",     0,    ALL },
  { "GLOBALLAYOUTRESIZEKEY",  0,    ALL },
  { "GLOBALMENU",             0,    G },
  { "GL_RENDERER",            R,    ALL },
  { "GL_VENDOR",              R,    ALL },
  { "GL_VERSION",             R,    ALL },
  { "GNUSTEPTHEME",           0,    C },
  { "GSKRENDERER",            0,    G4 },
  { "GTKDEVVERSION",          R,    G|G4 },
  { "GTKVERSION",             R,    G|G4 },
  { "HELPAPP",                0,    M|G|G4|E|F|HK },
  { "HINSTANCE",              R|P,  W },
  { "ICON",                   0,    ALL },
  { "IMAGEAUTOSCALE",         0,    ALL },
  { "IMAGEEXPORT_STATIC",     0,    ALL },
  { "IMAGESDPI",              0,    ALL },
  { "IMAGESTOCKAUTOSCALE",    0,    ALL },
  { "IMAGESTOCKSIZE",         0,    ALL },
  { "INPUTCALLBACKS",         0,    W|M|G|C|Q|A|HK },
  { "KEY",                    R,    ALL },
  { "KEYPRESS",               R,    ALL },
  { "KEYRELEASE",             R,    ALL },
  { "LANGUAGE",               0,    ALL },
  { "LASTERROR",              0,    W|WU },
  { "LINKFGCOLOR",            0,    ALL },
  { "LOCKLOOP",               0,    ALL },
  { "MENUBGCOLOR",            0,    W|C|Q|E|WU|F|HK|WS },
  { "MENUFGCOLOR",            R,    W|C|Q|E|WU|F|HK|WS },
  { "MODKEYSTATE",            R,    ALL },
  { "MONITORSCOUNT",          R,    W|G|C|Q|G4|E|WU|F|A|CT|HK|WS },
  { "MONITORSINFO",           R,    W|G|C|Q|G4|E|WU|F|A|CT|HK|WS },
  { "MOTIFNUMBER",            R,    M },
  { "MOTIFVERSION",           R,    M },
  { "MOUSEBUTTON",            R,    ALL },
  { "OVERLAYSCROLLBAR",       0,    G|G4 },
  { "PARENTDIALOG",           0,    ALL },
  { "QTBUILDTYPE",            R,    Q },
  { "QTDEVVERSION",           R,    Q },
  { "QTSTYLE",                0,    Q },
  { "QTVERSION",              R,    Q },
  { "SANDBOX",                R,    G|Q|G4|F },
  { "SB_BGCOLOR",             0,    M|G|G4 },
  { "SCREENDEPTH",            R,    ALL },
  { "SCREENDPI",              R,    ALL },
  { "SCREENSIZE",             R,    ALL },
  { "SCROLLBARSIZE",          R,    ALL },
  { "SHIFTKEY",               R,    ALL },
  { "SHORTCUTKEY",            R,    HK },
  { "SHOWMENUIMAGES",         0,    G|Q|G4|E|WU|F|HK },
  { "SINGLEINSTANCE",         0,    ALL },
  { "SYSTEM",                 R,    ALL },
  { "SYSTEMLANGUAGE",         R,    ALL },
  { "SYSTEMLOCALE",           R,    ALL },
  { "SYSTEMVERSION",          R,    ALL },
  { "TMPDIR",                 R,    ALL },
  { "TREEIMAGE24",            0,    ALL },
  { "TRUECOLORCANVAS",        R,    ALL },
  { "TXTBGCOLOR",             0,    ALL },
  { "TXTFGCOLOR",             0,    ALL },
  { "TXTHLCOLOR",             0,    ALL },
  { "USERNAME",               R,    ALL },
  { "UTF8AUTOCONVERT",        0,    G|C|Q|G4|WU|F|CT|HK|WS },
  { "UTF8MODE",               0,    W|G|C|Q|G4|E|WU|F|A|HK|WS },
  { "UTF8MODE_FILE",          0,    W },
  { "VERSION",                R,    ALL },
  { "VIRTUALSCREEN",          R,    W|G|C|Q|G4|E|WU|F|A|CT|HK|WS },
  { "WINDOWING",              R,    ALL },
  { "WINUIVERSION",           R,    WU },
  { "WL_DISPLAY",             R|P,  G|Q|G4|E|F },
  { "XDISPLAY",               R|P,  M|G|G4|E|F },
  { "XSCREEN",                R|P,  M|G|G4|E|F },
  { "XSERVERVENDOR",          R,    M|G|G4|F },
  { "XVENDORRELEASE",         R,    M|G|G4|F }
};

#define REG_COUNT ((int)(sizeof(registry)/sizeof(registry[0])))

IUP_SDK_API int iupGlobalRegCount(void)
{
  return REG_COUNT;
}

IUP_SDK_API const iGlobalRegEntry* iupGlobalRegAt(int index)
{
  if (index < 0 || index >= REG_COUNT)
    return NULL;
  return &registry[index];
}

IUP_SDK_API const iGlobalRegEntry* iupGlobalRegFind(const char* name)
{
  int lo, hi;

  if (!name)
    return NULL;

  lo = 0;
  hi = REG_COUNT - 1;
  while (lo <= hi)
  {
    int mid = (lo + hi) / 2;
    int cmp = strcmp(name, registry[mid].name);
    if (cmp == 0)
      return &registry[mid];
    if (cmp < 0) hi = mid - 1;
    else         lo = mid + 1;
  }
  return NULL;
}


IUP_API int IupGetAllGlobals(char** list, int n)
{
  int total, user_count, i, written;
  char** user_names = NULL;

  user_count = iupGetGlobalAttributes(NULL, 0);
  if (user_count > 0)
  {
    user_names = (char**)malloc(sizeof(char*) * user_count);
    if (user_names)
      user_count = iupGetGlobalAttributes(user_names, user_count);
    else
      user_count = 0;
  }

  total = REG_COUNT;
  for (i = 0; i < user_count; i++)
  {
    if (!iupGlobalRegFind(user_names[i]))
      total++;
  }

  if (!list || n == 0 || n == -1)
  {
    free(user_names);
    return total;
  }

  written = 0;
  for (i = 0; i < REG_COUNT && written < n; i++)
    list[written++] = (char*)registry[i].name;
  for (i = 0; i < user_count && written < n; i++)
  {
    if (!iupGlobalRegFind(user_names[i]))
      list[written++] = user_names[i];
  }

  free(user_names);
  return written;
}

IUP_API int IupGetGlobalInfo(const char* name, int* flags, int* drivers)
{
  const iGlobalRegEntry* e;

  iupASSERT(name != NULL);
  if (!name)
    return -1;

  e = iupGlobalRegFind(name);
  if (!e)
  {
    if (flags) *flags = 0;
    if (drivers) *drivers = 0;
    return 0;
  }

  if (flags) *flags = e->flags;
  if (drivers) *drivers = e->drivers;
  return 1;
}
