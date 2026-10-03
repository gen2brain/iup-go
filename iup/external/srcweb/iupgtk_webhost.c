/** \file
 * \brief Web Browser Control - GTK 3 host of the native web engine
 *
 * See Copyright Notice in "iup.h"
 */

#include <string.h>

#include <gtk/gtk.h>
#ifdef GDK_WINDOWING_WIN32
#include <gdk/gdkwin32.h>
#else
#include <gdk/gdkquartz.h>
#include <gdk/quartz/gdkquartz-cocoa-access.h>
#endif

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_classbase.h"

#include "iupgtk_drv.h"
#include "iupweb_host.h"

#ifdef GDK_WINDOWING_WIN32


static HWND gtkWebHostNative(GtkWidget* area)
{
  GdkWindow* window = gtk_widget_get_window(area);
  if (!window || !gdk_window_ensure_native(window))
    return NULL;
  return gdk_win32_window_get_handle(window);
}

static void gtkWebHostPlace(GtkWidget* area, GdkRectangle* allocation, Ihandle* ih)
{
  HWND hwnd = gtk_widget_get_realized(area) ? gtkWebHostNative(area) : NULL;
  RECT rect;
  (void)allocation;

  if (!hwnd)
    return;

  GetClientRect(hwnd, &rect);
  iupwebHostSetBounds(ih, 0, 0, rect.right, rect.bottom, 0, 0, rect.right, rect.bottom);
}

static void gtkWebHostRealize(GtkWidget* area, Ihandle* ih)
{
  HWND hwnd = gtkWebHostNative(area);
  if (!hwnd)
    return;

  iupwebHostSetParent(ih, hwnd);
  gtkWebHostPlace(area, NULL, ih);
}

static void gtkWebHostUnrealize(GtkWidget* area, Ihandle* ih)
{
  (void)area;
  iupwebHostSetParent(ih, NULL);
}

void* iupwebHostMap(Ihandle* ih)
{
  GtkWidget* area = gtk_drawing_area_new();
  ih->handle = area;

  g_signal_connect(area, "realize", G_CALLBACK(gtkWebHostRealize), ih);
  g_signal_connect(area, "unrealize", G_CALLBACK(gtkWebHostUnrealize), ih);
  g_signal_connect(area, "size-allocate", G_CALLBACK(gtkWebHostPlace), ih);

  iupgtkAddToParent(ih);
  gtk_widget_realize(area);

  return gtk_widget_get_realized(area) ? gtkWebHostNative(area) : NULL;
}

void iupwebHostLayoutUpdate(Ihandle* ih)
{
  iupdrvBaseLayoutUpdateMethod(ih);
}

void iupwebHostUnMap(Ihandle* ih)
{
  if (!ih->handle)
    return;

  g_signal_handlers_disconnect_by_data(ih->handle, ih);
  iupdrvBaseUnMapMethod(ih);
  ih->handle = NULL;
}

#else

typedef struct _IgtkWebHost
{
  Ihandle* ih;
  GtkWidget* widget;
  GdkFrameClock* clock;
  gulong paint_id;
  int rect[4];
  int clip[4];
} IgtkWebHost;

static NSView* gtkWebHostParent(GtkWidget* widget)
{
  GtkWidget* toplevel = gtk_widget_get_toplevel(widget);
  GdkWindow* window = gtk_widget_is_toplevel(toplevel) ? gtk_widget_get_window(toplevel) : NULL;
  return window ? gdk_quartz_window_get_nsview(window) : NULL;
}

