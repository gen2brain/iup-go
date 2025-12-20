/** \file
 * \brief GTK4 Base Functions
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <gtk/gtk.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupkey.h"

#include "iup_object.h"
#include "iup_childtree.h"
#include "iup_key.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_focus.h"
#include "iup_key.h"
#include "iup_image.h"
#include "iup_drv.h"
#include "iup_assert.h"
#include "iup_dialog.h"
#include "iup_dlglist.h"

#include "iupgtk4_drv.h"


/******************************************************************************
 * CSS Manager, GTK4 approach using display-wide CSS with widget classes
 *****************************************************************************/

typedef struct {
  char* bg_css;      /* background color CSS or NULL */
  char* fg_css;      /* foreground color CSS or NULL */
  char* padding_css; /* padding CSS or NULL */
  char* font_css;    /* font CSS or NULL */
  char* custom_css;  /* custom CSS properties or NULL */
} Igtk4WidgetStyle;

static GtkCssProvider* gtk4_css_provider = NULL;
static GHashTable* gtk4_widget_styles = NULL;  /* widget ptr -> Igtk4WidgetStyle* */
static GString* gtk4_css_buffer = NULL;
static GHashTable* gtk4_static_rules = NULL;   /* selector -> css_rules */

static void gtk4CssWidgetStyleFree(gpointer data)
{
  Igtk4WidgetStyle* style = (Igtk4WidgetStyle*)data;
  if (style)
  {
    g_free(style->bg_css);
    g_free(style->fg_css);
    g_free(style->padding_css);
    g_free(style->font_css);
    g_free(style->custom_css);
    g_free(style);
  }
}

static char* gtk4CssGetWidgetClassName(GtkWidget* widget)
{
  return g_strdup_printf("iup-w-%lx", (unsigned long)(uintptr_t)widget);
}

static void gtk4CssRebuildAndApply(void)
{
  GHashTableIter iter;
  gpointer key, value;

  if (!gtk4_css_provider || !gtk4_widget_styles)
    return;

  g_string_truncate(gtk4_css_buffer, 0);

  /* Add static rules first */
  if (gtk4_static_rules)
  {
    g_hash_table_iter_init(&iter, gtk4_static_rules);
    while (g_hash_table_iter_next(&iter, &key, &value))
    {
      g_string_append_printf(gtk4_css_buffer, "%s { %s }\n", (char*)key, (char*)value);
    }
  }

  /* Add per-widget rules */
  g_hash_table_iter_init(&iter, gtk4_widget_styles);
  while (g_hash_table_iter_next(&iter, &key, &value))
  {
    GtkWidget* widget = (GtkWidget*)key;
    Igtk4WidgetStyle* style = (Igtk4WidgetStyle*)value;
    char* class_name = gtk4CssGetWidgetClassName(widget);

    g_string_append_printf(gtk4_css_buffer, ".%s {\n", class_name);

    if (style->bg_css)
      g_string_append_printf(gtk4_css_buffer, "  %s\n", style->bg_css);
    if (style->fg_css)
      g_string_append_printf(gtk4_css_buffer, "  %s\n", style->fg_css);
    if (style->padding_css)
      g_string_append_printf(gtk4_css_buffer, "  %s\n", style->padding_css);
    if (style->font_css)
      g_string_append_printf(gtk4_css_buffer, "  %s\n", style->font_css);
    if (style->custom_css)
      g_string_append_printf(gtk4_css_buffer, "  %s\n", style->custom_css);

    g_string_append(gtk4_css_buffer, "}\n");

    /* Add state variants for background color */
    if (style->bg_css)
    {
      g_string_append_printf(gtk4_css_buffer, ".%s:disabled { %s }\n", class_name, style->bg_css);
    }

    /* Add state variants for foreground color */
    if (style->fg_css)
    {
      g_string_append_printf(gtk4_css_buffer, ".%s:hover { %s }\n", class_name, style->fg_css);
      g_string_append_printf(gtk4_css_buffer, ".%s:active { %s }\n", class_name, style->fg_css);
    }

    g_free(class_name);
  }

  gtk_css_provider_load_from_string(gtk4_css_provider, gtk4_css_buffer->str);
}

static Igtk4WidgetStyle* gtk4CssGetOrCreateWidgetStyle(GtkWidget* widget)
{
  Igtk4WidgetStyle* style = g_hash_table_lookup(gtk4_widget_styles, widget);

  if (!style)
  {
    char* class_name;

    style = g_new0(Igtk4WidgetStyle, 1);
    g_hash_table_insert(gtk4_widget_styles, widget, style);

    /* Add the CSS class to the widget */
    class_name = gtk4CssGetWidgetClassName(widget);
    gtk_widget_add_css_class(widget, class_name);
    g_free(class_name);
  }

  return style;
}

