/** \file
 * \brief IupTable control - GTK driver (GTK2/GTK3)
 *
 * See Copyright Notice in "iup.h"
 */

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#if GTK_CHECK_VERSION(3, 0, 0)
#include <gdk/gdkkeysyms-compat.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

#include "iupgtk_drv.h"
#include "iup_tablecontrol.h"


/* ========================================================================= */
/* GTK-specific data structure                                              */
/* ========================================================================= */

/* Custom virtual tree model for VIRTUALMODE */
typedef struct _IupGtkVirtualModel IupGtkVirtualModel;
typedef struct _IupGtkVirtualModelClass IupGtkVirtualModelClass;

struct _IupGtkVirtualModel
{
  GObject parent;
  Ihandle* ih;  /* Reference to IUP handle */
  gint stamp;   /* Random integer to check whether an iter belongs to our model */
};

struct _IupGtkVirtualModelClass
{
  GObjectClass parent_class;
};

GType iup_gtk_virtual_model_get_type(void);
#define IUP_TYPE_GTK_VIRTUAL_MODEL (iup_gtk_virtual_model_get_type())
#define IUP_GTK_VIRTUAL_MODEL(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), IUP_TYPE_GTK_VIRTUAL_MODEL, IupGtkVirtualModel))

typedef struct _IgtkTableData
{
  GtkWidget* scrolled_win;
  GtkWidget* tree_view;
  GtkListStore* store;  /* Used in normal mode */
  IupGtkVirtualModel* virtual_model;  /* Used in VIRTUALMODE */
  GType* column_types;  /* Array of column types */
  int is_virtual;  /* TRUE if using virtual model */

  /* Sorting state for virtual mode */
  int sort_column;      /* Currently sorted column (1-based, 0=none) */
  int sort_ascending;   /* Sort direction: 1=ascending, 0=descending */
} IgtkTableData;

#define IGTK_TABLE_DATA(ih) ((IgtkTableData*)(ih->data->native_data))

/* Forward declarations for model column mapping */
static int gtkTableModelColCount(Ihandle* ih);
static int gtkTableTextModelCol(Ihandle* ih, int iup_col);
static int gtkTableImageModelCol(int iup_col);

/* ========================================================================= */
/* Custom Virtual Tree Model Implementation                                  */
/* ========================================================================= */

static void iup_gtk_virtual_model_tree_model_init(GtkTreeModelIface *iface);

G_DEFINE_TYPE_WITH_CODE(IupGtkVirtualModel, iup_gtk_virtual_model, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE(GTK_TYPE_TREE_MODEL, iup_gtk_virtual_model_tree_model_init))

static void iup_gtk_virtual_model_init(IupGtkVirtualModel *model)
{
  model->stamp = g_random_int();  /* Random stamp to detect invalid iters */
}

static void iup_gtk_virtual_model_class_init(IupGtkVirtualModelClass *klass)
{
  /* Nothing special needed */
}

/* GtkTreeModel interface implementation */

static GtkTreeModelFlags iup_gtk_virtual_model_get_flags(GtkTreeModel *tree_model)
{
  return GTK_TREE_MODEL_LIST_ONLY;  /* Flat list, no hierarchy */
}

static gint iup_gtk_virtual_model_get_n_columns(GtkTreeModel *tree_model)
{
  IupGtkVirtualModel *model = IUP_GTK_VIRTUAL_MODEL(tree_model);
  if (!model->ih) return 0;
  return gtkTableModelColCount(model->ih);
}

static GType iup_gtk_virtual_model_get_column_type(GtkTreeModel *tree_model, gint index)
{
  IupGtkVirtualModel *model = IUP_GTK_VIRTUAL_MODEL(tree_model);
  if (model->ih && model->ih->data->show_image && (index % 2 == 0))
    return GDK_TYPE_PIXBUF;
  return G_TYPE_STRING;
}

static gboolean iup_gtk_virtual_model_get_iter(GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreePath *path)
{
  IupGtkVirtualModel *model = IUP_GTK_VIRTUAL_MODEL(tree_model);
  gint *indices = gtk_tree_path_get_indices(path);
  gint depth = gtk_tree_path_get_depth(path);

  if (depth != 1)  /* We only have one level */
    return FALSE;

  gint row = indices[0];
  if (row < 0 || row >= model->ih->data->num_lin)
    return FALSE;

  /* Store row index in iter */
  iter->stamp = model->stamp;
  iter->user_data = GINT_TO_POINTER(row);
  iter->user_data2 = NULL;
  iter->user_data3 = NULL;

  return TRUE;
}

static GtkTreePath *iup_gtk_virtual_model_get_path(GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  IupGtkVirtualModel *model = IUP_GTK_VIRTUAL_MODEL(tree_model);

  if (iter->stamp != model->stamp)
    return NULL;

  gint row = GPOINTER_TO_INT(iter->user_data);
  return gtk_tree_path_new_from_indices(row, -1);
}

static void iup_gtk_virtual_model_get_value(GtkTreeModel *tree_model, GtkTreeIter *iter, gint column, GValue *value)
{
  IupGtkVirtualModel *model = IUP_GTK_VIRTUAL_MODEL(tree_model);

  if (iter->stamp != model->stamp)
    return;

  gint row = GPOINTER_TO_INT(iter->user_data);

  if (model->ih->data->show_image && (column % 2 == 0))
  {
    g_value_init(value, GDK_TYPE_PIXBUF);

    gint iup_col = column / 2;
    char* image_name = iupTableGetCellImageCb(model->ih, row + 1, iup_col + 1);
    if (image_name)
    {
      GdkPixbuf* pixbuf = (GdkPixbuf*)iupImageGetImage(image_name, model->ih, 0, NULL);
      if (pixbuf && model->ih->data->fit_image)
      {
        int charheight;
        iupdrvFontGetCharSize(model->ih, NULL, &charheight);
        int available_height = charheight + 4;
        int img_height = gdk_pixbuf_get_height(pixbuf);
        if (img_height > available_height)
        {
          int img_width = gdk_pixbuf_get_width(pixbuf);
          int scaled_width = (img_width * available_height) / img_height;
          GdkPixbuf* scaled = gdk_pixbuf_scale_simple(pixbuf, scaled_width, available_height, GDK_INTERP_BILINEAR);
          g_value_take_object(value, scaled);
          return;
        }
      }
      g_value_set_object(value, pixbuf);
    }
    return;
  }

  g_value_init(value, G_TYPE_STRING);

  gint iup_col = model->ih->data->show_image ? (column / 2) : column;

  /* Query data via VALUE_CB (row and column are 1-based for IUP) */
  sIFnii value_cb = (sIFnii)IupGetCallback(model->ih, "VALUE_CB");
  if (value_cb)
  {
    char *cell_value = value_cb(model->ih, row + 1, iup_col + 1);
    if (cell_value)
    {
      g_value_set_string(value, cell_value);
      return;
    }
  }

  g_value_set_string(value, "");
}

static gboolean iup_gtk_virtual_model_iter_next(GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  IupGtkVirtualModel *model = IUP_GTK_VIRTUAL_MODEL(tree_model);

  if (iter->stamp != model->stamp)
    return FALSE;

  gint row = GPOINTER_TO_INT(iter->user_data);
  row++;

  if (row >= model->ih->data->num_lin)
    return FALSE;

  iter->user_data = GINT_TO_POINTER(row);
  return TRUE;
}

static gboolean iup_gtk_virtual_model_iter_children(GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *parent)
{
  IupGtkVirtualModel *model = IUP_GTK_VIRTUAL_MODEL(tree_model);

  /* Only root has children in a flat list */
  if (parent != NULL)
    return FALSE;

  if (model->ih->data->num_lin == 0)
    return FALSE;

  iter->stamp = model->stamp;
  iter->user_data = GINT_TO_POINTER(0);  /* First row */
  return TRUE;
}

static gboolean iup_gtk_virtual_model_iter_has_child(GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  return FALSE;  /* Flat list has no children */
}

static gint iup_gtk_virtual_model_iter_n_children(GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  IupGtkVirtualModel *model = IUP_GTK_VIRTUAL_MODEL(tree_model);

  /* If iter is NULL, return root level child count (total rows) */
  if (iter == NULL)
    return model->ih->data->num_lin;

  /* Rows have no children */
  return 0;
}

static gboolean iup_gtk_virtual_model_iter_nth_child(GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *parent, gint n)
{
  IupGtkVirtualModel *model = IUP_GTK_VIRTUAL_MODEL(tree_model);

  /* Only root can have children */
  if (parent != NULL)
    return FALSE;

  if (n < 0 || n >= model->ih->data->num_lin)
    return FALSE;

  iter->stamp = model->stamp;
  iter->user_data = GINT_TO_POINTER(n);
  return TRUE;
}

static gboolean iup_gtk_virtual_model_iter_parent(GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *child)
{
  return FALSE;  /* Flat list has no parents */
}

static void iup_gtk_virtual_model_tree_model_init(GtkTreeModelIface *iface)
{
  iface->get_flags = iup_gtk_virtual_model_get_flags;
  iface->get_n_columns = iup_gtk_virtual_model_get_n_columns;
  iface->get_column_type = iup_gtk_virtual_model_get_column_type;
  iface->get_iter = iup_gtk_virtual_model_get_iter;
  iface->get_path = iup_gtk_virtual_model_get_path;
  iface->get_value = iup_gtk_virtual_model_get_value;
  iface->iter_next = iup_gtk_virtual_model_iter_next;
  iface->iter_children = iup_gtk_virtual_model_iter_children;
  iface->iter_has_child = iup_gtk_virtual_model_iter_has_child;
  iface->iter_n_children = iup_gtk_virtual_model_iter_n_children;
  iface->iter_nth_child = iup_gtk_virtual_model_iter_nth_child;
  iface->iter_parent = iup_gtk_virtual_model_iter_parent;
}

static IupGtkVirtualModel *iup_gtk_virtual_model_new(Ihandle *ih)
{
  IupGtkVirtualModel *model = g_object_new(IUP_TYPE_GTK_VIRTUAL_MODEL, NULL);
  model->ih = ih;
  return model;
}

/* ========================================================================= */
/* Utility Functions                                                         */
/* ========================================================================= */

/* Forward declarations */
static void gtkTableColumnClicked(GtkTreeViewColumn* column, Ihandle* ih);

static gint gtkTableCompareFn(GtkTreeModel* model, GtkTreeIter* a, GtkTreeIter* b, gpointer user_data)
{
  gint sort_column = GPOINTER_TO_INT(user_data);
  gchar* val1 = NULL;
  gchar* val2 = NULL;

  gtk_tree_model_get(model, a, sort_column, &val1, -1);
  gtk_tree_model_get(model, b, sort_column, &val2, -1);

  gint result = g_strcmp0(val1, val2);

  g_free(val1);
  g_free(val2);

  return result;
}

static int gtkTableModelColCount(Ihandle* ih)
{
  if (ih->data->show_image)
    return ih->data->num_col * 2;
  return ih->data->num_col;
}

static int gtkTableTextModelCol(Ihandle* ih, int iup_col)
{
  if (ih->data->show_image)
    return iup_col * 2 + 1;
  return iup_col;
}

static int gtkTableImageModelCol(int iup_col)
{
  return iup_col * 2;
}

