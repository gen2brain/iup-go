/** \file
 * \brief iupsplit control
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


enum { ISPLIT_VERT, ISPLIT_HORIZ };
enum { ISPLIT_HIDE, ISPLIT_SHOW };

struct _IcontrolData
{
  /* aux */
  int is_holding;
  int start_pos, start_bar, start_size;

  /* attributes */
  int layoutdrag, autohide, showgrip, barsize;
  int orientation;  /* one of the types: ISPLIT_VERT, ISPLIT_HORIZ */
  int val;  /* split value: 0-1000, default 500 */
  int min, max;  /* used only to crop val */
};


static void iSplitAutoHideChild(Ihandle* child, int flag)
{
  if (flag==ISPLIT_HIDE)
  {
    if (IupGetInt(child, "VISIBLE"))
    {
      IupSetAttribute(child, "FLOATING", "IGNORE");
      IupSetAttribute(child, "VISIBLE", "NO");
    }
  }
  else 
  {
    if (!IupGetInt(child, "VISIBLE"))
    {
      IupSetAttribute(child, "FLOATING", "NO");
      IupSetAttribute(child, "VISIBLE", "YES");
    }
  }
}

static void iSplitAutoHide(Ihandle* ih)
{
  Ihandle *child1 = ih->firstchild->brother;
  if (child1)
  {
    int tol;
    Ihandle *child2 = child1->brother;

    if (ih->data->orientation == ISPLIT_VERT)
    {
      if (ih->currentwidth <= ih->data->barsize)
        tol = 50;
      else
        tol = (1000*ih->data->barsize)/ih->currentwidth;
    }
    else
    {
      if (ih->currentheight <= ih->data->barsize)
        tol = 50;
      else
        tol = (1000 * ih->data->barsize) / ih->currentheight;
    }

    iSplitAutoHideChild(child1, ih->data->val<=tol? ISPLIT_HIDE: ISPLIT_SHOW);

    if (child2)
      iSplitAutoHideChild(child2, ih->data->val>=(1000-tol)? ISPLIT_HIDE: ISPLIT_SHOW);
  }
}

static int iupRoundUp(double d)
{
  return (int)ceil(d);
}

static int iSplitGetWidth1(Ihandle* ih)
{
  /* This is the space available for the child,
     It does NOT depends on the child. */
  int width1 = iupRoundUp(((ih->currentwidth - ih->data->barsize)*ih->data->val) / 1000.0);
  if (width1 < 0) width1 = 0;
  return width1;
}

static int iSplitGetWidth2(Ihandle* ih, int width1)
{
  /* This is the space available for the child,
     It does NOT depends on the child. */
  int width2 = (ih->currentwidth-ih->data->barsize) - width1;
  if (width2 < 0) width2 = 0;
  return width2;
}

static int iSplitGetHeight1(Ihandle* ih)
{
  /* This is the space available for the child,
     It does NOT depends on the child. */
  int height1 = iupRoundUp(((ih->currentheight - ih->data->barsize)*ih->data->val) / 1000.0);
  if (height1 < 0) height1 = 0;
  return height1;
}

static int iSplitGetHeight2(Ihandle* ih, int height1)
{
  /* This is the space available for the child,
     It does NOT depends on the child. */
  int height2 = (ih->currentheight-ih->data->barsize) - height1;
  if (height2 < 0) height2 = 0;
  return height2;
}

static void iSplitCalcVal(Ihandle* ih, int size1)
{
  if (ih->data->orientation == ISPLIT_VERT)
    ih->data->val = (size1 * 1000) / (ih->currentwidth - ih->data->barsize);
  else
    ih->data->val = (size1 * 1000) / (ih->currentheight - ih->data->barsize);
}

static void iSplitAdjustVal(Ihandle* ih)
{
  if (ih->data->val < ih->data->min) 
    ih->data->val = ih->data->min;
  if (ih->data->val > ih->data->max) 
    ih->data->val = ih->data->max;

  if (ih->data->autohide)
    iSplitAutoHide(ih);  
}

