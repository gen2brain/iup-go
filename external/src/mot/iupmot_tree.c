#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/Container.h>
#include <Xm/IconG.h>
#include <Xm/ScrolledW.h>
#include <Xm/XmosP.h>
#include <Xm/Text.h>
#include <Xm/Transfer.h>
#include <Xm/DragDrop.h>
#include <X11/keysym.h>

#include <sys/stat.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>
#include <limits.h>
#include <time.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_childtree.h"
#include "iup_dialog.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_drvfont.h"
#include "iup_stdcontrols.h"
#include "iup_key.h"
#include "iup_image.h"
#include "iup_array.h"
#include "iup_tree.h"

#include "iupmot_drv.h"
#include "iupmot_color.h"


typedef struct _motTreeItemData
{
  Pixmap image, image_mask;
  Pixmap image_expanded, image_expanded_mask;
  unsigned char kind;
} motTreeItemData;


static void motTreeShowEditField(Ihandle* ih, Widget wItem);
static void motTreeRemoveNode(Ihandle* ih, Widget wItem, int del_data, int call_cb);


/*****************************************************************************/
/* COPYING ITEMS (Branches and its children)                                 */
/*****************************************************************************/
/* Insert the copied item in a new location. Returns the new item. */
static Widget motTreeCopyItem(Ihandle* ih, Widget wItem, Widget wParent, int pos, int is_copy)
{
  Widget wItemNew;
  XmString title;
  motTreeItemData *itemData;
  Pixel fgcolor, bgcolor;
  int num_args = 0;
  Arg args[30];
  Pixmap image = XmUNSPECIFIED_PIXMAP, mask = XmUNSPECIFIED_PIXMAP;
  unsigned char state;

  iupMOT_SETARG(args, num_args,  XmNentryParent, wParent);
  iupMOT_SETARG(args, num_args, XmNmarginHeight, ih->data->spacing);
  iupMOT_SETARG(args, num_args,  XmNmarginWidth, 0);
  iupMOT_SETARG(args, num_args, XmNnavigationType, XmTAB_GROUP);
  iupMOT_SETARG(args, num_args, XmNtraversalOn, True);
  iupMOT_SETARG(args, num_args, XmNshadowThickness, 0);

  XtVaGetValues(wItem, XmNlabelString, &title,
                          XmNuserData, &itemData,
                        XmNforeground, &fgcolor,
                      XmNoutlineState, &state,
                                       NULL);

  /* Get values to copy */
  XtVaGetValues(wItem, XmNsmallIconPixmap, &image, 
                          XmNsmallIconMask, &mask,
                                            NULL);

  if (is_copy) /* during a copy the itemdata reference is not reused */
  {
    /* create a new one */
    motTreeItemData* itemDataNew = malloc(sizeof(motTreeItemData));
    memcpy(itemDataNew, itemData, sizeof(motTreeItemData));
    itemData = itemDataNew;
  }

  iupMOT_SETARG(args, num_args,  XmNlabelString, title);
  iupMOT_SETARG(args, num_args,     XmNuserData, itemData);
  iupMOT_SETARG(args, num_args,   XmNforeground, fgcolor);
  iupMOT_SETARG(args, num_args, XmNoutlineState, state);

  iupMOT_SETARG(args, num_args, XmNviewType, XmSMALL_ICON);
  iupMOT_SETARG(args, num_args, XmNsmallIconPixmap, image);
  iupMOT_SETARG(args, num_args, XmNsmallIconMask, mask);

  iupMOT_SETARG(args, num_args,  XmNentryParent, wParent);
  iupMOT_SETARG(args, num_args, XmNpositionIndex, pos);

  XtVaGetValues(ih->handle, XmNbackground, &bgcolor, NULL);
  iupMOT_SETARG(args, num_args,   XmNbackground, bgcolor);

  /* Add the new node */
  wItemNew = XtCreateManagedWidget("icon", xmIconGadgetClass, ih->handle, args, num_args);

  ih->data->node_count++;

  XtRealizeWidget(wItemNew);

  return wItemNew;
}

static void motTreeChildRebuildCacheRec(Ihandle* ih, Widget wItem, int *id)
{
  WidgetList itemChildList = NULL;
  int i, numChild;

  /* Check whether we have child items */
  numChild = XmContainerGetItemChildren(ih->handle, wItem, &itemChildList);

  for (i = 0; i < numChild; i++)
  {
    (*id)++;
    ih->data->node_cache[*id].node_handle = itemChildList[i];

    /* go recursive to children */
    motTreeChildRebuildCacheRec(ih, itemChildList[i], id);
  }

  if (itemChildList) XtFree((char*)itemChildList);
}

static void motTreeRebuildNodeCache(Ihandle* ih, int id, Widget wItem)
{
  /* preserve cache user_data */
  ih->data->node_cache[id].node_handle = wItem;
  motTreeChildRebuildCacheRec(ih, wItem, &id);
}

static void motTreeCopyChildren(Ihandle* ih, Widget wItemSrc, Widget wItemDst, int is_copy)
{
  WidgetList wItemChildList = NULL;
  int i = 0;
  int numChild = XmContainerGetItemChildren(ih->handle, wItemSrc, &wItemChildList);
  while(i != numChild)
  {
    Widget wItemNew = motTreeCopyItem(ih, wItemChildList[i], wItemDst, i, is_copy);  /* Use the same order they were enumerated */

    /* Recursively transfer all the items */
    motTreeCopyChildren(ih, wItemChildList[i], wItemNew, is_copy);  

    /* Go to next sibling item */
    i++;
  }

  if (wItemChildList) XtFree((char*)wItemChildList);
}

/* Copies all items in a branch to a new location. Returns the new branch node. */
static Widget motTreeCopyMoveNode(Ihandle* ih, Widget wItemSrc, Widget wItemDst, int is_copy)
{
  Widget wItemNew, wParent;
  motTreeItemData *itemDataDst;
  unsigned char stateDst;
  int pos, id_new, count, id_src, id_dst;

  int old_count = ih->data->node_count;

  id_src = iupTreeFindNodeId(ih, wItemSrc);
  id_dst = iupTreeFindNodeId(ih, wItemDst);
  id_new = id_dst+1; /* contains the position for a copy operation */

  XtVaGetValues(wItemDst, XmNoutlineState, &stateDst, 
                          XmNuserData, &itemDataDst, 
                          NULL);

  if (itemDataDst->kind == ITREE_BRANCH && stateDst == XmEXPANDED)
  {
    /* copy as first child of expanded branch */
    wParent = wItemDst;
    pos = 0;
  }
  else
  {
    if (itemDataDst->kind == ITREE_BRANCH)
    {
      int child_count = iupdrvTreeTotalChildCount(ih, wItemDst);
      id_new += child_count;
    }

    /* copy as next brother of item or collapsed branch */
    XtVaGetValues(wItemDst, XmNentryParent, &wParent, NULL);
    XtVaGetValues(wItemDst, XmNpositionIndex, &pos, NULL);
    pos++;
  }

  /* move to the same place does nothing */
  if (!is_copy && id_new == id_src)
    return NULL;

  wItemNew = motTreeCopyItem(ih, wItemSrc, wParent, pos, is_copy);

  motTreeCopyChildren(ih, wItemSrc, wItemNew, is_copy);

  count = ih->data->node_count - old_count;
  iupTreeCopyMoveCache(ih, id_src, id_new, count, is_copy);

  if (!is_copy)
  {
    /* Deleting the node (and its children) from the old position */
    /* do not delete the itemdata, we reuse the references in CopyNode */
    motTreeRemoveNode(ih, wItemSrc, 0, 0);

    /* restore count, because we remove src */
    ih->data->node_count = old_count;

    /* compensate position for a move */
    if (id_new > id_src)
      id_new -= count;
  }

  motTreeRebuildNodeCache(ih, id_new, wItemNew);

  return wItemNew;
}

static void motTreeContainerDeselectAll(Ihandle *ih)
{
  XKeyEvent ev;

  memset(&ev, 0, sizeof(XKeyEvent));
  ev.type = KeyPress;
  ev.display = XtDisplay(ih->handle);
  ev.send_event = True;
  ev.root = RootWindow(iupmot_display, iupmot_screen);
  ev.time = clock()*CLOCKS_PER_SEC;
  ev.window = XtWindow(ih->handle);
  ev.state = ControlMask;
  ev.keycode = XK_backslash;
  ev.same_screen = True;

  XtCallActionProc(ih->handle, "ContainerDeselectAll", (XEvent*)&ev, 0, 0);
}

static void motTreeContainerSelectAll(Ihandle *ih)
{
  XKeyEvent ev;

  memset(&ev, 0, sizeof(XKeyEvent));
  ev.type = KeyPress;
  ev.display = XtDisplay(ih->handle);
  ev.send_event = True;
  ev.root = RootWindow(iupmot_display, iupmot_screen);
  ev.time = clock()*CLOCKS_PER_SEC;
  ev.window = XtWindow(ih->handle);
  ev.state = ControlMask;
  ev.keycode = XK_slash;
  ev.same_screen = True;

  XtCallActionProc(ih->handle, "ContainerSelectAll", (XEvent*)&ev, 0, 0);
}

static int motTreeIsNodeVisible(Widget wItem, Widget *wLastItemParent)
{
  unsigned char itemParentState;
  Widget wItemParent = NULL;
  XtVaGetValues(wItem, XmNentryParent, &wItemParent, NULL);
  if (!wItemParent || wItemParent == *wLastItemParent)
    return 1;
  while(wItemParent)
  {
    XtVaGetValues(wItemParent, XmNoutlineState, &itemParentState, NULL);
    if (itemParentState != XmEXPANDED)
      return 0;

    XtVaGetValues(wItemParent, XmNentryParent, &wItemParent, NULL);
  }

  /* save last parent */
  XtVaGetValues(wItem, XmNentryParent, &wItemParent, NULL);
  *wLastItemParent = wItemParent;
  return 1;
}

static Widget motTreeGetLastVisibleNode(Ihandle* ih)
{
  int i;
  Widget wLastItemParent = NULL;

  for (i = ih->data->node_count-1; i >= 0; i--)
  {
    if (motTreeIsNodeVisible(ih->data->node_cache[i].node_handle, &wLastItemParent))
      return ih->data->node_cache[i].node_handle;
  }

  return ih->data->node_cache[0].node_handle;  /* root is always visible */
}

static Widget motTreeGetNextVisibleNode(Ihandle* ih, Widget wItem, int count)
{
  int i, id;
  Widget wLastItemParent = NULL;

  id = iupTreeFindNodeId(ih, wItem);
  id += count;

  for (i = id; i < ih->data->node_count; i++)
  {
    if (motTreeIsNodeVisible(ih->data->node_cache[i].node_handle, &wLastItemParent))
      return ih->data->node_cache[i].node_handle;
  }

  return ih->data->node_cache[0].node_handle; /* root is always visible */
}

static Widget motTreeGetPreviousVisibleNode(Ihandle* ih, Widget wItem, int count)
{
  int i, id;
  Widget wLastItemParent = NULL;

  id = iupTreeFindNodeId(ih, wItem);
  id -= count;

  for (i = id; i >= 0; i--)
  {
    if (motTreeIsNodeVisible(ih->data->node_cache[i].node_handle, &wLastItemParent))
      return ih->data->node_cache[i].node_handle;
  }

  return motTreeGetLastVisibleNode(ih);
}

static void motTreeChildCountRec(Ihandle* ih, Widget wItem, int *count)
{
  WidgetList itemChildList = NULL;
  int i, numChild;

  /* Check whether we have child items */
  numChild = XmContainerGetItemChildren(ih->handle, wItem, &itemChildList);

  for (i = 0; i < numChild; i++)
  {
    (*count)++;

    /* go recursive to children */
    motTreeChildCountRec(ih, itemChildList[i], count);
  }

  if (itemChildList) XtFree((char*)itemChildList);
}

int iupdrvTreeTotalChildCount(Ihandle* ih, Widget wItem)
{
  int count = 0;
  motTreeChildCountRec(ih, wItem, &count);
  return count;
}

static void motTreeUpdateBgColor(Ihandle* ih, Pixel bgcolor)
{
  int i;
  for (i = 0; i < ih->data->node_count; i++)
  {
    XtVaSetValues(ih->data->node_cache[i].node_handle, XmNbackground, bgcolor, NULL);
  }
}

static void motTreeUpdateImages(Ihandle* ih, int mode)
{
  int i;
  /* called when one of the default images is changed */
  for (i = 0; i < ih->data->node_count; i++)
  {
    motTreeItemData *itemData;
    Widget wItem = ih->data->node_cache[i].node_handle;

    XtVaGetValues(wItem, XmNuserData, &itemData, NULL);

    if (itemData->kind == ITREE_BRANCH)
    {
      unsigned char itemState;
      XtVaGetValues(wItem, XmNoutlineState,  &itemState, NULL);
      
      if (itemState == XmEXPANDED)
      {
        if (mode == ITREE_UPDATEIMAGE_EXPANDED)
        {
          XtVaSetValues(wItem, XmNsmallIconPixmap, (itemData->image_expanded!=XmUNSPECIFIED_PIXMAP)? itemData->image_expanded: (Pixmap)ih->data->def_image_expanded, NULL);
          XtVaSetValues(wItem, XmNsmallIconMask, (itemData->image_expanded_mask!=XmUNSPECIFIED_PIXMAP)? itemData->image_expanded_mask: (Pixmap)ih->data->def_image_expanded_mask, NULL);
        }
      }
      else 
      {
        if (mode == ITREE_UPDATEIMAGE_COLLAPSED)
        {
          XtVaSetValues(wItem, XmNsmallIconPixmap, (itemData->image!=XmUNSPECIFIED_PIXMAP)? itemData->image: (Pixmap)ih->data->def_image_collapsed, NULL);
          XtVaSetValues(wItem, XmNsmallIconMask, (itemData->image_mask!=XmUNSPECIFIED_PIXMAP)? itemData->image_mask: (Pixmap)ih->data->def_image_collapsed_mask, NULL);
        }
      }
    }
    else 
    {
      if (mode == ITREE_UPDATEIMAGE_LEAF)
      {
        XtVaSetValues(wItem, XmNsmallIconPixmap, (itemData->image!=XmUNSPECIFIED_PIXMAP)? itemData->image: (Pixmap)ih->data->def_image_leaf, NULL);
        XtVaSetValues(wItem, XmNsmallIconMask, (itemData->image_mask!=XmUNSPECIFIED_PIXMAP)? itemData->image_mask: (Pixmap)ih->data->def_image_leaf_mask, NULL);
      }
    }
  }
}

