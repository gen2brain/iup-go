/** \file
 * \brief Tree Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_array.h"
#include "iup_tree.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_drvinfo.h"

#include "iupefl_drv.h"


typedef struct _IeflTreeNode
{
  Ihandle* ih;
  char* title;
  int kind;
  int selected;
  int expanded;
  void* userdata;
  Elm_Object_Item* item;

  char* image;
  char* image_expanded;
  int has_image;
  int has_image_expanded;
  unsigned char color[4];
  int has_color;
  char* font;
  int toggle_value;
  int toggle_visible;

  Eina_List* hidden_children;
  int dont_free;
} IeflTreeNode;

static Elm_Genlist_Item_Class* efl_tree_branch_itc = NULL;
static Elm_Genlist_Item_Class* efl_tree_leaf_itc = NULL;

static void eflTreeStartRenameEdit(Ihandle* ih, Elm_Object_Item* item);
static void eflTreeEndRenameEdit(Ihandle* ih, int apply);

static void eflTreeCopyNodeData(IeflTreeNode* src, IeflTreeNode* dst);
static void eflTreeCopyChildren(Ihandle* ih, Eo* tree, Elm_Object_Item* src_item, Elm_Object_Item* dst_parent);

static char* eflTreeTextGet(void* data, Evas_Object* obj, const char* part)
{
  IeflTreeNode* node = (IeflTreeNode*)data;
  (void)obj;
  (void)part;

  if (node && node->title)
    return strdup(node->title);

  return NULL;
}

static Evas_Object* eflTreeContentGet(void* data, Evas_Object* obj, const char* part)
{
  IeflTreeNode* node = (IeflTreeNode*)data;
  Ihandle* ih;
  const char* image_name = NULL;
  const char* theme_icon = NULL;
  Evas_Object* img;

  if (!node || !node->ih)
    return NULL;

  ih = node->ih;

  if (strcmp(part, "elm.swallow.icon") != 0)
    return NULL;

  if (node->has_image && node->image)
  {
    image_name = node->image;
  }
  else if (node->kind == ITREE_BRANCH)
  {
    if (node->expanded)
    {
      image_name = iupAttribGet(ih, "_EFL_IMAGEBRANCHEXPANDED");
      theme_icon = "folder-open";
    }
    else
    {
      image_name = iupAttribGet(ih, "_EFL_IMAGEBRANCHCOLLAPSED");
      theme_icon = "folder";
    }
  }
  else
  {
    image_name = iupAttribGet(ih, "_EFL_IMAGELEAF");
    theme_icon = "file";
  }

  if (image_name && image_name[0])
  {
    img = iupeflImageGetImageForParent(image_name, ih, 0, obj);
    if (img)
    {
      evas_object_size_hint_aspect_set(img, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
      return img;
    }
  }

  if (theme_icon)
  {
    img = elm_icon_add(obj);
    if (img && elm_icon_standard_set(img, theme_icon))
    {
      evas_object_size_hint_aspect_set(img, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
      return img;
    }
    if (img)
      evas_object_del(img);
  }

  return NULL;
}

static void eflTreeItemDel(void* data, Evas_Object* obj)
{
  IeflTreeNode* node = (IeflTreeNode*)data;
  (void)obj;

  if (node)
  {
    if (node->dont_free)
    {
      node->item = NULL;
      return;
    }

    if (node->title)
      free(node->title);
    if (node->font)
      free(node->font);
    if (node->image)
      free(node->image);
    if (node->image_expanded)
      free(node->image_expanded);
    if (node->hidden_children)
      eina_list_free(node->hidden_children);
    free(node);
  }
}

static void eflTreeInitItemClasses(void)
{
  if (!efl_tree_branch_itc)
  {
    efl_tree_branch_itc = elm_genlist_item_class_new();
    efl_tree_branch_itc->item_style = "default";
    efl_tree_branch_itc->func.text_get = eflTreeTextGet;
    efl_tree_branch_itc->func.content_get = eflTreeContentGet;
    efl_tree_branch_itc->func.state_get = NULL;
    efl_tree_branch_itc->func.del = eflTreeItemDel;
  }

  if (!efl_tree_leaf_itc)
  {
    efl_tree_leaf_itc = elm_genlist_item_class_new();
    efl_tree_leaf_itc->item_style = "default";
    efl_tree_leaf_itc->func.text_get = eflTreeTextGet;
    efl_tree_leaf_itc->func.content_get = eflTreeContentGet;
    efl_tree_leaf_itc->func.state_get = NULL;
    efl_tree_leaf_itc->func.del = eflTreeItemDel;
  }
}

/****************************************************************
                     Callbacks
****************************************************************/

static void eflTreeSelectedCallback(void* data, Evas_Object* obj, void* event_info)
{
  Ihandle* ih = (Ihandle*)data;
  Elm_Object_Item* item = (Elm_Object_Item*)event_info;
  IFnii cb;
  int id;

  (void)obj;

  if (iupAttribGet(ih, "_IUP_EFL_IGNORE_SELECTION"))
    return;

  if (!item)
    return;

  iupAttribSet(ih, "_IUP_EFL_PENDING_RENAME_ITEM", NULL);

  id = iupTreeFindNodeId(ih, (InodeHandle*)item);

  cb = (IFnii)IupGetCallback(ih, "SELECTION_CB");
  if (cb)
  {
    IeflTreeNode* node = (IeflTreeNode*)elm_object_item_data_get(item);
    cb(ih, id, 1);
    if (node)
      node->selected = 1;
  }
}

static void eflTreeUnselectedCallback(void* data, Evas_Object* obj, void* event_info)
{
  Ihandle* ih = (Ihandle*)data;
  Elm_Object_Item* item = (Elm_Object_Item*)event_info;
  IFnii cb;
  int id;

  (void)obj;

  if (iupAttribGet(ih, "_IUP_EFL_IGNORE_SELECTION"))
    return;

  if (!item)
    return;

  id = iupTreeFindNodeId(ih, (InodeHandle*)item);

  cb = (IFnii)IupGetCallback(ih, "SELECTION_CB");
  if (cb)
  {
    IeflTreeNode* node = (IeflTreeNode*)elm_object_item_data_get(item);
    cb(ih, id, 0);
    if (node)
      node->selected = 0;
  }
}

static void eflTreeExpandRequestCallback(void* data, Evas_Object* obj, void* event_info)
{
  Elm_Object_Item* item = (Elm_Object_Item*)event_info;

  (void)data;
  (void)obj;

  if (item && !elm_genlist_item_expanded_get(item))
    elm_genlist_item_expanded_set(item, EINA_TRUE);
}

static void eflTreeContractRequestCallback(void* data, Evas_Object* obj, void* event_info)
{
  Elm_Object_Item* item = (Elm_Object_Item*)event_info;

  (void)data;
  (void)obj;

  if (item && elm_genlist_item_expanded_get(item))
    elm_genlist_item_expanded_set(item, EINA_FALSE);
}

static void eflTreeRestoreChildren(Evas_Object* tree, Elm_Object_Item* parent_item, Eina_List* children)
{
  Eina_List* l;
  IeflTreeNode* child_node;
  Elm_Genlist_Item_Class* itc;

  EINA_LIST_FOREACH(children, l, child_node)
  {
    itc = (child_node->kind == ITREE_BRANCH) ? efl_tree_branch_itc : efl_tree_leaf_itc;

    child_node->dont_free = 0;
    child_node->item = elm_genlist_item_append(tree, itc, child_node, parent_item,
                                                (child_node->kind == ITREE_BRANCH) ? ELM_GENLIST_ITEM_TREE : ELM_GENLIST_ITEM_NONE,
                                                NULL, NULL);

    if (child_node->kind == ITREE_BRANCH && child_node->hidden_children)
    {
      elm_genlist_item_expanded_set(child_node->item, EINA_TRUE);
    }
  }
}

static void eflTreeExpandedCallback(void* data, Evas_Object* obj, void* event_info)
{
  Ihandle* ih = (Ihandle*)data;
  Elm_Object_Item* item = (Elm_Object_Item*)event_info;
  Evas_Object* tree = (Evas_Object*)obj;
  IFni cb;
  int id;
  IeflTreeNode* node;

  if (!item)
    return;

  if (iupAttribGet(ih, "_IUP_EFL_IGNORE_BRANCH_CB"))
    return;

  id = iupTreeFindNodeId(ih, (InodeHandle*)item);

  cb = (IFni)IupGetCallback(ih, "BRANCHOPEN_CB");
  if (cb)
    cb(ih, id);

  node = (IeflTreeNode*)elm_object_item_data_get(item);
  if (node)
  {
    node->expanded = 1;

    if (node->hidden_children)
    {
      eflTreeRestoreChildren(tree, item, node->hidden_children);
      eina_list_free(node->hidden_children);
      node->hidden_children = NULL;
    }
  }
}

static void eflTreeCollectChildren(Elm_Object_Item* parent_item, Eina_List** list)
{
  const Eina_List* subitems = elm_genlist_item_subitems_get(parent_item);
  const Eina_List* l;
  Elm_Object_Item* child_item;

  EINA_LIST_FOREACH(subitems, l, child_item)
  {
    IeflTreeNode* child_node = (IeflTreeNode*)elm_object_item_data_get(child_item);
    if (child_node)
    {
      if (child_node->kind == ITREE_BRANCH && child_node->expanded)
        eflTreeCollectChildren(child_item, &child_node->hidden_children);

      child_node->dont_free = 1;
      *list = eina_list_append(*list, child_node);
    }
  }
}

static void eflTreeContractedCallback(void* data, Evas_Object* obj, void* event_info)
{
  Ihandle* ih = (Ihandle*)data;
  Elm_Object_Item* item = (Elm_Object_Item*)event_info;
  IFni cb;
  int id;
  IeflTreeNode* node;

  (void)obj;

  if (!item)
    return;

  if (iupAttribGet(ih, "_IUP_EFL_IGNORE_BRANCH_CB"))
    return;

  id = iupTreeFindNodeId(ih, (InodeHandle*)item);

  cb = (IFni)IupGetCallback(ih, "BRANCHCLOSE_CB");
  if (cb)
    cb(ih, id);

  node = (IeflTreeNode*)elm_object_item_data_get(item);
  if (node)
  {
    node->expanded = 0;

    if (node->hidden_children)
    {
      eina_list_free(node->hidden_children);
      node->hidden_children = NULL;
    }
    eflTreeCollectChildren(item, &node->hidden_children);
  }

  elm_genlist_item_subitems_clear(item);
}

