/** \file
 * \brief Tree Control - FLTK implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Tree.H>
#include <FL/Fl_Tree_Item.H>
#include <FL/Fl_Input.H>
#include <FL/fl_draw.H>

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdint>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_tree.h"
}

#include "iupfltk_drv.h"


/****************************************************************************
 * Helper Functions
 ****************************************************************************/

static Fl_Tree_Item* fltkTreeGetItemFromId(Ihandle* ih, int id)
{
  if (id < 0 || id >= ih->data->node_count)
    return NULL;

  return (Fl_Tree_Item*)ih->data->node_cache[id].node_handle;
}

static int fltkTreeGetIdFromItem(Ihandle* ih, Fl_Tree_Item* item)
{
  if (!item)
    return -1;

  return iupTreeFindNodeId(ih, (InodeHandle*)item);
}

static int fltkTreeGetNodeKind(Fl_Tree_Item* item)
{
  return (int)(intptr_t)item->user_data();
}

static void fltkTreeSetNodeKind(Fl_Tree_Item* item, int kind)
{
  item->user_data((void*)(intptr_t)kind);
}

static int fltkTreeTotalChildCountRec(Fl_Tree_Item* item)
{
  int count = 0;
  for (int i = 0; i < item->children(); i++)
  {
    count++;
    count += fltkTreeTotalChildCountRec(item->child(i));
  }
  return count;
}

static void fltkTreeRebuildNodeCacheRec(Ihandle* ih, Fl_Tree_Item* item, int* id)
{
  for (int i = 0; i < item->children(); i++)
  {
    (*id)++;
    Fl_Tree_Item* child = item->child(i);
    ih->data->node_cache[*id].node_handle = (InodeHandle*)child;
    fltkTreeRebuildNodeCacheRec(ih, child, id);
  }
}

static void fltkTreeRebuildEntireCache(Ihandle* ih)
{
  Fl_Tree* tree = (Fl_Tree*)ih->handle;
  if (!tree)
    return;

  Fl_Tree_Item* root = tree->root();
  if (!root)
    return;

  if (!tree->showroot())
  {
    int id = -1;
    fltkTreeRebuildNodeCacheRec(ih, root, &id);
  }
  else
  {
    int id = 0;
    ih->data->node_cache[0].node_handle = (InodeHandle*)root;
    fltkTreeRebuildNodeCacheRec(ih, root, &id);
  }
}

static int fltkTreeConvertXYToPos(Ihandle* ih, int x, int y)
{
  (void)x;
  (void)y;

  Fl_Tree* tree = (Fl_Tree*)ih->handle;
  Fl_Tree_Item* item = (Fl_Tree_Item*)tree->find_clicked();
  if (item)
    return fltkTreeGetIdFromItem(ih, item);
  return -1;
}

/****************************************************************************
 * Custom Tree Widget
 ****************************************************************************/

static void fltkTreeEndRenameEdit(Ihandle* ih, int apply)
{
  Fl_Input* edit = (Fl_Input*)iupAttribGet(ih, "_IUPFLTK_RENAME_EDIT");
  if (!edit)
    return;

  iupAttribSet(ih, "_IUPFLTK_RENAME_EDIT", NULL);

  Fl_Tree_Item* item = (Fl_Tree_Item*)iupAttribGet(ih, "_IUPFLTK_RENAME_ITEM");
  iupAttribSet(ih, "_IUPFLTK_RENAME_ITEM", NULL);

  if (apply && item)
  {
    const char* new_text = edit->value();
    int id = fltkTreeGetIdFromItem(ih, item);

    IFnis cb = (IFnis)IupGetCallback(ih, "RENAME_CB");
    if (cb)
    {
      if (cb(ih, id, (char*)new_text) != IUP_IGNORE)
        item->label(new_text);
    }
    else
      item->label(new_text);
  }

  Fl_Window* win = edit->window();
  if (win)
  {
    win->remove(edit);
    win->cursor(FL_CURSOR_DEFAULT);
    win->redraw();
  }

  Fl::delete_widget(edit);
}

class IupFltkRenameInput : public Fl_Input
{
public:
  Ihandle* iup_handle;
  int active;

  IupFltkRenameInput(int x, int y, int w, int h, Ihandle* ih)
    : Fl_Input(x, y, w, h), iup_handle(ih), active(0) {}

  int handle(int event) override
  {
    if (event == FL_KEYBOARD)
    {

      if (Fl::event_key() == FL_Escape)
      {
        fltkTreeEndRenameEdit(iup_handle, 0);
        return 1;
      }
      if (Fl::event_key() == FL_Enter || Fl::event_key() == FL_KP_Enter)
      {
        if (active)
        {
          fltkTreeEndRenameEdit(iup_handle, 1);
          return 1;
        }
      }
    }
    else if (event == FL_UNFOCUS)
    {

      if (active)
      {
        fltkTreeEndRenameEdit(iup_handle, 1);
        return 1;
      }
    }
    else
    {

    }
    return Fl_Input::handle(event);
  }
};

static void fltkTreeStartRenameEdit(Ihandle* ih, Fl_Tree_Item* item)
{
  Fl_Tree* tree = (Fl_Tree*)ih->handle;

  if (!tree || !item || !ih->data->show_rename)
    return;

  if (iupAttribGet(ih, "_IUPFLTK_RENAME_EDIT"))
    fltkTreeEndRenameEdit(ih, 1);

  int id = fltkTreeGetIdFromItem(ih, item);

  IFni cbShowRename = (IFni)IupGetCallback(ih, "SHOWRENAME_CB");
  if (cbShowRename && cbShowRename(ih, id) == IUP_IGNORE)
    return;

  int lx = item->label_x();
  int ly = item->label_y();
  int lw = item->label_w();
  int lh = item->label_h();


  if (lw <= 0 || lh <= 0)
    return;

  Fl_Window* win = tree->window();
  if (!win) return;

  win->begin();
  IupFltkRenameInput* edit = new IupFltkRenameInput(lx, ly, lw, lh, ih);
  win->end();

  edit->value(item->label());
  edit->textfont(item->labelfont());
  edit->textsize(item->labelsize());

  iupAttribSet(ih, "_IUPFLTK_RENAME_EDIT", (char*)edit);
  iupAttribSet(ih, "_IUPFLTK_RENAME_ITEM", (char*)item);


  edit->show();
  edit->take_focus();
  edit->insert_position(0, (int)strlen(item->label()));
  edit->active = 1;
}

