/** \file
 * \brief Toggle Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <Xm/Xm.h>
#include <Xm/ToggleB.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_childtree.h"
#include "iup_attrib.h"
#include "iup_dialog.h"
#include "iup_str.h"
#include "iup_toggle.h"
#include "iup_drv.h"
#include "iup_image.h"
#include "iup_key.h"

#include "iupmot_drv.h"
#include "iupmot_color.h"


void iupdrvToggleAddBorders(Ihandle* ih, int *x, int *y)
{
  iupdrvButtonAddBorders(ih, x, y);
}

void iupdrvToggleAddCheckBox(Ihandle* ih, int *x, int *y, const char* str)
{
  int check_box = 15;  /* See XmNindicatorSize */
  (void)ih;

  /* has margins too */
  (*x) += 3 + check_box + 3;
  if ((*y) < 3 + check_box + 3) (*y) = 3 + check_box + 3; /* minimum height */
  else (*y) += 3+3;

  if (str && str[0]) /* add spacing between check box and text */
    (*x) += 4;
}


/*********************************************************************************/


static int motToggleSetBgColorAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_TEXT)
  {
    char* parent_value = iupAttribGetInheritNativeParent(ih, "BGCOLOR");
    if (!parent_value)
    {
      /* if not defined at a native parent, 
         then change the toggle button color to the given color instead using the default */
      if (iupdrvBaseSetBgColorAttrib(ih, value))  /* let XmChangeColor do its job */
      {
        parent_value = IupGetGlobal("DLGBGCOLOR");
        XtVaSetValues(ih->handle, XmNbackground, iupmotColorGetPixelStr(parent_value), NULL);  /* reset just the background */

        if (ih->data->is_radio)
          XtVaSetValues(ih->handle, XmNselectColor, iupmotColorGetPixel(0, 0, 0), NULL);
        XtVaSetValues(ih->handle, XmNunselectColor, iupmotColorGetPixelStr(value), NULL);
        return 1;
      }
    }
    else
    {
      /* ignore given value, must use only from parent */
      if (iupdrvBaseSetBgColorAttrib(ih, parent_value))
      {
        if (ih->data->is_radio)
          XtVaSetValues(ih->handle, XmNselectColor, iupmotColorGetPixel(0, 0, 0), NULL);
        XtVaSetValues(ih->handle, XmNunselectColor, iupmotColorGetPixelStr(parent_value), NULL);
        return 1;
      }
    }
    return 0; 
  }
  else
  {
    if (ih->data->flat)
    {
      /* ignore given value, must use only from parent */
      value = iupBaseNativeParentGetBgColor(ih);

      if (iupdrvBaseSetBgColorAttrib(ih, value))
        return 1;
    }

    return iupdrvBaseSetBgColorAttrib(ih, value); 
  }
}

static int motToggleSetBackgroundAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_TEXT)
  {
    Pixel color;

    /* ignore given value, must use only from parent */
    value = iupAttribGetInheritNativeParent(ih, "BACKGROUND");

    color = iupmotColorGetPixelStr(value);
    if (color != (Pixel)-1)
    {
      XtVaSetValues(ih->handle, XmNbackground, color, NULL);
      return 1;
    }
    else
    {
      Pixmap pixmap = (Pixmap)iupImageGetImage(value, ih, 0, NULL);
      if (pixmap)
      {
        XtVaSetValues(ih->handle, XmNbackgroundPixmap, pixmap, NULL);
        return 1;
      }
    }
  }
  else
  {
    if (ih->data->flat)
    {
      /* ignore given value, must use only from parent */
      value = iupAttribGetInheritNativeParent(ih, "BACKGROUND");

      if (iupdrvBaseSetBgColorAttrib(ih, value))
        return 1;
      else
      {
        Pixmap pixmap = (Pixmap)iupImageGetImage(value, ih, 0, NULL);
        if (pixmap)
        {
          XtVaSetValues(ih->handle, XmNbackgroundPixmap, pixmap, NULL);
          return 1;
        }
      }
    }
  }
  return 0;
}

