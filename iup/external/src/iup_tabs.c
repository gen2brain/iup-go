/** \file
* \brief iuptabs control
*
* See Copyright Notice in "iup.h"
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_stdcontrols.h"
#include "iup_layout.h"
#include "iup_image.h"
#include "iup_tabs.h"
#include "iup_varg.h"


char* iupTabsGetTabPaddingAttrib(Ihandle* ih)
{
  return iupStrReturnIntInt(ih->data->horiz_padding, ih->data->vert_padding, 'x');
}

static int iTabsGetMaxWidth(Ihandle* ih)
{
  int max_width = 0, width, pos;
  char *tabtitle, *tabimage;
  Ihandle* child;

  for (pos = 0, child = ih->firstchild; child; child = child->brother, pos++)
  {
    tabtitle = iupAttribGetId(ih, "TABTITLE", pos);
    if (!tabtitle) tabtitle = iupAttribGet(child, "TABTITLE");
    tabimage = iupAttribGetId(ih, "TABIMAGE", pos);
    if (!tabimage) tabimage = iupAttribGet(child, "TABIMAGE");
    if (!tabtitle && !tabimage)
      tabtitle = "     ";

    width = 0;
    if (tabtitle)
      width += iupdrvFontGetStringWidth(ih, tabtitle);

    if (tabimage)
    {
      void* img = iupImageGetImage(tabimage, ih, 0, NULL);
      if (img)
      {
        int w;
        iupdrvImageGetInfo(img, &w, NULL, NULL);
        width += w;
      }
    }

    if (width > max_width) max_width = width;
  }

  return max_width;
}

static int iTabsGetMaxHeight(Ihandle* ih)
{
  int max_height = 0, h, pos;
  char *tabimage;
  Ihandle* child;

  for (pos = 0, child = ih->firstchild; child; child = child->brother, pos++)
  {
    tabimage = iupAttribGetId(ih, "TABIMAGE", pos);
    if (!tabimage) tabimage = iupAttribGet(child, "TABIMAGE");

    if (tabimage)
    {
      void* img = iupImageGetImage(tabimage, ih, 0, NULL);
      if (img)
      {
        iupdrvImageGetInfo(img, NULL, &h, NULL);
        if (h > max_height) max_height = h;
      }
    }
  }

  iupdrvFontGetCharSize(ih, NULL, &h);
  if (h > max_height) max_height = h;

  return max_height;
}

static void iTabsGetDecorMargin(int *m, int *s)
{
  int e = iupdrvTabsExtraMargin();
  *m = 4 + e;
  *s = 2 + 2*e;
}

static void iTabsGetDecorSize(Ihandle* ih, int *width, int *height)
{
  int m, s;
  iTabsGetDecorMargin(&m, &s);

  if (ih->data->type == ITABS_LEFT || ih->data->type == ITABS_RIGHT)
  {
    if (ih->data->orientation == ITABS_HORIZONTAL)
    {
      int max_width = iTabsGetMaxWidth(ih);
      *width  = m + (3 + max_width + 3) + s + m;
      *height = m + m;

      if (iupdrvTabsExtraDecor(ih))
      {
        int h;
        iupdrvFontGetCharSize(ih, NULL, &h);
        *height += h + m;
      }
    }
    else
    {
      int max_height = iTabsGetMaxHeight(ih);
      *width  = m + (3 + max_height + 3) + s + m;
      *height = m + m;

      if (ih->handle && ih->data->is_multiline)
      {
        int num_lin = iupdrvTabsGetLineCountAttrib(ih);
        *width += (num_lin-1)*(3 + max_height + 3 + 1);
      }
    }
  }
  else /* "BOTTOM" or "TOP" */
  {
    if (ih->data->orientation == ITABS_HORIZONTAL)
    {
      int max_height = iTabsGetMaxHeight(ih);
      *width  = m + m;
      *height = m + (3 + max_height + 3) + s + m;

      if (ih->handle && ih->data->is_multiline)
      {
        int num_lin = iupdrvTabsGetLineCountAttrib(ih);
        *height += (num_lin-1)*(3 + max_height + 3 + 1);
      }

      if (iupdrvTabsExtraDecor(ih))
      {
        int h;
        iupdrvFontGetCharSize(ih, NULL, &h);
        *width += h + m;
      }
    }
    else
    {
      int max_width = iTabsGetMaxWidth(ih);
      *width  = m + m;
      *height = m + (3 + max_width + 3) + s + m;
    }
  }

  *width  += ih->data->horiz_padding;
  *height += ih->data->vert_padding;
}