static int motTreeIsNodeSelected(Widget wItem)
{
  unsigned char isSelected;
  XtVaGetValues(wItem, XmNvisualEmphasis, &isSelected, NULL);
  if(isSelected == XmSELECTED)
    return 1;
  else
    return 0;
}

static void motTreeSelectNode(Widget wItem, int select)
{
  if (select == -1)
    select = !motTreeIsNodeSelected(wItem);  /* toggle */

  if (select)
    XtVaSetValues(wItem, XmNvisualEmphasis, XmSELECTED, NULL);
  else
    XtVaSetValues(wItem, XmNvisualEmphasis, XmNOT_SELECTED, NULL);
}

static int motTreeSelectFunc(Ihandle* ih, Widget wItem, int id, int *select)
{
  int do_select = *select;
  if (do_select == -1)
    do_select = !motTreeIsNodeSelected(wItem); /* toggle */

  if (do_select)
    XtVaSetValues(wItem, XmNvisualEmphasis, XmSELECTED, NULL);
  else
    XtVaSetValues(wItem, XmNvisualEmphasis, XmNOT_SELECTED, NULL);

  (void)ih;
  (void)id;
  return 1;
}

static void motTreeInvertAllNodeMarking(Ihandle* ih)
{
  int select = -1;
  iupTreeForEach(ih, (iupTreeNodeFunc)motTreeSelectFunc, &select);
}

static void motTreeSelectRange(Ihandle* ih, Widget wItem1, Widget wItem2, int clear)
{
  int i;
  int id1 = iupTreeFindNodeId(ih, wItem1);
  int id2 = iupTreeFindNodeId(ih, wItem2);
  if (id1 > id2)
  {
    int tmp = id1;
    id1 = id2;
    id2 = tmp;
  }

  for (i = 0; i < ih->data->node_count; i++)
  {
    if (i < id1 || i > id2)
    {
      if (clear)
        XtVaSetValues(ih->data->node_cache[i].node_handle, XmNvisualEmphasis, XmNOT_SELECTED, NULL);
    }
    else
      XtVaSetValues(ih->data->node_cache[i].node_handle, XmNvisualEmphasis, XmSELECTED, NULL);
  }
}

void motTreeExpandCollapseAllNodes(Ihandle* ih, unsigned char itemState)
{
  int i;
  /* called when one of the default images is changed */
  for (i = 0; i < ih->data->node_count; i++)
  {
    motTreeItemData *itemData;
    Widget wItem = ih->data->node_cache[i].node_handle;

    XtVaGetValues(wItem, XmNuserData, &itemData, NULL);
  
    /* Check whether we have child items */
    if (itemData->kind == ITREE_BRANCH)
      XtVaSetValues(wItem, XmNoutlineState, itemState, NULL);
  }
}

static void motTreeDestroyItemData(Ihandle* ih, Widget wItem, int del_data, IFns cb, int id)
{
  motTreeItemData *itemData = NULL;
  XtVaGetValues(wItem, XmNuserData, &itemData, NULL);
  if (itemData)
  {
    if (cb) 
      cb(ih, (char*)ih->data->node_cache[id].userdata);

    if (del_data)
    {
      free(itemData);
      XtVaSetValues(wItem, XmNuserData, NULL, NULL);
    }
  }
}

static void motTreeRemoveNodeRec(Ihandle* ih, Widget wItem, int del_data, IFns cb, int *id)
{
  WidgetList itemChildList = NULL;
  int i, numChild;
  int node_id = *id;

  /* Check whether we have child items */
  /* remove from children first */
  numChild = XmContainerGetItemChildren(ih->handle, wItem, &itemChildList);
  for (i = 0; i < numChild; i++)
  {
    (*id)++;

    /* go recursive to children */
    motTreeRemoveNodeRec(ih, itemChildList[i], del_data, cb, id);
  }
  if (itemChildList) XtFree((char*)itemChildList);

  /* actually do it for the node */
  if (del_data || cb)
    motTreeDestroyItemData(ih, wItem, del_data, cb, node_id);
  XtDestroyWidget(wItem);  /* must manually destroy each node, this is NOT recursive */

  /* update count */
  ih->data->node_count--;
}

static void motTreeRemoveNode(Ihandle* ih, Widget wItem, int del_data, int call_cb)
{
  IFns cb = call_cb? (IFns)IupGetCallback(ih, "NODEREMOVED_CB"): NULL;
  int old_count = ih->data->node_count;
  int id = iupTreeFindNodeId(ih, wItem);
  int start_id = id;

  motTreeRemoveNodeRec(ih, wItem, del_data, cb, &id);

  if (call_cb)
    iupTreeDelFromCache(ih, start_id, old_count-ih->data->node_count);
}

static void motTreeSetFocusNode(Ihandle* ih, Widget wItem)
{
  iupAttribSet(ih, "_IUPTREE_LAST_FOCUS", (char*)wItem);
  XmProcessTraversal(wItem, XmTRAVERSE_CURRENT);
}

Widget iupdrvTreeGetFocusNode(Ihandle* ih)
{
  Widget wItem = XmGetFocusWidget(ih->handle);  /* returns the focus in the dialog */
  if (wItem && XtParent(wItem) == ih->handle) /* is a node */
    return wItem;

  return (Widget)iupAttribGet(ih, "_IUPTREE_LAST_FOCUS");
}

static void motTreeEnterLeaveWindowEvent(Widget w, Ihandle *ih, XEvent *evt, Boolean *cont)
{
  if (iupAttribGet(ih, "_IUPTREE_EDITFIELD"))
    return;

  /* usually when one Gadget is selected different than the previous one,
     leave/enter events are generated. But we could not find the exact condition, 
     so this is a workaround. Some leave events will be lost. */
  if (evt->type == EnterNotify)
  {
    if (iupAttribGet(ih, "_IUPTREE_IGNORE_ENTERLEAVE"))
    {
      iupAttribSet(ih, "_IUPTREE_IGNORE_ENTERLEAVE", NULL);
      return;
    }
  }
  else  if (evt->type == LeaveNotify)
  {
    if (iupAttribGet(ih, "_IUPTREE_IGNORE_ENTERLEAVE"))
      return;
  }

  iupmotEnterLeaveWindowEvent(w, ih, evt, cont);
}

static void motTreeFocusChangeEvent(Widget w, Ihandle *ih, XEvent *evt, Boolean *cont)
{
  unsigned char selpol;
  Widget wItem = XmGetFocusWidget(w);  /* returns the focus in the dialog */
  Widget wRoot = (Widget)iupAttribGet(ih, "_IUPTREE_ROOTITEM");

  if (XtParent(wItem) == w) /* is a node */
    iupAttribSet(ih, "_IUPTREE_LAST_FOCUS", (char*)wItem);

  if (wItem == NULL || wItem == wRoot)
  {
    iupmotFocusChangeEvent(w, ih, evt, cont);
    return;
  }

  XtVaGetValues(w, XmNselectionPolicy, &selpol, NULL);
  if (selpol != XmSINGLE_SELECT)
    return;

  if (evt->type == FocusIn && !iupStrBoolean(IupGetGlobal("CONTROLKEY")))
  {
    XtVaSetValues(w, XmNselectedObjects,  NULL, NULL);
    XtVaSetValues(w, XmNselectedObjectCount, 0, NULL);
    XtVaSetValues(wItem, XmNvisualEmphasis, XmSELECTED, NULL);
  }
}

void iupdrvTreeAddNode(Ihandle* ih, int id, int kind, const char* title, int add)
{
  Widget wItemPrev = iupTreeGetNode(ih, id);
  Widget wItemNew;
  XmString itemTitle;
  motTreeItemData *itemData;
  Pixel bgcolor, fgcolor;
  int kindPrev = 0, num_args = 0;
  Arg args[30];

  /* the previous node is not necessary only
     if adding the root in an empty tree or before the root. */
  if (!wItemPrev && id!=-1)
      return;

  if (!title)
    title = "";

  itemData = calloc(1, sizeof(motTreeItemData));
  itemData->image = XmUNSPECIFIED_PIXMAP;
  itemData->image_expanded = XmUNSPECIFIED_PIXMAP;
  itemData->image_mask = XmUNSPECIFIED_PIXMAP;
  itemData->image_expanded_mask = XmUNSPECIFIED_PIXMAP;
  itemData->kind = (unsigned char)kind;

  itemTitle = iupmotStringCreate(title);

  /* Get default colors */
  XtVaGetValues(ih->handle, XmNforeground, &fgcolor, NULL);
  XtVaGetValues(ih->handle, XmNbackground, &bgcolor, NULL);

  if (wItemPrev)
  {
    motTreeItemData *itemDataPrev;

    /* Get the kind of previous item */
    XtVaGetValues(wItemPrev, XmNuserData, &itemDataPrev, NULL);
    kindPrev = itemDataPrev->kind;

    if (kindPrev == ITREE_BRANCH && add)
    {
      /* wItemPrev is parent of the new item (firstchild of it) */
      iupMOT_SETARG(args, num_args, XmNentryParent, wItemPrev);
      iupMOT_SETARG(args, num_args, XmNpositionIndex, 0);
    }
    else
    {
      /* wItemPrev is sibling of the new item (set its parent to the new item) */
      Widget wItemParent;
      int pos;

      XtVaGetValues(wItemPrev, XmNentryParent, &wItemParent, NULL);
      XtVaGetValues(wItemPrev, XmNpositionIndex, &pos, NULL);

      iupMOT_SETARG(args, num_args, XmNentryParent, wItemParent);
      iupMOT_SETARG(args, num_args, XmNpositionIndex, pos+1);
    }
  }

  iupMOT_SETARG(args, num_args,     XmNuserData, itemData);
  iupMOT_SETARG(args, num_args,   XmNforeground, fgcolor);
  iupMOT_SETARG(args, num_args,   XmNbackground, bgcolor);
  iupMOT_SETARG(args, num_args, XmNmarginHeight, ih->data->spacing);
  iupMOT_SETARG(args, num_args,  XmNmarginWidth, 0);
  iupMOT_SETARG(args, num_args, XmNnavigationType, XmTAB_GROUP);
  iupMOT_SETARG(args, num_args, XmNtraversalOn, True);
  iupMOT_SETARG(args, num_args, XmNshadowThickness, 0);
  iupMOT_SETARG(args, num_args, XmNlabelString, itemTitle);

  iupMOT_SETARG(args, num_args, XmNviewType, XmSMALL_ICON);

  if (kind == ITREE_BRANCH)
  {
    if (ih->data->add_expanded)
    {
      iupMOT_SETARG(args, num_args, XmNsmallIconPixmap, ih->data->def_image_expanded);
      iupMOT_SETARG(args, num_args, XmNsmallIconMask, ih->data->def_image_expanded_mask);
    }
    else
    {
      iupMOT_SETARG(args, num_args, XmNsmallIconPixmap, ih->data->def_image_collapsed);
      iupMOT_SETARG(args, num_args, XmNsmallIconMask, ih->data->def_image_collapsed_mask);
    }
  }
  else
  {
    iupMOT_SETARG(args, num_args, XmNsmallIconPixmap, ih->data->def_image_leaf);
    iupMOT_SETARG(args, num_args, XmNsmallIconMask, ih->data->def_image_leaf_mask);
  }

  /* Add the new node */
  wItemNew = XtCreateManagedWidget("icon", xmIconGadgetClass, ih->handle, args, num_args);

  if (wItemPrev)
    iupTreeAddToCache(ih, add, kindPrev, wItemPrev, wItemNew);
  else
  {
    iupTreeAddToCache(ih, 0, 0, NULL, wItemNew);

    if (ih->data->node_count == 1)
    {
      /* MarkStart node */
      iupAttribSet(ih, "_IUPTREE_MARKSTART_NODE", (char*)wItemNew);

      /* Set the default VALUE (focus) */
      motTreeSetFocusNode(ih, wItemNew);

      /* when single selection when focus is set, node is also selected */
      if (ih->data->mark_mode == ITREE_MARK_SINGLE)
        motTreeSelectNode(wItemNew, 1);
    }
  }

  if (kind == ITREE_BRANCH)
  {
    iupAttribSet(ih, "_IUP_IGNORE_BRANCH_CB", "1");
    if (ih->data->add_expanded)
      XtVaSetValues(wItemNew, XmNoutlineState, XmEXPANDED, NULL);
    else
      XtVaSetValues(wItemNew, XmNoutlineState, XmCOLLAPSED, NULL);
    iupAttribSet(ih, "_IUP_IGNORE_BRANCH_CB", NULL);
  }

  XtRealizeWidget(wItemNew);
  XmStringFree(itemTitle);
}

/*****************************************************************************/

