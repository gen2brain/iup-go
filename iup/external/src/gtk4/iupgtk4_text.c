/** \file
 * \brief Text Control for GTK4
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

#include "iupgtk4_drv.h"


void iupdrvTextAddSpin(Ihandle* ih, int *w, int h)
{
  static int spin_min_width = -1;

  (void)h;
  (void)ih;

  /* Measure the minimum width required by GtkSpinButton */
  if (spin_min_width < 0)
  {
    GtkWidget *temp_spin = gtk_spin_button_new_with_range(0, 100, 1);
    int min_w, nat_w;

    gtk_widget_measure(temp_spin, GTK_ORIENTATION_HORIZONTAL, -1, &min_w, &nat_w, NULL, NULL);

    /* Use minimum width with fallback */
    spin_min_width = (min_w > 0) ? min_w : 130;

    g_object_ref_sink(temp_spin);
    g_object_unref(temp_spin);
  }

  /* Only enforce minimum width, don't force expansion */
  if (*w < spin_min_width)
    *w = spin_min_width;
}

static int gtk4_multiline_border_height = -1;
static int gtk4_multiline_border_width = -1;
static int gtk4_multiline_line_height = -1;
static int gtk4_entry_border_x = -1;
static int gtk4_entry_border_y = -1;
static int gtk4_entry_noframe_border_y = -1;
static int gtk4_entry_css_dec_x = 0;
static int gtk4_entry_char_adjust_x = 0;

static void gtk4TextMeasureEntryBorders(void)
{
  if (gtk4_entry_border_x < 0)
  {
    GtkWidget *temp_entry, *temp_entry_noframe;
    int entry_w, entry_h, entry_noframe_h;
    int char_width, char_height;
    PangoContext *context;
    PangoLayout *layout;

    /* Create entry with frame. Set width_chars=1 and max_width_chars=1 to isolate CSS decoration. */
    temp_entry = gtk_entry_new();
    gtk_entry_set_has_frame(GTK_ENTRY(temp_entry), TRUE);
    gtk_editable_set_width_chars(GTK_EDITABLE(temp_entry), 1);
    gtk_editable_set_max_width_chars(GTK_EDITABLE(temp_entry), 1);
    g_object_ref_sink(temp_entry);

    /* Create separate entry without frame */
    temp_entry_noframe = gtk_entry_new();
    gtk_entry_set_has_frame(GTK_ENTRY(temp_entry_noframe), FALSE);
    g_object_ref_sink(temp_entry_noframe);

    context = gtk_widget_get_pango_context(temp_entry);
    layout = pango_layout_new(context);
    pango_layout_set_text(layout, "W", -1);
    pango_layout_get_pixel_size(layout, &char_width, &char_height);

    /* Measure with frame, minimum width = 1*char_pixels + CSS decoration */
    gtk_widget_measure(temp_entry, GTK_ORIENTATION_HORIZONTAL, -1, &entry_w, NULL, NULL, NULL);
    gtk_widget_measure(temp_entry, GTK_ORIENTATION_VERTICAL, -1, NULL, &entry_h, NULL, NULL);

    /* Measure without frame, get minimum size */
    gtk_widget_measure(temp_entry_noframe, GTK_ORIENTATION_VERTICAL, -1, &entry_noframe_h, NULL, NULL, NULL);

    gtk4_entry_border_x = entry_w - char_width;
    gtk4_entry_border_y = entry_h - char_height;
    gtk4_entry_noframe_border_y = entry_noframe_h - char_height;

    if (gtk4_entry_border_x < 0) gtk4_entry_border_x = 10;
    if (gtk4_entry_border_y < 0) gtk4_entry_border_y = 10;
    if (gtk4_entry_noframe_border_y < 0) gtk4_entry_noframe_border_y = 0;

    {
      PangoFontMetrics* metrics = pango_context_get_metrics(context,
          pango_context_get_font_description(context),
          pango_context_get_language(context));
      int avg_w = pango_font_metrics_get_approximate_char_width(metrics);
      int digit_w = pango_font_metrics_get_approximate_digit_width(metrics);
      int gtk_char_pixels = (MAX(avg_w, digit_w) + PANGO_SCALE - 1) / PANGO_SCALE;

      gtk4_entry_css_dec_x = entry_w - gtk_char_pixels;
      gtk4_entry_char_adjust_x = char_width - gtk_char_pixels;
      if (gtk4_entry_css_dec_x < 2) gtk4_entry_css_dec_x = 2;
      if (gtk4_entry_char_adjust_x < 0) gtk4_entry_char_adjust_x = 0;

      pango_font_metrics_unref(metrics);
    }

    g_object_unref(layout);
    g_object_unref(temp_entry);
    g_object_unref(temp_entry_noframe);
  }
}

static void gtk4TextMeasureMultilineMetrics(void)
{
  if (gtk4_multiline_border_height < 0)
  {
    GtkWidget *temp_sw, *temp_tv;
    PangoContext *context;
    PangoLayout *layout;
    int layout_1line_h, layout_2line_h;
    int sw_empty_h;

    temp_tv = gtk_text_view_new();
    temp_sw = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(temp_sw), temp_tv);
    gtk_scrolled_window_set_has_frame(GTK_SCROLLED_WINDOW(temp_sw), TRUE);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(temp_sw), GTK_POLICY_NEVER, GTK_POLICY_NEVER);

    g_object_ref_sink(temp_sw);

    gtk_widget_measure(temp_sw, GTK_ORIENTATION_VERTICAL, -1, NULL, &sw_empty_h, NULL, NULL);

    {
      int sw_empty_w;
      gtk_widget_measure(temp_sw, GTK_ORIENTATION_HORIZONTAL, -1, &sw_empty_w, NULL, NULL, NULL);
      gtk4_multiline_border_width = sw_empty_w;
      if (gtk4_multiline_border_width < 0) gtk4_multiline_border_width = 2;
    }

    context = gtk_widget_get_pango_context(temp_tv);
    layout = pango_layout_new(context);

    pango_layout_set_text(layout, "X", -1);
    pango_layout_get_pixel_size(layout, NULL, &layout_1line_h);

    pango_layout_set_text(layout, "X\nX", -1);
    pango_layout_get_pixel_size(layout, NULL, &layout_2line_h);

    gtk4_multiline_line_height = layout_2line_h - layout_1line_h;
    if (gtk4_multiline_line_height <= 0) gtk4_multiline_line_height = 16;

    gtk4_multiline_border_height = sw_empty_h;
    if (gtk4_multiline_border_height < 0) gtk4_multiline_border_height = 2;

    g_object_unref(layout);
    g_object_unref(temp_sw);
  }
}