static void eflTreeDoubleClickCallback(void* data, Evas_Object* obj, void* event_info)
{
  Ihandle* ih = (Ihandle*)data;
  Elm_Object_Item* item = (Elm_Object_Item*)event_info;
  IFni cb_leaf, cb_branch;
  int id;

  (void)obj;

  if (!item)
    return;

  id = iupTreeFindNodeId(ih, (InodeHandle*)item);

  {
    IeflTreeNode* node = (IeflTreeNode*)elm_object_item_data_get(item);
    if (node)
    {
      if (node->kind == ITREE_LEAF)
      {
        cb_leaf = (IFni)IupGetCallback(ih, "EXECUTELEAF_CB");
        if (cb_leaf)
          cb_leaf(ih, id);
      }
      else
      {
        cb_branch = (IFni)IupGetCallback(ih, "EXECUTEBRANCH_CB");
        if (cb_branch)
          cb_branch(ih, id);
      }
    }
  }
}

static void eflTreeRightClickCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Efl_Input_Pointer* pointer = ev->info;
  IFni cb;
  Elm_Object_Item* item;
  Eina_Position2D pos;
  int button;

  button = efl_input_pointer_button_get(pointer);
  if (button != 3)
    return;

  pos = efl_input_pointer_position_get(pointer);

  item = elm_genlist_at_xy_item_get(ev->object, pos.x, pos.y, NULL);
  if (item)
  {
    int id = iupTreeFindNodeId(ih, (InodeHandle*)item);
    cb = (IFni)IupGetCallback(ih, "RIGHTCLICK_CB");
    if (cb)
      cb(ih, id);
  }
}

static void eflTreeKeyDownCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Efl_Input_Key* key = ev->info;
  Elm_Object_Item* item;
  const char* keyname;

  keyname = efl_input_key_name_get(key);
  if (!keyname)
    return;

  if (iupAttribGet(ih, "_IUP_EFL_EDITING_ENTRY"))
    return;

  if (strcmp(keyname, "F2") == 0 && ih->data->show_rename)
  {
    item = elm_genlist_selected_item_get(ev->object);
    if (item)
    {
      eflTreeStartRenameEdit(ih, item);
      efl_input_processed_set(key, EINA_TRUE);
    }
  }
}

static void eflTreeClickCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Efl_Input_Pointer* pointer = ev->info;
  Elm_Object_Item* item;
  Elm_Object_Item* selected;
  Eina_Position2D pos;
  int button;

  button = efl_input_pointer_button_get(pointer);
  if (button != 1)
    return;

  if (!ih->data->show_rename)
    return;

  if (iupAttribGet(ih, "_IUP_EFL_EDITING_ENTRY"))
    return;

  pos = efl_input_pointer_position_get(pointer);

  item = elm_genlist_at_xy_item_get(ev->object, pos.x, pos.y, NULL);
  if (!item)
    return;

  selected = elm_genlist_selected_item_get(ev->object);
  if (item == selected)
  {
    Elm_Object_Item* pending = (Elm_Object_Item*)iupAttribGet(ih, "_IUP_EFL_PENDING_RENAME_ITEM");
    if (pending == item)
    {
      iupAttribSet(ih, "_IUP_EFL_PENDING_RENAME_ITEM", NULL);
      eflTreeStartRenameEdit(ih, item);
      efl_input_processed_set(pointer, EINA_TRUE);
    }
    else
    {
      iupAttribSet(ih, "_IUP_EFL_PENDING_RENAME_ITEM", (char*)item);
    }
  }
  else
  {
    iupAttribSet(ih, "_IUP_EFL_PENDING_RENAME_ITEM", NULL);
  }
}

/****************************************************************
                     Rename Editing Support
****************************************************************/

static void eflTreeRenameActivatedCallback(void* data, Evas_Object* obj, void* event_info)
{
  Ihandle* ih = (Ihandle*)data;
  (void)obj;
  (void)event_info;

  if (!iupObjectCheck(ih))
    return;

  eflTreeEndRenameEdit(ih, 1);
}

static void eflTreeRenameAbortedCallback(void* data, Evas_Object* obj, void* event_info)
{
  Ihandle* ih = (Ihandle*)data;
  (void)obj;
  (void)event_info;

  if (!iupObjectCheck(ih))
    return;

  eflTreeEndRenameEdit(ih, 0);
}

static void eflTreeRenameFocusOutCallback(void* data, Evas_Object* obj, void* event_info)
{
  Ihandle* ih = (Ihandle*)data;
  (void)obj;
  (void)event_info;

  if (!iupObjectCheck(ih))
    return;

  if (!iupAttribGet(ih, "_IUP_EFL_EDITING_ENTRY"))
    return;

  eflTreeEndRenameEdit(ih, 1);
}

static void eflTreeStartRenameEdit(Ihandle* ih, Elm_Object_Item* item)
{
  Eo* tree = iupeflGetWidget(ih);
  IeflTreeNode* node;
  Evas_Object* track_obj;
  Evas_Object* entry;
  Evas_Object* icon;
  IFni cbShowRename;
  int id;
  int x, y, w, h;
  int text_x, text_w;
  const char* value;

  if (!tree || !item || !ih->data->show_rename)
    return;

  if (iupAttribGet(ih, "_IUP_EFL_EDITING_ENTRY"))
    eflTreeEndRenameEdit(ih, 1);

  node = (IeflTreeNode*)elm_object_item_data_get(item);
  if (!node)
    return;

  id = iupTreeFindNodeId(ih, (InodeHandle*)item);

  cbShowRename = (IFni)IupGetCallback(ih, "SHOWRENAME_CB");
  if (cbShowRename && cbShowRename(ih, id) == IUP_IGNORE)
    return;

  track_obj = elm_object_item_track(item);
  if (!track_obj)
    return;

  evas_object_geometry_get(track_obj, &x, &y, &w, &h);
  elm_object_item_untrack(item);

  text_x = x;
  text_w = w;

  icon = elm_object_item_part_content_get(item, "elm.swallow.icon");
  if (icon)
  {
    int icon_x, icon_y, iw, ih_icon;
    evas_object_geometry_get(icon, &icon_x, &icon_y, &iw, &ih_icon);
    text_x = icon_x + iw + 4;
    text_w = w - (text_x - x);
  }

  iupAttribSetStr(ih, "_IUP_EFL_ORIGINAL_TITLE", node->title);

  if (node->title)
    free(node->title);
  node->title = strdup("");
  elm_genlist_item_update(item);

  entry = elm_entry_add(elm_object_top_widget_get(tree));
  elm_entry_single_line_set(entry, EINA_TRUE);
  elm_entry_editable_set(entry, EINA_TRUE);
  elm_entry_scrollable_set(entry, EINA_FALSE);
  elm_entry_context_menu_disabled_set(entry, EINA_TRUE);

  {
    const char* saved_title = iupAttribGet(ih, "_IUP_EFL_ORIGINAL_TITLE");
    elm_object_text_set(entry, saved_title ? saved_title : "");
  }

  evas_object_move(entry, text_x, y);
  evas_object_resize(entry, text_w, h);
  evas_object_show(entry);

  elm_object_focus_set(entry, EINA_TRUE);

  value = iupAttribGetStr(ih, "RENAMECARET");
  if (value)
  {
    int pos = 1;
    if (iupStrToInt(value, &pos))
    {
      if (pos < 1) pos = 1;
      pos--;
      elm_entry_cursor_pos_set(entry, pos);
    }
  }

  value = iupAttribGetStr(ih, "RENAMESELECTION");
  if (value)
  {
    int start = 1, end = 1;
    if (iupStrToIntInt(value, &start, &end, ':') == 2)
    {
      if (start < 1) start = 1;
      if (end < 1) end = 1;
      start--;
      end--;
      elm_entry_select_region_set(entry, start, end);
    }
  }
  else
  {
    elm_entry_select_all(entry);
  }

  iupAttribSet(ih, "_IUP_EFL_EDITING_ENTRY", (char*)entry);
  iupAttribSet(ih, "_IUP_EFL_EDITING_ITEM", (char*)item);

  evas_object_smart_callback_add(entry, "activated", eflTreeRenameActivatedCallback, ih);
  evas_object_smart_callback_add(entry, "aborted", eflTreeRenameAbortedCallback, ih);
  evas_object_smart_callback_add(entry, "unfocused", eflTreeRenameFocusOutCallback, ih);
}

static void eflTreeEndRenameEdit(Ihandle* ih, int apply)
{
  Eo* tree = iupeflGetWidget(ih);
  Evas_Object* entry = (Evas_Object*)iupAttribGet(ih, "_IUP_EFL_EDITING_ENTRY");
  Elm_Object_Item* item = (Elm_Object_Item*)iupAttribGet(ih, "_IUP_EFL_EDITING_ITEM");
  IeflTreeNode* node;
  const char* original_title;
  char* new_text = NULL;
  int id;

  if (!entry || !item)
    return;

  original_title = iupAttribGet(ih, "_IUP_EFL_ORIGINAL_TITLE");

  node = (IeflTreeNode*)elm_object_item_data_get(item);

  if (!node)
  {
    evas_object_del(entry);
    iupAttribSet(ih, "_IUP_EFL_EDITING_ENTRY", NULL);
    iupAttribSet(ih, "_IUP_EFL_EDITING_ITEM", NULL);
    iupAttribSet(ih, "_IUP_EFL_ORIGINAL_TITLE", NULL);
    return;
  }

  id = iupTreeFindNodeId(ih, (InodeHandle*)item);

  {
    const char* markup = elm_entry_entry_get(entry);
    new_text = markup ? elm_entry_markup_to_utf8(markup) : NULL;
  }

  evas_object_smart_callback_del(entry, "activated", eflTreeRenameActivatedCallback);
  evas_object_smart_callback_del(entry, "aborted", eflTreeRenameAbortedCallback);
  evas_object_smart_callback_del(entry, "unfocused", eflTreeRenameFocusOutCallback);

  if (apply && new_text && (!original_title || strcmp(new_text, original_title) != 0))
  {
    IFnis cbRename = (IFnis)IupGetCallback(ih, "RENAME_CB");
    if (cbRename && cbRename(ih, id, new_text) == IUP_IGNORE)
    {
      if (node->title)
        free(node->title);
      node->title = original_title ? strdup(original_title) : NULL;
    }
    else
    {
      if (node->title)
        free(node->title);
      node->title = strdup(new_text);
    }
  }
  else
  {
    if (node->title)
      free(node->title);
    node->title = original_title ? strdup(original_title) : NULL;
  }

  if (new_text)
    free(new_text);

  evas_object_del(entry);

  if (tree && node->item)
    elm_genlist_item_update(node->item);

  iupAttribSet(ih, "_IUP_EFL_EDITING_ENTRY", NULL);
  iupAttribSet(ih, "_IUP_EFL_EDITING_ITEM", NULL);
  iupAttribSet(ih, "_IUP_EFL_ORIGINAL_TITLE", NULL);
}