static int motTreeSetImageExpandedAttrib(Ihandle* ih, int id, const char* value)
{
  motTreeItemData *itemData;
  unsigned char itemState;
  Widget wItem = iupTreeGetNode(ih, id);
  if (!wItem)
    return 0;

  XtVaGetValues(wItem, XmNuserData, &itemData, NULL);
  XtVaGetValues(wItem, XmNoutlineState, &itemState, NULL);
  itemData->image_expanded = (Pixmap)iupImageGetImage(value, ih, 0, NULL);
  if (!itemData->image_expanded)
  {
    itemData->image_expanded = XmUNSPECIFIED_PIXMAP;
    itemData->image_expanded_mask = XmUNSPECIFIED_PIXMAP;
  }
  else
  {
    itemData->image_expanded_mask = iupmotImageGetMask(value);
    if (!itemData->image_expanded_mask) itemData->image_expanded_mask = XmUNSPECIFIED_PIXMAP;
  }

  if (itemData->kind == ITREE_BRANCH && itemState == XmEXPANDED)
  {
    if (itemData->image_expanded == XmUNSPECIFIED_PIXMAP)
    {
      XtVaSetValues(wItem, XmNsmallIconPixmap, (Pixmap)ih->data->def_image_expanded, 
                            XmNsmallIconMask, (Pixmap)ih->data->def_image_expanded_mask, 
                            NULL);        
    }
    else
    {
      XtVaSetValues(wItem, XmNsmallIconPixmap, itemData->image_expanded, 
                            XmNsmallIconMask, itemData->image_expanded_mask, 
                            NULL);
    }
  }

  return 1;
}

static int motTreeSetImageAttrib(Ihandle* ih, int id, const char* value)
{
  motTreeItemData *itemData;
  Widget wItem = iupTreeGetNode(ih, id);
  if (!wItem)  
    return 0;

  XtVaGetValues(wItem, XmNuserData, &itemData, NULL);
  itemData->image = (Pixmap)iupImageGetImage(value, ih, 0, NULL);
  if (!itemData->image) 
  {
    itemData->image = XmUNSPECIFIED_PIXMAP;
    itemData->image_mask = XmUNSPECIFIED_PIXMAP;
  }
  else
  {
    itemData->image_mask = iupmotImageGetMask(value);
    if (!itemData->image_mask) itemData->image_mask = XmUNSPECIFIED_PIXMAP;
  }

  if (itemData->kind == ITREE_BRANCH)
  {
    unsigned char itemState;
    XtVaGetValues(wItem, XmNoutlineState, &itemState, NULL);
    if (itemState == XmCOLLAPSED)
    {
      if (itemData->image == XmUNSPECIFIED_PIXMAP)
      {
        XtVaSetValues(wItem, XmNsmallIconPixmap, (Pixmap)ih->data->def_image_collapsed, 
                              XmNsmallIconMask, (Pixmap)ih->data->def_image_collapsed_mask, 
                              NULL);
      }
      else
      {
        XtVaSetValues(wItem, XmNsmallIconPixmap, itemData->image, 
                              XmNsmallIconMask, itemData->image_mask, 
                              NULL);
      }
    }
  }
  else
  {
    if (itemData->image == XmUNSPECIFIED_PIXMAP)
    {
      XtVaSetValues(wItem, XmNsmallIconPixmap, (Pixmap)ih->data->def_image_leaf, 
                            XmNsmallIconMask, (Pixmap)ih->data->def_image_leaf_mask, 
                            NULL);
    }
    else
    {
      XtVaSetValues(wItem, XmNsmallIconPixmap, itemData->image, 
                            XmNsmallIconMask, itemData->image_mask, 
                            NULL);
    }
  }

  return 0;
}

static int motTreeSetImageBranchExpandedAttrib(Ihandle* ih, const char* value)
{
  ih->data->def_image_expanded = iupImageGetImage(value, ih, 0, NULL);
  if (!ih->data->def_image_expanded) 
  {
    ih->data->def_image_expanded = (void*)XmUNSPECIFIED_PIXMAP;
    ih->data->def_image_expanded_mask = (void*)XmUNSPECIFIED_PIXMAP;
  }
  else
  {
    ih->data->def_image_expanded_mask = (void*)iupmotImageGetMask(value);
    if (!ih->data->def_image_expanded_mask) ih->data->def_image_expanded_mask = (void*)XmUNSPECIFIED_PIXMAP;
  }
    
  /* Update all images, starting at root node */
  motTreeUpdateImages(ih, ITREE_UPDATEIMAGE_EXPANDED);

  return 1;
}

static int motTreeSetImageBranchCollapsedAttrib(Ihandle* ih, const char* value)
{
  ih->data->def_image_collapsed = iupImageGetImage(value, ih, 0, NULL);
  if (!ih->data->def_image_collapsed) 
  {
    ih->data->def_image_collapsed = (void*)XmUNSPECIFIED_PIXMAP;
    ih->data->def_image_collapsed_mask = (void*)XmUNSPECIFIED_PIXMAP;
  }
  else
  {
    ih->data->def_image_collapsed_mask = (void*)iupmotImageGetMask(value);
    if (!ih->data->def_image_collapsed_mask) ih->data->def_image_collapsed_mask = (void*)XmUNSPECIFIED_PIXMAP;
  }
   
  /* Update all images, starting at root node */
  motTreeUpdateImages(ih, ITREE_UPDATEIMAGE_COLLAPSED);

  return 1;
}

static int motTreeSetImageLeafAttrib(Ihandle* ih, const char* value)
{
  ih->data->def_image_leaf = iupImageGetImage(value, ih, 0, NULL);
  if (!ih->data->def_image_leaf) 
  {
    ih->data->def_image_leaf = (void*)XmUNSPECIFIED_PIXMAP;
    ih->data->def_image_leaf_mask = (void*)XmUNSPECIFIED_PIXMAP;
  }
  else
  {
    ih->data->def_image_leaf_mask = (void*)iupmotImageGetMask(value);
    if (!ih->data->def_image_leaf_mask) ih->data->def_image_leaf_mask = (void*)XmUNSPECIFIED_PIXMAP;
  }
    
  /* Update all images, starting at root node */
  motTreeUpdateImages(ih, ITREE_UPDATEIMAGE_LEAF);

  return 1;
}

static char* motTreeGetStateAttrib(Ihandle* ih, int id)
{
  int hasChildren;
  unsigned char itemState;
  Widget wItem = iupTreeGetNode(ih, id);
  if (!wItem)  
    return 0;

  XtVaGetValues(wItem, XmNnumChildren, &hasChildren, NULL);
  XtVaGetValues(wItem, XmNoutlineState, &itemState, NULL);
  
  if (hasChildren)
  {
    if(itemState == XmEXPANDED)
      return "EXPANDED";
    else
      return "COLLAPSED";
  }

  return NULL;
}

static int motTreeSetStateAttrib(Ihandle* ih, int id, const char* value)
{
  motTreeItemData *itemData;
  Widget wItem = iupTreeGetNode(ih, id);
  if (!wItem)
    return 0;

  XtVaGetValues(wItem, XmNuserData, &itemData, NULL);
  if (itemData->kind == ITREE_BRANCH)
  {
    iupAttribSet(ih, "_IUP_IGNORE_BRANCH_CB", "1");
    if (iupStrEqualNoCase(value, "EXPANDED"))
      XtVaSetValues(wItem, XmNoutlineState, XmEXPANDED, NULL);
    else 
      XtVaSetValues(wItem, XmNoutlineState, XmCOLLAPSED, NULL);
    iupAttribSet(ih, "_IUP_IGNORE_BRANCH_CB", NULL);
  }

  return 0;
}

static char* motTreeGetColorAttrib(Ihandle* ih, int id)
{
  unsigned char r, g, b;
  Pixel color;
  Widget wItem = iupTreeGetNode(ih, id);  
  if (!wItem)  
    return NULL;

  XtVaGetValues(wItem, XmNforeground, &color, NULL); 
  iupmotColorGetRGB(color, &r, &g, &b);

  return iupStrReturnStrf("%d %d %d", (int)r, (int)g, (int)b);
}

static int motTreeSetColorAttrib(Ihandle* ih, int id, const char* value)
{
  Pixel color;
  Widget wItem = iupTreeGetNode(ih, id);  
  if (!wItem)  
    return 0;

  color = iupmotColorGetPixelStr(value);
  if (color != (Pixel)-1)
    XtVaSetValues(wItem, XmNforeground, color, NULL);
  return 0; 
}

static char* motTreeGetDepthAttrib(Ihandle* ih, int id)
{
  int depth = -1;
  Widget wItem = iupTreeGetNode(ih, id);  
  if (!wItem)  
    return NULL;

  while(wItem != NULL)
  {
    XtVaGetValues(wItem, XmNentryParent, &wItem, NULL);
    depth++;
  }

  return iupStrReturnInt(depth);
}

static int motTreeSetMoveNodeAttrib(Ihandle* ih, int id, const char* value)
{
  Widget wItemDst, wParent, wItemSrc;

  if (!ih->handle)  /* do not do the action before map */
    return 0;
  wItemSrc = iupTreeGetNode(ih, id);
  if (!wItemSrc)
    return 0;
  wItemDst = iupTreeGetNodeFromString(ih, value);
  if (!wItemDst)
    return 0;

  /* If Drag item is an ancestor of Drop item then return */
  wParent = wItemDst;
  while(wParent)
  {
    XtVaGetValues(wParent, XmNentryParent, &wParent, NULL);
    if (wParent == wItemSrc)
      return 0;
  }

  /* Move the node and its children to the new position */
  motTreeCopyMoveNode(ih, wItemSrc, wItemDst, 0);

  return 0;
}

static int motTreeSetCopyNodeAttrib(Ihandle* ih, int id, const char* value)
{
  Widget wItemDst, wParent, wItemSrc;

  if (!ih->handle)  /* do not do the action before map */
    return 0;
  wItemSrc = iupTreeGetNode(ih, id);
  if (!wItemSrc)
    return 0;
  wItemDst = iupTreeGetNodeFromString(ih, value);
  if (!wItemDst)
    return 0;

  /* If Drag item is an ancestor of Drop item then return */
  wParent = wItemDst;
  while(wParent)
  {
    XtVaGetValues(wParent, XmNentryParent, &wParent, NULL);
    if (wParent == wItemSrc)
      return 0;
  }

  /* Copy the node and its children to the new position */
  motTreeCopyMoveNode(ih, wItemSrc, wItemDst, 1);

  return 0;
}

static char* motTreeGetParentAttrib(Ihandle* ih, int id)
{
  Widget wItemParent;
  Widget wItem = iupTreeGetNode(ih, id);
  if (!wItem)  
    return NULL;

  /* get the parent item */
  XtVaGetValues(wItem, XmNentryParent, &wItemParent, NULL);
  if (!wItemParent)
    return NULL;

  return iupStrReturnInt(iupTreeFindNodeId(ih, wItemParent));
}

static char* motTreeGetNextAttrib(Ihandle* ih, int id)
{
  Widget wItemParent;
  Widget wItem = iupTreeGetNode(ih, id);
  if (!wItem)  
    return NULL;

  /* get the parent item, it will not work for root nodes */
  XtVaGetValues(wItem, XmNentryParent, &wItemParent, NULL);
  if (wItemParent)
  {
    WidgetList wItemChildList = NULL;
    int i = 0;
    int numChild = XmContainerGetItemChildren(ih->handle, wItemParent, &wItemChildList);
    while (i != numChild)
    {
      if ((wItemChildList[i] == wItem) && (i + 1 < numChild))
      {
        Widget wItemNext = wItemChildList[i+1];
        XtFree((char*)wItemChildList);
        return iupStrReturnInt(iupTreeFindNodeId(ih, wItemNext));
      }

      /* Go to next sibling item */
      i++;
    }
    if (wItemChildList) XtFree((char*)wItemChildList);
  }

  return NULL;
}

static char* motTreeGetPreviousAttrib(Ihandle* ih, int id)
{
  Widget wItemParent;
  Widget wItem = iupTreeGetNode(ih, id);
  if (!wItem)
    return NULL;

  /* get the parent item, it will not work for root nodes */
  XtVaGetValues(wItem, XmNentryParent, &wItemParent, NULL);
  if (wItemParent)
  {
    WidgetList wItemChildList = NULL;
    int i = 0;
    int numChild = XmContainerGetItemChildren(ih->handle, wItemParent, &wItemChildList);
    while (i != numChild)
    {
      if ((wItemChildList[i] == wItem) && (i > 0))
      {
        Widget wItemPrevious = wItemChildList[i - 1];
        XtFree((char*)wItemChildList);
        return iupStrReturnInt(iupTreeFindNodeId(ih, wItemPrevious));
      }

      /* Go to next sibling item */
      i++;
    }
    if (wItemChildList) XtFree((char*)wItemChildList);
  }

  return NULL;
}

static char* motTreeGetLastAttrib(Ihandle* ih, int id)
{
  Widget wItemParent;
  Widget wItem = iupTreeGetNode(ih, id);
  if (!wItem)
    return NULL;

  /* get the parent item, it will not work for root nodes */
  XtVaGetValues(wItem, XmNentryParent, &wItemParent, NULL);
  if (wItemParent)
  {
    WidgetList wItemChildList = NULL;
    int numChild = XmContainerGetItemChildren(ih->handle, wItemParent, &wItemChildList);
    if (numChild)
    {
      Widget wItemLast = wItemChildList[numChild - 1];
      XtFree((char*)wItemChildList);
      return iupStrReturnInt(iupTreeFindNodeId(ih, wItemLast));
    }
    if (wItemChildList) XtFree((char*)wItemChildList);
  }

  return NULL;
}

static char* motTreeGetFirstAttrib(Ihandle* ih, int id)
{
  Widget wItemParent;
  Widget wItem = iupTreeGetNode(ih, id);
  if (!wItem)
    return NULL;

  /* get the parent item, it will not work for root nodes */
  XtVaGetValues(wItem, XmNentryParent, &wItemParent, NULL);
  if (wItemParent)
  {
    WidgetList wItemChildList = NULL;
    int numChild = XmContainerGetItemChildren(ih->handle, wItemParent, &wItemChildList);
    if (numChild)
    {
      Widget wItemFirst = wItemChildList[0];
      XtFree((char*)wItemChildList);
      return iupStrReturnInt(iupTreeFindNodeId(ih, wItemFirst));
    }
    if (wItemChildList) XtFree((char*)wItemChildList);
  }

  return NULL;
}

static char* motTreeGetChildCountAttrib(Ihandle* ih, int id)
{
  int ret;
  WidgetList wList = NULL;
  Widget wItem = iupTreeGetNode(ih, id);
  if (!wItem)  
    return NULL;

  ret = XmContainerGetItemChildren(ih->handle, wItem, &wList);
  if (wList) XtFree((char*)wList);
  return iupStrReturnInt(ret);
}

