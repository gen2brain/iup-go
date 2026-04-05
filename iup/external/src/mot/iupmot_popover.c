/** \file
 * \brief IupPopover control for Motif
 *
 * See Copyright Notice in "iup.h"
 */

#include <Xm/Xm.h>
#include <Xm/BulletinB.h>
#include <Xm/MwmUtil.h>

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_popover.h"

#include "iupmot_drv.h"


static Ihandle* mot_popover_autohide = NULL;
static Widget mot_popover_dialog_manager = NULL;

static void motPopoverClickHandler(Widget w, XtPointer client_data, XEvent* evt, Boolean* cont)
{
  Ihandle* ih = mot_popover_autohide;
  Widget shell;
  Position sx, sy;
  Dimension sw, sh;

  (void)w;
  (void)client_data;
  (void)cont;

  if (!ih || !ih->handle || evt->type != ButtonPress)
    return;

  shell = (Widget)ih->handle;

  XtVaGetValues(shell,
    XmNx, &sx, XmNy, &sy,
    XmNwidth, &sw, XmNheight, &sh,
    NULL);

  if (evt->xbutton.x_root >= sx && evt->xbutton.x_root < sx + (int)sw &&
      evt->xbutton.y_root >= sy && evt->xbutton.y_root < sy + (int)sh)
    return;

  IupSetAttribute(ih, "VISIBLE", "NO");
}

static void motPopoverInstallGlobalHandler(Ihandle* ih)
{
  Widget shell = (Widget)ih->handle;
  Window shell_win = XtWindow(shell);
  Ihandle* anchor = (Ihandle*)iupAttribGet(ih, "_IUP_POPOVER_ANCHOR");

  mot_popover_autohide = ih;
  mot_popover_dialog_manager = NULL;

  XtAddRawEventHandler(shell, ButtonPressMask, False,
    motPopoverClickHandler, NULL);

  if (anchor)
  {
    Ihandle* dlg = IupGetDialog(anchor);
    if (dlg && dlg->handle)
    {
      Widget dm = XtNameToWidget(dlg->handle, "*dialog_manager");
      if (dm)
      {
        mot_popover_dialog_manager = dm;
        XtAddEventHandler(dm, ButtonPressMask, False,
          (XtEventHandler)motPopoverClickHandler, NULL);
      }
    }
  }

  XGrabPointer(iupmot_display, shell_win, True,
    ButtonPressMask | ButtonReleaseMask,
    GrabModeAsync, GrabModeAsync,
    None, None, CurrentTime);

  XSetInputFocus(iupmot_display, shell_win, RevertToParent, CurrentTime);
}

static void motPopoverRemoveGlobalHandler(void)
{
  if (mot_popover_autohide)
  {
    Widget shell = (Widget)mot_popover_autohide->handle;

    XUngrabPointer(iupmot_display, CurrentTime);

    if (shell)
      XtRemoveRawEventHandler(shell, ButtonPressMask, False,
        motPopoverClickHandler, NULL);

    if (mot_popover_dialog_manager)
    {
      XtRemoveEventHandler(mot_popover_dialog_manager, ButtonPressMask, False,
        (XtEventHandler)motPopoverClickHandler, NULL);
      mot_popover_dialog_manager = NULL;
    }

    mot_popover_autohide = NULL;
  }
}

static int motPopoverSetVisibleAttrib(Ihandle* ih, const char* value)
{
  Widget shell;

  if (iupStrBoolean(value))
  {
    Ihandle* anchor = (Ihandle*)iupAttribGet(ih, "_IUP_POPOVER_ANCHOR");
    Widget anchor_widget;
    Widget inner_parent;
    Position ax, ay;
    Dimension aw, ah;
    int x, y;
    int width, height;

    if (!anchor || !anchor->handle)
      return 0;

    /* Map if not yet mapped */
    if (!ih->handle)
    {
      if (IupMap(ih) == IUP_ERROR)
        return 0;
    }

    shell = (Widget)ih->handle;
    anchor_widget = (Widget)anchor->handle;
    inner_parent = (Widget)iupAttribGet(ih, "_IUP_MOT_INNER_PARENT");

    /* Compute layout to get proper sizes */
    if (ih->firstchild)
      iupLayoutCompute(ih);

    /* Get computed size, with fallback to avoid zero size */
    width = ih->currentwidth > 0 ? ih->currentwidth : 100;
    height = ih->currentheight > 0 ? ih->currentheight : 50;

    XtVaGetValues(anchor_widget,
      XmNwidth, &aw,
      XmNheight, &ah,
      NULL);

    XtTranslateCoords(anchor_widget, 0, 0, &ax, &ay);

    iupPopoverCalcPosition(ih,
      ax, ay, aw, ah,
      width, height,
      &x, &y);

    /* Set size and position on shell first */
    XtVaSetValues(shell,
      XmNx, x,
      XmNy, y,
      XmNwidth, width,
      XmNheight, height,
      NULL);

    /* Now set size on inner container (after shell is sized) */
    if (inner_parent)
    {
      XtVaSetValues(inner_parent,
        XmNwidth, width,
        XmNheight, height,
        NULL);
    }

    /* Now update children positions with proper container size */
    if (ih->firstchild)
      iupLayoutUpdate(ih);

    XtPopup(shell, XtGrabNone);

    if (iupAttribGetBoolean(ih, "AUTOHIDE"))
      motPopoverInstallGlobalHandler(ih);

    {
      IFni show_cb = (IFni)IupGetCallback(ih, "SHOW_CB");
      if (show_cb)
        show_cb(ih, IUP_SHOW);
    }
  }
  else
  {
    if (ih->handle)
    {
      shell = (Widget)ih->handle;

      motPopoverRemoveGlobalHandler();

      XtPopdown(shell);

      {
        IFni show_cb = (IFni)IupGetCallback(ih, "SHOW_CB");
        if (show_cb)
          show_cb(ih, IUP_HIDE);
      }
    }
  }

  return 0;
}

