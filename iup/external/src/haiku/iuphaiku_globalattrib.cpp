/** \file
 * \brief Haiku Global Attributes (driver-side query/set)
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstddef>
#include <cstdio>
#include <cstdlib>

#include <InterfaceDefs.h>
#include <Point.h>
#include <Rect.h>
#include <Screen.h>
#include <View.h>

extern "C" {
#include "iup.h"
#include "iup_drv.h"
#include "iup_str.h"
#include "iup_singleinstance.h"
}

#include "iuphaiku_drv.h"


extern "C" IUP_SDK_API int iupdrvSetGlobal(const char* name, const char* value)
{
  if (iupStrEqual(name, "SINGLEINSTANCE"))
    return iupdrvSingleInstanceSet(value) ? 0 : 1;
  return 1;
}

extern "C" IUP_SDK_API char* iupdrvGetGlobal(const char* name)
{
  if (iupStrEqual(name, "MONITORSCOUNT"))
  {
    int count = 0;
    BScreen s(B_MAIN_SCREEN_ID);
    while (s.IsValid() && count < 32) { count++; if (s.SetToNext() != B_OK) break; }
    if (count == 0) count = 1;
    return iupStrReturnInt(count);
  }

  if (iupStrEqual(name, "VIRTUALSCREEN"))
  {
    BScreen s(B_MAIN_SCREEN_ID);
    BRect f = s.Frame();
    return iupStrReturnStrf("%d %d %d %d",
      (int)f.left, (int)f.top, (int)(f.Width() + 1), (int)(f.Height() + 1));
  }

  if (iupStrEqual(name, "SCREENSIZE"))
  {
    BScreen s(B_MAIN_SCREEN_ID);
    BRect f = s.Frame();
    return iupStrReturnStrf("%dx%d", (int)(f.Width() + 1), (int)(f.Height() + 1));
  }

  if (iupStrEqual(name, "FULLSIZE"))
  {
    /* Same as SCREENSIZE on Haiku - the desktop has no taskbar reservation
     * since BDeskbar floats over windows. */
    BScreen s(B_MAIN_SCREEN_ID);
    BRect f = s.Frame();
    return iupStrReturnStrf("%dx%d", (int)(f.Width() + 1), (int)(f.Height() + 1));
  }

  if (iupStrEqual(name, "MONITORSINFO"))
  {
    BScreen s(B_MAIN_SCREEN_ID);
    char* str = iupStrGetMemory(64 * 32);
    char* p = str;
    int idx = 0;
    while (s.IsValid() && idx < 32)
    {
      BRect f = s.Frame();
      int n = snprintf(p, 64, "%d %d %d %d\n",
        (int)f.left, (int)f.top, (int)(f.Width() + 1), (int)(f.Height() + 1));
      if (n < 0) break;
      p += n;
      idx++;
      if (s.SetToNext() != B_OK) break;
    }
    return str;
  }

  if (iupStrEqual(name, "SCREENDEPTH"))
  {
    BScreen s(B_MAIN_SCREEN_ID);
    color_space cs = s.ColorSpace();
    int depth = 32;
    if      (cs == B_RGB32 || cs == B_RGBA32) depth = 32;
    else if (cs == B_RGB24)                   depth = 24;
    else if (cs == B_RGB16 || cs == B_RGB15)  depth = 16;
    else if (cs == B_CMAP8)                   depth = 8;
    return iupStrReturnInt(depth);
  }

  if (iupStrEqual(name, "CURSORPOS"))
  {
    BPoint where;
    uint32 buttons = 0;
    get_mouse(&where, &buttons);
    return iupStrReturnIntInt((int)where.x, (int)where.y, 'x');
  }

  if (iupStrEqual(name, "MOUSEBUTTON"))
  {
    BPoint where;
    uint32 buttons = 0;
    get_mouse(&where, &buttons);
    char status[7] = "      ";
    if (buttons & B_PRIMARY_MOUSE_BUTTON)   status[0] = '1';
    if (buttons & B_TERTIARY_MOUSE_BUTTON)  status[1] = '2';
    if (buttons & B_SECONDARY_MOUSE_BUTTON) status[2] = '3';
    return iupStrReturnStr(status);
  }

  if (iupStrEqual(name, "DARKMODE"))
    return iupStrReturnBoolean(iuphaikuIsSystemDarkMode());

  /* "CTRL" in Windows-mode keymap, "ALT" in default Haiku keymap. */
  if (iupStrEqual(name, "SHORTCUTKEY"))
  {
    key_map* km = NULL;
    char* buf = NULL;
    get_key_map(&km, &buf);
    const char* result = "ALT";
    if (km)
    {
      /* PC keyboard scancodes: 0x5c left-ctrl, 0x5d left-alt. */
      if (km->left_command_key == 0x5c) result = "CTRL";
      free(km);
    }
    if (buf) free(buf);
    return (char*)result;
  }

  if (iupStrEqual(name, "TRUECOLORCANVAS"))
    return iupStrReturnBoolean(1);

  if (iupStrEqual(name, "UTF8MODE"))
    return iupStrReturnBoolean(1);

  if (iupStrEqual(name, "UTF8AUTOCONVERT"))
    return iupStrReturnBoolean(0);

  if (iupStrEqual(name, "SHOWMENUIMAGES"))
    return (char*)"NO";

  if (iupStrEqual(name, "EXEFILENAME"))
  {
    char* argv0 = IupGetGlobal("ARGV0");
    if (argv0)
    {
      char* full = realpath(argv0, NULL);
      if (full) { char* s = iupStrReturnStr(full); free(full); return s; }
    }
    return NULL;
  }

  return NULL;
}