void iupgtk4CssManagerInit(void)
{
  GdkDisplay* display;

  if (gtk4_css_provider)
    return;

  display = gdk_display_get_default();
  if (!display)
    return;

  gtk4_css_provider = gtk_css_provider_new();
  gtk4_widget_styles = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, gtk4CssWidgetStyleFree);
  gtk4_css_buffer = g_string_new("");
  gtk4_static_rules = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

  gtk_style_context_add_provider_for_display(display, GTK_STYLE_PROVIDER(gtk4_css_provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
}

void iupgtk4CssManagerFinish(void)
{
  if (gtk4_css_provider)
  {
    GdkDisplay* display = gdk_display_get_default();
    if (display)
      gtk_style_context_remove_provider_for_display(display, GTK_STYLE_PROVIDER(gtk4_css_provider));

    g_object_unref(gtk4_css_provider);
    gtk4_css_provider = NULL;
  }

  if (gtk4_widget_styles)
  {
    g_hash_table_destroy(gtk4_widget_styles);
    gtk4_widget_styles = NULL;
  }

  if (gtk4_css_buffer)
  {
    g_string_free(gtk4_css_buffer, TRUE);
    gtk4_css_buffer = NULL;
  }

  if (gtk4_static_rules)
  {
    g_hash_table_destroy(gtk4_static_rules);
    gtk4_static_rules = NULL;
  }
}

void iupgtk4CssSetWidgetBgColor(GtkWidget* widget, unsigned char r, unsigned char g, unsigned char b, int is_text)
{
  Igtk4WidgetStyle* style;
  GdkRGBA rgba;
  char* color_str;

  if (!gtk4_css_provider)
    iupgtk4CssManagerInit();

  style = gtk4CssGetOrCreateWidgetStyle(widget);

  iupgtk4ColorSetRGB(&rgba, r, g, b);
  color_str = gdk_rgba_to_string(&rgba);

  g_free(style->bg_css);
  style->bg_css = g_strdup_printf("background-color: %s;", color_str);

  g_free(color_str);

  (void)is_text;

  gtk4CssRebuildAndApply();
}

void iupgtk4CssSetWidgetFgColor(GtkWidget* widget, unsigned char r, unsigned char g, unsigned char b)
{
  Igtk4WidgetStyle* style;
  GdkRGBA rgba;
  char* color_str;

  if (!gtk4_css_provider)
    iupgtk4CssManagerInit();

  style = gtk4CssGetOrCreateWidgetStyle(widget);

  iupgtk4ColorSetRGB(&rgba, r, g, b);
  color_str = gdk_rgba_to_string(&rgba);

  g_free(style->fg_css);
  style->fg_css = g_strdup_printf("color: %s;", color_str);

  g_free(color_str);

  gtk4CssRebuildAndApply();
}

void iupgtk4CssSetWidgetPadding(GtkWidget* widget, int horiz, int vert)
{
  Igtk4WidgetStyle* style;

  if (!gtk4_css_provider)
    iupgtk4CssManagerInit();

  style = gtk4CssGetOrCreateWidgetStyle(widget);

  g_free(style->padding_css);
  style->padding_css = g_strdup_printf("padding: %dpx %dpx;", vert, horiz);

  gtk4CssRebuildAndApply();
}

void iupgtk4CssSetWidgetFont(GtkWidget* widget, const char* font_css)
{
  Igtk4WidgetStyle* style;

  if (!gtk4_css_provider)
    iupgtk4CssManagerInit();

  style = gtk4CssGetOrCreateWidgetStyle(widget);

  g_free(style->font_css);
  style->font_css = g_strdup(font_css);

  gtk4CssRebuildAndApply();
}

void iupgtk4CssSetWidgetCustom(GtkWidget* widget, const char* css_property, const char* css_value)
{
  Igtk4WidgetStyle* style;

  if (!gtk4_css_provider)
    iupgtk4CssManagerInit();

  style = gtk4CssGetOrCreateWidgetStyle(widget);

  g_free(style->custom_css);
  style->custom_css = g_strdup_printf("%s: %s;", css_property, css_value);

  gtk4CssRebuildAndApply();
}

void iupgtk4CssClearWidgetStyle(GtkWidget* widget)
{
  char* class_name;

  if (!gtk4_widget_styles)
    return;

  if (g_hash_table_remove(gtk4_widget_styles, widget))
  {
    class_name = gtk4CssGetWidgetClassName(widget);
    gtk_widget_remove_css_class(widget, class_name);
    g_free(class_name);

    gtk4CssRebuildAndApply();
  }
}

void iupgtk4CssAddStaticRule(const char* selector, const char* css_rules)
{
  if (!gtk4_css_provider)
    iupgtk4CssManagerInit();

  g_hash_table_insert(gtk4_static_rules, g_strdup(selector), g_strdup(css_rules));

  gtk4CssRebuildAndApply();
}

/******************************************************************************
 * End CSS Manager
 *****************************************************************************/

/* Custom Fixed container for absolute positioning */
typedef struct _iupGtk4Fixed
{
  GtkFixed parent_instance;
  Ihandle* ih;  /* Associated IUP dialog handle for resize notifications */
  int last_width;   /* Cached width to prevent redundant size-allocate notifications */
  int last_height;  /* Cached height to prevent redundant size-allocate notifications */
  int draw_border;  /* Flag to draw themed border frame (for canvas BORDER attribute) */
} iupGtk4Fixed;

typedef struct _iupGtk4FixedClass
{
  GtkFixedClass parent_class;
} iupGtk4FixedClass;

G_DEFINE_TYPE(iupGtk4Fixed, iup_gtk4_fixed, GTK_TYPE_FIXED)

static void iup_gtk4_fixed_snapshot(GtkWidget* widget, GtkSnapshot* snapshot)
{
  iupGtk4Fixed* fixed = (iupGtk4Fixed*)widget;

  /* Draw border frame if enabled (for canvas BORDER=YES) */
  if (fixed->draw_border)
  {
    int width = gtk_widget_get_width(widget);
    int height = gtk_widget_get_height(widget);

    if (width > 0 && height > 0)
    {
      GskRoundedRect rect;
      float border_width[4] = {1.0f, 1.0f, 1.0f, 1.0f};
      GdkRGBA border_color[4];
      GdkRGBA gray = {0.6f, 0.6f, 0.6f, 1.0f};

      border_color[0] = border_color[1] = border_color[2] = border_color[3] = gray;
      gsk_rounded_rect_init_from_rect(&rect,
                                      &GRAPHENE_RECT_INIT(0, 0, width, height),
                                      0);
      gtk_snapshot_append_border(snapshot, &rect, border_width, border_color);
    }
  }

  /* Chain up to parent to draw children */
  GTK_WIDGET_CLASS(iup_gtk4_fixed_parent_class)->snapshot(widget, snapshot);
}

static void iup_gtk4_fixed_class_init(iupGtk4FixedClass* klass)
{
  GtkWidgetClass* widget_class = GTK_WIDGET_CLASS(klass);

  /* Override snapshot to draw themed border frame */
  widget_class->snapshot = iup_gtk4_fixed_snapshot;
}

static void iup_gtk4_fixed_layout_allocate(GtkWidget* widget, int width, int height, int baseline)
{
  GtkWidget* child;
  iupGtk4Fixed* fixed = (iupGtk4Fixed*)widget;

  /* Notify IUP dialog of size change for layout recalculation.
     Only call for dialogs, other containers (like popover) don't have ih->data. */
  if (fixed->ih && iupStrEqual(fixed->ih->iclass->name, "dialog") &&
      (fixed->last_width != width || fixed->last_height != height))
  {
    fixed->last_width = width;
    fixed->last_height = height;

    extern void gtk4DialogSizeAllocate(GtkWidget* widget, int width, int height, int baseline, Ihandle* ih);
    gtk4DialogSizeAllocate(widget, width, height, baseline, fixed->ih);
  }

  (void)baseline;

  /* Present any popover children */
  for (child = gtk_widget_get_first_child(widget); child != NULL; child = gtk_widget_get_next_sibling(child))
  {
    if (GTK_IS_POPOVER(child))
      gtk_popover_present(GTK_POPOVER(child));
  }

  /* Allocate children manually with IUP-specified sizes and positions */
  for (child = gtk_widget_get_first_child(widget); child != NULL; child = gtk_widget_get_next_sibling(child))
  {
    int child_x, child_y;
    int child_width, child_height;
    int is_scrolled_window = GTK_IS_SCROLLED_WINDOW(child);

    if (!gtk_widget_get_visible(child))
      continue;

    /* Skip native widgets like popovers, they manage their own allocation */
    if (GTK_IS_NATIVE(child))
      continue;

    /* Get position and size from widget data */
    child_x = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(child), "_iup_x"));
    child_y = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(child), "_iup_y"));
    child_width = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(child), "_iup_width"));
    child_height = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(child), "_iup_height"));

    /* If no IUP size set, use preferred size */
    if (child_width <= 0 || child_height <= 0)
    {
      GtkRequisition child_req;
      gtk_widget_get_preferred_size(child, &child_req, NULL);
      if (child_width <= 0) child_width = child_req.width;
      if (child_height <= 0) child_height = child_req.height;
    }

    /* Enforce minimum widget size requirements */
    /* Except for widgets with VISIBLELINES, they need to be smaller than GTK's scrollbar minimum */
    {
      const char* visiblelines_set = (const char*)g_object_get_data(G_OBJECT(child), "iup-visiblelines-set");
      if (!visiblelines_set)
      {
        int min_w, min_h;
        gtk_widget_measure(child, GTK_ORIENTATION_HORIZONTAL, -1, &min_w, NULL, NULL, NULL);
        gtk_widget_measure(child, GTK_ORIENTATION_VERTICAL, -1, &min_h, NULL, NULL, NULL);

        if (child_width < min_w) child_width = min_w;
        if (child_height < min_h) child_height = min_h;
      }
    }

    (void)is_scrolled_window;

    /* Allocate child */
    gtk_widget_allocate(child, child_width, child_height, -1, gsk_transform_translate(NULL, &GRAPHENE_POINT_INIT(child_x, child_y)));
  }
}

