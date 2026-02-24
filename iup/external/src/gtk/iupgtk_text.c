/** \file
 * \brief Text Control
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
#include "iup_image.h"
#include "iup_mask.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_image.h"
#include "iup_key.h"
#include "iup_array.h"
#include "iup_text.h"

#include "iupgtk_drv.h"

#ifndef PANGO_WEIGHT_SEMIBOLD
#define PANGO_WEIGHT_SEMIBOLD 600
#endif


void iupdrvTextAddSpin(Ihandle* ih, int *w, int h)
{
  static int spin_min_width = -1;

  (void)h;
  (void)ih;

  /* Measure the minimum width required by GtkSpinButton */
  if (spin_min_width < 0)
  {
    GtkWidget *temp_window = gtk_offscreen_window_new();
    GtkWidget *temp_spin = gtk_spin_button_new_with_range(0, 100, 1);
    GtkAllocation allocation;

    /* Add to window, show, and realize to get actual allocated size */
    gtk_container_add(GTK_CONTAINER(temp_window), temp_spin);
    gtk_widget_show_all(temp_window);

#if GTK_CHECK_VERSION(3, 0, 0)
    int min_w, nat_w;
    gtk_widget_get_preferred_width(temp_spin, &min_w, &nat_w);

    /* Get the actual allocated size after realization */
    gtk_widget_get_allocation(temp_spin, &allocation);

    /* Use allocated width with fallback */
    spin_min_width = (allocation.width > 0) ? allocation.width : 130;
#else
    GtkRequisition requisition;
    gtk_widget_size_request(temp_spin, &requisition);
    gtk_widget_get_allocation(temp_spin, &allocation);

    spin_min_width = (allocation.width > 0) ? allocation.width : 130;
#endif

    gtk_widget_destroy(temp_window);
  }

  /* Only enforce minimum width, don't force expansion */
  if (*w < spin_min_width)
    *w = spin_min_width;
}

/* Cached measurements for text widget borders */
static int iupgtk_multiline_line_height = -1;
static int iupgtk_multiline_border_height = -1;
static int iupgtk_multiline_border_width = -1;
static int iupgtk_entry_border_x = -1;
static int iupgtk_entry_border_y = -1;
static int iupgtk_entry_noframe_border_y = -1;
static int iupgtk_entry_css_dec_x = 0;
static int iupgtk_entry_char_adjust_x = 0;

static void iupgtkTextMeasureEntryBorders(void)
{
  if (iupgtk_entry_border_x < 0)
  {
    GtkWidget *temp_window, *temp_entry, *temp_entry_noframe;
    int entry_w, entry_h, entry_noframe_h;
    int char_width, char_height;
    PangoContext *context;
    PangoLayout *layout;
    GtkWidget *vbox;

    temp_window = gtk_offscreen_window_new();
#if GTK_CHECK_VERSION(3, 0, 0)
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    vbox = gtk_vbox_new(FALSE, 0);
#endif
    gtk_container_add(GTK_CONTAINER(temp_window), vbox);

    /* Create entry with frame */
    temp_entry = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(temp_entry), 1);
    gtk_entry_set_has_frame(GTK_ENTRY(temp_entry), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), temp_entry, FALSE, FALSE, 0);

    /* Create separate entry without frame */
    temp_entry_noframe = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(temp_entry_noframe), 1);
    gtk_entry_set_has_frame(GTK_ENTRY(temp_entry_noframe), FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), temp_entry_noframe, FALSE, FALSE, 0);

    gtk_widget_show_all(temp_window);

    context = gtk_widget_get_pango_context(temp_entry);
    layout = pango_layout_new(context);
    pango_layout_set_text(layout, "W", -1);
    pango_layout_get_pixel_size(layout, &char_width, &char_height);

    /* Measure with frame */
#if GTK_CHECK_VERSION(3, 0, 0)
    gtk_widget_get_preferred_width(temp_entry, NULL, &entry_w);
    gtk_widget_get_preferred_height(temp_entry, NULL, &entry_h);
    /* Measure without frame, get minimum size */
    gtk_widget_get_preferred_height(temp_entry_noframe, &entry_noframe_h, NULL);
#else
    {
      GtkRequisition requisition;
      gtk_widget_size_request(temp_entry, &requisition);
      entry_w = requisition.width;
      entry_h = requisition.height;
      gtk_widget_size_request(temp_entry_noframe, &requisition);
      entry_noframe_h = requisition.height;
    }
#endif

    /* Border = total size - content size */
    iupgtk_entry_border_x = entry_w - char_width;
    iupgtk_entry_border_y = entry_h - char_height;
    iupgtk_entry_noframe_border_y = entry_noframe_h - char_height;

    if (iupgtk_entry_border_x < 0) iupgtk_entry_border_x = 10;
    if (iupgtk_entry_border_y < 0) iupgtk_entry_border_y = 10;
    if (iupgtk_entry_noframe_border_y < 0) iupgtk_entry_noframe_border_y = 0;

    /* Measure the per-char width difference between "W" and GTK's average char.
       GtkEntry uses approximate_char_width for width_chars sizing, but
       IUP core uses "W" width for VISIBLECOLUMNS. We store the exact CSS decoration
       and the per-char adjustment so iupdrvTextAddBorders can compensate. */
    {
      PangoFontMetrics* metrics = pango_context_get_metrics(context,
          pango_context_get_font_description(context),
          pango_context_get_language(context));
      int avg_w = pango_font_metrics_get_approximate_char_width(metrics);
      int digit_w = pango_font_metrics_get_approximate_digit_width(metrics);
      int gtk_char_pixels = (MAX(avg_w, digit_w) + PANGO_SCALE - 1) / PANGO_SCALE;

      iupgtk_entry_css_dec_x = entry_w - gtk_char_pixels;
      iupgtk_entry_char_adjust_x = char_width - gtk_char_pixels;
      if (iupgtk_entry_css_dec_x < 2) iupgtk_entry_css_dec_x = 2;
      if (iupgtk_entry_char_adjust_x < 0) iupgtk_entry_char_adjust_x = 0;

      pango_font_metrics_unref(metrics);
    }

    g_object_unref(layout);
    gtk_widget_destroy(temp_window);
  }
}

static void iupgtkTextMeasureMultilineMetrics(void)
{
  if (iupgtk_multiline_border_height < 0)
  {
    GtkWidget *temp_window, *temp_sw, *temp_tv;
    PangoContext *context;
    PangoLayout *layout;
    int layout_1line_h, layout_2line_h;
    int sw_h, tv_h;

    temp_window = gtk_offscreen_window_new();
    temp_tv = gtk_text_view_new();
    temp_sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(temp_sw), GTK_SHADOW_IN);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(temp_sw), GTK_POLICY_NEVER, GTK_POLICY_NEVER);
    gtk_container_add(GTK_CONTAINER(temp_sw), temp_tv);
    gtk_container_add(GTK_CONTAINER(temp_window), temp_sw);

    gtk_widget_show_all(temp_window);

    context = gtk_widget_get_pango_context(temp_tv);
    layout = pango_layout_new(context);

    pango_layout_set_text(layout, "X", -1);
    pango_layout_get_pixel_size(layout, NULL, &layout_1line_h);

    pango_layout_set_text(layout, "X\nX", -1);
    pango_layout_get_pixel_size(layout, NULL, &layout_2line_h);

    iupgtk_multiline_line_height = layout_2line_h - layout_1line_h;
    if (iupgtk_multiline_line_height <= 0)
      iupgtk_multiline_line_height = 16;

    /* Measure scrolled window with empty text view */
#if GTK_CHECK_VERSION(3, 0, 0)
    {
      GtkStyleContext* sw_ctx = gtk_widget_get_style_context(temp_sw);
      GtkStyleContext* tv_ctx = gtk_widget_get_style_context(temp_tv);
      GtkBorder sw_border, sw_padding, tv_padding;
      gtk_style_context_get_border(sw_ctx, GTK_STATE_FLAG_NORMAL, &sw_border);
      gtk_style_context_get_padding(sw_ctx, GTK_STATE_FLAG_NORMAL, &sw_padding);
      gtk_style_context_get_padding(tv_ctx, GTK_STATE_FLAG_NORMAL, &tv_padding);
      iupgtk_multiline_border_width = sw_border.left + sw_border.right +
                                      sw_padding.left + sw_padding.right +
                                      tv_padding.left + tv_padding.right;
    }

    gtk_widget_get_preferred_height(temp_sw, &sw_h, NULL);
    gtk_widget_get_preferred_height(temp_tv, &tv_h, NULL);
#else
    {
      GtkRequisition sw_req, tv_req;
      gtk_widget_size_request(temp_sw, &sw_req);
      gtk_widget_size_request(temp_tv, &tv_req);
      sw_h = sw_req.height;
      tv_h = tv_req.height;
      iupgtk_multiline_border_width = sw_req.width - tv_req.width;
    }
#endif

    iupgtk_multiline_border_height = sw_h - tv_h;
    if (iupgtk_multiline_border_height < 0)
      iupgtk_multiline_border_height = 2;
    if (iupgtk_multiline_border_width < 0)
      iupgtk_multiline_border_width = 2;

    g_object_unref(layout);
    gtk_widget_destroy(temp_window);
  }
}