static void fltkTreeDeferredRename(void* data)
{
  Ihandle* ih = (Ihandle*)data;
  if (!iupObjectCheck(ih)) return;
  if (!ih->handle) return;

  Fl_Tree* tree = (Fl_Tree*)ih->handle;
  Fl_Tree_Item* item = tree->first_selected_item();
  if (item)
    fltkTreeStartRenameEdit(ih, item);
}

class IupFltkTree : public Fl_Tree
{
public:
  Ihandle* iup_handle;
  Fl_Tree_Item* mark_start_node;

  IupFltkTree(int x, int y, int w, int h, Ihandle* ih)
    : Fl_Tree(x, y, w, h), iup_handle(ih), mark_start_node(NULL) {}

protected:
  void draw() override
  {
    Fl_Tree::draw();

    char* dnd_attr = iupAttribGet(iup_handle, "_IUPFLTK_DND_TARGET_LINE");
    if (dnd_attr)
    {
      int dnd_target = atoi(dnd_attr);
      Fl_Tree_Item* item = fltkTreeGetItemFromId(iup_handle, dnd_target);
      if (item)
      {
        int ly = item->label_y();
        int lh = item->label_h();
        int indicator_y = ly + lh;

        fl_color(FL_SELECTION_COLOR);
        fl_line_style(FL_SOLID, 2);
        fl_line(x() + 4, indicator_y, x() + w() - 4, indicator_y);
        fl_line_style(0);
      }
    }
  }

public:
  int handle(int event) override
  {
    switch (event)
    {
      case FL_DND_ENTER: case FL_DND_DRAG: case FL_DND_LEAVE: case FL_DND_RELEASE: case FL_PASTE:
        if (iupfltkDragDropHandleEvent(this, iup_handle, event))
          return 1;
        break;
      case FL_FOCUS:
      case FL_UNFOCUS:
      {
        int ret = Fl_Tree::handle(event);
        iupfltkFocusInOutEvent(this, iup_handle, event);
        return ret;
      }
      case FL_ENTER:
      case FL_LEAVE:
      {
        int ret = Fl_Tree::handle(event);
        iupfltkEnterLeaveEvent(this, iup_handle, event);
        return ret;
      }
      case FL_PUSH:
      {
        iupfltkDragDropHandleEvent(this, iup_handle, event);

        if (Fl::event_button() == FL_RIGHT_MOUSE)
        {
          Fl_Tree_Item* item = (Fl_Tree_Item*)find_clicked();
          if (item)
          {
            IFni cb = (IFni)IupGetCallback(iup_handle, "RIGHTCLICK_CB");
            if (cb)
            {
              int id = fltkTreeGetIdFromItem(iup_handle, item);
              cb(iup_handle, id);
            }
          }
          return 1;
        }

        if (Fl::event_clicks() > 0)
        {
          Fl_Tree_Item* item = (Fl_Tree_Item*)find_clicked();
          if (item)
          {
            int id = fltkTreeGetIdFromItem(iup_handle, item);
            int kind = fltkTreeGetNodeKind(item);

            if (kind == ITREE_LEAF)
            {
              IFni cb = (IFni)IupGetCallback(iup_handle, "EXECUTELEAF_CB");
              if (cb)
                cb(iup_handle, id);
            }
            else
            {
              IFni cb = (IFni)IupGetCallback(iup_handle, "EXECUTEBRANCH_CB");
              if (cb)
                cb(iup_handle, id);
            }
          }
        }

        return Fl_Tree::handle(event);
      }
      case FL_DRAG:
      {
        if (iupfltkDragDropHandleEvent(this, iup_handle, event))
          return 1;
        return Fl_Tree::handle(event);
      }
      case FL_RELEASE:
      {
        iupfltkDragDropHandleEvent(this, iup_handle, event);
        return Fl_Tree::handle(event);
      }
      case FL_KEYBOARD:
      {
        if (iupfltkKeyPressEvent(this, iup_handle))
          return 1;

        if (Fl::event_key() == FL_F + 2 && iup_handle->data->show_rename)
        {
          Fl::add_timeout(0.01, fltkTreeDeferredRename, (void*)iup_handle);
          return 1;
        }

        return Fl_Tree::handle(event);
      }
      default:
        return Fl_Tree::handle(event);
    }
    return Fl_Tree::handle(event);
  }
};

/****************************************************************************
 * Callback Dispatch
 ****************************************************************************/

static void fltkTreeCallback(Fl_Widget* w, void* data)
{
  Ihandle* ih = (Ihandle*)data;
  IupFltkTree* tree = (IupFltkTree*)w;

  if (iupAttribGet(ih, "_IUPTREE_IGNORE_SELECTION_CB"))
    return;

  Fl_Tree_Item* item = tree->callback_item();
  if (!item)
    return;

  int id = fltkTreeGetIdFromItem(ih, item);
  if (id < 0)
    return;

  Fl_Tree_Reason reason = tree->callback_reason();

  switch (reason)
  {
    case FL_TREE_REASON_SELECTED:
    {
      IFnii cb = (IFnii)IupGetCallback(ih, "SELECTION_CB");
      if (cb)
        cb(ih, id, 1);

      if (ih->data->mark_mode == ITREE_MARK_SINGLE)
        tree->mark_start_node = item;
      break;
    }
    case FL_TREE_REASON_DESELECTED:
    {
      IFnii cb = (IFnii)IupGetCallback(ih, "SELECTION_CB");
      if (cb)
        cb(ih, id, 0);
      break;
    }
    case FL_TREE_REASON_OPENED:
    {
      IFni cb = (IFni)IupGetCallback(ih, "BRANCHOPEN_CB");
      if (cb)
      {
        if (cb(ih, id) == IUP_IGNORE)
          tree->close(item, 0);
      }
      break;
    }
    case FL_TREE_REASON_CLOSED:
    {
      IFni cb = (IFni)IupGetCallback(ih, "BRANCHCLOSE_CB");
      if (cb)
      {
        if (cb(ih, id) == IUP_IGNORE)
          tree->open(item, 0);
      }
      break;
    }
    default:
      break;
  }
}