static void iup_gtk4_fixed_layout_measure(GtkWidget* widget, GtkOrientation orientation, int for_size,
                                           int* minimum, int* natural,
                                           int* minimum_baseline, int* natural_baseline)
{
  GtkWidget* child;
  int max_size = 0;
  iupGtk4Fixed* fixed = (iupGtk4Fixed*)widget;

  (void)for_size;

  /* If this Fixed is associated with a dialog, return minimal size. */
  if (fixed->ih && iupStrEqual(fixed->ih->iclass->name, "dialog"))
  {
    *minimum = 1;
    *natural = 1;
    if (minimum_baseline) *minimum_baseline = -1;
    if (natural_baseline) *natural_baseline = -1;
    return;
  }

  /* Measure all children and return the maximum extent in the requested orientation.
   * This ensures the container gets allocated enough space to display its children. */
  for (child = gtk_widget_get_first_child(widget); child != NULL; child = gtk_widget_get_next_sibling(child))
  {
    if (!gtk_widget_get_visible(child))
      continue;

    /* Skip nested iupGtk4Fixed wrappers (canvas sb_win, frame inner_parent). */
    if (G_TYPE_CHECK_INSTANCE_TYPE(child, iup_gtk4_fixed_get_type()))
      continue;

    int child_pos, child_size;
    int child_min, child_nat;

    /* Get child's requested size in this orientation */
    gtk_widget_measure(child, orientation, -1, &child_min, &child_nat, NULL, NULL);

    /* Get child's position in this orientation */
    if (orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      child_pos = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(child), "_iup_x"));
      child_size = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(child), "_iup_width"));
    }
    else
    {
      child_pos = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(child), "_iup_y"));
      child_size = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(child), "_iup_height"));
    }

    /* Use IUP size if set, but cap at reasonable size relative to natural. */
    if (child_size <= 0 || child_size > child_nat * 3)
      child_size = child_nat;

    /* Enforce minimum size just like allocate does, unless VISIBLELINES is set */
    const char* visiblelines_set = (const char*)g_object_get_data(G_OBJECT(child), "iup-visiblelines-set");
    if (!visiblelines_set && child_size < child_min)
      child_size = child_min;

    /* Calculate total extent needed (position + size) */
    int extent = child_pos + child_size;
    if (extent > max_size)
      max_size = extent;
  }

  *minimum = max_size;
  *natural = max_size;
  if (minimum_baseline) *minimum_baseline = -1;
  if (natural_baseline) *natural_baseline = -1;
}

static void iup_gtk4_fixed_init(iupGtk4Fixed* fixed)
{
  GtkLayoutManager* layout;

  fixed->ih = NULL;
  fixed->last_width = 0;
  fixed->last_height = 0;
  fixed->draw_border = 0;

  layout = gtk_custom_layout_new(
    (GtkCustomRequestModeFunc)NULL,
    (GtkCustomMeasureFunc)iup_gtk4_fixed_layout_measure,
    (GtkCustomAllocateFunc)iup_gtk4_fixed_layout_allocate
  );
  gtk_widget_set_layout_manager(GTK_WIDGET(fixed), layout);
}

static void iup_gtk4_fixed_put(iupGtk4Fixed* fixed, GtkWidget* widget, int x, int y)
{
  /* Since we use custom layout manager, manually set parent and store position */
  gtk_widget_set_parent(widget, GTK_WIDGET(fixed));

  /* Store position in widget data */
  g_object_set_data(G_OBJECT(widget), "_iup_x", GINT_TO_POINTER(x));
  g_object_set_data(G_OBJECT(widget), "_iup_y", GINT_TO_POINTER(y));
}

static void iup_gtk4_fixed_move(iupGtk4Fixed* fixed, GtkWidget* widget, int x, int y, int width, int height)
{
  (void)fixed;

  /* Store position and size in widget data for custom layout manager */
  g_object_set_data(G_OBJECT(widget), "_iup_x", GINT_TO_POINTER(x));
  g_object_set_data(G_OBJECT(widget), "_iup_y", GINT_TO_POINTER(y));
  g_object_set_data(G_OBJECT(widget), "_iup_width", GINT_TO_POINTER(width));
  g_object_set_data(G_OBJECT(widget), "_iup_height", GINT_TO_POINTER(height));

  /* Trigger layout update */
  gtk_widget_queue_allocate(GTK_WIDGET(fixed));
}

GtkWidget* iupgtk4NativeContainerNew(void)
{
  return g_object_new(iup_gtk4_fixed_get_type(), NULL);
}

void iupgtk4NativeContainerSetIhandle(GtkWidget* container, Ihandle* ih)
{
  iupGtk4Fixed* fixed = (iupGtk4Fixed*)container;
  fixed->ih = ih;
}

void iupgtk4NativeContainerAdd(GtkWidget* container, GtkWidget* widget)
{
  iup_gtk4_fixed_put((iupGtk4Fixed*)container, widget, 0, 0);
}

void iupgtk4NativeContainerMove(GtkWidget* container, GtkWidget* widget, int x, int y)
{
  /* Call with width=0, height=0 which means use preferred size */
  iup_gtk4_fixed_move((iupGtk4Fixed*)container, widget, x, y, 0, 0);
}

void iupgtk4NativeContainerSetBounds(GtkWidget* container, GtkWidget* widget, int x, int y, int width, int height)
{
  iup_gtk4_fixed_move((iupGtk4Fixed*)container, widget, x, y, width, height);
}

void iupgtk4FixedMove(iupGtk4Fixed* fixed, GtkWidget* widget, int x, int y)
{
  iup_gtk4_fixed_move(fixed, widget, x, y, 0, 0);
}

void iupgtk4NativeContainerSetBorder(GtkWidget* container, int enable)
{
  if (G_TYPE_CHECK_INSTANCE_TYPE(container, iup_gtk4_fixed_get_type()))
  {
    iupGtk4Fixed* fixed = (iupGtk4Fixed*)container;
    fixed->draw_border = enable;
    /* Queue redraw to show/hide border */
    gtk_widget_queue_draw(container);
  }
}

static int gtk4IsFixed(GtkWidget* widget)
{
  return G_TYPE_CHECK_INSTANCE_TYPE(widget, iup_gtk4_fixed_get_type());
}

static GtkWidget* gtk4GetNativeParent(Ihandle* ih)
{
  GtkWidget* widget = iupChildTreeGetNativeParentHandle(ih);
  GtkWidget* original_widget = widget;
  int step = 0;

  /* Special case: If widget is the menu VBox wrapper, get inner_parent from dialog */
  if (widget && G_TYPE_CHECK_INSTANCE_TYPE(widget, GTK_TYPE_BOX))
  {
    Ihandle* dialog = ih->parent;
    while (dialog && !iupStrEqual(dialog->iclass->name, "dialog"))
      dialog = dialog->parent;

    if (dialog)
    {
      GtkWidget* menu_box = (GtkWidget*)iupAttribGet(dialog, "_IUPGTK4_MENU_BOX");
      if (menu_box == widget)
      {
        GtkWidget* inner_parent = (GtkWidget*)iupAttribGet(dialog, "_IUPGTK4_INNER_PARENT");
        return inner_parent;
      }
    }
  }

  while (widget && !gtk4IsFixed(widget))
  {
    step++;
    widget = gtk_widget_get_parent(widget);
  }

  return widget;
}

