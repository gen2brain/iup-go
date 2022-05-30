/** \file
 * \brief ProgressBar control
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
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_stdcontrols.h"
#include "iup_layout.h"
#include "iup_progressbar.h"


void iProgressBarCropValue(Ihandle* ih)
{
  if(ih->data->value > ih->data->vmax)
    ih->data->value = ih->data->vmax;
  else if(ih->data->value < ih->data->vmin)
    ih->data->value = ih->data->vmin;
}

char* iProgressBarGetValueAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->value);
}

char* iProgressBarGetDashedAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->dashed);
}

static int iProgressBarSetMinAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDouble(value, &(ih->data->vmin)))
    iProgressBarCropValue(ih);
  return 0;
}

static char* iProgressBarGetMinAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->vmin);
}

static int iProgressBarSetMaxAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDouble(value, &(ih->data->vmax)))
    iProgressBarCropValue(ih);
  return 0;
}

static char* iProgressBarGetMaxAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->vmax);
}

static int iProgressBarCreateMethod(Ihandle* ih, void **params)
{
  (void)params;

  ih->data = iupALLOCCTRLDATA();

  /* default values */
  ih->data->vmax      = 1;
  ih->data->dashed    = 0;

  /* progress bar default size is 200x30 */
  IupSetAttribute(ih, "RASTERSIZE", "200x30");

  return IUP_NOERROR;
}

static void iProgressBarDestroyMethod(Ihandle* ih)
{
  if (ih->data->timer)
    IupDestroy(ih->data->timer);
}

Iclass* iupProgressBarNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "progressbar";
  ic->cons = "ProgressBar";
  ic->format = NULL; /* no parameters */
  ic->nativetype  = IUP_TYPECONTROL;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 0;

  /* Class functions */
  ic->New = iupProgressBarNewClass;
  ic->Create = iProgressBarCreateMethod;
  ic->Destroy = iProgressBarDestroyMethod;

  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;

  /* Common Callbacks */
  iupClassRegisterCallback(ic, "MAP_CB", "");
  iupClassRegisterCallback(ic, "UNMAP_CB", "");

  /* Common */
  iupBaseRegisterCommonAttrib(ic);

  /* Visual */
  iupBaseRegisterVisualAttrib(ic);

  /* IupProgressBar only */
  iupClassRegisterAttribute(ic, "MIN", iProgressBarGetMinAttrib, iProgressBarSetMinAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAX", iProgressBarGetMaxAttrib, iProgressBarSetMaxAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ORIENTATION", NULL, NULL, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_NOT_MAPPED);

  iupdrvProgressBarInitClass(ic);

  return ic;
}

IUP_API Ihandle* IupProgressBar(void)
{
  return IupCreate("progressbar");
}
