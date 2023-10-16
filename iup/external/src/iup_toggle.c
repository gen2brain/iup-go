/** \file
 * \brief Toggle Control.
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
#include "iup_toggle.h"
#include "iup_image.h"


static char* iToggleGetRadioAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->is_radio);
}

static int iToggleSetFlatAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)  /* set only before map */
    ih->data->flat = iupStrBoolean(value);
  return 0;
}

static char* iToggleGetFlatAttrib(Ihandle *ih)
{
  return iupStrReturnBoolean (ih->data->flat); 
}

char* iupToggleGetPaddingAttrib(Ihandle* ih)
{
  return iupStrReturnIntInt(ih->data->horiz_padding, ih->data->vert_padding, 'x');
}

static int iToggleCreateMethod(Ihandle* ih, void** params)
{
  if (params)
  {
    if (params[0]) iupAttribSetStr(ih, "TITLE", (char*)(params[0]));
    if (params[1]) iupAttribSetStr(ih, "ACTION", (char*)(params[1]));
  }
  ih->data = iupALLOCCTRLDATA();
  return IUP_NOERROR;
}

static void iToggleComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  int natural_w = 0, 
      natural_h = 0,
      type = ih->data->type;
  (void)children_expand; /* unset if not a container */

  if (!ih->handle)
  {
    /* if not mapped must initialize the internal values */
    char* value = iupAttribGet(ih, "IMAGE");
    if (value)
      type = IUP_TOGGLE_IMAGE;
    else
      type = IUP_TOGGLE_TEXT;
  }

  if (type == IUP_TOGGLE_IMAGE)
  {
    iupImageGetInfo(iupAttribGet(ih, "IMAGE"), &natural_w, &natural_h, NULL);

    /* even when IMPRESS is set, must compute the borders space */
    iupdrvToggleAddBorders(ih, &natural_w, &natural_h);

    natural_w += 2*ih->data->horiz_padding;
    natural_h += 2*ih->data->vert_padding;
  }
  else /* IUP_TOGGLE_TEXT */
  {
    /* must use IupGetAttribute to check from the native implementation */
    char* title = IupGetAttribute(ih, "TITLE");
    char* str = iupStrProcessMnemonic(title, NULL, 0);   /* remove & */
    iupdrvFontGetMultiLineStringSize(ih, str, &natural_w, &natural_h);

    iupdrvToggleAddCheckBox(ih, &natural_w, &natural_h, str);

    if (str && str != title) free(str);
  }

  *w = natural_w;
  *h = natural_h;
}


/******************************************************************************/


IUP_API Ihandle* IupToggle(const char* title, const char* action)
{
  void *params[3];
  params[0] = (void*)title;
  params[1] = (void*)action;
  params[2] = NULL;
  return IupCreatev("toggle", params);
}

Iclass* iupToggleNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "toggle";
  ic->format = "sa"; /* one string and one ACTION callback name */
  ic->format_attr = "TITLE";
  ic->nativetype = IUP_TYPECONTROL;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;

  /* Class functions */
  ic->New = iupToggleNewClass;
  ic->Create = iToggleCreateMethod;
  ic->ComputeNaturalSize = iToggleComputeNaturalSizeMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;

  /* Callbacks */
  iupClassRegisterCallback(ic, "ACTION", "i");
  iupClassRegisterCallback(ic, "VALUECHANGED_CB", "");

  /* Common Callbacks */
  iupBaseRegisterCommonCallbacks(ic);

  /* Common */
  iupBaseRegisterCommonAttrib(ic);

  /* Visual */
  iupBaseRegisterVisualAttrib(ic);

  iupClassRegisterAttribute(ic, "RADIO", iToggleGetRadioAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "3STATE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FLAT", iToggleGetFlatAttrib, iToggleSetFlatAttrib, IUPAF_SAMEASSYSTEM, "No", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "IGNORERADIO", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupdrvToggleInitClass(ic);

  return ic;
}
