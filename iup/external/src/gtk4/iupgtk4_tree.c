/** \file
 * \brief Tree Control
 *
 * See Copyright Notice in iup.h
 */

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

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
#include "iupgtk4_drv.h"


/*****************************************************************************/
/* Helper functions                                                          */
/*****************************************************************************/

int iupgtk4GetColor(const char* value, GdkRGBA *color)
{
  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
  {
    iupgtk4ColorSetRGB(color, r, g, b);
    return 1;
  }
  return 0;
}

/*****************************************************************************/
/* IupGtk4TreeNode GObject - Holds data for a single tree node               */
/*****************************************************************************/

#define IUP_GTK4_TYPE_TREE_NODE (iup_gtk4_tree_node_get_type())
G_DECLARE_FINAL_TYPE(IupGtk4TreeNode, iup_gtk4_tree_node, IUP_GTK4, TREE_NODE, GObject)

struct _IupGtk4TreeNode
{
  GObject parent_instance;

  char *title;
  int kind;                        /* ITREE_BRANCH or ITREE_LEAF */
  GdkTexture *image;
  GdkTexture *image_expanded;
  gboolean has_image;
  gboolean has_image_expanded;
  GdkRGBA *color;
  PangoFontDescription *font;
  gboolean selected;
  gboolean check;
  gboolean three_state;
  gboolean toggle_visible;

  GListStore *children;            /* Child nodes (NULL for leaves until first child added) */
  IupGtk4TreeNode *parent;         /* Parent node (NULL for root level) */
  void *userdata;                  /* IUP userdata */
  Ihandle *ih;                     /* Back-reference to IUP handle */
};

enum {
  PROP_NODE_0,
  PROP_NODE_TITLE,
  PROP_NODE_KIND,
  PROP_NODE_IMAGE,
  PROP_NODE_IMAGE_EXPANDED,
  PROP_NODE_HAS_IMAGE,
  PROP_NODE_HAS_IMAGE_EXPANDED,
  PROP_NODE_COLOR,
  PROP_NODE_FONT,
  PROP_NODE_SELECTED,
  PROP_NODE_CHECK,
  PROP_NODE_THREE_STATE,
  PROP_NODE_TOGGLE_VISIBLE,
  PROP_NODE_CHILDREN,
  PROP_NODE_EXPANDABLE,
  N_NODE_PROPERTIES
};

static GParamSpec *node_properties[N_NODE_PROPERTIES] = { NULL, };

G_DEFINE_TYPE(IupGtk4TreeNode, iup_gtk4_tree_node, G_TYPE_OBJECT)

static void
iup_gtk4_tree_node_dispose(GObject *object)
{
  IupGtk4TreeNode *self = IUP_GTK4_TREE_NODE(object);

  g_clear_object(&self->children);
  g_clear_object(&self->image);
  g_clear_object(&self->image_expanded);

  G_OBJECT_CLASS(iup_gtk4_tree_node_parent_class)->dispose(object);
}

static void
iup_gtk4_tree_node_finalize(GObject *object)
{
  IupGtk4TreeNode *self = IUP_GTK4_TREE_NODE(object);

  g_free(self->title);
  if (self->color)
    gdk_rgba_free(self->color);
  if (self->font)
    pango_font_description_free(self->font);

  G_OBJECT_CLASS(iup_gtk4_tree_node_parent_class)->finalize(object);
}

static void
iup_gtk4_tree_node_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  IupGtk4TreeNode *self = IUP_GTK4_TREE_NODE(object);

  switch (prop_id)
  {
    case PROP_NODE_TITLE:
      g_value_set_string(value, self->title);
      break;
    case PROP_NODE_KIND:
      g_value_set_int(value, self->kind);
      break;
    case PROP_NODE_IMAGE:
      g_value_set_object(value, self->image);
      break;
    case PROP_NODE_IMAGE_EXPANDED:
      g_value_set_object(value, self->image_expanded);
      break;
    case PROP_NODE_HAS_IMAGE:
      g_value_set_boolean(value, self->has_image);
      break;
    case PROP_NODE_HAS_IMAGE_EXPANDED:
      g_value_set_boolean(value, self->has_image_expanded);
      break;
    case PROP_NODE_COLOR:
      g_value_set_boxed(value, self->color);
      break;
    case PROP_NODE_FONT:
      g_value_set_boxed(value, self->font);
      break;
    case PROP_NODE_SELECTED:
      g_value_set_boolean(value, self->selected);
      break;
    case PROP_NODE_CHECK:
      g_value_set_boolean(value, self->check);
      break;
    case PROP_NODE_THREE_STATE:
      g_value_set_boolean(value, self->three_state);
      break;
    case PROP_NODE_TOGGLE_VISIBLE:
      g_value_set_boolean(value, self->toggle_visible);
      break;
    case PROP_NODE_CHILDREN:
      g_value_set_object(value, self->children);
      break;
    case PROP_NODE_EXPANDABLE:
      g_value_set_boolean(value, self->kind == ITREE_BRANCH);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}

