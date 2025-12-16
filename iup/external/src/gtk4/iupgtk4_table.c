/** \file
 * \brief IupTable control - GTK4 driver (GtkColumnView)
 *
 * See Copyright Notice in "iup.h"
 */

#include <gtk/gtk.h>
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

#include "iupgtk4_drv.h"
#include "iup_tablecontrol.h"


/* ========================================================================= */
/* Row Data Object for GListModel                                           */
/* ========================================================================= */

typedef struct _IupTableRow IupTableRow;
typedef struct _IupTableRowClass IupTableRowClass;

struct _IupTableRow
{
  GObject parent;
  gchar** values;  /* Array of string values (one per column) */
  gint num_cols;
  gint row_index;  /* 1-based row index for IUP callbacks */
  guint update_count;  /* Incremented whenever any value changes */
};

struct _IupTableRowClass
{
  GObjectClass parent_class;
};

enum {
  PROP_0,
  PROP_UPDATE,
  N_PROPS
};

static GParamSpec* row_props[N_PROPS] = { NULL, };

GType iup_table_row_get_type(void);
#define IUP_TYPE_TABLE_ROW (iup_table_row_get_type())
#define IUP_TABLE_ROW(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), IUP_TYPE_TABLE_ROW, IupTableRow))

G_DEFINE_TYPE(IupTableRow, iup_table_row, G_TYPE_OBJECT)

static void iup_table_row_finalize(GObject* object)
{
  IupTableRow* row = IUP_TABLE_ROW(object);

  if (row->values)
  {
    for (gint i = 0; i < row->num_cols; i++)
    {
      g_free(row->values[i]);
    }
    g_free(row->values);
  }

  G_OBJECT_CLASS(iup_table_row_parent_class)->finalize(object);
}

static void iup_table_row_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec)
{
  IupTableRow* row = IUP_TABLE_ROW(object);
  switch (prop_id)
  {
    case PROP_UPDATE:
      g_value_set_uint(value, row->update_count);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
  }
}

static void iup_table_row_init(IupTableRow* row)
{
  row->values = NULL;
  row->num_cols = 0;
  row->row_index = 0;
  row->update_count = 0;
}

static void iup_table_row_class_init(IupTableRowClass* klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = iup_table_row_finalize;
  object_class->get_property = iup_table_row_get_property;

  row_props[PROP_UPDATE] = g_param_spec_uint("update", "Update", "Update counter",
                                               0, G_MAXUINT, 0,
                                               G_PARAM_READABLE);
  g_object_class_install_properties(object_class, N_PROPS, row_props);
}

static IupTableRow* iup_table_row_new(gint num_cols, gint row_index)
{
  IupTableRow* row = g_object_new(IUP_TYPE_TABLE_ROW, NULL);
  row->num_cols = num_cols;
  row->row_index = row_index;
  row->values = g_new0(gchar*, num_cols);

  for (gint i = 0; i < num_cols; i++)
  {
    row->values[i] = g_strdup("");
  }

  return row;
}

static void iup_table_row_set_value(IupTableRow* row, gint col_index, const char* value)
{
  if (!row || col_index < 0 || col_index >= row->num_cols)
    return;

  g_free(row->values[col_index]);
  row->values[col_index] = g_strdup(value ? value : "");

  row->update_count++;
  g_object_notify_by_pspec(G_OBJECT(row), row_props[PROP_UPDATE]);
}

/* ========================================================================= */
/* Virtual Model for VIRTUALMODE (GListModel implementation)                */
/* ========================================================================= */

typedef struct _IupTableVirtualModel IupTableVirtualModel;
typedef struct _IupTableVirtualModelClass IupTableVirtualModelClass;

struct _IupTableVirtualModel
{
  GObject parent;
  Ihandle* ih;
};

struct _IupTableVirtualModelClass
{
  GObjectClass parent_class;
};

GType iup_table_virtual_model_get_type(void);
#define IUP_TYPE_TABLE_VIRTUAL_MODEL (iup_table_virtual_model_get_type())
#define IUP_TABLE_VIRTUAL_MODEL(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), IUP_TYPE_TABLE_VIRTUAL_MODEL, IupTableVirtualModel))

static void iup_table_virtual_model_list_model_init(GListModelInterface* iface);

G_DEFINE_TYPE_WITH_CODE(IupTableVirtualModel, iup_table_virtual_model, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE(G_TYPE_LIST_MODEL, iup_table_virtual_model_list_model_init))

static void iup_table_virtual_model_init(IupTableVirtualModel* model)
{
  model->ih = NULL;
}

static void iup_table_virtual_model_class_init(IupTableVirtualModelClass* klass)
{
}

static GType iup_table_virtual_model_get_item_type(GListModel* list)
{
  return IUP_TYPE_TABLE_ROW;
}

static guint iup_table_virtual_model_get_n_items(GListModel* list)
{
  IupTableVirtualModel* model = IUP_TABLE_VIRTUAL_MODEL(list);
  return model->ih ? model->ih->data->num_lin : 0;
}

static gpointer iup_table_virtual_model_get_item(GListModel* list, guint position)
{
  IupTableVirtualModel* model = IUP_TABLE_VIRTUAL_MODEL(list);

  if (!model->ih || position >= (guint)model->ih->data->num_lin)
    return NULL;

  IupTableRow* row = iup_table_row_new(model->ih->data->num_col, position + 1);

  sIFnii value_cb = (sIFnii)IupGetCallback(model->ih, "VALUE_CB");
  if (value_cb)
  {
    for (gint col = 0; col < model->ih->data->num_col; col++)
    {
      char* value = value_cb(model->ih, position + 1, col + 1);
      if (value)
      {
        g_free(row->values[col]);
        row->values[col] = g_strdup(value);
      }
    }
  }

  return row;
}

static void iup_table_virtual_model_list_model_init(GListModelInterface* iface)
{
  iface->get_item_type = iup_table_virtual_model_get_item_type;
  iface->get_n_items = iup_table_virtual_model_get_n_items;
  iface->get_item = iup_table_virtual_model_get_item;
}

static IupTableVirtualModel* iup_table_virtual_model_new(Ihandle* ih)
{
  IupTableVirtualModel* model = g_object_new(IUP_TYPE_TABLE_VIRTUAL_MODEL, NULL);
  model->ih = ih;
  return model;
}

/* ========================================================================= */
/* GTK-specific data structure                                              */
/* ========================================================================= */

typedef struct _Igtk4TableData
{
  GtkWidget* scrolled_win;
  GtkWidget* column_view;
  GListModel* model;  /* IupTableVirtualModel or GListStore */
  GtkSelectionModel* selection_model;
  int is_virtual;
  int num_columns;  /* Number of GtkColumnViewColumn objects created */
  int current_row;  /* Current focused row (1-based, 0=none) */
  int current_col;  /* Current focused column (1-based, 0=none) */
} Igtk4TableData;

#define IGTK4_TABLE_DATA(ih) ((Igtk4TableData*)(ih->data->native_data))

/* ========================================================================= */
/* Cell Factory Data                                                         */
/* ========================================================================= */

typedef struct _IupCellFactoryData
{
  Ihandle* ih;
  gint col_index;  /* 0-based column index */
} IupCellFactoryData;

/* ========================================================================= */
/* Cell Factory Callbacks                                                    */
/* ========================================================================= */

/* Track which GtkEditableLabel is currently being edited */
static GtkWidget* currently_editing_label = NULL;

/* Key handler to track Escape key for cancelling edits */
static gboolean on_text_key_pressed(GtkEventControllerKey* controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data)
{
  GtkWidget* label = GTK_WIDGET(user_data);

  if (keyval == GDK_KEY_Escape)
    g_object_set_data(G_OBJECT(label), "edit_cancelled", GINT_TO_POINTER(1));

  (void)controller;
  (void)keycode;
  (void)state;
  return FALSE;
}


