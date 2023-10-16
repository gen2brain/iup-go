/** \file
 * \brief iupflatscrollbox control
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
#include "iup_childtree.h"
#include "iup_flatscrollbar.h"


static Ihandle* iFlatScrollBoxGetChild(Ihandle *ih)
{
  /* ih->firstchild is vertical scrollbar */
  /* ih->firstchild->brother is horizontal scrollbar */
  return ih->firstchild->brother->brother;
}

static void iFlatScrollBoxUpdateChildPos(Ihandle *ih, Ihandle* child)
{
  int posx = iupAttribGetInt(ih, "POSX");
  int posy = iupAttribGetInt(ih, "POSY");
  char* offset = iupAttribGet(ih, "CHILDOFFSET");
  int sb_size = iupAttribGetInt(ih, "SCROLLBARSIZE");
  int x, y;

  if (iupAttribGetBoolean(ih, "SHOWFLOATING"))
    sb_size = 0;

  /* Native container, position is reset */
  x = 0;
  y = 0;

  if (offset) iupStrToIntInt(offset, &x, &y, 'x');

  if (iupAttribGetInt(ih, "DX") > iupAttribGetInt(ih, "XMAX") - sb_size)
    posx = 0;
  if (iupAttribGetInt(ih, "DY") > iupAttribGetInt(ih, "YMAX") - sb_size)
    posy = 0;

  x -= posx;
  y -= posy;

  /* Child coordinates are relative to client left-top corner. */
  iupBaseSetPosition(child, x, y);
}

static void iFlatScrollBoxUpdateLayout(Ihandle *ih, Ihandle *child)
{
  if (child->handle)
  {
    Icallback cb;

    iupLayoutUpdate(child);

    cb = IupGetCallback(ih, "LAYOUTUPDATE_CB");
    if (cb)
      cb(ih);
  }
}

static int iFlatScrollBoxScroll_CB(Ihandle *ih, int op, float posx, float posy)
{
  Ihandle* child = iFlatScrollBoxGetChild(ih);

  /* interactive updates */
  if ((op == IUP_SBDRAGH || op == IUP_SBDRAGV) && !iupAttribGetBoolean(ih, "LAYOUTDRAG"))
    return IUP_DEFAULT;

  if (child)
  {
    iFlatScrollBoxUpdateChildPos(ih, child);

    iFlatScrollBoxUpdateLayout(ih, child);
  }

  (void)posx;
  (void)posy;
  return IUP_DEFAULT;
}

static int iFlatScrollBoxFlatScroll_CB(Ihandle *ih)
{
  Ihandle* child = iFlatScrollBoxGetChild(ih);

  /* non interactive updates */

  if (child)
  {
    iFlatScrollBoxUpdateChildPos(ih, child);

    iFlatScrollBoxUpdateLayout(ih, child);
  }

  return IUP_DEFAULT;
}


/*****************************************************************************/


static int iFlatScrollBoxButton_CB(Ihandle *ih, int but, int pressed, int x, int y, char* status)
{
  if (but == IUP_BUTTON1 && pressed)
  {
    iupAttribSetInt(ih, "_IUP_START_X", x);
    iupAttribSetInt(ih, "_IUP_START_Y", y);
    iupAttribSetInt(ih, "_IUP_START_POSX", iupAttribGetInt(ih, "POSX"));
    iupAttribSetInt(ih, "_IUP_START_POSY", iupAttribGetInt(ih, "POSY"));
    iupAttribSet(ih, "_IUP_DRAG_SB", "1");
  }
  if (but == IUP_BUTTON1 && !pressed)
  {
    iupAttribSet(ih, "_IUP_DRAG_SB", NULL);
  }
  (void)status;
  return IUP_DEFAULT;
}

static int iFlatScrollBoxMotion_CB(Ihandle *ih, int x, int y, char* status)
{
  iupFlatScrollBarMotionUpdate(ih, x, y);

  if (iup_isbutton1(status) && iupAttribGet(ih, "_IUP_DRAG_SB"))
  {
    int start_x = iupAttribGetInt(ih, "_IUP_START_X");
    int start_y = iupAttribGetInt(ih, "_IUP_START_Y");
    int off_x = x - start_x;
    int off_y = y - start_y;
    int posx = iupAttribGetInt(ih, "_IUP_START_POSX");
    int posy = iupAttribGetInt(ih, "_IUP_START_POSY");
    posx -= off_x;  /* drag direction is opposite to scrollbar */
    posy -= off_y;

    iupFlatScrollBarSetPos(ih, posx, posy);
  }
  return IUP_DEFAULT;
}

