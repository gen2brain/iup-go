/** \file
 * \brief global attributes environment
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "iup.h"

#include "iup_hashtable.h"
#include "iup_globalattrib.h"
#include "iup_globalreg.h"
#include "iup_class.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_drvinfo.h"
#include "iup_assert.h"
#include "iup_str.h"
#include "iup_strmessage.h"
#include "iup_attrib.h"
#include "iup_dlglist.h"
#include "iup_object.h"


static Itable *iglobal_table = NULL;
static int iglobal_appearance = IUP_APPEARANCE_SYSTEM;

void iupGlobalAttribInit(void)
{
  iglobal_table = iupTableCreate(IUPTABLE_STRINGINDEXED);
}

void iupGlobalAttribFinish(void)
{
  iupTableDestroy(iglobal_table);
  iglobal_table = NULL;
  iglobal_appearance = IUP_APPEARANCE_SYSTEM;
}

static void iGlobalUpdateThemeTree(Ihandle* ih)
{
  Ihandle* child;

  if (ih->handle)
    iupClassObjectUpdateGlobalDefaults(ih);

  for (child = ih->firstchild; child; child = child->brother)
    iGlobalUpdateThemeTree(child);
}

IUP_SDK_API void iupGlobalUpdateThemeColors(void)
{
  Ihandle* dialog = iupDlgListFirst();
  while (dialog)
  {
    iGlobalUpdateThemeTree(dialog);
    IupRedraw(dialog, 1);
    dialog = iupDlgListNext();
  }
}

IUP_SDK_API void iupGlobalSetAppearanceColors(int dark)
{
  if (dark)
  {
    iupGlobalSetDefaultColorAttrib("DLGBGCOLOR", 32, 32, 32);
    iupGlobalSetDefaultColorAttrib("DLGFGCOLOR", 255, 255, 255);
    iupGlobalSetDefaultColorAttrib("TXTBGCOLOR", 45, 45, 45);
    iupGlobalSetDefaultColorAttrib("TXTFGCOLOR", 255, 255, 255);
    iupGlobalSetDefaultColorAttrib("MENUBGCOLOR", 43, 43, 43);
    iupGlobalSetDefaultColorAttrib("MENUFGCOLOR", 255, 255, 255);
    iupGlobalSetDefaultColorAttrib("LINKFGCOLOR", 100, 180, 255);
  }
  else
  {
    iupGlobalSetDefaultColorAttrib("DLGBGCOLOR", 240, 240, 240);
    iupGlobalSetDefaultColorAttrib("DLGFGCOLOR", 0, 0, 0);
    iupGlobalSetDefaultColorAttrib("TXTBGCOLOR", 255, 255, 255);
    iupGlobalSetDefaultColorAttrib("TXTFGCOLOR", 0, 0, 0);
    iupGlobalSetDefaultColorAttrib("MENUBGCOLOR", 240, 240, 240);
    iupGlobalSetDefaultColorAttrib("MENUFGCOLOR", 0, 0, 0);
    iupGlobalSetDefaultColorAttrib("LINKFGCOLOR", 0, 0, 238);
  }
}

IUP_SDK_API int iupGlobalGetAppearance(void)
{
  return iglobal_appearance;
}

IUP_SDK_API int iupGlobalIsDarkMode(void)
{
  unsigned char br, bg, bb, fr, fg, fb;

  if (!iupStrToRGB(IupGetGlobal("DLGBGCOLOR"), &br, &bg, &bb) ||
      !iupStrToRGB(IupGetGlobal("DLGFGCOLOR"), &fr, &fg, &fb))
    return 0;

  return (0.2126 * br + 0.7152 * bg + 0.0722 * bb) < (0.2126 * fr + 0.7152 * fg + 0.0722 * fb)? 1: 0;
}

static int iGlobalChangingDefaultColor(const char *name)
{
  if (iupClassIsGlobalDefault(name, 1))
  {
    char str[50];
    snprintf(str, sizeof(str), "_IUP_USER_DEFAULT_%s", name);
    iupTableSet(iglobal_table, str, (void*)"1", IUPTABLE_POINTER);  /* mark as changed by the User */
    return 1;
  }
  return 0;
}

int iupGlobalDefaultColorChanged(const char *name)
{
  char str[50];
  snprintf(str, sizeof(str), "_IUP_USER_DEFAULT_%s", name);
  return iupTableGet(iglobal_table, str) != NULL;
}

void iupGlobalSetDefaultColorAttrib(const char* name, int r, int g, int b)
{
  if (!iupGlobalDefaultColorChanged(name))
  {
    char value[50];
    snprintf(value, sizeof(value), "%d %d %d", r, g, b);
    iupTableSet(iglobal_table, name, (void*)value, IUPTABLE_STRING);
  }
}

