/** \file
 * \brief Scrollbar Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <Xm/Xm.h>
#include <Xm/ScrollBar.h>
#include <X11/keysym.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <memory.h>
#include <stdarg.h>
#include <limits.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_childtree.h"
#include "iup_attrib.h"
#include "iup_dialog.h"
#include "iup_str.h"
#include "iup_scrollbar.h"
#include "iup_drv.h"

#include "iupmot_drv.h"
#include "iupmot_color.h"


#define IMOT_SB_MAX 32767

void iupdrvScrollbarGetMinSize(Ihandle* ih, int *w, int *h)
{
  int sb_size = 20;

  if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
  {
    *w = 3 * sb_size;
    *h = sb_size;
  }
  else
  {
    *w = sb_size;
    *h = 3 * sb_size;
  }
}

static void motScrollbarUpdateNative(Ihandle* ih)
{
  double range = ih->data->vmax - ih->data->vmin;
  int ipage, ipos, istep, ipagestep;

  if (range <= 0)
    return;

  ipage = (int)((ih->data->pagesize / range) * IMOT_SB_MAX);
  if (ipage < 1) ipage = 1;
  if (ipage > IMOT_SB_MAX) ipage = IMOT_SB_MAX;

  ipos = (int)(((ih->data->val - ih->data->vmin) / range) * IMOT_SB_MAX);
  if (ipos < 0) ipos = 0;
  if (ipos > IMOT_SB_MAX - ipage) ipos = IMOT_SB_MAX - ipage;

  istep = (int)((ih->data->linestep / range) * IMOT_SB_MAX);
  if (istep < 1) istep = 1;

  ipagestep = (int)((ih->data->pagestep / range) * IMOT_SB_MAX);
  if (ipagestep < 1) ipagestep = 1;

  XtVaSetValues(ih->handle,
    XmNminimum, 0,
    XmNmaximum, IMOT_SB_MAX,
    XmNvalue, ipos,
    XmNsliderSize, ipage,
    XmNincrement, istep,
    XmNpageIncrement, ipagestep,
    NULL);
}

static int motScrollbarSetValueAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDouble(value, &(ih->data->val)))
  {
    iupScrollbarCropValue(ih);
    motScrollbarUpdateNative(ih);
  }
  return 0;
}

static int motScrollbarSetLineStepAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->linestep), 0.01))
    motScrollbarUpdateNative(ih);
  return 0;
}

static int motScrollbarSetPageStepAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->pagestep), 0.1))
    motScrollbarUpdateNative(ih);
  return 0;
}

static int motScrollbarSetPageSizeAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->pagesize), 0.1))
  {
    iupScrollbarCropValue(ih);
    motScrollbarUpdateNative(ih);
  }
  return 0;
}


/*********************************************************************************************/


static void motScrollbarCallback(Widget w, XtPointer client_data, XtPointer call_data)
{
  int op = (int)(intptr_t)client_data;
  Ihandle *ih;
  int ipos, ipage;
  double range, old_val;

  XtVaGetValues(w,
    XmNvalue, &ipos,
    XmNsliderSize, &ipage,
    XmNuserData, &ih,
    NULL);

  if (!ih) return;

  old_val = ih->data->val;
  range = ih->data->vmax - ih->data->vmin;

  ih->data->val = ((double)ipos / (double)IMOT_SB_MAX) * range + ih->data->vmin;
  iupScrollbarCropValue(ih);

  {
    IFniff scroll_cb = (IFniff)IupGetCallback(ih, "SCROLL_CB");
    if (scroll_cb)
    {
      float posx = 0, posy = 0;
      if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
        posx = (float)ih->data->val;
      else
        posy = (float)ih->data->val;

      scroll_cb(ih, op, posx, posy);
    }
  }

  {
    IFn valuechanged_cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
    if (valuechanged_cb)
    {
      if (ih->data->val != old_val)
        valuechanged_cb(ih);
    }
  }

  (void)call_data;
}


/*********************************************************************************************/