const char* iupgtk4GetWidgetClassName(GtkWidget* widget)
{
  return G_OBJECT_TYPE_NAME(widget);
}

void iupgtk4UpdateMnemonic(Ihandle* ih)
{
  GtkLabel* label = (GtkLabel*)iupAttribGet(ih, "_IUPGTK4_LABELMNEMONIC");
  if (label) gtk_label_set_mnemonic_widget(label, ih->handle);
}

IUP_SDK_API void iupdrvActivate(Ihandle* ih)
{
  gtk_widget_activate(ih->handle);
}

IUP_SDK_API void iupdrvReparent(Ihandle* ih)
{
  GtkWidget* old_parent;
  GtkWidget* new_parent = gtk4GetNativeParent(ih);
  GtkWidget* widget;
  GtkWidget* extraparent;

  if (!ih->handle || !GTK_IS_WIDGET(ih->handle))
  {
    return;
  }

  /* Check for extra parent, validate it separately */
  extraparent = (GtkWidget*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (extraparent && GTK_IS_WIDGET(extraparent))
    widget = extraparent;
  else
    widget = ih->handle;

  /* Validate new_parent pointer */
  if (!new_parent || !GTK_IS_WIDGET(new_parent))
  {
    return;
  }

  old_parent = gtk_widget_get_parent(widget);

  if (old_parent != new_parent)
  {
    /* Use gtk_widget_unparent/set_parent instead of reparent */
    if (old_parent)
    {
      /* Add reference before unparenting to prevent widget destruction */
      g_object_ref(widget);
      gtk_widget_unparent(widget);
    }
    gtk_widget_set_parent(widget, new_parent);
    gtk_widget_realize(widget);

    if (old_parent)
    {
      /* Remove the reference we added earlier */
      g_object_unref(widget);
    }
  }
}

IUP_DRV_API void iupgtk4AddToParent(Ihandle* ih)
{
  GtkWidget* parent = gtk4GetNativeParent(ih);
  GtkWidget* widget = (GtkWidget*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (!widget) widget = ih->handle;

  iupgtk4NativeContainerAdd(parent, widget);
}

void iupgtk4SetPosSize(GtkWidget* parent, GtkWidget* widget, int x, int y, int width, int height)
{
  /* Store both position and size in the Fixed container's child info */
  if (width > 0 && height > 0)
    iupgtk4NativeContainerSetBounds(parent, widget, x, y, width, height);
  else
    iupgtk4NativeContainerMove(parent, widget, x, y);
}

IUP_SDK_API void iupdrvBaseLayoutUpdateMethod(Ihandle* ih)
{
  GtkWidget* parent = gtk4GetNativeParent(ih);
  GtkWidget* widget = (GtkWidget*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (!widget) widget = ih->handle;

  int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
  if (visiblelines > 0)
  {
    /* For EDITBOX with VISIBLELINES using GtkFixed, set widget widths now that currentwidth is known */
    if (GTK_IS_FIXED(widget))
    {
      GtkWidget* entry = (GtkWidget*)iupAttribGet(ih, "_IUPGTK4_ENTRY");
      GtkWidget* scrolled_window = (GtkWidget*)iupAttribGet(ih, "_IUPGTK4_SCROLLED_WINDOW");

      if (entry && scrolled_window)
      {
        int entry_height = iupAttribGetInt(ih, "_IUPGTK4_ENTRY_HEIGHT");

        gtk_widget_set_size_request(entry, ih->currentwidth, entry_height);

        /* Get scrolled_window current height from size_request */
        int sw_width, sw_height;
        gtk_widget_get_size_request(scrolled_window, &sw_width, &sw_height);
        gtk_widget_set_size_request(scrolled_window, ih->currentwidth, sw_height);
      }
    }
  }

  iupgtk4SetPosSize(parent, widget, ih->x, ih->y, ih->currentwidth, ih->currentheight);
}

IUP_SDK_API void iupdrvBaseUnMapMethod(Ihandle* ih)
{
  GtkWidget* widget = (GtkWidget*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (!widget) widget = ih->handle;

  if (!widget)
    return;

  /* Try to get parent first - if this fails, widget is already destroyed */
  GtkWidget* parent = NULL;
  if (GTK_IS_WIDGET(widget))
    parent = gtk_widget_get_parent(widget);

  /* If widget doesn't have a parent and isn't a toplevel, it's already been cleaned up */
  if (!parent && GTK_IS_WIDGET(widget) && !GTK_IS_WINDOW(widget))
    return;

  if (GTK_IS_WIDGET(widget))
    gtk_widget_set_visible(widget, FALSE);

  if (GTK_IS_WIDGET(widget))
    gtk_widget_unrealize(widget);

  /* Need to unparent before destroying */
  if (GTK_IS_WIDGET(widget) && parent)
    gtk_widget_unparent(widget);
}

IUP_SDK_API void iupdrvPostRedraw(Ihandle* ih)
{
  gtk_widget_queue_draw(ih->handle);
}

IUP_SDK_API void iupdrvRedrawNow(Ihandle* ih)
{
  GdkDisplay* display;

  /* Queue the redraw */
  gtk_widget_queue_draw(ih->handle);

  /* Use gdk_display_sync() to force immediate processing */
  display = gtk_widget_get_display(ih->handle);
  if (display)
    gdk_display_sync(display);
}

static GtkWidget* gtk4GetWindowedParent(GtkWidget* widget)
{
  /* All widgets can receive input, find first parent with surface */
  GtkNative* native;
  while (widget)
  {
    native = gtk_widget_get_native(widget);
    if (native && gtk_native_get_surface(native))
      return widget;
    widget = gtk_widget_get_parent(widget);
  }
  return NULL;
}

IUP_SDK_API void iupdrvScreenToClient(Ihandle* ih, int* x, int* y)
{
  double win_x = 0, win_y = 0;
  double dx = 0, dy = 0;
  GtkWidget* wparent = gtk4GetWindowedParent(ih->handle);

  if (ih->handle != wparent && wparent)
  {
    graphene_point_t src_point = GRAPHENE_POINT_INIT(0, 0);
    graphene_point_t dest_point;
    if (gtk_widget_compute_point(ih->handle, wparent, &src_point, &dest_point))
    {
      dx = dest_point.x;
      dy = dest_point.y;
    }
  }

  if (wparent)
  {
    GdkSurface* surface = iupgtk4GetSurface(wparent);
    if (surface)
      iupgtk4SurfaceGetPointer(surface, &win_x, &win_y, NULL);
  }

  *x -= (int)(win_x + dx);
  *y -= (int)(win_y + dy);
}

IUP_SDK_API void iupdrvClientToScreen(Ihandle* ih, int* x, int* y)
{
  double win_x = 0, win_y = 0;
  double dx = 0, dy = 0;
  GtkWidget* wparent = gtk4GetWindowedParent(ih->handle);

  if (ih->handle != wparent && wparent)
  {
    graphene_point_t src_point = GRAPHENE_POINT_INIT(0, 0);
    graphene_point_t dest_point;
    if (gtk_widget_compute_point(ih->handle, wparent, &src_point, &dest_point))
    {
      dx = dest_point.x;
      dy = dest_point.y;
    }
  }

  if (wparent)
  {
    GdkSurface* surface = iupgtk4GetSurface(wparent);
    if (surface)
      iupgtk4SurfaceGetPointer(surface, &win_x, &win_y, NULL);
  }

  *x += (int)(win_x + dx);
  *y += (int)(win_y + dy);
}

IUP_DRV_API gboolean iupgtk4ShowHelp(GtkWidget* widget, gpointer arg1, Ihandle* ih)
{
  Icallback cb;
  (void)widget;
  (void)arg1;

  cb = IupGetCallback(ih, "HELP_CB");
  if (cb && cb(ih) == IUP_CLOSE)
    IupExitLoop();

  return FALSE;
}

int iupgtk4SetMnemonicTitle(Ihandle* ih, GtkLabel* label, const char* value)
{
  char c = '_';
  char* str;

  if (!value)
    value = "";

  str = iupStrProcessMnemonic(value, &c, 1);
  if (str != value)
  {
    gtk_label_set_text_with_mnemonic(label, iupgtk4StrConvertToSystem(str));
    free(str);
    return 1;
  }
  else
  {
    if (iupAttribGetBoolean(ih, "MARKUP"))
    {
      gtk_label_set_markup(label, iupgtk4StrConvertToSystem(str));
    }
    else
    {
      gtk_label_set_text(label, iupgtk4StrConvertToSystem(str));
    }
  }
  return 0;
}

IUP_SDK_API int iupdrvBaseSetZorderAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 0;
}

IUP_SDK_API void iupdrvSetVisible(Ihandle* ih, int visible)
{
  GtkWidget* container = (GtkWidget*)iupAttribGet(ih, "_IUP_EXTRAPARENT");

  if (!ih->handle || !GTK_IS_WIDGET(ih->handle))
    return;

  gtk_widget_set_visible(ih->handle, visible);
  if (container && GTK_IS_WIDGET(container))
    gtk_widget_set_visible(container, visible);
}

IUP_SDK_API int iupdrvIsVisible(Ihandle* ih)
{
  /* Validate handle before querying visibility */
  if (!ih->handle || !GTK_IS_WIDGET(ih->handle))
    return 0;

  if (gtk_widget_get_visible(ih->handle))
  {
    /* Must check parents since we use gtk_widget_set_visible individually */
    Ihandle* parent = ih->parent;
    while (parent)
    {
      if (parent->iclass->nativetype != IUP_TYPEVOID)
      {
        if (!parent->handle || !GTK_IS_WIDGET(parent->handle))
          return 0;
        if (!gtk_widget_get_visible(parent->handle))
          return 0;
      }
      parent = parent->parent;
    }
    return 1;
  }
  return 0;
}

IUP_SDK_API int iupdrvIsActive(Ihandle* ih)
{
  if (!ih->handle || !GTK_IS_WIDGET(ih->handle))
    return 0;

  return gtk_widget_is_sensitive(ih->handle);
}

IUP_SDK_API void iupdrvSetActive(Ihandle* ih, int enable)
{
  GtkWidget* container = (GtkWidget*)iupAttribGet(ih, "_IUP_EXTRAPARENT");

  if (!ih->handle || !GTK_IS_WIDGET(ih->handle))
    return;

  if (container && GTK_IS_WIDGET(container))
    gtk_widget_set_sensitive(container, enable);
  gtk_widget_set_sensitive(ih->handle, enable);
}

void iupgtk4ColorSetRGB(GdkRGBA* color, unsigned char r, unsigned char g, unsigned char b)
{
  color->red = iupgtk4ColorToDouble(r);
  color->green = iupgtk4ColorToDouble(g);
  color->blue = iupgtk4ColorToDouble(b);
  color->alpha = 1.0;
}

static GdkRGBA gtk4DarkerRGBA(GdkRGBA* rgba)
{
  GdkRGBA dark_rgba = {0, 0, 0, 1.0};

  dark_rgba.red = (rgba->red * 9) / 10;
  dark_rgba.green = (rgba->green * 9) / 10;
  dark_rgba.blue = (rgba->blue * 9) / 10;

  return dark_rgba;
}

static gdouble gtk4CROPDouble(gdouble x)
{
  if (x > 1.0) return 1.0;
  return x;
}

static GdkRGBA gtk4LighterRGBA(GdkRGBA* rgba)
{
  GdkRGBA light_rgba = {0, 0, 0, 1.0};

  light_rgba.red = gtk4CROPDouble((rgba->red * 11) / 10);
  light_rgba.green = gtk4CROPDouble((rgba->green * 11) / 10);
  light_rgba.blue = gtk4CROPDouble((rgba->blue * 11) / 10);

  return light_rgba;
}

static char* gtk4GetSelectedColorStr(void)
{
  GdkRGBA rgba;
  unsigned char r, g, b;
  iupStrToRGB(IupGetGlobal("TXTHLCOLOR"), &r, &g, &b);
  iupgtk4ColorSetRGB(&rgba, r, g, b);
  return gdk_rgba_to_string(&rgba);
}

void iupgtk4SetBgColor(InativeHandle* handle, unsigned char r, unsigned char g, unsigned char b)
{
  int is_txt = 0;

  if (GTK_IS_TEXT_VIEW(handle) || GTK_IS_ENTRY(handle))
    is_txt = 1;

  iupgtk4CssSetWidgetBgColor(handle, r, g, b, is_txt);
}

void iupgtk4SetFgColor(InativeHandle* handle, unsigned char r, unsigned char g, unsigned char b)
{
  iupgtk4CssSetWidgetFgColor(handle, r, g, b);
}

IUP_SDK_API int iupdrvBaseSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupgtk4SetBgColor(ih->handle, r, g, b);
  return 1;
}

IUP_SDK_API int iupdrvBaseSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupgtk4SetFgColor(ih->handle, r, g, b);
  return 1;
}

static GdkCursor* gtk4GetCursor(Ihandle* ih, const char* name)
{
  static struct {
    const char* iupname;
    const char* sysname;
  } table[] = {
    { "NONE",      NULL},
    { "NULL",      NULL},
    { "ARROW",     "default"},
    { "BUSY",      "wait"},
    { "CROSS",     "crosshair"},
    { "HAND",      "pointer"},
    { "HELP",      "help"},
    { "IUP",       "help"},
    { "MOVE",      "move"},
    { "PEN",       "pencil"},
    { "RESIZE_N",  "n-resize"},
    { "RESIZE_S",  "s-resize"},
    { "RESIZE_NS", "ns-resize"},
    { "SPLITTER_HORIZ", "ns-resize"},
    { "RESIZE_W",  "w-resize"},
    { "RESIZE_E",  "e-resize"},
    { "RESIZE_WE", "ew-resize"},
    { "SPLITTER_VERT", "ew-resize"},
    { "RESIZE_NE", "ne-resize"},
    { "RESIZE_SE", "se-resize"},
    { "RESIZE_NW", "nw-resize"},
    { "RESIZE_SW", "sw-resize"},
    { "TEXT",      "text"},
    { "UPARROW",   "default"}
  };

  GdkCursor* cur;
  char str[200];
  int i, count = sizeof(table) / sizeof(table[0]);

  /* Check cursor cache first (per control) */
  sprintf(str, "_IUPGTK4_CURSOR_%s", name);
  cur = (GdkCursor*)iupAttribGet(ih, str);
  if (cur)
    return cur;

  /* Check pre-defined IUP names */
  for (i = 0; i < count; i++)
  {
    if (iupStrEqualNoCase(name, table[i].iupname))
    {
      if (table[i].sysname)
        cur = gdk_cursor_new_from_name(table[i].sysname, NULL);
      else
        cur = NULL;  /* GTK4 has no blank cursor, use NULL */

      break;
    }
  }

  if (i == count)
  {
    /* Check for a name defined cursor */
    cur = iupImageGetCursor(name);
  }

  /* Save cursor in cache */
  iupAttribSet(ih, str, (char*)cur);

  return cur;
}

IUP_SDK_API int iupdrvBaseSetCursorAttrib(Ihandle* ih, const char* value)
{
  GdkCursor* cur = gtk4GetCursor(ih, value);

  gtk_widget_set_cursor(ih->handle, cur);

  return 1;
}

IUP_SDK_API int iupdrvGetScrollbarSize(void)
{
  static int size = 0;

  if (size == 0)
  {
    GtkWidget* sb = gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL, NULL);
    int min_width, nat_width;

    gtk_widget_measure(sb, GTK_ORIENTATION_HORIZONTAL, -1, &min_width, &nat_width, NULL, NULL);
    size = nat_width;

    g_object_ref_sink(sb);
    g_object_unref(sb);
  }

  return size;
}

