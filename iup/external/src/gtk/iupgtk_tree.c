/** \file
 * \brief Tree Control
 *
 * See Copyright Notice in iup.h
 */

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#if GTK_CHECK_VERSION(3, 0, 0)
#include <gdk/gdkkeysyms-compat.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_stdcontrols.h"
#include "iup_key.h"
#include "iup_image.h"
#include "iup_array.h"
#include "iup_tree.h"

#include "iup_drvinfo.h"
#include "iupgtk_drv.h"


/* TODO:
  Replace:
    cell-background-gdk
    foreground-gdk
  By:
    cell-background-rgba
    foreground-rgba
*/

/* IMPORTANT: 

  GtkTreeStore uses the "user_data" field of the GtkTreeIter 
  to store the node pointer that is position independent.
  So we use it as a reference to the node in the cache, just like in Motif and Windows.
  BUT if GTK change its implementation this must be changed also. See "gtk_tree_store.c".

  -----------------------------------------------------------------------------------
  ABOUT SELECTIONS:
     From the GTK documentation on GtkTreeSelection
  "Additionally, you cannot change the selection of a row on the model 
   that is not currently displayed by the view without expanding its parents first."
*/

enum                   /* comments show cell renderer associations */
{
  IUPGTK_NODE_IMAGE,   /* "pixbuf", "pixbuf-expander-closed" */
  IUPGTK_NODE_HAS_IMAGE,
  IUPGTK_NODE_IMAGE_EXPANDED, /* "pixbuf-expander-open" */
  IUPGTK_NODE_HAS_IMAGE_EXPANDED,
  IUPGTK_NODE_TITLE,   /* "text" */
  IUPGTK_NODE_KIND,    /* "is-expander" */
  IUPGTK_NODE_COLOR,   /* "foreground-gdk" */
  IUPGTK_NODE_FONT,    /* "font-desc" */
  IUPGTK_NODE_SELECTED,
  IUPGTK_NODE_CHECK,   /* "active" */
  IUPGTK_NODE_3STATE,  /* "inconsistent" */
  IUPGTK_NODE_TOGGLEVISIBLE, /* "visible" */
  IUPGTK_NODE_LAST_DATA  /* used as a count */
};

static void gtkTreeRebuildNodeCache(Ihandle* ih, int id, GtkTreeIter iterItem);

static void gtkTreeIterInit(Ihandle* ih, GtkTreeIter* iterItem, InodeHandle* node_handle)
{
  iterItem->stamp = ih->data->stamp;
  iterItem->user_data = node_handle;
  iterItem->user_data2 = NULL;
  iterItem->user_data3 = NULL;
}

static int gtkTreeFindNodeId(Ihandle* ih, GtkTreeIter* iterItem)
{
  return iupTreeFindNodeId(ih, iterItem->user_data);
}

static int gtkTreeTotalChildCount(Ihandle* ih, GtkTreeIter* iterItem)
{
  return iupdrvTreeTotalChildCount(ih, iterItem->user_data);
}

static int gtkTreeToggleGetCheck(Ihandle* ih, GtkTreeStore* store, GtkTreeIter iterItem)
{
  int isChecked;

  gtk_tree_model_get(GTK_TREE_MODEL(store), &iterItem, IUPGTK_NODE_3STATE, &isChecked, -1);
  if (isChecked && ih->data->show_toggle==2)
    return -1;

  gtk_tree_model_get(GTK_TREE_MODEL(store), &iterItem, IUPGTK_NODE_CHECK, &isChecked, -1);
  if (isChecked)
    return 1;
  else
    return 0;
}

/*****************************************************************************/
/* COPYING ITEMS (Branches and its children)                                 */
/*****************************************************************************/
/* Insert the copied item in a new location. Returns the new item. */
static void gtkTreeCopyItem(Ihandle* ih, GtkTreeModel* model, GtkTreeIter* iterItem, GtkTreeIter* iterParent, int position, GtkTreeIter *iterNewItem)
{
  GtkTreeStore* store = GTK_TREE_STORE(model);
  int kind;
  char* title;
  gboolean has_image, has_image_expanded;
  PangoFontDescription* font;
  GdkColor *color;
  GdkPixbuf* image, *image_expanded;

  gtk_tree_model_get(GTK_TREE_MODEL(store), iterItem, IUPGTK_NODE_IMAGE,      &image,
                                                      IUPGTK_NODE_HAS_IMAGE,      &has_image,
                                                      IUPGTK_NODE_IMAGE_EXPANDED,  &image_expanded,
                                                      IUPGTK_NODE_HAS_IMAGE_EXPANDED,  &has_image_expanded,
                                                      IUPGTK_NODE_TITLE,  &title,
                                                      IUPGTK_NODE_KIND,  &kind,
                                                      IUPGTK_NODE_COLOR, &color, 
                                                      IUPGTK_NODE_FONT, &font, 
                                                      -1);

  /* Add the new node */
  ih->data->node_count++;
  if (position == 2)
    gtk_tree_store_append(store, iterNewItem, iterParent);
  else if (position == 1)                                      /* copy as first child of expanded branch */
    gtk_tree_store_insert(store, iterNewItem, iterParent, 0);  /* iterParent is parent of the new item (firstchild of it) */
  else                                                                  /* copy as next brother of item or collapsed branch */
    gtk_tree_store_insert_after(store, iterNewItem, NULL, iterParent);  /* iterParent is sibling of the new item */

  gtk_tree_store_set(store, iterNewItem,  IUPGTK_NODE_IMAGE,      image,
                                          IUPGTK_NODE_HAS_IMAGE,  has_image,
                                          IUPGTK_NODE_IMAGE_EXPANDED,  image_expanded,
                                          IUPGTK_NODE_HAS_IMAGE_EXPANDED, has_image_expanded,
                                          IUPGTK_NODE_TITLE,  title,
                                          IUPGTK_NODE_KIND,  kind,
                                          IUPGTK_NODE_COLOR, color, 
                                          IUPGTK_NODE_FONT, font,
                                          IUPGTK_NODE_SELECTED, 0,
                                          IUPGTK_NODE_CHECK, 0,
                                          IUPGTK_NODE_3STATE, 0,
                                          IUPGTK_NODE_TOGGLEVISIBLE, 1,
                                          -1);
}

static void gtkTreeCopyChildren(Ihandle* ih, GtkTreeModel* model, GtkTreeIter *iterItemSrc, GtkTreeIter *iterItemDst)
{
  GtkTreeIter iterChildSrc;
  int hasItem = gtk_tree_model_iter_children(model, &iterChildSrc, iterItemSrc);  /* get the firstchild */
  while(hasItem)
  {
    GtkTreeIter iterNewItem;
    gtkTreeCopyItem(ih, model, &iterChildSrc, iterItemDst, 2, &iterNewItem);  /* append always */

    /* Recursively transfer all the items */
    gtkTreeCopyChildren(ih, model, &iterChildSrc, &iterNewItem);  

    /* Go to next sibling item */
    hasItem = gtk_tree_model_iter_next(model, &iterChildSrc);
  }
}

/* Copies all items in a branch to a new location. Returns the new branch node. */
static void gtkTreeCopyMoveNode(Ihandle* ih, GtkTreeModel* model, GtkTreeIter *iterItemSrc, GtkTreeIter *iterItemDst, GtkTreeIter* iterNewItem, int is_copy)
{
  int kind, position = 0; /* insert after iterItemDst */
  int id_new, count, id_src, id_dst;

  int old_count = ih->data->node_count;

  id_src = gtkTreeFindNodeId(ih, iterItemSrc);
  id_dst = gtkTreeFindNodeId(ih, iterItemDst);
  id_new = id_dst+1; /* contains the position for a copy operation */

  gtk_tree_model_get(model, iterItemDst, IUPGTK_NODE_KIND, &kind, -1);

  if (kind == ITREE_BRANCH)
  {
    GtkTreePath* path = gtk_tree_model_get_path(model, iterItemDst);
    if (gtk_tree_view_row_expanded(GTK_TREE_VIEW(ih->handle), path))
      position = 1;  /* insert as first child of iterItemDst */
    else
    {
      int child_count = gtkTreeTotalChildCount(ih, iterItemDst);
      id_new += child_count;
    }
    gtk_tree_path_free(path);
  }

  /* move to the same place does nothing */
  if (!is_copy && id_new == id_src)
  {
    iterNewItem->stamp = 0;
    return;
  }

  gtkTreeCopyItem(ih, model, iterItemSrc, iterItemDst, position, iterNewItem);  

  gtkTreeCopyChildren(ih, model, iterItemSrc, iterNewItem);

  count = ih->data->node_count - old_count;
  iupTreeCopyMoveCache(ih, id_src, id_new, count, is_copy);

  if (!is_copy)
  {
    /* Deleting the node of its old position */
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
    gtk_tree_store_remove(GTK_TREE_STORE(model), iterItemSrc);
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);

    /* restore count, because we remove src */
    ih->data->node_count = old_count;

    /* compensate position for a move */
    if (id_new > id_src)
      id_new -= count;
  }

  gtkTreeRebuildNodeCache(ih, id_new, *iterNewItem);
}

/*****************************************************************************/
/* FINDING ITEMS                                                             */
/*****************************************************************************/

static int gtkTreeIsNodeSelected(GtkTreeModel* model, GtkTreeIter *iterItem)
{
  gboolean selected = 0;
  gtk_tree_model_get(model, iterItem, IUPGTK_NODE_SELECTED, &selected, -1);
  return selected;
}

static void gtkTreeSelectNodeRaw(GtkTreeModel* model, GtkTreeIter *iterItem, int select)
{
  /* Cannot change the selection of a row on the model that is not currently displayed. 
     So we store the selection state here. And update the actual state when the node becames visible. */
  gtk_tree_store_set(GTK_TREE_STORE(model), iterItem, IUPGTK_NODE_SELECTED, select, -1);
}

static void gtkTreeSelectNode(GtkTreeModel* model, GtkTreeSelection* selection, GtkTreeIter *iterItem, int select)
{
  if (select == -1)
    select = !gtkTreeIsNodeSelected(model, iterItem); /* toggle */

  gtkTreeSelectNodeRaw(model, iterItem, select);

  if (select)
    gtk_tree_selection_select_iter(selection, iterItem);
  else
    gtk_tree_selection_unselect_iter(selection, iterItem);
}

static void gtkTreeSelectAll(Ihandle* ih, GtkTreeModel* model, GtkTreeSelection* selection, int selected)
{
  int i;
  GtkTreeIter iterItem;

  for (i = 0; i < ih->data->node_count; i++)
  {
    gtkTreeIterInit(ih, &iterItem, ih->data->node_cache[i].node_handle);
    gtkTreeSelectNodeRaw(model, &iterItem, selected);
  }

  if (selected)
    gtk_tree_selection_select_all(selection);
  else
    gtk_tree_selection_unselect_all(selection);
}

static void gtkTreeInvertAllNodeMarking(Ihandle* ih, GtkTreeModel* model, GtkTreeSelection* selection)
{
  int i;
  GtkTreeIter iterItem;

  for (i = 0; i < ih->data->node_count; i++)
  {
    gtkTreeIterInit(ih, &iterItem, ih->data->node_cache[i].node_handle);
    gtkTreeSelectNode(model, selection, &iterItem, -1);
  }
}

static void gtkTreeSelectRange(Ihandle* ih, GtkTreeModel* model, GtkTreeSelection* selection, GtkTreeIter *iterItem1, GtkTreeIter *iterItem2, int clear)
{
  int i;
  int id1 = gtkTreeFindNodeId(ih, iterItem1);
  int id2 = gtkTreeFindNodeId(ih, iterItem2);
  GtkTreeIter iterItem;

  if (id1 > id2)
  {
    int tmp = id1;
    id1 = id2;
    id2 = tmp;
  }

  for (i = 0; i < ih->data->node_count; i++)
  {
    gtkTreeIterInit(ih, &iterItem, ih->data->node_cache[i].node_handle);
    if (i < id1 || i > id2)
    {
      if (clear)
        gtkTreeSelectNode(model, selection, &iterItem, 0);
    }
    else
      gtkTreeSelectNode(model, selection, &iterItem, 1);
  }
}

static int gtkTreeIsNodeVisible(Ihandle* ih, GtkTreeModel* model, InodeHandle* node_handle, InodeHandle* *nodeLastParent)
{
  GtkTreeIter iterItem, iterParent;
  GtkTreePath* path;
  int is_visible;

  gtkTreeIterInit(ih, &iterItem, node_handle);

  if (!gtk_tree_model_iter_parent(model, &iterParent, &iterItem) ||
      iterParent.user_data == *nodeLastParent)
    return 1;

  path = gtk_tree_model_get_path(model, &iterParent);
  is_visible = gtk_tree_view_row_expanded(GTK_TREE_VIEW(ih->handle), path);
  gtk_tree_path_free(path);

  if (!is_visible)
    return 0;

  /* save last parent */
  *nodeLastParent = iterParent.user_data;
  return 1;
}

static void gtkTreeGetLastVisibleNode(Ihandle* ih, GtkTreeModel* model, GtkTreeIter *iterItem)
{
  int i;
  InodeHandle* nodeLastParent = NULL;

  for (i = ih->data->node_count-1; i >= 0; i--)
  {
    if (gtkTreeIsNodeVisible(ih, model, ih->data->node_cache[i].node_handle, &nodeLastParent))
    {
      gtkTreeIterInit(ih, iterItem, ih->data->node_cache[i].node_handle);
      return;
    }
  }

  if (ih->data->node_count)
    gtkTreeIterInit(ih, iterItem, ih->data->node_cache[0].node_handle);  /* root is always visible */
  else
    gtkTreeIterInit(ih, iterItem, NULL);  /* invalid iter */
}

static void gtkTreeGetNextVisibleNode(Ihandle* ih, GtkTreeModel* model, GtkTreeIter *iterItem, int count)
{
  int i, id;
  InodeHandle* nodeLastParent = NULL;

  id = gtkTreeFindNodeId(ih, iterItem);
  id += count;

  for (i = id; i < ih->data->node_count; i++)
  {
    if (gtkTreeIsNodeVisible(ih, model, ih->data->node_cache[i].node_handle, &nodeLastParent))
    {
      gtkTreeIterInit(ih, iterItem, ih->data->node_cache[i].node_handle);
      return;
    }
  }

  if (ih->data->node_count)
    gtkTreeIterInit(ih, iterItem, ih->data->node_cache[0].node_handle);  /* root is always visible */
  else
    gtkTreeIterInit(ih, iterItem, NULL);  /* invalid iter */
}

