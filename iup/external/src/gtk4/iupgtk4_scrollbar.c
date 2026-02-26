/** \file
 * \brief Scrollbar Control for GTK4
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
#include "iup_scrollbar.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_key.h"

#include "iupgtk4_drv.h"


void iupdrvScrollbarGetMinSize(Ihandle* ih, int *w, int *h)
{
  static int horiz_min_w = -1, horiz_min_h = -1;
  static int vert_min_w = -1, vert_min_h = -1;

  if (horiz_min_w < 0)
  {
    GtkAdjustment* adj = GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, 1.0, 0.01, 0.1, 0.1));
    GtkWidget* temp_horiz = gtk_scrollbar_new(GTK_ORIENTATION_HORIZONTAL, adj);
    GtkWidget* temp_vert = gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL, adj);

    gtk_widget_measure(temp_horiz, GTK_ORIENTATION_HORIZONTAL, -1, &horiz_min_w, NULL, NULL, NULL);
    gtk_widget_measure(temp_horiz, GTK_ORIENTATION_VERTICAL, -1, &horiz_min_h, NULL, NULL, NULL);
    gtk_widget_measure(temp_vert, GTK_ORIENTATION_HORIZONTAL, -1, &vert_min_w, NULL, NULL, NULL);
    gtk_widget_measure(temp_vert, GTK_ORIENTATION_VERTICAL, -1, &vert_min_h, NULL, NULL, NULL);

    g_object_ref_sink(temp_horiz);
    g_object_unref(temp_horiz);
    g_object_ref_sink(temp_vert);
    g_object_unref(temp_vert);

    if (horiz_min_w < 20) horiz_min_w = 20;
    if (horiz_min_h < 15) horiz_min_h = 15;
    if (vert_min_w < 15) vert_min_w = 15;
    if (vert_min_h < 20) vert_min_h = 20;
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
}

static void gtk4ScrollbarUpdateAdjustment(Ihandle* ih)
{
  GtkAdjustment* adjustment;
  double range = ih->data->vmax - ih->data->vmin;

  adjustment = gtk_scrollbar_get_adjustment(GTK_SCROLLBAR(ih->handle));

  gtk_adjustment_configure(adjustment,
    (ih->data->val - ih->data->vmin) / range,
    0.0,
    1.0,
    ih->data->linestep / range,
    ih->data->pagestep / range,
    ih->data->pagesize / range);
}

static int gtk4ScrollbarSetLineStepAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->linestep), 0.01))
    gtk4ScrollbarUpdateAdjustment(ih);
  return 0;
}

static int gtk4ScrollbarSetPageStepAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->pagestep), 0.1))
    gtk4ScrollbarUpdateAdjustment(ih);
  return 0;
}

static int gtk4ScrollbarSetPageSizeAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->pagesize), 0.1))
  {
    iupScrollbarCropValue(ih);
    gtk4ScrollbarUpdateAdjustment(ih);
  }
  return 0;
}

static int gtk4ScrollbarSetValueAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDouble(value, &(ih->data->val)))
  {
    GtkAdjustment* adjustment;
    double range = ih->data->vmax - ih->data->vmin;
    double fval;

    iupScrollbarCropValue(ih);

    fval = (ih->data->val - ih->data->vmin) / range;
    adjustment = gtk_scrollbar_get_adjustment(GTK_SCROLLBAR(ih->handle));
    gtk_adjustment_set_value(adjustment, fval);
  }
  return 0;
}


/*********************************************************************************************/