void iupdrvTextAddBorders(Ihandle* ih, int *x, int *y)
{
  /* Used also by IupCalendar in GTK */

  iupgtkTextMeasureEntryBorders();
  int border_size_x = iupgtk_entry_border_x;
  int border_size_y = iupgtk_entry_border_y;

  static int spin_natural_height = -1;
  if (iupAttribGetBoolean(ih, "SPIN") && spin_natural_height == -1)
  {
    GtkWidget *temp_window = gtk_offscreen_window_new();
    GtkWidget *temp_spin = gtk_spin_button_new_with_range(0, 100, 1);

    gtk_container_add(GTK_CONTAINER(temp_window), temp_spin);
    gtk_widget_show_all(temp_window);

#if GTK_CHECK_VERSION(3, 0, 0)
    gtk_widget_get_preferred_height(temp_spin, NULL, &spin_natural_height);
#else
    {
      GtkRequisition requisition;
      gtk_widget_size_request(temp_spin, &requisition);
      spin_natural_height = requisition.height;
    }
#endif

    if (spin_natural_height < 16) spin_natural_height = 34;

    gtk_widget_destroy(temp_window);
  }

  if (ih->data && ih->data->is_multiline)
  {
    int visiblecolumns = iupAttribGetInt(ih, "VISIBLECOLUMNS");
    int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
    iupgtkTextMeasureMultilineMetrics();

    (*x) += iupgtk_multiline_border_width - visiblecolumns * iupgtk_entry_char_adjust_x;

    if (visiblelines > 0)
    {
      int char_height;
      iupdrvFontGetCharSize(ih, NULL, &char_height);

      int iup_content_h = char_height * visiblelines;
      int gtk_content_h = iupgtk_multiline_line_height * visiblelines;
      int line_diff = iup_content_h - gtk_content_h;

      (*y) += iupgtk_multiline_border_height;
      (*y) -= line_diff;
    }
    else
    {
      (*y) += iupgtk_multiline_border_height;
    }
  }
  else
  {
    int visiblecolumns = iupAttribGetInt(ih, "VISIBLECOLUMNS");
    int border = iupgtk_entry_css_dec_x - visiblecolumns * iupgtk_entry_char_adjust_x;
    (*x) += border;

    if (iupAttribGetBoolean(ih, "SPIN"))
    {
      int add = spin_natural_height - (*y);
      if (add < 0) add = 0;
      (*y) += add;
    }
    else
    {
      (*y) += border_size_y;
    }
  }
}

void iupdrvTextAddExtraPadding(Ihandle* ih, int *w, int *h)
{
  (void)ih;
  iupgtkTextMeasureEntryBorders();
  if (w) *w += iupgtk_entry_noframe_border_y;  /* use same value for width */
  if (h) *h += iupgtk_entry_noframe_border_y;
}

static void gtkIntToRoman(int num, char* buf, int bufsize, int uppercase)
{
  static const int values[] = {1000, 900, 500, 400, 100, 90, 50, 40, 10, 9, 5, 4, 1};
  static const char* upper_sym[] = {"M", "CM", "D", "CD", "C", "XC", "L", "XL", "X", "IX", "V", "IV", "I"};
  static const char* lower_sym[] = {"m", "cm", "d", "cd", "c", "xc", "l", "xl", "x", "ix", "v", "iv", "i"};
  const char** sym = uppercase ? upper_sym : lower_sym;
  int i, pos = 0;

  if (num <= 0 || num > 3999)
  {
    buf[0] = '?';
    buf[1] = '\0';
    return;
  }

  for (i = 0; i < 13 && num > 0; i++)
  {
    while (num >= values[i] && pos < bufsize - 4)
    {
      int len = (int)strlen(sym[i]);
      memcpy(buf + pos, sym[i], len);
      pos += len;
      num -= values[i];
    }
  }
  buf[pos] = '\0';
}

static int gtkNumberingPrefix(int counter, const char* numbering, const char* style, char* buf, int bufsize)
{
  char number[64] = "";

  if (iupStrEqualNoCase(numbering, "BULLET"))
  {
    snprintf(buf, bufsize, "\xe2\x80\xa2\t");
    return (int)strlen(buf);
  }

  if (iupStrEqualNoCase(numbering, "ARABIC"))
    snprintf(number, sizeof(number), "%d", counter);
  else if (iupStrEqualNoCase(numbering, "LCLETTER"))
  {
    if (counter >= 1 && counter <= 26)
      snprintf(number, sizeof(number), "%c", 'a' + counter - 1);
    else
      snprintf(number, sizeof(number), "%d", counter);
  }
  else if (iupStrEqualNoCase(numbering, "UCLETTER"))
  {
    if (counter >= 1 && counter <= 26)
      snprintf(number, sizeof(number), "%c", 'A' + counter - 1);
    else
      snprintf(number, sizeof(number), "%d", counter);
  }
  else if (iupStrEqualNoCase(numbering, "LCROMAN"))
    gtkIntToRoman(counter, number, sizeof(number), 0);
  else if (iupStrEqualNoCase(numbering, "UCROMAN"))
    gtkIntToRoman(counter, number, sizeof(number), 1);
  else
    return 0;

  if (style)
  {
    if (iupStrEqualNoCase(style, "RIGHTPARENTHESIS"))
      snprintf(buf, bufsize, "%s)\t", number);
    else if (iupStrEqualNoCase(style, "PARENTHESES"))
      snprintf(buf, bufsize, "(%s)\t", number);
    else if (iupStrEqualNoCase(style, "PERIOD"))
      snprintf(buf, bufsize, "%s.\t", number);
    else if (iupStrEqualNoCase(style, "NONUMBER"))
      snprintf(buf, bufsize, "\t");
    else
      snprintf(buf, bufsize, "%s\t", number);
  }
  else
    snprintf(buf, bufsize, "%s\t", number);

  return (int)strlen(buf);
}

static void gtkTextApplyNumbering(GtkTextBuffer* buffer, GtkTextIter* start_iter, GtkTextIter* end_iter,
                                   const char* numbering, const char* style)
{
  int start_line = gtk_text_iter_get_line(start_iter);
  int end_line = gtk_text_iter_get_line(end_iter);
  int line;

  if (end_line > start_line && gtk_text_iter_get_line_offset(end_iter) == 0)
    end_line--;

  for (line = start_line; line <= end_line; line++)
  {
    GtkTextIter line_iter;
    char prefix[128];

    gtk_text_buffer_get_iter_at_line(buffer, &line_iter, line);

    if (gtkNumberingPrefix(line - start_line + 1, numbering, style, prefix, sizeof(prefix)) > 0)
      gtk_text_buffer_insert(buffer, &line_iter, prefix, -1);
  }

  gtk_text_buffer_get_iter_at_line(buffer, start_iter, start_line);
  gtk_text_buffer_get_iter_at_line(buffer, end_iter, end_line);
  if (!gtk_text_iter_ends_line(end_iter))
    gtk_text_iter_forward_to_line_end(end_iter);
}

static void gtkTextParseParagraphFormat(Ihandle* formattag, GtkTextTag* tag)
{
  int val;
  char* format;

  format = iupAttribGet(formattag, "INDENT");
  if (format && iupStrToInt(format, &val))
  {
    char* indent_offset = iupAttribGet(formattag, "INDENTOFFSET");
    if (indent_offset)
    {
      int ioval = 0;
      iupStrToInt(indent_offset, &ioval);
      g_object_set(G_OBJECT(tag), "left-margin", val, NULL);
      g_object_set(G_OBJECT(tag), "indent", ioval, NULL);
    }
    else
      g_object_set(G_OBJECT(tag), "indent", val, NULL);

    char* indent_right = iupAttribGet(formattag, "INDENTRIGHT");
    if (indent_right)
    {
      int irval = 0;
      iupStrToInt(indent_right, &irval);
      g_object_set(G_OBJECT(tag), "right-margin", irval, NULL);
    }
  }

  format = iupAttribGet(formattag, "ALIGNMENT");
  if (format)
  {
    if (iupStrEqualNoCase(format, "JUSTIFY"))
      val = GTK_JUSTIFY_FILL;
    else if (iupStrEqualNoCase(format, "RIGHT"))
      val = GTK_JUSTIFY_RIGHT;
    else if (iupStrEqualNoCase(format, "CENTER"))
      val = GTK_JUSTIFY_CENTER;
    else /* "LEFT" */
      val = GTK_JUSTIFY_LEFT;

    g_object_set(G_OBJECT(tag), "justification", val, NULL);
  }

  format = iupAttribGet(formattag, "TABSARRAY");
  {
    PangoTabArray *tabs;
    int pos, i = 0;
    PangoTabAlign align;
    char* str;

    tabs = pango_tab_array_new(32, FALSE);

    while (format)
    {
      str = iupStrDupUntil((const char**)&format, ' ');
      if (!str) break;
      pos = atoi(str);
      free(str);

      str = iupStrDupUntil((const char**)&format, ' ');
      if (!str) break;

#if PANGO_VERSION_CHECK(1, 50, 0)
      if (iupStrEqualNoCase(str, "RIGHT"))
        align = PANGO_TAB_RIGHT;
      else if (iupStrEqualNoCase(str, "CENTER"))
        align = PANGO_TAB_CENTER;
      else if (iupStrEqualNoCase(str, "DECIMAL"))
        align = PANGO_TAB_DECIMAL;
      else
#endif
        align = PANGO_TAB_LEFT;
      free(str);

      pango_tab_array_set_tab(tabs, i, align, iupGTK_PIXELS2PANGOUNITS(pos));
      i++;
      if (i == 32) break;
    }

    g_object_set(G_OBJECT(tag), "tabs", tabs, NULL);
    pango_tab_array_free(tabs);
  }

  format = iupAttribGet(formattag, "SPACEBEFORE");
  if (format && iupStrToInt(format, &val))
    g_object_set(G_OBJECT(tag), "pixels-above-lines", val, NULL);

  format = iupAttribGet(formattag, "SPACEAFTER");
  if (format && iupStrToInt(format, &val))
    g_object_set(G_OBJECT(tag), "pixels-below-lines", val, NULL);

  format = iupAttribGet(formattag, "LINESPACING");
  if (format && iupStrToInt(format, &val))
    g_object_set(G_OBJECT(tag), "pixels-inside-wrap", val, NULL);

  format = iupAttribGet(formattag, "NUMBERING");
  if (format && !iupStrEqualNoCase(format, "NONE"))
  {
    int numberingtab = 24;
    char* tab_str = iupAttribGet(formattag, "NUMBERINGTAB");
    if (tab_str)
      iupStrToInt(tab_str, &numberingtab);

    if (!iupAttribGet(formattag, "INDENT"))
    {
      g_object_set(G_OBJECT(tag), "left-margin", numberingtab, NULL);
      g_object_set(G_OBJECT(tag), "indent", -numberingtab, NULL);
    }

    if (!iupAttribGet(formattag, "TABSARRAY"))
    {
      PangoTabArray* tabs = pango_tab_array_new(1, FALSE);
      pango_tab_array_set_tab(tabs, 0, PANGO_TAB_LEFT, iupGTK_PIXELS2PANGOUNITS(numberingtab));
      g_object_set(G_OBJECT(tag), "tabs", tabs, NULL);
      pango_tab_array_free(tabs);
    }
  }
}

