/** \file
 * \brief IupLayoutDialog pre-defined dialog
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>
#include <limits.h>
#include <ctype.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_focus.h"
#include "iup_dlglist.h"
#include "iup_assert.h"
#include "iup_drvdraw.h"
#include "iup_draw.h"
#include "iup_image.h"
#include "iup_childtree.h"
#include "iup_dialog.h"
#include "iup_drv.h"
#include "iup_func.h"
#include "iup_register.h"
#include "iup_layout.h"



static int iLayoutPropertiesClose_CB(Ihandle* ih)
{
  IupHide(IupGetDialog(ih));
  return IUP_DEFAULT;
}

IUP_SDK_API void iupLayoutPropertiesUpdate(Ihandle* properties, Ihandle* ih)
{
  int i, j, attr_count, cb_count, total_count = IupGetClassAttributes(ih->iclass->name, NULL, 0);
  char **attr_names = (char **)malloc(total_count * sizeof(char *));
  Ihandle* list1 = (Ihandle*)iupAttribGet(properties, "_IUP_PROPLIST1");
  Ihandle* list2 = (Ihandle*)iupAttribGet(properties, "_IUP_PROPLIST2");
  Ihandle* list3 = (Ihandle*)iupAttribGet(properties, "_IUP_PROPLIST3");

  /* Clear everything */
  IupSetAttribute(list1, "REMOVEITEM", NULL);
  IupSetAttribute(list2, "REMOVEITEM", NULL);
  IupSetAttribute(list3, "REMOVEITEM", NULL);
  IupSetAttribute(IupGetDialogChild(properties, "VALUE1A"), "VALUE", "");
  IupSetAttribute(IupGetDialogChild(properties, "VALUE1B"), "TITLE", "");
  IupSetAttribute(IupGetDialogChild(properties, "VALUE1C"), "TITLE", "");
  IupSetAttribute(IupGetDialogChild(properties, "VALUE2"), "VALUE", "");
  IupSetAttribute(IupGetDialogChild(properties, "VALUE3"), "VALUE", "");
  IupSetAttribute(IupGetDialogChild(properties, "SETBUT"), "ACTIVE", "No");
  IupSetAttribute(IupGetDialogChild(properties, "SETCOLORBUT"), "VISIBLE", "No");
  IupSetAttribute(IupGetDialogChild(properties, "SETFONTBUT"), "VISIBLE", "No");
  IupSetAttribute(IupGetDialogChild(properties, "SHOWHANDLEBUT"), "VISIBLE", "No");
  IupSetAttribute(IupGetDialogChild(properties, "IMAGELBL"), "VISIBLE", "No");
  IupSetAttribute(IupGetDialogChild(properties, "IDTEXT"), "ACTIVE", "No");
  IupSetAttribute(IupGetDialogChild(properties, "IDLABEL"), "ACTIVE", "No");

  attr_count = IupGetClassAttributes(ih->iclass->name, attr_names, total_count);
  for (i = 0; i < attr_count; i++)
    IupSetAttributeId(list1, "", i + 1, attr_names[i]);

  cb_count = total_count - attr_count;
  IupGetClassCallbacks(ih->iclass->name, attr_names, cb_count);
  for (i = 0; i<cb_count; i++)
    IupSetAttributeId(list3, "", i + 1, attr_names[i]);

  attr_count = IupGetAllAttributes(ih, NULL, 0);
  if (attr_count > total_count)
    attr_names = (char **)realloc(attr_names, attr_count * sizeof(char *));

  attr_count = IupGetAllAttributes(ih, attr_names, attr_count);
  for (i = 0, j = 1; i < attr_count; i++)
  {
    if (!iupClassAttribIsRegistered(ih->iclass, attr_names[i]))
    {
      IupSetAttributeId(list2, "", j, attr_names[i]);
      j++;
    }
  }

  iupAttribSet(properties, "_IUP_PROPELEMENT", (char*)ih);

  IupStoreAttribute(IupGetDialogChild(properties, "ELEMTITLE"), "VALUE", iupLayoutGetElementTitle(ih));

  if (ih->iclass->nativetype == IUP_TYPEIMAGE)
  {
    IupSetAttributeHandle(IupGetDialogChild(properties, "ELEMIMAGE"), "IMAGE", ih);
    IupSetAttribute(IupGetDialogChild(properties, "ELEMIMAGE"), "VISIBLE", "Yes");
  }
  else
    IupSetAttribute(IupGetDialogChild(properties, "ELEMIMAGE"), "VISIBLE", "No");

  free(attr_names);
}

