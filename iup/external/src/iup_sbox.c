/** \file
 * \brief iupsbox control
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
#include "iup_childtree.h"



enum { ISBOX_NORTH, ISBOX_SOUTH, ISBOX_WEST, ISBOX_EAST };

struct _IcontrolData
{
  int w, h;
  int isholding;
  int start_x, start_y;
  int start_w, start_h;

  int direction;     /* one of the types: ISBOX_NORTH, ISBOX_SOUTH, ISBOX_WEST, ISBOX_EAST */
  int showgrip, layoutdrag;
};


static int iSboxGetYborder(Ihandle* ih, int bar_size)
{
  if (ih->data->direction == ISBOX_NORTH || ih->data->direction == ISBOX_SOUTH)
    return bar_size;
  else
    return 0;
}

static int iSboxGetXborder(Ihandle* ih, int bar_size)
{
  if (ih->data->direction == ISBOX_EAST || ih->data->direction == ISBOX_WEST)
    return bar_size;
  else
    return 0;
}

static void iSboxSaveDimension(Ihandle* ih, int w, int h)
{
  ih->data->w = w;
  ih->data->h = h;
  iupLayoutApplyMinMaxSize(ih, &(ih->data->w), &(ih->data->h));
}

static void iSboxAddDecorOffset(Ihandle* ih, int *x, int *y, int bar_size)
{
  /* skip north thumb if there is one */
  if (ih->data->direction == ISBOX_NORTH)
    *y += bar_size;

  /* skip west thumb if there is one */
  if (ih->data->direction == ISBOX_WEST)
    *x += bar_size;
}

static void iSboxGetFinalSize(Ihandle* ih, int direction, int *w, int *h)
{
  int final_x, final_y;
  int diff_x, diff_y;

  iupStrToIntInt(IupGetGlobal("CURSORPOS"), &final_x, &final_y, 'x');

  diff_x = final_x - ih->data->start_x;
  diff_y = final_y - ih->data->start_y;

  if(direction == ISBOX_WEST)
    diff_x = -diff_x;

  if(direction == ISBOX_NORTH)
    diff_y = -diff_y;

  *w = diff_x + ih->data->start_w;
  *h = diff_y + ih->data->start_h;
}


/*****************************************************************************\
|* Callbacks of canvas bar                                                   *|
\*****************************************************************************/


static int iSboxMotion_CB(Ihandle* bar, int x, int y, char *r)
{
  Ihandle* ih = bar->parent;

  if (ih->data->isholding)
  {
    int new_w, new_h;

    iSboxGetFinalSize(ih, ih->data->direction, &new_w, &new_h);

    if (ih->data->direction == ISBOX_WEST || ih->data->direction == ISBOX_EAST)
    {
      if (new_w != ih->data->w)
      {
        iSboxSaveDimension(ih, new_w, ih->data->h);
        if (ih->data->layoutdrag)
        {
          IupRefresh(ih);  /* may affect all the elements in the dialog */
          IupFlush();
        }
      }
    }
    else if (ih->data->direction == ISBOX_SOUTH || ih->data->direction == ISBOX_NORTH)
    {
      if (new_h != ih->data->h)
      {
        iSboxSaveDimension(ih, ih->data->w, new_h);
        if (ih->data->layoutdrag)
        {
          IupRefresh(ih);  /* may affect all the elements in the dialog */
          IupFlush();
        }
      }
    }
  }

  (void)x;
  (void)y;
  (void)r;
  return IUP_DEFAULT;
}

static int iSboxButton_CB(Ihandle* bar, int button, int pressed, int x, int y, char* status)
{
  Ihandle* ih = bar->parent;

  if (button!=IUP_BUTTON1)
    return IUP_DEFAULT;

  if (!ih->data->isholding && pressed)
  {
    ih->data->isholding = 1;

    /* Save the cursor position */
    iupStrToIntInt(IupGetGlobal("CURSORPOS"), &ih->data->start_x, &ih->data->start_y, 'x');

    /* Save the initial size */
    ih->data->start_w = ih->data->w;
    ih->data->start_h = ih->data->h;
  }
  else if (ih->data->isholding && !pressed)
    ih->data->isholding = 0;

  (void)x;
  (void)y;
  (void)status;
  return IUP_DEFAULT;
}

static int iSboxFocus_CB(Ihandle* bar, int focus)
{
  Ihandle* ih = bar->parent;

  if (!ih || focus) /* use only kill focus */
    return IUP_DEFAULT;

  if (ih->data->isholding)
    ih->data->isholding = 0;

  return IUP_DEFAULT;
}


/*****************************************************************************\
|* Attributes                                                                *|
\*****************************************************************************/


static char* iSboxGetClientSizeAttrib(Ihandle* ih)
{
  int bar_size = IupGetInt(ih->firstchild, "BARSIZE");
  int width = ih->currentwidth - iSboxGetXborder(ih, bar_size);
  int height = ih->currentheight - iSboxGetYborder(ih, bar_size);
  if (width < 0) width = 0;
  if (height < 0) height = 0;
  return iupStrReturnIntInt(width, height, 'x');
}