/****************************************************************
                     Driver Functions
****************************************************************/

void iupdrvTreeAddNode(Ihandle* ih, int id, int kind, const char* title, int add)
{
  Eo* tree = iupeflGetWidget(ih);
  IeflTreeNode* node;
  Elm_Object_Item* parent_item = NULL;
  Elm_Object_Item* ref_item = NULL;
  Elm_Object_Item* new_item;
  Elm_Genlist_Item_Class* itc;

  if (!tree)
    return;

  node = (IeflTreeNode*)calloc(1, sizeof(IeflTreeNode));
  if (!node)
    return;

  node->ih = ih;
  node->title = title ? strdup(title) : NULL;
  node->kind = kind;
  node->expanded = ih->data->add_expanded;

  itc = (kind == ITREE_BRANCH) ? efl_tree_branch_itc : efl_tree_leaf_itc;

  if (id == -1)
  {
    new_item = elm_genlist_item_append(tree, itc, node, NULL,
                                        (kind == ITREE_BRANCH) ? ELM_GENLIST_ITEM_TREE : ELM_GENLIST_ITEM_NONE,
                                        NULL, NULL);
  }
  else
  {
    InodeHandle* prev_handle = iupTreeGetNode(ih, id);
    if (prev_handle)
    {
      ref_item = (Elm_Object_Item*)prev_handle;
      IeflTreeNode* prev_node = (IeflTreeNode*)elm_object_item_data_get(ref_item);

      if (add)
      {
        if (prev_node && prev_node->kind == ITREE_BRANCH)
        {
          parent_item = ref_item;
          new_item = elm_genlist_item_prepend(tree, itc, node, parent_item,
                                               (kind == ITREE_BRANCH) ? ELM_GENLIST_ITEM_TREE : ELM_GENLIST_ITEM_NONE,
                                               NULL, NULL);
        }
        else
        {
          parent_item = elm_genlist_item_parent_get(ref_item);
          new_item = elm_genlist_item_insert_after(tree, itc, node, parent_item, ref_item,
                                                    (kind == ITREE_BRANCH) ? ELM_GENLIST_ITEM_TREE : ELM_GENLIST_ITEM_NONE,
                                                    NULL, NULL);
        }
      }
      else
      {
        parent_item = elm_genlist_item_parent_get(ref_item);
        new_item = elm_genlist_item_insert_before(tree, itc, node, parent_item, ref_item,
                                                   (kind == ITREE_BRANCH) ? ELM_GENLIST_ITEM_TREE : ELM_GENLIST_ITEM_NONE,
                                                   NULL, NULL);
      }
    }
    else
    {
      new_item = elm_genlist_item_append(tree, itc, node, NULL,
                                          (kind == ITREE_BRANCH) ? ELM_GENLIST_ITEM_TREE : ELM_GENLIST_ITEM_NONE,
                                          NULL, NULL);
    }
  }

  node->item = new_item;

  if (new_item)
  {
    int kindPrev = ITREE_LEAF;
    InodeHandle* prevNode = NULL;

    if (ref_item)
    {
      IeflTreeNode* prev_node = (IeflTreeNode*)elm_object_item_data_get(ref_item);
      if (prev_node)
        kindPrev = prev_node->kind;
      prevNode = (InodeHandle*)ref_item;
    }

    iupTreeAddToCache(ih, add, kindPrev, prevNode, (InodeHandle*)new_item);

    if (kind == ITREE_BRANCH && ih->data->add_expanded)
      elm_genlist_item_expanded_set(new_item, EINA_TRUE);
  }
  else
  {
    if (node->title)
      free(node->title);
    free(node);
  }
}

InodeHandle* iupdrvTreeGetFocusNode(Ihandle* ih)
{
  Eo* tree = iupeflGetWidget(ih);

  if (!tree)
    return NULL;

  return (InodeHandle*)elm_genlist_selected_item_get(tree);
}

int iupdrvTreeTotalChildCount(Ihandle* ih, InodeHandle* node_handle)
{
  Elm_Object_Item* item = (Elm_Object_Item*)node_handle;
  int count = 0;
  const Eina_List* subitems;
  const Eina_List* l;
  Elm_Object_Item* child;

  (void)ih;

  if (!item)
    return 0;

  subitems = elm_genlist_item_subitems_get(item);
  EINA_LIST_FOREACH(subitems, l, child)
  {
    count++;
    count += iupdrvTreeTotalChildCount(ih, (InodeHandle*)child);
  }

  return count;
}

void iupdrvTreeUpdateMarkMode(Ihandle* ih)
{
  Eo* tree = iupeflGetWidget(ih);

  if (!tree)
    return;

  if (ih->data->mark_mode == ITREE_MARK_MULTIPLE)
    elm_genlist_multi_select_set(tree, EINA_TRUE);
  else
    elm_genlist_multi_select_set(tree, EINA_FALSE);
}

static void eflTreeChildRebuildCacheRec(Ihandle* ih, Elm_Object_Item* item, int* id)
{
  const Eina_List* subitems = elm_genlist_item_subitems_get(item);
  const Eina_List* l;
  Elm_Object_Item* child;

  EINA_LIST_FOREACH(subitems, l, child)
  {
    (*id)++;
    ih->data->node_cache[*id].node_handle = (InodeHandle*)child;

    eflTreeChildRebuildCacheRec(ih, child, id);
  }
}

static void eflTreeRebuildNodeCache(Ihandle* ih, int id, Elm_Object_Item* item)
{
  ih->data->node_cache[id].node_handle = (InodeHandle*)item;
  eflTreeChildRebuildCacheRec(ih, item, &id);
}

void iupdrvTreeDragDropCopyNode(Ihandle* src, Ihandle* dst, InodeHandle* itemSrc, InodeHandle* itemDst)
{
  Eo* dst_tree = iupeflGetWidget(dst);
  Elm_Object_Item* src_item = (Elm_Object_Item*)itemSrc;
  Elm_Object_Item* dst_item = (Elm_Object_Item*)itemDst;
  IeflTreeNode* src_node;
  IeflTreeNode* dst_node;
  IeflTreeNode* new_node;
  Elm_Object_Item* dst_parent;
  Elm_Object_Item* new_item;
  Elm_Genlist_Item_Class* itc;
  int old_count;
  int id_dst;
  int id_new;
  int count;

  if (!dst_tree || !src_item || !dst_item)
    return;

  src_node = (IeflTreeNode*)elm_object_item_data_get(src_item);
  dst_node = (IeflTreeNode*)elm_object_item_data_get(dst_item);

  if (!src_node)
    return;

  old_count = dst->data->node_count;
  id_dst = iupTreeFindNodeId(dst, (InodeHandle*)dst_item);
  id_new = id_dst + 1;

  new_node = (IeflTreeNode*)calloc(1, sizeof(IeflTreeNode));
  if (!new_node)
    return;

  eflTreeCopyNodeData(src_node, new_node);
  new_node->ih = dst;

  eflTreeInitItemClasses();
  itc = (new_node->kind == ITREE_BRANCH) ? efl_tree_branch_itc : efl_tree_leaf_itc;

  if (dst_node && dst_node->kind == ITREE_BRANCH && elm_genlist_item_expanded_get(dst_item))
  {
    dst_parent = dst_item;
    new_item = elm_genlist_item_prepend(dst_tree, itc, new_node, dst_parent,
                                         (new_node->kind == ITREE_BRANCH) ? ELM_GENLIST_ITEM_TREE : ELM_GENLIST_ITEM_NONE,
                                         NULL, NULL);
  }
  else
  {
    dst_parent = elm_genlist_item_parent_get(dst_item);
    new_item = elm_genlist_item_insert_after(dst_tree, itc, new_node, dst_parent, dst_item,
                                              (new_node->kind == ITREE_BRANCH) ? ELM_GENLIST_ITEM_TREE : ELM_GENLIST_ITEM_NONE,
                                              NULL, NULL);
    if (dst_node && dst_node->kind == ITREE_BRANCH)
    {
      int child_count = iupdrvTreeTotalChildCount(dst, (InodeHandle*)dst_item);
      id_new += child_count;
    }
  }

  if (new_item)
  {
    new_node->item = new_item;
    dst->data->node_count++;

    if (new_node->kind == ITREE_BRANCH && new_node->expanded)
      elm_genlist_item_expanded_set(new_item, EINA_TRUE);

    eflTreeCopyChildren(dst, dst_tree, src_item, new_item);

    count = dst->data->node_count - old_count;
    iupTreeCopyMoveCache(dst, id_dst, id_new, count, 1);
    eflTreeRebuildNodeCache(dst, id_new, new_item);
  }
  else
  {
    if (new_node->title)
      free(new_node->title);
    if (new_node->font)
      free(new_node->font);
    free(new_node);
  }
}

/****************************************************************
                     Attributes
****************************************************************/

static int eflTreeSetValueAttrib(Ihandle* ih, const char* value)
{
  Eo* tree = iupeflGetWidget(ih);
  Elm_Object_Item* item;

  if (!tree)
    return 0;

  if (iupStrEqualNoCase(value, "ROOT") || iupStrEqualNoCase(value, "FIRST"))
    item = elm_genlist_first_item_get(tree);
  else if (iupStrEqualNoCase(value, "LAST"))
    item = elm_genlist_last_item_get(tree);
  else
  {
    int id = 0;
    if (iupStrToInt(value, &id))
      item = (Elm_Object_Item*)iupTreeGetNode(ih, id);
    else
      return 0;
  }

  if (item)
  {
    iupAttribSet(ih, "_IUP_EFL_IGNORE_SELECTION", "1");
    elm_genlist_item_selected_set(item, EINA_TRUE);
    iupAttribSet(ih, "_IUP_EFL_IGNORE_SELECTION", NULL);
  }

  return 0;
}

