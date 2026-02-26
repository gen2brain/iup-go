/** \file
 * \brief Scrollbar Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupkey.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_stdcontrols.h"
#include "iup_layout.h"
#include "iup_scrollbar.h"


void iupScrollbarCropValue(Ihandle* ih)
{
  double max_pos = ih->data->vmax - ih->data->pagesize;
  if (max_pos < ih->data->vmin)
    max_pos = ih->data->vmin;

  if (ih->data->val > max_pos)
    ih->data->val = max_pos;
  else if (ih->data->val < ih->data->vmin)
    ih->data->val = ih->data->vmin;
}

char* iupScrollbarGetValueAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->val);
}

char* iupScrollbarGetLineStepAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->linestep);
}

char* iupScrollbarGetPageStepAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->pagestep);
}

char* iupScrollbarGetPageSizeAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->pagesize);
}

static int iScrollbarSetMaxAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDouble(value, &(ih->data->vmax)))
    iupScrollbarCropValue(ih);
  return 0;
}

static char* iScrollbarGetMaxAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->vmax);
}

static int iScrollbarSetMinAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDouble(value, &(ih->data->vmin)))
    iupScrollbarCropValue(ih);
  return 0;
}

static char* iScrollbarGetMinAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->vmin);
}

static int iScrollbarSetOrientationAttrib(Ihandle* ih, const char *value)
{
  int min_w, min_h;

  if (ih->handle)
    return 0;

  if (iupStrEqualNoCase(value, "VERTICAL"))
  {
    ih->data->orientation = ISCROLLBAR_VERTICAL;
    iupdrvScrollbarGetMinSize(ih, &min_w, &min_h);
    IupSetfAttribute(ih, "RASTERSIZE", "%dx%d", min_w, 100);
  }
  else
  {
    ih->data->orientation = ISCROLLBAR_HORIZONTAL;
    iupdrvScrollbarGetMinSize(ih, &min_w, &min_h);
    IupSetfAttribute(ih, "RASTERSIZE", "%dx%d", 100, min_h);
  }

  return 0;
}

static char* iScrollbarGetOrientationAttrib(Ihandle* ih)
{
  if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
    return "HORIZONTAL";
  else
    return "VERTICAL";
}

static int iScrollbarSetInvertedAttrib(Ihandle* ih, const char *value)
{
  if (ih->handle)
    return 0;

  if (iupStrBoolean(value))
    ih->data->inverted = 1;
  else
    ih->data->inverted = 0;

  return 0;
}

static char* iScrollbarGetInvertedAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->inverted);
}

static int iScrollbarCreateMethod(Ihandle* ih, void **params)
{
  char* orientation = "HORIZONTAL";
  if (params && params[0])
    orientation = params[0];

  ih->data = iupALLOCCTRLDATA();

  iScrollbarSetOrientationAttrib(ih, orientation);

  ih->data->vmax = 1.00;
  ih->data->pagesize = 0.1;
  ih->data->linestep = 0.01;
  ih->data->pagestep = 0.1;

  return IUP_NOERROR;
}

Iclass* iupScrollbarNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "scrollbar";
  ic->format = "s";
  ic->format_attr = "ORIENTATION";
  ic->nativetype = IUP_TYPECONTROL;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;

  ic->New = iupScrollbarNewClass;
  ic->Create = iScrollbarCreateMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;

  iupClassRegisterCallback(ic, "VALUECHANGED_CB", "");
  iupClassRegisterCallback(ic, "SCROLL_CB", "iff");

  iupBaseRegisterCommonCallbacks(ic);

  iupBaseRegisterCommonAttrib(ic);

  iupBaseRegisterVisualAttrib(ic);

  iupClassRegisterAttribute(ic, "MAX", iScrollbarGetMaxAttrib, iScrollbarSetMaxAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "MIN", iScrollbarGetMinAttrib, iScrollbarSetMinAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "TYPE", iScrollbarGetOrientationAttrib, iScrollbarSetOrientationAttrib, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ORIENTATION", iScrollbarGetOrientationAttrib, iScrollbarSetOrientationAttrib, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INVERTED", iScrollbarGetInvertedAttrib, iScrollbarSetInvertedAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupdrvScrollbarInitClass(ic);

  return ic;
}

IUP_API Ihandle* IupScrollbar(const char *orientation)
{
  void *params[2];
  params[0] = (void*)orientation;
  params[1] = NULL;
  return IupCreatev("scrollbar", params);
}