static void gtk4ScrollbarValueChanged(GtkAdjustment *adjustment, Ihandle *ih)
{
  double old_val = ih->data->val;
  double fval;
  IFn valuechanged_cb;

  fval = gtk_adjustment_get_value(adjustment);

  ih->data->val = fval * (ih->data->vmax - ih->data->vmin) + ih->data->vmin;
  iupScrollbarCropValue(ih);

  valuechanged_cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
  if (valuechanged_cb)
  {
    if (ih->data->val != old_val)
      valuechanged_cb(ih);
  }

  /* GTK4 scrollbar doesn't provide scroll type info through adjustment value-changed,
     so SCROLL_CB gets a generic position operation */
  {
    IFniff scroll_cb = (IFniff)IupGetCallback(ih, "SCROLL_CB");
    if (scroll_cb && ih->data->val != old_val)
    {
      float posx = 0, posy = 0;
      int op;

      if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
      {
        posx = (float)ih->data->val;
        op = IUP_SBPOSH;
      }
      else
      {
        posy = (float)ih->data->val;
        op = IUP_SBPOSV;
      }

      scroll_cb(ih, op, posx, posy);
    }
  }
}

static gboolean gtk4ScrollbarKeyPressEvent(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, Ihandle *ih)
{
  (void)controller;
  (void)keycode;
  (void)state;

  if (ih->data->inverted)
  {
    if (keyval == GDK_KEY_Home || keyval == GDK_KEY_KP_Home)
    {
      GtkAdjustment* adjustment = gtk_scrollbar_get_adjustment(GTK_SCROLLBAR(ih->handle));
      double max_pos = 1.0 - gtk_adjustment_get_page_size(adjustment);
      gtk_adjustment_set_value(adjustment, max_pos);
      return TRUE;
    }
    if (keyval == GDK_KEY_End || keyval == GDK_KEY_KP_End)
    {
      GtkAdjustment* adjustment = gtk_scrollbar_get_adjustment(GTK_SCROLLBAR(ih->handle));
      gtk_adjustment_set_value(adjustment, 0.0);
      return TRUE;
    }
  }

  return FALSE;
}


/*********************************************************************************************/


static int gtk4ScrollbarMapMethod(Ihandle* ih)
{
  GtkAdjustment* adjustment;
  double range = ih->data->vmax - ih->data->vmin;

  adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(
    0,
    0,
    1.0,
    ih->data->linestep / range,
    ih->data->pagestep / range,
    ih->data->pagesize / range));

  if (!adjustment)
    return IUP_ERROR;

  if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
    ih->handle = gtk_scrollbar_new(GTK_ORIENTATION_HORIZONTAL, adjustment);
  else
    ih->handle = gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL, adjustment);

  if (!ih->handle)
    return IUP_ERROR;

  iupgtk4AddToParent(ih);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    iupgtk4SetCanFocus(ih->handle, 0);

  iupgtk4SetupEnterLeaveEvents(ih->handle, ih);
  iupgtk4SetupFocusEvents(ih->handle, ih);

  GtkEventController* key_controller = gtk_event_controller_key_new();
  gtk_widget_add_controller(ih->handle, key_controller);
  g_signal_connect(key_controller, "key-pressed", G_CALLBACK(gtk4ScrollbarKeyPressEvent), ih);

  iupgtk4SetupKeyEvents(ih->handle, ih);

  g_signal_connect(G_OBJECT(adjustment), "value-changed", G_CALLBACK(gtk4ScrollbarValueChanged), ih);

  if (ih->data->inverted)
    gtk_range_set_inverted(GTK_RANGE(ih->handle), TRUE);

  gtk_widget_realize(ih->handle);

  iupgtk4UpdateMnemonic(ih);

  return IUP_NOERROR;
}

void iupdrvScrollbarInitClass(Iclass* ic)
{
  ic->Map = gtk4ScrollbarMapMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "VALUE", iupScrollbarGetValueAttrib, gtk4ScrollbarSetValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINESTEP", iupScrollbarGetLineStepAttrib, gtk4ScrollbarSetLineStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESTEP", iupScrollbarGetPageStepAttrib, gtk4ScrollbarSetPageStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESIZE", iupScrollbarGetPageSizeAttrib, gtk4ScrollbarSetPageSizeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
}
