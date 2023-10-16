/** \file
 * \brief MultiBox Control.
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
#include "iup_normalizer.h"
#include "iup_varg.h"


/* Orientation */
enum {IMBOX_HORIZONTAL, IMBOX_VERTICAL};

struct _IcontrolData 
{
  int margin_horiz, margin_vert,
      gap_vert, gap_horiz,
      orientation, 

      /* calculated in SetChildrenCurrentSize */
      num_lin, /* non zero only for horizontal orientation */
      num_col, /* non zero only for vertical orientation */
      total_width, total_height;
};


/**********************************************************************************************/


static char* iMultiBoxGetClientSizeAttrib(Ihandle* ih)
{
  int width = ih->currentwidth;
  int height = ih->currentheight;
  width -= 2 * ih->data->margin_horiz;
  height -= 2*ih->data->margin_vert;
  if (width < 0) width = 0;
  if (height < 0) height = 0;
  return iupStrReturnIntInt(width, height, 'x');
}

static int iMultiBoxSetCGapHorizAttrib(Ihandle* ih, const char* value)
{
  int cgap, charheight;
  iupStrToInt(value, &cgap);
  iupdrvFontGetCharSize(ih, NULL, &charheight);
  ih->data->gap_horiz = iupHEIGHT2RASTER(cgap, charheight);
  return 0;
}

static int iMultiBoxSetCGapVertAttrib(Ihandle* ih, const char* value)
{
  int cgap, charwidth;
  iupStrToInt(value, &cgap);
  iupdrvFontGetCharSize(ih, &charwidth, NULL);
  ih->data->gap_vert = iupWIDTH2RASTER(cgap, charwidth);
  return 0;
}

static char* iMultiBoxGetCGapVertAttrib(Ihandle* ih)
{
  int charwidth;
  iupdrvFontGetCharSize(ih, &charwidth, NULL);
  return iupStrReturnInt(iupRASTER2WIDTH(ih->data->gap_vert, charwidth));
}

static char* iMultiBoxGetCGapHorizAttrib(Ihandle* ih)
{
  int charheight;
  iupdrvFontGetCharSize(ih, NULL, &charheight);
  return iupStrReturnInt(iupRASTER2HEIGHT(ih->data->gap_horiz, charheight));
}

static int iMultiBoxSetGapVertAttrib(Ihandle* ih, const char* value)
{
  iupStrToInt(value, &ih->data->gap_vert);
  return 0;
}

static char* iMultiBoxGetGapVertAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->gap_vert);
}

static int iMultiBoxSetGapHorizAttrib(Ihandle* ih, const char* value)
{
  iupStrToInt(value, &ih->data->gap_horiz);
  return 0;
}

static char* iMultiBoxGetGapHorizAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->gap_horiz);
}

static int iMultiBoxSetCMarginAttrib(Ihandle* ih, const char* value)
{
  int cmargin_x=-1, cmargin_y=-1;
  int charwidth, charheight;
  iupdrvFontGetCharSize(ih, &charwidth, &charheight);
  iupStrToIntInt(value, &cmargin_x, &cmargin_y, 'x');
  if (cmargin_x!=-1)
    ih->data->margin_horiz = iupWIDTH2RASTER(cmargin_x, charwidth);
  if (cmargin_y!=-1)
    ih->data->margin_vert = iupHEIGHT2RASTER(cmargin_y, charheight);
  return 1;
}

static char* iMultiBoxGetCMarginAttrib(Ihandle* ih)
{
  int charwidth, charheight;
  iupdrvFontGetCharSize(ih, &charwidth, &charheight);
  return iupStrReturnIntInt(iupRASTER2WIDTH(ih->data->margin_horiz, charwidth), iupRASTER2HEIGHT(ih->data->margin_vert, charheight), 'x');
}

static int iMultiBoxSetMarginAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->margin_horiz, &ih->data->margin_vert, 'x');
  return 0;
}

static char* iMultiBoxGetMarginAttrib(Ihandle* ih)
{
  return iupStrReturnIntInt(ih->data->margin_horiz, ih->data->margin_vert, 'x');
}

static int iMultiBoxSetOrientationAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "VERTICAL"))
    ih->data->orientation = IMBOX_VERTICAL;
  else if (iupStrEqualNoCase(value, "HORIZONTAL"))
    ih->data->orientation = IMBOX_HORIZONTAL;
  return 0;
}

