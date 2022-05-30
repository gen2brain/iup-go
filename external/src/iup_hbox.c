/** \file
 * \brief Hbox Control.
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
#include "iup_varg.h"


static int iHboxSetRasterSizeAttrib(Ihandle* ih, const char* value)
{
  if (!value)
  {
    ih->userwidth = 0;
    ih->userheight = 0;
  }
  else
  {
    int s = 0, d = 0;
    iupStrToIntInt(value, &s, &d, 'x');  /* second value will be ignored, can NOT set height */
    if (s > 0) 
    {
      ih->userheight = 0;
      ih->userwidth = s;
    }
  }
  iupAttribSet(ih, "SIZE", NULL); /* clear SIZE in hash table */
  return 0;
}

static int iHboxSetSizeAttrib(Ihandle* ih, const char* value)
{
  if (!value)
  {
    ih->userwidth = 0;
    ih->userheight = 0;
  }
  else
  {
    int s = 0, d = 0;
    iupStrToIntInt(value, &s, &d, 'x');  /* second value will be ignored, can NOT set height */
    if (s > 0) 
    {
      int charwidth, charheight;
      iupdrvFontGetCharSize(ih, &charwidth, &charheight);
      ih->userheight = 0;
      ih->userwidth = iupWIDTH2RASTER(s, charwidth);
    }
  }
  return 1;  /* always save in the hash table, so when FONT is changed SIZE can be updated */
}

static int iHboxSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "ABOTTOM"))
    ih->data->alignment = IUP_ALIGN_ABOTTOM;
  else if (iupStrEqualNoCase(value, "ACENTER"))
    ih->data->alignment = IUP_ALIGN_ACENTER;
  else if (iupStrEqualNoCase(value, "ATOP"))
    ih->data->alignment = IUP_ALIGN_ATOP;
  return 0;
}

static char* iHboxGetAlignmentAttrib(Ihandle* ih)
{
  static char* align2str[3] = {"ATOP", "ACENTER", "ABOTTOM"};
  return align2str[ih->data->alignment];
}

static char* iHboxGetOrientationAttrib(Ihandle* ih)
{
  (void)ih;
  return "HORIZONTAL";
}

static void iHboxComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  Ihandle* child;
  int total_natural_width, total_natural_height;

  /* calculate total children natural size */
  int children_count = 0;
  int children_natural_width = 0;
  int children_natural_maxwidth = 0;
  int children_natural_maxheight = 0;

  for (child = ih->firstchild; child; child = child->brother)
  {
    if (ih->data->expand_children)
      child->expand = ih->data->expand_children;

    /* update child natural size first */
    if (!(child->flags & IUP_FLOATING_IGNORE))
      iupBaseComputeNaturalSize(child);

    if (!(child->flags & IUP_FLOATING))
    {
      *children_expand |= child->expand;
      children_natural_maxwidth = iupMAX(children_natural_maxwidth, child->naturalwidth);
      children_natural_maxheight = iupMAX(children_natural_maxheight, child->naturalheight);
      children_count++;
    }
  }

  /* reset to max natural width and/or height if NORMALIZESIZE is defined */
  if (ih->data->normalize_size)
    iupNormalizeSizeBoxChild(ih, ih->data->normalize_size, children_natural_maxwidth, children_natural_maxheight);

  /* must be done after normalize */
  for (child = ih->firstchild; child; child = child->brother)
  {
    if (!(child->flags & IUP_FLOATING))
      children_natural_width += child->naturalwidth;
  }

  /* leave room at the element for the maximum natural size of the children when is_homogeneous */
  if (ih->data->is_homogeneous)
    children_natural_width = children_natural_maxwidth*children_count;

  /* compute the Hbox contents natural size */
  total_natural_width  = children_natural_width + (children_count-1)*ih->data->gap + 2*ih->data->margin_horiz;
  total_natural_height = children_natural_maxheight + 2*ih->data->margin_vert;

  /* Store to be used in iHboxCalcEmptyWidth */
  ih->data->total_natural_size = total_natural_width;

  *w = total_natural_width;
  *h = total_natural_height;
}

static int iHboxCalcHomogeneousWidth(Ihandle *ih)
{
  Ihandle* child;
  int homogeneous_width;

  int children_count=0;
  for (child = ih->firstchild; child; child = child->brother)
  {
    if (!(child->flags & IUP_FLOATING))
      children_count++;
  }
  if (children_count == 0)
    return 0;

  /* equal spaces for all elements */
  homogeneous_width = (ih->currentwidth - (children_count-1)*ih->data->gap - 2*ih->data->margin_horiz)/children_count;
  if (homogeneous_width<0) homogeneous_width = 0;
  return homogeneous_width;
}

static int iHboxCalcEmptyWidth(Ihandle *ih, int expand)
{
  /* This is the space that the child can be expanded. */
  Ihandle* child;
  int empty_width;

  int expand_count=0;
  for (child = ih->firstchild; child; child = child->brother)
  {
    if (!(child->flags & IUP_FLOATING) && child->expand & expand)
      expand_count++;
  }
  if (expand_count == 0)
    return 0;

  /* equal spaces for all expandable elements */
  empty_width = (ih->currentwidth - ih->data->total_natural_size)/expand_count;
  if (empty_width < 0) empty_width = 0;
  return empty_width;
}