static void gtkTableEnsureStore(Ihandle* ih)
{
  IgtkTableData* gtk_data = IGTK_TABLE_DATA(ih);
  int model_cols = gtkTableModelColCount(ih);

  if (gtk_data->store && gtk_tree_model_get_n_columns(GTK_TREE_MODEL(gtk_data->store)) == model_cols)
    return;

  if (gtk_data->column_types)
    free(gtk_data->column_types);

  gtk_data->column_types = (GType*)malloc(sizeof(GType) * model_cols);

  int i;
  if (ih->data->show_image)
  {
    for (i = 0; i < ih->data->num_col; i++)
    {
      gtk_data->column_types[i * 2] = GDK_TYPE_PIXBUF;
      gtk_data->column_types[i * 2 + 1] = G_TYPE_STRING;
    }
  }
  else
  {
    for (i = 0; i < ih->data->num_col; i++)
      gtk_data->column_types[i] = G_TYPE_STRING;
  }

  if (gtk_data->store)
    g_object_unref(gtk_data->store);

  gtk_data->store = gtk_list_store_newv(model_cols, gtk_data->column_types);
  gtk_tree_view_set_model(GTK_TREE_VIEW(gtk_data->tree_view), GTK_TREE_MODEL(gtk_data->store));

  for (i = 0; i < ih->data->num_col; i++)
  {
    int sort_col = gtkTableTextModelCol(ih, i);
    gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(gtk_data->store), sort_col, gtkTableCompareFn, GINT_TO_POINTER(sort_col), NULL);
  }
}

static void gtkTableUpdateColumns(Ihandle* ih)
{
  IgtkTableData* gtk_data = IGTK_TABLE_DATA(ih);

  /* Remove all existing columns */
  GList* columns = gtk_tree_view_get_columns(GTK_TREE_VIEW(gtk_data->tree_view));
  GList* iter;
  for (iter = columns; iter != NULL; iter = g_list_next(iter))
  {
    gtk_tree_view_remove_column(GTK_TREE_VIEW(gtk_data->tree_view), GTK_TREE_VIEW_COLUMN(iter->data));
  }
  g_list_free(columns);

  /* Add new columns */
  int col;
  for (col = 0; col < ih->data->num_col; col++)
  {
    int text_model_col = gtkTableTextModelCol(ih, col);
    GtkTreeViewColumn* column = gtk_tree_view_column_new();

    if (ih->data->show_image)
    {
      GtkCellRenderer* pix_renderer = gtk_cell_renderer_pixbuf_new();
      gtk_tree_view_column_pack_start(column, pix_renderer, FALSE);
      gtk_tree_view_column_add_attribute(column, pix_renderer, "pixbuf", gtkTableImageModelCol(col));
    }

    GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, TRUE);
    gtk_tree_view_column_add_attribute(column, renderer, "text", text_model_col);

    /* Check if sorting is allowed */
    if (ih->data->sortable)
    {
      if (gtk_data->is_virtual)
      {
        /* Virtual mode - disable GTK automatic sorting, use manual handling */
        gtk_tree_view_column_set_sort_column_id(column, -1);
        gtk_tree_view_column_set_clickable(column, TRUE);
        g_signal_connect(G_OBJECT(column), "clicked", G_CALLBACK(gtkTableColumnClicked), ih);
      }
      else
      {
        /* Normal mode - enable GTK's automatic sorting */
        gtk_tree_view_column_set_sort_column_id(column, text_model_col);
        gtk_tree_view_column_set_clickable(column, TRUE);
        g_signal_connect(G_OBJECT(column), "clicked", G_CALLBACK(gtkTableColumnClicked), ih);
      }
    }
    else
    {
      gtk_tree_view_column_set_sort_column_id(column, -1);
      gtk_tree_view_column_set_clickable(column, FALSE);
    }

    /* Check if column reordering is allowed */
    gtk_tree_view_column_set_reorderable(column, ih->data->allow_reorder);

    /* Handle column expansion and sizing */
    if (col == ih->data->num_col - 1)
    {
      /* Last column, expand to fill ONLY if STRETCHLAST=YES */
      if (ih->data->stretch_last)
      {
        gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
        gtk_tree_view_column_set_expand(column, TRUE);
        gtk_tree_view_column_set_resizable(column, FALSE);
      }
      else
      {
        /* Fit to content, don't stretch */
        gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
        gtk_tree_view_column_set_expand(column, FALSE);
        gtk_tree_view_column_set_resizable(column, FALSE);
      }
    }
    else
    {
      /* Non-last columns never expand */
      gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
      gtk_tree_view_column_set_expand(column, FALSE);
      gtk_tree_view_column_set_resizable(column, FALSE);
    }

    gtk_tree_view_append_column(GTK_TREE_VIEW(gtk_data->tree_view), column);
  }
}

/* ========================================================================= */
/* GTK Signal Callbacks                                                      */
/* ========================================================================= */

static void gtkTableCellEditingStarted(GtkCellRenderer* renderer, GtkCellEditable* editable, gchar* path_string, gpointer user_data)
{
  Ihandle* ih = (Ihandle*)user_data;
  IgtkTableData* gtk_data = IGTK_TABLE_DATA(ih);

  GtkTreePath* path = gtk_tree_path_new_from_string(path_string);
  int* indices = gtk_tree_path_get_indices(path);
  int lin = indices[0] + 1;  /* 1-based */
  gtk_tree_path_free(path);

  /* Find which column this renderer belongs to */
  GList* columns = gtk_tree_view_get_columns(GTK_TREE_VIEW(gtk_data->tree_view));
  int col = 0;
  GList* iter;
  for (iter = columns; iter != NULL; iter = g_list_next(iter))
  {
    GtkTreeViewColumn* column = GTK_TREE_VIEW_COLUMN(iter->data);
    GList* renderers = gtk_cell_layout_get_cells(GTK_CELL_LAYOUT(column));
    if (renderers && renderers->data == renderer)
    {
      col = g_list_index(columns, column) + 1;  /* 1-based */
      g_list_free(renderers);
      break;
    }
    if (renderers)
      g_list_free(renderers);
  }
  g_list_free(columns);

  if (col < 1)
    return;

  /* Call EDITBEGIN_CB - allow application to block editing */
  IFnii editbegin_cb = (IFnii)IupGetCallback(ih, "EDITBEGIN_CB");
  if (editbegin_cb)
  {
    int ret = editbegin_cb(ih, lin, col);
    if (ret == IUP_IGNORE)
    {
      /* Block editing by making the cell non-editable temporarily */
      gtk_cell_renderer_stop_editing(renderer, TRUE);
    }
  }

  (void)editable;
}

static void gtkTableCellEditingCanceled(GtkCellRenderer* renderer, gpointer user_data)
{
  Ihandle* ih = (Ihandle*)user_data;
  IgtkTableData* gtk_data = IGTK_TABLE_DATA(ih);

  /* Get the currently focused cell */
  GtkTreePath* path;
  GtkTreeViewColumn* column;
  gtk_tree_view_get_cursor(GTK_TREE_VIEW(gtk_data->tree_view), &path, &column);

  if (!path || !column)
    return;

  int* indices = gtk_tree_path_get_indices(path);
  int lin = indices[0] + 1;  /* 1-based */

  /* Find column index */
  GList* columns = gtk_tree_view_get_columns(GTK_TREE_VIEW(gtk_data->tree_view));
  int col = g_list_index(columns, column) + 1;  /* 1-based */
  g_list_free(columns);

  /* Get current cell value */
  char* value = iupdrvTableGetCellValue(ih, lin, col);

  /* Call EDITEND_CB with apply=0 (cancelled) */
  IFniisi editend_cb = (IFniisi)IupGetCallback(ih, "EDITEND_CB");
  if (editend_cb)
  {
    editend_cb(ih, lin, col, value ? value : "", 0);  /* 0 = cancelled */
  }

  gtk_tree_path_free(path);
}

static void gtkTableCellEdited(GtkCellRendererText* renderer, gchar* path_string, gchar* new_text, gpointer user_data)
{
  Ihandle* ih = (Ihandle*)user_data;
  IgtkTableData* gtk_data = IGTK_TABLE_DATA(ih);

  GtkTreePath* path = gtk_tree_path_new_from_string(path_string);
  int* indices = gtk_tree_path_get_indices(path);
  int lin = indices[0] + 1;  /* 1-based */
  gtk_tree_path_free(path);

  /* Find which column this renderer belongs to */
  GList* columns = gtk_tree_view_get_columns(GTK_TREE_VIEW(gtk_data->tree_view));
  int col = 0;
  GList* iter;
  for (iter = columns; iter != NULL; iter = g_list_next(iter))
  {
    GtkTreeViewColumn* column = GTK_TREE_VIEW_COLUMN(iter->data);
    GList* renderers = gtk_cell_layout_get_cells(GTK_CELL_LAYOUT(column));
    if (renderers && renderers->data == renderer)
    {
      col = g_list_index(columns, column) + 1;  /* 1-based */
      g_list_free(renderers);
      break;
    }
    if (renderers)
      g_list_free(renderers);
  }
  g_list_free(columns);

  if (col < 1)
    return;

  /* Get old value for comparison - must copy before it gets modified */
  char* old_text_ptr = iupdrvTableGetCellValue(ih, lin, col);
  char* old_text = old_text_ptr ? iupStrDup(old_text_ptr) : NULL;

  /* Call EDITEND_CB - allow application to validate/reject edit */
  IFniisi editend_cb = (IFniisi)IupGetCallback(ih, "EDITEND_CB");
  if (editend_cb)
  {
    int ret = editend_cb(ih, lin, col, new_text, 1);  /* 1 = accepted */
    if (ret == IUP_IGNORE)
      return;  /* Reject edit */
  }

  /* Call edition callback */
  IFniis cb = (IFniis)IupGetCallback(ih, "EDITION_CB");
  if (cb)
  {
    int ret = cb(ih, lin, col, new_text);
    if (ret == IUP_IGNORE)
      return;  /* Don't update cell */
  }

  /* Update cell value */
  iupdrvTableSetCellValue(ih, lin, col, new_text);

  /* Call value changed callback only if text actually changed */
  int text_changed = 0;
  if (!old_text && new_text && *new_text)
    text_changed = 1;  /* NULL -> non-empty */
  else if (old_text && !new_text)
    text_changed = 1;  /* non-empty -> NULL */
  else if (old_text && new_text && strcmp(old_text, new_text) != 0)
    text_changed = 1;  /* different text */

  if (text_changed)
  {
    IFnii vcb = (IFnii)IupGetCallback(ih, "VALUECHANGED_CB");
    if (vcb)
      vcb(ih, lin, col);
  }

  if (old_text)
    free(old_text);
}

static gboolean gtkTableButtonEvent(GtkWidget* widget, GdkEventButton* evt, Ihandle* ih)
{
  if (evt->type == GDK_BUTTON_PRESS && evt->button == 1)
  {
    /* Single click */
    GtkTreePath* path;
    GtkTreeViewColumn* column;

    if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget), (gint)evt->x, (gint)evt->y, &path, &column, NULL, NULL))
    {
      /* Check if this is the dummy column, ignore clicks on it */
      GtkTreeViewColumn* dummy_column = (GtkTreeViewColumn*)iupAttribGet(ih, "_IUPGTK_DUMMY_COLUMN");
      if (dummy_column && column == dummy_column)
      {
        gtk_tree_path_free(path);
        return TRUE;  /* Block event */
      }

      IFniis cb = (IFniis)IupGetCallback(ih, "CLICK_CB");
      if (cb)
      {
        GList* columns = gtk_tree_view_get_columns(GTK_TREE_VIEW(widget));
        int col_index = g_list_index(columns, column) + 1;  /* 1-based */
        int* indices = gtk_tree_path_get_indices(path);
        int lin = indices[0] + 1;  /* 1-based */
        g_list_free(columns);

        char status[IUPKEY_STATUS_SIZE] = "";
        iupgtkButtonKeySetStatus(evt->state, evt->button, status, 0);

        cb(ih, lin, col_index, status);
      }

      gtk_tree_path_free(path);
    }
  }

  return FALSE;  /* Allow default processing */
}