static int iSplitAdjustWidth1(Ihandle* ih, int *width1)
{
  Ihandle *child1 = ih->firstchild->brother;
  if (child1)
  {
    Ihandle *child2 = child1->brother;

    int min_width1 = *width1;
    iupLayoutApplyMinMaxSize(child1, &min_width1, NULL);
    if (min_width1 > *width1)
    {
      *width1 = min_width1;  /* minimum value for width1 */
      return 1;
    }

    if (child2)
    {
      int width2 = iSplitGetWidth2(ih, *width1);
      int min_width2 = width2;
      iupLayoutApplyMinMaxSize(child2, &min_width2, NULL);
      if (min_width2 > width2)
      {
        width2 = min_width2;  /* minimum value for width2 */
        *width1 = (ih->currentwidth-ih->data->barsize) - width2;  /* maximum value for width1 */
        return 1;
      }
    }
  }
  return 0;
}

static int iSplitAdjustHeight1(Ihandle* ih, int *height1)
{
  Ihandle *child1 = ih->firstchild->brother;
  if (child1)
  {
    Ihandle *child2 = child1->brother;

    int min_height1 = *height1;
    iupLayoutApplyMinMaxSize(child1, NULL, &min_height1);
    if (min_height1 > *height1)
    {
      *height1 = min_height1;  /* minimum value for height1 */
      return 1;
    }

    if (child2)
    {
      int height2 = iSplitGetHeight2(ih, *height1);
      int min_height2 = height2;
      iupLayoutApplyMinMaxSize(child2, NULL, &min_height2);
      if (min_height2 > height2)
      {
        height2 = min_height2;  /* minimum value for height2 */
        *height1 = (ih->currentheight-ih->data->barsize) - height2;  /* maximum value for height1 */
        return 1;
      }
    }
  }
  return 0;
}

static void iSplitSetBarPosition(Ihandle* ih)
{
  /* Update only the bar position, 
     used only when LAYOUTDRAG=NO */
  int x = ih->x, 
      y = ih->y;

  if (ih->data->orientation == ISPLIT_VERT)
  {
    /* bar */
    x += iSplitGetWidth1(ih);
    iupBaseSetPosition(ih->firstchild, x, y);
  }
  else /* ISPLIT_HORIZ */
  {
    /* bar */
    y += iSplitGetHeight1(ih);
    iupBaseSetPosition(ih->firstchild, x, y);
  }

  IupSetAttribute(ih->firstchild, "ZORDER", "TOP");
  iupClassObjectLayoutUpdate(ih->firstchild);
}


/*****************************************************************************\
|* Callbacks of canvas bar                                                   *|
\*****************************************************************************/


static int iSplitMotion_CB(Ihandle* bar, int x, int y, char *status)
{
  Ihandle* ih = bar->parent;

  if (ih->data->is_holding)
  {
    if (iup_isbutton1(status))  /* DRAG MOVE */
    {
      int old_val = ih->data->val;
      int cur_x, cur_y;

      iupStrToIntInt(IupGetGlobal("CURSORPOS"), &cur_x, &cur_y, 'x');

      if (ih->data->orientation == ISPLIT_VERT)
      {
        int width1 = ih->data->start_size + (cur_x - ih->data->start_pos);
        iSplitAdjustWidth1(ih, &width1);
        iSplitCalcVal(ih, width1);
      }
      else
      {
        int height1 = ih->data->start_size + (cur_y - ih->data->start_pos);
        iSplitAdjustHeight1(ih, &height1);
        iSplitCalcVal(ih, height1);
      }

      iSplitAdjustVal(ih);

      if (old_val != ih->data->val)
        iupBaseCallValueChangedCb(ih);

      if (ih->data->layoutdrag)
      {
        IupRefreshChildren(ih);
        IupFlush();
      }
      else
        iSplitSetBarPosition(ih);
    }
    else
      ih->data->is_holding = 0;
  }

  (void)x;
  (void)y;
  return IUP_DEFAULT;
}

static int iSplitButton_CB(Ihandle* bar, int button, int pressed, int x, int y, char* status)
{
  Ihandle* ih = bar->parent;

  if (button!=IUP_BUTTON1)
    return IUP_DEFAULT;

  if (!ih->data->is_holding && pressed)  /* DRAG BEGIN */
  {
    int cur_x, cur_y;

    ih->data->is_holding = 1;

    iupStrToIntInt(IupGetGlobal("CURSORPOS"), &cur_x, &cur_y, 'x');

    /* Save the cursor position and size */
    if (ih->data->orientation == ISPLIT_VERT)
    {
      ih->data->start_bar = ih->firstchild->x;
      ih->data->start_pos = cur_x;
      ih->data->start_size = ih->firstchild->x - ih->x;
    }
    else
    {
      ih->data->start_bar = ih->firstchild->y;
      ih->data->start_pos = cur_y;
      ih->data->start_size = ih->firstchild->y - ih->y;
    }
  }
  else if (ih->data->is_holding && !pressed)  /* DRAG END */
  {
    ih->data->is_holding = 0;

    /* Always refresh when releasing the mouse */
    IupRefreshChildren(ih);  
  }

  (void)x;
  (void)y;
  (void)status;
  return IUP_DEFAULT;
}

