/** \file
 * \brief Abstract Layout Management
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>  
#include <stdio.h>  
#include <stdarg.h>  

#include "iup.h"

#include "iup_object.h"
#include "iup_drv.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_layout.h"
#include "iup_assert.h" 

 
IUP_API void IupRefreshChildren(Ihandle* ih)
{
  int shrink;
  Ihandle *dialog, *child;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  /* must have children */
  if (!ih->firstchild)
    return;

  /* must be inside a dialog */
  /* it can not be a dialog */
  dialog = IupGetDialog(ih);
  if (!dialog || dialog==ih)
    return;

  /****** local iupLayoutCompute,
     but ih will not be changed, only its children. */

  shrink = iupAttribGetBoolean(dialog, "SHRINK");

  /* children only iupBaseComputeNaturalSize */
  {
    int w=0, h=0, children_expand=ih->expand;
    iupClassObjectComputeNaturalSize(ih, &w, &h, &children_expand);

    /* If the container natural size changed from inside, simply ignore the change */
  }

  /* children only iupBaseSetCurrentSize */
  iupClassObjectSetChildrenCurrentSize(ih, shrink);

  /* children only iupBaseSetPosition */
  iupClassObjectSetChildrenPosition(ih, ih->x, ih->y);


  /****** local iupLayoutUpdate,
     but ih will not be changed, only its children. */
  for (child = ih->firstchild; child; child = child->brother)
  {
    if (child->handle)
      iupLayoutUpdate(child);
  }
}

IUP_API void IupRefresh(Ihandle* ih)
{
  Ihandle* dialog;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  dialog = IupGetDialog(ih);
  if (dialog)
  {
    iupLayoutCompute(dialog);

    if (dialog->handle)
      iupLayoutUpdate(dialog);
  }
}

IUP_API void IupUpdate(Ihandle* ih)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (ih->handle && ih->iclass->nativetype != IUP_TYPEVOID)
    iupdrvPostRedraw(ih);
}

static void iLayoutDisplayUpdateChildren(Ihandle *ih)
{
  Ihandle* child;
  for (child = ih->firstchild; child; child = child->brother)
  {
    iLayoutDisplayUpdateChildren(child);
    IupUpdate(child);
  }
}

IUP_API void IupUpdateChildren(Ihandle* ih)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  iLayoutDisplayUpdateChildren(ih);
}

static void iLayoutDisplayRedrawChildren(Ihandle *ih)
{
  Ihandle* child;
  for (child = ih->firstchild; child; child = child->brother)
  {
    iLayoutDisplayRedrawChildren(child);

    if (child->handle && child->iclass->nativetype != IUP_TYPEVOID)
      iupdrvRedrawNow(child);
  }
}

IUP_API void IupRedraw(Ihandle* ih, int children)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (ih->handle && ih->iclass->nativetype != IUP_TYPEVOID)
    iupdrvRedrawNow(ih);

  if (children)
    iLayoutDisplayRedrawChildren(ih);
}

IUP_SDK_API void iupLayoutUpdate(Ihandle* ih)
{
  Ihandle* child;

  if (ih->flags & IUP_FLOATING_IGNORE)
    return;

  /* update size and position of the native control */
  iupClassObjectLayoutUpdate(ih);

  /* update its children */
  for (child = ih->firstchild; child; child = child->brother)
  {
    if (child->handle)
      iupLayoutUpdate(child);
  }
}

void iupLayoutCompute(Ihandle* ih)
{
  /* usually called only for the dialog */

  int shrink = iupAttribGetBoolean(ih, "SHRINK");

  /* Compute the natural size for all elements in the dialog,   
     using the minimum visible size and the defined user size.
     The minimum visible size is the size where all the controls can display
     all their contents.
     The defined user size is used to increase the value of the minimum visible size for containers,
     for standard controls will replace the minimum visible size.
     So the native size will be the maximum value between 
     minimum visible size and defined user size.
     Also calculates the expand configuration for each element, but expand is used only in SetChildrenCurrentSize.
     SEQUENCE: will first calculate the native size for the children, then for the element. */
  iupBaseComputeNaturalSize(ih);

  /* Set the current size (not reflected in the native element yet) based on
     the natural size and the expand configuration. 
     If shrink is 0 (default) the current size of containers can be only larger than the natural size,
     the result will depend on the EXPAND attribute.
     If shrink is 1 the containers can be resized to sizes smaller than the natural size.
     SEQUENCE: will first calculate the current size of the element, then for the children. */
  iupBaseSetCurrentSize(ih, 0, 0, shrink);

  /* Now that the current size is known, set the position of the elements 
     relative to the parent.
     SEQUENCE: will first set the position of the element, then for the children. */
  iupBaseSetPosition(ih, 0, 0);
}