static void gtkTableColumnClicked(GtkTreeViewColumn* column, Ihandle* ih)
{
  /* Called when user clicks a sortable column header */
  if (!ih->data->sortable)
    return;

  IgtkTableData* gtk_data = IGTK_TABLE_DATA(ih);
  if (!gtk_data)
    return;

  /* Get column index */
  GtkTreeView* tree_view = GTK_TREE_VIEW(gtk_tree_view_column_get_tree_view(column));
  GList* columns = gtk_tree_view_get_columns(tree_view);
  int col_index = g_list_index(columns, column) + 1;  /* 1-based */

  if (col_index < 1 || col_index > ih->data->num_col)
  {
    g_list_free(columns);
    return;
  }

  if (gtk_data->is_virtual)
  {
    /* Virtual mode - manually track sort state and call SORT_CB */
    /* Toggle sort direction for same column, ascending for new column */
    if (gtk_data->sort_column == col_index)
    {
      /* Same column, toggle direction */
      gtk_data->sort_ascending = !gtk_data->sort_ascending;
    }
    else
    {
      /* New column, start with ascending */
      gtk_data->sort_column = col_index;
      gtk_data->sort_ascending = 1;
    }

    /* Update sort indicators for all columns */
    GList* l;
    int i = 1;
    for (l = columns; l != NULL; l = l->next, i++)
    {
      GtkTreeViewColumn* col = GTK_TREE_VIEW_COLUMN(l->data);

      if (i == col_index)
      {
        /* This is the sorted column, show indicator */
        gtk_tree_view_column_set_sort_indicator(col, TRUE);
        gtk_tree_view_column_set_sort_order(col,
          gtk_data->sort_ascending ? GTK_SORT_ASCENDING : GTK_SORT_DESCENDING);
      }
      else
      {
        /* Other columns, hide indicator */
        gtk_tree_view_column_set_sort_indicator(col, FALSE);
      }
    }
  }

  g_list_free(columns);

  /* Call SORT_CB if it exists */
  IFni sort_cb = (IFni)IupGetCallback(ih, "SORT_CB");
  if (sort_cb)
  {
    sort_cb(ih, col_index);
  }
}

static void gtkTableCursorChanged(GtkTreeView* tree_view, Ihandle* ih)
{
  /* Called when cursor position changes (row or column) */
  GtkTreePath* path = NULL;
  GtkTreeViewColumn* column = NULL;
  gtk_tree_view_get_cursor(tree_view, &path, &column);

  /* If cursor moved to dummy column, move it back to last real column */
  GtkTreeViewColumn* dummy_column = (GtkTreeViewColumn*)iupAttribGet(ih, "_IUPGTK_DUMMY_COLUMN");
  if (dummy_column && column == dummy_column && path)
  {
    /* Get the last real column (one before dummy) */
    GtkTreeViewColumn* last_real_col = gtk_tree_view_get_column(tree_view, ih->data->num_col - 1);
    if (last_real_col)
    {
      gtk_tree_view_set_cursor(tree_view, path, last_real_col, FALSE);
    }
    gtk_tree_path_free(path);
    return;
  }

  IFnii cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
  if (!cb)
  {
    if (path)
      gtk_tree_path_free(path);
    return;
  }

  if (path && column)
  {
    gint* indices = gtk_tree_path_get_indices(path);
    int lin = indices[0] + 1;  /* Convert to 1-based */

    /* Find column index */
    int col = 1;
    GList* columns = gtk_tree_view_get_columns(tree_view);
    for (GList* l = columns; l != NULL; l = l->next, col++)
    {
      if ((GtkTreeViewColumn*)l->data == column)
        break;
    }
    g_list_free(columns);

    cb(ih, lin, col);
  }

  if (path)
    gtk_tree_path_free(path);
}

static void gtkTableSelectionChanged(GtkTreeSelection* selection, Ihandle* ih)
{
  IFnii cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
  if (!cb)
    return;

  GtkSelectionMode mode = gtk_tree_selection_get_mode(selection);

  if (mode == GTK_SELECTION_MULTIPLE)
  {
    /* For multiple selection, get list of selected rows */
    GList* rows = gtk_tree_selection_get_selected_rows(selection, NULL);
    if (rows)
    {
      /* Call callback for the first selected row (most recently selected) */
      GtkTreePath* path = (GtkTreePath*)rows->data;
      int* indices = gtk_tree_path_get_indices(path);
      int lin = indices[0] + 1;  /* 1-based */

      cb(ih, lin, 1);  /* Column is always 1 for row selection */

      g_list_free_full(rows, (GDestroyNotify)gtk_tree_path_free);
    }
    else
    {
      cb(ih, 0, 0);  /* Selection cleared */
    }
  }
  else
  {
    /* For single selection, use get_selected */
    GtkTreeModel* model;
    GtkTreeIter iter;

    if (gtk_tree_selection_get_selected(selection, &model, &iter))
    {
      GtkTreePath* path = gtk_tree_model_get_path(model, &iter);
      int* indices = gtk_tree_path_get_indices(path);
      int lin = indices[0] + 1;  /* 1-based */
      gtk_tree_path_free(path);

      cb(ih, lin, 1);  /* Column is always 1 for row selection */
    }
    else
    {
      cb(ih, 0, 0);  /* Selection cleared */
    }
  }
}

/* ========================================================================= */
/* Cell Data Functions                                                      */
/* ========================================================================= */

typedef struct {
  Ihandle* ih;
  int col;  /* 1-based column index */
} IgtkCellDataInfo;

static void gtkTableCellDataFunc(GtkTreeViewColumn* column, GtkCellRenderer* renderer,
                                   GtkTreeModel* model, GtkTreeIter* iter, gpointer user_data)
{
  IgtkCellDataInfo* info = (IgtkCellDataInfo*)user_data;
  Ihandle* ih = info->ih;
  int col = info->col;

  /* Get row number (1-based) */
  GtkTreePath* path = gtk_tree_model_get_path(model, iter);
  int* indices = gtk_tree_path_get_indices(path);
  int lin = indices[0] + 1;  /* 1-based */
  gtk_tree_path_free(path);

  /* In virtual mode, text values come from the virtual model's get_value() method.
   * This cell data function only needs to handle colors.
   * In normal mode, the text is already in the GtkListStore. */

  /* Hierarchy: per-cell (L:C) > per-column (:C) > per-row (L:*) > alternating > default */

  /* Background color */
  char* bgcolor = iupAttribGetId2(ih, "BGCOLOR", lin, col);  /* Try L:C */
  if (!bgcolor)
    bgcolor = iupAttribGetId2(ih, "BGCOLOR", 0, col);  /* Try :C (per-column) */
  if (!bgcolor)
    bgcolor = iupAttribGetId2(ih, "BGCOLOR", lin, 0);  /* Try L:* (per-row) */

  /* Check for alternating row colors if no explicit color is set */
  if (!bgcolor)
  {
    char* alternate_color = iupAttribGet(ih, "ALTERNATECOLOR");
    if (iupStrBoolean(alternate_color))
    {
      /* Use EVENROWCOLOR or ODDROWCOLOR based on row number */
      if (lin % 2 == 0)
        bgcolor = iupAttribGet(ih, "EVENROWCOLOR");
      else
        bgcolor = iupAttribGet(ih, "ODDROWCOLOR");
    }
  }

#if GTK_CHECK_VERSION(3, 0, 0)
  if (bgcolor && *bgcolor)
  {
    GdkRGBA color;
    if (gdk_rgba_parse(&color, bgcolor))
    {
      g_object_set(renderer, "background-rgba", &color, "background-set", TRUE, NULL);
    }
  }
  else
  {
    g_object_set(renderer, "background-set", FALSE, NULL);
  }
#else
  if (bgcolor && *bgcolor)
  {
    GdkColor color;
    if (gdk_color_parse(bgcolor, &color))
    {
      g_object_set(renderer, "background-gdk", &color, "background-set", TRUE, NULL);
    }
  }
  else
  {
    g_object_set(renderer, "background-set", FALSE, NULL);
  }
#endif

  /* Foreground color */
  char* fgcolor = iupAttribGetId2(ih, "FGCOLOR", lin, col);  /* Try L:C */
  if (!fgcolor)
    fgcolor = iupAttribGetId2(ih, "FGCOLOR", 0, col);  /* Try :C (per-column) */
  if (!fgcolor)
    fgcolor = iupAttribGetId2(ih, "FGCOLOR", lin, 0);  /* Try L:* (per-row) */

#if GTK_CHECK_VERSION(3, 0, 0)
  if (fgcolor && *fgcolor)
  {
    GdkRGBA color;
    if (gdk_rgba_parse(&color, fgcolor))
    {
      g_object_set(renderer, "foreground-rgba", &color, "foreground-set", TRUE, NULL);
    }
  }
  else
  {
    g_object_set(renderer, "foreground-set", FALSE, NULL);
  }
#else
  if (fgcolor && *fgcolor)
  {
    GdkColor color;
    if (gdk_color_parse(fgcolor, &color))
    {
      g_object_set(renderer, "foreground-gdk", &color, "foreground-set", TRUE, NULL);
    }
  }
  else
  {
    g_object_set(renderer, "foreground-set", FALSE, NULL);
  }
#endif

  /* Font */
  char* font = iupAttribGetId2(ih, "FONT", lin, col);  /* Try L:C */
  if (!font)
    font = iupAttribGetId2(ih, "FONT", 0, col);  /* Try :C (per-column) */
  if (!font)
    font = iupAttribGetId2(ih, "FONT", lin, 0);  /* Try L:* (per-row) */

  if (font && *font)
  {
    /* Get cached Pango font description */
    PangoFontDescription* fontdesc = iupgtkGetPangoFontDesc(font);
    if (fontdesc)
    {
      /* Set font, g_object_set will ref-count internally */
      g_object_set(renderer, "font-desc", fontdesc, NULL);
    }
  }
  else
  {
    /* Reset to default font by passing NULL */
    g_object_set(renderer, "font-desc", NULL, NULL);
  }
}

/* ========================================================================= */
/* Driver Functions - Keyboard Navigation                                   */
/* ========================================================================= */