static int iSboxSetBarSizeAttrib(Ihandle* ih, const char* value)
{
  IupSetAttribute(ih->firstchild, "BARSIZE", value);
  return 0;  /* do not store value in hash table */
}

static char* iSboxGetBarSizeAttrib(Ihandle* ih)
{
  return IupGetAttribute(ih->firstchild, "BARSIZE");
}

static int iSboxSetColorAttrib(Ihandle* ih, const char* value)
{
  IupSetAttribute(ih->firstchild, "COLOR", value);
  return 0;  /* do not store value in hash table */
}

static char* iSboxGetColorAttrib(Ihandle* ih)
{
  return IupGetAttribute(ih->firstchild, "COLOR");
}

static int iSboxSetDirectionAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle) /* only before map */
    return 0;

  if (iupStrEqualNoCase(value, "NORTH"))
    ih->data->direction = ISBOX_NORTH;
  else if(iupStrEqualNoCase(value, "SOUTH"))
    ih->data->direction = ISBOX_SOUTH;
  else if(iupStrEqualNoCase(value, "WEST"))
    ih->data->direction = ISBOX_WEST;
  else  /* Default = EAST */
    ih->data->direction = ISBOX_EAST;

  if (ih->data->direction == ISBOX_EAST || ih->data->direction == ISBOX_WEST)
  {
    IupSetAttribute(ih->firstchild, "ORIENTATION", "VERTICAL");
    IupSetAttribute(ih->firstchild, "CURSOR", "RESIZE_WE");
  }
  else
  {
    IupSetAttribute(ih->firstchild, "ORIENTATION", "HORIZONTAL");
    IupSetAttribute(ih->firstchild, "CURSOR", "RESIZE_NS");
  }

  return 0;  /* do not store value in hash table */
}

static int iSboxSetShowGripAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
  {
    ih->data->showgrip = 1;
    IupSetAttribute(ih->firstchild, "STYLE", "GRIP");
  }
  else
  {
    if (iupStrEqualNoCase(value, "LINES"))
    {
      IupSetAttribute(ih->firstchild, "STYLE", "DUALLINES");
      ih->data->showgrip = 2;
    }
    else
    {
      IupSetAttribute(ih->firstchild, "STYLE", "FILL");
      ih->data->showgrip = 0;
    }
  }

  return 0; /* do not store value in hash table */
}

static char* iSboxGetShowGripAttrib(Ihandle* ih)
{
  if (ih->data->showgrip == 2)
    return "LINES";
  else
    return iupStrReturnBoolean(ih->data->showgrip);
}

static int iSboxSetLayoutDragAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->layoutdrag = 1;
  else
    ih->data->layoutdrag = 0;

  return 0; /* do not store value in hash table */
}

static char* iSboxGetLayoutDragAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->layoutdrag);
}


/*****************************************************************************\
|* Methods                                                                   *|
\*****************************************************************************/


static void iSboxComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  int natural_w = ih->naturalwidth, 
      natural_h = ih->naturalheight;
  int bar_size = IupGetInt(ih->firstchild, "BARSIZE");

  /* only allow expand in the opposite direction, complement iupBaseContainerUpdateExpand */
  if (ih->data->direction == ISBOX_EAST || ih->data->direction == ISBOX_WEST)
      ih->expand &= ~IUP_EXPAND_WIDTH;
  else 
      ih->expand &= ~IUP_EXPAND_HEIGHT;

  /* always has at least one child, the bar */

  /* This is an unusual element, the iupBaseComputeNaturalSize logic is done twice, one here and one back there. */

  if (ih->firstchild->brother)
  {
    Ihandle* child = ih->firstchild->brother;

    /* update child natural size first */
    iupBaseComputeNaturalSize(child);

    *children_expand = child->expand;

    /* calculate as in iupBaseComputeNaturalSize */
    natural_w = iupMAX(natural_w, child->naturalwidth + iSboxGetXborder(ih, bar_size));
    natural_h = iupMAX(natural_h, child->naturalheight + iSboxGetYborder(ih, bar_size));
  }

  /* update control to fit its children according to direction */

  /* bar */
  if (ih->data->direction == ISBOX_EAST || ih->data->direction == ISBOX_WEST)
  {
    ih->data->w = iupMAX(natural_w, ih->data->w);
    ih->data->h = natural_h;
  }
  else  /* ISBOX_NORTH || ISBOX_SOUTH */
  {
    ih->data->w = natural_w;
    ih->data->h = iupMAX(natural_h, ih->data->h);
  }

  /* child */
  if (ih->firstchild->brother)
  {
    Ihandle* child = ih->firstchild->brother;
    child->naturalwidth = ih->data->w - iSboxGetXborder(ih, bar_size);
    child->naturalheight = ih->data->h - iSboxGetYborder(ih, bar_size);
  }

  *w = ih->data->w;
  *h = ih->data->h;
}