static char* eflTreeGetValueAttrib(Ihandle* ih)
{
  Eo* tree = iupeflGetWidget(ih);
  Elm_Object_Item* item;
  int id;

  if (!tree)
    return NULL;

  item = elm_genlist_selected_item_get(tree);
  if (!item)
    return NULL;

  id = iupTreeFindNodeId(ih, (InodeHandle*)item);
  if (id < 0)
    return NULL;

  return iupStrReturnInt(id);
}

static int eflTreeSetMarkStartAttrib(Ihandle* ih, const char* value)
{
  int id = 0;
  if (iupStrToInt(value, &id))
    iupAttribSet(ih, "_IUP_EFL_MARKSTART", (char*)iupTreeGetNode(ih, id));
  return 1;
}

static char* eflTreeGetMarkStartAttrib(Ihandle* ih)
{
  InodeHandle* node = (InodeHandle*)iupAttribGet(ih, "_IUP_EFL_MARKSTART");
  if (node)
    return iupStrReturnInt(iupTreeFindNodeId(ih, node));
  return NULL;
}

static int eflTreeSetTopItemAttrib(Ihandle* ih, const char* value)
{
  Eo* tree = iupeflGetWidget(ih);
  int id = 0;

  if (!tree)
    return 0;

  if (iupStrToInt(value, &id))
  {
    Elm_Object_Item* item = (Elm_Object_Item*)iupTreeGetNode(ih, id);
    if (item)
      elm_genlist_item_bring_in(item, ELM_GENLIST_ITEM_SCROLLTO_TOP);
  }

  return 0;
}

static int eflTreeSetBgColorAttrib(Ihandle* ih, const char* value)
{
  Eo* tree = iupeflGetWidget(ih);
  unsigned char r, g, b;

  if (!tree)
    return 0;

  if (iupStrToRGB(value, &r, &g, &b))
    iupeflSetColor(tree, r, g, b, 255);

  return 1;
}

static int eflTreeSetFgColorAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 1;
}

static IeflTreeNode* eflTreeGetNodeFromId(Ihandle* ih, int id)
{
  InodeHandle* node_handle = iupTreeGetNode(ih, id);
  if (!node_handle)
    return NULL;

  return (IeflTreeNode*)elm_object_item_data_get((Elm_Object_Item*)node_handle);
}

static int eflTreeGetNodeDepth(Elm_Object_Item* item)
{
  int depth = 0;
  Elm_Object_Item* parent = elm_genlist_item_parent_get(item);

  while (parent)
  {
    depth++;
    parent = elm_genlist_item_parent_get(parent);
  }

  return depth;
}

static char* eflTreeGetTitleAttrib(Ihandle* ih, int id)
{
  IeflTreeNode* node = eflTreeGetNodeFromId(ih, id);
  if (!node)
    return NULL;

  return iupStrReturnStr(node->title);
}

static int eflTreeSetTitleAttrib(Ihandle* ih, int id, const char* value)
{
  Eo* tree = iupeflGetWidget(ih);
  IeflTreeNode* node = eflTreeGetNodeFromId(ih, id);

  if (!node)
    return 0;

  if (node->title)
    free(node->title);

  node->title = value ? strdup(value) : NULL;

  if (tree && node->item)
    elm_genlist_item_update(node->item);

  return 0;
}

static char* eflTreeGetStateAttrib(Ihandle* ih, int id)
{
  IeflTreeNode* node = eflTreeGetNodeFromId(ih, id);

  if (!node)
    return NULL;

  if (node->kind != ITREE_BRANCH)
    return NULL;

  if (node->item && elm_genlist_item_expanded_get(node->item))
    return "EXPANDED";
  else
    return "COLLAPSED";
}

static int eflTreeSetStateAttrib(Ihandle* ih, int id, const char* value)
{
  IeflTreeNode* node = eflTreeGetNodeFromId(ih, id);

  if (!node || !node->item)
    return 0;

  if (node->kind != ITREE_BRANCH)
    return 0;

  iupAttribSet(ih, "_IUP_EFL_IGNORE_BRANCH_CB", "1");

  if (iupStrEqualNoCase(value, "EXPANDED"))
  {
    elm_genlist_item_expanded_set(node->item, EINA_TRUE);
    node->expanded = 1;
  }
  else
  {
    elm_genlist_item_expanded_set(node->item, EINA_FALSE);
    node->expanded = 0;
  }

  iupAttribSet(ih, "_IUP_EFL_IGNORE_BRANCH_CB", NULL);

  return 0;
}

static char* eflTreeGetKindAttrib(Ihandle* ih, int id)
{
  IeflTreeNode* node = eflTreeGetNodeFromId(ih, id);

  if (!node)
    return NULL;

  if (node->kind == ITREE_BRANCH)
    return "BRANCH";
  else
    return "LEAF";
}

static char* eflTreeGetDepthAttrib(Ihandle* ih, int id)
{
  InodeHandle* node_handle = iupTreeGetNode(ih, id);
  Elm_Object_Item* item;

  if (!node_handle)
    return NULL;

  item = (Elm_Object_Item*)node_handle;

  return iupStrReturnInt(eflTreeGetNodeDepth(item));
}

static char* eflTreeGetParentAttrib(Ihandle* ih, int id)
{
  InodeHandle* node_handle = iupTreeGetNode(ih, id);
  Elm_Object_Item* item;
  Elm_Object_Item* parent;
  int parent_id;

  if (!node_handle)
    return NULL;

  item = (Elm_Object_Item*)node_handle;
  parent = elm_genlist_item_parent_get(item);

  if (!parent)
    return NULL;

  parent_id = iupTreeFindNodeId(ih, (InodeHandle*)parent);
  if (parent_id < 0)
    return NULL;

  return iupStrReturnInt(parent_id);
}

static char* eflTreeGetNextAttrib(Ihandle* ih, int id)
{
  InodeHandle* node_handle = iupTreeGetNode(ih, id);
  Elm_Object_Item* item;
  Elm_Object_Item* next;
  Elm_Object_Item* parent;
  int next_id;

  if (!node_handle)
    return NULL;

  item = (Elm_Object_Item*)node_handle;
  parent = elm_genlist_item_parent_get(item);
  next = elm_genlist_item_next_get(item);

  while (next)
  {
    if (elm_genlist_item_parent_get(next) == parent)
    {
      next_id = iupTreeFindNodeId(ih, (InodeHandle*)next);
      if (next_id >= 0)
        return iupStrReturnInt(next_id);
    }
    next = elm_genlist_item_next_get(next);
  }

  return NULL;
}

static char* eflTreeGetPreviousAttrib(Ihandle* ih, int id)
{
  InodeHandle* node_handle = iupTreeGetNode(ih, id);
  Elm_Object_Item* item;
  Elm_Object_Item* prev;
  Elm_Object_Item* parent;
  int prev_id;

  if (!node_handle)
    return NULL;

  item = (Elm_Object_Item*)node_handle;
  parent = elm_genlist_item_parent_get(item);
  prev = elm_genlist_item_prev_get(item);

  while (prev)
  {
    if (elm_genlist_item_parent_get(prev) == parent)
    {
      prev_id = iupTreeFindNodeId(ih, (InodeHandle*)prev);
      if (prev_id >= 0)
        return iupStrReturnInt(prev_id);
    }
    prev = elm_genlist_item_prev_get(prev);
  }

  return NULL;
}

static char* eflTreeGetFirstAttrib(Ihandle* ih, int id)
{
  IeflTreeNode* node = eflTreeGetNodeFromId(ih, id);
  const Eina_List* subitems;
  Elm_Object_Item* first_child;
  int first_id;

  if (!node || !node->item)
    return NULL;

  if (node->kind != ITREE_BRANCH)
    return NULL;

  subitems = elm_genlist_item_subitems_get(node->item);
  if (!subitems)
    return NULL;

  first_child = (Elm_Object_Item*)eina_list_data_get(subitems);
  if (!first_child)
    return NULL;

  first_id = iupTreeFindNodeId(ih, (InodeHandle*)first_child);
  if (first_id < 0)
    return NULL;

  return iupStrReturnInt(first_id);
}

static char* eflTreeGetLastAttrib(Ihandle* ih, int id)
{
  IeflTreeNode* node = eflTreeGetNodeFromId(ih, id);
  const Eina_List* subitems;
  Elm_Object_Item* last_child;
  int last_id;

  if (!node || !node->item)
    return NULL;

  if (node->kind != ITREE_BRANCH)
    return NULL;

  subitems = elm_genlist_item_subitems_get(node->item);
  if (!subitems)
    return NULL;

  last_child = (Elm_Object_Item*)eina_list_last_data_get(subitems);
  if (!last_child)
    return NULL;

  last_id = iupTreeFindNodeId(ih, (InodeHandle*)last_child);
  if (last_id < 0)
    return NULL;

  return iupStrReturnInt(last_id);
}

static char* eflTreeGetChildCountAttrib(Ihandle* ih, int id)
{
  IeflTreeNode* node = eflTreeGetNodeFromId(ih, id);
  const Eina_List* subitems;

  if (!node || !node->item)
    return NULL;

  subitems = elm_genlist_item_subitems_get(node->item);

  return iupStrReturnInt(eina_list_count(subitems));
}

static char* eflTreeGetRootCountAttrib(Ihandle* ih)
{
  Eo* tree = iupeflGetWidget(ih);
  Elm_Object_Item* item;
  int count = 0;

  if (!tree)
    return NULL;

  item = elm_genlist_first_item_get(tree);
  while (item)
  {
    if (!elm_genlist_item_parent_get(item))
      count++;
    item = elm_genlist_item_next_get(item);
  }

  return iupStrReturnInt(count);
}

static char* eflTreeGetCountAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->node_count);
}

