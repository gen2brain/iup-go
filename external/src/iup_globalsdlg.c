/** \file
* \brief IupGloabsDialog pre-defined dialog
*
* See Copyright Notice in "iup.h"
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_globalattrib.h"
#include "iup_func.h"
#include "iup_str.h"
#include "iup_attrib.h"
#include "iup_names.h"


typedef struct _iRegisteredGlobal {
  char *name;
  int windows;
  int motif;
  int gtk;
  int readonly;
} iRegisteredGlobal;

/* sorted alphabetically */
static iRegisteredGlobal global_attribs[] = { 
  { "APPSHELL", 0, 0, 1, 1 }, 
  { "ARGV0", 0, 0, 1, 0 },
  { "AUTOREPEAT", 0, 1, 0, 0 }, 
  { "COMCTL32VER6", 1, 0, 0, 1 }, 
  { "COMPUTERNAME", 1, 1, 1, 1 }, 
  { "CONTROLKEY", 1, 1, 1, 1 }, 
  { "COPYRIGHT", 1, 1, 1, 1 }, 
  { "CURSORPOS", 1, 1, 1, 0 }, 
  { "DEFAULTBUTTONPADDING", 1, 1, 1, 0 },
  { "DEFAULTDECIMALSYMBOL", 1, 1, 1, 0 }, 
  { "DEFAULTFONT", 1, 1, 1, 0 }, 
  { "DEFAULTFONTFACE", 1, 1, 1, 0 }, 
  { "DEFAULTFONTSIZE", 1, 1, 1, 0 }, 
  { "DEFAULTFONTSTYLE", 1, 1, 1, 0 }, 
  { "DEFAULTPRECISION", 1, 1, 1, 0 },
  { "DEFAULTTHEME", 1, 1, 1, 0 }, 
  { "DLGBGCOLOR", 1, 1, 1, 0 }, 
  { "DLGFGCOLOR", 1, 1, 1, 0 },
  { "DLL_HINSTANCE", 1, 0, 0, 1 }, 
  { "DRAWUSEDIRECT2D", 1, 0, 0, 0 },
  { "DRIVER", 1, 1, 1, 1 }, 
  { "DWM_COMPOSITION", 1, 0, 0, 1 }, 
  { "EXEFILENAME", 1, 1, 1, 1 },
  { "EXITLOOP", 1, 1, 1, 0 }, 
  { "FREETYPEFONTS_DIR", 1, 1, 1, 0 },
  { "FULLSIZE", 1, 1, 1, 1 }, 
  { "GLOBALLAYOUTDLGKEY", 1, 1, 1, 0 }, 
  { "GLOBALLAYOUTRESIZEKEY", 1, 1, 1, 0 }, 
  { "GLOBALMENU", 0, 0, 1, 0 },
  { "GL_RENDERER", 1, 1, 1, 1 }, 
  { "GL_VENDOR", 1, 1, 1, 1 }, 
  { "GL_VERSION", 1, 1, 1, 1 }, 
  { "GTKDEVVERSION", 0, 0, 1, 1 },
  { "GTKVERSION", 0, 0, 1, 1 }, 
  { "HELPAPP", 0, 1, 1, 0 },
  { "HINSTANCE", 1, 0, 0, 1 },
  { "ICON", 1, 1, 1, 0 },
  { "IMAGEAUTOSCALE", 1, 1, 1, 0 }, 
  { "IMAGEEXPORT_STATIC", 1, 1, 1, 0 },
  { "IMAGESDPI", 1, 1, 1, 0 }, 
  { "IMAGESTOCKAUTOSCALE", 1, 1, 1, 0 },
  { "IMAGESTOCKSIZE", 1, 1, 1, 0 }, 
  { "INPUTCALLBACKS", 1, 1, 1, 0 }, 
  { "IUPIM_LASTERROR", 1, 1, 1, 1 },
  { "IUPLUA_THREADED", 1, 1, 1, 0 }, 
  { "KEY", 1, 1, 1, 1 }, 
  { "KEYPRESS", 1, 1, 1, 1 }, 
  { "KEYRELEASE", 1, 1, 1, 1 }, 
  { "LANGUAGE", 1, 1, 1, 0 },
  { "LASTERROR", 1, 0, 0, 0 }, 
  { "LEXERLUA", 1, 1, 1, 0 },
  { "LINKFGCOLOR", 1, 1, 1, 0 },
  { "LOCKLOOP", 1, 1, 1, 0 },
  { "LUA_ERROR_LABEL", 1, 1, 1, 0 },
  { "MENUBGCOLOR", 1, 0, 0, 0 }, 
  { "MENUFGCOLOR", 1, 0, 0, 1 }, 
  { "MGLFONTS", 1, 1, 1, 0 },
  { "MODKEYSTATE", 1, 1, 1, 1 }, 
  { "MONITORSCOUNT", 1, 0, 1, 1 }, 
  { "MONITORSINFO", 1, 0, 1, 1 }, 
  { "MOTIFNUMBER", 0, 1, 0, 1 }, 
  { "MOTIFVERSION", 0, 1, 0, 1 }, 
  { "MOUSEBUTTON", 1, 1, 1, 1 },
  { "OVERLAYSCROLLBAR", 0, 0, 1, 0 }, 
  { "PARENTDIALOG", 1, 1, 1, 0 },
  { "SB_BGCOLOR", 0, 1, 1, 0 }, 
  { "SCINTILLA_VERSION", 1, 1, 1, 1 },
  { "SCREENDEPTH", 1, 1, 1, 1 }, 
  { "SCREENDPI", 1, 1, 1, 1 }, 
  { "SCREENSIZE", 1, 1, 1, 1 }, 
  { "SCROLLBARSIZE", 1, 1, 1, 1 }, 
  { "SHIFTKEY", 1, 1, 1, 1 }, 
  { "SHOWMENUIMAGES", 0, 0, 1, 0 }, 
  { "SINGLEINSTANCE", 1, 0, 0, 0 }, 
  { "SYSTEM", 1, 1, 1, 1 }, 
  { "SYSTEMLANGUAGE", 1, 1, 1, 1 },
  { "SYSTEMLOCALE", 1, 1, 1, 1 }, 
  { "SYSTEMVERSION", 1, 1, 1, 1 }, 
  { "TREEIMAGE24", 1, 1, 1, 0 },
  { "TRUECOLORCANVAS", 1, 1, 1, 1 },
  { "TXTBGCOLOR", 1, 1, 1, 0 }, 
  { "TXTFGCOLOR", 1, 1, 1, 0 }, 
  { "TXTHLCOLOR", 1, 1, 1, 0 }, 
  { "USERNAME", 1, 1, 1, 1 }, 
  { "UTF8MODE", 1, 0, 1, 0 }, 
  { "UTF8MODE_FILE", 1, 0, 0, 0 }, 
  { "VERSION", 1, 1, 1, 1 }, 
  { "VIRTUALSCREEN", 1, 0, 1, 1 }, 
  { "XDISPLAY", 0, 1, 1, 1 }, 
  { "XSCREEN", 0, 1, 1, 1 }, 
  { "XSERVERVENDOR", 0, 1, 1, 1 }, 
  { "XVENDORRELEASE", 0, 1, 1, 1 },
  { NULL, 0, 0, 0, 0 }
};

