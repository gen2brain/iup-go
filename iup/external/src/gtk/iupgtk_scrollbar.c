/** \file
 * \brief Scrollbar Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#if GTK_CHECK_VERSION(3, 0, 0)
#include <gdk/gdkkeysyms-compat.h>
#endif

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
#include "iup_scrollbar.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_key.h"

#include "iupgtk_drv.h"


void iupdrvScrollbarGetMinSize(Ihandle* ih, int *w, int *h)
{
#if GTK_CHECK_VERSION(3, 0, 0)
  static int horiz_min_w = -1, horiz_min_h = -1;
  static int vert_min_w = -1, vert_min_h = -1;

  if (horiz_min_w < 0)
  {
    GtkWidget* temp_window = gtk_offscreen_window_new();
    GtkAdjustment* adj = GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, 1.0, 0.01, 0.1, 0.1));
    GtkWidget* temp_horiz = gtk_scrollbar_new(GTK_ORIENTATION_HORIZONTAL, adj);
    GtkWidget* temp_vert = gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL, adj);
    GtkRequisition horiz_req, vert_req;

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(box), temp_horiz, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), temp_vert, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(temp_window), box);

    gtk_widget_show_all(temp_window);
    gtk_widget_realize(temp_window);

    gtk_widget_get_preferred_size(temp_horiz, NULL, &horiz_req);
    gtk_widget_get_preferred_size(temp_vert, NULL, &vert_req);

    horiz_min_w = horiz_req.width;
    horiz_min_h = horiz_req.height;
    vert_min_w = vert_req.width;
    vert_min_h = vert_req.height;

    if (horiz_min_w < 20) horiz_min_w = 20;
    if (horiz_min_h < 15) horiz_min_h = 15;
    if (vert_min_w < 15) vert_min_w = 15;
    if (vert_min_h < 20) vert_min_h = 20;

    gtk_widget_destroy(temp_window);
  }

  if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
  {
    *w = horiz_min_w;
    *h = horiz_min_h;
  }
  else
  {
    *w = vert_min_w;
    *h = vert_min_h;
  }
#else
  if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
  {
    *w = 20;
    *h = 15;
  }
  else
  {
    *w = 15;
    *h = 20;
  }
#endif
}

static void gtkScrollbarUpdateAdjustment(Ihandle* ih)
{
  GtkAdjustment* adjustment;
  double range = ih->data->vmax - ih->data->vmin;

  adjustment = gtk_range_get_adjustment(GTK_RANGE(ih->handle));

  gtk_adjustment_configure(adjustment,
    (ih->data->val - ih->data->vmin) / range,                /* value */
    0.0,                                                       /* lower */
    1.0,                                                       /* upper */
    ih->data->linestep / range,                                /* step_increment */
    ih->data->pagestep / range,                                /* page_increment */
    ih->data->pagesize / range);                               /* page_size */
}

static int gtkScrollbarSetLineStepAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->linestep), 0.01))
    gtkScrollbarUpdateAdjustment(ih);
  return 0;
}

static int gtkScrollbarSetPageStepAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->pagestep), 0.1))
    gtkScrollbarUpdateAdjustment(ih);
  return 0;
}

static int gtkScrollbarSetPageSizeAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->pagesize), 0.1))
  {
    iupScrollbarCropValue(ih);
    gtkScrollbarUpdateAdjustment(ih);
  }
  return 0;
}

static int gtkScrollbarSetValueAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDouble(value, &(ih->data->val)))
  {
    double range = ih->data->vmax - ih->data->vmin;
    double fval;

    iupScrollbarCropValue(ih);

    fval = (ih->data->val - ih->data->vmin) / range;
    gtk_range_set_value(GTK_RANGE(ih->handle), fval);
  }
  return 0;
}


/*********************************************************************************************/


