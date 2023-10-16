/** \file
 * \brief IupBackgroundBox control
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
#include "iup_register.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_stdcontrols.h"
#include "iup_layout.h"
#include "iup_drv.h"
#include "iup_drvdraw.h"
#include "iup_draw.h"


/*****************************************************************************\
|* Methods                                                                   *|
\*****************************************************************************/

static char* iBackgroundBoxGetBgColorAttrib(Ihandle* ih)
{
  if (iupAttribGet(ih, "BGCOLOR"))
    return NULL;  /* get from the hash table */
  else
    return iupBaseNativeParentGetBgColorAttrib(ih);
}

static int iBackgroundBoxRedrawBackImage_CB(Ihandle* ih)
{                    
  char* bgimage = iupAttribGet(ih, "BACKIMAGE");
  char* bgcolor = iupAttribGet(ih, "BACKCOLOR");
  int backimage_zoom = iupAttribGetBoolean(ih, "BACKIMAGEZOOM");
  IdrawCanvas* dc = iupdrvDrawCreateCanvas(ih);

  if (!bgcolor)
    bgcolor = iBackgroundBoxGetBgColorAttrib(ih);

  iupFlatDrawBox(dc, 0, ih->currentwidth - 1, 0, ih->currentheight - 1, bgcolor, NULL, 1);

  if (backimage_zoom)
    iupdrvDrawImage(dc, bgimage, 0, bgcolor, 0, 0, ih->currentwidth, ih->currentheight);
  else
    iupdrvDrawImage(dc, bgimage, 0, bgcolor, 0, 0, -1, -1);

  iupdrvDrawFlush(dc);

  iupdrvDrawKillCanvas(dc);

  return IUP_DEFAULT;
}

static int iBackgroundBoxSetBackImageAttrib(Ihandle* ih, const char* value)
{
  if (value && value[0] != 0)
    IupSetCallback(ih, "ACTION", iBackgroundBoxRedrawBackImage_CB);
  else
    IupSetCallback(ih, "ACTION", NULL);

  IupUpdate(ih); /* post a redraw */
  return 1;  /* save on the hash table */
}

static char* iBackgroundBoxGetExpandAttrib(Ihandle* ih)
{
  if (iupAttribGetBoolean(ih, "CANVASBOX"))
    return iupBaseGetExpandAttrib(ih);
  else
    return iupBaseContainerGetExpandAttrib(ih);
}

static int iBackgroundBoxSetExpandAttrib(Ihandle* ih, const char* value)
{
  if (iupAttribGetBoolean(ih, "CANVASBOX"))
    return iupBaseSetExpandAttrib(ih, value);
  else
    return 1;  /* store on the hash table */
}

static int iBackgroundBoxGetBorder(Ihandle* ih)
{
  if (iupAttribGetBoolean(ih, "BORDER"))
    return 1;
  else
    return 0;
}

static char* iBackgroundBoxGetClientSizeAttrib(Ihandle* ih)
{
  int border = iBackgroundBoxGetBorder(ih);
  int width = ih->currentwidth - 2 * border;
  int height = ih->currentheight - 2 * border;

  if (iupAttribGetBoolean(ih, "DECORATION"))
  {
    int decorwidth = 0, decorheight = 0;
    IupGetIntInt(ih, "DECORSIZE", &decorwidth, &decorheight);
    width -= decorwidth;
    height -= decorheight;
  }

  if (width < 0) width = 0;
  if (height < 0) height = 0;

  return iupStrReturnIntInt(width, height, 'x');
}

static void iBackgroundBoxComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  if (iupAttribGetBoolean(ih, "CANVASBOX"))
  {
    /* Also set expand to its own expand so it will not depend on children */
    *children_expand = ih->expand;
  }

  if (ih->firstchild)
  {
    /* update child natural size first */
    iupBaseComputeNaturalSize(ih->firstchild);

    if (!iupAttribGetBoolean(ih, "CANVASBOX"))
    {
      int border = iBackgroundBoxGetBorder(ih);
      int width = ih->firstchild->naturalwidth + 2 * border;
      int height = ih->firstchild->naturalheight + 2 * border;

      if (iupAttribGetBoolean(ih, "DECORATION"))
      {
        int decorwidth = 0, decorheight = 0;
        IupGetIntInt(ih, "DECORSIZE", &decorwidth, &decorheight);
        width += decorwidth;
        height += decorheight;
      }

      *children_expand = ih->firstchild->expand;
      *w = width;
      *h = height;
    }
  }
}