static char* iMultiBoxGetOrientationAttrib(Ihandle* ih)
{
  if (ih->data->orientation==IMBOX_HORIZONTAL)
    return "HORIZONTAL";
  else
    return "VERTICAL";
}

static char* iMultiBoxGetNumColAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->num_col);
}

static char* iMultiBoxGetNumLinAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->num_lin);
}

static int iMultiBoxUpdateAttribFromFont(Ihandle* ih)
{
  char* value = iupAttribGet(ih, "CMARGIN");
  if (!value)
    value = iupAttribGet(ih, "NCMARGIN");
  if (value)
    iMultiBoxSetCMarginAttrib(ih, value);

  value = iupAttribGet(ih, "CGAPVERT");
  if (!value)
    value = iupAttribGet(ih, "NCGAPVERT");
  if (value)
    iMultiBoxSetCGapVertAttrib(ih, value);

  value = iupAttribGet(ih, "CGAPHORIZ");
  if (!value)
    value = iupAttribGet(ih, "NCGAPHORIZ");
  if (value)
    iMultiBoxSetCGapHorizAttrib(ih, value);

  return IUP_DEFAULT;
}


/**********************************************************************************/

static void iMultiBoxComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  Ihandle* child;
  int max_width = 0;
  int max_height = 0;

  for (child = ih->firstchild; child; child = child->brother)
  {
    /* update child natural size first */
    if (!(child->flags & IUP_FLOATING_IGNORE))
    {
      iupBaseComputeNaturalSize(child);

      if (child->naturalwidth > max_width)
        max_width = child->naturalwidth;
      if (child->naturalheight > max_height)
        max_height = child->naturalheight;
    }
  }

  /* Also set expand to its own expand so it will not depend on children */
  /* So it will be able to dynamically expand and contract to rearrange its children */
  *children_expand = ih->expand;

  /* at least will have this size, use it as a minimum size */
  *w = max_width + 2 * ih->data->margin_horiz;
  *h = max_height + 2 * ih->data->margin_vert;

  if (ih->data->orientation == IMBOX_HORIZONTAL)
  {
    if (ih->data->num_lin)
      *h = ih->data->total_height;
  }
  else
  {
    if (ih->data->num_col)
      *w = ih->data->total_width;
  }
}

static void iMultiBoxSetChildrenCurrentSizeMethod(Ihandle* ih, int shrink)
{
  Ihandle* child, *prev_child = NULL;
  int num_lin = 0;
  int num_col = 0;
  int lin_width = 0;
  int lin_height = 0;
  int col_height = 0;
  int col_width = 0;
  int total_width = 0, total_height = 0;
  int child_max_width = 0, child_max_height = 0;
  int child_min_width = 0, child_min_height = 0;
  int child_width, child_height;

  iupAttribGetIntInt(ih, "CHILDMAXSIZE", &child_max_width, &child_max_height, 'x');
  iupAttribGetIntInt(ih, "CHILDMINSPACE", &child_min_width, &child_min_height, 'x');

  for (child = ih->firstchild; child; prev_child = child, child = child->brother)
  {
    if (!(child->flags & IUP_FLOATING_IGNORE))
    {
      /* child here can belong to the current line or column, 
         OR can be placed in the next one */

      /* CHILDMAXSIZE affects child current size */
      child_width = child->naturalwidth; 
      child_height = child->naturalheight;
      if (child_max_width != 0 && child_width > child_max_width)
        child_width = child_max_width;
      if (child_max_height != 0 && child_height > child_max_height)
        child_height = child_max_height;


      /* update children to their own natural size limited to the maximum size if any */
      iupBaseSetCurrentSize(child, child_width, child_height, shrink);


      /* CHILDMINSPACE affects child used space size, but does not affect the child size */
      child_width = child->currentwidth;
      child_height = child->currentheight;
      if (child_min_width != 0 && child_width < child_min_width)
        child_width = child_min_width;
      if (child_min_height != 0 && child_height < child_min_height)
        child_height = child_min_height;


      if (ih->data->orientation == IMBOX_HORIZONTAL)
      {
        int line_break = iupStrBoolean(iupAttribGet(prev_child, "LINEBREAK")); /* no inherit, no default */

        int new_lin_width = lin_width + child_width;

        if (new_lin_width > ih->currentwidth - 2 * ih->data->margin_horiz || line_break)
        {
          if (!line_break && prev_child)
            iupAttribSet(prev_child, "_IUPMB_LINEBREAK", "1");

          num_lin++;

          total_height += lin_height + ih->data->gap_vert;
          if (lin_width > total_width)
            total_width = lin_width;

          /* next line */
          num_col = 1;
          lin_width = child_width;
          lin_height = child_height;
        }
        else
        {
          if (prev_child)
            iupAttribSet(prev_child, "_IUPMB_LINEBREAK", NULL);

          if (child_height > lin_height)
            lin_height = child_height;

          num_col++;
          lin_width = new_lin_width;
          if (num_col > 1)
            lin_width += ih->data->gap_horiz;  /* for the previous child */
        }
      }
      else
      {
        int column_break = iupStrBoolean(iupAttribGet(prev_child, "COLUMNBREAK")); /* no inherit, no default */

        int new_col_height = col_height + child_height;

        if (new_col_height > ih->currentheight - 2 * ih->data->margin_vert || column_break)
        {
          if (!column_break && prev_child)
            iupAttribSet(prev_child, "_IUPMB_COLUMNBREAK", "1");

          num_col++;

          total_width += col_width + ih->data->gap_horiz;
          if (col_height > total_height)
            total_height = col_height;

          /* next column */
          num_lin = 1;
          col_height = child_height;
          col_width = child_width;
        }
        else
        {
          if (prev_child)
            iupAttribSet(prev_child, "_IUPMB_COLUMNBREAK", NULL);

          if (child_width > col_width)
            col_width = child_width;

          num_lin++;
          col_height = new_col_height;
          if (num_lin > 1)
            col_height += ih->data->gap_vert;  /* for the previous child */
        }
      }
    }
  }

  if (ih->data->orientation == IMBOX_HORIZONTAL)
  {
    num_lin++;
    num_col = 0;

    /* last line contribution */
    total_height += lin_height;
    if (lin_width > total_width)
      total_width = lin_width;
  }
  else
  {
    num_col++;
    num_lin = 0;

    /* last column contribution */
    total_width += col_width;
    if (col_height > total_height)
      total_height = col_height;
  }

  total_width += 2 * ih->data->margin_horiz;
  total_height += 2 * ih->data->margin_vert;

  /* read only, computed only here */
  ih->data->num_lin = num_lin;
  ih->data->num_col = num_col;
  ih->data->total_height = total_height;
  ih->data->total_width = total_width;
}

