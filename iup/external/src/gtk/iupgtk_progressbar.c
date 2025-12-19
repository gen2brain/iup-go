/** \file
* \brief Progress bar Control
*
* See Copyright Notice in "iup.h"
*/

#undef GTK_DISABLE_DEPRECATED /* gtk_progress_bar_set_bar_style and gtk_progress_set_activity_mode are deprecated */
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
#include "iup_progressbar.h"
#include "iup_drv.h"

#include "iupgtk_drv.h"


void iupdrvProgressBarGetMinSize(Ihandle* ih, int* w, int* h)
{
#if GTK_CHECK_VERSION(3, 0, 0)
  static int horiz_min_w = -1, horiz_min_h = -1;
  static int vert_min_w = -1, vert_min_h = -1;

  if (horiz_min_w < 0)
  {
    GtkWidget* temp_window = gtk_offscreen_window_new();
    GtkWidget* temp_horiz = gtk_progress_bar_new();
    GtkWidget* temp_vert = gtk_progress_bar_new();
    GtkRequisition horiz_req, vert_req;

    gtk_orientable_set_orientation(GTK_ORIENTABLE(temp_horiz), GTK_ORIENTATION_HORIZONTAL);
    gtk_orientable_set_orientation(GTK_ORIENTABLE(temp_vert), GTK_ORIENTATION_VERTICAL);

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

    if (horiz_min_w < 1) horiz_min_w = 150;
    if (horiz_min_h < 1) horiz_min_h = 6;
    if (vert_min_w < 1) vert_min_w = 7;
    if (vert_min_h < 1) vert_min_h = 80;

    gtk_widget_destroy(temp_window);
  }

  if (iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "VERTICAL"))
  {
    *w = vert_min_w;
    *h = vert_min_h;
  }
  else
  {
    *w = horiz_min_w;
    *h = horiz_min_h;
  }
#else
  /* GTK2 fallback */
  if (iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "VERTICAL"))
  {
    *w = 7;
    *h = 80;
  }
  else
  {
    *w = 150;
    *h = 6;
  }
#endif
}

static int gtkProgressBarTimeCb(Ihandle* timer)
{
  Ihandle* ih = (Ihandle*)iupAttribGet(timer, "_IUP_PROGRESSBAR");
  gtk_progress_bar_pulse((GtkProgressBar*)ih->handle);
  return IUP_DEFAULT;
}

static int gtkProgressBarSetMarqueeAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->marquee)
    return 0;

  if (iupStrBoolean(value))
    IupSetAttribute(ih->data->timer, "RUN", "YES");
  else
    IupSetAttribute(ih->data->timer, "RUN", "NO");

  return 1;
}

static int gtkProgressBarSetValueAttrib(Ihandle* ih, const char* value)
{
  GtkProgressBar* pbar = (GtkProgressBar*)ih->handle;

  if (ih->data->marquee)
    return 0;

  if (!value)
    ih->data->value = 0;
  else
    iupStrToDouble(value, &(ih->data->value));

  iProgressBarCropValue(ih);

  gtk_progress_bar_set_fraction(pbar, (ih->data->value - ih->data->vmin) / (ih->data->vmax - ih->data->vmin));

  return 0;
}

#if !GTK_CHECK_VERSION(3, 0, 0)
static int gtkProgressBarSetDashedAttrib(Ihandle* ih, const char* value)
{
  GtkProgressBar* pbar = (GtkProgressBar*)ih->handle;

  if (ih->data->marquee)
    return 0;

  /* gtk_progress_bar_set_bar_style is deprecated, but we still use it in GTK2 */
  if (iupStrBoolean(value))
  {
    ih->data->dashed = 1;
    gtk_progress_bar_set_bar_style(pbar, GTK_PROGRESS_DISCRETE);
  }
  else /* Default */
  {
    ih->data->dashed = 0;
    gtk_progress_bar_set_bar_style(pbar, GTK_PROGRESS_CONTINUOUS);
  }

  return 0;
}
#endif

static int gtkProgressBarMapMethod(Ihandle* ih)
{
  ih->handle = gtk_progress_bar_new();
  if (!ih->handle)
    return IUP_ERROR;

  /* add to the parent, all GTK controls must call this. */
  iupgtkAddToParent(ih);

  gtk_widget_realize(ih->handle);

  if (iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "VERTICAL"))
  {
#if GTK_CHECK_VERSION(3, 0, 0)
    gtk_orientable_set_orientation(GTK_ORIENTABLE(ih->handle), GTK_ORIENTATION_VERTICAL);
    gtk_progress_bar_set_inverted(GTK_PROGRESS_BAR(ih->handle), TRUE);
#else
    gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(ih->handle), GTK_PROGRESS_BOTTOM_TO_TOP);
#endif

    if (ih->userheight < ih->userwidth)
    {
      int tmp = ih->userheight;
      ih->userheight = ih->userwidth;
      ih->userwidth = tmp;
    }
  }
  else
  {
#if GTK_CHECK_VERSION(3, 0, 0)
    gtk_orientable_set_orientation(GTK_ORIENTABLE(ih->handle), GTK_ORIENTATION_HORIZONTAL);
#else
    gtk_progress_bar_set_orientation((GtkProgressBar*)ih->handle, GTK_PROGRESS_LEFT_TO_RIGHT);
#endif
  }

  if (iupAttribGetBoolean(ih, "MARQUEE"))
  {
#if GTK_CHECK_VERSION(3, 0, 0)
    gtk_progress_bar_pulse(GTK_PROGRESS_BAR(ih->handle));
#else
    gtk_progress_set_activity_mode((GtkProgress*)ih->handle, TRUE);
#endif

    ih->data->marquee = 1;

    ih->data->timer = IupTimer();
    IupSetCallback(ih->data->timer, "ACTION_CB", (Icallback)gtkProgressBarTimeCb);
    IupSetAttribute(ih->data->timer, "TIME", "100");
    iupAttribSet(ih->data->timer, "_IUP_PROGRESSBAR", (char*)ih);

    gtk_progress_bar_set_pulse_step((GtkProgressBar*)ih->handle, 0.02);
  }
  else
    ih->data->marquee = 0;

  return IUP_NOERROR;
}

void iupdrvProgressBarInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = gtkProgressBarMapMethod;

  /* Driver Dependent Attribute functions */
  
  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  
  /* Special */
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);

  /* IupProgressBar only */
  iupClassRegisterAttribute(ic, "VALUE",  iProgressBarGetValueAttrib,  gtkProgressBarSetValueAttrib,  NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
#if !GTK_CHECK_VERSION(3, 0, 0)
  iupClassRegisterAttribute(ic, "DASHED", iProgressBarGetDashedAttrib, gtkProgressBarSetDashedAttrib, NULL, NULL, IUPAF_NO_INHERIT);
#endif
  iupClassRegisterAttribute(ic, "ORIENTATION", NULL, NULL, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARQUEE",     NULL, gtkProgressBarSetMarqueeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DASHED",      NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}