static void gtkTextParseCharacterFormat(Ihandle* formattag, GtkTextTag* tag)
{
  int val;
  char* format;

  format = iupAttribGet(formattag, "LANGUAGE");
  if (format)
    g_object_set(G_OBJECT(tag), "language", format, NULL);

  format = iupAttribGet(formattag, "STRETCH");
  if (format)
  {
    if (iupStrEqualNoCase(format, "EXTRA_CONDENSED"))
      val = PANGO_STRETCH_EXTRA_CONDENSED;  
    else if (iupStrEqualNoCase(format, "CONDENSED"))
      val = PANGO_STRETCH_CONDENSED;     
    else if (iupStrEqualNoCase(format, "SEMI_CONDENSED"))
      val = PANGO_STRETCH_SEMI_CONDENSED;  
    else if (iupStrEqualNoCase(format, "SEMI_EXPANDED"))
      val = PANGO_STRETCH_SEMI_EXPANDED;      
    else if (iupStrEqualNoCase(format, "EXPANDED"))
      val = PANGO_STRETCH_EXPANDED;  
    else if (iupStrEqualNoCase(format, "EXTRA_EXPANDED"))
      val = PANGO_STRETCH_EXTRA_EXPANDED;       
    else /* "NORMAL" */
      val = PANGO_STRETCH_NORMAL;   

    g_object_set(G_OBJECT(tag), "stretch", val, NULL);
  }

  format = iupAttribGet(formattag, "RISE");
  if (format)
  {
    val = 0;

    if (iupStrEqualNoCase(format, "SUPERSCRIPT"))
    {
      g_object_set(G_OBJECT(tag), "scale", PANGO_SCALE_X_SMALL, NULL);
      val = 10;  /* 10 pixels up */
    }
    else if (iupStrEqualNoCase(format, "SUBSCRIPT"))
    {
      g_object_set(G_OBJECT(tag), "scale", PANGO_SCALE_X_SMALL, NULL);
      val = -10;  /* 10 pixels down */
    } 
    else 
      iupStrToInt(format, &val);

    val = iupGTK_PIXELS2PANGOUNITS(val);
    g_object_set(G_OBJECT(tag), "rise", val, NULL);
  }

  format = iupAttribGet(formattag, "SMALLCAPS");
  if (format)
  {
    if (iupStrBoolean(format))
      val = PANGO_VARIANT_SMALL_CAPS;
    else
      val = PANGO_VARIANT_NORMAL;
    g_object_set(G_OBJECT(tag), "variant", val, NULL);
  }

  format = iupAttribGet(formattag, "ITALIC");
  if (format)
  {
    if (iupStrBoolean(format))
      val = PANGO_STYLE_ITALIC;
    else
      val = PANGO_STYLE_NORMAL;
    g_object_set(G_OBJECT(tag), "style", val, NULL);
  }

  format = iupAttribGet(formattag, "STRIKEOUT");
  if (format)
  {
    val = iupStrBoolean(format);
    g_object_set(G_OBJECT(tag), "strikethrough", val, NULL);
  }

  format = iupAttribGet(formattag, "PROTECTED");
  if (format)
  {
    val = iupStrBoolean(format);
    g_object_set(G_OBJECT(tag), "editable", val, NULL);
  }

  format = iupAttribGet(formattag, "FONTSIZE");
  if (format && iupStrToInt(format, &val))
  {
    if (val < 0)  /* in pixels */
    {
      val = iupGTK_PIXELS2PANGOUNITS(-val);
      g_object_set(G_OBJECT(tag), "size", val, NULL);
    }
    else  /* in points */
      g_object_set(G_OBJECT(tag), "size-points", (double)val, NULL);
  }

  format = iupAttribGet(formattag, "FONTSCALE");
  if (format)
  {
    double fval = 0;
    if (iupStrEqualNoCase(format, "XX-SMALL"))
      fval = PANGO_SCALE_XX_SMALL;
    else if (iupStrEqualNoCase(format, "X-SMALL"))
      fval = PANGO_SCALE_X_SMALL;
    else if (iupStrEqualNoCase(format, "SMALL"))
      fval = PANGO_SCALE_SMALL;
    else if (iupStrEqualNoCase(format, "MEDIUM"))
      fval = PANGO_SCALE_MEDIUM;
    else if (iupStrEqualNoCase(format, "LARGE"))
      fval = PANGO_SCALE_LARGE;
    else if (iupStrEqualNoCase(format, "X-LARGE"))
      fval = PANGO_SCALE_X_LARGE;
    else if (iupStrEqualNoCase(format, "XX-LARGE"))
      fval = PANGO_SCALE_XX_LARGE;
    else 
      iupStrToDouble(format, &fval);

    if (fval > 0)
      g_object_set(G_OBJECT(tag), "scale", fval, NULL);
  }

  format = iupAttribGet(formattag, "FONTFACE");
  if (format)
  {
    /* Map standard names to native names */
    const char* mapped_name = iupFontGetPangoName(format);
    if (mapped_name)
      g_object_set(G_OBJECT(tag), "family", mapped_name, NULL);
    else
      g_object_set(G_OBJECT(tag), "family", format, NULL);
  }

  format = iupAttribGet(formattag, "FGCOLOR");
  if (format)
  {
    unsigned char r, g, b;
    if (iupStrToRGB(format, &r, &g, &b))
    {
#if GTK_CHECK_VERSION(3, 4, 0)
      GdkRGBA rgba;
      iupgdkRGBASet(&rgba, r, g, b);
      g_object_set(G_OBJECT(tag), "foreground-rgba", &rgba, NULL);
#else
      GdkColor color;
      iupgdkColorSetRGB(&color, r, g, b);
      g_object_set(G_OBJECT(tag), "foreground-gdk", &color, NULL);
#endif
    }
  }

  format = iupAttribGet(formattag, "BGCOLOR");
  if (format)
  {
    unsigned char r, g, b;
    if (iupStrToRGB(format, &r, &g, &b))
    {
#if GTK_CHECK_VERSION(3, 4, 0)
      GdkRGBA rgba;
      iupgdkRGBASet(&rgba, r, g, b);
      g_object_set(G_OBJECT(tag), "background-rgba", &rgba, NULL);
#else
      GdkColor color;
      iupgdkColorSetRGB(&color, r, g, b);
      g_object_set(G_OBJECT(tag), "background-gdk", &color, NULL);
#endif
    }
  }

  format = iupAttribGet(formattag, "UNDERLINE");
  if (format)
  {
    if (iupStrEqualNoCase(format, "SINGLE"))
      val = PANGO_UNDERLINE_SINGLE;
    else if (iupStrEqualNoCase(format, "DOUBLE"))
      val = PANGO_UNDERLINE_DOUBLE;
    else /* "NONE" */
      val = PANGO_UNDERLINE_NONE;

    g_object_set(G_OBJECT(tag), "underline", val, NULL);
  }

  format = iupAttribGet(formattag, "WEIGHT");
  if (format)
  {
    if (iupStrEqualNoCase(format, "EXTRALIGHT"))
      val = PANGO_WEIGHT_ULTRALIGHT;  
    else if (iupStrEqualNoCase(format, "LIGHT"))
      val = PANGO_WEIGHT_LIGHT;     
    else if (iupStrEqualNoCase(format, "SEMIBOLD"))
      val = PANGO_WEIGHT_SEMIBOLD;  
    else if (iupStrEqualNoCase(format, "BOLD"))
      val = PANGO_WEIGHT_BOLD;      
    else if (iupStrEqualNoCase(format, "EXTRABOLD"))
      val = PANGO_WEIGHT_ULTRABOLD;  
    else if (iupStrEqualNoCase(format, "HEAVY"))
      val = PANGO_WEIGHT_HEAVY;       
    else /* "NORMAL" */
      val = PANGO_WEIGHT_NORMAL;   

    g_object_set(G_OBJECT(tag), "weight", val, NULL);
  }
}

static void gtkTextMoveIterToLinCol(GtkTextBuffer *buffer, GtkTextIter *iter, int lin, int col)
{
  int line_count, line_length;

  lin--; /* IUP starts at 1 */
  col--;

  line_count = gtk_text_buffer_get_line_count(buffer);
  if (lin < 0) lin = 0;
  if (lin >= line_count)
    lin = line_count-1;

  gtk_text_buffer_get_iter_at_line(buffer, iter, lin);
  line_length = gtk_text_iter_get_chars_in_line(iter);

  if (col < 0) col = 0;
  if (col > line_length)
    col = line_length;  /* after the last character */

  gtk_text_iter_set_line_offset(iter, col);
}

static void gtkTextGetLinColFromPosition(const GtkTextIter *iter, int *lin, int *col)
{
  *lin = gtk_text_iter_get_line(iter);
  *col = gtk_text_iter_get_line_offset(iter);

  (*lin)++; /* IUP starts at 1 */
  (*col)++;
}

static int gtkTextGetCharSize(Ihandle* ih)
{
  int charwidth;
  PangoFontMetrics* metrics;
  PangoContext* context;
  PangoFontDescription* fontdesc = (PangoFontDescription*)iupgtkGetPangoFontDescAttrib(ih);
  if (!fontdesc)
    return 0;

  context = gdk_pango_context_get();
  metrics = pango_context_get_metrics(context, fontdesc, pango_context_get_language(context));
  charwidth = pango_font_metrics_get_approximate_char_width(metrics);
  pango_font_metrics_unref(metrics);
  return charwidth;
}

void iupdrvTextConvertLinColToPos(Ihandle* ih, int lin, int col, int *pos)
{
  GtkTextIter iter;
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
  gtkTextMoveIterToLinCol(buffer, &iter, lin, col);
  *pos = gtk_text_iter_get_offset(&iter);
}

void iupdrvTextConvertPosToLinCol(Ihandle* ih, int pos, int *lin, int *col)
{
  GtkTextIter iter;
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
  gtk_text_buffer_get_iter_at_offset(buffer, &iter, pos);
  gtkTextGetLinColFromPosition(&iter, lin, col);
}

static int gtkTextConvertXYToPos(Ihandle* ih, int x, int y)
{
  if (ih->data->is_multiline)
  {
    GtkTextIter iter;
    gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(ih->handle), GTK_TEXT_WINDOW_WIDGET, x, y, &x, &y);
    gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(ih->handle), &iter, x, y);
    return gtk_text_iter_get_offset(&iter);
  }
  else
  {
    int trailing, off_x, off_y, pos;

    /* transform to Layout coordinates */
    gtk_entry_get_layout_offsets(GTK_ENTRY(ih->handle), &off_x, &off_y);
    x = iupGTK_PIXELS2PANGOUNITS(x - off_x); 
    y = iupGTK_PIXELS2PANGOUNITS(y - off_y);

    pango_layout_xy_to_index(gtk_entry_get_layout(GTK_ENTRY(ih->handle)), x, y, &pos, &trailing);
    return pos;
  }
}

