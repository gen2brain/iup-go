/** \file
 * \brief IupDetachBox control
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
#include "iup_drvdraw.h"
#include "iup_draw.h"


enum { IDBOX_VERT, IDBOX_HORIZ };

struct _IcontrolData
{
  /* aux */
  int is_holding;
  Ihandle *old_parent, *old_brother;

  /* attributes */
  int layoutdrag, barsize, showgrip;
  int orientation;     /* one of the types: IDBOX_VERT, IDBOX_HORIZ */
};


/*****************************************************************************\
|* Attributes                                                                *|
\*****************************************************************************/

static char* iDetachBoxGetClientSizeAttrib(Ihandle* ih)
{
  int width = ih->currentwidth;
  int height = ih->currentheight;

  if (IupGetInt(ih->firstchild, "VISIBLE"))
  {
    if (ih->data->orientation == IDBOX_VERT)
      width -= ih->data->barsize;
    else
      height -= ih->data->barsize;
  }

  if (width < 0) width = 0;
  if (height < 0) height = 0;
  return iupStrReturnIntInt(width, height, 'x');
}

static int iDetachBoxSetColorAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  IupUpdate(ih->firstchild);  /* only updates the bar */
  return 1;  /* store value in hash table */
}

static int iDetachBoxSetOrientationAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle) /* only before map */
    return 0;

  if (iupStrEqualNoCase(value, "HORIZONTAL"))
    ih->data->orientation = IDBOX_HORIZ;
  else  /* Default = VERTICAL */
    ih->data->orientation = IDBOX_VERT;

  return 0;  /* do not store value in hash table */
}

static int iDetachBoxSetBarSizeAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToInt(value, &ih->data->barsize))
  {
    if (ih->data->barsize == 0)
      IupSetAttribute(ih->firstchild, "VISIBLE", "No");

    if (ih->handle)
      IupRefreshChildren(ih);
  }

  return 0; /* do not store value in hash table */
}

static char* iDetachBoxGetBarSizeAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->barsize);
}

static int iDetachBoxSetRestoreAttrib(Ihandle* ih, const char* value)
{
  Ihandle *dlg = IupGetDialog(ih);
  Ihandle* new_parent = IupGetHandle(value);
  Ihandle* new_brother = NULL;

  if (!new_parent)
  {
    new_parent = ih->data->old_parent;
    new_brother = ih->data->old_brother;

    if (IupGetChildPos(new_parent, new_brother) == -1)  /* not a child of new_parent */
      new_brother = NULL;
  }

  /* Sets the new parent */
  IupReparent(ih, new_parent, new_brother);

  /* Show handler */
  if (ih->data->barsize)
    IupSetAttribute(ih->firstchild, "VISIBLE", "Yes");

  /* Updates/redraws the layout of the dialog */
  IupRefresh(new_parent);

  /* Reset previous parent and brother */
  ih->data->old_parent = NULL;
  ih->data->old_brother = NULL;

  IupDestroy(dlg);
  return 0;
}

static int iDetachDialogClose_CB(Ihandle* ih_dialog)
{
  Ihandle* ih = IupGetChild(ih_dialog, 0);
  IFnnii cb = (IFnnii)IupGetCallback(ih, "RESTORED_CB");
  if (cb)
  {
    int ret = cb(ih, ih->data->old_parent, 0, 0);
    if (ret == IUP_IGNORE)
      return IUP_DEFAULT;
  }
  IupSetAttribute(ih, "RESTORE", NULL);
  return IUP_IGNORE;
}