static void on_editing_notify(GObject* object, GParamSpec* pspec, gpointer user_data)
{
  IupCellFactoryData* data = (IupCellFactoryData*)user_data;
  Ihandle* ih = data->ih;
  Igtk4TableData* gtk_data = IGTK4_TABLE_DATA(ih);
  gint col = data->col_index;

  GtkEditableLabel* label = GTK_EDITABLE_LABEL(object);
  gboolean editing = gtk_editable_label_get_editing(label);

  if (editing)
  {
    /* Editing started */
    GtkListItem* list_item = g_object_get_data(object, "list_item");
    if (!list_item)
      return;

    IupTableRow* row = IUP_TABLE_ROW(gtk_list_item_get_item(list_item));
    if (!row)
      return;

    gint lin = row->row_index;

    /* Update selection to match the row being edited */
    if (GTK_IS_SINGLE_SELECTION(gtk_data->selection_model))
    {
      guint current_pos = gtk_single_selection_get_selected(GTK_SINGLE_SELECTION(gtk_data->selection_model));
      if (current_pos != (guint)(lin - 1))
      {
        gtk_single_selection_set_selected(GTK_SINGLE_SELECTION(gtk_data->selection_model), lin - 1);
        gtk_data->current_row = lin;
      }
    }

    /* Remember this label as currently editing */
    currently_editing_label = GTK_WIDGET(object);

    /* Store original value for comparison when editing ends */
    char* original_value = iupdrvTableGetCellValue(ih, lin, col + 1);
    g_object_set_data_full(object, "original_value", original_value ? g_strdup(original_value) : NULL, g_free);

    /* Clear edit_cancelled flag - default is accepted unless Escape is pressed */
    g_object_set_data(object, "edit_cancelled", GINT_TO_POINTER(0));

    /* Disable "editing.start" action during editing to allow Enter key to end editing */
    gtk_widget_action_set_enabled(GTK_WIDGET(object), "editing.start", FALSE);

    /* Connect to GtkText to monitor Escape key */
    GtkEditable* editable = GTK_EDITABLE(object);
    GtkWidget* delegate = GTK_WIDGET(gtk_editable_get_delegate(editable));
    if (delegate && GTK_IS_TEXT(delegate))
    {
      GtkEventController* key_controller = gtk_event_controller_key_new();
      g_signal_connect(key_controller, "key-pressed", G_CALLBACK(on_text_key_pressed), object);
      gtk_widget_add_controller(delegate, key_controller);
      g_object_set_data(object, "key_controller", key_controller);
    }

    /* Call EDITBEGIN_CB - allow application to block editing */
    IFnii editbegin_cb = (IFnii)IupGetCallback(ih, "EDITBEGIN_CB");
    if (editbegin_cb)
    {
      int ret = editbegin_cb(ih, lin, col + 1);
      if (ret == IUP_IGNORE)
      {
        gtk_editable_label_stop_editing(label, FALSE);
        return;
      }
    }
  }
  else
  {
    /* Editing ended */
    GtkListItem* list_item = g_object_get_data(object, "list_item");
    if (!list_item)
      return;

    IupTableRow* row = IUP_TABLE_ROW(gtk_list_item_get_item(list_item));
    if (!row)
      return;

    gint lin = row->row_index;
    const char* current_text = gtk_editable_get_text(GTK_EDITABLE(label));

    /* Get original value that was stored when editing started */
    const char* old_text = g_object_get_data(object, "original_value");

    /* Check if Escape was pressed - default is accepted (1) unless cancelled (0) */
    int edit_cancelled = GPOINTER_TO_INT(g_object_get_data(object, "edit_cancelled"));
    int apply = edit_cancelled ? 0 : 1;

    /* Remove key controller */
    GtkEventController* key_controller = g_object_get_data(object, "key_controller");
    if (key_controller)
    {
      GtkEditable* editable = GTK_EDITABLE(object);
      GtkWidget* delegate = GTK_WIDGET(gtk_editable_get_delegate(editable));
      if (delegate)
        gtk_widget_remove_controller(delegate, key_controller);
      g_object_set_data(object, "key_controller", NULL);
    }

    /* Call EDITEND_CB */
    IFniisi editend_cb = (IFniisi)IupGetCallback(ih, "EDITEND_CB");
    if (editend_cb)
      editend_cb(ih, lin, col + 1, (char*)current_text, apply);

    /* Call VALUECHANGED_CB only if edit was accepted AND text actually changed */
    if (apply)
    {
      int text_changed = 0;
      if (!old_text && current_text && *current_text)
        text_changed = 1;  /* NULL -> non-empty */
      else if (old_text && !current_text)
        text_changed = 1;  /* non-empty -> NULL */
      else if (old_text && current_text && strcmp(old_text, current_text) != 0)
        text_changed = 1;  /* different text */

      if (text_changed)
      {
        IFnii valuechanged_cb = (IFnii)IupGetCallback(ih, "VALUECHANGED_CB");
        if (valuechanged_cb)
          valuechanged_cb(ih, lin, col + 1);
      }
    }

    /* Re-enable "editing.start" action after editing ends */
    gtk_widget_action_set_enabled(GTK_WIDGET(object), "editing.start", TRUE);

    /* Clean up stored data */
    g_object_set_data(object, "edit_cancelled", NULL);
    g_object_set_data(object, "original_value", NULL);

    /* Clear currently_editing_label if it's this widget */
    if (currently_editing_label == GTK_WIDGET(object))
      currently_editing_label = NULL;
  }

  (void)pspec;
}

static void on_edit_done(GtkEditable* editable, gpointer user_data)
{
  IupCellFactoryData* data = (IupCellFactoryData*)user_data;
  Ihandle* ih = data->ih;
  gint col = data->col_index;

  GtkListItem* list_item = g_object_get_data(G_OBJECT(editable), "list_item");
  if (!list_item)
    return;

  IupTableRow* row = IUP_TABLE_ROW(gtk_list_item_get_item(list_item));
  if (!row)
    return;

  gint lin = row->row_index;
  const char* new_text = gtk_editable_get_text(editable);

  /* Note: EDITEND_CB is called from on_editing_notify when editing ends, not here */
  /* This "changed" signal is for EDITION_CB callback */

  IFniis edition_cb = (IFniis)IupGetCallback(ih, "EDITION_CB");
  if (edition_cb)
  {
    int ret = edition_cb(ih, lin, col + 1, (char*)new_text);
    if (ret == IUP_IGNORE)
      return;
  }

  iup_table_row_set_value(row, col, new_text);
}

static void on_label_click_pressed(GtkGestureClick* gesture, int n_press, double x, double y, gpointer user_data)
{
  GtkWidget* widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture));

  /* If another cell is currently being edited, stop it and block this click */
  if (currently_editing_label && currently_editing_label != widget)
  {
    if (GTK_IS_EDITABLE_LABEL(currently_editing_label))
    {
      gboolean still_editing = gtk_editable_label_get_editing(GTK_EDITABLE_LABEL(currently_editing_label));
      if (still_editing)
        gtk_editable_label_stop_editing(GTK_EDITABLE_LABEL(currently_editing_label), TRUE);
    }

    /* Block the click - don't let it start editing on the new cell */
    gtk_gesture_set_state(GTK_GESTURE(gesture), GTK_EVENT_SEQUENCE_CLAIMED);
  }

  (void)n_press;
  (void)x;
  (void)y;
  (void)user_data;
}

static void cell_factory_setup(GtkSignalListItemFactory* factory, GtkListItem* list_item, gpointer user_data)
{
  IupCellFactoryData* data = (IupCellFactoryData*)user_data;
  Ihandle* ih = data->ih;
  gint col = data->col_index;

  char* editable_str = iupAttribGetId(ih, "EDITABLE", col + 1);
  if (!editable_str)
    editable_str = iupAttribGet(ih, "EDITABLE");
  int is_editable = iupStrBoolean(editable_str);

  GtkWidget* box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_hexpand(box, TRUE);

  gtk_widget_add_css_class(box, "iup-table-cell-box");

  GtkWidget* widget;
  if (is_editable)
  {
    widget = gtk_editable_label_new("");
    gtk_editable_set_editable(GTK_EDITABLE(widget), TRUE);

    /* Add click handler to prevent starting edit if another cell is already editing */
    GtkGesture* click_gesture = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click_gesture), 0);  /* Any button */
    gtk_event_controller_set_propagation_phase(GTK_EVENT_CONTROLLER(click_gesture), GTK_PHASE_CAPTURE);
    g_signal_connect(click_gesture, "pressed", G_CALLBACK(on_label_click_pressed), NULL);
    gtk_widget_add_controller(widget, GTK_EVENT_CONTROLLER(click_gesture));

    g_signal_connect(widget, "notify::editing", G_CALLBACK(on_editing_notify), data);
    g_signal_connect(widget, "changed", G_CALLBACK(on_edit_done), data);
  }
  else
  {
    widget = gtk_label_new(NULL);
  }

  char align_name[50];
  sprintf(align_name, "ALIGNMENT%d", col + 1);
  char* alignment = iupAttribGet(ih, align_name);
  if (!alignment)
    alignment = iupAttribGetId(ih, "ALIGNMENT", col + 1);

  if (GTK_IS_LABEL(widget))
  {
    if (alignment && iupStrEqualNoCase(alignment, "ACENTER"))
      gtk_label_set_xalign(GTK_LABEL(widget), 0.5);
    else if (alignment && iupStrEqualNoCase(alignment, "ARIGHT"))
      gtk_label_set_xalign(GTK_LABEL(widget), 1.0);
    else
      gtk_label_set_xalign(GTK_LABEL(widget), 0.0);
  }

  gtk_widget_set_hexpand(widget, TRUE);
  gtk_box_append(GTK_BOX(box), widget);

  gtk_list_item_set_child(list_item, box);
}