static void gtkTreeGetPreviousVisibleNode(Ihandle* ih, GtkTreeModel* model, GtkTreeIter *iterItem, int count)
{
  int i, id;
  InodeHandle* nodeLastParent = NULL;

  id = gtkTreeFindNodeId(ih, iterItem);
  id -= count;

  for (i = id; i >= 0; i--)
  {
    if (gtkTreeIsNodeVisible(ih, model, ih->data->node_cache[i].node_handle, &nodeLastParent))
    {
      gtkTreeIterInit(ih, iterItem, ih->data->node_cache[i].node_handle);
      return;
    }
  }

  gtkTreeGetLastVisibleNode(ih, model, iterItem);
}

static void gtkTreeCallNodeRemovedRec(Ihandle* ih, GtkTreeModel* model, GtkTreeIter *iterItem, IFns cb, int *id)
{
  GtkTreeIter iterChild;
  int hasItem;
  int node_id = *id;

  /* Check whether we have child items */
  /* remove from children first */
  hasItem = gtk_tree_model_iter_children(model, &iterChild, iterItem);  /* get the firstchild */
  while(hasItem)
  {
    (*id)++;

    /* go recursive to children */
    gtkTreeCallNodeRemovedRec(ih, model, &iterChild, cb, id);

    /* Go to next sibling item */
    hasItem = gtk_tree_model_iter_next(model, &iterChild);
  }

  /* actually do it for the node, in GTK this is just the callback */
  cb(ih, (char*)ih->data->node_cache[node_id].userdata);

  /* update count */
  ih->data->node_count--;
}

static void gtkTreeCallNodeRemoved(Ihandle* ih, GtkTreeModel* model, GtkTreeIter *iterItem)
{
  int old_count = ih->data->node_count;
  int id = gtkTreeFindNodeId(ih, iterItem);
  int start_id = id;

  IFns cb = (IFns)IupGetCallback(ih, "NODEREMOVED_CB");
  if (cb) 
    gtkTreeCallNodeRemovedRec(ih, model, iterItem, cb, &id);
  else
  {
    int removed_count = gtkTreeTotalChildCount(ih, iterItem)+1;
    ih->data->node_count -= removed_count;
  }

  iupTreeDelFromCache(ih, start_id, old_count-ih->data->node_count);
}

static void gtkTreeCallNodeRemovedAll(Ihandle* ih)
{
  IFns cb = (IFns)IupGetCallback(ih, "NODEREMOVED_CB");
  int old_count = ih->data->node_count;

  if (cb)
  {
    int i;

    for (i = 0; i < ih->data->node_count; i++)
    {
      cb(ih, (char*)ih->data->node_cache[i].userdata);
    }
  }

  ih->data->node_count = 0;

  iupTreeDelFromCache(ih, 0, old_count);
}

static gboolean gtkTreeFindNodeFromString(Ihandle* ih, const char* value, GtkTreeIter *iterItem)
{
  InodeHandle* node_handle = iupTreeGetNodeFromString(ih, value);
  if (!node_handle)
    return FALSE;

  gtkTreeIterInit(ih, iterItem, node_handle);
  return TRUE;
}

static gboolean gtkTreeFindNode(Ihandle* ih, int id, GtkTreeIter *iterItem)
{
  InodeHandle* node_handle = iupTreeGetNode(ih, id);
  if (!node_handle)
    return FALSE;

  gtkTreeIterInit(ih, iterItem, node_handle);
  return TRUE;
}

static Iarray* gtkTreeGetSelectedArrayId(Ihandle* ih)
{
  Iarray* selarray = iupArrayCreate(1, sizeof(int));
  int i;
  GtkTreeIter iterItem;
  GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));

  for (i = 0; i < ih->data->node_count; i++)
  {
    gtkTreeIterInit(ih, &iterItem, ih->data->node_cache[i].node_handle);
    if (gtkTreeIsNodeSelected(model, &iterItem))
    {
      int* id_hitem = (int*)iupArrayInc(selarray);
      int j = iupArrayCount(selarray);
      id_hitem[j-1] = i;
    }
  }

  return selarray;
}

static void gtkTreeSetFocus(Ihandle* ih, GtkTreePath* pathFocus, GtkTreeIter* iterItemFocus, gboolean edit)
{
  int old_select = 0;
  GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));

  /* in a multiselection set_cursor will unselect all other nodes
     so must save and restore selection */
  Iarray* markedArray = NULL;
  if (ih->data->mark_mode==ITREE_MARK_MULTIPLE)
    markedArray = gtkTreeGetSelectedArrayId(ih);

  if (gtkTreeIsNodeSelected(model, iterItemFocus))
    old_select = 1;

  iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
  gtk_tree_view_set_cursor(GTK_TREE_VIEW(ih->handle), pathFocus, NULL, edit);
  iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);

  if (!old_select)
  {
    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ih->handle));
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
    gtkTreeSelectNode(model, selection, iterItemFocus, 0);
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
  }

  if (markedArray)
  {
    int count = iupArrayCount(markedArray);
    if (count > 0)
    {
      GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ih->handle));
      int i, *id_hitem = (int*)iupArrayGetData(markedArray);
      GtkTreeIter iterItem;

      iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
      for (i=0; i<count; i++)
      {
        gtkTreeIterInit(ih, &iterItem, ih->data->node_cache[id_hitem[i]].node_handle);
        gtkTreeSelectNode(model, selection, &iterItem, 1);
      }
      iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
    }

    iupArrayDestroy(markedArray);
  }
}


/*****************************************************************************/
/* MANIPULATING IMAGES                                                       */
/*****************************************************************************/
static void gtkTreeUpdateImages(Ihandle* ih, int mode)
{
  GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));
  GtkTreeIter iterItem;
  int i, kind;

  for (i=0; i<ih->data->node_count; i++)
  {
    gtkTreeIterInit(ih, &iterItem, ih->data->node_cache[i].node_handle);

    gtk_tree_model_get(model, &iterItem, IUPGTK_NODE_KIND, &kind, -1);

    if (kind == ITREE_BRANCH)
    {
      if (mode == ITREE_UPDATEIMAGE_EXPANDED)
      {
        gboolean has_image_expanded = FALSE;
        gtk_tree_model_get(model, &iterItem, IUPGTK_NODE_HAS_IMAGE_EXPANDED, &has_image_expanded, -1);
        if (!has_image_expanded)
          gtk_tree_store_set(GTK_TREE_STORE(model), &iterItem, IUPGTK_NODE_IMAGE_EXPANDED, ih->data->def_image_expanded, -1);
      }
      else if(mode == ITREE_UPDATEIMAGE_COLLAPSED)
      {
        gboolean has_image = FALSE;
        gtk_tree_model_get(model, &iterItem, IUPGTK_NODE_HAS_IMAGE, &has_image, -1);
        if (!has_image)
          gtk_tree_store_set(GTK_TREE_STORE(model), &iterItem, IUPGTK_NODE_IMAGE, ih->data->def_image_collapsed, -1);
      }
    }
    else 
    {
      if (mode == ITREE_UPDATEIMAGE_LEAF)
      {
        gboolean has_image = FALSE;
        gtk_tree_model_get(model, &iterItem, IUPGTK_NODE_HAS_IMAGE, &has_image, -1);
        if (!has_image)
          gtk_tree_store_set(GTK_TREE_STORE(model), &iterItem, IUPGTK_NODE_IMAGE, ih->data->def_image_leaf, -1);
      }
    }
  }
}

static void gtkTreeExpandItem(Ihandle* ih, GtkTreePath* path, int expand)
{
  if (expand == -1)
    expand = !gtk_tree_view_row_expanded(GTK_TREE_VIEW(ih->handle), path); /* toggle */

  if (expand)
    gtk_tree_view_expand_row(GTK_TREE_VIEW(ih->handle), path, FALSE);
  else
    gtk_tree_view_collapse_row(GTK_TREE_VIEW(ih->handle), path);
}

int iupgtkGetColor(const char* value, GdkColor *color)
{
  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
  {
    iupgdkColorSetRGB(color, r, g, b);
    return 1;
  }
  return 0;
}

/*****************************************************************************/
/* ADDING ITEMS                                                              */
/*****************************************************************************/
void iupdrvTreeAddNode(Ihandle* ih, int id, int kind, const char* title, int add)
{
  GtkTreeStore* store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle)));
  GtkTreeIter iterPrev, iterNewItem, iterParent;
  GtkTreePath* path;
  GdkColor color = {0L,0,0,0};
  int kindPrev = -1;

  /* the previous node is not necessary only
     if adding the root in an empty tree or before the root. */
  if (!gtkTreeFindNode(ih, id, &iterPrev) && id!=-1)
      return;

  if (id >= 0)
    gtk_tree_model_get(GTK_TREE_MODEL(store), &iterPrev, IUPGTK_NODE_KIND, &kindPrev, -1);

  if (kindPrev != -1)
  {
    /* Add the new node */
    if (kindPrev == ITREE_BRANCH && add)
      gtk_tree_store_insert(store, &iterNewItem, &iterPrev, 0);  /* iterPrev is parent of the new item (firstchild of it) */
    else
      gtk_tree_store_insert_after(store, &iterNewItem, NULL, &iterPrev);  /* iterPrev is sibling of the new item */
    iupTreeAddToCache(ih, add, kindPrev, iterPrev.user_data, iterNewItem.user_data);
  }
  else
  {
    if (id == -1)
      gtk_tree_store_prepend(store, &iterNewItem, NULL);  /* before the root node */
    else
      gtk_tree_store_append(store, &iterNewItem, NULL);  /* root node in an empty tree */
    iupTreeAddToCache(ih, 0, 0, NULL, iterNewItem.user_data);

    /* store the stamp for the tree */
    ih->data->stamp = iterNewItem.stamp;
  }

  iupgtkGetColor(iupAttribGetStr(ih, "FGCOLOR"), &color);

  if (!title)
    title = "";

  /* set the attributes of the new node */
  gtk_tree_store_set(store, &iterNewItem, IUPGTK_NODE_HAS_IMAGE, FALSE,
                                          IUPGTK_NODE_HAS_IMAGE_EXPANDED, FALSE,
                                          IUPGTK_NODE_TITLE, iupgtkStrConvertToSystem(title),
                                          IUPGTK_NODE_KIND, kind,
                                          IUPGTK_NODE_COLOR, &color, 
                                          IUPGTK_NODE_SELECTED, 0,
                                          IUPGTK_NODE_CHECK, 0,
                                          IUPGTK_NODE_3STATE, 0,
                                          IUPGTK_NODE_TOGGLEVISIBLE, 1,
                                          -1);

  if (kind == ITREE_LEAF)
    gtk_tree_store_set(store, &iterNewItem, IUPGTK_NODE_IMAGE, ih->data->def_image_leaf, -1);
  else
    gtk_tree_store_set(store, &iterNewItem, IUPGTK_NODE_IMAGE, ih->data->def_image_collapsed,
                                            IUPGTK_NODE_IMAGE_EXPANDED, ih->data->def_image_expanded, -1);

  if (kindPrev != -1)
  {
    if (kindPrev == ITREE_BRANCH && add)
      iterParent = iterPrev;
    else if (!gtk_tree_model_iter_parent(GTK_TREE_MODEL(store), &iterParent, &iterNewItem))
      return;

    /* If this is the first child of the parent, then handle the ADDEXPANDED attribute */
    if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), &iterParent) == 1)
    {
      path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iterParent);
      iupAttribSet(ih, "_IUPTREE_IGNORE_BRANCH_CB", "1");
      if (ih->data->add_expanded)
        gtk_tree_view_expand_row(GTK_TREE_VIEW(ih->handle), path, FALSE);
      else
        gtk_tree_view_collapse_row(GTK_TREE_VIEW(ih->handle), path);
      iupAttribSet(ih, "_IUPTREE_IGNORE_BRANCH_CB", NULL);
      gtk_tree_path_free(path);
    }
  }
  else
  {
    if (ih->data->node_count == 1)
    {
      /* MarkStart node */
      iupAttribSet(ih, "_IUPTREE_MARKSTART_NODE", (char*)iterNewItem.user_data);

      iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");

      /* Set the default VALUE (focus) */
      path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iterNewItem);
      gtk_tree_view_set_cursor(GTK_TREE_VIEW(ih->handle), path, NULL, FALSE);
      gtk_tree_path_free(path);

      /* when single selection when focus is set, node is also selected */
      /* set_cursor will also select the node, so unselect it here if not single */
      if (ih->data->mark_mode != ITREE_MARK_SINGLE)
        gtkTreeSelectNode(GTK_TREE_MODEL(store), gtk_tree_view_get_selection(GTK_TREE_VIEW(ih->handle)), &iterNewItem, 0);

      iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
    }
  }
}


/*****************************************************************************/
/* AUXILIAR FUNCTIONS                                                        */
/*****************************************************************************/


static void gtkTreeChildRebuildCacheRec(Ihandle* ih, GtkTreeModel *model, GtkTreeIter *iterItem, int *id)
{
  GtkTreeIter iterChild;
  int hasItem = gtk_tree_model_iter_children(model, &iterChild, iterItem);  /* get the firstchild */
  while(hasItem)
  {
    (*id)++;
    ih->data->node_cache[*id].node_handle = iterChild.user_data;

    /* go recursive to children */
    gtkTreeChildRebuildCacheRec(ih, model, &iterChild, id);

    /* Go to next sibling item */
    hasItem = gtk_tree_model_iter_next(model, &iterChild);
  }
}

static void gtkTreeRebuildNodeCache(Ihandle* ih, int id, GtkTreeIter iterItem)
{
  /* preserve cache user_data */
  GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));
  ih->data->node_cache[id].node_handle = iterItem.user_data;
  gtkTreeChildRebuildCacheRec(ih, model, &iterItem, &id);
}

static void gtkTreeChildCountRec(GtkTreeModel *model, GtkTreeIter *iterItem, int *count)
{
  GtkTreeIter iterChild = {0,0,0,0};
  int hasItem = gtk_tree_model_iter_children(model, &iterChild, iterItem);  /* get the firstchild */
  while(hasItem)
  {
    (*count)++;

    /* go recursive to children */
    gtkTreeChildCountRec(model, &iterChild, count);

    /* Go to next sibling item */
    hasItem = gtk_tree_model_iter_next(model, &iterChild);
  }
}