static int iDetachBoxSetDetachAttrib(Ihandle* ih, const char* value)
{
  int cur_x, cur_y;
  IFnnii detachedCB = (IFnnii)IupGetCallback(ih, "DETACHED_CB");

  /* Create new dialog */
  Ihandle *new_parent = IupDialog(NULL);
  Ihandle *old_dialog = IupGetDialog(ih);

  /* Set new dialog as child of the current application */
  IupSetAttributeHandle(new_parent, "PARENTDIALOG", old_dialog);

  if (iupAttribGetBoolean(ih, "RESTOREWHENCLOSED"))
    IupSetCallback(new_parent, "CLOSE_CB", iDetachDialogClose_CB);

  iupStrToIntInt(IupGetGlobal("CURSORPOS"), &cur_x, &cur_y, 'x');

  if (detachedCB)
  {
    int ret = detachedCB(ih, new_parent, cur_x, cur_y);
    if (ret == IUP_IGNORE)
    {
      IupDestroy(new_parent);
      return IUP_DEFAULT;
    }
  }

  /* set user size of the detachbox as the current size of the child */
  IupSetStrAttribute(ih, "RASTERSIZE", IupGetAttribute(ih->firstchild->brother, "RASTERSIZE"));

  /* Save current parent and reference child */
  ih->data->old_parent = ih->parent;
  ih->data->old_brother = ih->brother;

  IupMap(new_parent);

  /* Sets the new parent */
  IupReparent(ih, new_parent, NULL);

  /* Hide handler */
  IupSetAttribute(ih->firstchild, "VISIBLE", "No");

  /* force a dialog resize since IupMap already computed the dialog size */
  IupSetAttribute(new_parent, "RASTERSIZE", NULL);

  /* Maps and shows the new dialog */
  IupShowXY(new_parent, cur_x, cur_y);

  /* reset user size of the detachbox */
  IupSetAttribute(ih, "USERSIZE", NULL);

  /* Updates/redraws the layout of the old dialog */
  IupRefresh(old_dialog);

  (void)value;
  return 0;
}

static int iDetachBoxSetShowGripAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->showgrip = 1;
  else
  {
    ih->data->showgrip = 0;
    if (ih->data->barsize > 5)
      iDetachBoxSetBarSizeAttrib(ih, "5");
  }

  return 0; /* do not store value in hash table */
}

static char* iDetachBoxGetShowGripAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean (ih->data->showgrip); 
}

static char* iDetachBoxGetOldParentHandleAttrib(Ihandle* ih)
{
  return (char*)ih->data->old_parent;
}

static char* iDetachBoxGetOldBrotherHandleAttrib(Ihandle* ih)
{
  return (char*)ih->data->old_brother;
}

/*****************************************************************************\
|* Callbacks                                                                 *|
\*****************************************************************************/

static int iDetachBoxK_Any_CB(Ihandle* ih, int key)
{
  if(ih->data->is_holding && key == K_ESC)  /* DRAG CANCEL */
  {
    ih->data->is_holding = 0;
    /* Restore cursor */
    IupSetAttribute(ih->firstchild, "CURSOR", "MOVE");
  }

  return IUP_CONTINUE;
}