static int iSplitFocus_CB(Ihandle* bar, int focus)
{
  Ihandle* ih = bar->parent;

  if (!ih || focus) /* use only kill focus */
    return IUP_DEFAULT;

  if (ih->data->is_holding)
    ih->data->is_holding = 0;

  return IUP_DEFAULT;
}


/*****************************************************************************\
|* Attributes                                                                *|
\*****************************************************************************/


static char* iSplitGetClientSizeAttrib(Ihandle* ih)
{
  int width = ih->currentwidth;
  int height = ih->currentheight;

  if (ih->data->orientation == ISPLIT_VERT)
    width -= ih->data->barsize;
  else
    height -= ih->data->barsize;

  if (width < 0) width = 0;
  if (height < 0) height = 0;
  return iupStrReturnIntInt(width, height, 'x');
}

static int iSplitSetColorAttrib(Ihandle* ih, const char* value)
{
  if (value != NULL && ih->data->showgrip == 0)
    IupSetAttribute(ih->firstchild, "STYLE", "FILL");

  IupSetStrAttribute(ih->firstchild, "COLOR", value);
  return 0;
}

static char* iSplitGetColorAttrib(Ihandle* ih)
{
  return IupGetAttribute(ih->firstchild, "COLOR");
}

static int iSplitSetOrientationAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle) /* only before map */
    return 0;

  IupSetStrAttribute(ih->firstchild, "ORIENTATION", value);

  if (iupStrEqualNoCase(value, "HORIZONTAL"))
    ih->data->orientation = ISPLIT_HORIZ;
  else  /* Default = VERTICAL */
    ih->data->orientation = ISPLIT_VERT;

  if (ih->data->orientation == ISPLIT_VERT)
    IupSetAttribute(ih->firstchild, "CURSOR", "SPLITTER_VERT");
  else
    IupSetAttribute(ih->firstchild, "CURSOR", "SPLITTER_HORIZ");

  return 0;  /* do not store value in hash table */
}

static int iSplitSetValueAttrib(Ihandle* ih, const char* value)
{
  if (!value)
  {
    ih->data->val = -1;  /* reset to calculate in Natural size */

    if (ih->handle)
      IupRefreshChildren(ih);  
  }
  else
  {
    int val;
    if (iupStrToInt(value, &val))
    {
      ih->data->val = val;
      iSplitAdjustVal(ih);

      if (ih->handle)
        IupRefreshChildren(ih);  
    }
  }

  return 0; /* do not store value in hash table */
}

static char* iSplitGetValueAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->val);
}

static int iSplitSetBarSizeAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToInt(value, &ih->data->barsize))
  {
    IupSetInt(ih->firstchild, "BARSIZE", ih->data->barsize);

    if (ih->data->barsize == 0)
      IupSetAttribute(ih->firstchild, "VISIBLE", "NO");
    else
      IupSetAttribute(ih->firstchild, "VISIBLE", "YES");

    if (ih->data->val != -1)
      iSplitAdjustVal(ih);  /* because of autohide */

    if (ih->handle)
      IupRefreshChildren(ih);  
  }
  return 0; /* do not store value in hash table */
}

static char* iSplitGetBarSizeAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->barsize);
}

static int iSplitSetMinMaxAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToIntInt(value, &ih->data->min, &ih->data->max, ':'))
  {
    if (ih->data->min > ih->data->max)
    {
      int t = ih->data->min;
      ih->data->min = ih->data->max;
      ih->data->max = t;
    }
    if (ih->data->min < 0) ih->data->min = 0;
    if (ih->data->max > 1000) ih->data->max = 1000;

    if (ih->data->val != -1)
      iSplitAdjustVal(ih);

    if (ih->handle)
      IupRefreshChildren(ih);  
  }
  return 0; /* do not store value in hash table */
}

