/** \file
 * \brief List Control for GTK4
 *
 * See Copyright Notice in "iup.h"
 */

#include <gtk/gtk.h>

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
#include "iup_mask.h"
#include "iup_key.h"
#include "iup_image.h"
#include "iup_list.h"
#include "iup_childtree.h"

#include "iupgtk4_drv.h"


/* ========================================================================= */
/* List Item Data Object for GListModel                                     */
/* ========================================================================= */

#define IUP_TYPE_LIST_ITEM (iup_list_item_get_type())
#define IUP_LIST_ITEM(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), IUP_TYPE_LIST_ITEM, IupListItem))

typedef struct _IupListItem IupListItem;
typedef struct _IupListItemClass IupListItemClass;

struct _IupListItem
{
  GObject parent;
  gchar* text;
  GdkTexture* image;
  gboolean enabled;
  gpointer user_data;
  guint update_count;
};

struct _IupListItemClass
{
  GObjectClass parent_class;
};

enum {
  ITEM_PROP_0,
  ITEM_PROP_TEXT,
  ITEM_PROP_IMAGE,
  ITEM_PROP_ENABLED,
  ITEM_PROP_UPDATE,
  ITEM_N_PROPS
};

static GParamSpec* item_props[ITEM_N_PROPS] = { NULL, };

GType iup_list_item_get_type(void);
G_DEFINE_TYPE(IupListItem, iup_list_item, G_TYPE_OBJECT)

static void iup_list_item_finalize(GObject* object)
{
  IupListItem* item = IUP_LIST_ITEM(object);

  g_free(item->text);
  if (item->image)
    g_object_unref(item->image);

  G_OBJECT_CLASS(iup_list_item_parent_class)->finalize(object);
}

static void iup_list_item_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec)
{
  IupListItem* item = IUP_LIST_ITEM(object);

  switch (prop_id)
  {
    case ITEM_PROP_TEXT:
      g_value_set_string(value, item->text);
      break;
    case ITEM_PROP_IMAGE:
      g_value_set_object(value, item->image);
      break;
    case ITEM_PROP_ENABLED:
      g_value_set_boolean(value, item->enabled);
      break;
    case ITEM_PROP_UPDATE:
      g_value_set_uint(value, item->update_count);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
  }
}

static void iup_list_item_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec)
{
  IupListItem* item = IUP_LIST_ITEM(object);

  switch (prop_id)
  {
    case ITEM_PROP_TEXT:
      g_free(item->text);
      item->text = g_value_dup_string(value);
      item->update_count++;
      g_object_notify_by_pspec(object, item_props[ITEM_PROP_UPDATE]);
      break;
    case ITEM_PROP_IMAGE:
      if (item->image)
        g_object_unref(item->image);
      item->image = g_value_dup_object(value);
      item->update_count++;
      g_object_notify_by_pspec(object, item_props[ITEM_PROP_UPDATE]);
      break;
    case ITEM_PROP_ENABLED:
      item->enabled = g_value_get_boolean(value);
      item->update_count++;
      g_object_notify_by_pspec(object, item_props[ITEM_PROP_UPDATE]);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
  }
}

static void iup_list_item_init(IupListItem* item)
{
  item->text = g_strdup("");
  item->image = NULL;
  item->enabled = TRUE;
  item->user_data = NULL;
  item->update_count = 0;
}

static void iup_list_item_class_init(IupListItemClass* klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS(klass);

  object_class->finalize = iup_list_item_finalize;
  object_class->get_property = iup_list_item_get_property;
  object_class->set_property = iup_list_item_set_property;

  item_props[ITEM_PROP_TEXT] = g_param_spec_string("text", "Text", "Item text", "", G_PARAM_READWRITE);
  item_props[ITEM_PROP_IMAGE] = g_param_spec_object("image", "Image", "Item image", GDK_TYPE_TEXTURE, G_PARAM_READWRITE);
  item_props[ITEM_PROP_ENABLED] = g_param_spec_boolean("enabled", "Enabled", "Item enabled state", TRUE, G_PARAM_READWRITE);
  item_props[ITEM_PROP_UPDATE] = g_param_spec_uint("update", "Update", "Update counter", 0, G_MAXUINT, 0, G_PARAM_READABLE);

  g_object_class_install_properties(object_class, ITEM_N_PROPS, item_props);
}

static IupListItem* iup_list_item_new(const char* text)
{
  IupListItem* item = g_object_new(IUP_TYPE_LIST_ITEM, NULL);
  if (text)
  {
    g_free(item->text);
    item->text = g_strdup(text);
  }
  return item;
}

static void iup_list_item_set_text(IupListItem* item, const char* text)
{
  if (!item)
    return;

  g_object_set(item, "text", text ? text : "", NULL);
}

static const char* iup_list_item_get_text(IupListItem* item)
{
  return item ? item->text : NULL;
}

static void iup_list_item_set_image(IupListItem* item, GdkTexture* image)
{
  if (!item)
    return;

  g_object_set(item, "image", image, NULL);
}

static GdkTexture* iup_list_item_get_image(IupListItem* item)
{
  return item ? item->image : NULL;
}

static void iup_list_item_set_enabled(IupListItem* item, gboolean enabled)
{
  if (!item)
    return;

  g_object_set(item, "enabled", enabled, NULL);
}

static gboolean iup_list_item_get_enabled(IupListItem* item)
{
  return item ? item->enabled : FALSE;
}

static void iup_list_item_set_user_data(IupListItem* item, gpointer data)
{
  if (item)
    item->user_data = data;
}

static gpointer iup_list_item_get_user_data(IupListItem* item)
{
  return item ? item->user_data : NULL;
}

/* ========================================================================= */

enum
{
  IUPGTK4_LIST_IMAGE,
  IUPGTK4_LIST_TEXT,
  IUPGTK4_LIST_LAST_DATA
};

static GListStore* gtk4ListGetGListStore(Ihandle* ih)
{
  return (GListStore*)iupAttribGet(ih, "_IUPGTK4_GLISTSTORE");
}

static void gtk4ListSetGListStore(Ihandle* ih, GListStore* store)
{
  iupAttribSet(ih, "_IUPGTK4_GLISTSTORE", (char*)store);
}

static GtkSelectionModel* gtk4ListGetSelectionModel(Ihandle* ih)
{
  return (GtkSelectionModel*)iupAttribGet(ih, "_IUPGTK4_SELECTIONMODEL");
}

static void gtk4ListSetSelectionModel(Ihandle* ih, GtkSelectionModel* model)
{
  iupAttribSet(ih, "_IUPGTK4_SELECTIONMODEL", (char*)model);
}

static GtkListItemFactory* gtk4ListGetFactory(Ihandle* ih)
{
  return (GtkListItemFactory*)iupAttribGet(ih, "_IUPGTK4_FACTORY");
}

static void gtk4ListSetFactory(Ihandle* ih, GtkListItemFactory* factory)
{
  iupAttribSet(ih, "_IUPGTK4_FACTORY", (char*)factory);
}

void iupdrvListAddItemSpace(Ihandle* ih, int *h)
{
  static int spacing = -1;

  if (spacing == -1)
  {
    /* Measure actual GTK4 list item spacing dynamically */
    GtkStringList* string_list = gtk_string_list_new(NULL);
    gtk_string_list_append(string_list, "Item 1");
    gtk_string_list_append(string_list, "Item 2");

    GtkSingleSelection* selection = gtk_single_selection_new(G_LIST_MODEL(string_list));
    GtkWidget* list_view = gtk_list_view_new(GTK_SELECTION_MODEL(selection), NULL);

    /* Create a simple factory for text items */
    GtkListItemFactory* factory = gtk_signal_list_item_factory_new();
    gtk_list_view_set_factory(GTK_LIST_VIEW(list_view), factory);

    /* Measure the list with 2 items */
    int min_height, nat_height;
    gtk_widget_measure(list_view, GTK_ORIENTATION_VERTICAL, -1, &min_height, &nat_height, NULL, NULL);

    /* Get text height */
    int char_height;
    iupdrvFontGetCharSize(ih, NULL, &char_height);

    /* Calculate spacing: (total_height - 2*text_height) / 2 items */
    int total_spacing = nat_height - (2 * char_height);
    spacing = (total_spacing > 0) ? (total_spacing / 2) : 2;

    /* Cleanup */
    g_object_unref(factory);
    g_object_ref_sink(list_view);
    g_object_unref(list_view);
  }

  *h += spacing;
}