static char* motTreeGetRootCountAttrib(Ihandle* ih)
{
  Widget wItemParent;
  int i, count = 0;
  for (i = 0; i < ih->data->node_count; i++)
  {
    XtVaGetValues(ih->data->node_cache[i].node_handle, XmNentryParent, &wItemParent, NULL);
    if (!wItemParent)
      count++;
  }
  return iupStrReturnInt(count);
}

static char* motTreeGetKindAttrib(Ihandle* ih, int id)
{
  motTreeItemData *itemData;
  Widget wItem = iupTreeGetNode(ih, id);
  if (!wItem)  
    return NULL;

  XtVaGetValues(wItem, XmNuserData, &itemData, NULL);

  if(itemData->kind == ITREE_BRANCH)
    return "BRANCH";
  else
    return "LEAF";
}

static char* motTreeGetValueAttrib(Ihandle* ih)
{
  Widget wItem = iupdrvTreeGetFocusNode(ih);
  if (!wItem)
  {
    if (ih->data->node_count)
      return "0"; /* default VALUE is root */
    else
      return "-1";
  }

  return iupStrReturnInt(iupTreeFindNodeId(ih, wItem));
}

static char* motTreeGetMarkedNodesAttrib(Ihandle* ih)
{
  char* str;
  int i;

  if (ih->data->mark_mode==ITREE_MARK_SINGLE)
    return NULL;

  str = iupStrGetMemory(ih->data->node_count+1);

  for (i=0; i<ih->data->node_count; i++)
  {
    if (motTreeIsNodeSelected(ih->data->node_cache[i].node_handle))
      str[i] = '+';
    else
      str[i] = '-';
  }

  str[ih->data->node_count] = 0;
  return str;
}

static int motTreeSetMarkedNodesAttrib(Ihandle* ih, const char* value)
{
  int count, i;

  if (ih->data->mark_mode==ITREE_MARK_SINGLE || !value)
    return 0;

  count = (int)strlen(value);
  if (count > ih->data->node_count)
    count = ih->data->node_count;

  for (i=0; i<count; i++)
  {
    if (value[i] == '+')
      XtVaSetValues(ih->data->node_cache[i].node_handle, XmNvisualEmphasis, XmSELECTED, NULL);
    else
      XtVaSetValues(ih->data->node_cache[i].node_handle, XmNvisualEmphasis, XmNOT_SELECTED, NULL);
  }

  return 0;
}

static int motTreeSetMarkAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->mark_mode==ITREE_MARK_SINGLE)
    return 0;

  if(iupStrEqualNoCase(value, "CLEARALL"))
    motTreeContainerDeselectAll(ih);
  else if(iupStrEqualNoCase(value, "MARKALL"))
    motTreeContainerSelectAll(ih);
  else if(iupStrEqualNoCase(value, "INVERTALL")) /* INVERTALL *MUST* appear before INVERT, or else INVERTALL will never be called. */
    motTreeInvertAllNodeMarking(ih);
  else if(iupStrEqualPartial(value, "INVERT"))
  {
    Widget wItem = iupTreeGetNodeFromString(ih, &value[strlen("INVERT")]);
    if (!wItem)  
      return 0;

    motTreeSelectNode(wItem, -1);
  }
  else if(iupStrEqualNoCase(value, "BLOCK"))
  {
    Widget wItem = (Widget)iupAttribGet(ih, "_IUPTREE_MARKSTART_NODE");
    Widget wFocusItem = iupdrvTreeGetFocusNode(ih);
    if(!wFocusItem || !wItem)
      return 0;
    motTreeSelectRange(ih, wFocusItem, wItem, 0);
  }
  else
  {
    Widget wItem1, wItem2;
    char str1[50], str2[50];
    if (iupStrToStrStr(value, str1, str2, '-')!=2)
      return 0;

    wItem1 = iupTreeGetNodeFromString(ih, str1);
    if (!wItem1)  
      return 0;
    wItem2 = iupTreeGetNodeFromString(ih, str2);
    if (!wItem2)  
      return 0;

    motTreeSelectRange(ih, wItem1, wItem2, 0);
  }

  return 1;
} 

static int motTreeSetValueAttrib(Ihandle* ih, const char* value)
{
  Widget wItem, wItemParent;

  if (motTreeSetMarkAttrib(ih, value))
    return 0;

  if (iupStrEqualNoCase(value, "ROOT") || iupStrEqualNoCase(value, "FIRST"))
    wItem = (Widget)iupAttribGet(ih, "_IUPTREE_ROOTITEM");
  else if(iupStrEqualNoCase(value, "LAST"))
    wItem = motTreeGetLastVisibleNode(ih);
  else if(iupStrEqualNoCase(value, "PGUP"))
  {
    Widget wItemFocus = iupdrvTreeGetFocusNode(ih);
    if(!wItemFocus)
      return 0;

    wItem = motTreeGetPreviousVisibleNode(ih, wItemFocus, 10);
  }
  else if(iupStrEqualNoCase(value, "PGDN"))
  {
    Widget wItemFocus = iupdrvTreeGetFocusNode(ih);
    if(!wItemFocus)
      return 0;

    wItem = motTreeGetNextVisibleNode(ih, wItemFocus, 10);
  }
  else if(iupStrEqualNoCase(value, "NEXT"))
  {
    Widget wItemFocus = iupdrvTreeGetFocusNode(ih);
    if (!wItemFocus)
      return 0;

    wItem = motTreeGetNextVisibleNode(ih, wItemFocus, 1);
  }
  else if(iupStrEqualNoCase(value, "PREVIOUS"))
  {
    Widget wItemFocus = iupdrvTreeGetFocusNode(ih);
    if(!wItemFocus)
      return 0;

    wItem = motTreeGetPreviousVisibleNode(ih, wItemFocus, 1);
  }
  else if (iupStrEqualNoCase(value, "CLEAR"))
  {
    /* clear previous selection */
    XtVaSetValues(ih->handle, XmNselectedObjects, NULL, NULL);
    XtVaSetValues(ih->handle, XmNselectedObjectCount, 0, NULL);
    return 0;
  }
  else
    wItem = iupTreeGetNodeFromString(ih, value);

  if (!wItem)  
    return 0;

  /* select */
  if (ih->data->mark_mode==ITREE_MARK_SINGLE)
  {
    /* clear previous selection */
    XtVaSetValues(ih->handle, XmNselectedObjects,  NULL, NULL);
    XtVaSetValues(ih->handle, XmNselectedObjectCount, 0, NULL);

    XtVaSetValues(wItem, XmNvisualEmphasis, XmSELECTED, NULL);
  }


  /* expand all parents */
  XtVaGetValues(wItem, XmNentryParent, &wItemParent, NULL);
  while(wItemParent)
  {
    XtVaSetValues(wItemParent, XmNoutlineState, XmEXPANDED, NULL);
    XtVaGetValues(wItemParent, XmNentryParent, &wItemParent, NULL);
  }

  /* set focus (will scroll to visible) */
  motTreeSetFocusNode(ih, wItem);

  iupAttribSetInt(ih, "_IUPTREE_OLDVALUE", iupTreeFindNodeId(ih, wItem));

  return 0;
} 

static int motTreeSetMarkStartAttrib(Ihandle* ih, const char* value)
{
  Widget wItem = iupTreeGetNodeFromString(ih, value);
  if (!wItem)  
    return 0;

  iupAttribSet(ih, "_IUPTREE_MARKSTART_NODE", (char*)wItem);

  return 1;
}

static char* motTreeGetMarkedAttrib(Ihandle* ih, int id)
{
  Widget wItem = iupTreeGetNode(ih, id);
  if (!wItem)  
    return NULL;

  return iupStrReturnBoolean (motTreeIsNodeSelected(wItem)); 
}

static int motTreeSetMarkedAttrib(Ihandle* ih, int id, const char* value)
{
  Widget wItem = iupTreeGetNode(ih, id);
  if (!wItem)  
    return 0;

  if (iupStrBoolean(value))
    XtVaSetValues(wItem, XmNvisualEmphasis, XmSELECTED, NULL);
  else
    XtVaSetValues(wItem, XmNvisualEmphasis, XmNOT_SELECTED, NULL);

  return 0;
}

static char* motTreeGetTitleAttrib(Ihandle* ih, int id)
{
  char *title;
  XmString itemTitle;
  Widget wItem = iupTreeGetNode(ih, id);
  if (!wItem)  
    return NULL;
  XtVaGetValues(wItem, XmNlabelString, &itemTitle, NULL);
  title = iupmotReturnXmString(itemTitle);
  XmStringFree(itemTitle);
  return title;
}

static int motTreeSetTitleAttrib(Ihandle* ih, int id, const char* value)
{
  Widget wItem = iupTreeGetNode(ih, id);
  if (!wItem)  
    return 0;

  if (!value)
    value = "";

  iupmotSetXmString(wItem, XmNlabelString, value);

  return 0;
}

static int motTreeSetTitleFontAttrib(Ihandle* ih, int id, const char* value)
{
  XmFontList fontlist = NULL;
  Widget wItem = iupTreeGetNode(ih, id);
  if (!wItem)  
    return 0;

  if (value)
    fontlist = iupmotGetFontList(iupAttribGetId(ih, "TITLEFOUNDRY", id), value);

  XtVaSetValues(wItem, XmNrenderTable, fontlist, NULL);

  return 0;
}

static char* motTreeGetTitleFontAttrib(Ihandle* ih, int id)
{
  XmFontList fontlist;
  Widget wItem = iupTreeGetNode(ih, id);
  if (!wItem)  
    return NULL;

  XtVaGetValues(wItem, XmNrenderTable, &fontlist, NULL);
  return iupmotFindFontList(fontlist);
}

static int motTreeSetRenameAttrib(Ihandle* ih, const char* value)
{  
  if (ih->data->show_rename)
  {
    Widget wItemFocus = iupdrvTreeGetFocusNode(ih);
    motTreeShowEditField(ih, wItemFocus);
  }

  (void)value;
  return 0;
}

static void motTreeRemoveAllNodes(Ihandle* ih, int call_cb)
{
  IFns cb = call_cb? (IFns)IupGetCallback(ih, "NODEREMOVED_CB"): NULL;
  int i, old_count = ih->data->node_count;
  Widget wItem;

  for (i = 0; i < ih->data->node_count; i++)
  {
    wItem = ih->data->node_cache[i].node_handle;

    motTreeDestroyItemData(ih, wItem, 1, cb, i);

    XtDestroyWidget(wItem);  /* must manually destroy each node, this is NOT recursive */
  }

  ih->data->node_count = 0;

  if (call_cb)
    iupTreeDelFromCache(ih, 0, old_count);
}

static int motTreeSetDelNodeAttrib(Ihandle* ih, int id, const char* value)
{
  if (!ih->handle)  /* do not do the action before map */
    return 0;
  if (iupStrEqualNoCase(value, "ALL"))
  {
    motTreeRemoveAllNodes(ih, 1);
    return 0;
  }
  if(iupStrEqualNoCase(value, "SELECTED"))  /* selected here means the reference node */
  {
    Widget wItem = iupTreeGetNode(ih, id);
    if(!wItem)
      return 0;

    /* deleting the reference node (and it's children) */
    motTreeRemoveNode(ih, wItem, 1, 1);
  }
  else if(iupStrEqualNoCase(value, "CHILDREN"))  /* children of the reference node */
  {
    int numChild, i;
    WidgetList wItemList = NULL;
    Widget wItem = iupTreeGetNode(ih, id);

    if(!wItem)
      return 0;

    /* deleting the reference node children only */
    numChild = XmContainerGetItemChildren(ih->handle, wItem, &wItemList);
    for(i = 0; i < numChild; i++)
      motTreeRemoveNode(ih, wItemList[i], 1, 1);
    if (wItemList) XtFree((char*)wItemList);
  }
  else if(iupStrEqualNoCase(value, "MARKED"))  /* Delete the array of marked nodes */
  {
    int i;
    for(i = 0; i < ih->data->node_count; /* increment only if not removed */)
    {
      if (motTreeIsNodeSelected(ih->data->node_cache[i].node_handle))
        motTreeRemoveNode(ih, ih->data->node_cache[i].node_handle, 1, 1);
      else
        i++;
    }
  }

  return 0;
}

static char* motTreeGetIndentationAttrib(Ihandle* ih)
{
  Dimension indent;
  XtVaGetValues(ih->handle, XmNoutlineIndentation, &indent, NULL);
  return iupStrReturnInt((int)indent);
}

static int motTreeSetIndentationAttrib(Ihandle* ih, const char* value)
{
  int indent;
  if (iupStrToInt(value, &indent))
    XtVaSetValues(ih->handle, XmNoutlineIndentation, (Dimension)indent, NULL);
  return 0;
}

static int motTreeSetTopItemAttrib(Ihandle* ih, const char* value)
{
  Widget wItem = iupTreeGetNodeFromString(ih, value);
  Widget sb_win;
  Widget wItemParent;

  if (!wItem)
    return 0;

  /* expand all parents */
  XtVaGetValues(wItem, XmNentryParent, &wItemParent, NULL);
  while(wItemParent)
  {
    XtVaSetValues(wItemParent, XmNoutlineState, XmEXPANDED, NULL);
    XtVaGetValues(wItemParent, XmNentryParent, &wItemParent, NULL);
  }

  sb_win = (Widget)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  XmScrollVisible(sb_win, wItem, 0, 0);

  return 0;
}

static int motTreeSpacingFunc(Ihandle* ih, Widget wItem, int id, void *data)
{
  XtVaSetValues(wItem, XmNmarginHeight, ih->data->spacing, NULL);
  (void)data;
  (void)id;
  return 1;
}

