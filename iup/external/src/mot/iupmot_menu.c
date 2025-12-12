/** \file
 * \brief Menu Resources
 *
 * See Copyright Notice in "iup.h"
 */

#include <Xm/Xm.h>
#include <Xm/RowColumn.h>
#include <Xm/Separator.h>
#include <Xm/CascadeB.h>
#include <Xm/ToggleB.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>
#include <stdint.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_childtree.h"
#include "iup_attrib.h"
#include "iup_dialog.h"
#include "iup_str.h"
#include "iup_label.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_image.h"
#include "iup_menu.h"

#include "iupmot_drv.h"
#include "iupmot_color.h"


static int mot_menu_exitloop = 0;

int iupdrvMenuPopup(Ihandle* ih, int x, int y)
{
  char* value = iupAttribGet(ih, "POPUPALIGN");
  XButtonEvent ev;
  ev.x_root = x;
  ev.y_root = y;

  if (value)
  {
    int width, height;
    char value1[30], value2[30];
    iupStrToStrStr(value, value1, value2, ':');

    iupmotGetWindowSize(ih, &width, &height); /* Have to ideia if this is going to work */

    if (iupStrEqualNoCase(value1, "ARIGHT"))
      ev.x_root -= width;
    else if (iupStrEqualNoCase(value1, "ACENTER"))
      ev.x_root -= width / 2;

    if (iupStrEqualNoCase(value2, "ABOTTOM"))
      ev.y_root -= height;
    else if (iupStrEqualNoCase(value2, "ACENTER"))
      ev.y_root -= height / 2;
  }

  XmMenuPosition(ih->handle, &ev);

  XtManageChild(ih->handle);

  mot_menu_exitloop = 0;
  while (!mot_menu_exitloop)
    XtAppProcessEvent(iupmot_appcontext, XtIMAll);
  mot_menu_exitloop = 0;

  return IUP_NOERROR;
}

IUP_SDK_API int iupdrvMenuGetMenuBarSize(Ihandle* ih)
{
  int ch;
  iupdrvFontGetCharSize(ih, NULL, &ch);
  return 5 + ch + 5;
}


/*******************************************************************************************/


static void motItemActivateCallback(Widget w, Ihandle* ih, XtPointer call_data)
{
  Icallback cb;

  if (XmIsToggleButton(ih->handle) && !iupAttribGetBoolean(ih, "AUTOTOGGLE") && !iupAttribGetBoolean(ih->parent, "RADIO"))
  {
    /* Motif by default will do autotoggle */
    XmToggleButtonSetState(ih->handle, !XmToggleButtonGetState(ih->handle),0);
  }

  cb = IupGetCallback(ih, "ACTION");
  if (cb && cb(ih)==IUP_CLOSE)
    IupExitLoop();

  (void)w;
  (void)call_data;
}

static void motItemArmCallback(Widget w, Ihandle* ih, XtPointer call_data)
{
  Icallback cb = IupGetCallback(ih, "HIGHLIGHT_CB");
  if (cb)
    cb(ih);

  (void)w;
  (void)call_data;
}

static void motMenuMapCallback(Widget w, Ihandle* ih, XtPointer call_data)
{
  Icallback cb = IupGetCallback(ih, "OPEN_CB");
  if (!cb && ih->parent) cb = (Icallback)IupGetCallback(ih->parent, "OPEN_CB");  /* check also in the Submenu */
  if (cb) cb(ih);

  (void)w;
  (void)call_data;
}

static void motMenuUnmapCallback(Widget w, Ihandle* ih, XtPointer call_data)
{
  Icallback cb = IupGetCallback(ih, "MENUCLOSE_CB");
  if (!cb && ih->parent) cb = (Icallback)IupGetCallback(ih->parent, "MENUCLOSE_CB");  /* check also in the Submenu */
  if (cb) cb(ih);

  (void)w;
  (void)call_data;
}

static void motPopupMenuUnmapCallback(Widget w, Ihandle* ih, XtPointer call_data)
{
  motMenuUnmapCallback(w, ih, call_data);
  mot_menu_exitloop = 1;
}


/*******************************************************************************************/


static void motMenuUnMapMethod(Ihandle* ih)
{
  if (iupMenuIsMenuBar(ih))
  {
    XtDestroyWidget(ih->handle);
    ih->parent = NULL;
  }
  else
    XtDestroyWidget(XtParent(ih->handle));  /* in this case the RowColumn widget is a child of a MenuShell. */
}