static void iLayoutPropertiesAttribValueChanged(Ihandle* ih, const char* name)
{
  char* def_value;
  int flags;
  Ihandle* elem = (Ihandle*)iupAttribGetInherit(ih, "_IUP_PROPELEMENT");
  char* value = iupAttribGetLocal(elem, name);  /* do NOT check for inherited values */
  Ihandle* txt1 = IupGetDialogChild(ih, "VALUE1A");
  Ihandle* colorbut = IupGetDialogChild(ih, "SETCOLORBUT");
  Ihandle* handlebut = IupGetDialogChild(ih, "SHOWHANDLEBUT");
  Ihandle* imagelbl = IupGetDialogChild(ih, "IMAGELBL");

  iupClassGetAttribNameInfo(elem->iclass, name, &def_value, &flags);

  if (iupLayoutAttributeHasChanged(elem, name, value, def_value, flags))
    IupSetAttribute(txt1, "FGCOLOR", "255 0 0");
  else
    IupSetAttribute(txt1, "FGCOLOR", "0 0 0");

  if (!(flags&IUPAF_READONLY) &&
      !(flags&IUPAF_NO_STRING))
  {
    if (strstr(name, "COLOR") != NULL) /* if COLOR in attribute name, show the color selection button */
      IupStoreAttribute(colorbut, "BGCOLOR", value);  /* set it even if it is NULL */
  }

  if ((flags&IUPAF_IHANDLENAME || (flags & IUPAF_IHANDLE)))
  {
    Ihandle* handle;
    if (flags & IUPAF_IHANDLE)
    {
      iupAttribSet(handlebut, "_IUP_HANDLE", value);
      handle = (Ihandle*)value;
    }
    else
    {
      iupAttribSet(handlebut, "_IUP_HANDLE", NULL);
      handle = IupGetHandle(value);
    }

    if (iupObjectCheck(handle) && handle->iclass->nativetype == IUP_TYPEIMAGE)
    {
      IupSetAttribute(imagelbl, "VISIBLE", "Yes");
      IupSetAttributeHandle(imagelbl, "IMAGE", handle);
    }
    else
      IupSetAttribute(imagelbl, "VISIBLE", "No");
  }
}

static int iLayoutPropertiesIdTextChanged_CB(Ihandle* id_text)
{
  char* id = IupGetAttribute(id_text, "VALUE");
  if (id && id[0] != 0)
  {
    char* def_value;
    int flags;
    char *value, name[100];
    Ihandle* elem = (Ihandle*)iupAttribGetInherit(id_text, "_IUP_PROPELEMENT");
    Ihandle* txt1 = IupGetDialogChild(id_text, "VALUE1A");
    Ihandle* list1 = (Ihandle*)iupAttribGetInherit(id_text, "_IUP_PROPLIST1");
    char* itemlist1 = IupGetAttribute(list1, "VALUE");
    if (!itemlist1)
      return IUP_DEFAULT;

    strcpy(name, IupGetAttribute(list1, itemlist1));

    iupClassGetAttribNameInfo(elem->iclass, name, &def_value, &flags);

    strcat(name, id);

    value = IupGetAttribute(elem, name);
    if (value)
    {
      if (flags&IUPAF_NO_STRING)
        IupSetfAttribute(txt1, "VALUE", "%p", value);
      else
        IupStoreAttribute(txt1, "VALUE", value);
    }
    else
      IupSetAttribute(txt1, "VALUE", "NULL");

    iLayoutPropertiesAttribValueChanged(id_text, name);
  }
  return IUP_DEFAULT;
}

static void iLayoutPropertiesCallAttribChanged(Ihandle* dlg, char* name)
{
  IFns cb = (IFns)IupGetCallback(dlg, "ATTRIBCHANGED_CB");
  if (cb)
    cb(dlg, name);
}

