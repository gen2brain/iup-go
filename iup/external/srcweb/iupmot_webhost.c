/** \file
 * \brief Web Browser Control - Motif host of the native web engine
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdint.h>

#include <Xm/Xm.h>
#include <Xm/DrawingA.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_classbase.h"
#include "iup_childtree.h"
#include "iup_dialog.h"

#include "iupmot_drv.h"
#include "iupweb_host.h"


static void motWebHostPlace(Ihandle* ih)
{
  Dimension width = 0, height = 0;

  if (!ih->handle || !XtIsRealized((Widget)ih->handle))
    return;

  XtVaGetValues((Widget)ih->handle, XmNwidth, &width, XmNheight, &height, NULL);
  iupwebHostSetBounds(ih, 0, 0, width, height, 0, 0, width, height);
}

static void motWebHostResizeCallback(Widget w, Ihandle* ih, XtPointer call_data)
{
  (void)w;
  (void)call_data;
  motWebHostPlace(ih);
}

static void motWebHostFocusEvent(Widget w, Ihandle* ih, XEvent* evt, Boolean* cont)
{
  if (evt->xfocus.detail == NotifyPointer)
    return;

  if (evt->type == FocusIn)
    iupwebHostSetFocus(ih, 1);
  else if (evt->type == FocusOut)
    iupwebHostSetFocus(ih, 0);

  iupmotFocusChangeEvent(w, ih, evt, cont);
}

static void motWebHostKeyEvent(Widget w, Ihandle* ih, XEvent* evt, Boolean* cont)
{
  (void)w;

  iupwebHostKeyEvent(ih, evt->type == KeyPress, evt->xkey.keycode, evt->xkey.state, evt->xkey.time);
  *cont = False;
}

void* iupwebHostMap(Ihandle* ih)
{
  int num_args = 0;
  Arg args[20];

  iupMOT_SETARG(args, num_args, XmNmarginHeight, 0);
  iupMOT_SETARG(args, num_args, XmNmarginWidth, 0);
  iupMOT_SETARG(args, num_args, XmNshadowThickness, 0);
  iupMOT_SETARG(args, num_args, XmNresizePolicy, XmRESIZE_NONE);
  iupMOT_SETARG(args, num_args, XmNnavigationType, XmTAB_GROUP);
  iupMOT_SETARG(args, num_args, XmNtraversalOn, iupAttribGetBoolean(ih, "CANFOCUS") ? True : False);
  iupMOT_SETARG(args, num_args, XmNx, 0);
  iupMOT_SETARG(args, num_args, XmNy, 0);
  iupMOT_SETARG(args, num_args, XmNwidth, 10);
  iupMOT_SETARG(args, num_args, XmNheight, 10);

  ih->handle = XtCreateManagedWidget(
    iupDialogGetChildIdStr(ih),
    xmDrawingAreaWidgetClass,
    iupChildTreeGetNativeParentHandle(ih),
    args, num_args);

  if (!ih->handle)
    return NULL;

  ih->serial = iupDialogGetChildId(ih);

  XtAddCallback(ih->handle, XmNresizeCallback, (XtCallbackProc)motWebHostResizeCallback, (XtPointer)ih);

  XtAddEventHandler(ih->handle, FocusChangeMask, False, (XtEventHandler)motWebHostFocusEvent, (XtPointer)ih);
  XtAddEventHandler(ih->handle, KeyPressMask, False, (XtEventHandler)motWebHostKeyEvent, (XtPointer)ih);
  XtAddEventHandler(ih->handle, KeyReleaseMask, False, (XtEventHandler)motWebHostKeyEvent, (XtPointer)ih);

  XtRealizeWidget(ih->handle);
  XSync(iupmot_display, False);

  return (void*)(uintptr_t)XtWindow(ih->handle);
}

void iupwebHostLayoutUpdate(Ihandle* ih)
{
  iupdrvBaseLayoutUpdateMethod(ih);
  motWebHostPlace(ih);
}

void iupwebHostUnMap(Ihandle* ih)
{
  if (!ih->handle)
    return;

  iupdrvBaseUnMapMethod(ih);
  ih->handle = NULL;
}
