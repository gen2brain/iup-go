/** \file
 * \brief GTK4 CSS Manager
 *
 * Display-wide CSS with content-addressed shared classes for styling.
 *
 * See Copyright Notice in "iup.h"
 */

#include <string.h>

#include "iup.h"
#include "iup_object.h"

#include "iupgtk4_drv.h"

typedef struct {
  char* bg_css;
  char* fg_css;
  char* padding_css;
  char* font_css;
  char* custom_css;
  GHashTable* sub_rules;
  char* class_name;
} Igtk4WidgetStyle;

static GtkCssProvider* gtk4_css_provider = NULL;
static GHashTable* gtk4_widget_styles = NULL;
static GHashTable* gtk4_dirty_widgets = NULL;
static GHashTable* gtk4_style_classes = NULL;
static GString* gtk4_styles_css = NULL;
static GString* gtk4_css_buffer = NULL;
static GHashTable* gtk4_static_rules = NULL;
static guint gtk4_css_idle_id = 0;
static guint gtk4_class_count = 0;
static int gtk4_provider_dirty = 0;

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
    if (style->sub_rules)
      g_hash_table_destroy(style->sub_rules);
    g_free(style->class_name);
    g_free(style);
  }
}

static int gtk4CssStyleIsEmpty(Igtk4WidgetStyle* style)
{
  return !style->bg_css && !style->fg_css && !style->padding_css &&
         !style->font_css && !style->custom_css &&
         (!style->sub_rules || g_hash_table_size(style->sub_rules) == 0);
}

static void gtk4CssAppendRules(GString* out, const char* class_name, Igtk4WidgetStyle* style)
{
  g_string_append_printf(out, ".%s {\n", class_name);

  if (style->bg_css)
    g_string_append_printf(out, "  %s\n", style->bg_css);
  if (style->fg_css)
    g_string_append_printf(out, "  %s\n", style->fg_css);
  if (style->padding_css)
    g_string_append_printf(out, "  %s\n", style->padding_css);
  if (style->font_css)
    g_string_append_printf(out, "  %s\n", style->font_css);
  if (style->custom_css)
    g_string_append_printf(out, "  %s\n", style->custom_css);

  g_string_append(out, "}\n");

  if (style->bg_css)
    g_string_append_printf(out, ".%s:disabled { %s }\n", class_name, style->bg_css);

  if (style->fg_css)
  {
    g_string_append_printf(out, ".%s:hover { %s }\n", class_name, style->fg_css);
    g_string_append_printf(out, ".%s:active { %s }\n", class_name, style->fg_css);
  }

  if (style->sub_rules && g_hash_table_size(style->sub_rules) > 0)
  {
    GList *keys, *l;
    keys = g_hash_table_get_keys(style->sub_rules);
    keys = g_list_sort(keys, (GCompareFunc)strcmp);
    for (l = keys; l; l = l->next)
      g_string_append_printf(out, ".%s%s { %s }\n", class_name, (char*)l->data,
                             (char*)g_hash_table_lookup(style->sub_rules, l->data));
    g_list_free(keys);
  }
}

static void gtk4CssLoadProvider(void)
{
  GHashTableIter iter;
  gpointer key, value;

  g_string_truncate(gtk4_css_buffer, 0);

  if (gtk4_static_rules)
  {
    g_hash_table_iter_init(&iter, gtk4_static_rules);
    while (g_hash_table_iter_next(&iter, &key, &value))
      g_string_append_printf(gtk4_css_buffer, "%s { %s }\n", (char*)key, (char*)value);
  }

  g_string_append_len(gtk4_css_buffer, gtk4_styles_css->str, gtk4_styles_css->len);

  gtk_css_provider_load_from_string(gtk4_css_provider, gtk4_css_buffer->str);
  gtk4_provider_dirty = 0;
}

static void gtk4CssApplyWidget(GtkWidget* widget, Igtk4WidgetStyle* style)
{
  char* new_class = NULL;

  if (!gtk4CssStyleIsEmpty(style))
  {
    GString* key = g_string_new(NULL);
    gtk4CssAppendRules(key, "@", style);

    new_class = g_hash_table_lookup(gtk4_style_classes, key->str);
    if (!new_class)
    {
      new_class = g_strdup_printf("iup-s-%u", ++gtk4_class_count);
      gtk4CssAppendRules(gtk4_styles_css, new_class, style);
      g_hash_table_insert(gtk4_style_classes, g_string_free(key, FALSE), new_class);
      key = NULL;
      gtk4_provider_dirty = 1;
    }

    if (key)
      g_string_free(key, TRUE);
  }

  if (g_strcmp0(style->class_name, new_class) != 0)
  {
    if (style->class_name)
      gtk_widget_remove_css_class(widget, style->class_name);
    if (new_class)
      gtk_widget_add_css_class(widget, new_class);
    g_free(style->class_name);
    style->class_name = g_strdup(new_class);
  }
}

static void gtk4CssDoApply(void)
{
  GHashTableIter iter;
  gpointer key;

  if (!gtk4_css_provider)
    return;

  g_hash_table_iter_init(&iter, gtk4_dirty_widgets);
  while (g_hash_table_iter_next(&iter, &key, NULL))
  {
    Igtk4WidgetStyle* style = g_hash_table_lookup(gtk4_widget_styles, key);
    if (style)
      gtk4CssApplyWidget((GtkWidget*)key, style);
  }
  g_hash_table_remove_all(gtk4_dirty_widgets);

  if (gtk4_provider_dirty)
    gtk4CssLoadProvider();
}

