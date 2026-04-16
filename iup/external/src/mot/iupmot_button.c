/** \file
 * \brief Button Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <Xm/Xm.h>
#include <Xm/PushB.h>

#include <stdio.h>
#include <memory.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_childtree.h"
#include "iup_attrib.h"
#include "iup_dialog.h"
#include "iup_str.h"
#include "iup_button.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_image.h"

#include "iupmot_drv.h"


IUP_SDK_API void iupdrvButtonAddBorders(Ihandle* ih, int *x, int *y)
{
  int border_size = 2*5;

  if (ih)
  {
    char* image = iupAttribGet(ih, "IMAGE");
    char* title = iupAttribGet(ih, "TITLE");
    if (!image && (!title || !*title) && iupAttribGet(ih, "BGCOLOR"))
    {
      int charwidth, charheight;
      iupdrvFontGetCharSize(ih, &charwidth, &charheight);
      (*x) += charheight;
    }
  }

  (*x) += border_size;
  (*y) += border_size;

  if (ih && iupAttribGetBoolean(ih, "SHOWASDEFAULT"))
  {
    int increase = 2 * 1 + 2 + 1;
    (*x) += increase * 2;
    (*y) += increase * 2;
  }
}

static int motButtonSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type & IUP_BUTTON_TEXT)
  {
    iupmotSetMnemonicTitle(ih, NULL, 0, value);
    return 1;
  }

  return 0;
}

static int motButtonSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  unsigned char align;
  char value1[30], value2[30];

  iupStrToStrStr(value, value1, sizeof(value1), value2, sizeof(value2), ':');   /* value2 is ignored, NOT supported in Motif */

  if (iupStrEqualNoCase(value1, "ARIGHT"))
    align = XmALIGNMENT_END;
  else if (iupStrEqualNoCase(value1, "ALEFT"))
    align = XmALIGNMENT_BEGINNING;
  else /* "ACENTER" (default) */
    align = XmALIGNMENT_CENTER;

  XtVaSetValues (ih->handle, XmNalignment, align, NULL);
  return 1;
}

static int motButtonSetImageAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type & IUP_BUTTON_IMAGE)
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

static int motButtonSetImInactiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    iupmotSetPixmap(ih, value, XmNlabelInsensitivePixmap, 0);
    return 1;
  }
  else
    return 0;
}

static int motButtonSetImPressAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    iupmotSetPixmap(ih, value, XmNarmPixmap, 0);
    return 1;
  }
  else
    return 0;
}

static int motButtonSetPaddingAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqual(value, "DEFAULTBUTTONPADDING"))
    value = IupGetGlobal("DEFAULTBUTTONPADDING");

  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');
  if (ih->handle)
  {
    XtVaSetValues(ih->handle, XmNmarginHeight, ih->data->vert_padding,
                              XmNmarginWidth, ih->data->horiz_padding, NULL);
    return 0;
  }
  else
    return 1; /* store until not mapped, when mapped will be set again */
}

static int motButtonSetBgColorAttrib(Ihandle* ih, const char* value)
{
  if (iupAttribGet(ih, "IMPRESS") || iupAttribGetBoolean(ih, "FLAT"))
  {
    /* ignore given value, must use only from parent */
    value = iupBaseNativeParentGetBgColor(ih);

    if (iupdrvBaseSetBgColorAttrib(ih, value))
      return 1;
  }

  return iupdrvBaseSetBgColorAttrib(ih, value);
}

static int motButtonSetBackgroundAttrib(Ihandle* ih, const char* value)
{
  if (iupAttribGet(ih, "IMPRESS") || iupAttribGetBoolean(ih, "FLAT"))
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
  return 0;
}

static void motButtonActivateCallback(Widget w, Ihandle* ih, XtPointer call_data)
{
  Icallback cb;

  /* Must manually hide the tip if the button is pressed. */
  iupmotTipLeaveNotify();

  cb = IupGetCallback(ih, "ACTION");
  if (cb)
  {
    if (cb(ih) == IUP_CLOSE)
      IupExitLoop();
  }
  (void)w;
  (void)call_data;
}