IUP_SDK_API void iupdrvBaseRegisterCommonAttrib(Iclass* ic)
{
  iupClassRegisterAttribute(ic, iupgtk4GetNativeFontIdName(), iupgtk4GetFontIdAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT | IUPAF_NO_STRING);
  iupClassRegisterAttribute(ic, "PANGOFONTDESC", iupgtk4GetPangoFontDescAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT | IUPAF_NO_STRING);
  iupClassRegisterAttribute(ic, "PANGOLAYOUT", iupgtk4GetPangoLayoutAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT | IUPAF_NO_STRING);
}

IUP_SDK_API void iupdrvBaseRegisterVisualAttrib(Iclass* ic)
{
  iupClassRegisterAttribute(ic, "TIPMARKUP", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TIPICON", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_DEFAULT);
}

IUP_SDK_API void iupdrvWarpPointer(int x, int y)
{
  /* Pointer warping not supported */
  (void)x;
  (void)y;
}

IUP_SDK_API void iupdrvSendKey(int key, int press)
{
  /* Event synthesis not supported */
  (void)key;
  (void)press;
}

IUP_SDK_API void iupdrvSendMouse(int x, int y, int bt, int status)
{
  /* Event synthesis not supported */
  (void)x;
  (void)y;
  (void)bt;
  (void)status;
}

void iupgtk4ClearSizeStyleCSS(GtkWidget* widget)
{
  iupgtk4CssClearWidgetStyle(widget);
}

void iupgtk4SetMargin(GtkWidget* widget, int horiz_padding, int vert_padding)
{
  iupgtk4CssSetWidgetPadding(widget, horiz_padding, vert_padding);
}

GdkSurface* iupgtk4GetSurface(GtkWidget* widget)
{
  GtkNative* native;

  if (!widget)
    return NULL;

  native = gtk_widget_get_native(widget);
  if (!native)
    return NULL;

  return gtk_native_get_surface(native);
}

void iupgtk4SurfaceGetPointer(GdkSurface* surface, double* x, double* y, GdkModifierType* mask)
{
  GdkDevice* device;

  if (!surface)
    return;

  device = gdk_seat_get_pointer(gdk_display_get_default_seat(gdk_surface_get_display(surface)));
  gdk_surface_get_device_position(surface, device, x, y, mask);
}

int iupgtk4IsSystemDarkMode(void)
{
#if GTK_CHECK_VERSION(4, 20, 0)
  /* GTK 4.20+ has gtk-interface-color-scheme which is the system-wide setting */
  GtkSettings* settings = gtk_settings_get_default();
  int color_scheme = 0;
  g_object_get(settings, "gtk-interface-color-scheme", &color_scheme, NULL);
  /* GTK_INTERFACE_COLOR_SCHEME_DARK = 2 */
  return (color_scheme == 2) ? 1 : 0;
#elif GTK_CHECK_VERSION(4, 10, 0)
  /* For GTK4 4.10-4.19, measure actual widget colors */
  GtkWidget* temp_window;
  GdkRGBA fg;
  double fg_lum;
  int is_dark;

  temp_window = gtk_window_new();
  gtk_widget_realize(temp_window);

  /* Get foreground color, in dark themes, foreground is light (high luminance) */
  gtk_widget_get_color(temp_window, &fg);

  /* Calculate relative luminance using standard formula (ITU-R BT.709) */
  fg_lum = 0.2126 * fg.red + 0.7152 * fg.green + 0.0722 * fg.blue;

  /* In dark mode, foreground text is light (high luminance, > 0.5) */
  is_dark = (fg_lum > 0.5) ? 1 : 0;

  gtk_window_destroy(GTK_WINDOW(temp_window));

  return is_dark;
#else
  /* GTK4 < 4.10: fallback to checking app preference (not ideal but best we can do)
     gtk-application-prefer-dark-theme only indicates if the APP requested dark theme. */
  GtkSettings* settings = gtk_settings_get_default();
  gboolean dark = FALSE;
  g_object_get(settings, "gtk-application-prefer-dark-theme", &dark, NULL);
  return dark ? 1 : 0;
#endif
}

IUP_SDK_API void iupdrvSleep(int time)
{
  g_usleep(time*1000);  /* mili to micro */
}

IUP_SDK_API void iupdrvSetAccessibleTitle(Ihandle *ih, const char* title)
{
  GtkWidget* widget = (GtkWidget*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (!widget)
    widget = (GtkWidget*)ih->handle;

  if (!widget || !GTK_IS_WIDGET(widget))
    return;

  /* Set accessible label for screen readers */
  if (!title || title[0] == 0)
    gtk_accessible_update_property(GTK_ACCESSIBLE(widget), GTK_ACCESSIBLE_PROPERTY_LABEL, "", -1);
  else
    gtk_accessible_update_property(GTK_ACCESSIBLE(widget), GTK_ACCESSIBLE_PROPERTY_LABEL, title, -1);
}

int iupgtk4IsVisible(GtkWidget* widget)
{
  if (!widget || !GTK_IS_WIDGET(widget))
    return 0;

  return gtk_widget_get_visible(widget) && gtk_widget_get_child_visible(widget);
}

static void gtk4FocusEnter(GtkEventControllerFocus* controller, Ihandle* ih)
{
  (void)controller;

  if (!iupObjectCheck(ih))
    return;

  /* even when ACTIVE=NO the widget gets this event */
  if (!iupdrvIsActive(ih))
    return;

  iupCallGetFocusCb(ih);
}

static void gtk4FocusLeave(GtkEventControllerFocus* controller, Ihandle* ih)
{
  (void)controller;

  if (!iupObjectCheck(ih))
    return;

  iupCallKillFocusCb(ih);
}

void iupgtk4SetupFocusEvents(GtkWidget* widget, Ihandle* ih)
{
  GtkEventController* focus_controller = gtk_event_controller_focus_new();
  gtk_widget_add_controller(widget, focus_controller);
  g_signal_connect(focus_controller, "enter", G_CALLBACK(gtk4FocusEnter), ih);
  g_signal_connect(focus_controller, "leave", G_CALLBACK(gtk4FocusLeave), ih);
}

void iupgtk4SetupKeyEvents(GtkWidget* widget, Ihandle* ih)
{
  /* Create key controller for generic key event handling (K_ANY callback support) */
  GtkEventController* key_controller = gtk_event_controller_key_new();
  gtk_widget_add_controller(widget, key_controller);
  g_signal_connect(key_controller, "key-pressed", G_CALLBACK(iupgtk4KeyPressEvent), ih);
}

static void gtk4EnterNotify(GtkEventControllerMotion* controller, double x, double y, Ihandle* ih)
{
  Icallback cb;
  (void)controller;
  (void)x;
  (void)y;

  if (!iupObjectCheck(ih))
    return;

  cb = IupGetCallback(ih, "ENTERWINDOW_CB");
  if (cb)
    cb(ih);
}

static void gtk4LeaveNotify(GtkEventControllerMotion* controller, Ihandle* ih)
{
  Icallback cb;
  (void)controller;

  if (!iupObjectCheck(ih))
    return;

  cb = IupGetCallback(ih, "LEAVEWINDOW_CB");
  if (cb)
    cb(ih);
}

void iupgtk4SetupEnterLeaveEvents(GtkWidget* widget, Ihandle* ih)
{
  GtkEventController* motion_controller = gtk_event_controller_motion_new();
  gtk_widget_add_controller(widget, motion_controller);
  g_signal_connect(motion_controller, "enter", G_CALLBACK(gtk4EnterNotify), ih);
  g_signal_connect(motion_controller, "leave", G_CALLBACK(gtk4LeaveNotify), ih);
}

IUP_DRV_API void iupgtk4ButtonPressed(GtkGestureClick* gesture, int n_press, double x, double y, Ihandle* ih)
{
  IFniiiis cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
  if (cb)
  {
    int button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));
    int b = IUP_BUTTON1 + (button - 1);
    int doubleclick = (n_press == 2) ? 1 : 0;
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    GdkModifierType state = gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(gesture));

    iupgtk4ButtonKeySetStatus(state, button, status, doubleclick);

    int ret = cb(ih, b, 1, (int)x, (int)y, status);  /* press = 1 */
    if (ret == IUP_CLOSE)
      IupExitLoop();
    else if (ret == IUP_IGNORE)
    {
      /* Claiming the gesture prevents further event propagation */
      gtk_gesture_set_state(GTK_GESTURE(gesture), GTK_EVENT_SEQUENCE_CLAIMED);
    }
  }
}