/****************************************************************************
 * Driver Functions
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvTreeAddNode(Ihandle* ih, int id, int kind, const char* title, int add)
{
  IupFltkTree* tree = (IupFltkTree*)ih->handle;
  Fl_Tree_Item* new_item = NULL;
  Fl_Tree_Item* ref_item = NULL;
  int kindPrev = -1;

  if (id >= 0 && id < ih->data->node_count)
  {
    ref_item = fltkTreeGetItemFromId(ih, id);
    if (ref_item)
      kindPrev = fltkTreeGetNodeKind(ref_item);
  }

  if (ref_item)
  {
    if (kindPrev == ITREE_BRANCH && add)
    {
      new_item = ref_item->insert(tree->prefs(), title ? title : "", 0);
    }
    else
    {
      Fl_Tree_Item* parent = ref_item->parent();
      if (parent)
      {
        int index = parent->find_child(ref_item);
        if (index >= 0)
          new_item = parent->insert(tree->prefs(), title ? title : "", index + 1);
        else
          new_item = tree->add(parent, title ? title : "");
      }
    }

    if (new_item)
    {
      fltkTreeSetNodeKind(new_item, kind);
      iupTreeAddToCache(ih, add, kindPrev, (InodeHandle*)ref_item, (InodeHandle*)new_item);
    }
  }
  else
  {
    Fl_Tree_Item* root = tree->root();
    if (!root)
    {
      tree->root_label("");
      root = tree->root();
    }

    new_item = tree->add(root, title ? title : "");

    if (new_item)
    {
      fltkTreeSetNodeKind(new_item, kind);
      iupTreeAddToCache(ih, 0, 0, NULL, (InodeHandle*)new_item);
    }
  }

  if (!new_item)
    return;

  if (kind == ITREE_BRANCH)
  {
    if (ih->data->add_expanded)
      new_item->open();
    else
      new_item->close();
  }

  if (ih->data->node_count == 1)
  {
    tree->mark_start_node = new_item;
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
    tree->set_item_focus(new_item);
    if (ih->data->mark_mode == ITREE_MARK_SINGLE)
      tree->select(new_item, 0);
    else
      new_item->deselect();
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
  }

  fltkTreeRebuildEntireCache(ih);
  tree->redraw();
}

extern "C" IUP_SDK_API InodeHandle* iupdrvTreeGetFocusNode(Ihandle* ih)
{
  if (!ih->handle)
    return NULL;

  IupFltkTree* tree = (IupFltkTree*)ih->handle;
  Fl_Tree_Item* item = tree->get_item_focus();

  return (InodeHandle*)item;
}

extern "C" IUP_SDK_API int iupdrvTreeTotalChildCount(Ihandle* ih, InodeHandle* node_handle)
{
  Fl_Tree_Item* item = (Fl_Tree_Item*)node_handle;
  if (!item)
    return 0;

  (void)ih;
  return fltkTreeTotalChildCountRec(item);
}

extern "C" IUP_SDK_API void iupdrvTreeUpdateMarkMode(Ihandle* ih)
{
  IupFltkTree* tree = (IupFltkTree*)ih->handle;
  if (!tree)
    return;

  if (ih->data->mark_mode == ITREE_MARK_SINGLE)
    tree->selectmode(FL_TREE_SELECT_SINGLE);
  else if (ih->data->mark_mode == ITREE_MARK_MULTIPLE)
    tree->selectmode(FL_TREE_SELECT_MULTI);
}

static Fl_Tree_Item* fltkTreeDragDropCopyItem(Ihandle* src, Ihandle* dst, Fl_Tree_Item* src_item, Fl_Tree_Item* dst_parent, int position, int append)
{
  IupFltkTree* tree_dst = (IupFltkTree*)dst->handle;
  const char* title = src_item->label();
  int kind = fltkTreeGetNodeKind(src_item);

  Fl_Tree_Item* new_item = NULL;

  if (append)
    new_item = tree_dst->add(dst_parent, title ? title : "");
  else
    new_item = dst_parent->insert(tree_dst->prefs(), title ? title : "", position);

  if (!new_item)
    return NULL;

  fltkTreeSetNodeKind(new_item, kind);

  if (src_item->usericon())
    new_item->usericon(src_item->usericon());

  new_item->labelfgcolor(src_item->labelfgcolor());
  new_item->labelbgcolor(src_item->labelbgcolor());
  new_item->labelfont(src_item->labelfont());
  new_item->labelsize(src_item->labelsize());

  if (kind == ITREE_BRANCH)
  {
    if (src_item->is_open())
      new_item->open();
    else
      new_item->close();
  }

  int src_id = fltkTreeGetIdFromItem(src, src_item);
  if (src_id >= 0)
  {
    char* imgexp = iupAttribGetId(src, "_IUPFLTK_IMGEXP", src_id);
    if (imgexp)
    {
      int new_id = dst->data->node_count;
      iupAttribSetStrId(dst, "_IUPFLTK_IMGEXP", new_id, imgexp);
    }
  }

  dst->data->node_count++;

  return new_item;
}

static void fltkTreeDragDropCopyChildren(Ihandle* src, Ihandle* dst, Fl_Tree_Item* src_item, Fl_Tree_Item* dst_item)
{
  for (int i = 0; i < src_item->children(); i++)
  {
    Fl_Tree_Item* child = src_item->child(i);
    Fl_Tree_Item* new_child = fltkTreeDragDropCopyItem(src, dst, child, dst_item, 0, 1);
    if (new_child)
      fltkTreeDragDropCopyChildren(src, dst, child, new_child);
  }
}

extern "C" IUP_SDK_API void iupdrvTreeDragDropCopyNode(Ihandle* src, Ihandle* dst, InodeHandle* itemSrc, InodeHandle* itemDst)
{
  Fl_Tree_Item* src_item = (Fl_Tree_Item*)itemSrc;
  Fl_Tree_Item* dst_item = (Fl_Tree_Item*)itemDst;

  int old_count = dst->data->node_count;

  int id_dst = iupTreeFindNodeId(dst, itemDst);
  int id_new = id_dst + 1;

  int kind = fltkTreeGetNodeKind(dst_item);
  Fl_Tree_Item* new_item = NULL;

  if (kind == ITREE_BRANCH && dst_item->is_open())
  {
    new_item = fltkTreeDragDropCopyItem(src, dst, src_item, dst_item, 0, 0);
  }
  else
  {
    if (kind == ITREE_BRANCH)
    {
      int child_count = iupdrvTreeTotalChildCount(dst, itemDst);
      id_new += child_count;
    }

    Fl_Tree_Item* parent = dst_item->parent();
    if (parent)
    {
      int index = parent->find_child(dst_item);
      new_item = fltkTreeDragDropCopyItem(src, dst, src_item, parent, index + 1, 0);
    }
  }

  if (!new_item)
    return;

  fltkTreeDragDropCopyChildren(src, dst, src_item, new_item);

  int count = dst->data->node_count - old_count;
  iupTreeCopyMoveCache(dst, id_dst, id_new, count, 1);
  fltkTreeRebuildEntireCache(dst);

  IupFltkTree* tree_dst = (IupFltkTree*)dst->handle;
  tree_dst->redraw();
}

/****************************************************************************
 * Attribute Setters/Getters
 ****************************************************************************/

