/** \file
* \brief Progress bar Control
*
* See Copyright Notice in "iup.h"
*/

#include <Xm/Xm.h>
#include <Xm/Scale.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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
#include "iup_progressbar.h"
#include "iup_drv.h"

#include "iupmot_drv.h"
#include "iupmot_color.h"


static int motProgressBarTimeCb(Ihandle* timer)
{
  Ihandle* ih = (Ihandle*)iupAttribGet(timer, "_IUP_PROGRESSBAR");
  int value;
  XtVaGetValues(ih->handle, XmNvalue, &value, NULL);
  value += SHRT_MAX/50;
  if (value > SHRT_MAX)
  {
    unsigned char dir;
    value = 0;
    XtVaGetValues(ih->handle, XmNprocessingDirection, &dir, NULL);
    if (dir == XmMAX_ON_RIGHT)
      XtVaSetValues(ih->handle, XmNprocessingDirection, XmMAX_ON_LEFT, NULL);
    else
      XtVaSetValues(ih->handle, XmNprocessingDirection, XmMAX_ON_RIGHT, NULL);
  }
  XtVaSetValues(ih->handle, XmNvalue, value, NULL);
  return IUP_DEFAULT;
}

static int motProgressBarSetMarqueeAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->marquee)
    return 0;

  if (iupStrBoolean(value))
    IupSetAttribute(ih->data->timer, "RUN", "YES");
  else
    IupSetAttribute(ih->data->timer, "RUN", "NO");

  return 1;
}

static int motProgressBarSetValueAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->marquee)
    return 0;

  if (!value)
    ih->data->value = 0;
  else
    iupStrToDouble(value, &(ih->data->value));

  iProgressBarCropValue(ih);

  XtVaSetValues(ih->handle, XmNvalue, (int)(SHRT_MAX * (ih->data->value - ih->data->vmin) / (ih->data->vmax - ih->data->vmin)), NULL);

  return 0;
}

static int motProgressBarSetBgColorAttrib(Ihandle* ih, const char* value)
{
  if (!iupAttribGetStr(ih, "FGCOLOR"))
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
      Widget w = XtNameToWidget(ih->handle, "*Scrollbar");
      XtVaSetValues(w, XmNtroughColor, color, NULL);
    }
  }

  return iupdrvBaseSetBgColorAttrib(ih, value);
}


static int motProgressBarSetFgColorAttrib(Ihandle* ih, const char* value)
{
  Pixel color = iupmotColorGetPixelStr(value);
  if (color != (Pixel)-1)
  {
    Widget w = XtNameToWidget(ih->handle, "*Scrollbar");
    XtVaSetValues(w, XmNtroughColor, color, NULL);
  }

  return 1;
}

static void motProgressBarLayoutUpdateMethod(Ihandle *ih)
{
  unsigned char orientation;
  XtVaGetValues(ih->handle, XmNorientation, &orientation, NULL);

  if (orientation == XmVERTICAL)
    XtVaSetValues(ih->handle, XmNscaleWidth, ih->currentwidth, NULL);
  else
    XtVaSetValues(ih->handle, XmNscaleHeight, ih->currentheight, NULL);

  iupdrvBaseLayoutUpdateMethod(ih);
}

static int motProgressBarMapMethod(Ihandle* ih)
{
  int num_args = 0;
  Arg args[30];
 
  /* Core */
  iupMOT_SETARG(args, num_args, XmNmappedWhenManaged, False);  /* not visible when managed */
  iupMOT_SETARG(args, num_args, XmNx, 0);  /* x-position */
  iupMOT_SETARG(args, num_args, XmNy, 0);  /* y-position */
  iupMOT_SETARG(args, num_args, XmNwidth, 10);  /* default width to avoid 0 */
  iupMOT_SETARG(args, num_args, XmNheight, 10); /* default height to avoid 0 */
  /* Primitive */
  iupMOT_SETARG(args, num_args, XmNtraversalOn, False);
  iupMOT_SETARG(args, num_args, XmNhighlightThickness, 0);
  /* Scale */
  iupMOT_SETARG(args, num_args, XmNminimum,   0);
  iupMOT_SETARG(args, num_args, XmNmaximum, SHRT_MAX);
  iupMOT_SETARG(args, num_args, XmNsliderMark, XmNONE);
  iupMOT_SETARG(args, num_args, XmNsliderVisual, XmTROUGH_COLOR);
  iupMOT_SETARG(args, num_args, XmNeditable, False);
  iupMOT_SETARG(args, num_args, XmNshowValue, XmNONE);

  if (iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "VERTICAL"))
  {
    iupMOT_SETARG(args, num_args, XmNorientation, XmVERTICAL);

    if (ih->userheight < ih->userwidth)
    {
      int tmp = ih->userheight;
      ih->userheight = ih->userwidth;
      ih->userwidth = tmp;
    }
  }
  else
    iupMOT_SETARG(args, num_args, XmNorientation, XmHORIZONTAL);

  if (iupAttribGetBoolean(ih, "MARQUEE"))
  {
    ih->data->marquee = 1;
    ih->data->timer = IupTimer();
    IupSetCallback(ih->data->timer, "ACTION_CB", (Icallback)motProgressBarTimeCb);
    IupSetAttribute(ih->data->timer, "TIME", "100");
    iupAttribSet(ih->data->timer, "_IUP_PROGRESSBAR", (char*)ih);
    iupMOT_SETARG(args, num_args, XmNslidingMode, XmSLIDER);
  }
  else
  {
    iupMOT_SETARG(args, num_args, XmNslidingMode, XmTHERMOMETER);
    ih->data->marquee = 0;
  }
  
  ih->handle = XtCreateManagedWidget(
    iupDialogGetChildIdStr(ih),  /* child identifier */
    xmScaleWidgetClass,          /* widget class */
    iupChildTreeGetNativeParentHandle(ih), /* widget parent */
    args, num_args);

  if (!ih->handle)
    return IUP_ERROR;

  ih->serial = iupDialogGetChildId(ih); /* must be after using the string */

  XtAddEventHandler(ih->handle, EnterWindowMask, False, (XtEventHandler)iupmotEnterLeaveWindowEvent, (XtPointer)ih);
  XtAddEventHandler(ih->handle, LeaveWindowMask, False, (XtEventHandler)iupmotEnterLeaveWindowEvent, (XtPointer)ih);

  /* initialize the widget */
  XtRealizeWidget(ih->handle);

  return IUP_NOERROR;
}

void iupdrvProgressBarInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = motProgressBarMapMethod;
  ic->LayoutUpdate = motProgressBarLayoutUpdateMethod;

  /* Driver Dependent Attribute functions */

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, motProgressBarSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  /* Special */
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, motProgressBarSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);

  /* IupProgressBar only */
  iupClassRegisterAttribute(ic, "VALUE",  iProgressBarGetValueAttrib,  motProgressBarSetValueAttrib,  NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ORIENTATION", NULL, NULL, "HORIZONTAL", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARQUEE",     NULL, motProgressBarSetMarqueeAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  /* NOT supported */
  iupClassRegisterAttribute(ic, "DASHED", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED);
}