static void iSboxSetChildrenCurrentSizeMethod(Ihandle* ih, int shrink)
{
  int bar_size = IupGetInt(ih->firstchild, "BARSIZE");

  /* bar */
  if ((ih->data->direction == ISBOX_NORTH || ih->data->direction == ISBOX_SOUTH))
  {
    ih->firstchild->currentwidth  = ih->currentwidth;
    ih->firstchild->currentheight = bar_size;
  }
  else
  {
    ih->firstchild->currentwidth = bar_size;
    ih->firstchild->currentheight = ih->currentheight;
  }

  /* child */
  if (ih->firstchild->brother)
  {
    int width = ih->currentwidth - iSboxGetXborder(ih, bar_size);
    int height = ih->currentheight - iSboxGetYborder(ih, bar_size);
    if (width < 0) width = 0;
    if (height < 0) height = 0;

    iupBaseSetCurrentSize(ih->firstchild->brother, width, height, shrink);
  }
}

static void iSboxSetChildrenPositionMethod(Ihandle* ih, int x, int y)
{
  int posx = 0, posy = 0;
  int bar_size = IupGetInt(ih->firstchild, "BARSIZE");

  /* bar */
  if (ih->data->direction == ISBOX_EAST)
  {
    posx = ih->data->w - bar_size;
    if (posx<0) posx = 0;
  }
  if (ih->data->direction == ISBOX_SOUTH)
  {
    posy = ih->data->h - bar_size;
    if (posy<0) posy = 0;
  }

  iupBaseSetPosition(ih->firstchild, x+posx, y+posy);

  /* child */
  if (ih->firstchild->brother)
  {  
    iSboxAddDecorOffset(ih, &x, &y, bar_size);
    iupBaseSetPosition(ih->firstchild->brother, x, y);
  } 
}

static int iSboxCreateMethod(Ihandle* ih, void** params)
{
  Ihandle* bar;

  ih->data = iupALLOCCTRLDATA();

  ih->data->direction = ISBOX_EAST;
  ih->data->h = -1;
  ih->data->w = -1;
  ih->data->layoutdrag = 1;

  bar = IupFlatSeparator();
  bar->flags |= IUP_INTERNAL;
  iupChildTreeAppend(ih, bar);  /* bar will always be the firstchild */

  IupSetAttribute(bar, "EXPAND", "NO");
  IupSetAttribute(bar, "CURSOR", "RESIZE_WE");
  IupSetAttribute(bar, "ORIENTATION", "VERTICAL");
  IupSetAttribute(bar, "STYLE", "FILL");
  IupSetAttribute(bar, "COLOR", "160 160 160");

  /* Setting callbacks */
  IupSetCallback(bar, "BUTTON_CB", (Icallback) iSboxButton_CB);
  IupSetCallback(bar, "FOCUS_CB",  (Icallback) iSboxFocus_CB);
  IupSetCallback(bar, "MOTION_CB", (Icallback) iSboxMotion_CB);

  if (params)
  {
    Ihandle** iparams = (Ihandle**)params;
    if (*iparams)
      IupAppend(ih, *iparams);
  }

  return IUP_NOERROR;
}

Iclass* iupSboxNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name   = "sbox";
  ic->format = "h";   /* one Ihandle* */
  ic->nativetype = IUP_TYPEVOID;
  ic->childtype = IUP_CHILDMANY+2; /* canvas+child */
  ic->is_interactive = 0;

  /* Class functions */
  ic->New = iupSboxNewClass;
  ic->Create  = iSboxCreateMethod;
  ic->Map     = iupBaseTypeVoidMapMethod;

  ic->ComputeNaturalSize = iSboxComputeNaturalSizeMethod;
  ic->SetChildrenCurrentSize     = iSboxSetChildrenCurrentSizeMethod;
  ic->SetChildrenPosition        = iSboxSetChildrenPositionMethod;

  /* Base Callbacks */
  iupBaseRegisterBaseCallbacks(ic);

  /* Common */
  iupBaseRegisterCommonAttrib(ic);

  /* Base Container */
  iupClassRegisterAttribute(ic, "CLIENTSIZE", iSboxGetClientSizeAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTOFFSET", iupBaseGetClientOffsetAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EXPAND", iupBaseContainerGetExpandAttrib, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  /* IupSbox only */
  iupClassRegisterAttribute(ic, "COLOR", iSboxGetColorAttrib, iSboxSetColorAttrib, IUPAF_SAMEASSYSTEM, "160 160 160", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DIRECTION", NULL, iSboxSetDirectionAttrib, IUPAF_SAMEASSYSTEM, "EAST", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWGRIP", iSboxGetShowGripAttrib, iSboxSetShowGripAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BARSIZE", iSboxGetBarSizeAttrib, iSboxSetBarSizeAttrib, IUPAF_SAMEASSYSTEM, "5", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LAYOUTDRAG", iSboxGetLayoutDragAttrib, iSboxSetLayoutDragAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  return ic;
}

IUP_API Ihandle* IupSbox(Ihandle* child)
{
  void *children[2];
  children[0] = (void*)child;
  children[1] = NULL;
  return IupCreatev("sbox", children);
}