static gboolean gtkTableKeyPressEvent(GtkWidget* widget, GdkEventKey* event, Ihandle* ih)
{
  IgtkTableData* gtk_data = IGTK_TABLE_DATA(ih);
  GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gtk_data->tree_view));
  GtkTreePath* path = NULL;
  GtkTreeViewColumn* column = NULL;

  /* Get current cursor position */
  gtk_tree_view_get_cursor(GTK_TREE_VIEW(gtk_data->tree_view), &path, &column);

  if (!path)
    return FALSE;  /* No cell selected, let GTK handle it */

  gint* indices = gtk_tree_path_get_indices(path);
  int lin = indices[0] + 1;  /* Convert 0-based to 1-based */

  /* Find current column index */
  int col = 1;  /* Default to first column */
  GList* columns = gtk_tree_view_get_columns(GTK_TREE_VIEW(gtk_data->tree_view));
  for (GList* l = columns; l != NULL; l = l->next, col++)
  {
    if ((GtkTreeViewColumn*)l->data == column)
      break;
  }
  g_list_free(columns);

  gboolean handled = FALSE;

  /* Handle navigation keys */
  switch (event->keyval)
  {
    case GDK_Left:
      /* Move to previous column */
      if (col > 1)
      {
        GtkTreeViewColumn* prev_col = gtk_tree_view_get_column(GTK_TREE_VIEW(gtk_data->tree_view), col - 2);
        if (prev_col)
        {
          gtk_tree_view_set_cursor(GTK_TREE_VIEW(gtk_data->tree_view), path, prev_col, FALSE);
          handled = TRUE;
        }
      }
      break;

    case GDK_Right:
      /* Move to next column */
      if (col < ih->data->num_col)
      {
        GtkTreeViewColumn* next_col = gtk_tree_view_get_column(GTK_TREE_VIEW(gtk_data->tree_view), col);
        if (next_col)
        {
          gtk_tree_view_set_cursor(GTK_TREE_VIEW(gtk_data->tree_view), path, next_col, FALSE);
          handled = TRUE;
        }
      }
      break;

    case GDK_Tab:
      /* Move to next cell (right, or next row if at end) */
      if (col < ih->data->num_col)
      {
        /* Move right */
        GtkTreeViewColumn* next_col = gtk_tree_view_get_column(GTK_TREE_VIEW(gtk_data->tree_view), col);
        if (next_col)
        {
          gtk_tree_view_set_cursor(GTK_TREE_VIEW(gtk_data->tree_view), path, next_col, FALSE);
          handled = TRUE;
        }
      }
      else if (lin < ih->data->num_lin)
      {
        /* At end of row, move to first column of next row */
        gtk_tree_path_next(path);
        GtkTreeViewColumn* first_col = gtk_tree_view_get_column(GTK_TREE_VIEW(gtk_data->tree_view), 0);
        if (first_col)
        {
          gtk_tree_view_set_cursor(GTK_TREE_VIEW(gtk_data->tree_view), path, first_col, FALSE);
          handled = TRUE;
        }
      }
      break;

    case GDK_ISO_Left_Tab:  /* Shift+Tab */
      /* Move to previous cell (left, or previous row if at start) */
      if (col > 1)
      {
        /* Move left */
        GtkTreeViewColumn* prev_col = gtk_tree_view_get_column(GTK_TREE_VIEW(gtk_data->tree_view), col - 2);
        if (prev_col)
        {
          gtk_tree_view_set_cursor(GTK_TREE_VIEW(gtk_data->tree_view), path, prev_col, FALSE);
          handled = TRUE;
        }
      }
      else if (lin > 1)
      {
        /* At start of row, move to last column of previous row */
        gtk_tree_path_prev(path);
        GtkTreeViewColumn* last_col = gtk_tree_view_get_column(GTK_TREE_VIEW(gtk_data->tree_view), ih->data->num_col - 1);
        if (last_col)
        {
          gtk_tree_view_set_cursor(GTK_TREE_VIEW(gtk_data->tree_view), path, last_col, FALSE);
          handled = TRUE;
        }
      }
      break;

    case GDK_Escape:
      /* Just let it propagate */
      break;

    case GDK_c:
    case GDK_C:
      /* Copy current cell to clipboard */
      if (event->state & GDK_CONTROL_MASK)
      {
        /* Get value from GTK model */
        IgtkTableData* gtk_data = IGTK_TABLE_DATA(ih);
        GtkTreeIter iter;
        gchar* value = NULL;

        if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(gtk_data->store), &iter, NULL, lin - 1))
        {
          gtk_tree_model_get(GTK_TREE_MODEL(gtk_data->store), &iter, gtkTableTextModelCol(ih, col - 1), &value, -1);
        }

        if (value && *value)
        {
          IupStoreGlobal("CLIPBOARD", value);
          g_free(value);
          handled = TRUE;
        }
        else if (value)
        {
          g_free(value);
        }
      }
      break;

    case GDK_v:
    case GDK_V:
      /* Paste from clipboard to current cell */
      if (event->state & GDK_CONTROL_MASK)
      {
        /* Check if cell is editable */
        char name[50];
        sprintf(name, "EDITABLE%d", col);
        char* editable = iupAttribGet(ih, name);
        if (!editable)
          editable = iupAttribGet(ih, "EDITABLE");

        if (iupStrBoolean(editable))
        {
          const char* text = IupGetGlobal("CLIPBOARD");
          if (text && *text)
          {
            /* Set the cell value */
            iupAttribSetId2(ih, "", lin, col, text);

            /* Update the GTK model */
            IgtkTableData* gtk_data = IGTK_TABLE_DATA(ih);
            GtkTreeIter iter;
            if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(gtk_data->store), &iter, NULL, lin - 1))
            {
              gtk_list_store_set(gtk_data->store, &iter, gtkTableTextModelCol(ih, col - 1), text, -1);
            }

            /* Call VALUECHANGED_CB if defined */
            IFnii value_cb = (IFnii)IupGetCallback(ih, "VALUECHANGED_CB");
            if (value_cb)
              value_cb(ih, lin, col);

            handled = TRUE;
          }
        }
      }
      break;
  }

  gtk_tree_path_free(path);

  /* Call standard IUP key press event for K_ANY callback */
  if (!handled)
    return iupgtkKeyPressEvent(widget, event, ih);

  return handled;  /* TRUE = stop event propagation, FALSE = continue */
}

/* Draw callback to overlay dashed border around focused cell */
#if GTK_CHECK_VERSION(3, 0, 0)
static gboolean gtkTableDrawFocusRect(GtkWidget* widget, cairo_t* cr, Ihandle* ih)
#else
static gboolean gtkTableDrawFocusRect(GtkWidget* widget, GdkEventExpose* event, Ihandle* ih)
#endif
{
  IgtkTableData* gtk_data = IGTK_TABLE_DATA(ih);

  if (!gtk_data)
    return FALSE;

  /* Check if FOCUSRECT is enabled */
  if (!iupAttribGetBoolean(ih, "FOCUSRECT"))
    return FALSE;

  GtkTreePath* path = NULL;
  GtkTreeViewColumn* column = NULL;
  gtk_tree_view_get_cursor(GTK_TREE_VIEW(gtk_data->tree_view), &path, &column);

  if (!path || !column)
    return FALSE;

  /* Don't draw focus rect on dummy column */
  GtkTreeViewColumn* dummy_column = (GtkTreeViewColumn*)iupAttribGet(ih, "_IUPGTK_DUMMY_COLUMN");
  if (dummy_column && column == dummy_column)
  {
    gtk_tree_path_free(path);
    return FALSE;
  }

  /* Get cell area */
  GdkRectangle cell_area;
  gtk_tree_view_get_cell_area(GTK_TREE_VIEW(gtk_data->tree_view), path, column, &cell_area);
  gtk_tree_path_free(path);

  /* Convert to widget coordinates */
  gtk_tree_view_convert_bin_window_to_widget_coords(GTK_TREE_VIEW(gtk_data->tree_view), cell_area.x, cell_area.y, &cell_area.x, &cell_area.y);

#if GTK_CHECK_VERSION(3, 0, 0)
  /* Get theme border color for subtle focus indicator */
  GtkStyleContext* context = gtk_widget_get_style_context(widget);
  GdkRGBA color;
#if GTK_CHECK_VERSION(3, 16, 0)
  if (!gtk_style_context_lookup_color(context, "borders", &color) &&
      !gtk_style_context_lookup_color(context, "theme_unfocused_border_color", &color))
  {
    color.red = 0.5; color.green = 0.5; color.blue = 0.5; color.alpha = 1.0;
  }
#else
  gtk_style_context_get_border_color(context, gtk_widget_get_state_flags(widget), &color);
#endif

  /* Draw dashed border */
  cairo_set_source_rgba(cr, color.red, color.green, color.blue, 0.5);  /* More subtle with lower opacity */
  cairo_set_line_width(cr, 1.0);

  /* Set dash pattern */
  double dashes[] = {2.0, 2.0};
  cairo_set_dash(cr, dashes, 2, 0);

  /* Draw rectangle with minimal inset, closer to cell edge */
  cairo_rectangle(cr, cell_area.x + 0.5, cell_area.y + 0.5, cell_area.width - 1, cell_area.height - 1);
  cairo_stroke(cr);
#else
  /* GTK2 */
  GtkStyle* style = gtk_widget_get_style(widget);
  GdkWindow* window = gtk_tree_view_get_bin_window(GTK_TREE_VIEW(gtk_data->tree_view));

  if (!window)
    return FALSE;

  /* Convert widget coords back to bin window coords for GTK2 */
  int bin_x, bin_y;
  gtk_tree_view_convert_widget_to_bin_window_coords(GTK_TREE_VIEW(gtk_data->tree_view), cell_area.x, cell_area.y, &bin_x, &bin_y);

#ifndef GDK_DISABLE_DEPRECATED
  {
    GdkGC* gc = gdk_gc_new(window);

    /* Set dashed line style */
    gint8 dashes[2] = {2, 2};
    gdk_gc_set_dashes(gc, 0, dashes, 2);

    GdkGCValues gcval;
    gcval.line_style = GDK_LINE_ON_OFF_DASH;
    gcval.line_width = 1;
    gdk_gc_set_values(gc, &gcval, GDK_GC_LINE_STYLE | GDK_GC_LINE_WIDTH);

    /* Use subtle dark color */
    GdkColor* border_color = &style->dark[GTK_STATE_NORMAL];
    gdk_gc_set_rgb_fg_color(gc, border_color);

    gdk_draw_rectangle(window, gc, FALSE, bin_x, bin_y, cell_area.width - 1, cell_area.height - 1);

    g_object_unref(gc);
  }
#else
  {
    cairo_t* cr = gdk_cairo_create(window);
    if (!cr)
      return FALSE;

    /* Use subtle dark color */
    GdkColor* border_color = &style->dark[GTK_STATE_NORMAL];
    gdk_cairo_set_source_color(cr, border_color);
    cairo_set_line_width(cr, 1.0);

    /* Set dash pattern */
    double dashes[] = {2.0, 2.0};
    cairo_set_dash(cr, dashes, 2, 0);

    /* Draw rectangle */
    cairo_rectangle(cr, bin_x + 0.5, bin_y + 0.5, cell_area.width - 1, cell_area.height - 1);
    cairo_stroke(cr);

    cairo_destroy(cr);
  }
#endif
#endif

  return FALSE;  /* Allow other handlers to run */
}

/* ========================================================================= */
/* Driver Functions - Widget Lifecycle                                      */
/* ========================================================================= */