static void gtkTextScrollToVisible(Ihandle* ih)
{
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
  GtkTextMark* mark = gtk_text_buffer_get_insert(buffer);
  gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(ih->handle), mark);
}


/*******************************************************************************************/


static int gtkTextSelectionGetIter(Ihandle* ih, const char* value, GtkTextIter* start_iter, GtkTextIter* end_iter)
{
  int lin_start=1, col_start=1, lin_end=1, col_end=1;
  GtkTextBuffer *buffer;

  /* Used only when MULTILINE=YES */

  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
    gtk_text_buffer_get_start_iter(buffer, start_iter);
    *end_iter = *start_iter;
    return 1;
  }

  if (iupStrEqualNoCase(value, "ALL"))
  {
    gtk_text_buffer_get_start_iter(buffer, start_iter);
    gtk_text_buffer_get_end_iter(buffer, end_iter);
    return 1;
  }

  if (sscanf(value, "%d,%d:%d,%d", &lin_start, &col_start, &lin_end, &col_end)!=4) 
    return 0;

  if (lin_start<1 || col_start<1 || lin_end<1 || col_end<1) 
    return 0;

  gtkTextMoveIterToLinCol(buffer, start_iter, lin_start, col_start);
  gtkTextMoveIterToLinCol(buffer, end_iter, lin_end, col_end);

  return 1;
}

static int gtkTextSelectionPosGetIter(Ihandle* ih, const char* value, GtkTextIter* start_iter, GtkTextIter* end_iter)
{
  int start=0, end=0;
  GtkTextBuffer *buffer;

  /* Used only when MULTILINE=YES */

  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
    gtk_text_buffer_get_start_iter(buffer, start_iter);
    *end_iter = *start_iter;
    return 1;
  }

  if (iupStrEqualNoCase(value, "ALL"))
  {
    gtk_text_buffer_get_start_iter(buffer, start_iter);
    gtk_text_buffer_get_end_iter(buffer, end_iter);
    return 1;
  }

  if (iupStrToIntInt(value, &start, &end, ':')!=2) 
    return 0;

  if(start<0 || end<0) 
    return 0;

  gtk_text_buffer_get_iter_at_offset(buffer, start_iter, start);
  gtk_text_buffer_get_iter_at_offset(buffer, end_iter, end);

  return 1;
}

static int gtkTextSetSelectionAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->is_multiline)
  {
     GtkTextIter start_iter, end_iter;
     if (gtkTextSelectionGetIter(ih, value, &start_iter, &end_iter))
     {
       GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
       gtk_text_buffer_select_range(buffer, &start_iter, &end_iter);
     }
  }
  else
  {
    int start, end;

    if (!value || iupStrEqualNoCase(value, "NONE"))
    {
      start = 0;
      end = 0;
    }
    else if (iupStrEqualNoCase(value, "ALL"))
    {
      start = 0;
      end = -1;  /* a negative value means all */
    }
    else
    {
      start=1, end=1;
      if (iupStrToIntInt(value, &start, &end, ':')!=2) 
        return 0;

      if(start<1 || end<1) 
        return 0;

      start--; /* IUP starts at 1 */
      end--;
    }

    gtk_editable_select_region(GTK_EDITABLE(ih->handle), start, end);
  }

  return 0;
}

static char* gtkTextGetSelectionAttrib(Ihandle* ih)
{
  if (ih->data->is_multiline)
  {
    int start_col, start_lin, end_col, end_lin;

    GtkTextIter start_iter, end_iter;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
    if (gtk_text_buffer_get_selection_bounds(buffer, &start_iter, &end_iter))
    {
      gtkTextGetLinColFromPosition(&start_iter, &start_lin, &start_col);
      gtkTextGetLinColFromPosition(&end_iter,   &end_lin,   &end_col);

      return iupStrReturnStrf("%d,%d:%d,%d", start_lin, start_col, end_lin, end_col);
    }
  }
  else
  {
    int start, end;
    if (gtk_editable_get_selection_bounds(GTK_EDITABLE(ih->handle), &start, &end))
    {
      start++; /* IUP starts at 1 */
      end++;
      return iupStrReturnIntInt((int)start, (int)end, ':');
    }
  }

  return NULL;
}

static int gtkTextSetSelectionPosAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->is_multiline)
  {
    GtkTextIter start_iter, end_iter;
    if (gtkTextSelectionPosGetIter(ih, value, &start_iter, &end_iter))
    {
      GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
      gtk_text_buffer_select_range(buffer, &start_iter, &end_iter);
    }
  }
  else
  {
    int start, end;

    if (!value || iupStrEqualNoCase(value, "NONE"))
    {
      start = 0;
      end = 0;
    }
    else if (iupStrEqualNoCase(value, "ALL"))
    {
      start = 0;
      end = -1;  /* a negative value means all */
    }
    else
    {
      start=0, end=0;
      if (iupStrToIntInt(value, &start, &end, ':')!=2) 
        return 0;

      if(start<0 || end<0) 
        return 0;
    }

    gtk_editable_select_region(GTK_EDITABLE(ih->handle), start, end);
  }

  return 0;
}

static char* gtkTextGetSelectionPosAttrib(Ihandle* ih)
{
  int start, end;

  if (ih->data->is_multiline)
  {
    GtkTextIter start_iter, end_iter;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
    if (gtk_text_buffer_get_selection_bounds(buffer, &start_iter, &end_iter))
    {
      start = gtk_text_iter_get_offset(&start_iter);
      end = gtk_text_iter_get_offset(&end_iter);

      return iupStrReturnIntInt((int)start, (int)end, ':');
    }
  }
  else
  {
    if (gtk_editable_get_selection_bounds(GTK_EDITABLE(ih->handle), &start, &end))
      return iupStrReturnIntInt((int)start, (int)end, ':');
  }

  return NULL;
}

static int gtkTextSetSelectedTextAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  if (ih->data->is_multiline)
  {
    GtkTextIter start_iter, end_iter;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
    if (gtk_text_buffer_get_selection_bounds(buffer, &start_iter, &end_iter))
    {
      ih->data->disable_callbacks = 1;
      gtk_text_buffer_delete(buffer, &start_iter, &end_iter);
      gtk_text_buffer_insert(buffer, &start_iter, iupgtkStrConvertToSystem(value), -1);
      ih->data->disable_callbacks = 0;
    }
  }
  else
  {
    int start, end;
    if (gtk_editable_get_selection_bounds(GTK_EDITABLE(ih->handle), &start, &end))
    {
      ih->data->disable_callbacks = 1;
      gtk_editable_delete_selection(GTK_EDITABLE(ih->handle));
      gtk_editable_insert_text(GTK_EDITABLE(ih->handle), iupgtkStrConvertToSystem(value), -1, &start);
      ih->data->disable_callbacks = 0;
    }
  }

  return 0;
}

static char* gtkTextGetCountAttrib(Ihandle* ih)
{
  int count;
  if (ih->data->is_multiline)
  {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
    count = gtk_text_buffer_get_char_count(buffer);
  }
  else
  {
#if GTK_CHECK_VERSION(2, 14, 0)
    count = gtk_entry_get_text_length(GTK_ENTRY(ih->handle));
#else
    count = strlen(gtk_entry_get_text(GTK_ENTRY(ih->handle)));
#endif
  }
  return iupStrReturnInt(count);
}

static char* gtkTextGetLineCountAttrib(Ihandle* ih)
{
  if (ih->data->is_multiline)
  {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
    int linecount = gtk_text_buffer_get_line_count(buffer);
    return iupStrReturnInt(linecount);
  }
  else
    return "1";
}

static char* gtkTextGetSelectedTextAttrib(Ihandle* ih)
{
  if (ih->data->is_multiline)
  {
    GtkTextIter start_iter, end_iter;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
    if (gtk_text_buffer_get_selection_bounds(buffer, &start_iter, &end_iter))
      return iupStrReturnStr(iupgtkStrConvertFromSystem(gtk_text_buffer_get_text(buffer, &start_iter, &end_iter, TRUE)));
  }
  else
  {
    int start, end;
    if (gtk_editable_get_selection_bounds(GTK_EDITABLE(ih->handle), &start, &end))
    {
      char* selectedtext = gtk_editable_get_chars(GTK_EDITABLE(ih->handle), start, end);
      char* str = iupStrReturnStr(iupgtkStrConvertFromSystem(selectedtext));
      g_free(selectedtext);
      return str;
    }
  }

  return NULL;
}

static int gtkTextSetCaretAttrib(Ihandle* ih, const char* value)
{
  int pos = 1;

  if (!value)
    return 0;

  if (ih->data->is_multiline)
  {
    int lin = 1, col = 1;
    GtkTextIter iter;

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));

    iupStrToIntInt(value, &lin, &col, ',');

    gtkTextMoveIterToLinCol(buffer, &iter, lin, col);

    gtk_text_buffer_place_cursor(buffer, &iter);
    gtkTextScrollToVisible(ih);
  }
  else
  {
    iupStrToInt(value, &pos);
    pos--; /* IUP starts at 1 */
    if (pos < 0) pos = 0;

    gtk_editable_set_position(GTK_EDITABLE(ih->handle), pos);
  }

  return 0;
}

static char* gtkTextGetCaretAttrib(Ihandle* ih)
{
  if (ih->data->is_multiline)
  {
    int col, lin;
    GtkTextIter iter;

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
    gtk_text_buffer_get_iter_at_mark(buffer, &iter, gtk_text_buffer_get_insert(buffer));
    gtkTextGetLinColFromPosition(&iter, &lin, &col);

    return iupStrReturnIntInt(lin, col, ',');
  }
  else
  {
    int pos = gtk_editable_get_position(GTK_EDITABLE(ih->handle));
    pos++; /* IUP starts at 1 */
    return iupStrReturnInt(pos);
  }
}