static gboolean gtkScrollbarChangeValue(GtkRange *range, GtkScrollType scroll, double fval, Ihandle *ih)
{
  double old_val = ih->data->val;
  double adj_page_size, range_val;
  IFn valuechanged_cb;
  IFniff scroll_cb;
  int op = -1;

  adj_page_size = ih->data->pagesize / (ih->data->vmax - ih->data->vmin);

  if (fval < 0.0)
    fval = 0.0;
  if (fval > 1.0 - adj_page_size)
    fval = 1.0 - adj_page_size;

  ih->data->val = fval * (ih->data->vmax - ih->data->vmin) + ih->data->vmin;
  iupScrollbarCropValue(ih);

  range_val = (ih->data->val - ih->data->vmin) / (ih->data->vmax - ih->data->vmin);
  gtk_range_set_value(GTK_RANGE(ih->handle), range_val);

  if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
  {
    switch (scroll)
    {
    case GTK_SCROLL_STEP_BACKWARD: op = IUP_SBLEFT; break;
    case GTK_SCROLL_STEP_FORWARD:  op = IUP_SBRIGHT; break;
    case GTK_SCROLL_PAGE_BACKWARD: op = IUP_SBPGLEFT; break;
    case GTK_SCROLL_PAGE_FORWARD:  op = IUP_SBPGRIGHT; break;
    case GTK_SCROLL_JUMP:          op = IUP_SBDRAGH; break;
    default:                       op = IUP_SBPOSH; break;
    }
  }
  else
  {
    switch (scroll)
    {
    case GTK_SCROLL_STEP_BACKWARD: op = IUP_SBUP; break;
    case GTK_SCROLL_STEP_FORWARD:  op = IUP_SBDN; break;
    case GTK_SCROLL_PAGE_BACKWARD: op = IUP_SBPGUP; break;
    case GTK_SCROLL_PAGE_FORWARD:  op = IUP_SBPGDN; break;
    case GTK_SCROLL_JUMP:          op = IUP_SBDRAGV; break;
    default:                       op = IUP_SBPOSV; break;
    }
  }

  scroll_cb = (IFniff)IupGetCallback(ih, "SCROLL_CB");
  if (scroll_cb)
  {
    float posx = 0, posy = 0;
    if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
      posx = (float)ih->data->val;
    else
      posy = (float)ih->data->val;

    scroll_cb(ih, op, posx, posy);
  }

  valuechanged_cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
  if (valuechanged_cb)
  {
    if (ih->data->val != old_val)
      valuechanged_cb(ih);
  }

  (void)range;
  return TRUE;
}

static gboolean gtkScrollbarKeyPressEvent(GtkWidget *widget, GdkEventKey *evt, Ihandle *ih)
{
  if (iupgtkKeyPressEvent(widget, evt, ih) == TRUE)
    return TRUE;

  if (ih->data->inverted)
  {
    if (evt->keyval == GDK_Home || evt->keyval == GDK_KP_Home)
    {
      double max_pos = (ih->data->vmax - ih->data->pagesize - ih->data->vmin) / (ih->data->vmax - ih->data->vmin);
      gtk_range_set_value(GTK_RANGE(ih->handle), max_pos);
      gtkScrollbarChangeValue(GTK_RANGE(ih->handle), GTK_SCROLL_START, max_pos, ih);
      return TRUE;
    }
    if (evt->keyval == GDK_End || evt->keyval == GDK_KP_End)
    {
      gtk_range_set_value(GTK_RANGE(ih->handle), 0.0);
      gtkScrollbarChangeValue(GTK_RANGE(ih->handle), GTK_SCROLL_END, 0.0, ih);
      return TRUE;
    }
  }

  return FALSE;
}


/*********************************************************************************************/


static int gtkScrollbarMapMethod(Ihandle* ih)
{
  double range = ih->data->vmax - ih->data->vmin;
  GtkAdjustment* adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
    0,                                   /* value */
    0,                                   /* lower */
    1.0,                                 /* upper */
    ih->data->linestep / range,          /* step_increment */
    ih->data->pagestep / range,          /* page_increment */
    ih->data->pagesize / range));        /* page_size */

  if (!adjustment)
    return IUP_ERROR;

#if GTK_CHECK_VERSION(3, 0, 0)
  if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
    ih->handle = gtk_scrollbar_new(GTK_ORIENTATION_HORIZONTAL, adjustment);
  else
    ih->handle = gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL, adjustment);
#else
  if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
    ih->handle = gtk_hscrollbar_new(adjustment);
  else
    ih->handle = gtk_vscrollbar_new(adjustment);
#endif

  if (!ih->handle)
    return IUP_ERROR;

  iupgtkAddToParent(ih);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    iupgtkSetCanFocus(ih->handle, 0);

  g_signal_connect(G_OBJECT(ih->handle), "enter-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "leave-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "focus-in-event",     G_CALLBACK(iupgtkFocusInOutEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "focus-out-event",    G_CALLBACK(iupgtkFocusInOutEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "show-help",          G_CALLBACK(iupgtkShowHelp),        ih);

  g_signal_connect(G_OBJECT(ih->handle), "key-press-event",    G_CALLBACK(gtkScrollbarKeyPressEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "change-value",       G_CALLBACK(gtkScrollbarChangeValue), ih);

  if (ih->data->inverted)
    gtk_range_set_inverted(GTK_RANGE(ih->handle), TRUE);

  gtk_widget_realize(ih->handle);

  iupgtkUpdateMnemonic(ih);

  return IUP_NOERROR;
}

void iupdrvScrollbarInitClass(Iclass* ic)
{
  ic->Map = gtkScrollbarMapMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "VALUE", iupScrollbarGetValueAttrib, gtkScrollbarSetValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINESTEP", iupScrollbarGetLineStepAttrib, gtkScrollbarSetLineStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESTEP", iupScrollbarGetPageStepAttrib, gtkScrollbarSetPageStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESIZE", iupScrollbarGetPageSizeAttrib, gtkScrollbarSetPageSizeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
}