static void iTabsGetDecorOffset(Ihandle* ih, int *dx, int *dy)
{
  int m, s;
  iTabsGetDecorMargin(&m, &s);

  if (ih->data->type == ITABS_LEFT || ih->data->type == ITABS_RIGHT)
  {
    if (ih->data->type == ITABS_LEFT)
    {
      if (ih->data->orientation == ITABS_HORIZONTAL)
      {
        int max_width = iTabsGetMaxWidth(ih);
        *dx = m + (3 + max_width + 3) + s;
      }
      else
      {
        int max_height = iTabsGetMaxHeight(ih);
        *dx = m + (3 + max_height + 3) + s;

        if (ih->handle && ih->data->is_multiline)
        {
          int num_lin = iupdrvTabsGetLineCountAttrib(ih);
          *dx += (num_lin-1)*(3 + max_height + 3 + 1);
        }
      }
    }
    else
      *dx = m;

    *dy = m;
  }
  else /* "BOTTOM" or "TOP" */
  {
    if (ih->data->type == ITABS_TOP)
    {
      if (ih->data->orientation == ITABS_HORIZONTAL)
      {
        int max_height = iTabsGetMaxHeight(ih);
        *dy = m + (3 + max_height + 3) + s;

        if (ih->handle && ih->data->is_multiline)
        {
          int num_lin = iupdrvTabsGetLineCountAttrib(ih);
          *dy += (num_lin-1)*(3 + max_height + 3 + 1);
        }
      }
      else
      {
        int max_width = iTabsGetMaxWidth(ih);
        *dy = m + (3 + max_width + 3) + s;
      }
    }
    else
      *dy = m;

    *dx = m;
  }

  *dx += ih->data->horiz_padding;
  *dy += ih->data->vert_padding;
}

void iupTabsCheckCurrentTab(Ihandle* ih, int pos, int removed)
{
  int cur_pos = iupdrvTabsGetCurrentTab(ih);
  if (cur_pos == pos)
  {
    int p;

    /* if given tab is the current tab, 
       then the current tab must be changed to a visible tab */
    Ihandle* child;

    /* this function is called after the child has being removed from the hierarchy,
       but before the system tab being removed. */

    p = 0;
    if (removed && p == pos)
      p++;

    for (child = ih->firstchild; child; child = child->brother)
    {
      if (p != pos && iupdrvTabsIsTabVisible(child, p))
      {
        iupdrvTabsSetCurrentTab(ih, p);

        if (!iupAttribGetBoolean(ih, "CHILDSIZEALL"))
          IupRefresh(ih);

        return;
      }

      p++;
      if (removed && p == pos)
        p++;  /* increment twice to compensate for child already removed */
    }
  }
}

static void iTabsSetTab(Ihandle* ih, Ihandle* child, int pos)
{
  if (ih->handle)
  {
    int cur_pos = iupdrvTabsGetCurrentTab(ih);
    if (cur_pos != pos && iupdrvTabsIsTabVisible(child, pos))
    {
      iupdrvTabsSetCurrentTab(ih, pos);

      if (!iupAttribGetBoolean(ih, "CHILDSIZEALL"))
        IupRefresh(ih);
    }
  }
  else
    iupAttribSet(ih, "_IUPTABS_VALUE_HANDLE", (char*)child);
}


/* ------------------------------------------------------------------------- */
/* TABS - Sets and Gets - Attribs                                           */
/* ------------------------------------------------------------------------- */

char* iupTabsGetTabTypeAttrib(Ihandle* ih)
{
  switch(ih->data->type)
  {
  case ITABS_BOTTOM:
    return "BOTTOM";
  case ITABS_LEFT:
    return "LEFT";
  case ITABS_RIGHT:
    return "RIGHT";
  default:
    return "TOP";
  }
}

char* iupTabsGetTabOrientationAttrib(Ihandle* ih)
{
  if (ih->data->orientation == ITABS_HORIZONTAL)
    return "HORIZONTAL";
  else
    return "VERTICAL";
}

static char* iTabsGetCountAttrib(Ihandle* ih)
{
  return iupStrReturnInt(IupGetChildCount(ih));
}

static int iTabsSetValueHandleAttrib(Ihandle* ih, const char* value)
{
  int pos;
  Ihandle *child;

  child = (Ihandle*)value;

  if (!iupObjectCheck(child))
    return 0;

  pos = IupGetChildPos(ih, child);
  if (pos != -1) /* found child */
    iTabsSetTab(ih, child, pos);

  return 0;
}