static int iDetachBoxAction_CB(Ihandle* bar)
{
  Ihandle* ih = bar->parent;
  IdrawCanvas* dc = iupdrvDrawCreateCanvas(bar);

  iupDrawParentBackground(dc, ih);

  if (ih->data->showgrip)
  {
    int i, w, h, x, y, count;
    long color = iupDrawStrToColor(IupGetAttribute(ih, "COLOR"), iupDrawColor(160, 160, 160, 255));
    long bgcolor;
    iupdrvDrawGetSize(dc, &w, &h);

    if (iupDrawRed(color) + iupDrawGreen(color) + iupDrawBlue(color) > 3 * 190)
      bgcolor = iupDrawColor(100, 100, 100, 255);
    else
      bgcolor = iupDrawColor(255, 255, 255, 255);

    if (ih->data->orientation == IDBOX_VERT)
    {
      x = ih->data->barsize/2-1;
      y = 2;
      count = (h-2)/4;
    }
    else
    {
      x = 2;
      y = ih->data->barsize/2-1;
      count = (w-2)/4;
    }

    for (i = 0; i < count; i++)
    {
      iupdrvDrawRectangle(dc, x + 1, y + 1, x + 2, y + 2, bgcolor, IUP_DRAW_FILL, 1);
      iupdrvDrawRectangle(dc, x, y, x + 1, y + 1, color, IUP_DRAW_FILL, 1);

      if(i < count - 1)
      {
        iupdrvDrawRectangle(dc, x + 3, y + 3, x + 4, y + 4, bgcolor, IUP_DRAW_FILL, 1);
        iupdrvDrawRectangle(dc, x + 2, y + 2, x + 3, y + 3, color, IUP_DRAW_FILL, 1);
      }

      iupdrvDrawRectangle(dc, x + 5, y + 1, x + 6, y + 2, bgcolor, IUP_DRAW_FILL, 1);
      iupdrvDrawRectangle(dc, x + 4, y, x + 5, y + 1, color, IUP_DRAW_FILL, 1);

      if (ih->data->orientation == IDBOX_VERT)
        y += 4;
      else
        x += 4;
    }
  }
  else
  {
    int w, h, x, y;
    long color = iupDrawStrToColor(IupGetAttribute(ih, "COLOR"), iupDrawColor(160, 160, 160, 255));

    iupdrvDrawGetSize(dc, &w, &h);

    if (ih->data->orientation == IDBOX_VERT)
    {
      x = ih->data->barsize/2-1;
      y = 2;
    }
    else
    {
      x = 2;
      y = ih->data->barsize/2-1;
    }

    iupdrvDrawRectangle(dc, x, y, x + w, y + h, color, IUP_DRAW_FILL, 1);
  }
  
  iupdrvDrawFlush(dc);

  iupdrvDrawKillCanvas(dc);

  return IUP_DEFAULT;
}

static int iDetachBoxButton_CB(Ihandle* bar, int button, int pressed, int x, int y, char* status)
{
  Ihandle* ih = bar->parent;

  if (button != IUP_BUTTON1)
    return IUP_DEFAULT;

  if (!ih->data->is_holding && pressed)  /* DRAG BEGIN */
  {
    ih->data->is_holding = 1;
    
    /* Change cursor */
    IupSetAttribute(bar, "CURSOR", "IupDetachBoxCursor");
  }
  else if (ih->data->is_holding && !pressed)  /* DRAG END */
  {
    ih->data->is_holding = 0;

    /* Restores the cursor */
    IupSetAttribute(bar, "CURSOR", "MOVE");

    iDetachBoxSetDetachAttrib(ih, NULL);
  }

  (void)x;
  (void)y;
  (void)status;
  return IUP_DEFAULT;
}

static int iDetachBoxFocus_CB(Ihandle* bar, int focus)
{
  Ihandle* ih = bar->parent;

  if (!ih || focus) /* use only kill focus */
    return IUP_DEFAULT;

  if (ih->data->is_holding)
    ih->data->is_holding = 0;

  return IUP_DEFAULT;
}


/*****************************************************************************\
|* Methods                                                                   *|
\*****************************************************************************/

static void iDetachBoxComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  int natural_w = 0, 
      natural_h = 0;

  /* bar */
  if (IupGetInt(ih->firstchild, "VISIBLE"))
  {
    if (ih->data->orientation == IDBOX_VERT)
      natural_w += ih->data->barsize;
    else
      natural_h += ih->data->barsize;
  }

  if (ih->firstchild->brother)
  {
    Ihandle* child = ih->firstchild->brother;

    /* update child natural size first */
    iupBaseComputeNaturalSize(child);

    if (ih->data->orientation == IDBOX_VERT)
    {
      natural_w += child->naturalwidth;
      natural_h = iupMAX(natural_h, child->naturalheight);
    }
    else  /* IDBOX_HORIZ */
    {
      natural_w = iupMAX(natural_w, child->naturalwidth);
      natural_h += child->naturalheight;
    }

    *children_expand |= child->expand;
  }

  *w = natural_w;
  *h = natural_h;
}