static int iFlatScrollBoxWheel_CB(Ihandle* ih, float delta)
{
  iupFlatScrollBarWheelUpdate(ih, delta);
  return IUP_DEFAULT;
}


/*****************************************************************************/


static int iFlatScrollBoxGetChildPosition(Ihandle* ih, Ihandle* child, int *posx, int *posy)
{
  while (child->parent && child != ih)
  {
    int off_x, off_y;

    *posx += child->x;
    *posy += child->y;

    child = iupChildTreeGetNativeParent(child);

    IupGetIntInt(child, "CLIENTOFFSET", &off_x, &off_y);
    *posx += off_x;
    *posy += off_y;
  }

  if (!child->parent)
    return 0;
  else
    return 1;
}

static int iFlatScrollBoxSetScrollToChildHandleAttrib(Ihandle* ih, const char* value)
{
  Ihandle* child = (Ihandle*)value;
  if (iupObjectCheck(child))
  {
    int posx = iupAttribGetInt(ih, "POSX");
    int posy = iupAttribGetInt(ih, "POSY");
    if (iFlatScrollBoxGetChildPosition(ih, child, &posx, &posy))
      iupFlatScrollBarSetPos(ih, posx, posy);
  }
  return 0;
}

static int iFlatScrollBoxSetScrollToChildAttrib(Ihandle* ih, const char* value)
{
  return iFlatScrollBoxSetScrollToChildHandleAttrib(ih, (char*)IupGetHandle(value));
}

static int iFlatScrollBoxSetScrollToAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "TOP"))
  {
    iupFlatScrollBarSetPos(ih, 0, 0);
  }
  else if (iupStrEqualNoCase(value, "BOTTOM"))
    iupFlatScrollBarSetPos(ih, 0, iupAttribGetInt(ih, "YMAX") - iupAttribGetInt(ih, "DY"));
  else
  {
    int posx, posy;
    if (iupStrToIntInt(value, &posx, &posy, ',') == 2)
      iupFlatScrollBarSetPos(ih, posx, posy);
  }
  return 0;
}

static char* iFlatScrollBoxGetExpandAttrib(Ihandle* ih)
{
  if (iupAttribGetBoolean(ih, "CANVASBOX"))
    return iupBaseGetExpandAttrib(ih);
  else
    return iupBaseContainerGetExpandAttrib(ih);
}

static int iFlatScrollBoxSetExpandAttrib(Ihandle* ih, const char* value)
{
  if (iupAttribGetBoolean(ih, "CANVASBOX"))
    return iupBaseSetExpandAttrib(ih, value);
  else
    return 1;  /* store on the hash table */
}


/*******************************************************************************************************/


static void iFlatScrollBoxComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  Ihandle* child = iFlatScrollBoxGetChild(ih);
  if (child)
  {
    /* update child natural size first */
    iupBaseComputeNaturalSize(child);
  }

  if (!iupAttribGetBoolean(ih, "CANVASBOX"))
  {
    /* ScrollBox size does not depends on the child size,
     its natural size must be 0 to be free of restrictions. */
    if (ih->currentwidth == 0 && ih->currentheight == 0 && child)
    {
      *w = child->naturalwidth;
      *h = child->naturalheight;
    }
    else
    {
      *w = 0;
      *h = 0;
    }
  }

  /* Also set expand to its own expand so it will not depend on children */
  *children_expand = ih->expand;
}