static int fltkTreeSetTitleAttrib(Ihandle* ih, int id, const char* value)
{
  Fl_Tree_Item* item = fltkTreeGetItemFromId(ih, id);

  if (!item && id == 0 && ih->data->node_count == 0)
  {
    IupFltkTree* tree = (IupFltkTree*)ih->handle;
    Fl_Tree_Item* root = tree->root();
    if (!root)
    {
      tree->root_label("");
      root = tree->root();
    }

    item = tree->add(root, value ? value : "");
    if (item)
    {
      fltkTreeSetNodeKind(item, ITREE_BRANCH);
      ih->data->node_count = 1;
      ih->data->node_cache[0].node_handle = (InodeHandle*)item;

      if (ih->data->add_expanded)
        item->open();

      tree->mark_start_node = item;
      tree->set_item_focus(item);
    }

    tree->redraw();
    return 0;
  }

  if (!item)
    return 0;

  if (value)
    item->label(value);

  IupFltkTree* tree = (IupFltkTree*)ih->handle;
  tree->redraw();

  return 0;
}

static char* fltkTreeGetTitleAttrib(Ihandle* ih, int id)
{
  Fl_Tree_Item* item = fltkTreeGetItemFromId(ih, id);
  if (!item)
    return NULL;

  return iupStrReturnStr(item->label());
}

static int fltkTreeSetStateAttrib(Ihandle* ih, int id, const char* value)
{
  Fl_Tree_Item* item = fltkTreeGetItemFromId(ih, id);
  if (!item)
    return 0;

  IupFltkTree* tree = (IupFltkTree*)ih->handle;

  if (iupStrEqualNoCase(value, "EXPANDED"))
    tree->open(item, 0);
  else
    tree->close(item, 0);

  tree->redraw();
  return 0;
}

static char* fltkTreeGetStateAttrib(Ihandle* ih, int id)
{
  Fl_Tree_Item* item = fltkTreeGetItemFromId(ih, id);
  if (!item)
    return NULL;

  if (item->is_open())
    return (char*)"EXPANDED";
  else
    return (char*)"COLLAPSED";
}

static char* fltkTreeGetKindAttrib(Ihandle* ih, int id)
{
  Fl_Tree_Item* item = fltkTreeGetItemFromId(ih, id);
  if (!item)
    return NULL;

  if (fltkTreeGetNodeKind(item) == ITREE_BRANCH)
    return (char*)"BRANCH";
  else
    return (char*)"LEAF";
}

static char* fltkTreeGetDepthAttrib(Ihandle* ih, int id)
{
  Fl_Tree_Item* item = fltkTreeGetItemFromId(ih, id);
  if (!item)
    return NULL;

  int depth = item->depth();

  IupFltkTree* tree = (IupFltkTree*)ih->handle;
  if (!tree->showroot())
    depth--;

  return iupStrReturnInt(depth);
}

static char* fltkTreeGetParentAttrib(Ihandle* ih, int id)
{
  Fl_Tree_Item* item = fltkTreeGetItemFromId(ih, id);
  if (!item)
    return NULL;

  Fl_Tree_Item* parent = item->parent();
  if (!parent)
    return NULL;

  IupFltkTree* tree = (IupFltkTree*)ih->handle;
  if (!tree->showroot() && parent == tree->root())
    return NULL;

  int parent_id = fltkTreeGetIdFromItem(ih, parent);
  if (parent_id < 0)
    return NULL;

  return iupStrReturnInt(parent_id);
}

static char* fltkTreeGetChildCountAttrib(Ihandle* ih, int id)
{
  Fl_Tree_Item* item = fltkTreeGetItemFromId(ih, id);
  if (!item)
    return NULL;

  return iupStrReturnInt(item->children());
}

static char* fltkTreeGetCountAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->node_count);
}

static int fltkTreeSetMarkedAttrib(Ihandle* ih, int id, const char* value)
{
  Fl_Tree_Item* item = fltkTreeGetItemFromId(ih, id);
  if (!item)
    return 0;

  IupFltkTree* tree = (IupFltkTree*)ih->handle;

  if (iupStrBoolean(value))
    tree->select(item, 0);
  else
    tree->deselect(item, 0);

  tree->redraw();
  return 0;
}

static char* fltkTreeGetMarkedAttrib(Ihandle* ih, int id)
{
  Fl_Tree_Item* item = fltkTreeGetItemFromId(ih, id);
  if (!item)
    return NULL;

  return iupStrReturnBoolean(item->is_selected());
}

