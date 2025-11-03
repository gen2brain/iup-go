/** \file
 * \brief GTK Driver Core
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
#include <gdk/gdkx.h>
#endif

#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/gdkwayland.h>
#endif

#ifdef GDK_WINDOWING_WIN32
#include <gdk/gdkwin32.h>
#endif

#ifdef GDK_WINDOWING_QUARTZ
#include <gdk/gdkquartz.h>
#endif

#include "iup.h"

#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_object.h"
#include "iup_globalattrib.h"

#include "iupgtk_drv.h"

#if defined(GDK_WINDOWING_WIN32)
#if GTK_CHECK_VERSION(3, 0, 0)
#define IUPGTK_GDK_WINDOW_HWND(w) gdk_win32_window_get_handle(w)
#else
#define IUPGTK_GDK_WINDOW_HWND(w) GDK_WINDOW_HWND(w)
#endif
#endif

#if defined(GDK_WINDOWING_X11)
#if GTK_CHECK_VERSION(3, 0, 0)
#define IUPGTK_GDK_WINDOW_XID(w) gdk_x11_window_get_xid(w)
#define IUPGTK_GDK_DISPLAY_XDISPLAY(d) gdk_x11_display_get_xdisplay(d)
#else
#define IUPGTK_GDK_WINDOW_XID(w) GDK_WINDOW_XID(w)
#define IUPGTK_GDK_DISPLAY_XDISPLAY(d) GDK_DISPLAY_XDISPLAY(d)
#endif

/* X11 function pointers - loaded dynamically to avoid hard linking */
typedef struct _XDisplay Display;
typedef struct _XGC* GC;
typedef unsigned long XID;

#include <dlfcn.h>

static void* x11_lib = NULL;
static GC (*_XCreateGC)(Display*, XID, unsigned long, void*) = NULL;
static int (*_XFreeGC)(Display*, GC) = NULL;
static int (*_XDefaultScreen)(Display*) = NULL;
static char* (*_XServerVendor)(Display*) = NULL;
static int (*_XVendorRelease)(Display*) = NULL;

static int x11_load_functions(void)
{
  if (x11_lib)
    return 1;

  x11_lib = dlopen("libX11.so.6", RTLD_LAZY);
  if (!x11_lib)
    x11_lib = dlopen("libX11.so", RTLD_LAZY);

  if (!x11_lib)
    return 0;

  _XCreateGC = (GC (*)(Display*, XID, unsigned long, void*))dlsym(x11_lib, "XCreateGC");
  _XFreeGC = (int (*)(Display*, GC))dlsym(x11_lib, "XFreeGC");
  _XDefaultScreen = (int (*)(Display*))dlsym(x11_lib, "XDefaultScreen");
  _XServerVendor = (char* (*)(Display*))dlsym(x11_lib, "XServerVendor");
  _XVendorRelease = (int (*)(Display*))dlsym(x11_lib, "XVendorRelease");

  if (!_XCreateGC || !_XFreeGC || !_XDefaultScreen || !_XServerVendor || !_XVendorRelease)
  {
    dlclose(x11_lib);
    x11_lib = NULL;
    return 0;
  }

  return 1;
}

/* XVisualIDFromVisual is a macro in X11, define it as inline function */
static inline XID x11_visual_id_from_visual(Visual* v) {
    return v ? v->visualid : 0;
}
#endif