static char* iSplitGetMinMaxAttrib(Ihandle* ih)
{
  return iupStrReturnIntInt(ih->data->min, ih->data->max, ':');
}

static int iSplitSetLayoutDragAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->layoutdrag = 1;
  else
    ih->data->layoutdrag = 0;

  return 0; /* do not store value in hash table */
}

static char* iSplitGetLayoutDragAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean (ih->data->layoutdrag); 
}

static int iSplitSetShowGripAttrib(Ihandle* ih, const char* value)
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
      if (iupAttribGet(ih, "COLOR") != NULL)
        IupSetAttribute(ih->firstchild, "STYLE", "FILL");
      else
        IupSetAttribute(ih->firstchild, "STYLE", "EMPTY");

      ih->data->showgrip = 0;

      if (ih->data->barsize == 5)
        iSplitSetBarSizeAttrib(ih, "3");
    }
  }

  return 0; /* do not store value in hash table */
}

static char* iSplitGetShowGripAttrib(Ihandle* ih)
{
  if (ih->data->showgrip == 2)
    return "LINES";
  else
    return iupStrReturnBoolean (ih->data->showgrip); 
}

static int iSplitSetAutoHideAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->autohide = 1;
  else
  {
    Ihandle *child1 = ih->firstchild->brother;
    if (child1)
    {
      Ihandle *child2 = child1->brother;
      iSplitAutoHideChild(child1, ISPLIT_SHOW);
      if (child2)
        iSplitAutoHideChild(child2, ISPLIT_SHOW);
    }

    ih->data->autohide = 0;
  }

  if (ih->data->val != -1)
    iSplitAdjustVal(ih);

  if (ih->handle)
    IupRefreshChildren(ih);  

  return 0; /* do not store value in hash table */
}

static char* iSplitGetAutoHideAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean (ih->data->autohide); 
}


/*****************************************************************************\
|* Methods                                                                   *|
\*****************************************************************************/


static void iSplitComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  int natural_w = 0, 
      natural_h = 0;
  Ihandle *child1, *child2 = NULL;
  child1 = ih->firstchild->brother;
  if (child1)
    child2 = child1->brother;

  /* bar */
  if (ih->data->orientation == ISPLIT_VERT)
    natural_w += ih->data->barsize;
  else
    natural_h += ih->data->barsize;

  if (child1 && !(child1->flags & IUP_FLOATING_IGNORE))
  {
    /* update child natural size first */
    iupBaseComputeNaturalSize(child1);

    if (ih->data->orientation == ISPLIT_VERT)
    {
      natural_w += child1->naturalwidth;
      natural_h = iupMAX(natural_h, child1->naturalheight);
    }
    else
    {
      natural_w = iupMAX(natural_w, child1->naturalwidth);
      natural_h += child1->naturalheight;
    }

    *children_expand |= child1->expand;
  }

  if (child2 && !(child2->flags & IUP_FLOATING_IGNORE))
  {
    /* update child natural size first */
    iupBaseComputeNaturalSize(child2);

    if (ih->data->orientation == ISPLIT_VERT)
    {
      natural_w += child2->naturalwidth;
      natural_h = iupMAX(natural_h, child2->naturalheight);
    }
    else
    {
      natural_w = iupMAX(natural_w, child2->naturalwidth);
      natural_h += child2->naturalheight;
    }

    *children_expand |= child2->expand;
  }

  if (ih->data->val == -1)  /* first time or reset, re-compute value from natural size */
  {
    if (child1)
    {
      /* just is just an initial value based on natural size of the split and the child, similar to iSplitCalcVal */
      if (ih->data->orientation == ISPLIT_VERT)
        ih->data->val = (child1->naturalwidth*1000)/(natural_w - ih->data->barsize);
      else
        ih->data->val = (child1->naturalheight*1000)/(natural_h - ih->data->barsize);
    }
    else
      ih->data->val = ih->data->min;

    iSplitAdjustVal(ih);
  }

  *w = natural_w;
  *h = natural_h;
}