static gboolean gtk4CssIdleApply(gpointer data)
{
  (void)data;
  gtk4_css_idle_id = 0;
  gtk4CssDoApply();
  return G_SOURCE_REMOVE;
}

static void gtk4CssSchedule(void)
{
  if (!gtk4_css_provider)
    return;

  if (gtk4_css_idle_id == 0)
    gtk4_css_idle_id = g_idle_add_full(G_PRIORITY_HIGH_IDLE, gtk4CssIdleApply, NULL, NULL);
}

static void gtk4CssMarkDirty(GtkWidget* widget)
{
  g_hash_table_add(gtk4_dirty_widgets, widget);
  gtk4CssSchedule();
}

IUP_DRV_API void iupgtk4CssFlush(void)
{
  if (gtk4_css_idle_id)
  {
    g_source_remove(gtk4_css_idle_id);
    gtk4_css_idle_id = 0;
    gtk4CssDoApply();
  }
}

static void gtk4CssWidgetDestroyed(gpointer data, GObject* where_the_object_was)
{
  (void)data;
  if (gtk4_dirty_widgets)
    g_hash_table_remove(gtk4_dirty_widgets, where_the_object_was);
  if (gtk4_widget_styles)
    g_hash_table_remove(gtk4_widget_styles, where_the_object_was);
}

static Igtk4WidgetStyle* gtk4CssGetOrCreateWidgetStyle(GtkWidget* widget)
{
  Igtk4WidgetStyle* style = g_hash_table_lookup(gtk4_widget_styles, widget);

  if (!style)
  {
    style = g_new0(Igtk4WidgetStyle, 1);
    g_hash_table_insert(gtk4_widget_styles, widget, style);

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
  gtk4_dirty_widgets = g_hash_table_new(g_direct_hash, g_direct_equal);
  gtk4_style_classes = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
  gtk4_styles_css = g_string_new("");
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

  if (gtk4_dirty_widgets)
  {
    g_hash_table_destroy(gtk4_dirty_widgets);
    gtk4_dirty_widgets = NULL;
  }

  if (gtk4_style_classes)
  {
    g_hash_table_destroy(gtk4_style_classes);
    gtk4_style_classes = NULL;
  }

  if (gtk4_styles_css)
  {
    g_string_free(gtk4_styles_css, TRUE);
    gtk4_styles_css = NULL;
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

  gtk4_class_count = 0;
  gtk4_provider_dirty = 0;
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

  gtk4CssMarkDirty(widget);
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

  gtk4CssMarkDirty(widget);
}

IUP_DRV_API void iupgtk4CssSetWidgetPadding(GtkWidget* widget, int horiz, int vert)
{
  Igtk4WidgetStyle* style;

  if (!gtk4_css_provider)
    iupgtk4CssManagerInit();

  style = gtk4CssGetOrCreateWidgetStyle(widget);

  g_free(style->padding_css);
  style->padding_css = g_strdup_printf("padding: %dpx %dpx;", vert, horiz);

  gtk4CssMarkDirty(widget);
}

IUP_DRV_API void iupgtk4CssSetWidgetFont(GtkWidget* widget, const char* font_css)
{
  Igtk4WidgetStyle* style;

  if (!gtk4_css_provider)
    iupgtk4CssManagerInit();

  style = gtk4CssGetOrCreateWidgetStyle(widget);

  g_free(style->font_css);
  style->font_css = g_strdup(font_css);

  gtk4CssMarkDirty(widget);
}

IUP_DRV_API void iupgtk4CssSetWidgetCustom(GtkWidget* widget, const char* css_property, const char* css_value)
{
  Igtk4WidgetStyle* style;

  if (!gtk4_css_provider)
    iupgtk4CssManagerInit();

  style = gtk4CssGetOrCreateWidgetStyle(widget);

  g_free(style->custom_css);
  style->custom_css = g_strdup_printf("%s: %s;", css_property, css_value);

  gtk4CssMarkDirty(widget);
}

IUP_DRV_API void iupgtk4CssSetWidgetSubRule(GtkWidget* widget, const char* sub_selector, const char* css_decl)
{
  Igtk4WidgetStyle* style;

  if (!gtk4_css_provider)
    iupgtk4CssManagerInit();

  style = gtk4CssGetOrCreateWidgetStyle(widget);

  if (!style->sub_rules)
    style->sub_rules = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

  if (css_decl)
    g_hash_table_insert(style->sub_rules, g_strdup(sub_selector), g_strdup(css_decl));
  else
    g_hash_table_remove(style->sub_rules, sub_selector);

  gtk4CssMarkDirty(widget);
}

IUP_DRV_API void iupgtk4CssClearWidgetStyle(GtkWidget* widget)
{
  Igtk4WidgetStyle* style;

  if (!gtk4_widget_styles)
    return;

  style = g_hash_table_lookup(gtk4_widget_styles, widget);
  if (style)
  {
    if (style->class_name)
      gtk_widget_remove_css_class(widget, style->class_name);

    g_hash_table_remove(gtk4_dirty_widgets, widget);
    g_hash_table_remove(gtk4_widget_styles, widget);
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
    gtk4CssMarkDirty(widget);
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
    gtk4CssMarkDirty(widget);
  }
}

IUP_DRV_API void iupgtk4CssAddStaticRule(const char* selector, const char* css_rules)
{
  if (!gtk4_css_provider)
    iupgtk4CssManagerInit();

  g_hash_table_insert(gtk4_static_rules, g_strdup(selector), g_strdup(css_rules));

  gtk4_provider_dirty = 1;
  gtk4CssSchedule();
}