static void on_row_update(IupTableRow* row, GParamSpec* pspec, gpointer user_data)
{
  GtkListItem* list_item = GTK_LIST_ITEM(user_data);
  GtkWidget* box = gtk_list_item_get_child(list_item);
  if (!box)
    return;

  IupCellFactoryData* data = g_object_get_data(G_OBJECT(box), "iup-factory-data");
  if (!data)
    return;

  gint col = data->col_index;
  if (col >= row->num_cols)
    return;

  GtkWidget* widget = gtk_widget_get_first_child(box);
  if (!widget)
    return;

  if (GTK_IS_EDITABLE_LABEL(widget))
  {
    /* Don't update text if currently editing - it would reset the cursor position! */
    gboolean editing = gtk_editable_label_get_editing(GTK_EDITABLE_LABEL(widget));
    if (!editing)
    {
      g_signal_handlers_block_by_func(widget, G_CALLBACK(on_edit_done), data);
      gtk_editable_set_text(GTK_EDITABLE(widget), row->values[col]);
      g_signal_handlers_unblock_by_func(widget, G_CALLBACK(on_edit_done), data);
    }
  }
  else if (GTK_IS_LABEL(widget))
  {
    gtk_label_set_text(GTK_LABEL(widget), row->values[col]);
  }
}

static void cell_factory_bind(GtkSignalListItemFactory* factory, GtkListItem* list_item, gpointer user_data)
{
  IupCellFactoryData* data = (IupCellFactoryData*)user_data;
  Ihandle* ih = data->ih;
  gint col = data->col_index;

  GtkWidget* box = gtk_list_item_get_child(list_item);
  GtkWidget* widget = gtk_widget_get_first_child(box);
  IupTableRow* row = IUP_TABLE_ROW(gtk_list_item_get_item(list_item));

  if (!row || col >= row->num_cols)
  {
    return;
  }

  g_object_set_data(G_OBJECT(box), "iup-factory-data", data);

  gint lin = row->row_index;
  gboolean is_selected = gtk_list_item_get_selected(list_item);

  if (GTK_IS_EDITABLE_LABEL(widget))
  {
    g_object_set_data(G_OBJECT(widget), "list_item", list_item);

    g_signal_handlers_block_by_func(widget, G_CALLBACK(on_edit_done), data);
    gtk_editable_set_text(GTK_EDITABLE(widget), row->values[col]);
    g_signal_handlers_unblock_by_func(widget, G_CALLBACK(on_edit_done), data);
  }
  else
  {
    gtk_label_set_text(GTK_LABEL(widget), row->values[col]);
  }

  gulong handler_id = g_signal_connect(row, "notify::update", G_CALLBACK(on_row_update), list_item);
  g_object_set_data(G_OBJECT(list_item), "update-handler", GUINT_TO_POINTER(handler_id));
  g_object_set_data(G_OBJECT(list_item), "bound-row", row);


  PangoAttrList* attr_list = NULL;

  char* fgcolor = iupAttribGetId2(ih, "FGCOLOR", lin, col + 1);
  if (!fgcolor)
    fgcolor = iupAttribGetId2(ih, "FGCOLOR", 0, col + 1);
  if (!fgcolor)
    fgcolor = iupAttribGetId2(ih, "FGCOLOR", lin, 0);

  if (fgcolor && *fgcolor)
  {
    GdkRGBA color;
    if (gdk_rgba_parse(&color, fgcolor))
    {
      if (!attr_list)
        attr_list = pango_attr_list_new();

      PangoAttribute* attr = pango_attr_foreground_new(
        (guint16)(color.red * 65535),
        (guint16)(color.green * 65535),
        (guint16)(color.blue * 65535));
      pango_attr_list_insert(attr_list, attr);
    }
  }

  char* bgcolor = NULL;
  if (!is_selected)
  {
    bgcolor = iupAttribGetId2(ih, "BGCOLOR", lin, col + 1);
    if (!bgcolor)
      bgcolor = iupAttribGetId2(ih, "BGCOLOR", 0, col + 1);
    if (!bgcolor)
      bgcolor = iupAttribGetId2(ih, "BGCOLOR", lin, 0);

    if (!bgcolor)
    {
      char* alternate_color = iupAttribGet(ih, "ALTERNATECOLOR");
      if (iupStrBoolean(alternate_color))
      {
        if (lin % 2 == 0)
          bgcolor = iupAttribGet(ih, "EVENROWCOLOR");
        else
          bgcolor = iupAttribGet(ih, "ODDROWCOLOR");
      }
    }
  }

  if (bgcolor && *bgcolor)
  {
    /* Generate a CSS class name from the color value */
    char class_name[64];
    snprintf(class_name, sizeof(class_name), "iup-cell-bg-%s", bgcolor);
    /* Replace non-alphanumeric characters that aren't valid in CSS class names */
    for (char* p = class_name; *p; p++)
    {
      if (!g_ascii_isalnum(*p) && *p != '-')
        *p = '-';
    }

    /* Remove old color class if any */
    const char* old_class = g_object_get_data(G_OBJECT(box), "iup-bgcolor-class");
    if (old_class)
    {
      gtk_widget_remove_css_class(box, old_class);
    }

    /* Add static CSS rule for this color (only done once per unique color) */
    /* Use row:not(:selected) so selection highlight shows through */
    char css_rules[128];
    char selector[96];
    snprintf(css_rules, sizeof(css_rules), "background-color: %s;", bgcolor);
    snprintf(selector, sizeof(selector), "row:not(:selected) .%s", class_name);
    iupgtk4CssAddStaticRule(selector, css_rules);

    /* Apply the class to this cell */
    gtk_widget_add_css_class(box, class_name);
    g_object_set_data_full(G_OBJECT(box), "iup-bgcolor-class", g_strdup(class_name), g_free);
  }
  else
  {
    /* Remove old color class if any */
    const char* old_class = g_object_get_data(G_OBJECT(box), "iup-bgcolor-class");
    if (old_class)
    {
      gtk_widget_remove_css_class(box, old_class);
      g_object_set_data(G_OBJECT(box), "iup-bgcolor-class", NULL);
    }
  }

  char* font = iupAttribGetId2(ih, "FONT", lin, col + 1);
  if (!font)
    font = iupAttribGetId2(ih, "FONT", 0, col + 1);
  if (!font)
    font = iupAttribGetId2(ih, "FONT", lin, 0);

  if (font && *font)
  {
    PangoFontDescription* fontdesc = iupgtk4GetPangoFontDesc(font);
    if (fontdesc)
    {
      if (!attr_list)
        attr_list = pango_attr_list_new();

      PangoAttribute* attr = pango_attr_font_desc_new(fontdesc);
      pango_attr_list_insert(attr_list, attr);
    }
  }

  if (GTK_IS_LABEL(widget))
  {
    if (attr_list)
    {
      gtk_label_set_attributes(GTK_LABEL(widget), attr_list);
      pango_attr_list_unref(attr_list);
    }
    else
    {
      gtk_label_set_attributes(GTK_LABEL(widget), NULL);
    }
  }
  else if (attr_list)
  {
    pango_attr_list_unref(attr_list);
  }
}

static void cell_factory_unbind(GtkSignalListItemFactory* factory, GtkListItem* list_item, gpointer user_data)
{
  gulong handler_id = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(list_item), "update-handler"));
  if (handler_id)
  {
    IupTableRow* row = g_object_get_data(G_OBJECT(list_item), "bound-row");
    if (row)
    {
      g_signal_handler_disconnect(row, handler_id);
      g_object_set_data(G_OBJECT(list_item), "update-handler", NULL);
      g_object_set_data(G_OBJECT(list_item), "bound-row", NULL);
    }
  }
}

static void cell_factory_teardown(GtkSignalListItemFactory* factory, GtkListItem* list_item, gpointer user_data)
{
}

/* ========================================================================= */
/* Signal Callbacks                                                          */
/* ========================================================================= */

static void on_selection_changed(GtkSelectionModel* selection, guint position, guint n_items, Ihandle* ih)
{
  Igtk4TableData* gtk_data = IGTK4_TABLE_DATA(ih);

  if (GTK_IS_SINGLE_SELECTION(selection))
  {
    guint pos = gtk_single_selection_get_selected(GTK_SINGLE_SELECTION(selection));
    if (pos != GTK_INVALID_LIST_POSITION)
    {
      gtk_data->current_row = pos + 1;
      if (gtk_data->current_col == 0)
        gtk_data->current_col = 1;
    }
    else
    {
      gtk_data->current_row = 0;
      gtk_data->current_col = 0;
    }
  }
  else if (GTK_IS_MULTI_SELECTION(selection))
  {
    GtkBitset* bitset = gtk_selection_model_get_selection(selection);
    if (bitset && gtk_bitset_get_size(bitset) > 0)
    {
      guint first = gtk_bitset_get_nth(bitset, 0);
      gtk_data->current_row = first + 1;
      if (gtk_data->current_col == 0)
        gtk_data->current_col = 1;
    }
    else
    {
      gtk_data->current_row = 0;
      gtk_data->current_col = 0;
    }
  }

  IFnii cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
  if (cb)
  {
    cb(ih, gtk_data->current_row, gtk_data->current_col);
  }
}