static int motTreeSetSpacingAttrib(Ihandle* ih, const char* value)
{
  iupStrToInt(value, &ih->data->spacing);

  if (ih->handle)
  {
    iupTreeForEach(ih, (iupTreeNodeFunc)motTreeSpacingFunc, 0);
    return 0;
  }
  else
    return 1; /* store until not mapped, when mapped will be set again */
}

static int motTreeSetExpandAllAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    motTreeExpandCollapseAllNodes(ih, XmEXPANDED);
  else
    motTreeExpandCollapseAllNodes(ih, XmCOLLAPSED);

  return 0;
}

static int motTreeSetBgColorAttrib(Ihandle* ih, const char* value)
{
  Widget sb_win = (Widget)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  Pixel color;

  /* ignore given value, must use only from parent for the scrollbars */
  char* parent_value = iupBaseNativeParentGetBgColor(ih);

  color = iupmotColorGetPixelStr(parent_value);
  if (color != (Pixel)-1 && iupStrBoolean(IupGetGlobal("SB_BGCOLOR")))
  {
    Widget sb = NULL;

    iupmotSetBgColor(sb_win, color);

    XtVaGetValues(sb_win, XmNverticalScrollBar, &sb, NULL);
    if (sb) iupmotSetBgColor(sb, color);

    XtVaGetValues(sb_win, XmNhorizontalScrollBar, &sb, NULL);
    if (sb) iupmotSetBgColor(sb, color);
  }

  color = iupmotColorGetPixelStr(value);
  if (color != (Pixel)-1)
  {
    Widget clipwin = NULL;

    XtVaGetValues(sb_win, XmNclipWindow, &clipwin, NULL);
    if (clipwin) iupmotSetBgColor(clipwin, color);

    /* Update all children, starting at root node */
    motTreeUpdateBgColor(ih, color);
  }

  iupdrvBaseSetBgColorAttrib(ih, value);   /* use given value for contents */

  /* update internal image cache */
  iupTreeUpdateImages(ih);

  return 1;
}

static int motTreeSetFgColorAttrib(Ihandle* ih, const char* value)
{
  Pixel color = iupmotColorGetPixelStr(value);
  if (color != (Pixel)-1)
    XtVaSetValues(ih->handle, XmNforeground, color, NULL);

  return 1;
}

static int motTreeSetHlColorAttrib(Ihandle* ih, const char* value)
{
  Pixel color = iupmotColorGetPixelStr(value);
  if (color != (Pixel)-1)
    XtVaSetValues(ih->handle, XmNselectColor, color, NULL);

  return 1;
}

void iupdrvTreeUpdateMarkMode(Ihandle *ih)
{
  XtVaSetValues(ih->handle, XmNselectionPolicy, (ih->data->mark_mode==ITREE_MARK_SINGLE)? XmSINGLE_SELECT: XmEXTENDED_SELECT, NULL);
}

/************************************************************************************************/


static void motTreeSetRenameCaretPos(Widget cbEdit, const char* value)
{
  int pos = 1;

  if (iupStrToInt(value, &pos))
  {
    if (pos < 1) pos = 1;
    pos--; /* IUP starts at 1 */

    XtVaSetValues(cbEdit, XmNcursorPosition, pos, NULL);
  }
}

static void motTreeSetRenameSelectionPos(Widget cbEdit, const char* value)
{
  int start = 1, end = 1;

  if (iupStrToIntInt(value, &start, &end, ':') != 2) 
    return;

  if(start < 1 || end < 1) 
    return;

  start--; /* IUP starts at 1 */
  end--;

  XmTextSetSelection(cbEdit, start, end, CurrentTime);
}

/*****************************************************************************/

static int motTreeCallBranchCloseCb(Ihandle* ih, Widget wItem)
{
  IFni cbBranchClose;

  if (iupAttribGet(ih, "_IUP_IGNORE_BRANCH_CB"))
    return IUP_DEFAULT;

  cbBranchClose = (IFni)IupGetCallback(ih, "BRANCHCLOSE_CB");
  if (cbBranchClose)
    return cbBranchClose(ih, iupTreeFindNodeId(ih, wItem));

  return IUP_DEFAULT;
}

static int motTreeCallBranchOpenCb(Ihandle* ih, Widget wItem)
{
  IFni cbBranchOpen;
  
  if (iupAttribGet(ih, "_IUP_IGNORE_BRANCH_CB"))
    return IUP_DEFAULT;

  cbBranchOpen = (IFni)IupGetCallback(ih, "BRANCHOPEN_CB");
  if (cbBranchOpen)
    return cbBranchOpen(ih, iupTreeFindNodeId(ih, wItem));

  return IUP_DEFAULT;
}

static void motTreeFindRange(Ihandle* ih, WidgetList wSelectedItemList, int countItems, int *id1, int *id2)
{
  int i = 0, id;

  *id1 = ih->data->node_count;
  *id2 = -1;

  for(i = 0; i < countItems; i++)
  {
    int is_icon = XmIsIconGadget(wSelectedItemList[i]); /* this line generates a warning in some compilers */
    if (is_icon)
    {
      id = iupTreeFindNodeId(ih, wSelectedItemList[i]);
      if (id < *id1)
        *id1 = id;
      if (id > *id2)
        *id2 = id;
    }
  }

  /* interactive selection of several nodes will NOT select hidden nodes,
     so make sure that they are selected. */
  for(i = *id1; i <= *id2; i++)
  {
    if (!motTreeIsNodeSelected(ih->data->node_cache[i].node_handle))
      XtVaSetValues(ih->data->node_cache[i].node_handle, XmNvisualEmphasis, XmSELECTED, NULL);
  }

  /* if last selected item is a branch, then select its children */
  iupTreeSelectLastCollapsedBranch(ih, id2);
}

static Iarray* motTreeGetSelectedArrayId(Ihandle* ih, WidgetList wSelectedItemList, int countItems)
{
  Iarray* selarray = iupArrayCreate(1, sizeof(int));
  int i;

  for(i = 0; i < countItems; i++)
  {
    int is_icon = XmIsIconGadget(wSelectedItemList[i]); /* this line generates a warning in some compilers */
    if (is_icon)
    {
      int* id_hitem = (int*)iupArrayInc(selarray);
      int j = iupArrayCount(selarray);
      id_hitem[j-1] = iupTreeFindNodeId(ih, wSelectedItemList[i]);
    }
  }

  return selarray;
}

static void motTreeCallMultiUnSelectionCb(Ihandle* ih, int new_select_id)
{
  /* called when several items are unselected at once */
  IFnIi cbMulti = (IFnIi)IupGetCallback(ih, "MULTIUNSELECTION_CB");
  IFnii cbSelec = (IFnii)IupGetCallback(ih, "SELECTION_CB");
  if (cbSelec || cbMulti)
  {
    WidgetList wSelectedItemList = NULL;
    int countItems = 0;

    XtVaGetValues(ih->handle, XmNselectedObjects, &wSelectedItemList,
                          XmNselectedObjectCount, &countItems, NULL);
    if (countItems > 0)
    {
      Iarray* markedArray = motTreeGetSelectedArrayId(ih, wSelectedItemList, countItems);
      int* id_hitem = (int*)iupArrayGetData(markedArray);
      int i, count = iupArrayCount(markedArray);

      if (cbMulti)
      {
        for (i=0; i<count; i++)
        {
          if (id_hitem[i] == new_select_id)
          {
            memcpy(id_hitem + i, id_hitem + i+1, (count-i-1)*sizeof(int));
            count--;
            break;
          }
        }

        cbMulti(ih, id_hitem, count);
      }
      else
      {
        for (i=0; i<count; i++)
        {
          if (id_hitem[i] != new_select_id)
            cbSelec(ih, id_hitem[i], 0);
        }
      }

      iupArrayDestroy(markedArray);
    }
  }
}

static void motTreeCallMultiSelectionCb(Ihandle* ih)
{
  /* called when several items are selected at once
     using the Shift key pressed, or dragging the mouse. */
  IFnIi cbMulti = (IFnIi)IupGetCallback(ih, "MULTISELECTION_CB");
  IFnii cbSelec = (IFnii)IupGetCallback(ih, "SELECTION_CB");
  WidgetList wSelectedItemList = NULL;
  int countItems, id1, id2, i;

  XtVaGetValues(ih->handle, XmNselectedObjects, &wSelectedItemList,
                        XmNselectedObjectCount, &countItems, NULL);
  if (countItems == 0)
    return;

  /* Must be a continuous range of selection ids */
  motTreeFindRange(ih, wSelectedItemList, countItems, &id1, &id2);
  countItems = id2-id1+1;

  if (cbMulti)
  {
    int* id_rowItem = malloc(sizeof(int) * countItems);

    for(i = 0; i < countItems; i++)
      id_rowItem[i] = id1+i;

    cbMulti(ih, id_rowItem, countItems);

    free(id_rowItem);
  }
  else if (cbSelec)
  {
    for (i=0; i<countItems; i++)
      cbSelec(ih, id1+i, 1);
  }
}

static int motTreeConvertXYToPos(Ihandle* ih, int x, int y)
{
  Widget wItem = XmObjectAtPoint(ih->handle, (Position)x, (Position)y);
  if (wItem)
    return iupTreeFindNodeId(ih, wItem);
  return -1;
}

static void motTreeCallRightClickCb(Ihandle* ih, int x, int y)
{
  Widget wItem = XmObjectAtPoint(ih->handle, (Position)x, (Position)y);
  if (wItem)
  {
    IFni cbRightClick = (IFni)IupGetCallback(ih, "RIGHTCLICK_CB");
    if (cbRightClick)
        cbRightClick(ih, iupTreeFindNodeId(ih, wItem));
  }
}

static void motTreeCallRenameCb(Ihandle* ih)
{
  IFnis cbRename;
  Widget wItem, wEdit;
  int ignore = 0;
  String title = NULL;

  wItem = (Widget)iupAttribGet(ih, "_IUPTREE_SELECTED");
  wEdit = (Widget)iupAttribGet(ih, "_IUPTREE_EDITFIELD");

  XtVaGetValues((Widget)iupAttribGet(ih, "_IUPTREE_EDITFIELD"), XmNvalue, &title, NULL);

  cbRename = (IFnis)IupGetCallback(ih, "RENAME_CB");
  if (cbRename)
  {
    if (cbRename(ih, iupTreeFindNodeId(ih, wItem), title) == IUP_IGNORE)
      ignore = 1;
  }

  if (!ignore)
    iupmotSetXmString(wItem, XmNlabelString, title);

  XtDestroyWidget(wEdit);

  iupAttribSet(ih, "_IUPTREE_EDITFIELD", NULL);
  iupAttribSet(ih, "_IUPTREE_SELECTED",  NULL);
}

static int motTreeCallDragDropCb(Ihandle* ih, Widget wItemDrag, Widget wItemDrop, int *is_ctrl)
{
  IFniiii cbDragDrop = (IFniiii)IupGetCallback(ih, "DRAGDROP_CB");
  int is_shift = 0;
  char key[5];
  iupdrvGetKeyState(key);
  if (key[0] == 'S')
    is_shift = 1;
  if (key[1] == 'C')
    *is_ctrl = 1;
  else
    *is_ctrl = 0;

  if (cbDragDrop)
  {
    int drag_id = iupTreeFindNodeId(ih, wItemDrag);
    int drop_id = iupTreeFindNodeId(ih, wItemDrop);
    return cbDragDrop(ih, drag_id, drop_id, is_shift, *is_ctrl);
  }

  return IUP_CONTINUE; /* allow to move by default if callback not defined */
}

static void motTreeEditFocusChangeEvent(Widget w, Ihandle *ih, XEvent *evt, Boolean *cont)
{
  if (evt->type == FocusOut)
    motTreeCallRenameCb(ih);

  (void)cont;
  (void)w;
}

static void motTreeEditKeyPressEvent(Widget w, Ihandle *ih, XKeyEvent *evt, Boolean *cont)
{
  KeySym motcode = iupmotKeycodeToKeysym(evt);
  if (motcode == XK_Return)
  {
    Widget wItem = (Widget)iupAttribGet(ih, "_IUPTREE_SELECTED");
    motTreeCallRenameCb(ih);
    motTreeSetFocusNode(ih, wItem);
  }
  else if (motcode == XK_Escape)
  {
    Widget wEdit = (Widget)iupAttribGet(ih, "_IUPTREE_EDITFIELD");
    Widget wItem = (Widget)iupAttribGet(ih, "_IUPTREE_SELECTED");

    XtDestroyWidget(wEdit);
    motTreeSetFocusNode(ih, wItem);

    iupAttribSet(ih, "_IUPTREE_EDITFIELD", NULL);
    iupAttribSet(ih, "_IUPTREE_SELECTED",  NULL);
  }

  (void)cont;
  (void)w;
}

static void motTreeScrollbarOffset(Widget sb_win, Position *x, Position *y)
{
  Widget sb_horiz, sb_vert;
  XtVaGetValues(sb_win, XmNhorizontalScrollBar, &sb_horiz, NULL);
  if (sb_horiz) 
  {
    int pos;
    XtVaGetValues(sb_horiz, XmNvalue, &pos, NULL);
    *x = *x - (Position)pos;
  }
  XtVaGetValues(sb_win, XmNverticalScrollBar, &sb_vert, NULL);
  if (sb_vert) 
  {
    int pos;
    XtVaGetValues(sb_vert, XmNvalue, &pos, NULL);
    *y = *y - (Position)pos;
  }
}