static int eflTreeSetExpandAllAttrib(Ihandle* ih, const char* value)
{
  Eo* tree = iupeflGetWidget(ih);
  Elm_Object_Item* item;
  int expand;

  if (!tree)
    return 0;

  expand = iupStrBoolean(value);

  iupAttribSet(ih, "_IUP_EFL_IGNORE_BRANCH_CB", "1");

  item = elm_genlist_first_item_get(tree);
  while (item)
  {
    IeflTreeNode* node = (IeflTreeNode*)elm_object_item_data_get(item);
    if (node && node->kind == ITREE_BRANCH)
    {
      elm_genlist_item_expanded_set(item, expand ? EINA_TRUE : EINA_FALSE);
      node->expanded = expand;
    }
    item = elm_genlist_item_next_get(item);
  }

  iupAttribSet(ih, "_IUP_EFL_IGNORE_BRANCH_CB", NULL);

  return 0;
}

static char* eflTreeGetMarkedAttrib(Ihandle* ih, int id)
{
  InodeHandle* node_handle = iupTreeGetNode(ih, id);
  Elm_Object_Item* item;

  if (!node_handle)
    return NULL;

  item = (Elm_Object_Item*)node_handle;

  if (elm_genlist_item_selected_get(item))
    return "Yes";
  else
    return "No";
}

static int eflTreeSetMarkedAttrib(Ihandle* ih, int id, const char* value)
{
  InodeHandle* node_handle = iupTreeGetNode(ih, id);
  Elm_Object_Item* item;

  if (!node_handle)
    return 0;

  item = (Elm_Object_Item*)node_handle;

  iupAttribSet(ih, "_IUP_EFL_IGNORE_SELECTION", "1");

  if (iupStrBoolean(value))
    elm_genlist_item_selected_set(item, EINA_TRUE);
  else
    elm_genlist_item_selected_set(item, EINA_FALSE);

  iupAttribSet(ih, "_IUP_EFL_IGNORE_SELECTION", NULL);

  return 0;
}

static int eflTreeSetMarkAttrib(Ihandle* ih, const char* value)
{
  Eo* tree = iupeflGetWidget(ih);
  Elm_Object_Item* item;

  if (!tree)
    return 0;

  iupAttribSet(ih, "_IUP_EFL_IGNORE_SELECTION", "1");

  if (iupStrEqualNoCase(value, "CLEARALL"))
  {
    item = elm_genlist_first_item_get(tree);
    while (item)
    {
      elm_genlist_item_selected_set(item, EINA_FALSE);
      item = elm_genlist_item_next_get(item);
    }
  }
  else if (iupStrEqualNoCase(value, "MARKALL"))
  {
    item = elm_genlist_first_item_get(tree);
    while (item)
    {
      elm_genlist_item_selected_set(item, EINA_TRUE);
      item = elm_genlist_item_next_get(item);
    }
  }
  else if (iupStrEqualNoCase(value, "INVERTALL"))
  {
    item = elm_genlist_first_item_get(tree);
    while (item)
    {
      if (elm_genlist_item_selected_get(item))
        elm_genlist_item_selected_set(item, EINA_FALSE);
      else
        elm_genlist_item_selected_set(item, EINA_TRUE);
      item = elm_genlist_item_next_get(item);
    }
  }
  else if (iupStrEqualPartial(value, "BLOCK"))
  {
    InodeHandle* start_node = (InodeHandle*)iupAttribGet(ih, "_IUP_EFL_MARKSTART");
    if (start_node)
    {
      Elm_Object_Item* start_item = (Elm_Object_Item*)start_node;
      Elm_Object_Item* end_item = elm_genlist_selected_item_get(tree);
      int start_id, end_id, i;
      int mark = 1;

      if (!end_item)
        end_item = elm_genlist_first_item_get(tree);

      start_id = iupTreeFindNodeId(ih, start_node);
      end_id = iupTreeFindNodeId(ih, (InodeHandle*)end_item);

      if (start_id > end_id)
      {
        int tmp = start_id;
        start_id = end_id;
        end_id = tmp;
      }

      for (i = start_id; i <= end_id; i++)
      {
        InodeHandle* node_h = iupTreeGetNode(ih, i);
        if (node_h)
          elm_genlist_item_selected_set((Elm_Object_Item*)node_h, mark ? EINA_TRUE : EINA_FALSE);
      }
    }
  }

  iupAttribSet(ih, "_IUP_EFL_IGNORE_SELECTION", NULL);

  return 1;
}

static char* eflTreeGetMarkedNodesAttrib(Ihandle* ih)
{
  Eo* tree = iupeflGetWidget(ih);
  char* str;
  int i;

  if (!tree)
    return NULL;

  str = iupStrGetMemory(ih->data->node_count + 1);

  for (i = 0; i < ih->data->node_count; i++)
  {
    InodeHandle* node_handle = iupTreeGetNode(ih, i);
    if (node_handle)
    {
      Elm_Object_Item* item = (Elm_Object_Item*)node_handle;
      if (elm_genlist_item_selected_get(item))
        str[i] = '+';
      else
        str[i] = '-';
    }
    else
      str[i] = '-';
  }

  str[ih->data->node_count] = 0;

  return str;
}

static int eflTreeSetMarkedNodesAttrib(Ihandle* ih, const char* value)
{
  Eo* tree = iupeflGetWidget(ih);
  int count, i;

  if (!tree || !value)
    return 0;

  count = (int)strlen(value);
  if (count > ih->data->node_count)
    count = ih->data->node_count;

  iupAttribSet(ih, "_IUP_EFL_IGNORE_SELECTION", "1");

  for (i = 0; i < count; i++)
  {
    InodeHandle* node_handle = iupTreeGetNode(ih, i);
    if (node_handle)
    {
      Elm_Object_Item* item = (Elm_Object_Item*)node_handle;
      if (value[i] == '+')
        elm_genlist_item_selected_set(item, EINA_TRUE);
      else
        elm_genlist_item_selected_set(item, EINA_FALSE);
    }
  }

  iupAttribSet(ih, "_IUP_EFL_IGNORE_SELECTION", NULL);

  return 0;
}

static void eflTreeCallNodeRemovedRec(Ihandle* ih, Elm_Object_Item* item, IFns cb, int* id)
{
  const Eina_List* subitems;
  const Eina_List* l;
  Elm_Object_Item* child;

  subitems = elm_genlist_item_subitems_get(item);
  EINA_LIST_FOREACH(subitems, l, child)
  {
    eflTreeCallNodeRemovedRec(ih, child, cb, id);
  }

  if (cb)
  {
    IeflTreeNode* node = (IeflTreeNode*)elm_object_item_data_get(item);
    if (node && node->userdata)
      cb(ih, (char*)node->userdata);
  }

  (*id)++;
}

static void eflTreeDelNodeRec(Ihandle* ih, Elm_Object_Item* item, IFns cb)
{
  const Eina_List* subitems;
  Eina_List* subitems_copy = NULL;
  const Eina_List* l;
  Elm_Object_Item* child;
  int id = 0;

  subitems = elm_genlist_item_subitems_get(item);
  EINA_LIST_FOREACH(subitems, l, child)
  {
    subitems_copy = eina_list_append(subitems_copy, child);
  }

  EINA_LIST_FOREACH(subitems_copy, l, child)
  {
    eflTreeDelNodeRec(ih, child, cb);
  }

  eina_list_free(subitems_copy);

  if (cb)
  {
    IeflTreeNode* node = (IeflTreeNode*)elm_object_item_data_get(item);
    if (node && node->userdata)
      cb(ih, (char*)node->userdata);
  }

  ih->data->node_count--;

  efl_ui_focus_manager_calc_unregister(iupeflGetWidget(ih), item);
  elm_object_item_del(item);
}

static int eflTreeSetDelNodeAttrib(Ihandle* ih, int id, const char* value)
{
  Eo* tree = iupeflGetWidget(ih);
  IFns cb;

  if (!tree)
    return 0;

  cb = (IFns)IupGetCallback(ih, "NODEREMOVED_CB");

  if (iupStrEqualNoCase(value, "ALL"))
  {
    Elm_Object_Item* item = elm_genlist_first_item_get(tree);
    while (item)
    {
      Elm_Object_Item* next = elm_genlist_item_next_get(item);
      if (!elm_genlist_item_parent_get(item))
        eflTreeDelNodeRec(ih, item, cb);
      item = next;
    }

    iupTreeDelFromCache(ih, 0, -1);
  }
  else if (iupStrEqualNoCase(value, "SELECTED"))
  {
    Elm_Object_Item* item = elm_genlist_selected_item_get(tree);
    if (item)
    {
      int node_id = iupTreeFindNodeId(ih, (InodeHandle*)item);
      int child_count = iupdrvTreeTotalChildCount(ih, (InodeHandle*)item);

      eflTreeDelNodeRec(ih, item, cb);

      iupTreeDelFromCache(ih, node_id, child_count + 1);
    }
  }
  else if (iupStrEqualNoCase(value, "CHILDREN"))
  {
    InodeHandle* node_handle = iupTreeGetNode(ih, id);
    if (node_handle)
    {
      Elm_Object_Item* item = (Elm_Object_Item*)node_handle;
      int node_id = id;
      int child_count = iupdrvTreeTotalChildCount(ih, node_handle);
      const Eina_List* subitems;
      Eina_List* subitems_copy = NULL;
      const Eina_List* l;
      Elm_Object_Item* child;

      subitems = elm_genlist_item_subitems_get(item);
      EINA_LIST_FOREACH(subitems, l, child)
      {
        subitems_copy = eina_list_append(subitems_copy, child);
      }

      EINA_LIST_FOREACH(subitems_copy, l, child)
      {
        eflTreeDelNodeRec(ih, child, cb);
      }

      eina_list_free(subitems_copy);

      iupTreeDelFromCache(ih, node_id + 1, child_count);
    }
  }
  else if (iupStrEqualNoCase(value, "MARKED"))
  {
    Eina_List* marked_items = NULL;
    Eina_List* l;
    Elm_Object_Item* item;

    item = elm_genlist_first_item_get(tree);
    while (item)
    {
      if (elm_genlist_item_selected_get(item))
        marked_items = eina_list_append(marked_items, item);
      item = elm_genlist_item_next_get(item);
    }

    EINA_LIST_FOREACH(marked_items, l, item)
    {
      if (elm_object_item_widget_get(item))
      {
        int node_id = iupTreeFindNodeId(ih, (InodeHandle*)item);
        int child_count = iupdrvTreeTotalChildCount(ih, (InodeHandle*)item);

        eflTreeDelNodeRec(ih, item, cb);

        iupTreeDelFromCache(ih, node_id, child_count + 1);
      }
    }

    eina_list_free(marked_items);
  }
  else
  {
    InodeHandle* node_handle = iupTreeGetNode(ih, id);
    if (node_handle)
    {
      Elm_Object_Item* item = (Elm_Object_Item*)node_handle;
      int child_count = iupdrvTreeTotalChildCount(ih, node_handle);

      eflTreeDelNodeRec(ih, item, cb);

      iupTreeDelFromCache(ih, id, child_count + 1);
    }
  }

  return 0;
}