static char* iTabsGetValueHandleAttrib(Ihandle* ih)
{
  if (ih->handle)
  {
    int pos = iupdrvTabsGetCurrentTab(ih);
    return (char*)IupGetChild(ih, pos);
  }
  else
    return iupAttribGet(ih, "_IUPTABS_VALUE_HANDLE");
}

static int iTabsSetValuePosAttrib(Ihandle* ih, const char* value)
{
  Ihandle* child;
  int pos;

  if (!iupStrToInt(value, &pos))
    return 0;

  child = IupGetChild(ih, pos);
  if (child) /* found child */
    iTabsSetTab(ih, child, pos);
 
  return 0;
}

static char* iTabsGetValuePosAttrib(Ihandle* ih)
{
  if (ih->handle)
  {
    int pos = iupdrvTabsGetCurrentTab(ih);
    return iupStrReturnInt(pos);
  }
  else
  {
    Ihandle* child = (Ihandle*)iupAttribGet(ih, "_IUPTABS_VALUE_HANDLE");
    int pos = IupGetChildPos(ih, child);
    if (pos != -1) /* found child */
      return iupStrReturnInt(pos);
  }

  return NULL;
}

static int iTabsSetValueAttrib(Ihandle* ih, const char* value)
{
  Ihandle *child;

  if (!value)
    return 0;

  child = IupGetHandle(value);
  if (!child)
    return 0;

  iTabsSetValueHandleAttrib(ih, (char*)child);

  return 0;
}

static char* iTabsGetValueAttrib(Ihandle* ih)
{
  Ihandle* child = (Ihandle*)iTabsGetValueHandleAttrib(ih);
  return IupGetName(child);  /* Name is guarantied at AddedMethod */
}

static char* iTabsGetClientSizeAttrib(Ihandle* ih)
{
  int width, height, decorwidth, decorheight;
  width = ih->currentwidth;
  height = ih->currentheight;
  iTabsGetDecorSize(ih, &decorwidth, &decorheight);
  width -= decorwidth;
  height -= decorheight;
  if (width < 0) width = 0;
  if (height < 0) height = 0;
  return iupStrReturnIntInt(width, height,'x');
}

static char* iTabsGetClientOffsetAttrib(Ihandle* ih)
{
  int dx, dy;
  iTabsGetDecorOffset(ih, &dx, &dy);
  return iupStrReturnIntInt(dx, dy, 'x');
}

char* iupTabsGetTabVisibleAttrib(Ihandle* ih, int pos)
{
  Ihandle* child = IupGetChild(ih, pos);
  if (child)
    return iupStrReturnBoolean(iupdrvTabsIsTabVisible(child, pos));
  else
    return NULL;
}

char* iupTabsGetTitleAttrib(Ihandle* ih, int pos)
{
  Ihandle* child = IupGetChild(ih, pos);
  if (child)
    return iupAttribGet(child, "TABTITLE");
  else
    return NULL;
}

static char* iTabsGetShowCloseAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean (ih->data->show_close); 
}

static int iTabsSetShowCloseAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->show_close = 1;
  else
    ih->data->show_close = 0;

  return 0;
}

/* ------------------------------------------------------------------------- */
/* TABS - Methods                                                            */
/* ------------------------------------------------------------------------- */

static void iTabsComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  Ihandle* child;
  int children_naturalwidth, children_naturalheight;
  int decorwidth, decorheight;
  int childSizeAll = iupAttribGetBoolean(ih, "CHILDSIZEALL");
  Ihandle* current_child = childSizeAll? NULL: IupGetChild(ih, iupdrvTabsGetCurrentTab(ih));

  /* calculate total children natural size (even for hidden children) */
  children_naturalwidth = 0;
  children_naturalheight = 0;

  for (child = ih->firstchild; child; child = child->brother)
  {
    /* update child natural size first */
    if (!(child->flags & IUP_FLOATING_IGNORE))
      iupBaseComputeNaturalSize(child);

    if (!childSizeAll && child != current_child)
      continue;

    if (!(child->flags & IUP_FLOATING))
    {
      *children_expand |= child->expand;
      children_naturalwidth = iupMAX(children_naturalwidth, child->naturalwidth);
      children_naturalheight = iupMAX(children_naturalheight, child->naturalheight);
    }
  }

  iTabsGetDecorSize(ih, &decorwidth, &decorheight);

  *w = children_naturalwidth + decorwidth;
  *h = children_naturalheight + decorheight;
}