static void motTreeShowEditField(Ihandle* ih, Widget wItem)
{
  int num_args = 0, w_img = 0;
  Arg args[30];
  Position x, y;
  Dimension w, h;
  char* child_id = iupDialogGetChildIdStr(ih);
  Widget cbEdit;
  XmString title;
  char* value;
  Pixel color;
  Pixmap image = XmUNSPECIFIED_PIXMAP;
  XmFontList fontlist;
  Widget sb_win = (Widget)iupAttribGet(ih, "_IUP_EXTRAPARENT");

  IFni cbShowRename = (IFni)IupGetCallback(ih, "SHOWRENAME_CB");
  if (cbShowRename && cbShowRename(ih, iupTreeFindNodeId(ih, wItem))==IUP_IGNORE)
    return;

  XtVaGetValues(wItem, XmNx, &x, 
                       XmNy, &y,
                       XmNwidth, &w, 
                       XmNheight, &h, 
                       XmNlabelString, &title,
                       XmNforeground, &color,
                       XmNrenderTable, &fontlist,
                       NULL);

  XtVaGetValues(wItem, XmNsmallIconPixmap, &image, NULL);

  motTreeScrollbarOffset(sb_win, &x, &y);
  iupdrvImageGetInfo((void*)image, &w_img, NULL, NULL);
  w_img += 3; /* add some room for borders */

  value = iupmotGetXmString(title);

  iupMOT_SETARG(args, num_args, XmNx, x+w_img);      /* x-position */
  iupMOT_SETARG(args, num_args, XmNy, y);         /* y-position */
  iupMOT_SETARG(args, num_args, XmNwidth, w-w_img);  /* default width to avoid 0 */
  iupMOT_SETARG(args, num_args, XmNheight, h);    /* default height to avoid 0 */
  iupMOT_SETARG(args, num_args, XmNmarginHeight, ih->data->spacing);  /* default padding */
  iupMOT_SETARG(args, num_args, XmNmarginWidth, 0);
  iupMOT_SETARG(args, num_args, XmNforeground, color);
  iupMOT_SETARG(args, num_args, XmNrenderTable, fontlist);
  iupMOT_SETARG(args, num_args, XmNvalue, value);
  iupMOT_SETARG(args, num_args, XmNtraversalOn, True);

  cbEdit = XtCreateManagedWidget(
    child_id,       /* child identifier */
    xmTextWidgetClass,   /* widget class */
    sb_win,
    args, num_args);

  if (value)
    XtFree(value);

  XtAddEventHandler(cbEdit, EnterWindowMask, False, (XtEventHandler)iupmotEnterLeaveWindowEvent, (XtPointer)ih);
  XtAddEventHandler(cbEdit, LeaveWindowMask, False, (XtEventHandler)iupmotEnterLeaveWindowEvent, (XtPointer)ih);
  XtAddEventHandler(cbEdit, FocusChangeMask, False, (XtEventHandler)motTreeEditFocusChangeEvent, (XtPointer)ih);
  XtAddEventHandler(cbEdit, KeyPressMask,    False, (XtEventHandler)motTreeEditKeyPressEvent, (XtPointer)ih);

  iupAttribSet(ih, "_IUPTREE_SELECTED",  (char*)wItem);
  iupAttribSet(ih, "_IUPTREE_EDITFIELD", (char*)cbEdit);

  XmProcessTraversal(cbEdit, XmTRAVERSE_CURRENT);

  XmTextSetSelection(cbEdit, (XmTextPosition)0, (XmTextPosition)XmTextGetLastPosition(cbEdit), CurrentTime);

  value = iupAttribGetStr(ih, "RENAMECARET");
  if (value)
    motTreeSetRenameCaretPos(cbEdit, value);

  value = iupAttribGetStr(ih, "RENAMESELECTION");
  if (value)
    motTreeSetRenameSelectionPos(cbEdit, value);

  /* the parents callbacks can be called while editing 
     so we must avoid their processing if _IUPTREE_EDITFIELD is defined. */
}

static void motTreeSelectionCallback(Widget w, Ihandle* ih, XmContainerSelectCallbackStruct *nptr)
{
  IFnii cbSelec;
  int is_ctrl = 0;
  (void)w;
  (void)nptr;

  if (ih->data->mark_mode == ITREE_MARK_MULTIPLE)
  {
    char key[5];
    iupdrvGetKeyState(key);
    if (key[0] == 'S')
      return;
    else if (key[1] == 'C')
      is_ctrl = 1;

    if (nptr->selected_item_count>1 && !is_ctrl)
    {
      if (IupGetCallback(ih, "MULTISELECTION_CB"))
      {
        /* current selection same as the initial selection */
        if (nptr->auto_selection_type==XmAUTO_NO_CHANGE)
          motTreeCallMultiSelectionCb(ih);
      }
      else
      {
        /* current selection is caused by button drag */
        if (nptr->auto_selection_type==XmAUTO_MOTION)
          motTreeCallMultiSelectionCb(ih);
      }
      return;
    }
  }

  cbSelec = (IFnii)IupGetCallback(ih, "SELECTION_CB");
  if (cbSelec)
  {
    Widget wItemFocus = iupdrvTreeGetFocusNode(ih);
    int curpos = iupTreeFindNodeId(ih, wItemFocus);

    if (is_ctrl)
    {
      cbSelec(ih, curpos, motTreeIsNodeSelected(wItemFocus));
      iupAttribSetInt(ih, "_IUPTREE_OLDVALUE", -2); /* invalid value signalizing that its state was toggled */
    }
    else
    {
      int oldpos = iupAttribGetInt(ih, "_IUPTREE_OLDVALUE");
      if (oldpos != curpos)
      {
        if (oldpos >= 0)
          cbSelec(ih, oldpos, 0);  /* unselected */
        cbSelec(ih, curpos, 1);  /*   selected */

        iupAttribSetInt(ih, "_IUPTREE_OLDVALUE", curpos);
      }
    }
  }
}

static void motTreeDefaultActionCallback(Widget w, Ihandle* ih, XmContainerSelectCallbackStruct *nptr)
{
  unsigned char itemState;
  WidgetList wSelectedItemList = NULL;
  int countItems;
  motTreeItemData *itemData;
  Widget wItem;
  (void)w;

  wSelectedItemList = nptr->selected_items;
  countItems = nptr->selected_item_count;

  if (!countItems || (Widget)iupAttribGet(ih, "_IUPTREE_EDITFIELD"))
    return;

  /* this works also when using multiple selection */
  wItem = wSelectedItemList[0];

  XtVaGetValues(wItem, XmNoutlineState, &itemState,
                       XmNuserData, &itemData, NULL);

  if (itemData->kind == ITREE_BRANCH)
  {
    IFni cbExecuteBranch = (IFni)IupGetCallback(ih, "EXECUTEBRANCH_CB");
    if (cbExecuteBranch)
      cbExecuteBranch(ih, iupTreeFindNodeId(ih, wItem));

    if (itemState == XmEXPANDED)
      XtVaSetValues(wItem, XmNoutlineState,  XmCOLLAPSED, NULL);
    else
      XtVaSetValues(wItem, XmNoutlineState,  XmEXPANDED, NULL);
  }
  else
  {
    IFni cbExecuteLeaf = (IFni)IupGetCallback(ih, "EXECUTELEAF_CB");
    if (cbExecuteLeaf)
      cbExecuteLeaf(ih, iupTreeFindNodeId(ih, wItem));
  }
}

static void motTreeOutlineChangedCallback(Widget w, Ihandle* ih, XmContainerOutlineCallbackStruct *nptr)
{
  motTreeItemData *itemData;
  XtVaGetValues(nptr->item, XmNuserData, &itemData, NULL);

  if (nptr->reason == XmCR_EXPANDED)
  {
    if (motTreeCallBranchOpenCb(ih, nptr->item) == IUP_IGNORE)
      nptr->new_outline_state = XmCOLLAPSED; /* prevent the change */
    else
    {
      XtVaSetValues(nptr->item, XmNsmallIconPixmap, (itemData->image_expanded!=XmUNSPECIFIED_PIXMAP)? itemData->image_expanded: (Pixmap)ih->data->def_image_expanded, NULL);
      XtVaSetValues(nptr->item, XmNsmallIconMask, (itemData->image_expanded_mask!=XmUNSPECIFIED_PIXMAP)? itemData->image_expanded_mask: (Pixmap)ih->data->def_image_expanded_mask, NULL);
    }
  }
  else if (nptr->reason == XmCR_COLLAPSED)
  {
    if (motTreeCallBranchCloseCb(ih, nptr->item) == IUP_IGNORE)
      nptr->new_outline_state = XmEXPANDED;  /* prevent the change */
    else
    {
      XtVaSetValues(nptr->item, XmNsmallIconPixmap, (itemData->image!=XmUNSPECIFIED_PIXMAP)? itemData->image: (Pixmap)ih->data->def_image_collapsed, NULL);
      XtVaSetValues(nptr->item, XmNsmallIconMask, (itemData->image_mask!=XmUNSPECIFIED_PIXMAP)? itemData->image_mask: (Pixmap)ih->data->def_image_collapsed_mask, NULL);
    }
  }

  (void)w;
}

static void motTreeTraverseObscuredCallback(Widget widget, Ihandle* ih, XmTraverseObscuredCallbackStruct *cbs)
{
  (void)ih;
  /* allow to do automatic scroll when navigating in the tree */
  XmScrollVisible(widget, cbs->traversal_destination, 10, 10);
}

static void motTreeKeyReleaseEvent(Widget w, Ihandle *ih, XKeyEvent *evt, Boolean *cont)
{
  KeySym motcode;

  if (iupAttribGet(ih, "_IUPTREE_EDITFIELD"))
    return;

  motcode = iupmotKeycodeToKeysym(evt);
  if (motcode == XK_Down || motcode == XK_U || motcode == XK_Home || motcode == XK_End)
  {
    if (ih->data->mark_mode==ITREE_MARK_MULTIPLE && (evt->state & ShiftMask))
      motTreeCallMultiSelectionCb(ih);
  }

  (void)w;
  (void)cont;
}

static void motTreeKeyPressEvent(Widget w, Ihandle *ih, XKeyEvent *evt, Boolean *cont)
{
  KeySym motcode;

  if (iupAttribGet(ih, "_IUPTREE_EDITFIELD"))
    return;

  *cont = True;
  iupmotKeyPressEvent(w, ih, (XEvent*)evt, cont);
  if (*cont == False)
    return;

  motcode = iupmotKeycodeToKeysym(evt);
  if (motcode == XK_F2)
    motTreeSetRenameAttrib(ih, NULL);
  else if (motcode == XK_F1)
    iupmotHelpCallback(w, ih, NULL);
  else if ((motcode == XK_Down || motcode == XK_Up) && (evt->state & ControlMask))
  {
    Widget wItem;
    Widget wItemFocus = iupdrvTreeGetFocusNode(ih);

    /* Ctrl+Arrows move only focus */
    if (motcode == XK_Down)
      wItem = motTreeGetNextVisibleNode(ih, wItemFocus, 1);
    else
      wItem = motTreeGetPreviousVisibleNode(ih, wItemFocus, 1);

    motTreeSetFocusNode(ih, wItem);
    *cont = False;
  }
  else if(motcode == XK_Home || motcode == XK_End)
  {
    Widget wRoot = (Widget)iupAttribGet(ih, "_IUPTREE_ROOTITEM");
    Widget wItem;

    /* Not processed by Motif */

    if (motcode == XK_Home)
      wItem = wRoot;
    else /* motcode == XK_End */
      wItem = motTreeGetLastVisibleNode(ih);

    /* Ctrl+Arrows move only focus */
    if (!(evt->state & ControlMask))
    {
      /* Shift+Arrows select block */
      if (evt->state & ShiftMask)
      {
        Widget wItemFocus = iupdrvTreeGetFocusNode(ih);
        if (!wItemFocus)
          return;
        motTreeSelectRange(ih, wItemFocus, wItem, 1);
      }
      else
      {
        /* clear previous selection */
        XtVaSetValues(ih->handle, XmNselectedObjects,  NULL, NULL);
        XtVaSetValues(ih->handle, XmNselectedObjectCount, 0, NULL);

        XtVaSetValues(wItem, XmNvisualEmphasis, XmSELECTED, NULL);
      }
    }

    motTreeSetFocusNode(ih, wItem);
    *cont = False;
  }
  else if(motcode == XK_space && (evt->state & ControlMask))
  {
    Widget wItemFocus = iupdrvTreeGetFocusNode(ih);
    if (wItemFocus)
      motTreeSelectNode(wItemFocus, -1);
  }
}

static void motTreeButtonEvent(Widget w, Ihandle* ih, XButtonEvent* evt, Boolean* cont)
{
  (void)w;
  (void)cont;

  if (iupAttribGet(ih, "_IUPTREE_EDITFIELD"))
    return;

  *cont = True;
  iupmotButtonPressReleaseEvent(w, ih, (XEvent*)evt, cont);
  if (*cont == False)
    return;

  if (evt->type==ButtonPress)
  {   
    iupAttribSet(ih, "_IUPTREE_IGNORE_ENTERLEAVE", "1");

    if (evt->button==Button1)
    {
      Widget wItem = XmObjectAtPoint(ih->handle, (Position)evt->x, (Position)evt->y);
      static Widget wLastItem = NULL;
      static Time last = 0;
      int clicktwice = 0, doubleclicktime = XtGetMultiClickTime(iupmot_display);
      int elapsed = (int)(evt->time - last);
      last = evt->time;

      /* stay away from double click and leave some room for single clicks */
      if (elapsed > (3*doubleclicktime)/2 && elapsed <= 3*doubleclicktime)
        clicktwice = 1;
    
      if (clicktwice && wLastItem && wLastItem==wItem)
      {
        motTreeSetRenameAttrib(ih, NULL);
        *cont = False;
      }
      wLastItem = wItem;

      if (wItem && ih->data->mark_mode==ITREE_MARK_MULTIPLE && 
          !(evt->state & ShiftMask) &&
          !(evt->state & ControlMask))
      {
        /* simple click with mark_mode==ITREE_MARK_MULTIPLE and !Shift and !Ctrl */
        /* do not call the callback for the new selected item */
        int new_select_id = iupTreeFindNodeId(ih, wItem);
        if (new_select_id != -1)
          motTreeCallMultiUnSelectionCb(ih, new_select_id);
      }
    }
    else if (evt->button==Button3)
      motTreeCallRightClickCb(ih, evt->x, evt->y);
  }
  else if (evt->type==ButtonRelease)
  {   
    if (evt->button==Button1)
    {
      if (ih->data->mark_mode==ITREE_MARK_MULTIPLE && (evt->state & ShiftMask))
        motTreeCallMultiSelectionCb(ih);
    }
  }
}