static int motScrollbarMapMethod(Ihandle* ih)
{
  int num_args = 0;
  Arg args[30];

  iupMOT_SETARG(args, num_args, XmNmappedWhenManaged, False);
  iupMOT_SETARG(args, num_args, XmNx, 0);
  iupMOT_SETARG(args, num_args, XmNy, 0);
  iupMOT_SETARG(args, num_args, XmNwidth, 10);
  iupMOT_SETARG(args, num_args, XmNheight, 10);

  if (iupAttribGetBoolean(ih, "CANFOCUS"))
    iupMOT_SETARG(args, num_args, XmNtraversalOn, True);
  else
    iupMOT_SETARG(args, num_args, XmNtraversalOn, False);

  iupMOT_SETARG(args, num_args, XmNhighlightThickness, 2);
  iupMOT_SETARG(args, num_args, XmNnavigationType, XmTAB_GROUP);
  iupMOT_SETARG(args, num_args, XmNminimum, 0);
  iupMOT_SETARG(args, num_args, XmNmaximum, IMOT_SB_MAX);

  if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
  {
    iupMOT_SETARG(args, num_args, XmNorientation, XmHORIZONTAL);
    if (ih->data->inverted)
      iupMOT_SETARG(args, num_args, XmNprocessingDirection, XmMAX_ON_LEFT);
    else
      iupMOT_SETARG(args, num_args, XmNprocessingDirection, XmMAX_ON_RIGHT);
  }
  else
  {
    iupMOT_SETARG(args, num_args, XmNorientation, XmVERTICAL);
    if (ih->data->inverted)
      iupMOT_SETARG(args, num_args, XmNprocessingDirection, XmMAX_ON_TOP);
    else
      iupMOT_SETARG(args, num_args, XmNprocessingDirection, XmMAX_ON_BOTTOM);
  }

  ih->handle = XtCreateManagedWidget(
    iupDialogGetChildIdStr(ih),
    xmScrollBarWidgetClass,
    iupChildTreeGetNativeParentHandle(ih),
    args, num_args);

  if (!ih->handle)
    return IUP_ERROR;

  XtVaSetValues(ih->handle, XmNuserData, ih, NULL);

  XtAddEventHandler(ih->handle, EnterWindowMask, False, (XtEventHandler)iupmotEnterLeaveWindowEvent, (XtPointer)ih);
  XtAddEventHandler(ih->handle, LeaveWindowMask, False, (XtEventHandler)iupmotEnterLeaveWindowEvent, (XtPointer)ih);
  XtAddEventHandler(ih->handle, FocusChangeMask, False, (XtEventHandler)iupmotFocusChangeEvent, (XtPointer)ih);
  XtAddEventHandler(ih->handle, KeyPressMask, False, (XtEventHandler)iupmotKeyPressEvent, (XtPointer)ih);

  if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
  {
    XtAddCallback(ih->handle, XmNvalueChangedCallback, (XtCallbackProc)motScrollbarCallback, (XtPointer)(intptr_t)IUP_SBPOSH);
    XtAddCallback(ih->handle, XmNdragCallback, (XtCallbackProc)motScrollbarCallback, (XtPointer)(intptr_t)IUP_SBDRAGH);
    XtAddCallback(ih->handle, XmNdecrementCallback, (XtCallbackProc)motScrollbarCallback, (XtPointer)(intptr_t)IUP_SBLEFT);
    XtAddCallback(ih->handle, XmNincrementCallback, (XtCallbackProc)motScrollbarCallback, (XtPointer)(intptr_t)IUP_SBRIGHT);
    XtAddCallback(ih->handle, XmNpageDecrementCallback, (XtCallbackProc)motScrollbarCallback, (XtPointer)(intptr_t)IUP_SBPGLEFT);
    XtAddCallback(ih->handle, XmNpageIncrementCallback, (XtCallbackProc)motScrollbarCallback, (XtPointer)(intptr_t)IUP_SBPGRIGHT);
  }
  else
  {
    XtAddCallback(ih->handle, XmNvalueChangedCallback, (XtCallbackProc)motScrollbarCallback, (XtPointer)(intptr_t)IUP_SBPOSV);
    XtAddCallback(ih->handle, XmNdragCallback, (XtCallbackProc)motScrollbarCallback, (XtPointer)(intptr_t)IUP_SBDRAGV);
    XtAddCallback(ih->handle, XmNdecrementCallback, (XtCallbackProc)motScrollbarCallback, (XtPointer)(intptr_t)IUP_SBUP);
    XtAddCallback(ih->handle, XmNincrementCallback, (XtCallbackProc)motScrollbarCallback, (XtPointer)(intptr_t)IUP_SBDN);
    XtAddCallback(ih->handle, XmNpageDecrementCallback, (XtCallbackProc)motScrollbarCallback, (XtPointer)(intptr_t)IUP_SBPGUP);
    XtAddCallback(ih->handle, XmNpageIncrementCallback, (XtCallbackProc)motScrollbarCallback, (XtPointer)(intptr_t)IUP_SBPGDN);
  }

  XtAddCallback(ih->handle, XmNhelpCallback, (XtCallbackProc)iupmotHelpCallback, (XtPointer)ih);

  ih->serial = iupDialogGetChildId(ih);

  motScrollbarUpdateNative(ih);

  XtRealizeWidget(ih->handle);

  return IUP_NOERROR;
}

void iupdrvScrollbarInitClass(Iclass* ic)
{
  ic->Map = motScrollbarMapMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "VALUE", iupScrollbarGetValueAttrib, motScrollbarSetValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINESTEP", iupScrollbarGetLineStepAttrib, motScrollbarSetLineStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESTEP", iupScrollbarGetPageStepAttrib, motScrollbarSetPageStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESIZE", iupScrollbarGetPageSizeAttrib, motScrollbarSetPageSizeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
}