static void iSplitSetChildrenCurrentSizeMethod(Ihandle* ih, int shrink)
{
  int old_val = ih->data->val;
  Ihandle *child1, *child2 = NULL;
  child1 = ih->firstchild->brother;
  if (child1)
    child2 = child1->brother;

  if (ih->data->orientation == ISPLIT_VERT)
  {
    int width1 = iSplitGetWidth1(ih);
    if (iSplitAdjustWidth1(ih, &width1))    /* this will check for child1 and child2 */
      iSplitCalcVal(ih, width1);  /* has a MINMAX size, must fix split value */

    if (child1 && !(child1->flags & IUP_FLOATING_IGNORE))
    {
      iupBaseSetCurrentSize(child1, width1, ih->currentheight, shrink);

      if (!(child1->flags & IUP_FLOATING_IGNORE) && child1->currentwidth > width1)
      {
        /* has a minimum size, must fix split value */
        width1 = child1->currentwidth;
        iSplitCalcVal(ih, width1);
      }
    }

    /* bar */
    ih->firstchild->currentwidth  = ih->data->barsize;
    ih->firstchild->currentheight = ih->currentheight;

    if (child2 && !(child2->flags & IUP_FLOATING_IGNORE))
    {
      int width2 = iSplitGetWidth2(ih, width1);
      iupBaseSetCurrentSize(child2, width2, ih->currentheight, shrink);

      if (child2->currentwidth > width2)
      {
        /* has a minimum size, must fix split value */
        width2 = child2->currentwidth;
        width1 = (ih->currentwidth-ih->data->barsize) - width2;
        iSplitCalcVal(ih, width1);
        if (child1)
          iupBaseSetCurrentSize(child1, width1, ih->currentheight, shrink);
      }
    }
  }
  else /* ISPLIT_HORIZ */
  {
    int height1 = iSplitGetHeight1(ih);
    if (iSplitAdjustHeight1(ih, &height1))  /* this will check for child1 and child2 */
      iSplitCalcVal(ih, height1);  /* has a MINMAX size, must fix split value */

    if (child1 && !(child1->flags & IUP_FLOATING_IGNORE))
    {
      iupBaseSetCurrentSize(child1, ih->currentwidth, height1, shrink);

      if (child1->currentheight > height1)
      {
        /* has a minimum size, must fix split value */
        height1 = child1->currentheight;
        iSplitCalcVal(ih, height1);
      }
    }

    /* bar */
    ih->firstchild->currentwidth  = ih->currentwidth;
    ih->firstchild->currentheight = ih->data->barsize;

    if (child2 && !(child2->flags & IUP_FLOATING_IGNORE))
    {
      int height2 = iSplitGetHeight2(ih, height1);
      iupBaseSetCurrentSize(child2, ih->currentwidth, height2, shrink);

      if (child2->currentheight > height2)
      {
        /* has a minimum size, must fix split value */
        height2 = child2->currentheight;
        height1 = (ih->currentheight-ih->data->barsize) - height2;
        iSplitCalcVal(ih, height1);
        if (child1)
          iupBaseSetCurrentSize(child1, ih->currentwidth, height1, shrink);
      }
    }
  }

  if (old_val != ih->data->val)
    iupBaseCallValueChangedCb(ih);
}

static void iSplitSetChildrenPositionMethod(Ihandle* ih, int x, int y)
{
  Ihandle *child1, *child2 = NULL;
  child1 = ih->firstchild->brother;
  if (child1)
    child2 = child1->brother;

  if (ih->data->orientation == ISPLIT_VERT)
  {
    if (child1 && !(child1->flags & IUP_FLOATING_IGNORE))
      iupBaseSetPosition(child1, x, y);

    /* bar */
    x += iSplitGetWidth1(ih);
    iupBaseSetPosition(ih->firstchild, x, y);

    if (child2 && !(child2->flags & IUP_FLOATING_IGNORE))
    {
      x += ih->data->barsize;
      iupBaseSetPosition(child2, x, y);
    }
  }
  else /* ISPLIT_HORIZ */
  {
    if (child1 && !(child1->flags & IUP_FLOATING_IGNORE))
      iupBaseSetPosition(child1, x, y);

    /* bar */
    y += iSplitGetHeight1(ih);
    iupBaseSetPosition(ih->firstchild, x, y);

    if (child2 && !(child2->flags & IUP_FLOATING_IGNORE))
    {
      y += ih->data->barsize;
      iupBaseSetPosition(child2, x, y);
    }
  }
}