static void motTreeDragTransferProc(Widget drop_context, XtPointer client_data, Atom *seltype, Atom *type, XtPointer value, unsigned long *length, int format)
{
  Atom atomTreeItem = XInternAtom(iupmot_display, "TREE_ITEM", False);
  Widget wItemDrop = (Widget)client_data;
  Widget wItemDrag = (Widget)value;

  if (wItemDrag && wItemDrop && *type == atomTreeItem)
  {
    int equal_nodes = 0;
    Widget wParent;
    Ihandle* ih = NULL;
    int is_ctrl;

    /* If Drag item is an ancestor or equal to Drop item then return */
    wParent = wItemDrop;
    while(wParent)
    {
      if (wParent == wItemDrag)
      {
        if (!iupAttribGetBoolean(ih, "DROPEQUALDRAG"))
          return;

        equal_nodes = 1;
        break;
      }

      XtVaGetValues(wParent, XmNentryParent, &wParent, NULL);
    }

    XtVaGetValues(XtParent(wItemDrag), XmNuserData, &ih, NULL);

    if (motTreeCallDragDropCb(ih, wItemDrag, wItemDrop, &is_ctrl) == IUP_CONTINUE && !equal_nodes)
    {
      /* Copy or move the dragged item to the new position. */
      Widget wItemNew = motTreeCopyMoveNode(ih, wItemDrag, wItemDrop, is_ctrl);

      /* Set focus and selection */
      if (wItemNew)
      {
        /* unselect all, select new node */
        XtVaSetValues(ih->handle, XmNselectedObjects,  NULL, NULL);
        XtVaSetValues(ih->handle, XmNselectedObjectCount, 0, NULL);
        XtVaSetValues(wItemNew, XmNvisualEmphasis, XmSELECTED, NULL);

        motTreeSetFocusNode(ih, wItemNew);
      }
    }
  }

  (void)drop_context;
  (void)seltype;
  (void)format;
  (void)length;
}

static void motTreeDragDropProc(Widget w, XtPointer client_data, XmDropProcCallbackStruct* drop_data)
{
  Atom atomTreeItem = XInternAtom(iupmot_display, "TREE_ITEM", False);
  XmDropTransferEntryRec transferList[2];
  Arg args[10];
  int i, num_args = 0;
  Widget wItemDrop, drop_context;
  Cardinal numExportTargets;
  Atom *exportTargets;
  Boolean found = False;
  (void)client_data;

  drop_context = drop_data->dragContext;

  /* retrieve the data targets */
  XtVaGetValues(drop_context, XmNexportTargets, &exportTargets,
                              XmNnumExportTargets, &numExportTargets, 
                              NULL);

  for (i = 0; i < (int)numExportTargets; i++) 
  {
    if (exportTargets[i] == atomTreeItem) 
    {
      found = True;
      break;
    }
  }

  wItemDrop = XmObjectAtPoint(w, drop_data->x, drop_data->y);
  if (!wItemDrop)
    found = False;

  num_args = 0;
  if ((!found) || (drop_data->dropAction != XmDROP) ||  (drop_data->operation != XmDROP_COPY && drop_data->operation != XmDROP_MOVE)) 
  {
    iupMOT_SETARG(args, num_args, XmNtransferStatus, XmTRANSFER_FAILURE);
    iupMOT_SETARG(args, num_args, XmNnumDropTransfers, 0);
  }
  else 
  {
    /* set up transfer requests for drop site */
    transferList[0].target = atomTreeItem;
    transferList[0].client_data = (XtPointer)wItemDrop;
    iupMOT_SETARG(args, num_args, XmNdropTransfers, transferList);
    iupMOT_SETARG(args, num_args, XmNnumDropTransfers, 1);
    iupMOT_SETARG(args, num_args, XmNtransferProc, motTreeDragTransferProc);
  }

  XmDropTransferStart(drop_context, args, num_args);
}

static void motTreeDragDropFinishCallback(Widget drop_context, XtPointer client_data, XtPointer call_data)
{
  Widget source_icon = NULL;
  XtVaGetValues(drop_context, XmNsourceCursorIcon, &source_icon, NULL);
  if (source_icon)
    XtDestroyWidget(source_icon);
  (void)call_data;
  (void)client_data;
}

static void motTreeDragMotionCallback(Widget drop_context, Widget wItemDrag, XmDragMotionCallbackStruct* drag_motion)
{
  Ihandle* ih = NULL;
  XtVaGetValues(XtParent(wItemDrag), XmNuserData, &ih, NULL);
  if (!iupAttribGet(ih, "NODRAGFEEDBACK"))
  {
    Widget wItem;
    int x = drag_motion->x;
    int y = drag_motion->y;
    iupdrvScreenToClient(ih, &x, &y);
    wItem = XmObjectAtPoint(ih->handle, (Position)x, (Position)y);
    if (wItem)
    {
      XtVaSetValues(ih->handle, XmNselectedObjects,  NULL, NULL);
      XtVaSetValues(ih->handle, XmNselectedObjectCount, 0, NULL);
      XtVaSetValues(wItem, XmNvisualEmphasis, XmSELECTED, NULL);
    }
  }
  (void)drop_context;
}

static Boolean motTreeConvertProc(Widget drop_context, Atom *selection, Atom *target, Atom *type_return,
                                  XtPointer *value_return, unsigned long *length_return, int *format_return)
{
  Atom atomMotifDrop = XInternAtom(iupmot_display, "_MOTIF_DROP", False);
  Atom atomTreeItem = XInternAtom(iupmot_display, "TREE_ITEM", False);
  Widget wItemDrag = NULL;

  /* check if we are dealing with a drop */
  if (*selection != atomMotifDrop || *target != atomTreeItem)
    return False;

  XtVaGetValues(drop_context, XmNclientData, &wItemDrag, NULL);
  if (!wItemDrag)
    return False;

  /* format the value for transfer */
  *type_return = atomTreeItem;
  *value_return = (XtPointer)wItemDrag;
  *length_return = 1;
  *format_return = 32;
  return True;
}

static void motTreeDragStart(Widget w, XButtonEvent* evt, String* params, Cardinal* num_params)
{
  Atom atomTreeItem = XInternAtom(iupmot_display, "TREE_ITEM", False);
  Atom exportList[1];
  Widget drag_icon, drop_context;
  Pixmap pixmap = 0, mask = 0;
  unsigned char typeWidget;
  int num_args = 0;
  Arg args[40];
  Pixel fg, bg;
  Widget wItemDrag = XmObjectAtPoint(w, (Position)evt->x, (Position)evt->y);
  if (!wItemDrag)
    return;

  XtVaGetValues(wItemDrag, XmNviewType, &typeWidget,
                           XmNbackground, &bg,
                           XmNforeground, &fg,
                           XmNsmallIconPixmap, &pixmap, 
                           XmNsmallIconMask, &mask, 
                           NULL);

  iupMOT_SETARG(args, num_args, XmNpixmap, pixmap);
  iupMOT_SETARG(args, num_args, XmNmask, mask);
  drag_icon = XmCreateDragIcon(w, "drag_icon", args, num_args);

  exportList[0] = atomTreeItem;

  /* specify resources for DragContext for the transfer */
  num_args = 0;
  iupMOT_SETARG(args, num_args, XmNcursorBackground, bg);
  iupMOT_SETARG(args, num_args, XmNcursorForeground, fg);
  /* iupMOT_SETARG(args, num_args, XmNsourcePixmapIcon, drag_icon);  works, but only outside the dialog, inside disapears */
  iupMOT_SETARG(args, num_args, XmNsourceCursorIcon, drag_icon);  /* does not work, shows the default cursor */
  iupMOT_SETARG(args, num_args, XmNexportTargets, exportList);
  iupMOT_SETARG(args, num_args, XmNnumExportTargets, 1);
  iupMOT_SETARG(args, num_args, XmNdragOperations, XmDROP_MOVE|XmDROP_COPY);
  iupMOT_SETARG(args, num_args, XmNconvertProc, motTreeConvertProc);
  iupMOT_SETARG(args, num_args, XmNclientData, wItemDrag);

  /* start the drag and register a callback to clean up when done */
  drop_context = XmDragStart(w, (XEvent*)evt, args, num_args);
  XtAddCallback(drop_context, XmNdragDropFinishCallback, (XtCallbackProc)motTreeDragDropFinishCallback, NULL);
  XtAddCallback(drop_context, XmNdragMotionCallback, (XtCallbackProc)motTreeDragMotionCallback, (XtPointer)wItemDrag);

  (void)params;
  (void)num_params;
}

static void motTreeDragDropEnable(Widget w)
{
  Atom atomTreeItem = XInternAtom(iupmot_display, "TREE_ITEM", False);
  Atom importList[1];
  Arg args[40];
  int num_args = 0;
  char dragTranslations[] = "#override <Btn2Down>: iupTreeStartDrag()";
  static int do_rec = 0;
  if (!do_rec)
  {
    XtActionsRec rec = {"iupTreeStartDrag", (XtActionProc)motTreeDragStart};
    XtAppAddActions(iupmot_appcontext, &rec, 1);
    do_rec = 1;
  }
  XtOverrideTranslations(w, XtParseTranslationTable(dragTranslations));

  importList[0] = atomTreeItem;
  iupMOT_SETARG(args, num_args, XmNimportTargets, importList);
  iupMOT_SETARG(args, num_args, XmNnumImportTargets, 1);
  iupMOT_SETARG(args, num_args, XmNdropSiteOperations, XmDROP_MOVE|XmDROP_COPY);
  iupMOT_SETARG(args, num_args, XmNdropProc, motTreeDragDropProc);
  XmDropSiteUpdate(w, args, num_args);
}

/*******************************************************************************************/

static Widget motTreeDragDropCopyItem(Ihandle* src, Ihandle* dst, Widget wItem, Widget wParent, int pos)
{
  Widget wItemNew;
  XmString title;
  motTreeItemData *itemData;
  Pixel fgcolor, bgcolor;
  int num_args = 0;
  Arg args[30];
  Pixmap image = XmUNSPECIFIED_PIXMAP, mask = XmUNSPECIFIED_PIXMAP;
  unsigned char state;
  motTreeItemData* itemDataNew;
  (void)src;

  iupMOT_SETARG(args, num_args, XmNentryParent, wParent);
  iupMOT_SETARG(args, num_args, XmNmarginHeight, dst->data->spacing);
  iupMOT_SETARG(args, num_args, XmNmarginWidth, 0);
  iupMOT_SETARG(args, num_args, XmNnavigationType, XmTAB_GROUP);
  iupMOT_SETARG(args, num_args, XmNtraversalOn, True);
  iupMOT_SETARG(args, num_args, XmNshadowThickness, 0);

  XtVaGetValues(wItem, XmNlabelString, &title,
                          XmNuserData, &itemData,
                        XmNforeground, &fgcolor,
                      XmNoutlineState, &state,
                                       NULL);

  /* Get values to copy */
  XtVaGetValues(wItem, XmNsmallIconPixmap, &image, 
                         XmNsmallIconMask, &mask,
                                           NULL);

  /* create a new one */
  itemDataNew = malloc(sizeof(motTreeItemData));
  memcpy(itemDataNew, itemData, sizeof(motTreeItemData));
  itemData = itemDataNew;

  iupMOT_SETARG(args, num_args, XmNlabelString, title);
  iupMOT_SETARG(args, num_args, XmNuserData, itemData);
  iupMOT_SETARG(args, num_args, XmNforeground, fgcolor);
  iupMOT_SETARG(args, num_args, XmNoutlineState, state);

  iupMOT_SETARG(args, num_args, XmNviewType, XmSMALL_ICON);
  iupMOT_SETARG(args, num_args, XmNsmallIconPixmap, image);
  iupMOT_SETARG(args, num_args, XmNsmallIconMask, mask);

  iupMOT_SETARG(args, num_args, XmNentryParent, wParent);
  iupMOT_SETARG(args, num_args, XmNpositionIndex, pos);

  XtVaGetValues(dst->handle, XmNbackground, &bgcolor, NULL);
  iupMOT_SETARG(args, num_args, XmNbackground, bgcolor);

  /* Add the new node */
  wItemNew = XtCreateManagedWidget("icon", xmIconGadgetClass, dst->handle, args, num_args);

  dst->data->node_count++;

  XtRealizeWidget(wItemNew);

  return wItemNew;
}

static void motTreeDragDropCopyChildren(Ihandle* src, Ihandle* dst, Widget wItemSrc, Widget wItemDst)
{
  WidgetList wItemChildList = NULL;
  int i = 0;
  int numChild = XmContainerGetItemChildren(src->handle, wItemSrc, &wItemChildList);
  while(i != numChild)
  {
    Widget wItemNew = motTreeDragDropCopyItem(src, dst, wItemChildList[i], wItemDst, i);  /* Use the same order they were enumerated */

    /* Recursively transfer all the items */
    motTreeDragDropCopyChildren(src, dst, wItemChildList[i], wItemNew);  

    /* Go to next sibling item */
    i++;
  }

  if (wItemChildList) XtFree((char*)wItemChildList);
}