static void iDetachBoxSetChildrenCurrentSizeMethod(Ihandle* ih, int shrink)
{
  /* bar */
  if (ih->data->orientation == IDBOX_VERT)
  {
    ih->firstchild->currentwidth  = ih->data->barsize;
    ih->firstchild->currentheight = ih->currentheight;
  }
  else  /* IDBOX_HORIZ */
  {
    ih->firstchild->currentwidth  = ih->currentwidth;
    ih->firstchild->currentheight = ih->data->barsize;
  }

  /* child */
  if (ih->firstchild->brother)
  {
    int width = ih->currentwidth;
    int height = ih->currentheight;

    if (IupGetInt(ih->firstchild, "VISIBLE"))
    {
      if (ih->data->orientation == IDBOX_VERT)
        width -= ih->data->barsize;
      else
        height -= ih->data->barsize;
    }

    if (width < 0) width = 0;
    if (height < 0) height = 0;

    iupBaseSetCurrentSize(ih->firstchild->brother, width, height, shrink);
  }
}

static void iDetachBoxSetChildrenPositionMethod(Ihandle* ih, int x, int y)
{
  /* bar */
  iupBaseSetPosition(ih->firstchild, x, y);

  /* child */
  if (ih->data->orientation == IDBOX_VERT)
  {
    if (IupGetInt(ih->firstchild, "VISIBLE"))
      x += ih->data->barsize;
    iupBaseSetPosition(ih->firstchild->brother, x, y);
  }
  else  /* IDBOX_HORIZ */
  {
    if (IupGetInt(ih->firstchild, "VISIBLE"))
      y += ih->data->barsize;
    iupBaseSetPosition(ih->firstchild->brother, x, y);
  }
}

static int iDetachBoxCreateMethod(Ihandle* ih, void** params)
{
  Ihandle* bar;

  ih->data = iupALLOCCTRLDATA();

  ih->data->orientation = IDBOX_VERT;
  ih->data->barsize = 10;
  ih->data->showgrip = 1;

  bar = IupCanvas(NULL);
  bar->flags |= IUP_INTERNAL;
  iupChildTreeAppend(ih, bar);  /* bar will always be the firstchild */

  IupSetAttribute(bar, "CANFOCUS", "NO");
  IupSetAttribute(bar, "BORDER", "NO");
  IupSetAttribute(bar, "EXPAND", "NO");
  IupSetAttribute(bar, "CURSOR", "MOVE");

  /* Setting canvas bar callbacks */
  IupSetCallback(bar, "BUTTON_CB", (Icallback) iDetachBoxButton_CB);
  IupSetCallback(bar, "FOCUS_CB", (Icallback) iDetachBoxFocus_CB);
  IupSetCallback(bar, "ACTION", (Icallback) iDetachBoxAction_CB);

  /* Setting element callbacks */
  IupSetCallback(ih, "K_ANY", (Icallback) iDetachBoxK_Any_CB);

  if (params)
  {
    Ihandle** iparams = (Ihandle**)params;
    if (*iparams)
      IupAppend(ih, *iparams);
  }

  return IUP_NOERROR;
}