static void iBackgroundBoxSetChildrenCurrentSizeMethod(Ihandle* ih, int shrink)
{
  if (ih->firstchild)
  {
    Ihandle* child = ih->firstchild;

    if (iupAttribGetBoolean(ih, "CANVASBOX"))
    {
      /* update child to its own natural size */
      iupBaseSetCurrentSize(child, child->naturalwidth, child->naturalheight, shrink);
    }
    else
    {
      int border = iBackgroundBoxGetBorder(ih);
      int width = ih->currentwidth - 2 * border;
      int height = ih->currentheight - 2 * border;

      if (iupAttribGetBoolean(ih, "DECORATION"))
      {
        int decorwidth = 0, decorheight = 0;
        IupGetIntInt(ih, "DECORSIZE", &decorwidth, &decorheight);
        width -= decorwidth;
        height -= decorheight;
      }

      if (width < 0) width = 0;
      if (height < 0) height = 0;

      iupBaseSetCurrentSize(child, width, height, shrink);
    }
  }
}

static void iBackgroundBoxSetChildrenPositionMethod(Ihandle* ih, int x, int y)
{
  if (ih->firstchild)
  {
    char* offset = iupAttribGet(ih, "CHILDOFFSET");

    /* Native container, position is reset */
    x = 0;
    y = 0;

    if (offset) iupStrToIntInt(offset, &x, &y, 'x');

    if (iupAttribGetBoolean(ih, "DECORATION"))
    {
      int decor_x = 0, decor_y = 0;
      IupGetIntInt(ih, "DECOROFFSET", &decor_x, &decor_y);
      x += decor_x;
      y += decor_y;
    }

    iupBaseSetPosition(ih->firstchild, x, y);
  }
}

static int iBackgroundBoxCreateMethod(Ihandle* ih, void** params)
{
  if (params)
  {
    Ihandle** iparams = (Ihandle**)params;
    if (iparams[0]) IupAppend(ih, iparams[0]);
  }

  return IUP_NOERROR;
}

IUP_SDK_API Iclass* iupBackgroundBoxNewBaseClass(Iclass* ic_base)   /* Used by BackgroundBox and GLBackgroundBox */
{
  Iclass* ic = iupClassNew(ic_base);

  ic->format = "h";   /* one Ihandle* */
  ic->nativetype = IUP_TYPECANVAS;
  ic->childtype = IUP_CHILDMANY+1;  /* 1 child */
  ic->is_interactive = 1;

  /* Class functions */
  ic->New = iupBackgroundBoxNewClass;
  ic->Create  = iBackgroundBoxCreateMethod;

  ic->ComputeNaturalSize = iBackgroundBoxComputeNaturalSizeMethod;
  ic->SetChildrenCurrentSize = iBackgroundBoxSetChildrenCurrentSizeMethod;
  ic->SetChildrenPosition = iBackgroundBoxSetChildrenPositionMethod;

  /* Base Container */
  iupClassRegisterAttribute(ic, "EXPAND", iBackgroundBoxGetExpandAttrib, iBackgroundBoxSetExpandAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTOFFSET", iupBaseCanvasGetClientOffsetAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTSIZE", iBackgroundBoxGetClientSizeAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  /* Native Container */
  iupClassRegisterAttribute(ic, "CHILDOFFSET", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  /* replace IupCanvas behavior */
  iupClassRegisterReplaceAttribFunc(ic, "BGCOLOR", iBackgroundBoxGetBgColorAttrib, NULL);
  iupClassRegisterReplaceAttribDef(ic, "BGCOLOR", "DLGBGCOLOR", NULL);

  iupClassRegisterReplaceAttribDef  (ic, "BORDER", "NO", NULL);
  iupClassRegisterReplaceAttribFlags(ic, "BORDER", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CANFOCUS", NULL, NULL, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NO_INHERIT);

  /* New */
  iupClassRegisterAttribute(ic, "CANVASBOX", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DECORATION", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DECORSIZE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DECOROFFSET", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BACKIMAGE", NULL, iBackgroundBoxSetBackImageAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKIMAGEZOOM", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKCOLOR", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  return ic;
}

Iclass* iupBackgroundBoxNewClass(void)
{
  Iclass* ic = iupBackgroundBoxNewBaseClass(iupRegisterFindClass("canvas"));

  ic->name = "backgroundbox";
  ic->cons = "BackgroundBox";

  return ic;
}

IUP_API Ihandle* IupBackgroundBox(Ihandle* child)
{
  void *children[2];
  children[0] = (void*)child;
  children[1] = NULL;
  return IupCreatev("backgroundbox", children);
}