void iupdrvListAddBorders(Ihandle* ih, int *x, int *y)
{
  static int editbox_border_x = -1, editbox_border_y = -1, editbox_entry_natural_height = -1;
  static int dropdown_border_x = -1, dropdown_border_y = -1, dropdown_natural_height = -1;
  static int dropdown_editbox_border_x = -1;
  static int scrolled_window_frame_border = -1;
  static int css_frame_border_compensation = -1;

  int border_size = 2 * 5;  /* Base border: 2 * 5px */
  int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");

  /* Initialize constants once */
  if (scrolled_window_frame_border == -1)
  {
    /* GTK4 scrolled_window has no internal overhead, only external CSS border */
    scrolled_window_frame_border = 2;  /* CSS border, same for all scrolled_window types */
  }
  if (css_frame_border_compensation == -1)
  {
    /* CSS .frame border compensation for plain lists */
    css_frame_border_compensation = 2;  /* 1px top + 1px bottom */
  }

  (*x) += border_size;

  /* For plain lists with VISIBLELINES, add CSS border compensation */
  if (!ih->data->is_dropdown && !ih->data->has_editbox && visiblelines > 0)
  {
    (*y) += css_frame_border_compensation;
  }
  else
  {
    (*y) += border_size;
  }

  /* GtkBox with 4px spacing between image and text */
  if (ih->data->show_image)
    (*x) += 4;

  if (ih->data->is_dropdown)
  {
    if (dropdown_border_x == -1)
    {
      /* Measure dropdown borders dynamically */
      GtkWidget* temp_combo = gtk_drop_down_new(NULL, NULL);
      int min_w, nat_w, min_h, nat_h;

      gtk_widget_measure(temp_combo, GTK_ORIENTATION_HORIZONTAL, -1, &min_w, &nat_w, NULL, NULL);
      gtk_widget_measure(temp_combo, GTK_ORIENTATION_VERTICAL, -1, &min_h, &nat_h, NULL, NULL);

      /* Store natural height for future reference */
      dropdown_natural_height = nat_h;

      /* Dropdown borders are the widget size minus content area */
      dropdown_border_x = (nat_w > 50) ? 22 : 20;  /* Approximate dropdown arrow + spacing */

      /* Calculate border needed to reach widget's natural height */
      /* Get typical single-item content height */
      int char_width, char_height;
      iupdrvFontGetCharSize(ih, &char_width, &char_height);
      int typical_item_height = char_height;
      iupdrvListAddItemSpace(ih, &typical_item_height);

      /* dropdown_border_y should add enough to reach natural height */
      /* Final height = typical_item_height + base_border + dropdown_border_y = dropdown_natural_height */
      dropdown_border_y = dropdown_natural_height - typical_item_height - border_size;

      g_object_ref_sink(temp_combo);
      g_object_unref(temp_combo);
    }

    (*x) += dropdown_border_x;

    if (ih->data->has_editbox)
    {
      if (dropdown_editbox_border_x == -1)
        dropdown_editbox_border_x = 5;  /* Additional space for editbox combo */
      (*x) += dropdown_editbox_border_x;
    }
    else
    {
      (*y) += dropdown_border_y;
      (*x) += dropdown_border_y;
    }
  }
  else
  {
    int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");

    if (ih->data->has_editbox)
    {
      if (editbox_border_x == -1 || editbox_border_y == -1)
      {
        /* Measure actual GtkEntry widget with CSS applied */
        GtkWidget *temp_entry = gtk_entry_new();

        {
          GtkStyleContext *context = gtk_widget_get_style_context(temp_entry);
          GtkCssProvider *provider = gtk_css_provider_new();
          gtk_css_provider_load_from_string(GTK_CSS_PROVIDER(provider),
            "*{ padding-left: 2px; padding-right: 2px; min-height: 24px; }");
          gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
          g_object_unref(provider);
        }

        int min_w, nat_w, min_h, nat_h;

        gtk_widget_measure(temp_entry, GTK_ORIENTATION_HORIZONTAL, -1, &min_w, &nat_w, NULL, NULL);
        gtk_widget_measure(temp_entry, GTK_ORIENTATION_VERTICAL, -1, &min_h, &nat_h, NULL, NULL);

        editbox_entry_natural_height = nat_h;
        editbox_border_x = 0;  /* No extra X needed */
        /* Entry height + small gap for VBox spacing between entry and list */
        int entry_gap = 3;
        editbox_border_y = editbox_entry_natural_height + entry_gap;

        g_object_ref_sink(temp_entry);
        g_object_unref(temp_entry);
      }

      (*x) += editbox_border_x;

      /* For VISIBLELINES, use different border logic */
      if (visiblelines > 0)
      {
        /* For EDITBOX, VISIBLELINES includes the entry line.
         * We need to remove one item's height from the calculation to show (VISIBLELINES - 1) items in the list part. */
        int char_width, char_height;
        iupdrvFontGetCharSize(ih, &char_width, &char_height);
        int item_height = char_height;
        iupdrvListAddItemSpace(ih, &item_height);
        (*y) -= item_height;

        /* Only add entry height + scrolled_window frame borders */
        (*y) -= border_size; /* Remove the base border that was added earlier */
        (*y) += editbox_entry_natural_height; /* Add entry height (no gap for GtkFixed) */
        (*y) += (scrolled_window_frame_border - css_frame_border_compensation); /* Add frame borders, minus CSS already in content */
      }
      else
      {
        (*y) += editbox_border_y;
      }

      if (ih->data->sb && !visiblelines)
      {
        (*y) += iupdrvGetScrollbarSize();
      }
    }
    else if (ih->data->sb && !visiblelines)
    {
      (*y) += iupdrvGetScrollbarSize();
    }
  }
}

static int gtk4ListConvertXYToPos(Ihandle* ih, int x, int y)
{
  if (!ih->data->is_dropdown)
  {
    GListStore* store = gtk4ListGetGListStore(ih);

    if (store)
    {
      GtkWidget *picked = gtk_widget_pick(ih->handle, x, y, GTK_PICK_DEFAULT);
      if (picked)
      {
        /* Walk up widget hierarchy to find GtkListItemWidget or GtkBox with position data */
        GtkWidget *current = picked;
        int depth = 0;
        while (current && current != ih->handle)
        {
          const char *type_name = G_OBJECT_TYPE_NAME(current);

          /* Check for GtkListItemWidget first - this is the container for list items */
          if (strcmp(type_name, "GtkListItemWidget") == 0)
          {
            /* Get the first child of GtkListItemWidget, which should be our GtkBox */
            GtkWidget *child = gtk_widget_get_first_child(current);
            if (child && strcmp(G_OBJECT_TYPE_NAME(child), "GtkBox") == 0)
            {
              gpointer data = g_object_get_data(G_OBJECT(child), "iup-list-position");
              if (data)
              {
                guint position = GPOINTER_TO_UINT(data) - 1;  /* Stored as position+1 to avoid NULL */
                if (position != GTK_INVALID_LIST_POSITION)
                  return (int)position + 1;  /* IUP uses 1-based indexing */
              }
            }
            /* Found GtkListItemWidget but no valid position */
            break;
          }

          /* Also check if current widget itself is our GtkBox with position data */
          if (strcmp(type_name, "GtkBox") == 0)
          {
            gpointer data = g_object_get_data(G_OBJECT(current), "iup-list-position");
            if (data)
            {
              guint position = GPOINTER_TO_UINT(data) - 1;  /* Stored as position+1 to avoid NULL */
              if (position != GTK_INVALID_LIST_POSITION)
                return (int)position + 1;  /* IUP uses 1-based indexing */
            }
            /* This GtkBox doesn't have position data, keep searching up for GtkListItemWidget */
          }

          current = gtk_widget_get_parent(current);
          depth++;
          if (depth > 20) break;  /* Safety limit */
        }
      }
      return -1;
    }
  }

  return -1;
}

int iupdrvListGetCount(Ihandle* ih)
{
  GListStore* store = gtk4ListGetGListStore(ih);
  return g_list_model_get_n_items(G_LIST_MODEL(store));
}

void iupdrvListAppendItem(Ihandle* ih, const char* value)
{
  GListStore* store = gtk4ListGetGListStore(ih);

  if (store)
  {
    IupListItem* item = iup_list_item_new(iupgtk4StrConvertToSystem(value));

    int pos = g_list_model_get_n_items(G_LIST_MODEL(store));
    char attr_name[30];
    sprintf(attr_name, "IMAGE%d", pos + 1);
    const char* image_name = iupAttribGet(ih, attr_name);
    if (image_name)
    {
      GdkPaintable* paintable = (GdkPaintable*)iupImageGetImage(image_name, ih, 0, NULL);
      if (paintable)
      {
        GdkTexture* texture = GDK_TEXTURE(paintable);
        iup_list_item_set_image(item, texture);
      }
    }

    iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
    g_list_store_append(store, item);
    iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
    g_object_unref(item);
  }
}

void iupdrvListInsertItem(Ihandle* ih, int pos, const char* value)
{
  GListStore* store = gtk4ListGetGListStore(ih);

  if (store)
  {
    IupListItem* item = iup_list_item_new(iupgtk4StrConvertToSystem(value));

    char attr_name[30];
    sprintf(attr_name, "IMAGE%d", pos + 1);
    const char* image_name = iupAttribGet(ih, attr_name);
    if (image_name)
    {
      GdkPaintable* paintable = (GdkPaintable*)iupImageGetImage(image_name, ih, 0, NULL);
      if (paintable)
      {
        GdkTexture* texture = GDK_TEXTURE(paintable);
        iup_list_item_set_image(item, texture);
      }
    }

    iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
    g_list_store_insert(store, pos, item);
    iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
    g_object_unref(item);

    iupListUpdateOldValue(ih, pos, 0);
  }
}

void iupdrvListRemoveItem(Ihandle* ih, int pos)
{
  GListStore* store = gtk4ListGetGListStore(ih);

  if (store)
  {
    if (ih->data->is_dropdown && !ih->data->has_editbox)
    {
      GtkDropDown* dropdown = GTK_DROP_DOWN(ih->handle);
      guint curpos = gtk_drop_down_get_selected(dropdown);

      if (pos == (int)curpos)
      {
        int new_pos = -1;
        if (curpos > 0)
          new_pos = curpos - 1;
        else if (iupdrvListGetCount(ih) > 1)
          new_pos = 0;

        iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
        if (new_pos >= 0)
          gtk_drop_down_set_selected(dropdown, new_pos);
        else
          gtk_drop_down_set_selected(dropdown, GTK_INVALID_LIST_POSITION);
        iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
      }
    }

    iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
    g_list_store_remove(store, pos);
    iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);

    iupListUpdateOldValue(ih, pos, 1);
  }
}

void iupdrvListRemoveAllItems(Ihandle* ih)
{
  GListStore* store = gtk4ListGetGListStore(ih);

  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
  g_list_store_remove_all(store);
  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
}

static int gtk4ListSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  if (ih->handle)
  {
    if (ih->data->is_dropdown)
    {
      GtkCellRenderer* renderer = (GtkCellRenderer*)iupAttribGet(ih, "_IUPGTK4_RENDERER");
      if (renderer)
        iupgtk4UpdateObjectFont(ih, G_OBJECT(renderer));
    }

    if (ih->data->has_editbox)
    {
      GtkEntry* entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
      if (entry)
        iupgtk4UpdateWidgetFont(ih, (GtkWidget*)entry);
    }
  }
  return 1;
}

static char* gtk4ListGetIdValueAttrib(Ihandle* ih, int id)
{
  int pos = iupListGetPosAttrib(ih, id);
  if (pos >= 0)
  {
    GListStore* store = gtk4ListGetGListStore(ih);

    if (store)
    {
      IupListItem* item = g_list_model_get_item(G_LIST_MODEL(store), pos);
      if (item)
      {
        const char* text = iup_list_item_get_text(item);
        char* ret_str = iupStrReturnStr(iupgtk4StrConvertFromSystem(text));
        g_object_unref(item);
        return ret_str;
      }
    }
  }
  return NULL;
}