static char* eflTreeGetColorAttrib(Ihandle* ih, int id)
{
  IeflTreeNode* node = eflTreeGetNodeFromId(ih, id);

  if (!node || !node->has_color)
    return NULL;

  return iupStrReturnRGB(node->color[0], node->color[1], node->color[2]);
}

static int eflTreeSetColorAttrib(Ihandle* ih, int id, const char* value)
{
  Eo* tree = iupeflGetWidget(ih);
  IeflTreeNode* node = eflTreeGetNodeFromId(ih, id);
  unsigned char r, g, b;

  if (!node)
    return 0;

  if (!iupStrToRGB(value, &r, &g, &b))
  {
    node->has_color = 0;
  }
  else
  {
    node->color[0] = r;
    node->color[1] = g;
    node->color[2] = b;
    node->color[3] = 255;
    node->has_color = 1;
  }

  if (tree && node->item)
    elm_genlist_item_update(node->item);

  return 0;
}

static char* eflTreeGetTitleFontAttrib(Ihandle* ih, int id)
{
  IeflTreeNode* node = eflTreeGetNodeFromId(ih, id);

  if (!node)
    return NULL;

  return node->font;
}

static int eflTreeSetTitleFontAttrib(Ihandle* ih, int id, const char* value)
{
  Eo* tree = iupeflGetWidget(ih);
  IeflTreeNode* node = eflTreeGetNodeFromId(ih, id);

  if (!node)
    return 0;

  if (node->font)
    free(node->font);

  node->font = value ? strdup(value) : NULL;

  if (tree && node->item)
    elm_genlist_item_update(node->item);

  return 0;
}

static void eflTreeCopyNodeData(IeflTreeNode* src, IeflTreeNode* dst)
{
  dst->ih = src->ih;
  dst->title = src->title ? strdup(src->title) : NULL;
  dst->kind = src->kind;
  dst->expanded = src->expanded;
  dst->has_image = src->has_image;
  dst->has_image_expanded = src->has_image_expanded;
  dst->image = src->image ? strdup(src->image) : NULL;
  dst->image_expanded = src->image_expanded ? strdup(src->image_expanded) : NULL;
  dst->has_color = src->has_color;
  memcpy(dst->color, src->color, 4);
  dst->font = src->font ? strdup(src->font) : NULL;
  dst->selected = 0;
  dst->toggle_value = 0;
  dst->toggle_visible = src->toggle_visible;
  dst->userdata = NULL;
}

static void eflTreeCopyChildren(Ihandle* ih, Eo* tree, Elm_Object_Item* src_item, Elm_Object_Item* dst_parent)
{
  const Eina_List* subitems;
  const Eina_List* l;
  Elm_Object_Item* child;
  Elm_Genlist_Item_Class* itc;

  subitems = elm_genlist_item_subitems_get(src_item);
  EINA_LIST_FOREACH(subitems, l, child)
  {
    IeflTreeNode* src_node = (IeflTreeNode*)elm_object_item_data_get(child);
    IeflTreeNode* new_node;
    Elm_Object_Item* new_item;

    if (!src_node)
      continue;

    new_node = (IeflTreeNode*)calloc(1, sizeof(IeflTreeNode));
    if (!new_node)
      continue;

    eflTreeCopyNodeData(src_node, new_node);

    itc = (new_node->kind == ITREE_BRANCH) ? efl_tree_branch_itc : efl_tree_leaf_itc;
    new_item = elm_genlist_item_append(tree, itc, new_node, dst_parent,
                                        (new_node->kind == ITREE_BRANCH) ? ELM_GENLIST_ITEM_TREE : ELM_GENLIST_ITEM_NONE,
                                        NULL, NULL);

    if (new_item)
    {
      new_node->item = new_item;
      ih->data->node_count++;

      if (new_node->kind == ITREE_BRANCH && new_node->expanded)
        elm_genlist_item_expanded_set(new_item, EINA_TRUE);

      eflTreeCopyChildren(ih, tree, child, new_item);
    }
    else
    {
      if (new_node->title)
        free(new_node->title);
      if (new_node->font)
        free(new_node->font);
      free(new_node);
    }
  }
}

static int eflTreeSetCopyNodeAttrib(Ihandle* ih, int id, const char* value)
{
  Eo* tree = iupeflGetWidget(ih);
  InodeHandle* src_handle;
  InodeHandle* dst_handle;
  Elm_Object_Item* src_item;
  Elm_Object_Item* dst_item;
  Elm_Object_Item* dst_parent;
  IeflTreeNode* src_node;
  IeflTreeNode* new_node;
  Elm_Object_Item* new_item;
  Elm_Genlist_Item_Class* itc;
  int dst_id;
  int position = 0;

  if (!tree || !ih->handle)
    return 0;

  src_handle = iupTreeGetNode(ih, id);
  if (!src_handle)
    return 0;

  if (!iupStrToInt(value, &dst_id))
    return 0;

  dst_handle = iupTreeGetNode(ih, dst_id);
  if (!dst_handle)
    return 0;

  src_item = (Elm_Object_Item*)src_handle;
  dst_item = (Elm_Object_Item*)dst_handle;
  src_node = (IeflTreeNode*)elm_object_item_data_get(src_item);

  if (!src_node)
    return 0;

  {
    Elm_Object_Item* parent = dst_item;
    while (parent)
    {
      if (parent == src_item)
        return 0;
      parent = elm_genlist_item_parent_get(parent);
    }
  }

  new_node = (IeflTreeNode*)calloc(1, sizeof(IeflTreeNode));
  if (!new_node)
    return 0;

  eflTreeCopyNodeData(src_node, new_node);

  itc = (new_node->kind == ITREE_BRANCH) ? efl_tree_branch_itc : efl_tree_leaf_itc;

  {
    IeflTreeNode* dst_node = (IeflTreeNode*)elm_object_item_data_get(dst_item);
    if (dst_node && dst_node->kind == ITREE_BRANCH && elm_genlist_item_expanded_get(dst_item))
    {
      dst_parent = dst_item;
      new_item = elm_genlist_item_prepend(tree, itc, new_node, dst_parent,
                                           (new_node->kind == ITREE_BRANCH) ? ELM_GENLIST_ITEM_TREE : ELM_GENLIST_ITEM_NONE,
                                           NULL, NULL);
    }
    else
    {
      dst_parent = elm_genlist_item_parent_get(dst_item);
      new_item = elm_genlist_item_insert_after(tree, itc, new_node, dst_parent, dst_item,
                                                (new_node->kind == ITREE_BRANCH) ? ELM_GENLIST_ITEM_TREE : ELM_GENLIST_ITEM_NONE,
                                                NULL, NULL);
    }
  }

  if (new_item)
  {
    new_node->item = new_item;
    ih->data->node_count++;

    if (new_node->kind == ITREE_BRANCH && new_node->expanded)
      elm_genlist_item_expanded_set(new_item, EINA_TRUE);

    eflTreeCopyChildren(ih, tree, src_item, new_item);
  }
  else
  {
    if (new_node->title)
      free(new_node->title);
    if (new_node->font)
      free(new_node->font);
    free(new_node);
  }

  return 0;
}

static int eflTreeSetMoveNodeAttrib(Ihandle* ih, int id, const char* value)
{
  Eo* tree = iupeflGetWidget(ih);
  InodeHandle* src_handle;
  InodeHandle* dst_handle;
  Elm_Object_Item* src_item;
  Elm_Object_Item* dst_item;
  Elm_Object_Item* dst_parent;
  IeflTreeNode* src_node;
  IeflTreeNode* dst_node;
  Elm_Object_Item* new_item;
  Elm_Genlist_Item_Class* itc;
  int dst_id;
  int src_count;

  if (!tree || !ih->handle)
    return 0;

  src_handle = iupTreeGetNode(ih, id);
  if (!src_handle)
    return 0;

  if (!iupStrToInt(value, &dst_id))
    return 0;

  dst_handle = iupTreeGetNode(ih, dst_id);
  if (!dst_handle)
    return 0;

  src_item = (Elm_Object_Item*)src_handle;
  dst_item = (Elm_Object_Item*)dst_handle;
  src_node = (IeflTreeNode*)elm_object_item_data_get(src_item);
  dst_node = (IeflTreeNode*)elm_object_item_data_get(dst_item);

  if (!src_node)
    return 0;

  {
    Elm_Object_Item* parent = dst_item;
    while (parent)
    {
      if (parent == src_item)
        return 0;
      parent = elm_genlist_item_parent_get(parent);
    }
  }

  src_count = iupdrvTreeTotalChildCount(ih, src_handle);

  itc = (src_node->kind == ITREE_BRANCH) ? efl_tree_branch_itc : efl_tree_leaf_itc;

  if (dst_node && dst_node->kind == ITREE_BRANCH && elm_genlist_item_expanded_get(dst_item))
  {
    dst_parent = dst_item;
    new_item = elm_genlist_item_prepend(tree, itc, src_node, dst_parent,
                                         (src_node->kind == ITREE_BRANCH) ? ELM_GENLIST_ITEM_TREE : ELM_GENLIST_ITEM_NONE,
                                         NULL, NULL);
  }
  else
  {
    dst_parent = elm_genlist_item_parent_get(dst_item);
    new_item = elm_genlist_item_insert_after(tree, itc, src_node, dst_parent, dst_item,
                                              (src_node->kind == ITREE_BRANCH) ? ELM_GENLIST_ITEM_TREE : ELM_GENLIST_ITEM_NONE,
                                              NULL, NULL);
  }

  if (new_item)
  {
    src_node->item = new_item;

    if (src_node->kind == ITREE_BRANCH && src_node->expanded)
      elm_genlist_item_expanded_set(new_item, EINA_TRUE);

    eflTreeCopyChildren(ih, tree, src_item, new_item);

    efl_ui_focus_manager_calc_unregister(tree, src_item);
    elm_object_item_del(src_item);
  }

  return 0;
}