void iupdrvTextAddBorders(Ihandle* ih, int *x, int *y)
{
  gtk4TextMeasureEntryBorders();

  static int spin_natural_height = -1;
  if (iupAttribGetBoolean(ih, "SPIN") && spin_natural_height == -1)
  {
    GtkWidget *temp_spin = gtk_spin_button_new_with_range(0, 100, 1);

    int spin_h;
    gtk_widget_measure(temp_spin, GTK_ORIENTATION_VERTICAL, -1, NULL, &spin_h, NULL, NULL);

    spin_natural_height = spin_h;
    if (spin_natural_height < 16) spin_natural_height = 34;

    g_object_ref_sink(temp_spin);
    g_object_unref(temp_spin);
  }

  if (ih->data && ih->data->is_multiline)
  {
    int visiblecolumns = iupAttribGetInt(ih, "VISIBLECOLUMNS");
    int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");

    gtk4TextMeasureMultilineMetrics();

    (*x) += gtk4_multiline_border_width - visiblecolumns * gtk4_entry_char_adjust_x;

    if (visiblelines > 0)
    {
      int char_height;
      iupdrvFontGetCharSize(ih, NULL, &char_height);

      int iup_content_h = char_height * visiblelines;
      int gtk_content_h = gtk4_multiline_line_height * visiblelines;
      int line_diff = iup_content_h - gtk_content_h;

      (*y) += gtk4_multiline_border_height;
      (*y) -= line_diff;
    }
    else
    {
      (*y) += gtk4_multiline_border_height;
    }
  }
  else
  {
    int visiblecolumns = iupAttribGetInt(ih, "VISIBLECOLUMNS");
    int border = gtk4_entry_css_dec_x - visiblecolumns * gtk4_entry_char_adjust_x;
    (*x) += border;

    if (iupAttribGetBoolean(ih, "SPIN"))
    {
      int before = *y;
      int add = spin_natural_height - before;
      if (add < 0) add = 0;
      (*y) += add;
    }
    else
    {
      (*y) += gtk4_entry_border_y;
    }
  }
}

void iupdrvTextAddExtraPadding(Ihandle* ih, int *w, int *h)
{
  (void)ih;
  gtk4TextMeasureEntryBorders();
  if (w) *w += gtk4_entry_noframe_border_y;
  if (h) *h += gtk4_entry_noframe_border_y;
}

static void gtkTextMoveIterToLinCol(GtkTextBuffer* buffer, GtkTextIter* iter, int lin, int col)
{
  int line_count = gtk_text_buffer_get_line_count(buffer);
  if (lin < 1) lin = 1;
  if (lin > line_count) lin = line_count;
  lin--;
  col--;

  gtk_text_buffer_get_iter_at_line(buffer, iter, lin);

  if (col != 0)
  {
    int line_length = gtk_text_iter_get_chars_in_line(iter);
    if (line_length > 0)
    {
      if (col > line_length)
        col = line_length;
      gtk_text_iter_set_line_offset(iter, col);
    }
  }
}

static void gtkTextGetLinColFromPosition(const GtkTextIter *iter, int *lin, int *col)
{
  *lin = gtk_text_iter_get_line(iter);
  *col = gtk_text_iter_get_line_offset(iter);
  (*lin)++;
  (*col)++;
}

void iupdrvTextConvertLinColToPos(Ihandle* ih, int lin, int col, int *pos)
{
  if (ih->data->is_multiline)
  {
    GtkTextIter iter;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
    gtkTextMoveIterToLinCol(buffer, &iter, lin, col);
    *pos = gtk_text_iter_get_offset(&iter);
  }
  else
  {
    *pos = col - 1;
  }
}

void iupdrvTextConvertPosToLinCol(Ihandle* ih, int pos, int *lin, int *col)
{
  if (ih->data->is_multiline)
  {
    GtkTextIter iter;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
    gtk_text_buffer_get_iter_at_offset(buffer, &iter, pos);
    gtkTextGetLinColFromPosition(&iter, lin, col);
  }
  else
  {
    *col = pos + 1;
    *lin = 1;
  }
}

static int gtkTextConvertXYToPos(Ihandle* ih, int x, int y)
{
  if (ih->data->is_multiline)
  {
    GtkTextIter iter;
    int mx = x, my = y;
    gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(ih->handle), GTK_TEXT_WINDOW_WIDGET, x, y, &mx, &my);
    gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(ih->handle), &iter, mx, my);
    return gtk_text_iter_get_offset(&iter);
  }
  else
  {
    int pos = 0;
    int trailing;
    const char* text = gtk_editable_get_text(GTK_EDITABLE(ih->handle));
    PangoLayout* layout = gtk_widget_create_pango_layout(ih->handle, text);
    pango_layout_xy_to_index(layout, x * PANGO_SCALE, y * PANGO_SCALE, &pos, &trailing);
    g_object_unref(layout);
    return pos + trailing;
  }
}

static void gtkTextScrollToVisible(Ihandle* ih)
{
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
  GtkTextMark* mark = gtk_text_buffer_get_insert(buffer);
  gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(ih->handle), mark);
}

static gboolean gtk4TextMoveCursor(GtkWidget *widget, GtkMovementStep step, gint count, gboolean extend_selection, Ihandle* ih)
{
  IFniii cb = (IFniii)IupGetCallback(ih, "CARET_CB");
  if (cb)
  {
    int pos, lin, col;
    if (ih->data->is_multiline)
    {
      GtkTextIter iter;
      GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
      gtk_text_buffer_get_iter_at_mark(buffer, &iter, gtk_text_buffer_get_insert(buffer));
      pos = gtk_text_iter_get_offset(&iter);
      gtkTextGetLinColFromPosition(&iter, &lin, &col);
    }
    else
    {
      pos = gtk_editable_get_position(GTK_EDITABLE(ih->handle));
      lin = 1;
      col = pos + 1;
    }
    cb(ih, lin, col, pos);
  }

  (void)widget;
  (void)step;
  (void)count;
  (void)extend_selection;
  return FALSE;
}

static gboolean gtk4TextKeyReleaseEvent(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, Ihandle *ih)
{
  (void)controller;
  (void)keyval;
  (void)keycode;
  (void)state;

  IFniii cb = (IFniii)IupGetCallback(ih, "CARET_CB");
  if (cb)
  {
    int pos, lin, col;
    if (ih->data->is_multiline)
    {
      GtkTextIter iter;
      GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
      gtk_text_buffer_get_iter_at_mark(buffer, &iter, gtk_text_buffer_get_insert(buffer));
      pos = gtk_text_iter_get_offset(&iter);
      gtkTextGetLinColFromPosition(&iter, &lin, &col);
    }
    else
    {
      pos = gtk_editable_get_position(GTK_EDITABLE(ih->handle));
      lin = 1;
      col = pos + 1;
    }
    cb(ih, lin, col, pos);
  }

  return FALSE;
}

static gboolean gtk4TextButtonEvent(GtkGestureClick *gesture, int n_press, double x, double y, Ihandle *ih)
{
  (void)gesture;
  (void)n_press;
  (void)x;
  (void)y;

  IFniii cb = (IFniii)IupGetCallback(ih, "CARET_CB");
  if (cb)
  {
    int pos, lin, col;
    if (ih->data->is_multiline)
    {
      GtkTextIter iter;
      GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
      gtk_text_buffer_get_iter_at_mark(buffer, &iter, gtk_text_buffer_get_insert(buffer));
      pos = gtk_text_iter_get_offset(&iter);
      gtkTextGetLinColFromPosition(&iter, &lin, &col);
    }
    else
    {
      pos = gtk_editable_get_position(GTK_EDITABLE(ih->handle));
      lin = 1;
      col = pos + 1;
    }
    cb(ih, lin, col, pos);
  }

  return FALSE;
}

static void gtk4TextBufferDeleteRange(GtkTextBuffer *buffer, GtkTextIter *start, GtkTextIter *end, Ihandle* ih)
{
  if (!ih->data->disable_callbacks)
  {
    int start_pos, end_pos;
    IFnis cb = (IFnis)IupGetCallback(ih, "ACTION");
    if (cb)
    {
      int ret;
      char *text, *suffix;

      start_pos = gtk_text_iter_get_offset(start);
      end_pos = gtk_text_iter_get_offset(end);

      text = gtk_text_buffer_get_text(buffer, start, end, FALSE);

      suffix = &text[end_pos - start_pos];
      ret = cb(ih, start_pos + 1, (char*)iupgtk4StrConvertFromSystem(text));
      if (ret == IUP_IGNORE)
      {
        g_signal_stop_emission_by_name(buffer, "delete_range");
        return;
      }
    }
  }

  (void)buffer;
}