static void
iup_gtk4_tree_node_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  IupGtk4TreeNode *self = IUP_GTK4_TREE_NODE(object);

  switch (prop_id)
  {
    case PROP_NODE_TITLE:
      g_free(self->title);
      self->title = g_value_dup_string(value);
      break;
    case PROP_NODE_KIND:
      self->kind = g_value_get_int(value);
      break;
    case PROP_NODE_IMAGE:
      g_set_object(&self->image, g_value_get_object(value));
      break;
    case PROP_NODE_IMAGE_EXPANDED:
      g_set_object(&self->image_expanded, g_value_get_object(value));
      break;
    case PROP_NODE_HAS_IMAGE:
      self->has_image = g_value_get_boolean(value);
      break;
    case PROP_NODE_HAS_IMAGE_EXPANDED:
      self->has_image_expanded = g_value_get_boolean(value);
      break;
    case PROP_NODE_COLOR:
      if (self->color)
        gdk_rgba_free(self->color);
      self->color = g_value_dup_boxed(value);
      break;
    case PROP_NODE_FONT:
      if (self->font)
        pango_font_description_free(self->font);
      self->font = g_value_dup_boxed(value);
      break;
    case PROP_NODE_SELECTED:
      self->selected = g_value_get_boolean(value);
      break;
    case PROP_NODE_CHECK:
      self->check = g_value_get_boolean(value);
      break;
    case PROP_NODE_THREE_STATE:
      self->three_state = g_value_get_boolean(value);
      break;
    case PROP_NODE_TOGGLE_VISIBLE:
      self->toggle_visible = g_value_get_boolean(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}

static void
iup_gtk4_tree_node_class_init(IupGtk4TreeNodeClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->dispose = iup_gtk4_tree_node_dispose;
  object_class->finalize = iup_gtk4_tree_node_finalize;
  object_class->get_property = iup_gtk4_tree_node_get_property;
  object_class->set_property = iup_gtk4_tree_node_set_property;

  node_properties[PROP_NODE_TITLE] =
    g_param_spec_string("title", NULL, NULL, "", G_PARAM_READWRITE);

  node_properties[PROP_NODE_KIND] =
    g_param_spec_int("kind", NULL, NULL, ITREE_BRANCH, ITREE_LEAF, ITREE_LEAF, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  node_properties[PROP_NODE_IMAGE] =
    g_param_spec_object("image", NULL, NULL, GDK_TYPE_TEXTURE, G_PARAM_READWRITE);

  node_properties[PROP_NODE_IMAGE_EXPANDED] =
    g_param_spec_object("image-expanded", NULL, NULL, GDK_TYPE_TEXTURE, G_PARAM_READWRITE);

  node_properties[PROP_NODE_HAS_IMAGE] =
    g_param_spec_boolean("has-image", NULL, NULL, FALSE, G_PARAM_READWRITE);

  node_properties[PROP_NODE_HAS_IMAGE_EXPANDED] =
    g_param_spec_boolean("has-image-expanded", NULL, NULL, FALSE, G_PARAM_READWRITE);

  node_properties[PROP_NODE_COLOR] =
    g_param_spec_boxed("color", NULL, NULL, GDK_TYPE_RGBA, G_PARAM_READWRITE);

  node_properties[PROP_NODE_FONT] =
    g_param_spec_boxed("font", NULL, NULL, PANGO_TYPE_FONT_DESCRIPTION, G_PARAM_READWRITE);

  node_properties[PROP_NODE_SELECTED] =
    g_param_spec_boolean("selected", NULL, NULL, FALSE, G_PARAM_READWRITE);

  node_properties[PROP_NODE_CHECK] =
    g_param_spec_boolean("check", NULL, NULL, FALSE, G_PARAM_READWRITE);

  node_properties[PROP_NODE_THREE_STATE] =
    g_param_spec_boolean("three-state", NULL, NULL, FALSE, G_PARAM_READWRITE);

  node_properties[PROP_NODE_TOGGLE_VISIBLE] =
    g_param_spec_boolean("toggle-visible", NULL, NULL, TRUE, G_PARAM_READWRITE);

  node_properties[PROP_NODE_CHILDREN] =
    g_param_spec_object("children", NULL, NULL, G_TYPE_LIST_MODEL, G_PARAM_READABLE);

  node_properties[PROP_NODE_EXPANDABLE] =
    g_param_spec_boolean("expandable", NULL, NULL, FALSE, G_PARAM_READABLE);

  g_object_class_install_properties(object_class, N_NODE_PROPERTIES, node_properties);
}

static void
iup_gtk4_tree_node_init(IupGtk4TreeNode *self)
{
  self->title = g_strdup("");
  self->kind = ITREE_LEAF;
  self->image = NULL;
  self->image_expanded = NULL;
  self->has_image = FALSE;
  self->has_image_expanded = FALSE;
  self->color = NULL;
  self->font = NULL;
  self->selected = FALSE;
  self->check = FALSE;
  self->three_state = FALSE;
  self->toggle_visible = TRUE;
  self->children = NULL;
  self->parent = NULL;
  self->userdata = NULL;
  self->ih = NULL;
}

static IupGtk4TreeNode*
iup_gtk4_tree_node_new(Ihandle *ih, int kind, const char *title)
{
  IupGtk4TreeNode *node = g_object_new(IUP_GTK4_TYPE_TREE_NODE,
                                        "kind", kind,
                                        "title", title ? title : "",
                                        NULL);
  node->ih = ih;

  /* Branches always have a children store, even if empty */
  if (kind == ITREE_BRANCH)
    node->children = g_list_store_new(IUP_GTK4_TYPE_TREE_NODE);

  return node;
}

static void
iup_gtk4_tree_node_add_child(IupGtk4TreeNode *parent, IupGtk4TreeNode *child, int position)
{
  if (!parent->children)
    parent->children = g_list_store_new(IUP_GTK4_TYPE_TREE_NODE);

  child->parent = parent;

  if (position < 0 || position >= (int)g_list_model_get_n_items(G_LIST_MODEL(parent->children)))
    g_list_store_append(parent->children, child);
  else
    g_list_store_insert(parent->children, position, child);
}

static void
iup_gtk4_tree_node_remove_child(IupGtk4TreeNode *parent, int position)
{
  if (parent->children && position >= 0 && position < (int)g_list_model_get_n_items(G_LIST_MODEL(parent->children)))
    g_list_store_remove(parent->children, position);
}

static int
iup_gtk4_tree_node_get_child_count(IupGtk4TreeNode *node)
{
  if (!node->children)
    return 0;
  return g_list_model_get_n_items(G_LIST_MODEL(node->children));
}

static IupGtk4TreeNode*
iup_gtk4_tree_node_get_child(IupGtk4TreeNode *node, int index)
{
  if (!node->children)
    return NULL;
  return g_list_model_get_item(G_LIST_MODEL(node->children), index);
}


/*****************************************************************************/
/* GtkTreeListModel child model creation function                            */
/*****************************************************************************/

static GListModel*
iupgtk4TreeCreateChildModel(gpointer item, gpointer user_data)
{
  IupGtk4TreeNode *node = IUP_GTK4_TREE_NODE(item);
  (void)user_data;

  /* Only branches can have children */
  if (node->kind == ITREE_BRANCH && node->children != NULL)
  {
    /* Return a reference to the children store */
    return G_LIST_MODEL(g_object_ref(node->children));
  }

  return NULL;
}


/*****************************************************************************/
/* Helper functions                                                          */
/*****************************************************************************/

/* Find position of a node in the flattened tree list */
static int
iupgtk4TreeGetVisiblePosition(Ihandle *ih, IupGtk4TreeNode *target_node)
{
  GtkTreeListModel *tree_model = GTK_TREE_LIST_MODEL(iupAttribGet(ih, "_IUPGTK4_TREE_MODEL"));
  guint n_items = g_list_model_get_n_items(G_LIST_MODEL(tree_model));
  guint i;

  for (i = 0; i < n_items; i++)
  {
    GtkTreeListRow *row = gtk_tree_list_model_get_row(tree_model, i);
    if (row)
    {
      gpointer item = gtk_tree_list_row_get_item(row);
      g_object_unref(row);

      if (item == target_node)
      {
        g_object_unref(item);
        return (int)i;
      }
      g_object_unref(item);
    }
  }

  return -1;
}

/* Find node by IUP id (uses the cache) */
static IupGtk4TreeNode*
iupgtk4TreeGetNodeFromId(Ihandle *ih, int id)
{
  InodeHandle *handle = iupTreeGetNode(ih, id);
  return (IupGtk4TreeNode*)handle;
}

/* Forward declarations for helper functions */
static int iupgtk4TreeFindPositionInParent(IupGtk4TreeNode *node);
static GListStore* iupgtk4TreeGetParentStore(Ihandle *ih, IupGtk4TreeNode *node);

/* Notify the store that a node's visual properties changed, triggering a rebind */
static void
iupgtk4TreeNotifyNodeChanged(Ihandle *ih, IupGtk4TreeNode *node)
{
  GListStore *parent_store = iupgtk4TreeGetParentStore(ih, node);
  if (parent_store)
  {
    int pos = iupgtk4TreeFindPositionInParent(node);
    if (pos >= 0)
      g_list_model_items_changed(G_LIST_MODEL(parent_store), pos, 1, 1);
  }
}

/* Find IUP id for a node */
static int
iupgtk4TreeFindNodeId(Ihandle *ih, IupGtk4TreeNode *node)
{
  return iupTreeFindNodeId(ih, (InodeHandle*)node);
}

/* Find position of node in its parent's children list */
static int
iupgtk4TreeFindPositionInParent(IupGtk4TreeNode *node)
{
  GListStore *parent_store;
  guint n_items, i;

  if (!node->parent)
  {
    /* Root level - get from ih */
    GListStore *root_store = (GListStore*)iupAttribGet(node->ih, "_IUPGTK4_ROOT_STORE");
    parent_store = root_store;
  }
  else
  {
    parent_store = node->parent->children;
  }

  if (!parent_store)
    return -1;

  n_items = g_list_model_get_n_items(G_LIST_MODEL(parent_store));
  for (i = 0; i < n_items; i++)
  {
    gpointer item = g_list_model_get_item(G_LIST_MODEL(parent_store), i);
    gboolean found = (item == node);
    g_object_unref(item);
    if (found)
      return (int)i;
  }

  return -1;
}

/* Get the GListStore that contains a node */
static GListStore*
iupgtk4TreeGetParentStore(Ihandle *ih, IupGtk4TreeNode *node)
{
  if (!node->parent)
    return (GListStore*)iupAttribGet(ih, "_IUPGTK4_ROOT_STORE");
  return node->parent->children;
}

/* Count total children recursively */
static int
iupgtk4TreeTotalChildCountRec(IupGtk4TreeNode *node)
{
  int count = 0;
  int i, n;

  if (!node->children)
    return 0;

  n = g_list_model_get_n_items(G_LIST_MODEL(node->children));
  for (i = 0; i < n; i++)
  {
    IupGtk4TreeNode *child = g_list_model_get_item(G_LIST_MODEL(node->children), i);
    count++;
    count += iupgtk4TreeTotalChildCountRec(child);
    g_object_unref(child);
  }

  return count;
}

/* Get toggle check state: 1=checked, 0=unchecked, -1=indeterminate */
static int
iupgtk4TreeToggleGetCheck(IupGtk4TreeNode *node, int show_toggle)
{
  if (node->three_state && show_toggle == 2)
    return -1;
  return node->check ? 1 : 0;
}

/* Get the GtkTreeListRow for a node at a given position */
static GtkTreeListRow*
iupgtk4TreeGetRowAtPosition(Ihandle *ih, int pos)
{
  GtkTreeListModel *tree_model = GTK_TREE_LIST_MODEL(iupAttribGet(ih, "_IUPGTK4_TREE_MODEL"));
  return gtk_tree_list_model_get_row(tree_model, pos);
}

/* Check if a node is currently visible (all ancestors expanded) */
static gboolean
iupgtk4TreeIsNodeVisible(Ihandle *ih, IupGtk4TreeNode *node)
{
  GtkTreeListModel *tree_model = GTK_TREE_LIST_MODEL(iupAttribGet(ih, "_IUPGTK4_TREE_MODEL"));
  IupGtk4TreeNode *current = node->parent;

  while (current)
  {
    /* Find the row for this ancestor */
    int pos = iupgtk4TreeGetVisiblePosition(ih, current);
    if (pos < 0)
      return FALSE;

    GtkTreeListRow *row = gtk_tree_list_model_get_row(tree_model, pos);
    if (!row)
      return FALSE;

    gboolean expanded = gtk_tree_list_row_get_expanded(row);
    g_object_unref(row);

    if (!expanded)
      return FALSE;

    current = current->parent;
  }

  return TRUE;
}


/*****************************************************************************/
/* Item widget structure - used by factory callbacks and rename functions    */
/*****************************************************************************/

typedef struct {
  GtkWidget *expander;
  GtkWidget *box;
  GtkWidget *check;
  GtkWidget *image;
  GtkWidget *label;       /* GtkLabel if !show_rename, GtkOverlay containing GtkLabel if show_rename */
  gulong row_notify_handler;
  gulong check_toggled_handler;
} IupGtk4TreeItemWidgets;

/*****************************************************************************/
/* Rename editing support using GtkLabel + GtkOverlay + GtkText              */
/*****************************************************************************/

/* Forward declaration */
static void iupgtk4TreeStartRenameEditing(GtkListItem *list_item);

/* Helper to clean up the rename entry widget */
static void
iupgtk4TreeCleanupRenameEntry(GtkWidget *entry)
{
  GtkWidget *overlay = gtk_widget_get_parent(entry);
  GtkWidget *label;

  if (!overlay || !GTK_IS_OVERLAY(overlay))
    return;

  label = gtk_overlay_get_child(GTK_OVERLAY(overlay));
  gtk_overlay_remove_overlay(GTK_OVERLAY(overlay), entry);
  if (label)
    gtk_widget_set_visible(label, TRUE);
}

/* Called when the GtkText entry is activated (Enter pressed) or loses focus */
static void
iupgtk4TreeFinishRenameEditing(GtkWidget *entry, gpointer user_data)
{
  GtkListItem *list_item = GTK_LIST_ITEM(user_data);
  GtkTreeListRow *row;
  gpointer item;
  IupGtk4TreeNode *node;
  Ihandle *ih;
  int id;
  GtkWidget *overlay;
  GtkWidget *label;
  const char *new_text;
  const char *old_text;

  overlay = gtk_widget_get_parent(entry);
  if (!overlay || !GTK_IS_OVERLAY(overlay))
    return;

  row = gtk_list_item_get_item(list_item);
  if (!row)
    return;

  item = gtk_tree_list_row_get_item(row);
  if (!item)
    return;

  node = IUP_GTK4_TREE_NODE(item);
  ih = node->ih;
  id = iupgtk4TreeFindNodeId(ih, node);

  old_text = iupAttribGet(ih, "_IUPTREE_RENAME_OLDTEXT");
  if (!old_text)
  {
    g_object_unref(item);
    return;
  }

  new_text = gtk_editable_get_text(GTK_EDITABLE(entry));

  if (new_text && strcmp(new_text, old_text) != 0)
  {
    IFnis cbRename = (IFnis)IupGetCallback(ih, "RENAME_CB");
    if (cbRename)
    {
      if (cbRename(ih, id, iupgtk4StrConvertFromSystem(new_text)) != IUP_IGNORE)
      {
        g_free(node->title);
        node->title = g_strdup(new_text);

        label = gtk_overlay_get_child(GTK_OVERLAY(overlay));
        if (label)
          gtk_label_set_text(GTK_LABEL(label), new_text);
      }
    }
    else
    {
      g_free(node->title);
      node->title = g_strdup(new_text);

      label = gtk_overlay_get_child(GTK_OVERLAY(overlay));
      if (label)
        gtk_label_set_text(GTK_LABEL(label), new_text);
    }
  }

  iupAttribSet(ih, "_IUPTREE_RENAME_OLDTEXT", NULL);
  iupAttribSet(ih, "_IUPTREE_EDITING_ENTRY", NULL);

  iupgtk4TreeCleanupRenameEntry(entry);

  g_object_unref(item);
}

/* Called when Escape is pressed during editing */
static void
iupgtk4TreeCancelRenameEditing(GtkWidget *entry, gpointer user_data)
{
  GtkListItem *list_item = GTK_LIST_ITEM(user_data);
  GtkWidget *overlay;
  GtkTreeListRow *row;
  gpointer item;

  overlay = gtk_widget_get_parent(entry);
  if (!overlay || !GTK_IS_OVERLAY(overlay))
    return;

  row = gtk_list_item_get_item(list_item);
  if (row)
  {
    item = gtk_tree_list_row_get_item(row);
    if (item)
    {
      IupGtk4TreeNode *node = IUP_GTK4_TREE_NODE(item);
      Ihandle *ih = node->ih;
      iupAttribSet(ih, "_IUPTREE_RENAME_OLDTEXT", NULL);
      iupAttribSet(ih, "_IUPTREE_EDITING_ENTRY", NULL);
      g_object_unref(item);
    }
  }

  iupgtk4TreeCleanupRenameEntry(entry);
}

/* Key press handler for the entry - handle Escape */
static gboolean
iupgtk4TreeRenameEntryKeyPressed(GtkEventControllerKey *controller, guint keyval,
                                  guint keycode, GdkModifierType state, gpointer user_data)
{
  if (keyval == GDK_KEY_Escape)
  {
    GtkWidget *entry = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(controller));
    iupgtk4TreeCancelRenameEditing(entry, user_data);
    return TRUE;
  }
  return FALSE;

  (void)keycode;
  (void)state;
}

/* Focus out handler - finish editing when focus leaves */
static void
iupgtk4TreeRenameEntryFocusOut(GtkEventControllerFocus *controller, gpointer user_data)
{
  GtkWidget *entry = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(controller));
  iupgtk4TreeFinishRenameEditing(entry, user_data);
}

/* Start rename editing on the specified list item */
static void
iupgtk4TreeStartRenameEditing(GtkListItem *list_item)
{
  IupGtk4TreeItemWidgets *widgets;
  GtkTreeListRow *row;
  gpointer item;
  IupGtk4TreeNode *node;
  Ihandle *ih;
  int id;
  GtkWidget *entry;
  GtkWidget *label;
  GtkEventController *key_controller;
  GtkEventController *focus_controller;

  widgets = g_object_get_data(G_OBJECT(list_item), "iup-widgets");
  if (!widgets || !widgets->label)
    return;

  row = gtk_list_item_get_item(list_item);
  if (!row)
    return;

  item = gtk_tree_list_row_get_item(row);
  if (!item)
    return;

  node = IUP_GTK4_TREE_NODE(item);
  ih = node->ih;
  id = iupgtk4TreeFindNodeId(ih, node);

  IFni cbShowRename = (IFni)IupGetCallback(ih, "SHOWRENAME_CB");
  if (cbShowRename && cbShowRename(ih, id) == IUP_IGNORE)
  {
    g_object_unref(item);
    return;
  }

  iupAttribSet(ih, "_IUPTREE_RENAME_OLDTEXT", node->title);

  label = gtk_overlay_get_child(GTK_OVERLAY(widgets->label));

  entry = gtk_text_new();
  gtk_editable_set_text(GTK_EDITABLE(entry), node->title ? node->title : "");
  gtk_widget_set_halign(entry, GTK_ALIGN_FILL);
  gtk_widget_set_valign(entry, GTK_ALIGN_FILL);
  gtk_widget_set_hexpand(entry, TRUE);

  g_signal_connect(entry, "activate", G_CALLBACK(iupgtk4TreeFinishRenameEditing), list_item);

  key_controller = gtk_event_controller_key_new();
  g_signal_connect(key_controller, "key-pressed", G_CALLBACK(iupgtk4TreeRenameEntryKeyPressed), list_item);
  gtk_widget_add_controller(entry, key_controller);

  focus_controller = gtk_event_controller_focus_new();
  g_signal_connect(focus_controller, "leave", G_CALLBACK(iupgtk4TreeRenameEntryFocusOut), list_item);
  gtk_widget_add_controller(entry, focus_controller);

  gtk_widget_set_visible(label, FALSE);
  gtk_overlay_add_overlay(GTK_OVERLAY(widgets->label), entry);

  iupAttribSet(ih, "_IUPTREE_EDITING_ENTRY", (char*)entry);

  const char *caret = iupAttribGetStr(ih, "RENAMECARET");
  if (caret)
  {
    int pos = 0;
    if (iupStrToInt(caret, &pos))
    {
      pos--;
      if (pos < 0) pos = 0;
      gtk_editable_set_position(GTK_EDITABLE(entry), pos);
    }
  }

  const char *selection = iupAttribGetStr(ih, "RENAMESELECTION");
  if (selection)
  {
    int start = 0, end = 0;
    if (iupStrToIntInt(selection, &start, &end, ':') == 2)
    {
      start--;
      end--;
      if (start < 0) start = 0;
      if (end < 0) end = 0;
      gtk_editable_select_region(GTK_EDITABLE(entry), start, end);
    }
  }
  else
  {
    gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1);
  }

  gtk_widget_grab_focus(entry);

  g_object_unref(item);
}

