/** \file
 * \brief GTK System Tray Driver (XEmbed/GtkStatusIcon)
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
#include "iup_class.h"
#include "iup_tray.h"

#include "iupgtk_drv.h"

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

/******************************************************************************/
/* Driver Interface Implementation                                            */
/******************************************************************************/

int iupdrvTraySetVisible(Ihandle *ih, int visible)
{
  GtkStatusIcon* status_icon = gtkGetStatusIcon(ih);
  gtk_status_icon_set_visible(status_icon, visible);
  return 1;
}

int iupdrvTraySetTip(Ihandle *ih, const char *value)
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

int iupdrvTraySetImage(Ihandle *ih, const char *value)
{
  GtkStatusIcon* status_icon = gtkGetStatusIcon(ih);
  GdkPixbuf* icon = (GdkPixbuf*)iupImageGetIcon(value);
  gtk_status_icon_set_from_pixbuf(status_icon, icon);
  return 1;
}

int iupdrvTraySetMenu(Ihandle *ih, Ihandle *menu)
{
  /* XEmbed tray protocol doesn't support automatic menu popup via MENU attribute.
   * Applications should use TRAYCLICK_CB callback to show menu manually via IupPopup(). */
  (void)ih;
  (void)menu;
  return 0;
}

static gboolean gtkXEmbedDeferredExitLoop(gpointer data)
{
  (void)data;
  if (gtk_main_level() > 1)
    return G_SOURCE_CONTINUE;
  IupExitLoop();
  return G_SOURCE_REMOVE;
}

void iupdrvTrayDestroy(Ihandle *ih)
{
  GtkStatusIcon* status_icon = (GtkStatusIcon*)iupAttribGet(ih, "_IUPGTK_STATUSICON");

  if (status_icon)
  {
    g_object_unref(status_icon);
    iupAttribSet(ih, "_IUPGTK_STATUSICON", NULL);

    g_idle_add(gtkXEmbedDeferredExitLoop, NULL);
  }
}

int iupdrvTrayIsAvailable(void)
{
  return 1;
}

void iupdrvTrayInitClass(Iclass* ic)
{
  iupClassRegisterAttribute(ic, "TIPMARKUP", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "MENU", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TIPBALLOON", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TIPBALLOONTITLE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TIPBALLOONTITLEICON", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TIPDELAY", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
}

#endif