static void gtk4TextBufferInsertText(GtkTextBuffer *buffer, GtkTextIter *pos, gchar *text, gint len, Ihandle* ih)
{
  IFnis cb;
  int ret;
  char* insert_value;
  int start_pos;

  if (ih->data->disable_callbacks)
    return;

  cb = (IFnis)IupGetCallback(ih, "ACTION");

  start_pos = gtk_text_iter_get_offset(pos);

  if (len < 0)
    insert_value = iupStrDup((char*)iupgtk4StrConvertFromSystem(text));
  else
  {
    insert_value = malloc(len + 1);
    memcpy(insert_value, text, len);
    insert_value[len] = 0;
    insert_value = (char*)iupgtk4StrConvertFromSystem(insert_value);
  }

  ret = iupEditCallActionCb(ih, cb, insert_value, start_pos, start_pos, ih->data->mask, ih->data->nc, 0, 1);  /* GTK4 is always UTF-8 */
  if (ret == 0)
  {
    g_signal_stop_emission_by_name(buffer, "insert_text");
    free(insert_value);
    return;
  }
  else if (ret != -1)
  {
    char new_insert[2];
    new_insert[0] = (char)ret;  /* replace key */
    new_insert[1] = 0;

    ih->data->disable_callbacks = 1;
    gtk_text_buffer_insert(buffer, pos, new_insert, 1);
    ih->data->disable_callbacks = 0;

    g_signal_stop_emission_by_name(buffer, "insert_text");
    free(insert_value);
    return;
  }

  free(insert_value);
  (void)buffer;
  (void)len;
}

static const char* gtk4TextGetLinkUrlAtIter(GtkTextIter* iter)
{
  GSList* tags = gtk_text_iter_get_tags(iter);
  GSList* item;
  const char* url = NULL;

  for (item = tags; item != NULL; item = item->next)
  {
    GtkTextTag* tag = (GtkTextTag*)item->data;
    url = (const char*)g_object_get_data(G_OBJECT(tag), "iup-link-url");
    if (url)
      break;
  }

  g_slist_free(tags);
  return url;
}

static void gtk4TextLinkClick(GtkGestureClick *gesture, int n_press, double x, double y, Ihandle *ih)
{
  GtkTextView* text_view = GTK_TEXT_VIEW(gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture)));
  GtkTextIter iter;
  int buf_x, buf_y;

  (void)n_press;

  gtk_text_view_window_to_buffer_coords(text_view, GTK_TEXT_WINDOW_WIDGET, (int)x, (int)y, &buf_x, &buf_y);
  gtk_text_view_get_iter_at_location(text_view, &iter, buf_x, buf_y);

  {
    const char* url = gtk4TextGetLinkUrlAtIter(&iter);
    if (url)
    {
      IFns cb = (IFns)IupGetCallback(ih, "LINK_CB");
      if (cb)
      {
        int ret = cb(ih, (char*)url);
        if (ret == IUP_CLOSE)
          IupExitLoop();
        else if (ret == IUP_DEFAULT)
          IupHelp(url);
      }
      else
        IupHelp(url);
    }
  }
}

static void gtk4TextLinkMotion(GtkEventControllerMotion *controller, double x, double y, Ihandle *ih)
{
  GtkWidget* widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(controller));
  GtkTextView* text_view = GTK_TEXT_VIEW(widget);
  GtkTextIter iter;
  int buf_x, buf_y;

  gtk_text_view_window_to_buffer_coords(text_view, GTK_TEXT_WINDOW_WIDGET, (int)x, (int)y, &buf_x, &buf_y);
  gtk_text_view_get_iter_at_location(text_view, &iter, buf_x, buf_y);

  {
    const char* url = gtk4TextGetLinkUrlAtIter(&iter);
    if (url)
      gtk_widget_set_cursor_from_name(widget, "pointer");
    else
      gtk_widget_set_cursor_from_name(widget, "text");
  }

  (void)ih;
}

static void gtk4TextEntryDeleteText(GtkEditable *editable, gint start_pos, gint end_pos, Ihandle* ih)
{
  if (!ih->data->disable_callbacks)
  {
    IFnis cb = (IFnis)IupGetCallback(ih, "ACTION");
    if (cb)
    {
      int ret;
      char *text, *suffix;
      const char* entry_text = gtk_editable_get_text(editable);

      text = iupStrDup((char*)iupgtk4StrConvertFromSystem(entry_text));

      suffix = &text[end_pos];
      text[end_pos] = 0;

      ret = cb(ih, start_pos + 1, &text[start_pos]);

      text[end_pos] = *suffix;
      free(text);

      if (ret == IUP_IGNORE)
      {
        g_signal_stop_emission_by_name(editable, "delete_text");
        return;
      }
    }
  }

  (void)editable;
}

static void gtk4TextEntryInsertText(GtkEditable *editable, const gchar *text, gint len, gint *pos, Ihandle* ih)
{
  IFnis cb;
  int ret;
  char* insert_value;

  if (ih->data->disable_callbacks)
    return;

  cb = (IFnis)IupGetCallback(ih, "ACTION");

  if (len < 0)
    insert_value = iupStrDup((char*)iupgtk4StrConvertFromSystem(text));
  else
  {
    insert_value = malloc(len + 1);
    memcpy(insert_value, text, len);
    insert_value[len] = 0;
    insert_value = (char*)iupgtk4StrConvertFromSystem(insert_value);
  }

  ret = iupEditCallActionCb(ih, cb, insert_value, *pos, *pos, ih->data->mask, ih->data->nc, 0, 1);  /* GTK4 is always UTF-8 */
  if (ret == 0)
  {
    g_signal_stop_emission_by_name(editable, "insert_text");
    free(insert_value);
    return;
  }
  else if (ret != -1)
  {
    char new_insert[2];
    new_insert[0] = (char)ret;  /* replace key */
    new_insert[1] = 0;

    ih->data->disable_callbacks = 1;
    gtk_editable_insert_text(editable, new_insert, 1, pos);
    ih->data->disable_callbacks = 0;

    g_signal_stop_emission_by_name(editable, "insert_text");
    free(insert_value);
    return;
  }

  free(insert_value);
  (void)editable;
  (void)len;
}

static void gtk4TextChanged(GtkWidget *widget, Ihandle* ih)
{
  if (!ih->data->disable_callbacks)
  {
    IFn cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
    if (cb)
      cb(ih);
  }

  (void)widget;
}

static void gtk4TextSpinValueChanged(GtkSpinButton *spinbutton, Ihandle* ih)
{
  if (!ih->data->disable_callbacks)
  {
    IFni cb = (IFni)IupGetCallback(ih, "SPIN_CB");
    if (cb)
    {
      int pos = gtk_spin_button_get_value_as_int(spinbutton);
      cb(ih, pos);
    }
  }

  (void)spinbutton;
}

static gboolean gtk4TextSpinOutput(GtkSpinButton *spinbutton, Ihandle* ih)
{
  if (iupAttribGet(ih, "_IUPGTK4_SPIN_NOAUTO"))
  {
    return TRUE;
  }
  (void)spinbutton;
  (void)ih;
  return FALSE;
}

