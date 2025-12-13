/** \file
 * \brief GTK4 Font mapping
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>

#include "iup.h"

#include "iup_str.h"
#include "iup_attrib.h"
#include "iup_array.h"
#include "iup_object.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_assert.h"

#include "iupgtk4_drv.h"


typedef struct _Igtk4Font
{
  char font[200];
  PangoFontDescription* fontdesc;
  PangoAttribute* strikethrough;
  PangoAttribute* underline;
  PangoLayout* layout;
  int charwidth, charheight;
} Igtk4Font;

static Iarray* gtk4_fonts = NULL;
static PangoContext *gtk4_fonts_context = NULL;

static void gtk4FontUpdateLayout(Igtk4Font* gtkfont, PangoLayout* layout)
{
  PangoAttrList *attrs;

  pango_layout_set_font_description(layout, gtkfont->fontdesc);

  attrs = pango_layout_get_attributes(layout);
  if (!attrs)
  {
    attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attribute_copy(gtkfont->strikethrough));
    pango_attr_list_insert(attrs, pango_attribute_copy(gtkfont->underline));
    pango_layout_set_attributes(layout, attrs);
    pango_attr_list_unref(attrs);
  }
  else
  {
    pango_attr_list_change(attrs, pango_attribute_copy(gtkfont->strikethrough));
    pango_attr_list_change(attrs, pango_attribute_copy(gtkfont->underline));
  }
}

static Igtk4Font* gtk4FindFont(const char *font)
{
  PangoFontMetrics* metrics;
  PangoFontDescription* fontdesc;
  int i,
      is_underline = 0,
      is_strikeout = 0,
      count = iupArrayCount(gtk4_fonts);

  Igtk4Font* fonts = (Igtk4Font*)iupArrayGetData(gtk4_fonts);

  /* Check if the font already exists in cache */
  for (i = 0; i < count; i++)
  {
    if (iupStrEqualNoCase(font, fonts[i].font))
      return &fonts[i];
  }

  /* not found, create a new one */
  {
    int size = 8, is_pango = 0;
    int is_bold = 0,
      is_italic = 0;
    char typeface[1024];
    const char* mapped_name;

    /* same as iupGetFontInfo, but mark if pango  */
    if (!iupFontParseWin(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    {
      if (!iupFontParseX(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
      {
        if (!iupFontParsePango(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
          return NULL;
        else
          is_pango = 1;
      }
    }

    /* Map standard names to native names */
    mapped_name = iupFontGetPangoName(typeface);
    if (mapped_name)
    {
      strcpy(typeface, mapped_name);
      is_pango = 0;
    }

    if (is_pango && !is_underline && !is_strikeout && size>0)
      fontdesc = pango_font_description_from_string(font);
    else
    {
      char new_font[200];
      if (size<0)
      {
        double res = 96.0;  /* default DPI */
        GdkDisplay *display = gdk_display_get_default();
        if (display)
        {
          GListModel *monitors = gdk_display_get_monitors(display);
          GdkMonitor *monitor = g_list_model_get_item(monitors, 0);
          if (monitor)
          {
            /* Apply scale factor to base DPI */
            int scale_factor = gdk_monitor_get_scale_factor(monitor);
            res = 96.0 * scale_factor;
            g_object_unref(monitor);
          }
        }

        size = iupRound((-size * 72.0) / res); /* from pixels to points */
      }

      sprintf(new_font, "%s, %s%s%d", typeface, is_bold?"Bold ":"", is_italic?"Italic ":"", size);

      fontdesc = pango_font_description_from_string(new_font);
    }
  }

  if (!fontdesc)
    return NULL;

  /* create room in the array */
  fonts = (Igtk4Font*)iupArrayInc(gtk4_fonts);

  strcpy(fonts[i].font, font);

  /* these are all released in iupdrvFontFinish */
  fonts[i].fontdesc = fontdesc;
  fonts[i].strikethrough = pango_attr_strikethrough_new(is_strikeout? TRUE: FALSE);
  fonts[i].underline = pango_attr_underline_new(is_underline? PANGO_UNDERLINE_SINGLE: PANGO_UNDERLINE_NONE);
  fonts[i].layout = pango_layout_new(gtk4_fonts_context);

  metrics = pango_context_get_metrics(gtk4_fonts_context, fontdesc, pango_context_get_language(gtk4_fonts_context));
  fonts[i].charheight = pango_font_metrics_get_ascent(metrics) + pango_font_metrics_get_descent(metrics);
  fonts[i].charheight = iupGTK4_PANGOUNITS2PIXELS(fonts[i].charheight);
  fonts[i].charwidth = pango_font_metrics_get_approximate_char_width(metrics);
  fonts[i].charwidth = iupGTK4_PANGOUNITS2PIXELS(fonts[i].charwidth);
  pango_font_metrics_unref(metrics);

  gtk4FontUpdateLayout(&(fonts[i]), fonts[i].layout);  /* for strikeout and underline */

  return &fonts[i];
}

static Igtk4Font* gtk4FontCreateNativeFont(Ihandle* ih, const char* value)
{
  Igtk4Font *gtkfont = gtk4FindFont(value);
  if (!gtkfont)
  {
    iupERROR1("Failed to create Font: %s", value);
    return NULL;
  }

  iupAttribSet(ih, "_IUP_GTK4FONT", (char*)gtkfont);
  return gtkfont;
}

static Igtk4Font* gtk4FontGet(Ihandle *ih)
{
  Igtk4Font* gtkfont = (Igtk4Font*)iupAttribGet(ih, "_IUP_GTK4FONT");
  if (!gtkfont)
  {
    gtkfont = gtk4FontCreateNativeFont(ih, iupGetFontValue(ih));
    if (!gtkfont)
      gtkfont = gtk4FontCreateNativeFont(ih, IupGetGlobal("DEFAULTFONT"));
  }
  return gtkfont;
}

static void gtk4FontUpdateWidget(Ihandle* ih, GtkWidget* widget, PangoFontDescription* fontdesc)
{
  Igtk4Font* gtkfont;

  /* gtk_widget_override_font removed
     CSS is now the primary way to set fonts, but we can still use Pango contexts */

  /* For labels, we can use Pango attributes */
  gtkfont = gtk4FontGet(ih);
  if (!gtkfont)
    return;

  if (GTK_IS_LABEL(widget))
  {
    PangoAttrList *attrs = pango_attr_list_new();

    /* Add font description as attribute */
    PangoAttribute *font_attr = pango_attr_font_desc_new(fontdesc);
    pango_attr_list_insert(attrs, font_attr);

    pango_attr_list_insert(attrs, pango_attribute_copy(gtkfont->strikethrough));
    pango_attr_list_insert(attrs, pango_attribute_copy(gtkfont->underline));
    gtk_label_set_attributes (GTK_LABEL(widget), attrs);
    pango_attr_list_unref(attrs);
  }
  else if (GTK_IS_EDITABLE(widget))
  {
    /* GtkEntry is now GtkText under GtkEditable interface. For editable widgets, we need to use CSS */
    char css_font[512];
    const char *family = pango_font_description_get_family(fontdesc);
    int size = pango_font_description_get_size(fontdesc);

    if (size > 0)
    {
      if (pango_font_description_get_size_is_absolute(fontdesc))
        sprintf(css_font, "font-family: %s; font-size: %dpx;", family ? family : "sans", size / PANGO_SCALE);
      else
        sprintf(css_font, "font-family: %s; font-size: %dpt;", family ? family : "sans", size / PANGO_SCALE);

      iupgtk4CssSetWidgetFont(widget, css_font);
    }
  }
}

void iupgtk4UpdateWidgetFont(Ihandle *ih, GtkWidget* widget)
{
  PangoFontDescription* fontdesc = (PangoFontDescription*)iupgtk4GetPangoFontDescAttrib(ih);
  gtk4FontUpdateWidget(ih, widget, fontdesc);
}

void iupgtk4UpdateObjectFont(Ihandle* ih, gpointer object)
{
  PangoAttrList *attrs;

  Igtk4Font* gtkfont = gtk4FontGet(ih);
  if (!gtkfont)
    return;

  g_object_set(object, "font-desc", gtkfont->fontdesc, NULL);

  g_object_get(object, "attributes", &attrs, NULL);
  if (!attrs)
  {
    attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attribute_copy(gtkfont->strikethrough));
    pango_attr_list_insert(attrs, pango_attribute_copy(gtkfont->underline));
    g_object_set(object, "attributes", attrs, NULL);
  }
  else
  {
    pango_attr_list_change(attrs, pango_attribute_copy(gtkfont->strikethrough));
    pango_attr_list_change(attrs, pango_attribute_copy(gtkfont->underline));
  }
}

IUP_SDK_API char* iupdrvGetSystemFont(void)
{
  static char str[200]; /* must return a static string, because it will be used as the default value for the FONT attribute */

  /* gtk_style_context_get removed - use CSS and default font */
  GtkSettings *settings = gtk_settings_get_default();
  gchar *font_name = NULL;

  if (settings)
    g_object_get(settings, "gtk-font-name", &font_name, NULL);

  if (font_name)
  {
    strncpy(str, font_name, sizeof(str) - 1);
    str[sizeof(str) - 1] = '\0';
    g_free(font_name);
  }
  else
    strcpy(str, "Sans, 10");

  return str;
}

PangoLayout* iupgtk4GetPangoLayout(const char* value)
{
  Igtk4Font *gtkfont = gtk4FindFont(value);
  if (gtkfont)
    return gtkfont->layout;
  else
    return NULL;
}

char* iupgtk4GetPangoLayoutAttrib(Ihandle *ih)
{
  Igtk4Font* gtkfont = gtk4FontGet(ih);
  if (gtkfont)
    return (char*)gtkfont->layout;
  else
    return NULL;
}

PangoFontDescription* iupgtk4GetPangoFontDesc(const char* value)
{
  Igtk4Font *gtkfont = gtk4FindFont(value);
  if (gtkfont)
    return gtkfont->fontdesc;
  else
    return NULL;
}

char* iupgtk4GetPangoFontDescAttrib(Ihandle *ih)
{
  Igtk4Font* gtkfont = gtk4FontGet(ih);
  if (gtkfont)
    return (char*)gtkfont->fontdesc;
  else
    return NULL;
}

char* iupgtk4GetFontIdAttrib(Ihandle *ih)
{
  /* GdkFont completely removed - no font IDs available */
  (void)ih;
  return NULL;
}

IUP_SDK_API int iupdrvSetFontAttrib(Ihandle* ih, const char* value)
{
  Igtk4Font* gtkfont = gtk4FontCreateNativeFont(ih, value);
  if (!gtkfont)
    return 0;

  /* If FONT is changed, must update the SIZE attribute */
  iupBaseUpdateAttribFromFont(ih);

  /* FONT attribute must be able to be set before mapping, so the font is enable for size calculation. */
  if (ih->handle && (ih->iclass->nativetype != IUP_TYPEVOID))
    gtk4FontUpdateWidget(ih, ih->handle, gtkfont->fontdesc);

  return 1;
}

static void gtk4FontGetTextSize(Ihandle* ih, Igtk4Font* gtkfont, const char* str, int len, int *w, int *h)
{
  int max_w = 0, line_count = 1;

  if (!gtkfont)
  {
    if (w) *w = 0;
    if (h) *h = 0;
    return;
  }

  if (!str)
  {
    if (w) *w = 0;
    if (h) *h = gtkfont->charheight * 1;
    return;
  }

  if (h)
    line_count = iupStrLineCount(str, len);

  if (str[0])
  {
    int dummy_h;
    int orig_len = len;
    char* text = iupgtk4StrConvertToSystemLen(str, &len);

    if (strstr(str, "star") != NULL || strstr(str, "image") != NULL) {
    }

    if (iupAttribGetBoolean(ih, "MARKUP"))
    {
      pango_layout_set_attributes(gtkfont->layout, NULL);
      pango_layout_set_markup(gtkfont->layout, text, len);
    }
    else
      pango_layout_set_text(gtkfont->layout, text, len);

    pango_layout_get_pixel_size(gtkfont->layout, &max_w, &dummy_h);

    if (strstr(str, "star") != NULL || strstr(str, "image") != NULL) {
    }
  }

  if (w) *w = max_w;
  if (h) *h = gtkfont->charheight * line_count;
}

IUP_SDK_API void iupdrvFontGetMultiLineStringSize(Ihandle* ih, const char* str, int *w, int *h)
{
  Igtk4Font* gtkfont = gtk4FontGet(ih);
  if (gtkfont)
    gtk4FontGetTextSize(ih, gtkfont, str, str? (int)strlen(str): 0, w, h);
}

IUP_SDK_API void iupdrvFontGetTextSize(const char* font, const char* str, int len, int *w, int *h)
{
  Igtk4Font *gtkfont = gtk4FindFont(font);
  if (gtkfont)
    gtk4FontGetTextSize(NULL, gtkfont, str, len, w, h);
}

IUP_SDK_API void iupdrvFontGetFontDim(const char* font, int *max_width, int *line_height, int *ascent, int *descent)
{
  Igtk4Font *gtkfont = gtk4FindFont(font);
  if (gtkfont)
  {
    PangoFontMetrics* metrics;
    int charwidth, charheight, charascent, chardescent;

    if (!gtkfont->fontdesc)
      return;

    metrics = pango_context_get_metrics(gtk4_fonts_context, gtkfont->fontdesc, pango_context_get_language(gtk4_fonts_context));
    charascent = pango_font_metrics_get_ascent(metrics);
    chardescent = pango_font_metrics_get_descent(metrics);
    charheight = charascent + chardescent;
    charwidth = pango_font_metrics_get_approximate_char_width(metrics);

    if (max_width)   *max_width = (((charwidth)+PANGO_SCALE / 2) / PANGO_SCALE);
    if (line_height) *line_height = (((charheight)+PANGO_SCALE / 2) / PANGO_SCALE);
    if (ascent)      *ascent = (((charascent)+PANGO_SCALE / 2) / PANGO_SCALE);
    if (descent)     *descent = (((chardescent)+PANGO_SCALE / 2) / PANGO_SCALE);

    pango_font_metrics_unref(metrics);
  }
}

IUP_SDK_API int iupdrvFontGetStringWidth(Ihandle* ih, const char* str)
{
  Igtk4Font* gtkfont;
  int len, w;
  const char* line_end, *text;

  if (!str || str[0]==0)
    return 0;

  gtkfont = gtk4FontGet(ih);
  if (!gtkfont)
    return 0;

  /* do it only for the first line, if any */
  line_end = strchr(str, '\n');
  if (line_end)
    len = (int)(line_end-str);
  else
    len = (int)strlen(str);

  text = iupgtk4StrConvertToSystemLen(str, &len);

  if (iupAttribGetBoolean(ih, "MARKUP"))
  {
    pango_layout_set_attributes(gtkfont->layout, NULL);
    pango_layout_set_markup(gtkfont->layout, text, len);
  }
  else
    pango_layout_set_text(gtkfont->layout, text, len);

  pango_layout_get_pixel_size(gtkfont->layout, &w, NULL);

  return w;
}

IUP_SDK_API void iupdrvFontGetCharSize(Ihandle* ih, int *charwidth, int *charheight)
{
  Igtk4Font* gtkfont = gtk4FontGet(ih);
  if (!gtkfont)
  {
    if (charwidth)  *charwidth = 0;
    if (charheight) *charheight = 0;
    return;
  }

  if (charheight)
    *charheight = gtkfont->charheight;

  if (charwidth)
    *charwidth = gtkfont->charwidth;
}

void iupdrvFontInit(void)
{
  gtk4_fonts = iupArrayCreate(50, sizeof(Igtk4Font));

  /* gdk_display_create_pango_context removed. Use pango_font_map_create_context instead */
  PangoFontMap *font_map = pango_cairo_font_map_get_default();
  gtk4_fonts_context = pango_font_map_create_context(font_map);
  pango_context_set_language(gtk4_fonts_context, gtk_get_default_language());
}

void iupdrvFontFinish(void)
{
  int i, count = iupArrayCount(gtk4_fonts);
  Igtk4Font* fonts = (Igtk4Font*)iupArrayGetData(gtk4_fonts);
  for (i = 0; i < count; i++)
  {
    pango_font_description_free(fonts[i].fontdesc);
    g_object_unref(fonts[i].layout);
    pango_attribute_destroy(fonts[i].strikethrough);
    pango_attribute_destroy(fonts[i].underline);
  }
  iupArrayDestroy(gtk4_fonts);
  g_object_unref(gtk4_fonts_context);
}