/* Click handler for GtkLabel - handles selection and triggers editing */
static void
iupgtk4TreeLabelClickPressed(GtkGestureClick *gesture, int n_press, double x, double y, gpointer user_data)
{
  GtkListItem *list_item = GTK_LIST_ITEM(user_data);
  guint button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));

  GtkTreeListRow *row = gtk_list_item_get_item(list_item);
  if (!row)
    return;

  gpointer item = gtk_tree_list_row_get_item(row);
  if (!item)
    return;

  IupGtk4TreeNode *node = IUP_GTK4_TREE_NODE(item);
  Ihandle *ih = node->ih;
  int id = iupgtk4TreeFindNodeId(ih, node);

  /* Handle right-click */
  if (button == GDK_BUTTON_SECONDARY)
  {
    gtk_gesture_set_state(GTK_GESTURE(gesture), GTK_EVENT_SEQUENCE_CLAIMED);

    IFni cb = (IFni)IupGetCallback(ih, "RIGHTCLICK_CB");
    if (cb)
      cb(ih, id);

    g_object_unref(item);
    return;
  }

  /* Handle left-click */
  if (button == GDK_BUTTON_PRIMARY)
  {
    gboolean is_selected = gtk_list_item_get_selected(list_item);

    if (is_selected && n_press == 1)
    {
      gtk_gesture_set_state(GTK_GESTURE(gesture), GTK_EVENT_SEQUENCE_CLAIMED);
      iupgtk4TreeStartRenameEditing(list_item);
      g_object_unref(item);
      return;
    }
    else if (n_press == 2)
    {
      gtk_gesture_set_state(GTK_GESTURE(gesture), GTK_EVENT_SEQUENCE_CLAIMED);
      iupgtk4TreeStartRenameEditing(list_item);
      g_object_unref(item);
      return;
    }
    else
    {
      gtk_gesture_set_state(GTK_GESTURE(gesture), GTK_EVENT_SEQUENCE_CLAIMED);

      GtkWidget *listview = ih->handle;
      GtkSelectionModel *selection = gtk_list_view_get_model(GTK_LIST_VIEW(listview));
      guint position = gtk_list_item_get_position(list_item);
      gtk_selection_model_select_item(selection, position, TRUE);

      g_object_unref(item);
      return;
    }
  }

  g_object_unref(item);
  (void)x;
  (void)y;
}

/*****************************************************************************/
/* GtkSignalListItemFactory callbacks                                        */
/*****************************************************************************/

static void
iupgtk4TreeRowExpandedChanged(GObject *row, GParamSpec *pspec, gpointer user_data)
{
  GtkListItem *list_item = GTK_LIST_ITEM(user_data);
  IupGtk4TreeItemWidgets *widgets = g_object_get_data(G_OBJECT(list_item), "iup-widgets");
  GtkTreeListRow *tree_row = GTK_TREE_LIST_ROW(row);

  if (!widgets)
    return;

  gpointer item = gtk_tree_list_row_get_item(tree_row);
  if (!item)
    return;

  IupGtk4TreeNode *node = IUP_GTK4_TREE_NODE(item);
  Ihandle *ih = node->ih;
  gboolean expanded = gtk_tree_list_row_get_expanded(tree_row);

  /* Update image based on expanded state */
  GdkTexture *texture = NULL;
  if (node->kind == ITREE_BRANCH)
  {
    if (expanded)
    {
      if (node->has_image_expanded && node->image_expanded)
        texture = node->image_expanded;
      else
        texture = ih->data->def_image_expanded;
    }
    else
    {
      if (node->has_image && node->image)
        texture = node->image;
      else
        texture = ih->data->def_image_collapsed;
    }
  }

  if (texture && widgets->image)
  {
    gtk_image_set_from_paintable(GTK_IMAGE(widgets->image), GDK_PAINTABLE(texture));
  }

  /* Call IUP callbacks if not being ignored */
  if (!iupAttribGet(ih, "_IUPTREE_IGNORE_BRANCH_CB"))
  {
    int id = iupgtk4TreeFindNodeId(ih, node);
    if (expanded)
    {
      IFni cb = (IFni)IupGetCallback(ih, "BRANCHOPEN_CB");
      if (cb)
        cb(ih, id);
    }
    else
    {
      IFni cb = (IFni)IupGetCallback(ih, "BRANCHCLOSE_CB");
      if (cb)
        cb(ih, id);
    }
  }

  g_object_unref(item);
  (void)pspec;
}

static void
iupgtk4TreeCheckToggled(GtkCheckButton *check, gpointer user_data)
{
  GtkListItem *list_item = GTK_LIST_ITEM(user_data);
  GtkTreeListRow *row = gtk_list_item_get_item(list_item);

  if (!row)
    return;

  gpointer item = gtk_tree_list_row_get_item(row);
  if (!item)
    return;

  IupGtk4TreeNode *node = IUP_GTK4_TREE_NODE(item);
  Ihandle *ih = node->ih;

  gboolean active = gtk_check_button_get_active(check);
  node->check = active;

  /* Call toggle callback */
  IFnii cb = (IFnii)IupGetCallback(ih, "TOGGLEVALUE_CB");
  if (cb)
  {
    int id = iupgtk4TreeFindNodeId(ih, node);
    cb(ih, id, active ? 1 : 0);
  }

  /* Handle MARKWHENTOGGLE */
  if (iupAttribGetBoolean(ih, "MARKWHENTOGGLE"))
  {
    int id = iupgtk4TreeFindNodeId(ih, node);
    IupSetAttributeId(ih, "MARKED", id, active ? "Yes" : "No");
  }

  g_object_unref(item);
}

static void
iupgtk4TreeSetupCb(GtkListItemFactory *factory, GtkListItem *list_item, gpointer user_data)
{
  Ihandle *ih = (Ihandle*)user_data;
  IupGtk4TreeItemWidgets *widgets = g_new0(IupGtk4TreeItemWidgets, 1);

  /* Create widget hierarchy */

  widgets->expander = gtk_tree_expander_new();
  widgets->box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  widgets->image = gtk_image_new();

  if (ih->data->show_rename)
  {
    GtkWidget *overlay;
    GtkWidget *label;
    GtkGesture *click_gesture;

    overlay = gtk_overlay_new();
    label = gtk_label_new(NULL);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_widget_set_hexpand(label, TRUE);
    gtk_overlay_set_child(GTK_OVERLAY(overlay), label);
    gtk_widget_set_hexpand(overlay, TRUE);

    click_gesture = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click_gesture), 0);
    g_signal_connect(click_gesture, "pressed", G_CALLBACK(iupgtk4TreeLabelClickPressed), list_item);
    gtk_widget_add_controller(label, GTK_EVENT_CONTROLLER(click_gesture));

    g_object_set_data(G_OBJECT(overlay), "iup-list-item", list_item);

    widgets->label = overlay;
  }
  else
  {
    widgets->label = gtk_label_new(NULL);
    gtk_label_set_xalign(GTK_LABEL(widgets->label), 0.0);
    gtk_widget_set_hexpand(widgets->label, TRUE);
  }

  if (ih->data->show_toggle)
  {
    widgets->check = gtk_check_button_new();
    gtk_box_append(GTK_BOX(widgets->box), widgets->check);
  }

  gtk_box_append(GTK_BOX(widgets->box), widgets->image);
  gtk_box_append(GTK_BOX(widgets->box), widgets->label);

  gtk_tree_expander_set_child(GTK_TREE_EXPANDER(widgets->expander), widgets->box);
  gtk_list_item_set_child(list_item, widgets->expander);

  /* Disable list item focus, keep it in expander for keybindings */
  gtk_list_item_set_focusable(list_item, FALSE);

  /* Store widget references */
  g_object_set_data_full(G_OBJECT(list_item), "iup-widgets", widgets, g_free);

  (void)factory;
}

static void
iupgtk4TreeBindCb(GtkListItemFactory *factory, GtkListItem *list_item, gpointer user_data)
{
  Ihandle *ih = (Ihandle*)user_data;
  IupGtk4TreeItemWidgets *widgets = g_object_get_data(G_OBJECT(list_item), "iup-widgets");
  GtkTreeListRow *row = gtk_list_item_get_item(list_item);

  if (!widgets || !row)
    return;

  gpointer item = gtk_tree_list_row_get_item(row);
  if (!item)
    return;

  IupGtk4TreeNode *node = IUP_GTK4_TREE_NODE(item);

  /* Store reference from node to its current list_item for rename lookup */
  g_object_set_data(G_OBJECT(node), "_iup_list_item", list_item);

  /* Set the tree list row on expander */
  gtk_tree_expander_set_list_row(GTK_TREE_EXPANDER(widgets->expander), row);

  /* Bind title - widgets->label is GtkOverlay (if show_rename) or GtkLabel */
  if (ih->data->show_rename)
  {
    GtkWidget *label = gtk_overlay_get_child(GTK_OVERLAY(widgets->label));
    gtk_label_set_text(GTK_LABEL(label), node->title ? node->title : "");
  }
  else
  {
    gtk_label_set_text(GTK_LABEL(widgets->label), node->title ? node->title : "");
  }

  /* Bind image based on kind and expanded state */
  GdkTexture *texture = NULL;
  gboolean expanded = gtk_tree_list_row_get_expanded(row);

  if (node->kind == ITREE_BRANCH)
  {
    if (expanded)
    {
      if (node->has_image_expanded && node->image_expanded)
        texture = node->image_expanded;
      else
        texture = ih->data->def_image_expanded;
    }
    else
    {
      if (node->has_image && node->image)
        texture = node->image;
      else
        texture = ih->data->def_image_collapsed;
    }
  }
  else
  {
    if (node->has_image && node->image)
      texture = node->image;
    else
      texture = ih->data->def_image_leaf;
  }

  if (texture)
  {
    gtk_image_set_from_paintable(GTK_IMAGE(widgets->image), GDK_PAINTABLE(texture));
  }
  else
  {
    gtk_image_clear(GTK_IMAGE(widgets->image));
  }

  /* Apply font and color */
  {
    GtkWidget *label_widget = ih->data->show_rename ?
      gtk_overlay_get_child(GTK_OVERLAY(widgets->label)) : widgets->label;

    if (node->font)
    {
      PangoAttrList *attrs = pango_attr_list_new();
      pango_attr_list_insert(attrs, pango_attr_font_desc_new(node->font));
      gtk_label_set_attributes(GTK_LABEL(label_widget), attrs);
      pango_attr_list_unref(attrs);
    }
    else
    {
      gtk_label_set_attributes(GTK_LABEL(label_widget), NULL);
    }

    if (node->color)
    {
      PangoAttrList *attrs = gtk_label_get_attributes(GTK_LABEL(label_widget));
      if (!attrs)
        attrs = pango_attr_list_new();
      else
        attrs = pango_attr_list_copy(attrs);

      PangoAttribute *color_attr = pango_attr_foreground_new(
        (guint16)(node->color->red * 65535),
        (guint16)(node->color->green * 65535),
        (guint16)(node->color->blue * 65535));
      pango_attr_list_insert(attrs, color_attr);
      gtk_label_set_attributes(GTK_LABEL(label_widget), attrs);
      pango_attr_list_unref(attrs);
    }
  }

  /* Handle toggle */
  if (widgets->check)
  {
    gtk_widget_set_visible(widgets->check, node->toggle_visible);
    gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets->check), node->check);

    if (ih->data->show_toggle == 2)
      gtk_check_button_set_inconsistent(GTK_CHECK_BUTTON(widgets->check), node->three_state);

    /* Connect toggle signal */
    widgets->check_toggled_handler = g_signal_connect(widgets->check, "toggled",
                                                       G_CALLBACK(iupgtk4TreeCheckToggled), list_item);
  }

  /* Watch for expand/collapse changes */
  widgets->row_notify_handler = g_signal_connect(row, "notify::expanded",
                                                  G_CALLBACK(iupgtk4TreeRowExpandedChanged), list_item);

  g_object_unref(item);
  (void)factory;
}

static void
iupgtk4TreeUnbindCb(GtkListItemFactory *factory, GtkListItem *list_item, gpointer user_data)
{
  IupGtk4TreeItemWidgets *widgets = g_object_get_data(G_OBJECT(list_item), "iup-widgets");
  GtkTreeListRow *row = gtk_list_item_get_item(list_item);

  /* Clear the node->list_item reference */
  if (row)
  {
    gpointer item = gtk_tree_list_row_get_item(row);
    if (item)
    {
      g_object_set_data(G_OBJECT(item), "_iup_list_item", NULL);
      g_object_unref(item);
    }
  }

  if (widgets)
  {
    /* Disconnect signals */
    if (row && widgets->row_notify_handler)
    {
      g_signal_handler_disconnect(row, widgets->row_notify_handler);
      widgets->row_notify_handler = 0;
    }

    if (widgets->check && widgets->check_toggled_handler)
    {
      g_signal_handler_disconnect(widgets->check, widgets->check_toggled_handler);
      widgets->check_toggled_handler = 0;
    }

    /* Clear expander's list row reference */
    gtk_tree_expander_set_list_row(GTK_TREE_EXPANDER(widgets->expander), NULL);
  }

  (void)factory;
  (void)user_data;
}

static void
iupgtk4TreeTeardownCb(GtkListItemFactory *factory, GtkListItem *list_item, gpointer user_data)
{
  /* Widget data is automatically freed by g_object_set_data_full */
  (void)factory;
  (void)list_item;
  (void)user_data;
}

/*****************************************************************************/
/* Selection handling                                                        */
/*****************************************************************************/

static void
iupgtk4TreeSelectionChanged(GtkSelectionModel *selection, guint position, guint n_items, gpointer user_data)
{
  Ihandle *ih = (Ihandle*)user_data;
  GtkTreeListModel *tree_model;
  IFnii cbSelec;
  guint i;

  if (iupAttribGet(ih, "_IUPTREE_IGNORE_SELECTION_CB"))
    return;

  tree_model = GTK_TREE_LIST_MODEL(iupAttribGet(ih, "_IUPGTK4_TREE_MODEL"));
  if (!tree_model)
    return;

  cbSelec = (IFnii)IupGetCallback(ih, "SELECTION_CB");

  for (i = position; i < position + n_items; i++)
  {
    GtkTreeListRow *row = gtk_tree_list_model_get_row(tree_model, i);
    if (row)
    {
      gpointer item = gtk_tree_list_row_get_item(row);
      if (item)
      {
        IupGtk4TreeNode *node = IUP_GTK4_TREE_NODE(item);
        gboolean selected = gtk_selection_model_is_selected(selection, i);

        /* Always update node's selection state */
        node->selected = selected;

        /* Call callback if registered */
        if (cbSelec)
        {
          int id = iupgtk4TreeFindNodeId(ih, node);
          cbSelec(ih, id, selected ? 1 : 0);
        }

        g_object_unref(item);
      }
      g_object_unref(row);
    }
  }
}