static gint gtk4TextSpinInput(GtkSpinButton *spinbutton, gdouble *new_val, Ihandle* ih)
{
  int pos;
  const char* value = gtk_editable_get_text(GTK_EDITABLE(spinbutton));

  if (!value || value[0] == 0)
    return GTK_INPUT_ERROR;

  if (iupStrToInt(value, &pos))
  {
    *new_val = (double)pos;
    return TRUE;
  }

  (void)ih;
  return GTK_INPUT_ERROR;
}

static char* gtk4TextGetValueAttrib(Ihandle* ih)
{
  char* value;

  if (ih->data->is_multiline)
  {
    GtkTextIter start_iter;
    GtkTextIter end_iter;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
    gtk_text_buffer_get_start_iter(buffer, &start_iter);
    gtk_text_buffer_get_end_iter(buffer, &end_iter);
    value = iupStrReturnStr(iupgtk4StrConvertFromSystem(gtk_text_buffer_get_text(buffer, &start_iter, &end_iter, TRUE)));
  }
  else
    value = iupStrReturnStr(iupgtk4StrConvertFromSystem(gtk_editable_get_text(GTK_EDITABLE(ih->handle))));

  if (!value) value = "";

  return value;
}

static char* gtk4TextGetLineValueAttrib(Ihandle* ih)
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
    return iupStrReturnStr(iupgtk4StrConvertFromSystem(gtk_text_buffer_get_text(buffer, &start_iter, &end_iter, TRUE)));
  }
  else
    return gtk4TextGetValueAttrib(ih);
}

static int gtk4TextSetValueAttrib(Ihandle* ih, const char* value)
{
  if (!value) value = "";
  ih->data->disable_callbacks = 1;
  if (ih->data->is_multiline)
  {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
    gtk_text_buffer_set_text(buffer, iupgtk4StrConvertToSystem(value), -1);
  }
  else
    gtk_editable_set_text(GTK_EDITABLE(ih->handle), iupgtk4StrConvertToSystem(value));
  ih->data->disable_callbacks = 0;
  return 0;
}

static int gtk4TextSetInsertAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)
    return 0;
  if (!value)
    return 0;

  ih->data->disable_callbacks = 1;
  if (ih->data->is_multiline)
  {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
    gtk_text_buffer_insert_at_cursor(buffer, iupgtk4StrConvertToSystem(value), -1);
  }
  else
  {
    int pos = gtk_editable_get_position(GTK_EDITABLE(ih->handle));
    gtk_editable_insert_text(GTK_EDITABLE(ih->handle), iupgtk4StrConvertToSystem(value), -1, &pos);
  }
  ih->data->disable_callbacks = 0;

  return 0;
}

static int gtk4TextSetAppendAttrib(Ihandle* ih, const char* value)
{
  int pos;
  if (!ih->handle)
    return 0;
  ih->data->disable_callbacks = 1;
  if (ih->data->is_multiline)
  {
    GtkTextIter iter;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
    gtk_text_buffer_get_end_iter(buffer, &iter);
    pos = gtk_text_buffer_get_char_count(buffer);
    if (ih->data->append_newline && pos != 0)
      gtk_text_buffer_insert(buffer, &iter, "\n", 1);
    gtk_text_buffer_insert(buffer, &iter, iupgtk4StrConvertToSystem(value), -1);

    gtk_text_buffer_get_end_iter(buffer, &iter);
    gtk_text_buffer_place_cursor(buffer, &iter);
    gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(ih->handle), gtk_text_buffer_get_insert(buffer));
  }
  else
  {
    pos = strlen(gtk_editable_get_text(GTK_EDITABLE(ih->handle))) + 1;
    gtk_editable_insert_text(GTK_EDITABLE(ih->handle), iupgtk4StrConvertToSystem(value), -1, &pos);
  }
  ih->data->disable_callbacks = 0;
  return 0;
}

static char* gtk4TextGetSelectionAttrib(Ihandle* ih)
{
  if (ih->data->is_multiline)
  {
    GtkTextIter start_iter, end_iter;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));

    if (gtk_text_buffer_get_selection_bounds(buffer, &start_iter, &end_iter))
    {
      int start_lin, start_col, end_lin, end_col;
      gtkTextGetLinColFromPosition(&start_iter, &start_lin, &start_col);
      gtkTextGetLinColFromPosition(&end_iter, &end_lin, &end_col);
      return iupStrReturnStrf("%d,%d:%d,%d", start_lin, start_col, end_lin, end_col);
    }
  }
  else
  {
    int start = 0, end = 0;
    if (gtk_editable_get_selection_bounds(GTK_EDITABLE(ih->handle), &start, &end))
      return iupStrReturnIntInt(start + 1, end + 1, ':');
  }

  return NULL;
}

static int gtk4TextSelectionGetIter(Ihandle* ih, const char* value, GtkTextIter* start_iter, GtkTextIter* end_iter)
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

static int gtk4TextSelectionPosGetIter(Ihandle* ih, const char* value, GtkTextIter* start_iter, GtkTextIter* end_iter)
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

static int gtk4TextSetSelectionAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  if (ih->data->is_multiline)
  {
    int lin_start = 1, col_start = 1, lin_end = 1, col_end = 1;
    GtkTextIter start_iter, end_iter;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));

    if (iupStrEqualNoCase(value, "ALL"))
    {
      gtk_text_buffer_get_start_iter(buffer, &start_iter);
      gtk_text_buffer_get_end_iter(buffer, &end_iter);
      gtk_text_buffer_select_range(buffer, &start_iter, &end_iter);
      return 0;
    }

    if (sscanf(value, "%d,%d:%d,%d", &lin_start, &col_start, &lin_end, &col_end) != 4)
      return 0;

    gtkTextMoveIterToLinCol(buffer, &start_iter, lin_start, col_start);
    gtkTextMoveIterToLinCol(buffer, &end_iter, lin_end, col_end);

    gtk_text_buffer_select_range(buffer, &start_iter, &end_iter);
  }
  else
  {
    int start = 1, end = 1;

    if (iupStrEqualNoCase(value, "ALL"))
    {
      gtk_editable_select_region(GTK_EDITABLE(ih->handle), 0, -1);
      return 0;
    }

    if (iupStrToIntInt(value, &start, &end, ':') != 2)
      return 0;

    if (start < 1) start = 1;
    if (end < 1) end = 1;

    gtk_editable_select_region(GTK_EDITABLE(ih->handle), start - 1, end - 1);
  }

  return 0;
}

static char* gtk4TextGetSelectionPosAttrib(Ihandle* ih)
{
  if (ih->data->is_multiline)
  {
    GtkTextIter start_iter, end_iter;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));

    if (gtk_text_buffer_get_selection_bounds(buffer, &start_iter, &end_iter))
    {
      int start = gtk_text_iter_get_offset(&start_iter);
      int end = gtk_text_iter_get_offset(&end_iter);
      return iupStrReturnIntInt(start, end, ':');
    }
  }
  else
  {
    int start = 0, end = 0;
    if (gtk_editable_get_selection_bounds(GTK_EDITABLE(ih->handle), &start, &end))
      return iupStrReturnIntInt(start, end, ':');
  }

  return NULL;
}