static int fltkTreeSetImageAttrib(Ihandle* ih, int id, const char* value)
{
  Fl_Tree_Item* item = fltkTreeGetItemFromId(ih, id);
  if (!item)
    return 0;

  if (value)
  {
    Fl_Image* img = (Fl_Image*)iupImageGetImage(value, ih, 0, NULL);
    if (img)
      item->usericon(img);
  }
  else
  {
    item->usericon(NULL);
  }

  IupFltkTree* tree = (IupFltkTree*)ih->handle;
  tree->redraw();
  return 0;
}

static int fltkTreeSetImageExpandedAttrib(Ihandle* ih, int id, const char* value)
{
  iupAttribSetStrId(ih, "_IUPFLTK_IMGEXP", id, value);
  return 0;
}

static int fltkTreeSetColorAttrib(Ihandle* ih, int id, const char* value)
{
  Fl_Tree_Item* item = fltkTreeGetItemFromId(ih, id);
  if (!item)
    return 0;

  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
    item->labelfgcolor(fl_rgb_color(r, g, b));

  IupFltkTree* tree = (IupFltkTree*)ih->handle;
  tree->redraw();
  return 0;
}

static char* fltkTreeGetColorAttrib(Ihandle* ih, int id)
{
  Fl_Tree_Item* item = fltkTreeGetItemFromId(ih, id);
  if (!item)
    return NULL;

  Fl_Color c = item->labelfgcolor();
  unsigned char r, g, b;
  Fl::get_color(c, r, g, b);
  return iupStrReturnRGB(r, g, b);
}

static int fltkTreeSetTitleFontAttrib(Ihandle* ih, int id, const char* value)
{
  Fl_Tree_Item* item = fltkTreeGetItemFromId(ih, id);
  if (!item)
    return 0;

  int fl_font = 0, fl_size = 0;
  if (value)
  {
    iupAttribSetStr(ih, "_IUPFLTK_TMPFONT", value);
    if (iupfltkGetFont(ih, &fl_font, &fl_size))
    {
      item->labelfont((Fl_Font)fl_font);
      item->labelsize((Fl_Fontsize)fl_size);
    }
    iupAttribSet(ih, "_IUPFLTK_TMPFONT", NULL);
  }

  IupFltkTree* tree = (IupFltkTree*)ih->handle;
  tree->redraw();
  return 0;
}

static int fltkTreeSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  IupFltkTree* tree = (IupFltkTree*)ih->handle;
  tree->color(fl_rgb_color(r, g, b));
  tree->redraw();
  return 1;
}

static char* fltkTreeGetBgColorAttrib(Ihandle* ih)
{
  IupFltkTree* tree = (IupFltkTree*)ih->handle;
  Fl_Color c = tree->color();
  unsigned char r, g, b;
  Fl::get_color(c, r, g, b);
  return iupStrReturnStrf("%d %d %d", r, g, b);
}

static int fltkTreeSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  IupFltkTree* tree = (IupFltkTree*)ih->handle;
  tree->item_labelfgcolor(fl_rgb_color(r, g, b));
  tree->redraw();
  return 1;
}

static int fltkTreeSetImageLeafAttrib(Ihandle* ih, const char* value)
{
  ih->data->def_image_leaf = iupImageGetImage(value, ih, 0, NULL);
  return 1;
}

static int fltkTreeSetImageBranchCollapsedAttrib(Ihandle* ih, const char* value)
{
  ih->data->def_image_collapsed = iupImageGetImage(value, ih, 0, NULL);
  return 1;
}

static int fltkTreeSetImageBranchExpandedAttrib(Ihandle* ih, const char* value)
{
  ih->data->def_image_expanded = iupImageGetImage(value, ih, 0, NULL);
  return 1;
}

static int fltkTreeSetTopItemAttrib(Ihandle* ih, const char* value)
{
  int id = 0;
  iupStrToInt(value, &id);
  Fl_Tree_Item* item = fltkTreeGetItemFromId(ih, id);
  if (!item)
    return 0;

  IupFltkTree* tree = (IupFltkTree*)ih->handle;
  tree->show_item_top(item);
  return 0;
}

static int fltkTreeSetSpacingAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)
    return 0;

  int val = 0;
  iupStrToInt(value, &val);

  IupFltkTree* tree = (IupFltkTree*)ih->handle;
  tree->linespacing(val);
  tree->redraw();
  return 0;
}

static int fltkTreeSetValueAttrib(Ihandle* ih, const char* value)
{
  IupFltkTree* tree = (IupFltkTree*)ih->handle;

  if (iupStrEqualNoCase(value, "ROOT") || iupStrEqualNoCase(value, "FIRST"))
  {
    Fl_Tree_Item* item = fltkTreeGetItemFromId(ih, 0);
    if (item)
    {
      tree->deselect_all(NULL, 0);
      tree->select(item, 0);
      tree->set_item_focus(item);
    }
  }
  else if (iupStrEqualNoCase(value, "LAST"))
  {
    if (ih->data->node_count > 0)
    {
      Fl_Tree_Item* item = fltkTreeGetItemFromId(ih, ih->data->node_count - 1);
      if (item)
      {
        tree->deselect_all(NULL, 0);
        tree->select(item, 0);
        tree->set_item_focus(item);
      }
    }
  }
  else if (iupStrEqualNoCase(value, "PGUP"))
  {
    Fl_Tree_Item* focus = tree->get_item_focus();
    if (focus)
    {
      Fl_Tree_Item* item = focus;
      for (int i = 0; i < 10 && item; i++)
        item = tree->prev(item);
      if (!item)
        item = tree->first();
      if (item)
      {
        tree->deselect_all(NULL, 0);
        tree->select(item, 0);
        tree->set_item_focus(item);
      }
    }
  }
  else if (iupStrEqualNoCase(value, "PGDN"))
  {
    Fl_Tree_Item* focus = tree->get_item_focus();
    if (focus)
    {
      Fl_Tree_Item* item = focus;
      for (int i = 0; i < 10 && item; i++)
        item = tree->next(item);
      if (!item)
        item = tree->last();
      if (item)
      {
        tree->deselect_all(NULL, 0);
        tree->select(item, 0);
        tree->set_item_focus(item);
      }
    }
  }
  else if (iupStrEqualNoCase(value, "NEXT"))
  {
    Fl_Tree_Item* focus = tree->get_item_focus();
    if (focus)
    {
      Fl_Tree_Item* item = tree->next(focus);
      if (item)
      {
        tree->deselect_all(NULL, 0);
        tree->select(item, 0);
        tree->set_item_focus(item);
      }
    }
  }
  else if (iupStrEqualNoCase(value, "PREVIOUS"))
  {
    Fl_Tree_Item* focus = tree->get_item_focus();
    if (focus)
    {
      Fl_Tree_Item* item = tree->prev(focus);
      if (item)
      {
        tree->deselect_all(NULL, 0);
        tree->select(item, 0);
        tree->set_item_focus(item);
      }
    }
  }
  else if (iupStrEqualNoCase(value, "CLEAR"))
  {
    tree->deselect_all(NULL, 0);
  }
  else
  {
    int id = 0;
    iupStrToInt(value, &id);
    Fl_Tree_Item* item = fltkTreeGetItemFromId(ih, id);
    if (item)
    {
      tree->deselect_all(NULL, 0);
      tree->select(item, 0);
      tree->set_item_focus(item);
      tree->show_item(item);
    }
  }

  tree->redraw();
  return 0;
}