static void iMultiBoxSetChildrenPositionMethod(Ihandle* ih, int x, int y)
{
  Ihandle* child;
  int lin_height = 0;
  int col_width = 0;
  int child_min_width = 0, child_min_height = 0;
  int child_width, child_height;
  int dx = ih->data->margin_horiz; 
  int dy = ih->data->margin_vert;

  iupAttribGetIntInt(ih, "CHILDMINSPACE", &child_min_width, &child_min_height, 'x');

  for (child = ih->firstchild; child; child = child->brother)
  {
    if (!(child->flags & IUP_FLOATING_IGNORE))
    {
      /* child here belongs to the current line or column */

      iupBaseSetPosition(child, x + dx, y + dy);

      /* CHILDMINSPACE affects child used space size, but does not affect the child size */
      child_width = child->currentwidth;
      child_height = child->currentheight;
      if (child_min_width != 0 && child_width < child_min_width)
        child_width = child_min_width;
      if (child_min_height != 0 && child_height < child_min_height)
        child_height = child_min_height;


      if (ih->data->orientation == IMBOX_HORIZONTAL)
      {
        int line_break = iupStrBoolean(iupAttribGet(child, "LINEBREAK")); /* no inherit, no default */
        if (!line_break)
          line_break = iupAttribGetInt(child, "_IUPMB_LINEBREAK");

        if (child_height > lin_height)
          lin_height = child_height;

        if (line_break)
        {
          dx = ih->data->margin_horiz;
          dy += lin_height + ih->data->gap_vert;

          /* re-start */
          lin_height = 0;
        }
        else
        {
          dx += child_width + ih->data->gap_horiz;
        }
      }
      else
      {
        int column_break = iupStrBoolean(iupAttribGet(child, "COLUMNBREAK")); /* no inherit, no default */
        if (!column_break)
          column_break = iupAttribGetInt(child, "_IUPMB_COLUMNBREAK");

        if (child_width > col_width)
          col_width = child_width;

        if (column_break)
        {
          dx += col_width + ih->data->gap_horiz;
          dy = ih->data->margin_vert;

          /* re-start */
          col_width = 0;
        }
        else
        {
          dy += child_height + ih->data->gap_vert;
        }
      }
    }
  }
}