static char* motPopoverGetVisibleAttrib(Ihandle* ih)
{
  Widget shell;
  XWindowAttributes wa;

  if (!ih->handle)
    return "NO";

  shell = (Widget)ih->handle;

  /* Popup shells are not "managed" in Xt sense, check window visibility instead */
  if (!XtIsRealized(shell))
    return "NO";

  XGetWindowAttributes(iupmot_display, XtWindow(shell), &wa);
  return iupStrReturnBoolean(wa.map_state == IsViewable);
}

static void motPopoverLayoutUpdateMethod(Ihandle* ih)
{
  if (ih->firstchild)
  {
    ih->iclass->SetChildrenPosition(ih, 0, 0);
    iupLayoutUpdate(ih->firstchild);
  }
}

static int motPopoverMapMethod(Ihandle* ih)
{
  Widget shell;
  Widget inner_parent;
  Arg args[20];
  int num_args = 0;

  XtSetArg(args[num_args], XmNmappedWhenManaged, False); num_args++;
  XtSetArg(args[num_args], XmNoverrideRedirect, True); num_args++;
  XtSetArg(args[num_args], XmNsaveUnder, True); num_args++;
  XtSetArg(args[num_args], XmNborderWidth, 1); num_args++;

  shell = XtCreatePopupShell("popover",
    transientShellWidgetClass,
    iupmot_appshell,
    args, num_args);

  if (!shell)
    return IUP_ERROR;

  ih->handle = shell;

  num_args = 0;
  XtSetArg(args[num_args], XmNmarginWidth, 0); num_args++;
  XtSetArg(args[num_args], XmNmarginHeight, 0); num_args++;
  XtSetArg(args[num_args], XmNresizePolicy, XmRESIZE_NONE); num_args++;
  XtSetArg(args[num_args], XmNnavigationType, XmTAB_GROUP); num_args++;

  inner_parent = XtCreateManagedWidget("container",
    xmBulletinBoardWidgetClass,
    shell,
    args, num_args);

  if (!inner_parent)
  {
    XtDestroyWidget(shell);
    return IUP_ERROR;
  }

  iupAttribSet(ih, "_IUP_MOT_INNER_PARENT", (char*)inner_parent);


  /* Set initial size to avoid error during realize.
     Actual size will be set in SetVisibleAttrib before showing. */
  XtVaSetValues(shell, XmNwidth, 1, XmNheight, 1, NULL);

  /* Realize the shell so children can create their X windows.
     The shell won't be visible because mappedWhenManaged is False. */
  XtRealizeWidget(shell);

  return IUP_NOERROR;
}

static void motPopoverUnMapMethod(Ihandle* ih)
{
  Widget shell = (Widget)ih->handle;

  if (shell)
  {
    XtPopdown(shell);
    XtDestroyWidget(shell);
  }

  ih->handle = NULL;
}

static void* motPopoverGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
  (void)child;
  return iupAttribGet(ih, "_IUP_MOT_INNER_PARENT");
}

IUP_SDK_API void iupdrvPopoverInitClass(Iclass* ic)
{
  ic->Map = motPopoverMapMethod;
  ic->UnMap = motPopoverUnMapMethod;
  ic->LayoutUpdate = motPopoverLayoutUpdateMethod;
  ic->GetInnerNativeContainerHandle = motPopoverGetInnerNativeContainerHandleMethod;

  /* Override VISIBLE attribute, NOT_MAPPED because setter handles mapping */
  iupClassRegisterAttribute(ic, "VISIBLE", motPopoverGetVisibleAttrib, motPopoverSetVisibleAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
}