static int gtk3TableCalculateColumnWidth(Ihandle* ih, int col_index)
{
  IgtkTableData* gtk_data = IGTK_TABLE_DATA(ih);
  int max_width = 0;
  int max_rows_to_check = (ih->data->num_lin > 100) ? 100 : ih->data->num_lin;
  int iup_col = col_index + 1;
  int charheight = 0;
  int image_extra = 0;

  if (ih->data->show_image)
  {
    iupdrvFontGetCharSize(ih, NULL, &charheight);
    image_extra = charheight + 4;
  }

  /* Measure column title */
  GtkTreeViewColumn* column = gtk_tree_view_get_column(GTK_TREE_VIEW(gtk_data->tree_view), col_index);
  if (column)
  {
    const char* title = gtk_tree_view_column_get_title(column);
    if (title && title[0])
    {
      int title_width = iupdrvFontGetStringWidth(ih, title) + 8;
      if (title_width > max_width)
        max_width = title_width;
    }
  }

  /* Measure cell content in first N rows */
  GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(gtk_data->tree_view));
  if (model)
  {
    GtkTreeIter iter;
    int row = 0;
    if (gtk_tree_model_get_iter_first(model, &iter))
    {
      do {
        int cell_width = 0;
        GValue value = G_VALUE_INIT;
        gtk_tree_model_get_value(model, &iter, gtkTableTextModelCol(ih, col_index), &value);
        if (G_VALUE_HOLDS_STRING(&value))
        {
          const char* text = g_value_get_string(&value);
          if (text && text[0])
            cell_width = iupdrvFontGetStringWidth(ih, text);
        }
        g_value_unset(&value);
        cell_width += 8;

        if (ih->data->show_image)
        {
          char* image_name = iupAttribGetId2(ih, "IMAGE", row + 1, iup_col);
          if (!image_name)
            image_name = iupTableGetCellImageCb(ih, row + 1, iup_col);
          if (image_name)
            cell_width += image_extra;
        }

        if (cell_width > max_width)
          max_width = cell_width;

        row++;
      } while (row < max_rows_to_check && gtk_tree_model_iter_next(model, &iter));
    }
  }

  if (max_width < 20)
    max_width = 20;

  return max_width;
}

static void gtkTableLayoutUpdateMethod(Ihandle* ih)
{
  IgtkTableData* gtk_data = IGTK_TABLE_DATA(ih);
  GtkWidget* widget = gtk_data->scrolled_win;
  int width = ih->currentwidth;
  int height = ih->currentheight;

  /* If VISIBLELINES is set, clamp height to target */
  int target_height = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "iup-table-target-height"));
  if (target_height > 0 && height > target_height)
    height = target_height;

  /* If VISIBLECOLUMNS is set, clamp width to show exactly N columns */
  int visible_columns = iupAttribGetInt(ih, "VISIBLECOLUMNS");
  if (visible_columns > 0 && gtk_data->tree_view)
  {
    int c, cols_width = 0;
    int num_cols = visible_columns;
    if (num_cols > ih->data->num_col)
      num_cols = ih->data->num_col;

    for (c = 0; c < num_cols; c++)
    {
      GtkTreeViewColumn* column = gtk_tree_view_get_column(GTK_TREE_VIEW(gtk_data->tree_view), c);
      if (column)
      {
        /* Try rendered width first (works after display) */
        int col_width = gtk_tree_view_column_get_width(column);
        if (col_width <= 0)
        {
          /* Calculate from content */
          col_width = gtk3TableCalculateColumnWidth(ih, c);
        }
        cols_width += col_width;
      }
    }

    int sb_size = iupdrvGetScrollbarSize();
    int border = 2;

    /* Only add vertical scrollbar width if it will actually be visible */
    int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
    int need_vert_sb = (visiblelines > 0 && ih->data->num_lin > visiblelines);
    int vert_sb_width = need_vert_sb ? sb_size : 0;

    int target_width = cols_width + vert_sb_width + border;
    if (width > target_width)
      width = target_width;
  }

  /* Get the GtkFixed parent container */
  GtkWidget* parent = gtk_widget_get_parent(widget);
  while (parent && !GTK_IS_FIXED(parent))
    parent = gtk_widget_get_parent(parent);

  if (parent)
  {
    iupgtkNativeContainerMove(parent, widget, ih->x, ih->y);
    gtk_widget_set_size_request(widget, width, height);
  }
}

static int gtkTableMapMethod(Ihandle* ih)
{
  GtkListStore* store;
  int i, col;

  /* Allocate GTK-specific data */
  IgtkTableData* gtk_data = (IgtkTableData*)malloc(sizeof(IgtkTableData));
  memset(gtk_data, 0, sizeof(IgtkTableData));
  ih->data->native_data = gtk_data;

  /* Check if using virtual mode */
  gtk_data->is_virtual = iupAttribGetBoolean(ih, "VIRTUALMODE");

  GtkTreeModel* model = NULL;

  if (gtk_data->is_virtual)
  {
    /* Create virtual model, no data storage, queries via VALUE_CB */
    gtk_data->virtual_model = iup_gtk_virtual_model_new(ih);
    model = GTK_TREE_MODEL(gtk_data->virtual_model);
  }
  else
  {
    /* Create standard GtkListStore */
    if (ih->data->num_col > 0)
    {
      int model_cols = gtkTableModelColCount(ih);
      gtk_data->column_types = (GType*)malloc(sizeof(GType) * model_cols);
      if (ih->data->show_image)
      {
        for (i = 0; i < ih->data->num_col; i++)
        {
          gtk_data->column_types[i * 2] = GDK_TYPE_PIXBUF;
          gtk_data->column_types[i * 2 + 1] = G_TYPE_STRING;
        }
      }
      else
      {
        for (i = 0; i < ih->data->num_col; i++)
          gtk_data->column_types[i] = G_TYPE_STRING;
      }

      gtk_data->store = gtk_list_store_newv(model_cols, gtk_data->column_types);
    }
    else
    {
      /* Create empty store with one column as placeholder */
      GType type = G_TYPE_STRING;
      gtk_data->column_types = (GType*)malloc(sizeof(GType));
      gtk_data->column_types[0] = G_TYPE_STRING;
      gtk_data->store = gtk_list_store_newv(1, gtk_data->column_types);
    }

    store = gtk_data->store;
    model = GTK_TREE_MODEL(store);
  }

  /* Create scrolled window */
  gtk_data->scrolled_win = gtk_scrolled_window_new(NULL, NULL);
  /* AUTOMATIC shows scrollbars only when needed */
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(gtk_data->scrolled_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  /* Create tree view with model */
  gtk_data->tree_view = gtk_tree_view_new_with_model(model);
  if (!gtk_data->is_virtual)
    g_object_unref(store);  /* Tree view holds reference */

  /* Set tree view properties */
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(gtk_data->tree_view), TRUE);
  gtk_tree_view_set_enable_search(GTK_TREE_VIEW(gtk_data->tree_view), FALSE);

  /* Enable fixed height mode for virtual mode, prevents GTK from iterating all rows */
  if (gtk_data->is_virtual)
    gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(gtk_data->tree_view), TRUE);

  /* Ensure tree view can receive focus and events */
  gtk_widget_set_can_focus(gtk_data->tree_view, TRUE);
#if GTK_CHECK_VERSION(3, 0, 0)
  gtk_widget_set_focus_on_click(gtk_data->tree_view, TRUE);
