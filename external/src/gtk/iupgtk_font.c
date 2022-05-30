/** \file
 * \brief GTK Font mapping
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

#include "iupgtk_drv.h"


typedef struct _IgtkFont
{
  char font[200];
  PangoFontDescription* fontdesc;
  PangoAttribute* strikethrough;
  PangoAttribute* underline;
  PangoLayout* layout;
  int charwidth, charheight;
} IgtkFont;

static Iarray* gtk_fonts = NULL;
static PangoContext *gtk_fonts_context = NULL;

static void gtkFontUpdateLayout(IgtkFont* gtkfont, PangoLayout* layout)
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

#if 0
static int gtkFontTypefaceCheck(const char* typeface)
{
  PangoFontFamily **families = NULL;
  int i, n_families = 0;
  pango_context_list_families(gtk_fonts_context,  &families, &n_families);

  for (i = 0; i < n_families; i++)
  {
    if (iupStrEqualNoCase(typeface, pango_font_family_get_name(families[i])))
    {
      g_free(families);
      return 1;
    }
  }

  g_free(families);
  return 0;
}
#endif

static IgtkFont* gtkFindFont(const char *font)
{
  PangoFontMetrics* metrics;
  PangoFontDescription* fontdesc;
  int i, 
      is_underline = 0,
      is_strikeout = 0,
      count = iupArrayCount(gtk_fonts);

  IgtkFont* fonts = (IgtkFont*)iupArrayGetData(gtk_fonts);

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
#if GTK_CHECK_VERSION(2, 10, 0)
        double res = gdk_screen_get_resolution(gdk_screen_get_default()); /* dpi */
#else
        double res = ((double)gdk_screen_get_width(gdk_screen_get_default()) / 
                      (double)gdk_screen_get_width_mm(gdk_screen_get_default())); /* pixels/mm */
        res *= 25.4; /* dpi */
#endif
        /* The default value is 96, meaning that a 10 point font will be 13 pixels high. 
           (10 * 96 / 72 = 13.3) */
        /* 1 point = 1/72 inch  */
        /* points = (pixels*72)/dpi */
        size = iupRound((-size * 72.0) / res); /* from pixels to points */
      }

      sprintf(new_font, "%s, %s%s%d", typeface, is_bold?"Bold ":"", is_italic?"Italic ":"", size);

      fontdesc = pango_font_description_from_string(new_font);
    }
  }

  if (!fontdesc) 
    return NULL;

  /* create room in the array */
  fonts = (IgtkFont*)iupArrayInc(gtk_fonts);

  strcpy(fonts[i].font, font);

  /* these are all released in iupdrvFontFinish */
  fonts[i].fontdesc = fontdesc;
  fonts[i].strikethrough = pango_attr_strikethrough_new(is_strikeout? TRUE: FALSE);
  fonts[i].underline = pango_attr_underline_new(is_underline? PANGO_UNDERLINE_SINGLE: PANGO_UNDERLINE_NONE);
  fonts[i].layout = pango_layout_new(gtk_fonts_context);

  metrics = pango_context_get_metrics(gtk_fonts_context, fontdesc, pango_context_get_language(gtk_fonts_context));
  fonts[i].charheight = pango_font_metrics_get_ascent(metrics) + pango_font_metrics_get_descent(metrics);
  fonts[i].charheight = iupGTK_PANGOUNITS2PIXELS(fonts[i].charheight);
  fonts[i].charwidth = pango_font_metrics_get_approximate_char_width(metrics);
  fonts[i].charwidth = iupGTK_PANGOUNITS2PIXELS(fonts[i].charwidth);
  pango_font_metrics_unref(metrics); 

  gtkFontUpdateLayout(&(fonts[i]), fonts[i].layout);  /* for strikeout and underline */

  return &fonts[i];
}

static IgtkFont* gtkFontCreateNativeFont(Ihandle* ih, const char* value)
{
  IgtkFont *gtkfont = gtkFindFont(value);
  if (!gtkfont)
  {
    iupERROR1("Failed to create Font: %s", value);
    return NULL;
  }

  iupAttribSet(ih, "_IUP_GTKFONT", (char*)gtkfont);
  return gtkfont;
}

static IgtkFont* gtkFontGet(Ihandle *ih)
{
  IgtkFont* gtkfont = (IgtkFont*)iupAttribGet(ih, "_IUP_GTKFONT");
  if (!gtkfont)
  {
    gtkfont = gtkFontCreateNativeFont(ih, iupGetFontValue(ih));
    if (!gtkfont)
      gtkfont = gtkFontCreateNativeFont(ih, IupGetGlobal("DEFAULTFONT"));
  }
  return gtkfont;
}

