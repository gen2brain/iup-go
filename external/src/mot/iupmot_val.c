/** \file
 * \brief Valuator Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <Xm/Xm.h>
#include <Xm/Scale.h>
#include <Xm/SeparatoG.h>
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
#include "iup_val.h"
#include "iup_image.h"
#include "iup_drv.h"

#include "iupmot_drv.h"
#include "iupmot_color.h"


void iupdrvValGetMinSize(Ihandle* ih, int *w, int *h)
{
  int ticks_size = 0;
  if (iupAttribGetInt(ih, "SHOWTICKS"))
    ticks_size = 8;

  if (ih->data->orientation == IVAL_HORIZONTAL)
  {
    *w = 30;
    *h = 20+ticks_size;
  }
  else
  {
    *w = 20+ticks_size;
    *h = 30;
  }
}

static void motValRemoveOldTicks(Widget scale)
{
  WidgetList children = (WidgetList)0;
  Cardinal num_children = (Cardinal)0;
  Cardinal i;
  String name;

  XtVaGetValues(scale, XmNchildren, &children, 
                       XmNnumChildren, &num_children, 
                       NULL);

  for (i = 0; i < num_children; i++) 
  {
    if (XmIsSeparatorGadget(children[i])) 
    {
      if ((name = XtName(children[i])) != (String)0) 
      {
        if (iupStrEqual(name, "BigTic") ||
            iupStrEqual(name, "MedTic") ||
            iupStrEqual(name, "SmallTic")) 
        {
          XtDestroyWidget(children[i]);
        }
      }
    }
  }
}

static int motValSetShowTicksAttrib(Ihandle* ih, const char* value)
{
  int tick_freq, show_ticks;

  if (!ih->data->show_ticks)  /* can only set if already not zero */
    return 0;

  show_ticks = atoi(value);
  if (show_ticks<2) show_ticks=2;
  ih->data->show_ticks = show_ticks;

  motValRemoveOldTicks(ih->handle);

  /* Defines the interval frequency for tick marks */
  tick_freq = SHRT_MAX/(show_ticks-1);
  XmScaleSetTicks(ih->handle, tick_freq, 0, 0, 8, 0, 0);
  return 0;
}

static int motValSetPageStepAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->pagestep), 0.1))
  {
    int pagesize = (int)(ih->data->pagestep*SHRT_MAX);
    XtVaSetValues(ih->handle, XmNscaleMultiple, pagesize, NULL);
  }
  return 0; /* do not store value in hash table */
}

static int motValSetStepAttrib(Ihandle* ih, const char* value)
{
  iupStrToDoubleDef(value, &(ih->data->step), 0.01);
  return 0; /* do not store value in hash table */
}

static int motValSetValueAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDouble(value, &(ih->data->val)))
  {
    int ival;
    iupValCropValue(ih);
    ival = (int)(((ih->data->val - ih->data->vmin) / (ih->data->vmax - ih->data->vmin))*SHRT_MAX);
    XtVaSetValues(ih->handle, XmNvalue, ival, NULL);
  }

  return 0; /* do not store value in hash table */
}

static int motValSetBgColorAttrib(Ihandle* ih, const char* value)
{
  Pixel color;
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  r = (r*8)/10;
  g = (g*8)/10;
  b = (b*8)/10;

  color = iupmotColorGetPixel(r, g, b);
  if (color != (Pixel)-1)
  {
    Widget sb = XtNameToWidget(ih->handle, "*Scrollbar");
    if (sb)
      XtVaSetValues(sb, XmNtroughColor, color, NULL);
  }

  return iupdrvBaseSetBgColorAttrib(ih, value);
}

static int motValSetBackgroundAttrib(Ihandle* ih, const char* value)
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
  return 0; 
}


/*********************************************************************************************/


static void motValCallAction(Ihandle* ih, int ival, int cb_state)
{
  double old_val = ih->data->val;
  IFn cb;

  ih->data->val = (((double)ival/(double)SHRT_MAX)*(ih->data->vmax - ih->data->vmin)) + ih->data->vmin;
  iupValCropValue(ih);

  cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
  if (cb)
  {
    if (ih->data->val == old_val)
      return;

    cb(ih);
  }
  else
  {
    IFnd cb_old;
    if (cb_state == 0)
      cb_old = (IFnd) IupGetCallback(ih, "MOUSEMOVE_CB");
    else if (cb_state == -1)
      cb_old = (IFnd) IupGetCallback(ih, "BUTTON_RELEASE_CB");
    else 
      cb_old = (IFnd) IupGetCallback(ih, "BUTTON_PRESS_CB");

    if (cb_old)
      cb_old(ih, ih->data->val);
  }
}