void iupdrvTreeDragDropCopyNode(Ihandle* src, Ihandle* dst, InodeHandle *itemSrc, InodeHandle *itemDst)
{
  Widget wItemNew, wParent, wItemSrc = itemSrc, wItemDst = itemDst;
  motTreeItemData *itemDataDst;
  unsigned char stateDst;
  int pos, id_new, count, id_dst;

  int old_count = dst->data->node_count;

  id_dst = iupTreeFindNodeId(dst, wItemDst);
  id_new = id_dst+1;  /* contains the position for a copy operation */

  XtVaGetValues(wItemDst, XmNoutlineState, &stateDst, 
                          XmNuserData, &itemDataDst, 
                          NULL);

  if (itemDataDst->kind == ITREE_BRANCH && stateDst == XmEXPANDED)
  {
    /* copy as first child of expanded branch */
    wParent = wItemDst;
    pos = 0;
  }
  else
  {
    if (itemDataDst->kind == ITREE_BRANCH)
    {
      int child_count = iupdrvTreeTotalChildCount(dst, wItemDst);
      id_new += child_count;
    }

    /* copy as next brother of item or collapsed branch */
    XtVaGetValues(wItemDst, XmNentryParent, &wParent, NULL);
    XtVaGetValues(wItemDst, XmNpositionIndex, &pos, NULL);
    pos++;
  }

  wItemNew = motTreeDragDropCopyItem(src, dst, wItemSrc, wParent, pos);

  motTreeDragDropCopyChildren(src, dst, wItemSrc, wItemNew);

  count = dst->data->node_count - old_count;
  iupTreeCopyMoveCache(dst, id_dst, id_new, count, 1);  /* update only the dst control cache */
  motTreeRebuildNodeCache(dst, id_new, wItemNew);
}

/*******************************************************************************************/

static int motTreeMapMethod(Ihandle* ih)
{
  int num_args = 0;
  Arg args[40];
  Widget parent = iupChildTreeGetNativeParentHandle(ih);
  char* child_id = iupDialogGetChildIdStr(ih);
  Widget sb_win;

  /******************************/
  /* Create the scrolled window */
  /******************************/
  iupMOT_SETARG(args, num_args, XmNmappedWhenManaged, False);  /* not visible when managed */
  iupMOT_SETARG(args, num_args, XmNscrollingPolicy, XmAUTOMATIC);
  iupMOT_SETARG(args, num_args, XmNvisualPolicy, XmVARIABLE); 
  iupMOT_SETARG(args, num_args, XmNscrollBarDisplayPolicy, XmAS_NEEDED);
  iupMOT_SETARG(args, num_args, XmNspacing, 0); /* no space between scrollbars and text */
  iupMOT_SETARG(args, num_args, XmNborderWidth, 0);
  iupMOT_SETARG(args, num_args, XmNshadowThickness, 2);

  sb_win = XtCreateManagedWidget(
    child_id,  /* child identifier */
    xmScrolledWindowWidgetClass, /* widget class */
    parent,                      /* widget parent */
    args, num_args);

  if (!sb_win)
     return IUP_ERROR;

  XtAddCallback (sb_win, XmNtraverseObscuredCallback, (XtCallbackProc)motTreeTraverseObscuredCallback, (XtPointer)ih);

  parent = sb_win;
  child_id = "container";

  num_args = 0;
 
  iupMOT_SETARG(args, num_args, XmNx, 0);  /* x-position */
  iupMOT_SETARG(args, num_args, XmNy, 0);  /* y-position */
  iupMOT_SETARG(args, num_args, XmNwidth, 10);  /* default width to avoid 0 */
  iupMOT_SETARG(args, num_args, XmNheight, 10); /* default height to avoid 0 */

  iupMOT_SETARG(args, num_args, XmNmarginHeight, 0);  /* default padding */
  iupMOT_SETARG(args, num_args, XmNmarginWidth, 0);

  if (iupAttribGetBoolean(ih, "CANFOCUS"))
    iupMOT_SETARG(args, num_args, XmNtraversalOn, True);
  else
    iupMOT_SETARG(args, num_args, XmNtraversalOn, False);

  iupMOT_SETARG(args, num_args, XmNnavigationType, XmTAB_GROUP);
  iupMOT_SETARG(args, num_args, XmNhighlightThickness, 2);
  iupMOT_SETARG(args, num_args, XmNshadowThickness, 0);

  iupMOT_SETARG(args, num_args, XmNlayoutType, XmOUTLINE);
  iupMOT_SETARG(args, num_args, XmNentryViewType, XmSMALL_ICON);
  iupMOT_SETARG(args, num_args, XmNselectionPolicy, XmSINGLE_SELECT);
  iupMOT_SETARG(args, num_args, XmNoutlineIndentation, 20);

  if (iupAttribGetBoolean(ih, "HIDELINES"))
    iupMOT_SETARG(args, num_args, XmNoutlineLineStyle,  XmNO_LINE);
  else
    iupMOT_SETARG(args, num_args, XmNoutlineLineStyle, XmSINGLE);

  if (iupAttribGetBoolean(ih, "HIDEBUTTONS"))
    iupMOT_SETARG(args, num_args, XmNoutlineButtonPolicy, XmOUTLINE_BUTTON_ABSENT);
  else
    iupMOT_SETARG(args, num_args, XmNoutlineButtonPolicy, XmOUTLINE_BUTTON_PRESENT);

  ih->handle = XtCreateManagedWidget(
    child_id,       /* child identifier */
    xmContainerWidgetClass,   /* widget class */
    parent,         /* widget parent */
    args, num_args);

  if (!ih->handle)
    return IUP_ERROR;

  ih->serial = iupDialogGetChildId(ih); /* must be after using the string */

  iupAttribSet(ih, "_IUP_EXTRAPARENT", (char*)parent);

  XtAddEventHandler(ih->handle, EnterWindowMask, False, (XtEventHandler)motTreeEnterLeaveWindowEvent, (XtPointer)ih);
  XtAddEventHandler(ih->handle, LeaveWindowMask, False, (XtEventHandler)motTreeEnterLeaveWindowEvent, (XtPointer)ih);
  XtAddEventHandler(ih->handle, FocusChangeMask, False, (XtEventHandler)motTreeFocusChangeEvent,  (XtPointer)ih);
  XtAddEventHandler(ih->handle, KeyPressMask,    False, (XtEventHandler)motTreeKeyPressEvent,    (XtPointer)ih);
  XtAddEventHandler(ih->handle, KeyReleaseMask,  False, (XtEventHandler)motTreeKeyReleaseEvent,  (XtPointer)ih);
  XtAddEventHandler(ih->handle, ButtonPressMask|ButtonReleaseMask, False, (XtEventHandler)motTreeButtonEvent, (XtPointer)ih);
  XtAddEventHandler(ih->handle, PointerMotionMask, False, (XtEventHandler)iupmotPointerMotionEvent, (XtPointer)ih);

  /* Callbacks */
  /* XtAddCallback(ih->handle, XmNhelpCallback,           (XtCallbackProc)iupmotHelpCallback, (XtPointer)ih);  NOT WORKING */
  XtAddCallback(ih->handle, XmNoutlineChangedCallback, (XtCallbackProc)motTreeOutlineChangedCallback, (XtPointer)ih);
  XtAddCallback(ih->handle, XmNdefaultActionCallback,  (XtCallbackProc)motTreeDefaultActionCallback,  (XtPointer)ih);
  XtAddCallback(ih->handle, XmNselectionCallback,      (XtCallbackProc)motTreeSelectionCallback,      (XtPointer)ih);

  XtRealizeWidget(parent);

  if (ih->data->show_dragdrop || (IupGetInt(ih, "DRAGDROPTREE")))  /* Enable drag and drop support between trees */
  {
    motTreeDragDropEnable(ih->handle);
    XtVaSetValues(ih->handle, XmNuserData, ih, NULL);  /* to be used in motTreeDragTransferProc */
  }
  else
    iupmotDisableDragSource(ih->handle);

  /* Force background update before setting the images */
  {
    char* value = iupAttribGet(ih, "BGCOLOR");
    if (value)
    {
      motTreeSetBgColorAttrib(ih, value);
      iupAttribSet(ih, "BGCOLOR", NULL);
    }
  }

  /* Initialize the default images */
  ih->data->def_image_leaf = iupImageGetImage(iupAttribGetStr(ih, "IMAGELEAF"), ih, 0, NULL);
  if (!ih->data->def_image_leaf) 
  {
    ih->data->def_image_leaf = (void*)XmUNSPECIFIED_PIXMAP;
    ih->data->def_image_leaf_mask = (void*)XmUNSPECIFIED_PIXMAP;
  }
  else
  {
    ih->data->def_image_leaf_mask = (void*)iupmotImageGetMask(iupAttribGetStr(ih, "IMAGELEAF"));
    if (!ih->data->def_image_leaf_mask) ih->data->def_image_leaf_mask = (void*)XmUNSPECIFIED_PIXMAP;
  }

  ih->data->def_image_collapsed = iupImageGetImage(iupAttribGetStr(ih, "IMAGEBRANCHCOLLAPSED"), ih, 0, NULL);
  if (!ih->data->def_image_collapsed) 
  {
    ih->data->def_image_collapsed = (void*)XmUNSPECIFIED_PIXMAP;
    ih->data->def_image_collapsed_mask = (void*)XmUNSPECIFIED_PIXMAP;
  }
  else
  {
    ih->data->def_image_collapsed_mask = (void*)iupmotImageGetMask(iupAttribGetStr(ih, "IMAGEBRANCHCOLLAPSED"));
    if (!ih->data->def_image_collapsed_mask) ih->data->def_image_collapsed_mask = (void*)XmUNSPECIFIED_PIXMAP;
  }

  ih->data->def_image_expanded = iupImageGetImage(iupAttribGetStr(ih, "IMAGEBRANCHEXPANDED"), ih, 0, NULL);
  if (!ih->data->def_image_expanded) 
  {
    ih->data->def_image_expanded = (void*)XmUNSPECIFIED_PIXMAP;
    ih->data->def_image_expanded_mask = (void*)XmUNSPECIFIED_PIXMAP;
  }
  else
  {
    ih->data->def_image_expanded_mask = (void*)iupmotImageGetMask(iupAttribGetStr(ih, "IMAGEBRANCHEXPANDED"));
    if (!ih->data->def_image_expanded_mask) ih->data->def_image_expanded_mask = (void*)XmUNSPECIFIED_PIXMAP;
  }

  if (iupAttribGetInt(ih, "ADDROOT"))
    iupdrvTreeAddNode(ih, -1, ITREE_BRANCH, "", 0);

  IupSetCallback(ih, "_IUP_XY2POS_CB", (Icallback)motTreeConvertXYToPos);

  iupdrvTreeUpdateMarkMode(ih);

  return IUP_NOERROR;
}

static void motTreeUnMapMethod(Ihandle* ih)
{
  motTreeRemoveAllNodes(ih, 0);

  ih->data->node_count = 0;

  iupdrvBaseUnMapMethod(ih);
}

void iupdrvTreeInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = motTreeMapMethod;
  ic->UnMap = motTreeUnMapMethod;

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, motTreeSetBgColorAttrib, "TXTBGCOLOR", NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, motTreeSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "HLCOLOR", NULL, motTreeSetHlColorAttrib, IUPAF_SAMEASSYSTEM, "TXTHLCOLOR", IUPAF_NO_INHERIT);

  /* IupTree Attributes - GENERAL */
  iupClassRegisterAttribute(ic, "EXPANDALL", NULL, motTreeSetExpandAllAttrib, NULL, NULL, IUPAF_WRITEONLY||IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INDENTATION", motTreeGetIndentationAttrib, motTreeSetIndentationAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "SPACING", iupTreeGetSpacingAttrib, motTreeSetSpacingAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "TOPITEM", NULL, motTreeSetTopItemAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  /* IupTree Attributes - IMAGES */
  iupClassRegisterAttributeId(ic, "IMAGE", NULL, motTreeSetImageAttrib, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IMAGEEXPANDED", NULL, motTreeSetImageExpandedAttrib, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "IMAGELEAF",            NULL, motTreeSetImageLeafAttrib, IUPAF_SAMEASSYSTEM, "IMGLEAF", IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEBRANCHCOLLAPSED", NULL, motTreeSetImageBranchCollapsedAttrib, IUPAF_SAMEASSYSTEM, "IMGCOLLAPSED", IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEBRANCHEXPANDED",  NULL, motTreeSetImageBranchExpandedAttrib, IUPAF_SAMEASSYSTEM, "IMGEXPANDED", IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);

  /* IupTree Attributes - NODES */
  iupClassRegisterAttributeId(ic, "STATE",  motTreeGetStateAttrib,  motTreeSetStateAttrib, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "DEPTH",  motTreeGetDepthAttrib,  NULL, IUPAF_READONLY|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "KIND",   motTreeGetKindAttrib,   NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "PARENT", motTreeGetParentAttrib, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "NEXT", motTreeGetNextAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "PREVIOUS", motTreeGetPreviousAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "LAST", motTreeGetLastAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "FIRST", motTreeGetFirstAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "COLOR", motTreeGetColorAttrib, motTreeSetColorAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TITLE",  motTreeGetTitleAttrib,  motTreeSetTitleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "CHILDCOUNT", motTreeGetChildCountAttrib, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TITLEFONT", motTreeGetTitleFontAttrib, motTreeSetTitleFontAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ROOTCOUNT", motTreeGetRootCountAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);

  /* IupTree Attributes - MARKS */
  iupClassRegisterAttributeId(ic, "MARKED", motTreeGetMarkedAttrib,  motTreeSetMarkedAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute  (ic, "MARK",      NULL, motTreeSetMarkAttrib,      NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute  (ic, "STARTING",  NULL, motTreeSetMarkStartAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute  (ic, "MARKSTART", NULL, motTreeSetMarkStartAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute  (ic, "MARKEDNODES", motTreeGetMarkedNodesAttrib, motTreeSetMarkedNodesAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute  (ic, "VALUE", motTreeGetValueAttrib, motTreeSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  /* IupTree Attributes - ACTION */
  iupClassRegisterAttributeId(ic, "DELNODE", NULL, motTreeSetDelNodeAttrib, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RENAME", NULL, motTreeSetRenameAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "MOVENODE", NULL, motTreeSetMoveNodeAttrib, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "COPYNODE", NULL, motTreeSetCopyNodeAttrib, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  /* not supported in Motif */
  iupClassRegisterAttributeId(ic, "TOGGLEVALUE", NULL, NULL, IUPAF_NOT_SUPPORTED);
  iupClassRegisterAttributeId(ic, "SHOWTOGGLE", NULL, NULL, IUPAF_NOT_SUPPORTED);
}