static void on_click(GtkGestureClick* gesture, int n_press, double x, double y, Ihandle* ih)
{
  Igtk4TableData* gtk_data = IGTK4_TABLE_DATA(ih);
  guint button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));

  if (GTK_IS_SINGLE_SELECTION(gtk_data->selection_model))
  {
    guint pos = gtk_single_selection_get_selected(GTK_SINGLE_SELECTION(gtk_data->selection_model));
    if (pos != GTK_INVALID_LIST_POSITION)
    {
      int old_row = gtk_data->current_row;
      int old_col = gtk_data->current_col;

      gtk_data->current_row = pos + 1;

      GListModel* columns = gtk_column_view_get_columns(GTK_COLUMN_VIEW(gtk_data->column_view));
      guint n_columns = g_list_model_get_n_items(columns);
      double column_x = 0;

      for (guint i = 0; i < ih->data->num_col; i++)
      {
        GtkColumnViewColumn* column = g_list_model_get_item(columns, i);
        if (column)
        {
          int column_width = gtk_column_view_column_get_fixed_width(column);
          if (column_width <= 0)
            column_width = 100;

          if (x >= column_x && x < column_x + column_width)
          {
            gtk_data->current_col = i + 1;
            g_object_unref(column);
            break;
          }

          column_x += column_width;
          g_object_unref(column);
        }
      }

      IFniis click_cb = (IFniis)IupGetCallback(ih, "CLICK_CB");
      if (click_cb)
      {
        char status[IUPKEY_STATUS_SIZE] = "";
        GdkModifierType state = gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(gesture));
        iupgtk4ButtonKeySetStatus(state, button, status, 0);

        click_cb(ih, gtk_data->current_row, gtk_data->current_col, status);
      }
    }
  }
}

static gboolean on_key_pressed(GtkEventControllerKey* controller, guint keyval, guint keycode, GdkModifierType state, Ihandle* ih)
{
  Igtk4TableData* gtk_data = IGTK4_TABLE_DATA(ih);

  if (keyval == GDK_KEY_Tab || keyval == GDK_KEY_ISO_Left_Tab)
  {
    int old_col = gtk_data->current_col;
    int old_row = gtk_data->current_row;

    if (state & GDK_SHIFT_MASK)
    {
      gtk_data->current_col--;
      if (gtk_data->current_col < 1)
      {
        gtk_data->current_col = ih->data->num_col;
        if (gtk_data->current_row > 1)
          gtk_data->current_row--;
        else
          gtk_data->current_row = ih->data->num_lin;
      }
    }
    else
    {
      gtk_data->current_col++;
      if (gtk_data->current_col > ih->data->num_col)
      {
        gtk_data->current_col = 1;
        if (gtk_data->current_row < ih->data->num_lin)
          gtk_data->current_row++;
        else
          gtk_data->current_row = 1;
      }
    }

    if (gtk_data->current_row != old_row && GTK_IS_SINGLE_SELECTION(gtk_data->selection_model))
      gtk_single_selection_set_selected(GTK_SINGLE_SELECTION(gtk_data->selection_model), gtk_data->current_row - 1);

    IFnii cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
    if (cb)
      cb(ih, gtk_data->current_row, gtk_data->current_col);

    return TRUE;
  }
  else if (keyval == GDK_KEY_Left || keyval == GDK_KEY_Right)
  {
    /* Check if currently editing, if so, let arrow keys move cursor in text */
    GtkWidget* current = gtk_data->column_view;
    while (current)
    {
      if (GTK_IS_EDITABLE_LABEL(current))
      {
        gboolean editing = gtk_editable_label_get_editing(GTK_EDITABLE_LABEL(current));
        if (editing)
          return FALSE;
        break;
      }
      current = gtk_widget_get_focus_child(current);
    }

    /* Not editing, navigate between cells */
    if (keyval == GDK_KEY_Left && gtk_data->current_col > 1)
    {
      gtk_data->current_col--;

      IFnii cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
      if (cb)
        cb(ih, gtk_data->current_row, gtk_data->current_col);

      return FALSE;
    }
    else if (keyval == GDK_KEY_Right && gtk_data->current_col < ih->data->num_col)
    {
      gtk_data->current_col++;

      IFnii cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
      if (cb)
        cb(ih, gtk_data->current_row, gtk_data->current_col);

      return FALSE;
    }
  }
  else if (keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter)
  {
    /* First check if we're already editing, if so, let Enter end the edit */
    GtkWidget* current = gtk_data->column_view;
    while (current)
    {
      if (GTK_IS_EDITABLE_LABEL(current))
      {
        gboolean editing = gtk_editable_label_get_editing(GTK_EDITABLE_LABEL(current));
        if (editing)
          return FALSE;
        break;
      }
      current = gtk_widget_get_focus_child(current);
    }

    if (GTK_IS_SINGLE_SELECTION(gtk_data->selection_model))
    {
      guint pos = gtk_single_selection_get_selected(GTK_SINGLE_SELECTION(gtk_data->selection_model));
      if (pos != GTK_INVALID_LIST_POSITION)
      {
        gtk_data->current_row = pos + 1;
      }
    }

    if (gtk_data->current_col == 0)
      gtk_data->current_col = 1;

    int row = gtk_data->current_row;
    int col = gtk_data->current_col;

    char* editable_str = iupAttribGetId(ih, "EDITABLE", col);
    if (!editable_str)
      editable_str = iupAttribGet(ih, "EDITABLE");

    if (iupStrBoolean(editable_str))
    {
      /* First, ensure the correct row is selected */
      if (GTK_IS_SINGLE_SELECTION(gtk_data->selection_model))
      {
        if (row > 0 && row <= ih->data->num_lin)
        {
          guint current_pos = gtk_single_selection_get_selected(GTK_SINGLE_SELECTION(gtk_data->selection_model));
          if (current_pos != (guint)(row - 1))
          {
            gtk_single_selection_set_selected(GTK_SINGLE_SELECTION(gtk_data->selection_model), row - 1);

            /* Give GTK a moment to update focus after selection change */
            while (g_main_context_pending(NULL))
              g_main_context_iteration(NULL, FALSE);
          }
        }
      }

      /* Find the GtkEditableLabel for the target column by searching the row */
      GtkWidget* editable_label = NULL;

      /* Walk down focus chain to the row widget */
      GtkWidget* row_widget = NULL;
      GtkWidget* walk = gtk_data->column_view;
      while (walk)
      {
        const char* type_name = G_OBJECT_TYPE_NAME(walk);
        if (g_strcmp0(type_name, "GtkColumnViewRowWidget") == 0)
        {
          row_widget = walk;
          break;
        }
        walk = gtk_widget_get_focus_child(walk);
      }

      if (row_widget)
      {
        /* Iterate through all cell widgets in the row to find the right column */
        GtkWidget* cell = gtk_widget_get_first_child(row_widget);
        while (cell && !editable_label)
        {
          /* Each cell contains a GtkBox which contains the GtkEditableLabel */
          GtkWidget* box = gtk_widget_get_first_child(cell);
          if (box)
          {
            GtkWidget* label = gtk_widget_get_first_child(box);
            if (label && GTK_IS_EDITABLE_LABEL(label))
            {
              /* Check if this is the correct column */
              IupCellFactoryData* factory_data = g_object_get_data(G_OBJECT(box), "iup-factory-data");
              if (factory_data && factory_data->col_index + 1 == col)
              {
                editable_label = label;
                break;
              }
            }
          }
          cell = gtk_widget_get_next_sibling(cell);
        }
      }

      if (editable_label)
      {
        gtk_editable_label_start_editing(GTK_EDITABLE_LABEL(editable_label));
        return TRUE;
      }
      else
        return FALSE;
    }

    return TRUE;
  }
  else if ((keyval == GDK_KEY_c || keyval == GDK_KEY_C) && (state & GDK_CONTROL_MASK))
  {
    if (gtk_data->current_row > 0 && gtk_data->current_col > 0)
    {
      char* value = iupdrvTableGetCellValue(ih, gtk_data->current_row, gtk_data->current_col);
      if (value)
      {
        IupSetGlobal("CLIPBOARD", value);
      }
    }
    return TRUE;
  }
  else if ((keyval == GDK_KEY_v || keyval == GDK_KEY_V) && (state & GDK_CONTROL_MASK))
  {
    const char* text = IupGetGlobal("CLIPBOARD");
    if (gtk_data->current_row > 0 && gtk_data->current_col > 0)
    {
      if (text && *text)
      {
        iupdrvTableSetCellValue(ih, gtk_data->current_row, gtk_data->current_col, text);
        gtk_widget_queue_draw(gtk_data->column_view);

        IFnii cb = (IFnii)IupGetCallback(ih, "VALUECHANGED_CB");
        if (cb)
          cb(ih, gtk_data->current_row, gtk_data->current_col);
      }
    }
    return TRUE;
  }

  return iupgtk4KeyPressEvent(controller, keyval, keycode, state, ih);
}