static void iDetachBoxCreateCursor(void)
{
  Ihandle *imgcursor;
  unsigned char detach_img_cur[16*16] = 
  {
    0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    0,0,1,4,4,4,4,4,4,4,4,4,4,4,4,1,
    0,0,1,4,4,4,4,4,4,4,4,4,4,4,4,1,
    0,0,1,3,3,3,3,3,3,3,3,3,3,3,3,1,
    0,0,1,3,3,3,2,2,2,3,3,3,3,3,3,1,
    1,1,1,1,1,1,2,2,2,1,1,1,1,1,3,1,
    1,4,4,4,4,4,2,2,2,4,4,4,4,1,3,1,
    1,4,4,4,4,4,2,2,2,4,4,4,4,1,1,1,
    1,3,3,3,3,3,2,2,2,3,3,3,3,1,0,0,
    1,3,3,3,2,2,2,2,2,2,2,3,3,1,0,0,
    1,3,3,3,3,2,2,2,2,2,3,3,3,1,0,0,
    1,3,3,3,3,3,2,2,2,3,3,3,3,1,0,0,
    1,3,3,3,3,3,3,2,3,3,3,3,3,1,0,0,
    1,3,3,3,3,3,3,3,3,3,3,3,3,1,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0
  };

  imgcursor = IupImage(16, 16, detach_img_cur);
  IupSetAttribute(imgcursor, "0", "BGCOLOR"); 
  IupSetAttribute(imgcursor, "1", "0 0 0"); 
  IupSetAttribute(imgcursor, "2", "110 150 255"); 
  IupSetAttribute(imgcursor, "3", "255 255 255"); 
  IupSetAttribute(imgcursor, "4", "64 92 255"); 
  IupSetHandle("IupDetachBoxCursor", imgcursor); 
}

Iclass* iupDetachBoxNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "detachbox";
  ic->cons = "DetachBox";
  ic->format = "h";   /* one Ihandle* */
  ic->nativetype = IUP_TYPEVOID;
  ic->childtype = IUP_CHILDMANY+2; /* canvas+child */
  ic->is_interactive = 0;

  /* Class functions */
  ic->New    = iupDetachBoxNewClass;
  ic->Create = iDetachBoxCreateMethod;
  ic->Map    = iupBaseTypeVoidMapMethod;

  ic->ComputeNaturalSize     = iDetachBoxComputeNaturalSizeMethod;
  ic->SetChildrenCurrentSize = iDetachBoxSetChildrenCurrentSizeMethod;
  ic->SetChildrenPosition    = iDetachBoxSetChildrenPositionMethod;

  /* Base Callbacks */
  iupBaseRegisterBaseCallbacks(ic);

  /* Callbacks */
  iupClassRegisterCallback(ic, "DETACHED_CB", "nii");
  iupClassRegisterCallback(ic, "RESTORED_CB", "nii");

  /* Common */
  iupBaseRegisterCommonAttrib(ic);

  /* Base Container */
  iupClassRegisterAttribute(ic, "CLIENTSIZE", iDetachBoxGetClientSizeAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTOFFSET", iupBaseGetClientOffsetAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EXPAND", iupBaseContainerGetExpandAttrib, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  /* IupDetachBox only */
  iupClassRegisterAttribute(ic, "COLOR", NULL, iDetachBoxSetColorAttrib, IUPAF_SAMEASSYSTEM, "160 160 160", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ORIENTATION", NULL, iDetachBoxSetOrientationAttrib, IUPAF_SAMEASSYSTEM, "VERTICAL", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BARSIZE", iDetachBoxGetBarSizeAttrib, iDetachBoxSetBarSizeAttrib, IUPAF_SAMEASSYSTEM, "10", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWGRIP", iDetachBoxGetShowGripAttrib, iDetachBoxSetShowGripAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OLDPARENT_HANDLE", iDetachBoxGetOldParentHandleAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT | IUPAF_IHANDLE|IUPAF_NO_STRING);
  iupClassRegisterAttribute(ic, "OLDBROTHER_HANDLE", iDetachBoxGetOldBrotherHandleAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT | IUPAF_IHANDLE | IUPAF_NO_STRING);
  iupClassRegisterAttribute(ic, "RESTORE", NULL, iDetachBoxSetRestoreAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DETACH", NULL, iDetachBoxSetDetachAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RESTOREWHENCLOSED", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  if (!IupGetHandle("IupDetachBoxCursor"))
    iDetachBoxCreateCursor();

  return ic;
}

IUP_API Ihandle* IupDetachBox(Ihandle* child)
{
  void *children[2];
  children[0] = (void*)child;
  children[1] = NULL;
  return IupCreatev("detachbox", children);
}
