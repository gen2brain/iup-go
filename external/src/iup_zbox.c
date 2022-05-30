/** \file
 * \brief Zbox Control.
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
#include "iup_varg.h"


enum{IZBOX_ALIGN_NORTH, IZBOX_ALIGN_SOUTH, IZBOX_ALIGN_WEST, IZBOX_ALIGN_EAST,
     IZBOX_ALIGN_NE, IZBOX_ALIGN_SE, IZBOX_ALIGN_NW, IZBOX_ALIGN_SW,
     IZBOX_ALIGN_ACENTER};

struct _IcontrolData 
{
  int alignment;
  Ihandle* value_handle;
};

static int iZboxCreateMethod(Ihandle* ih, void** params)
{
  ih->data = iupALLOCCTRLDATA();

  ih->data->alignment = IZBOX_ALIGN_NW;

  if (params)
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

static void iZboxChildAddedMethod(Ihandle* ih, Ihandle* child)
{
  /* make sure it has at least one name */
  if (!iupAttribGetHandleName(child))
    iupAttribSetHandleName(child);

  if (!ih->data->value_handle)
  {
    IupSetAttribute(child, "VISIBLE", IupGetAttribute(ih, "VISIBLE"));
    ih->data->value_handle = child;
  }
  else
    IupSetAttribute(child, "VISIBLE", "NO");
}

static void iZboxChildRemovedMethod(Ihandle* ih, Ihandle* child, int pos)
{
  (void)pos;

  if (child == ih->data->value_handle)
  {
    /* reset to the first child, even if it is NULL */
    if (ih->firstchild)
      IupSetAttribute(ih->firstchild, "VISIBLE", IupGetAttribute(ih, "VISIBLE"));
    ih->data->value_handle = ih->firstchild;

    if (!iupAttribGetBoolean(ih, "CHILDSIZEALL"))
      IupRefresh(ih);
  }
}

static int iZboxSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "NORTH") || iupStrEqualNoCase(value, "ATOP"))
    ih->data->alignment = IZBOX_ALIGN_NORTH;
  else if (iupStrEqualNoCase(value, "SOUTH") || iupStrEqualNoCase(value, "ABOTTOM"))
    ih->data->alignment = IZBOX_ALIGN_SOUTH;
  else if (iupStrEqualNoCase(value, "WEST") || iupStrEqualNoCase(value, "ALEFT"))
    ih->data->alignment = IZBOX_ALIGN_WEST;
  else if (iupStrEqualNoCase(value, "EAST") || iupStrEqualNoCase(value, "ARIGHT"))
    ih->data->alignment = IZBOX_ALIGN_EAST;
  else if (iupStrEqualNoCase(value, "NE"))
    ih->data->alignment = IZBOX_ALIGN_NE;
  else if (iupStrEqualNoCase(value, "SE"))
    ih->data->alignment = IZBOX_ALIGN_SE;
  else if (iupStrEqualNoCase(value, "NW"))
    ih->data->alignment = IZBOX_ALIGN_NW;
  else if (iupStrEqualNoCase(value, "SW"))
    ih->data->alignment = IZBOX_ALIGN_SW;
  else if (iupStrEqualNoCase(value, "ACENTER"))
    ih->data->alignment = IZBOX_ALIGN_ACENTER;
  return 0;
}

static char* iZboxGetAlignmentAttrib(Ihandle* ih)
{
  static char* align2str[9] = {"NORTH", "SOUTH", "WEST", "EAST",
                               "NE", "SE", "NW", "SW",
                               "ACENTER"};
  return align2str[ih->data->alignment];
}

static int iZboxSetValueHandleAttrib(Ihandle* ih, const char* value)
{
  int visible;
  Ihandle* old_handle, *new_handle, *child;

  new_handle = (Ihandle*)value;
  if (!iupObjectCheck(new_handle))
    return 0;

  old_handle = ih->data->value_handle;
  if (!iupObjectCheck(old_handle))
    old_handle = NULL;

  if (old_handle == new_handle)
    return 0;

  visible = IupGetInt(ih, "VISIBLE");

  for (child = ih->firstchild; child; child = child->brother)
  {
    if (child == new_handle) /* found child */
    {
      if (old_handle && old_handle != new_handle)
        IupSetAttribute(old_handle, "VISIBLE", "NO");

      IupSetAttribute(new_handle, "VISIBLE", visible? "YES": "NO");
      ih->data->value_handle = new_handle;

      if (!iupAttribGetBoolean(ih, "CHILDSIZEALL"))
        IupRefresh(ih);

      return 0;
    }
  }
 
  return 0;
}