/* ========================================================================= */
/* Sorting                                                                   */
/* ========================================================================= */

typedef struct _IupSortData
{
  Ihandle* ih;
  gint col;
} IupSortData;

static int table_sort_func(gconstpointer a, gconstpointer b, gpointer user_data)
{
  IupSortData* sort_data = (IupSortData*)user_data;

  if (!sort_data || !sort_data->ih)
    return GTK_ORDERING_EQUAL;

  if (!sort_data->ih->data->sortable)
    return GTK_ORDERING_EQUAL;

  Igtk4TableData* gtk_data = IGTK4_TABLE_DATA(sort_data->ih);
  if (!gtk_data)
    return GTK_ORDERING_EQUAL;

  /* In virtual mode, don't perform automatic sorting */
  if (gtk_data->is_virtual)
    return GTK_ORDERING_EQUAL;

  /* Normal mode - perform automatic sorting */
  IupTableRow* row_a = IUP_TABLE_ROW((gpointer)a);
  IupTableRow* row_b = IUP_TABLE_ROW((gpointer)b);
  gint col = sort_data->col;

  if (!row_a || !row_b || col >= row_a->num_cols || col >= row_b->num_cols)
    return GTK_ORDERING_EQUAL;

  int result = g_strcmp0(row_a->values[col], row_b->values[col]);
  if (result < 0)
    return GTK_ORDERING_SMALLER;
  else if (result > 0)
    return GTK_ORDERING_LARGER;
  else
    return GTK_ORDERING_EQUAL;
}

/* ========================================================================= */
/* GTK4 Signal Callbacks                                                     */
/* ========================================================================= */

static void gtk4TableSorterChanged(GtkSorter* sorter, GtkSorterChange change, gpointer user_data)
{
  Ihandle* ih = (Ihandle*)user_data;
  (void)change;  /* Unused */

  if (!ih || !ih->data->sortable)
    return;

  /* Called in both virtual and non-virtual mode when column header is clicked */
  Igtk4TableData* gtk_data = IGTK4_TABLE_DATA(ih);
  if (!gtk_data || !gtk_data->column_view)
    return;

  /* Get the primary sort column (the one that was just clicked) */
  GtkColumnViewSorter* view_sorter = GTK_COLUMN_VIEW_SORTER(sorter);
  GtkColumnViewColumn* primary_column = gtk_column_view_sorter_get_primary_sort_column(view_sorter);

  if (!primary_column)
    return;

  /* Find the index of this column */
  GListModel* columns = gtk_column_view_get_columns(GTK_COLUMN_VIEW(gtk_data->column_view));
  guint n_columns = g_list_model_get_n_items(columns);

  for (guint i = 0; i < n_columns; i++)
  {
    GtkColumnViewColumn* column = g_list_model_get_item(columns, i);
    if (column == primary_column)
    {
      IFni sort_cb = (IFni)IupGetCallback(ih, "SORT_CB");
      if (sort_cb)
      {
        sort_cb(ih, (int)i + 1);  /* 1-based column index */
      }
      g_object_unref(column);
      break;
    }
    if (column)
      g_object_unref(column);
  }
}

/* ========================================================================= */
/* Driver Functions - Widget Lifecycle                                      */
/* ========================================================================= */

