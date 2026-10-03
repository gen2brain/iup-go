/** \file
 * \brief Web Browser Control - GTK 4 host of the native web engine
 *
 * See Copyright Notice in "iup.h"
 */

#include <math.h>
#include <string.h>

#include <gtk/gtk.h>
#ifdef GDK_WINDOWING_WIN32
#include <gdk/win32/gdkwin32.h>
#else
#include <gdk/macos/gdkmacos.h>
#endif

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_classbase.h"

#include "iupgtk4_drv.h"
#include "iupweb_host.h"


typedef struct _IgtkWebHost
{
  Ihandle* ih;
  GtkWidget* widget;
  GdkFrameClock* clock;
  gulong paint_id;
  int rect[4];
  int clip[4];
} IgtkWebHost;

static void* gtk4WebHostParent(GtkWidget* widget)
{
  GtkNative* native = gtk_widget_get_native(widget);
  GdkSurface* surface = native ? gtk_native_get_surface(native) : NULL;

#ifdef GDK_WINDOWING_WIN32
  HWND hwnd;

  if (!surface || !GDK_IS_WIN32_SURFACE(surface))
    return NULL;

  hwnd = (HWND)gdk_win32_surface_get_handle(surface);
  if (hwnd)
    SetWindowLongPtr(hwnd, GWL_STYLE, GetWindowLongPtr(hwnd, GWL_STYLE) | WS_CLIPCHILDREN);
  return hwnd;
#else
  if (!surface || !GDK_IS_MACOS_SURFACE(surface))
    return NULL;

  return gdk_macos_surface_get_native_window((GdkMacosSurface*)surface);
#endif
}

static void gtk4WebHostToNative(const graphene_rect_t* r, double sx, double sy, int scale, int* out)
{
  out[0] = (int)floor((r->origin.x + sx) * scale + 0.5);
  out[1] = (int)floor((r->origin.y + sy) * scale + 0.5);
  out[2] = (int)floor((r->origin.x + sx + r->size.width) * scale + 0.5) - out[0];
  out[3] = (int)floor((r->origin.y + sy + r->size.height) * scale + 0.5) - out[1];
}

static void gtk4WebHostPlace(IgtkWebHost* host)
{
  GtkNative* native = gtk_widget_get_native(host->widget);
  graphene_rect_t bounds, visible;
  int rect[4] = {0, 0, 0, 0};
  int clip[4] = {0, 0, 0, 0};

  if (native && gtk_widget_get_mapped(host->widget) &&
      gtk_widget_compute_bounds(host->widget, GTK_WIDGET(native), &bounds))
  {
    Ihandle* parent;
    graphene_rect_t area;
    double sx, sy;
#ifdef GDK_WINDOWING_WIN32
    int scale = gdk_surface_get_scale_factor(gtk_native_get_surface(native));
#else
    int scale = 1;
#endif
    int shown = 1;

    visible = bounds;
    if (gtk_widget_compute_bounds(iupgtk4NativeGetContent(native), GTK_WIDGET(native), &area))
      shown = graphene_rect_intersection(&visible, &area, &visible);

    for (parent = host->ih->parent; parent && shown && parent->iclass->nativetype != IUP_TYPEDIALOG; parent = parent->parent)
    {
      if (parent->iclass->nativetype != IUP_TYPEVOID && parent->handle &&
          gtk_widget_compute_bounds((GtkWidget*)parent->handle, GTK_WIDGET(native), &area))
        shown = graphene_rect_intersection(&visible, &area, &visible);
    }

    gtk_native_get_surface_transform(native, &sx, &sy);
    gtk4WebHostToNative(&bounds, sx, sy, scale, rect);
    if (shown)
      gtk4WebHostToNative(&visible, sx, sy, scale, clip);
  }

  if (memcmp(rect, host->rect, sizeof(rect)) == 0 && memcmp(clip, host->clip, sizeof(clip)) == 0)
    return;

  memcpy(host->rect, rect, sizeof(rect));
  memcpy(host->clip, clip, sizeof(clip));
  iupwebHostSetBounds(host->ih, rect[0], rect[1], rect[2], rect[3], clip[0], clip[1], clip[2], clip[3]);
}

static void gtk4WebHostAfterPaint(GdkFrameClock* clock, IgtkWebHost* host)
{
  (void)clock;
  gtk4WebHostPlace(host);
}

static void gtk4WebHostMapped(GtkWidget* widget, IgtkWebHost* host)
{
  (void)widget;
  gtk4WebHostPlace(host);
}

static void gtk4WebHostRealize(GtkWidget* widget, IgtkWebHost* host)
{
  void* parent = gtk4WebHostParent(widget);
  if (parent)
    iupwebHostSetParent(host->ih, parent);

  host->clock = gtk_widget_get_frame_clock(widget);
  if (host->clock)
    host->paint_id = g_signal_connect(host->clock, "after-paint", G_CALLBACK(gtk4WebHostAfterPaint), host);

  host->clip[2] = -1;
  gtk4WebHostPlace(host);
}

static void gtk4WebHostUnrealize(GtkWidget* widget, IgtkWebHost* host)
{
  (void)widget;

  if (host->clock && host->paint_id)
    g_signal_handler_disconnect(host->clock, host->paint_id);
  host->clock = NULL;
  host->paint_id = 0;

  iupwebHostSetParent(host->ih, NULL);
}

void* iupwebHostMap(Ihandle* ih)
{
  IgtkWebHost* host = g_new0(IgtkWebHost, 1);
  GtkWidget* widget = gtk_drawing_area_new();

  host->ih = ih;
  host->widget = widget;
  host->clip[2] = -1;
  ih->handle = widget;
  iupAttribSet(ih, "_IUPGTK4_WEBHOST", (char*)host);

  g_signal_connect(widget, "realize", G_CALLBACK(gtk4WebHostRealize), host);
  g_signal_connect(widget, "unrealize", G_CALLBACK(gtk4WebHostUnrealize), host);
  g_signal_connect(widget, "map", G_CALLBACK(gtk4WebHostMapped), host);
  g_signal_connect(widget, "unmap", G_CALLBACK(gtk4WebHostMapped), host);

  iupgtk4AddToParent(ih);

  if (!gtk_widget_get_realized(widget))
    return NULL;

  return gtk4WebHostParent(widget);
}

void iupwebHostLayoutUpdate(Ihandle* ih)
{
  iupdrvBaseLayoutUpdateMethod(ih);
}

void iupwebHostUnMap(Ihandle* ih)
{
  IgtkWebHost* host = (IgtkWebHost*)iupAttribGet(ih, "_IUPGTK4_WEBHOST");
  iupAttribSet(ih, "_IUPGTK4_WEBHOST", NULL);

  if (host)
  {
    if (host->clock && host->paint_id)
      g_signal_handler_disconnect(host->clock, host->paint_id);
    g_signal_handlers_disconnect_by_data(host->widget, host);
    g_free(host);
  }

  if (ih->handle)
    iupdrvBaseUnMapMethod(ih);
  ih->handle = NULL;
}