static int motToggleSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_TEXT)
  {
    iupmotSetMnemonicTitle(ih, NULL, 0, value);
    return 1;
  }

  return 0;
}

static int motToggleSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  unsigned char align;
  char value1[30], value2[30];

  if (ih->data->type == IUP_TOGGLE_TEXT)
    return 0;

  iupStrToStrStr(value, value1, value2, ':');   /* value2 is ignored, NOT supported in Motif */

  if (iupStrEqualNoCase(value1, "ARIGHT"))
    align = XmALIGNMENT_END;
  else if (iupStrEqualNoCase(value1, "ALEFT"))
    align = XmALIGNMENT_BEGINNING;
  else /* "ACENTER" (default) */
    align = XmALIGNMENT_CENTER;

  XtVaSetValues (ih->handle, XmNalignment, align, NULL);
  return 1;
}

static int motToggleSetImageAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    iupmotSetPixmap(ih, value, XmNlabelPixmap, 0);

    if (!iupAttribGet(ih, "IMINACTIVE"))
    {
      /* if not active and IMINACTIVE is not defined 
         then automatically create one based on IMAGE */
      iupmotSetPixmap(ih, value, XmNlabelInsensitivePixmap, 1); /* make_inactive */
    }
    return 1;
  }
  else
    return 0;
}

static int motToggleSetImInactiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    iupmotSetPixmap(ih, value, XmNlabelInsensitivePixmap, 0);
    return 1;
  }
  else
    return 0;
}

static int motToggleSetImPressAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    iupmotSetPixmap(ih, value, XmNselectPixmap, 0);
    return 1;
  }
  else
    return 0;
}

static int motToggleSetValueAttrib(Ihandle* ih, const char* value)
{
  Ihandle *radio;
  unsigned char check;

  if (iupStrEqualNoCase(value,"TOGGLE"))
    check = (unsigned char)-1;
  else if (iupStrEqualNoCase(value,"NOTDEF"))
    check = XmINDETERMINATE;
  else if (iupStrBoolean(value))
    check = XmSET;
  else
    check = XmUNSET;

  /* This is necessary because Motif toggle does not have support for radio. 
     It is implemented using an external RadioBox that we do not use. */
  radio = iupRadioFindToggleParent(ih);
  if (radio)
  {
    Ihandle* last_tg;
    unsigned char oldcheck;

    if (check == (unsigned char)-1)
      check = XmSET;

    XtVaGetValues(ih->handle, XmNset, &oldcheck, NULL);

    last_tg = (Ihandle*)iupAttribGet(radio, "_IUPMOT_LASTTOGGLE");
    if (check)
    {
      if (iupObjectCheck(last_tg) && last_tg != ih)
        XtVaSetValues(last_tg->handle, XmNset, XmUNSET, NULL);
      iupAttribSet(radio, "_IUPMOT_LASTTOGGLE", (char*)ih);
    }

    if (last_tg != ih && oldcheck != check)
      XtVaSetValues(ih->handle, XmNset, check, NULL);
  }
  else
  {
    if (check == (unsigned char)-1)
    {
      unsigned char oldcheck;
      XtVaGetValues(ih->handle, XmNset, &oldcheck, NULL);

      if (oldcheck == XmSET)
        check = XmUNSET;
      else
        check = XmSET;
    }

    XtVaSetValues(ih->handle, XmNset, check, NULL);
  }

  return 0;
}

static char* motToggleGetValueAttrib(Ihandle* ih)
{
  int check;
  unsigned char set = 0;
  XtVaGetValues (ih->handle, XmNset, &set, NULL);
  check = set;
  if (check == XmINDETERMINATE) check = -1;
  return iupStrReturnChecked(check);
}