int iupdrvTreeTotalChildCount(Ihandle* ih, InodeHandle* node_handle)
{
  int count = 0;
  GtkTreeIter iterItem;
  GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));
  gtkTreeIterInit(ih, &iterItem, node_handle);
  gtkTreeChildCountRec(model, &iterItem, &count);
  return count;
}

InodeHandle* iupdrvTreeGetFocusNode(Ihandle* ih)
{
  GtkTreePath* path = NULL;
  gtk_tree_view_get_cursor(GTK_TREE_VIEW(ih->handle), &path, NULL);
  if (path)
  {
    GtkTreeIter iterItem;
    GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));
    gtk_tree_model_get_iter(model, &iterItem, path);
    gtk_tree_path_free(path);
    return iterItem.user_data;
  }

  return NULL;
}

static void gtkTreeOpenCloseEvent(Ihandle* ih)
{
  GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));
  GtkTreeIter iterItem;
  GtkTreePath* path;
  int kind;

  if (!gtkTreeFindNode(ih, IUP_INVALID_ID, &iterItem))  /* focus node */
    return;

  path = gtk_tree_model_get_path(model, &iterItem);
  if (path)
  {
    gtk_tree_model_get(model, &iterItem, IUPGTK_NODE_KIND, &kind, -1);

    if (kind == ITREE_LEAF)  /* leafs */
      gtk_tree_view_row_activated(GTK_TREE_VIEW(ih->handle), path, (GtkTreeViewColumn*)iupAttribGet(ih, "_IUPGTK_COLUMN"));     
    else  /* branches */
      gtkTreeExpandItem(ih, path, -1); /* toggle */

    gtk_tree_path_free(path);
  }
}

typedef struct _gtkTreeSelectMinMax
{
  Ihandle* ih;
  int id1, id2;
} gtkTreeSelectMinMax;

static gboolean gtkTreeSelected_Foreach_Func(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iterItem, gtkTreeSelectMinMax *minmax)
{
  int id = gtkTreeFindNodeId(minmax->ih, iterItem);
  if (id < minmax->id1)
    minmax->id1 = id;
  if (id > minmax->id2)
    minmax->id2 = id;

  (void)model;
  (void)path;
  return FALSE; /* do not stop walking the store, call us with next row */
}

/*****************************************************************************/
/* CALLBACKS                                                                 */
/*****************************************************************************/
static void gtkTreeCallMultiSelectionCb(Ihandle* ih)
{
  /* called when a continuous range of several items are selected at once
     using the Shift key pressed, or dragging the mouse. */
  GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));
  GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ih->handle));
  GtkTreeIter iterItem;
  int i = 0, countItems;
  gtkTreeSelectMinMax minmax;

  minmax.ih = ih;
  minmax.id1 = ih->data->node_count;
  minmax.id2 = -1;

  gtk_tree_selection_selected_foreach(selection, (GtkTreeSelectionForeachFunc)gtkTreeSelected_Foreach_Func, &minmax);
  if (minmax.id2 == -1)
    return;

  /* interactive selection of several nodes will NOT select hidden nodes,
      so make sure that their selection state is stored. */
  for(i = minmax.id1; i <= minmax.id2; i++)
  {
    gtkTreeIterInit(ih, &iterItem, ih->data->node_cache[i].node_handle);
    gtkTreeSelectNodeRaw(model, &iterItem, 1);
  }

  /* if last selected item is a branch, then select its children */
  iupTreeSelectLastCollapsedBranch(ih, &(minmax.id2));

  countItems = minmax.id2-minmax.id1+1;

  if (countItems > 0)
  {
    IFnIi cbMulti = (IFnIi)IupGetCallback(ih, "MULTISELECTION_CB");
    IFnii cbSelec = (IFnii)IupGetCallback(ih, "SELECTION_CB");
    if (cbMulti)
    {
      int* id_rowItem = (int*)malloc(sizeof(int) * countItems);

      for(i = 0; i < countItems; i++)
        id_rowItem[i] = minmax.id1+i;

      cbMulti(ih, id_rowItem, countItems);

      free(id_rowItem);
    }
    else if (cbSelec)
    {
      for (i=0; i<countItems; i++)
        cbSelec(ih, minmax.id1+i, 1);
    }
  }
}


/*****************************************************************************/
/* GET AND SET ATTRIBUTES                                                    */
/*****************************************************************************/


static char* gtkTreeGetIndentationAttrib(Ihandle* ih)
{
#if GTK_CHECK_VERSION(2, 12, 0)
  return iupStrReturnInt(gtk_tree_view_get_level_indentation(GTK_TREE_VIEW(ih->handle)));
#else
  return NULL;
#endif
}

static int gtkTreeSetIndentationAttrib(Ihandle* ih, const char* value)
{
#if GTK_CHECK_VERSION(2, 12, 0)
  int indent;
  if (iupStrToInt(value, &indent))
    gtk_tree_view_set_level_indentation(GTK_TREE_VIEW(ih->handle), indent);
#endif
  return 0;
}

static int gtkTreeSetTopItemAttrib(Ihandle* ih, const char* value)
{
  GtkTreeStore* store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle)));
  GtkTreeIter iterItem;
  GtkTreePath* path;
  int kind;

  if (!gtkTreeFindNodeFromString(ih, value, &iterItem))
    return 0;

  path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iterItem);

  gtk_tree_model_get(GTK_TREE_MODEL(store), &iterItem, IUPGTK_NODE_KIND, &kind, -1);
  if (kind == ITREE_LEAF)
    gtk_tree_view_expand_to_path(GTK_TREE_VIEW(ih->handle), path);
  else
  {
    int expanded = gtk_tree_view_row_expanded(GTK_TREE_VIEW(ih->handle), path);
    gtk_tree_view_expand_to_path(GTK_TREE_VIEW(ih->handle), path);
    if (!expanded) gtk_tree_view_collapse_row(GTK_TREE_VIEW(ih->handle), path);
  }

  gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(ih->handle), path, NULL, TRUE, 0, 0);  /* scroll to visible, top */

  gtk_tree_path_free(path);
 
  return 0;
}

static int gtkTreeSetSpacingAttrib(Ihandle* ih, const char* value)
{
  iupStrToInt(value, &ih->data->spacing);

  if (ih->handle)
  {
    GtkCellRenderer *renderer_chk = (GtkCellRenderer*)iupAttribGet(ih, "_IUPGTK_RENDERER_CHECK");
    GtkCellRenderer *renderer_img = (GtkCellRenderer*)iupAttribGet(ih, "_IUPGTK_RENDERER_IMG");
    GtkCellRenderer *renderer_txt = (GtkCellRenderer*)iupAttribGet(ih, "_IUPGTK_RENDERER_TEXT");
    if (renderer_chk) g_object_set(G_OBJECT(renderer_chk), "ypad", ih->data->spacing, NULL);
    g_object_set(G_OBJECT(renderer_img), "ypad", ih->data->spacing, NULL);
    g_object_set(G_OBJECT(renderer_txt), "ypad", ih->data->spacing, NULL);
    return 0;
  }
  else
    return 1; /* store until not mapped, when mapped will be set again */
}

static int gtkTreeSetExpandAllAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    gtk_tree_view_expand_all(GTK_TREE_VIEW(ih->handle));
  else
    gtk_tree_view_collapse_all(GTK_TREE_VIEW(ih->handle));

  return 0;
}

static char* gtkTreeGetDepthAttrib(Ihandle* ih, int id)
{
  GtkTreeStore* store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle)));
  GtkTreeIter iterItem;

  if (!gtkTreeFindNode(ih, id, &iterItem))
    return NULL;

  return iupStrReturnInt(gtk_tree_store_iter_depth(store, &iterItem));
}

static int gtkTreeSetMoveNodeAttrib(Ihandle* ih, int id, const char* value)
{
  GtkTreeModel* model;
  GtkTreeIter iterItemSrc, iterItemDst, iterNewItem;
  GtkTreeIter iterParent, iterNextParent;

  if (!ih->handle)  /* do not do the action before map */
    return 0;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));
  if (!gtkTreeFindNode(ih, id, &iterItemSrc))
    return 0;

  if (!gtkTreeFindNodeFromString(ih, value, &iterItemDst))
    return 0;

  /* If Drag item is an ancestor of Drop item then return */
  iterParent = iterItemDst;
  while(gtk_tree_model_iter_parent(model, &iterNextParent, &iterParent))
  {
    if (iterNextParent.user_data == iterItemSrc.user_data)
      return 0;
    iterParent = iterNextParent;
  }

  /* Move the node and its children to the new position */
  gtkTreeCopyMoveNode(ih, model, &iterItemSrc, &iterItemDst, &iterNewItem, 0);

  return 0;
}

static int gtkTreeSetCopyNodeAttrib(Ihandle* ih, int id, const char* value)
{
  GtkTreeModel* model;
  GtkTreeIter iterItemSrc, iterItemDst, iterNewItem;
  GtkTreeIter iterParent, iterNextParent;

  if (!ih->handle)  /* do not do the action before map */
    return 0;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));
  if (!gtkTreeFindNode(ih, id, &iterItemSrc))
    return 0;

  if (!gtkTreeFindNodeFromString(ih, value, &iterItemDst))
    return 0;

  /* If Drag item is an ancestor of Drop item then return */
  iterParent = iterItemDst;
  while(gtk_tree_model_iter_parent(model, &iterNextParent, &iterParent))
  {
    if (iterNextParent.user_data == iterItemSrc.user_data)
      return 0;
    iterParent = iterNextParent;
  }

  /* Copy the node and its children to the new position */
  gtkTreeCopyMoveNode(ih, model, &iterItemSrc, &iterItemDst, &iterNewItem, 1);

  return 0;
}

static char* gtkTreeGetColorAttrib(Ihandle* ih, int id)
{
  GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));
  GtkTreeIter iterItem;
  GdkColor *color;

  if (!gtkTreeFindNode(ih, id, &iterItem))
    return NULL;

  gtk_tree_model_get(model, &iterItem, IUPGTK_NODE_COLOR, &color, -1);
  if (!color)
    return NULL;

  return iupStrReturnStrf("%d %d %d", iupCOLOR16TO8(color->red),
                                   iupCOLOR16TO8(color->green),
                                   iupCOLOR16TO8(color->blue));
}

static int gtkTreeSetColorAttrib(Ihandle* ih, int id, const char* value)
{
  GtkTreeStore* store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle)));
  GtkTreeIter iterItem;
  GdkColor color;
  unsigned char r, g, b;

  if (!gtkTreeFindNode(ih, id, &iterItem))
    return 0;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupgdkColorSetRGB(&color, r, g, b);
  gtk_tree_store_set(store, &iterItem, IUPGTK_NODE_COLOR, &color, -1);

  return 0;
}

static char* gtkTreeGetParentAttrib(Ihandle* ih, int id)
{
  GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));
  GtkTreeIter iterItem;
  GtkTreeIter iterParent;

  if (!gtkTreeFindNode(ih, id, &iterItem))
    return NULL;

  if (!gtk_tree_model_iter_parent(model, &iterParent, &iterItem))
    return NULL;

  return iupStrReturnInt(gtkTreeFindNodeId(ih, &iterParent));
}

static char* gtkTreeGetNextAttrib(Ihandle* ih, int id)
{
  GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));
  GtkTreeIter iterItem;

  if (!gtkTreeFindNode(ih, id, &iterItem))
    return NULL;

  if (!gtk_tree_model_iter_next(model, &iterItem))
    return NULL;

  return iupStrReturnInt(gtkTreeFindNodeId(ih, &iterItem));
}

static char* gtkTreeGetLastAttrib(Ihandle* ih, int id)
{
  GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));
  GtkTreeIter iterItem;
  GtkTreeIter iterItemLast;
  int found = 1;

  if (!gtkTreeFindNode(ih, id, &iterItem))
    return NULL;

  while (found)
  {
    iterItemLast = iterItem;
    found = gtk_tree_model_iter_next(model, &iterItem);
  }

  return iupStrReturnInt(gtkTreeFindNodeId(ih, &iterItemLast));
}

static char* gtkTreeGetPreviousAttrib(Ihandle* ih, int id)
{
  GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));
  GtkTreeIter iterItem;

  if (!gtkTreeFindNode(ih, id, &iterItem))
    return NULL;

#if GTK_CHECK_VERSION(3, 0, 0)
  if (!gtk_tree_model_iter_previous(model, &iterItem))
    return NULL;
#else
  {
    GtkTreeIter iterItemPrevious;
    GtkTreeIter iterItemNext;
    GtkTreeIter iterParent;
    int found = 1;

    /* get first */
    if (gtk_tree_model_iter_parent(model, &iterParent, &iterItem))
      gtk_tree_model_iter_children(model, &iterItemPrevious, &iterParent);
    else
      gtk_tree_model_get_iter_first(model, &iterItemPrevious);

    if (iterItemPrevious.user_data == iterItem.user_data)
      return NULL;

    do
    {
      iterItemNext = iterItemPrevious;
      found = gtk_tree_model_iter_next(model, &iterItemNext);
      if (found && iterItemNext.user_data == iterItem.user_data)
        break;
    } while (found);

    iterItem = iterItemPrevious;
  }
#endif

  return iupStrReturnInt(gtkTreeFindNodeId(ih, &iterItem));
}


static char* gtkTreeGetFirstAttrib(Ihandle* ih, int id)
{
  GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));
  GtkTreeIter iterItem;
  GtkTreeIter iterItemFirst;
  GtkTreeIter iterParent;

  if (!gtkTreeFindNode(ih, id, &iterItem))
    return NULL;

  if (!gtk_tree_model_iter_parent(model, &iterParent, &iterItem))
    return "0";

  if (!gtk_tree_model_iter_children(model, &iterItemFirst, &iterParent))
    return NULL;

  return iupStrReturnInt(gtkTreeFindNodeId(ih, &iterItemFirst));
}

static char* gtkTreeGetChildCountAttrib(Ihandle* ih, int id)
{
  GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));
  GtkTreeIter iterItem;

  if (!gtkTreeFindNode(ih, id, &iterItem))
    return NULL;

  return iupStrReturnInt(gtk_tree_model_iter_n_children(model, &iterItem));
}

static char* gtkTreeGetRootCountAttrib(Ihandle* ih)
{
  GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));
  return iupStrReturnInt(gtk_tree_model_iter_n_children(model, NULL));
}