static int gtk4TextSetSelectionPosAttrib(Ihandle* ih, const char* value)
{
  int start = 0, end = 0;

  if (!value)
    return 0;

  if (iupStrEqualNoCase(value, "ALL"))
  {
    if (ih->data->is_multiline)
    {
      GtkTextIter start_iter, end_iter;
      GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
      gtk_text_buffer_get_start_iter(buffer, &start_iter);
      gtk_text_buffer_get_end_iter(buffer, &end_iter);
      gtk_text_buffer_select_range(buffer, &start_iter, &end_iter);
    }
    else
      gtk_editable_select_region(GTK_EDITABLE(ih->handle), 0, -1);
    return 0;
  }

  if (iupStrToIntInt(value, &start, &end, ':') != 2)
    return 0;

  if (start < 0) start = 0;
  if (end < 0) end = 0;

  if (ih->data->is_multiline)
  {
    GtkTextIter start_iter, end_iter;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
    gtk_text_buffer_get_iter_at_offset(buffer, &start_iter, start);
    gtk_text_buffer_get_iter_at_offset(buffer, &end_iter, end);
    gtk_text_buffer_select_range(buffer, &start_iter, &end_iter);
  }
  else
    gtk_editable_select_region(GTK_EDITABLE(ih->handle), start, end);

  return 0;
}

static char* gtk4TextGetSelectedTextAttrib(Ihandle* ih)
{
  if (ih->data->is_multiline)
  {
    GtkTextIter start_iter, end_iter;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));

    if (gtk_text_buffer_get_selection_bounds(buffer, &start_iter, &end_iter))
      return iupStrReturnStr(iupgtk4StrConvertFromSystem(gtk_text_buffer_get_text(buffer, &start_iter, &end_iter, TRUE)));
  }
  else
  {
    int start, end;
    if (gtk_editable_get_selection_bounds(GTK_EDITABLE(ih->handle), &start, &end))
    {
      char* selectedtext = iupStrDup(gtk_editable_get_text(GTK_EDITABLE(ih->handle)));
      selectedtext[end] = 0;
      return iupStrReturnStr(iupgtk4StrConvertFromSystem(selectedtext + start));
    }
  }
  return NULL;
}

static int gtk4TextSetSelectedTextAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  ih->data->disable_callbacks = 1;
  if (ih->data->is_multiline)
  {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
    gtk_text_buffer_delete_selection(buffer, TRUE, TRUE);
    gtk_text_buffer_insert_at_cursor(buffer, iupgtk4StrConvertToSystem(value), -1);
  }
  else
    gtk_editable_delete_selection(GTK_EDITABLE(ih->handle));

  ih->data->disable_callbacks = 0;

  return 0;
}

static char* gtk4TextGetCaretAttrib(Ihandle* ih)
{
  if (ih->data->is_multiline)
  {
    int lin, col;
    GtkTextIter iter;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
    gtk_text_buffer_get_iter_at_mark(buffer, &iter, gtk_text_buffer_get_insert(buffer));
    gtkTextGetLinColFromPosition(&iter, &lin, &col);
    return iupStrReturnIntInt(lin, col, ',');
  }
  else
  {
    int pos = gtk_editable_get_position(GTK_EDITABLE(ih->handle));
    return iupStrReturnIntInt(1, pos + 1, ',');
  }
}

static int gtk4TextSetCaretAttrib(Ihandle* ih, const char* value)
{
  int lin = 1, col = 1;
  iupStrToIntInt(value, &lin, &col, ',');
  if (lin < 1) lin = 1;
  if (col < 1) col = 1;

  if (ih->data->is_multiline)
  {
    GtkTextIter iter;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
    gtkTextMoveIterToLinCol(buffer, &iter, lin, col);
    gtk_text_buffer_place_cursor(buffer, &iter);
    gtkTextScrollToVisible(ih);
  }
  else
  {
    col--;
    gtk_editable_set_position(GTK_EDITABLE(ih->handle), col);
  }

  return 0;
}

static char* gtk4TextGetCaretPosAttrib(Ihandle* ih)
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

static int gtk4TextSetCaretPosAttrib(Ihandle* ih, const char* value)
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

static int gtk4TextSetScrollToAttrib(Ihandle* ih, const char* value)
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
    pos--;
    gtk_editable_set_position(GTK_EDITABLE(ih->handle), pos);
  }

  return 0;
}

static int gtk4TextSetScrollToPosAttrib(Ihandle* ih, const char* value)
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

static int gtk4TextSetNCAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)
    return 0;

  if (!ih->data->is_multiline)
  {
    int nc = 0;
    iupStrToInt(value, &nc);
    if (nc > 0)
      gtk_entry_set_max_length(GTK_ENTRY(ih->handle), nc);
    else
      gtk_entry_set_max_length(GTK_ENTRY(ih->handle), 0);
  }

  return 0;
}

static int gtk4TextSetClipboardAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "COPY"))
  {
    if (ih->data->is_multiline)
    {
      GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
      GdkClipboard *clipboard = gtk_widget_get_clipboard(ih->handle);
      gtk_text_buffer_copy_clipboard(buffer, clipboard);
    }
    else
    {
      GdkClipboard* clipboard = gtk_widget_get_clipboard(ih->handle);
      const char* text = gtk_editable_get_text(GTK_EDITABLE(ih->handle));
      gdk_clipboard_set_text(clipboard, text);
    }
  }
  else if (iupStrEqualNoCase(value, "CUT"))
  {
    if (ih->data->is_multiline)
    {
      GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
      GdkClipboard *clipboard = gtk_widget_get_clipboard(ih->handle);
      gtk_text_buffer_cut_clipboard(buffer, clipboard, TRUE);
    }
    else
    {
      GdkClipboard* clipboard = gtk_widget_get_clipboard(ih->handle);
      const char* text = gtk_editable_get_text(GTK_EDITABLE(ih->handle));
      gdk_clipboard_set_text(clipboard, text);
      gtk_editable_delete_selection(GTK_EDITABLE(ih->handle));
    }
  }
  else if (iupStrEqualNoCase(value, "PASTE"))
  {
    if (ih->data->is_multiline)
    {
      GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
      GdkClipboard *clipboard = gtk_widget_get_clipboard(ih->handle);
      gtk_text_buffer_paste_clipboard(buffer, clipboard, NULL, TRUE);
    }
    else
    {
      /* Paste is async in GTK4, just trigger async read */
      GdkClipboard* clipboard = gtk_widget_get_clipboard(ih->handle);
      gdk_clipboard_read_text_async(clipboard, NULL, NULL, NULL);
    }
  }
  else if (iupStrEqualNoCase(value, "CLEAR"))
  {
    if (ih->data->is_multiline)
    {
      GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
      gtk_text_buffer_delete_selection(buffer, TRUE, TRUE);
    }
    else
      gtk_editable_delete_selection(GTK_EDITABLE(ih->handle));
  }
  return 0;
}

static char* gtk4TextGetReadOnlyAttrib(Ihandle* ih)
{
  if (ih->data->is_multiline)
    return iupStrReturnBoolean(!gtk_text_view_get_editable(GTK_TEXT_VIEW(ih->handle)));
  else
    return iupStrReturnBoolean(!gtk_editable_get_editable(GTK_EDITABLE(ih->handle)));
}

static int gtk4TextSetReadOnlyAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->is_multiline)
    gtk_text_view_set_editable(GTK_TEXT_VIEW(ih->handle), !iupStrBoolean(value));
  else
    gtk_editable_set_editable(GTK_EDITABLE(ih->handle), !iupStrBoolean(value));
  return 0;
}

static int gtk4TextSetAlignmentAttrib(Ihandle* ih, const char* value)
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
  else
  {
    xalign = 0.0f;
    justification = GTK_JUSTIFY_LEFT;
  }

  if (!ih->data->is_multiline)
  {
    gtk_entry_set_alignment(GTK_ENTRY(ih->handle), xalign);
  }
  else
  {
    gtk_text_view_set_justification(GTK_TEXT_VIEW(ih->handle), justification);
  }

  return 1;
}