static void gtk4ButtonReleased(GtkGestureClick* gesture, int n_press, double x, double y, Ihandle* ih)
{
  IFniiiis cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
  if (cb)
  {
    int button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));
    int b = IUP_BUTTON1 + (button - 1);
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    GdkModifierType state = gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(gesture));

    iupgtk4ButtonKeySetStatus(state, button, status, 0);

    int ret = cb(ih, b, 0, (int)x, (int)y, status);  /* press = 0 */
    if (ret == IUP_CLOSE)
      IupExitLoop();
    else if (ret == IUP_IGNORE)
      gtk_gesture_set_state(GTK_GESTURE(gesture), GTK_EVENT_SEQUENCE_CLAIMED);
  }

  (void)n_press;
}

static void gtk4MotionNotify(GtkEventControllerMotion* controller, double x, double y, Ihandle* ih)
{
  IFniis cb = (IFniis)IupGetCallback(ih, "MOTION_CB");
  if (cb)
  {
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    GdkModifierType state = gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(controller));

    iupgtk4ButtonKeySetStatus(state, 0, status, 0);

    cb(ih, (int)x, (int)y, status);
  }
}

void iupgtk4SetupButtonEvents(GtkWidget* widget, Ihandle* ih)
{
  GtkGesture* click_gesture = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click_gesture), 0);  /* 0 = all buttons */
  gtk_widget_add_controller(widget, GTK_EVENT_CONTROLLER(click_gesture));

  g_signal_connect(click_gesture, "pressed", G_CALLBACK(iupgtk4ButtonPressed), ih);
  g_signal_connect(click_gesture, "released", G_CALLBACK(gtk4ButtonReleased), ih);
}