static int iGlobalGetRegisteredAttributesCount(void)
{
  int count = 0;
  while (global_attribs[count].name != NULL)
    count++;
  return count;
}

static int iGlobalIsRegistered(char *name)
{
  int i;

  for (i = 0; global_attribs[i].name != NULL; i++)
  {
    if (iupStrEqual(global_attribs[i].name, name))
      return 1;
  }

  return 0;
}

static int iGlobalsCompareNames(const void* i1, const void* i2)
{
  char* name1 = *((char**)i1);
  char* name2 = *((char**)i2);
  return strcmp(name1, name2);
}

static int iGlobalGetAppAttributes(char **names)
{
  int total_count = iupGetGlobalAttributes(NULL, 0);
  char ** gnames = NULL;
  int gcount, count, i;

  gnames = (char **)malloc(sizeof(char*)* total_count);

  gcount = iupGetGlobalAttributes(gnames, total_count);

  count = 0;
  for (i = 0; i < gcount; i++)
  {
    if (iGlobalIsRegistered(gnames[i]))
      continue;
    if (names != NULL)
      names[count] = gnames[i];
    count++;
  }

  free(gnames);

  return count;
}

static void iGlobalsUpdate(Ihandle* dlg)
{
  int i, j, attr_count, total_count;
  Ihandle* list1 = (Ihandle*)iupAttribGet(dlg, "_IUP_PROPLIST1");
  Ihandle* txt1 = (Ihandle*)iupAttribGetInherit(list1, "_IUP_ATTVALUE");
  Ihandle* setbut1 = (Ihandle*)iupAttribGetInherit(list1, "_IUP_SETBUTTON");
  Ihandle* colorbut1 = (Ihandle*)iupAttribGetInherit(list1, "_IUP_COLORBUTTON");
  Ihandle* fontbut1 = (Ihandle*)iupAttribGetInherit(list1, "_IUP_FONTBUTTON");
  Ihandle* list2 = (Ihandle*)iupAttribGetInherit(dlg, "_IUP_PROPLIST2");
  Ihandle* txt2 = (Ihandle*)iupAttribGetInherit(list2, "_IUP_ATTVALUE");
  Ihandle* setbut2 = (Ihandle*)iupAttribGetInherit(list2, "_IUP_SETBUTTON");
  Ihandle* colorbut2 = (Ihandle*)iupAttribGetInherit(list2, "_IUP_COLORBUTTON");
  Ihandle* fontbut2 = (Ihandle*)iupAttribGetInherit(list2, "_IUP_FONTBUTTON");
  Ihandle* list3 = (Ihandle*)iupAttribGet(dlg, "_IUP_FUNCLIST");
  Ihandle* txt3 = (Ihandle*)iupAttribGetInherit(list3, "_IUP_FUNCVALUE");
  Ihandle* list4 = (Ihandle*)iupAttribGet(dlg, "_IUP_NAMESLIST");
  Ihandle* txt4 = (Ihandle*)iupAttribGetInherit(list4, "_IUP_NAMESVALUE");
  Ihandle* show_auto_tg = IupGetDialogChild(dlg, "SHOWAUTO");
  int show_auto = IupGetInt(show_auto_tg, "VALUE");
  char **attr_names;

  /* Clear everything */
  IupSetAttribute(list1, "REMOVEITEM", NULL);
  IupSetAttribute(txt1, "VALUE", "");
  IupSetAttribute(setbut1, "VALUE", "");
  IupSetAttribute(colorbut1, "ACTIVE", "No");
  IupSetAttribute(fontbut1, "VISIBLE", "No");
  IupSetAttribute(list2, "REMOVEITEM", NULL);
  IupSetAttribute(txt2, "VALUE", "");
  IupSetAttribute(setbut2, "VALUE", "");
  IupSetAttribute(colorbut2, "ACTIVE", "No");
  IupSetAttribute(fontbut2, "VISIBLE", "No");
  IupSetAttribute(list3, "REMOVEITEM", NULL);
  IupSetAttribute(txt3, "VALUE", "");
  IupSetAttribute(list4, "REMOVEITEM", NULL);
  IupSetAttribute(txt4, "VALUE", "");

  total_count = iGlobalGetAppAttributes(NULL);
  if (total_count > 0)
  {
    attr_names = (char **)malloc(total_count * sizeof(char *));
    attr_count = iGlobalGetAppAttributes(attr_names);
    qsort(attr_names, total_count, sizeof(char*), iGlobalsCompareNames);
    for (i = 0; i < attr_count; i++)
    {
      IupSetAttributeId(list2, "", i + 1, attr_names[i]);
      IupSetIntId(list2, "_IUP_READONLY", i + 1, 0);
    }

    free(attr_names);
  }

  total_count = iGlobalGetRegisteredAttributesCount();
  
  j = 0;
  for (i = 0; i < total_count; i++)
  {
    if (!(iupStrEqualNoCase(IupGetGlobal("DRIVER"), "GTK") && global_attribs[i].gtk) &&
        !(iupStrEqualNoCase(IupGetGlobal("DRIVER"), "Motif") && global_attribs[i].motif) &&
        !(iupStrEqualNoCase(IupGetGlobal("DRIVER"), "Win32") && global_attribs[i].windows))
        continue;

    IupSetAttributeId(list1, "", j + 1, global_attribs[i].name);
    IupSetIntId(list1, "_IUP_READONLY", j + 1, global_attribs[i].readonly);
    j++;
  }

  total_count = iupGetFunctions(NULL, 0);
  attr_names = (char **)malloc(total_count * sizeof(char *));
  attr_count = iupGetFunctions(attr_names, total_count);
  for (i = 0; i < attr_count; i++)
    IupSetAttributeId(list3, "", i + 1, attr_names[i]);
  free(attr_names);

  total_count = IupGetAllNames(NULL, 0);
  attr_names = (char **)malloc(total_count * sizeof(char *));
  attr_count = IupGetAllNames(attr_names, total_count);
  j = 1;
  for (i = 0; i < attr_count; i++)
  {
    if (show_auto || !iupStrEqualPartial(attr_names[i], "_IUP_NAME"))
    {
      IupSetAttributeId(list4, "", j, attr_names[i]);
      j++;
    }
  }

  free(attr_names);
}