/*****************************************************************************/
/* Row activation                                                            */
/*****************************************************************************/

static void
iupgtk4TreeRowActivated(GtkListView *list_view, guint position, gpointer user_data)
{
  Ihandle *ih = (Ihandle*)user_data;
  GtkTreeListModel *tree_model = GTK_TREE_LIST_MODEL(iupAttribGet(ih, "_IUPGTK4_TREE_MODEL"));

  GtkTreeListRow *row = gtk_tree_list_model_get_row(tree_model, position);
  if (!row)
    return;

  gpointer item = gtk_tree_list_row_get_item(row);
  if (!item)
  {
    g_object_unref(row);
    return;
  }

  IupGtk4TreeNode *node = IUP_GTK4_TREE_NODE(item);
  int id = iupgtk4TreeFindNodeId(ih, node);

  if (node->kind == ITREE_LEAF)
  {
    IFni cb = (IFni)IupGetCallback(ih, "EXECUTELEAF_CB");
    if (cb)
      cb(ih, id);
  }
  else
  {
    IFni cb = (IFni)IupGetCallback(ih, "EXECUTEBRANCH_CB");
    if (cb)
      cb(ih, id);

    /* Toggle expand/collapse for branches */
    gboolean expanded = gtk_tree_list_row_get_expanded(row);
    gtk_tree_list_row_set_expanded(row, !expanded);
  }

  g_object_unref(item);
  g_object_unref(row);
  (void)list_view;
}

/*****************************************************************************/
/* Event controllers                                                         */
/*****************************************************************************/

static void
iupgtk4TreeButtonPressed(GtkGestureClick *gesture, int n_press, double x, double y, Ihandle *ih)
{
  int button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));

  /* Call common button handling for BUTTON_CB callback */
  iupgtk4ButtonPressed(gesture, n_press, x, y, ih);

  if (n_press == 1 && button == 3)  /* right single click */
  {
    IFni cbRightClick = (IFni)IupGetCallback(ih, "RIGHTCLICK_CB");
    if (cbRightClick)
    {
      int id = IupGetInt(ih, "VALUE");
      if (id >= 0)
        cbRightClick(ih, id);
    }
  }
}

static void
iupgtk4TreeStartRenameNode(Ihandle *ih, IupGtk4TreeNode *node)
{
  GtkListItem *list_item;

  if (!ih->data->show_rename || !node)
    return;

  /* Get the list_item directly from the node (set during bind) */
  list_item = g_object_get_data(G_OBJECT(node), "_iup_list_item");
  if (list_item)
    iupgtk4TreeStartRenameEditing(list_item);
}

static gboolean
iupgtk4TreeKeyPressed(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, Ihandle *ih)
{
  /* Call common key handling first */
  if (iupgtk4KeyPressEvent(controller, keyval, keycode, state, ih) == TRUE)
    return TRUE;

  if (keyval == GDK_KEY_F2 && ih->data->show_rename)
  {
    int id = IupGetInt(ih, "VALUE");
    if (id >= 0)
    {
      IupGtk4TreeNode *node = iupgtk4TreeGetNodeFromId(ih, id);
      if (node)
        iupgtk4TreeStartRenameNode(ih, node);
    }
    return TRUE;
  }

  return FALSE;
}

static void
iupgtk4TreeSetupEventControllers(Ihandle *ih)
{
  GtkWidget *listview = ih->handle;

  /* Button events via GtkGestureClick */
  GtkGesture *click_gesture = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click_gesture), 0);  /* all buttons */
  gtk_widget_add_controller(listview, GTK_EVENT_CONTROLLER(click_gesture));
  g_signal_connect(click_gesture, "pressed", G_CALLBACK(iupgtk4TreeButtonPressed), ih);

  /* Key events via GtkEventControllerKey */
  GtkEventController *key_controller = gtk_event_controller_key_new();
  gtk_widget_add_controller(listview, key_controller);
  g_signal_connect(key_controller, "key-pressed", G_CALLBACK(iupgtk4TreeKeyPressed), ih);
}

/*****************************************************************************/
/* Default images                                                            */
/*****************************************************************************/

static GdkTexture*
iupgtk4TreeGetThemeIcon(Ihandle *ih, const char *icon_name, int size)
{
  GdkTexture *texture = NULL;

  GtkIconTheme *icon_theme = gtk_icon_theme_get_for_display(gdk_display_get_default());
  GtkIconPaintable *icon_paintable = gtk_icon_theme_lookup_icon(icon_theme, icon_name, NULL, size, 1, GTK_TEXT_DIR_NONE, 0);
  if (icon_paintable)
  {
    GFile *file = gtk_icon_paintable_get_file(icon_paintable);
    if (file)
    {
      GError *error = NULL;
      texture = gdk_texture_new_from_file(file, &error);
      if (error)
      {
        g_error_free(error);
        texture = NULL;
      }
    }
    g_object_unref(icon_paintable);
  }
  (void)ih;

  return texture;
}

static void
iupgtk4TreeInitDefaultImages(Ihandle *ih)
{
  char *img_name;

  /* Leaf image */
  img_name = iupAttribGetStr(ih, "IMAGELEAF");
  if (img_name && !iupStrEqualNoCase(img_name, "IMGLEAF"))
    ih->data->def_image_leaf = iupImageGetImage(img_name, ih, 0, NULL);
  else
  {
    ih->data->def_image_leaf = iupgtk4TreeGetThemeIcon(ih, "text-x-generic", 16);
    if (ih->data->def_image_leaf)
      iupAttribSet(ih, "_IUPGTK4_THEMED_LEAF", (char*)ih->data->def_image_leaf);
  }

  /* Collapsed branch image */
  img_name = iupAttribGetStr(ih, "IMAGEBRANCHCOLLAPSED");
  if (img_name && !iupStrEqualNoCase(img_name, "IMGCOLLAPSED"))
    ih->data->def_image_collapsed = iupImageGetImage(img_name, ih, 0, NULL);
  else
  {
    ih->data->def_image_collapsed = iupgtk4TreeGetThemeIcon(ih, "folder", 16);
    if (ih->data->def_image_collapsed)
      iupAttribSet(ih, "_IUPGTK4_THEMED_COLLAPSED", (char*)ih->data->def_image_collapsed);
  }

  /* Expanded branch image */
  img_name = iupAttribGetStr(ih, "IMAGEBRANCHEXPANDED");
  if (img_name && !iupStrEqualNoCase(img_name, "IMGEXPANDED"))
    ih->data->def_image_expanded = iupImageGetImage(img_name, ih, 0, NULL);
  else
  {
    ih->data->def_image_expanded = iupgtk4TreeGetThemeIcon(ih, "folder-open", 16);
    if (!ih->data->def_image_expanded)
      ih->data->def_image_expanded = iupgtk4TreeGetThemeIcon(ih, "folder", 16);
    if (ih->data->def_image_expanded)
      iupAttribSet(ih, "_IUPGTK4_THEMED_EXPANDED", (char*)ih->data->def_image_expanded);
  }
}

/*****************************************************************************/
/* Node cache management                                                     */
/*****************************************************************************/

static void
iupgtk4TreeRebuildCacheRec(Ihandle *ih, IupGtk4TreeNode *node, int *id)
{
  int i, n;

  if (!node->children)
    return;

  n = g_list_model_get_n_items(G_LIST_MODEL(node->children));
  for (i = 0; i < n; i++)
  {
    IupGtk4TreeNode *child = g_list_model_get_item(G_LIST_MODEL(node->children), i);
    (*id)++;
    ih->data->node_cache[*id].node_handle = (InodeHandle*)child;
    iupgtk4TreeRebuildCacheRec(ih, child, id);
    g_object_unref(child);
  }
}

static void
iupgtk4TreeRebuildNodeCache(Ihandle *ih, int start_id, IupGtk4TreeNode *start_node)
{
  int id = start_id;
  ih->data->node_cache[id].node_handle = (InodeHandle*)start_node;
  iupgtk4TreeRebuildCacheRec(ih, start_node, &id);
}

/*****************************************************************************/
/* ADDING ITEMS                                                              */
/*****************************************************************************/

void iupdrvTreeAddNode(Ihandle *ih, int id, int kind, const char *title, int add)
{
  GListStore *root_store = (GListStore*)iupAttribGet(ih, "_IUPGTK4_ROOT_STORE");
  IupGtk4TreeNode *new_node, *ref_node = NULL;
  GListStore *target_store;
  int insert_pos = 0;
  int kindPrev = -1;

  if (!root_store)
    return;

  /* Get reference node if not adding root */
  if (id >= 0)
  {
    ref_node = iupgtk4TreeGetNodeFromId(ih, id);
    if (!ref_node)
      return;
    kindPrev = ref_node->kind;
  }

  /* Create new node */
  new_node = iup_gtk4_tree_node_new(ih, kind, title);

  /* Set default images */
  if (kind == ITREE_LEAF)
  {
    if (ih->data->def_image_leaf)
    {
      new_node->image = g_object_ref(ih->data->def_image_leaf);
    }
  }
  else
  {
    if (ih->data->def_image_collapsed)
      new_node->image = g_object_ref(ih->data->def_image_collapsed);
    if (ih->data->def_image_expanded)
      new_node->image_expanded = g_object_ref(ih->data->def_image_expanded);
  }

  /* Set foreground color */
  {
    GdkRGBA color;
    if (iupgtk4GetColor(iupAttribGetStr(ih, "FGCOLOR"), &color))
      new_node->color = gdk_rgba_copy(&color);
  }

  /* Determine where to insert */
  if (id == -1)
  {
    /* Insert before root (prepend to root store) */
    target_store = root_store;
    insert_pos = 0;
    new_node->parent = NULL;
  }
  else if (kindPrev == -1)
  {
    /* No reference node found, append to root store */
    target_store = root_store;
    insert_pos = g_list_model_get_n_items(G_LIST_MODEL(root_store));
    new_node->parent = NULL;
  }
  else if (kindPrev == ITREE_BRANCH && add)
  {
    /* Add as first child of branch */
    if (!ref_node->children)
      ref_node->children = g_list_store_new(IUP_GTK4_TYPE_TREE_NODE);
    target_store = ref_node->children;
    insert_pos = 0;
    new_node->parent = ref_node;
  }
  else
  {
    /* Insert after ref_node (as sibling) */
    target_store = iupgtk4TreeGetParentStore(ih, ref_node);
    insert_pos = iupgtk4TreeFindPositionInParent(ref_node) + 1;
    new_node->parent = ref_node->parent;
  }

  /* Insert into store */
  g_list_store_insert(target_store, insert_pos, new_node);

  /* Update IUP cache */
  iupTreeAddToCache(ih, add, kindPrev, (InodeHandle*)ref_node, (InodeHandle*)new_node);

  /* Handle first node setup */
  if (ih->data->node_count == 1)
  {
    /* MarkStart node */
    iupAttribSet(ih, "_IUPTREE_MARKSTART_NODE", (char*)new_node);

    /* Set default selection/focus */
    GtkSelectionModel *selection = GTK_SELECTION_MODEL(iupAttribGet(ih, "_IUPGTK4_SELECTION"));
    if (selection)
    {
      iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
      if (ih->data->mark_mode == ITREE_MARK_SINGLE)
        gtk_selection_model_select_item(selection, 0, TRUE);
      iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
    }
  }

  /* Handle ADDEXPANDED for first child of a branch */
  if (ref_node && kindPrev == ITREE_BRANCH && add)
  {
    int child_count = g_list_model_get_n_items(G_LIST_MODEL(ref_node->children));
    if (child_count == 1)
    {
      /* This is the first child - set expand state based on ADDEXPANDED */
      int pos = iupgtk4TreeGetVisiblePosition(ih, ref_node);
      if (pos >= 0)
      {
        GtkTreeListModel *tree_model = GTK_TREE_LIST_MODEL(iupAttribGet(ih, "_IUPGTK4_TREE_MODEL"));
        GtkTreeListRow *row = gtk_tree_list_model_get_row(tree_model, pos);
        if (row)
        {
          iupAttribSet(ih, "_IUPTREE_IGNORE_BRANCH_CB", "1");
          gtk_tree_list_row_set_expanded(row, ih->data->add_expanded);
          iupAttribSet(ih, "_IUPTREE_IGNORE_BRANCH_CB", NULL);
          g_object_unref(row);
        }
      }
    }
  }

  g_object_unref(new_node);
}

int iupdrvTreeTotalChildCount(Ihandle *ih, InodeHandle *node_handle)
{
  IupGtk4TreeNode *node = (IupGtk4TreeNode*)node_handle;
  return iupgtk4TreeTotalChildCountRec(node);
}

InodeHandle* iupdrvTreeGetFocusNode(Ihandle *ih)
{
  /* In GTK4 ListView, we don't have a direct "cursor" concept. Return the first selected item instead */
  GtkSelectionModel *selection = GTK_SELECTION_MODEL(iupAttribGet(ih, "_IUPGTK4_SELECTION"));
  GtkTreeListModel *tree_model = GTK_TREE_LIST_MODEL(iupAttribGet(ih, "_IUPGTK4_TREE_MODEL"));

  if (!selection || !tree_model)
    return NULL;

  /* Find first selected item */
  guint n_items = g_list_model_get_n_items(G_LIST_MODEL(tree_model));
  for (guint i = 0; i < n_items; i++)
  {
    if (gtk_selection_model_is_selected(selection, i))
    {
      GtkTreeListRow *row = gtk_tree_list_model_get_row(tree_model, i);
      if (row)
      {
        gpointer item = gtk_tree_list_row_get_item(row);
        g_object_unref(row);
        if (item)
        {
          InodeHandle *handle = (InodeHandle*)item;
          g_object_unref(item);
          return handle;
        }
      }
    }
  }

  return NULL;
}

