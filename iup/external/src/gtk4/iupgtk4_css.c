/** \file
 * \brief GTK4 CSS Manager
 *
 * Display-wide CSS with per-widget classes for styling.
 *
 * See Copyright Notice in "iup.h"
 */

#include <gtk/gtk.h>

#include "iup.h"
#include "iup_object.h"

#include "iupgtk4_drv.h"

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

static guint gtk4_css_idle_id = 0;

static void gtk4CssDoRebuild(void)
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

static gboolean gtk4CssIdleRebuild(gpointer data)
{
  (void)data;
  gtk4_css_idle_id = 0;
  gtk4CssDoRebuild();
  return G_SOURCE_REMOVE;
}

static void gtk4CssRebuildAndApply(void)
{
  if (!gtk4_css_provider)
    return;

  if (gtk4_css_idle_id == 0)
    gtk4_css_idle_id = g_idle_add_full(G_PRIORITY_HIGH_IDLE, gtk4CssIdleRebuild, NULL, NULL);
}

IUP_DRV_API void iupgtk4CssFlush(void)
{
  if (gtk4_css_idle_id)
  {
    g_source_remove(gtk4_css_idle_id);
    gtk4_css_idle_id = 0;
    gtk4CssDoRebuild();
  }
}

static void gtk4CssWidgetDestroyed(gpointer data, GObject* where_the_object_was)
{
  (void)data;
  if (gtk4_widget_styles)
    g_hash_table_remove(gtk4_widget_styles, where_the_object_was);
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

    g_object_weak_ref(G_OBJECT(widget), gtk4CssWidgetDestroyed, NULL);
  }

  return style;
}

IUP_DRV_API void iupgtk4CssManagerInit(void)
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

IUP_DRV_API void iupgtk4CssManagerFinish(void)
{
  if (gtk4_css_idle_id)
  {
    g_source_remove(gtk4_css_idle_id);
    gtk4_css_idle_id = 0;
  }

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

IUP_DRV_API void iupgtk4CssSetWidgetBgColor(GtkWidget* widget, unsigned char r, unsigned char g, unsigned char b, int is_text)
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

IUP_DRV_API void iupgtk4CssSetWidgetFgColor(GtkWidget* widget, unsigned char r, unsigned char g, unsigned char b)
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

IUP_DRV_API void iupgtk4CssSetWidgetPadding(GtkWidget* widget, int horiz, int vert)
{
  Igtk4WidgetStyle* style;

  if (!gtk4_css_provider)
    iupgtk4CssManagerInit();

  style = gtk4CssGetOrCreateWidgetStyle(widget);

  g_free(style->padding_css);
  style->padding_css = g_strdup_printf("padding: %dpx %dpx;", vert, horiz);

  gtk4CssRebuildAndApply();
}

IUP_DRV_API void iupgtk4CssSetWidgetFont(GtkWidget* widget, const char* font_css)
{
  Igtk4WidgetStyle* style;

  if (!gtk4_css_provider)
    iupgtk4CssManagerInit();

  style = gtk4CssGetOrCreateWidgetStyle(widget);

  g_free(style->font_css);
  style->font_css = g_strdup(font_css);

  gtk4CssRebuildAndApply();
}

IUP_DRV_API void iupgtk4CssSetWidgetCustom(GtkWidget* widget, const char* css_property, const char* css_value)
{
  Igtk4WidgetStyle* style;

  if (!gtk4_css_provider)
    iupgtk4CssManagerInit();

  style = gtk4CssGetOrCreateWidgetStyle(widget);

  g_free(style->custom_css);
  style->custom_css = g_strdup_printf("%s: %s;", css_property, css_value);

  gtk4CssRebuildAndApply();
}

IUP_DRV_API void iupgtk4CssClearWidgetStyle(GtkWidget* widget)
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

IUP_DRV_API void iupgtk4CssResetWidgetPadding(GtkWidget* widget)
{
  Igtk4WidgetStyle* style;

  if (!gtk4_widget_styles)
    return;

  style = g_hash_table_lookup(gtk4_widget_styles, widget);
  if (style && style->padding_css)
  {
    g_free(style->padding_css);
    style->padding_css = NULL;
    gtk4CssRebuildAndApply();
  }
}

IUP_DRV_API void iupgtk4CssResetWidgetCustom(GtkWidget* widget)
{
  Igtk4WidgetStyle* style;

  if (!gtk4_widget_styles)
    return;

  style = g_hash_table_lookup(gtk4_widget_styles, widget);
  if (style && style->custom_css)
  {
    g_free(style->custom_css);
    style->custom_css = NULL;
    gtk4CssRebuildAndApply();
  }
}

IUP_DRV_API void iupgtk4CssAddStaticRule(const char* selector, const char* css_rules)
{
  if (!gtk4_css_provider)
    iupgtk4CssManagerInit();

  g_hash_table_insert(gtk4_static_rules, g_strdup(selector), g_strdup(css_rules));

  gtk4CssRebuildAndApply();
}