static char* iZboxGetValueHandleAttrib(Ihandle* ih)
{
  return (char*)(ih->data->value_handle);
}

static int iZboxSetValuePosAttrib(Ihandle* ih, const char* value)
{
  Ihandle* child;
  int pos, i;

  if (!iupStrToInt(value, &pos))
    return 0;

  for (i=0, child=ih->firstchild; child; child = child->brother, i++)
  {
    if (i == pos) /* found child */
    {
      iZboxSetValueHandleAttrib(ih, (char*)child);
      return 0;
    }
  }
 
  return 0;
}

static char* iZboxGetValuePosAttrib(Ihandle* ih)
{
  Ihandle* child;
  int pos;

  if (!iupObjectCheck(ih->data->value_handle))
    return NULL;

  for (pos=0, child = ih->firstchild; child; child = child->brother, pos++)
  {
    if (child == ih->data->value_handle) /* found child */
      return iupStrReturnInt(pos);
  }

  return NULL;
}

static int iZboxSetValueAttrib(Ihandle* ih, const char* value)
{
  Ihandle *new_handle;

  if (!value)
    return 0;

  new_handle = IupGetHandle(value);
  if (!new_handle)
    return 0;

  iZboxSetValueHandleAttrib(ih, (char*)new_handle);

  return 0;
}

static char* iZboxGetValueAttrib(Ihandle* ih)
{
  Ihandle* child;
  int pos;

  if (!iupObjectCheck(ih->data->value_handle))
    return NULL;

  for (pos=0, child = ih->firstchild; child; child = child->brother, pos++)
  {
    if (child == ih->data->value_handle) /* found child, just checking */
      return IupGetName(ih->data->value_handle);  /* Name is guarantied at AddedMethod */
  }

  return NULL;
}

static int iZboxSetVisibleAttrib(Ihandle* ih, const char* value)
{
  if (iupObjectCheck(ih->data->value_handle))
    IupSetAttribute(ih->data->value_handle, "VISIBLE", (char*)value);
  return 1;  /* must be 1 to mark when set at the element */
}

static void iZboxComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  Ihandle* child;
  int children_naturalwidth, children_naturalheight;
  int childSizeAll = iupAttribGetBoolean(ih, "CHILDSIZEALL");

  /* calculate total children natural size (even for hidden children) */
  children_naturalwidth = 0;
  children_naturalheight = 0;

  for (child = ih->firstchild; child; child = child->brother)
  {
    /* update child natural size first */
    if (!(child->flags & IUP_FLOATING_IGNORE))
      iupBaseComputeNaturalSize(child);

    if (!childSizeAll && child != ih->data->value_handle)
      continue;

    if (!(child->flags & IUP_FLOATING))
    {
      *children_expand |= child->expand;
      children_naturalwidth = iupMAX(children_naturalwidth, child->naturalwidth);
      children_naturalheight = iupMAX(children_naturalheight, child->naturalheight);
    }
  }

  *w = children_naturalwidth;
  *h = children_naturalheight;
}

static void iZboxSetChildrenCurrentSizeMethod(Ihandle* ih, int shrink)
{
  Ihandle* child;
  for (child = ih->firstchild; child; child = child->brother)
  {
    if (!(child->flags & IUP_FLOATING))
      iupBaseSetCurrentSize(child, ih->currentwidth, ih->currentheight, shrink);
  }
}