static void motButtonKeyPressEvent(Widget w, Ihandle* ih, XEvent *evt, Boolean *cont)
{
  int code = iupmotKeyDecode((XKeyEvent*)evt);
  if (code == K_CR)
  {
    iupdrvActivate(ih);
    *cont = False;
    return;
  }

  iupmotKeyPressEvent(w, ih, evt, cont);
}

static void motButtonEnterLeaveWindowEvent(Widget w, Ihandle* ih, XEvent *evt, Boolean *cont)
{
  /* Used only when FLAT=Yes, to manage relief */

  iupmotEnterLeaveWindowEvent(w, ih, evt, cont);

  if (evt->type == EnterNotify)
    XtVaSetValues(ih->handle, XmNshadowThickness, 2, NULL);
  else  if (evt->type == LeaveNotify)
    XtVaSetValues(ih->handle, XmNshadowThickness, 0, NULL);
}

static int motButtonMapMethod(Ihandle* ih)
{
  int has_border = 1;
  char* value;
  char* title;
  int num_args = 0;
  Arg args[30];

  value = iupAttribGet(ih, "IMAGE");
  title = iupAttribGet(ih, "TITLE");

  if (value)
  {
    ih->data->type = IUP_BUTTON_IMAGE;

    /* Check if we have both image and text */
    if (title && *title != 0)
    {
      ih->data->type |= IUP_BUTTON_TEXT;  /* IUP_BUTTON_BOTH */
      iupMOT_SETARG(args, num_args, XmNlabelType, XmPIXMAP_AND_STRING);
    }
    else
    {
      iupMOT_SETARG(args, num_args, XmNlabelType, XmPIXMAP);
    }
  }
  else
  {
    ih->data->type = IUP_BUTTON_TEXT;
    iupMOT_SETARG(args, num_args, XmNlabelType, XmSTRING);
  }

  if (ih->data->type == IUP_BUTTON_IMAGE &&
      iupAttribGet(ih, "IMPRESS") &&
      !iupAttribGetBoolean(ih, "IMPRESSBORDER"))
    has_border = 0;

  /* Core */
  iupMOT_SETARG(args, num_args, XmNmappedWhenManaged, False);  /* not visible when managed */
  iupMOT_SETARG(args, num_args, XmNx, 0);  /* x-position */
  iupMOT_SETARG(args, num_args, XmNy, 0);  /* y-position */
  iupMOT_SETARG(args, num_args, XmNwidth, 10);  /* default width to avoid 0 */
  iupMOT_SETARG(args, num_args, XmNheight, 10); /* default height to avoid 0 */
  /* Label */
  iupMOT_SETARG(args, num_args, XmNrecomputeSize, False);  /* no automatic resize from text */
  iupMOT_SETARG(args, num_args, XmNmarginHeight, 0);  /* default padding */
  iupMOT_SETARG(args, num_args, XmNmarginWidth, 0);
  iupMOT_SETARG(args, num_args, XmNmarginTop, 0);     /* no extra margins */
  iupMOT_SETARG(args, num_args, XmNmarginLeft, 0);
  iupMOT_SETARG(args, num_args, XmNmarginBottom, 0);
  iupMOT_SETARG(args, num_args, XmNmarginRight, 0);
  /* PushButton */
  iupMOT_SETARG(args, num_args, XmNfillOnArm, False);

  /* Primitive */
  if (iupAttribGetBoolean(ih, "CANFOCUS"))
    iupMOT_SETARG(args, num_args, XmNtraversalOn, True);
  else
    iupMOT_SETARG(args, num_args, XmNtraversalOn, False);
  iupMOT_SETARG(args, num_args, XmNhighlightThickness, 2);
  iupMOT_SETARG(args, num_args, XmNnavigationType, XmTAB_GROUP);

  ih->handle = XtCreateManagedWidget(
    iupDialogGetChildIdStr(ih),  /* child identifier */
    xmPushButtonWidgetClass,     /* widget class */
    iupChildTreeGetNativeParentHandle(ih), /* widget parent */
    args, num_args);

  if (!ih->handle)
    return IUP_ERROR;

  ih->serial = iupDialogGetChildId(ih); /* must be after using the string */

  XtAddCallback(ih->handle, XmNhelpCallback, (XtCallbackProc)iupmotHelpCallback, (XtPointer)ih);

  if (has_border && iupAttribGetBoolean(ih, "FLAT"))
  {
    XtVaSetValues(ih->handle, XmNshadowThickness, 0, NULL);

    XtAddEventHandler(ih->handle, EnterWindowMask, False, (XtEventHandler)motButtonEnterLeaveWindowEvent, (XtPointer)ih);
    XtAddEventHandler(ih->handle, LeaveWindowMask, False, (XtEventHandler)motButtonEnterLeaveWindowEvent, (XtPointer)ih);
  }
  else
  {
    if (!has_border)
    {
      /* In Motif the button will lose its focus feedback also */
      XtVaSetValues(ih->handle, XmNhighlightThickness, 0, NULL);
      XtVaSetValues(ih->handle, XmNshadowThickness, 0, NULL);
    }
    else
      XtVaSetValues(ih->handle, XmNshadowThickness, 2, NULL);

    XtAddEventHandler(ih->handle, EnterWindowMask, False, (XtEventHandler)iupmotEnterLeaveWindowEvent, (XtPointer)ih);
    XtAddEventHandler(ih->handle, LeaveWindowMask, False, (XtEventHandler)iupmotEnterLeaveWindowEvent, (XtPointer)ih);
  }

  XtAddEventHandler(ih->handle, FocusChangeMask, False, (XtEventHandler)iupmotFocusChangeEvent, (XtPointer)ih);
  XtAddEventHandler(ih->handle, KeyPressMask,    False, (XtEventHandler)motButtonKeyPressEvent, (XtPointer)ih);

  XtAddCallback(ih->handle, XmNactivateCallback, (XtCallbackProc)motButtonActivateCallback, (XtPointer)ih);
  XtAddEventHandler(ih->handle, ButtonPressMask|ButtonReleaseMask, False, (XtEventHandler)iupmotButtonPressReleaseEvent, (XtPointer)ih);

  if (iupStrBoolean(IupGetGlobal("INPUTCALLBACKS")))
    XtAddEventHandler(ih->handle, PointerMotionMask, False, (XtEventHandler)iupmotDummyPointerMotionEvent, NULL);

  /* Disable Drag Source */
  iupmotDisableDragSource(ih->handle);

  /* initialize the widget */
  XtRealizeWidget(ih->handle);

  if (ih->data->type & IUP_BUTTON_TEXT)
    iupmotSetXmString(ih->handle, XmNlabelString, "");

  return IUP_NOERROR;
}

static int motButtonSetShowAsDefaultAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)
    return 1;
  XtVaSetValues(ih->handle, XmNshowAsDefault, iupStrBoolean(value) ? 1 : 0, NULL);
  return 1;
}

IUP_SDK_API void iupdrvButtonInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = motButtonMapMethod;

  /* Driver Dependent Attribute functions */

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", iupmotGetBgColorAttrib, motButtonSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_NO_SAVE);
  iupClassRegisterAttribute(ic, "BACKGROUND", NULL, motButtonSetBackgroundAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  /* Special */
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, iupdrvBaseSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TITLE", NULL, motButtonSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  /* IupButton only */
  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, motButtonSetAlignmentAttrib, "ACENTER:ACENTER", NULL, IUPAF_NO_INHERIT);  /* force new default value */
  iupClassRegisterAttribute(ic, "IMAGE", NULL, motButtonSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, motButtonSetImInactiveAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, motButtonSetImPressAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "PADDING", iupButtonGetPaddingAttrib, motButtonSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "SHOWASDEFAULT", NULL, motButtonSetShowAsDefaultAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  /* NOT supported */
  iupClassRegisterAttribute(ic, "MARKUP", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED);
  iupClassRegisterAttribute(ic, "IMAGEPOSITION", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPACING", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}