void iupdrvTreeUpdateMarkMode(Ihandle *ih)
{
  GtkSelectionModel *old_selection = GTK_SELECTION_MODEL(iupAttribGet(ih, "_IUPGTK4_SELECTION"));
  GtkTreeListModel *tree_model = GTK_TREE_LIST_MODEL(iupAttribGet(ih, "_IUPGTK4_TREE_MODEL"));
  GtkSelectionModel *new_selection;

  if (!tree_model)
    return;

  /* Create new selection model with appropriate mode */
  if (ih->data->mark_mode == ITREE_MARK_SINGLE)
    new_selection = GTK_SELECTION_MODEL(gtk_single_selection_new(G_LIST_MODEL(g_object_ref(tree_model))));
  else
    new_selection = GTK_SELECTION_MODEL(gtk_multi_selection_new(G_LIST_MODEL(g_object_ref(tree_model))));

  /* Connect selection changed signal */
  g_signal_connect(new_selection, "selection-changed", G_CALLBACK(iupgtk4TreeSelectionChanged), ih);

  /* Update ListView */
  gtk_list_view_set_model(GTK_LIST_VIEW(ih->handle), new_selection);

  /* Store new selection model */
  iupAttribSet(ih, "_IUPGTK4_SELECTION", (char*)new_selection);

  /* Enable rubberband for multi-selection */
  if (ih->data->mark_mode == ITREE_MARK_MULTIPLE && iupAttribGetBoolean(ih, "RUBBERBAND"))
    gtk_list_view_set_enable_rubberband(GTK_LIST_VIEW(ih->handle), TRUE);
  else
    gtk_list_view_set_enable_rubberband(GTK_LIST_VIEW(ih->handle), FALSE);

  /* Release old selection model */
  if (old_selection)
    g_object_unref(old_selection);
}

/*****************************************************************************/
/* XY to Position conversion for drag-drop                                   */
/*****************************************************************************/

static int gtkTreeConvertXYToPos(Ihandle* ih, int x, int y)
{
  GtkWidget *widget;
  GtkWidget *expander;
  gpointer item;
  IupGtk4TreeNode *node;
  int id;

  /* Pick the widget at the given coordinates */
  widget = gtk_widget_pick(ih->handle, (double)x, (double)y, GTK_PICK_DEFAULT);
  if (!widget)
    return -1;

  /* Find the GtkTreeExpander ancestor - this is our top-level item widget.
     Note: GtkListItem is NOT a widget in GTK4, it's a GObject that manages widgets.
     We use GtkTreeExpander which knows about its list row. */
  expander = gtk_widget_get_ancestor(widget, GTK_TYPE_TREE_EXPANDER);
  if (!expander)
    return -1;

  /* Get the item directly from the expander */
  item = gtk_tree_expander_get_item(GTK_TREE_EXPANDER(expander));
  if (!item)
    return -1;

  /* The item is our IupGtk4TreeNode */
  node = IUP_GTK4_TREE_NODE(item);
  id = iupTreeFindNodeId(ih, (InodeHandle*)node);

  return id;
}

/*****************************************************************************/
/* Map/Unmap methods                                                         */
/*****************************************************************************/

static int gtkTreeMapMethod(Ihandle *ih)
{
  GtkWidget *listview;
  GtkWidget *scrolled_window;
  GListStore *root_store;
  GtkTreeListModel *tree_model;
  GtkSelectionModel *selection;
  GtkListItemFactory *factory;

  /* Create root data store */
  root_store = g_list_store_new(IUP_GTK4_TYPE_TREE_NODE);
  iupAttribSet(ih, "_IUPGTK4_ROOT_STORE", (char*)root_store);

  /* Create tree list model */
  tree_model = gtk_tree_list_model_new(
    G_LIST_MODEL(root_store),
    FALSE,                              /* passthrough = FALSE (we need GtkTreeListRow) */
    ih->data->add_expanded,             /* autoexpand */
    iupgtk4TreeCreateChildModel,
    ih,
    NULL
  );
  iupAttribSet(ih, "_IUPGTK4_TREE_MODEL", (char*)tree_model);

  /* Create selection model */
  if (ih->data->mark_mode == ITREE_MARK_SINGLE)
    selection = GTK_SELECTION_MODEL(gtk_single_selection_new(G_LIST_MODEL(tree_model)));
  else
    selection = GTK_SELECTION_MODEL(gtk_multi_selection_new(G_LIST_MODEL(tree_model)));
  iupAttribSet(ih, "_IUPGTK4_SELECTION", (char*)selection);

  /* Create factory */
  factory = gtk_signal_list_item_factory_new();
  g_signal_connect(factory, "setup", G_CALLBACK(iupgtk4TreeSetupCb), ih);
  g_signal_connect(factory, "bind", G_CALLBACK(iupgtk4TreeBindCb), ih);
  g_signal_connect(factory, "unbind", G_CALLBACK(iupgtk4TreeUnbindCb), ih);
  g_signal_connect(factory, "teardown", G_CALLBACK(iupgtk4TreeTeardownCb), ih);

  /* Create list view */
  listview = gtk_list_view_new(selection, factory);
  ih->handle = listview;

  /* Configure list view */
  gtk_list_view_set_single_click_activate(GTK_LIST_VIEW(listview), FALSE);
  if (ih->data->mark_mode == ITREE_MARK_MULTIPLE && iupAttribGetBoolean(ih, "RUBBERBAND"))
    gtk_list_view_set_enable_rubberband(GTK_LIST_VIEW(listview), TRUE);

  /* Scrolled window */
  scrolled_window = gtk_scrolled_window_new();
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), listview);
  gtk_scrolled_window_set_has_frame(GTK_SCROLLED_WINDOW(scrolled_window), TRUE);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  iupAttribSet(ih, "_IUP_EXTRAPARENT", (char*)scrolled_window);

  /* Connect signals */
  g_signal_connect(listview, "activate", G_CALLBACK(iupgtk4TreeRowActivated), ih);
  g_signal_connect(selection, "selection-changed", G_CALLBACK(iupgtk4TreeSelectionChanged), ih);

  /* Event controllers */
  iupgtk4TreeSetupEventControllers(ih);

  /* Setup common GTK4 events */
  iupgtk4SetupEnterLeaveEvents((GtkWidget*)scrolled_window, ih);
  iupgtk4SetupFocusEvents((GtkWidget*)scrolled_window, ih);
  iupgtk4SetupMotionEvents((GtkWidget*)scrolled_window, ih);

  /* Add to parent */
  iupgtk4AddToParent(ih);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    iupgtk4SetCanFocus(listview, 0);

  /* Realize widgets */
  gtk_widget_realize((GtkWidget*)scrolled_window);
  gtk_widget_realize(listview);

  /* Initialize default images */
  iupgtk4TreeInitDefaultImages(ih);

  /* Add root node if ADDROOT=YES */
  if (iupAttribGetInt(ih, "ADDROOT"))
    iupdrvTreeAddNode(ih, -1, ITREE_BRANCH, "", 0);

  /* Configure for DROP of files */
  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSet(ih, "DROPFILESTARGET", "YES");

  /* Update mnemonic */
  iupgtk4UpdateMnemonic(ih);

  /* Register XY to position callback for drag-drop */
  IupSetCallback(ih, "_IUP_XY2POS_CB", (Icallback)gtkTreeConvertXYToPos);

  return IUP_NOERROR;
}

static void gtkTreeUnMapMethod(Ihandle *ih)
{
  GdkTexture *texture;
  GtkListView *list_view = GTK_LIST_VIEW(ih->handle);

  ih->data->node_count = 0;

  /* Disconnect the model from the ListView BEFORE destroying anything */
  if (list_view)
    gtk_list_view_set_model(list_view, NULL);

  /* Destroy the native widget, this releases all GTK objects */
  iupdrvBaseUnMapMethod(ih);

  /* Free themed icons if they were created, these we DO own */
  texture = (GdkTexture*)iupAttribGet(ih, "_IUPGTK4_THEMED_LEAF");
  if (texture)
    g_object_unref(texture);

  texture = (GdkTexture*)iupAttribGet(ih, "_IUPGTK4_THEMED_COLLAPSED");
  if (texture)
    g_object_unref(texture);

  texture = (GdkTexture*)iupAttribGet(ih, "_IUPGTK4_THEMED_EXPANDED");
  if (texture)
    g_object_unref(texture);
}

/*****************************************************************************/
/* GET AND SET ATTRIBUTES                                                    */
/*****************************************************************************/

static char* gtkTreeGetIndentationAttrib(Ihandle *ih)
{
  /* GtkListView doesn't have direct indentation control;
     GtkTreeExpander handles indentation automatically */
  return "0";
}

static int gtkTreeSetIndentationAttrib(Ihandle *ih, const char *value)
{
  /* GtkTreeExpander handles indentation automatically */
  (void)ih;
  (void)value;
  return 0;
}

static int gtkTreeSetTopItemAttrib(Ihandle *ih, const char *value)
{
  IupGtk4TreeNode *node;
  int id;

  if (!iupStrToInt(value, &id))
    return 0;

  node = iupgtk4TreeGetNodeFromId(ih, id);
  if (!node)
    return 0;

  /* Scroll to make item visible */
  int pos = iupgtk4TreeGetVisiblePosition(ih, node);
  if (pos >= 0)
    gtk_list_view_scroll_to(GTK_LIST_VIEW(ih->handle), pos, GTK_LIST_SCROLL_FOCUS, NULL);

  return 0;
}

static int gtkTreeSetSpacingAttrib(Ihandle *ih, const char *value)
{
  iupStrToInt(value, &ih->data->spacing);
  /* Spacing is applied in factory bind callback via widget properties */
  return ih->handle ? 0 : 1;
}

static int gtkTreeSetExpandAllAttrib(Ihandle *ih, const char *value)
{
  GtkTreeListModel *tree_model = GTK_TREE_LIST_MODEL(iupAttribGet(ih, "_IUPGTK4_TREE_MODEL"));
  gboolean expand = iupStrBoolean(value);
  guint n_items, i;

  if (!tree_model)
    return 0;

  iupAttribSet(ih, "_IUPTREE_IGNORE_BRANCH_CB", "1");

  n_items = g_list_model_get_n_items(G_LIST_MODEL(tree_model));
  for (i = 0; i < n_items; i++)
  {
    GtkTreeListRow *row = gtk_tree_list_model_get_row(tree_model, i);
    if (row)
    {
      if (gtk_tree_list_row_is_expandable(row))
        gtk_tree_list_row_set_expanded(row, expand);
      g_object_unref(row);
    }

    /* Re-get n_items as expanding may have changed it */
    if (expand)
      n_items = g_list_model_get_n_items(G_LIST_MODEL(tree_model));
  }

  iupAttribSet(ih, "_IUPTREE_IGNORE_BRANCH_CB", NULL);

  return 0;
}

static char* gtkTreeGetDepthAttrib(Ihandle *ih, int id)
{
  IupGtk4TreeNode *node = iupgtk4TreeGetNodeFromId(ih, id);
  int depth = 0;

  if (!node)
    return NULL;

  /* Count parents */
  IupGtk4TreeNode *p = node->parent;
  while (p)
  {
    depth++;
    p = p->parent;
  }

  return iupStrReturnInt(depth);
}

static char* gtkTreeGetKindAttrib(Ihandle *ih, int id)
{
  IupGtk4TreeNode *node = iupgtk4TreeGetNodeFromId(ih, id);
  if (!node)
    return NULL;

  return (node->kind == ITREE_BRANCH) ? "BRANCH" : "LEAF";
}

static char* gtkTreeGetParentAttrib(Ihandle *ih, int id)
{
  IupGtk4TreeNode *node = iupgtk4TreeGetNodeFromId(ih, id);
  if (!node || !node->parent)
    return NULL;

  return iupStrReturnInt(iupgtk4TreeFindNodeId(ih, node->parent));
}

static char* gtkTreeGetStateAttrib(Ihandle *ih, int id)
{
  IupGtk4TreeNode *node = iupgtk4TreeGetNodeFromId(ih, id);
  if (!node || node->kind != ITREE_BRANCH)
    return NULL;

  int pos = iupgtk4TreeGetVisiblePosition(ih, node);
  if (pos < 0)
    return "COLLAPSED";

  GtkTreeListModel *tree_model = GTK_TREE_LIST_MODEL(iupAttribGet(ih, "_IUPGTK4_TREE_MODEL"));
  GtkTreeListRow *row = gtk_tree_list_model_get_row(tree_model, pos);
  if (!row)
    return "COLLAPSED";

  gboolean expanded = gtk_tree_list_row_get_expanded(row);
  g_object_unref(row);

  return expanded ? "EXPANDED" : "COLLAPSED";
}

