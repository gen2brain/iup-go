/** \file
 * \brief GTK4 Driver Core
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#ifdef GDK_WINDOWING_X11
#include <gdk/x11/gdkx.h>
#endif

#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/wayland/gdkwayland.h>
#endif

#ifdef GDK_WINDOWING_WIN32
#include <gdk/win32/gdkwin32.h>
#endif

#ifdef GDK_WINDOWING_MACOS
#include <gdk/macos/gdkmacos.h>
#endif

#include "iup.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_object.h"
#include "iup_globalattrib.h"

#include "iupgtk4_drv.h"

char* iupgtk4GetNativeWidgetHandle(GtkWidget *widget)
{
  if (!widget)
    return NULL;

  GtkNative* native = gtk_widget_get_native(widget);
  if (!native)
    return NULL;

  GdkSurface* surface = gtk_native_get_surface(native);
  if (!surface)
    return NULL;

#ifdef GDK_WINDOWING_X11
  if (GDK_IS_X11_SURFACE(surface))
  {
    return (char*)(uintptr_t)gdk_x11_surface_get_xid(surface);
  }
#endif

#ifdef GDK_WINDOWING_WAYLAND
  if (GDK_IS_WAYLAND_SURFACE(surface))
  {
    return (char*)gdk_wayland_surface_get_wl_surface(surface);
  }
#endif

#ifdef GDK_WINDOWING_WIN32
  if (GDK_IS_WIN32_SURFACE(surface))
  {
    return (char*)gdk_win32_surface_get_handle(surface);
  }
#endif

#ifdef GDK_WINDOWING_MACOS
  if (GDK_IS_MACOS_SURFACE(surface))
  {
    return (char*)surface;
  }
#endif

  return NULL;
}

const char* iupgtk4GetNativeWindowHandleName(void)
{
  GdkDisplay* display = gdk_display_get_default();
  if (!display) return "NULL";

#ifdef GDK_WINDOWING_X11
  if (GDK_IS_X11_DISPLAY(display))
    return "XWINDOW";
#endif

#ifdef GDK_WINDOWING_WAYLAND
  if (GDK_IS_WAYLAND_DISPLAY(display))
    return "WL_SURFACE";
#endif

#ifdef GDK_WINDOWING_WIN32
  if (GDK_IS_WIN32_DISPLAY(display))
    return "HWND";
#endif

#ifdef GDK_WINDOWING_MACOS
  if (GDK_IS_MACOS_DISPLAY(display))
    return "GDKSURFACE";
#endif

  return "UNKNOWN";
}

char* iupgtk4GetNativeWindowHandleAttrib(Ihandle* ih)
{
  return iupgtk4GetNativeWidgetHandle((GtkWidget*)ih->handle);
}

IUP_SDK_API void* iupdrvGetDisplay(void)
{
#ifdef GDK_WINDOWING_X11
  GdkDisplay* display = gdk_display_get_default();
  if (display && GDK_IS_X11_DISPLAY(display))
    return gdk_x11_display_get_xdisplay(display);
#endif
  return NULL;
}

void iupgtk4StrRelease(void)
{
  /* GTK4 uses UTF-8 internally, no conversion needed */
}

char* iupgtk4StrConvertToSystem(const char* str)
{
  return (char*)str;
}

char* iupgtk4StrConvertFromSystem(const char* str)
{
  return (char*)str;
}

static void gtkSetGlobalAttrib(void)
{
  GdkDisplay* display = gdk_display_get_default();
  if (!display) return;

#ifdef GDK_WINDOWING_X11
  if (GDK_IS_X11_DISPLAY(display))
  {
    Display* xdisplay = gdk_x11_display_get_xdisplay(display);
    IupSetGlobal("XDISPLAY", (char*)xdisplay);
    IupSetGlobal("XSCREEN", (char*)(long)iupgtk4X11GetDefaultScreen(xdisplay));
    IupSetGlobal("XSERVERVENDOR", iupgtk4X11GetServerVendor(xdisplay));
    IupSetInt(NULL, "XVENDORRELEASE", iupgtk4X11GetVendorRelease(xdisplay));
    IupSetGlobal("GDK_WINDOWING", "X11");
  }
#endif

#ifdef GDK_WINDOWING_WAYLAND
  if (GDK_IS_WAYLAND_DISPLAY(display))
  {
    struct wl_display* wl_display = gdk_wayland_display_get_wl_display(display);
    IupSetGlobal("WL_DISPLAY", (char*)wl_display);
    IupSetGlobal("GDK_WINDOWING", "WAYLAND");
  }
#endif

#ifdef GDK_WINDOWING_WIN32
  if (GDK_IS_WIN32_DISPLAY(display))
  {
    IupSetGlobal("GDK_WINDOWING", "WIN32");
  }
#endif

#ifdef GDK_WINDOWING_MACOS
  if (GDK_IS_MACOS_DISPLAY(display))
  {
    IupSetGlobal("GDK_WINDOWING", "MACOS");
  }
#endif
}