static char* gtkTreeGetKindAttrib(Ihandle* ih, int id)
{
  GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));
  GtkTreeIter iterItem;
  int kind;

  if (!gtkTreeFindNode(ih, id, &iterItem))
    return NULL;

  gtk_tree_model_get(model, &iterItem, IUPGTK_NODE_KIND, &kind, -1);

  if (kind == ITREE_BRANCH)
    return "BRANCH";
  else
    return "LEAF";
}

static char* gtkTreeGetStateAttrib(Ihandle* ih, int id)
{
  GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));
  GtkTreeIter iterItem;
  int kind;

  if (!gtkTreeFindNode(ih, id, &iterItem))
    return NULL;

  gtk_tree_model_get(model, &iterItem, IUPGTK_NODE_KIND, &kind, -1);

  if (kind == ITREE_BRANCH)
  {
    GtkTreePath* path = gtk_tree_model_get_path(model, &iterItem);
    int expanded = gtk_tree_view_row_expanded(GTK_TREE_VIEW(ih->handle), path);
    gtk_tree_path_free(path);

    if (expanded)
      return "EXPANDED";
    else
      return "COLLAPSED";
  }

  return NULL;
}

static int gtkTreeSetStateAttrib(Ihandle* ih, int id, const char* value)
{
  GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));
  GtkTreeIter iterItem;
  int kind;

  if (!gtkTreeFindNode(ih, id, &iterItem))
    return 0;

  gtk_tree_model_get(model, &iterItem, IUPGTK_NODE_KIND, &kind, -1);
  if (kind == ITREE_BRANCH)
  {
    GtkTreePath* path = gtk_tree_model_get_path(model, &iterItem);
    iupAttribSet(ih, "_IUPTREE_IGNORE_BRANCH_CB", "1");
    gtkTreeExpandItem(ih, path, iupStrEqualNoCase(value, "EXPANDED"));
    iupAttribSet(ih, "_IUPTREE_IGNORE_BRANCH_CB", NULL);
    gtk_tree_path_free(path);
  }

  return 0;
}

static char* gtkTreeGetTitleAttrib(Ihandle* ih, int id)
{
  char* title;
  GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));
  GtkTreeIter iterItem;
  if (!gtkTreeFindNode(ih, id, &iterItem))
    return NULL;
  gtk_tree_model_get(model, &iterItem, IUPGTK_NODE_TITLE, &title, -1);
  return iupStrReturnStr(iupgtkStrConvertFromSystem(title));
}

static int gtkTreeSetTitleAttrib(Ihandle* ih, int id, const char* value)
{
  GtkTreeStore* store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle)));
  GtkTreeIter iterItem;
  if (!gtkTreeFindNode(ih, id, &iterItem))
    return 0;
  if (!value)
    value = "";
  gtk_tree_store_set(store, &iterItem, IUPGTK_NODE_TITLE, iupgtkStrConvertToSystem(value), -1);
  return 0;
}

static int gtkTreeSetTitleFontAttrib(Ihandle* ih, int id, const char* value)
{
  PangoFontDescription* fontdesc = NULL;
  GtkTreeStore* store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle)));
  GtkTreeIter iterItem;
  if (!gtkTreeFindNode(ih, id, &iterItem))
    return 0;
  if (value)
    fontdesc = iupgtkGetPangoFontDesc(value);
  gtk_tree_store_set(store, &iterItem, IUPGTK_NODE_FONT, fontdesc, -1);
  return 0;
}

static char* gtkTreeGetTitleFontAttrib(Ihandle* ih, int id)
{
  PangoFontDescription* fontdesc;
  GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));
  GtkTreeIter iterItem;
  if (!gtkTreeFindNode(ih, id, &iterItem))
    return NULL;
  gtk_tree_model_get(model, &iterItem, IUPGTK_NODE_FONT, &fontdesc, -1);
  return pango_font_description_to_string(fontdesc);
}

static char* gtkTreeGetValueAttrib(Ihandle* ih)
{
  GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));
  GtkTreePath* path = NULL;

  gtk_tree_view_get_cursor(GTK_TREE_VIEW(ih->handle), &path, NULL);
  if (path)
  {
    GtkTreeIter iterItem;
    gtk_tree_model_get_iter(model, &iterItem, path);
    gtk_tree_path_free(path);

    return iupStrReturnInt(gtkTreeFindNodeId(ih, &iterItem));
  }

  if (ih->data->node_count)
    return "0"; /* default VALUE is root */
  else
    return "-1";
}

static char* gtkTreeGetMarkedNodesAttrib(Ihandle* ih)
{
  char* str = iupStrGetMemory(ih->data->node_count+1);
  GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));
  GtkTreeIter iterItem;
  int i;

  for (i=0; i<ih->data->node_count; i++)
  {
    gtkTreeIterInit(ih, &iterItem, ih->data->node_cache[i].node_handle);
    if (gtkTreeIsNodeSelected(model, &iterItem))
      str[i] = '+';
    else
      str[i] = '-';
  }

  str[ih->data->node_count] = 0;
  return str;
}

static int gtkTreeSetMarkedNodesAttrib(Ihandle* ih, const char* value)
{
  int count, i;
  GtkTreeModel* model;
  GtkTreeIter iterItem;
  GtkTreeSelection* selection;

  if (ih->data->mark_mode==ITREE_MARK_SINGLE || !value)
    return 0;

  count = (int)strlen(value);
  if (count > ih->data->node_count)
    count = ih->data->node_count;

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ih->handle));
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));

  iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");

  for (i=0; i<count; i++)
  {
    gtkTreeIterInit(ih, &iterItem, ih->data->node_cache[i].node_handle);
    if (value[i] == '+')
      gtkTreeSelectNode(model, selection, &iterItem, 1);
    else
      gtkTreeSelectNode(model, selection, &iterItem, 0);
  }

  iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);

  return 0;
}

static int gtkTreeSetMarkAttrib(Ihandle* ih, const char* value)
{
  GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));
  GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ih->handle));

  if (ih->data->mark_mode==ITREE_MARK_SINGLE)
    return 0;

  if(iupStrEqualNoCase(value, "BLOCK"))
  {
    GtkTreeIter iterItem1, iterItem2;
    GtkTreePath* pathFocus;
    gtk_tree_view_get_cursor(GTK_TREE_VIEW(ih->handle), &pathFocus, NULL);
    gtk_tree_model_get_iter(model, &iterItem1, pathFocus);
    gtk_tree_path_free(pathFocus);

    gtkTreeIterInit(ih, &iterItem2, iupAttribGet(ih, "_IUPTREE_MARKSTART_NODE"));

    gtkTreeSelectRange(ih, model, selection, &iterItem1, &iterItem2, 0);
  }
  else if(iupStrEqualNoCase(value, "CLEARALL"))
    gtkTreeSelectAll(ih, model, selection, 0);
  else if(iupStrEqualNoCase(value, "MARKALL"))
    gtkTreeSelectAll(ih, model, selection, 1);
  else if(iupStrEqualNoCase(value, "INVERTALL")) /* INVERTALL *MUST* appear before INVERT, or else INVERTALL will never be called. */
    gtkTreeInvertAllNodeMarking(ih, model, selection);
  else if(iupStrEqualPartial(value, "INVERT"))
  {
    /* iupStrEqualPartial allows the use of "INVERTid" form */
    GtkTreeIter iterItem;
    if (!gtkTreeFindNodeFromString(ih, &value[strlen("INVERT")], &iterItem))
      return 0;

    gtkTreeSelectNode(model, selection, &iterItem, -1);  /* toggle */
  }
  else
  {
    GtkTreeIter iterItem1, iterItem2;
    char str1[50], str2[50];
    if (iupStrToStrStr(value, str1, str2, '-')!=2)
      return 0;

    if (!gtkTreeFindNodeFromString(ih, str1, &iterItem1))
      return 0;
    if (!gtkTreeFindNodeFromString(ih, str2, &iterItem2))
      return 0;

    gtkTreeSelectRange(ih, model, selection, &iterItem1, &iterItem2, 0);
  }

  return 1;
} 

static int gtkTreeSetValueAttrib(Ihandle* ih, const char* value)
{
  GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));
  GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ih->handle));
  GtkTreeIter iterItem;
  GtkTreePath* path;
  int kind;

  if (gtkTreeSetMarkAttrib(ih, value))
    return 0;

  if (iupStrEqualNoCase(value, "ROOT") || iupStrEqualNoCase(value, "FIRST"))
    gtk_tree_model_get_iter_first(model, &iterItem);
  else if(iupStrEqualNoCase(value, "LAST"))
    gtkTreeGetLastVisibleNode(ih, model, &iterItem);
  else if(iupStrEqualNoCase(value, "PGUP"))
  {
    GtkTreePath* pathFocus;
    gtk_tree_view_get_cursor(GTK_TREE_VIEW(ih->handle), &pathFocus, NULL);
    gtk_tree_model_get_iter(model, &iterItem, pathFocus);
    gtk_tree_path_free(pathFocus);

    gtkTreeGetPreviousVisibleNode(ih, model, &iterItem, 10);
  }
  else if(iupStrEqualNoCase(value, "PGDN"))
  {
    GtkTreePath* pathFocus;
    gtk_tree_view_get_cursor(GTK_TREE_VIEW(ih->handle), &pathFocus, NULL);
    gtk_tree_model_get_iter(model, &iterItem, pathFocus);
    gtk_tree_path_free(pathFocus);

    gtkTreeGetNextVisibleNode(ih, model, &iterItem, 10);
  }
  else if(iupStrEqualNoCase(value, "NEXT"))
  {
    GtkTreePath* pathFocus;
    gtk_tree_view_get_cursor(GTK_TREE_VIEW(ih->handle), &pathFocus, NULL);
    gtk_tree_model_get_iter(model, &iterItem, pathFocus);
    gtk_tree_path_free(pathFocus);

    gtkTreeGetNextVisibleNode(ih, model, &iterItem, 1);
  }
  else if(iupStrEqualNoCase(value, "PREVIOUS"))
  {
    GtkTreePath* pathFocus;
    gtk_tree_view_get_cursor(GTK_TREE_VIEW(ih->handle), &pathFocus, NULL);
    gtk_tree_model_get_iter(model, &iterItem, pathFocus);
    gtk_tree_path_free(pathFocus);

    gtkTreeGetPreviousVisibleNode(ih, model, &iterItem, 1);
  }
  else if (iupStrEqualNoCase(value, "CLEAR"))
  {
    GtkTreePath* pathFocus;
    gtk_tree_view_get_cursor(GTK_TREE_VIEW(ih->handle), &pathFocus, NULL);
    gtk_tree_model_get_iter(model, &iterItem, pathFocus);
    gtk_tree_path_free(pathFocus);

    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
    gtkTreeSelectNode(model, selection, &iterItem, 0);
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
    return 0;
  }
  else
  {
    if (!gtkTreeFindNodeFromString(ih, value, &iterItem))
      return 0;
  }

  if (!iterItem.user_data)
    return 0;

  /* select */
  if (ih->data->mark_mode==ITREE_MARK_SINGLE)
  {
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
    gtkTreeSelectNode(model, selection, &iterItem, 1);
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
  }

  path = gtk_tree_model_get_path(model, &iterItem);

  /* make it visible */
  gtk_tree_model_get(model, &iterItem, IUPGTK_NODE_KIND, &kind, -1);
  if (kind == ITREE_LEAF)
    gtk_tree_view_expand_to_path(GTK_TREE_VIEW(ih->handle), path);
  else
  {
    int expanded = gtk_tree_view_row_expanded(GTK_TREE_VIEW(ih->handle), path);
    gtk_tree_view_expand_to_path(GTK_TREE_VIEW(ih->handle), path);
    if (!expanded) gtk_tree_view_collapse_row(GTK_TREE_VIEW(ih->handle), path);
  }

  gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(ih->handle), path, NULL, FALSE, 0, 0);  /* scroll to visible, minimum change */

  gtkTreeSetFocus(ih, path, &iterItem, FALSE);

  gtk_tree_path_free(path);

  iupAttribSetInt(ih, "_IUPTREE_OLDVALUE", gtkTreeFindNodeId(ih, &iterItem));

  return 0;
} 

static int gtkTreeSetMarkStartAttrib(Ihandle* ih, const char* value)
{
  GtkTreeIter iterItem;
  if (!gtkTreeFindNodeFromString(ih, value, &iterItem))
    return 0;

  iupAttribSet(ih, "_IUPTREE_MARKSTART_NODE", (char*)iterItem.user_data);

  return 1;
}

static char* gtkTreeGetMarkedAttrib(Ihandle* ih, int id)
{
  GtkTreeModel* model;
  GtkTreeIter iterItem;
  if (!gtkTreeFindNode(ih, id, &iterItem))
    return 0;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));
  return iupStrReturnBoolean (gtkTreeIsNodeSelected(model, &iterItem)); 
}

static int gtkTreeSetMarkedAttrib(Ihandle* ih, int id, const char* value)
{
  GtkTreeModel* model;
  GtkTreeSelection* selection;
  GtkTreeIter iterItem;
  if (!gtkTreeFindNode(ih, id, &iterItem))
    return 0;

  iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ih->handle));
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));

  if (iupStrBoolean(value))
    gtkTreeSelectNode(model, selection, &iterItem, 1);
  else
    gtkTreeSelectNode(model, selection, &iterItem, 0);

  iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);

  return 0;
}

static char* gtkTreeGetToggleValueAttrib(Ihandle* ih, int id)
{
  GtkTreeStore* store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle)));
  GtkTreeIter iterItem;

  if (!ih->data->show_toggle || !gtkTreeFindNode(ih, id, &iterItem))
    return NULL;

  return iupStrReturnChecked(gtkTreeToggleGetCheck(ih, store, iterItem));
}

static int gtkTreeSetToggleValueAttrib(Ihandle* ih, int id, const char* value)
{
  GtkTreeStore* store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle)));
  GtkTreeIter iterItem;

  if (!ih->data->show_toggle || !gtkTreeFindNode(ih, id, &iterItem))
    return 0;

  if(ih->data->show_toggle==2 && iupStrEqualNoCase(value, "NOTDEF"))
  {
    gtk_tree_store_set(store, &iterItem, IUPGTK_NODE_3STATE, TRUE, -1);
  }
  else
  {
    gtk_tree_store_set(store, &iterItem, IUPGTK_NODE_3STATE, FALSE, -1);

    if(iupStrEqualNoCase(value, "ON"))
      gtk_tree_store_set(store, &iterItem, IUPGTK_NODE_CHECK, TRUE, -1);
    else
      gtk_tree_store_set(store, &iterItem, IUPGTK_NODE_CHECK, FALSE, -1);
  }

  return 0;
}

