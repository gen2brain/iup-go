/** \file
 * \brief GTK System Tray (XEmbed)
 *
 * See Copyright Notice in "iup.h"
 */

#undef GTK_DISABLE_DEPRECATED /* Since GTK 3.14 gtk_status_icon is deprecated. */
#include <gtk/gtk.h>

#include <stdlib.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"

#include "iupgtk_drv.h"

/* gtk_status_icon - deprecated in 3.14, but still available in 3.22 */
#if GTK_CHECK_VERSION(2, 10, 0)

static int gtkTrayDoubleClick(int button)
{
  static int last_button = -1;
  static GTimer* timer = NULL;

  if (last_button == -1 || last_button != button)
  {
    last_button = button;
    if (timer)
      g_timer_destroy(timer);
    timer = g_timer_new();
    return 0;
  }
  else
  {
    double seconds;

    if (!timer)  /* just in case */
      return 0;

    seconds = g_timer_elapsed(timer, NULL);
    if (seconds < 0.4)
    {
      /* reset state */
      g_timer_destroy(timer);
      timer = NULL;
      last_button = -1;
      return 1;
    }
    else
    {
      g_timer_reset(timer);
      return 0;
    }
  }
}

static void gtkTrayAction(GtkStatusIcon *status_icon, Ihandle *ih)
{
  /* from GTK source code it is called only when button==1 and pressed==1 */
  int button = 1;
  int pressed = 1;
  int dclick = gtkTrayDoubleClick(button);
  IFniii cb = (IFniii)IupGetCallback(ih, "TRAYCLICK_CB");

  if (cb && cb(ih, button, pressed, dclick) == IUP_CLOSE)
    IupExitLoop();

  (void)status_icon;
}

static void gtkTrayPopupMenu(GtkStatusIcon *status_icon, guint gbutton, guint activate_time, Ihandle *ih)
{
  /* from GTK source code it is called only when button==3 and pressed==1 */
  int button = 3;
  int pressed = 1;
  int dclick = gtkTrayDoubleClick(button);
  IFniii cb = (IFniii)IupGetCallback(ih, "TRAYCLICK_CB");

  if (cb && cb(ih, button, pressed, dclick) == IUP_CLOSE)
    IupExitLoop();

  (void)activate_time;
  (void)gbutton;
  (void)status_icon;
}

static GtkStatusIcon* gtkGetStatusIcon(Ihandle *ih)
{
  GtkStatusIcon* status_icon = (GtkStatusIcon*)iupAttribGet(ih, "_IUPGTK_STATUSICON");

  if (!status_icon)
  {
    status_icon = gtk_status_icon_new();

    g_signal_connect(G_OBJECT(status_icon), "activate", G_CALLBACK(gtkTrayAction), ih);
    g_signal_connect(G_OBJECT(status_icon), "popup-menu", G_CALLBACK(gtkTrayPopupMenu), ih);

    iupAttribSet(ih, "_IUPGTK_STATUSICON", (char*)status_icon);
  }

  return status_icon;
}

int iupgtkSetTrayAttrib(Ihandle *ih, const char *value)
{
  GtkStatusIcon* status_icon = gtkGetStatusIcon(ih);
  gtk_status_icon_set_visible(status_icon, iupStrBoolean(value));
  return 1;
}

int iupgtkSetTrayTipAttrib(Ihandle *ih, const char *value)
{
  GtkStatusIcon* status_icon = gtkGetStatusIcon(ih);

#if GTK_CHECK_VERSION(2, 16, 0)
  if (value)
  {
    gtk_status_icon_set_has_tooltip(status_icon, TRUE);
    if (iupAttribGetBoolean(ih, "TIPMARKUP"))
      gtk_status_icon_set_tooltip_markup(status_icon, value);
    else
      gtk_status_icon_set_tooltip_text(status_icon, value);
  }
  else
    gtk_status_icon_set_has_tooltip(status_icon, FALSE);
#else
  gtk_status_icon_set_tooltip(status_icon, value);
#endif

  return 1;
}

int iupgtkSetTrayImageAttrib(Ihandle *ih, const char *value)
{
  GtkStatusIcon* status_icon = gtkGetStatusIcon(ih);
  GdkPixbuf* icon = (GdkPixbuf*)iupImageGetIcon(value);
  gtk_status_icon_set_from_pixbuf(status_icon, icon);
  return 1;
}

int iupgtkSetTrayMenuAttrib(Ihandle *ih, const char *value)
{
  /* XEmbed tray doesn't support automatic menu popup via TRAYMENU.
   * Applications should use TRAYCLICK_CB callback to show menu manually.
   */
  (void)ih;
  (void)value;
  return 0;
}

int iupgtkTrayCleanup(Ihandle *ih)
{
  GtkStatusIcon* status_icon = (GtkStatusIcon*)iupAttribGet(ih, "_IUPGTK_STATUSICON");

  if (status_icon)
  {
    g_object_unref(status_icon);
    iupAttribSet(ih, "_IUPGTK_STATUSICON", NULL);
    return 1; /* Tray was cleaned up */
  }
  return 0; /* No tray to clean up */
}

#endif
