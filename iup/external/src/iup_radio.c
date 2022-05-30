/** \file
 * \brief Radio Control.
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


IUP_SDK_API Ihandle *iupRadioFindToggleParent(Ihandle* ih_toggle)
{
  Ihandle *p;

  if (iupAttribGetBoolean(ih_toggle, "IGNORERADIO"))
    return NULL;

  for (p = ih_toggle; p->parent; p = p->parent)
  {
    if (p->iclass->nativetype == IUP_TYPEVOID &&
        IupClassMatch(p, "radio"))
      return p;
  }

  return NULL;
}

static int iRadioFindToggleChild(Ihandle* ih, Ihandle* ih_toggle)
{
  Ihandle* child;

  if (ih == ih_toggle) /* found child that match the toggle */
    return 1;

  for (child = ih->firstchild; child; child = child->brother)
  {
    if (iRadioFindToggleChild(child, ih_toggle))
      return 1;
  }

  return 0;
}

static int iRadioChildIsToggle(Ihandle* child)
{
  if (IupClassMatch(child, "toggle") || 
      IupClassMatch(child, "gltoggle") ||
      IupClassMatch(child, "flattoggle") ||
      (IupClassMatch(child, "flatbutton") && iupAttribGetBoolean(child, "TOGGLE")))
    return 1;
  else
    return 0;
}

static Ihandle* iRadioGetToggleChildOn(Ihandle* ih)
{
  Ihandle* child;

  /* found child that is a toggle and it is ON */
  if (iRadioChildIsToggle(ih) && IupGetInt(ih, "VALUE"))
    return ih;

  for (child = ih->firstchild; child; child = child->brother)
  {
    Ihandle* ih_toggle = iRadioGetToggleChildOn(child);
    if (ih_toggle)
      return ih_toggle;
  }

  return NULL;
}

/******************************************************************************/


static int iRadioSetValueHandleAttrib(Ihandle* ih, const char* value)
{
  Ihandle* ih_toggle = (Ihandle*)value;
  if (!iupObjectCheck(ih_toggle))
    return 0;

  if (!iRadioChildIsToggle(ih_toggle))
    return 0;

  if (iRadioFindToggleChild(ih->firstchild, ih_toggle))
    IupSetAttribute(ih_toggle, "VALUE", "ON");
 
  return 0;
}

static char* iRadioGetValueHandleAttrib(Ihandle* ih)
{
  return (char*)iRadioGetToggleChildOn(ih->firstchild);
}

static int iRadioSetValueAttrib(Ihandle* ih, const char* value)
{
  Ihandle *ih_toggle;

  if (!value)
    return 0;

  ih_toggle = IupGetHandle(value);
  if (!ih_toggle)
    return 0;

  iRadioSetValueHandleAttrib(ih, (char*)ih_toggle);
  return 0;
}

static char* iRadioGetValueAttrib(Ihandle* ih)
{
  Ihandle *ih_toggle = (Ihandle*)iRadioGetValueHandleAttrib(ih);
  return IupGetName(ih_toggle);  /* Name is guarantied at Toggle MapMethod */
}

/******************************************************************************/


static int iRadioCreateMethod(Ihandle* ih, void** params)
{
  if (params)
  {
    Ihandle** iparams = (Ihandle**)params;
    if (*iparams)
      IupAppend(ih, *iparams);
  }
  return IUP_NOERROR;
}

static void iRadioComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  Ihandle* child = ih->firstchild;
  if (child)
  {
    /* update child natural size first */
    iupBaseComputeNaturalSize(child);

    *children_expand = child->expand;
    *w = child->naturalwidth;
    *h = child->naturalheight;
  }
}

static void iRadioSetChildrenCurrentSizeMethod(Ihandle* ih, int shrink)
{
  if (ih->firstchild)
    iupBaseSetCurrentSize(ih->firstchild, ih->currentwidth, ih->currentheight, shrink);
}

static void iRadioSetChildrenPositionMethod(Ihandle* ih, int x, int y)
{
  if (ih->firstchild)
    iupBaseSetPosition(ih->firstchild, x, y);
}


/******************************************************************************/


IUP_API Ihandle* IupRadio(Ihandle* child)
{
  void *children[2];
  children[0] = (void*)child;
  children[1] = NULL;
  return IupCreatev("radio", children);
}

Iclass* iupRadioNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "radio";
  ic->format = "h"; /* one Ihandle* */
  ic->nativetype = IUP_TYPEVOID;
  ic->childtype = IUP_CHILDMANY+1;  /* 1 child */
  ic->is_interactive = 0;

  /* Class functions */
  ic->New = iupRadioNewClass;
  ic->Create = iRadioCreateMethod;
  ic->Map = iupBaseTypeVoidMapMethod;
  ic->ComputeNaturalSize = iRadioComputeNaturalSizeMethod;
  ic->SetChildrenCurrentSize = iRadioSetChildrenCurrentSizeMethod;
  ic->SetChildrenPosition = iRadioSetChildrenPositionMethod;

  /* Base Callbacks */
  iupBaseRegisterBaseCallbacks(ic);

  /* Common */
  iupBaseRegisterCommonAttrib(ic);

  /* Base Container */
  iupClassRegisterAttribute(ic, "EXPAND", iupBaseContainerGetExpandAttrib, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTSIZE", iupBaseGetClientSizeAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTOFFSET", iupBaseGetClientOffsetAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  /* Radio only */
  iupClassRegisterAttribute(ic, "VALUE", iRadioGetValueAttrib, iRadioSetValueAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE_HANDLE", iRadioGetValueHandleAttrib, iRadioSetValueHandleAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT | IUPAF_IHANDLE | IUPAF_NO_STRING);

  return ic;
}