static int motToggleSetPaddingAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqual(value, "DEFAULTBUTTONPADDING"))
    value = IupGetGlobal("DEFAULTBUTTONPADDING");

  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');
  if (ih->handle && ih->data->type == IUP_TOGGLE_IMAGE)
  {
    XtVaSetValues(ih->handle, XmNmarginHeight, ih->data->vert_padding,
                              XmNmarginWidth, ih->data->horiz_padding, NULL);
    return 0;
  }
  else
    return 1; /* store until not mapped, when mapped will be set again */
}

static char* motToggleGetSelectColorAttrib(Ihandle* ih)
{
  unsigned char r, g, b;
  Pixel color;
  XtVaGetValues(ih->handle, XmNselectColor, &color, NULL); 
  iupmotColorGetRGB(color, &r, &g, &b);
  return iupStrReturnStrf("%d %d %d", (int)r, (int)g, (int)b);
}

static int motToggleSetSelectColorAttrib(Ihandle* ih, const char *value)
{
  Pixel color = iupmotColorGetPixelStr(value);
  if (color != (Pixel)-1)
    XtVaSetValues(ih->handle, XmNselectColor, color, NULL);
  return 1;
}

static void motToggleValueChangedCallback(Widget w, Ihandle* ih, XmToggleButtonCallbackStruct* call_data)
{
  Ihandle *radio;
  IFni cb;
  int check = call_data->set;

  /* Must manually hide the tip if the toggle is pressed. */
  iupmotTipLeaveNotify();

  /* This is necessary because Motif toggle does not have support for radio. 
     It is implemented using an external RadioBox that we do not use. */
  radio = iupRadioFindToggleParent(ih);
  if (radio)
  {
    if (check)
    {
      Ihandle* last_tg = (Ihandle*)iupAttribGet(radio, "_IUPMOT_LASTTOGGLE");
      if (iupObjectCheck(last_tg) && last_tg != ih)
      {
        /* uncheck last toggle */
        XtVaSetValues(last_tg->handle, XmNset, XmUNSET, NULL);

        cb = (IFni) IupGetCallback(last_tg, "ACTION");
        if (cb && cb(last_tg, 0) == IUP_CLOSE)
            IupExitLoop();

        if (iupObjectCheck(ih))
          iupBaseCallValueChangedCb(last_tg);
      }
      iupAttribSet(radio, "_IUPMOT_LASTTOGGLE", (char*)ih);

      if (last_tg != ih)
      {
        cb = (IFni)IupGetCallback(ih, "ACTION");
        if (cb && cb (ih, 1) == IUP_CLOSE)
            IupExitLoop();

        if (iupObjectCheck(ih))
          iupBaseCallValueChangedCb(ih);
      }
    }
    else
    {
      /* Force stay checked */
      XtVaSetValues(ih->handle, XmNset, XmSET, NULL);
    }
  }
  else
  {
    if (check == XmINDETERMINATE)
       check = -1;

    cb = (IFni)IupGetCallback(ih, "ACTION");
    if (cb)
    {
      if (cb(ih, check) == IUP_CLOSE) 
        IupExitLoop();
    }

    if (iupObjectCheck(ih))
      iupBaseCallValueChangedCb(ih);
  }

  (void)w;
}

static void motToggleEnterLeaveWindowEvent(Widget w, Ihandle* ih, XEvent *evt, Boolean *cont)
{
  /* Used only when FLAT=Yes */
  unsigned char check = 0;

  iupmotEnterLeaveWindowEvent(w, ih, evt, cont);

  XtVaGetValues (ih->handle, XmNset, &check, NULL);
  if (check == XmSET)
    XtVaSetValues(ih->handle, XmNshadowThickness, 2, NULL);
  else
  {
    if (evt->type == EnterNotify)
      XtVaSetValues(ih->handle, XmNshadowThickness, 2, NULL);
    else  if (evt->type == LeaveNotify)
      XtVaSetValues(ih->handle, XmNshadowThickness, 0, NULL);
  }
}