static void iZboxSetChildrenPositionMethod(Ihandle* ih, int x, int y)
{
  int dx = 0, dy = 0;
  Ihandle* child;

  for (child = ih->firstchild; child; child = child->brother)
  {
    if (!(child->flags & IUP_FLOATING))
    {
      switch (ih->data->alignment)
      {
      case IZBOX_ALIGN_ACENTER:
        dx=(ih->currentwidth-child->currentwidth)/2;
        dy=(ih->currentheight-child->currentheight)/2;
        break;
      case IZBOX_ALIGN_NORTH:
        dx=(ih->currentwidth-child->currentwidth)/2;
        dy=0;
        break;
      case IZBOX_ALIGN_SOUTH:
        dx=(ih->currentwidth-child->currentwidth)/2;
        dy=ih->currentheight-child->currentheight;
        break;
      case IZBOX_ALIGN_WEST:
        dx=0;
        dy=(ih->currentheight-child->currentheight)/2;
        break;
      case IZBOX_ALIGN_EAST:
        dx=ih->currentwidth-child->currentwidth;
        dy=(ih->currentheight-child->currentheight)/2;
        break;
      case IZBOX_ALIGN_NE:
        dx=ih->currentwidth-child->currentwidth;
        dy=0;
        break;
      case IZBOX_ALIGN_SE:
        dx=ih->currentwidth-child->currentwidth;
        dy=ih->currentheight-child->currentheight;
        break;
      case IZBOX_ALIGN_SW:
        dx=0;
        dy=ih->currentheight-child->currentheight;
        break;
      case IZBOX_ALIGN_NW:
      default:
        dx=0;
        dy=0;
        break;
      }
      if (dx<0) dx = 0;
      if (dy<0) dy = 0;
                     
      /* update child */
      iupBaseSetPosition(child, x+dx, y+dy);
    }
  }
}


/******************************************************************************/


IUP_API Ihandle* IupZboxv(Ihandle **children)
{
  return IupCreatev("zbox", (void**)children);
}

IUP_API Ihandle* IupZboxV(Ihandle* child, va_list arglist)
{
  return IupCreateV("zbox", child, arglist);
}

IUP_API Ihandle* IupZbox(Ihandle* child, ...)
{
  Ihandle *ih;

  va_list arglist;
  va_start(arglist, child);
  ih = IupCreateV("zbox", child, arglist);
  va_end(arglist);

  return ih;
}

Iclass* iupZboxNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "zbox";
  ic->format = "g"; /* array of Ihandle */
  ic->nativetype = IUP_TYPEVOID;
  ic->childtype = IUP_CHILDMANY;  /* can have children */
  ic->is_interactive = 0;

  /* Class functions */
  ic->New = iupZboxNewClass;
  ic->Create = iZboxCreateMethod;
  ic->Map = iupBaseTypeVoidMapMethod;
  ic->ChildAdded = iZboxChildAddedMethod;
  ic->ChildRemoved = iZboxChildRemovedMethod;

  ic->ComputeNaturalSize = iZboxComputeNaturalSizeMethod;
  ic->SetChildrenCurrentSize = iZboxSetChildrenCurrentSizeMethod;
  ic->SetChildrenPosition = iZboxSetChildrenPositionMethod;

  /* Base Callbacks */
  iupBaseRegisterBaseCallbacks(ic);

  /* Common */
  iupBaseRegisterCommonAttrib(ic);

  /* Base Container */
  iupClassRegisterAttribute(ic, "EXPAND", iupBaseContainerGetExpandAttrib, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTSIZE", iupBaseGetClientSizeAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTOFFSET", iupBaseGetClientOffsetAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  /* Zbox only */
  iupClassRegisterAttribute(ic, "ALIGNMENT", iZboxGetAlignmentAttrib, iZboxSetAlignmentAttrib, IUPAF_SAMEASSYSTEM, "NW", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", iZboxGetValueAttrib, iZboxSetValueAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUEPOS", iZboxGetValuePosAttrib, iZboxSetValuePosAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE_HANDLE", iZboxGetValueHandleAttrib, iZboxSetValueHandleAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT | IUPAF_IHANDLE | IUPAF_NO_STRING);
  iupClassRegisterAttribute(ic, "CHILDSIZEALL", NULL, NULL, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NO_INHERIT);

  /* Intercept VISIBLE since ZBOX works by showing and hiding its children */
  iupClassRegisterAttribute(ic, "VISIBLE", NULL, iZboxSetVisibleAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_SAVE|IUPAF_NOT_MAPPED);

  return ic;
}