IUP_SDK_API void iupLayoutApplyMinMaxSize(Ihandle* ih, int *w, int *h)
{
  if (ih->flags & IUP_MINSIZE)
  {
    char* value = iupAttribGet(ih, "MINSIZE");
    int min_w = 0, min_h = 0;          /* MINSIZE default value */
    iupStrToIntInt(value, &min_w, &min_h, 'x');
    if (w && *w < min_w) *w = min_w;
    if (h && *h < min_h) *h = min_h;
  }

  if (ih->flags & IUP_MAXSIZE)
  {
    char* value = iupAttribGet(ih, "MAXSIZE");
    int max_w = 65535, max_h = 65535;  /* MAXSIZE default value */
    iupStrToIntInt(value, &max_w, &max_h, 'x');
    if (w && *w > max_w) *w = max_w;
    if (h && *h > max_h) *h = max_h;
  }
}

void iupBaseComputeNaturalSize(Ihandle* ih)
{
  /* always initialize the natural size using the user size */
  ih->naturalwidth = ih->userwidth;
  ih->naturalheight = ih->userheight;

  if (ih->iclass->childtype != IUP_CHILDNONE || 
      ih->iclass->nativetype == IUP_TYPEDIALOG)  /* pre-defined dialogs can restrict the number of children */
  {
    int w=0, h=0, children_expand=0;  /* if there is no children will not expand, when not a dialog */

    /* If a container then update the "expand" member from the EXPAND attribute.
       The ih->expand member can not be used for the container attribute because
       it is used to combine the container value with the children value. */
    iupBaseContainerUpdateExpand(ih);

    /* for containers always compute */
    iupClassObjectComputeNaturalSize(ih, &w, &h, &children_expand);

    if (ih->iclass->nativetype == IUP_TYPEDIALOG)
    {
      /* only update the natural size if user size is not defined. */
      /* IupDialog is the only container where this must be done */ 
      /* if the natural size is bigger than the actual dialog size then
         the dialog will be resized, if smaller then the dialog remains with the same size. */
      ih->expand |= children_expand;
      if (ih->naturalwidth <= 0) ih->naturalwidth = iupMAX(ih->currentwidth, w);
      if (ih->naturalheight <= 0) ih->naturalheight = iupMAX(ih->currentheight, h);
    }
    else
    {
      /* combine to only expand if the children can expand */
      ih->expand &= children_expand; 
      ih->naturalwidth = iupMAX(ih->naturalwidth, w);
      ih->naturalheight = iupMAX(ih->naturalheight, h);
    }
  }
  else 
  {
    /* for non-container only compute if user size is not defined */
    if (ih->naturalwidth <= 0 || ih->naturalheight <= 0)
    {
      int w=0, h=0, 
          children_expand;  /* unused if not a container */
      iupClassObjectComputeNaturalSize(ih, &w, &h, &children_expand);

      if (ih->naturalwidth <= 0) ih->naturalwidth = w;
      if (ih->naturalheight <= 0) ih->naturalheight = h;
    }
  }

  /* crop the natural size */
  iupLayoutApplyMinMaxSize(ih, &(ih->naturalwidth), &(ih->naturalheight));
}

void iupBaseSetCurrentSize(Ihandle* ih, int w, int h, int shrink)
{
  if (ih->iclass->nativetype == IUP_TYPEDIALOG)
  {
    /* w and h parameters here are ignored, because they are always 0 for the dialog. */

    /* current size is zero before map and when reset by the application */
    /* after that the current size must follow the actual size of the dialog */
    if (!ih->currentwidth)  ih->currentwidth  = ih->naturalwidth;
    if (!ih->currentheight) ih->currentheight = ih->naturalheight;
  }
  else
  {
    if (ih->iclass->childtype != IUP_CHILDNONE && !shrink)
    {
      /* shrink is only used by containers, usually is 0 */
      /* for non containers is always 1, so they always can be smaller than the natural size */
      w = iupMAX(ih->naturalwidth, w);
      h = iupMAX(ih->naturalheight, h);
    }

    /* if expand use the given size, else use the natural size */
    ih->currentwidth = (ih->expand & IUP_EXPAND_WIDTH || ih->expand & IUP_EXPAND_WFREE) ? w : ih->naturalwidth;
    ih->currentheight = (ih->expand & IUP_EXPAND_HEIGHT || ih->expand & IUP_EXPAND_HFREE) ? h : ih->naturalheight;
  }

  /* crop also the current size if some expanded */
  if (ih->expand & IUP_EXPAND_WIDTH || ih->expand & IUP_EXPAND_HEIGHT ||
      ih->expand & IUP_EXPAND_WFREE || ih->expand & IUP_EXPAND_HFREE)
    iupLayoutApplyMinMaxSize(ih, &(ih->currentwidth), &(ih->currentheight));

  if (ih->firstchild)
    iupClassObjectSetChildrenCurrentSize(ih, shrink);
}

void iupBaseSetPosition(Ihandle* ih, int x, int y)
{
  ih->x = x;
  ih->y = y;

  if (ih->firstchild)
    iupClassObjectSetChildrenPosition(ih, x, y);
}