static int iMultiBoxCreateMethod(Ihandle* ih, void** params)
{
  ih->data = iupALLOCCTRLDATA();

  if (params)
  {
    Ihandle** iparams = (Ihandle**)params;
    while (*iparams) 
    {
      IupAppend(ih, *iparams);
      iparams++;
    }
  }

  IupSetCallback(ih, "UPDATEATTRIBFROMFONT_CB", iMultiBoxUpdateAttribFromFont);

  return IUP_NOERROR;
}


/******************************************************************************/


IUP_API Ihandle* IupMultiBoxv(Ihandle **children)
{
  return IupCreatev("multibox", (void**)children);
}

IUP_API Ihandle* IupMultiBoxV(Ihandle* child, va_list arglist)
{
  return IupCreateV("multibox", child, arglist);
}

IUP_API Ihandle* IupMultiBox(Ihandle* child, ...)
{
  Ihandle *ih;

  va_list arglist;
  va_start(arglist, child);
  ih = IupCreateV("multibox", child, arglist);
  va_end(arglist);

  return ih;
}

Iclass* iupMultiBoxNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "multibox";
  ic->cons = "MultiBox";
  ic->format = "g"; /* array of Ihandle */
  ic->nativetype = IUP_TYPEVOID;
  ic->childtype = IUP_CHILDMANY;  /* can have children */
  ic->is_interactive = 0;

  /* Class functions */
  ic->New = iupMultiBoxNewClass;
  ic->Create = iMultiBoxCreateMethod;
  ic->Map = iupBaseTypeVoidMapMethod;
  ic->ComputeNaturalSize = iMultiBoxComputeNaturalSizeMethod;
  ic->SetChildrenCurrentSize = iMultiBoxSetChildrenCurrentSizeMethod;
  ic->SetChildrenPosition = iMultiBoxSetChildrenPositionMethod;

  /* Base Callbacks */
  iupBaseRegisterBaseCallbacks(ic);

  /* Callbacks */
  iupClassRegisterCallback(ic, "UPDATEATTRIBFROMFONT_CB", "");

  /* Common */
  iupBaseRegisterCommonAttrib(ic);

  /* it does not behaves as a container regarding EXPAND */
  iupClassRegisterAttribute(ic, "EXPAND", iupBaseGetExpandAttrib, iupBaseSetExpandAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  /* Base Container */
  iupClassRegisterAttribute(ic, "CLIENTSIZE", iMultiBoxGetClientSizeAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTOFFSET", iupBaseGetClientOffsetAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  /* boxes */
  iupClassRegisterAttribute(ic, "MARGIN", iMultiBoxGetMarginAttrib, iMultiBoxSetMarginAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "CMARGIN", iMultiBoxGetCMarginAttrib, iMultiBoxSetCMarginAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NO_SAVE|IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "NMARGIN", iMultiBoxGetMarginAttrib, iMultiBoxSetMarginAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NO_SAVE|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NCMARGIN", iMultiBoxGetCMarginAttrib, iMultiBoxSetCMarginAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NO_SAVE|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  /* MultiBox only */
  iupClassRegisterAttribute(ic, "GAPVERT", iMultiBoxGetGapVertAttrib, iMultiBoxSetGapVertAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "CGAPVERT", iMultiBoxGetCGapVertAttrib, iMultiBoxSetCGapVertAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_SAVE|IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "GAPHORIZ", iMultiBoxGetGapHorizAttrib, iMultiBoxSetGapHorizAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "CGAPHORIZ", iMultiBoxGetCGapHorizAttrib, iMultiBoxSetCGapHorizAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_SAVE|IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "NGAPVERT", iMultiBoxGetGapVertAttrib, iMultiBoxSetGapVertAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_SAVE|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NCGAPVERT", iMultiBoxGetCGapVertAttrib, iMultiBoxSetCGapVertAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_SAVE|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NGAPHORIZ", iMultiBoxGetGapHorizAttrib, iMultiBoxSetGapHorizAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_SAVE|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NCGAPHORIZ", iMultiBoxGetCGapHorizAttrib, iMultiBoxSetCGapHorizAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_SAVE|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ORIENTATION", iMultiBoxGetOrientationAttrib, iMultiBoxSetOrientationAttrib, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NUMCOL", iMultiBoxGetNumColAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NUMLIN", iMultiBoxGetNumLinAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "CHILDMAXSIZE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CHILDMINSPACE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  /* on children
    LINEBREAK
    COLUMNBREAK
  */

  return ic;
}