static int motMenuMapMethod(Ihandle* ih)
{
  int num_args = 0;
  Arg args[30];

  if (iupMenuIsMenuBar(ih))
  {
    /* top level menu used for MENU attribute in IupDialog (a menu bar) */

    ih->handle = XtVaCreateManagedWidget(
                   iupMenuGetChildIdStr(ih),
                   xmRowColumnWidgetClass, 
                   iupChildTreeGetNativeParentHandle(ih),
                   XmNrowColumnType, XmMENU_BAR,
                   XmNmarginHeight, 0,
                   XmNmarginWidth, 0,
                   NULL);
    if (!ih->handle)
      return IUP_ERROR;
  }
  else
  {
    /* all XmCreate* functions here, also create RowColumn Widgets. */

    if (ih->parent)
    {
      /* parent is a submenu */

      if (iupAttribGetBoolean(ih, "RADIO"))
      {
        iupMOT_SETARG(args, num_args, XmNpacking, XmPACK_COLUMN);
        iupMOT_SETARG(args, num_args, XmNradioBehavior, TRUE);
      }

      ih->handle = XmCreatePulldownMenu(
                     ih->parent->handle, 
                     iupMenuGetChildIdStr(ih),
                     args,
                     num_args);
      if (!ih->handle)
        return IUP_ERROR;

      /* update the CascadeButton */
      XtVaSetValues(ih->parent->handle, XmNsubMenuId, ih->handle, NULL);  

      XtAddCallback(ih->handle, XmNmapCallback, (XtCallbackProc)motMenuMapCallback, (XtPointer)ih);
      XtAddCallback(ih->handle, XmNunmapCallback, (XtCallbackProc)motMenuUnmapCallback, (XtPointer)ih);
    }
    else
    {
      /* top level menu used for IupPopup */

      iupMOT_SETARG(args, num_args, XmNpopupEnabled, XmPOPUP_AUTOMATIC);

      ih->handle = XmCreatePopupMenu(
                     iupmot_appshell, 
                     iupMenuGetChildIdStr(ih),
                     args,
                     num_args);
      if (!ih->handle)
        return IUP_ERROR;

      XtAddCallback(ih->handle, XmNmapCallback, (XtCallbackProc)motMenuMapCallback, (XtPointer)ih);
      XtAddCallback(ih->handle, XmNunmapCallback, (XtCallbackProc)motPopupMenuUnmapCallback, (XtPointer)ih);
    }
  }

  ih->serial = iupMenuGetChildId(ih); /* must be after using the string */

  return IUP_NOERROR;
}

void iupdrvMenuInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = motMenuMapMethod;
  ic->UnMap = motMenuUnMapMethod;

  /* Used by iupdrvMenuGetMenuBarSize */
  iupClassRegisterAttribute(ic, "FONT", NULL, NULL, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);  /* inherited */

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
}


/*******************************************************************************************/


static int motItemSetTitleImageAttrib(Ihandle* ih, const char* value)
{
  if (XmIsToggleButton(ih->handle))
    iupmotSetPixmap(ih, value, XmNlabelPixmap, 0);
  return 1;
}

static int motItemSetTitleAttrib(Ihandle* ih, const char* value)
{
  char *str;

  if (!value)
  {
    str = "     ";
    value = str;
  }
  else
    str = iupMenuProcessTitle(ih, value);

  if (XmIsToggleButton(ih->handle))
  {
    char *p = strchr(str, '\t');
    if (p)
    {
      int offset = (int)(p-str);
      char* new_value = iupStrDup(str);
      char* acc_value = new_value + offset + 1;
      new_value[offset] = 0;
      iupmotSetMnemonicTitle(ih, NULL, 0, new_value);
      iupmotSetXmString(ih->handle, XmNacceleratorText, acc_value);
      free(new_value);

      if (str != value) free(str);
      return 1;
    }
  }

  iupmotSetMnemonicTitle(ih, NULL, 0, str);

  if (str != value) free(str);
  return 1;
}

static int motItemSetValueAttrib(Ihandle* ih, const char* value)
{
  if (XmIsToggleButton(ih->handle))
  {
    if (iupAttribGetBoolean(ih->parent, "RADIO"))
      value = "ON";

    XmToggleButtonSetState(ih->handle, iupStrBoolean(value),0);
  }
  return 0;
}

static char* motItemGetValueAttrib(Ihandle* ih)
{
  return iupStrReturnChecked(XmIsToggleButton(ih->handle) && XmToggleButtonGetState(ih->handle));
}

/*******************************************************************************************/