char* iupgtkGetNativeWidgetHandle(GtkWidget *widget)
{
  GdkWindow* window = iupgtkGetWindow(widget);
  if (!window)
    return NULL;

#if GTK_CHECK_VERSION(3, 0, 0)
  /* GTK3+: Use runtime checks for backend detection */

#ifdef GDK_WINDOWING_X11
  if (GDK_IS_X11_WINDOW(window))
  {
    return (char*)IUPGTK_GDK_WINDOW_XID(window);
  }
#endif

#ifdef GDK_WINDOWING_WAYLAND
  if (GDK_IS_WAYLAND_WINDOW(window))
  {
    /* Return wl_surface. */
    return (char*)gdk_wayland_window_get_wl_surface(window);
  }
#endif

#ifdef GDK_WINDOWING_WIN32
  if (GDK_IS_WIN32_WINDOW(window))
  {
    return (char*)IUPGTK_GDK_WINDOW_HWND(window);
  }
#endif

#ifdef GDK_WINDOWING_QUARTZ
  if (GDK_IS_QUARTZ_WINDOW(window))
  {
    return (char*)window;
  }
#endif

#else
  /* GTK2: Use compile-time detection since runtime checks are not available */

#ifdef GDK_WINDOWING_X11
  return (char*)IUPGTK_GDK_WINDOW_XID(window);
#elif defined(GDK_WINDOWING_WIN32)
  return (char*)IUPGTK_GDK_WINDOW_HWND(window);
#elif defined(GDK_WINDOWING_QUARTZ)
  return (char*)window;
#endif

#endif /* GTK_CHECK_VERSION(3, 0, 0) */

  /* Fallback or unsupported backend */
  return NULL;
}

const char* iupgtkGetNativeWindowHandleName(void)
{
#if GTK_CHECK_VERSION(3, 0, 0)
  GdkDisplay* display = gdk_display_get_default();
  if (!display) return "NULL";

#ifdef GDK_WINDOWING_X11
  if (GDK_IS_X11_DISPLAY(display))
  {
    return "XWINDOW";
  }
#endif

#ifdef GDK_WINDOWING_WAYLAND
  if (GDK_IS_WAYLAND_DISPLAY(display))
  {
    return "WL_SURFACE";
  }
#endif

#ifdef GDK_WINDOWING_WIN32
  if (GDK_IS_WIN32_DISPLAY(display))
  {
    return "HWND";
  }
#endif

#ifdef GDK_WINDOWING_QUARTZ
  if (GDK_IS_QUARTZ_DISPLAY(display))
  {
    return "GDKWINDOW";
  }
#endif

#else
  /* GTK2: Use compile-time detection */

#ifdef GDK_WINDOWING_X11
  return "XWINDOW";
#elif defined(GDK_WINDOWING_WIN32)
  return "HWND";
#elif defined(GDK_WINDOWING_QUARTZ)
  return "GDKWINDOW";
#endif

#endif /* GTK_CHECK_VERSION(3, 0, 0) */

  return "UNKNOWN";
}

const char* iupgtkGetNativeFontIdName(void)
{
#if GTK_CHECK_VERSION(3, 0, 0)
  GdkDisplay* display = gdk_display_get_default();
  if (!display) return NULL;

#ifdef GDK_WINDOWING_X11
  if (GDK_IS_X11_DISPLAY(display))
  {
    return "XFONTID";
  }
#endif

#ifdef GDK_WINDOWING_WIN32
  if (GDK_IS_WIN32_DISPLAY(display))
  {
    return "HFONT";
  }
#endif

#else
  /* GTK2: Use compile-time detection */

#ifdef GDK_WINDOWING_X11
  return "XFONTID";
#elif defined(GDK_WINDOWING_WIN32)
  return "HFONT";
#endif

#endif /* GTK_CHECK_VERSION(3, 0, 0) */

  /* Wayland, Quartz, and others use Pango/Fontconfig/CoreText. */
  return NULL;
}