static void gtk4TableLayoutUpdateMethod(Ihandle* ih)
{
  Igtk4TableData* gtk_data = IGTK4_TABLE_DATA(ih);
  GtkWidget* widget = gtk_data->scrolled_win;
  int width = ih->currentwidth;
  int height = ih->currentheight;

  /* If VISIBLELINES is set, clamp height to target */
  int target_height = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "iup-table-target-height"));
  if (target_height > 0)
  {
    if (height > target_height)
      height = target_height;
  }

  /* If VISIBLECOLUMNS is set, clamp width to show exactly N columns */
  int visible_columns = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "iup-table-visible-columns"));
  if (visible_columns > 0 && gtk_data->column_view)
  {
    int c, cols_width = 0;
    int num_cols = visible_columns;
    if (num_cols > ih->data->num_col)
      num_cols = ih->data->num_col;

    GListModel* columns = gtk_column_view_get_columns(GTK_COLUMN_VIEW(gtk_data->column_view));
    guint n_columns = g_list_model_get_n_items(columns);

    for (c = 0; c < num_cols && c < (int)n_columns; c++)
    {
      GtkColumnViewColumn* column = g_list_model_get_item(columns, c);
      if (column)
      {
        int col_width = gtk_column_view_column_get_fixed_width(column);
        if (col_width <= 0)
          col_width = 80;  /* fallback to default */
        cols_width += col_width;
        g_object_unref(column);
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

  /* Get the parent container and position the widget */
  GtkWidget* parent = gtk_widget_get_parent(widget);
  if (parent)
    iupgtk4NativeContainerSetBounds(parent, widget, ih->x, ih->y, width, height);
}

static int gtk4TableMapMethod(Ihandle* ih)
{
  Igtk4TableData* gtk_data = (Igtk4TableData*)malloc(sizeof(Igtk4TableData));
  memset(gtk_data, 0, sizeof(Igtk4TableData));
  ih->data->native_data = gtk_data;

  gtk_data->is_virtual = iupAttribGetBoolean(ih, "VIRTUALMODE");

  if (gtk_data->is_virtual)
  {
    IupTableVirtualModel* vmodel = iup_table_virtual_model_new(ih);
    gtk_data->model = G_LIST_MODEL(vmodel);
  }
  else
  {
    GListStore* store = g_list_store_new(IUP_TYPE_TABLE_ROW);
    gtk_data->model = G_LIST_MODEL(store);
  }

  gtk_data->column_view = gtk_column_view_new(NULL);

  for (int col = 0; col < ih->data->num_col; col++)
  {
    IupCellFactoryData* factory_data = g_new0(IupCellFactoryData, 1);
    factory_data->ih = ih;
    factory_data->col_index = col;

    GtkListItemFactory* factory = gtk_signal_list_item_factory_new();
    g_signal_connect(factory, "setup", G_CALLBACK(cell_factory_setup), factory_data);
    g_signal_connect(factory, "bind", G_CALLBACK(cell_factory_bind), factory_data);
    g_signal_connect(factory, "unbind", G_CALLBACK(cell_factory_unbind), factory_data);
    g_signal_connect(factory, "teardown", G_CALLBACK(cell_factory_teardown), factory_data);

    GtkColumnViewColumn* column = gtk_column_view_column_new("", factory);
    gtk_column_view_column_set_resizable(column, ih->data->user_resize);

    /* Check if width was set before mapping */
    char name[50];
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
        /* Explicit width set, use fixed width, don't stretch */
        gtk_column_view_column_set_fixed_width(column, col_width);
        gtk_column_view_column_set_expand(column, FALSE);
      }
      else if (ih->data->stretch_last)
      {
        /* No explicit width and stretching enabled, expand to fill */
        gtk_column_view_column_set_expand(column, TRUE);
      }
      else
      {
        /* No explicit width and stretching disabled, fit to content */
        gtk_column_view_column_set_expand(column, FALSE);
      }
    }
    else
    {
      /* Non-last columns */
      if (has_explicit_width)
      {
        gtk_column_view_column_set_fixed_width(column, col_width);
      }
      gtk_column_view_column_set_expand(column, FALSE);
    }

    if (ih->data->sortable)
    {
      if (gtk_data->is_virtual)
      {
        /* Virtual mode: Use NULL sorter (get_order returns NONE) */
        /* This makes header clickable but doesn't fetch items */
        GtkCustomSorter* sorter = gtk_custom_sorter_new(NULL, NULL, NULL);
        g_object_set_data_full(G_OBJECT(column), "iup-sorter", g_object_ref(sorter), g_object_unref);

        gtk_column_view_column_set_sorter(column, GTK_SORTER(sorter));

        g_object_unref(sorter);
      }
      else
      {
        /* Non-virtual mode: Use real sort function */
        IupSortData* sort_data = g_new0(IupSortData, 1);
        sort_data->ih = ih;
        sort_data->col = col;

        GtkCustomSorter* sorter = gtk_custom_sorter_new(table_sort_func, sort_data, g_free);
        g_object_set_data_full(G_OBJECT(column), "iup-sorter", g_object_ref(sorter), g_object_unref);

        gtk_column_view_column_set_sorter(column, GTK_SORTER(sorter));

        g_object_unref(sorter);
      }
    }

    gtk_column_view_append_column(GTK_COLUMN_VIEW(gtk_data->column_view), column);
    gtk_data->num_columns++;
  }

  /* Check if we need a dummy expanding column to prevent GTK4 scrollbar errors.
   * GTK4 requires at least one expanding column or scrollbar calculations fail.
   */
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
    int col_width = 0;
    last_col_has_width = (width_str && iupStrToInt(width_str, &col_width) && col_width > 0);
  }

  if (!ih->data->stretch_last || last_col_has_width)
  {
    GtkColumnViewColumn* dummy_column = gtk_column_view_column_new("", NULL);
    gtk_column_view_column_set_expand(dummy_column, TRUE);
    gtk_column_view_column_set_resizable(dummy_column, FALSE);
    gtk_column_view_append_column(GTK_COLUMN_VIEW(gtk_data->column_view), dummy_column);
  }

  GListModel* model_for_selection;

  if (gtk_data->is_virtual)
  {
    /* Virtual mode - use model directly (no GtkSortListModel) */
    /* Sorters return EQUAL so no actual sorting happens */
    model_for_selection = gtk_data->model;

    /* Connect sorter changed signal for SORT_CB callback */
    if (ih->data->sortable)
    {
      GtkSorter* sorter = gtk_column_view_get_sorter(GTK_COLUMN_VIEW(gtk_data->column_view));
      g_signal_connect(G_OBJECT(sorter), "changed", G_CALLBACK(gtk4TableSorterChanged), ih);
    }
  }
  else
  {
    /* Normal mode - use GtkSortListModel for automatic sorting */
    GtkSorter* sorter = gtk_column_view_get_sorter(GTK_COLUMN_VIEW(gtk_data->column_view));
    GtkSortListModel* sort_model = gtk_sort_list_model_new(gtk_data->model, sorter);
    model_for_selection = G_LIST_MODEL(sort_model);

    /* Connect sorter changed signal for SORT_CB callback */
    if (ih->data->sortable)
    {
      g_signal_connect(G_OBJECT(sorter), "changed", G_CALLBACK(gtk4TableSorterChanged), ih);
    }
  }

  char* selmode = iupAttribGetStr(ih, "SELECTIONMODE");
  if (!selmode)
    selmode = "SINGLE";

  if (iupStrEqualNoCase(selmode, "NONE"))
  {
    gtk_data->selection_model = GTK_SELECTION_MODEL(gtk_no_selection_new(model_for_selection));
  }
  else if (iupStrEqualNoCase(selmode, "MULTIPLE"))
  {
    gtk_data->selection_model = GTK_SELECTION_MODEL(gtk_multi_selection_new(model_for_selection));
  }
  else
  {
    GtkSingleSelection* single_sel = gtk_single_selection_new(model_for_selection);
    gtk_single_selection_set_autoselect(single_sel, FALSE);
    gtk_single_selection_set_can_unselect(single_sel, TRUE);
    gtk_data->selection_model = GTK_SELECTION_MODEL(single_sel);
  }

  gtk_column_view_set_model(GTK_COLUMN_VIEW(gtk_data->column_view), gtk_data->selection_model);

  /* Clear initial selection after model is attached to view */
  if (GTK_IS_SINGLE_SELECTION(gtk_data->selection_model))
  {
    gtk_single_selection_set_selected(GTK_SINGLE_SELECTION(gtk_data->selection_model), GTK_INVALID_LIST_POSITION);
  }
  else if (GTK_IS_MULTI_SELECTION(gtk_data->selection_model))
  {
    gtk_selection_model_unselect_all(gtk_data->selection_model);
  }

  GtkCssProvider* global_provider = gtk_css_provider_new();
  gtk_css_provider_load_from_string(global_provider,
    ".iup-table-cell-box {\n"
    "  padding: 0;\n"
    "  margin: 0;\n"
    "}\n"
    ".iup-table-cell-box > label,\n"
    ".iup-table-cell-box > editablelabel {\n"
    "  padding: 6px;\n"
    "}\n"
    "columnview row > cell {\n"
    "  padding: 0;\n"
    "}\n");
  gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(global_provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
  g_object_unref(global_provider);


  gtk_data->scrolled_win = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(gtk_data->scrolled_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(gtk_data->scrolled_win), gtk_data->column_view);

  ih->handle = gtk_data->column_view;
  iupAttribSet(ih, "_IUP_EXTRAPARENT", (char*)gtk_data->scrolled_win);

  g_signal_connect(gtk_data->selection_model, "selection-changed", G_CALLBACK(on_selection_changed), ih);

  GtkGesture* click_gesture = gtk_gesture_click_new();
  gtk_widget_add_controller(gtk_data->column_view, GTK_EVENT_CONTROLLER(click_gesture));
  g_signal_connect(click_gesture, "pressed", G_CALLBACK(on_click), ih);

  GtkEventController* key_controller = gtk_event_controller_key_new();
  gtk_event_controller_set_propagation_phase(key_controller, GTK_PHASE_CAPTURE);
  gtk_widget_add_controller(gtk_data->column_view, key_controller);
  g_signal_connect(key_controller, "key-pressed", G_CALLBACK(on_key_pressed), ih);

  for (int col = 1; col <= ih->data->num_col; col++)
  {
    char name[50];
    sprintf(name, "TITLE%d", col);
    char* title = iupAttribGet(ih, name);
    if (title)
      iupdrvTableSetColTitle(ih, col, title);
  }

  if (!gtk_data->is_virtual && ih->data->num_lin > 0 && ih->data->num_col > 0)
  {
    GListStore* store = G_LIST_STORE(gtk_data->model);

    for (int lin = 1; lin <= ih->data->num_lin; lin++)
    {
      IupTableRow* row = iup_table_row_new(ih->data->num_col, lin);

      for (int col = 1; col <= ih->data->num_col; col++)
      {
        char* value = iupAttribGetId2(ih, "", lin, col);
        if (value && *value)
        {
          g_free(row->values[col - 1]);
          row->values[col - 1] = g_strdup(value);
        }
      }

      g_list_store_append(store, row);
      g_object_unref(row);
    }
  }

  iupgtk4AddToParent(ih);

  iupdrvTableSetShowGrid(ih, iupAttribGetBoolean(ih, "SHOWGRID"));

  /* Store target height for VISIBLELINES clamping in LayoutUpdate */
  int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
  if (visiblelines > 0)
  {
    int row_height = iupdrvTableGetRowHeight(ih);
    int header_height = iupdrvTableGetHeaderHeight(ih);
    int sb_size = iupdrvGetScrollbarSize();

    /* Only add horizontal scrollbar height if it will actually be visible */
    int visiblecolumns = iupAttribGetInt(ih, "VISIBLECOLUMNS");
    int need_horiz_sb = (visiblecolumns > 0 && ih->data->num_col > visiblecolumns);
    int horiz_sb_height = need_horiz_sb ? sb_size : 0;

    int content_height = header_height + (row_height * visiblelines) + horiz_sb_height + 2;
    g_object_set_data(G_OBJECT(gtk_data->scrolled_win), "iup-table-target-height", GINT_TO_POINTER(content_height));
  }

  /* Store VISIBLECOLUMNS for width clamping in LayoutUpdate */
  int visiblecolumns = iupAttribGetInt(ih, "VISIBLECOLUMNS");
  if (visiblecolumns > 0)
    g_object_set_data(G_OBJECT(gtk_data->scrolled_win), "iup-table-visible-columns", GINT_TO_POINTER(visiblecolumns));

  return IUP_NOERROR;
}

static void gtk4TableUnMapMethod(Ihandle* ih)
{
  Igtk4TableData* gtk_data = IGTK4_TABLE_DATA(ih);

  if (gtk_data)
  {
    free(gtk_data);
    ih->data->native_data = NULL;
  }

  iupdrvBaseUnMapMethod(ih);
}

/* ========================================================================= */
/* Driver Functions - Table Structure                                       */
/* ========================================================================= */

void iupdrvTableSetNumCol(Ihandle* ih, int num_col)
{
  if (num_col < 0)
    num_col = 0;

  ih->data->num_col = num_col;
}

void iupdrvTableSetNumLin(Ihandle* ih, int num_lin)
{
  Igtk4TableData* gtk_data = IGTK4_TABLE_DATA(ih);

  if (!gtk_data)
    return;

  if (num_lin < 0)
    num_lin = 0;

  ih->data->num_lin = num_lin;

  if (gtk_data->is_virtual)
  {
    g_list_model_items_changed(gtk_data->model, 0, g_list_model_get_n_items(gtk_data->model), num_lin);
    return;
  }

  GListStore* store = G_LIST_STORE(gtk_data->model);
  guint current_rows = g_list_model_get_n_items(gtk_data->model);

  if (num_lin > (int)current_rows)
  {
    for (int i = current_rows; i < num_lin; i++)
    {
      IupTableRow* row = iup_table_row_new(ih->data->num_col, i + 1);
      g_list_store_append(store, row);
      g_object_unref(row);
    }
  }
  else if (num_lin < (int)current_rows)
  {
    g_list_store_splice(store, num_lin, current_rows - num_lin, NULL, 0);
  }
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
  Igtk4TableData* gtk_data = IGTK4_TABLE_DATA(ih);

  if (!gtk_data || gtk_data->is_virtual)
    return;

  GListStore* store = G_LIST_STORE(gtk_data->model);

  if (pos <= 0 || pos > ih->data->num_lin + 1)
    pos = ih->data->num_lin + 1;

  IupTableRow* row = iup_table_row_new(ih->data->num_col, pos);
  g_list_store_insert(store, pos - 1, row);
  g_object_unref(row);

  ih->data->num_lin++;
}