#endif

  /* Set selection mode */
  GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gtk_data->tree_view));
  char* selmode = iupAttribGetStr(ih, "SELECTIONMODE");
  if (!selmode)
    selmode = "SINGLE";  /* Default */

  if (iupStrEqualNoCase(selmode, "NONE"))
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_NONE);
  else if (iupStrEqualNoCase(selmode, "MULTIPLE"))
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
  else  /* SINGLE or anything else */
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

  /* Create columns if we have any */
  if (ih->data->num_col > 0)
  {
    for (col = 0; col < ih->data->num_col; col++)
    {
      GtkCellRenderer* renderer = gtk_cell_renderer_text_new();

      /* Store renderer pointer for later access */
      char renderer_name[50];
      sprintf(renderer_name, "_IUPGTK_RENDERER_%d", col);
      iupAttribSet(ih, renderer_name, (char*)renderer);

      /* Check if column should be editable */
      char name[50];
      sprintf(name, "EDITABLE%d", col + 1);
      char* editable_str = iupAttribGet(ih, name);
      if (!editable_str)
        editable_str = iupAttribGet(ih, "EDITABLE");  /* Global editable */

      if (iupStrBoolean(editable_str))
      {
        g_object_set(G_OBJECT(renderer), "editable", TRUE, NULL);

        /* Verify it was set */
        gboolean is_editable = FALSE;
        g_object_get(G_OBJECT(renderer), "editable", &is_editable, NULL);
      }

      /* Check if column has alignment set (applies to entire column: header + cells) */
      sprintf(name, "ALIGNMENT%d", col + 1);
      char* align_str = iupAttribGet(ih, name);
      float xalign = 0.0f;  /* Default: left */

      if (align_str)
      {
        if (iupStrEqualNoCase(align_str, "ARIGHT") || iupStrEqualNoCase(align_str, "RIGHT"))
          xalign = 1.0f;  /* Right */
        else if (iupStrEqualNoCase(align_str, "ACENTER") || iupStrEqualNoCase(align_str, "CENTER"))
          xalign = 0.5f;  /* Center */

        /* Apply alignment to cell renderer (for cell content) */
        g_object_set(G_OBJECT(renderer), "xalign", xalign, NULL);
      }

      /* Connect signals for cell editing */
      g_signal_connect(G_OBJECT(renderer), "editing-started", G_CALLBACK(gtkTableCellEditingStarted), ih);
      g_signal_connect(G_OBJECT(renderer), "edited", G_CALLBACK(gtkTableCellEdited), ih);
      g_signal_connect(G_OBJECT(renderer), "editing-canceled", G_CALLBACK(gtkTableCellEditingCanceled), ih);

      int text_model_col = gtkTableTextModelCol(ih, col);
      GtkTreeViewColumn* column = gtk_tree_view_column_new();

      if (ih->data->show_image)
      {
        GtkCellRenderer* pix_renderer = gtk_cell_renderer_pixbuf_new();
        gtk_tree_view_column_pack_start(column, pix_renderer, FALSE);
        gtk_tree_view_column_add_attribute(column, pix_renderer, "pixbuf", gtkTableImageModelCol(col));
      }

      gtk_tree_view_column_pack_start(column, renderer, TRUE);
      gtk_tree_view_column_add_attribute(column, renderer, "text", text_model_col);

      gtk_tree_view_column_set_resizable(column, TRUE);

      gtk_tree_view_column_set_reorderable(column, ih->data->allow_reorder);

      /* Set up cell data function for colors */
      IgtkCellDataInfo* data_info = (IgtkCellDataInfo*)malloc(sizeof(IgtkCellDataInfo));
      data_info->ih = ih;
      data_info->col = col + 1;  /* 1-based */
      gtk_tree_view_column_set_cell_data_func(column, renderer, gtkTableCellDataFunc, data_info, free);  /* free as destroy notify */

      /* Check if width was set before mapping */
      sprintf(name, "RASTERWIDTH%d", col + 1);
      char* width_str = iupAttribGet(ih, name);
      if (!width_str)
      {
        sprintf(name, "WIDTH%d", col + 1);
        width_str = iupAttribGet(ih, name);
      }

      int col_width = 0;
      int has_explicit_width = (width_str && iupStrToInt(width_str, &col_width) && col_width > 0);

      /* Handle last column */
      if (col == ih->data->num_col - 1)
      {
        if (has_explicit_width)
        {
          /* Explicit width set, use FIXED sizing */
          gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
          gtk_tree_view_column_set_fixed_width(column, col_width);
          gtk_tree_view_column_set_expand(column, FALSE);
        }
        else if (ih->data->stretch_last && !gtk_data->is_virtual)
        {
          /* No explicit width and stretching enabled, expand to fill (not in virtual mode) */
          gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
          gtk_tree_view_column_set_expand(column, TRUE);
        }
        else
        {
          /* No explicit width and stretching disabled (or virtual mode), use FIXED mode */
          const char* title = gtk_tree_view_column_get_title(column);
          int fixed_width = 80;  /* Default minimum */

          if (title && title[0])
          {
            int title_width = iupdrvFontGetStringWidth(ih, (char*)title);
            title_width += 20;  /* Padding for sort arrow */
            if (title_width > fixed_width)
              fixed_width = title_width;
          }

          gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
          gtk_tree_view_column_set_fixed_width(column, fixed_width);
          gtk_tree_view_column_set_expand(column, gtk_data->is_virtual ? FALSE : ih->data->stretch_last);
        }
      }
      else
      {
        /* Non-last columns */
        if (has_explicit_width)
        {
          gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
          gtk_tree_view_column_set_fixed_width(column, col_width);
        }
        else if (gtk_data->is_virtual)
        {
          /* Virtual mode requires FIXED sizing for fixed_height_mode */
          gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
        }
        else
        {
          gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
        }
        gtk_tree_view_column_set_expand(column, FALSE);
      }

      /* Check if column should be sortable (from ih->data) */
      IgtkTableData* gtk_data = IGTK_TABLE_DATA(ih);
      if (ih->data->sortable)
      {
        if (gtk_data->is_virtual)
        {
          /* Virtual mode - disable GTK automatic sorting, use manual handling */
          gtk_tree_view_column_set_sort_column_id(column, -1);
          gtk_tree_view_column_set_clickable(column, TRUE);
          /* Connect clicked signal for manual sort handling */
          g_signal_connect(G_OBJECT(column), "clicked", G_CALLBACK(gtkTableColumnClicked), ih);
        }
        else
        {
          /* Normal mode - enable GTK's automatic sorting and connect clicked signal for SORT_CB notification */
          gtk_tree_view_column_set_sort_column_id(column, text_model_col);
          gtk_tree_view_column_set_clickable(column, TRUE);
          g_signal_connect(G_OBJECT(column), "clicked", G_CALLBACK(gtkTableColumnClicked), ih);
        }
      }
      else
      {
        gtk_tree_view_column_set_sort_column_id(column, -1);
        gtk_tree_view_column_set_clickable(column, FALSE);
      }

      /* Apply alignment to column header AFTER making it clickable
       * (clickable adds a button widget which has its own default alignment) */
      if (align_str)
        gtk_tree_view_column_set_alignment(column, xalign);

      gtk_tree_view_append_column(GTK_TREE_VIEW(gtk_data->tree_view), column);
    }

    /* if ALL columns have expand=FALSE, it gives extra space to the LAST column.
     * To prevent this when STRETCHLAST=NO, add a visible dummy column with expand=TRUE to absorb the extra space.
     * Also add dummy if last column has explicit width, in that case it shouldn't stretch even if STRETCHLAST=YES */
    int last_col_has_width = 0;
    {
      char name[50];
      sprintf(name, "RASTERWIDTH%d", ih->data->num_col);
      char* width_str = iupAttribGet(ih, name);
      if (!width_str)
      {
        sprintf(name, "WIDTH%d", ih->data->num_col);
        width_str = iupAttribGet(ih, name);
      }
      int width = 0;
      last_col_has_width = (width_str && iupStrToInt(width_str, &width) && width > 0);
    }

    if (!ih->data->stretch_last || last_col_has_width)
    {
      GtkCellRenderer* dummy_renderer = gtk_cell_renderer_text_new();
      GtkTreeViewColumn* dummy_column = gtk_tree_view_column_new_with_attributes("", dummy_renderer, NULL);

      /* Must be VISIBLE (GTK finds last_column by skipping invisible ones) but with minimal width */
      gtk_tree_view_column_set_visible(dummy_column, TRUE);
      gtk_tree_view_column_set_sizing(dummy_column, GTK_TREE_VIEW_COLUMN_FIXED);
      gtk_tree_view_column_set_fixed_width(dummy_column, 1);
      gtk_tree_view_column_set_expand(dummy_column, TRUE);
      gtk_tree_view_column_set_resizable(dummy_column, FALSE);
      gtk_tree_view_column_set_reorderable(dummy_column, FALSE);
      gtk_tree_view_column_set_clickable(dummy_column, FALSE);

      gtk_tree_view_append_column(GTK_TREE_VIEW(gtk_data->tree_view), dummy_column);

      /* Store dummy column pointer so we can check for it in event handlers */
      iupAttribSet(ih, "_IUPGTK_DUMMY_COLUMN", (char*)dummy_column);
    }
  }

  /* Add tree view to scrolled window */
  gtk_container_add(GTK_CONTAINER(gtk_data->scrolled_win), gtk_data->tree_view);

  /* Set the main handle to tree view, scrolled window as extra parent */
  ih->handle = gtk_data->tree_view;
  iupAttribSet(ih, "_IUP_EXTRAPARENT", (char*)gtk_data->scrolled_win);

  /* Connect signals */
  g_signal_connect(gtk_data->tree_view, "button-press-event", G_CALLBACK(gtkTableButtonEvent), ih);
  g_signal_connect(gtk_data->tree_view, "key-press-event", G_CALLBACK(gtkTableKeyPressEvent), ih);
  g_signal_connect(gtk_data->tree_view, "cursor-changed", G_CALLBACK(gtkTableCursorChanged), ih);
#if GTK_CHECK_VERSION(3, 0, 0)
  g_signal_connect_after(gtk_data->tree_view, "draw", G_CALLBACK(gtkTableDrawFocusRect), ih);
#else
  g_signal_connect_after(gtk_data->tree_view, "expose-event", G_CALLBACK(gtkTableDrawFocusRect), ih);
#endif

  /* Show all widgets */
  gtk_widget_show(gtk_data->tree_view);
  gtk_widget_show(gtk_data->scrolled_win);

  /* Add to parent */
  iupgtkAddToParent(ih);

  /* Ensure TreeView can receive focus for cell editing */
  if (iupAttribGetBoolean(ih, "CANFOCUS"))
    iupgtkSetCanFocus(ih->handle, 1);  /* Explicitly enable if CANFOCUS=YES */

  /* Apply stored attributes that need widget to be created */

  /* Set column titles */
  for (col = 1; col <= ih->data->num_col; col++)
  {
    char name[50];
    sprintf(name, "TITLE%d", col);
    char* title = iupAttribGet(ih, name);
    if (title)
      iupdrvTableSetColTitle(ih, col, title);
  }

  /* Set number of rows */
  if (ih->data->num_lin > 0)
    iupdrvTableSetNumLin(ih, ih->data->num_lin);

  /* Set show grid */
  iupdrvTableSetShowGrid(ih, iupAttribGetBoolean(ih, "SHOWGRID"));

  /* Set height constraint when VISIBLELINES is set.
     Store target height in widget data for size-allocate handler to clamp.
     GtkFixed ignores vexpand/valign, so we must use size-allocate clamping. */
  int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
  if (visiblelines > 0)
  {
    int row_height = iupdrvTableGetRowHeight(ih);
    int header_height = iupdrvTableGetHeaderHeight(ih);
    int content_height = header_height + (row_height * visiblelines);
    content_height += 2;  /* scrolled window border */

    g_object_set_data(G_OBJECT(gtk_data->scrolled_win), "iup-table-target-height", GINT_TO_POINTER(content_height));
  }

  return IUP_NOERROR;
}

static void gtkTableUnMapMethod(Ihandle* ih)
{
  IgtkTableData* gtk_data = IGTK_TABLE_DATA(ih);

  if (gtk_data)
  {
    /* For virtual mode, set num_lin to 0 so GTK sees empty model during detach */
    if (gtk_data->is_virtual && ih->data)
    {
      ih->data->num_lin = 0;
    }

    /* Detach model from tree_view BEFORE destruction to avoid expensive iteration during cleanup */
    if (gtk_data->tree_view)
    {
      gtk_tree_view_set_model(GTK_TREE_VIEW(gtk_data->tree_view), NULL);
    }

    /* Clean up virtual model if it was used */
    if (gtk_data->virtual_model)
    {
      g_object_unref(gtk_data->virtual_model);
    }

    if (gtk_data->column_types)
      free(gtk_data->column_types);

    free(gtk_data);
    ih->data->native_data = NULL;
  }

  iupdrvBaseUnMapMethod(ih);
}

/* ========================================================================= */
/* Helper function for dialog to detach virtual table models early         */
/* ========================================================================= */

static void iupgtkTableDetachVirtualModelsRecursive(Ihandle* ih)
{
  Ihandle* child;

  if (!ih)
    return;

  /* Check if this element is a virtual table */
  if (iupStrEqual(ih->iclass->name, "table"))
  {
    IgtkTableData* gtk_data = IGTK_TABLE_DATA(ih);
    if (gtk_data && gtk_data->is_virtual && gtk_data->tree_view)
    {
      ih->data->num_lin = 0;
      gtk_tree_view_set_model(GTK_TREE_VIEW(gtk_data->tree_view), NULL);
    }
  }

  /* Recursively process all children */
  child = ih->firstchild;
  while (child)
  {
    iupgtkTableDetachVirtualModelsRecursive(child);
    child = child->brother;
  }
}

void iupgtkTableDetachVirtualModels(Ihandle* dialog)
{
  iupgtkTableDetachVirtualModelsRecursive(dialog);
}

/* ========================================================================= */
/* Driver Functions - Table Structure                                       */
/* ========================================================================= */

void iupdrvTableSetNumCol(Ihandle* ih, int num_col)
{
  if (num_col < 0)
    num_col = 0;

  ih->data->num_col = num_col;

  /* Recreate store with new number of columns */
  gtkTableEnsureStore(ih);

  /* Update columns in tree view */
  gtkTableUpdateColumns(ih);
}

void iupdrvTableSetNumLin(Ihandle* ih, int num_lin)
{
  IgtkTableData* gtk_data = IGTK_TABLE_DATA(ih);

  if (!gtk_data)
    return;

  if (num_lin < 0)
    num_lin = 0;

  ih->data->num_lin = num_lin;

  /* In virtual mode, just trigger redraw */
  if (gtk_data->is_virtual)
  {
    if (gtk_data->tree_view)
      gtk_widget_queue_draw(gtk_data->tree_view);
    return;
  }

  /* Normal mode: manage actual rows in GtkListStore */
  if (!gtk_data->store)
    return;

  int current_rows = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(gtk_data->store), NULL);

  if (num_lin > current_rows)
  {
    /* Add rows */
    int i;
    for (i = current_rows; i < num_lin; i++)
    {
      GtkTreeIter iter;
      gtk_list_store_append(gtk_data->store, &iter);

      /* Initialize all cells to empty string */
      int col;
      for (col = 0; col < ih->data->num_col; col++)
      {
        gtk_list_store_set(gtk_data->store, &iter, gtkTableTextModelCol(ih, col), "", -1);
      }
    }
  }
  else if (num_lin < current_rows)
  {
    /* Remove rows */
    while (current_rows > num_lin)
    {
      GtkTreeIter iter;
      GtkTreePath* path = gtk_tree_path_new_from_indices(num_lin, -1);

      if (gtk_tree_model_get_iter(GTK_TREE_MODEL(gtk_data->store), &iter, path))
      {
        gtk_list_store_remove(gtk_data->store, &iter);
      }

      gtk_tree_path_free(path);
      current_rows--;
    }
  }

  ih->data->num_lin = num_lin;

  int final_rows = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(gtk_data->store), NULL);
}