static int gtkTextSetCaretPosAttrib(Ihandle* ih, const char* value)
{
  int pos = 0;

  if (!value)
    return 0;

  iupStrToInt(value, &pos);
  if (pos < 0) pos = 0;

  if (ih->data->is_multiline)
  {
    GtkTextIter iter;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
    gtk_text_buffer_get_iter_at_offset(buffer, &iter, pos);
    gtk_text_buffer_place_cursor(buffer, &iter);
    gtkTextScrollToVisible(ih);
  }
  else
    gtk_editable_set_position(GTK_EDITABLE(ih->handle), pos);

  return 0;
}

static char* gtkTextGetCaretPosAttrib(Ihandle* ih)
{
  int pos;

  if (ih->data->is_multiline)
  {
    GtkTextIter iter;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
    gtk_text_buffer_get_iter_at_mark(buffer, &iter, gtk_text_buffer_get_insert(buffer));
    pos = gtk_text_iter_get_offset(&iter);
  }
  else
    pos = gtk_editable_get_position(GTK_EDITABLE(ih->handle));

  return iupStrReturnInt(pos);
}

static int gtkTextSetScrollToAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  if (ih->data->is_multiline)
  {
    int lin = 1, col = 1;
    GtkTextIter iter;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));

    iupStrToIntInt(value, &lin, &col, ',');
    if (lin < 1) lin = 1;
    if (col < 1) col = 1;

    gtkTextMoveIterToLinCol(buffer, &iter, lin, col);
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(ih->handle), &iter, 0, FALSE, 0, 0);
  }
  else
  {
    int pos = 1;
    iupStrToInt(value, &pos);
    if (pos < 1) pos = 1;
    pos--;  /* return to GTK reference */
    gtk_editable_set_position(GTK_EDITABLE(ih->handle), pos);
  }

  return 0;
}

static int gtkTextSetScrollToPosAttrib(Ihandle* ih, const char* value)
{
  int pos = 0;

  if (!value)
    return 0;

  iupStrToInt(value, &pos);
  if (pos < 0) pos = 0;

  if (ih->data->is_multiline)
  {
    GtkTextIter iter;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
    gtk_text_buffer_get_iter_at_offset(buffer, &iter, pos);
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(ih->handle), &iter, 0, FALSE, 0, 0);
  }
  else
    gtk_editable_set_position(GTK_EDITABLE(ih->handle), pos);

  return 0;
}

static int gtkTextSetValueAttrib(Ihandle* ih, const char* value)
{
  if (!value) value = "";
  ih->data->disable_callbacks = 1;
  if (ih->data->is_multiline)
  {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
    gtk_text_buffer_set_text(buffer, iupgtkStrConvertToSystem(value), -1);
  }
  else
    gtk_entry_set_text(GTK_ENTRY(ih->handle), iupgtkStrConvertToSystem(value));
  ih->data->disable_callbacks = 0;
  return 0;
}

static char* gtkTextGetValueAttrib(Ihandle* ih)
{
  char* value;

  if (ih->data->is_multiline)
  {
    GtkTextIter start_iter;
    GtkTextIter end_iter;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
    gtk_text_buffer_get_start_iter(buffer, &start_iter);
    gtk_text_buffer_get_end_iter(buffer, &end_iter);
    value = iupStrReturnStr(iupgtkStrConvertFromSystem(gtk_text_buffer_get_text(buffer, &start_iter, &end_iter, TRUE)));
  }
  else
    value = iupStrReturnStr(iupgtkStrConvertFromSystem(gtk_entry_get_text(GTK_ENTRY(ih->handle))));

  if (!value) value = "";

  return value;
}
                       
static char* gtkTextGetLineValueAttrib(Ihandle* ih)
{
  if (ih->data->is_multiline)
  {
    GtkTextIter start_iter, end_iter, iter;
    int lin;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
    gtk_text_buffer_get_iter_at_mark(buffer, &iter, gtk_text_buffer_get_insert(buffer));
    lin = gtk_text_iter_get_line(&iter);
    gtk_text_buffer_get_iter_at_line(buffer, &start_iter, lin);
    gtk_text_buffer_get_iter_at_line(buffer, &end_iter, lin);
    gtk_text_iter_forward_to_line_end(&end_iter);
    return iupStrReturnStr(iupgtkStrConvertFromSystem(gtk_text_buffer_get_text(buffer, &start_iter, &end_iter, TRUE)));
  }
  else
    return gtkTextGetValueAttrib(ih);
}

static int gtkTextSetInsertAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)  /* do not do the action before map */
    return 0;
  if (!value)
    return 0;

  ih->data->disable_callbacks = 1;
  if (ih->data->is_multiline)
  {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
    gtk_text_buffer_insert_at_cursor(buffer, iupgtkStrConvertToSystem(value), -1);
  }
  else
  {
    gint pos = gtk_editable_get_position(GTK_EDITABLE(ih->handle));
    gtk_editable_insert_text(GTK_EDITABLE(ih->handle), iupgtkStrConvertToSystem(value), -1, &pos);
  }
  ih->data->disable_callbacks = 0;

  return 0;
}

static int gtkTextSetAppendAttrib(Ihandle* ih, const char* value)
{
  gint pos;
  if (!ih->handle)  /* do not do the action before map */
    return 0;
  ih->data->disable_callbacks = 1;
  if (ih->data->is_multiline)
  {
    GtkTextIter iter;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
    gtk_text_buffer_get_end_iter(buffer, &iter);
    pos = gtk_text_buffer_get_char_count(buffer);
    if (ih->data->append_newline && pos!=0)
      gtk_text_buffer_insert(buffer, &iter, "\n", 1);
    gtk_text_buffer_insert(buffer, &iter, iupgtkStrConvertToSystem(value), -1);

    /* Move cursor to end and scroll to show it */
    gtk_text_buffer_get_end_iter(buffer, &iter);
    gtk_text_buffer_place_cursor(buffer, &iter);
    gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(ih->handle), gtk_text_buffer_get_insert(buffer));
  }
  else
  {
#if GTK_CHECK_VERSION(2, 14, 0)
    pos = gtk_entry_get_text_length(GTK_ENTRY(ih->handle))+1;
#else
    pos = strlen(gtk_entry_get_text(GTK_ENTRY(ih->handle)))+1;
#endif
    gtk_editable_insert_text(GTK_EDITABLE(ih->handle), iupgtkStrConvertToSystem(value), -1, &pos);
  }
  ih->data->disable_callbacks = 0;
  return 0;
}

static int gtkTextSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  gfloat xalign;
  GtkJustification justification;

  if (iupStrEqualNoCase(value, "ARIGHT"))
  {
    xalign = 1.0f;
    justification = GTK_JUSTIFY_RIGHT;
  }
  else if (iupStrEqualNoCase(value, "ACENTER"))
  {
    xalign = 0.5f;
    justification = GTK_JUSTIFY_CENTER;
  }
  else /* "ALEFT" */
  {
    xalign = 0;
    justification = GTK_JUSTIFY_LEFT;
  }

  if (ih->data->is_multiline)
    gtk_text_view_set_justification(GTK_TEXT_VIEW(ih->handle), justification);
  else
    gtk_entry_set_alignment(GTK_ENTRY(ih->handle), xalign);

  return 1;
}

static int gtkTextSetPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');
  if (ih->handle)
  {
    if (ih->data->is_multiline)
    {
      gtk_text_view_set_left_margin(GTK_TEXT_VIEW(ih->handle), ih->data->horiz_padding);
      gtk_text_view_set_right_margin(GTK_TEXT_VIEW(ih->handle), ih->data->horiz_padding);
      ih->data->vert_padding = 0;
    }
    else
    {
#if GTK_CHECK_VERSION(3, 4, 0)
      iupgtkSetMargin(ih->handle, ih->data->horiz_padding, ih->data->vert_padding, 1);
#else
#if GTK_CHECK_VERSION(2, 10, 0)
      GtkBorder border;
      border.bottom = border.top = (gint16)ih->data->vert_padding;
      border.left = border.right = (gint16)ih->data->horiz_padding;
      gtk_entry_set_inner_border(GTK_ENTRY(ih->handle), &border);
#endif
#endif
    }
    return 0;
  }
  else
    return 1; /* store until not mapped, when mapped will be set again */
}

static int gtkTextSetNCAttrib(Ihandle* ih, const char* value)
{
  if (!iupStrToInt(value, &ih->data->nc))
    ih->data->nc = INT_MAX;

  if (ih->handle)
  {
    if (!ih->data->is_multiline)
      gtk_entry_set_max_length(GTK_ENTRY(ih->handle), ih->data->nc);
    return 0;
  }
  else
    return 1; /* store until not mapped, when mapped will be set again */
}

static int gtkTextSetClipboardAttrib(Ihandle *ih, const char *value)
{
  ih->data->disable_callbacks = 1;
  if (iupStrEqualNoCase(value, "COPY"))
  {
    if (ih->data->is_multiline)
    {
      GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
      GtkClipboard *clipboard = gtk_clipboard_get(gdk_atom_intern("CLIPBOARD", FALSE));
      gtk_text_buffer_copy_clipboard(buffer, clipboard);
    }
    else
      gtk_editable_copy_clipboard(GTK_EDITABLE(ih->handle));
  }
  else if (iupStrEqualNoCase(value, "CUT"))
  {
    if (ih->data->is_multiline)
    {
      GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
      GtkClipboard *clipboard = gtk_clipboard_get(gdk_atom_intern("CLIPBOARD", FALSE));
      gtk_text_buffer_cut_clipboard(buffer, clipboard, TRUE);
    }
    else
      gtk_editable_cut_clipboard(GTK_EDITABLE(ih->handle));
  }
  else if (iupStrEqualNoCase(value, "PASTE"))
  {
    if (ih->data->is_multiline)
    {
      GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
      GtkClipboard *clipboard = gtk_clipboard_get(gdk_atom_intern("CLIPBOARD", FALSE));
      gtk_text_buffer_paste_clipboard(buffer, clipboard, NULL, TRUE);
    }
    else
      gtk_editable_paste_clipboard(GTK_EDITABLE(ih->handle));
  }
  else if (iupStrEqualNoCase(value, "CLEAR"))
  {
    if (ih->data->is_multiline)
    {
      GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
      gtk_text_buffer_delete_selection(buffer, FALSE, TRUE);
    }
    else
      gtk_editable_delete_selection(GTK_EDITABLE(ih->handle));
  }
  ih->data->disable_callbacks = 0;
  return 0;
}

static int gtkTextSetReadOnlyAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->is_multiline)
    gtk_text_view_set_editable(GTK_TEXT_VIEW(ih->handle), !iupStrBoolean(value));
  else
    gtk_editable_set_editable(GTK_EDITABLE(ih->handle), !iupStrBoolean(value));
  return 0;
}

static char* gtkTextGetReadOnlyAttrib(Ihandle* ih)
{
  int editable;
  if (ih->data->is_multiline)
    editable = gtk_text_view_get_editable(GTK_TEXT_VIEW(ih->handle));
  else
    editable = gtk_editable_get_editable(GTK_EDITABLE(ih->handle));
  return iupStrReturnBoolean (!editable); 
}

#if GTK_CHECK_VERSION(3, 20, 0)
#endif

static int gtkTextSetFgColorAttrib(Ihandle* ih, const char* value)
{
#if GTK_CHECK_VERSION(3, 20, 0)
  char *fg;
  char *css;
  GtkCssProvider *provider;
  GdkRGBA rgba;
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupgdkRGBASet(&rgba, r, g, b);

  fg = gdk_rgba_to_string(&rgba);

  /* style background color using CSS */
  provider = gtk_css_provider_new();

  css = g_strdup_printf("* { caret-color: %s; }", fg);
  gtk_style_context_add_provider(gtk_widget_get_style_context(ih->handle), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
  gtk_css_provider_load_from_data(provider, css, -1, NULL);

  g_free(fg);
  g_free(css);
  g_object_unref(provider);
#endif

  return iupdrvBaseSetFgColorAttrib(ih, value);
}

static int gtkTextSetBgColorAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->is_multiline && iupStrBoolean(IupGetGlobal("SB_BGCOLOR")))
  {
    GtkScrolledWindow* scrolled_window = (GtkScrolledWindow*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
    unsigned char r, g, b;

    /* ignore given value, must use only from parent for the scrollbars */
    char* parent_value = iupBaseNativeParentGetBgColor(ih);

    if (iupStrToRGB(parent_value, &r, &g, &b))
    {
      GtkWidget* sb;

      iupgtkSetBgColor((GtkWidget*)scrolled_window, r, g, b);

#if GTK_CHECK_VERSION(2, 8, 0)
      sb = gtk_scrolled_window_get_hscrollbar(scrolled_window);
      if (sb) iupgtkSetBgColor(sb, r, g, b);

      sb = gtk_scrolled_window_get_vscrollbar(scrolled_window);
      if (sb) iupgtkSetBgColor(sb, r, g, b);
#endif
    }
  }

  return iupdrvBaseSetBgColorAttrib(ih, value);
}

static int gtkTextSetTabSizeAttrib(Ihandle* ih, const char* value)
{
  PangoTabArray *tabs;
  int tabsize, charwidth;
  if (!ih->data->is_multiline)
    return 0;

  iupStrToInt(value, &tabsize);
  charwidth = gtkTextGetCharSize(ih);
  tabsize *= charwidth;
  tabs = pango_tab_array_new_with_positions(1, FALSE, PANGO_TAB_LEFT, tabsize);
  gtk_text_view_set_tabs(GTK_TEXT_VIEW(ih->handle), tabs);
  pango_tab_array_free(tabs);
  return 1;
}

static int gtkTextSetCueBannerAttrib(Ihandle *ih, const char *value)
{
  if (!ih->data->is_multiline)
  {
#if GTK_CHECK_VERSION(3, 2, 0)
    gtk_entry_set_placeholder_text(GTK_ENTRY(ih->handle), iupgtkStrConvertToSystem(value));
    return 1;
#else
    (void)value;
#endif
  }
  return 0;
}

static int gtkTextSetOverwriteAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->is_multiline)
    return 0;
  gtk_text_view_set_overwrite(GTK_TEXT_VIEW(ih->handle), iupStrBoolean(value));
  return 0;
}

static char* gtkTextGetOverwriteAttrib(Ihandle* ih)
{
  if (!ih->data->is_multiline)
    return NULL;
  return iupStrReturnChecked(gtk_text_view_get_overwrite(GTK_TEXT_VIEW(ih->handle)));
}

void* iupdrvTextAddFormatTagStartBulk(Ihandle* ih)
{
  GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
  gtk_text_buffer_begin_user_action(buffer);
  return NULL;
}

void iupdrvTextAddFormatTagStopBulk(Ihandle* ih, void* state)
{
  GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
  gtk_text_buffer_end_user_action(buffer);
  (void)state;
}

void iupdrvTextAddFormatTag(Ihandle* ih, Ihandle* formattag, int bulk)
{
  GtkTextBuffer *buffer;
  GtkTextIter start_iter, end_iter;
  GtkTextTag* tag;
  char *selection;
  (void)bulk;

  if (!ih->data->is_multiline)
    return;

  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));

  selection = iupAttribGet(formattag, "SELECTION");
  if (selection)
  {
    if (!gtkTextSelectionGetIter(ih, selection, &start_iter, &end_iter))
      return;
  }
  else
  {
    char* selectionpos = iupAttribGet(formattag, "SELECTIONPOS");
    if (selectionpos)
    {
      if (!gtkTextSelectionPosGetIter(ih, selectionpos, &start_iter, &end_iter))
        return;
    }
    else
    {
      GtkTextMark* mark = gtk_text_buffer_get_insert(buffer);
      gtk_text_buffer_get_iter_at_mark(buffer, &start_iter, mark);
      gtk_text_buffer_get_iter_at_mark(buffer, &end_iter, mark);
    }
  }

  {
    char* numbering = iupAttribGet(formattag, "NUMBERING");
    if (numbering && !iupStrEqualNoCase(numbering, "NONE"))
    {
      char* numbering_style = iupAttribGet(formattag, "NUMBERINGSTYLE");
      gtkTextApplyNumbering(buffer, &start_iter, &end_iter, numbering, numbering_style);
    }
  }

  {
    char* image_name = iupAttribGet(formattag, "IMAGE");
    if (image_name)
    {
      GdkPixbuf* pixbuf = (GdkPixbuf*)iupImageGetImage(image_name, ih, 0, NULL);
      if (pixbuf)
      {
        int img_w, img_h;
        int new_w = 0, new_h = 0;
        char* attr;
        GdkPixbuf* scaled = NULL;

        iupImageGetInfo(image_name, &img_w, &img_h, NULL);

        attr = iupAttribGet(formattag, "WIDTH");
        if (attr) iupStrToInt(attr, &new_w);
        attr = iupAttribGet(formattag, "HEIGHT");
        if (attr) iupStrToInt(attr, &new_h);

        if ((new_w > 0 && new_w != img_w) || (new_h > 0 && new_h != img_h))
        {
          if (new_w <= 0) new_w = img_w;
          if (new_h <= 0) new_h = img_h;
          scaled = gdk_pixbuf_scale_simple(pixbuf, new_w, new_h, GDK_INTERP_BILINEAR);
          pixbuf = scaled;
        }

        if (!gtk_text_iter_equal(&start_iter, &end_iter))
          gtk_text_buffer_delete(buffer, &start_iter, &end_iter);

        gtk_text_buffer_insert_pixbuf(buffer, &start_iter, pixbuf);

        if (scaled)
          g_object_unref(scaled);
      }
      return;
    }
  }

  tag = gtk_text_buffer_create_tag(buffer, NULL, NULL);
  gtkTextParseParagraphFormat(formattag, tag);
  gtkTextParseCharacterFormat(formattag, tag);
  gtk_text_buffer_apply_tag(buffer, tag, &start_iter, &end_iter);
}

static int gtkTextSetRemoveFormattingAttrib(Ihandle* ih, const char* value)
{
  GtkTextBuffer *buffer;
  GtkTextIter start_iter, end_iter;

  if (!ih->data->is_multiline)
    return 0;

  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
  if (iupStrEqualNoCase(value, "ALL"))
  {
    gtk_text_buffer_get_start_iter(buffer, &start_iter);
    gtk_text_buffer_get_end_iter(buffer, &end_iter);
    gtk_text_buffer_remove_all_tags(buffer, &start_iter, &end_iter);
  }
  else
  {
    if (gtk_text_buffer_get_selection_bounds(buffer, &start_iter, &end_iter))
      gtk_text_buffer_remove_all_tags(buffer, &start_iter, &end_iter);
  }

  return 0;
}


/************************************************************************************************/

static gint gtkTextSpinInput(GtkSpinButton *spin, gdouble *val, Ihandle* ih)
{
  (void)spin;
  *val = (double)iupAttribGetInt(ih, "_IUPGTK_SPIN_VALUE");
  /* called only when SPINAUTO=NO */
  return TRUE; /* disable input update */
}

static gboolean gtkTextSpinOutput(GtkSpinButton *spin, Ihandle* ih)
{
  if (iupAttribGet(ih, "_IUPGTK_SPIN_NOAUTO"))
  {
    GtkAdjustment* adjustment = gtk_spin_button_get_adjustment(spin);
    iupAttribSetInt(ih, "_IUPGTK_SPIN_VALUE", (int)gtk_adjustment_get_value(adjustment));
    return TRUE; /* disable output update */
  }
  else
  {
    if (ih->data->disable_callbacks==0)
      ih->data->disable_callbacks = 2; /* disable other text callbacks processing, but allow spin */
    return FALSE; /* let the system update the display */
  }
}

static void gtkTextSpinValueChanged(GtkSpinButton* spin, Ihandle* ih)
{
  IFni cb;
  (void)spin;

  if (ih->data->disable_callbacks == 1)
    return;
  ih->data->disable_callbacks = 0;  /* enable other text callbacks processing */

  cb = (IFni)IupGetCallback(ih, "SPIN_CB");
  if (cb) 
  {
    int pos, ret;
    if (iupAttribGet(ih, "_IUPGTK_SPIN_NOAUTO"))
      pos = iupAttribGetInt(ih, "_IUPGTK_SPIN_VALUE");
    else
      pos = gtk_spin_button_get_value_as_int((GtkSpinButton*)ih->handle);

    ret = cb(ih, pos);
    if (ret == IUP_IGNORE)
    {
      pos = iupAttribGetInt(ih, "_IUPGTK_SPIN_OLDVALUE");

      /* restore previous value */
      ih->data->disable_callbacks = 1;
      gtk_spin_button_set_value((GtkSpinButton*)ih->handle, (double)pos);
      ih->data->disable_callbacks = 0;

      if (iupAttribGet(ih, "_IUPGTK_SPIN_NOAUTO"))
        iupAttribSetInt(ih, "_IUPGTK_SPIN_VALUE", pos);
      ih->data->disable_callbacks = 0;
      return;
    }
  }

  iupBaseCallValueChangedCb(ih);

  iupAttribSetInt(ih, "_IUPGTK_SPIN_OLDVALUE", gtk_spin_button_get_value_as_int((GtkSpinButton*)ih->handle));
}