static void iGlobalTableSet(const char *name, const char *value, int store)
{
  if (!value)
    iupTableRemove(iglobal_table, name);
  else if (store)
    iupTableSet(iglobal_table, name, (void*)value, IUPTABLE_STRING);
  else
    iupTableSet(iglobal_table, name, (void*)value, IUPTABLE_POINTER);
}

static void iGlobalSet(const char *name, const char *value, int store)
{
  iupASSERT(name!=NULL);
  if (!name) return;

  if (iupStrEqual(name, "DEFAULTFONTSIZE"))
  {
    iupSetDefaultFontSizeGlobalAttrib(value);
    return;
  }
  if (iupStrEqual(name, "DEFAULTFONTSTYLE"))
  {
    iupSetDefaultFontStyleGlobalAttrib(value);
    return;
  }
  if (iupStrEqual(name, "DEFAULTFONTFACE"))
  {
    iupSetDefaultFontFaceGlobalAttrib(value);
    return;
  }
  if (iupStrEqual(name, "KEYPRESS"))
  {
    int key;
    if (iupStrToInt(value, &key))
      iupdrvSendKey(key, 0x01);
    return;
  }
  if (iupStrEqual(name, "KEYRELEASE"))
  {
    int key;
    if (iupStrToInt(value, &key))
      iupdrvSendKey(key, 0x02);
    return;
  }
  if (iupStrEqual(name, "KEY"))
  {
    int key;
    if (iupStrToInt(value, &key))
      iupdrvSendKey(key, 0x03);
    return;
  }
  if (iupStrEqual(name, "LANGUAGE"))
  {
    char* old_language = (char*)iupTableGet(iglobal_table, "LANGUAGE");
    if (!iupStrEqualNoCase(old_language, value))  /* if different from the current */
    {
      iGlobalTableSet(name, value, store);
      iupStrMessageUpdateLanguage(value);
    }
    return;
  }
  if (iupStrEqual(name, "CURSORPOS"))
  {
    int x, y;
    if (iupStrToIntInt(value, &x, &y, 'x') == 2)
      iupdrvWarpPointer(x, y);
    return;
  }
  if (iupStrEqual(name, "MOUSEBUTTON"))
  {
    int x, y, status;
    char bt;
    if (value && sscanf(value, "%dx%d %c %d", &x, &y, &bt, &status) == 4)
      iupdrvSendMouse(x, y, bt, status);
    return;
  }
  if (iupStrEqual(name, "APPEARANCE"))
  {
    int appearance;
    if (iupStrEqualNoCase(value, "DARK"))
      appearance = IUP_APPEARANCE_DARK;
    else if (iupStrEqualNoCase(value, "LIGHT"))
      appearance = IUP_APPEARANCE_LIGHT;
    else if (!value || iupStrEqualNoCase(value, "SYSTEM"))
      appearance = IUP_APPEARANCE_SYSTEM;
    else
      return;

    if (appearance != iglobal_appearance)
    {
      iglobal_appearance = appearance;

      if (iglobal_table)  /* before IupOpen it is applied by iupdrvOpen */
      {
        iupdrvSetAppearance(appearance);
        iupGlobalUpdateThemeColors();
      }
    }
    return;
  }
  if (iupStrEqual(name, "APPID"))
  {
    if (iupdrvSetGlobalAppIDAttrib(value))
      iGlobalTableSet(name, value, store);
    return;
  }
  if (iupStrEqual(name, "APPNAME"))
  {
    if (iupdrvSetGlobalAppNameAttrib(value))
      iGlobalTableSet(name, value, store);
    return;
  }

  if (iGlobalChangingDefaultColor(name) ||
      iupdrvSetGlobal(name, value))
    iGlobalTableSet(name, value, store);
}

IUP_API void IupSetGlobal(const char *name, const char *value)
{
  iGlobalSet(name, value, 0);
}

IUP_API void IupStoreGlobal(const char *name, const char *value)
{
  iGlobalSet(name, value, 1);
}

IUP_API void IupSetStrGlobal(const char *name, const char *value)
{
  iGlobalSet(name, value, 1);
}