void iupdrvTableAddCol(Ihandle* ih, int pos)
{
  iupdrvTableSetNumCol(ih, ih->data->num_col + 1);
}

void iupdrvTableDelCol(Ihandle* ih, int pos)
{
  if (ih->data->num_col > 0)
    iupdrvTableSetNumCol(ih, ih->data->num_col - 1);
}

void iupdrvTableAddLin(Ihandle* ih, int pos)
{
  IgtkTableData* gtk_data = IGTK_TABLE_DATA(ih);

  if (!gtk_data || !gtk_data->store)
    return;

  GtkTreeIter iter;

  if (pos <= 0 || pos > ih->data->num_lin + 1)
  {
    /* Append */
    gtk_list_store_append(gtk_data->store, &iter);
  }
  else
  {
    /* Insert at position (pos-1 because GTK is 0-based) */
    GtkTreeIter iter_pos;
    GtkTreePath* path = gtk_tree_path_new_from_indices(pos - 1, -1);

    if (gtk_tree_model_get_iter(GTK_TREE_MODEL(gtk_data->store), &iter_pos, path))
    {
      gtk_list_store_insert_before(gtk_data->store, &iter, &iter_pos);
    }
    else
    {
      gtk_list_store_append(gtk_data->store, &iter);
    }

    gtk_tree_path_free(path);
  }

  /* Initialize all cells to empty string */
  int col;
  for (col = 0; col < ih->data->num_col; col++)
  {
    gtk_list_store_set(gtk_data->store, &iter, gtkTableTextModelCol(ih, col), "", -1);
  }

  ih->data->num_lin++;
}

void iupdrvTableDelLin(Ihandle* ih, int pos)
{
  IgtkTableData* gtk_data = IGTK_TABLE_DATA(ih);

  if (!gtk_data || !gtk_data->store)
    return;

  if (pos < 1 || pos > ih->data->num_lin)
    return;

  GtkTreeIter iter;
  GtkTreePath* path = gtk_tree_path_new_from_indices(pos - 1, -1);

  if (gtk_tree_model_get_iter(GTK_TREE_MODEL(gtk_data->store), &iter, path))
  {
    gtk_list_store_remove(gtk_data->store, &iter);
    ih->data->num_lin--;
  }

  gtk_tree_path_free(path);
}

/* ========================================================================= */
/* Driver Functions - Cell Operations                                       */
/* ========================================================================= */

void iupdrvTableSetCellValue(Ihandle* ih, int lin, int col, const char* value)
{
  IgtkTableData* gtk_data = IGTK_TABLE_DATA(ih);

  if (!gtk_data || !gtk_data->store)
  {
    return;
  }

  if (lin < 1 || lin > ih->data->num_lin || col < 1 || col > ih->data->num_col)
  {
    return;
  }

  GtkTreeIter iter;
  GtkTreePath* path = gtk_tree_path_new_from_indices(lin - 1, -1);

  if (gtk_tree_model_get_iter(GTK_TREE_MODEL(gtk_data->store), &iter, path))
  {
    int model_col = gtkTableTextModelCol(ih, col - 1);
    gtk_list_store_set(gtk_data->store, &iter, model_col, value ? value : "", -1);
  }

  gtk_tree_path_free(path);
}

char* iupdrvTableGetCellValue(Ihandle* ih, int lin, int col)
{
  IgtkTableData* gtk_data = IGTK_TABLE_DATA(ih);

  if (!gtk_data || !gtk_data->store)
    return NULL;

  if (lin < 1 || lin > ih->data->num_lin || col < 1 || col > ih->data->num_col)
    return NULL;

  GtkTreeIter iter;
  GtkTreePath* path = gtk_tree_path_new_from_indices(lin - 1, -1);
  char* value = NULL;

  if (gtk_tree_model_get_iter(GTK_TREE_MODEL(gtk_data->store), &iter, path))
  {
    gchar* gtk_value;
    int model_col = gtkTableTextModelCol(ih, col - 1);
    gtk_tree_model_get(GTK_TREE_MODEL(gtk_data->store), &iter, model_col, &gtk_value, -1);

    if (gtk_value)
    {
      value = iupStrReturnStr(gtk_value);
      g_free(gtk_value);
    }
  }

  gtk_tree_path_free(path);
  return value;
}

void iupdrvTableSetCellImage(Ihandle* ih, int lin, int col, const char* image)
{
  IgtkTableData* gtk_data = IGTK_TABLE_DATA(ih);

  if (!gtk_data || !gtk_data->store)
    return;

  if (lin < 1 || lin > ih->data->num_lin || col < 1 || col > ih->data->num_col)
    return;

  GtkTreeIter iter;
  GtkTreePath* path = gtk_tree_path_new_from_indices(lin - 1, -1);

  if (gtk_tree_model_get_iter(GTK_TREE_MODEL(gtk_data->store), &iter, path))
  {
    int model_col = gtkTableImageModelCol(col - 1);
    GdkPixbuf* pixbuf = NULL;

    if (image)
    {
      pixbuf = (GdkPixbuf*)iupImageGetImage(image, ih, 0, NULL);
      if (pixbuf && ih->data->fit_image)
      {
        int charheight;
        iupdrvFontGetCharSize(ih, NULL, &charheight);
        int available_height = charheight + 4;
        int img_height = gdk_pixbuf_get_height(pixbuf);
        if (img_height > available_height)
        {
          int img_width = gdk_pixbuf_get_width(pixbuf);
          int scaled_width = (img_width * available_height) / img_height;
          GdkPixbuf* scaled = gdk_pixbuf_scale_simple(pixbuf, scaled_width, available_height, GDK_INTERP_BILINEAR);
          gtk_list_store_set(gtk_data->store, &iter, model_col, scaled, -1);
          g_object_unref(scaled);
          gtk_tree_path_free(path);
          return;
        }
      }
    }

    gtk_list_store_set(gtk_data->store, &iter, model_col, pixbuf, -1);
  }

  gtk_tree_path_free(path);
}

/* ========================================================================= */
/* Driver Functions - Column Operations                                     */
/* ========================================================================= */

void iupdrvTableSetColTitle(Ihandle* ih, int col, const char* title)
{
  IgtkTableData* gtk_data = IGTK_TABLE_DATA(ih);

  if (col < 1 || col > ih->data->num_col)
    return;

  GtkTreeViewColumn* column = gtk_tree_view_get_column(GTK_TREE_VIEW(gtk_data->tree_view), col - 1);
  if (column)
  {
    gtk_tree_view_column_set_title(column, title ? title : "");
  }
}

char* iupdrvTableGetColTitle(Ihandle* ih, int col)
{
  IgtkTableData* gtk_data = IGTK_TABLE_DATA(ih);

  if (col < 1 || col > ih->data->num_col)
    return NULL;

  GtkTreeViewColumn* column = gtk_tree_view_get_column(GTK_TREE_VIEW(gtk_data->tree_view), col - 1);
  if (column)
  {
    const gchar* title = gtk_tree_view_column_get_title(column);
    return iupStrReturnStr(title);
  }

  return NULL;
}

void iupdrvTableSetColWidth(Ihandle* ih, int col, int width)
{
  IgtkTableData* gtk_data = IGTK_TABLE_DATA(ih);

  if (col < 1 || col > ih->data->num_col)
    return;

  GtkTreeViewColumn* column = gtk_tree_view_get_column(GTK_TREE_VIEW(gtk_data->tree_view), col - 1);
  if (column)
  {
    /* Set the explicit width first */
    gtk_tree_view_column_set_fixed_width(column, width);

    /* Determine sizing mode based on USERRESIZE setting */
    if (ih->data->user_resize)
    {
      /* AUTOSIZE with resizable to allow user manual resizing */
      gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
      gtk_tree_view_column_set_resizable(column, TRUE);
    }
    else
    {
      /* FIXED width when explicit width is set */
      gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
      gtk_tree_view_column_set_resizable(column, FALSE);
    }
  }
}

int iupdrvTableGetColWidth(Ihandle* ih, int col)
{
  IgtkTableData* gtk_data = IGTK_TABLE_DATA(ih);

  if (col < 1 || col > ih->data->num_col)
    return 0;

  GtkTreeViewColumn* column = gtk_tree_view_get_column(GTK_TREE_VIEW(gtk_data->tree_view), col - 1);
  if (column)
  {
    return gtk_tree_view_column_get_width(column);
  }

  return 0;
}

/* ========================================================================= */
/* Driver Functions - Selection                                             */
/* ========================================================================= */

void iupdrvTableSetFocusCell(Ihandle* ih, int lin, int col)
{
  IgtkTableData* gtk_data = IGTK_TABLE_DATA(ih);

  if (lin < 1 || lin > ih->data->num_lin || col < 1 || col > ih->data->num_col)
    return;

  GtkTreePath* path = gtk_tree_path_new_from_indices(lin - 1, -1);
  GtkTreeViewColumn* column = gtk_tree_view_get_column(GTK_TREE_VIEW(gtk_data->tree_view), col - 1);

  if (column)
  {
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(gtk_data->tree_view), path, column, FALSE);
  }

  gtk_tree_path_free(path);

  /* Explicitly trigger ENTERITEM_CB callback for programmatic focus changes */
  IFnii enteritem_cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
  if (enteritem_cb)
  {
    enteritem_cb(ih, lin, col);
  }
}

void iupdrvTableGetFocusCell(Ihandle* ih, int* lin, int* col)
{
  IgtkTableData* gtk_data = IGTK_TABLE_DATA(ih);

  GtkTreePath* path;
  GtkTreeViewColumn* column;

  gtk_tree_view_get_cursor(GTK_TREE_VIEW(gtk_data->tree_view), &path, &column);

  if (path)
  {
    int* indices = gtk_tree_path_get_indices(path);
    *lin = indices[0] + 1;  /* 1-based */

    if (column)
    {
      GList* columns = gtk_tree_view_get_columns(GTK_TREE_VIEW(gtk_data->tree_view));
      *col = g_list_index(columns, column) + 1;  /* 1-based */
      g_list_free(columns);
    }
    else
    {
      *col = 1;
    }

    gtk_tree_path_free(path);
  }
  else
  {
    *lin = 1;
    *col = 1;
  }
}

/* ========================================================================= */
/* Driver Functions - Scrolling                                             */
/* ========================================================================= */

void iupdrvTableScrollToCell(Ihandle* ih, int lin, int col)
{
  IgtkTableData* gtk_data = IGTK_TABLE_DATA(ih);

  if (lin < 1 || lin > ih->data->num_lin || col < 1 || col > ih->data->num_col)
    return;

  GtkTreePath* path = gtk_tree_path_new_from_indices(lin - 1, -1);
  GtkTreeViewColumn* column = gtk_tree_view_get_column(GTK_TREE_VIEW(gtk_data->tree_view), col - 1);

  gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(gtk_data->tree_view), path, column, FALSE, 0.0, 0.0);

  gtk_tree_path_free(path);
}

/* ========================================================================= */
/* Driver Functions - Display                                               */
/* ========================================================================= */