static char* fltkTreeGetValueAttrib(Ihandle* ih)
{
  IupFltkTree* tree = (IupFltkTree*)ih->handle;
  Fl_Tree_Item* item = tree->get_item_focus();
  if (!item)
    return NULL;

  int id = fltkTreeGetIdFromItem(ih, item);
  if (id < 0)
    return NULL;

  return iupStrReturnInt(id);
}

static int fltkTreeSetMarkAttrib(Ihandle* ih, const char* value)
{
  IupFltkTree* tree = (IupFltkTree*)ih->handle;

  if (iupStrEqualNoCase(value, "BLOCK"))
  {
    Fl_Tree_Item* mark_start = tree->mark_start_node;
    Fl_Tree_Item* current = tree->get_item_focus();

    if (mark_start && current)
    {
      int id_start = fltkTreeGetIdFromItem(ih, mark_start);
      int id_end = fltkTreeGetIdFromItem(ih, current);

      if (id_start > id_end)
      {
        int tmp = id_start;
        id_start = id_end;
        id_end = tmp;
      }

      for (int i = id_start; i <= id_end; i++)
      {
        Fl_Tree_Item* item = fltkTreeGetItemFromId(ih, i);
        if (item)
          tree->select(item, 0);
      }
    }
  }
  else if (iupStrEqualNoCase(value, "CLEARALL"))
  {
    tree->deselect_all(NULL, 0);
  }
  else if (iupStrEqualNoCase(value, "MARKALL"))
  {
    tree->select_all(NULL, 0);
  }
  else if (iupStrEqualNoCase(value, "INVERTALL"))
  {
    for (int i = 0; i < ih->data->node_count; i++)
    {
      Fl_Tree_Item* item = fltkTreeGetItemFromId(ih, i);
      if (item)
      {
        if (item->is_selected())
          tree->deselect(item, 0);
        else
          tree->select(item, 0);
      }
    }
  }

  tree->redraw();
  return 1;
}

static int fltkTreeSetMarkStartAttrib(Ihandle* ih, const char* value)
{
  int id = 0;
  iupStrToInt(value, &id);
  Fl_Tree_Item* item = fltkTreeGetItemFromId(ih, id);
  if (item)
  {
    IupFltkTree* tree = (IupFltkTree*)ih->handle;
    tree->mark_start_node = item;
  }
  return 1;
}

static char* fltkTreeGetMarkedNodesAttrib(Ihandle* ih)
{
  char* str = iupStrGetMemory(ih->data->node_count + 1);

  for (int i = 0; i < ih->data->node_count; i++)
  {
    Fl_Tree_Item* item = fltkTreeGetItemFromId(ih, i);
    if (item && item->is_selected())
      str[i] = '+';
    else
      str[i] = '-';
  }

  str[ih->data->node_count] = 0;
  return str;
}

static int fltkTreeSetMarkedNodesAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  IupFltkTree* tree = (IupFltkTree*)ih->handle;
  int len = (int)strlen(value);

  for (int i = 0; i < ih->data->node_count && i < len; i++)
  {
    Fl_Tree_Item* item = fltkTreeGetItemFromId(ih, i);
    if (item)
    {
      if (value[i] == '+')
        tree->select(item, 0);
      else
        tree->deselect(item, 0);
    }
  }

  tree->redraw();
  return 0;
}