static int gtkTextSetSpinMinAttrib(Ihandle* ih, const char* value)
{
  if (GTK_IS_SPIN_BUTTON(ih->handle))
  {
    int min;
    if (iupStrToInt(value, &min))
    {
      int max = iupAttribGetInt(ih, "SPINMAX");

      ih->data->disable_callbacks = 1;
      gtk_spin_button_set_range((GtkSpinButton*)ih->handle, (double)min, (double)max);
      ih->data->disable_callbacks = 0;
    }
  }
  return 1;
}

static int gtkTextSetSpinMaxAttrib(Ihandle* ih, const char* value)
{
  if (GTK_IS_SPIN_BUTTON(ih->handle))
  {
    int max;
    if (iupStrToInt(value, &max))
    {
      int min = iupAttribGetInt(ih, "SPINMIN");
      ih->data->disable_callbacks = 1;
      gtk_spin_button_set_range((GtkSpinButton*)ih->handle, (double)min, (double)max);
      ih->data->disable_callbacks = 0;
    }
  }
  return 1;
}

static int gtkTextSetSpinIncAttrib(Ihandle* ih, const char* value)
{
  if (GTK_IS_SPIN_BUTTON(ih->handle))
  {
    int inc;
    if (iupStrToInt(value, &inc))
      gtk_spin_button_set_increments((GtkSpinButton*)ih->handle, (double)inc, (double)(inc*10));
  }
  return 1;
}

static int gtkTextSetSpinValueAttrib(Ihandle* ih, const char* value)
{
  if (GTK_IS_SPIN_BUTTON(ih->handle))
  {
    int pos;
    if (iupStrToInt(value, &pos))
    {
      ih->data->disable_callbacks = 1;
      gtk_spin_button_set_value((GtkSpinButton*)ih->handle, (double)pos);
      ih->data->disable_callbacks = 0;

      if (iupAttribGet(ih, "_IUPGTK_SPIN_NOAUTO"))
        iupAttribSetInt(ih, "_IUPGTK_SPIN_VALUE", pos);
    }
  }
  return 1;
}

static char* gtkTextGetSpinValueAttrib(Ihandle* ih)
{
  if (GTK_IS_SPIN_BUTTON(ih->handle))
  {
    int pos;

    if (iupAttribGet(ih, "_IUPGTK_SPIN_NOAUTO"))
      pos = iupAttribGetInt(ih, "_IUPGTK_SPIN_VALUE");
    else
    {
      gtk_spin_button_update((GtkSpinButton*)ih->handle);
      pos = gtk_spin_button_get_value_as_int((GtkSpinButton*)ih->handle);
    }

    return iupStrReturnInt(pos);
  }
  return NULL;
}


/**********************************************************************************************************/


static void gtkTextMoveCursor(GtkWidget *w, GtkMovementStep step, gint count, gboolean extend_selection, Ihandle* ih)
{
  int col, lin, pos;

  IFniii cb = (IFniii)IupGetCallback(ih, "CARET_CB");
  if (!cb) return;

  if (ih->data->is_multiline)
  {
    GtkTextIter iter;

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
    gtk_text_buffer_get_iter_at_mark(buffer, &iter, gtk_text_buffer_get_insert(buffer));
    gtkTextGetLinColFromPosition(&iter, &lin, &col);
    pos = gtk_text_iter_get_offset(&iter);
  }
  else
  {
    pos = gtk_editable_get_position(GTK_EDITABLE(ih->handle));
    col = pos;
    col++; /* IUP starts at 1 */
    lin = 1;
  }

  if (pos != ih->data->last_caret_pos)
  {
    ih->data->last_caret_pos = pos;

    cb(ih, lin, col, pos);
  }

  (void)w;
  (void)step;
  (void)count;
  (void)extend_selection;
}

static gboolean gtkTextKeyReleaseEvent(GtkWidget *widget, GdkEventKey *evt, Ihandle *ih)
{
  gtkTextMoveCursor(NULL, 0, 0, 0, ih);
  (void)widget;
  (void)evt;
  return FALSE;
}

static gboolean gtkTextButtonEvent(GtkWidget *widget, GdkEventButton *evt, Ihandle *ih)
{
  gtkTextMoveCursor(NULL, 0, 0, 0, ih);
  return iupgtkButtonEvent(widget, evt, ih);
}

static void gtkTextEntryDeleteText(GtkEditable *editable, int start, int end, Ihandle* ih)
{
  IFnis cb = (IFnis)IupGetCallback(ih, "ACTION");
  int ret;

  if (ih->data->disable_callbacks)
    return;

  ret = iupEditCallActionCb(ih, cb, NULL, start, end, ih->data->mask, ih->data->nc, 1, iupgtkStrGetUTF8Mode());
  if (ret == 0)
    g_signal_stop_emission_by_name (editable, "delete_text");
}

static void gtkTextEntryInsertText(GtkEditable *editable, char *insert_value, int len, int *pos, Ihandle* ih)
{
  IFnis cb = (IFnis)IupGetCallback(ih, "ACTION");
  int ret;

  if (ih->data->disable_callbacks)
    return;

  ret = iupEditCallActionCb(ih, cb, iupgtkStrConvertFromSystem(insert_value), *pos, *pos, ih->data->mask, ih->data->nc, 0, iupgtkStrGetUTF8Mode());
  if (ret == 0)
    g_signal_stop_emission_by_name(editable, "insert_text");
  else if (ret != -1)
  {
    insert_value[0] = (char)ret;  /* replace key */

    ih->data->disable_callbacks = 1;
    gtk_editable_insert_text(editable, insert_value, 1, pos);
    ih->data->disable_callbacks = 0;

    g_signal_stop_emission_by_name(editable, "insert_text"); 
  }

  (void)len;
}

static void gtkTextBufferDeleteRange(GtkTextBuffer *textbuffer, GtkTextIter *start_iter, GtkTextIter *end_iter, Ihandle* ih)
{
  IFnis cb = (IFnis)IupGetCallback(ih, "ACTION");
  int start, end;
  int ret;

  if (ih->data->disable_callbacks)
    return;

  start = gtk_text_iter_get_offset(start_iter);
  end = gtk_text_iter_get_offset(end_iter);

  ret = iupEditCallActionCb(ih, cb, NULL, start, end, ih->data->mask, ih->data->nc, 1, iupgtkStrGetUTF8Mode());
  if (ret == 0)
    g_signal_stop_emission_by_name (textbuffer, "delete_range");
}

static void gtkTextBufferInsertText(GtkTextBuffer *textbuffer, GtkTextIter *pos_iter, gchar *insert_value, gint len, Ihandle* ih)
{
  IFnis cb = (IFnis)IupGetCallback(ih, "ACTION");
  int ret, pos;

  if (ih->data->disable_callbacks)
    return;

  pos = gtk_text_iter_get_offset(pos_iter);

  ret = iupEditCallActionCb(ih, cb, iupgtkStrConvertFromSystem(insert_value), pos, pos, ih->data->mask, ih->data->nc, 0, iupgtkStrGetUTF8Mode());
  if (ret == 0)
    g_signal_stop_emission_by_name(textbuffer, "insert_text");
  else if (ret != -1)
  {
    insert_value[0] = (char)ret;  /* replace key */

    ih->data->disable_callbacks = 1;
    gtk_text_buffer_insert(textbuffer, pos_iter, insert_value, 1);
    ih->data->disable_callbacks = 0;

    g_signal_stop_emission_by_name(textbuffer, "insert_text"); 
  }

  (void)len;
}

static void gtkTextChanged(void* dummy, Ihandle* ih)
{
  if (ih->data->disable_callbacks)
    return;

  iupBaseCallValueChangedCb(ih);
  (void)dummy;
}

/**********************************************************************************************************/

/* Callback to track scrolled window size allocation and clamp if needed */
static void gtkTextScrolledWindowSizeAllocate(GtkWidget* widget, GdkRectangle* allocation, gpointer user_data)
{
  Ihandle* ih = (Ihandle*)user_data;
  int sw_req_w, sw_req_h;

  gtk_widget_get_size_request(widget, &sw_req_w, &sw_req_h);

  int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
  if (visiblelines > 0 && sw_req_h > 0 && allocation->height > sw_req_h)
  {
    /* Create a new clamped allocation and apply it */
    GtkAllocation clamped = *allocation;
    clamped.height = sw_req_h;

    /* Block this signal handler to prevent recursion */
    g_signal_handlers_block_by_func(widget, gtkTextScrolledWindowSizeAllocate, user_data);

    /* Apply the clamped allocation - this will allocate children correctly */
    gtk_widget_size_allocate(widget, &clamped);

    /* Unblock the signal handler */
    g_signal_handlers_unblock_by_func(widget, gtkTextScrolledWindowSizeAllocate, user_data);

    /* Update the allocation parameter to reflect what we actually did */
    *allocation = clamped;
  }
}