static int iLayoutPropertiesSet_CB(Ihandle* button)
{
  Ihandle* list1 = (Ihandle*)iupAttribGetInherit(button, "_IUP_PROPLIST1");
  char* item = IupGetAttribute(list1, "VALUE");
  if (item)
  {
    Ihandle* dlg = IupGetDialog(button);
    Ihandle* elem = (Ihandle*)iupAttribGetInherit(button, "_IUP_PROPELEMENT");
    Ihandle* txt1 = IupGetDialogChild(button, "VALUE1A");
    char* value = IupGetAttribute(txt1, "VALUE");
    char* name = IupGetAttribute(list1, item);
    Ihandle* id_text = IupGetDialogChild(button, "IDTEXT");

    if (IupGetInt(id_text, "ACTIVE"))
    {
      char nameid[100];
      char* id = IupGetAttribute(id_text, "VALUE");
      if (!id || id[0] == 0)
        id = "";

      sprintf(nameid, "%s%s", name, id);

      if (!value || iupStrEqual(value, "NULL"))
        IupSetAttribute(elem, nameid, NULL);
      else
        IupStoreAttribute(elem, nameid, value);
    }
    else
    {
      if (!value || iupStrEqual(value, "NULL"))
        IupSetAttribute(elem, name, NULL);
      else
        IupStoreAttribute(elem, name, value);
    }

    iLayoutPropertiesAttribValueChanged(button, name);

    iLayoutPropertiesCallAttribChanged(dlg, name);
  }
  return IUP_DEFAULT;
}

static int iLayoutPropertiesSetColor_CB(Ihandle *colorbut)
{
  Ihandle* color_dlg = IupColorDlg();
  IupSetAttributeHandle(color_dlg, "PARENTDIALOG", IupGetDialog(colorbut));
  IupSetAttribute(color_dlg, "TITLE", "Choose Color");
  IupStoreAttribute(color_dlg, "VALUE", IupGetAttribute(colorbut, "BGCOLOR"));

  IupPopup(color_dlg, IUP_CENTER, IUP_CENTER);

  if (IupGetInt(color_dlg, "STATUS") == 1)
  {
    Ihandle* dlg = IupGetDialog(colorbut);
    Ihandle* elem = (Ihandle*)iupAttribGetInherit(colorbut, "_IUP_PROPELEMENT");
    Ihandle* list1 = (Ihandle*)iupAttribGetInherit(colorbut, "_IUP_PROPLIST1");
    Ihandle* txt1 = IupGetDialogChild(colorbut, "VALUE1A");
    char* value = IupGetAttribute(color_dlg, "VALUE");
    char* name = IupGetAttribute(list1, IupGetAttribute(list1, "VALUE"));
    Ihandle* id_text = IupGetDialogChild(colorbut, "IDTEXT");

    IupStoreAttribute(txt1, "VALUE", value);
    IupStoreAttribute(colorbut, "BGCOLOR", value);

    if (IupGetInt(id_text, "ACTIVE"))
    {
      char nameid[100];
      char* id = IupGetAttribute(id_text, "VALUE");
      if (!id || id[0] == 0)
        id = "";

      sprintf(nameid, "%s%s", name, id);
      IupStoreAttribute(elem, nameid, value);
    }
    else
      IupStoreAttribute(elem, name, value);

    iLayoutPropertiesAttribValueChanged(colorbut, name);

    iLayoutPropertiesCallAttribChanged(dlg, name);
  }

  IupDestroy(color_dlg);

  return IUP_DEFAULT;
}