static void iFlatScrollBoxUpdateVisibleScrollArea(Ihandle* ih, int view_width, int view_height, int sb_horiz, int sb_vert)
{
  /* this is available drawing size not considering the scrollbars (BORDER=NO) */
  int canvas_width = ih->currentwidth,
    canvas_height = ih->currentheight;

  if (!iupAttribGetBoolean(ih, "SHOWFLOATING"))
  {
    int sb_size = iupAttribGetInt(ih, "SCROLLBARSIZE");

    /* if child is greater than scrollbox in one direction,
    then it has scrollbars
    but this affects the opposite direction */

    if (view_width > ih->currentwidth && sb_horiz)  /* check for horizontal scrollbar */
      canvas_height -= sb_size;                /* affect vertical size */
    if (view_height > ih->currentheight && sb_vert)
      canvas_width -= sb_size;

    if (view_width <= ih->currentwidth && view_width > canvas_width && sb_horiz)
      canvas_height -= sb_size;
    if (view_height <= ih->currentheight && view_height > canvas_height && sb_vert)
      canvas_width -= sb_size;
  }

  if (canvas_width < 0) canvas_width = 0;
  if (canvas_height < 0) canvas_height = 0;

  if (sb_horiz)
    IupSetInt(ih, "DX", canvas_width);
  else
    IupSetAttribute(ih, "DX", "0");

  if (sb_vert)
    IupSetInt(ih, "DY", canvas_height);
  else
    IupSetAttribute(ih, "DY", "0");
}

static void iFlatScrollBoxSetChildrenCurrentSizeMethod(Ihandle* ih, int shrink)
{
  Ihandle* child = iFlatScrollBoxGetChild(ih);
  if (child)
  {
    int w, h, has_sb_horiz=0, has_sb_vert=0;
    int sb = iupFlatScrollBarGet(ih);
    int sb_vert = sb & IUP_SB_VERT;
    int sb_horiz = sb & IUP_SB_HORIZ;

    /* If child is greater than scrollbox area, use child natural size,
       else use current scrollbox size;
       So this will let the child be greater than the scrollbox,
       or let the child expand to the scrollbox.  */

    if (child->naturalwidth > ih->currentwidth)
    {
      w = child->naturalwidth;

      if (sb_horiz)
        has_sb_horiz = 1;
    }
    else
      w = ih->currentwidth;  /* expand space */

    if (child->naturalheight > ih->currentheight)
    {
      h = child->naturalheight;

      if (sb_vert)
        has_sb_vert = 1;
    }
    else
      h = ih->currentheight; /* expand space */

    if (!iupAttribGetBoolean(ih, "SHOWFLOATING"))
    {
      int sb_size = iupAttribGetInt(ih, "SCROLLBARSIZE");

      if (!has_sb_horiz && has_sb_vert)
        w -= sb_size;  /* reduce expand space */

      if (has_sb_horiz && !has_sb_vert)
        h -= sb_size;  /* reduce expand space */
    }

    /* Now w and h is a possible child size */
    iupBaseSetCurrentSize(child, w, h, shrink);

    /* Now we use the actual child size as the virtual area */
    if (sb_horiz)
      iupAttribSetInt(ih, "XMAX", child->currentwidth);
    else
      iupAttribSet(ih, "XMAX", "0");

    if (sb_vert)
      iupAttribSetInt(ih, "YMAX", child->currentheight);
    else
      iupAttribSet(ih, "YMAX", "0");

    /* Finally update the visible area */
    iFlatScrollBoxUpdateVisibleScrollArea(ih, child->currentwidth, child->currentheight, sb_horiz, sb_vert);
  }
  else
  {
    iupAttribSet(ih, "XMAX", "0");
    iupAttribSet(ih, "YMAX", "0");

    IupSetAttribute(ih, "DX", "0");
    IupSetAttribute(ih, "DY", "0");
  }

  iupFlatScrollBarSetChildrenCurrentSize(ih, shrink);
}

static void iFlatScrollBoxSetChildrenPositionMethod(Ihandle* ih, int x, int y)
{
  Ihandle* child = iFlatScrollBoxGetChild(ih);
  if (child)
    iFlatScrollBoxUpdateChildPos(ih, child);

  iupFlatScrollBarSetChildrenPosition(ih);

  (void)x;
  (void)y;
}


/******************************************************************************/


static int iFlatScrollBoxCreateMethod(Ihandle* ih, void** params)
{
  /* must be first */
  iupFlatScrollBarCreate(ih);

  /* Setting callbacks */
  IupSetCallback(ih, "FLATSCROLL_CB", (Icallback)iFlatScrollBoxFlatScroll_CB);
  IupSetCallback(ih, "SCROLL_CB", (Icallback)iFlatScrollBoxScroll_CB);
  IupSetCallback(ih, "BUTTON_CB", (Icallback)iFlatScrollBoxButton_CB);
  IupSetCallback(ih, "MOTION_CB", (Icallback)iFlatScrollBoxMotion_CB);
  IupSetCallback(ih, "WHEEL_CB", (Icallback)iFlatScrollBoxWheel_CB);

  IupSetAttribute(ih, "CANFOCUS", "NO");
  IupSetAttribute(ih, "WHEELDROPFOCUS", "YES");

  if (params)
  {
    Ihandle** iparams = (Ihandle**)params;
    if (iparams[0]) IupAppend(ih, iparams[0]);  /* firstchild->brother->brother */
  }

  return IUP_NOERROR;
}