static void eflTreeUpdateImages(Ihandle* ih, int update_type)
{
  Evas_Object* tree = (Evas_Object*)ih->handle;
  Elm_Object_Item* item;

  if (!tree)
    return;

  item = elm_genlist_first_item_get(tree);
  while (item)
  {
    IeflTreeNode* node = (IeflTreeNode*)elm_object_item_data_get(item);
    if (node)
    {
      int update = 0;
      if (update_type == ITREE_UPDATEIMAGE_LEAF && node->kind == ITREE_LEAF && !node->has_image)
        update = 1;
      else if (update_type == ITREE_UPDATEIMAGE_COLLAPSED && node->kind == ITREE_BRANCH && !node->has_image)
        update = 1;
      else if (update_type == ITREE_UPDATEIMAGE_EXPANDED && node->kind == ITREE_BRANCH && !node->has_image_expanded)
        update = 1;
      if (update)
        elm_genlist_item_update(item);
    }
    item = elm_genlist_item_next_get(item);
  }
}

static int eflTreeSetImageLeafAttrib(Ihandle* ih, const char* value)
{
  ih->data->def_image_leaf = iupImageGetImage(value, ih, 0, NULL);
  iupAttribSetStr(ih, "_EFL_IMAGELEAF", value);
  eflTreeUpdateImages(ih, ITREE_UPDATEIMAGE_LEAF);
  return 1;
}

static int eflTreeSetImageBranchCollapsedAttrib(Ihandle* ih, const char* value)
{
  ih->data->def_image_collapsed = iupImageGetImage(value, ih, 0, NULL);
  iupAttribSetStr(ih, "_EFL_IMAGEBRANCHCOLLAPSED", value);
  eflTreeUpdateImages(ih, ITREE_UPDATEIMAGE_COLLAPSED);
  return 1;
}

static int eflTreeSetImageBranchExpandedAttrib(Ihandle* ih, const char* value)
{
  ih->data->def_image_expanded = iupImageGetImage(value, ih, 0, NULL);
  iupAttribSetStr(ih, "_EFL_IMAGEBRANCHEXPANDED", value);
  eflTreeUpdateImages(ih, ITREE_UPDATEIMAGE_EXPANDED);
  return 1;
}

static int eflTreeSetImageAttrib(Ihandle* ih, int id, const char* value)
{
  IeflTreeNode* node = eflTreeGetNodeFromId(ih, id);
  Eo* tree = iupeflGetWidget(ih);

  if (!node)
    return 0;

  if (node->image)
  {
    free(node->image);
    node->image = NULL;
  }

  if (value && value[0])
  {
    node->image = strdup(value);
    node->has_image = 1;
  }
  else
    node->has_image = 0;

  if (tree && node->item)
    elm_genlist_item_update(node->item);

  return 0;
}

static int eflTreeSetImageExpandedAttrib(Ihandle* ih, int id, const char* value)
{
  IeflTreeNode* node = eflTreeGetNodeFromId(ih, id);
  Eo* tree = iupeflGetWidget(ih);

  if (!node)
    return 0;

  if (node->kind != ITREE_BRANCH)
    return 0;

  if (node->image_expanded)
  {
    free(node->image_expanded);
    node->image_expanded = NULL;
  }

  if (value && value[0])
  {
    node->image_expanded = strdup(value);
    node->has_image_expanded = 1;
  }
  else
    node->has_image_expanded = 0;

  if (tree && node->item)
    elm_genlist_item_update(node->item);

  return 0;
}

static char* eflTreeGetToggleValueAttrib(Ihandle* ih, int id)
{
  IeflTreeNode* node = eflTreeGetNodeFromId(ih, id);

  if (!node)
    return NULL;

  if (!ih->data->show_toggle)
    return NULL;

  if (node->toggle_value)
    return "ON";
  else
    return "OFF";
}

static int eflTreeSetToggleValueAttrib(Ihandle* ih, int id, const char* value)
{
  IeflTreeNode* node = eflTreeGetNodeFromId(ih, id);
  Eo* tree = iupeflGetWidget(ih);

  if (!node)
    return 0;

  if (!ih->data->show_toggle)
    return 0;

  if (iupStrEqualNoCase(value, "ON"))
    node->toggle_value = 1;
  else
    node->toggle_value = 0;

  if (tree && node->item)
    elm_genlist_item_update(node->item);

  return 0;
}

static char* eflTreeGetToggleVisibleAttrib(Ihandle* ih, int id)
{
  IeflTreeNode* node = eflTreeGetNodeFromId(ih, id);

  if (!node)
    return NULL;

  if (node->toggle_visible)
    return "Yes";
  else
    return "No";
}

static int eflTreeSetToggleVisibleAttrib(Ihandle* ih, int id, const char* value)
{
  IeflTreeNode* node = eflTreeGetNodeFromId(ih, id);
  Eo* tree = iupeflGetWidget(ih);

  if (!node)
    return 0;

  if (iupStrBoolean(value))
    node->toggle_visible = 1;
  else
    node->toggle_visible = 0;

  if (tree && node->item)
    elm_genlist_item_update(node->item);

  return 0;
}

static int eflTreeSetShowRenameAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->show_rename = 1;
  else
    ih->data->show_rename = 0;
  return 0;
}

static int eflTreeSetRenameAttrib(Ihandle* ih, const char* value)
{
  Eo* tree = iupeflGetWidget(ih);
  Elm_Object_Item* item;

  (void)value;

  if (!tree || !ih->data->show_rename)
    return 0;

  item = elm_genlist_selected_item_get(tree);
  if (item)
    eflTreeStartRenameEdit(ih, item);

  return 0;
}

/****************************************************************
                     Drag and Drop Support
****************************************************************/

static int eflTreeConvertXYToPos(Ihandle* ih, int x, int y);

static int efl_tree_drag_start_x = 0;
static int efl_tree_drag_start_y = 0;
static Ihandle* efl_tree_drag_source = NULL;
static Eina_Bool efl_tree_drag_active = EINA_FALSE;

static void eflTreeDragFinishedCb(void *data, const Efl_Event *ev)
{
  (void)data;
  (void)ev;
  efl_tree_drag_active = EINA_FALSE;
  efl_tree_drag_source = NULL;
}

static void eflTreeDragPointerDownCb(void *data, const Efl_Event *ev)
{
  Ihandle* ih = (Ihandle*)data;
  Efl_Input_Pointer* pointer = ev->info;
  Eina_Position2D pos;
  int button;

  if (efl_tree_drag_active)
    return;

  button = efl_input_pointer_button_get(pointer);
  if (button != 1)
    return;

  pos = efl_input_pointer_position_get(pointer);
  efl_tree_drag_start_x = pos.x;
  efl_tree_drag_start_y = pos.y;

  iupAttribSet(ih, "_IUPEFL_TREE_DRAG_PENDING", "1");
}

static void eflTreeDragPointerMoveCb(void *data, const Efl_Event *ev)
{
  Ihandle* ih = (Ihandle*)data;
  Efl_Input_Pointer* pointer = ev->info;
  Eina_Position2D pos;
  int dx, dy;
  Eo* tree;
  int idDrag;

  if (efl_tree_drag_active)
    return;

  if (!iupAttribGet(ih, "_IUPEFL_TREE_DRAG_PENDING"))
    return;

  pos = efl_input_pointer_position_get(pointer);
  dx = pos.x - efl_tree_drag_start_x;
  dy = pos.y - efl_tree_drag_start_y;

  if (dx*dx + dy*dy > 25)
  {
    iupAttribSet(ih, "_IUPEFL_TREE_DRAG_PENDING", NULL);

    tree = iupeflGetWidget(ih);
    idDrag = eflTreeConvertXYToPos(ih, efl_tree_drag_start_x, efl_tree_drag_start_y);

    if (idDrag >= 0 && tree)
    {
      Eina_Content* content;
      Eina_Slice slice;

      iupAttribSetInt(ih, "_IUP_TREE_SOURCEPOS", idDrag);
      efl_tree_drag_source = ih;

      slice.mem = &efl_tree_drag_source;
      slice.len = sizeof(Ihandle*);
      content = eina_content_new(slice, "application/x-iup-tree-item");

      if (content)
      {
        Evas* evas = evas_object_evas_get(tree);
        Efl_Input_Device* seat = evas_default_device_get(evas, EVAS_DEVICE_CLASS_SEAT);
        unsigned int seat_id = seat ? evas_device_seat_id_get(seat) : 1;

        efl_tree_drag_active = EINA_TRUE;
        efl_ui_dnd_drag_start(tree, content, "copy", seat_id);
      }
    }
  }
}

static void eflTreeDragPointerUpCb(void *data, const Efl_Event *ev)
{
  Ihandle* ih = (Ihandle*)data;
  (void)ev;
  iupAttribSet(ih, "_IUPEFL_TREE_DRAG_PENDING", NULL);
}

static void eflTreeDropCb(void *data, const Efl_Event *ev)
{
  Ihandle* ih = (Ihandle*)data;
  Efl_Ui_Drop_Dropped_Event* drop_ev = ev->info;
  int idDrop;
  Ihandle* ih_source;
  int idDrag;
  int is_shift = 0;
  int is_ctrl = 0;
  int equal_nodes = 0;
  char key[5];
  IFniiii cb;

  if (!ih || !efl_tree_drag_source)
    return;

  idDrop = eflTreeConvertXYToPos(ih, drop_ev->dnd.position.x, drop_ev->dnd.position.y);
  if (idDrop < 0)
    idDrop = ih->data->node_count - 1;

  ih_source = efl_tree_drag_source;
  idDrag = iupAttribGetInt(ih_source, "_IUP_TREE_SOURCEPOS");

  iupdrvGetKeyState(key);
  if (key[0] == 'S')
    is_shift = 1;
  if (key[1] == 'C')
    is_ctrl = 1;

  if (ih_source == ih)
  {
    Elm_Object_Item* itemSrc = (Elm_Object_Item*)iupTreeGetNode(ih, idDrag);
    Elm_Object_Item* itemDst = (Elm_Object_Item*)iupTreeGetNode(ih, idDrop);

    if (idDrag == idDrop)
    {
      if (!iupAttribGetBoolean(ih, "DROPEQUALDRAG"))
        return;
      equal_nodes = 1;
    }

    if (!equal_nodes && itemSrc && itemDst)
    {
      Elm_Object_Item* parent = elm_genlist_item_parent_get(itemDst);
      while (parent)
      {
        if (parent == itemSrc)
          return;
        parent = elm_genlist_item_parent_get(parent);
      }
    }
  }

  cb = (IFniiii)IupGetCallback(ih, "DRAGDROP_CB");
  if (cb && cb(ih_source, idDrag, idDrop, is_shift, is_ctrl) == IUP_CONTINUE && !equal_nodes)
  {
    InodeHandle* itemSrc = iupTreeGetNode(ih_source, idDrag);
    InodeHandle* itemDst = iupTreeGetNode(ih, idDrop);

    if (itemSrc && itemDst)
    {
      iupdrvTreeDragDropCopyNode(ih_source, ih, itemSrc, itemDst);

      if (!is_ctrl)
      {
        IupSetAttributeId(ih_source, "DELNODE", idDrag, "SELECTED");
      }
    }
  }
}