void* iupgtkGetNativeGraphicsContext(GtkWidget* widget)
{
  GdkWindow* window = iupgtkGetWindow(widget);
  if (!window) return NULL;

#if GTK_CHECK_VERSION(3, 0, 0)
  GdkDisplay* display = gdk_window_get_display(window);

#ifdef GDK_WINDOWING_X11
  if (GDK_IS_X11_DISPLAY(display))
  {
    if (!x11_load_functions())
      return NULL;
    return (void*)_XCreateGC(IUPGTK_GDK_DISPLAY_XDISPLAY(display), IUPGTK_GDK_WINDOW_XID(window), 0, NULL);
  }
#endif

#ifdef GDK_WINDOWING_WIN32
  if (GDK_IS_WIN32_DISPLAY(display))
  {
    return GetDC(IUPGTK_GDK_WINDOW_HWND(window));
  }
#endif

#ifdef GDK_WINDOWING_QUARTZ
  if (GDK_IS_QUARTZ_DISPLAY(display))
  {
    /* In GTK3/Quartz/Wayland, drawing is done via Cairo. */
    return NULL;
  }
#endif

#else
  /* GTK2: Use compile-time backend detection */

#ifdef GDK_WINDOWING_X11
  Display* xdisplay = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
  if (!x11_load_functions())
    return NULL;
  return (void*)_XCreateGC(xdisplay, IUPGTK_GDK_WINDOW_XID(window), 0, NULL);
#elif defined(GDK_WINDOWING_WIN32)
  return GetDC(IUPGTK_GDK_WINDOW_HWND(window));
#elif defined(GDK_WINDOWING_QUARTZ)
  if (window)
    return (void*)gdk_gc_new((GdkDrawable*)window);
#endif

#endif /* GTK_CHECK_VERSION(3, 0, 0) */

  /* Wayland and GTK3 use Cairo contexts for drawing. */
  return NULL;
}

void iupgtkReleaseNativeGraphicsContext(GtkWidget* widget, void* gc)
{
  if (!gc) return;

  GdkWindow* window = iupgtkGetWindow(widget);

#if GTK_CHECK_VERSION(3, 0, 0)
  GdkDisplay* display = gdk_display_get_default();
  if (!display) return;

#ifdef GDK_WINDOWING_X11
  if (GDK_IS_X11_DISPLAY(display))
  {
    if (x11_load_functions())
      _XFreeGC(IUPGTK_GDK_DISPLAY_XDISPLAY(display), (GC)gc);
    return;
  }
#endif

#ifdef GDK_WINDOWING_WIN32
  if (GDK_IS_WIN32_DISPLAY(display))
  {
    if (window)
      ReleaseDC(IUPGTK_GDK_WINDOW_HWND(window), (HDC)gc);
    return;
  }
#endif

#ifdef GDK_WINDOWING_QUARTZ
  if (GDK_IS_QUARTZ_DISPLAY(display))
  {
    /* Nothing to do for GTK3/Quartz */
    return;
  }
#endif

#else
  /* GTK2: Use compile-time backend detection */

#ifdef GDK_WINDOWING_X11
  Display* xdisplay = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
  if (x11_load_functions())
    _XFreeGC(xdisplay, (GC)gc);
#elif defined(GDK_WINDOWING_WIN32)
  if (window)
    ReleaseDC(IUPGTK_GDK_WINDOW_HWND(window), (HDC)gc);
#elif defined(GDK_WINDOWING_QUARTZ)
  g_object_unref(gc);
#endif

#endif /* GTK_CHECK_VERSION(3, 0, 0) */

  (void)widget;
}

IUP_SDK_API void* iupdrvGetDisplay(void)
{
  GdkDisplay* display = gdk_display_get_default();
  if (!display) return NULL;

#if GTK_CHECK_VERSION(3, 0, 0)

#ifdef GDK_WINDOWING_X11
  if (GDK_IS_X11_DISPLAY(display))
  {
    return IUPGTK_GDK_DISPLAY_XDISPLAY(display);
  }
#endif

#ifdef GDK_WINDOWING_WAYLAND
  if (GDK_IS_WAYLAND_DISPLAY(display))
  {
    return gdk_wayland_display_get_wl_display(display);
  }
#endif

#ifdef GDK_WINDOWING_QUARTZ
  if (GDK_IS_QUARTZ_DISPLAY(display))
  {
    return (void*)display;
  }
#endif

#else
  /* GTK2: Use compile-time detection */

#ifdef GDK_WINDOWING_X11
  return IUPGTK_GDK_DISPLAY_XDISPLAY(display);
#elif defined(GDK_WINDOWING_QUARTZ)
  return (void*)display;
#endif

#endif /* GTK_CHECK_VERSION(3, 0, 0) */

  return NULL;
}