static void motValIncPageValue(Ihandle *ih, int dir)
{
  int pagesize, ival;
  pagesize = (int)(ih->data->pagestep*SHRT_MAX);

  if (ih->data->inverted)
    dir *= -1;

  XtVaGetValues(ih->handle, XmNvalue, &ival, NULL);
  ival += dir*pagesize;
  if (ival < 0) ival = 0;
  if (ival > SHRT_MAX) ival = SHRT_MAX;
  XtVaSetValues(ih->handle, XmNvalue, ival, NULL);

  motValCallAction(ih, ival, 1);
}

static void motValIncLineValue(Ihandle *ih, int dir)
{
  int linesize, ival;
  linesize = (int)(ih->data->step*SHRT_MAX);

  if (ih->data->inverted)
    dir *= -1;

  XtVaGetValues(ih->handle, XmNvalue, &ival, NULL);
  ival += dir*linesize;
  if (ival < 0) ival = 0;
  if (ival > SHRT_MAX) ival = SHRT_MAX;
  XtVaSetValues(ih->handle, XmNvalue, ival, NULL);

  motValCallAction(ih, ival, 1);
}

static void motValKeyPressEvent(Widget w, Ihandle *ih, XKeyEvent *evt, Boolean *cont)
{
  KeySym motcode;

  *cont = True;
  iupmotKeyPressEvent(w, ih, (XEvent*)evt, cont);
  if (*cont == False)
    return;

  motcode = iupmotKeycodeToKeysym(evt);

  /* add missing support for left/right in vertical
    and up/down in horizontal */
  if (motcode == XK_Left || motcode == XK_Up)
  {
    motValIncLineValue(ih, -1);
    *cont = False;
    return;
  }
  if (motcode == XK_Right || motcode == XK_Down)
  {
    motValIncLineValue(ih, 1);
    *cont = False;
    return;
  }
  if (motcode == XK_Prior)
  {
    motValIncPageValue(ih, -1);
    *cont = False;
    return;
  }
  if (motcode == XK_Next)
  {
    motValIncPageValue(ih, 1);
    *cont = False;
    return;
  }

  /* change Home and End default behaviour */
  if (ih->data->inverted)
  {
    if (motcode==XK_Home)
    {
      int ival = SHRT_MAX;  /* set to maximum */
      XtVaSetValues(ih->handle, XmNvalue, ival, NULL);
      motValCallAction(ih, ival, 1);
      *cont = False;
      return;
    }
    if (motcode==XK_End)
    {
      int ival = 0; /* set to minimum */
      XtVaSetValues(ih->handle, XmNvalue, ival, NULL);
      motValCallAction(ih, ival, 1);
      *cont = False;
      return;
    }
  }
  else
  {
    if (motcode==XK_Home)
    {
      int ival = 0; /* set to minimum */
      XtVaSetValues(ih->handle, XmNvalue, ival, NULL);
      motValCallAction(ih, ival, 1);
      *cont = False;
      return;
    }
    if (motcode==XK_End)
    {
      int ival = SHRT_MAX;  /* set to maximum */
      XtVaSetValues(ih->handle, XmNvalue, ival, NULL);
      motValCallAction(ih, ival, 1);
      *cont = False;
      return;
    }
  }
}

static void motValValueChangedCallback(Widget w, Ihandle* ih, XmScaleCallbackStruct *cbs)
{
  int cb_state = 1;

  if(cbs->reason == XmCR_DRAG)
    cb_state = 0;
  else if (cbs->event && (cbs->event->type==ButtonRelease || cbs->event->type==KeyRelease))
   cb_state = -1;

  motValCallAction(ih, cbs->value, cb_state);

  (void)w;  
}

static void motValButtonPressReleaseEvent(Widget w, Ihandle* ih, XButtonEvent* evt, Boolean* cont)
{
  (void)w;
  (void)cont;

  /* When Button1 is pressed, the Scrollbar loses its focus to the scale,
     So we avoid calling GETFOCUS/KILLFOCUS. 
  */

  if (evt->type==ButtonPress && evt->button==Button1)
  {
    iupAttribSet(ih, "_IUPVAL_IGNOREFOCUS", "1");
  }
  if (evt->type==ButtonRelease && evt->button==Button1)
  {
    iupAttribSet(ih, "_IUPVAL_IGNOREFOCUS", NULL);
    iupAttribSet(ih, "_IUPVAL_IGNOREKILLFOCUS", "1");
  }
}