static int gtk4ListSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;

  GtkScrolledWindow* scrolled_window = (GtkScrolledWindow*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (scrolled_window && !ih->data->is_dropdown && iupStrBoolean(IupGetGlobal("SB_BGCOLOR")))
  {
    char* parent_value = iupBaseNativeParentGetBgColor(ih);

    if (iupStrToRGB(parent_value, &r, &g, &b))
    {
      GtkWidget* sb;

      if (!GTK_IS_SCROLLED_WINDOW(scrolled_window))
        scrolled_window = (GtkScrolledWindow*)iupAttribGet(ih, "_IUPGTK4_SCROLLED_WINDOW");

      iupgtk4SetBgColor((GtkWidget*)scrolled_window, r, g, b);

      sb = gtk_scrolled_window_get_hscrollbar(scrolled_window);
      if (sb) iupgtk4SetBgColor(sb, r, g, b);

      sb = gtk_scrolled_window_get_vscrollbar(scrolled_window);
      if (sb) iupgtk4SetBgColor(sb, r, g, b);
    }
  }

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  if (ih->data->has_editbox)
  {
    GtkWidget* entry = (GtkWidget*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
    iupgtk4SetBgColor(entry, r, g, b);
  }

  if (!ih->data->is_dropdown)
  {
    GtkCellRenderer* renderer = (GtkCellRenderer*)iupAttribGet(ih, "_IUPGTK4_RENDERER");
    if (renderer)
    {
      GdkRGBA rgba;
      rgba.red = r / 255.0;
      rgba.green = g / 255.0;
      rgba.blue = b / 255.0;
      rgba.alpha = 1.0;
      g_object_set(G_OBJECT(renderer), "cell-background-rgba", &rgba, NULL);
    }
  }

  return iupdrvBaseSetBgColorAttrib(ih, value);
}

static int gtk4ListSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupgtk4SetFgColor(ih->handle, r, g, b);

  if (ih->data->has_editbox)
  {
    GtkWidget* entry = (GtkWidget*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
    iupgtk4SetFgColor(entry, r, g, b);
  }

  if (!ih->data->is_dropdown)
  {
    GtkCellRenderer* renderer = (GtkCellRenderer*)iupAttribGet(ih, "_IUPGTK4_RENDERER");
    if (renderer)
    {
      GdkRGBA rgba;
      rgba.red = r / 255.0;
      rgba.green = g / 255.0;
      rgba.blue = b / 255.0;
      rgba.alpha = 1.0;
      g_object_set(G_OBJECT(renderer), "foreground-rgba", &rgba, NULL);
    }
  }

  return 1;
}

static char* gtk4ListGetValueAttrib(Ihandle* ih)
{
  if (ih->data->has_editbox)
  {
    GtkEntry* entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
    return iupStrReturnStr(iupgtk4StrConvertFromSystem(gtk_editable_get_text(GTK_EDITABLE(entry))));
  }
  else
  {
    GListStore* store = gtk4ListGetGListStore(ih);

    if (store)
    {
      if (ih->data->is_dropdown)
      {
        guint pos = gtk_drop_down_get_selected(GTK_DROP_DOWN(ih->handle));
        if (pos == GTK_INVALID_LIST_POSITION)
          return NULL;
        return iupStrReturnInt((int)pos + 1);
      }
      else
      {
        GtkSelectionModel* selection = gtk4ListGetSelectionModel(ih);

        if (!ih->data->is_multiple)
        {
          guint pos = gtk_single_selection_get_selected(GTK_SINGLE_SELECTION(selection));
          if (pos == GTK_INVALID_LIST_POSITION)
            return NULL;
          return iupStrReturnInt((int)pos + 1);
        }
        else
        {
          int count = iupdrvListGetCount(ih);
          char* str = iupStrGetMemory(count+1);
          memset(str, '-', count);
          str[count] = 0;

          GtkBitset* bitset = gtk_selection_model_get_selection(selection);

          for (int i = 0; i < count; i++)
          {
            if (gtk_bitset_contains(bitset, i))
              str[i] = '+';
          }

          gtk_bitset_unref(bitset);
          return str;
        }
      }
    }
  }

  return NULL;
}

static int gtk4ListSetValueAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->has_editbox)
  {
    GtkEntry* entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
    if (!value) value = "";
    iupAttribSet(ih, "_IUPGTK4_DISABLE_TEXT_CB", "1");
    gtk_editable_set_text(GTK_EDITABLE(entry), iupgtk4StrConvertToSystem(value));
    iupAttribSet(ih, "_IUPGTK4_DISABLE_TEXT_CB", NULL);
  }
  else
  {
    GListStore* store = gtk4ListGetGListStore(ih);

    if (store)
    {
      if (ih->data->is_dropdown)
      {
        int pos;
        int count = iupdrvListGetCount(ih);
        iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
        if (iupStrToInt(value, &pos) == 1 && (pos > 0 && pos <= count))
        {
          gtk_drop_down_set_selected(GTK_DROP_DOWN(ih->handle), pos - 1);
          iupAttribSetInt(ih, "_IUPLIST_OLDVALUE", pos);
        }
        else
        {
          gtk_drop_down_set_selected(GTK_DROP_DOWN(ih->handle), GTK_INVALID_LIST_POSITION);
          iupAttribSet(ih, "_IUPLIST_OLDVALUE", NULL);
        }
        iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
      }
      else
      {
        GtkSelectionModel* selection = gtk4ListGetSelectionModel(ih);

        if (!ih->data->is_multiple)
        {
          int pos;
          iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
          if (iupStrToInt(value, &pos) == 1)
          {
            gtk_single_selection_set_selected(GTK_SINGLE_SELECTION(selection), pos - 1);
            iupAttribSetInt(ih, "_IUPLIST_OLDVALUE", pos);
          }
          else
          {
            gtk_single_selection_set_selected(GTK_SINGLE_SELECTION(selection), GTK_INVALID_LIST_POSITION);
            iupAttribSet(ih, "_IUPLIST_OLDVALUE", NULL);
          }
          iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
        }
        else
        {
          int i, len, count;

          iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");

          gtk_selection_model_unselect_all(selection);

          if (!value)
          {
            iupAttribSet(ih, "_IUPLIST_OLDVALUE", NULL);
            iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
            return 0;
          }

          len = (int)strlen(value);
          count = iupdrvListGetCount(ih);
          if (len < count)
            count = len;

          for (i = 0; i < count; i++)
          {
            if (value[i] == '+')
              gtk_selection_model_select_item(selection, i, FALSE);
          }
          iupAttribSetStr(ih, "_IUPLIST_OLDVALUE", value);
          iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
        }
      }
    }
  }

  return 0;
}

static int gtk4ListSetShowDropdownAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->is_dropdown)
  {
    GListStore* store = gtk4ListGetGListStore(ih);

    if (store)
    {
      (void)value;
    }
  }
  return 0;
}

static int gtk4ListSetTopItemAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->is_dropdown)
  {
    int pos = 1;
    if (iupStrToInt(value, &pos))
    {
      gtk_list_view_scroll_to(GTK_LIST_VIEW(ih->handle), pos - 1, GTK_LIST_SCROLL_FOCUS, NULL);
    }
  }
  return 0;
}

static int gtk4ListSetSpacingAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->is_dropdown)
    return 0;

  if (!iupStrToInt(value, &ih->data->spacing))
    ih->data->spacing = 0;

  if (ih->handle)
  {
    GtkCellRenderer* renderer = (GtkCellRenderer*)iupAttribGet(ih, "_IUPGTK4_RENDERER");
    if (renderer)
      g_object_set(G_OBJECT(renderer), "xpad", ih->data->spacing, "ypad", ih->data->spacing, NULL);
    return 0;
  }
  else
    return 1;
}

static int gtk4ListSetPaddingAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->has_editbox)
    return 0;

  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');
  if (ih->handle)
  {
    GtkEntry* entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
    iupgtk4SetMargin(GTK_WIDGET(entry), ih->data->horiz_padding, ih->data->vert_padding);
    return 0;
  }
  else
    return 1;
}

static int gtk4ListSetSelectionAttrib(Ihandle* ih, const char* value)
{
  int start=1, end=1;
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return 0;

  entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
    gtk_editable_select_region(GTK_EDITABLE(entry), 0, 0);
    return 0;
  }

  if (iupStrEqualNoCase(value, "ALL"))
  {
    gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1);
    return 0;
  }

  if (iupStrToIntInt(value, &start, &end, ':')!=2)
    return 0;

  if (start<1 || end<1)
    return 0;

  start--;
  end--;

  gtk_editable_select_region(GTK_EDITABLE(entry), start, end);

  return 0;
}

static char* gtk4ListGetSelectionAttrib(Ihandle* ih)
{
  int start, end;
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return NULL;

  entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
  if (gtk_editable_get_selection_bounds(GTK_EDITABLE(entry), &start, &end))
  {
    start++;
    end++;
    return iupStrReturnIntInt(start, end, ':');
  }

  return NULL;
}

static char* gtk4ListGetSelectionPosAttrib(Ihandle* ih)
{
  int start, end;
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return NULL;

  entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
  if (gtk_editable_get_selection_bounds(GTK_EDITABLE(entry), &start, &end))
    return iupStrReturnIntInt(start, end, ':');

  return NULL;
}

static int gtk4ListSetSelectionPosAttrib(Ihandle* ih, const char* value)
{
  int start=0, end=0;
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return 0;

  entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
    gtk_editable_select_region(GTK_EDITABLE(entry), 0, 0);
    return 0;
  }

  if (iupStrEqualNoCase(value, "ALL"))
  {
    gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1);
    return 0;
  }

  if (iupStrToIntInt(value, &start, &end, ':')!=2)
    return 0;

  if (start<0 || end<0)
    return 0;

  gtk_editable_select_region(GTK_EDITABLE(entry), start, end);

  return 0;
}

static char* gtk4ListGetSelectedTextAttrib(Ihandle* ih)
{
  int start, end;
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return NULL;

  entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
  if (gtk_editable_get_selection_bounds(GTK_EDITABLE(entry), &start, &end))
  {
    const char* full_text = gtk_editable_get_text(GTK_EDITABLE(entry));
    char* selectedtext = iupStrDup(full_text);
    selectedtext[end] = 0;
    char* str = iupStrReturnStr(iupgtk4StrConvertFromSystem(selectedtext + start));
    free(selectedtext);
    return str;
  }

  return NULL;
}