static char* gtkTreeGetToggleVisibleAttrib(Ihandle* ih, int id)
{
  GtkTreeStore* store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle)));
  GtkTreeIter iterItem;
  int value;

  if (!ih->data->show_toggle || !gtkTreeFindNode(ih, id, &iterItem))
    return NULL;

  gtk_tree_model_get(GTK_TREE_MODEL(store), &iterItem, IUPGTK_NODE_TOGGLEVISIBLE, &value, -1);

  return iupStrReturnBoolean (value); 
}

static int gtkTreeSetToggleVisibleAttrib(Ihandle* ih, int id, const char* value)
{
  GtkTreeStore* store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle)));
  GtkTreeIter iterItem;

  if (!ih->data->show_toggle || !gtkTreeFindNode(ih, id, &iterItem))
    return 0;

  if(iupStrBoolean(value))
    gtk_tree_store_set(store, &iterItem, IUPGTK_NODE_TOGGLEVISIBLE, TRUE, -1);
  else
    gtk_tree_store_set(store, &iterItem, IUPGTK_NODE_TOGGLEVISIBLE, FALSE, -1);

  return 0;
}

static int gtkTreeSetDelNodeAttrib(Ihandle* ih, int id, const char* value)
{
  if (!ih->handle)  /* do not do the action before map */
    return 0;
  if (iupStrEqualNoCase(value, "ALL"))
  {
    GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));
    gtkTreeCallNodeRemovedAll(ih);

    /* deleting the reference node (and it's children) */
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
    gtk_tree_store_clear(GTK_TREE_STORE(model));
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
    return 0;
  }
  if (iupStrEqualNoCase(value, "SELECTED"))  /* selected here means the reference node */
  {
    GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));
    GtkTreeIter iterItem;

    if (!gtkTreeFindNode(ih, id, &iterItem))
      return 0;

    gtkTreeCallNodeRemoved(ih, model, &iterItem);

    /* deleting the reference node (and it's children) */
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
    gtk_tree_store_remove(GTK_TREE_STORE(model), &iterItem);
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
  }
  else if(iupStrEqualNoCase(value, "CHILDREN"))  /* children of the reference node */
  {
    GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));
    GtkTreeIter iterItem, iterChild;
    int hasChildren;

    if (!gtkTreeFindNode(ih, id, &iterItem))
      return 0;

    hasChildren = gtk_tree_model_iter_children(model, &iterChild, &iterItem);

    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");

    /* deleting the reference node children */
    while(hasChildren)
    {
      gtkTreeCallNodeRemoved(ih, model, &iterChild);
      hasChildren = gtk_tree_store_remove(GTK_TREE_STORE(model), &iterChild);
    }

    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
  }
  else if(iupStrEqualNoCase(value, "MARKED"))  /* Delete the array of marked nodes */
  {
    int i;
    GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));
    GtkTreeIter iterItem;

    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");

    for(i = 0; i < ih->data->node_count; /* increment only if not removed */)
    {
      gtkTreeIterInit(ih, &iterItem, ih->data->node_cache[i].node_handle);
      if (gtkTreeIsNodeSelected(model, &iterItem))
      {
        gtkTreeCallNodeRemoved(ih, model, &iterItem);
        gtk_tree_store_remove(GTK_TREE_STORE(model), &iterItem);
      }
      else
        i++;
    }

    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
  }

  return 0;
}

static int gtkTreeSetRenameAttrib(Ihandle* ih, const char* value)
{  
  if (ih->data->show_rename)
  {
    GtkTreePath* path;
    GtkTreeViewColumn *focus_column;
    GtkTreeIter iterItem;
    gtk_tree_view_get_cursor(GTK_TREE_VIEW(ih->handle), &path, &focus_column);
    gtk_tree_model_get_iter(gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle)), &iterItem, path);
    gtkTreeSetFocus(ih, path, &iterItem, TRUE); /* start editing */
    gtk_tree_path_free(path);
  }

  (void)value;
  return 0;
}

static int gtkTreeSetImageExpandedAttrib(Ihandle* ih, int id, const char* value)
{
  int kind;
  GtkTreeStore*  store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle)));
  GdkPixbuf* pixExpand = iupImageGetImage(value, ih, 0, NULL);
  GtkTreeIter iterItem;
  if (!gtkTreeFindNode(ih, id, &iterItem))
    return 0;

  gtk_tree_model_get(GTK_TREE_MODEL(store), &iterItem, IUPGTK_NODE_KIND, &kind, -1);

  if (kind == ITREE_BRANCH)
  {
    if (pixExpand)
      gtk_tree_store_set(store, &iterItem, IUPGTK_NODE_IMAGE_EXPANDED, pixExpand, 
                                           IUPGTK_NODE_HAS_IMAGE_EXPANDED, TRUE, -1);
    else
      gtk_tree_store_set(store, &iterItem, IUPGTK_NODE_IMAGE_EXPANDED, ih->data->def_image_expanded, 
                                           IUPGTK_NODE_HAS_IMAGE_EXPANDED, FALSE, -1);
  }

  return 1;
}

static int gtkTreeSetImageAttrib(Ihandle* ih, int id, const char* value)
{
  GtkTreeStore* store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle)));
  GdkPixbuf* pixImage = iupImageGetImage(value, ih, 0, NULL);
  GtkTreeIter iterItem;
  if (!gtkTreeFindNode(ih, id, &iterItem))
    return 0;

  if (pixImage)
  {
    gtk_tree_store_set(store, &iterItem, IUPGTK_NODE_IMAGE, pixImage, 
                                         IUPGTK_NODE_HAS_IMAGE, TRUE, -1);
  }
  else
  {
    int kind;
    gtk_tree_model_get(GTK_TREE_MODEL(store), &iterItem, IUPGTK_NODE_KIND, &kind, -1);
    if (kind == ITREE_BRANCH)
      gtk_tree_store_set(store, &iterItem, IUPGTK_NODE_IMAGE, ih->data->def_image_collapsed, 
                                           IUPGTK_NODE_HAS_IMAGE, FALSE, -1);
    else
      gtk_tree_store_set(store, &iterItem, IUPGTK_NODE_IMAGE, ih->data->def_image_leaf, 
                                           IUPGTK_NODE_HAS_IMAGE, FALSE, -1);
  }

  return 0;
}

static int gtkTreeSetImageBranchExpandedAttrib(Ihandle* ih, const char* value)
{
  ih->data->def_image_expanded = iupImageGetImage(value, ih, 0, NULL);

  /* Update all images */
  gtkTreeUpdateImages(ih, ITREE_UPDATEIMAGE_EXPANDED);

  return 1;
}

static int gtkTreeSetImageBranchCollapsedAttrib(Ihandle* ih, const char* value)
{
  ih->data->def_image_collapsed = iupImageGetImage(value, ih, 0, NULL);

  /* Update all images */
  gtkTreeUpdateImages(ih, ITREE_UPDATEIMAGE_COLLAPSED);

  return 1;
}

static int gtkTreeSetImageLeafAttrib(Ihandle* ih, const char* value)
{
  ih->data->def_image_leaf = iupImageGetImage(value, ih, 0, NULL);

  /* Update all images */
  gtkTreeUpdateImages(ih, ITREE_UPDATEIMAGE_LEAF);

  return 1;
}

static int gtkTreeSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;

  GtkScrolledWindow* scrolled_window = (GtkScrolledWindow*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (scrolled_window && iupStrBoolean(IupGetGlobal("SB_BGCOLOR")))
  {
    /* ignore given value, must use only from parent for the scrollbars */
    char* parent_value = iupBaseNativeParentGetBgColor(ih);

    if (iupStrToRGB(parent_value, &r, &g, &b))
    {
      GtkWidget* sb;

      if (!GTK_IS_SCROLLED_WINDOW(scrolled_window))
        scrolled_window = (GtkScrolledWindow*)iupAttribGet(ih, "_IUPGTK_SCROLLED_WINDOW");

      iupgtkSetBgColor((GtkWidget*)scrolled_window, r, g, b);

#if GTK_CHECK_VERSION(2, 8, 0)
      sb = gtk_scrolled_window_get_hscrollbar(scrolled_window);
      if (sb) iupgtkSetBgColor(sb, r, g, b);

      sb = gtk_scrolled_window_get_vscrollbar(scrolled_window);
      if (sb) iupgtkSetBgColor(sb, r, g, b);
#endif
    }
  }

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  {
    GtkCellRenderer *renderer_chk = (GtkCellRenderer*)iupAttribGet(ih, "_IUPGTK_RENDERER_CHECK");
    GtkCellRenderer* renderer_txt = (GtkCellRenderer*)iupAttribGet(ih, "_IUPGTK_RENDERER_TEXT");
    GtkCellRenderer* renderer_img = (GtkCellRenderer*)iupAttribGet(ih, "_IUPGTK_RENDERER_IMG");
    GdkColor color;
    iupgdkColorSetRGB(&color, r, g, b);
    if (renderer_chk) g_object_set(G_OBJECT(renderer_chk), "cell-background-gdk", &color, NULL);
    g_object_set(G_OBJECT(renderer_txt), "cell-background-gdk", &color, NULL);
    g_object_set(G_OBJECT(renderer_img), "cell-background-gdk", &color, NULL);
  }

  iupdrvBaseSetBgColorAttrib(ih, value);   /* use given value for contents */

  /* no need to update internal image cache in GTK */

  return 1;
}

static int gtkTreeSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupgtkSetFgColor(ih->handle, r, g, b);

  {
    GtkCellRenderer* renderer_txt = (GtkCellRenderer*)iupAttribGet(ih, "_IUPGTK_RENDERER_TEXT");
    GdkColor color;
    iupgdkColorSetRGB(&color, r, g, b);
    g_object_set(G_OBJECT(renderer_txt), "foreground-gdk", &color, NULL);
    g_object_get(G_OBJECT(renderer_txt), "foreground-gdk", &color, NULL);
    color.blue = 0;
  }

  return 1;
}

static int gtkTreeSetShowRenameAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->show_rename = 1;
  else
    ih->data->show_rename = 0;

  if (ih->handle)
  {
    GtkCellRenderer *renderer_txt = (GtkCellRenderer*)iupAttribGet(ih, "_IUPGTK_RENDERER_TEXT");
    g_object_set(G_OBJECT(renderer_txt), "editable", ih->data->show_rename, NULL);
  }

  return 0;
}

void iupdrvTreeUpdateMarkMode(Ihandle *ih)
{
  GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ih->handle));
  gtk_tree_selection_set_mode(selection, (ih->data->mark_mode==ITREE_MARK_SINGLE)? GTK_SELECTION_SINGLE: GTK_SELECTION_MULTIPLE);

  if (ih->data->mark_mode==ITREE_MARK_MULTIPLE && !ih->data->show_dragdrop)
  {
#if GTK_CHECK_VERSION(2, 10, 0)
    if (iupAttribGetInt(ih, "RUBBERBAND"))
      gtk_tree_view_set_rubber_banding(GTK_TREE_VIEW(ih->handle), TRUE);
#endif
  }
}


/***********************************************************************************************/


static void gtkTreeSetRenameCaretPos(GtkCellEditable *editable, const char* value)
{
  int pos = 1;

  if (iupStrToInt(value, &pos))
  {
    if (pos < 1) pos = 1;
    pos--; /* IUP starts at 1 */

    gtk_editable_set_position(GTK_EDITABLE(editable), pos);
  }
}

static void gtkTreeSetRenameSelectionPos(GtkCellEditable *editable, const char* value)
{
  int start = 1, end = 1;

  if (iupStrToIntInt(value, &start, &end, ':') != 2) 
    return;

  if(start < 1 || end < 1) 
    return;

  start--; /* IUP starts at 1 */
  end--;

  gtk_editable_select_region(GTK_EDITABLE(editable), start, end);
}

/*****************************************************************************/
/* SIGNALS                                                                   */
/*****************************************************************************/

static void gtkTreeCellTextEditingStarted(GtkCellRenderer *cell, GtkCellEditable *editable, const gchar *path_string, Ihandle *ih)
{
  char* value;
  GtkTreeIter iterItem;
  PangoFontDescription* fontdesc = NULL;
  GdkColor *color = NULL;
  GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));
  IFni cbShowRename;

  gtk_tree_model_get_iter_from_string(model, &iterItem, path_string);

  cbShowRename = (IFni)IupGetCallback(ih, "SHOWRENAME_CB");
  if (cbShowRename && cbShowRename(ih, gtkTreeFindNodeId(ih, &iterItem))==IUP_IGNORE)
  {
    /* TODO: none of these worked:
    gtk_cell_renderer_stop_editing(cell, TRUE);
    gtk_cell_editable_editing_done(editable);  */
    gtk_editable_set_editable(GTK_EDITABLE(editable), FALSE);
    return;
  }

  value = iupAttribGetStr(ih, "RENAMECARET");
  if (value)
    gtkTreeSetRenameCaretPos(editable, value);

  value = iupAttribGetStr(ih, "RENAMESELECTION");
  if (value)
    gtkTreeSetRenameSelectionPos(editable, value);

  gtk_tree_model_get(model, &iterItem, IUPGTK_NODE_FONT, &fontdesc, -1);
  if (fontdesc)
  {
#if GTK_CHECK_VERSION(3, 0, 0)
    gtk_widget_override_font(GTK_WIDGET(editable), fontdesc);
#else
    gtk_widget_modify_font(GTK_WIDGET(editable), fontdesc);
#endif
  }

  gtk_tree_model_get(model, &iterItem, IUPGTK_NODE_COLOR, &color, -1);
  if (color)
    iupgtkSetFgColor(GTK_WIDGET(editable), iupgtkColorFromDouble(color->red), 
                                               iupgtkColorFromDouble(color->green), 
                                               iupgtkColorFromDouble(color->blue));

  (void)cell;
}

static void gtkTreeCellTextEdited(GtkCellRendererText *cell, gchar *path_string, gchar *new_text, Ihandle* ih)
{
  GtkTreeModel* model;
  GtkTreeIter iterItem;
  IFnis cbRename;

  if (!new_text)
    new_text = "";

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));
  if (!gtk_tree_model_get_iter_from_string(model, &iterItem, path_string))
    return;

  cbRename = (IFnis)IupGetCallback(ih, "RENAME_CB");
  if (cbRename)
  {
    if (cbRename(ih, gtkTreeFindNodeId(ih, &iterItem), iupgtkStrConvertFromSystem(new_text)) == IUP_IGNORE)
      return;
  }

  /* It is the responsibility of the application to update the model and store new_text at the position indicated by path. */
  gtk_tree_store_set(GTK_TREE_STORE(model), &iterItem, IUPGTK_NODE_TITLE, new_text, -1);

  (void)cell;
}