void iupgtkPushVisualAndColormap(void* visual, void* colormap)
{
  /* This function is for X11 compatibility in GTK2.
     It is deprecated in GTK3 and irrelevant for Wayland/Win32/Quartz.
  */

#if GTK_CHECK_VERSION(3, 0, 0)
  (void)visual;
  (void)colormap;

#else /* GTK2 */

#ifdef GDK_WINDOWING_X11
  GdkColormap* gdk_colormap;
  GdkVisual* gdk_visual;

#if GTK_CHECK_VERSION(2, 24, 0)
  GdkScreen* screen = gdk_screen_get_default();
  gdk_visual = gdk_x11_screen_lookup_visual(screen, x11_visual_id_from_visual((Visual*)visual));
#else
  gdk_visual = gdkx_visual_get(x11_visual_id_from_visual((Visual*)visual));
#endif
  if (colormap)
    gdk_colormap = gdk_x11_colormap_foreign_new(gdk_visual, (Colormap)colormap);
  else
    gdk_colormap = gdk_colormap_new(gdk_visual, FALSE);

  gtk_widget_push_colormap(gdk_colormap);
  /* gtk_widget_push_visual is deprecated */

#elif defined(GDK_WINDOWING_QUARTZ)
  GdkColormap* gdk_colormap;
  GdkVisual *gdk_visual = gdk_visual_get_best();

  gdk_colormap = gdk_colormap_new(gdk_visual, FALSE);

  gtk_widget_push_colormap(gdk_colormap);
#else
  (void)visual;
  (void)colormap;
#endif

#endif /* GTK version check */
}

