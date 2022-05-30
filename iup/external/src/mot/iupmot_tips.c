/** \file
 * \brief Motif Driver TIPS Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <Xm/Xm.h>
#include <Xm/Label.h>

#include <stdio.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_drvinfo.h"

#include "iupmot_drv.h"
#include "iupmot_color.h"


/* Default offset from cursor */
#define TIP_X_OFFSET    12
#define TIP_Y_OFFSET    12

typedef struct _Imottips
{
  Widget        Dialog;
  Widget        Label;
  XtIntervalId  ShowTimerId;       /* Timer: from enterwin to show tip    */
  XtIntervalId  HideTimerId;       /* Timer: from visible to hide tip    */
  unsigned long ShowInterval;      /* Time for tipPopupTimerId         */
  unsigned long HideInterval;      /* Time for tipActiveTimerId        */
  int           Visible;
  Ihandle*      ih;
} Imottips;

static Imottips mot_tips = {0, 0, 0, 0, 0, 0, 0, 0};

static void motTipsShow(void)
{
  char* value;
  int x, y;
  IFnii cb;

  iupdrvGetCursorPos(&x, &y);

  cb = (IFnii)IupGetCallback(mot_tips.ih, "TIPS_CB");
  if (cb)
  {
    int wx = x, wy = y;
    iupdrvScreenToClient(mot_tips.ih, &wx, &wy);
    cb(mot_tips.ih, wx, wy);
  }

  value = iupAttribGet(mot_tips.ih, "TIPRECT");
  if (value)
  {
    int x1, x2, y1, y2, wx = x, wy = y;
    sscanf(value, "%d %d %d %d", &x1, &y1, &x2, &y2);
    iupdrvScreenToClient(mot_tips.ih, &wx, &wy);
    if (wx < x1 || wx > x2 || wy < y1 || wy > y2)
    {
      iupmotTipLeaveNotify();
      return;
    }
  }

  value = iupAttribGetStr(mot_tips.ih, "TIPBGCOLOR");
  if (value)
    XtVaSetValues(mot_tips.Label, XmNbackground, iupmotColorGetPixelStr(value), NULL);

  value = iupAttribGetStr(mot_tips.ih, "TIPFGCOLOR");
  if (value)
    XtVaSetValues(mot_tips.Label, XmNforeground, iupmotColorGetPixelStr(value), NULL);

  /* set label font */
  {
    XmFontList fontlist = (XmFontList)iupmotGetFontListAttrib(mot_tips.ih);
    value = iupAttribGetStr(mot_tips.ih, "TIPFONT");
    if (value)
    {
      if (iupStrEqualNoCase(value, "SYSTEM"))
        fontlist = NULL;
      else
        fontlist = iupmotGetFontList(iupAttribGet(mot_tips.ih, "TIPFOUNDRY"), value);
    }

    if (fontlist)
      XtVaSetValues(mot_tips.Label, XmNfontList, fontlist, NULL);
  }
 
  /* set label contents */
  value = iupAttribGet(mot_tips.ih, "TIP");
  iupmotSetXmString(mot_tips.Label, XmNlabelString, value);

  /* set label size */
  {
    int lw = 0, lh = 0;
    iupdrvFontGetMultiLineStringSize(mot_tips.ih, value, &lw, &lh);

    /* add room for margin */
    lw += 2*(2);
    lh += 2*(2);  

    XtVaSetValues(mot_tips.Label,
      XmNwidth, (XtArgVal)lw,
      XmNheight, (XtArgVal)lh,
      NULL);
    XtVaSetValues(mot_tips.Dialog,
      XmNwidth, (XtArgVal)lw,
      XmNheight, (XtArgVal)lh,
      NULL);
  }
  
  XtVaSetValues(mot_tips.Dialog, 
    XmNx, (XtArgVal)(x+TIP_X_OFFSET),
    XmNy, (XtArgVal)(y+TIP_Y_OFFSET), 
    NULL);

  if (mot_tips.Visible)
    XRaiseWindow(iupmot_display, XtWindow(mot_tips.Dialog));
  else
    XtMapWidget(mot_tips.Dialog);

  mot_tips.ShowTimerId = (XtIntervalId) NULL;
  mot_tips.Visible = TRUE;
}

static void motTipsHide(void)
{
  mot_tips.HideTimerId = (XtIntervalId) NULL;
  iupmotTipLeaveNotify();
}

static void motTipsInit(Ihandle *ih)
{
  if (!mot_tips.Dialog)
  {
    mot_tips.Dialog = XtVaAppCreateShell(" ", "tip",
      overrideShellWidgetClass, iupmot_display,
      XmNmappedWhenManaged, False,
      NULL);

    mot_tips.Label = XtVaCreateManagedWidget("label tip",
      xmLabelWidgetClass, mot_tips.Dialog,
      NULL);

    XtRealizeWidget(mot_tips.Dialog);
  }

  mot_tips.ih = ih;
  mot_tips.ShowTimerId  = (XtIntervalId) NULL;
  mot_tips.HideTimerId  = (XtIntervalId) NULL;
  mot_tips.Visible      = FALSE;
  mot_tips.ShowInterval = 500;  /* 1/2 second to show */

  {
    int delay = IupGetInt(ih, "TIPDELAY");  /* must use IupGetInt to use inheritance */
    mot_tips.HideInterval = delay + mot_tips.ShowInterval;
  }
}

int iupdrvBaseSetTipVisibleAttrib(Ihandle* ih, const char* value)
{
  if (mot_tips.ih != ih)
    return 0;

  if (iupStrBoolean(value))
    iupmotTipEnterNotify(ih);
  else
    iupmotTipLeaveNotify();

  return 0;
}

char* iupdrvBaseGetTipVisibleAttrib(Ihandle* ih)
{
  if (mot_tips.ih != ih)
    return NULL;

  return iupStrReturnBoolean (mot_tips.Visible); 
}

void iupmotTipEnterNotify(Ihandle *ih)
{
  iupmotTipLeaveNotify();

  if (!iupAttribGet(ih, "TIP"))
    return;

  motTipsInit(ih);

  mot_tips.ShowTimerId = XtAppAddTimeOut( /* EnterWin to Show */
    iupmot_appcontext,
    mot_tips.ShowInterval,
    (XtTimerCallbackProc)motTipsShow,
    NULL);
  mot_tips.HideTimerId = XtAppAddTimeOut( /* Visible to Hide */
    iupmot_appcontext,
    mot_tips.HideInterval,
    (XtTimerCallbackProc)motTipsHide,
    NULL);
}

void iupmotTipLeaveNotify(void)
{
  if (mot_tips.ShowTimerId)
  {
    XtRemoveTimeOut(mot_tips.ShowTimerId);
    mot_tips.ShowTimerId = (XtIntervalId) NULL;
  }
  if (mot_tips.HideTimerId)
  {
    XtRemoveTimeOut(mot_tips.HideTimerId);
    mot_tips.HideTimerId = (XtIntervalId) NULL;
  }
  if (mot_tips.Visible)
  {
    XtUnmapWidget(mot_tips.Dialog);
    mot_tips.Visible = FALSE;
  }
}

void iupmotTipsFinish(void)
{
  if (mot_tips.Dialog)
  {
    XtDestroyWidget(mot_tips.Dialog);
    memset(&mot_tips, 0, sizeof(Imottips));
  }
}

int iupdrvBaseSetTipAttrib(Ihandle* ih, const char* value)
{
  /* implement this stub here because of the other drivers. */
  (void)ih;
  (void)value;
  return 1;
}