static int gtkTreeCallDragDropCb(Ihandle* ih, GtkTreeIter *iterDrag, GtkTreeIter *iterDrop, int *is_ctrl)
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
    int drag_id = gtkTreeFindNodeId(ih, iterDrag);
    int drop_id = gtkTreeFindNodeId(ih, iterDrop);
    return cbDragDrop(ih, drag_id, drop_id, is_shift, *is_ctrl);
  }

  return IUP_CONTINUE; /* allow to move by default if callback not defined */
}

static void gtkTreeDragDataReceived(GtkWidget *widget, GdkDragContext *context, gint x, gint y, 
                                    GtkSelectionData *selection_data, guint info, guint time, Ihandle* ih)
{
  GtkTreePath* pathDrag = (GtkTreePath*)iupAttribGet(ih, "_IUPTREE_DRAGITEM");
  GtkTreePath* pathDrop = NULL;
  int is_ctrl;

  gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(ih->handle), x, y, &pathDrop, NULL);

  if (pathDrag && pathDrop)
  {
    int equal_nodes = 0, has_parent = 1;
    GtkTreeIter iterDrag, iterDrop, iterParent, iterNextParent;
    GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));

    gtk_tree_model_get_iter(model, &iterDrag, pathDrag);
    gtk_tree_model_get_iter(model, &iterDrop, pathDrop);

    /* If Drag item is an ancestor or equal to Drop item then return */
    iterParent = iterDrop;
    while (has_parent)
    {
      if (iterParent.user_data == iterDrag.user_data)
      {
        if (!iupAttribGetBoolean(ih, "DROPEQUALDRAG"))
          goto gtkTreeDragDataReceived_FINISH;

        equal_nodes = 1;
        break;
      }

      has_parent = gtk_tree_model_iter_parent(model, &iterNextParent, &iterParent);
      iterParent = iterNextParent;
    }

    if (gtkTreeCallDragDropCb(ih, &iterDrag, &iterDrop, &is_ctrl) == IUP_CONTINUE && !equal_nodes)
    {
      GtkTreeIter iterNewItem;

      /* Copy or move the dragged item to the new position. */
      gtkTreeCopyMoveNode(ih, model, &iterDrag, &iterDrop, &iterNewItem, is_ctrl);

      /* set focus and selection */
      if (iterNewItem.stamp)
      {
        GtkTreePath *pathNew;
        GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ih->handle));

        pathNew = gtk_tree_model_get_path(model, &iterNewItem);
        gtkTreeSelectNode(model, selection, &iterNewItem, 1);

        gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(ih->handle), pathNew, NULL, FALSE, 0, 0);  /* scroll to visible, minimum change */

        /* unselect all, select new node and focus */
        iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
        gtk_tree_view_set_cursor(GTK_TREE_VIEW(ih->handle), pathNew, NULL, FALSE);
        iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);

        gtk_tree_path_free(pathNew);
      }
    }
  }


gtkTreeDragDataReceived_FINISH:
  if (pathDrag) gtk_tree_path_free(pathDrag);
  if (pathDrop) gtk_tree_path_free(pathDrop);

  iupAttribSet(ih, "_IUPTREE_DRAGITEM", NULL);

  (void)widget;
  (void)info;
  (void)context;
  (void)time;
  (void)selection_data;
}

static gboolean gtkTreeDragMotion(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, Ihandle* ih)
{
  GtkTreePath* path;
  GtkTreeViewDropPosition pos;
  if (gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(ih->handle), x, y, &path, &pos))
  {
    if (pos == GTK_TREE_VIEW_DROP_BEFORE) pos = GTK_TREE_VIEW_DROP_INTO_OR_BEFORE;
    if (pos == GTK_TREE_VIEW_DROP_AFTER) pos = GTK_TREE_VIEW_DROP_INTO_OR_AFTER;
    /* highlight row */
    gtk_tree_view_set_drag_dest_row(GTK_TREE_VIEW(widget), path, pos);
    gtk_tree_path_free(path);

#if GTK_CHECK_VERSION(2, 22, 0)
    gdk_drag_status(context, gdk_drag_context_get_suggested_action(context), time);
#else
    gdk_drag_status(context, context->suggested_action, time);
#endif
    return TRUE;
  }

  return FALSE;
}

static void gtkTreeDragBegin(GtkWidget *widget, GdkDragContext *context, Ihandle* ih)
{
  int x = iupAttribGetInt(ih, "_IUPTREE_DRAG_X");
  int y = iupAttribGetInt(ih, "_IUPTREE_DRAG_Y");
  GtkTreePath* path;
  if (gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(ih->handle), x, y, &path, NULL))
    iupAttribSet(ih, "_IUPTREE_DRAGITEM", (char*)path);

  (void)context;
  (void)widget;
}

static gboolean gtkTreeSelectionFunc(GtkTreeSelection *selection, GtkTreeModel *model, GtkTreePath *path, gboolean old_selected, Ihandle* ih)
{
  /* every change (programaticaly or interactively) to the selection state will call this function,
     so we use it to keep our storage updated. 
     But it will not be called when we select hidden nodes. */
  GtkTreeIter iterItem;
  gtk_tree_model_get_iter(model, &iterItem, path);
  gtkTreeSelectNodeRaw(model, &iterItem, !old_selected);
  (void)ih;
  (void)selection;
  return TRUE;
}

static void gtkTreeSelectionChanged(GtkTreeSelection* selection, Ihandle* ih)
{
  IFnii cbSelec;
  int is_ctrl = 0;
  (void)selection;

  if (iupAttribGet(ih, "_IUPTREE_IGNORE_SELECTION_CB"))
    return;

  if (ih->data->mark_mode == ITREE_MARK_MULTIPLE)
  {
    char key[5];
    iupdrvGetKeyState(key);
    if (key[0] == 'S')
      return;
    else if (key[1] == 'C')
      is_ctrl = 1;

    if (iupAttribGetInt(ih, "_IUPTREE_EXTENDSELECT")==2 && !is_ctrl)
    {
      iupAttribSet(ih, "_IUPTREE_EXTENDSELECT", NULL);
      gtkTreeCallMultiSelectionCb(ih);
      return;
    }
  }

  cbSelec = (IFnii)IupGetCallback(ih, "SELECTION_CB");
  if (cbSelec)
  {
    int curpos = -1, is_selected = 0;
    GtkTreeIter iterFocus;
    GtkTreePath* pathFocus;
    GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));

    gtk_tree_view_get_cursor(GTK_TREE_VIEW(ih->handle), &pathFocus, NULL);
    if (pathFocus)
    {
      gtk_tree_model_get_iter(model, &iterFocus, pathFocus);
      gtk_tree_path_free(pathFocus);
      curpos = gtkTreeFindNodeId(ih, &iterFocus);
      is_selected = gtkTreeIsNodeSelected(model, &iterFocus);
    }

    if (curpos == -1)
      return;

    if (is_ctrl) 
    {
      cbSelec(ih, curpos, is_selected);
      iupAttribSetInt(ih, "_IUPTREE_OLDVALUE", -2); /* invalid value signalizing that its state was toggled */
    }
    else
    {
      int oldpos = iupAttribGetInt(ih, "_IUPTREE_OLDVALUE");
      if(oldpos != curpos)
      {
        if (oldpos >= 0)
          cbSelec(ih, oldpos, 0);  /* unselected */
        cbSelec(ih, curpos, 1);  /*   selected */

        iupAttribSetInt(ih, "_IUPTREE_OLDVALUE", curpos);
      }
    }
  }
}

static void gtkTreeUpdateSelectionChildren(Ihandle* ih, GtkTreeModel* model, GtkTreeSelection* selection, GtkTreeIter *iterItem)
{
  int expanded;
  GtkTreeIter iterChild;
  int hasItem = gtk_tree_model_iter_children(model, &iterChild, iterItem);  /* get the firstchild */
  while(hasItem)
  {
    if (gtkTreeIsNodeSelected(model, &iterChild))
      gtk_tree_selection_select_iter(selection, &iterChild);

    expanded = 0;
    if (gtk_tree_model_iter_has_child(model, &iterChild))
    {
      GtkTreePath* path = gtk_tree_model_get_path(model, &iterChild);
      expanded = gtk_tree_view_row_expanded(GTK_TREE_VIEW(ih->handle), path);
      gtk_tree_path_free(path);
    }

    /* Recursive only if expanded */
    if (expanded)
      gtkTreeUpdateSelectionChildren(ih, model, selection, &iterChild);  

    /* Go to next sibling item */
    hasItem = gtk_tree_model_iter_next(model, &iterChild);
  }
}

static void gtkTreeRowExpanded(GtkTreeView* tree_view, GtkTreeIter *iterItem, GtkTreePath *path, Ihandle* ih)
{
  GtkTreeSelection* selection = gtk_tree_view_get_selection(tree_view);
  GtkTreeModel* model = gtk_tree_view_get_model(tree_view);
  iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
  gtkTreeUpdateSelectionChildren(ih, model, selection, iterItem);
  iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
  (void)path;
}

static gboolean gtkTreeTestExpandRow(GtkTreeView* tree_view, GtkTreeIter *iterItem, GtkTreePath *path, Ihandle* ih)
{
  IFni cbBranchOpen = (IFni)IupGetCallback(ih, "BRANCHOPEN_CB");
  if (cbBranchOpen)
  {
    if (iupAttribGet(ih, "_IUPTREE_IGNORE_BRANCH_CB"))
      return FALSE;

    if (cbBranchOpen(ih, gtkTreeFindNodeId(ih, iterItem)) == IUP_IGNORE)
      return TRUE;  /* prevent the change */
  }

  (void)path;
  (void)tree_view;
  return FALSE;
}

static gboolean gtkTreeTestCollapseRow(GtkTreeView* tree_view, GtkTreeIter *iterItem, GtkTreePath *path, Ihandle* ih)
{
  IFni cbBranchClose = (IFni)IupGetCallback(ih, "BRANCHCLOSE_CB");
  if (cbBranchClose)
  {
    if (iupAttribGet(ih, "_IUPTREE_IGNORE_BRANCH_CB"))
      return FALSE;

    if (cbBranchClose(ih, gtkTreeFindNodeId(ih, iterItem)) == IUP_IGNORE)
      return TRUE;
  }

  (void)path;
  (void)tree_view;
  return FALSE;
}

static void gtkTreeRowActived(GtkTreeView* tree_view, GtkTreePath *path, GtkTreeViewColumn *column, Ihandle* ih)
{
  GtkTreeIter iterItem;
  GtkTreeModel* model;
  int kind;  /* used for nodes defined as branches, but do not have children */
  IFni cbExecuteLeaf  = (IFni)IupGetCallback(ih, "EXECUTELEAF_CB");
  IFni cbExecuteBranch = (IFni)IupGetCallback(ih, "EXECUTEBRANCH_CB");
  if (!cbExecuteLeaf && !cbExecuteBranch)
    return;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));
  gtk_tree_model_get_iter(model, &iterItem, path);
  gtk_tree_model_get(model, &iterItem, IUPGTK_NODE_KIND, &kind, -1);

  /* just to leaf nodes */
  if (kind == ITREE_LEAF)
  {
    if (cbExecuteLeaf)
      cbExecuteLeaf(ih, gtkTreeFindNodeId(ih, &iterItem));
  }
  else
  {
    if (cbExecuteBranch)
      cbExecuteBranch(ih, gtkTreeFindNodeId(ih, &iterItem));
  }

  (void)column;
  (void)tree_view;
}

static int gtkTreeConvertXYToPos(Ihandle* ih, int x, int y)
{
  GtkTreePath* path;
  if (gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(ih->handle), x, y, &path, NULL))
  {
    GtkTreeIter iterItem;
    GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));
    gtk_tree_model_get_iter(model, &iterItem, path);
    gtk_tree_path_free (path);
    return gtkTreeFindNodeId(ih, &iterItem);
  }
  return -1;
}

static void gtkTreeCallMultiUnSelectionCb(Ihandle* ih, int new_select_id)
{
  /* called when several items are unselected at once */
  IFnIi cbMulti = (IFnIi)IupGetCallback(ih, "MULTIUNSELECTION_CB");
  IFnii cbSelec = (IFnii)IupGetCallback(ih, "SELECTION_CB");
  if (cbSelec || cbMulti)
  {
    Iarray* markedArray = gtkTreeGetSelectedArrayId(ih);
    int* id_hitem = (int*)iupArrayGetData(markedArray);
    int count = iupArrayCount(markedArray);
    if (count > 0)
    {
      int i;

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
    }

    iupArrayDestroy(markedArray);
  }
}

static void gtkTreeCallRightClickCb(Ihandle* ih, int x, int y)
{
  GtkTreePath* path;
  if (gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(ih->handle), x, y, &path, NULL))
  {
    IFni cbRightClick = (IFni)IupGetCallback(ih, "RIGHTCLICK_CB");
    if (cbRightClick)
    {
      GtkTreeIter iterItem;
      GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));
      gtk_tree_model_get_iter(model, &iterItem, path);
      cbRightClick(ih, gtkTreeFindNodeId(ih, &iterItem));
    }

    gtk_tree_path_free (path);
  }
}
      
static int gtkTreeIsBranchButton(GtkTreeModel* model, GtkTreeIter *iter, int cell_x)
{
  int kind;
  gtk_tree_model_get(model, iter, IUPGTK_NODE_KIND, &kind, -1);

  if (kind == ITREE_BRANCH) /* if branch must check if just expanded/contracted */
  {
    int depth = gtk_tree_store_iter_depth(GTK_TREE_STORE(model), iter);
    if (cell_x < (depth+1)*16)
      return 1;
  }

  return 0;
}