static void gtkSetGlobalAttrib(void)
{
  GdkDisplay* display = gdk_display_get_default();
  if (!display) return;

#if GTK_CHECK_VERSION(3, 0, 0)

#ifdef GDK_WINDOWING_X11
  if (GDK_IS_X11_DISPLAY(display))
  {
    Display* xdisplay = IUPGTK_GDK_DISPLAY_XDISPLAY(display);
    IupSetGlobal("XDISPLAY", (char*)xdisplay);
    if (x11_load_functions())
    {
      IupSetGlobal("XSCREEN", (char*)(long)_XDefaultScreen(xdisplay));
      IupSetGlobal("XSERVERVENDOR", _XServerVendor(xdisplay));
      IupSetInt(NULL, "XVENDORRELEASE", _XVendorRelease(xdisplay));
    }
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

#ifdef GDK_WINDOWING_QUARTZ
  if (GDK_IS_QUARTZ_DISPLAY(display))
  {
    IupSetGlobal("GDK_WINDOWING", "QUARTZ");
  }
#endif

#ifdef GDK_WINDOWING_WIN32
  if (GDK_IS_WIN32_DISPLAY(display))
  {
    IupSetGlobal("GDK_WINDOWING", "WIN32");
  }
#endif

#else
  /* GTK2: Set X11 attributes if on X11 */

#ifdef GDK_WINDOWING_X11
  Display* xdisplay = IUPGTK_GDK_DISPLAY_XDISPLAY(display);
  IupSetGlobal("XDISPLAY", (char*)xdisplay);
  if (x11_load_functions())
  {
    IupSetGlobal("XSCREEN", (char*)(long)_XDefaultScreen(xdisplay));
    IupSetGlobal("XSERVERVENDOR", _XServerVendor(xdisplay));
    IupSetInt(NULL, "XVENDORRELEASE", _XVendorRelease(xdisplay));
  }
  IupSetGlobal("GDK_WINDOWING", "X11");
#endif

#endif /* GTK_CHECK_VERSION(3, 0, 0) */
}

char* iupgtkGetNativeWindowHandleAttrib(Ihandle* ih)
{
  /* Used only in Canvas and Dialog */
  return iupgtkGetNativeWidgetHandle(ih->handle);
}

#if GTK_CHECK_VERSION(3, 0, 0)
static void gtkSetGlobalColorAttrib(const char* name, GdkRGBA *color)
{
  iupGlobalSetDefaultColorAttrib(name, (int)iupgtkColorFromDouble(color->red),
                                       (int)iupgtkColorFromDouble(color->green),
                                       (int)iupgtkColorFromDouble(color->blue));
}
#else
static void gtkSetGlobalColorAttrib(const char* name, GdkColor *color)
{
  iupGlobalSetDefaultColorAttrib(name, (int)iupCOLOR16TO8(color->red),
                                       (int)iupCOLOR16TO8(color->green),
                                       (int)iupCOLOR16TO8(color->blue));
}
#endif

static void gtkUpdateGlobalColors(GtkWidget* dialog, GtkWidget* text)
{
#if GTK_CHECK_VERSION(3, 0, 0)
#ifdef XWIN32  /* TODO: Workaround for GTK3 on Win32, should remove this code in the future */
#define gtkColorToRGBA(color, color3) {color3.red = color.red/65535.0; color3.green = color.green/65535.0; color3.blue = color.blue/65535.0; color3.alpha = 1.0;}

  GdkRGBA color3;
  GtkStyle* style = gtk_widget_get_style(dialog);

  GdkColor color = style->bg[GTK_STATE_NORMAL];
  gtkColorToRGBA(color, color3);
  gtkSetGlobalColorAttrib("DLGBGCOLOR", &color3);

  color = style->fg[GTK_STATE_NORMAL];
  gtkColorToRGBA(color, color3);
  gtkSetGlobalColorAttrib("DLGFGCOLOR", &color3);

  style = gtk_widget_get_style(text);

  color = style->base[GTK_STATE_NORMAL];
  gtkColorToRGBA(color, color3);
  gtkSetGlobalColorAttrib("TXTBGCOLOR", &color3);

  color = style->text[GTK_STATE_NORMAL];
  gtkColorToRGBA(color, color3);
  gtkSetGlobalColorAttrib("TXTFGCOLOR", &color3);

  color = style->base[GTK_STATE_SELECTED];
  gtkColorToRGBA(color, color3);
  gtkSetGlobalColorAttrib("TXTHLCOLOR", &color3);
#else /* Not Windows */
  GdkRGBA color;
  GtkStyleContext* context = gtk_widget_get_style_context(dialog);

  gtk_style_context_get_background_color(context, GTK_STATE_FLAG_NORMAL, &color);
  gtkSetGlobalColorAttrib("DLGBGCOLOR", &color);

  gtk_style_context_get_color(context, GTK_STATE_FLAG_NORMAL, &color);
  gtkSetGlobalColorAttrib("DLGFGCOLOR", &color);

  context = gtk_widget_get_style_context(text);

  gtk_style_context_get_color(context, GTK_STATE_FLAG_NORMAL, &color);
  gtkSetGlobalColorAttrib("TXTFGCOLOR", &color);

  gtk_style_context_get_background_color(context, GTK_STATE_FLAG_NORMAL, &color);
  if (color.alpha == 0) { color.red = 1; color.green = 1; color.blue = 1; }  /* TODO: workaround for GTK > 3.14 */
  gtkSetGlobalColorAttrib("TXTBGCOLOR", &color);

  gtk_style_context_get_background_color(context, GTK_STATE_FLAG_SELECTED, &color);
  gtkSetGlobalColorAttrib("TXTHLCOLOR", &color);
#endif
#else /* GTK 2.xx */
  GtkStyle* style = gtk_widget_get_style(dialog);

  GdkColor color = style->bg[GTK_STATE_NORMAL];
  gtkSetGlobalColorAttrib("DLGBGCOLOR", &color);

  color = style->fg[GTK_STATE_NORMAL];
  gtkSetGlobalColorAttrib("DLGFGCOLOR", &color);

  style = gtk_widget_get_style(text);

  color = style->base[GTK_STATE_NORMAL];
  gtkSetGlobalColorAttrib("TXTBGCOLOR", &color);

  color = style->text[GTK_STATE_NORMAL];
  gtkSetGlobalColorAttrib("TXTFGCOLOR", &color);

  color = style->base[GTK_STATE_SELECTED];
  gtkSetGlobalColorAttrib("TXTHLCOLOR", &color);
#endif

  iupGlobalSetDefaultColorAttrib("LINKFGCOLOR", 0, 0, 238);
}

static void gtkSetGlobalColors(void)
{
  GtkWidget* dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  GtkWidget* text = gtk_entry_new();
  gtk_container_add((GtkContainer*)dialog, text);
  gtk_widget_show(text);
  gtk_widget_realize(text);
  gtk_widget_realize(dialog);
  gtkUpdateGlobalColors(dialog, text);
  gtk_widget_unrealize(dialog);
  gtk_widget_destroy(dialog);
}

/* #define IUPGTK_DEBUG */

#if defined(IUPGTK_DEBUG)
static void iupgtk_log(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data)
{
  const char* log_lvl = "";
  (void)user_data;

  if (!log_domain)
    log_domain = "";

  switch (log_level & G_LOG_LEVEL_MASK)
  {
  case G_LOG_LEVEL_ERROR:
    log_lvl = "ERROR";
    break;
  case G_LOG_LEVEL_CRITICAL:
    log_lvl = "CRITICAL";
    break;
  case G_LOG_LEVEL_WARNING:
    log_lvl = "WARNING";
    break;
  case G_LOG_LEVEL_MESSAGE:
    log_lvl = "MESSAGE";
    break;
  case G_LOG_LEVEL_INFO:
    log_lvl = "INFO";
    break;
  case G_LOG_LEVEL_DEBUG:
    log_lvl = "DEBUG";
    break;
  }

  IupMessagef("IUPGTK Log", "%s [%s] %s", log_domain, log_lvl, message);
}
#endif

int iupdrvOpen(int *argc, char ***argv)
{
  char* value;

  if (!gtk_init_check(argc, argv))
    return IUP_ERROR;

  /* reset to the C default numeric locale after gtk_init */
  setlocale(LC_NUMERIC, "C");

#if defined(IUPGTK_DEBUG)
  g_log_set_default_handler(iupgtk_log, NULL);
#endif

  IupSetGlobal("DRIVER", "GTK");

  IupStoreGlobal("SYSTEMLANGUAGE", pango_language_to_string(gtk_get_default_language()));

  /* driver system version */
  IupSetfAttribute(NULL, "GTKVERSION", "%d.%d.%d", gtk_major_version,
                                                   gtk_minor_version,
                                                   gtk_micro_version);
  IupSetfAttribute(NULL, "GTKDEVVERSION", "%d.%d.%d", GTK_MAJOR_VERSION,
                                                      GTK_MINOR_VERSION,
                                                      GTK_MICRO_VERSION);

  if (argv && *argv && (*argv)[0] && (*argv)[0][0] != 0)
    IupStoreGlobal("ARGV0", (*argv)[0]);

  gtkSetGlobalAttrib();

  gtkSetGlobalColors();

  IupSetGlobal("SHOWMENUIMAGES", "YES");

  value = getenv("UBUNTU_MENUPROXY");  /* for now only in Ubuntu */
  if (value && (iupStrEqualNoCase(value, "libappmenu.so") || iupStrEqualNoCase(value, "1")))
    IupSetGlobal("GLOBALMENU", "Yes");

  return IUP_NOERROR;
}

void iupdrvClose(void)
{
  iupgtkStrRelease();
}