static void iHboxSetChildrenCurrentSizeMethod(Ihandle* ih, int shrink)
{
  /* update children */
  Ihandle* child;
  int empty_w0 = 0, empty_w1 = 0, client_height;

  if (ih->data->is_homogeneous)
    ih->data->homogeneous_size = iHboxCalcHomogeneousWidth(ih);
  else
  {
    ih->data->homogeneous_size = 0;

    /* must calculate the space left for each control to grow inside the container */
    /* W1 means there is an EXPAND enabled inside */
    if (ih->expand & IUP_EXPAND_W1)
      empty_w1 = iHboxCalcEmptyWidth(ih, IUP_EXPAND_W1);
    /* Not W1 and W0 means that EXPAND is not enabled but there are some IupFill inside */
    else if (ih->expand & IUP_EXPAND_W0)
      empty_w0 = iHboxCalcEmptyWidth(ih, IUP_EXPAND_W0);
  }

  client_height = ih->currentheight - 2*ih->data->margin_vert;
  if (client_height<0) client_height=0;

  for (child = ih->firstchild; child; child = child->brother)
  {
    if (!(child->flags & IUP_FLOATING))
    {
      if (ih->data->homogeneous_size)
        iupBaseSetCurrentSize(child, ih->data->homogeneous_size, client_height, shrink);
      else
      {
        int empty = (child->expand & IUP_EXPAND_W1)? empty_w1: ((child->expand & IUP_EXPAND_W0)? empty_w0: 0);
        char* weight_str = iupAttribGet(child, "EXPANDWEIGHT");
        if (weight_str)
        {
          double weight;
          if (iupStrToDouble(weight_str, &weight))
            empty = iupRound(empty * weight);
        }
        iupBaseSetCurrentSize(child, child->naturalwidth+empty, client_height, shrink);
      }
    }
    else if (!(child->flags & IUP_FLOATING_IGNORE))
    {
      /* update children to their own natural size */
      iupBaseSetCurrentSize(child, child->naturalwidth, child->naturalheight, shrink);
    }
  }
}

static void iHboxSetChildrenPositionMethod(Ihandle* ih, int x, int y)
{
  int dy, client_height;
  Ihandle* child;

  x += ih->data->margin_horiz;
  y += ih->data->margin_vert;

  client_height = ih->currentheight - 2*ih->data->margin_vert;
  if (client_height<0) client_height=0;

  for (child = ih->firstchild; child; child = child->brother)
  {
    if (!(child->flags & IUP_FLOATING))
    {
      if (ih->data->alignment == IUP_ALIGN_ACENTER)
        dy = (client_height - child->currentheight)/2;
      else if (ih->data->alignment == IUP_ALIGN_ABOTTOM)
        dy = client_height - child->currentheight;
      else  /* IUP_ALIGN_ATOP */
        dy = 0;
      if (dy<0) dy = 0;
                      
      /* update child */
      iupBaseSetPosition(child, x, y+dy);

      /* calculate next */
      if (ih->data->homogeneous_size)
        x += ih->data->homogeneous_size + ih->data->gap;
      else
        x += child->currentwidth + ih->data->gap;
    }
  }
}


/******************************************************************************/


IUP_API Ihandle* IupHboxv(Ihandle **children)
{
  return IupCreatev("hbox", (void**)children);
}

IUP_API Ihandle* IupHboxV(Ihandle* child, va_list arglist)
{
  return IupCreateV("hbox", child, arglist);
}

IUP_API Ihandle* IupHbox(Ihandle* child, ...)
{
  Ihandle *ih;

  va_list arglist;
  va_start(arglist, child);
  ih = IupCreateV("hbox", child, arglist);
  va_end(arglist);

  return ih;
}

Iclass* iupHboxNewClass(void)
{
  Iclass* ic = iupBoxNewClassBase();

  ic->name = "hbox";

  /* Class functions */
  ic->New = iupHboxNewClass;
  ic->ComputeNaturalSize = iHboxComputeNaturalSizeMethod;
  ic->SetChildrenCurrentSize = iHboxSetChildrenCurrentSizeMethod;
  ic->SetChildrenPosition = iHboxSetChildrenPositionMethod;

  /* Overwrite Common */
  iupClassRegisterAttribute(ic, "SIZE", iupBaseGetSizeAttrib, iHboxSetSizeAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RASTERSIZE", iupBaseGetRasterSizeAttrib, iHboxSetRasterSizeAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ORIENTATION", iHboxGetOrientationAttrib, NULL, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  /* Hbox only */
  iupClassRegisterAttribute(ic, "ALIGNMENT", iHboxGetAlignmentAttrib, iHboxSetAlignmentAttrib, IUPAF_SAMEASSYSTEM, "ATOP", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  return ic;
}