static int iSplitCreateMethod(Ihandle* ih, void** params)
{
  Ihandle* bar;

  ih->data = iupALLOCCTRLDATA();

  ih->data->orientation = ISPLIT_VERT;
  ih->data->val = -1;
  ih->data->layoutdrag = 1;
  ih->data->autohide = 0;
  ih->data->barsize = 5;
  ih->data->showgrip = 1;
  ih->data->min = 0; 
  ih->data->max = 1000;

  bar = IupFlatSeparator();
  iupChildTreeAppend(ih, bar);  /* bar will always be the firstchild */
  bar->flags |= IUP_INTERNAL;

  IupSetAttribute(bar, "EXPAND", "NO");
  IupSetAttribute(bar, "CURSOR", "SPLITTER_VERT");
  IupSetAttribute(bar, "STYLE", "GRIP");
  IupSetAttribute(bar, "ORIENTATION", "VERTICAL");

  /* Setting callbacks */
  IupSetCallback(bar, "BUTTON_CB", (Icallback) iSplitButton_CB);
  IupSetCallback(bar, "FOCUS_CB",  (Icallback) iSplitFocus_CB);
  IupSetCallback(bar, "MOTION_CB", (Icallback) iSplitMotion_CB);

  if (params)
  {
    Ihandle** iparams = (Ihandle**)params;
    if (iparams[0]) IupAppend(ih, iparams[0]);
    if (iparams[1]) IupAppend(ih, iparams[1]);
  }

  return IUP_NOERROR;
}

Iclass* iupSplitNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name   = "split";
  ic->format = "hh";   /* two Ihandle*(s) */
  ic->nativetype = IUP_TYPEVOID;
  ic->childtype = IUP_CHILDMANY+3;  /* canvas+child+child */
  ic->is_interactive = 0;

  /* Class functions */
  ic->New = iupSplitNewClass;
  ic->Create  = iSplitCreateMethod;
  ic->Map     = iupBaseTypeVoidMapMethod;

  ic->ComputeNaturalSize = iSplitComputeNaturalSizeMethod;
  ic->SetChildrenCurrentSize = iSplitSetChildrenCurrentSizeMethod;
  ic->SetChildrenPosition    = iSplitSetChildrenPositionMethod;

  /* Base Callbacks */
  iupBaseRegisterBaseCallbacks(ic);

  /* Callbacks */
  iupClassRegisterCallback(ic, "VALUECHANGED_CB", "");

  /* Common */
  iupBaseRegisterCommonAttrib(ic);

  /* Base Container */
  iupClassRegisterAttribute(ic, "EXPAND", iupBaseContainerGetExpandAttrib, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTSIZE", iSplitGetClientSizeAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTOFFSET", iupBaseGetClientOffsetAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_READONLY|IUPAF_NO_INHERIT);

  /* IupSplit only */
  iupClassRegisterAttribute(ic, "DIRECTION", NULL, iSplitSetOrientationAttrib, IUPAF_SAMEASSYSTEM, "VERTICAL", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", iSplitGetValueAttrib, iSplitSetValueAttrib, IUPAF_SAMEASSYSTEM, "500", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LAYOUTDRAG", iSplitGetLayoutDragAttrib, iSplitSetLayoutDragAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AUTOHIDE", iSplitGetAutoHideAttrib, iSplitSetAutoHideAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MINMAX", iSplitGetMinMaxAttrib, iSplitSetMinMaxAttrib, IUPAF_SAMEASSYSTEM, "0:1000", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);


  iupClassRegisterAttribute(ic, "COLOR", iSplitGetColorAttrib, iSplitSetColorAttrib, IUPAF_SAMEASSYSTEM, "160 160 160", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ORIENTATION", NULL, iSplitSetOrientationAttrib, IUPAF_SAMEASSYSTEM, "VERTICAL", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWGRIP", iSplitGetShowGripAttrib, iSplitSetShowGripAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BARSIZE", iSplitGetBarSizeAttrib, iSplitSetBarSizeAttrib, IUPAF_SAMEASSYSTEM, "5", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);


  return ic;
}

IUP_API Ihandle* IupSplit(Ihandle* child1, Ihandle* child2)
{
  void *children[3];
  children[0] = (void*)child1;
  children[1] = (void*)child2;
  children[2] = NULL;
  return IupCreatev("split", children);
}