static int fltkTreeSetDelNodeAttrib(Ihandle* ih, int id, const char* value)
{
  if (!ih->handle)
    return 0;

  IupFltkTree* tree = (IupFltkTree*)ih->handle;

  if (iupStrEqualNoCase(value, "ALL"))
  {
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");

    tree->mark_start_node = NULL;

    IFns cb = (IFns)IupGetCallback(ih, "NODEREMOVED_CB");
    if (cb)
    {
      for (int i = 0; i < ih->data->node_count; i++)
        cb(ih, (char*)ih->data->node_cache[i].userdata);
    }

    tree->clear();
    iupTreeDelFromCache(ih, 0, ih->data->node_count);

    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
    return 0;
  }

  if (iupStrEqualNoCase(value, "SELECTED"))
  {
    Fl_Tree_Item* item = fltkTreeGetItemFromId(ih, id);
    if (!item)
      return 0;

    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");

    IFns cb = (IFns)IupGetCallback(ih, "NODEREMOVED_CB");
    if (cb)
    {
      int count = 1 + iupdrvTreeTotalChildCount(ih, (InodeHandle*)item);
      for (int i = id + count - 1; i >= id; i--)
      {
        if (i < ih->data->node_count)
          cb(ih, (char*)ih->data->node_cache[i].userdata);
      }
    }

    int count = 1 + iupdrvTreeTotalChildCount(ih, (InodeHandle*)item);

    if (tree->mark_start_node)
    {
      int ms_id = fltkTreeGetIdFromItem(ih, tree->mark_start_node);
      if (ms_id >= id && ms_id < id + count)
        tree->mark_start_node = NULL;
    }

    tree->remove(item);
    iupTreeDelFromCache(ih, id, count);
    fltkTreeRebuildEntireCache(ih);

    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
    tree->redraw();
    return 0;
  }

  if (iupStrEqualNoCase(value, "CHILDREN"))
  {
    Fl_Tree_Item* item = fltkTreeGetItemFromId(ih, id);
    if (!item)
      return 0;

    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");

    while (item->children() > 0)
    {
      Fl_Tree_Item* child = item->child(0);
      int child_id = fltkTreeGetIdFromItem(ih, child);

      IFns cb = (IFns)IupGetCallback(ih, "NODEREMOVED_CB");
      if (cb)
      {
        int count = 1 + iupdrvTreeTotalChildCount(ih, (InodeHandle*)child);
        for (int i = child_id + count - 1; i >= child_id; i--)
        {
          if (i < ih->data->node_count)
            cb(ih, (char*)ih->data->node_cache[i].userdata);
        }
      }

      int count = 1 + iupdrvTreeTotalChildCount(ih, (InodeHandle*)child);

      if (tree->mark_start_node)
      {
        int ms_id = fltkTreeGetIdFromItem(ih, tree->mark_start_node);
        if (ms_id >= child_id && ms_id < child_id + count)
          tree->mark_start_node = NULL;
      }

      tree->remove(child);
      iupTreeDelFromCache(ih, child_id, count);
    }

    fltkTreeRebuildEntireCache(ih);

    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
    tree->redraw();
    return 0;
  }

  if (iupStrEqualNoCase(value, "MARKED"))
  {
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");

    for (int i = ih->data->node_count - 1; i >= 0; i--)
    {
      Fl_Tree_Item* item = fltkTreeGetItemFromId(ih, i);
      if (item && item->is_selected())
      {
        IFns cb = (IFns)IupGetCallback(ih, "NODEREMOVED_CB");
        if (cb)
        {
          int count = 1 + iupdrvTreeTotalChildCount(ih, (InodeHandle*)item);
          for (int j = i + count - 1; j >= i; j--)
          {
            if (j < ih->data->node_count)
              cb(ih, (char*)ih->data->node_cache[j].userdata);
          }
        }

        int count = 1 + iupdrvTreeTotalChildCount(ih, (InodeHandle*)item);

        if (tree->mark_start_node)
        {
          int ms_id = fltkTreeGetIdFromItem(ih, tree->mark_start_node);
          if (ms_id >= i && ms_id < i + count)
            tree->mark_start_node = NULL;
        }

        tree->remove(item);
        iupTreeDelFromCache(ih, i, count);
      }
    }

    fltkTreeRebuildEntireCache(ih);

    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
    tree->redraw();
    return 0;
  }

  return 0;
}

static int fltkTreeSetExpandAllAttrib(Ihandle* ih, const char* value)
{
  IupFltkTree* tree = (IupFltkTree*)ih->handle;

  if (iupStrBoolean(value))
  {
    for (Fl_Tree_Item* item = tree->first(); item; item = tree->next(item))
    {
      if (fltkTreeGetNodeKind(item) == ITREE_BRANCH)
        tree->open(item, 0);
    }
  }
  else
  {
    for (Fl_Tree_Item* item = tree->first(); item; item = tree->next(item))
    {
      if (fltkTreeGetNodeKind(item) == ITREE_BRANCH)
        tree->close(item, 0);
    }
  }

  tree->redraw();
  return 0;
}

static int fltkTreeSetRenameAttrib(Ihandle* ih, const char* value)
{
  (void)value;

  if (!ih->data->show_rename)
    return 0;

  Fl::add_timeout(0.01, fltkTreeDeferredRename, (void*)ih);
  return 0;
}

static int fltkTreeSetShowRenameAttrib(Ihandle* ih, const char* value)
{
  ih->data->show_rename = iupStrBoolean(value);
  return 0;
}

static char* fltkTreeGetNextAttrib(Ihandle* ih, int id)
{
  if (id + 1 < ih->data->node_count)
    return iupStrReturnInt(id + 1);
  return NULL;
}

static char* fltkTreeGetPreviousAttrib(Ihandle* ih, int id)
{
  if (id > 0)
    return iupStrReturnInt(id - 1);
  return NULL;
}

static char* fltkTreeGetFirstAttrib(Ihandle* ih, int id)
{
  Fl_Tree_Item* item = fltkTreeGetItemFromId(ih, id);
  if (!item || item->children() == 0)
    return NULL;

  Fl_Tree_Item* child = item->child(0);
  int child_id = fltkTreeGetIdFromItem(ih, child);
  if (child_id < 0)
    return NULL;

  return iupStrReturnInt(child_id);
}

static char* fltkTreeGetLastAttrib(Ihandle* ih, int id)
{
  Fl_Tree_Item* item = fltkTreeGetItemFromId(ih, id);
  if (!item)
    return NULL;

  Fl_Tree_Item* last = item;
  while (last->children() > 0)
    last = last->child(last->children() - 1);

  if (last == item)
    return NULL;

  int last_id = fltkTreeGetIdFromItem(ih, last);
  if (last_id < 0)
    return NULL;

  return iupStrReturnInt(last_id);
}

static char* fltkTreeGetRootCountAttrib(Ihandle* ih)
{
  if (!ih->handle)
    return NULL;

  IupFltkTree* tree = (IupFltkTree*)ih->handle;
  Fl_Tree_Item* root = tree->root();
  if (!root)
    return iupStrReturnInt(0);

  if (tree->showroot())
    return iupStrReturnInt(1);
  else
    return iupStrReturnInt(root->children());
}

static int fltkTreeSetIndentationAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)
    return 0;

  int indent = 0;
  iupStrToInt(value, &indent);

  IupFltkTree* tree = (IupFltkTree*)ih->handle;
  tree->connectorwidth(indent);
  tree->redraw();
  return 0;
}

static char* fltkTreeGetIndentationAttrib(Ihandle* ih)
{
  if (!ih->handle)
    return NULL;

  IupFltkTree* tree = (IupFltkTree*)ih->handle;
  return iupStrReturnInt(tree->connectorwidth());
}