static gboolean gtkTreeButtonEvent(GtkWidget *treeview, GdkEventButton *evt, Ihandle* ih)
{
  if (iupgtkButtonEvent(treeview, evt, ih) == TRUE)
    return TRUE;

  if (evt->type == GDK_BUTTON_PRESS && evt->button == 3)  /* right single click */
  {
    gtkTreeCallRightClickCb(ih, (int)evt->x, (int)evt->y);
    return TRUE;
  }
  else if (evt->type == GDK_2BUTTON_PRESS && evt->button == 1)  /* left double click */
  {
    GtkTreePath *path;

    if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(treeview), (gint) evt->x, (gint) evt->y, &path, NULL, NULL, NULL))
    {
      GtkTreeIter iter;
      GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));
      int kind;  /* used for nodes defined as branches, but do not have children */

      gtk_tree_model_get_iter(model, &iter, path);
      gtk_tree_model_get(model, &iter, IUPGTK_NODE_KIND, &kind, -1);

      if (kind == ITREE_BRANCH)
        gtkTreeExpandItem(ih, path, -1); /* toggle */

      gtk_tree_path_free(path);
    }
  }
  else if (evt->type == GDK_BUTTON_PRESS && evt->button == 1)  /* left single press */
  {
    iupAttribSetInt(ih, "_IUPTREE_DRAG_X", (int)evt->x);
    iupAttribSetInt(ih, "_IUPTREE_DRAG_Y", (int)evt->y);

    if (ih->data->mark_mode==ITREE_MARK_MULTIPLE && 
        !(evt->state & GDK_SHIFT_MASK) && !(evt->state & GDK_CONTROL_MASK))
    {
      /* simple click with mark_mode==ITREE_MARK_MULTIPLE and !Shift and !Ctrl */
      /* do not call the callback for the new selected item */
      GtkTreePath *path;
      int new_select_id = -1, cell_x;
      GtkTreeViewColumn *column;

      if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(treeview), (gint)evt->x, (gint)evt->y, &path, &column, &cell_x, NULL))
      {
        GtkTreeIter iterItem;
        GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle));

        gtk_tree_model_get_iter(model, &iterItem, path);

        if (!gtkTreeIsBranchButton(model, &iterItem, cell_x))
          new_select_id = gtkTreeFindNodeId(ih, &iterItem);

        gtk_tree_path_free(path);
      }

      if (new_select_id != -1)
        gtkTreeCallMultiUnSelectionCb(ih, new_select_id);
      iupAttribSet(ih, "_IUPTREE_EXTENDSELECT", "1");
    }
  }
  else if (evt->type == GDK_BUTTON_RELEASE && evt->button == 1)  /* left single release */
  {
    if (ih->data->mark_mode==ITREE_MARK_MULTIPLE && (evt->state & GDK_SHIFT_MASK))
      gtkTreeCallMultiSelectionCb(ih); /* Multi Selection Callback */

    if (ih->data->mark_mode==ITREE_MARK_MULTIPLE && 
        !(evt->state & GDK_SHIFT_MASK) && !(evt->state & GDK_CONTROL_MASK))
    {
      if (iupAttribGet(ih, "_IUPTREE_EXTENDSELECT"))
        iupAttribSet(ih, "_IUPTREE_EXTENDSELECT", "2");
    }
  }

  return FALSE;
}

static gboolean gtkTreeKeyReleaseEvent(GtkWidget *widget, GdkEventKey *evt, Ihandle *ih)
{
  if (ih->data->mark_mode==ITREE_MARK_MULTIPLE && (evt->state & GDK_SHIFT_MASK))
  {
    if (evt->keyval == GDK_Up || evt->keyval == GDK_Down || evt->keyval == GDK_Home || evt->keyval == GDK_End)
      gtkTreeCallMultiSelectionCb(ih); /* Multi Selection Callback */
  }

  (void)widget;
  return TRUE;
}

static gboolean gtkTreeKeyPressEvent(GtkWidget *widget, GdkEventKey *evt, Ihandle *ih)
{
  if (iupgtkKeyPressEvent(widget, evt, ih) == TRUE)
    return TRUE;

  if (evt->keyval == GDK_F2)
  {
    gtkTreeSetRenameAttrib(ih, NULL);
    return TRUE;
  }
  else if (evt->keyval == GDK_Return || evt->keyval == GDK_KP_Enter)
  {
    gtkTreeOpenCloseEvent(ih);
    return TRUE;
  }

  return FALSE;
}

static void gtkTreeEnableDragDrop(Ihandle* ih)
{
  const GtkTargetEntry row_targets[] = {
    /* use a custom target to avoid the internal gtkTreView DND */
    { "IUP_TREE_TARGET", GTK_TARGET_SAME_WIDGET, 0 }
  };

  gtk_tree_view_enable_model_drag_source (GTK_TREE_VIEW(ih->handle),
            GDK_BUTTON1_MASK,
            row_targets,
            G_N_ELEMENTS (row_targets),
            GDK_ACTION_MOVE|GDK_ACTION_COPY);
  gtk_tree_view_enable_model_drag_dest (GTK_TREE_VIEW(ih->handle),
          row_targets,
          G_N_ELEMENTS (row_targets),
          GDK_ACTION_MOVE|GDK_ACTION_COPY);

  /* let gtkTreView begin the drag, then use the signal to save the drag start path */
  g_signal_connect_after(G_OBJECT(ih->handle),  "drag-begin", G_CALLBACK(gtkTreeDragBegin), ih);

  /* to avoid drop between nodes. */
  g_signal_connect(G_OBJECT(ih->handle), "drag-motion", G_CALLBACK(gtkTreeDragMotion), ih);

  /* to avoid the internal gtkTreView DND, we do the drop manually */
  g_signal_connect(G_OBJECT(ih->handle), "drag-data-received", G_CALLBACK(gtkTreeDragDataReceived), ih);
}

static void gtkTreeToggleCB(Ihandle *ih, GtkTreeIter *iterItem, int check)
{
  IFnii cbToggle = (IFnii)IupGetCallback(ih, "TOGGLEVALUE_CB");
  if (cbToggle)
    cbToggle(ih, gtkTreeFindNodeId(ih, iterItem), check);

  if (iupAttribGetBoolean(ih, "MARKWHENTOGGLE"))
  {
    int id = gtkTreeFindNodeId(ih, iterItem);
    IupSetAttributeId(ih, "MARKED", id, check > 0? "Yes" : "No");
  }
}

static void gtkTreeToggled(GtkCellRendererToggle *cell_renderer, gchar *path, Ihandle *ih)
{
  GtkTreeStore* store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle)));
  GtkTreeIter iterItem;
  int check;

  /* called for two states only */

  if(!gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(store), &iterItem, path))
    return;

  check = gtkTreeToggleGetCheck(ih, store, iterItem);

  if(check)  /* GOTO check == 0 */
  {
    gtk_tree_store_set(store, &iterItem, IUPGTK_NODE_CHECK, FALSE, -1);
    gtkTreeToggleCB(ih, &iterItem, 0);
  }
  else  /* GOTO check == 1 */
  {
    gtk_tree_store_set(store, &iterItem, IUPGTK_NODE_CHECK, TRUE, -1);
    gtkTreeToggleCB(ih, &iterItem, 1);
  }

  (void)cell_renderer;
}

static int gtkTreeToggleUpdate3StateCheck(Ihandle *ih, int x, int y, int keyb)
{
  GtkTreeStore* store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(ih->handle)));
  GtkTreePath* path;
  GtkTreeIter iterItem;
  int check;

  if(keyb)
    gtk_tree_view_get_cursor(GTK_TREE_VIEW(ih->handle), &path, NULL);
  else
    gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(ih->handle), x, y, &path, NULL, NULL, NULL);

  if(!gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iterItem, path) || ((x == 0) && (y == 0)))
  {
    gtk_tree_path_free(path);
    return FALSE;
  }

  check = gtkTreeToggleGetCheck(ih, store, iterItem);
  if (check == 1)  /* GOTO check == -1 */
  {
    gtk_tree_store_set(store, &iterItem, IUPGTK_NODE_3STATE, TRUE, -1);
    gtk_tree_store_set(store, &iterItem, IUPGTK_NODE_CHECK, FALSE, -1);
    gtkTreeToggleCB(ih, &iterItem, -1);
  }
  else if (check == -1)  /* GOTO check == 0 */
  {
    gtk_tree_store_set(store, &iterItem, IUPGTK_NODE_3STATE, FALSE, -1);
    gtk_tree_store_set(store, &iterItem, IUPGTK_NODE_CHECK, FALSE, -1);
    gtkTreeToggleCB(ih, &iterItem, 0);
  }
  else  /* (check == 0)  GOTO check == 1 */
  {
    gtk_tree_store_set(store, &iterItem, IUPGTK_NODE_3STATE, FALSE, -1);
    gtk_tree_store_set(store, &iterItem, IUPGTK_NODE_CHECK, TRUE, -1);
    gtkTreeToggleCB(ih, &iterItem, 1);
  }

  gtk_tree_path_free(path);
  return TRUE; /* ignore message to avoid change toggle state */
}

static gboolean gtkTreeToggle3StateButtonEvent(GtkWidget *widget, GdkEventButton *evt, Ihandle *ih)
{
  gtk_tree_view_unset_rows_drag_source ((GtkTreeView*)ih->handle);
  gtk_tree_view_unset_rows_drag_dest ((GtkTreeView*)ih->handle);

  if (evt->type == GDK_BUTTON_RELEASE)
  {
    if (gtkTreeToggleUpdate3StateCheck(ih, (gint)evt->x, (gint)evt->y, 0))
      return TRUE; /* ignore message to avoid change toggle state */
  }

  gtkTreeEnableDragDrop(ih);

  (void)widget;
  return FALSE;
}

static gboolean gtkTreeToggle3StateKeyEvent(GtkWidget *widget, GdkEventKey *evt, Ihandle *ih)
{
  if (evt->type == GDK_KEY_PRESS)
  {
    if (evt->keyval == GDK_space || evt->keyval == GDK_Return)
      return TRUE; /* ignore message to avoid change toggle state */
  }
  else
  {
    if (evt->keyval == GDK_space || evt->keyval == GDK_Return)
    {
      if (gtkTreeToggleUpdate3StateCheck(ih, 0, 0, 1))
        return TRUE; /* ignore message to avoid change toggle state */
    }
  }

  (void)widget;
  return FALSE;
}

/*****************************************************************************/

static void gtkTreeDragDropCopyItem(Ihandle* src, Ihandle* dst, GtkTreeIter* iterItem, GtkTreeIter* iterParent, int position, GtkTreeIter *iterNewItem)
{
  GtkTreeStore* storeSrc = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(src->handle)));
  GtkTreeStore* storeDst = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(dst->handle)));
  int kind;
  char* title;
  gboolean has_image, has_image_expanded;
  PangoFontDescription* font;
  GdkColor *color;
  GdkPixbuf* image, *image_expanded;

  gtk_tree_model_get(GTK_TREE_MODEL(storeSrc), iterItem, IUPGTK_NODE_IMAGE, &image,
                                                         IUPGTK_NODE_HAS_IMAGE, &has_image,
                                                         IUPGTK_NODE_IMAGE_EXPANDED, &image_expanded,
                                                         IUPGTK_NODE_HAS_IMAGE_EXPANDED, &has_image_expanded,
                                                         IUPGTK_NODE_TITLE, &title,
                                                         IUPGTK_NODE_KIND, &kind,
                                                         IUPGTK_NODE_COLOR, &color, 
                                                         IUPGTK_NODE_FONT, &font, 
                                                         -1);

  /* Add the new node */
  dst->data->node_count++;
  if (position == 2)
    gtk_tree_store_append(storeDst, iterNewItem, iterParent);
  else if (position == 1)                                      /* copy as first child of expanded branch */
    gtk_tree_store_insert(storeDst, iterNewItem, iterParent, 0);  /* iterParent is parent of the new item (firstchild of it) */
  else                                                                  /* copy as next brother of item or collapsed branch */
    gtk_tree_store_insert_after(storeDst, iterNewItem, NULL, iterParent);  /* iterParent is sibling of the new item */

  gtk_tree_store_set(storeDst, iterNewItem,  IUPGTK_NODE_IMAGE, image,
                                             IUPGTK_NODE_HAS_IMAGE, has_image,
                                             IUPGTK_NODE_IMAGE_EXPANDED, image_expanded,
                                             IUPGTK_NODE_HAS_IMAGE_EXPANDED, has_image_expanded,
                                             IUPGTK_NODE_TITLE, title,
                                             IUPGTK_NODE_KIND, kind,
                                             IUPGTK_NODE_COLOR, color, 
                                             IUPGTK_NODE_FONT, font,
                                             IUPGTK_NODE_SELECTED, 0,
                                             IUPGTK_NODE_CHECK, 0,
                                             IUPGTK_NODE_3STATE, 0,
                                             IUPGTK_NODE_TOGGLEVISIBLE, 1,
                                             -1);
}

static void gtkTreeDragDropCopyChildren(Ihandle* src, Ihandle* dst, GtkTreeIter *iterItemSrc, GtkTreeIter *iterItemDst)
{
  GtkTreeIter iterChildSrc;
  GtkTreeModel* modelSrc = gtk_tree_view_get_model(GTK_TREE_VIEW(src->handle));
  int hasItem = gtk_tree_model_iter_children(modelSrc, &iterChildSrc, iterItemSrc);  /* get the firstchild */
  while(hasItem)
  {
    GtkTreeIter iterNewItem;
    gtkTreeDragDropCopyItem(src, dst, &iterChildSrc, iterItemDst, 2, &iterNewItem);  /* append always */

    /* Recursively transfer all the items */
    gtkTreeDragDropCopyChildren(src, dst, &iterChildSrc, &iterNewItem);  

    /* Go to next sibling item */
    hasItem = gtk_tree_model_iter_next(modelSrc, &iterChildSrc);
  }
}

void iupdrvTreeDragDropCopyNode(Ihandle* src, Ihandle* dst, InodeHandle *itemSrc, InodeHandle *itemDst)
{
  int kind, position = 0;  /* insert after iterItemDst */
  int id_new, count, id_dst;
  GtkTreeIter iterNewItem, iterItemSrc, iterItemDst;
  GtkTreeModel* modelDst = gtk_tree_view_get_model(GTK_TREE_VIEW(dst->handle));

  int old_count = dst->data->node_count;

  gtkTreeIterInit(src, &iterItemSrc, itemSrc);
  gtkTreeIterInit(dst, &iterItemDst, itemDst);

  id_dst = gtkTreeFindNodeId(dst, &iterItemDst);
  id_new = id_dst+1;  /* contains the position for a copy operation */

  gtk_tree_model_get(modelDst, &iterItemDst, IUPGTK_NODE_KIND, &kind, -1);

  if (kind == ITREE_BRANCH)
  {
    GtkTreePath* path = gtk_tree_model_get_path(modelDst, &iterItemDst);
    if (gtk_tree_view_row_expanded(GTK_TREE_VIEW(dst->handle), path))
      position = 1;  /* insert as first child of iterItemDst */
    else
    {
      int child_count = gtkTreeTotalChildCount(dst, &iterItemDst);
      id_new += child_count;
    }
    gtk_tree_path_free(path);
  }

  gtkTreeDragDropCopyItem(src, dst, &iterItemSrc, &iterItemDst, position, &iterNewItem);  

  gtkTreeDragDropCopyChildren(src, dst, &iterItemSrc, &iterNewItem);

  count = dst->data->node_count - old_count;
  iupTreeCopyMoveCache(dst, id_dst, id_new, count, 1);  /* update only the dst control cache */
  gtkTreeRebuildNodeCache(dst, id_new, iterNewItem);
}