static int gtk4ListSetCaretAttrib(Ihandle* ih, const char* value)
{
  int pos = 1;
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return 0;
  if (!value)
    return 0;

  iupStrToInt(value, &pos);
  pos--;
  if (pos < 0) pos = 0;

  entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
  gtk_editable_set_position(GTK_EDITABLE(entry), pos);

  return 0;
}

static char* gtk4ListGetCaretAttrib(Ihandle* ih)
{
  if (ih->data->has_editbox)
  {
    GtkEntry* entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
    int pos = gtk_editable_get_position(GTK_EDITABLE(entry));
    pos++;
    return iupStrReturnInt((int)pos);
  }
  else
    return NULL;
}

static int gtk4ListSetCaretPosAttrib(Ihandle* ih, const char* value)
{
  int pos = 0;
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return 0;
  if (!value)
    return 0;

  iupStrToInt(value, &pos);
  if (pos < 0) pos = 0;

  entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
  gtk_editable_set_position(GTK_EDITABLE(entry), pos);

  return 0;
}

static char* gtk4ListGetCaretPosAttrib(Ihandle* ih)
{
  if (ih->data->has_editbox)
  {
    GtkEntry* entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
    int pos = gtk_editable_get_position(GTK_EDITABLE(entry));
    return iupStrReturnInt((int)pos);
  }
  else
    return NULL;
}

static int gtk4ListSetScrollToAttrib(Ihandle* ih, const char* value)
{
  int pos = 1;
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return 0;
  if (!value)
    return 0;

  iupStrToInt(value, &pos);
  if (pos < 1) pos = 1;
  pos--;

  entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
  gtk_editable_set_position(GTK_EDITABLE(entry), pos);

  return 0;
}

static int gtk4ListSetScrollToPosAttrib(Ihandle* ih, const char* value)
{
  int pos = 0;
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return 0;
  if (!value)
    return 0;

  iupStrToInt(value, &pos);
  if (pos < 0) pos = 0;

  entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
  gtk_editable_set_position(GTK_EDITABLE(entry), pos);

  return 0;
}

static int gtk4ListSetInsertAttrib(Ihandle* ih, const char* value)
{
  int pos;
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return 0;
  if (!value)
    return 0;

  iupAttribSet(ih, "_IUPGTK4_DISABLE_TEXT_CB", "1");
  entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
  pos = gtk_editable_get_position(GTK_EDITABLE(entry));
  gtk_editable_insert_text(GTK_EDITABLE(entry), iupgtk4StrConvertToSystem(value), -1, &pos);
  iupAttribSet(ih, "_IUPGTK4_DISABLE_TEXT_CB", NULL);

  return 0;
}

static int gtk4ListSetAppendAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->has_editbox)
  {
    GtkEntry* entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
    int pos = (int)strlen(gtk_editable_get_text(GTK_EDITABLE(entry)))+1;
    iupAttribSet(ih, "_IUPGTK4_DISABLE_TEXT_CB", "1");
    gtk_editable_insert_text(GTK_EDITABLE(entry), iupgtk4StrConvertToSystem(value), -1, &pos);
    iupAttribSet(ih, "_IUPGTK4_DISABLE_TEXT_CB", NULL);
  }
  return 0;
}

static int gtk4ListSetNCAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->has_editbox)
    return 0;

  if (!iupStrToInt(value, &ih->data->nc))
    ih->data->nc = INT_MAX;

  if (ih->handle)
  {
    GtkEntry* entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
    gtk_entry_set_max_length(entry, ih->data->nc);
    return 0;
  }
  else
    return 1;
}

static int gtk4ListSetClipboardAttrib(Ihandle *ih, const char *value)
{
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return 0;

  iupAttribSet(ih, "_IUPGTK4_DISABLE_TEXT_CB", "1");
  entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
  if (iupStrEqualNoCase(value, "COPY"))
  {
    GdkClipboard* clipboard = gtk_widget_get_clipboard((GtkWidget*)entry);
    gdk_clipboard_set_text(clipboard, gtk_editable_get_text(GTK_EDITABLE(entry)));
  }
  else if (iupStrEqualNoCase(value, "CUT"))
  {
    GdkClipboard* clipboard = gtk_widget_get_clipboard((GtkWidget*)entry);
    gdk_clipboard_set_text(clipboard, gtk_editable_get_text(GTK_EDITABLE(entry)));
    gtk_editable_delete_selection(GTK_EDITABLE(entry));
  }
  else if (iupStrEqualNoCase(value, "PASTE"))
  {
    GdkClipboard* clipboard = gtk_widget_get_clipboard((GtkWidget*)entry);
    gdk_clipboard_read_text_async(clipboard, NULL, NULL, NULL);
  }
  else if (iupStrEqualNoCase(value, "CLEAR"))
    gtk_editable_delete_selection(GTK_EDITABLE(entry));
  iupAttribSet(ih, "_IUPGTK4_DISABLE_TEXT_CB", NULL);
  return 0;
}

static int gtk4ListSetReadOnlyAttrib(Ihandle* ih, const char* value)
{
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return 0;
  entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
  gtk_editable_set_editable(GTK_EDITABLE(entry), !iupStrBoolean(value));
  return 0;
}

static char* gtk4ListGetReadOnlyAttrib(Ihandle* ih)
{
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return NULL;
  entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
  return iupStrReturnBoolean(!gtk_editable_get_editable(GTK_EDITABLE(entry)));
}

static int gtk4ListSetImageAttrib(Ihandle* ih, int id, const char* value)
{
  GListStore* store = gtk4ListGetGListStore(ih);
  GdkPaintable* paintable = (GdkPaintable*)iupImageGetImage(value, ih, 0, NULL);
  int pos = iupListGetPosAttrib(ih, id);

  if (store)
  {
    IupListItem* item = g_list_model_get_item(G_LIST_MODEL(store), pos);
    if (!item)
      return 0;
    if (paintable)
    {
      GdkTexture* texture = GDK_TEXTURE(paintable);
      iup_list_item_set_image(item, texture);
    }
    else
    {
      iup_list_item_set_image(item, NULL);
    }

    g_list_model_items_changed(G_LIST_MODEL(store), pos, 1, 1);

    g_object_unref(item);
    return 1;
  }

  return 0;
}

static void gtk4ListEditMoveCursor(GtkWidget* entry, GtkMovementStep step, gint count, gboolean extend_selection, Ihandle* ih)
{
  int pos;

  IFniii cb = (IFniii)IupGetCallback(ih, "CARET_CB");
  if (!cb) return;

  pos = gtk_editable_get_position(GTK_EDITABLE(entry));

  if (pos != ih->data->last_caret_pos)
  {
    ih->data->last_caret_pos = pos;

    cb(ih, 1, pos+1, pos);
  }

  (void)step;
  (void)count;
  (void)extend_selection;
}

static gboolean gtk4ListEditKeyPressEvent(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, Ihandle *ih)
{
  GtkWidget* entry = (GtkWidget*)iupAttribGet(ih, "_IUPGTK4_ENTRY");

  if (iupgtk4KeyPressEvent(controller, keyval, keycode, state, ih) == TRUE)
  {
    return TRUE;
  }

  if ((keyval == GDK_KEY_Up || keyval == GDK_KEY_KP_Up) ||
      (keyval == GDK_KEY_Prior || keyval == GDK_KEY_KP_Page_Up) ||
      (keyval == GDK_KEY_Down || keyval == GDK_KEY_KP_Down) ||
      (keyval == GDK_KEY_Next || keyval == GDK_KEY_KP_Page_Down))
  {
    GListStore* store = gtk4ListGetGListStore(ih);

    if (store)
    {
      int pos = -1;
      GtkSelectionModel* selection = gtk4ListGetSelectionModel(ih);
      guint selected = gtk_single_selection_get_selected(GTK_SINGLE_SELECTION(selection));

      if (selected != GTK_INVALID_LIST_POSITION)
        pos = (int)selected;

      if (pos == -1)
        pos = 0;
      else if (keyval == GDK_KEY_Up || keyval == GDK_KEY_KP_Up)
      {
        pos--;
        if (pos < 0) pos = 0;
      }
      else if (keyval == GDK_KEY_Prior || keyval == GDK_KEY_KP_Page_Up)
      {
        pos -= 5;
        if (pos < 0) pos = 0;
      }
      else if (keyval == GDK_KEY_Down || keyval == GDK_KEY_KP_Down)
      {
        int count = iupdrvListGetCount(ih);
        pos++;
        if (pos > count-1) pos = count-1;
      }
      else if (keyval == GDK_KEY_Next || keyval == GDK_KEY_KP_Page_Down)
      {
        int count = iupdrvListGetCount(ih);
        pos += 5;
        if (pos > count-1) pos = count-1;
      }

      if (pos != -1)
      {
        iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
        gtk_single_selection_set_selected(GTK_SINGLE_SELECTION(selection), pos);
        iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
        iupAttribSetInt(ih, "_IUPLIST_OLDVALUE", pos+1);

        IupListItem* item = g_list_model_get_item(G_LIST_MODEL(store), pos);
        if (item)
        {
          const char* text = iup_list_item_get_text(item);
          if (text)
            gtk_editable_set_text(GTK_EDITABLE(entry), text);
          g_object_unref(item);
        }
      }
    }
  }

  return FALSE;
}