static int iLayoutPropertiesSetFont_CB(Ihandle *fontbut)
{
  Ihandle* font_dlg = IupFontDlg();
  Ihandle* txt1 = IupGetDialogChild(fontbut, "VALUE1A");
  IupSetAttributeHandle(font_dlg, "PARENTDIALOG", IupGetDialog(fontbut));
  IupSetAttribute(font_dlg, "TITLE", "Choose Font");
  IupStoreAttribute(font_dlg, "VALUE", IupGetAttribute(txt1, "VALUE"));

  IupPopup(font_dlg, IUP_CENTER, IUP_CENTER);

  if (IupGetInt(font_dlg, "STATUS") == 1)
  {
    Ihandle* dlg = IupGetDialog(fontbut);
    Ihandle* elem = (Ihandle*)iupAttribGetInherit(fontbut, "_IUP_PROPELEMENT");
    Ihandle* list1 = (Ihandle*)iupAttribGetInherit(fontbut, "_IUP_PROPLIST1");
    char* value = IupGetAttribute(font_dlg, "VALUE");
    char* name = IupGetAttribute(list1, IupGetAttribute(list1, "VALUE"));
    Ihandle* id_text = IupGetDialogChild(fontbut, "IDTEXT");

    IupStoreAttribute(txt1, "VALUE", value);

    if (IupGetInt(id_text, "ACTIVE"))
    {
      char nameid[100];
      char* id = IupGetAttribute(id_text, "VALUE");
      if (!id || id[0] == 0)
        id = "";

      sprintf(nameid, "%s%s", name, id);
      IupStoreAttribute(elem, nameid, value);
    }
    else
      IupStoreAttribute(elem, name, value);

    iLayoutPropertiesAttribValueChanged(fontbut, name);

    iLayoutPropertiesCallAttribChanged(dlg, name);
  }

  IupDestroy(font_dlg);

  return IUP_DEFAULT;
}

static int iLayoutPropertiesShowHandle_CB(Ihandle *handlebut)
{
  Ihandle* elem = (Ihandle*)iupAttribGet(handlebut, "_IUP_HANDLE");
  if (!elem)  /* Handle Name */
  {
    Ihandle* txt1 = IupGetDialogChild(handlebut, "VALUE1A");
    char* name = IupGetAttribute(txt1, "VALUE");
    elem = IupGetHandle(name);
  }

  if (iupObjectCheck(elem))
  {
    Ihandle* dlg = IupElementPropertiesDialog(IupGetDialog(handlebut), elem);
    IupPopup(dlg, IUP_CENTERPARENT, IUP_CENTERPARENT);
    IupDestroy(dlg);
  }
  else
  {
    if (elem == NULL)
      IupMessageError(IupGetDialog(handlebut), "Handle not set");
    else
      IupMessageError(IupGetDialog(handlebut), "Invalid Handle!");
  }
  return IUP_DEFAULT;
}