static int gtk4TextSetPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');

  if (ih->handle && !ih->data->is_multiline)
  {
    iupgtk4SetMargin(ih->handle, ih->data->horiz_padding, ih->data->vert_padding);
    return 0;
  }
  else
    return 1;
}

static int gtk4TextSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupgtk4SetBgColor(ih->handle, r, g, b);

  return 1;
}

static int gtk4TextSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupgtk4SetFgColor(ih->handle, r, g, b);

  return 1;
}

static char* gtk4TextGetCountAttrib(Ihandle* ih)
{
  int count;
  if (ih->data->is_multiline)
  {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
    count = gtk_text_buffer_get_char_count(buffer);
  }
  else
  {
    const char* value = gtk_editable_get_text(GTK_EDITABLE(ih->handle));
    count = (int)strlen(value);
  }
  return iupStrReturnInt(count);
}

static char* gtk4TextGetLineCountAttrib(Ihandle* ih)
{
  int count;
  if (ih->data->is_multiline)
  {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
    count = gtk_text_buffer_get_line_count(buffer);
  }
  else
    count = 1;

  return iupStrReturnInt(count);
}

static int gtk4TextSetSpinMinAttrib(Ihandle* ih, const char* value)
{
  if (GTK_IS_SPIN_BUTTON(ih->handle))
  {
    int min;
    if (iupStrToInt(value, &min))
    {
      int max = iupAttribGetInt(ih, "SPINMAX");
      gtk_spin_button_set_range((GtkSpinButton*)ih->handle, (double)min, (double)max);
    }
  }
  return 1;
}

static int gtk4TextSetSpinMaxAttrib(Ihandle* ih, const char* value)
{
  if (GTK_IS_SPIN_BUTTON(ih->handle))
  {
    int max;
    if (iupStrToInt(value, &max))
    {
      int min = iupAttribGetInt(ih, "SPINMIN");
      gtk_spin_button_set_range((GtkSpinButton*)ih->handle, (double)min, (double)max);
    }
  }
  return 1;
}

static int gtk4TextSetSpinIncAttrib(Ihandle* ih, const char* value)
{
  if (GTK_IS_SPIN_BUTTON(ih->handle))
  {
    int inc;
    if (iupStrToInt(value, &inc))
      gtk_spin_button_set_increments((GtkSpinButton*)ih->handle, (double)inc, 0);
  }
  return 1;
}

static char* gtk4TextGetSpinValueAttrib(Ihandle* ih)
{
  if (GTK_IS_SPIN_BUTTON(ih->handle))
  {
    int pos = gtk_spin_button_get_value_as_int((GtkSpinButton*)ih->handle);
    return iupStrReturnInt(pos);
  }
  return NULL;
}

static int gtk4TextSetSpinValueAttrib(Ihandle* ih, const char* value)
{
  if (GTK_IS_SPIN_BUTTON(ih->handle))
  {
    int pos;
    if (iupStrToInt(value, &pos))
      gtk_spin_button_set_value((GtkSpinButton*)ih->handle, (double)pos);
  }
  return 0;
}

static int gtk4TextGetCharSize(Ihandle* ih)
{
  int charwidth;
  PangoFontMetrics* metrics;
  PangoContext* context;
  PangoFontDescription* fontdesc = (PangoFontDescription*)iupgtk4GetPangoFontDescAttrib(ih);
  if (!fontdesc)
    return 0;

  context = gtk_widget_get_pango_context(ih->handle);
  metrics = pango_context_get_metrics(context, fontdesc, pango_context_get_language(context));
  charwidth = pango_font_metrics_get_approximate_char_width(metrics);
  pango_font_metrics_unref(metrics);
  return charwidth;
}

static int gtk4TextSetTabSizeAttrib(Ihandle* ih, const char* value)
{
  PangoTabArray *tabs;
  int tabsize, charwidth;
  if (!ih->data->is_multiline)
    return 0;

  iupStrToInt(value, &tabsize);
  charwidth = gtk4TextGetCharSize(ih);
  tabsize *= charwidth;
  tabs = pango_tab_array_new_with_positions(1, FALSE, PANGO_TAB_LEFT, tabsize);
  gtk_text_view_set_tabs(GTK_TEXT_VIEW(ih->handle), tabs);
  pango_tab_array_free(tabs);
  return 1;
}

static int gtk4TextSetCueBannerAttrib(Ihandle *ih, const char *value)
{
  if (!ih->data->is_multiline)
  {
    gtk_entry_set_placeholder_text(GTK_ENTRY(ih->handle), iupgtk4StrConvertToSystem(value));
    return 1;
  }
  return 0;
}