static int iGlobalsList_CB(Ihandle *list1, char *name, int item, int state)
{
  (void)item;
  if (state)
  {
    char* value = IupGetGlobal(name);
    Ihandle* txt1 = (Ihandle *)iupAttribGetInherit(list1, "_IUP_ATTVALUE");
    Ihandle* setbut = (Ihandle *)iupAttribGetInherit(list1, "_IUP_SETBUTTON");
    Ihandle* colorbut = (Ihandle *)iupAttribGetInherit(list1, "_IUP_COLORBUTTON");
    Ihandle* fontbut = (Ihandle *)iupAttribGetInherit(list1, "_IUP_FONTBUTTON");

    if (value)
    {
      if (iupGlobalIsPointer(name))
        IupSetStrf(txt1, "VALUE", "%p", value);
      else
        IupStoreAttribute(txt1, "VALUE", value);
    }
    else
      IupSetAttribute(txt1, "VALUE", "NULL");

    IupSetInt(setbut, "ACTIVE", !IupGetIntId(list1, "_IUP_READONLY", item));
    IupSetInt(txt1, "READONLY", IupGetIntId(list1, "_IUP_READONLY", item));

    if (!IupGetIntId(list1, "_IUP_READONLY", item) && strstr(name, "COLOR") != NULL) /* if COLOR in attribute name, show the color selection button */
    {
      IupStoreAttribute(colorbut, "BGCOLOR", value);  /* set it even if it is NULL */
      IupSetAttribute(colorbut, "VISIBLE", "Yes");
    }
    else
      IupSetAttribute(colorbut, "VISIBLE", "No");

    if (!IupGetIntId(list1, "_IUP_READONLY", item) && strstr(name, "FONT") != NULL) /* if FONT in attribute name, show the color selection button */
      IupSetAttribute(fontbut, "VISIBLE", "Yes");
    else
      IupSetAttribute(fontbut, "VISIBLE", "No");
  }

  return IUP_DEFAULT;
}