static void gtkWebHostPlace(IgtkWebHost* host)
{
  GtkWidget* toplevel = gtk_widget_get_toplevel(host->widget);
  int rect[4] = {0, 0, 0, 0};
  int clip[4] = {0, 0, 0, 0};
  int x, y;

  if (gtk_widget_is_toplevel(toplevel) && gtk_widget_is_drawable(host->widget) &&
      gtk_widget_translate_coordinates(host->widget, toplevel, 0, 0, &x, &y))
  {
    GtkAllocation alloc;
    Ihandle* parent;
    int x1, y1, x2, y2;

    gtk_widget_get_allocation(host->widget, &alloc);
    x1 = x; y1 = y; x2 = x + alloc.width; y2 = y + alloc.height;

    for (parent = host->ih->parent; parent && parent->iclass->nativetype != IUP_TYPEDIALOG; parent = parent->parent)
    {
      GtkWidget* widget = (GtkWidget*)parent->handle;
      int px, py;

      if (parent->iclass->nativetype == IUP_TYPEVOID || !widget ||
          !gtk_widget_translate_coordinates(widget, toplevel, 0, 0, &px, &py))
        continue;

      gtk_widget_get_allocation(widget, &alloc);
      if (px > x1) x1 = px;
      if (py > y1) y1 = py;
      if (px + alloc.width < x2) x2 = px + alloc.width;
      if (py + alloc.height < y2) y2 = py + alloc.height;
    }

    gtk_widget_get_allocation(host->widget, &alloc);
    rect[0] = x; rect[1] = y; rect[2] = alloc.width; rect[3] = alloc.height;
    if (x2 > x1 && y2 > y1)
    {
      clip[0] = x1; clip[1] = y1; clip[2] = x2 - x1; clip[3] = y2 - y1;
    }
  }

  if (memcmp(rect, host->rect, sizeof(rect)) == 0 && memcmp(clip, host->clip, sizeof(clip)) == 0)
    return;

  memcpy(host->rect, rect, sizeof(rect));
  memcpy(host->clip, clip, sizeof(clip));
  iupwebHostSetBounds(host->ih, rect[0], rect[1], rect[2], rect[3], clip[0], clip[1], clip[2], clip[3]);
}

static void gtkWebHostAfterPaint(GdkFrameClock* clock, IgtkWebHost* host)
{
  (void)clock;
  gtkWebHostPlace(host);
}

static void gtkWebHostChanged(GtkWidget* widget, IgtkWebHost* host)
{
  (void)widget;
  gtkWebHostPlace(host);
}

static void gtkWebHostAllocate(GtkWidget* widget, GdkRectangle* allocation, IgtkWebHost* host)
{
  (void)widget;
  (void)allocation;
  gtkWebHostPlace(host);
}

static void gtkWebHostRealize(GtkWidget* widget, IgtkWebHost* host)
{
  NSView* view = gtkWebHostParent(widget);
  if (view)
    iupwebHostSetParent(host->ih, view);

  host->clock = gtk_widget_get_frame_clock(widget);
  if (host->clock)
    host->paint_id = g_signal_connect(host->clock, "after-paint", G_CALLBACK(gtkWebHostAfterPaint), host);

  host->clip[2] = -1;
  gtkWebHostPlace(host);
}

static void gtkWebHostUnrealize(GtkWidget* widget, IgtkWebHost* host)
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
  GtkWidget* area = gtk_drawing_area_new();

  host->ih = ih;
  host->widget = area;
  host->clip[2] = -1;
  ih->handle = area;
  iupAttribSet(ih, "_IUPGTK_WEBHOST", (char*)host);

  g_signal_connect(area, "realize", G_CALLBACK(gtkWebHostRealize), host);
  g_signal_connect(area, "unrealize", G_CALLBACK(gtkWebHostUnrealize), host);
  g_signal_connect(area, "map", G_CALLBACK(gtkWebHostChanged), host);
  g_signal_connect(area, "unmap", G_CALLBACK(gtkWebHostChanged), host);
  g_signal_connect(area, "size-allocate", G_CALLBACK(gtkWebHostAllocate), host);

  iupgtkAddToParent(ih);
  gtk_widget_realize(area);

  return gtk_widget_get_realized(area) ? gtkWebHostParent(area) : NULL;
}

void iupwebHostLayoutUpdate(Ihandle* ih)
{
  iupdrvBaseLayoutUpdateMethod(ih);
}

void iupwebHostUnMap(Ihandle* ih)
{
  IgtkWebHost* host = (IgtkWebHost*)iupAttribGet(ih, "_IUPGTK_WEBHOST");
  iupAttribSet(ih, "_IUPGTK_WEBHOST", NULL);

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

#endif