static void gtkFontUpdateWidget(Ihandle* ih, GtkWidget* widget, PangoFontDescription* fontdesc)
{
  IgtkFont* gtkfont;

#if GTK_CHECK_VERSION(3, 0, 0)
  gtk_widget_override_font(widget, fontdesc);
#else
  gtk_widget_modify_font(widget, fontdesc);
#endif

  gtkfont = gtkFontGet(ih);
  if (!gtkfont)
    return;

  if (GTK_IS_LABEL(widget))
  {
    PangoAttrList *attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attribute_copy(gtkfont->strikethrough));
    pango_attr_list_insert(attrs, pango_attribute_copy(gtkfont->underline));
    gtk_label_set_attributes (GTK_LABEL(widget), attrs);
    pango_attr_list_unref(attrs);
  }

  if (GTK_IS_ENTRY(widget))
  {
    /* TODO: This is NOT working. */
    PangoLayout* layout = gtk_entry_get_layout(GTK_ENTRY(widget));
    gtkFontUpdateLayout(gtkfont, layout);  /* for strikeout and underline */
  }
}

void iupgtkUpdateWidgetFont(Ihandle *ih, GtkWidget* widget)
{
  PangoFontDescription* fontdesc = (PangoFontDescription*)iupgtkGetPangoFontDescAttrib(ih);
  gtkFontUpdateWidget(ih, widget, fontdesc);
}

void iupgtkUpdateObjectFont(Ihandle* ih, gpointer object)
{
  PangoAttrList *attrs;

  IgtkFont* gtkfont = gtkFontGet(ih);
  if (!gtkfont)
    return;

  g_object_set(object, "font-desc", gtkfont->fontdesc, NULL);

  g_object_get(object, "attributes", &attrs, NULL);
  if (!attrs) 
  {
    attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attribute_copy(gtkfont->strikethrough));
    pango_attr_list_insert(attrs, pango_attribute_copy(gtkfont->underline));
    g_object_set(object, "attributes", attrs, NULL);  /* TODO: does this reference attrs? */
    /* pango_attr_list_unref(attrs); */
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
  const PangoFontDescription* font_desc = NULL;
  GtkWidget* widget = gtk_invisible_new();
  gtk_widget_realize(widget);
#if GTK_CHECK_VERSION(3, 0, 0)
  {
    GtkStyleContext* context = gtk_widget_get_style_context(widget);
    if (context)
    {
#if GTK_CHECK_VERSION(3, 8, 0)
      gtk_style_context_get(context, GTK_STATE_FLAG_NORMAL, "font", &font_desc, NULL);
#else
      font_desc = gtk_style_context_get_font(context, GTK_STATE_FLAG_NORMAL);
#endif
    }
  }
#else
  {
    GtkStyle* style = gtk_widget_get_style(widget);
    if (style)
       font_desc = style->font_desc;
  }
#endif
  if (!font_desc)
    strcpy(str, "Sans, 10");
  else
  {
    char* desc = pango_font_description_to_string(font_desc);
    strcpy(str, desc);
    g_free(desc);
  }
  gtk_widget_unrealize(widget);
  gtk_widget_destroy(widget);
  return str;
}

PangoLayout* iupgtkGetPangoLayout(const char* value)
{
  IgtkFont *gtkfont = gtkFindFont(value);
  if (gtkfont)
    return gtkfont->layout;
  else
    return NULL;
}

char* iupgtkGetPangoLayoutAttrib(Ihandle *ih)
{
  IgtkFont* gtkfont = gtkFontGet(ih);
  if (gtkfont)
    return (char*)gtkfont->layout;
  else
    return NULL;
}

PangoFontDescription* iupgtkGetPangoFontDesc(const char* value)
{
  IgtkFont *gtkfont = gtkFindFont(value);
  if (gtkfont)
    return gtkfont->fontdesc;
  else
    return NULL;
}

char* iupgtkGetPangoFontDescAttrib(Ihandle *ih)
{
  IgtkFont* gtkfont = gtkFontGet(ih);
  if (gtkfont)
    return (char*)gtkfont->fontdesc;
  else
    return NULL;
}