static int gtkTreeSetStateAttrib(Ihandle *ih, int id, const char *value)
{
  IupGtk4TreeNode *node = iupgtk4TreeGetNodeFromId(ih, id);
  if (!node || node->kind != ITREE_BRANCH)
    return 0;

  int pos = iupgtk4TreeGetVisiblePosition(ih, node);
  if (pos < 0)
    return 0;

  GtkTreeListModel *tree_model = GTK_TREE_LIST_MODEL(iupAttribGet(ih, "_IUPGTK4_TREE_MODEL"));
  GtkTreeListRow *row = gtk_tree_list_model_get_row(tree_model, pos);
  if (!row)
    return 0;

  iupAttribSet(ih, "_IUPTREE_IGNORE_BRANCH_CB", "1");
  gtk_tree_list_row_set_expanded(row, iupStrEqualNoCase(value, "EXPANDED"));
  iupAttribSet(ih, "_IUPTREE_IGNORE_BRANCH_CB", NULL);

  g_object_unref(row);

  return 0;
}

static char* gtkTreeGetTitleAttrib(Ihandle *ih, int id)
{
  IupGtk4TreeNode *node = iupgtk4TreeGetNodeFromId(ih, id);
  if (!node)
    return NULL;

  return iupStrReturnStr(node->title);
}

static int gtkTreeSetTitleAttrib(Ihandle *ih, int id, const char *value)
{
  IupGtk4TreeNode *node = iupgtk4TreeGetNodeFromId(ih, id);
  if (!node)
    return 0;

  g_free(node->title);
  node->title = g_strdup(value ? value : "");

  iupgtk4TreeNotifyNodeChanged(ih, node);

  return 0;
}

static char* gtkTreeGetValueAttrib(Ihandle *ih)
{
  GtkSelectionModel *selection = GTK_SELECTION_MODEL(iupAttribGet(ih, "_IUPGTK4_SELECTION"));
  GtkTreeListModel *tree_model = GTK_TREE_LIST_MODEL(iupAttribGet(ih, "_IUPGTK4_TREE_MODEL"));

  if (!selection || !tree_model)
    return "-1";

  /* Find first selected item */
  guint n_items = g_list_model_get_n_items(G_LIST_MODEL(tree_model));
  for (guint i = 0; i < n_items; i++)
  {
    if (gtk_selection_model_is_selected(selection, i))
    {
      GtkTreeListRow *row = gtk_tree_list_model_get_row(tree_model, i);
      if (row)
      {
        gpointer item = gtk_tree_list_row_get_item(row);
        g_object_unref(row);
        if (item)
        {
          int id = iupgtk4TreeFindNodeId(ih, (IupGtk4TreeNode*)item);
          g_object_unref(item);
          return iupStrReturnInt(id);
        }
      }
    }
  }

  if (ih->data->node_count)
    return "0";
  return "-1";
}

static int gtkTreeSetValueAttrib(Ihandle *ih, const char *value)
{
  GtkSelectionModel *selection = GTK_SELECTION_MODEL(iupAttribGet(ih, "_IUPGTK4_SELECTION"));
  GtkTreeListModel *tree_model = GTK_TREE_LIST_MODEL(iupAttribGet(ih, "_IUPGTK4_TREE_MODEL"));
  IupGtk4TreeNode *node;
  int id;

  if (!selection || !tree_model)
    return 0;

  /* Handle special values */
  if (iupStrEqualNoCase(value, "ROOT") || iupStrEqualNoCase(value, "FIRST"))
    id = 0;
  else if (iupStrEqualNoCase(value, "LAST"))
    id = ih->data->node_count - 1;
  else if (!iupStrToInt(value, &id))
    return 0;

  node = iupgtk4TreeGetNodeFromId(ih, id);
  if (!node)
    return 0;

  int pos = iupgtk4TreeGetVisiblePosition(ih, node);
  if (pos < 0)
  {
    /* Node is hidden, expand parents to make it visible */
    IupGtk4TreeNode *p = node->parent;
    while (p)
    {
      int ppos = iupgtk4TreeGetVisiblePosition(ih, p);
      if (ppos >= 0)
      {
        GtkTreeListRow *row = gtk_tree_list_model_get_row(tree_model, ppos);
        if (row)
        {
          gtk_tree_list_row_set_expanded(row, TRUE);
          g_object_unref(row);
        }
      }
      p = p->parent;
    }
    pos = iupgtk4TreeGetVisiblePosition(ih, node);
  }

  if (pos >= 0)
  {
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
    gtk_selection_model_select_item(selection, pos, TRUE);
    gtk_list_view_scroll_to(GTK_LIST_VIEW(ih->handle), pos, GTK_LIST_SCROLL_FOCUS, NULL);
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
  }

  return 0;
}

static char* gtkTreeGetMarkedAttrib(Ihandle *ih, int id)
{
  IupGtk4TreeNode *node = iupgtk4TreeGetNodeFromId(ih, id);
  if (!node)
    return NULL;

  return iupStrReturnBoolean(node->selected);
}

static int gtkTreeSetMarkedAttrib(Ihandle *ih, int id, const char *value)
{
  IupGtk4TreeNode *node = iupgtk4TreeGetNodeFromId(ih, id);
  GtkSelectionModel *selection = GTK_SELECTION_MODEL(iupAttribGet(ih, "_IUPGTK4_SELECTION"));

  if (!node || !selection)
    return 0;

  int pos = iupgtk4TreeGetVisiblePosition(ih, node);
  gboolean select = iupStrBoolean(value);

  iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");

  node->selected = select;
  if (pos >= 0)
  {
    if (select)
      gtk_selection_model_select_item(selection, pos, FALSE);
    else
      gtk_selection_model_unselect_item(selection, pos);
  }

  iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);

  return 0;
}

static char* gtkTreeGetColorAttrib(Ihandle *ih, int id)
{
  IupGtk4TreeNode *node = iupgtk4TreeGetNodeFromId(ih, id);
  if (!node || !node->color)
    return NULL;

  return iupStrReturnStrf("%d %d %d",
                          (int)(node->color->red * 255),
                          (int)(node->color->green * 255),
                          (int)(node->color->blue * 255));
}

static int gtkTreeSetColorAttrib(Ihandle *ih, int id, const char *value)
{
  IupGtk4TreeNode *node = iupgtk4TreeGetNodeFromId(ih, id);
  GdkRGBA color;
  unsigned char r, g, b;

  if (!node)
    return 0;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupgtk4ColorSetRGB(&color, r, g, b);

  if (node->color)
    gdk_rgba_free(node->color);
  node->color = gdk_rgba_copy(&color);

  iupgtk4TreeNotifyNodeChanged(ih, node);

  return 0;
}

static char* gtkTreeGetTitleFontAttrib(Ihandle *ih, int id)
{
  IupGtk4TreeNode *node = iupgtk4TreeGetNodeFromId(ih, id);
  if (!node || !node->font)
    return NULL;

  return pango_font_description_to_string(node->font);
}

static int gtkTreeSetTitleFontAttrib(Ihandle *ih, int id, const char *value)
{
  IupGtk4TreeNode *node = iupgtk4TreeGetNodeFromId(ih, id);
  if (!node)
    return 0;

  if (node->font)
  {
    pango_font_description_free(node->font);
    node->font = NULL;
  }

  if (value)
  {
    PangoFontDescription *fontdesc = iupgtk4GetPangoFontDesc(value);
    if (fontdesc)
      node->font = pango_font_description_copy(fontdesc);
  }

  iupgtk4TreeNotifyNodeChanged(ih, node);

  return 0;
}

static char* gtkTreeGetToggleValueAttrib(Ihandle *ih, int id)
{
  IupGtk4TreeNode *node = iupgtk4TreeGetNodeFromId(ih, id);
  if (!node || !ih->data->show_toggle)
    return NULL;

  return iupStrReturnChecked(iupgtk4TreeToggleGetCheck(node, ih->data->show_toggle));
}

static int gtkTreeSetToggleValueAttrib(Ihandle *ih, int id, const char *value)
{
  IupGtk4TreeNode *node = iupgtk4TreeGetNodeFromId(ih, id);
  if (!node || !ih->data->show_toggle)
    return 0;

  if (ih->data->show_toggle == 2 && iupStrEqualNoCase(value, "NOTDEF"))
  {
    node->three_state = TRUE;
    node->check = FALSE;
  }
  else
  {
    node->three_state = FALSE;
    node->check = iupStrEqualNoCase(value, "ON");
  }

  iupgtk4TreeNotifyNodeChanged(ih, node);

  return 0;
}

static char* gtkTreeGetToggleVisibleAttrib(Ihandle *ih, int id)
{
  IupGtk4TreeNode *node = iupgtk4TreeGetNodeFromId(ih, id);
  if (!node || !ih->data->show_toggle)
    return NULL;

  return iupStrReturnBoolean(node->toggle_visible);
}

static int gtkTreeSetToggleVisibleAttrib(Ihandle *ih, int id, const char *value)
{
  IupGtk4TreeNode *node = iupgtk4TreeGetNodeFromId(ih, id);
  if (!node || !ih->data->show_toggle)
    return 0;

  node->toggle_visible = iupStrBoolean(value);
  iupgtk4TreeNotifyNodeChanged(ih, node);

  return 0;
}

static int gtkTreeSetImageAttrib(Ihandle *ih, int id, const char *value)
{
  IupGtk4TreeNode *node = iupgtk4TreeGetNodeFromId(ih, id);
  if (!node)
    return 0;

  GdkTexture *texture = iupImageGetImage(value, ih, 0, NULL);

  if (texture)
  {
    g_set_object(&node->image, texture);
    node->has_image = TRUE;
  }
  else
  {
    if (node->kind == ITREE_BRANCH && ih->data->def_image_collapsed)
      g_set_object(&node->image, ih->data->def_image_collapsed);
    else if (ih->data->def_image_leaf)
      g_set_object(&node->image, ih->data->def_image_leaf);
    node->has_image = FALSE;
  }

  iupgtk4TreeNotifyNodeChanged(ih, node);

  return 0;
}

static int gtkTreeSetImageExpandedAttrib(Ihandle *ih, int id, const char *value)
{
  IupGtk4TreeNode *node = iupgtk4TreeGetNodeFromId(ih, id);
  if (!node || node->kind != ITREE_BRANCH)
    return 0;

  GdkTexture *texture = iupImageGetImage(value, ih, 0, NULL);

  if (texture)
  {
    g_set_object(&node->image_expanded, texture);
    node->has_image_expanded = TRUE;
  }
  else
  {
    if (ih->data->def_image_expanded)
      g_set_object(&node->image_expanded, ih->data->def_image_expanded);
    node->has_image_expanded = FALSE;
  }

  iupgtk4TreeNotifyNodeChanged(ih, node);

  return 0;
}

static int gtkTreeSetImageLeafAttrib(Ihandle *ih, const char *value)
{
  ih->data->def_image_leaf = iupImageGetImage(value, ih, 0, NULL);

  /* Update all leaf nodes */
  GListStore *root_store = (GListStore*)iupAttribGet(ih, "_IUPGTK4_ROOT_STORE");
  if (root_store)
  {
    /* Would need to iterate all nodes recursively and update those without custom images */
    /* For now, changes will be reflected on next rebind */
  }

  return 1;
}

static int gtkTreeSetImageBranchCollapsedAttrib(Ihandle *ih, const char *value)
{
  ih->data->def_image_collapsed = iupImageGetImage(value, ih, 0, NULL);
  return 1;
}

static int gtkTreeSetImageBranchExpandedAttrib(Ihandle *ih, const char *value)
{
  ih->data->def_image_expanded = iupImageGetImage(value, ih, 0, NULL);
  return 1;
}