static void iTabsSetChildrenCurrentSizeMethod(Ihandle* ih, int shrink)
{
  Ihandle* child;
  int width, height, decorwidth, decorheight;

  iTabsGetDecorSize(ih, &decorwidth, &decorheight);

  width = ih->currentwidth-decorwidth;
  height = ih->currentheight-decorheight;
  if (width < 0) width = 0;
  if (height < 0) height = 0;

  for (child = ih->firstchild; child; child = child->brother)
  {
    if (!(child->flags & IUP_FLOATING))
      iupBaseSetCurrentSize(child, width, height, shrink);
  }
}

static void iTabsSetChildrenPositionMethod(Ihandle* ih, int x, int y)
{
  /* In all systems, each tab is a native window covering the client area.
     Child coordinates are relative to client left-top corner of the tab page. */
  Ihandle* child;
  char* offset = iupAttribGet(ih, "CHILDOFFSET");

  /* Native container, position is reset */
  x = 0;
  y = 0;

  if (offset) iupStrToIntInt(offset, &x, &y, 'x');

  for (child = ih->firstchild; child; child = child->brother)
  {
    if (!(child->flags & IUP_FLOATING))
      iupBaseSetPosition(child, x, y);
  }
}

static void* iTabsGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
  while (child && child->parent != ih)
    child = child->parent;
  if (child)
    return iupAttribGet(child, "_IUPTAB_CONTAINER");
  else
    return NULL;
}

static int iTabsCreateMethod(Ihandle* ih, void **params)
{
  ih->data = iupALLOCCTRLDATA();

  /* add children */
  if(params)
  {
    Ihandle** iparams = (Ihandle**)params;
    while (*iparams) 
    {
      IupAppend(ih, *iparams);
      iparams++;
    }
  }
  return IUP_NOERROR;
}

Iclass* iupTabsNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "tabs";
  ic->format = "g"; /* array of Ihandle */
  ic->nativetype = IUP_TYPECONTROL;
  ic->childtype = IUP_CHILDMANY;  /* can have children */
  ic->is_interactive = 1;
  ic->has_attrib_id = 1;

  /* Class functions */
  ic->New = iupTabsNewClass;
  ic->Create  = iTabsCreateMethod;
  ic->GetInnerNativeContainerHandle = iTabsGetInnerNativeContainerHandleMethod;

  ic->ComputeNaturalSize = iTabsComputeNaturalSizeMethod;
  ic->SetChildrenCurrentSize     = iTabsSetChildrenCurrentSizeMethod;
  ic->SetChildrenPosition        = iTabsSetChildrenPositionMethod;

  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;

  /* IupTabs Callbacks */
  iupClassRegisterCallback(ic, "TABCHANGE_CB", "nn");
  iupClassRegisterCallback(ic, "TABCHANGEPOS_CB", "ii");
  iupClassRegisterCallback(ic, "RIGHTCLICK_CB", "i");
  iupClassRegisterCallback(ic, "FOCUS_CB", "i");

  /* Common Callbacks */
  iupBaseRegisterCommonCallbacks(ic);

  /* Common */
  iupBaseRegisterCommonAttrib(ic);

  /* Visual */
  iupBaseRegisterVisualAttrib(ic);

  /* IupTabs only */
  iupClassRegisterAttribute(ic, "VALUE", iTabsGetValueAttrib, iTabsSetValueAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUEPOS", iTabsGetValuePosAttrib, iTabsSetValuePosAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_SAVE|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE_HANDLE", iTabsGetValueHandleAttrib, iTabsSetValueHandleAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT | IUPAF_IHANDLE | IUPAF_NO_STRING);
  iupClassRegisterAttribute(ic, "COUNT", iTabsGetCountAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWCLOSE", iTabsGetShowCloseAttrib, iTabsSetShowCloseAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CHILDSIZEALL", NULL, NULL, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NO_INHERIT);

  /* Base Container */
  iupClassRegisterAttribute(ic, "CLIENTSIZE", iTabsGetClientSizeAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTOFFSET", iTabsGetClientOffsetAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EXPAND", iupBaseContainerGetExpandAttrib, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  /* Native Container */
  iupClassRegisterAttribute(ic, "CHILDOFFSET", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupdrvTabsInitClass(ic);

  return ic;
}

IUP_API Ihandle* IupTabsv(Ihandle** params)
{
  return IupCreatev("tabs", (void**)params);
}

IUP_API Ihandle* IupTabsV(Ihandle* child, va_list arglist)
{
  return IupCreateV("tabs", child, arglist);
}

IUP_API Ihandle* IupTabs(Ihandle* child, ...)
{
  Ihandle *ih;

  va_list arglist;
  va_start(arglist, child);
  ih = IupCreateV("tabs", child, arglist);
  va_end(arglist);

  return ih;
}
