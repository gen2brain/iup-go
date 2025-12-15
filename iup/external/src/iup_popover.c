/** \file
 * \brief IupPopover control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_assert.h"
#include "iup_register.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_stdcontrols.h"
#include "iup_layout.h"
#include "iup_drv.h"
#include "iup_childtree.h"
#include "iup_class.h"

#include "iup_popover.h"


/*****************************************************************************\
|* Methods                                                                   *|
\*****************************************************************************/

static int iPopoverSetAnchorAttrib(Ihandle* ih, const char* value)
{
  /* Can only be set before map */
  if (ih->handle)
    return 0;

  /* value is the handle name set by IupSetAttributeHandle */
  if (value)
  {
    Ihandle* anchor = IupGetHandle(value);
    if (anchor)
      iupAttribSet(ih, "_IUP_POPOVER_ANCHOR", (char*)anchor);
  }

  return 1; /* store the name */
}

static char* iPopoverGetExpandAttrib(Ihandle* ih)
{
  /* Popover does not expand, it uses its child's natural size */
  (void)ih;
  return "NO";
}

static int iPopoverSetExpandAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 0;  /* do not store, popover does not expand */
}

static void iPopoverComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  if (ih->firstchild)
  {
    /* update child natural size first */
    iupBaseComputeNaturalSize(ih->firstchild);

    *w = ih->firstchild->naturalwidth;
    *h = ih->firstchild->naturalheight;
    *children_expand = ih->firstchild->expand;
  }
  else
  {
    *w = 0;
    *h = 0;
    *children_expand = 0;
  }
}

static void iPopoverSetChildrenCurrentSizeMethod(Ihandle* ih, int shrink)
{
  if (ih->firstchild)
  {
    /* Child fills the popover content area */
    iupBaseSetCurrentSize(ih->firstchild, ih->currentwidth, ih->currentheight, shrink);
  }
}

static void iPopoverSetChildrenPositionMethod(Ihandle* ih, int x, int y)
{
  if (ih->firstchild)
  {
    /* Child is at origin of popover content area */
    (void)x;
    (void)y;
    iupBaseSetPosition(ih->firstchild, 0, 0);
  }
}

static int iPopoverCreateMethod(Ihandle* ih, void** params)
{
  if (params)
  {
    Ihandle** iparams = (Ihandle**)params;
    if (iparams[0])
      IupAppend(ih, iparams[0]);
  }

  return IUP_NOERROR;
}

/*****************************************************************************\
|* Class Registration                                                        *|
\*****************************************************************************/

Iclass* iupPopoverNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "popover";
  ic->cons = "Popover";
  ic->format = "h";   /* one Ihandle* */
  ic->nativetype = IUP_TYPECONTROL;
  ic->childtype = IUP_CHILDMANY+1;  /* 1 child */
  ic->is_interactive = 0;

  /* Class functions */
  ic->New = iupPopoverNewClass;
  ic->Create = iPopoverCreateMethod;

  ic->ComputeNaturalSize = iPopoverComputeNaturalSizeMethod;
  ic->SetChildrenCurrentSize = iPopoverSetChildrenCurrentSizeMethod;
  ic->SetChildrenPosition = iPopoverSetChildrenPositionMethod;

  /* Callbacks */
  iupClassRegisterCallback(ic, "SHOW_CB", "i");

  /* Common, must be before driver so driver can override VISIBLE */
  iupBaseRegisterCommonAttrib(ic);

  /* Driver initializes Map/UnMap and attributes (including VISIBLE override) */
  iupdrvPopoverInitClass(ic);

  /* Override EXPAND, popover does not expand */
  iupClassRegisterAttribute(ic, "EXPAND", iPopoverGetExpandAttrib, iPopoverSetExpandAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  /* Popover specific attributes */
  iupClassRegisterAttribute(ic, "ANCHOR", NULL, iPopoverSetAnchorAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "POSITION", NULL, NULL, IUPAF_SAMEASSYSTEM, "BOTTOM", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ARROW", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AUTOHIDE", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);

  return ic;
}

IUP_API Ihandle* IupPopover(Ihandle* child)
{
  void *children[2];
  children[0] = (void*)child;
  children[1] = NULL;
  return IupCreatev("popover", children);
}