static int iGlobalsSet_CB(Ihandle* button)
{
  Ihandle* list1 = (Ihandle*)iupAttribGetInherit(button, "_IUP_PROPLIST");
  char* item = IupGetAttribute(list1, "VALUE");
  if (item)
  {
    Ihandle* txt1 = (Ihandle *)iupAttribGetInherit(list1, "_IUP_ATTVALUE");
    char* value = IupGetAttribute(txt1, "VALUE");
    char* name = IupGetAttribute(list1, item);

    if (!value || iupStrEqual(value, "NULL"))
      IupSetGlobal(name, NULL);
    else
      IupSetStrGlobal(name, value);

    if (strstr(name, "COLOR") != NULL)
    {
      Ihandle* colorbut = (Ihandle *)iupAttribGetInherit(list1, "_IUP_COLORBUTTON");
      IupStoreAttribute(colorbut, "BGCOLOR", value);  /* set it even if it is NULL */
      IupSetStrGlobal("BGCOLOR", value);
    }
  }
  return IUP_DEFAULT;
}

static int iGlobalsSetColor_CB(Ihandle *colorbut)
{
  Ihandle* color_dlg = IupColorDlg();
  IupSetAttributeHandle(color_dlg, "PARENTDIALOG", IupGetDialog(colorbut));
  IupSetAttribute(color_dlg, "TITLE", "Choose Color");
  IupStoreAttribute(color_dlg, "VALUE", IupGetAttribute(colorbut, "BGCOLOR"));

  IupPopup(color_dlg, IUP_CENTER, IUP_CENTER);

  if (IupGetInt(color_dlg, "STATUS") == 1)
  {
    Ihandle* list1 = (Ihandle*)iupAttribGetInherit(colorbut, "_IUP_PROPLIST");
    Ihandle* txt1 = (Ihandle *)iupAttribGetInherit(list1, "_IUP_ATTVALUE");
    char* value = IupGetAttribute(color_dlg, "VALUE");
    char* name = IupGetAttribute(list1, IupGetAttribute(list1, "VALUE"));

    IupStoreAttribute(txt1, "VALUE", value);

    if (strstr(name, "COLOR") != NULL)
    {
      IupStoreAttribute(colorbut, "BGCOLOR", value);
      IupSetStrGlobal(name, value);
    }

    IupUpdate(IupGetDialog(colorbut));

    IupDestroy(color_dlg);
  }

  return IUP_DEFAULT;
}

static int iGlobalsSetFont_CB(Ihandle *fontbut)
{
  Ihandle* font_dlg = IupFontDlg();
  Ihandle* list1 = (Ihandle*)iupAttribGetInherit(fontbut, "_IUP_PROPLIST");
  Ihandle* txt1 = (Ihandle *)iupAttribGetInherit(list1, "_IUP_ATTVALUE");
  IupSetAttributeHandle(font_dlg, "PARENTDIALOG", IupGetDialog(fontbut));
  IupSetAttribute(font_dlg, "TITLE", "Choose Font");
  IupStoreAttribute(font_dlg, "VALUE", IupGetAttribute(txt1, "VALUE"));

  IupPopup(font_dlg, IUP_CENTER, IUP_CENTER);

  if (IupGetInt(font_dlg, "STATUS") == 1)
  {
    char* value = IupGetAttribute(font_dlg, "VALUE");
    char* name = IupGetAttribute(list1, IupGetAttribute(list1, "VALUE"));

    IupStoreAttribute(txt1, "VALUE", value);

    IupSetStrGlobal(name, value);
  }

  IupDestroy(font_dlg);

  return IUP_DEFAULT;
}

static int iGlobalsSetNewAttrib_CB(Ihandle* button)
{
  char* name = IupGetAttribute(IupGetDialogChild(button, "NEW_ATTRIB_NAME"), "VALUE");
  char* value = IupGetAttribute(IupGetDialogChild(button, "NEW_ATTRIB_VALUE"), "VALUE");
  if (!value || iupStrEqual(value, "NULL"))
    IupSetGlobal(name, NULL);
  else
    IupSetStrGlobal(name, value);

  iGlobalsUpdate(IupGetDialog(button));

  return IUP_DEFAULT;
}

static int iGlobalsFunctionsList_CB(Ihandle *list, char *name, int item, int state)
{
  (void)item;
  if (state)
  {
    Icallback cb = IupGetFunction(name);
    Ihandle* txt = (Ihandle *)iupAttribGetInherit(list, "_IUP_FUNCVALUE");

    if (cb)
      IupSetfAttribute(txt, "VALUE", "%p", cb);
    else
      IupSetAttribute(txt, "VALUE", "NULL");
  }

  return IUP_DEFAULT;
}