static int gtk4TextMapMethod(Ihandle* ih)
{
  GtkScrolledWindow* scrolled_window = NULL;

  if (ih->data->is_multiline)
  {
    GtkPolicyType hscrollbar_policy, vscrollbar_policy;
    int wordwrap = 0;
    int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");

    ih->handle = gtk_text_view_new();
    if (!ih->handle)
      return IUP_ERROR;

    scrolled_window = (GtkScrolledWindow*)gtk_scrolled_window_new();
    if (!scrolled_window)
      return IUP_ERROR;

    gtk_scrolled_window_set_child(scrolled_window, ih->handle);

    if (visiblelines > 0)
    {
      /* Mark scrolled_window so layout manager doesn't enforce GTK's scrollbar minimum */
      g_object_set_data(G_OBJECT(scrolled_window), "iup-visiblelines-set", (gpointer)"1");
    }

    iupAttribSet(ih, "_IUP_EXTRAPARENT", (char*)scrolled_window);

    ih->data->has_formatting = 1;

    if (iupAttribGetBoolean(ih, "WORDWRAP"))
    {
      wordwrap = 1;
      ih->data->sb &= ~IUP_SB_HORIZ;
    }

    if (iupAttribGetBoolean(ih, "BORDER"))
      gtk_scrolled_window_set_has_frame(scrolled_window, TRUE);
    else
      gtk_scrolled_window_set_has_frame(scrolled_window, FALSE);

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

    if (wordwrap)
      gtk_text_view_set_wrap_mode((GtkTextView*)ih->handle, GTK_WRAP_WORD);
  }
  else
  {
    if (iupAttribGetBoolean(ih, "SPIN"))
      ih->handle = gtk_spin_button_new_with_range((double)iupAttribGetInt(ih, "SPINMIN"), (double)iupAttribGetInt(ih, "SPINMAX"), (double)iupAttribGetInt(ih, "SPININC"));
    else
      ih->handle = gtk_entry_new();

    if (!ih->handle)
      return IUP_ERROR;

    ih->data->has_formatting = 0;

    /* Set natural alignment, prevent entry from expanding vertically beyond its natural height */
    gtk_widget_set_vexpand(ih->handle, FALSE);
    gtk_widget_set_valign(ih->handle, GTK_ALIGN_CENTER);

    if (GTK_IS_ENTRY(ih->handle))
      gtk_entry_set_has_frame((GtkEntry*)ih->handle, iupAttribGetBoolean(ih, "BORDER"));

    if (!iupAttribGetBoolean(ih, "BORDER"))
    {
      gtk_widget_add_css_class(ih->handle, "iup-text-noborder");
      iupgtk4CssAddStaticRule(".iup-text-noborder", "border: none;");
    }

    if (iupAttribGetBoolean(ih, "PASSWORD"))
      gtk_entry_set_visibility((GtkEntry*)ih->handle, FALSE);

    if (GTK_IS_SPIN_BUTTON(ih->handle))
    {
      gtk_spin_button_set_numeric((GtkSpinButton*)ih->handle, FALSE);
      gtk_spin_button_set_digits((GtkSpinButton*)ih->handle, 0);

      gtk_spin_button_set_wrap((GtkSpinButton*)ih->handle, iupAttribGetBoolean(ih, "SPINWRAP"));

      g_signal_connect(G_OBJECT(ih->handle), "value-changed", G_CALLBACK(gtk4TextSpinValueChanged), ih);
      g_signal_connect(G_OBJECT(ih->handle), "output", G_CALLBACK(gtk4TextSpinOutput), ih);

      if (!iupAttribGetBoolean(ih, "SPINAUTO"))
      {
        g_signal_connect(G_OBJECT(ih->handle), "input", G_CALLBACK(gtk4TextSpinInput), ih);
        iupAttribSet(ih, "_IUPGTK4_SPIN_NOAUTO", "1");
      }
    }
  }

  iupgtk4AddToParent(ih);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    iupgtk4SetCanFocus(ih->handle, 0);

  iupgtk4SetupEnterLeaveEvents(ih->handle, ih);
  iupgtk4SetupFocusEvents(ih->handle, ih);
  iupgtk4SetupKeyEvents(ih->handle, ih);

  GtkEventController* key_controller = gtk_event_controller_key_new();
  gtk_widget_add_controller(ih->handle, key_controller);
  g_signal_connect(key_controller, "key-released", G_CALLBACK(gtk4TextKeyReleaseEvent), ih);

  GtkGesture* click_gesture = gtk_gesture_click_new();
  gtk_widget_add_controller(ih->handle, GTK_EVENT_CONTROLLER(click_gesture));
  g_signal_connect(click_gesture, "released", G_CALLBACK(gtk4TextButtonEvent), ih);

  iupgtk4SetupMotionEvents(ih->handle, ih);

  if (ih->data->is_multiline)
  {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ih->handle));
    g_signal_connect(G_OBJECT(buffer), "delete-range", G_CALLBACK(gtk4TextBufferDeleteRange), ih);
    g_signal_connect(G_OBJECT(buffer), "insert-text", G_CALLBACK(gtk4TextBufferInsertText), ih);
    g_signal_connect(G_OBJECT(buffer), "changed", G_CALLBACK(gtk4TextChanged), ih);

    {
      GtkGesture* link_gesture = gtk_gesture_click_new();
      gtk_widget_add_controller(ih->handle, GTK_EVENT_CONTROLLER(link_gesture));
      g_signal_connect(link_gesture, "released", G_CALLBACK(gtk4TextLinkClick), ih);

      GtkEventController* link_motion = gtk_event_controller_motion_new();
      gtk_widget_add_controller(ih->handle, link_motion);
      g_signal_connect(link_motion, "motion", G_CALLBACK(gtk4TextLinkMotion), ih);
    }
  }
  else
  {
    /* GtkEntry uses a delegate for text editing. We need to connect signals to the delegate, not the entry itself */
    GtkEditable *editable_delegate = gtk_editable_get_delegate(GTK_EDITABLE(ih->handle));
    g_signal_connect(G_OBJECT(editable_delegate), "delete-text", G_CALLBACK(gtk4TextEntryDeleteText), ih);
    g_signal_connect(G_OBJECT(editable_delegate), "insert-text", G_CALLBACK(gtk4TextEntryInsertText), ih);
    g_signal_connect(G_OBJECT(ih->handle), "changed", G_CALLBACK(gtk4TextChanged), ih);
  }

  if (scrolled_window)
    gtk_widget_realize((GtkWidget*)scrolled_window);
  gtk_widget_realize(ih->handle);

  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSet(ih, "DROPFILESTARGET", "YES");

  iupgtk4UpdateMnemonic(ih);

  if (ih->data->formattags)
    iupTextUpdateFormatTags(ih);

  IupSetCallback(ih, "_IUP_XY2POS_CB", (Icallback)gtkTextConvertXYToPos);

  return IUP_NOERROR;
}

static void gtk4IntToRoman(int num, char* buf, int bufsize, int uppercase)
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

static int gtk4NumberingPrefix(int counter, const char* numbering, const char* style, char* buf, int bufsize)
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
    gtk4IntToRoman(counter, number, sizeof(number), 0);
  else if (iupStrEqualNoCase(numbering, "UCROMAN"))
    gtk4IntToRoman(counter, number, sizeof(number), 1);
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

static void gtk4TextApplyNumbering(GtkTextBuffer* buffer, GtkTextIter* start_iter, GtkTextIter* end_iter,
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

    if (gtk4NumberingPrefix(line - start_line + 1, numbering, style, prefix, sizeof(prefix)) > 0)
      gtk_text_buffer_insert(buffer, &line_iter, prefix, -1);
  }

  gtk_text_buffer_get_iter_at_line(buffer, start_iter, start_line);
  gtk_text_buffer_get_iter_at_line(buffer, end_iter, end_line);
  if (!gtk_text_iter_ends_line(end_iter))
    gtk_text_iter_forward_to_line_end(end_iter);
}

static void gtk4TextParseParagraphFormat(Ihandle* formattag, GtkTextTag* tag)
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
    else
      val = GTK_JUSTIFY_LEFT;

    g_object_set(G_OBJECT(tag), "justification", val, NULL);
  }

  format = iupAttribGet(formattag, "TABSARRAY");
  {
    PangoTabArray *tabs;
    int pos, i = 0;
    PangoTabAlign align;
    char* str;

    tabs = pango_tab_array_new(32, TRUE);
    while (format && *format && i < 32)
    {
      str = iupStrDupUntil((const char**)&format, ' ');
      if (!str) break;
      pos = atoi(str);
      free(str);

      str = iupStrDupUntil((const char**)&format, ' ');
      if (!str) break;

      if (iupStrEqualNoCase(str, "RIGHT"))
        align = PANGO_TAB_RIGHT;
      else if (iupStrEqualNoCase(str, "CENTER"))
        align = PANGO_TAB_CENTER;
      else if (iupStrEqualNoCase(str, "DECIMAL"))
        align = PANGO_TAB_DECIMAL;
      else
        align = PANGO_TAB_LEFT;
      free(str);

      pango_tab_array_set_tab(tabs, i, align, iupGTK4_PIXELS2PANGOUNITS(pos));
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
      PangoTabArray* tabs = pango_tab_array_new(1, TRUE);
      pango_tab_array_set_tab(tabs, 0, PANGO_TAB_LEFT, numberingtab);
      g_object_set(G_OBJECT(tag), "tabs", tabs, NULL);
      pango_tab_array_free(tabs);
    }
  }
}