void iupgtk4SetupMotionEvents(GtkWidget* widget, Ihandle* ih)
{
  GtkEventController* motion = gtk_event_controller_motion_new();
  gtk_widget_add_controller(widget, motion);

  g_signal_connect(motion, "motion", G_CALLBACK(gtk4MotionNotify), ih);
}

char* iupgtk4StrConvertToSystemLen(const char* str, int *len)
{
  /* GTK4 uses UTF-8 like GTK3, no conversion needed */
  if (len) *len = (int)strlen(str);
  return (char*)str;
}

char* iupgtk4StrConvertFromFilename(const char* str)
{
  /* GTK4 file names are already in UTF-8 */
  return (char*)str;
}

char* iupgtk4StrConvertToFilename(const char* str)
{
  /* GTK4 file names are already in UTF-8 */
  return (char*)str;
}

const char* iupgtk4GetNativeFontIdName(void)
{
  /* GTK4 uses Pango fonts like GTK3 */
  return "PANGOFONTDESC";
}

GtkWindow* iupgtk4GetTransientFor(Ihandle* ih)
{
  InativeHandle* parent = iupDialogGetNativeParent(ih);
  if (parent)
    return (GtkWindow*)parent;

  /* Try to find parent from focused element */
  {
    Ihandle* ih_focus = IupGetFocus();
    if (ih_focus)
    {
      Ihandle* dlg = IupGetDialog(ih_focus);
      if (dlg && dlg->handle)
        return (GtkWindow*)dlg->handle;
    }
  }

  /* Fallback: find first visible IUP dialog */
  {
    Ihandle* dlg_iter = iupDlgListFirst();
    while (dlg_iter)
    {
      if (dlg_iter->handle && dlg_iter != ih && iupdrvIsVisible(dlg_iter))
        return (GtkWindow*)dlg_iter->handle;
      dlg_iter = iupDlgListNext();
    }
  }

  return NULL;
}

#ifdef GDK_WINDOWING_X11
#include <gdk/x11/gdkx.h>
#include <dlfcn.h>

static void* x11_lib = NULL;
static int (*_XDefaultScreen)(Display*) = NULL;
static char* (*_XServerVendor)(Display*) = NULL;
static int (*_XVendorRelease)(Display*) = NULL;
static int (*_XMoveWindow)(Display*, Window, int, int) = NULL;
static int (*_XSync)(Display*, int) = NULL;
static Atom (*_XInternAtom)(Display*, const char*, int) = NULL;
static int (*_XSetWMNormalHints)(Display*, Window, XSizeHints*) = NULL;
static int (*_XChangeProperty)(Display*, Window, Atom, Atom, int, int, const unsigned char*, int) = NULL;