static int gtkTextMapMethod(Ihandle* ih)
{
  GtkScrolledWindow* scrolled_window = NULL;

  if (ih->data->is_multiline)
  {
    GtkPolicyType hscrollbar_policy, vscrollbar_policy;
    int wordwrap = 0;

    ih->handle = gtk_text_view_new();
    if (!ih->handle)
      return IUP_ERROR;

    scrolled_window = (GtkScrolledWindow*)gtk_scrolled_window_new(NULL, NULL);
    if (!scrolled_window)
      return IUP_ERROR;

    gtk_container_add((GtkContainer*)scrolled_window, ih->handle);
    gtk_widget_show((GtkWidget*)scrolled_window);

    iupAttribSet(ih, "_IUP_EXTRAPARENT", (char*)scrolled_window);

    /* formatting is always supported when MULTILINE=YES */
    ih->data->has_formatting = 1;

    if (iupAttribGetBoolean(ih, "WORDWRAP"))
    {
      wordwrap = 1;
      ih->data->sb &= ~IUP_SB_HORIZ;  /* must remove the horizontal scrollbar */
    }

    if (iupAttribGetBoolean(ih, "BORDER"))
      gtk_scrolled_window_set_shadow_type(scrolled_window, GTK_SHADOW_IN); 
    else
      gtk_scrolled_window_set_shadow_type(scrolled_window, GTK_SHADOW_NONE);

    if (ih->data->sb & IUP_SB_HORIZ)
    {
      if (iupAttribGetBoolean(ih, "AUTOHIDE"))
        hscrollbar_policy = GTK_POLICY_AUTOMATIC;
      else
        hscrollbar_policy = GTK_POLICY_ALWAYS;
    }
    else
      hscrollbar_policy = GTK_POLICY_NEVER;

    if (ih->data->sb & IUP_SB_VERT)
    {
      if (iupAttribGetBoolean(ih, "AUTOHIDE"))
        vscrollbar_policy = GTK_POLICY_AUTOMATIC;
      else
        vscrollbar_policy = GTK_POLICY_ALWAYS;
    }
    else
      vscrollbar_policy = GTK_POLICY_NEVER;

    gtk_scrolled_window_set_policy(scrolled_window, hscrollbar_policy, vscrollbar_policy);

    /* Track scrolled window size allocation for VISIBLELINES clamping */
    g_signal_connect(G_OBJECT(scrolled_window), "size-allocate", G_CALLBACK(gtkTextScrolledWindowSizeAllocate), ih);

    /* Mark scrolled_window with VISIBLELINES flag so iupgtkSetPosSize can set size correctly */
    {
      int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
      if (visiblelines > 0)
      {
        /* Mark scrolled_window so iupgtkSetPosSize will use IUP's calculated height directly */
        g_object_set_data(G_OBJECT(scrolled_window), "iup-visiblelines-set", (gpointer)"1");
      }
    }

    if (wordwrap)
      gtk_text_view_set_wrap_mode((GtkTextView*)ih->handle, GTK_WRAP_WORD);

    gtk_widget_add_events(ih->handle, GDK_ENTER_NOTIFY_MASK|GDK_LEAVE_NOTIFY_MASK);
  }
  else
  {
    if (iupAttribGetBoolean(ih, "SPIN"))
      ih->handle = gtk_spin_button_new_with_range((double)iupAttribGetInt(ih, "SPINMIN"), (double)iupAttribGetInt(ih, "SPINMAX"), (double)iupAttribGetInt(ih, "SPININC"));  /* It inherits from GtkEntry */
    else
      ih->handle = gtk_entry_new();

    if (!ih->handle)
      return IUP_ERROR;

    /* formatting is never supported when MULTILINE=NO */
    ih->data->has_formatting = 0;

#if GTK_CHECK_VERSION(3, 0, 0)
    /* Set natural alignment */
    gtk_widget_set_hexpand(ih->handle, FALSE);
    gtk_widget_set_vexpand(ih->handle, FALSE);
    gtk_widget_set_halign(ih->handle, GTK_ALIGN_FILL);
    gtk_widget_set_valign(ih->handle, GTK_ALIGN_CENTER);
#endif

    gtk_entry_set_has_frame((GtkEntry*)ih->handle, iupAttribGetBoolean(ih, "BORDER"));
    gtk_entry_set_width_chars((GtkEntry*)ih->handle, 1);  /* minimum size */
#if GTK_CHECK_VERSION(3, 14, 0)
    if (!iupAttribGetBoolean(ih, "BORDER"))
    {
      GtkStyleContext *context = gtk_widget_get_style_context(ih->handle);
      GtkCssProvider *provider = gtk_css_provider_new();
      gtk_css_provider_load_from_data(GTK_CSS_PROVIDER(provider), "*{ border: none; }", -1, NULL);
      gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
      g_object_unref(provider);
    }
#endif

    if (iupAttribGetBoolean(ih, "PASSWORD"))
      gtk_entry_set_visibility((GtkEntry*)ih->handle, FALSE);

    if (GTK_IS_SPIN_BUTTON(ih->handle))
    {
      gtk_spin_button_set_numeric((GtkSpinButton*)ih->handle, FALSE);
      gtk_spin_button_set_digits((GtkSpinButton*)ih->handle, 0);

      gtk_spin_button_set_wrap((GtkSpinButton*)ih->handle, iupAttribGetBoolean(ih, "SPINWRAP"));

      g_signal_connect(G_OBJECT(ih->handle), "value-changed", G_CALLBACK(gtkTextSpinValueChanged), ih);
      g_signal_connect(G_OBJECT(ih->handle), "output", G_CALLBACK(gtkTextSpinOutput), ih);

      if (!iupAttribGetBoolean(ih, "SPINAUTO"))
      {
        g_signal_connect(G_OBJECT(ih->handle), "input", G_CALLBACK(gtkTextSpinInput), ih);
        iupAttribSet(ih, "_IUPGTK_SPIN_NOAUTO", "1");
      }
    }

  }

  /* add to the parent, all GTK controls must call this. */
  iupgtkAddToParent(ih);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    iupgtkSetCanFocus(ih->handle, 0);

  g_signal_connect(G_OBJECT(ih->handle), "enter-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "leave-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "focus-in-event",     G_CALLBACK(iupgtkFocusInOutEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "focus-out-event",    G_CALLBACK(iupgtkFocusInOutEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "key-press-event",    G_CALLBACK(iupgtkKeyPressEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "show-help",          G_CALLBACK(iupgtkShowHelp), ih);

  g_signal_connect_after(G_OBJECT(ih->handle), "move-cursor", G_CALLBACK(gtkTextMoveCursor), ih);  /* only report some caret movements */
  g_signal_connect_after(G_OBJECT(ih->handle), "key-release-event", G_CALLBACK(gtkTextKeyReleaseEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "button-press-event", G_CALLBACK(gtkTextButtonEvent), ih);  /* if connected "after" then it is ignored */
  g_signal_connect(G_OBJECT(ih->handle), "button-release-event",G_CALLBACK(gtkTextButtonEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "motion-notify-event",G_CALLBACK(iupgtkMotionNotifyEvent), ih);

  if (ih->data->is_multiline)
  {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
    g_signal_connect(G_OBJECT(buffer), "delete-range", G_CALLBACK(gtkTextBufferDeleteRange), ih);
    g_signal_connect(G_OBJECT(buffer), "insert-text", G_CALLBACK(gtkTextBufferInsertText), ih);
    g_signal_connect(G_OBJECT(buffer), "changed", G_CALLBACK(gtkTextChanged), ih);
  }
  else
  {
    g_signal_connect(G_OBJECT(ih->handle), "delete-text", G_CALLBACK(gtkTextEntryDeleteText), ih);
    g_signal_connect(G_OBJECT(ih->handle), "insert-text", G_CALLBACK(gtkTextEntryInsertText), ih);
    g_signal_connect(G_OBJECT(ih->handle), "changed", G_CALLBACK(gtkTextChanged), ih);
  }

  if (scrolled_window)
    gtk_widget_realize((GtkWidget*)scrolled_window);
  gtk_widget_realize(ih->handle);

  /* configure for DRAG&DROP */
  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSet(ih, "DROPFILESTARGET", "YES");

  /* update a mnemonic in a label if necessary */
  iupgtkUpdateMnemonic(ih);

  if (ih->data->formattags)
    iupTextUpdateFormatTags(ih);

  IupSetCallback(ih, "_IUP_XY2POS_CB", (Icallback)gtkTextConvertXYToPos);

  return IUP_NOERROR;
}

void iupdrvTextInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = gtkTextMapMethod;

  /* Driver Dependent Attribute functions */

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, gtkTextSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);

  /* Special */
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, gtkTextSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);

  /* IupText only */
  iupClassRegisterAttribute(ic, "PADDING", iupTextGetPaddingAttrib, gtkTextSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "VALUE", gtkTextGetValueAttrib, gtkTextSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINEVALUE", gtkTextGetLineValueAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTEDTEXT", gtkTextGetSelectedTextAttrib, gtkTextSetSelectedTextAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTION", gtkTextGetSelectionAttrib, gtkTextSetSelectionAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTIONPOS", gtkTextGetSelectionPosAttrib, gtkTextSetSelectionPosAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARET", gtkTextGetCaretAttrib, gtkTextSetCaretAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARETPOS", gtkTextGetCaretPosAttrib, gtkTextSetCaretPosAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERT", NULL, gtkTextSetInsertAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPEND", NULL, gtkTextSetAppendAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "READONLY", gtkTextGetReadOnlyAttrib, gtkTextSetReadOnlyAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "NC", iupTextGetNCAttrib, gtkTextSetNCAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "CLIPBOARD", NULL, gtkTextSetClipboardAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTO", NULL, gtkTextSetScrollToAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTOPOS", NULL, gtkTextSetScrollToPosAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINMIN", NULL, gtkTextSetSpinMinAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINMAX", NULL, gtkTextSetSpinMaxAttrib, IUPAF_SAMEASSYSTEM, "100", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPININC", NULL, gtkTextSetSpinIncAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINVALUE", gtkTextGetSpinValueAttrib, gtkTextSetSpinValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COUNT", gtkTextGetCountAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINECOUNT", gtkTextGetLineCountAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);

  /* IupText Windows and GTK only */
  iupClassRegisterAttribute(ic, "ADDFORMATTAG", NULL, iupTextSetAddFormatTagAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ADDFORMATTAG_HANDLE", NULL, iupTextSetAddFormatTagHandleAttrib, NULL, NULL, IUPAF_IHANDLE | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, gtkTextSetAlignmentAttrib, IUPAF_SAMEASSYSTEM, "ALEFT", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATTING", iupTextGetFormattingAttrib, iupTextSetFormattingAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OVERWRITE", gtkTextGetOverwriteAttrib, gtkTextSetOverwriteAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "REMOVEFORMATTING", NULL, gtkTextSetRemoveFormattingAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABSIZE", NULL, gtkTextSetTabSizeAttrib, "8", NULL, IUPAF_DEFAULT);  /* force new default value */
  iupClassRegisterAttribute(ic, "PASSWORD", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUEBANNER", NULL, gtkTextSetCueBannerAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  /* Not Supported */
  iupClassRegisterAttribute(ic, "FILTER", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLVISIBLE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}