static void gtk4TextParseCharacterFormat(Ihandle* formattag, GtkTextTag* tag)
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
    else
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

    val = iupGTK4_PIXELS2PANGOUNITS(val);
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
      val = iupGTK4_PIXELS2PANGOUNITS(-val);
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
      GdkRGBA color;
      iupgtk4ColorSetRGB(&color, r, g, b);
      g_object_set(G_OBJECT(tag), "foreground-rgba", &color, NULL);
    }
  }

  format = iupAttribGet(formattag, "BGCOLOR");
  if (format)
  {
    unsigned char r, g, b;
    if (iupStrToRGB(format, &r, &g, &b))
    {
      GdkRGBA color;
      iupgtk4ColorSetRGB(&color, r, g, b);
      g_object_set(G_OBJECT(tag), "background-rgba", &color, NULL);
    }
  }

  format = iupAttribGet(formattag, "UNDERLINE");
  if (format)
  {
    if (iupStrEqualNoCase(format, "SINGLE"))
      val = PANGO_UNDERLINE_SINGLE;
    else if (iupStrEqualNoCase(format, "DOUBLE"))
      val = PANGO_UNDERLINE_DOUBLE;
    else
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
    else
      val = PANGO_WEIGHT_NORMAL;

    g_object_set(G_OBJECT(tag), "weight", val, NULL);
  }

  format = iupAttribGet(formattag, "LINK");
  if (format)
  {
    g_object_set_data_full(G_OBJECT(tag), "iup-link-url", g_strdup(format), g_free);

    if (!iupAttribGet(formattag, "FGCOLOR"))
    {
      GdkRGBA color;
      iupgtk4ColorSetRGB(&color, 0, 0, 255);
      g_object_set(G_OBJECT(tag), "foreground-rgba", &color, NULL);
    }

    if (!iupAttribGet(formattag, "UNDERLINE"))
      g_object_set(G_OBJECT(tag), "underline", PANGO_UNDERLINE_SINGLE, NULL);
  }
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
    if (!gtk4TextSelectionGetIter(ih, selection, &start_iter, &end_iter))
      return;
  }
  else
  {
    char* selectionpos = iupAttribGet(formattag, "SELECTIONPOS");
    if (selectionpos)
    {
      if (!gtk4TextSelectionPosGetIter(ih, selectionpos, &start_iter, &end_iter))
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
      gtk4TextApplyNumbering(buffer, &start_iter, &end_iter, numbering, numbering_style);
    }
  }

  {
    char* image_name = iupAttribGet(formattag, "IMAGE");
    if (image_name)
    {
      GdkPaintable* paintable = (GdkPaintable*)iupImageGetImage(image_name, ih, 0, NULL);
      if (paintable)
      {
        int img_w, img_h;
        int new_w = 0, new_h = 0;
        char* attr;

        iupImageGetInfo(image_name, &img_w, &img_h, NULL);

        attr = iupAttribGet(formattag, "WIDTH");
        if (attr) iupStrToInt(attr, &new_w);
        attr = iupAttribGet(formattag, "HEIGHT");
        if (attr) iupStrToInt(attr, &new_h);

        if ((new_w > 0 && new_w != img_w) || (new_h > 0 && new_h != img_h))
        {
          GskRenderNode* node;
          GskRenderer* renderer;
          GdkTexture* scaled;

          if (new_w <= 0) new_w = img_w;
          if (new_h <= 0) new_h = img_h;

          renderer = gsk_cairo_renderer_new();
          if (gsk_renderer_realize(renderer, NULL, NULL))
          {
            node = gsk_texture_scale_node_new(GDK_TEXTURE(paintable),
                     &GRAPHENE_RECT_INIT(0, 0, new_w, new_h),
                     GSK_SCALING_FILTER_LINEAR);
            scaled = gsk_renderer_render_texture(renderer, node, NULL);
            gsk_render_node_unref(node);
            gsk_renderer_unrealize(renderer);

            paintable = GDK_PAINTABLE(scaled);

            if (!gtk_text_iter_equal(&start_iter, &end_iter))
              gtk_text_buffer_delete(buffer, &start_iter, &end_iter);

            gtk_text_buffer_insert_paintable(buffer, &start_iter, paintable);
            g_object_unref(scaled);
            g_object_unref(renderer);
            return;
          }
          g_object_unref(renderer);
        }

        if (!gtk_text_iter_equal(&start_iter, &end_iter))
          gtk_text_buffer_delete(buffer, &start_iter, &end_iter);

        gtk_text_buffer_insert_paintable(buffer, &start_iter, paintable);
      }
      return;
    }
  }

  tag = gtk_text_buffer_create_tag(buffer, NULL, NULL);
  gtk4TextParseParagraphFormat(formattag, tag);
  gtk4TextParseCharacterFormat(formattag, tag);
  gtk_text_buffer_apply_tag(buffer, tag, &start_iter, &end_iter);
}

static int gtk4TextSetOverwriteAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->is_multiline)
    return 0;
  gtk_text_view_set_overwrite(GTK_TEXT_VIEW(ih->handle), iupStrBoolean(value));
  return 0;
}

static char* gtk4TextGetOverwriteAttrib(Ihandle* ih)
{
  if (!ih->data->is_multiline)
    return NULL;
  return iupStrReturnChecked(gtk_text_view_get_overwrite(GTK_TEXT_VIEW(ih->handle)));
}

static int gtk4TextSetRemoveFormattingAttrib(Ihandle* ih, const char* value)
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

void iupdrvTextInitClass(Iclass* ic)
{
  ic->Map = gtk4TextMapMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, gtk4TextSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, gtk4TextSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "PADDING", iupTextGetPaddingAttrib, gtk4TextSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "VALUE", gtk4TextGetValueAttrib, gtk4TextSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINEVALUE", gtk4TextGetLineValueAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTEDTEXT", gtk4TextGetSelectedTextAttrib, gtk4TextSetSelectedTextAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTION", gtk4TextGetSelectionAttrib, gtk4TextSetSelectionAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTIONPOS", gtk4TextGetSelectionPosAttrib, gtk4TextSetSelectionPosAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARET", gtk4TextGetCaretAttrib, gtk4TextSetCaretAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARETPOS", gtk4TextGetCaretPosAttrib, gtk4TextSetCaretPosAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERT", NULL, gtk4TextSetInsertAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPEND", NULL, gtk4TextSetAppendAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "READONLY", gtk4TextGetReadOnlyAttrib, gtk4TextSetReadOnlyAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "NC", iupTextGetNCAttrib, gtk4TextSetNCAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "CLIPBOARD", NULL, gtk4TextSetClipboardAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTO", NULL, gtk4TextSetScrollToAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTOPOS", NULL, gtk4TextSetScrollToPosAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINMIN", NULL, gtk4TextSetSpinMinAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINMAX", NULL, gtk4TextSetSpinMaxAttrib, IUPAF_SAMEASSYSTEM, "100", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPININC", NULL, gtk4TextSetSpinIncAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINVALUE", gtk4TextGetSpinValueAttrib, gtk4TextSetSpinValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COUNT", gtk4TextGetCountAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINECOUNT", gtk4TextGetLineCountAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, gtk4TextSetAlignmentAttrib, IUPAF_SAMEASSYSTEM, "ALEFT", IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "WORDWRAP", NULL, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATTING", iupTextGetFormattingAttrib, iupTextSetFormattingAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ADDFORMATTAG", NULL, iupTextSetAddFormatTagAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ADDFORMATTAG_HANDLE", NULL, iupTextSetAddFormatTagHandleAttrib, NULL, NULL, IUPAF_IHANDLE | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OVERWRITE", gtk4TextGetOverwriteAttrib, gtk4TextSetOverwriteAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "REMOVEFORMATTING", NULL, gtk4TextSetRemoveFormattingAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPENDNEWLINE", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VISIBLECOLUMNS", NULL, NULL, IUPAF_SAMEASSYSTEM, "5", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VISIBLELINES", NULL, NULL, IUPAF_SAMEASSYSTEM, "3", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "CANFOCUS", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "TABSIZE", NULL, gtk4TextSetTabSizeAttrib, "8", NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "PASSWORD", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUEBANNER", NULL, gtk4TextSetCueBannerAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  /* Not Supported */
  iupClassRegisterAttribute(ic, "FILTER", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLVISIBLE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}