IUP_API char* IupGetGlobal(const char *name)
{
  char* value;

  iupASSERT(name!=NULL);
  if (!name)
    return NULL;

  if (iupStrEqual(name, "DEFAULTFONTSIZE"))
    return iupGetDefaultFontSizeGlobalAttrib();
  if (iupStrEqual(name, "DEFAULTFONTSTYLE"))
    return iupGetDefaultFontStyleGlobalAttrib();
  if (iupStrEqual(name, "DEFAULTFONTFACE"))
    return iupGetDefaultFontFaceGlobalAttrib();
  if (iupStrEqual(name, "CURSORPOS"))
  {
    int x, y;
    iupdrvGetCursorPos(&x, &y);
    return iupStrReturnIntInt(x, y, 'x');
  }
  if (iupStrEqual(name, "SHIFTKEY"))
  {
    char key[5];
    iupdrvGetKeyState(key);
    return iupStrReturnChecked(key[0] == 'S');
  }
  if (iupStrEqual(name, "CONTROLKEY"))
  {
    char key[5];
    iupdrvGetKeyState(key);
    return iupStrReturnChecked(key[1] == 'C');
  }
  if (iupStrEqual(name, "MODKEYSTATE"))
  {
    char *str = iupStrGetMemory(5);
    iupdrvGetKeyState(str);
    return str;
  }
  if (iupStrEqual(name, "FONTLIST"))
  {
    char** families;
    int i, count = iupdrvFontGetFamilyList(&families);
    char* str = NULL;

    if (count > 0 && families)
    {
      int size = 0;
      for (i = 0; i < count; i++)
        size += (int)strlen(families[i]) + 1;

      str = iupStrGetMemory(size + 1);
      str[0] = 0;

      for (i = 0; i < count; i++)
      {
        strcat(str, families[i]);
        strcat(str, "\n");
        free(families[i]);
      }
    }

    if (families)
      free(families);

    return str;
  }
  if (iupStrEqual(name, "SCREENSIZE"))
  {
    int w, h;
    iupdrvGetScreenSize(&w, &h);
    return iupStrReturnIntInt(w, h, 'x');
  }
  if (iupStrEqual(name, "FULLSIZE"))
  {
    int w, h;
    iupdrvGetFullSize(&w, &h);
    return iupStrReturnIntInt(w, h, 'x');
  }
  if (iupStrEqual(name, "SCREENDEPTH"))
  {
    int bpp = iupdrvGetScreenDepth();
    return iupStrReturnInt(bpp);
  }
  if (iupStrEqual(name, "SCREENDPI"))
  {
    double dpi = iupdrvGetScreenDpi();
    return iupStrReturnDouble(dpi);
  }
  if (iupStrEqual(name, "DARKMODE"))
    return iupStrReturnBoolean(iupGlobalIsDarkMode());
  if (iupStrEqual(name, "APPEARANCE"))
  {
    if (iglobal_appearance == IUP_APPEARANCE_DARK)
      return "DARK";
    if (iglobal_appearance == IUP_APPEARANCE_LIGHT)
      return "LIGHT";
    return "SYSTEM";
  }
  if (iupStrEqual(name, "SYSTEMLOCALE"))
    return iupdrvLocaleInfo();
  if (iupStrEqual(name, "SCROLLBARSIZE"))
    return iupStrReturnInt(iupdrvGetScrollbarSize());
  if (iupStrEqual(name, "TOUCHREADY"))
  {
    /* driver answers if it detects touch; otherwise it is definitively No */
    char* touch = iupdrvGetGlobal(name);
    return touch ? touch : iupStrReturnBoolean(0);
  }

  {
    int kind = -1;
    if (iupStrEqual(name, "CACHEDIR"))       kind = IUP_USER_DIR_CACHE;
    else if (iupStrEqual(name, "DATADIR"))   kind = IUP_USER_DIR_DATA;
    else if (iupStrEqual(name, "CONFIGDIR")) kind = IUP_USER_DIR_CONFIG;
    else if (iupStrEqual(name, "TMPDIR"))    kind = IUP_USER_DIR_TEMP;
    if (kind != -1)
    {
      char buffer[10240];
      if (iupdrvGetUserDir(buffer, (int)sizeof(buffer), kind))
        return iupStrReturnStr(buffer);
      return NULL;
    }
  }

  value = iupdrvGetGlobal(name);

  if (!value)
    value = (char*)iupTableGet(iglobal_table, name);

  return value;
}

IUP_SDK_API int iupGlobalIsPointer(const char* name)
{
  const iGlobalRegEntry* e = iupGlobalRegFind(name);
  return (e && (e->flags & IUPGF_POINTER)) ? 1 : 0;
}

int iupGetGlobalAttributes(char** names, int n)
{
  int count = iupTableCount(iglobal_table);
  char * name;
  int i = 0;

  if (n == 0 || n == -1)
    return count;

  name = iupTableFirst(iglobal_table);
  while (name)
  {
    if (!iupATTRIB_ISINTERNAL(name))
    {
      names[i] = name;
      i++;
      if (i == n)
        break;
    }

    name = iupTableNext(iglobal_table);
  }

  return i;
}