static int iGlobalsFunctionReset_CB(Ihandle* bt)
{
  Ihandle* list = (Ihandle*)iupAttribGetInherit(bt, "_IUP_FUNCLIST");
  char* name = IupGetAttribute(list, "VALUESTRING");
  if (name)
  {
    Ihandle* txt = (Ihandle *)iupAttribGetInherit(bt, "_IUP_FUNCVALUE");

    IupSetFunction(name, NULL);
    IupSetAttribute(txt, "VALUE", "NULL");
    IupSetInt(list, "REMOVEITEM", IupGetInt(list, "VALUE"));
  }
  return IUP_DEFAULT;
}

static int iGlobalsNamesList_CB(Ihandle *list, char *name, int item, int state)
{
  (void)item;
  if (state)
  {
    Ihandle* txt = (Ihandle *)iupAttribGetInherit(list, "_IUP_NAMESVALUE");
    Ihandle* elem = IupGetHandle(name);

    if (elem)
      IupSetfAttribute(txt, "VALUE", "%p (%s)", elem, elem->iclass->name);
    else
      IupSetAttribute(txt, "VALUE", "NULL");
  }

  return IUP_DEFAULT;
}

static int iGlobalsNameReset_CB(Ihandle* bt)
{
  Ihandle* list = (Ihandle*)iupAttribGetInherit(bt, "_IUP_NAMESLIST");
  char* name = IupGetAttribute(list, "VALUESTRING");
  if (name)
  {
    Ihandle* txt = (Ihandle *)iupAttribGetInherit(bt, "_IUP_NAMESVALUE");

    IupSetHandle(name, NULL);
    IupSetAttribute(txt, "VALUE", "NULL");
    IupSetInt(list, "REMOVEITEM", IupGetInt(list, "VALUE"));
  }
  return IUP_DEFAULT;
}

static int iGlobalsNameProperties_CB(Ihandle* bt)
{
  Ihandle* list = (Ihandle*)iupAttribGetInherit(bt, "_IUP_NAMESLIST");
  char* name = IupGetAttribute(list, "VALUESTRING");
  if (name)
  {
    Ihandle* elem = IupGetHandle(name);
    Ihandle* dlg = IupElementPropertiesDialog(IupGetDialog(list), elem);
    IupPopup(dlg, IUP_CENTERPARENT, IUP_CENTERPARENT);
    IupDestroy(dlg);
  }
  return IUP_DEFAULT;
}

static int iGlobalsNameFind_CB(Ihandle* bt)
{
  Ihandle* list = (Ihandle*)iupAttribGetInherit(bt, "_IUP_NAMESLIST");
  char* name = IupGetAttribute(list, "VALUESTRING");
  if (name)
  {
    Ihandle* elem = IupGetHandle(name);
    int count = iupNamesFindAll(elem, NULL, 0);
    if (count > 1)
    {
      int i, total_len = 0, max_len = count * 50;
      char* str = malloc(max_len);
      char** names = malloc(count * sizeof(char*));
      iupNamesFindAll(elem, names, count);
      str[0] = 0;
      for (i = 0; i < count; i++)
      {
        total_len += (int)strlen(names[i]);
        if (total_len > max_len)
        {
          max_len += 10 * (total_len - max_len);
          str = realloc(str, max_len);
        }
        strcat(str, names[i]);
        strcat(str, "\n");
      }
      IupGetText("Other Names", str, -1);  /* read-only */
      free(names);
      free(str);
    }
    else
      IupMessage("Other Names", "No other names.");
  }
  return IUP_DEFAULT;
}

static int iGlobalsNamesShowAuto_CB(Ihandle* tg, int state)
{
  (void)state;
  iGlobalsUpdate(IupGetDialog(tg));
  return IUP_DEFAULT;
}

static int iGlobalsNameCheckHandles_CB(Ihandle* bt)
{
  Ihandle* list = (Ihandle*)iupAttribGetInherit(bt, "_IUP_NAMESLIST");
  int i, count = IupGetInt(list, "COUNT");
  int log_size = 0, log_max_size = count * 50;
  char* log = malloc(log_max_size);

  for (i = 0; i < count; i++)
  {
    char* name = IupGetAttributeId(list, "", i+1);
    Ihandle* elem = IupGetHandle(name);
    if (!iupObjectCheck(elem))
    {
      int name_len = (int)strlen(name);
      if (log_size + name_len + 1 > log_max_size)
      {
        log_max_size += 10 * name_len;
        log = realloc(log, log_max_size);
      }

      memcpy(log + log_size, name, name_len);
      memcpy(log + log_size + name_len, "\n", 1);
      log_size += name_len + 1;
    }
  }

  if (log_size != 0)
  {
    log[log_size] = 0;
    IupGetText("Invalid Handles", log, -1);  /* read-only */
  }
  else
    IupMessage("Invalid Handles", "All handles are valid!");

  free(log);
  return IUP_DEFAULT;
}