static int gtkTreeSetBgColorAttrib(Ihandle *ih, const char *value)
{
  unsigned char r, g, b;

  GtkScrolledWindow *scrolled_window = (GtkScrolledWindow*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (scrolled_window && iupStrBoolean(IupGetGlobal("SB_BGCOLOR")))
  {
    char *parent_value = iupBaseNativeParentGetBgColor(ih);

    if (iupStrToRGB(parent_value, &r, &g, &b))
    {
      GtkWidget *sb;

      iupgtk4SetBgColor((GtkWidget*)scrolled_window, r, g, b);

      sb = gtk_scrolled_window_get_hscrollbar(scrolled_window);
      if (sb) iupgtk4SetBgColor(sb, r, g, b);

      sb = gtk_scrolled_window_get_vscrollbar(scrolled_window);
      if (sb) iupgtk4SetBgColor(sb, r, g, b);
    }
  }

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupdrvBaseSetBgColorAttrib(ih, value);

  return 1;
}

static int gtkTreeSetFgColorAttrib(Ihandle *ih, const char *value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupgtk4SetFgColor(ih->handle, r, g, b);

  return 1;
}

static int gtkTreeSetShowRenameAttrib(Ihandle *ih, const char *value)
{
  ih->data->show_rename = iupStrBoolean(value) ? 1 : 0;
  return 0;
}

static char* gtkTreeGetChildCountAttrib(Ihandle *ih, int id)
{
  IupGtk4TreeNode *node = iupgtk4TreeGetNodeFromId(ih, id);
  if (!node)
    return NULL;

  return iupStrReturnInt(iup_gtk4_tree_node_get_child_count(node));
}

static char* gtkTreeGetRootCountAttrib(Ihandle *ih)
{
  GListStore *root_store = (GListStore*)iupAttribGet(ih, "_IUPGTK4_ROOT_STORE");
  if (!root_store)
    return "0";

  return iupStrReturnInt(g_list_model_get_n_items(G_LIST_MODEL(root_store)));
}

static char* gtkTreeGetNextAttrib(Ihandle *ih, int id)
{
  IupGtk4TreeNode *node = iupgtk4TreeGetNodeFromId(ih, id);
  if (!node)
    return NULL;

  GListStore *parent_store = iupgtk4TreeGetParentStore(ih, node);
  if (!parent_store)
    return NULL;

  int pos = iupgtk4TreeFindPositionInParent(node);
  if (pos < 0 || pos >= (int)g_list_model_get_n_items(G_LIST_MODEL(parent_store)) - 1)
    return NULL;

  IupGtk4TreeNode *next = g_list_model_get_item(G_LIST_MODEL(parent_store), pos + 1);
  if (!next)
    return NULL;

  int next_id = iupgtk4TreeFindNodeId(ih, next);
  g_object_unref(next);

  return iupStrReturnInt(next_id);
}

static char* gtkTreeGetPreviousAttrib(Ihandle *ih, int id)
{
  IupGtk4TreeNode *node = iupgtk4TreeGetNodeFromId(ih, id);
  if (!node)
    return NULL;

  GListStore *parent_store = iupgtk4TreeGetParentStore(ih, node);
  if (!parent_store)
    return NULL;

  int pos = iupgtk4TreeFindPositionInParent(node);
  if (pos <= 0)
    return NULL;

  IupGtk4TreeNode *prev = g_list_model_get_item(G_LIST_MODEL(parent_store), pos - 1);
  if (!prev)
    return NULL;

  int prev_id = iupgtk4TreeFindNodeId(ih, prev);
  g_object_unref(prev);

  return iupStrReturnInt(prev_id);
}

/*****************************************************************************/
/* MARK OPERATIONS                                                           */
/*****************************************************************************/

static char* gtkTreeGetFirstAttrib(Ihandle *ih, int id)
{
  IupGtk4TreeNode *node = iupgtk4TreeGetNodeFromId(ih, id);
  if (!node)
    return NULL;

  GListStore *parent_store = iupgtk4TreeGetParentStore(ih, node);
  if (!parent_store || g_list_model_get_n_items(G_LIST_MODEL(parent_store)) == 0)
    return "0";

  IupGtk4TreeNode *first = g_list_model_get_item(G_LIST_MODEL(parent_store), 0);
  if (!first)
    return NULL;

  int first_id = iupgtk4TreeFindNodeId(ih, first);
  g_object_unref(first);

  return iupStrReturnInt(first_id);
}

static char* gtkTreeGetLastAttrib(Ihandle *ih, int id)
{
  IupGtk4TreeNode *node = iupgtk4TreeGetNodeFromId(ih, id);
  if (!node)
    return NULL;

  GListStore *parent_store = iupgtk4TreeGetParentStore(ih, node);
  if (!parent_store)
    return NULL;

  guint n = g_list_model_get_n_items(G_LIST_MODEL(parent_store));
  if (n == 0)
    return NULL;

  IupGtk4TreeNode *last = g_list_model_get_item(G_LIST_MODEL(parent_store), n - 1);
  if (!last)
    return NULL;

  int last_id = iupgtk4TreeFindNodeId(ih, last);
  g_object_unref(last);

  return iupStrReturnInt(last_id);
}

static int gtkTreeSetMarkStartAttrib(Ihandle *ih, const char *value)
{
  int id;
  if (!iupStrToInt(value, &id))
    return 0;

  IupGtk4TreeNode *node = iupgtk4TreeGetNodeFromId(ih, id);
  if (!node)
    return 0;

  iupAttribSet(ih, "_IUPTREE_MARKSTART_NODE", (char*)node);
  return 1;
}

static void iupgtk4TreeSelectAll(Ihandle *ih, gboolean select)
{
  int i;
  GtkSelectionModel *selection = GTK_SELECTION_MODEL(iupAttribGet(ih, "_IUPGTK4_SELECTION"));

  if (!selection)
    return;

  iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");

  for (i = 0; i < ih->data->node_count; i++)
  {
    IupGtk4TreeNode *node = iupgtk4TreeGetNodeFromId(ih, i);
    if (node)
      node->selected = select;
  }

  if (select)
    gtk_selection_model_select_all(selection);
  else
    gtk_selection_model_unselect_all(selection);

  iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
}

static void iupgtk4TreeSelectRange(Ihandle *ih, int id1, int id2)
{
  int i;
  GtkSelectionModel *selection = GTK_SELECTION_MODEL(iupAttribGet(ih, "_IUPGTK4_SELECTION"));

  if (!selection)
    return;

  if (id1 > id2)
  {
    int tmp = id1;
    id1 = id2;
    id2 = tmp;
  }

  iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");

  for (i = id1; i <= id2; i++)
  {
    IupGtk4TreeNode *node = iupgtk4TreeGetNodeFromId(ih, i);
    if (node)
    {
      node->selected = TRUE;
      int pos = iupgtk4TreeGetVisiblePosition(ih, node);
      if (pos >= 0)
        gtk_selection_model_select_item(selection, pos, FALSE);
    }
  }

  iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
}

static int gtkTreeSetMarkAttrib(Ihandle *ih, const char *value)
{
  GtkSelectionModel *selection = GTK_SELECTION_MODEL(iupAttribGet(ih, "_IUPGTK4_SELECTION"));

  if (ih->data->mark_mode == ITREE_MARK_SINGLE || !selection)
    return 0;

  if (iupStrEqualNoCase(value, "BLOCK"))
  {
    IupGtk4TreeNode *mark_start = (IupGtk4TreeNode*)iupAttribGet(ih, "_IUPTREE_MARKSTART_NODE");
    int focus_id = IupGetInt(ih, "VALUE");

    if (mark_start && focus_id >= 0)
    {
      int start_id = iupgtk4TreeFindNodeId(ih, mark_start);
      iupgtk4TreeSelectRange(ih, start_id, focus_id);
    }
  }
  else if (iupStrEqualNoCase(value, "CLEARALL"))
    iupgtk4TreeSelectAll(ih, FALSE);
  else if (iupStrEqualNoCase(value, "MARKALL"))
    iupgtk4TreeSelectAll(ih, TRUE);
  else if (iupStrEqualNoCase(value, "INVERTALL"))
  {
    int i;
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");

    for (i = 0; i < ih->data->node_count; i++)
    {
      IupGtk4TreeNode *node = iupgtk4TreeGetNodeFromId(ih, i);
      if (node)
      {
        node->selected = !node->selected;
        int pos = iupgtk4TreeGetVisiblePosition(ih, node);
        if (pos >= 0)
        {
          if (node->selected)
            gtk_selection_model_select_item(selection, pos, FALSE);
          else
            gtk_selection_model_unselect_item(selection, pos);
        }
      }
    }

    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
  }
  else if (iupStrEqualPartial(value, "INVERT"))
  {
    int id;
    if (iupStrToInt(&value[strlen("INVERT")], &id))
    {
      IupGtk4TreeNode *node = iupgtk4TreeGetNodeFromId(ih, id);
      if (node)
      {
        iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
        node->selected = !node->selected;
        int pos = iupgtk4TreeGetVisiblePosition(ih, node);
        if (pos >= 0)
        {
          if (node->selected)
            gtk_selection_model_select_item(selection, pos, FALSE);
          else
            gtk_selection_model_unselect_item(selection, pos);
        }
        iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
      }
    }
  }
  else
  {
    char str1[50], str2[50];
    if (iupStrToStrStr(value, str1, str2, '-') == 2)
    {
      int id1, id2;
      if (iupStrToInt(str1, &id1) && iupStrToInt(str2, &id2))
        iupgtk4TreeSelectRange(ih, id1, id2);
    }
  }

  return 1;
}

static char* gtkTreeGetMarkedNodesAttrib(Ihandle *ih)
{
  char *str = iupStrGetMemory(ih->data->node_count + 1);
  int i;

  for (i = 0; i < ih->data->node_count; i++)
  {
    IupGtk4TreeNode *node = iupgtk4TreeGetNodeFromId(ih, i);
    str[i] = (node && node->selected) ? '+' : '-';
  }

  str[ih->data->node_count] = 0;
  return str;
}

static int gtkTreeSetMarkedNodesAttrib(Ihandle *ih, const char *value)
{
  int count, i;
  GtkSelectionModel *selection = GTK_SELECTION_MODEL(iupAttribGet(ih, "_IUPGTK4_SELECTION"));

  if (ih->data->mark_mode == ITREE_MARK_SINGLE || !value || !selection)
    return 0;

  count = (int)strlen(value);
  if (count > ih->data->node_count)
    count = ih->data->node_count;

  iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");

  for (i = 0; i < count; i++)
  {
    IupGtk4TreeNode *node = iupgtk4TreeGetNodeFromId(ih, i);
    if (node)
    {
      gboolean select = (value[i] == '+');
      node->selected = select;
      int pos = iupgtk4TreeGetVisiblePosition(ih, node);
      if (pos >= 0)
      {
        if (select)
          gtk_selection_model_select_item(selection, pos, FALSE);
        else
          gtk_selection_model_unselect_item(selection, pos);
      }
    }
  }

  iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);

  return 0;
}

static int gtkTreeSetRenameAttrib(Ihandle *ih, const char *value)
{
  if (ih->data->show_rename)
  {
    int id = IupGetInt(ih, "VALUE");
    if (id >= 0)
    {
      IupGtk4TreeNode *node = iupgtk4TreeGetNodeFromId(ih, id);
      if (node)
        iupgtk4TreeStartRenameNode(ih, node);
    }
  }
  (void)value;
  return 0;
}

/*****************************************************************************/
/* DELETE NODES                                                              */
/*****************************************************************************/

static void iupgtk4TreeCallNodeRemovedRec(Ihandle *ih, IupGtk4TreeNode *node, IFns cb, int *id)
{
  int i, n;
  int node_id = *id;

  /* Process children first (depth-first) */
  if (node->children)
  {
    n = g_list_model_get_n_items(G_LIST_MODEL(node->children));
    for (i = 0; i < n; i++)
    {
      IupGtk4TreeNode *child = g_list_model_get_item(G_LIST_MODEL(node->children), i);
      (*id)++;
      iupgtk4TreeCallNodeRemovedRec(ih, child, cb, id);
      g_object_unref(child);
    }
  }

  /* Call callback for this node */
  cb(ih, (char*)ih->data->node_cache[node_id].userdata);

  ih->data->node_count--;
}

static void iupgtk4TreeCallNodeRemoved(Ihandle *ih, IupGtk4TreeNode *node)
{
  int old_count = ih->data->node_count;
  int id = iupgtk4TreeFindNodeId(ih, node);
  int start_id = id;

  IFns cb = (IFns)IupGetCallback(ih, "NODEREMOVED_CB");
  if (cb)
    iupgtk4TreeCallNodeRemovedRec(ih, node, cb, &id);
  else
  {
    int removed_count = iupgtk4TreeTotalChildCountRec(node) + 1;
    ih->data->node_count -= removed_count;
  }

  iupTreeDelFromCache(ih, start_id, old_count - ih->data->node_count);
}

static int gtkTreeSetDelNodeAttrib(Ihandle *ih, int id, const char *value)
{
  GListStore *root_store = (GListStore*)iupAttribGet(ih, "_IUPGTK4_ROOT_STORE");

  if (!ih->handle || !root_store)
    return 0;

  if (iupStrEqualNoCase(value, "ALL"))
  {
    /* Call callbacks for all nodes */
    IFns cb = (IFns)IupGetCallback(ih, "NODEREMOVED_CB");
    if (cb)
    {
      int i;
      for (i = 0; i < ih->data->node_count; i++)
        cb(ih, (char*)ih->data->node_cache[i].userdata);
    }

    int old_count = ih->data->node_count;
    ih->data->node_count = 0;
    iupTreeDelFromCache(ih, 0, old_count);

    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
    g_list_store_remove_all(root_store);
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);

    return 0;
  }

  if (iupStrEqualNoCase(value, "SELECTED"))
  {
    IupGtk4TreeNode *node = iupgtk4TreeGetNodeFromId(ih, id);
    if (!node)
      return 0;

    GListStore *parent_store = iupgtk4TreeGetParentStore(ih, node);
    int pos = iupgtk4TreeFindPositionInParent(node);

    if (parent_store && pos >= 0)
    {
      iupgtk4TreeCallNodeRemoved(ih, node);

      iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
      g_list_store_remove(parent_store, pos);
      iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
    }

    return 0;
  }

  if (iupStrEqualNoCase(value, "CHILDREN"))
  {
    IupGtk4TreeNode *node = iupgtk4TreeGetNodeFromId(ih, id);
    if (!node || !node->children)
      return 0;

    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");

    /* Remove children in reverse order */
    int n = g_list_model_get_n_items(G_LIST_MODEL(node->children));
    while (n > 0)
    {
      IupGtk4TreeNode *child = g_list_model_get_item(G_LIST_MODEL(node->children), n - 1);
      iupgtk4TreeCallNodeRemoved(ih, child);
      g_object_unref(child);
      g_list_store_remove(node->children, n - 1);
      n--;
    }

    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);

    return 0;
  }

  if (iupStrEqualNoCase(value, "MARKED"))
  {
    /* Delete all marked nodes */
    int i;
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");

    for (i = ih->data->node_count - 1; i >= 0; i--)
    {
      IupGtk4TreeNode *node = iupgtk4TreeGetNodeFromId(ih, i);
      if (node && node->selected)
      {
        GListStore *parent_store = iupgtk4TreeGetParentStore(ih, node);
        int pos = iupgtk4TreeFindPositionInParent(node);

        if (parent_store && pos >= 0)
        {
          iupgtk4TreeCallNodeRemoved(ih, node);
          g_list_store_remove(parent_store, pos);
        }
      }
    }

    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);

    return 0;
  }

  return 0;
}

/*****************************************************************************/
/* COPY/MOVE NODES                                                           */
/*****************************************************************************/