static void motRecentItemActivateCallback(Widget w, XtPointer client_data, XtPointer call_data)
{
  int index = (int)(intptr_t)client_data;
  Ihandle* menu = NULL;
  Icallback recent_cb;
  Ihandle* config;
  char attr_name[32];
  const char* filename;

  XtVaGetValues(w, XmNuserData, &menu, NULL);
  if (!menu)
    return;

  recent_cb = (Icallback)iupAttribGet(menu, "_IUP_RECENT_CB");
  config = (Ihandle*)iupAttribGet(menu, "_IUP_CONFIG");

  if (!recent_cb || !config)
    return;

  sprintf(attr_name, "_IUP_RECENT_FILE%d", index);
  filename = iupAttribGet(menu, attr_name);

  if (filename)
  {
    IupSetStrAttribute(config, "RECENTFILENAME", filename);
    IupSetStrAttribute(config, "TITLE", filename);
    config->parent = menu;

    if (recent_cb(config) == IUP_CLOSE)
      IupExitLoop();

    config->parent = NULL;
    IupSetAttribute(config, "RECENTFILENAME", NULL);
    IupSetAttribute(config, "TITLE", NULL);
  }

  (void)call_data;
}

int iupdrvRecentMenuInit(Ihandle* menu, int max_recent, Icallback recent_cb)
{
  iupAttribSetInt(menu, "_IUP_RECENT_MAX", max_recent);
  iupAttribSet(menu, "_IUP_RECENT_CB", (char*)recent_cb);
  iupAttribSetInt(menu, "_IUP_RECENT_COUNT", 0);
  return 0;
}

int iupdrvRecentMenuUpdate(Ihandle* menu, const char** filenames, int count, Icallback recent_cb)
{
  Widget menu_widget;
  int max_recent, existing, i;

  if (!menu || !menu->handle)
    return -1;

  menu_widget = (Widget)menu->handle;
  max_recent = iupAttribGetInt(menu, "_IUP_RECENT_MAX");
  existing = iupAttribGetInt(menu, "_IUP_RECENT_COUNT");

  if (count > max_recent)
    count = max_recent;

  iupAttribSet(menu, "_IUP_RECENT_CB", (char*)recent_cb);

  for (i = 0; i < count; i++)
  {
    char attr_name[32];
    Widget item;
    XmString xm_title;

    sprintf(attr_name, "_IUP_RECENT_FILE%d", i);
    iupAttribSetStr(menu, attr_name, filenames[i]);

    sprintf(attr_name, "_IUP_RECENT_ITEM%d", i);
    item = (Widget)iupAttribGet(menu, attr_name);

    xm_title = XmStringCreateLocalized((char*)filenames[i]);

    if (item)
    {
      XtVaSetValues(item, XmNlabelString, xm_title, NULL);
    }
    else
    {
      item = XtVaCreateManagedWidget("recentitem",
        xmCascadeButtonWidgetClass, menu_widget,
        XmNlabelString, xm_title,
        XmNuserData, menu,
        XmNpositionIndex, i,
        NULL);

      XtAddCallback(item, XmNactivateCallback,
        (XtCallbackProc)motRecentItemActivateCallback, (XtPointer)(intptr_t)i);

      iupAttribSet(menu, attr_name, (char*)item);
    }

    XmStringFree(xm_title);
  }

  for (; i < existing; i++)
  {
    char attr_name[32];
    Widget item;

    sprintf(attr_name, "_IUP_RECENT_ITEM%d", i);
    item = (Widget)iupAttribGet(menu, attr_name);
    if (item)
    {
      XtDestroyWidget(item);
      iupAttribSet(menu, attr_name, NULL);
    }

    sprintf(attr_name, "_IUP_RECENT_FILE%d", i);
    iupAttribSet(menu, attr_name, NULL);
  }

  iupAttribSetInt(menu, "_IUP_RECENT_COUNT", count);
  return 0;
}

/*******************************************************************************************/