static int motToggleMapMethod(Ihandle* ih)
{
  Ihandle* radio = iupRadioFindToggleParent(ih);
  char* value;
  int num_args = 0;
  Arg args[40];

  if (radio)
    ih->data->is_radio = 1;

  value = iupAttribGet(ih, "IMAGE");
  if (value)
  {
    ih->data->type = IUP_TOGGLE_IMAGE;
    iupMOT_SETARG(args, num_args, XmNlabelType, XmPIXMAP); 
  }
  else
  {
    ih->data->type = IUP_TOGGLE_TEXT;
    iupMOT_SETARG(args, num_args, XmNlabelType, XmSTRING); 
  }

  /* Core */
  iupMOT_SETARG(args, num_args, XmNmappedWhenManaged, False);  /* not visible when managed */
  iupMOT_SETARG(args, num_args, XmNx, 0);  /* x-position */
  iupMOT_SETARG(args, num_args, XmNy, 0);  /* y-position */
  iupMOT_SETARG(args, num_args, XmNwidth, 10);  /* default width to avoid 0 */
  iupMOT_SETARG(args, num_args, XmNheight, 10); /* default height to avoid 0 */
  /* Primitive */
  if (iupAttribGetBoolean(ih, "CANFOCUS"))
    iupMOT_SETARG(args, num_args, XmNtraversalOn, True);
  else
    iupMOT_SETARG(args, num_args, XmNtraversalOn, False);
  iupMOT_SETARG(args, num_args, XmNhighlightThickness, 2);
  iupMOT_SETARG(args, num_args, XmNnavigationType, XmTAB_GROUP);
  /* Label */
  iupMOT_SETARG(args, num_args, XmNrecomputeSize, False);  /* no automatic resize from text */
  iupMOT_SETARG(args, num_args, XmNmarginHeight, 0);  /* default padding */
  iupMOT_SETARG(args, num_args, XmNmarginWidth, 0);
  iupMOT_SETARG(args, num_args, XmNmarginTop, 0);     /* no extra margins */
  iupMOT_SETARG(args, num_args, XmNmarginLeft, 0);
  iupMOT_SETARG(args, num_args, XmNmarginBottom, 0);
  iupMOT_SETARG(args, num_args, XmNmarginRight, 0);

  if (ih->data->is_radio)
  {
    iupMOT_SETARG(args, num_args, XmNtoggleMode, XmTOGGLE_BOOLEAN);
    iupMOT_SETARG(args, num_args, XmNindicatorType, XmONE_OF_MANY_ROUND);

    if (!iupAttribGet(radio, "_IUPMOT_LASTTOGGLE"))
    {
      /* this is the first toggle in the radio, and the last toggle with VALUE=ON */
      iupAttribSet(ih, "VALUE","ON");
    }

    /* make sure it has at least one name */
    if (!iupAttribGetHandleName(ih))
      iupAttribSetHandleName(ih);
  }
  else
  {
    if (ih->data->type == IUP_TOGGLE_TEXT && iupAttribGetBoolean(ih, "3STATE"))
      iupMOT_SETARG(args, num_args, XmNtoggleMode, XmTOGGLE_INDETERMINATE);
    else
      iupMOT_SETARG(args, num_args, XmNtoggleMode, XmTOGGLE_BOOLEAN);
    iupMOT_SETARG(args, num_args, XmNindicatorType, XmN_OF_MANY);
  }

  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    iupMOT_SETARG(args, num_args, XmNindicatorOn, XmINDICATOR_NONE);
    iupMOT_SETARG(args, num_args, XmNalignment, XmALIGNMENT_CENTER);
    iupMOT_SETARG(args, num_args, XmNshadowThickness, 2);
  }
  else
  {
    iupMOT_SETARG(args, num_args, XmNspacing, 3);
    iupMOT_SETARG(args, num_args, XmNindicatorOn, XmINDICATOR_CHECK_BOX);
    iupMOT_SETARG(args, num_args, XmNalignment, XmALIGNMENT_BEGINNING);

    if (ih->data->is_radio)
    {
      iupMOT_SETARG(args, num_args, XmNindicatorSize, 13);
      iupMOT_SETARG(args, num_args, XmNselectColor, iupmotColorGetPixel(0, 0, 0));
    }
    else
      iupMOT_SETARG(args, num_args, XmNindicatorSize, 15);

    iupMOT_SETARG(args, num_args, XmNshadowThickness, 0);
    iupMOT_SETARG(args, num_args, XmNdetailShadowThickness, 2);
  }

  ih->handle = XtCreateManagedWidget(
    iupDialogGetChildIdStr(ih),  /* child identifier */
    xmToggleButtonWidgetClass,     /* widget class */
    iupChildTreeGetNativeParentHandle(ih), /* widget parent */
    args, num_args);

  if (!ih->handle)
    return IUP_ERROR;

  ih->serial = iupDialogGetChildId(ih); /* must be after using the string */

  XtAddCallback(ih->handle, XmNhelpCallback, (XtCallbackProc)iupmotHelpCallback, (XtPointer)ih);

  if (ih->data->type == IUP_TOGGLE_IMAGE && ih->data->flat)
  {
    XtVaSetValues(ih->handle, XmNshadowThickness, 0, NULL);
    XtAddEventHandler(ih->handle, EnterWindowMask, False, (XtEventHandler)motToggleEnterLeaveWindowEvent, (XtPointer)ih);
    XtAddEventHandler(ih->handle, LeaveWindowMask, False, (XtEventHandler)motToggleEnterLeaveWindowEvent, (XtPointer)ih);
  }
  else
  {
    XtAddEventHandler(ih->handle, EnterWindowMask, False, (XtEventHandler)iupmotEnterLeaveWindowEvent, (XtPointer)ih);
    XtAddEventHandler(ih->handle, LeaveWindowMask, False, (XtEventHandler)iupmotEnterLeaveWindowEvent, (XtPointer)ih);
  }

  XtAddEventHandler(ih->handle, FocusChangeMask, False, (XtEventHandler)iupmotFocusChangeEvent, (XtPointer)ih);
  XtAddEventHandler(ih->handle, KeyPressMask,    False, (XtEventHandler)iupmotKeyPressEvent,    (XtPointer)ih);

  XtAddCallback(ih->handle, XmNvalueChangedCallback, (XtCallbackProc)motToggleValueChangedCallback, (XtPointer)ih);

  if (iupStrBoolean(IupGetGlobal("INPUTCALLBACKS")))
    XtAddEventHandler(ih->handle, PointerMotionMask, False, (XtEventHandler)iupmotDummyPointerMotionEvent, NULL);

  /* Disable Drag Source */
  iupmotDisableDragSource(ih->handle);

  /* initialize the widget */
  XtRealizeWidget(ih->handle);

  if (ih->data->type == IUP_TOGGLE_TEXT)
    iupmotSetXmString(ih->handle, XmNlabelString, "");

  return IUP_NOERROR;
}

void iupdrvToggleInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = motToggleMapMethod;

  /* Driver Dependent Attribute functions */

  /* Overwrite Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", iupmotGetBgColorAttrib, motToggleSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_NO_SAVE|IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BACKGROUND", NULL, motToggleSetBackgroundAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  /* Special */
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, iupdrvBaseSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TITLE", NULL, motToggleSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  /* IupToggle only */
  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, motToggleSetAlignmentAttrib, IUPAF_SAMEASSYSTEM, "ACENTER:ACENTER", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, motToggleSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, motToggleSetImInactiveAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, motToggleSetImPressAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", motToggleGetValueAttrib, motToggleSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTCOLOR", motToggleGetSelectColorAttrib, motToggleSetSelectColorAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "PADDING", iupToggleGetPaddingAttrib, motToggleSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);

  /* NOT supported */
  iupClassRegisterAttribute(ic, "RIGHTBUTTON", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "MARKUP", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_DEFAULT);
}
