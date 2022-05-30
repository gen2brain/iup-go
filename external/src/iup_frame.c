/** \file
 * \brief Frame Control.
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
#include "iup_frame.h"


int iupFrameGetTitleHeight(Ihandle* ih)
{
  int height;
  if (iupdrvFrameGetTitleHeight(ih, &height))
    return height;

  iupdrvFontGetCharSize(ih, NULL, &height);
  return height;
}

static void iFrameGetDecorSize(Ihandle* ih, int *width, int *height)
{
  if (iupdrvFrameGetDecorSize(ih, width, height))
    return;

  *width = 5;
  *height = 5;

  if (iupAttribGet(ih, "_IUPFRAME_HAS_TITLE") || iupAttribGet(ih, "TITLE"))
    (*height) += iupFrameGetTitleHeight(ih);
}

char* iupFrameGetBgColorAttrib(Ihandle* ih)
{
  if (iupAttribGet(ih, "_IUPFRAME_HAS_BGCOLOR"))
    return NULL;  /* get from the hash table */
  else
    return iupBaseNativeParentGetBgColorAttrib(ih);
}

static char* iFrameGetClientSizeAttrib(Ihandle* ih)
{
  int width, height, decorwidth, decorheight;
  width = ih->currentwidth;
  height = ih->currentheight;
  iFrameGetDecorSize(ih, &decorwidth, &decorheight);
  width -= decorwidth;
  height -= decorheight;
  if (width < 0) width = 0;
  if (height < 0) height = 0;
  return iupStrReturnIntInt(width, height, 'x');
}

static char* iFrameGetClientOffsetAttrib(Ihandle* ih)
{
  int dx = 0, dy = 0;

  /* In Windows the position of the child is still
  relative to the top-left corner of the frame.
  So we must manually add the decorations. */
  if (!iupdrvFrameHasClientOffset(ih))
  {
    /* GTK and Motif Only */

    iupdrvFrameGetDecorOffset(ih, &dx, &dy);

    if (iupAttribGet(ih, "_IUPFRAME_HAS_TITLE") || iupAttribGet(ih, "TITLE"))
      dy += iupFrameGetTitleHeight(ih);
  }

  return iupStrReturnIntInt(dx, dy, 'x');
}

static int iFrameCreateMethod(Ihandle* ih, void** params)
{
  if (params)
  {
    Ihandle** iparams = (Ihandle**)params;
    if (*iparams)
      IupAppend(ih, *iparams);
  }

  return IUP_NOERROR;
}

static void iFrameComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  int decorwidth, decorheight;
  Ihandle* child = ih->firstchild;

  iFrameGetDecorSize(ih, &decorwidth, &decorheight);
  *w = decorwidth;
  *h = decorheight;

  if (child)
  {
    /* update child natural size first */
    iupBaseComputeNaturalSize(child);

    *children_expand = child->expand;
    *w += child->naturalwidth;
    *h += child->naturalheight;
  }
}

static void iFrameSetChildrenCurrentSizeMethod(Ihandle* ih, int shrink)
{
  int width, height, decorwidth, decorheight;

  iFrameGetDecorSize(ih, &decorwidth, &decorheight);

  width = ih->currentwidth-decorwidth;
  height = ih->currentheight-decorheight;
  if (width < 0) width = 0;
  if (height < 0) height = 0;

  if (ih->firstchild)
    iupBaseSetCurrentSize(ih->firstchild, width, height, shrink);
}

static void iFrameSetChildrenPositionMethod(Ihandle* ih, int x, int y)
{
  if (ih->firstchild)
  {
    char* offset = iupAttribGet(ih, "CHILDOFFSET");

    /* Native container, position is reset */
    x = 0;
    y = 0;

    if (offset) iupStrToIntInt(offset, &x, &y, 'x');

    /* In Windows the position of the child is still
    relative to the top-left corner of the frame.
    So we must manually add the decorations. */
    if (iupdrvFrameHasClientOffset(ih))
    {
      /* Windows Only */
      int dx = 0, dy = 0;
      iupdrvFrameGetDecorOffset(ih, &dx, &dy);

      if (iupAttribGet(ih, "_IUPFRAME_HAS_TITLE") || iupAttribGet(ih, "TITLE"))
        dy += iupFrameGetTitleHeight(ih);

      x += dx;
      y += dy;
    }

    /* Child coordinates are relative to client left-top corner. */
    iupBaseSetPosition(ih->firstchild, x, y);
  }
}


/******************************************************************************/


IUP_API Ihandle* IupFrame(Ihandle* child)
{
  void *children[2];
  children[0] = (void*)child;
  children[1] = NULL;
  return IupCreatev("frame", children);
}

Iclass* iupFrameNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "frame";
  ic->format = "h"; /* one Ihandle* */
  ic->nativetype = IUP_TYPECONTROL;
  ic->childtype = IUP_CHILDMANY+1;   /* 1 child */
  ic->is_interactive = 0;

  /* Class functions */
  ic->New = iupFrameNewClass;
  ic->Create = iFrameCreateMethod;

  ic->ComputeNaturalSize = iFrameComputeNaturalSizeMethod;
  ic->SetChildrenCurrentSize = iFrameSetChildrenCurrentSizeMethod;
  ic->SetChildrenPosition = iFrameSetChildrenPositionMethod;

  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;

  /* Common Callbacks */
  iupClassRegisterCallback(ic, "MAP_CB", "");
  iupClassRegisterCallback(ic, "UNMAP_CB", "");
  iupClassRegisterCallback(ic, "FOCUS_CB", "i");

  /* Common */
  iupBaseRegisterCommonAttrib(ic);

  /* Visual */
  iupBaseRegisterVisualAttrib(ic);

  /* Base Container */
  iupClassRegisterAttribute(ic, "CLIENTSIZE", iFrameGetClientSizeAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTOFFSET", iFrameGetClientOffsetAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EXPAND", iupBaseContainerGetExpandAttrib, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  /* Native Container */
  iupClassRegisterAttribute(ic, "CHILDOFFSET", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  /* IupFrame only */
  iupClassRegisterAttribute(ic, "SUNKEN", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupdrvFrameInitClass(ic);

  return ic;
}
