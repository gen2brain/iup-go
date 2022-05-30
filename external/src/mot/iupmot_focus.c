/** \file
 * \brief Motif Focus
 *
 * See Copyright Notice in "iup.h"
 */

#include <Xm/Xm.h>

#include <stdio.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_focus.h"
#include "iup_attrib.h"
#include "iup_assert.h" 
#include "iup_drv.h" 

#include "iupmot_drv.h"

void iupdrvSetFocus(Ihandle *ih)
{
  XmProcessTraversal(ih->handle, XmTRAVERSE_CURRENT);
}

void iupmotFocusChangeEvent(Widget w, Ihandle *ih, XEvent *evt, Boolean *cont)
{
  (void)w;
  (void)cont;

  if (evt->type == FocusIn)
    iupCallGetFocusCb(ih);
  else if (evt->type == FocusOut)
    iupCallKillFocusCb(ih);
}
