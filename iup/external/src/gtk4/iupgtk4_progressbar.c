/** \file
* \brief Progress bar Control for GTK4
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
#include "iup_progressbar.h"
#include "iup_drv.h"

#include "iupgtk4_drv.h"

static int gtk4ProgressBarTimeCb(Ihandle* timer)
{
  Ihandle* ih = (Ihandle*)iupAttribGet(timer, "_IUP_PROGRESSBAR");
  gtk_progress_bar_pulse((GtkProgressBar*)ih->handle);
  return IUP_DEFAULT;
}

static int gtk4ProgressBarSetMarqueeAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->marquee)
    return 0;

  if (iupStrBoolean(value))
    IupSetAttribute(ih->data->timer, "RUN", "YES");
  else
    IupSetAttribute(ih->data->timer, "RUN", "NO");

  return 1;
}

static int gtk4ProgressBarSetValueAttrib(Ihandle* ih, const char* value)
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

static int gtk4ProgressBarMapMethod(Ihandle* ih)
{
  ih->handle = gtk_progress_bar_new();
  if (!ih->handle)
    return IUP_ERROR;

  iupgtk4AddToParent(ih);

  gtk_widget_realize(ih->handle);

  if (iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "VERTICAL"))
  {
    gtk_orientable_set_orientation(GTK_ORIENTABLE(ih->handle), GTK_ORIENTATION_VERTICAL);
    gtk_progress_bar_set_inverted(GTK_PROGRESS_BAR(ih->handle), TRUE);

    if (ih->userheight < ih->userwidth)
    {
      int tmp = ih->userheight;
      ih->userheight = ih->userwidth;
      ih->userwidth = tmp;
    }
  }
  else
  {
    gtk_orientable_set_orientation(GTK_ORIENTABLE(ih->handle), GTK_ORIENTATION_HORIZONTAL);
  }

  if (iupAttribGetBoolean(ih, "MARQUEE"))
  {
    gtk_progress_bar_pulse(GTK_PROGRESS_BAR(ih->handle));

    ih->data->marquee = 1;

    ih->data->timer = IupTimer();
    IupSetCallback(ih->data->timer, "ACTION_CB", (Icallback)gtk4ProgressBarTimeCb);
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
  ic->Map = gtk4ProgressBarMapMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "VALUE", iProgressBarGetValueAttrib, gtk4ProgressBarSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ORIENTATION", NULL, NULL, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARQUEE", NULL, gtk4ProgressBarSetMarqueeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DASHED", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}