static int x11_load_functions(void)
{
  if (x11_lib)
    return 1;

  x11_lib = dlopen("libX11.so.6", RTLD_LAZY);
  if (!x11_lib)
    x11_lib = dlopen("libX11.so", RTLD_LAZY);

  if (!x11_lib)
    return 0;

  _XDefaultScreen = (int (*)(Display*))dlsym(x11_lib, "XDefaultScreen");
  _XServerVendor = (char* (*)(Display*))dlsym(x11_lib, "XServerVendor");
  _XVendorRelease = (int (*)(Display*))dlsym(x11_lib, "XVendorRelease");
  _XMoveWindow = (int (*)(Display*, Window, int, int))dlsym(x11_lib, "XMoveWindow");
  _XSync = (int (*)(Display*, int))dlsym(x11_lib, "XSync");
  _XInternAtom = (Atom (*)(Display*, const char*, int))dlsym(x11_lib, "XInternAtom");
  _XSetWMNormalHints = (int (*)(Display*, Window, XSizeHints*))dlsym(x11_lib, "XSetWMNormalHints");
  _XChangeProperty = (int (*)(Display*, Window, Atom, Atom, int, int, const unsigned char*, int))dlsym(x11_lib, "XChangeProperty");

  if (!_XDefaultScreen || !_XServerVendor || !_XVendorRelease || !_XMoveWindow || !_XSync || !_XSetWMNormalHints || !_XChangeProperty)
  {
    dlclose(x11_lib);
    x11_lib = NULL;
    return 0;
  }

  return 1;
}

int iupgtk4X11MoveWindow(void* xdisplay, unsigned long xwindow, int x, int y)
{
  XSizeHints hints;

  if (!x11_load_functions())
    return 0;

  memset(&hints, 0, sizeof(XSizeHints));
  hints.flags = PPosition | USPosition;
  hints.x = x;
  hints.y = y;
  _XSetWMNormalHints((Display*)xdisplay, (Window)xwindow, &hints);

  _XMoveWindow((Display*)xdisplay, (Window)xwindow, x, y);

  _XSync((Display*)xdisplay, 0);
  return 1;
}

int iupgtk4X11Sync(void* xdisplay)
{
  if (!x11_load_functions())
    return 0;

  _XSync((Display*)xdisplay, 0);
  return 1;
}

int iupgtk4X11HideFromTaskbar(void* xdisplay, unsigned long xwindow)
{
  Atom net_wm_window_type, net_wm_window_type_popup_menu;
  Atom net_wm_state, net_wm_state_skip_taskbar, net_wm_state_skip_pager;
  Atom states[2];

  if (!x11_load_functions())
    return 0;

  /* Set window type to POPUP_MENU */
  net_wm_window_type = _XInternAtom((Display*)xdisplay, "_NET_WM_WINDOW_TYPE", 0);
  net_wm_window_type_popup_menu = _XInternAtom((Display*)xdisplay, "_NET_WM_WINDOW_TYPE_POPUP_MENU", 0);

  _XChangeProperty((Display*)xdisplay, (Window)xwindow, net_wm_window_type, 4, 32, 0, (unsigned char*)&net_wm_window_type_popup_menu, 1);

  /* Skip taskbar and pager */
  net_wm_state = _XInternAtom((Display*)xdisplay, "_NET_WM_STATE", 0);
  net_wm_state_skip_taskbar = _XInternAtom((Display*)xdisplay, "_NET_WM_STATE_SKIP_TASKBAR", 0);
  net_wm_state_skip_pager = _XInternAtom((Display*)xdisplay, "_NET_WM_STATE_SKIP_PAGER", 0);

  states[0] = net_wm_state_skip_taskbar;
  states[1] = net_wm_state_skip_pager;

  _XChangeProperty((Display*)xdisplay, (Window)xwindow, net_wm_state, 4, 32, 0, (unsigned char*)states, 2);

  _XSync((Display*)xdisplay, 0);
  return 1;
}

int iupgtk4X11GetDefaultScreen(void* xdisplay)
{
  if (!x11_load_functions())
    return 0;
  return _XDefaultScreen((Display*)xdisplay);
}

char* iupgtk4X11GetServerVendor(void* xdisplay)
{
  if (!x11_load_functions())
    return NULL;
  return _XServerVendor((Display*)xdisplay);
}

int iupgtk4X11GetVendorRelease(void* xdisplay)
{
  if (!x11_load_functions())
    return 0;
  return _XVendorRelease((Display*)xdisplay);
}

void iupgtk4X11Cleanup(void)
{
  if (x11_lib)
  {
    dlclose(x11_lib);
    x11_lib = NULL;
  }
}
#endif

#ifdef GDK_WINDOWING_WIN32
#include <gdk/win32/gdkwin32.h>

int iupgtk4Win32MoveWindow(void* hwnd, int x, int y)
{
  if (!hwnd)
    return 0;

  SetWindowPos((HWND)hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
  return 1;
}

int iupgtk4Win32HideFromTaskbar(void* hwnd)
{
  LONG_PTR exstyle;

  if (!hwnd)
    return 0;

  exstyle = GetWindowLongPtr((HWND)hwnd, GWL_EXSTYLE);
  exstyle |= WS_EX_TOOLWINDOW;
  exstyle &= ~WS_EX_APPWINDOW;
  SetWindowLongPtr((HWND)hwnd, GWL_EXSTYLE, exstyle);

  return 1;
}

#endif

#ifdef GDK_WINDOWING_MACOS
#include <gdk/macos/gdkmacos.h>
#include <objc/runtime.h>
#include <objc/message.h>

typedef struct { double x; double y; } IupNSPoint;
typedef struct { IupNSPoint origin; IupNSPoint size; } IupNSRect;

int iupgtk4MacosMoveWindow(void* nswindow, int x, int y)
{
  SEL sel_setFrameOrigin;
  SEL sel_screen;
  SEL sel_frame;
  id screen;
  IupNSPoint origin;

  if (!nswindow)
    return 0;

  sel_setFrameOrigin = sel_registerName("setFrameOrigin:");
  sel_screen = sel_registerName("screen");
  sel_frame = sel_registerName("frame");

  screen = ((id (*)(id, SEL))objc_msgSend)((id)nswindow, sel_screen);
  if (screen)
  {
    IupNSRect screen_frame = ((IupNSRect (*)(id, SEL))objc_msgSend)(screen, sel_frame);
    origin.x = (double)x;
    origin.y = screen_frame.size.y - (double)y;
  }
  else
  {
    origin.x = (double)x;
    origin.y = (double)y;
  }

  ((void (*)(id, SEL, IupNSPoint))objc_msgSend)((id)nswindow, sel_setFrameOrigin, origin);

  return 1;
}

int iupgtk4MacosHideFromTaskbar(void* nswindow)
{
  SEL sel_setLevel;
  SEL sel_setCollectionBehavior;
  long popup_level = 101;
  unsigned long behavior;

  if (!nswindow)
    return 0;

  sel_setLevel = sel_registerName("setLevel:");
  sel_setCollectionBehavior = sel_registerName("setCollectionBehavior:");

  ((void (*)(id, SEL, long))objc_msgSend)((id)nswindow, sel_setLevel, popup_level);

  behavior = (1 << 7) | (1 << 9);
  ((void (*)(id, SEL, unsigned long))objc_msgSend)((id)nswindow, sel_setCollectionBehavior, behavior);

  return 1;
}
#endif