void iupdrvTableDelLin(Ihandle* ih, int pos)
{
  Igtk4TableData* gtk_data = IGTK4_TABLE_DATA(ih);

  if (!gtk_data || gtk_data->is_virtual)
    return;

  if (pos < 1 || pos > ih->data->num_lin)
    return;

  GListStore* store = G_LIST_STORE(gtk_data->model);
  g_list_store_remove(store, pos - 1);

  ih->data->num_lin--;
}

/* ========================================================================= */
/* Driver Functions - Cell Operations                                       */
/* ========================================================================= */

void iupdrvTableSetCellValue(Ihandle* ih, int lin, int col, const char* value)
{
  Igtk4TableData* gtk_data = IGTK4_TABLE_DATA(ih);

  if (lin < 1 || lin > ih->data->num_lin || col < 1 || col > ih->data->num_col)
  {
    return;
  }

  if (!gtk_data || gtk_data->is_virtual)
  {
    return;
  }

  IupTableRow* row = IUP_TABLE_ROW(g_list_model_get_item(gtk_data->model, lin - 1));

  if (row)
  {
    iup_table_row_set_value(row, col - 1, value);
    g_object_unref(row);
  }
}

char* iupdrvTableGetCellValue(Ihandle* ih, int lin, int col)
{
  Igtk4TableData* gtk_data = IGTK4_TABLE_DATA(ih);

  if (!gtk_data || gtk_data->is_virtual)
    return NULL;

  if (lin < 1 || lin > ih->data->num_lin || col < 1 || col > ih->data->num_col)
    return NULL;

  IupTableRow* row = IUP_TABLE_ROW(g_list_model_get_item(gtk_data->model, lin - 1));

  if (row)
  {
    char* value = iupStrReturnStr(row->values[col - 1]);
    g_object_unref(row);
    return value;
  }

  return NULL;
}

/* ========================================================================= */
/* Driver Functions - Column Operations                                     */
/* ========================================================================= */

/* Calculate optimal column width based on header and cell content */
static int gtk4TableCalculateColumnWidth(Ihandle* ih, int col_index)
{
  Igtk4TableData* gtk_data = IGTK4_TABLE_DATA(ih);
  int max_width = 0;
  int max_rows_to_check = (ih->data->num_lin > 100) ? 100 : ih->data->num_lin;

  /* Measure column title */
  GListModel* columns = gtk_column_view_get_columns(GTK_COLUMN_VIEW(gtk_data->column_view));
  GtkColumnViewColumn* column = g_list_model_get_item(columns, col_index);
  if (column)
  {
    const gchar* title = gtk_column_view_column_get_title(column);
    if (title && *title)
    {
      int title_width = iupdrvFontGetStringWidth(ih, (char*)title);
      title_width += 20;  /* Padding for sort indicator */
      if (title_width > max_width)
        max_width = title_width;
    }
    g_object_unref(column);
  }

  /* Measure cell content (check first N rows for performance) */
  for (int lin = 1; lin <= max_rows_to_check; lin++)
  {
    char* cell_value = iupAttribGetId2(ih, "", lin, col_index + 1);
    if (cell_value && *cell_value)
    {
      int cell_width = iupdrvFontGetStringWidth(ih, cell_value);
      cell_width += 16;  /* Add padding (8px left + 8px right) */
      if (cell_width > max_width)
        max_width = cell_width;
    }
  }

  return max_width;
}

void iupdrvTableSetColTitle(Ihandle* ih, int col, const char* title)
{
  Igtk4TableData* gtk_data = IGTK4_TABLE_DATA(ih);

  if (col < 1 || col > ih->data->num_col)
    return;

  GListModel* columns = gtk_column_view_get_columns(GTK_COLUMN_VIEW(gtk_data->column_view));
  GtkColumnViewColumn* column = g_list_model_get_item(columns, col - 1);

  if (column)
  {
    gtk_column_view_column_set_title(column, title ? title : "");

    /* Calculate and set width for columns without explicit width */
    int col_index = col - 1;

    /* Check if this column has explicit width */
    char width_name[50];
    sprintf(width_name, "RASTERWIDTH%d", col);
    char* width_str = iupAttribGet(ih, width_name);
    if (!width_str)
    {
      sprintf(width_name, "WIDTH%d", col);
      width_str = iupAttribGet(ih, width_name);
    }

    /* Only calculate width for columns without explicit width and not the last column (which may stretch) */
    if (!width_str && (col_index < ih->data->num_col - 1 || !ih->data->stretch_last))
    {
      /* When using GtkSortListModel (normal mode sortable), don't set fixed width */
      Igtk4TableData* gtk_data = IGTK4_TABLE_DATA(ih);
      if (!ih->data->sortable || gtk_data->is_virtual)
      {
        int calculated_width = gtk4TableCalculateColumnWidth(ih, col_index);
        gtk_column_view_column_set_fixed_width(column, calculated_width);
      }
    }

    g_object_unref(column);
  }
}

char* iupdrvTableGetColTitle(Ihandle* ih, int col)
{
  Igtk4TableData* gtk_data = IGTK4_TABLE_DATA(ih);

  if (col < 1 || col > ih->data->num_col)
    return NULL;

  GListModel* columns = gtk_column_view_get_columns(GTK_COLUMN_VIEW(gtk_data->column_view));
  GtkColumnViewColumn* column = g_list_model_get_item(columns, col - 1);

  if (column)
  {
    const gchar* title = gtk_column_view_column_get_title(column);
    g_object_unref(column);
    return iupStrReturnStr(title);
  }

  return NULL;
}

void iupdrvTableSetColWidth(Ihandle* ih, int col, int width)
{
  Igtk4TableData* gtk_data = IGTK4_TABLE_DATA(ih);

  if (col < 1 || col > ih->data->num_col)
    return;

  GListModel* columns = gtk_column_view_get_columns(GTK_COLUMN_VIEW(gtk_data->column_view));
  GtkColumnViewColumn* column = g_list_model_get_item(columns, col - 1);

  if (column)
  {
    gtk_column_view_column_set_fixed_width(column, width);
    gtk_column_view_column_set_resizable(column, ih->data->user_resize);
    g_object_unref(column);
  }
}

int iupdrvTableGetColWidth(Ihandle* ih, int col)
{
  Igtk4TableData* gtk_data = IGTK4_TABLE_DATA(ih);

  if (col < 1 || col > ih->data->num_col)
    return 0;

  GListModel* columns = gtk_column_view_get_columns(GTK_COLUMN_VIEW(gtk_data->column_view));
  GtkColumnViewColumn* column = g_list_model_get_item(columns, col - 1);

  if (column)
  {
    int width = gtk_column_view_column_get_fixed_width(column);
    g_object_unref(column);
    return width;
  }

  return 0;
}

/* ========================================================================= */
/* Driver Functions - Selection                                             */
/* ========================================================================= */

void iupdrvTableSetFocusCell(Ihandle* ih, int lin, int col)
{
  Igtk4TableData* gtk_data = IGTK4_TABLE_DATA(ih);

  if (lin < 1 || lin > ih->data->num_lin || col < 1 || col > ih->data->num_col)
    return;

  /* Update current row and column tracking BEFORE setting selection */
  gtk_data->current_row = lin;
  gtk_data->current_col = col;

  /* Set row selection - this will trigger selection changed signal which calls ENTERITEM_CB */
  if (GTK_IS_SINGLE_SELECTION(gtk_data->selection_model))
    gtk_single_selection_set_selected(GTK_SINGLE_SELECTION(gtk_data->selection_model), lin - 1);

  /* Trigger widget redraw to show focus in correct column */
  gtk_widget_queue_draw(gtk_data->column_view);
}

void iupdrvTableGetFocusCell(Ihandle* ih, int* lin, int* col)
{
  Igtk4TableData* gtk_data = IGTK4_TABLE_DATA(ih);

  if (!gtk_data)
  {
    *lin = 0;
    *col = 0;
    return;
  }

  *lin = gtk_data->current_row;
  *col = gtk_data->current_col;
}

/* ========================================================================= */
/* Driver Functions - Scrolling                                             */
/* ========================================================================= */

