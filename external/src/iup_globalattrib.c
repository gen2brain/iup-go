/** \file
 * \brief global attributes environment
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>      
#include <stdio.h>      
#include <string.h>      

#include "iup.h" 

#include "iup_table.h"
#include "iup_globalattrib.h"
#include "iup_table.h"
#include "iup_class.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_drvinfo.h"
#include "iup_assert.h"
#include "iup_str.h"
#include "iup_strmessage.h"
#include "iup_attrib.h"


static Itable *iglobal_table = NULL;

void iupGlobalAttribInit(void)
{
  iglobal_table = iupTableCreate(IUPTABLE_STRINGINDEXED);
}

void iupGlobalAttribFinish(void)
{
  iupTableDestroy(iglobal_table);
  iglobal_table = NULL;
}

static int iGlobalChangingDefaultColor(const char *name)
{
  if (iupClassIsGlobalDefault(name, 1))
  {
    char str[50] = "_IUP_USER_DEFAULT_";
    strcat(str, name);
    iupTableSet(iglobal_table, str, (void*)"1", IUPTABLE_POINTER);  /* mark as changed by the User */
    return 1;
  }
  return 0;
}

int iupGlobalDefaultColorChanged(const char *name)
{
  char str[50] = "_IUP_USER_DEFAULT_";
  strcat(str, name);
  return iupTableGet(iglobal_table, str) != NULL;
}

void iupGlobalSetDefaultColorAttrib(const char* name, int r, int g, int b)
{
  if (!iupGlobalDefaultColorChanged(name))
  {
    char value[50];
    sprintf(value, "%d %d %d", r, g, b);
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
    if (!iupStrEqualNoCase(old_language, value))  /* if different than the current */
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
  if (iupStrEqual(name, "SYSTEMLOCALE"))
    return iupdrvLocaleInfo();
  if (iupStrEqual(name, "SCROLLBARSIZE"))
    return iupStrReturnInt(iupdrvGetScrollbarSize());

  value = iupdrvGetGlobal(name);

  if (!value)
    value = (char*)iupTableGet(iglobal_table, name);

  return value;
}

IUP_SDK_API int iupGlobalIsPointer(const char* name)
{
  static struct {
    const char *name;
  } ptr_table[] = {
#ifndef GTK_MAC
  #ifdef WIN32
    {"HINSTANCE"},
    {"DLL_HINSTANCE"},
  #else
    {"XDISPLAY"},
    {"XSCREEN"},
    {"APPSHELL"},
  #endif
#endif
  };
#define PTR_TABLE_SIZE ((sizeof ptr_table)/(sizeof ptr_table[0]))

  if (name)
  {
    int i;
    for (i = 0; i < PTR_TABLE_SIZE; i++)
    {
      if (iupStrEqualNoCase(name, ptr_table[i].name))
        return 1;
    }
  }

  return 0;
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