void iupdrvTableRedraw(Ihandle* ih)
{
  IgtkTableData* gtk_data = IGTK_TABLE_DATA(ih);

  gtk_widget_queue_draw(gtk_data->tree_view);
}

void iupdrvTableSetShowGrid(Ihandle* ih, int show)
{
  IgtkTableData* gtk_data = IGTK_TABLE_DATA(ih);

  gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(gtk_data->tree_view), show ? GTK_TREE_VIEW_GRID_LINES_BOTH : GTK_TREE_VIEW_GRID_LINES_NONE);
}

int iupdrvTableGetBorderWidth(Ihandle* ih)
{
  (void)ih;
  /* GtkScrolledWindow doesn't add extra border width */
  return 0;
}

static int gtk3_table_row_height_nogrid = -1;
static int gtk3_table_row_height_grid = -1;
static int gtk3_table_header_height = -1;

static void gtk3TableMeasureRowMetrics(Ihandle* ih, int with_grid)
{
  int* row_height_ptr = with_grid ? &gtk3_table_row_height_grid : &gtk3_table_row_height_nogrid;
  if (*row_height_ptr >= 0 && gtk3_table_header_height >= 0)
    return;

  GtkWidget* temp_window = gtk_offscreen_window_new();
  GtkListStore* store = gtk_list_store_new(1, G_TYPE_STRING);
  GtkWidget* tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));

  /* Add a column with text renderer */
  GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
  (void)renderer;
  GtkTreeViewColumn* column = gtk_tree_view_column_new_with_attributes(
    "Test", renderer, "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

  /* Add two rows, measure height as Y difference between them */
  GtkTreeIter iter;
  gtk_list_store_append(store, &iter);
  gtk_list_store_set(store, &iter, 0, "X", -1);
  gtk_list_store_append(store, &iter);
  gtk_list_store_set(store, &iter, 0, "X", -1);

  /* Match grid lines setting */
  if (with_grid)
    gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(tree_view), GTK_TREE_VIEW_GRID_LINES_BOTH);

  gtk_container_add(GTK_CONTAINER(temp_window), tree_view);
  gtk_widget_show_all(temp_window);
  gtk_widget_realize(temp_window);
  gtk_widget_realize(tree_view);

  /* Measure row height by comparing Y positions of row 0 and row 1 */
  {
    GtkTreePath* path0 = gtk_tree_path_new_from_string("0");
    GtkTreePath* path1 = gtk_tree_path_new_from_string("1");
    GdkRectangle rect0, rect1;

    gtk_tree_view_get_background_area(GTK_TREE_VIEW(tree_view), path0, NULL, &rect0);
    gtk_tree_view_get_background_area(GTK_TREE_VIEW(tree_view), path1, NULL, &rect1);

    *row_height_ptr = rect1.y - rect0.y;

    gtk_tree_path_free(path0);
    gtk_tree_path_free(path1);
  }

  /* Fallback if measurement didn't work */
  if (*row_height_ptr <= 0)
  {
    int charheight;
    iupdrvFontGetCharSize(ih, NULL, &charheight);
    *row_height_ptr = charheight + 4 + (with_grid ? 1 : 0);
  }

  /* Measure header height */
  if (gtk3_table_header_height < 0)
  {
#if GTK_CHECK_VERSION(3, 0, 0)
    GtkWidget* header_button = gtk_tree_view_column_get_button(column);
    if (header_button)
    {
      GtkRequisition req;
      gtk_widget_get_preferred_size(header_button, NULL, &req);
      gtk3_table_header_height = req.height;
    }
    else
    {
      gtk3_table_header_height = *row_height_ptr + 4;
    }
#else
    int charheight;
    iupdrvFontGetCharSize(ih, NULL, &charheight);
    gtk3_table_header_height = charheight + 8;
#endif
  }

  /* Clean up */
  g_object_unref(store);
  gtk_widget_destroy(temp_window);
}

int iupdrvTableGetRowHeight(Ihandle* ih)
{
  IgtkTableData* gtk_data = IGTK_TABLE_DATA(ih);

  /* If table is mapped and has at least 2 rows, measure pitch between rows */
  if (gtk_data && gtk_data->tree_view)
  {
    GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(gtk_data->tree_view));
    int row_count = model ? gtk_tree_model_iter_n_children(model, NULL) : 0;

    if (row_count >= 2)
    {
      GtkTreePath* path0 = gtk_tree_path_new_from_string("0");
      GtkTreePath* path1 = gtk_tree_path_new_from_string("1");
      GdkRectangle rect0, rect1;

      gtk_tree_view_get_background_area(GTK_TREE_VIEW(gtk_data->tree_view), path0, NULL, &rect0);
      gtk_tree_view_get_background_area(GTK_TREE_VIEW(gtk_data->tree_view), path1, NULL, &rect1);

      int row_height = rect1.y - rect0.y;

      gtk_tree_path_free(path0);
      gtk_tree_path_free(path1);

      if (row_height > 0)
        return row_height;
    }
  }

  /* Fallback to pre-measured value */
  int with_grid = iupAttribGetBoolean(ih, "SHOWGRID");
  gtk3TableMeasureRowMetrics(ih, with_grid);
  return with_grid ? gtk3_table_row_height_grid : gtk3_table_row_height_nogrid;
}

int iupdrvTableGetHeaderHeight(Ihandle* ih)
{
  IgtkTableData* gtk_data = IGTK_TABLE_DATA(ih);

#if GTK_CHECK_VERSION(3, 0, 0)
  /* If table is mapped, measure from actual header */
  if (gtk_data && gtk_data->tree_view)
  {
    GList* columns = gtk_tree_view_get_columns(GTK_TREE_VIEW(gtk_data->tree_view));
    if (columns)
    {
      GtkTreeViewColumn* column = GTK_TREE_VIEW_COLUMN(columns->data);
      GtkWidget* button = gtk_tree_view_column_get_button(column);
      if (button)
      {
        GtkRequisition req;
        gtk_widget_get_preferred_size(button, NULL, &req);
        g_list_free(columns);
        return req.height;
      }
      g_list_free(columns);
    }
  }
#else
  (void)gtk_data;
#endif

  gtk3TableMeasureRowMetrics(ih, iupAttribGetBoolean(ih, "SHOWGRID"));
  return gtk3_table_header_height;
}

void iupdrvTableAddBorders(Ihandle* ih, int* w, int* h)
{
  int sb_size = iupdrvGetScrollbarSize();

  /* GtkScrolledWindow: add vertical scrollbar width */
  *w += sb_size;

  /* GtkScrolledWindow frame border */
  *h += 2;

  /* Add horizontal scrollbar height when VISIBLECOLUMNS causes it to appear */
  int visiblecolumns = iupAttribGetInt(ih, "VISIBLECOLUMNS");
  if (visiblecolumns > 0 && ih->data->num_col > visiblecolumns)
    *h += sb_size;
}


/* ========================================================================= */
/* Attribute Handlers                                                        */
/* ========================================================================= */

static int gtkTableSetSortableAttrib(Ihandle* ih, const char* value)
{
  /* Store value in ih->data first (like IupTree SHOWRENAME pattern) */
  if (iupStrBoolean(value))
    ih->data->sortable = 1;
  else
    ih->data->sortable = 0;

  /* Apply to native widget if it exists */
  if (ih->handle)
  {
    IgtkTableData* gtk_data = IGTK_TABLE_DATA(ih);
    if (gtk_data && gtk_data->tree_view)
    {
      GList* columns = gtk_tree_view_get_columns(GTK_TREE_VIEW(gtk_data->tree_view));

      for (GList* l = columns; l != NULL; l = l->next)
      {
        GtkTreeViewColumn* column = GTK_TREE_VIEW_COLUMN(l->data);
        int col_index = g_list_index(columns, column);

        if (ih->data->sortable)
        {
          gtk_tree_view_column_set_sort_column_id(column, gtkTableTextModelCol(ih, col_index));
          gtk_tree_view_column_set_clickable(column, TRUE);
        }
        else
        {
          gtk_tree_view_column_set_sort_column_id(column, -1);
          gtk_tree_view_column_set_clickable(column, FALSE);
        }
      }

      g_list_free(columns);
    }
  }
  return 0; /* do not store in hash table */
}

static int gtkTableSetAllowReorderAttrib(Ihandle* ih, const char* value)
{
  /* Store value in ih->data first (like IupTree SHOWRENAME pattern) */
  if (iupStrBoolean(value))
    ih->data->allow_reorder = 1;
  else
    ih->data->allow_reorder = 0;

  /* Apply to native widget if it exists */
  if (ih->handle)
  {
    IgtkTableData* gtk_data = IGTK_TABLE_DATA(ih);
    if (gtk_data && gtk_data->tree_view)
    {
      GList* columns = gtk_tree_view_get_columns(GTK_TREE_VIEW(gtk_data->tree_view));

      for (GList* l = columns; l != NULL; l = l->next)
      {
        GtkTreeViewColumn* column = GTK_TREE_VIEW_COLUMN(l->data);
        gtk_tree_view_column_set_reorderable(column, ih->data->allow_reorder);
      }

      g_list_free(columns);
    }
  }
  return 0; /* do not store in hash table */
}

static int gtkTableSetUserResizeAttrib(Ihandle* ih, const char* value)
{
  IgtkTableData* gtk_data = IGTK_TABLE_DATA(ih);

  /* First, update ih->data->user_resize flag */
  if (iupStrBoolean(value))
    ih->data->user_resize = 1;
  else
    ih->data->user_resize = 0;

  if (!gtk_data || !gtk_data->tree_view)
    return 0;

  /* Update resize modes for all existing columns */
  int col;
  for (col = 0; col < ih->data->num_col; col++)
  {
    GtkTreeViewColumn* column = gtk_tree_view_get_column(GTK_TREE_VIEW(gtk_data->tree_view), col);
    if (column)
    {
      if (ih->data->user_resize)
      {
        /* AUTOSIZE with resizable for all columns */
        gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
        gtk_tree_view_column_set_resizable(column, TRUE);
      }
      else
      {
        /* Return to FIXED/AUTOSIZE based on whether width was set */
        if (gtk_tree_view_column_get_fixed_width(column) > 0)
        {
          gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
          gtk_tree_view_column_set_resizable(column, FALSE);
        }
        else
        {
          gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
          gtk_tree_view_column_set_resizable(column, FALSE);
        }
      }
    }
  }

  return 0; /* do not store in hash table */
}

/* ========================================================================= */
/* Driver Class Initialization                                              */
/* ========================================================================= */

void iupdrvTableInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = gtkTableMapMethod;
  ic->UnMap = gtkTableUnMapMethod;
  ic->LayoutUpdate = gtkTableLayoutUpdateMethod;

  /* Register GTK-specific attributes */
  iupClassRegisterAttribute(ic, "FONT", NULL, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NO_SAVE | IUPAF_NOT_MAPPED);

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);

  /* Native size */
  iupClassRegisterAttribute(ic, "SIZE", NULL, NULL, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NOT_MAPPED);

  /* Replace core SET handlers to update native widget */
  iupClassRegisterReplaceAttribFunc(ic, "SORTABLE", NULL, gtkTableSetSortableAttrib);
  iupClassRegisterReplaceAttribFunc(ic, "ALLOWREORDER", NULL, gtkTableSetAllowReorderAttrib);
  iupClassRegisterReplaceAttribFunc(ic, "USERRESIZE", NULL, gtkTableSetUserResizeAttrib);
}
