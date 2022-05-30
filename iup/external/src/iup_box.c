/** \file
 * \brief Base for Vbox and Hbox Controls.
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
#include "iup_box.h"
#include "iup_normalizer.h"


static char* iBoxGetClientSizeAttrib(Ihandle* ih)
{
  int width = ih->currentwidth;
  int height = ih->currentheight;
  width -= 2*ih->data->margin_horiz;
  height -= 2*ih->data->margin_vert;
  if (width < 0) width = 0;
  if (height < 0) height = 0;
  return iupStrReturnIntInt(width, height, 'x');
}

static int iBoxSetCGapAttrib(Ihandle* ih, const char* value)
{
  int cgap;
  iupStrToInt(value, &cgap);
  if (IupClassMatch(ih, "vbox"))
  {
    int charheight;
    iupdrvFontGetCharSize(ih, NULL, &charheight);
    ih->data->gap = iupHEIGHT2RASTER(cgap, charheight);
  }
  else
  {
    int charwidth;
    iupdrvFontGetCharSize(ih, &charwidth, NULL);
    ih->data->gap = iupWIDTH2RASTER(cgap, charwidth);
  }
  return 0;
}

static char* iBoxGetCGapAttrib(Ihandle* ih)
{
  if (IupClassMatch(ih, "vbox"))
  {
    int charheight;
    iupdrvFontGetCharSize(ih, NULL, &charheight);
    return iupStrReturnInt(iupRASTER2HEIGHT(ih->data->gap, charheight));
  }
  else
  {
    int charwidth;
    iupdrvFontGetCharSize(ih, &charwidth, NULL);
    return iupStrReturnInt(iupRASTER2WIDTH(ih->data->gap, charwidth));
  }
}

static int iBoxSetGapAttrib(Ihandle* ih, const char* value)
{
  iupStrToInt(value, &ih->data->gap);
  return 0;
}

static char* iBoxGetGapAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->gap);
}

static int iBoxSetHomogeneousAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->is_homogeneous = 1;
  else
    ih->data->is_homogeneous = 0;
  return 0;
}

static char* iBoxGetHomogeneousAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean (ih->data->is_homogeneous); 
}

static int iBoxSetExpandChildrenAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
  {
    if (IupClassMatch(ih, "vbox"))
      ih->data->expand_children = IUP_EXPAND_WIDTH;    /* in vert. box, expand horizontally */
    else
      ih->data->expand_children = IUP_EXPAND_HEIGHT;   /* in horiz. box, expand vertically */
  }
  else
    ih->data->expand_children = 0;
  return 0;
}

static char* iBoxGetExpandChildrenAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean (ih->data->expand_children); 
}

static int iBoxSetNormalizeSizeAttrib(Ihandle* ih, const char* value)
{
  ih->data->normalize_size = iupNormalizeGetNormalizeSize(value);
  return 0;
}

static char* iBoxGetNormalizeSizeAttrib(Ihandle* ih)
{
  return iupNormalizeGetNormalizeSizeStr(ih->data->normalize_size);
}

static int iBoxSetCMarginAttrib(Ihandle* ih, const char* value)
{
  int cmargin_x=-1, cmargin_y=-1;
  int charwidth, charheight;
  iupdrvFontGetCharSize(ih, &charwidth, &charheight);
  iupStrToIntInt(value, &cmargin_x, &cmargin_y, 'x');
  if (cmargin_x!=-1)
    ih->data->margin_horiz = iupWIDTH2RASTER(cmargin_x, charwidth);
  if (cmargin_y!=-1)
    ih->data->margin_vert = iupHEIGHT2RASTER(cmargin_y, charheight);
  return 0;
}

static char* iBoxGetCMarginAttrib(Ihandle* ih)
{
  int charwidth, charheight;
  iupdrvFontGetCharSize(ih, &charwidth, &charheight);
  return iupStrReturnIntInt(iupRASTER2WIDTH(ih->data->margin_horiz, charwidth), iupRASTER2HEIGHT(ih->data->margin_vert, charheight), 'x');
}

static int iBoxSetMarginAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->margin_horiz, &ih->data->margin_vert, 'x');
  return 0;
}

static char* iBoxGetMarginAttrib(Ihandle* ih)
{
  return iupStrReturnIntInt(ih->data->margin_horiz, ih->data->margin_vert, 'x');
}

static int iBoxUpdateAttribFromFont(Ihandle* ih)
{
  char* value = iupAttribGet(ih, "CMARGIN");
  if (!value)
    value = iupAttribGet(ih, "NCMARGIN");
  if (value)
    iBoxSetCMarginAttrib(ih, value);

  value = iupAttribGet(ih, "CGAP");
  if (!value)
    value = iupAttribGet(ih, "NCGAP");
  if (value)
    iBoxSetCGapAttrib(ih, value);

  return 0;
}


/******************************************************************************/

static int iBoxCreateMethod(Ihandle* ih, void** params)
{
  ih->data = iupALLOCCTRLDATA();

  if (params)
  {
    Ihandle** iparams = (Ihandle**)params;
    while (*iparams)
    {
      if (iupObjectCheck(*iparams))
        IupAppend(ih, *iparams);
      iparams++;
    }
  }

  IupSetCallback(ih, "UPDATEATTRIBFROMFONT_CB", iBoxUpdateAttribFromFont);

  return IUP_NOERROR;
}

Iclass* iupBoxNewClassBase(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->format = "g"; /* array of Ihandle */
  ic->nativetype = IUP_TYPEVOID;
  ic->childtype = IUP_CHILDMANY;  /* can have children */
  ic->is_interactive = 0;

  /* Class functions */
  ic->New = iupBoxNewClassBase;
  ic->Create = iBoxCreateMethod;
  ic->Map = iupBaseTypeVoidMapMethod;

  /* Base Callbacks */
  iupBaseRegisterBaseCallbacks(ic);

  /* Internal Callback */
  iupClassRegisterCallback(ic, "UPDATEATTRIBFROMFONT_CB", "");

  /* Common */
  iupBaseRegisterCommonAttrib(ic);

  /* Base Container */
  iupClassRegisterAttribute(ic, "EXPAND", iupBaseContainerGetExpandAttrib, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTSIZE", iBoxGetClientSizeAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTOFFSET", iupBaseGetClientOffsetAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  /* boxes only */
  iupClassRegisterAttribute(ic, "GAP", iBoxGetGapAttrib, iBoxSetGapAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "CGAP", iBoxGetCGapAttrib, iBoxSetCGapAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_SAVE|IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "NGAP", iBoxGetGapAttrib, iBoxSetGapAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_SAVE|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NCGAP", iBoxGetCGapAttrib, iBoxSetCGapAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_SAVE|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARGIN", iBoxGetMarginAttrib, iBoxSetMarginAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "CMARGIN", iBoxGetCMarginAttrib, iBoxSetCMarginAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NO_SAVE|IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "NMARGIN", iBoxGetMarginAttrib, iBoxSetMarginAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NO_SAVE|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NCMARGIN", iBoxGetCMarginAttrib, iBoxSetCMarginAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NO_SAVE|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "EXPANDCHILDREN", iBoxGetExpandChildrenAttrib, iBoxSetExpandChildrenAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HOMOGENEOUS", iBoxGetHomogeneousAttrib, iBoxSetHomogeneousAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NORMALIZESIZE", iBoxGetNormalizeSizeAttrib, iBoxSetNormalizeSizeAttrib, IUPAF_SAMEASSYSTEM, "NONE", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  return ic;
}