static void eflTreeEnableDragDrop(Ihandle* ih)
{
  Eo* tree = iupeflGetWidget(ih);

  if (!tree)
    return;

  efl_event_callback_add(tree, EFL_EVENT_POINTER_DOWN, eflTreeDragPointerDownCb, ih);
  efl_event_callback_add(tree, EFL_EVENT_POINTER_MOVE, eflTreeDragPointerMoveCb, ih);
  efl_event_callback_add(tree, EFL_EVENT_POINTER_UP, eflTreeDragPointerUpCb, ih);
  efl_event_callback_add(tree, EFL_UI_DND_EVENT_DROP_DROPPED, eflTreeDropCb, ih);
  efl_event_callback_add(tree, EFL_UI_DND_EVENT_DRAG_FINISHED, eflTreeDragFinishedCb, ih);
}

static int eflTreeSetShowDragDropAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
  {
    ih->data->show_dragdrop = 1;
    if (ih->handle)
      eflTreeEnableDragDrop(ih);
  }
  else
  {
    ih->data->show_dragdrop = 0;
  }
  return 1;
}

/****************************************************************
                     XY to Position Conversion
****************************************************************/

static int eflTreeConvertXYToPos(Ihandle* ih, int x, int y)
{
  Evas_Object* tree = iupeflGetWidget(ih);
  Elm_Object_Item* item;
  int posret = 0;
  int id;

  if (!tree)
    return -1;

  item = elm_genlist_at_xy_item_get(tree, x, y, &posret);

  if (item)
  {
    id = iupTreeFindNodeId(ih, (InodeHandle*)item);
    if (id >= 0)
    {
      if (posret == -1 && id > 0)
        return id - 1;
      return id;
    }
  }

  return -1;
}

/****************************************************************
                     Methods
****************************************************************/

static int eflTreeMapMethod(Ihandle* ih)
{
  Eo* parent;
  Eo* tree;

  parent = iupeflGetParentWidget(ih);
  if (!parent)
    return IUP_ERROR;

  eflTreeInitItemClasses();

  tree = elm_genlist_add(parent);
  if (!tree)
    return IUP_ERROR;

  ih->handle = (InativeHandle*)tree;

  elm_genlist_mode_set(tree, ELM_LIST_SCROLL);

  if (ih->data->mark_mode == ITREE_MARK_MULTIPLE)
    elm_genlist_multi_select_set(tree, EINA_TRUE);

  evas_object_smart_callback_add(tree, "selected", eflTreeSelectedCallback, ih);
  evas_object_smart_callback_add(tree, "unselected", eflTreeUnselectedCallback, ih);
  evas_object_smart_callback_add(tree, "expand,request", eflTreeExpandRequestCallback, ih);
  evas_object_smart_callback_add(tree, "contract,request", eflTreeContractRequestCallback, ih);
  evas_object_smart_callback_add(tree, "expanded", eflTreeExpandedCallback, ih);
  evas_object_smart_callback_add(tree, "contracted", eflTreeContractedCallback, ih);
  evas_object_smart_callback_add(tree, "clicked,double", eflTreeDoubleClickCallback, ih);

  efl_event_callback_add(tree, EFL_EVENT_POINTER_DOWN, eflTreeRightClickCallback, ih);
  efl_event_callback_add(tree, EFL_EVENT_POINTER_DOWN, eflTreeClickCallback, ih);
  efl_event_callback_add(tree, EFL_EVENT_KEY_DOWN, eflTreeKeyDownCallback, ih);

  iupeflBaseAddCallbacks(ih, tree);

  iupeflAddToParent(ih);

  if (iupAttribGetInt(ih, "ADDROOT"))
    iupdrvTreeAddNode(ih, -1, ITREE_BRANCH, "", 0);

  IupSetCallback(ih, "_IUP_XY2POS_CB", (Icallback)eflTreeConvertXYToPos);

  if (ih->data->show_dragdrop)
    eflTreeEnableDragDrop(ih);

  return IUP_NOERROR;
}

static void eflTreeUnMapMethod(Ihandle* ih)
{
  Eo* tree = iupeflGetWidget(ih);

  eflTreeEndRenameEdit(ih, 0);

  if (tree)
  {
    if (ih->data->show_dragdrop)
    {
      efl_event_callback_del(tree, EFL_EVENT_POINTER_DOWN, eflTreeDragPointerDownCb, ih);
      efl_event_callback_del(tree, EFL_EVENT_POINTER_MOVE, eflTreeDragPointerMoveCb, ih);
      efl_event_callback_del(tree, EFL_EVENT_POINTER_UP, eflTreeDragPointerUpCb, ih);
      efl_event_callback_del(tree, EFL_UI_DND_EVENT_DROP_DROPPED, eflTreeDropCb, ih);
      efl_event_callback_del(tree, EFL_UI_DND_EVENT_DRAG_FINISHED, eflTreeDragFinishedCb, ih);
    }
    evas_object_smart_callback_del(tree, "selected", eflTreeSelectedCallback);
    evas_object_smart_callback_del(tree, "unselected", eflTreeUnselectedCallback);
    evas_object_smart_callback_del(tree, "expand,request", eflTreeExpandRequestCallback);
    evas_object_smart_callback_del(tree, "contract,request", eflTreeContractRequestCallback);
    evas_object_smart_callback_del(tree, "expanded", eflTreeExpandedCallback);
    evas_object_smart_callback_del(tree, "contracted", eflTreeContractedCallback);
    evas_object_smart_callback_del(tree, "clicked,double", eflTreeDoubleClickCallback);
    efl_event_callback_del(tree, EFL_EVENT_POINTER_DOWN, eflTreeRightClickCallback, ih);
    efl_event_callback_del(tree, EFL_EVENT_POINTER_DOWN, eflTreeClickCallback, ih);
    efl_event_callback_del(tree, EFL_EVENT_KEY_DOWN, eflTreeKeyDownCallback, ih);
    elm_genlist_clear(tree);
    iupeflDelete(tree);
  }

  ih->handle = NULL;
}

void iupdrvTreeInitClass(Iclass* ic)
{
  ic->Map = eflTreeMapMethod;
  ic->UnMap = eflTreeUnMapMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, eflTreeSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, eflTreeSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "EXPANDALL", NULL, eflTreeSetExpandAllAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOPITEM", NULL, eflTreeSetTopItemAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "STATE", eflTreeGetStateAttrib, eflTreeSetStateAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "DEPTH", eflTreeGetDepthAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "KIND", eflTreeGetKindAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "PARENT", eflTreeGetParentAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "NEXT", eflTreeGetNextAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "PREVIOUS", eflTreeGetPreviousAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "FIRST", eflTreeGetFirstAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "LAST", eflTreeGetLastAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TITLE", eflTreeGetTitleAttrib, eflTreeSetTitleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "COLOR", eflTreeGetColorAttrib, eflTreeSetColorAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TITLEFONT", eflTreeGetTitleFontAttrib, eflTreeSetTitleFontAttrib, IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "CHILDCOUNT", eflTreeGetChildCountAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ROOTCOUNT", eflTreeGetRootCountAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COUNT", eflTreeGetCountAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "MARKED", eflTreeGetMarkedAttrib, eflTreeSetMarkedAttrib, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARK", NULL, eflTreeSetMarkAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STARTING", NULL, eflTreeSetMarkStartAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKSTART", eflTreeGetMarkStartAttrib, eflTreeSetMarkStartAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKEDNODES", eflTreeGetMarkedNodesAttrib, eflTreeSetMarkedNodesAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "VALUE", eflTreeGetValueAttrib, eflTreeSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "DELNODE", NULL, eflTreeSetDelNodeAttrib, IUPAF_NOT_MAPPED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "MOVENODE", NULL, eflTreeSetMoveNodeAttrib, IUPAF_NOT_MAPPED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "COPYNODE", NULL, eflTreeSetCopyNodeAttrib, IUPAF_NOT_MAPPED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "IMAGE", NULL, eflTreeSetImageAttrib, IUPAF_IHANDLENAME | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IMAGEEXPANDED", NULL, eflTreeSetImageExpandedAttrib, IUPAF_IHANDLENAME | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGELEAF", NULL, eflTreeSetImageLeafAttrib, IUPAF_SAMEASSYSTEM, "IMGLEAF", IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEBRANCHCOLLAPSED", NULL, eflTreeSetImageBranchCollapsedAttrib, IUPAF_SAMEASSYSTEM, "IMGCOLLAPSED", IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEBRANCHEXPANDED", NULL, eflTreeSetImageBranchExpandedAttrib, IUPAF_SAMEASSYSTEM, "IMGEXPANDED", IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "TOGGLEVALUE", eflTreeGetToggleValueAttrib, eflTreeSetToggleValueAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TOGGLEVISIBLE", eflTreeGetToggleVisibleAttrib, eflTreeSetToggleVisibleAttrib, IUPAF_NO_INHERIT);

  iupClassRegisterReplaceAttribFunc(ic, "SHOWRENAME", NULL, eflTreeSetShowRenameAttrib);
  iupClassRegisterAttribute(ic, "RENAME", NULL, eflTreeSetRenameAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  iupClassRegisterReplaceAttribFunc(ic, "SHOWDRAGDROP", NULL, eflTreeSetShowDragDropAttrib);

  iupClassRegisterAttribute(ic, "SCROLLVISIBLE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
}
