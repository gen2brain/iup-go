/** \file
 * \brief Menu Resources.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_dialog.h"
#include "iup_str.h"
#include "iup_assert.h"
#include "iup_key.h"
#include "iup_stdcontrols.h"
#include "iup_drvinfo.h"
#include "iup_menu.h"
#include "iup_varg.h"


struct _IcontrolData 
{
  int child_id;       /* serial number used by child controls */
};

static Ihandle* iMenuGetTopMenu(Ihandle* ih)
{
  for (; ih->parent; ih = ih->parent)
    ; /* empty*/
  return ih;
}

int iupMenuGetChildId(Ihandle* ih)
{
  Ihandle* dlg = IupGetDialog(ih);
  if (dlg)
    return iupDialogGetChildId(ih);
  else
  {
    int id;
    ih = iMenuGetTopMenu(ih);
    if (!ih) return -1;
    id = ih->data->child_id;
    if (id == 0) id = 100; /* initial number */
    ih->data->child_id = id+1;
    return id;
  }
}

char* iupMenuGetChildIdStr(Ihandle* ih)
{
  /* Used only in Motif */
  Ihandle* dlg = IupGetDialog(ih);
  if (dlg)
    return iupDialogGetChildIdStr(ih);
  else
  {
    Ihandle* dialog = iMenuGetTopMenu(ih);
    return iupStrReturnStrf("iup-%s-%d", ih->iclass->name, dialog->data->child_id);
  }
}

int iupMenuIsMenuBar(Ihandle* ih)
{
  if (ih->parent && ih->parent->iclass->nativetype == IUP_TYPEDIALOG)
    return 1;
  else
    return 0;
}

static void iMenuAdjustPos(int *x, int *y)
{
  int cursor_x = 0, cursor_y = 0;
  int screen_width = 0, screen_height = 0;

  if (*x == IUP_CENTER || *y == IUP_CENTER ||
      *x == IUP_RIGHT  || *y == IUP_RIGHT)
    iupdrvGetScreenSize(&screen_width, &screen_height);

  if (*x == IUP_MOUSEPOS || *y == IUP_MOUSEPOS)
    iupdrvGetCursorPos(&cursor_x, &cursor_y);

  switch (*x)
  {
  case IUP_CENTER:
    *x = screen_width/2;
    break;
  case IUP_LEFT:
    *x = 0;
    break;
  case IUP_RIGHT:
    *x = screen_width;
    break;
  case IUP_MOUSEPOS:
    *x = cursor_x;
    break;
  }

  switch (*y)
  {
  case IUP_CENTER:
    *y = screen_height/2;
    break;
  case IUP_LEFT:
    *y = 0;
    break;
  case IUP_RIGHT:
    *y = screen_height;
    break;
  case IUP_MOUSEPOS:
    *y = cursor_y;
    break;
  }

  iupdrvAddScreenOffset(x, y, 1);
}

char* iupMenuProcessTitle(Ihandle* ih, const char* title)
{
  char* str;

  char* key = iupAttribGet(ih, "KEY");  /* NOT the same definition as the global KEY attribute */
  if (!key) return (char*)title;

  str = strchr(title, (int)(*key));
  if (str)
  {
    int len = (int)strlen(title);
    char *new_title = malloc(len+1+1);
    int pos = (int)(str - title);
    memcpy(new_title, title, pos);
    new_title[pos] = '&';
    memcpy(new_title+pos+1, title+pos, len-pos+1);
    return new_title;
  }

  return (char*)title;
}

int iupMenuPopup(Ihandle* ih, int x, int y)
{
  iMenuAdjustPos(&x, &y);
  return iupdrvMenuPopup(ih, x, y);
}


/******************************************************************/


static int iItemCreateMethod(Ihandle* ih, void** params)
{
  if (params)
  {
    if (params[0]) iupAttribSetStr(ih, "TITLE", (char*)(params[0]));
    if (params[1]) iupAttribSetStr(ih, "ACTION", (char*)(params[1]));
  }
  return IUP_NOERROR;
}

static int iSubmenuCreateMethod(Ihandle* ih, void** params)
{
  if (params)
  {
    if (params[0]) iupAttribSetStr(ih, "TITLE", (char*)(params[0]));
    if (params[1]) 
    {
      Ihandle* child = (Ihandle*)(params[1]);
      if (child->iclass->nativetype == IUP_TYPEMENU)
        IupAppend(ih, child);
    }
  }
  return IUP_NOERROR;
}

static int iMenuCreateMethod(Ihandle* ih, void** params)
{
  ih->data = iupALLOCCTRLDATA();

  if (params)
  {
    Ihandle** iparams = (Ihandle**)params;
    while (*iparams) 
    {
      Ihandle* child = (Ihandle*)(*iparams);
      if (child->iclass->nativetype == IUP_TYPEMENU)
        IupAppend(ih, child);
      iparams++;
    }
  }

  return IUP_NOERROR;
}