static int fltkTreeSetTitleFgColorAttrib(Ihandle* ih, int id, const char* value)
{
  return fltkTreeSetColorAttrib(ih, id, value);
}

static int fltkTreeSetTitleBgColorAttrib(Ihandle* ih, int id, const char* value)
{
  Fl_Tree_Item* item = fltkTreeGetItemFromId(ih, id);
  if (!item)
    return 0;

  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
    item->labelbgcolor(fl_rgb_color(r, g, b));

  IupFltkTree* tree = (IupFltkTree*)ih->handle;
  tree->redraw();
  return 0;
}

/****************************************************************************
 * Map / UnMap
 ****************************************************************************/

static int fltkTreeMapMethod(Ihandle* ih)
{
  IupFltkTree* tree = new IupFltkTree(0, 0, 10, 10, ih);
  tree->end();

  ih->handle = (InativeHandle*)tree;

  tree->showroot(0);
  tree->connectorstyle(FL_TREE_CONNECTOR_DOTTED);
  tree->selectmode(FL_TREE_SELECT_SINGLE);
  tree->item_reselect_mode(FL_TREE_SELECTABLE_ALWAYS);
  tree->when(FL_WHEN_CHANGED);

  iupdrvTreeUpdateMarkMode(ih);

  tree->callback(fltkTreeCallback, (void*)ih);

  iupfltkAddToParent(ih);

  int indent = iupAttribGetInt(ih, "INDENTATION");
  if (indent > 0)
    tree->connectorwidth(indent);

  int spacing = ih->data->spacing;
  if (spacing > 0)
    tree->linespacing(spacing);

  IupSetCallback(ih, "_IUP_XY2POS_CB", (Icallback)fltkTreeConvertXYToPos);

  return IUP_NOERROR;
}

static void fltkTreeUnMapMethod(Ihandle* ih)
{
  IupFltkTree* tree = (IupFltkTree*)ih->handle;
  if (tree)
  {
    delete tree;
    ih->handle = NULL;
  }

  ih->data->node_count = 0;
}

static void fltkTreeLayoutUpdateMethod(Ihandle* ih)
{
  iupdrvBaseLayoutUpdateMethod(ih);
}

/****************************************************************************
 * Class Registration
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvTreeInitClass(Iclass* ic)
{
  ic->Map = fltkTreeMapMethod;
  ic->UnMap = fltkTreeUnMapMethod;
  ic->LayoutUpdate = fltkTreeLayoutUpdateMethod;

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", fltkTreeGetBgColorAttrib, fltkTreeSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, fltkTreeSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);

  /* IupTree Attributes - GENERAL */
  iupClassRegisterAttribute(ic, "SHOWRENAME", NULL, fltkTreeSetShowRenameAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "RENAME", NULL, fltkTreeSetRenameAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOPITEM", NULL, fltkTreeSetTopItemAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COUNT", fltkTreeGetCountAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ROOTCOUNT", fltkTreeGetRootCountAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EXPANDALL", NULL, fltkTreeSetExpandAllAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INDENTATION", fltkTreeGetIndentationAttrib, fltkTreeSetIndentationAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "SPACING", iupTreeGetSpacingAttrib, fltkTreeSetSpacingAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "EMPTYAS3STATE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKWHENTOGGLE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  /* IupTree Attributes - IMAGES */
  iupClassRegisterAttribute(ic, "IMAGELEAF", NULL, fltkTreeSetImageLeafAttrib, IUPAF_SAMEASSYSTEM, "IMGLEAF", IUPAF_IHANDLENAME | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEBRANCHCOLLAPSED", NULL, fltkTreeSetImageBranchCollapsedAttrib, IUPAF_SAMEASSYSTEM, "IMGCOLLAPSED", IUPAF_IHANDLENAME | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEBRANCHEXPANDED", NULL, fltkTreeSetImageBranchExpandedAttrib, IUPAF_SAMEASSYSTEM, "IMGEXPANDED", IUPAF_IHANDLENAME | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  /* IupTree Attributes - NODES */
  iupClassRegisterAttributeId(ic, "STATE", fltkTreeGetStateAttrib, fltkTreeSetStateAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "DEPTH", fltkTreeGetDepthAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "KIND", fltkTreeGetKindAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "PARENT", fltkTreeGetParentAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "NEXT", fltkTreeGetNextAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "PREVIOUS", fltkTreeGetPreviousAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "FIRST", fltkTreeGetFirstAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "LAST", fltkTreeGetLastAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "CHILDCOUNT", fltkTreeGetChildCountAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TITLE", fltkTreeGetTitleAttrib, fltkTreeSetTitleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TITLEFONT", NULL, fltkTreeSetTitleFontAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TITLEFGCOLOR", NULL, fltkTreeSetTitleFgColorAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TITLEBGCOLOR", NULL, fltkTreeSetTitleBgColorAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "COLOR", fltkTreeGetColorAttrib, fltkTreeSetColorAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IMAGE", NULL, fltkTreeSetImageAttrib, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IMAGEEXPANDED", NULL, fltkTreeSetImageExpandedAttrib, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);

  /* IupTree Attributes - MARKS */
  iupClassRegisterAttributeId(ic, "MARKED", fltkTreeGetMarkedAttrib, fltkTreeSetMarkedAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARK", NULL, fltkTreeSetMarkAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STARTING", NULL, fltkTreeSetMarkStartAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKSTART", NULL, fltkTreeSetMarkStartAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKEDNODES", fltkTreeGetMarkedNodesAttrib, fltkTreeSetMarkedNodesAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", fltkTreeGetValueAttrib, fltkTreeSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  /* IupTree Attributes - TOGGLE */
  iupClassRegisterAttributeId(ic, "TOGGLEVALUE", NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TOGGLEVISIBLE", NULL, NULL, IUPAF_NO_INHERIT);

  /* IupTree Attributes - ACTION */
  iupClassRegisterAttribute(ic, "ADDROOT", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "DELNODE", NULL, fltkTreeSetDelNodeAttrib, IUPAF_NOT_MAPPED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "COPYNODE", NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "MOVENODE", NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "RUBBERBAND", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
}