static int iGlobalsTabChangePos_CB(Ihandle* ih, int new_pos, int old_pos)
{
  (void)old_pos;
  switch (new_pos)
  {
    case 0:
      IupSetAttribute(ih, "TIP", "Application Global attributes.");
      break;
    case 1:
      IupSetAttribute(ih, "TIP", "Registered Global attributes.");
      break;
    case 2:
      IupSetAttribute(ih, "TIP", "Functions.");
      break;
    case 3:
      IupSetAttribute(ih, "TIP", "Names.");
      break;
  }

  /* In GTK the TIP appears for all children */
  /* TODO: move this code to iupgtk_tabs.c */
  if (iupStrEqualNoCase(IupGetGlobal("DRIVER"), "GTK"))
  {
    char* tabtype = IupGetAttribute(ih, "TABTYPE");
    int x = 0;
    int y = 0;
    int w = ih->currentwidth;
    int h = ih->currentheight;
    int cw = 0, ch = 0;

    IupGetIntInt(ih, "CLIENTSIZE", &cw, &ch);

    /* TABORIENTATION is ignored */
    if (iupStrEqualNoCase(tabtype, "BOTTOM"))
    {
      y += ch;  /* position after the client area */
      h -= ch;
    }
    else if (iupStrEqualNoCase(tabtype, "RIGHT"))
    {
      x += cw;  /* position after the client area */
      w -= cw;
    }
    else if (iupStrEqualNoCase(tabtype, "LEFT"))
      w -= cw;
    else  /* TOP */
      h -= ch;

    IupSetfAttribute(ih, "TIPRECT", "%d %d %d %d", x, y, x + w, y + h);
  }

  IupSetAttribute(ih, "TIPVISIBLE", "YES");
  return IUP_DEFAULT;
}

static int iGlobalsClose_CB(Ihandle* ih)
{
  IupHide(IupGetDialog(ih));
  return IUP_DEFAULT;
}

