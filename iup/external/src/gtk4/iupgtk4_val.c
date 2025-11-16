/** \file
 * \brief Valuator Control for GTK4
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
#include "iup_val.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_key.h"

#include "iupgtk4_drv.h"


void iupdrvValGetMinSize(Ihandle* ih, int *w, int *h)
{
  static int horiz_min_w = -1, horiz_min_h = -1;
  static int vert_min_w = -1, vert_min_h = -1;

  /* Measure the minimum sizes required by GtkScale */
  if (horiz_min_w < 0)
  {
    GtkAdjustment* adj = GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, 1.0, 0.01, 0.1, 0));
    GtkWidget* temp_horiz = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, adj);
    GtkWidget* temp_vert = gtk_scale_new(GTK_ORIENTATION_VERTICAL, adj);

    gtk_widget_measure(temp_horiz, GTK_ORIENTATION_HORIZONTAL, -1, &horiz_min_w, NULL, NULL, NULL);
    gtk_widget_measure(temp_horiz, GTK_ORIENTATION_VERTICAL, -1, &horiz_min_h, NULL, NULL, NULL);
    gtk_widget_measure(temp_vert, GTK_ORIENTATION_HORIZONTAL, -1, &vert_min_w, NULL, NULL, NULL);
    gtk_widget_measure(temp_vert, GTK_ORIENTATION_VERTICAL, -1, &vert_min_h, NULL, NULL, NULL);

    g_object_ref_sink(temp_horiz);
    g_object_unref(temp_horiz);
    g_object_ref_sink(temp_vert);
    g_object_unref(temp_vert);
  }

  if (ih->data->orientation == IVAL_HORIZONTAL)
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

static int gtk4ValSetStepAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->step), 0.01))
    gtk_range_set_increments(GTK_RANGE(ih->handle), ih->data->step, ih->data->pagestep);
  return 0;
}

static int gtk4ValSetPageStepAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->pagestep), 0.1))
    gtk_range_set_increments(GTK_RANGE(ih->handle), ih->data->step, ih->data->pagestep);
  return 0;
}

static int gtk4ValSetValueAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDouble(value, &(ih->data->val)))
  {
    double fval;
    fval = (ih->data->val - ih->data->vmin) / (ih->data->vmax - ih->data->vmin);
    gtk_range_set_value(GTK_RANGE(ih->handle), fval);
  }
  return 0;
}

static void gtk4ValButtonReleased(GtkGestureClick *gesture, int n_press, double x, double y, Ihandle *ih)
{
  iupAttribSet(ih, "_IUP_BUTTON_RELEASE", "1");
  (void)gesture;
  (void)n_press;
  (void)x;
  (void)y;
}

static gboolean gtk4ValChangeValue(GtkRange *range, GtkScrollType scroll, double fval, Ihandle *ih)
{
  double old_val = ih->data->val;
  IFn cb;

  if (fval < 0.0)
    gtk_range_set_value(GTK_RANGE(ih->handle), 0.0);
  if (fval > 1.0)
    gtk_range_set_value(GTK_RANGE(ih->handle), 1.0);

  ih->data->val = fval*(ih->data->vmax - ih->data->vmin) + ih->data->vmin;
  iupValCropValue(ih);

  cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
  if (cb)
  {
    if (ih->data->val == old_val)
      return FALSE;

    cb(ih);
  }
  else
  {
    IFnd cb_old;
    if (scroll == GTK_SCROLL_JUMP)
    {
      if (iupAttribGet(ih, "_IUP_BUTTON_RELEASE"))
      {
        cb_old = (IFnd)IupGetCallback(ih, "BUTTON_RELEASE_CB");
        iupAttribSet(ih, "_IUP_BUTTON_RELEASE", NULL);
      }
      else
        cb_old = (IFnd)IupGetCallback(ih, "MOUSEMOVE_CB");
    }
    else if((scroll >= GTK_SCROLL_STEP_BACKWARD) && (scroll <= GTK_SCROLL_END))
      cb_old = (IFnd)IupGetCallback(ih, "BUTTON_PRESS_CB");
    else
      cb_old = (IFnd)IupGetCallback(ih, "BUTTON_RELEASE_CB");

    if (cb_old)
      cb_old(ih, ih->data->val);
  }

  if (fval < 0.0 || fval > 1.0)
    return TRUE;

  (void)range;
  return FALSE;
}

static gboolean gtk4ValKeyPressEvent(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, Ihandle *ih)
{
  (void)controller;
  (void)keycode;
  (void)state;

  if (ih->data->inverted)
  {
    if (keyval==GDK_KEY_Home || keyval==GDK_KEY_KP_Home)
    {
      gtk_range_set_value(GTK_RANGE(ih->handle), 1.0);
      gtk4ValChangeValue(GTK_RANGE(ih->handle), GTK_SCROLL_START, 1.0, ih);
      return TRUE;
    }
    if (keyval==GDK_KEY_End || keyval==GDK_KEY_KP_End)
    {
      gtk_range_set_value(GTK_RANGE(ih->handle), 0.0);
      gtk4ValChangeValue(GTK_RANGE(ih->handle), GTK_SCROLL_END, 0.0, ih);
      return TRUE;
    }
  }

  return FALSE;
}

static int gtk4ValMapMethod(Ihandle* ih)
{
  GtkAdjustment* adjustment = GTK_ADJUSTMENT(gtk_adjustment_new (0, 0, 1.0, 0.01, 0.1, 0));
  if (!adjustment)
    return IUP_ERROR;

  if (ih->data->orientation == IVAL_HORIZONTAL)
    ih->handle = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, adjustment);
  else
    ih->handle = gtk_scale_new(GTK_ORIENTATION_VERTICAL, adjustment);

  if (!ih->handle)
    return IUP_ERROR;

  iupgtk4AddToParent(ih);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    iupgtk4SetCanFocus(ih->handle, 0);

  iupgtk4SetupEnterLeaveEvents(ih->handle, ih);
  iupgtk4SetupFocusEvents(ih->handle, ih);

  GtkEventController* key_controller = gtk_event_controller_key_new();
  gtk_widget_add_controller(ih->handle, key_controller);
  g_signal_connect(key_controller, "key-pressed", G_CALLBACK(gtk4ValKeyPressEvent), ih);

  iupgtk4SetupKeyEvents(ih->handle, ih);

  g_signal_connect(G_OBJECT(ih->handle), "change-value",  G_CALLBACK(gtk4ValChangeValue),  ih);

  GtkGesture* click_gesture = gtk_gesture_click_new();
  gtk_widget_add_controller(ih->handle, GTK_EVENT_CONTROLLER(click_gesture));
  g_signal_connect(click_gesture, "released", G_CALLBACK(gtk4ValButtonReleased), ih);

  gtk_scale_set_draw_value(GTK_SCALE(ih->handle), FALSE);
  gtk_range_set_range(GTK_RANGE(ih->handle), 0.0, 1.0);

  if (ih->data->inverted)
    gtk_range_set_inverted(GTK_RANGE(ih->handle), TRUE);

  gtk_widget_realize(ih->handle);

  iupgtk4UpdateMnemonic(ih);

  return IUP_NOERROR;
}

void iupdrvValInitClass(Iclass* ic)
{
  ic->Map = gtk4ValMapMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "VALUE", iupValGetValueAttrib, gtk4ValSetValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESTEP", iupValGetPageStepAttrib, gtk4ValSetPageStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STEP", iupValGetStepAttrib, gtk4ValSetStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "TICKSPOS", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED);
  iupClassRegisterAttribute(ic, "SHOWTICKS", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED);
}
