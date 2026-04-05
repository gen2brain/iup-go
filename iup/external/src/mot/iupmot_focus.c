/** \file
 * \brief Motif Focus
 *
 * See Copyright Notice in "iup.h"
 */

#include <Xm/Xm.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_focus.h"
#include "iup_drv.h"

#include "iupmot_drv.h"

IUP_SDK_API void iupdrvSetFocus(Ihandle *ih)
{
  XmProcessTraversal(ih->handle, XmTRAVERSE_CURRENT);
}

IUP_DRV_API void iupmotFocusChangeEvent(Widget w, Ihandle *ih, XEvent *evt, Boolean *cont)
{
  (void)w;
  (void)cont;

  if (evt->type == FocusIn)
    iupCallGetFocusCb(ih);
  else if (evt->type == FocusOut)
    iupCallKillFocusCb(ih);
}
