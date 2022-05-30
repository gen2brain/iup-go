/** \file
 * \brief Valuator Control
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
#include "iup_val.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_key.h"

#include "iupgtk_drv.h"


void iupdrvValGetMinSize(Ihandle* ih, int *w, int *h)
{
  /* LAYOUT_DECORATION_ESTIMATE */
  if (ih->data->orientation == IVAL_HORIZONTAL)
  {
    *w = 20;
    *h = 35;
  }
  else
  {
    *w = 35;
    *h = 20;
  }
}

static int gtkValSetStepAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->step), 0.01))
    gtk_range_set_increments(GTK_RANGE(ih->handle), ih->data->step, ih->data->pagestep);
  return 0; /* do not store value in hash table */
}

static int gtkValSetPageStepAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->pagestep), 0.1))
    gtk_range_set_increments(GTK_RANGE(ih->handle), ih->data->step, ih->data->pagestep);
  return 0; /* do not store value in hash table */
}

static int gtkValSetValueAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDouble(value, &(ih->data->val)))
  {
    double fval;
    fval = (ih->data->val - ih->data->vmin) / (ih->data->vmax - ih->data->vmin);
    gtk_range_set_value(GTK_RANGE(ih->handle), fval);
  }
  return 0; /* do not store value in hash table */
}


/*********************************************************************************************/


static gboolean gtkValButtonReleaseEvent(GtkWidget *widget, GdkEventButton *evt, Ihandle *ih)
{
  (void)widget;
  (void)evt;
  iupAttribSet(ih, "_IUP_BUTTON_RELEASE", "1");
  return FALSE;
}

static gboolean gtkValChangeValue(GtkRange *range, GtkScrollType scroll, double fval, Ihandle *ih)
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
    if (scroll == GTK_SCROLL_JUMP) /* scroll == 1 */
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

static gboolean gtkValKeyPressEvent(GtkWidget *widget, GdkEventKey *evt, Ihandle *ih)
{
  if (iupgtkKeyPressEvent(widget, evt, ih) == TRUE)
    return TRUE;

  /* change Home and End default behaviour */
  if (ih->data->inverted)
  {
    if (evt->keyval==GDK_Home || evt->keyval==GDK_KP_Home)
    {
      /* set to maximum */
      gtk_range_set_value(GTK_RANGE(ih->handle), 1.0);
      gtkValChangeValue(GTK_RANGE(ih->handle), GTK_SCROLL_START, 1.0, ih);
      return TRUE;
    }
    if (evt->keyval==GDK_End || evt->keyval==GDK_KP_End)
    {
      /* set to minimum */
      gtk_range_set_value(GTK_RANGE(ih->handle), 0.0);
      gtkValChangeValue(GTK_RANGE(ih->handle), GTK_SCROLL_END, 0.0, ih);
      return TRUE;
    }
  }

  return FALSE;
}

/*********************************************************************************************/


static int gtkValMapMethod(Ihandle* ih)
{
  /* value, lower, upper, step_increment, page_increment, page_size */
  /* page_size value only makes a difference for scrollbar widgets  */
  GtkAdjustment* adjustment = GTK_ADJUSTMENT(gtk_adjustment_new (0, 0, 1.0, 0.01, 0.1, 0));
  if (!adjustment)
    return IUP_ERROR;

#if GTK_CHECK_VERSION(3, 0, 0)
  if (ih->data->orientation == IVAL_HORIZONTAL)
    ih->handle = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, adjustment);
  else
    ih->handle = gtk_scale_new(GTK_ORIENTATION_VERTICAL, adjustment);
#else
  if (ih->data->orientation == IVAL_HORIZONTAL)
    ih->handle = gtk_hscale_new(adjustment);
  else
    ih->handle = gtk_vscale_new(adjustment);
#endif

  if (!ih->handle)
    return IUP_ERROR;

  /* add to the parent, all GTK controls must call this. */
  iupgtkAddToParent(ih);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    iupgtkSetCanFocus(ih->handle, 0);

  g_signal_connect(G_OBJECT(ih->handle), "enter-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "leave-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "focus-in-event",     G_CALLBACK(iupgtkFocusInOutEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "focus-out-event",    G_CALLBACK(iupgtkFocusInOutEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "show-help",          G_CALLBACK(iupgtkShowHelp),        ih);

  g_signal_connect(G_OBJECT(ih->handle), "key-press-event",    G_CALLBACK(gtkValKeyPressEvent),   ih);
  g_signal_connect(G_OBJECT(ih->handle), "change-value",  G_CALLBACK(gtkValChangeValue),  ih);
  g_signal_connect(G_OBJECT(ih->handle), "button-release-event",G_CALLBACK(gtkValButtonReleaseEvent), ih);

  /* configure the scale */
  gtk_scale_set_draw_value(GTK_SCALE(ih->handle), FALSE);
  gtk_range_set_range(GTK_RANGE(ih->handle), 0.0, 1.0);

  if (ih->data->inverted)
    gtk_range_set_inverted(GTK_RANGE(ih->handle), TRUE);

  gtk_widget_realize(ih->handle);

  /* update a mnemonic in a label if necessary */
  iupgtkUpdateMnemonic(ih);

  return IUP_NOERROR;
}

void iupdrvValInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = gtkValMapMethod;

  /* Driver Dependent Attribute functions */
  
  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT); 

  /* IupVal only */
  iupClassRegisterAttribute(ic, "VALUE", iupValGetValueAttrib, gtkValSetValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESTEP", iupValGetPageStepAttrib, gtkValSetPageStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STEP", iupValGetStepAttrib, gtkValSetStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  /* NOT supported */
  iupClassRegisterAttribute(ic, "TICKSPOS", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "SHOWTICKS", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED);
}