static void motValFocusChangeEvent(Widget w, Ihandle *ih, XEvent *evt, Boolean *cont)
{
  if (iupAttribGet(ih, "_IUPVAL_IGNOREFOCUS"))
    return;

  if (evt->type == FocusOut && iupAttribGet(ih, "_IUPVAL_IGNOREKILLFOCUS"))
  {
    iupAttribSet(ih, "_IUPVAL_IGNOREKILLFOCUS", NULL);
    return;
  }

  iupmotFocusChangeEvent(w, ih, evt, cont);
}


/*********************************************************************************************/


static int motValMapMethod(Ihandle* ih)
{
  int num_args = 0;
  Arg args[30];
  int show_ticks;

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
  /* Scale */
  iupMOT_SETARG(args, num_args, XmNminimum, 0);
  iupMOT_SETARG(args, num_args, XmNmaximum, SHRT_MAX);
  iupMOT_SETARG(args, num_args, XmNslidingMode, XmSLIDER);
  iupMOT_SETARG(args, num_args, XmNsliderMark, XmETCHED_LINE);
  iupMOT_SETARG(args, num_args, XmNsliderSize, 16);
  iupMOT_SETARG(args, num_args, XmNshowValue, XmNONE);

  if (ih->data->orientation == IVAL_HORIZONTAL)
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
    iupDialogGetChildIdStr(ih),  /* child identifier */
    xmScaleWidgetClass,          /* widget class */
    iupChildTreeGetNativeParentHandle(ih), /* widget parent */
    args, num_args);

  if (!ih->handle)
    return IUP_ERROR;

  /* callbacks */
  XtAddEventHandler(ih->handle, EnterWindowMask, False, (XtEventHandler)iupmotEnterLeaveWindowEvent, (XtPointer)ih);
  XtAddEventHandler(ih->handle, LeaveWindowMask, False, (XtEventHandler)iupmotEnterLeaveWindowEvent, (XtPointer)ih);

  /* XmScale changes the default behavior of these, must set directly into the scrollbar                           */
  /* XtAddEventHandler(ih->handle, FocusChangeMask, False, (XtEventHandler)iupmotFocusChangeEvent, (XtPointer)ih); */
  /* XtAddEventHandler(ih->handle, KeyPressMask,    False, (XtEventHandler)iupmotKeyPressEvent, (XtPointer)ih);    */

  XtAddCallback(ih->handle, XmNdragCallback, (XtCallbackProc)motValValueChangedCallback, (XtPointer)ih);
  XtAddCallback(ih->handle, XmNvalueChangedCallback, (XtCallbackProc)motValValueChangedCallback, (XtPointer)ih);
  XtAddCallback(ih->handle, XmNhelpCallback, (XtCallbackProc)iupmotHelpCallback, (XtPointer)ih);

  {
    Widget sb = XtNameToWidget(ih->handle, "*Scrollbar");  /* TODO: Test this in other Motifs */
    if (sb)
    {
      XtAddEventHandler(sb, FocusChangeMask, False, (XtEventHandler)motValFocusChangeEvent, (XtPointer)ih);
      XtAddEventHandler(sb, KeyPressMask,    False, (XtEventHandler)motValKeyPressEvent, (XtPointer)ih);
      XtAddEventHandler(sb, ButtonPressMask|ButtonReleaseMask, False, (XtEventHandler)motValButtonPressReleaseEvent, (XtPointer)ih);
    }
  }

  /* Ticks can only be created before XtRealizeWidget */
  show_ticks = iupAttribGetInt(ih, "SHOWTICKS");
  if (show_ticks)
  {
    if (show_ticks<2) show_ticks=2;
    ih->data->show_ticks = show_ticks; /* non zero value, can be changed later, but not to zero */
  }

  ih->serial = iupDialogGetChildId(ih); /* must be after using the string */

  /* initialize the widget */
  XtRealizeWidget(ih->handle);

  return IUP_NOERROR;
}

void iupdrvValInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = motValMapMethod;

  /* Driver Dependent Attribute functions */

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, motValSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BACKGROUND", NULL, motValSetBackgroundAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  /* IupVal only */
  iupClassRegisterAttribute(ic, "VALUE", iupValGetValueAttrib, motValSetValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);  
  iupClassRegisterAttribute(ic, "PAGESTEP", iupValGetPageStepAttrib, motValSetPageStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWTICKS", iupValGetShowTicksAttrib, motValSetShowTicksAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "STEP", iupValGetStepAttrib, motValSetStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  /* NOT supported */
  iupClassRegisterAttribute(ic, "TICKSPOS", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NOT_MAPPED);
}