void iupdrvTableScrollToCell(Ihandle* ih, int lin, int col)
{
  Igtk4TableData* gtk_data = IGTK4_TABLE_DATA(ih);

  if (lin < 1 || lin > ih->data->num_lin)
    return;

  gtk_widget_activate_action(gtk_data->column_view, "list.scroll-to-item", "u", lin - 1);
}

/* ========================================================================= */
/* Driver Functions - Display                                               */
/* ========================================================================= */

void iupdrvTableRedraw(Ihandle* ih)
{
  Igtk4TableData* gtk_data = IGTK4_TABLE_DATA(ih);

  if (gtk_data->is_virtual)
  {
    /* In virtual mode, invalidate only first 100 items to refresh visible content */
    /* This avoids fetching all items while still updating the display */
    guint n_items = g_list_model_get_n_items(gtk_data->model);
    guint invalidate_count = (n_items > 100) ? 100 : n_items;

    if (invalidate_count > 0)
    {
      g_list_model_items_changed(gtk_data->model, 0, invalidate_count, invalidate_count);
    }
  }
  else
  {
    /* In non-virtual mode, reset factories to trigger rebind */
    GListModel* columns = gtk_column_view_get_columns(GTK_COLUMN_VIEW(gtk_data->column_view));
    guint n_columns = g_list_model_get_n_items(columns);

    for (guint i = 0; i < n_columns; i++)
    {
      GtkColumnViewColumn* column = g_list_model_get_item(columns, i);
      if (column)
      {
        GtkListItemFactory* factory = gtk_column_view_column_get_factory(column);
        if (factory)
        {
          g_object_ref(factory);
          gtk_column_view_column_set_factory(column, NULL);
          gtk_column_view_column_set_factory(column, factory);
          g_object_unref(factory);
        }
        g_object_unref(column);
      }
    }
  }
}

void iupdrvTableSetShowGrid(Ihandle* ih, int show)
{
  Igtk4TableData* gtk_data = IGTK4_TABLE_DATA(ih);

  gtk_column_view_set_show_column_separators(GTK_COLUMN_VIEW(gtk_data->column_view), show);
  gtk_column_view_set_show_row_separators(GTK_COLUMN_VIEW(gtk_data->column_view), show);
}

int iupdrvTableGetBorderWidth(Ihandle* ih)
{
  (void)ih;
  /* GtkScrolledWindow doesn't add extra border width */
  return 0;
}

static int gtk4_table_row_height = -1;
static int gtk4_table_header_height = -1;

static void gtk4TableMeasureRowMetrics(Ihandle* ih)
{
  if (gtk4_table_row_height >= 0)
    return;

  GtkWidget* temp_window = gtk_window_new();

  /* Create a label like the one used in table cells */
  GtkWidget* temp_label = gtk_label_new("Wg");

  /* Create a box like the cell container */
  GtkWidget* temp_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_append(GTK_BOX(temp_box), temp_label);

  gtk_window_set_child(GTK_WINDOW(temp_window), temp_box);

  /* Realize to get accurate measurements */
  gtk_widget_realize(temp_window);

  /* Measure the box (cell container) */
  int min_h, nat_h;
  gtk_widget_measure(temp_box, GTK_ORIENTATION_VERTICAL, -1, &min_h, &nat_h, NULL, NULL);

  /* GtkColumnView adds padding around each row */
  gtk4_table_row_height = nat_h + 6;

  /* Header */
  GtkWidget* temp_header_label = gtk_label_new("Header");
  GtkWidget* temp_header_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_append(GTK_BOX(temp_header_box), temp_header_label);

  /* Measure header */
  int header_min, header_nat;
  gtk_widget_measure(temp_header_label, GTK_ORIENTATION_VERTICAL, -1, &header_min, &header_nat, NULL, NULL);
  gtk4_table_header_height = header_nat + 10;

  /* Clean up */
  gtk_window_destroy(GTK_WINDOW(temp_window));

  (void)ih;
}

int iupdrvTableGetRowHeight(Ihandle* ih)
{
  Igtk4TableData* gtk_data = IGTK4_TABLE_DATA(ih);

  /* If table is mapped and has rows, try to measure actual row */
  if (gtk_data && gtk_data->column_view && ih->data->num_lin > 0)
  {
    /* Get the selection model to access items */
    GtkSelectionModel* model = gtk_column_view_get_model(GTK_COLUMN_VIEW(gtk_data->column_view));
    if (model)
    {
      GListModel* list_model = G_LIST_MODEL(model);
      guint n_items = g_list_model_get_n_items(list_model);
      if (n_items > 0)
      {
        /* Measure the column view's natural height and calculate row height */
        int min_h, nat_h;
        gtk_widget_measure(gtk_data->column_view, GTK_ORIENTATION_VERTICAL, -1, &min_h, &nat_h, NULL, NULL);

        /* Subtract header height and divide by number of items */
        gtk4TableMeasureRowMetrics(ih);
        int content_height = nat_h - gtk4_table_header_height;
        if (content_height > 0 && n_items > 0)
        {
          int measured_row = content_height / n_items;
          if (measured_row > 0)
            return measured_row;
        }
      }
    }
  }

  /* Fallback to pre-measured value */
  gtk4TableMeasureRowMetrics(ih);
  return gtk4_table_row_height;
}

int iupdrvTableGetHeaderHeight(Ihandle* ih)
{
  gtk4TableMeasureRowMetrics(ih);
  return gtk4_table_header_height;
}

void iupdrvTableAddBorders(Ihandle* ih, int* w, int* h)
{
  (void)ih;
  int sb_size = iupdrvGetScrollbarSize();

  /* GtkScrolledWindow: add scrollbar width */
  *w += sb_size;

  /* GtkScrolledWindow frame border */
  *h += 2;
}

/* ========================================================================= */
/* Attribute Handlers                                                        */
/* ========================================================================= */

static int gtk4TableSetSortableAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->sortable = 1;
  else
    ih->data->sortable = 0;

  if (ih->handle)
  {
    Igtk4TableData* gtk_data = IGTK4_TABLE_DATA(ih);
    if (gtk_data && gtk_data->column_view)
    {
      GListModel* columns = gtk_column_view_get_columns(GTK_COLUMN_VIEW(gtk_data->column_view));
      guint n_columns = g_list_model_get_n_items(columns);

      for (guint i = 0; i < n_columns; i++)
      {
        GtkColumnViewColumn* column = g_list_model_get_item(columns, i);
        if (column)
        {
          if (ih->data->sortable)
          {
            GtkSorter* sorter = g_object_get_data(G_OBJECT(column), "iup-sorter");
            gtk_column_view_column_set_sorter(column, sorter);
          }
          else
          {
            gtk_column_view_column_set_sorter(column, NULL);
          }
          g_object_unref(column);
        }
      }
    }
  }

  return 0;
}

static int gtk4TableSetAllowReorderAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->allow_reorder = 1;
  else
    ih->data->allow_reorder = 0;

  Igtk4TableData* gtk_data = IGTK4_TABLE_DATA(ih);
  if (gtk_data && gtk_data->column_view)
  {
    gtk_column_view_set_reorderable(GTK_COLUMN_VIEW(gtk_data->column_view), ih->data->allow_reorder);
  }

  return 0;
}

static int gtk4TableSetUserResizeAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->user_resize = 1;
  else
    ih->data->user_resize = 0;

  if (ih->handle)
  {
    Igtk4TableData* gtk_data = IGTK4_TABLE_DATA(ih);
    if (gtk_data && gtk_data->column_view)
    {
      GListModel* columns = gtk_column_view_get_columns(GTK_COLUMN_VIEW(gtk_data->column_view));
      guint n_columns = g_list_model_get_n_items(columns);

      for (guint i = 0; i < n_columns; i++)
      {
        GtkColumnViewColumn* column = g_list_model_get_item(columns, i);
        if (column)
        {
          gtk_column_view_column_set_resizable(column, ih->data->user_resize);
          g_object_unref(column);
        }
      }
    }
  }

  return 0;
}

/* ========================================================================= */
/* Driver Class Initialization                                              */
/* ========================================================================= */

void iupdrvTableInitClass(Iclass* ic)
{
  ic->Map = gtk4TableMapMethod;
  ic->UnMap = gtk4TableUnMapMethod;
  ic->LayoutUpdate = gtk4TableLayoutUpdateMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NO_SAVE | IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "SIZE", NULL, NULL, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NOT_MAPPED);

  /* FOCUSRECT not supported - GtkColumnView architecture makes dashed border complex */
  iupClassRegisterAttribute(ic, "FOCUSRECT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);

  iupClassRegisterReplaceAttribFunc(ic, "SORTABLE", NULL, gtk4TableSetSortableAttrib);
  iupClassRegisterReplaceAttribFunc(ic, "ALLOWREORDER", NULL, gtk4TableSetAllowReorderAttrib);
  iupClassRegisterReplaceAttribFunc(ic, "USERRESIZE", NULL, gtk4TableSetUserResizeAttrib);
}