static int motItemMapMethod(Ihandle* ih)
{
  int pos;

  if (!ih->parent)
    return IUP_ERROR;

  /* Menu bar can contain only CascadeButtons */
  if (iupMenuIsMenuBar(ih->parent))
  {
    ih->handle = XtVaCreateManagedWidget(
                   iupMenuGetChildIdStr(ih),
                   xmCascadeButtonWidgetClass, 
                   ih->parent->handle,
                   NULL);

    XtAddCallback(ih->handle, XmNactivateCallback, (XtCallbackProc)motItemActivateCallback, (XtPointer)ih);
    XtAddCallback(ih->handle, XmNcascadingCallback, (XtCallbackProc)motItemArmCallback, (XtPointer)ih);
  }
  else
  {
    int num_args = 0;
    Arg args[10];

    if (iupAttribGetBoolean(ih->parent, "RADIO"))
    {
      iupMOT_SETARG(args, num_args, XmNtoggleMode, XmTOGGLE_BOOLEAN);
      iupMOT_SETARG(args, num_args, XmNindicatorType, XmONE_OF_MANY_ROUND);
      iupMOT_SETARG(args, num_args, XmNindicatorOn, XmINDICATOR_CHECK_BOX);
      iupMOT_SETARG(args, num_args, XmNindicatorSize, 13);
      iupMOT_SETARG(args, num_args, XmNselectColor, iupmotColorGetPixel(0, 0, 0));
    }
    else
    {
      if (iupAttribGetBoolean(ih, "HIDEMARK"))
        iupMOT_SETARG(args, num_args, XmNindicatorOn, XmINDICATOR_NONE);
      else
        iupMOT_SETARG(args, num_args, XmNindicatorOn, XmINDICATOR_CHECK);
      iupMOT_SETARG(args, num_args, XmNlabelType, iupAttribGet(ih, "TITLEIMAGE")? XmPIXMAP: XmSTRING);
    }

    ih->handle = XtCreateManagedWidget(
                   iupMenuGetChildIdStr(ih),
                   xmToggleButtonWidgetClass, 
                   ih->parent->handle,
                   args,
                   num_args);

    XtAddCallback(ih->handle, XmNvalueChangedCallback, (XtCallbackProc)motItemActivateCallback, (XtPointer)ih);
    XtAddCallback(ih->handle, XmNarmCallback, (XtCallbackProc)motItemArmCallback, (XtPointer)ih);
  }

  if (!ih->handle)
    return IUP_ERROR;

  ih->serial = iupMenuGetChildId(ih); /* must be after using the string */

  XtAddCallback (ih->handle, XmNhelpCallback, (XtCallbackProc)iupmotHelpCallback, (XtPointer)ih);

  if (iupStrBoolean(IupGetGlobal("INPUTCALLBACKS")))
    XtAddEventHandler(ih->handle, PointerMotionMask, False, (XtEventHandler)iupmotDummyPointerMotionEvent, NULL);

  pos = IupGetChildPos(ih->parent, ih);
  XtVaSetValues(ih->handle, XmNpositionIndex, pos, NULL);   /* RowColumn Constraint */

  iupUpdateFontAttrib(ih);

  return IUP_NOERROR;
}

static int motSubmenuMapMethod(Ihandle* ih)
{
  int pos;

  if (!ih->parent)
    return IUP_ERROR;

  ih->handle = XtVaCreateManagedWidget(
                 iupMenuGetChildIdStr(ih),
                 xmCascadeButtonWidgetClass,
                 ih->parent->handle,
                 NULL);

  if (!ih->handle)
    return IUP_ERROR;

  ih->serial = iupMenuGetChildId(ih); /* must be after using the string */

  pos = IupGetChildPos(ih->parent, ih);
  XtVaSetValues(ih->handle, XmNpositionIndex, pos, NULL);   /* RowColumn Constraint */

  XtAddCallback(ih->handle, XmNcascadingCallback, (XtCallbackProc)motItemArmCallback, (XtPointer)ih);

  if (iupStrBoolean(IupGetGlobal("INPUTCALLBACKS")))
    XtAddEventHandler(ih->handle, PointerMotionMask, False, (XtEventHandler)iupmotDummyPointerMotionEvent, NULL);

  iupUpdateFontAttrib(ih);

  return IUP_NOERROR;
}

static int motSeparatorMapMethod(Ihandle* ih)
{
  int pos;

  if (!ih->parent)
    return IUP_ERROR;

  ih->handle = XtVaCreateManagedWidget(
                 iupMenuGetChildIdStr(ih),
                 xmSeparatorWidgetClass,
                 ih->parent->handle,
                 NULL);

  if (!ih->handle)
    return IUP_ERROR;

  ih->serial = iupMenuGetChildId(ih); /* must be after using the string */

  pos = IupGetChildPos(ih->parent, ih);
  XtVaSetValues(ih->handle, XmNpositionIndex, pos, NULL);  /* RowColumn Constraint */

  return IUP_NOERROR;
}

void iupdrvItemInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = motItemMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;

  /* Common */
  iupClassRegisterAttribute(ic, "FONT", NULL, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);  /* inherited */

  /* Visual */
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, iupBaseSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  /* IupItem only */
  iupClassRegisterAttribute(ic, "VALUE", motItemGetValueAttrib, motItemSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLE", NULL, motItemSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEIMAGE", NULL, motItemSetTitleImageAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  /* IupItem Gtk and Motif only */
  iupClassRegisterAttribute(ic, "HIDEMARK", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED);
}

void iupdrvSubmenuInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = motSubmenuMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;

  /* Common */
  iupClassRegisterAttribute(ic, "FONT", NULL, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);  /* inherited */

  /* Visual */
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, iupBaseSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  /* IupSubmenu only */
  iupClassRegisterAttribute(ic, "TITLE", NULL, motItemSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
}

void iupdrvSeparatorInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = motSeparatorMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
}