static void iFlatScrollBoxDestroyMethod(Ihandle* ih)
{
  iupFlatScrollBarRelease(ih);
}

Iclass* iupFlatScrollBoxNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("canvas"));

  ic->name   = "flatscrollbox";
  ic->cons = "FlatScrollBox";
  ic->format = "h";   /* one Ihandle* */
  ic->nativetype = IUP_TYPECANVAS;
  ic->childtype = IUP_CHILDMANY+3;  /* 1 child + 2 scrollbars */
  ic->is_interactive = 1;

  /* Class functions */
  ic->New = iupFlatScrollBoxNewClass;
  ic->Create  = iFlatScrollBoxCreateMethod;
  ic->Destroy = iFlatScrollBoxDestroyMethod;

  ic->ComputeNaturalSize = iFlatScrollBoxComputeNaturalSizeMethod;
  ic->SetChildrenCurrentSize = iFlatScrollBoxSetChildrenCurrentSizeMethod;
  ic->SetChildrenPosition = iFlatScrollBoxSetChildrenPositionMethod;

  iupClassRegisterCallback(ic, "LAYOUTUPDATE_CB", "");

  /* Base Container */
  iupClassRegisterAttribute(ic, "EXPAND", iFlatScrollBoxGetExpandAttrib, iFlatScrollBoxSetExpandAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTOFFSET", iupBaseCanvasGetClientOffsetAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTSIZE", iupBaseCanvasGetClientSizeAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_READONLY | IUPAF_NO_INHERIT);

  /* Native Container */
  iupClassRegisterAttribute(ic, "CHILDOFFSET", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  /* replace IupCanvas behavior */
  iupClassRegisterReplaceAttribFunc(ic, "BGCOLOR", iupBaseNativeParentGetBgColorAttrib, NULL);
  iupClassRegisterReplaceAttribDef(ic, "BGCOLOR", "DLGBGCOLOR", NULL);
  iupClassRegisterReplaceAttribDef(ic, "BORDER", "NO", NULL);
  iupClassRegisterReplaceAttribFlags(ic, "BORDER", IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterReplaceAttribFlags(ic, "SCROLLBAR", IUPAF_READONLY | IUPAF_NO_INHERIT);  /* will be always NO, but it will behave as always Yes */
  iupClassRegisterAttribute(ic, "YAUTOHIDE", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_READONLY | IUPAF_NO_INHERIT);  /* will be always Yes */
  iupClassRegisterAttribute(ic, "XAUTOHIDE", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_READONLY | IUPAF_NO_INHERIT);  /* will be always Yes */
  iupClassRegisterReplaceAttribFlags(ic, "XMIN", IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterReplaceAttribFlags(ic, "XMAX", IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterReplaceAttribFlags(ic, "YMIN", IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterReplaceAttribFlags(ic, "YMAX", IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterReplaceAttribFlags(ic, "LINEX", IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterReplaceAttribFlags(ic, "LINEY", IUPAF_NO_SAVE | IUPAF_NO_INHERIT);

  /* Scrollbox */
  iupClassRegisterAttribute(ic, "SCROLLTO", NULL, iFlatScrollBoxSetScrollToAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTOCHILD", NULL, iFlatScrollBoxSetScrollToChildAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTOCHILD_HANDLE", NULL, iFlatScrollBoxSetScrollToChildHandleAttrib, NULL, NULL, IUPAF_IHANDLE | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CANVASBOX", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LAYOUTDRAG", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);

  /* Flat Scrollbar */
  iupFlatScrollBarRegister(ic);

  iupClassRegisterAttribute(ic, "FLATSCROLLBAR", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);

  return ic;
}

IUP_API Ihandle* IupFlatScrollBox(Ihandle* child)
{
  void *children[2];
  children[0] = (void*)child;
  children[1] = NULL;
  return IupCreatev("flatscrollbox", children);
}