/******************************************************************************************/


Iclass* iupSeparatorNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "separator";
  ic->format = NULL;  /* no parameters */
  ic->nativetype = IUP_TYPEMENU;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 0;

  ic->New = iupSeparatorNewClass;

  /* Common */
  iupClassRegisterAttribute(ic, "WID", iupBaseGetWidAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT|IUPAF_NO_STRING);
  iupClassRegisterAttribute(ic, "NAME", NULL, iupBaseSetNameAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupdrvSeparatorInitClass(ic);

  return ic;
}

Iclass* iupItemNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "item";
  ic->format = "sa";  /* one string and one ACTION callback name */
  ic->format_attr = "TITLE";
  ic->nativetype = IUP_TYPEMENU;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;

  /* Class functions */
  ic->New = iupItemNewClass;
  ic->Create = iItemCreateMethod;

  /* Callbacks */
  iupClassRegisterCallback(ic, "HIGHLIGHT_CB", "");
  iupClassRegisterCallback(ic, "ACTION", "");

  /* Base Callbacks */
  iupBaseRegisterBaseCallbacks(ic);

  /* Common Callbacks */
  iupClassRegisterCallback(ic, "HELP_CB", "");

  /* Common */
  iupClassRegisterAttribute(ic, "WID", iupBaseGetWidAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT|IUPAF_NO_STRING);
  iupClassRegisterAttribute(ic, "NAME", NULL, iupBaseSetNameAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HANDLENAME", NULL, NULL, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "AUTOTOGGLE", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "KEY", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupdrvItemInitClass(ic);

  return ic;
}

Iclass* iupSubmenuNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "submenu";
  ic->format = "sh"; /* one string and one Ihandle */
  ic->format_attr = "TITLE";
  ic->nativetype = IUP_TYPEMENU;
  ic->childtype = IUP_CHILDMANY+1;  /* 1 child */
  ic->is_interactive = 1;

  /* Class functions */
  ic->New = iupSubmenuNewClass;
  ic->Create = iSubmenuCreateMethod;

  /* Base Callbacks */
  iupBaseRegisterBaseCallbacks(ic);

  /* Callbacks */
  iupClassRegisterCallback(ic, "HIGHLIGHT_CB", "");

  /* Common */
  iupClassRegisterAttribute(ic, "WID", iupBaseGetWidAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT|IUPAF_NO_STRING);
  iupClassRegisterAttribute(ic, "NAME", NULL, iupBaseSetNameAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HANDLENAME", NULL, NULL, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "KEY", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupdrvSubmenuInitClass(ic);

  return ic;
}

Iclass* iupMenuNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "menu";
  ic->format = "g"; /* (Ihandle**) */
  ic->nativetype = IUP_TYPEMENU;
  ic->childtype = IUP_CHILDMANY;  /* can have children */
  ic->is_interactive = 1;

  /* Class functions */
  ic->New = iupMenuNewClass;
  ic->Create = iMenuCreateMethod;

  /* Base Callbacks */
  iupBaseRegisterBaseCallbacks(ic);

  /* Callbacks */
  iupClassRegisterCallback(ic, "OPEN_CB", "");
  iupClassRegisterCallback(ic, "MENUCLOSE_CB", "");

  /* Common */
  iupClassRegisterAttribute(ic, "WID", iupBaseGetWidAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT|IUPAF_NO_STRING);
  iupClassRegisterAttribute(ic, "NAME", NULL, iupBaseSetNameAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "RADIO", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupdrvMenuInitClass(ic);

  return ic;
}

/************************************************************************/

IUP_API Ihandle* IupItem(const char* title, const char* action)
{
  void *params[2];
  params[0] = (void*)title;
  params[1] = (void*)action;
  return IupCreatev("item", params);
}

IUP_API Ihandle* IupSubmenu(const char* title, Ihandle* child)
{
  void *params[2];
  params[0] = (void*)title;
  params[1] = (void*)child;
  return IupCreatev("submenu", params);
}

IUP_API Ihandle* IupMenuv(Ihandle **children)
{
  return IupCreatev("menu", (void**)children);
}

IUP_API Ihandle* IupMenuV(Ihandle* child, va_list arglist)
{
  return IupCreateV("menu", child, arglist);
}

IUP_API Ihandle* IupMenu(Ihandle *child, ...)
{
  Ihandle *ih;

  va_list arglist;
  va_start(arglist, child);
  ih = IupCreateV("menu", child, arglist);
  va_end(arglist);

  return ih;
}

IUP_API Ihandle* IupSeparator(void)
{
  return IupCreate("separator");
}