char* iupgtkGetFontIdAttrib(Ihandle *ih)
{
  /* Used by IupGLCanvas for IupGLUseFont */
  IgtkFont* gtkfont = gtkFontGet(ih);
  if (!gtkfont)
    return NULL;
  else
  {
#if GTK_CHECK_VERSION(3, 0, 0)
    return NULL;  /* TODO: not available yet. */
#else
    /* both functions are marked as deprecated in GDK (since 2.22) */
    GdkFont* gdk_font = gdk_font_from_description(gtkfont->fontdesc);
    return (char*)gdk_font_id(gdk_font);  /* In UNIX will return an X Font ID, 
                                             in Win32 will return an HFONT */
#endif
  }
}

IUP_SDK_API int iupdrvSetFontAttrib(Ihandle* ih, const char* value)
{
  IgtkFont* gtkfont = gtkFontCreateNativeFont(ih, value);
  if (!gtkfont)
    return 0;

  /* If FONT is changed, must update the SIZE attribute */
  iupBaseUpdateAttribFromFont(ih);

  /* FONT attribute must be able to be set before mapping, 
    so the font is enable for size calculation. */
  if (ih->handle && (ih->iclass->nativetype != IUP_TYPEVOID))
    gtkFontUpdateWidget(ih, ih->handle, gtkfont->fontdesc);

  return 1;
}

static void gtkFontGetTextSize(Ihandle* ih, IgtkFont* gtkfont, const char* str, int len, int *w, int *h)
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
    char* text = iupgtkStrConvertToSystemLen(str, &len);

    if (iupAttribGetBoolean(ih, "MARKUP"))
    {
      pango_layout_set_attributes(gtkfont->layout, NULL);
      pango_layout_set_markup(gtkfont->layout, text, len);
    }
    else
      pango_layout_set_text(gtkfont->layout, text, len);

    pango_layout_get_pixel_size(gtkfont->layout, &max_w, &dummy_h);
  }

  if (w) *w = max_w;
  if (h) *h = gtkfont->charheight * line_count;
}

IUP_SDK_API void iupdrvFontGetMultiLineStringSize(Ihandle* ih, const char* str, int *w, int *h)
{
  IgtkFont* gtkfont = gtkFontGet(ih);
  if (gtkfont)
    gtkFontGetTextSize(ih, gtkfont, str, str? (int)strlen(str): 0, w, h);
}

IUP_SDK_API void iupdrvFontGetTextSize(const char* font, const char* str, int len, int *w, int *h)
{
  IgtkFont *gtkfont = gtkFindFont(font);
  if (gtkfont)
    gtkFontGetTextSize(NULL, gtkfont, str, len, w, h);
}

IUP_SDK_API void iupdrvFontGetFontDim(const char* font, int *max_width, int *line_height, int *ascent, int *descent)
{
  IgtkFont *gtkfont = gtkFindFont(font);
  if (gtkfont)
  {
    PangoFontMetrics* metrics;
    int charwidth, charheight, charascent, chardescent;

    if (!gtkfont->fontdesc)
      return;

    metrics = pango_context_get_metrics(gtk_fonts_context, gtkfont->fontdesc, pango_context_get_language(gtk_fonts_context));
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
  IgtkFont* gtkfont;
  int len, w;
  const char* line_end, *text;

  if (!str || str[0]==0)
    return 0;

  gtkfont = gtkFontGet(ih);
  if (!gtkfont)
    return 0;

  /* do it only for the first line, if any */
  line_end = strchr(str, '\n');
  if (line_end)
    len = (int)(line_end-str);
  else
    len = (int)strlen(str);

  text = iupgtkStrConvertToSystemLen(str, &len);

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
  IgtkFont* gtkfont = gtkFontGet(ih);
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
  gtk_fonts = iupArrayCreate(50, sizeof(IgtkFont));
  gtk_fonts_context = gdk_pango_context_get();
  pango_context_set_language(gtk_fonts_context, gtk_get_default_language());
}

void iupdrvFontFinish(void)
{
  int i, count = iupArrayCount(gtk_fonts);
  IgtkFont* fonts = (IgtkFont*)iupArrayGetData(gtk_fonts);
  for (i = 0; i < count; i++)
  {
    pango_font_description_free(fonts[i].fontdesc);
    g_object_unref(fonts[i].layout);
    pango_attribute_destroy(fonts[i].strikethrough);
    pango_attribute_destroy(fonts[i].underline);
  }
  iupArrayDestroy(gtk_fonts);
  g_object_unref(gtk_fonts_context);
}