static gboolean gtk4ListEditKeyReleaseEvent(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, Ihandle *ih)
{
  GtkWidget* entry = (GtkWidget*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
  gtk4ListEditMoveCursor(entry, 0, 0, 0, ih);
  (void)controller;
  (void)keyval;
  (void)keycode;
  (void)state;
  return FALSE;
}

static void gtk4ListEditButtonPressed(GtkGestureClick *gesture, int n_press, double x, double y, Ihandle *ih)
{
  GtkWidget* entry = (GtkWidget*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
  gtk4ListEditMoveCursor(entry, 0, 0, 0, ih);
  (void)gesture;
  (void)n_press;
  (void)x;
  (void)y;
}

static void gtk4ListEditDeleteText(GtkEditable *editable, int start, int end, Ihandle* ih)
{
  IFnis cb = (IFnis)IupGetCallback(ih, "EDIT_CB");
  int ret;

  if (iupAttribGet(ih, "_IUPGTK4_DISABLE_TEXT_CB"))
    return;

  ret = iupEditCallActionCb(ih, cb, NULL, start, end, ih->data->mask, ih->data->nc, 1, 1);
  if (ret == 0)
    g_signal_stop_emission_by_name(editable, "delete_text");
}

static void gtk4ListEditInsertText(GtkEditable *editable, char *insert_value, int len, int *pos, Ihandle* ih)
{
  IFnis cb = (IFnis)IupGetCallback(ih, "EDIT_CB");
  int ret;

  if (iupAttribGet(ih, "_IUPGTK4_DISABLE_TEXT_CB"))
    return;

  ret = iupEditCallActionCb(ih, cb, iupgtk4StrConvertFromSystem(insert_value), *pos, *pos, ih->data->mask, ih->data->nc, 0, 1);
  if (ret == 0)
    g_signal_stop_emission_by_name(editable, "insert_text");
  else if (ret != -1)
  {
    insert_value[0] = (char)ret;

    iupAttribSet(ih, "_IUPGTK4_DISABLE_TEXT_CB", "1");
    gtk_editable_insert_text(editable, insert_value, 1, pos);
    iupAttribSet(ih, "_IUPGTK4_DISABLE_TEXT_CB", NULL);

    g_signal_stop_emission_by_name(editable, "insert_text");
  }

  (void)len;
}

static void gtk4ListEditChanged(void* dummy, Ihandle* ih)
{
  if (iupAttribGet(ih, "_IUPGTK4_DISABLE_TEXT_CB"))
    return;

  iupBaseCallValueChangedCb(ih);
  (void)dummy;
}

static gboolean gtk4ListSimpleKeyPressEvent(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, Ihandle *ih)
{
  /* First, propagate to parent callbacks (K_ANY, etc.) */
  if (iupgtk4KeyPressEvent(controller, keyval, keycode, state, ih) == TRUE)
    return TRUE;

  if (keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter)
    return TRUE;
  return FALSE;
}

static void gtk4ListFactory_setup(GtkListItemFactory* factory, GtkListItem* list_item, gpointer user_data)
{
  GtkWidget* box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  GtkWidget* image = gtk_image_new();
  GtkWidget* label = gtk_label_new("");

  gtk_label_set_xalign(GTK_LABEL(label), 0.0);
  gtk_widget_set_hexpand(label, TRUE);

  gtk_widget_set_valign(image, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(label, GTK_ALIGN_CENTER);

  gtk_box_append(GTK_BOX(box), image);
  gtk_box_append(GTK_BOX(box), label);

  gtk_list_item_set_child(list_item, box);

  (void)factory;
  (void)user_data;
}

static void gtk4ListItem_onPropertyUpdate(GObject* object, GParamSpec* pspec, gpointer user_data)
{
  IupListItem* item = IUP_LIST_ITEM(object);
  GtkListItem* list_item = GTK_LIST_ITEM(user_data);

  if (!list_item || !item)
    return;

  /* Update widgets based on current item properties */
  GtkWidget* box = gtk_list_item_get_child(list_item);
  if (!box)
    return;

  GtkWidget* image = gtk_widget_get_first_child(box);
  GtkWidget* label = gtk_widget_get_last_child(box);

  const char* text = iup_list_item_get_text(item);
  gtk_label_set_text(GTK_LABEL(label), text ? text : "");

  GdkTexture* texture = iup_list_item_get_image(item);
  if (texture)
  {
    gtk_image_set_from_paintable(GTK_IMAGE(image), GDK_PAINTABLE(texture));
    gtk_widget_set_visible(image, TRUE);
  }
  else
  {
    gtk_image_clear(GTK_IMAGE(image));
    gtk_widget_set_visible(image, FALSE);
  }

  gboolean enabled = iup_list_item_get_enabled(item);
  gtk_widget_set_sensitive(box, enabled);

  (void)pspec;
}

static void gtk4ListFactory_bind(GtkListItemFactory* factory, GtkListItem* list_item, gpointer user_data)
{
  Ihandle* ih = (Ihandle*)user_data;
  IupListItem* item = IUP_LIST_ITEM(gtk_list_item_get_item(list_item));

  if (!item)
    return;

  GtkWidget* box = gtk_list_item_get_child(list_item);
  GtkWidget* image = gtk_widget_get_first_child(box);
  GtkWidget* label = gtk_widget_get_last_child(box);

  const char* text = iup_list_item_get_text(item);
  gtk_label_set_text(GTK_LABEL(label), text ? text : "");

  GdkTexture* texture = iup_list_item_get_image(item);
  if (texture)
  {
    gtk_image_set_from_paintable(GTK_IMAGE(image), GDK_PAINTABLE(texture));
    gtk_widget_set_visible(image, TRUE);
  }
  else
  {
    gtk_image_clear(GTK_IMAGE(image));
    gtk_widget_set_visible(image, FALSE);
  }

  gboolean enabled = iup_list_item_get_enabled(item);
  gtk_widget_set_sensitive(box, enabled);

  /* Store position on the box widget for drag-drop XY-to-position conversion
     Store position+1 to avoid storing 0 (NULL pointer) which would fail in retrieval checks */
  guint position = gtk_list_item_get_position(list_item);
  g_object_set_data(G_OBJECT(box), "iup-list-position", GUINT_TO_POINTER(position + 1));

  /* Connect to item's property update signal to handle dynamic changes */
  gulong handler_id = g_signal_connect(item, "notify::update", G_CALLBACK(gtk4ListItem_onPropertyUpdate), list_item);
  g_object_set_data(G_OBJECT(item), "iup-notify-handler", (gpointer)handler_id);

  (void)factory;
}

static void gtk4ListFactory_unbind(GtkListItemFactory* factory, GtkListItem* list_item, gpointer user_data)
{
  IupListItem* item = IUP_LIST_ITEM(gtk_list_item_get_item(list_item));

  if (item)
  {
    /* Disconnect property update signal */
    gulong handler_id = (gulong)g_object_get_data(G_OBJECT(item), "iup-notify-handler");
    if (handler_id)
    {
      g_signal_handler_disconnect(item, handler_id);
      g_object_set_data(G_OBJECT(item), "iup-notify-handler", NULL);
    }
  }

  (void)factory;
  (void)user_data;
}

static GtkListItemFactory* gtk4ListCreateFactory(Ihandle* ih)
{
  GtkListItemFactory* factory = gtk_signal_list_item_factory_new();

  g_signal_connect(factory, "setup", G_CALLBACK(gtk4ListFactory_setup), ih);
  g_signal_connect(factory, "bind", G_CALLBACK(gtk4ListFactory_bind), ih);
  g_signal_connect(factory, "unbind", G_CALLBACK(gtk4ListFactory_unbind), ih);

  return factory;
}

static void gtk4ListViewSelectionChanged(GtkSelectionModel* selection, guint position, guint n_items, gpointer user_data)
{
  Ihandle* ih = (Ihandle*)user_data;
  IFnsii cb;
  IFns multi_cb;

  if (!ih || iupAttribGet(ih, "_IUPLIST_IGNORE_ACTION"))
    return;

  if (ih->data->has_editbox && !ih->data->is_multiple)
  {
    guint selected = gtk_single_selection_get_selected(GTK_SINGLE_SELECTION(selection));
    if (selected != GTK_INVALID_LIST_POSITION)
    {
      GListStore* store = gtk4ListGetGListStore(ih);
      if (store)
      {
        IupListItem* item = g_list_model_get_item(G_LIST_MODEL(store), selected);
        if (item)
        {
          const char* text = iup_list_item_get_text(item);
          GtkEntry* entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
          if (entry && text)
          {
            iupAttribSet(ih, "_IUPGTK4_DISABLE_TEXT_CB", "1");
            gtk_editable_set_text(GTK_EDITABLE(entry), text);
            iupAttribSet(ih, "_IUPGTK4_DISABLE_TEXT_CB", NULL);
          }
          g_object_unref(item);
        }
      }
    }
  }

  cb = (IFnsii)IupGetCallback(ih, "ACTION");
  multi_cb = (IFns)IupGetCallback(ih, "MULTISELECT_CB");

  if (multi_cb || cb)
  {
    if (multi_cb && ih->data->is_multiple)
    {
      GtkBitset* bitset = gtk_selection_model_get_selection(selection);
      guint64 size = g_list_model_get_n_items(G_LIST_MODEL(gtk4ListGetGListStore(ih)));
      int* pos = malloc(size * sizeof(int));
      int sel_count = 0;

      for (guint64 i = 0; i < size; i++)
      {
        if (gtk_bitset_contains(bitset, i))
        {
          pos[sel_count] = (int)i;
          sel_count++;
        }
      }

      iupListMultipleCallActionCb(ih, cb, multi_cb, pos, sel_count);
      free(pos);
      gtk_bitset_unref(bitset);
    }
    else if (cb && !ih->data->is_multiple)
    {
      guint selected = gtk_single_selection_get_selected(GTK_SINGLE_SELECTION(selection));
      if (selected != GTK_INVALID_LIST_POSITION)
      {
        int pos = (int)selected + 1;
        iupListSingleCallActionCb(ih, cb, pos);
      }
    }
  }

  if (!ih->data->has_editbox)
    iupBaseCallValueChangedCb(ih);

  (void)position;
  (void)n_items;
}

static void gtk4ListViewItemActivated(GtkListView* list_view, guint position, gpointer user_data)
{
  Ihandle* ih = (Ihandle*)user_data;
  IFnis cb = (IFnis)IupGetCallback(ih, "DBLCLICK_CB");

  if (cb)
  {
    IupListItem* item = g_list_model_get_item(G_LIST_MODEL(gtk4ListGetGListStore(ih)), position);

    if (item)
    {
      const char* text = iup_list_item_get_text(item);
      cb(ih, (int)position + 1, (char*)text);
      g_object_unref(item);
    }
  }

  (void)list_view;
}

static void gtk4ListUpdateMinSize(Ihandle* ih)
{
  if (!ih->data->is_dropdown)
  {
    GtkScrolledWindow* scrolled_window = (GtkScrolledWindow*)iupAttribGet(ih, "_IUPGTK4_SCROLLED_WINDOW");
    if (!scrolled_window)
    {
      scrolled_window = (GtkScrolledWindow*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
      if (!scrolled_window || !GTK_IS_SCROLLED_WINDOW(scrolled_window))
        return;
    }

    int natural_w = 0, natural_h = 0;
    int char_width, char_height;
    int visiblelines, item_count;
    int visiblecolumns;

    iupdrvFontGetCharSize(ih, &char_width, &char_height);

    visiblecolumns = iupAttribGetInt(ih, "VISIBLECOLUMNS");
    if (visiblecolumns)
    {
      natural_w = iupdrvFontGetStringWidth(ih, "WWWWWWWWWW");
      natural_w = (visiblecolumns * natural_w) / 10;
    }
    else
    {
      item_count = iupdrvListGetCount(ih);
      for (int i = 1; i <= item_count; i++)
      {
        char* value = IupGetAttributeId(ih, "", i);
        if (value)
        {
          int item_w = iupdrvFontGetStringWidth(ih, value);
          if (item_w > natural_w)
            natural_w = item_w;
        }
      }

      if (natural_w == 0)
        natural_w = iupdrvFontGetStringWidth(ih, "WWWWW");
    }

    natural_w += 2 * ih->data->spacing;

    item_count = iupdrvListGetCount(ih);
    if (item_count == 0) item_count = 1;

    visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
    if (visiblelines)
      item_count = visiblelines;

    natural_h = char_height;
    iupdrvListAddItemSpace(ih, &natural_h);
    natural_h += 2 * ih->data->spacing;
    natural_h = natural_h * item_count;

    iupdrvListAddBorders(ih, &natural_w, &natural_h);

    if (ih->data->sb)
    {
      int sb_size = iupdrvGetScrollbarSize();
      natural_w += sb_size;  /* vertical scrollbar affects horizontal size only */
    }

    if (visiblelines > 0)
    {
      /* Calculate content height (item height * visible lines) without borders */
      int content_h = char_height;
      iupdrvListAddItemSpace(ih, &content_h);
      content_h += 2 * ih->data->spacing;
      content_h = content_h * visiblelines;
    }

    if (!ih->data->has_editbox && visiblelines == 0)
    {
      /* Subtract frame border because min_content is for content area INSIDE the frame
         iupdrvListAddBorders added 2 pixels for the frame, but min_content should not include it */
      gtk_scrolled_window_set_min_content_height(scrolled_window, natural_h - 2);
      gtk_scrolled_window_set_min_content_width(scrolled_window, natural_w - 2);
    }

    {
      int actual_min_w, actual_nat_w, actual_min_h, actual_nat_h;
      gtk_widget_measure(GTK_WIDGET(scrolled_window), GTK_ORIENTATION_HORIZONTAL, -1, &actual_min_w, &actual_nat_w, NULL, NULL);
      gtk_widget_measure(GTK_WIDGET(scrolled_window), GTK_ORIENTATION_VERTICAL, -1, &actual_min_h, &actual_nat_h, NULL, NULL);
    }
  }
  else
  {
    int natural_w = 0, natural_h = 0;
    int char_width, char_height;
    int visiblecolumns, item_count;
    int sb_size = iupdrvGetScrollbarSize();

    iupdrvFontGetCharSize(ih, &char_width, &char_height);

    visiblecolumns = iupAttribGetInt(ih, "VISIBLECOLUMNS");
    if (visiblecolumns)
    {
      natural_w = iupdrvFontGetStringWidth(ih, "WWWWWWWWWW");
      natural_w = (visiblecolumns * natural_w) / 10;
    }
    else
    {
      item_count = iupdrvListGetCount(ih);
      for (int i = 1; i <= item_count; i++)
      {
        char* value = IupGetAttributeId(ih, "", i);
        if (value)
        {
          int item_w = iupdrvFontGetStringWidth(ih, value);
          if (item_w > natural_w)
            natural_w = item_w;
        }
      }

      if (natural_w == 0)
        natural_w = iupdrvFontGetStringWidth(ih, "WWWWW");
    }

    natural_h = char_height;

    iupdrvListAddBorders(ih, &natural_w, &natural_h);

    natural_w += sb_size;
    if (natural_h < sb_size)
      natural_h = sb_size;

    if (ih->data->has_editbox)
    {
      natural_w += 2 * ih->data->horiz_padding;
      natural_h += 2 * ih->data->vert_padding;
    }

    gtk_widget_set_size_request(ih->handle, natural_w, natural_h);

    {
      int actual_min_w, actual_nat_w, actual_min_h, actual_nat_h;
      gtk_widget_measure(ih->handle, GTK_ORIENTATION_HORIZONTAL, -1, &actual_min_w, &actual_nat_w, NULL, NULL);
      gtk_widget_measure(ih->handle, GTK_ORIENTATION_VERTICAL, -1, &actual_min_h, &actual_nat_h, NULL, NULL);
    }
  }
}

static void gtk4DropDownSelectionChanged(GObject* object, GParamSpec* pspec, gpointer user_data)
{
  Ihandle* ih = (Ihandle*)user_data;
  GtkDropDown* dropdown = GTK_DROP_DOWN(object);
  IFnsii cb;

  if (iupAttribGet(ih, "_IUPLIST_IGNORE_ACTION"))
    return;

  if (ih->data->has_editbox)
  {
    guint selected = gtk_drop_down_get_selected(dropdown);
    if (selected != GTK_INVALID_LIST_POSITION)
    {
      GListStore* store = gtk4ListGetGListStore(ih);
      if (store)
      {
        IupListItem* item = g_list_model_get_item(G_LIST_MODEL(store), selected);
        if (item)
        {
          const char* text = iup_list_item_get_text(item);
          GtkEntry* entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
          if (entry && text)
          {
            iupAttribSet(ih, "_IUPGTK4_DISABLE_TEXT_CB", "1");
            gtk_editable_set_text(GTK_EDITABLE(entry), text);
            iupAttribSet(ih, "_IUPGTK4_DISABLE_TEXT_CB", NULL);
          }
          g_object_unref(item);
        }
      }
    }
  }

  cb = (IFnsii)IupGetCallback(ih, "ACTION");
  if (cb)
  {
    guint selected = gtk_drop_down_get_selected(dropdown);

    if (selected != GTK_INVALID_LIST_POSITION)
    {
      int pos = (int)selected + 1;
      iupListSingleCallActionCb(ih, cb, pos);
    }
  }

  if (!ih->data->has_editbox)
    iupBaseCallValueChangedCb(ih);

  (void)pspec;
}

static gboolean gtk4ListDebugAllocation(gpointer user_data)
{
  Ihandle* ih = (Ihandle*)user_data;
  GtkScrolledWindow* sw = (GtkScrolledWindow*)iupAttribGet(ih, "_IUPGTK4_DEBUG_LIST_SW");
  if (sw && ih->handle)
  {
    int sw_w = gtk_widget_get_width(GTK_WIDGET(sw));
    int sw_h = gtk_widget_get_height(GTK_WIDGET(sw));
    int lv_w = gtk_widget_get_width(ih->handle);
    int lv_h = gtk_widget_get_height(ih->handle);

    int char_width, char_height;
    iupdrvFontGetCharSize(ih, &char_width, &char_height);
    int item_height = char_height;
    iupdrvListAddItemSpace(ih, &item_height);

  }
  return G_SOURCE_REMOVE;
}

static int gtk4ListMapMethod(Ihandle* ih)
{
  GtkScrolledWindow* scrolled_window = NULL;
  GListStore* glist_store;
  GtkListItemFactory* factory;
  GtkSelectionModel* selection_model;

  glist_store = g_list_store_new(IUP_TYPE_LIST_ITEM);
  gtk4ListSetGListStore(ih, glist_store);

  factory = gtk4ListCreateFactory(ih);
  gtk4ListSetFactory(ih, factory);

  if (ih->data->is_dropdown)
  {
    selection_model = GTK_SELECTION_MODEL(gtk_single_selection_new(G_LIST_MODEL(glist_store)));

    if (ih->data->has_editbox)
    {
      g_object_set(selection_model, "autoselect", FALSE, "can-unselect", TRUE, NULL);
    }

    gtk4ListSetSelectionModel(ih, selection_model);

    ih->handle = (GtkWidget*)gtk_drop_down_new(G_LIST_MODEL(glist_store), NULL);

    if (!ih->handle)
      return IUP_ERROR;

    gtk_drop_down_set_factory(GTK_DROP_DOWN(ih->handle), factory);
    gtk_drop_down_set_list_factory(GTK_DROP_DOWN(ih->handle), factory);
    gtk_drop_down_set_enable_search(GTK_DROP_DOWN(ih->handle), ih->data->has_editbox ? TRUE : FALSE);

    {
      int natural_w = 0, natural_h = 0;
      int char_width, char_height;
      int visiblecolumns, item_count;
      int sb_size = iupdrvGetScrollbarSize();

      iupdrvFontGetCharSize(ih, &char_width, &char_height);

      visiblecolumns = iupAttribGetInt(ih, "VISIBLECOLUMNS");
      if (visiblecolumns)
      {
        natural_w = iupdrvFontGetStringWidth(ih, "WWWWWWWWWW");
        natural_w = (visiblecolumns * natural_w) / 10;
      }
      else
      {
        item_count = iupdrvListGetCount(ih);
        for (int i = 1; i <= item_count; i++)
        {
          char* value = IupGetAttributeId(ih, "", i);
          if (value)
          {
            int item_w = iupdrvFontGetStringWidth(ih, value);
            if (item_w > natural_w)
              natural_w = item_w;
          }
        }

        if (natural_w == 0)
          natural_w = iupdrvFontGetStringWidth(ih, "WWWWW");
      }

      natural_h = char_height;

      iupdrvListAddBorders(ih, &natural_w, &natural_h);

      natural_w += sb_size;
      if (natural_h < sb_size)
        natural_h = sb_size;

      if (ih->data->has_editbox)
      {
        natural_w += 2 * ih->data->horiz_padding;
        natural_h += 2 * ih->data->vert_padding;
      }

      /* Apply calculated natural size to dropdown widget */
      gtk_widget_set_size_request(ih->handle, natural_w, natural_h);

      /* Store natural_w for entry widget sizing below */
      if (ih->data->has_editbox)
        iupAttribSetInt(ih, "_IUP_DROPDOWN_NATURAL_W", natural_w);
    }

    if (ih->data->has_editbox)
    {
      GtkWidget *entry = gtk_entry_new();
      iupAttribSet(ih, "_IUPGTK4_ENTRY", (char*)entry);

      /* Apply same CSS as IupText single-line to reduce padding */
      {
        GtkStyleContext *context = gtk_widget_get_style_context(entry);
        GtkCssProvider *provider = gtk_css_provider_new();
        gtk_css_provider_load_from_string(GTK_CSS_PROVIDER(provider),
          "*{ padding-left: 2px; padding-right: 2px; min-height: 24px; }");
        gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        g_object_unref(provider);
      }

      /* Measure GtkEntry's natural width with the same text width as dropdown */
      int entry_width = iupAttribGetInt(ih, "_IUP_DROPDOWN_NATURAL_W");
      if (entry_width > 0)
      {
        /* GtkEntry needs extra space for borders/padding compared to list */
        static int entry_extra_space = -1;

        if (entry_extra_space == -1)
        {
          /* Measure GtkEntry borders by creating a temp entry with CSS */
          GtkWidget* temp_entry = gtk_entry_new();

          /* Apply same CSS to get accurate measurements */
          {
            GtkStyleContext *context = gtk_widget_get_style_context(temp_entry);
            GtkCssProvider *provider = gtk_css_provider_new();
            gtk_css_provider_load_from_string(GTK_CSS_PROVIDER(provider),
              "*{ padding-left: 2px; padding-right: 2px; min-height: 24px; }");
            gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
            g_object_unref(provider);
          }

          gtk_editable_set_text(GTK_EDITABLE(temp_entry), "WWWWWWWWWW");

          int min_w, nat_w;
          gtk_widget_measure(temp_entry, GTK_ORIENTATION_HORIZONTAL, -1, &min_w, &nat_w, NULL, NULL);

          /* Calculate extra space: measured width - text width */
          int text_w = iupdrvFontGetStringWidth(ih, "WWWWWWWWWW");
          entry_extra_space = (nat_w > text_w) ? (nat_w - text_w) : 10;

          g_object_ref_sink(temp_entry);
          g_object_unref(temp_entry);
        }

        /* Add entry's extra space to the dropdown width */
        int entry_total_width = entry_width + entry_extra_space;
        gtk_widget_set_size_request(entry, entry_total_width, -1);

        /* Store total width for vbox sizing below */
        iupAttribSetInt(ih, "_IUP_ENTRY_TOTAL_WIDTH", entry_total_width);
      }

      GtkBox* vbox = (GtkBox*)gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
      gtk_box_set_homogeneous(vbox, FALSE);
      gtk_widget_set_vexpand((GtkWidget*)vbox, FALSE);
      gtk_widget_set_hexpand((GtkWidget*)vbox, FALSE);

      gtk_widget_set_vexpand(entry, FALSE);
      gtk_widget_set_valign(entry, GTK_ALIGN_CENTER);
      iupgtk4ClearSizeStyleCSS(entry);

      gtk_box_append(vbox, entry);
      gtk_box_append(vbox, ih->handle);

      iupAttribSet(ih, "_IUP_EXTRAPARENT", (char*)vbox);

      iupgtk4SetupEnterLeaveEvents(entry, ih);
      iupgtk4SetupFocusEvents(entry, ih);
      iupgtk4SetupKeyEvents(entry, ih);

      GtkEventController* key_controller = gtk_event_controller_key_new();
      gtk_widget_add_controller(entry, key_controller);
      g_signal_connect(key_controller, "key-pressed", G_CALLBACK(gtk4ListEditKeyPressEvent), ih);
      g_signal_connect(key_controller, "key-released", G_CALLBACK(gtk4ListEditKeyReleaseEvent), ih);

      GtkGesture* click_gesture = gtk_gesture_click_new();
      gtk_widget_add_controller(entry, GTK_EVENT_CONTROLLER(click_gesture));
      g_signal_connect(click_gesture, "pressed", G_CALLBACK(gtk4ListEditButtonPressed), ih);

      g_signal_connect(G_OBJECT(entry), "delete-text", G_CALLBACK(gtk4ListEditDeleteText), ih);
      g_signal_connect(G_OBJECT(entry), "insert-text", G_CALLBACK(gtk4ListEditInsertText), ih);

      if (!iupAttribGetBoolean(ih, "CANFOCUS"))
      {
        iupgtk4SetCanFocus(ih->handle, 0);
        iupgtk4SetCanFocus(entry, 0);
      }
    }
    else
    {
      iupgtk4SetupFocusEvents(ih->handle, ih);
      iupgtk4SetupEnterLeaveEvents(ih->handle, ih);
      iupgtk4SetupKeyEvents(ih->handle, ih);

      if (!iupAttribGetBoolean(ih, "CANFOCUS"))
        iupgtk4SetCanFocus(ih->handle, 0);
    }

    g_signal_connect(ih->handle, "notify::selected", G_CALLBACK(gtk4DropDownSelectionChanged), ih);
  }
  else
  {
    GtkPolicyType scrollbar_policy;

    if (!ih->data->has_editbox && ih->data->is_multiple)
      selection_model = GTK_SELECTION_MODEL(gtk_multi_selection_new(G_LIST_MODEL(glist_store)));
    else
    {
      selection_model = GTK_SELECTION_MODEL(gtk_single_selection_new(G_LIST_MODEL(glist_store)));

      if (ih->data->has_editbox)
      {
        g_object_set(selection_model, "autoselect", FALSE, "can-unselect", TRUE, NULL);
      }
    }

    gtk4ListSetSelectionModel(ih, selection_model);

    ih->handle = (GtkWidget*)gtk_list_view_new(selection_model, factory);

    if (!ih->handle)
      return IUP_ERROR;

    gtk_list_view_set_show_separators(GTK_LIST_VIEW(ih->handle), FALSE);
    gtk_list_view_set_single_click_activate(GTK_LIST_VIEW(ih->handle), FALSE);

    scrolled_window = (GtkScrolledWindow*)gtk_scrolled_window_new();

    /* Force scrolled_window to request correct height for VISIBLELINES */
    {
      int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
      if (visiblelines > 0 && !ih->data->has_editbox)
      {
        int char_width, char_height;
        iupdrvFontGetCharSize(ih, &char_width, &char_height);

        int content_h = char_height;
        iupdrvListAddItemSpace(ih, &content_h);
        content_h += 2 * ih->data->spacing;
        content_h = content_h * visiblelines;

        /* Total height: content + CSS border compensation (2px) */
        /* GTK4 scrolled_window has NO internal overhead - only external CSS border */
        int total_h = content_h + 2;

        /* Set size_request on scrolled_window - CSS will take 2px, leaving exact content size for child */
        gtk_widget_set_size_request(GTK_WIDGET(scrolled_window), -1, total_h);
        gtk_widget_set_vexpand(GTK_WIDGET(scrolled_window), FALSE);
        gtk_widget_set_valign(GTK_WIDGET(scrolled_window), GTK_ALIGN_START);

        /* Mark scrolled_window so layout manager doesn't enforce GTK's scrollbar minimum */
        g_object_set_data(G_OBJECT(scrolled_window), "iup-visiblelines-set", (gpointer)"1");

        /* Store pointers for idle callback */
        iupAttribSet(ih, "_IUPGTK4_DEBUG_LIST_SW", (char*)scrolled_window);
        g_idle_add(gtk4ListDebugAllocation, ih);
      }
    }

    if (ih->data->has_editbox)
    {
      int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
      GtkWidget* container;

      /* For VISIBLELINES, use GtkFixed to avoid GtkBox enforcing scrolled_window's 46px minimum */
      if (visiblelines > 0)
      {
        container = gtk_fixed_new();
      }
      else
      {
        container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_box_set_homogeneous(GTK_BOX(container), FALSE);
      }

      gtk_widget_set_vexpand(container, FALSE);
      gtk_widget_set_hexpand(container, FALSE);

      GtkWidget *entry = gtk_entry_new();
      gtk_widget_set_vexpand(entry, FALSE);
      gtk_widget_set_hexpand(entry, FALSE);
      gtk_widget_set_valign(entry, GTK_ALIGN_CENTER);
      iupgtk4ClearSizeStyleCSS(entry);

      /* Apply same CSS as IupText single-line to reduce padding */
      {
        GtkStyleContext *context = gtk_widget_get_style_context(entry);
        GtkCssProvider *provider = gtk_css_provider_new();
        gtk_css_provider_load_from_string(GTK_CSS_PROVIDER(provider),
          "*{ padding-left: 2px; padding-right: 2px; min-height: 24px; }");
        gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        g_object_unref(provider);
      }

      iupAttribSet(ih, "_IUPGTK4_ENTRY", (char*)entry);

      if (visiblelines > 0)
      {
        /* VISIBLELINES: Use GtkFixed for exact positioning */

        /* Set size_request for scrolled_window */
        int char_width, char_height;
        iupdrvFontGetCharSize(ih, &char_width, &char_height);

        int content_h = char_height;
        iupdrvListAddItemSpace(ih, &content_h);
        content_h += 2 * ih->data->spacing;
        content_h = content_h * visiblelines;

        /* Total height: content + CSS border compensation (2px) */
        /* GTK4 scrolled_window has NO internal overhead - only external CSS border */
        int scrolled_window_frame = 2;  /* CSS border, same as plain list and multiline text */
        int total_h = content_h + scrolled_window_frame;

        /* Set height with size_request - width will be set later in LayoutUpdate when ih->currentwidth is known */
        gtk_widget_set_size_request(GTK_WIDGET(scrolled_window), -1, total_h);
        gtk_widget_set_vexpand(GTK_WIDGET(scrolled_window), FALSE);
        gtk_widget_set_valign(GTK_WIDGET(scrolled_window), GTK_ALIGN_START);

        /* Measure entry height for positioning */
        int entry_min_h, entry_nat_h;
        gtk_widget_measure(entry, GTK_ORIENTATION_VERTICAL, -1, &entry_min_h, &entry_nat_h, NULL, NULL);

        /* Position widgets in GtkFixed */
        /* Entry at top (0, 0) */
        gtk_fixed_put(GTK_FIXED(container), entry, 0, 0);

        /* Scrolled_window below entry */
        gtk_fixed_put(GTK_FIXED(container), GTK_WIDGET(scrolled_window), 0, entry_nat_h);

        /* Store entry natural height for LayoutUpdate */
        iupAttribSetInt(ih, "_IUPGTK4_ENTRY_HEIGHT", entry_nat_h);

        /* Mark container and scrolled_window so layout manager doesn't enforce GTK's minimums */
        g_object_set_data(G_OBJECT(container), "iup-visiblelines-set", (gpointer)"1");
        g_object_set_data(G_OBJECT(scrolled_window), "iup-visiblelines-set", (gpointer)"1");

        /* Store pointers for idle callback */
        iupAttribSet(ih, "_IUPGTK4_DEBUG_LIST_SW", (char*)scrolled_window);
        g_idle_add(gtk4ListDebugAllocation, ih);
      }
      else
      {
        /* Normal EDITBOX: Use GtkBox */
        gtk_box_append(GTK_BOX(container), entry);
        gtk_widget_set_vexpand((GtkWidget*)scrolled_window, TRUE);
        gtk_box_append(GTK_BOX(container), (GtkWidget*)scrolled_window);
      }

      iupAttribSet(ih, "_IUP_EXTRAPARENT", (char*)container);
      iupAttribSet(ih, "_IUPGTK4_SCROLLED_WINDOW", (char*)scrolled_window);

      iupgtk4SetCanFocus(ih->handle, 0);
      if (!iupAttribGetBoolean(ih, "CANFOCUS"))
        iupgtk4SetCanFocus(entry, 0);

      iupgtk4SetupEnterLeaveEvents(entry, ih);
      iupgtk4SetupFocusEvents(entry, ih);
      iupgtk4SetupKeyEvents(entry, ih);

      GtkEventController* key_controller = gtk_event_controller_key_new();
      gtk_widget_add_controller(entry, key_controller);
      g_signal_connect(key_controller, "key-pressed", G_CALLBACK(gtk4ListEditKeyPressEvent), ih);
      g_signal_connect(key_controller, "key-released", G_CALLBACK(gtk4ListEditKeyReleaseEvent), ih);

      GtkGesture* click_gesture = gtk_gesture_click_new();
      gtk_widget_add_controller(entry, GTK_EVENT_CONTROLLER(click_gesture));
      g_signal_connect(click_gesture, "pressed", G_CALLBACK(gtk4ListEditButtonPressed), ih);

      g_signal_connect(G_OBJECT(entry), "delete-text", G_CALLBACK(gtk4ListEditDeleteText), ih);
      g_signal_connect(G_OBJECT(entry), "insert-text", G_CALLBACK(gtk4ListEditInsertText), ih);
      g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(gtk4ListEditChanged), ih);
    }
    else
    {
      iupAttribSet(ih, "_IUP_EXTRAPARENT", (char*)scrolled_window);

      if (!iupAttribGetBoolean(ih, "CANFOCUS"))
        iupgtk4SetCanFocus(ih->handle, 0);

      iupgtk4SetupEnterLeaveEvents(ih->handle, ih);
      iupgtk4SetupFocusEvents(ih->handle, ih);

      GtkEventController* key_controller = gtk_event_controller_key_new();
      gtk_widget_add_controller(ih->handle, key_controller);
      g_signal_connect(key_controller, "key-pressed", G_CALLBACK(gtk4ListSimpleKeyPressEvent), ih);
    }

    gtk_scrolled_window_set_child(scrolled_window, ih->handle);
    gtk_scrolled_window_set_has_frame(scrolled_window, TRUE);

    if (ih->data->sb)
    {
      if (iupAttribGetBoolean(ih, "AUTOHIDE"))
        scrollbar_policy = GTK_POLICY_AUTOMATIC;
      else
        scrollbar_policy = GTK_POLICY_ALWAYS;
    }
    else
    {
      /* GTK4's max_content_height only works when scrollbar policy != NEVER
         So when VISIBLELINES is set, use AUTOMATIC to enable max_content_height clamping */
      int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
      if (visiblelines > 0)
        scrollbar_policy = GTK_POLICY_AUTOMATIC;
      else
        scrollbar_policy = GTK_POLICY_NEVER;
    }

    gtk_scrolled_window_set_policy(scrolled_window, scrollbar_policy, scrollbar_policy);

    {
      int natural_w = 0, natural_h = 0;
      int char_width, char_height;
      int visiblelines, item_count;
      int visiblecolumns;

      iupdrvFontGetCharSize(ih, &char_width, &char_height);

      visiblecolumns = iupAttribGetInt(ih, "VISIBLECOLUMNS");
      if (visiblecolumns)
      {
        natural_w = iupdrvFontGetStringWidth(ih, "WWWWWWWWWW");
        natural_w = (visiblecolumns * natural_w) / 10;
      }
      else
      {
        item_count = iupdrvListGetCount(ih);
        for (int i = 1; i <= item_count; i++)
        {
          char* value = IupGetAttributeId(ih, "", i);
          if (value)
          {
            int item_w = iupdrvFontGetStringWidth(ih, value);
            if (item_w > natural_w)
              natural_w = item_w;
          }
        }

        if (natural_w == 0)
          natural_w = iupdrvFontGetStringWidth(ih, "WWWWW");
      }

      natural_w += 2 * ih->data->spacing;

      item_count = iupdrvListGetCount(ih);
      if (item_count == 0) item_count = 1;

      visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
      if (visiblelines)
        item_count = visiblelines;

      natural_h = char_height;
      iupdrvListAddItemSpace(ih, &natural_h);
      natural_h += 2 * ih->data->spacing;
      natural_h = natural_h * item_count;

      iupdrvListAddBorders(ih, &natural_w, &natural_h);

      if (ih->data->sb)
      {
        int sb_size = iupdrvGetScrollbarSize();
        natural_w += sb_size;
        natural_h += sb_size;
      }

    }

    g_signal_connect(selection_model, "selection-changed", G_CALLBACK(gtk4ListViewSelectionChanged), ih);
    g_signal_connect(ih->handle, "activate", G_CALLBACK(gtk4ListViewItemActivated), ih);

    iupgtk4SetupMotionEvents(ih->handle, ih);
    iupgtk4SetupButtonEvents(ih->handle, ih);
  }

  iupgtk4ClearSizeStyleCSS(ih->handle);

  iupgtk4AddToParent(ih);

  if (scrolled_window)
    gtk_widget_realize((GtkWidget*)scrolled_window);
  gtk_widget_realize(ih->handle);

  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSet(ih, "DROPFILESTARGET", "YES");

  IupSetCallback(ih, "_IUP_XY2POS_CB", (Icallback)gtk4ListConvertXYToPos);

  iupListSetInitialItems(ih);

  /* Tell GTK widgets the computed size since GtkListView doesn't naturally report sizes */
  gtk4ListUpdateMinSize(ih);

  iupgtk4UpdateMnemonic(ih);

  return IUP_NOERROR;
}

void* iupdrvListGetImageHandle(Ihandle* ih, int id)
{
  GListStore* store = gtk4ListGetGListStore(ih);

  if (store)
  {
    IupListItem* item = g_list_model_get_item(G_LIST_MODEL(store), id - 1);
    if (item)
    {
      GdkTexture* texture = iup_list_item_get_image(item);
      g_object_unref(item);
      return texture;
    }
    return NULL;
  }
}

int iupdrvListSetImageHandle(Ihandle* ih, int id, void* hImage)
{
  GListStore* store = gtk4ListGetGListStore(ih);

  if (store)
  {
    IupListItem* item = g_list_model_get_item(G_LIST_MODEL(store), id);
    if (item)
    {
      iup_list_item_set_image(item, (GdkTexture*)hImage);
      g_object_unref(item);
    }
    return 0;
  }
}

static char* gtk4ListGetImageNativeHandleAttribId(Ihandle* ih, int id)
{
  GListStore* store = gtk4ListGetGListStore(ih);

  if (store)
  {
    IupListItem* item = g_list_model_get_item(G_LIST_MODEL(store), id - 1);
    if (item)
    {
      GdkTexture* texture = iup_list_item_get_image(item);
      g_object_unref(item);
      return (char*)texture;
    }
    return NULL;
  }
}

void iupdrvListInitClass(Iclass* ic)
{
  ic->Map = gtk4ListMapMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, gtk4ListSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, gtk4ListSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, gtk4ListSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "VALUE", gtk4ListGetValueAttrib, gtk4ListSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IDVALUE", gtk4ListGetIdValueAttrib, iupListSetIdValueAttrib, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SHOWDROPDOWN", NULL, gtk4ListSetShowDropdownAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOPITEM", NULL, gtk4ListSetTopItemAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPACING", iupListGetSpacingAttrib, gtk4ListSetSpacingAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "PADDING", iupListGetPaddingAttrib, gtk4ListSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "SELECTEDTEXT", gtk4ListGetSelectedTextAttrib, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTION", gtk4ListGetSelectionAttrib, gtk4ListSetSelectionAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTIONPOS", gtk4ListGetSelectionPosAttrib, gtk4ListSetSelectionPosAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARET", gtk4ListGetCaretAttrib, gtk4ListSetCaretAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARETPOS", gtk4ListGetCaretPosAttrib, gtk4ListSetCaretPosAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERT", NULL, gtk4ListSetInsertAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPEND", NULL, gtk4ListSetAppendAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "READONLY", gtk4ListGetReadOnlyAttrib, gtk4ListSetReadOnlyAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "NC", iupListGetNCAttrib, gtk4ListSetNCAttrib, NULL, NULL, IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "CLIPBOARD", NULL, gtk4ListSetClipboardAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTO", NULL, gtk4ListSetScrollToAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTOPOS", NULL, gtk4ListSetScrollToPosAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "IMAGE", NULL, gtk4ListSetImageAttrib, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IMAGENATIVEHANDLE", gtk4ListGetImageNativeHandleAttribId, NULL, IUPAF_NO_STRING | IUPAF_READONLY | IUPAF_NO_INHERIT);
}