static void gtkSetGlobalColorAttrib(const char* name, GdkRGBA *color)
{
  iupGlobalSetDefaultColorAttrib(name,
    (int)(color->red * 255.0),
    (int)(color->green * 255.0),
    (int)(color->blue * 255.0));
}

static void gtkUpdateGlobalColors(GtkWidget* dialog, GtkWidget* text)
{
  GdkRGBA color;
  GtkStyleContext* context;

  context = gtk_widget_get_style_context(dialog);

  gtk_widget_get_color(dialog, &color);
  gtkSetGlobalColorAttrib("DLGFGCOLOR", &color);

  if (gtk_style_context_lookup_color(context, "theme_bg_color", &color))
  {
    gtkSetGlobalColorAttrib("DLGBGCOLOR", &color);
  }
  else
  {
    color.red = 0.94; color.green = 0.94; color.blue = 0.94; color.alpha = 1.0;
    gtkSetGlobalColorAttrib("DLGBGCOLOR", &color);
  }

  context = gtk_widget_get_style_context(text);

  gtk_widget_get_color(text, &color);
  gtkSetGlobalColorAttrib("TXTFGCOLOR", &color);

  if (gtk_style_context_lookup_color(context, "theme_base_color", &color))
  {
    gtkSetGlobalColorAttrib("TXTBGCOLOR", &color);
  }
  else
  {
    color.red = 1.0; color.green = 1.0; color.blue = 1.0; color.alpha = 1.0;
    gtkSetGlobalColorAttrib("TXTBGCOLOR", &color);
  }

  if (gtk_style_context_lookup_color(context, "theme_selected_bg_color", &color))
  {
    gtkSetGlobalColorAttrib("TXTHLCOLOR", &color);
  }
  else
  {
    color.red = 0.2; color.green = 0.4; color.blue = 0.8; color.alpha = 1.0;
    gtkSetGlobalColorAttrib("TXTHLCOLOR", &color);
  }

  if (gtk_style_context_lookup_color(context, "link_color", &color) ||
      gtk_style_context_lookup_color(context, "theme_link_color", &color))
  {
    gtkSetGlobalColorAttrib("LINKFGCOLOR", &color);
  }
  else
  {
    iupGlobalSetDefaultColorAttrib("LINKFGCOLOR", 0, 0, 238);
  }

  context = gtk_widget_get_style_context(dialog);

  gtk_widget_get_color(dialog, &color);
  gtkSetGlobalColorAttrib("MENUFGCOLOR", &color);

  if (gtk_style_context_lookup_color(context, "theme_bg_color", &color))
  {
    gtkSetGlobalColorAttrib("MENUBGCOLOR", &color);
  }
  else
  {
    color.red = 0.94; color.green = 0.94; color.blue = 0.94; color.alpha = 1.0;
    gtkSetGlobalColorAttrib("MENUBGCOLOR", &color);
  }
}

static void gtkSetGlobalColors(void)
{
  GtkWidget* dialog = gtk_window_new();
  GtkWidget* text = gtk_entry_new();
  gtk_window_set_child(GTK_WINDOW(dialog), text);
  gtk_widget_show(text);
  gtk_widget_realize(text);
  gtk_widget_realize(dialog);
  gtkUpdateGlobalColors(dialog, text);
  gtk_window_destroy(GTK_WINDOW(dialog));
}

int iupdrvOpen(int *argc, char ***argv)
{
  (void)argc;
  (void)argv;

  /* gtk_init() no longer accepts argc/argv */
  gtk_init();

  setlocale(LC_NUMERIC, "C");

  IupSetGlobal("DRIVER", "GTK4");

  IupSetfAttribute(NULL, "GTKVERSION", "%d.%d.%d",
    gtk_get_major_version(),
    gtk_get_minor_version(),
    gtk_get_micro_version());

  IupSetfAttribute(NULL, "GTKDEVVERSION", "%d.%d.%d",
    GTK_MAJOR_VERSION,
    GTK_MINOR_VERSION,
    GTK_MICRO_VERSION);

  if (argv && *argv && (*argv)[0] && (*argv)[0][0] != 0)
    IupStoreGlobal("ARGV0", (*argv)[0]);

  gtkSetGlobalAttrib();
  gtkSetGlobalColors();

  IupSetGlobal("SHOWMENUIMAGES", "YES");

  return IUP_NOERROR;
}

void iupdrvClose(void)
{
  iupgtk4StrRelease();
  iupgtk4LoopCleanup();

#ifdef GDK_WINDOWING_X11
  iupgtk4X11Cleanup();
#endif
}

int iupdrvSetGlobalAppIDAttrib(const char* value)
{
  static int appid_set = 0;
  if (appid_set || !value || !value[0])
    return 0;

  g_set_prgname(value);
  IupStoreGlobal("_IUP_APPID_INTERNAL", value);
  appid_set = 1;
  return 1;
}

int iupdrvSetGlobalAppNameAttrib(const char* value)
{
  static int appname_set = 0;
  if (appname_set || !value || !value[0])
    return 0;

  appname_set = 1;
  return 1;
}
