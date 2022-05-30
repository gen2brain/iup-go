/** \file
 * \brief Valuator Control
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
#include "iup_val.h"


void iupValCropValue(Ihandle* ih)
{
  if (ih->data->val > ih->data->vmax) 
    ih->data->val = ih->data->vmax;
  else if (ih->data->val < ih->data->vmin) 
    ih->data->val = ih->data->vmin;
}

char* iupValGetShowTicksAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->show_ticks);
}

char* iupValGetValueAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->val);
}

char* iupValGetStepAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->step);
}

char* iupValGetPageStepAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->pagestep);
}

static int iValSetMaxAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDouble(value, &(ih->data->vmax)))
    iupValCropValue(ih);
  return 0; /* do not store value in hash table */
}

static char* iValGetMaxAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->vmax);
}

static int iValSetMinAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDouble(value, &(ih->data->vmin)))
    iupValCropValue(ih);
  return 0; /* do not store value in hash table */
}

static char* iValGetMinAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->vmin);
}

static int iValSetOrientationAttrib(Ihandle* ih, const char *value)
{
  int min_w, min_h;

  /* valid only before map */
  if (ih->handle)
    return 0;

  if (iupStrEqualNoCase(value, "VERTICAL"))
  {
    ih->data->orientation = IVAL_VERTICAL;
    iupdrvValGetMinSize(ih, &min_w, &min_h);
    /* val natural vertical size is MinWx100 */
    IupSetfAttribute(ih, "RASTERSIZE", "%dx%d", min_w, 100);
  }
  else /* "HORIZONTAL" */
  {
    ih->data->orientation = IVAL_HORIZONTAL;
    iupdrvValGetMinSize(ih, &min_w, &min_h);
    /* val natural horizontal size is 100xMinH */
    IupSetfAttribute(ih, "RASTERSIZE", "%dx%d", 100, min_h);
  }

  return 0; /* do not store value in hash table */
}

static char* iValGetOrientationAttrib(Ihandle* ih)
{
  if (ih->data->orientation == IVAL_HORIZONTAL)
    return "HORIZONTAL";
  else /* (ih->data->orientation == IVAL_VERTICAL) */
    return "VERTICAL";
}

static int iValSetInvertedAttrib(Ihandle* ih, const char *value)
{
  /* valid only before map */
  if (ih->handle)
    return 0;

  if (iupStrBoolean(value))
    ih->data->inverted = 1;
  else
    ih->data->inverted = 0;

  return 0; /* do not store value in hash table */
}

static char* iValGetInvertedAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean (ih->data->inverted); 
}

static int iValCreateMethod(Ihandle* ih, void **params)
{
  char* orientation = "HORIZONTAL";
  if (params && params[0])
    orientation = params[0];

  ih->data = iupALLOCCTRLDATA();

  iValSetOrientationAttrib(ih, orientation);
  if (ih->data->orientation == IVAL_VERTICAL)
    ih->data->inverted = 1;  /* default is YES when vertical */

  ih->data->vmax = 1.00;
  ih->data->step = 0.01;
  ih->data->pagestep = 0.10;

  return IUP_NOERROR; 
}

Iclass* iupValNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "val";
  ic->format = "s"; /* one string */
  ic->format_attr = "ORIENTATION";
  ic->nativetype = IUP_TYPECONTROL;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;

  /* Class functions */
  ic->New = iupValNewClass;
  ic->Create  = iValCreateMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;

  /* Callbacks */
  iupClassRegisterCallback(ic, "VALUECHANGED_CB", "");
  iupClassRegisterCallback(ic, "MOUSEMOVE_CB", "d");
  iupClassRegisterCallback(ic, "BUTTON_PRESS_CB", "d");
  iupClassRegisterCallback(ic, "BUTTON_RELEASE_CB", "d");

  /* Common Callbacks */
  iupBaseRegisterCommonCallbacks(ic);

  /* Common */
  iupBaseRegisterCommonAttrib(ic);

  /* Visual */
  iupBaseRegisterVisualAttrib(ic);

  /* IupVal only */
  iupClassRegisterAttribute(ic, "MAX", iValGetMaxAttrib, iValSetMaxAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "MIN", iValGetMinAttrib, iValSetMinAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "TYPE", iValGetOrientationAttrib, iValSetOrientationAttrib, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ORIENTATION", iValGetOrientationAttrib, iValSetOrientationAttrib, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INVERTED", iValGetInvertedAttrib, iValSetInvertedAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupdrvValInitClass(ic);

  return ic;
}

IUP_API Ihandle* IupVal(const char *orientation)
{
  void *params[2];
  params[0] = (void*)orientation;
  params[1] = NULL;
  return IupCreatev("val", params);
}