static int iLayoutPropertiesList1_CB(Ihandle *list1, char *name, int item, int state)
{
  (void)item;
  if (state)
  {
    char* def_value;
    int flags;
    Ihandle* elem = (Ihandle*)iupAttribGetInherit(list1, "_IUP_PROPELEMENT");
    char* value = iupAttribGetLocal(elem, name);  /* do NOT check for inherited values */
    Ihandle* txt1 = IupGetDialogChild(list1, "VALUE1A");
    Ihandle* lbl2 = IupGetDialogChild(list1, "VALUE1B");
    Ihandle* lbl3 = IupGetDialogChild(list1, "VALUE1C");
    Ihandle* setbut = IupGetDialogChild(list1, "SETBUT");
    Ihandle* colorbut = IupGetDialogChild(list1, "SETCOLORBUT");
    Ihandle* fontbut = IupGetDialogChild(list1, "SETFONTBUT");
    Ihandle* handlebut = IupGetDialogChild(list1, "SHOWHANDLEBUT");
    Ihandle* id_text = IupGetDialogChild(list1, "IDTEXT");
    Ihandle* id_label = IupGetDialogChild(list1, "IDLABEL");

    iupClassGetAttribNameInfo(elem->iclass, name, &def_value, &flags);

    if (value)
    {
      if (flags&IUPAF_NO_STRING)
        IupSetfAttribute(txt1, "VALUE", "%p", value);
      else
        IupStoreAttribute(txt1, "VALUE", value);
    }
    else
      IupSetAttribute(txt1, "VALUE", "NULL");

    if (def_value)
      IupStoreAttribute(lbl2, "TITLE", def_value);
    else
      IupSetAttribute(lbl2, "TITLE", "NULL");

    IupSetfAttribute(lbl3, "TITLE", "%s\n%s%s%s%s%s", flags&(IUPAF_NO_INHERIT | IUPAF_NO_STRING) ? "NON Inheritable" : "Inheritable",
                     flags&IUPAF_NO_STRING ? "NOT a String\n" : "",
                     flags&IUPAF_HAS_ID ? "Has ID\n" : "",
                     flags&IUPAF_READONLY ? "Read-Only\n" : (flags&IUPAF_WRITEONLY ? "Write-Only\n" : ""),
                     flags&IUPAF_IHANDLENAME ? "Ihandle* name\n" : "",
                     flags&IUPAF_NOT_SUPPORTED ? "NOT SUPPORTED in this driver" : "");

    if (!(flags&IUPAF_READONLY) &&
        !(flags&IUPAF_NO_STRING))
    {
      IupSetAttribute(setbut, "ACTIVE", "Yes");
      IupSetAttribute(txt1, "READONLY", "No");

      if (strstr(name, "COLOR") != NULL) /* if COLOR in attribute name, show the color selection button */
        IupSetAttribute(colorbut, "VISIBLE", "Yes");
      else
        IupSetAttribute(colorbut, "VISIBLE", "No");

      if (strstr(name, "FONT") != NULL &&      /* if FONT in attribute name, show the font selection button, */
          strstr(name, "FONTFACE") == NULL &&  /* but don't show partial font attributes */
          strstr(name, "FONTSTYLE") == NULL &&
          strstr(name, "FONTSIZE") == NULL)
        IupSetAttribute(fontbut, "VISIBLE", "Yes");
      else
        IupSetAttribute(fontbut, "VISIBLE", "No");
    }
    else
    {
      IupSetAttribute(txt1, "READONLY", "Yes");
      IupSetAttribute(setbut, "ACTIVE", "No");
      IupSetAttribute(colorbut, "VISIBLE", "No");
      IupSetAttribute(fontbut, "VISIBLE", "No");
    }

    if (flags&IUPAF_HAS_ID)
    {
      IupSetAttribute(id_text, "ACTIVE", "Yes");
      IupSetAttribute(id_label, "ACTIVE", "Yes");
    }
    else
    {
      IupSetAttribute(id_text, "ACTIVE", "No");
      IupSetAttribute(id_label, "ACTIVE", "No");
    }

    if (flags&IUPAF_IHANDLENAME || flags & IUPAF_IHANDLE)
      IupSetAttribute(handlebut, "VISIBLE", "Yes");
    else
      IupSetAttribute(handlebut, "VISIBLE", "No");

    iLayoutPropertiesAttribValueChanged(list1, name);
  }

  return IUP_DEFAULT;
}

static int iLayoutPropertiesList2_CB(Ihandle *list2, char *name, int item, int state)
{
  (void)item;
  if (state)
  {
    Ihandle* elem = (Ihandle*)iupAttribGetInherit(list2, "_IUP_PROPELEMENT");
    char* value = iupAttribGet(elem, name);
    Ihandle* lbl = IupGetDialogChild(list2, "VALUE2");
    if (value)
      IupSetfAttribute(lbl, "VALUE", "%p", value);
    else
      IupSetAttribute(lbl, "VALUE", "NULL");
  }
  return IUP_DEFAULT;
}

static int iLayoutPropertiesGetAsString_CB(Ihandle *button)
{
  Ihandle* elem = (Ihandle*)iupAttribGetInherit(button, "_IUP_PROPELEMENT");
  Ihandle* list2 = (Ihandle*)iupAttribGetInherit(button, "_IUP_PROPLIST2");
  char* item = IupGetAttribute(list2, "VALUE");
  if (item)
  {
    char* value = iupAttribGet(elem, IupGetAttribute(list2, item));
    Ihandle* lbl = IupGetDialogChild(button, "VALUE2");
    if (value)
      IupStoreAttribute(lbl, "VALUE", value);
    else
      IupSetAttribute(lbl, "VALUE", "NULL");
  }
  return IUP_DEFAULT;
}

static int iLayoutPropertiesSetStr_CB(Ihandle* button)
{
  Ihandle* dlg = IupGetDialog(button);
  Ihandle* elem = (Ihandle*)iupAttribGetInherit(button, "_IUP_PROPELEMENT");
  char* name = IupGetAttribute(IupGetDialogChild(button, "NAME22"), "VALUE");
  char* value = IupGetAttribute(IupGetDialogChild(button, "VALUE22"), "VALUE");
  if (!value || iupStrEqual(value, "NULL"))
    IupSetAttribute(elem, name, NULL);
  else
    IupStoreAttribute(elem, name, value);

  iupLayoutPropertiesUpdate(dlg, elem);

  iLayoutPropertiesCallAttribChanged(dlg, name);

  return IUP_DEFAULT;
}

