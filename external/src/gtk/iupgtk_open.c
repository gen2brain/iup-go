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

#include "iup.h"

#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_object.h"
#include "iup_globalattrib.h"

#include "iupgtk_drv.h"


#if defined(GDK_NULL)   /******************************** Dummy definitions ************************************/

char* iupgtkGetNativeWidgetHandle(GtkWidget *widget)
{
  return NULL;
}

const char* iupgtkGetNativeWindowHandleName(void)
{
  return "????";
}

const char* iupgtkGetNativeFontIdName(void)
{
  return "????";
}

void* iupgtkGetNativeGraphicsContext(GtkWidget* widget)
{
  return NULL;
}

void iupgtkReleaseNativeGraphicsContext(GtkWidget* widget, void* gc)
{
}

IUP_SDK_API void* iupdrvGetDisplay(void)
{
  return NULL;
}

void iupgtkPushVisualAndColormap(void* visual, void* colormap)
{
}

static void gtkSetGlobalAttrib(void)
{
}

#elif defined(GDK_WINDOWING_QUARTZ)   /******************************** MacOSX ************************************/
#include <gdk/gdk.h>

char* iupgtkGetNativeWidgetHandle(GtkWidget *widget)
{
  GdkWindow* window = iupgtkGetWindow(widget);
  if (window)
    return (char*)window;
  else
    return NULL;
}

const char* iupgtkGetNativeWindowHandleName(void)
{
  return "????";
}

const char* iupgtkGetNativeFontIdName(void)
{
  return "????";
}

void* iupgtkGetNativeGraphicsContext(GtkWidget* widget)
{
  return (void*)gdk_gc_new((GdkDrawable*)iupgtkGetWindow(widget));
}

void iupgtkReleaseNativeGraphicsContext(GtkWidget* widget, void* gc)
{
  g_object_unref(gc);
  (void)widget;
}

IUP_SDK_API void* iupdrvGetDisplay(void)
{
  GdkDisplay* display = gdk_display_get_default();
  return display;
}

void iupgtkPushVisualAndColormap(void* visual, void* colormap)
{
#if GTK_CHECK_VERSION(3, 0, 0)
  (void)visual;
  (void)colormap;
#else
  GdkColormap* gdk_colormap;
  GdkVisual *gdk_visual = gdk_visual_get_best();

  gdk_colormap = gdk_colormap_new(gdk_visual, FALSE);

  gtk_widget_push_colormap(gdk_colormap);

  /* gtk_widget_push_visual is now deprecated */
#endif
}

static void gtkSetGlobalAttrib(void)
{
}

#elif defined(GDK_WINDOWING_WIN32)   /******************************** Windows ************************************/
#include <gdk/gdkwin32.h>

char* iupgtkGetNativeWidgetHandle(GtkWidget *widget)
{
  GdkWindow* window = iupgtkGetWindow(widget);
  if (window)
    return (char*)GDK_WINDOW_HWND(window);
  else
    return NULL;
}

const char* iupgtkGetNativeWindowHandleName(void)
{
  return "HWND";
}

const char* iupgtkGetNativeFontIdName(void)
{
  return "HFONT";
}

void* iupgtkGetNativeGraphicsContext(GtkWidget* widget)
{
  return GetDC(GDK_WINDOW_HWND(iupgtkGetWindow(widget)));
}

void iupgtkReleaseNativeGraphicsContext(GtkWidget* widget, void* gc)
{
  ReleaseDC(GDK_WINDOW_HWND(iupgtkGetWindow(widget)), (HDC)gc);
}

IUP_SDK_API void* iupdrvGetDisplay(void)
{
  return NULL;
}

void iupgtkPushVisualAndColormap(void* visual, void* colormap)
{
  (void)visual;
  (void)colormap;
}

static void gtkSetGlobalAttrib(void)
{
}

#elif defined(GDK_WINDOWING_X11)          /******************************** X11 ************************************/
#include <gdk/gdkx.h>

char* iupgtkGetNativeWidgetHandle(GtkWidget *widget)
{
  GdkWindow* window = iupgtkGetWindow(widget);
  if (window)
    return (char*)GDK_WINDOW_XID(window);
  else
    return NULL;
}

const char* iupgtkGetNativeWindowHandleName(void)
{
  return "XWINDOW";
}

const char* iupgtkGetNativeFontIdName(void)
{
  return "XFONTID";
}

void* iupgtkGetNativeGraphicsContext(GtkWidget* widget)
{
  GdkDisplay* display = gdk_display_get_default();
  return (void*)XCreateGC(GDK_DISPLAY_XDISPLAY(display), GDK_WINDOW_XID(iupgtkGetWindow(widget)), 0, NULL);
}

void iupgtkReleaseNativeGraphicsContext(GtkWidget* widget, void* gc)
{
  GdkDisplay* display = gdk_display_get_default();
  XFreeGC(GDK_DISPLAY_XDISPLAY(display), (GC)gc);
  (void)widget;
}

IUP_SDK_API void* iupdrvGetDisplay(void)
{
  GdkDisplay* display = gdk_display_get_default();
  return GDK_DISPLAY_XDISPLAY(display);
}

void iupgtkPushVisualAndColormap(void* visual, void* colormap)
{
#if GTK_CHECK_VERSION(3, 0, 0)
  (void)visual;
  (void)colormap;
#else
  GdkColormap* gdk_colormap;
#if GTK_CHECK_VERSION(2, 24, 0)
  GdkScreen* screen = gdk_screen_get_default();
  GdkVisual* gdk_visual = gdk_x11_screen_lookup_visual(screen, XVisualIDFromVisual((Visual*)visual));
#else
  GdkVisual* gdk_visual = gdkx_visual_get(XVisualIDFromVisual((Visual*)visual));
#endif
  if (colormap)
    gdk_colormap = gdk_x11_colormap_foreign_new(gdk_visual, (Colormap)colormap);
  else
    gdk_colormap = gdk_colormap_new(gdk_visual, FALSE);

  gtk_widget_push_colormap(gdk_colormap);

  /* gtk_widget_push_visual is now deprecated */
#endif
}

static void gtkSetGlobalAttrib(void)
{
  GdkDisplay* display = gdk_display_get_default();
  Display* xdisplay = GDK_DISPLAY_XDISPLAY(display);
  IupSetGlobal("XDISPLAY", (char*)xdisplay);
  IupSetGlobal("XSCREEN", (char*)XDefaultScreen(xdisplay));
  IupSetGlobal("XSERVERVENDOR", ServerVendor(xdisplay));
  IupSetInt(NULL, "XVENDORRELEASE", VendorRelease(xdisplay));
}

#endif

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

/* TODO:  (deprecated)

foreground-gdk => -rgba
background-gdk

gtk_widget_reparent has been deprecated since version 3.14 and should not be used in newly-written code.
Use gtk_container_remove() and gtk_container_add().
*/