/*****************************************************************************/

static int gtkTreeMapMethod(Ihandle* ih)
{
  GtkScrolledWindow* scrolled_window = NULL;
  GtkTreeStore *store;
  GtkCellRenderer *renderer_img, *renderer_txt;
  GtkTreeSelection* selection;
  GtkTreeViewColumn *column;

  store = gtk_tree_store_new(IUPGTK_NODE_LAST_DATA, 
    GDK_TYPE_PIXBUF,                 /* IUPGTK_NODE_IMAGE */
    G_TYPE_BOOLEAN,                  /* IUPGTK_NODE_HAS_IMAGE */
    GDK_TYPE_PIXBUF,                 /* IUPGTK_NODE_IMAGE_EXPANDED */
    G_TYPE_BOOLEAN,                  /* IUPGTK_NODE_HAS_IMAGE_EXPANDED */
    G_TYPE_STRING,                   /* IUPGTK_NODE_TITLE */
    G_TYPE_INT,                      /* IUPGTK_NODE_KIND */
    GDK_TYPE_COLOR,                  /* IUPGTK_NODE_COLOR */
    PANGO_TYPE_FONT_DESCRIPTION,     /* IUPGTK_NODE_FONT */
    G_TYPE_BOOLEAN,                  /* IUPGTK_NODE_SELECTED */
    G_TYPE_BOOLEAN,                  /* IUPGTK_NODE_CHECK */
    G_TYPE_BOOLEAN,                  /* IUPGTK_NODE_3STATE */
    G_TYPE_BOOLEAN);                 /* IUPGTK_NODE_TOGGLEVISIBLE */

  ih->handle = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));

  g_object_unref(store);

  if (!ih->handle)
    return IUP_ERROR;

  scrolled_window = (GtkScrolledWindow*)gtk_scrolled_window_new(NULL, NULL);
  iupAttribSet(ih, "_IUP_EXTRAPARENT", (char*)scrolled_window);

  /* Column and renderers */
  column = gtk_tree_view_column_new();
  iupAttribSet(ih, "_IUPGTK_COLUMN",   (char*)column);

  if(ih->data->show_toggle)
  {
    GtkCellRenderer *renderer_chk = gtk_cell_renderer_toggle_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(column), renderer_chk, FALSE);

    if(ih->data->show_toggle==2)
    {
      gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(column), renderer_chk, "active", IUPGTK_NODE_CHECK, 
                                                                            "inconsistent", IUPGTK_NODE_3STATE, 
                                                                            "visible", IUPGTK_NODE_TOGGLEVISIBLE, 
                                                                            NULL);
      g_signal_connect(G_OBJECT(ih->handle), "button-press-event",  G_CALLBACK(gtkTreeToggle3StateButtonEvent), ih);
      g_signal_connect(G_OBJECT(ih->handle), "button-release-event",G_CALLBACK(gtkTreeToggle3StateButtonEvent), ih);
      g_signal_connect(G_OBJECT(ih->handle), "key-press-event",  G_CALLBACK(gtkTreeToggle3StateKeyEvent), ih);
      g_signal_connect(G_OBJECT(ih->handle), "key-release-event",  G_CALLBACK(gtkTreeToggle3StateKeyEvent), ih);
    }
    else
    {
      gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(column), renderer_chk, "active", IUPGTK_NODE_CHECK, 
                                                                            "visible", IUPGTK_NODE_TOGGLEVISIBLE, 
                                                                            NULL);
      g_signal_connect(G_OBJECT(renderer_chk), "toggled", G_CALLBACK(gtkTreeToggled), ih);
   }

    iupAttribSet(ih, "_IUPGTK_RENDERER_CHECK", (char*)renderer_chk);
  }

  renderer_img = gtk_cell_renderer_pixbuf_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(column), renderer_img, FALSE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(column), renderer_img, "pixbuf", IUPGTK_NODE_IMAGE,
                                                          "pixbuf-expander-open", IUPGTK_NODE_IMAGE_EXPANDED,
                                                        "pixbuf-expander-closed", IUPGTK_NODE_IMAGE, 
                                                            NULL);
  iupAttribSet(ih, "_IUPGTK_RENDERER_IMG", (char*)renderer_img);

  renderer_txt = gtk_cell_renderer_text_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(column), renderer_txt, TRUE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(column), renderer_txt, "text", IUPGTK_NODE_TITLE,
                                                                 "is-expander", IUPGTK_NODE_KIND,
                                                                   "font-desc", IUPGTK_NODE_FONT,
                                                              "foreground-gdk", IUPGTK_NODE_COLOR, 
                                                                  NULL);
  iupAttribSet(ih, "_IUPGTK_RENDERER_TEXT", (char*)renderer_txt);

  if (ih->data->show_rename)
    g_object_set(G_OBJECT(renderer_txt), "editable", TRUE, NULL);

  g_object_set(G_OBJECT(renderer_txt), "xpad", 0, NULL);
  g_object_set(G_OBJECT(renderer_txt), "ypad", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(ih->handle), column);

  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(ih->handle), FALSE);
  gtk_tree_view_set_enable_search(GTK_TREE_VIEW(ih->handle), FALSE);

#if GTK_CHECK_VERSION(2, 10, 0)
  if (iupAttribGetBoolean(ih, "HIDELINES"))
    gtk_tree_view_set_enable_tree_lines(GTK_TREE_VIEW(ih->handle), FALSE);
  else
    gtk_tree_view_set_enable_tree_lines(GTK_TREE_VIEW(ih->handle), TRUE);
#endif

#if GTK_CHECK_VERSION(2, 12, 0)
  if (iupAttribGetBoolean(ih, "HIDEBUTTONS"))
    gtk_tree_view_set_show_expanders(GTK_TREE_VIEW(ih->handle), FALSE);
  else
    gtk_tree_view_set_show_expanders(GTK_TREE_VIEW(ih->handle), TRUE);
#endif

  gtk_container_add((GtkContainer*)scrolled_window, ih->handle);
  gtk_widget_show((GtkWidget*)scrolled_window);
  gtk_scrolled_window_set_shadow_type(scrolled_window, GTK_SHADOW_IN); 

  gtk_scrolled_window_set_policy(scrolled_window, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ih->handle));
  gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
  gtk_tree_selection_set_select_function(selection, (GtkTreeSelectionFunc)gtkTreeSelectionFunc, ih, NULL);

  gtk_tree_view_set_reorderable(GTK_TREE_VIEW(ih->handle), FALSE);

  if (ih->data->show_dragdrop)
    gtkTreeEnableDragDrop(ih);

  /* callbacks */
  g_signal_connect(selection,            "changed", G_CALLBACK(gtkTreeSelectionChanged), ih);
  
  g_signal_connect(renderer_txt, "editing-started", G_CALLBACK(gtkTreeCellTextEditingStarted), ih);
  g_signal_connect(renderer_txt,          "edited", G_CALLBACK(gtkTreeCellTextEdited), ih);

  g_signal_connect(G_OBJECT(ih->handle), "enter-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "leave-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "focus-in-event",     G_CALLBACK(iupgtkFocusInOutEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "focus-out-event",    G_CALLBACK(iupgtkFocusInOutEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "show-help",          G_CALLBACK(iupgtkShowHelp), ih);
  g_signal_connect(G_OBJECT(ih->handle), "motion-notify-event",G_CALLBACK(iupgtkMotionNotifyEvent), ih);

  g_signal_connect(G_OBJECT(ih->handle),    "row-expanded",    G_CALLBACK(gtkTreeRowExpanded), ih);
  g_signal_connect(G_OBJECT(ih->handle),    "test-expand-row", G_CALLBACK(gtkTreeTestExpandRow), ih);
  g_signal_connect(G_OBJECT(ih->handle),  "test-collapse-row", G_CALLBACK(gtkTreeTestCollapseRow), ih);
  g_signal_connect(G_OBJECT(ih->handle),      "row-activated", G_CALLBACK(gtkTreeRowActived), ih);
  g_signal_connect(G_OBJECT(ih->handle),    "key-press-event", G_CALLBACK(gtkTreeKeyPressEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle),  "key-release-event", G_CALLBACK(gtkTreeKeyReleaseEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "button-press-event", G_CALLBACK(gtkTreeButtonEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "button-release-event",G_CALLBACK(gtkTreeButtonEvent), ih);

  /* add to the parent, all GTK controls must call this. */
  iupgtkAddToParent(ih);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    iupgtkSetCanFocus(ih->handle, 0);

  gtk_widget_realize((GtkWidget*)scrolled_window);
  gtk_widget_realize(ih->handle);

  /* Initialize the default images */
  ih->data->def_image_leaf = iupImageGetImage(iupAttribGetStr(ih, "IMAGELEAF"), ih, 0, NULL);
  ih->data->def_image_collapsed = iupImageGetImage(iupAttribGetStr(ih, "IMAGEBRANCHCOLLAPSED"), ih, 0, NULL);
  ih->data->def_image_expanded = iupImageGetImage(iupAttribGetStr(ih, "IMAGEBRANCHEXPANDED"), ih, 0, NULL);

  if (iupAttribGetInt(ih, "ADDROOT"))
    iupdrvTreeAddNode(ih, -1, ITREE_BRANCH, "", 0);

  /* configure for DRAG&DROP of files */
  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSet(ih, "DROPFILESTARGET", "YES");

  IupSetCallback(ih, "_IUP_XY2POS_CB", (Icallback)gtkTreeConvertXYToPos);

  iupdrvTreeUpdateMarkMode(ih);

  /* update a mnemonic in a label if necessary */
  iupgtkUpdateMnemonic(ih);

  return IUP_NOERROR;
}

static void gtkTreeUnMapMethod(Ihandle* ih)
{
  ih->data->node_count = 0;

  iupdrvBaseUnMapMethod(ih);
}

void iupdrvTreeInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = gtkTreeMapMethod;
  ic->UnMap = gtkTreeUnMapMethod;

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, gtkTreeSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, gtkTreeSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);

  /* IupTree Attributes - GENERAL */
  iupClassRegisterAttribute(ic, "EXPANDALL", NULL, gtkTreeSetExpandAllAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INDENTATION", gtkTreeGetIndentationAttrib, gtkTreeSetIndentationAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "SPACING", iupTreeGetSpacingAttrib, gtkTreeSetSpacingAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "TOPITEM", NULL, gtkTreeSetTopItemAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  /* IupTree Attributes - IMAGES */
  iupClassRegisterAttributeId(ic, "IMAGE", NULL, gtkTreeSetImageAttrib, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IMAGEEXPANDED", NULL, gtkTreeSetImageExpandedAttrib, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "IMAGELEAF",            NULL, gtkTreeSetImageLeafAttrib, IUPAF_SAMEASSYSTEM, "IMGLEAF", IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEBRANCHCOLLAPSED", NULL, gtkTreeSetImageBranchCollapsedAttrib, IUPAF_SAMEASSYSTEM, "IMGCOLLAPSED", IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEBRANCHEXPANDED",  NULL, gtkTreeSetImageBranchExpandedAttrib, IUPAF_SAMEASSYSTEM, "IMGEXPANDED", IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);

  /* IupTree Attributes - NODES */
  iupClassRegisterAttributeId(ic, "STATE",  gtkTreeGetStateAttrib,  gtkTreeSetStateAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "DEPTH",  gtkTreeGetDepthAttrib,  NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "KIND",   gtkTreeGetKindAttrib,   NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "PARENT", gtkTreeGetParentAttrib, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "NEXT", gtkTreeGetNextAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "LAST", gtkTreeGetLastAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "PREVIOUS", gtkTreeGetPreviousAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "FIRST", gtkTreeGetFirstAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "COLOR", gtkTreeGetColorAttrib, gtkTreeSetColorAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TITLE",  gtkTreeGetTitleAttrib,  gtkTreeSetTitleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TOGGLEVALUE", gtkTreeGetToggleValueAttrib, gtkTreeSetToggleValueAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TOGGLEVISIBLE", gtkTreeGetToggleVisibleAttrib, gtkTreeSetToggleVisibleAttrib, IUPAF_NO_INHERIT);
  
  /* Change the set method for GTK */
  iupClassRegisterReplaceAttribFunc(ic, "SHOWRENAME", NULL, gtkTreeSetShowRenameAttrib);

  iupClassRegisterAttributeId(ic, "CHILDCOUNT", gtkTreeGetChildCountAttrib, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TITLEFONT",  gtkTreeGetTitleFontAttrib,  gtkTreeSetTitleFontAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ROOTCOUNT", gtkTreeGetRootCountAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);

  /* IupTree Attributes - MARKS */
  iupClassRegisterAttributeId(ic, "MARKED", gtkTreeGetMarkedAttrib, gtkTreeSetMarkedAttrib, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute  (ic, "MARK",      NULL, gtkTreeSetMarkAttrib,      NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute  (ic, "STARTING",  NULL, gtkTreeSetMarkStartAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute  (ic, "MARKSTART", NULL, gtkTreeSetMarkStartAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute  (ic, "MARKEDNODES", gtkTreeGetMarkedNodesAttrib, gtkTreeSetMarkedNodesAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "MARKWHENTOGGLE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute  (ic, "VALUE", gtkTreeGetValueAttrib, gtkTreeSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  /* IupTree Attributes - ACTION */
  iupClassRegisterAttributeId(ic, "DELNODE", NULL, gtkTreeSetDelNodeAttrib, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RENAME", NULL, gtkTreeSetRenameAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "MOVENODE", NULL, gtkTreeSetMoveNodeAttrib, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "COPYNODE", NULL, gtkTreeSetCopyNodeAttrib, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  /* IupTree Attributes - GTK Only */
  iupClassRegisterAttribute  (ic, "RUBBERBAND", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
}