static int iLayoutPropertiesList3_CB(Ihandle *list3, char *text, int item, int state)
{
  (void)item;
  if (state)
  {
    Ihandle* elem = (Ihandle*)iupAttribGetInherit(list3, "_IUP_PROPELEMENT");
    Icallback cb = IupGetCallback(elem, text);
    Ihandle* txt = IupGetDialogChild(list3, "VALUE3");
    if (cb)
    {
      char* cb_name = iupGetCallbackName(elem, text);
      if (cb_name)
        IupSetfAttribute(txt, "VALUE", "%p [\"%s\"]", cb, cb_name);
      else
        IupSetfAttribute(txt, "VALUE", "%p", cb);
    }
    else
    {
      char* cb_name = iupGetCallbackName(elem, text);
      if (cb_name)
        IupSetfAttribute(txt, "VALUE", "NULL [\"%s\"]", cb_name);
      else
        IupSetAttribute(txt, "VALUE", "NULL");
    }
  }
  return IUP_DEFAULT;
}

static int iLayoutPropertiesTabChangePos_CB(Ihandle* ih, int new_pos, int old_pos)
{
  (void)old_pos;
  switch (new_pos)
  {
  case 0:
    IupSetAttribute(ih, "TIP", "All attributes that are known by the element.");
    break;
  case 1:
    IupSetAttribute(ih, "TIP", "Custom attributes set by the application.");
    break;
  case 2:
    IupSetAttribute(ih, "TIP", "All callbacks that are known by the element.");
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

static Ihandle* iLayoutPropertiesCreateDialog(Ihandle* parent)
{
  Ihandle *list1, *list2, *list3, *close, *dlg, *dlg_box, *button_box, *colorbut, *fontbut, *handlebut,
    *tabs, *box1, *box11, *box2, *box22, *box3, *box33, *set, *id_text, *id_label, *imagelbl;

  close = IupButton("Close", NULL);
  IupSetStrAttribute(close, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
  IupSetCallback(close, "ACTION", (Icallback)iLayoutPropertiesClose_CB);

  button_box = IupHbox(
    IupFill(),
    close,
    NULL);
  IupSetAttribute(button_box, "MARGIN", "0x0");

  list1 = IupList(NULL);
  IupSetCallback(list1, "ACTION", (Icallback)iLayoutPropertiesList1_CB);
  IupSetAttribute(list1, "VISIBLELINES", "15");
  IupSetAttribute(list1, "VISIBLECOLUMNS", "11");
  IupSetAttribute(list1, "SORT", "Yes");
  IupSetAttribute(list1, "EXPAND", "VERTICAL");

  list2 = IupList(NULL);
  IupSetCallback(list2, "ACTION", (Icallback)iLayoutPropertiesList2_CB);
  IupSetAttribute(list2, "VISIBLELINES", "15");
  IupSetAttribute(list2, "VISIBLECOLUMNS", "11");
  IupSetAttribute(list2, "SORT", "Yes");
  IupSetAttribute(list2, "EXPAND", "VERTICAL");

  list3 = IupList(NULL);
  IupSetCallback(list3, "ACTION", (Icallback)iLayoutPropertiesList3_CB);
  IupSetAttribute(list3, "VISIBLELINES", "15");
  IupSetAttribute(list3, "VISIBLECOLUMNS", "14");
  IupSetAttribute(list3, "SORT", "Yes");
  IupSetAttribute(list3, "EXPAND", "VERTICAL");

  set = IupButton("Set", NULL);
  IupSetCallback(set, "ACTION", iLayoutPropertiesSet_CB);
  IupSetStrAttribute(set, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
  IupSetAttribute(set, "NAME", "SETBUT");

  id_text = IupText(NULL);
  IupSetCallback(id_text, "VALUECHANGED_CB", (Icallback)iLayoutPropertiesIdTextChanged_CB);
  IupSetAttribute(id_text, "VISIBLECOLUMNS", "5");
  IupSetAttribute(id_text, "NAME", "IDTEXT");

  id_label = IupLabel("Id:");
  IupSetAttribute(id_label, "NAME", "IDLABEL");

  colorbut = IupButton(NULL, NULL);
  IupSetAttribute(colorbut, "SIZE", "20x10");
  IupStoreAttribute(colorbut, "BGCOLOR", "0 0 0");
  IupSetCallback(colorbut, "ACTION", (Icallback)iLayoutPropertiesSetColor_CB);
  IupSetAttribute(colorbut, "NAME", "SETCOLORBUT");
  IupSetAttribute(colorbut, "VISIBLE", "NO");

  fontbut = IupButton("F", NULL);
  IupSetAttribute(fontbut, "SIZE", "20x10");
  IupStoreAttribute(fontbut, "FONT", "Times, Bold Italic 12");
  IupSetCallback(fontbut, "ACTION", (Icallback)iLayoutPropertiesSetFont_CB);
  IupSetAttribute(fontbut, "NAME", "SETFONTBUT");
  IupSetAttribute(fontbut, "VISIBLE", "NO");

  handlebut = IupButton("Handle", NULL);
  IupSetCallback(handlebut, "ACTION", (Icallback)iLayoutPropertiesShowHandle_CB);
  IupSetAttribute(handlebut, "NAME", "SHOWHANDLEBUT");
  IupSetAttribute(handlebut, "VISIBLE", "NO");

  imagelbl = IupLabel(NULL);
  IupSetAttribute(imagelbl, "NAME", "IMAGELBL");
  IupSetAttribute(imagelbl, "IMAGE", "IMGEMPTY");
  IupSetAttribute(imagelbl, "ALIGNMENT", "ACENTER:ACENTER");
  IupSetAttribute(imagelbl, "VISIBLE", "NO");
  IupSetAttribute(imagelbl, "RASTERSIZE", "32x32");
  IupSetAttribute(imagelbl, "EXPAND", "HORIZONTALFREE");

  box11 = IupVbox(
    IupLabel("Value:"),
    IupSetAttributes(IupHbox(IupSetAttributes(IupText(NULL), "MULTILINE=Yes, ALIGNMENT=ALEFT:ATOP, EXPAND=YES, NAME=VALUE1A"), IupSetAttributes(IupVbox(set, id_label, id_text, colorbut, fontbut, handlebut, imagelbl, NULL), "GAP=3"), NULL), "GAP=10"),
    IupSetAttributes(IupFill(), "RASTERSIZE=10"),
    IupLabel("Default Value:"),
    IupFrame(IupSetAttributes(IupLabel(NULL), "ALIGNMENT=ALEFT:ATOP, EXPAND=HORIZONTAL, NAME=VALUE1B")),
    IupSetAttributes(IupFill(), "RASTERSIZE=10"),
    IupLabel("Other Info:"),
    IupFrame(IupSetAttributes(IupLabel(NULL), "SIZE=90x48, ALIGNMENT=ALEFT:ATOP, NAME=VALUE1C")),
    NULL);
  IupSetAttribute(box11, "MARGIN", "0x0");
  IupSetAttribute(box11, "GAP", "0");

  box22 = IupVbox(
    IupLabel("Value:"),
    IupSetAttributes(IupText(NULL), "MULTILINE=Yes, ALIGNMENT=ALEFT:ATOP, EXPAND=YES, NAME=VALUE2, READONLY=Yes"),
    IupSetAttributes(IupFill(), "RASTERSIZE=10"),
    IupSetCallbacks(IupSetAttributes(IupButton("Get as String", NULL), "PADDING=3x3"), "ACTION", iLayoutPropertiesGetAsString_CB, NULL),
    IupLabel("IMPORTANT: if the attribute is not a string\nthis can crash the application."),
    IupSetAttributes(IupFill(), "SIZE=60"),
    NULL);
  IupSetAttribute(box22, "MARGIN", "0x0");
  IupSetAttribute(box22, "GAP", "0");

  box33 = IupVbox(
    IupLabel("Value:"),
    IupSetAttributes(IupText(NULL), "EXPAND=HORIZONTAL, READONLY=Yes, NAME=VALUE3"),
    NULL);
  IupSetAttribute(box33, "MARGIN", "0x0");
  IupSetAttribute(box33, "GAP", "0");

  box1 = IupHbox(IupSetAttributes(IupVbox(IupLabel("Name:"), list1, NULL), "MARGIN=0x0, GAP=0"), box11, NULL);
  box2 = IupHbox(IupSetAttributes(IupVbox(IupLabel("Name:"), list2, NULL), "MARGIN=0x0, GAP=0"), box22, NULL);
  box3 = IupHbox(IupSetAttributes(IupVbox(IupLabel("Name:"), list3, NULL), "MARGIN=0x0, GAP=0"), box33, NULL);

  box2 = IupSetAttributes(IupVbox(
    box2,
    IupSetAttributes(IupFrame(IupSetAttributes(IupHbox(
      IupSetAttributes(IupVbox(IupLabel("Name:"), IupSetAttributes(IupText(NULL), "VISIBLECOLUMNS=9, NAME=NAME22"), NULL), "GAP=0, MARGIN=5x5"),
      IupSetAttributes(IupVbox(IupLabel("Value:"), IupSetAttributes(IupText(NULL), "EXPAND=HORIZONTAL, NAME=VALUE22"), NULL), "GAP=0, MARGIN=5x5"),
      IupSetAttributes(IupVbox(IupLabel(""), IupSetCallbacks(IupSetAttributes(IupButton("Set", NULL), "PADDING=3x0, TIP=\"Sets an attribute value. Actually can be any attribute, registered or custom.\""), "ACTION", iLayoutPropertiesSetStr_CB, NULL), NULL), "GAP=0, MARGIN=5x5"),
      NULL), "ALIGNMENT=ACENTER")), "TITLE=\"Set Attribute\""), 
    NULL), "MARGIN=0x0");

  tabs = IupTabs(box1, box2, box3, NULL);
  IupSetAttribute(tabs, "TABTITLE0", "Registered Attributes");
  IupSetAttribute(tabs, "TABTITLE1", "Custom Attributes");
  IupSetAttribute(tabs, "TABTITLE2", "Callbacks");
  IupSetCallback(tabs, "TABCHANGEPOS_CB", (Icallback)iLayoutPropertiesTabChangePos_CB);
  iLayoutPropertiesTabChangePos_CB(tabs, 0, 0);

  dlg_box = IupVbox(
    IupSetAttributes(IupHbox(
      IupSetAttributes(IupText(NULL), "EXPAND=HORIZONTAL, READONLY=Yes, NAME=ELEMTITLE, BORDER=NO"),
      IupSetAttributes(IupLabel(NULL), "RASTERSIZE=32x32, IMAGE=IMGEMPTY, NAME=ELEMIMAGE, VISIBLE=NO"),
      NULL), "MARGIN=0x0, GAP=5"),
    tabs,
    button_box,
    NULL);

  IupSetAttribute(dlg_box, "MARGIN", "10x10");
  IupSetAttribute(dlg_box, "GAP", "10");

  dlg = IupDialog(dlg_box);
  IupSetAttribute(dlg, "TITLE", "Element Properties");
  IupSetAttribute(dlg, "MINBOX", "NO");
  IupSetAttribute(dlg, "MAXBOX", "NO");
  IupSetAttributeHandle(dlg, "DEFAULTENTER", close);
  IupSetAttributeHandle(dlg, "DEFAULTESC", close);
  if (parent) IupSetAttributeHandle(dlg, "PARENTDIALOG", parent);
  IupSetAttribute(dlg, "ICON", IupGetGlobal("ICON"));
  iupAttribSet(dlg, "_IUP_PROPLIST1", (char*)list1);
  iupAttribSet(dlg, "_IUP_PROPLIST2", (char*)list2);
  iupAttribSet(dlg, "_IUP_PROPLIST3", (char*)list3);

  IupStoreAttribute(IupGetDialogChild(dlg, "ELEMTITLE"), "BGCOLOR", IupGetAttribute(dlg, "BGCOLOR"));

  return dlg;
}

IUP_API Ihandle* IupElementPropertiesDialog(Ihandle* parent, Ihandle* elem)
{
  Ihandle* dlg = iLayoutPropertiesCreateDialog(parent);
  iupLayoutPropertiesUpdate(dlg, elem);
  return dlg;
}
