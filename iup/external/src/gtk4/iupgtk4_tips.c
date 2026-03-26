/** \file
 * \brief GTK4 Driver TIPS management
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>

#include <gtk/gtk.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_str.h"
#include "iup_attrib.h"
#include "iup_image.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_drvfont.h"

#include "iupgtk4_drv.h"


static void gtk4TooltipSetTitle(Ihandle* ih, GtkWidget* widget, const char* value)
{
  if (iupAttribGetBoolean(ih, "TIPMARKUP"))
    gtk_widget_set_tooltip_markup(widget, iupgtk4StrConvertToSystem(value));
  else
    gtk_widget_set_tooltip_text(widget, iupgtk4StrConvertToSystem(value));
}

static gboolean gtk4QueryTooltip(GtkWidget *widget, gint _x, gint _y, gboolean keyboard_mode, GtkTooltip *tooltip, Ihandle* ih)
{
  char* value;

  IFnii cb = (IFnii)IupGetCallback(ih, "TIPS_CB");
  if (cb)
  {
    int x, y;
    iupdrvGetCursorPos(&x, &y);
    iupdrvScreenToClient(ih, &x, &y);
    cb(ih, x, y);
  }

  value = iupAttribGet(ih, "TIPRECT");
  if (value && !keyboard_mode)
  {
    GdkRectangle rect;
    int x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    if (sscanf(value, "%d %d %d %d", &x1, &y1, &x2, &y2) != 4)
      return FALSE;
    rect.x = x1;
    rect.y = y1;
    rect.width = x2-x1+1;
    rect.height = y2-y1+1;
    gtk_tooltip_set_tip_area(tooltip, &rect);
  }
  else
    gtk_tooltip_set_tip_area(tooltip, NULL);

  value = iupAttribGet(ih, "TIPICON");
  if (!value)
    gtk_tooltip_set_icon(tooltip, NULL);
  else
  {
    GdkPaintable* icon = (GdkPaintable*)iupImageGetIcon(value);
    if (icon)
      gtk_tooltip_set_icon(tooltip, icon);
  }

  value = iupAttribGet(ih, "TIP");
  {
    const char* tipfont = iupAttribGet(ih, "TIPFONT");
    const char* tipbgcolor = iupAttribGet(ih, "TIPBGCOLOR");
    const char* tipfgcolor = iupAttribGet(ih, "TIPFGCOLOR");

    if (tipfont || tipbgcolor || tipfgcolor)
    {
      GtkWidget* label = gtk_label_new(NULL);
      PangoAttrList* attrs = pango_attr_list_new();

      if (iupAttribGetBoolean(ih, "TIPMARKUP"))
        gtk_label_set_markup(GTK_LABEL(label), iupgtk4StrConvertToSystem(value));
      else
        gtk_label_set_text(GTK_LABEL(label), iupgtk4StrConvertToSystem(value));

      if (tipfont && !iupStrEqualNoCase(tipfont, "SYSTEM"))
      {
        PangoFontDescription* fontdesc = iupgtk4GetPangoFontDesc(tipfont);
        if (fontdesc)
          pango_attr_list_insert(attrs, pango_attr_font_desc_new(fontdesc));
      }

      if (tipfgcolor)
      {
        unsigned char r, g, b;
        if (iupStrToRGB(tipfgcolor, &r, &g, &b))
          pango_attr_list_insert(attrs, pango_attr_foreground_new(r * 257, g * 257, b * 257));
      }

      if (tipbgcolor)
      {
        unsigned char r, g, b;
        if (iupStrToRGB(tipbgcolor, &r, &g, &b))
          pango_attr_list_insert(attrs, pango_attr_background_new(r * 257, g * 257, b * 257));
      }

      gtk_label_set_attributes(GTK_LABEL(label), attrs);
      pango_attr_list_unref(attrs);

      gtk_tooltip_set_custom(tooltip, label);

      (void)_y;
      (void)_x;
      (void)widget;
      return TRUE;
    }
    else
    {
      /* Set tooltip text on the GtkTooltip object, not the widget.
         Setting it on the widget from within query-tooltip causes infinite recursion */
      gtk_tooltip_set_custom(tooltip, NULL);
      if (value)
      {
        if (iupAttribGetBoolean(ih, "TIPMARKUP"))
          gtk_tooltip_set_markup(tooltip, iupgtk4StrConvertToSystem(value));
        else
          gtk_tooltip_set_text(tooltip, iupgtk4StrConvertToSystem(value));
      }
    }
  }

  (void)_y;
  (void)_x;
  (void)widget;
  return FALSE;
}

static GtkWidget* gtk4TipGetWidget(Ihandle* ih)
{
  GtkWidget* widget = (GtkWidget*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (!widget)
    widget = ih->handle;
  return widget;
}

int iupdrvBaseSetTipAttrib(Ihandle* ih, const char* value)
{
  GtkWidget* widget = gtk4TipGetWidget(ih);

  if (!iupAttribGet(ih, "_IUPGTK4_TIP_CONNECTED"))
  {
    g_signal_connect(widget, "query-tooltip", G_CALLBACK(gtk4QueryTooltip), ih);
    iupAttribSet(ih, "_IUPGTK4_TIP_CONNECTED", "1");
  }

  gtk4TooltipSetTitle(ih, widget, value);

  return 1;
}

int iupdrvBaseSetTipVisibleAttrib(Ihandle* ih, const char* value)
{
  GtkWidget* widget = gtk4TipGetWidget(ih);

  if (!gtk_widget_get_has_tooltip(widget))
    return 0;

  if (iupStrBoolean(value))
    gtk_widget_trigger_tooltip_query(widget);
  else
  {
    gtk_widget_set_has_tooltip(widget, FALSE);
    gtk_widget_set_has_tooltip(widget, TRUE);
  }

  return 0;
}

char* iupdrvBaseGetTipVisibleAttrib(Ihandle* ih)
{
  /* Cannot determine if tooltip is currently visible.
     gtk_widget_get_tooltip_window was removed and tooltip internals are hidden. */
  (void)ih;
  return NULL;
}