static void iupgtk4TreeCopyNodeRec(Ihandle *ih, IupGtk4TreeNode *src, IupGtk4TreeNode *dst_parent, int position)
{
  /* Create copy of source node */
  IupGtk4TreeNode *new_node = iup_gtk4_tree_node_new(ih, src->kind, src->title);

  /* Copy properties */
  if (src->image)
    new_node->image = g_object_ref(src->image);
  if (src->image_expanded)
    new_node->image_expanded = g_object_ref(src->image_expanded);
  new_node->has_image = src->has_image;
  new_node->has_image_expanded = src->has_image_expanded;
  if (src->color)
    new_node->color = gdk_rgba_copy(src->color);
  if (src->font)
    new_node->font = pango_font_description_copy(src->font);
  new_node->toggle_visible = src->toggle_visible;

  /* Add to parent */
  new_node->parent = dst_parent;
  if (dst_parent)
  {
    if (!dst_parent->children)
      dst_parent->children = g_list_store_new(IUP_GTK4_TYPE_TREE_NODE);
    g_list_store_insert(dst_parent->children, position, new_node);
  }
  else
  {
    GListStore *root_store = (GListStore*)iupAttribGet(ih, "_IUPGTK4_ROOT_STORE");
    g_list_store_insert(root_store, position, new_node);
  }

  ih->data->node_count++;

  /* Copy children recursively */
  if (src->children)
  {
    int i, n = g_list_model_get_n_items(G_LIST_MODEL(src->children));
    for (i = 0; i < n; i++)
    {
      IupGtk4TreeNode *child = g_list_model_get_item(G_LIST_MODEL(src->children), i);
      iupgtk4TreeCopyNodeRec(ih, child, new_node, i);
      g_object_unref(child);
    }
  }

  g_object_unref(new_node);
}

static int gtkTreeSetMoveNodeAttrib(Ihandle *ih, int id, const char *value)
{
  IupGtk4TreeNode *src_node, *dst_node;
  int dst_id;

  if (!ih->handle)
    return 0;

  src_node = iupgtk4TreeGetNodeFromId(ih, id);
  if (!src_node)
    return 0;

  if (!iupStrToInt(value, &dst_id))
    return 0;

  dst_node = iupgtk4TreeGetNodeFromId(ih, dst_id);
  if (!dst_node)
    return 0;

  /* Check if src is ancestor of dst */
  IupGtk4TreeNode *p = dst_node->parent;
  while (p)
  {
    if (p == src_node)
      return 0;  /* Can't move to descendant */
    p = p->parent;
  }

  /* Determine destination position */
  GListStore *src_store = iupgtk4TreeGetParentStore(ih, src_node);
  int src_pos = iupgtk4TreeFindPositionInParent(src_node);

  GListStore *dst_store;
  int dst_pos;

  if (dst_node->kind == ITREE_BRANCH)
  {
    /* Insert as first child of branch */
    if (!dst_node->children)
      dst_node->children = g_list_store_new(IUP_GTK4_TYPE_TREE_NODE);
    dst_store = dst_node->children;
    dst_pos = 0;
    src_node->parent = dst_node;
  }
  else
  {
    /* Insert after dst_node */
    dst_store = iupgtk4TreeGetParentStore(ih, dst_node);
    dst_pos = iupgtk4TreeFindPositionInParent(dst_node) + 1;
    src_node->parent = dst_node->parent;
  }

  /* Remove from old position */
  g_object_ref(src_node);
  g_list_store_remove(src_store, src_pos);

  /* Insert at new position */
  g_list_store_insert(dst_store, dst_pos, src_node);
  g_object_unref(src_node);

  return 0;
}

static int gtkTreeSetCopyNodeAttrib(Ihandle *ih, int id, const char *value)
{
  IupGtk4TreeNode *src_node, *dst_node;
  int dst_id;

  if (!ih->handle)
    return 0;

  src_node = iupgtk4TreeGetNodeFromId(ih, id);
  if (!src_node)
    return 0;

  if (!iupStrToInt(value, &dst_id))
    return 0;

  dst_node = iupgtk4TreeGetNodeFromId(ih, dst_id);
  if (!dst_node)
    return 0;

  /* Check if src is ancestor of dst */
  IupGtk4TreeNode *p = dst_node->parent;
  while (p)
  {
    if (p == src_node)
      return 0;
    p = p->parent;
  }

  /* Determine destination */
  int dst_pos;
  IupGtk4TreeNode *dst_parent;

  if (dst_node->kind == ITREE_BRANCH)
  {
    dst_parent = dst_node;
    dst_pos = 0;
  }
  else
  {
    dst_parent = dst_node->parent;
    dst_pos = iupgtk4TreeFindPositionInParent(dst_node) + 1;
  }

  /* Copy recursively */
  iupgtk4TreeCopyNodeRec(ih, src_node, dst_parent, dst_pos);

  return 0;
}

/* Helper for cross-tree copy - copies node from src tree to dst tree */
static void iupgtk4TreeCrossTreeCopyNodeRec(Ihandle *dst, IupGtk4TreeNode *src_node, IupGtk4TreeNode *dst_parent, int position)
{
  /* Create copy of source node in destination tree */
  IupGtk4TreeNode *new_node = iup_gtk4_tree_node_new(dst, src_node->kind, src_node->title);

  /* Copy properties */
  if (src_node->image)
    new_node->image = g_object_ref(src_node->image);
  if (src_node->image_expanded)
    new_node->image_expanded = g_object_ref(src_node->image_expanded);
  new_node->has_image = src_node->has_image;
  new_node->has_image_expanded = src_node->has_image_expanded;
  if (src_node->color)
    new_node->color = gdk_rgba_copy(src_node->color);
  if (src_node->font)
    new_node->font = pango_font_description_copy(src_node->font);
  new_node->toggle_visible = src_node->toggle_visible;

  /* Add to parent in destination tree */
  new_node->parent = dst_parent;
  if (dst_parent)
  {
    if (!dst_parent->children)
      dst_parent->children = g_list_store_new(IUP_GTK4_TYPE_TREE_NODE);
    g_list_store_insert(dst_parent->children, position, new_node);
  }
  else
  {
    GListStore *root_store = (GListStore*)iupAttribGet(dst, "_IUPGTK4_ROOT_STORE");
    g_list_store_insert(root_store, position, new_node);
  }

  dst->data->node_count++;

  /* Copy children recursively */
  if (src_node->children)
  {
    int i, n = g_list_model_get_n_items(G_LIST_MODEL(src_node->children));
    for (i = 0; i < n; i++)
    {
      IupGtk4TreeNode *child = g_list_model_get_item(G_LIST_MODEL(src_node->children), i);
      iupgtk4TreeCrossTreeCopyNodeRec(dst, child, new_node, i);
      g_object_unref(child);
    }
  }

  g_object_unref(new_node);
}

void iupdrvTreeDragDropCopyNode(Ihandle *src, Ihandle *dst, InodeHandle *itemSrc, InodeHandle *itemDst)
{
  IupGtk4TreeNode *src_node = (IupGtk4TreeNode*)itemSrc;
  IupGtk4TreeNode *dst_node = (IupGtk4TreeNode*)itemDst;
  int position = 0;
  IupGtk4TreeNode *dst_parent;
  int old_count = dst->data->node_count;
  int id_dst, id_new, count;

  id_dst = iupgtk4TreeFindNodeId(dst, dst_node);
  id_new = id_dst + 1;

  /* Determine where to insert based on destination node type */
  if (dst_node->kind == ITREE_BRANCH)
  {
    /* Check if branch is expanded */
    int pos = iupgtk4TreeGetVisiblePosition(dst, dst_node);
    if (pos >= 0)
    {
      GtkTreeListModel *tree_model = GTK_TREE_LIST_MODEL(iupAttribGet(dst, "_IUPGTK4_TREE_MODEL"));
      GtkTreeListRow *row = gtk_tree_list_model_get_row(tree_model, pos);
      if (row)
      {
        gboolean expanded = gtk_tree_list_row_get_expanded(row);
        g_object_unref(row);
        if (expanded)
        {
          /* Insert as first child of expanded branch */
          dst_parent = dst_node;
          position = 0;
        }
        else
        {
          /* Insert after collapsed branch (and all its children) */
          int child_count = iupgtk4TreeTotalChildCountRec(dst_node);
          id_new += child_count;
          dst_parent = dst_node->parent;
          position = iupgtk4TreeFindPositionInParent(dst_node) + 1;
        }
      }
      else
      {
        dst_parent = dst_node->parent;
        position = iupgtk4TreeFindPositionInParent(dst_node) + 1;
      }
    }
    else
    {
      dst_parent = dst_node->parent;
      position = iupgtk4TreeFindPositionInParent(dst_node) + 1;
    }
  }
  else
  {
    /* Leaf - insert after it as sibling */
    dst_parent = dst_node->parent;
    position = iupgtk4TreeFindPositionInParent(dst_node) + 1;
  }

  /* Copy the node and its children */
  iupgtk4TreeCrossTreeCopyNodeRec(dst, src_node, dst_parent, position);

  /* Update cache */
  count = dst->data->node_count - old_count;
  iupTreeCopyMoveCache(dst, id_dst, id_new, count, 1);

  {
    GListStore *store;
    IupGtk4TreeNode *new_node;

    if (dst_parent)
      store = dst_parent->children;
    else
      store = (GListStore*)iupAttribGet(dst, "_IUPGTK4_ROOT_STORE");

    new_node = g_list_model_get_item(G_LIST_MODEL(store), position);
    iupgtk4TreeRebuildNodeCache(dst, id_new, new_node);
    g_object_unref(new_node);
  }
}

/*****************************************************************************/
/* Class initialization                                                      */
/*****************************************************************************/

void iupdrvTreeInitClass(Iclass *ic)
{
  /* Driver Dependent Class functions */
  ic->Map = gtkTreeMapMethod;
  ic->UnMap = gtkTreeUnMapMethod;

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, gtkTreeSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, gtkTreeSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);

  /* IupTree Attributes - GENERAL */
  iupClassRegisterAttribute(ic, "EXPANDALL", NULL, gtkTreeSetExpandAllAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INDENTATION", gtkTreeGetIndentationAttrib, gtkTreeSetIndentationAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "SPACING", iupTreeGetSpacingAttrib, gtkTreeSetSpacingAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "TOPITEM", NULL, gtkTreeSetTopItemAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  /* IupTree Attributes - IMAGES */
  iupClassRegisterAttributeId(ic, "IMAGE", NULL, gtkTreeSetImageAttrib, IUPAF_IHANDLENAME | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IMAGEEXPANDED", NULL, gtkTreeSetImageExpandedAttrib, IUPAF_IHANDLENAME | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "IMAGELEAF", NULL, gtkTreeSetImageLeafAttrib, IUPAF_SAMEASSYSTEM, "IMGLEAF", IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEBRANCHCOLLAPSED", NULL, gtkTreeSetImageBranchCollapsedAttrib, IUPAF_SAMEASSYSTEM, "IMGCOLLAPSED", IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEBRANCHEXPANDED", NULL, gtkTreeSetImageBranchExpandedAttrib, IUPAF_SAMEASSYSTEM, "IMGEXPANDED", IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);

  /* IupTree Attributes - NODES */
  iupClassRegisterAttributeId(ic, "STATE", gtkTreeGetStateAttrib, gtkTreeSetStateAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "DEPTH", gtkTreeGetDepthAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "KIND", gtkTreeGetKindAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "PARENT", gtkTreeGetParentAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "NEXT", gtkTreeGetNextAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "PREVIOUS", gtkTreeGetPreviousAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "COLOR", gtkTreeGetColorAttrib, gtkTreeSetColorAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TITLE", gtkTreeGetTitleAttrib, gtkTreeSetTitleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TOGGLEVALUE", gtkTreeGetToggleValueAttrib, gtkTreeSetToggleValueAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TOGGLEVISIBLE", gtkTreeGetToggleVisibleAttrib, gtkTreeSetToggleVisibleAttrib, IUPAF_NO_INHERIT);

  iupClassRegisterReplaceAttribFunc(ic, "SHOWRENAME", NULL, gtkTreeSetShowRenameAttrib);

  iupClassRegisterAttributeId(ic, "CHILDCOUNT", gtkTreeGetChildCountAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TITLEFONT", gtkTreeGetTitleFontAttrib, gtkTreeSetTitleFontAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ROOTCOUNT", gtkTreeGetRootCountAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "FIRST", gtkTreeGetFirstAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "LAST", gtkTreeGetLastAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);

  /* IupTree Attributes - MARKS */
  iupClassRegisterAttributeId(ic, "MARKED", gtkTreeGetMarkedAttrib, gtkTreeSetMarkedAttrib, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARK", NULL, gtkTreeSetMarkAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STARTING", NULL, gtkTreeSetMarkStartAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKSTART", NULL, gtkTreeSetMarkStartAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKEDNODES", gtkTreeGetMarkedNodesAttrib, gtkTreeSetMarkedNodesAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKWHENTOGGLE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", gtkTreeGetValueAttrib, gtkTreeSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  /* IupTree Attributes - ACTION */
  iupClassRegisterAttribute(ic, "RENAME", NULL, gtkTreeSetRenameAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "DELNODE", NULL, gtkTreeSetDelNodeAttrib, IUPAF_NOT_MAPPED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "MOVENODE", NULL, gtkTreeSetMoveNodeAttrib, IUPAF_NOT_MAPPED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "COPYNODE", NULL, gtkTreeSetCopyNodeAttrib, IUPAF_NOT_MAPPED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  /* IupTree Attributes - GTK Only */
  iupClassRegisterAttribute(ic, "RUBBERBAND", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);

  /* Not Supported in new implementation */
  iupClassRegisterAttribute(ic, "SCROLLVISIBLE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
}
