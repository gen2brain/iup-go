/** \file
 * \brief cbox control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_stdcontrols.h"
#include "iup_layout.h"
#include "iup_varg.h"


static int iCboxCreateMethod(Ihandle* ih, void** params)
{
  if (params)
  {
    Ihandle** iparams = (Ihandle**)params;
    while (*iparams) 
    {
      IupAppend(ih, *iparams);
      iparams++;
    }
  }

  return IUP_NOERROR;
}

static void iCboxComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  Ihandle* child;
  int children_naturalwidth, children_naturalheight;
  int cx, cy;

  /* calculate total children natural size (even for hidden children) */
  children_naturalwidth = 0;
  children_naturalheight = 0;

  for (child = ih->firstchild; child; child = child->brother)
  {
    /* update child natural size first */
    iupBaseComputeNaturalSize(child);

    cx = iupAttribGetInt(child, "CX");
    cy = iupAttribGetInt(child, "CY");

    *children_expand |= child->expand;
    children_naturalwidth = iupMAX(children_naturalwidth, cx+child->naturalwidth);
    children_naturalheight = iupMAX(children_naturalheight, cy+child->naturalheight);
  }

  *w = children_naturalwidth;
  *h = children_naturalheight;
}

static void iCboxSetChildrenCurrentSizeMethod(Ihandle* ih, int shrink)
{
  Ihandle* child;
  for (child = ih->firstchild; child; child = child->brother)
  {
    /* update children to their own natural size */
    iupBaseSetCurrentSize(child, child->naturalwidth, child->naturalheight, shrink);
  }
}

static void iCboxSetChildrenPositionMethod(Ihandle* ih, int x, int y)
{
  int cx, cy;
  Ihandle* child;

  for (child = ih->firstchild; child; child = child->brother)
  {
    cx = iupAttribGetInt(child, "CX");
    cy = iupAttribGetInt(child, "CY");

    /* update child */
    iupBaseSetPosition(child, x+cx, y+cy);
  }
}


/******************************************************************************/


IUP_API Ihandle* IupCboxv(Ihandle** children)
{
  return IupCreatev("cbox", (void**)children);
}

IUP_API Ihandle* IupCboxV(Ihandle* child, va_list arglist)
{
  return IupCreateV("cbox", child, arglist);
}

IUP_API Ihandle* IupCbox (Ihandle * child,...)
{
  Ihandle *ih;

  va_list arglist;
  va_start(arglist, child);
  ih = IupCreateV("cbox", child, arglist);
  va_end(arglist);

  return ih;
}

Iclass* iupCboxNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "cbox";
  ic->format = "g"; /* array of Ihandle */
  ic->nativetype = IUP_TYPEVOID;
  ic->childtype = IUP_CHILDMANY;  /* can have children */
  ic->is_interactive = 0;

  /* Class functions */
  ic->New = iupCboxNewClass;
  ic->Create = iCboxCreateMethod;
  ic->Map = iupBaseTypeVoidMapMethod;

  ic->ComputeNaturalSize = iCboxComputeNaturalSizeMethod;
  ic->SetChildrenCurrentSize = iCboxSetChildrenCurrentSizeMethod;
  ic->SetChildrenPosition = iCboxSetChildrenPositionMethod;

  /* Base Callbacks */
  iupBaseRegisterBaseCallbacks(ic);

  /* Common */
  iupBaseRegisterCommonAttrib(ic);

  /* Base Container */
  iupClassRegisterAttribute(ic, "EXPAND", iupBaseContainerGetExpandAttrib, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTSIZE", iupBaseGetClientSizeAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTOFFSET", iupBaseGetClientOffsetAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  return ic;
}