static Ihandle* iGlobalsCreateDialog(void)
{
  Ihandle *list1, *list2, *list3, *list4, *close, *dlg, *dlg_box, *button_box, *colorbut1, *fontbut1, *colorbut2, *fontbut2,
    *tabs, *box1, *box11, *box12, *box13, *box14, *box2, *box3, *box4, *set1, *set2, *value1, *value2, *value3, *value4;

  close = IupButton("Close", NULL);
  IupSetStrAttribute(close, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
  IupSetCallback(close, "ACTION", (Icallback)iGlobalsClose_CB);

  button_box = IupHbox(
    IupFill(),
    close,
    NULL);
  IupSetAttribute(button_box, "MARGIN", "0x0");

  list1 = IupList(NULL);
  IupSetCallback(list1, "ACTION", (Icallback)iGlobalsList_CB);
  IupSetAttribute(list1, "VISIBLELINES", "15");
  IupSetAttribute(list1, "VISIBLECOLUMNS", "16");
  IupSetAttribute(list1, "EXPAND", "VERTICAL");

  set1 = IupButton("Set", NULL);
  IupSetCallback(set1, "ACTION", iGlobalsSet_CB);
  IupSetStrAttribute(set1, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));

  colorbut1 = IupButton(NULL, NULL);
  IupSetAttribute(colorbut1, "SIZE", "20x10");
  IupStoreAttribute(colorbut1, "BGCOLOR", "0 0 0");
  IupSetCallback(colorbut1, "ACTION", (Icallback)iGlobalsSetColor_CB);
  IupSetAttribute(colorbut1, "VISIBLE", "NO");

  fontbut1 = IupButton("F", NULL);
  IupSetAttribute(fontbut1, "SIZE", "20x10");
  IupStoreAttribute(fontbut1, "FONT", "Times, Bold Italic 12");
  IupSetCallback(fontbut1, "ACTION", (Icallback)iGlobalsSetFont_CB);
  IupSetAttribute(fontbut1, "VISIBLE", "NO");

  value1 = IupText(NULL);
  IupSetAttribute(value1, "MULTILINE", "Yes");
  IupSetAttribute(value1, "ALIGNMENT", "ALEFT:ATOP");
  IupSetAttribute(value1, "EXPAND", "YES");

  list2 = IupList(NULL);
  IupSetCallback(list2, "ACTION", (Icallback)iGlobalsList_CB);
  IupSetAttribute(list2, "VISIBLELINES", "15");
  IupSetAttribute(list2, "VISIBLECOLUMNS", "16");
  IupSetAttribute(list2, "EXPAND", "VERTICAL");

  set2 = IupButton("Set", NULL);
  IupSetCallback(set2, "ACTION", iGlobalsSet_CB);
  IupSetStrAttribute(set2, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));

  colorbut2 = IupButton(NULL, NULL);
  IupSetAttribute(colorbut2, "SIZE", "20x10");
  IupStoreAttribute(colorbut2, "BGCOLOR", "0 0 0");
  IupSetCallback(colorbut2, "ACTION", (Icallback)iGlobalsSetColor_CB);
  IupSetAttribute(colorbut2, "VISIBLE", "NO");

  fontbut2 = IupButton("F", NULL);
  IupSetAttribute(fontbut2, "SIZE", "20x10");
  IupStoreAttribute(fontbut2, "FONT", "Times, Bold Italic 12");
  IupSetCallback(fontbut2, "ACTION", (Icallback)iGlobalsSetFont_CB);
  IupSetAttribute(fontbut2, "VISIBLE", "NO");

  value2 = IupText(NULL);
  IupSetAttribute(value2, "MULTILINE", "Yes");
  IupSetAttribute(value2, "ALIGNMENT", "ALEFT:ATOP");
  IupSetAttribute(value2, "EXPAND", "YES");

  list3 = IupList(NULL);
  IupSetCallback(list3, "ACTION", (Icallback)iGlobalsFunctionsList_CB);
  IupSetAttribute(list3, "VISIBLELINES", "15");
  IupSetAttribute(list3, "VISIBLECOLUMNS", "16");
  IupSetAttribute(list3, "SORT", "Yes");
  IupSetAttribute(list3, "EXPAND", "VERTICAL");

  value3 = IupText(NULL);
  IupSetAttribute(value3, "EXPAND", "HORIZONTAL");
  IupSetAttribute(value3, "READONLY", "Yes");

  list4 = IupList(NULL);
  IupSetCallback(list4, "ACTION", (Icallback)iGlobalsNamesList_CB);
  IupSetAttribute(list4, "VISIBLELINES", "15");
  IupSetAttribute(list4, "VISIBLECOLUMNS", "16");
  IupSetAttribute(list4, "SORT", "Yes");
  IupSetAttribute(list4, "EXPAND", "VERTICAL");

  value4 = IupText(NULL);
  IupSetAttribute(value4, "EXPAND", "HORIZONTAL");
  IupSetAttribute(value4, "READONLY", "Yes");

  box11 = IupVbox(
    IupLabel("Value:"),
    IupSetAttributes(IupHbox(value1, IupSetAttributes(IupVbox(set1, colorbut1, fontbut1, NULL), "GAP=3"), NULL), "GAP=10"),
    NULL);
  IupSetAttribute(box11, "MARGIN", "0x0");
  IupSetAttribute(box11, "GAP", "0");

  box12 = IupVbox(
    IupLabel("Value:"),
    IupSetAttributes(IupHbox(value2, IupSetAttributes(IupVbox(set2, colorbut2, fontbut2, NULL), "GAP=3"), NULL), "GAP=10"),
    NULL);
  IupSetAttribute(box12, "MARGIN", "0x0");
  IupSetAttribute(box12, "GAP", "0");

  box13 = IupVbox(
    IupLabel("Value:"),
    value3, 
    IupSetCallbacks(IupSetAttributes(IupButton("Reset Value", NULL), "PADDING=DEFAULTBUTTONPADDING"), "ACTION", iGlobalsFunctionReset_CB, NULL),
    NULL);
  IupSetAttribute(box13, "MARGIN", "0x0");
  IupSetAttribute(box13, "GAP", "0");

  box14 = IupVbox(
    IupLabel("Value:"),
    value4, 
    IupSetCallbacks(IupSetAttributes(IupButton("Reset Value", NULL), "PADDING=DEFAULTBUTTONPADDING, NORMALIZERGROUP=IupGlobalNamesNorm"), "ACTION", iGlobalsNameReset_CB, NULL),
    IupSetCallbacks(IupSetAttributes(IupButton("Properties...", NULL), "PADDING=DEFAULTBUTTONPADDING, NORMALIZERGROUP=IupGlobalNamesNorm"), "ACTION", iGlobalsNameProperties_CB, NULL),
    IupSetCallbacks(IupSetAttributes(IupButton("Other Names...", NULL), "PADDING=DEFAULTBUTTONPADDING, NORMALIZERGROUP=IupGlobalNamesNorm"), "ACTION", iGlobalsNameFind_CB, NULL),
    IupSetAttributes(IupFlatSeparator(), "ORIENTATION=HORIZONTAL"),
    IupSetCallbacks(IupSetAttributes(IupToggle("Show Auto Names", NULL), "NAME=SHOWAUTO, TIP=\"Show Automatic Generated Names (_IUP_NAME*)\", NORMALIZERGROUP=IupGlobalNamesNorm"), "ACTION", (Icallback)iGlobalsNamesShowAuto_CB, NULL),
    IupSetCallbacks(IupSetAttributes(IupButton("Check Handles...", NULL), "PADDING=DEFAULTBUTTONPADDING, NORMALIZERGROUP=IupGlobalNamesNorm"), "ACTION", iGlobalsNameCheckHandles_CB, NULL),
    NULL);
  IupSetAttribute(box14, "MARGIN", "0x0");
  IupSetAttribute(box14, "GAP", "3");

  box1 = IupHbox(IupSetAttributes(IupVbox(IupLabel("Name:"), list1, NULL), "MARGIN=0x0, GAP=0"), box11, NULL);
  iupAttribSet(box1, "_IUP_PROPLIST", (char*)list1);
  iupAttribSet(box1, "_IUP_ATTVALUE", (char*)value1);
  iupAttribSet(box1, "_IUP_SETBUTTON", (char*)set1);
  iupAttribSet(box1, "_IUP_COLORBUTTON", (char*)colorbut1);
  iupAttribSet(box1, "_IUP_FONTBUTTON", (char*)fontbut1);

  box2 = IupHbox(IupSetAttributes(IupVbox(IupLabel("Name:"), list2, NULL), "MARGIN=0x0, GAP=0"), box12, NULL);
  iupAttribSet(box2, "_IUP_PROPLIST", (char*)list2);
  iupAttribSet(box2, "_IUP_ATTVALUE", (char*)value2);
  iupAttribSet(box2, "_IUP_SETBUTTON", (char*)set2);
  iupAttribSet(box2, "_IUP_COLORBUTTON", (char*)colorbut2);
  iupAttribSet(box2, "_IUP_FONTBUTTON", (char*)fontbut2);

  box2 = IupSetAttributes(IupVbox(
    box2,
    IupSetAttributes(IupFrame(IupSetAttributes(IupHbox(IupSetAttributes(IupVbox(IupLabel("Name:"), IupSetAttributes(IupText(NULL), "VISIBLECOLUMNS=9, NAME=NEW_ATTRIB_NAME"), NULL), "GAP=0, MARGIN=5x5"),
    IupSetAttributes(IupVbox(IupLabel("Value:"), IupSetAttributes(IupText(NULL), "EXPAND=HORIZONTAL, NAME=NEW_ATTRIB_VALUE"), NULL), "GAP=0, MARGIN=5x5"),
    IupSetAttributes(IupVbox(IupLabel(""), IupSetCallbacks(IupSetAttributes(IupButton("Set", NULL), "PADDING=3x0, TIP=\"Sets an attribute value. Actually can be any attribute, registered or custom.\""), "ACTION", iGlobalsSetNewAttrib_CB, NULL), NULL), "GAP=0, MARGIN=5x5"),
    NULL), "ALIGNMENT=ACENTER")), "TITLE=\"Set Attribute\""),
    NULL), "MARGIN=0x0");

  box3 = IupHbox(IupSetAttributes(IupVbox(IupLabel("Name:"), list3, NULL), "MARGIN=0x0, GAP=0"), box13, NULL);
  iupAttribSet(box3, "_IUP_FUNCLIST", (char*)list3);
  iupAttribSet(box3, "_IUP_FUNCVALUE", (char*)value3);

  box4 = IupHbox(IupSetAttributes(IupVbox(IupLabel("Name:"), list4, NULL), "MARGIN=0x0, GAP=0"), box14, NULL);
  iupAttribSet(box4, "_IUP_NAMESLIST", (char*)list4);
  iupAttribSet(box4, "_IUP_NAMESVALUE", (char*)value4);

  tabs = IupTabs(box1, box2, box3, box4, NULL);
  IupSetAttribute(tabs, "TABTITLE0", "Registered Attributes");
  IupSetAttribute(tabs, "TABTITLE1", "Custom Attributes");
  IupSetAttribute(tabs, "TABTITLE2", "Functions");
  IupSetAttribute(tabs, "TABTITLE3", "Names");
  IupSetCallback(tabs, "TABCHANGEPOS_CB", (Icallback)iGlobalsTabChangePos_CB);
  iGlobalsTabChangePos_CB(tabs, 0, 0);

  dlg_box = IupVbox(
    tabs,
    button_box,
    NULL);

  IupSetAttribute(dlg_box, "MARGIN", "10x10");
  IupSetAttribute(dlg_box, "GAP", "10");
  IupAppend(box1, IupGetHandle("IupGlobalNamesNorm"));  /* to automatically normalize when the dialog is resized. Must be placed before the normalized controls. */

  dlg = IupDialog(dlg_box);
  IupSetAttribute(dlg, "TITLE", "Globals");
  IupSetAttributeHandle(dlg, "DEFAULTENTER", close);
  IupSetAttributeHandle(dlg, "DEFAULTESC", close);
  IupSetAttribute(dlg, "ICON", IupGetGlobal("ICON"));
  iupAttribSet(dlg, "_IUP_PROPLIST1", (char*)list1);
  iupAttribSet(dlg, "_IUP_PROPLIST2", (char*)list2);
  iupAttribSet(dlg, "_IUP_FUNCLIST", (char*)list3);
  iupAttribSet(dlg, "_IUP_NAMESLIST", (char*)list4);
  IupSetAttribute(dlg, "SIZE", "250x");
  IupSetAttribute(dlg, "MINBOX", "NO");
  IupSetAttribute(dlg, "MAXBOX", "NO");

  return dlg;
}

IUP_API Ihandle* IupGlobalsDialog(void)
{
  Ihandle* dlg = iGlobalsCreateDialog();
  iGlobalsUpdate(dlg);
  return dlg;
}
