/** \file
 * \brief IupDialog class for GTK4
 *
 * See Copyright Notice in "iup.h"
 */

#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/wayland/gdkwayland.h>
#endif

#include <stdlib.h>
#include <limits.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_class.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_str.h"
#define _IUPDLG_PRIVATE
#include "iup_dialog.h"
#include "iup_image.h"

#include "iupgtk4_drv.h"
#include "iupgtk4_x11.h"


static void gtk4DialogSetMinMax(Ihandle* ih, int min_w, int min_h, int max_w, int max_h);

static void gtk4DialogChildDestroyEvent(GtkWindow* window, Ihandle* ih)
{
  (void)window;

  if (iupObjectCheck(ih))
    IupDestroy(ih);
}

IUP_SDK_API void iupdrvDialogSetParent(Ihandle* ih, InativeHandle* parent)
{
  GtkWindow* old_transient = gtk_window_get_transient_for((GtkWindow*)ih->handle);
  if (old_transient)
    g_signal_handlers_disconnect_by_func(old_transient, gtk4DialogChildDestroyEvent, ih);

  gtk_window_set_transient_for((GtkWindow*)ih->handle, (GtkWindow*)parent);

  g_signal_connect(G_OBJECT(parent), "destroy", G_CALLBACK(gtk4DialogChildDestroyEvent), ih);
}

IUP_SDK_API int iupdrvDialogIsVisible(Ihandle* ih)
{
  return iupdrvIsVisible(ih);
}

IUP_SDK_API void iupdrvDialogGetSize(Ihandle* ih, InativeHandle* handle, int* w, int* h)
{
  int width, height;
  int border = 0, caption = 0, menu;

  if (!handle)
    handle = ih->handle;

  gtk_window_get_default_size((GtkWindow*)handle, &width, &height);

  if (ih)
    iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

  if (w) *w = width + 2 * border;
  if (h) *h = height + 2 * border + caption;
}

IUP_SDK_API void iupdrvDialogSetVisible(Ihandle* ih, int visible)
{
  gtk_widget_set_visible(ih->handle, visible);
}

IUP_SDK_API void iupdrvDialogGetPosition(Ihandle* ih, InativeHandle* handle, int* x, int* y)
{
  if (!handle)
    handle = ih->handle;

  if (handle && GTK_IS_WIDGET(handle) && gtk_widget_get_visible(handle))
  {
    GdkSurface* surface = iupgtk4GetSurface(handle);

#ifdef GDK_WINDOWING_X11
    if (surface && iupgtk4X11IsBackend())
    {
      int gx = 0, gy = 0;
      if (iupgtk4X11GetWindowPosition(surface, &gx, &gy))
      {
        if (x) *x = gx;
        if (y) *y = gy;
        return;
      }
    }
#endif

    if (surface)
    {
      double dx, dy;
      iupgtk4SurfaceGetPointer(surface, &dx, &dy, NULL);
      if (x) *x = (int)dx;
      if (y) *y = (int)dy;
    }
  }
  else if (ih)
  {
    if (x) *x = iupAttribGetInt(ih, "_IUPGTK4_OLD_X");
    if (y) *y = iupAttribGetInt(ih, "_IUPGTK4_OLD_Y");
  }
}

IUP_SDK_API void iupdrvDialogSetPosition(Ihandle* ih, int x, int y)
{
#ifdef GDK_WINDOWING_X11
  if (iupgtk4X11IsBackend())
  {
    GdkSurface* surface = iupgtk4GetSurface(ih->handle);
    if (surface && iupgtk4X11MoveWindow(surface, x, y))
      return;
  }
#endif

  /* On Wayland and other backends, window positioning is not supported */
  (void)ih;
  (void)x;
  (void)y;
}

static int gtk4DialogGetMenuSize(Ihandle* ih)
{
  /* Menu bar is inside the window content area (in VBox), not external decoration.
   * GTK4 VBox automatically positions inner_parent below the menu bar. */
  (void)ih;
  return 0;
}

static int gtk4DialogGetMenuBarSize(Ihandle* ih)
{
  if (ih->data->menu && !iupStrBoolean(IupGetGlobal("GLOBALMENU")))
    return iupdrvMenuGetMenuBarSize(ih->data->menu);
  else
    return 0;
}

static void gtk4DialogGetWindowDecor(Ihandle* ih, int* win_border, int* win_caption)
{
  GdkSurface* surface = iupgtk4GetSurface(ih->handle);
  GtkWidget* child;

  *win_border = 0;
  *win_caption = 0;

  if (!surface)
    return;

  child = gtk_window_get_child(GTK_WINDOW(ih->handle));
  if (child && gtk_widget_get_width(child) > 0 && gtk_widget_get_height(child) > 0)
  {
    int window_width = gtk_widget_get_width(ih->handle);
    int window_height = gtk_widget_get_height(ih->handle);
    int child_width = gtk_widget_get_width(child);
    int child_height = gtk_widget_get_height(child);

    *win_caption = window_height - child_height;
    if (*win_caption < 0) *win_caption = 0;

    *win_border = (window_width - child_width) / 2;
    if (*win_border < 0) *win_border = 0;
  }
}

IUP_SDK_API void iupdrvDialogGetDecoration(Ihandle* ih, int* border, int* caption, int* menu)
{
  static int native_caption = 0;
  static int native_border = 0;

  int has_titlebar = iupAttribGetBoolean(ih, "RESIZE") ||
                     iupAttribGetBoolean(ih, "MAXBOX") ||
                     iupAttribGetBoolean(ih, "MINBOX") ||
                     iupAttribGetBoolean(ih, "MENUBOX") ||
                     iupAttribGet(ih, "TITLE");

  int has_border = has_titlebar ||
                   iupAttribGetBoolean(ih, "RESIZE") ||
                   iupAttribGetBoolean(ih, "BORDER");

  *menu = gtk4DialogGetMenuBarSize(ih);

  if (ih->handle && ih->currentheight > 0)
  {
    GtkWidget* root = GTK_WIDGET(ih->handle);
    gboolean has_csd = gtk_widget_has_css_class(root, "csd");
    gboolean has_solid_csd = gtk_widget_has_css_class(root, "solid-csd");

    if (has_csd && !has_solid_csd)
    {
      /* CSD with shadow: currentheight is Fixed container (content area).
       * Return caption=0 so IUP doesn't subtract it again for children. */
      *border = 0;
      *caption = 0;
      return;
    }
  }

  if (ih->handle && iupdrvIsVisible(ih))
  {
    int win_border = 0, win_caption = 0;

    gtk4DialogGetWindowDecor(ih, &win_border, &win_caption);

    if (win_border >= 0 && win_caption >= 0)
    {
      *border = 0;
      if (has_border)
        *border = win_border;

      *caption = 0;
      if (has_titlebar)
        *caption = win_caption;

      if (win_border > 0 && win_border < 100)
        native_border = win_border;
      if (win_caption > 0 && win_caption < 200)
        native_caption = win_caption;

      if (iupAttribGetBoolean(ih, "HIDETITLEBAR"))
        *caption = 0;

      return;
    }
  }

  *border = 0;
  if (has_border)
    *border = (native_border)? native_border: 0;

  *caption = 0;
  if (has_titlebar)
    *caption = (native_caption)? native_caption: 0;

  if (iupAttribGetBoolean(ih, "HIDETITLEBAR"))
    *caption = 0;
}

IUP_SDK_API int iupdrvDialogSetPlacement(Ihandle* ih)
{
  char* placement;
  int old_state = ih->data->show_state;
  ih->data->show_state = IUP_SHOW;

  if (iupAttribGetBoolean(ih, "FULLSCREEN"))
  {
    gtk_window_fullscreen((GtkWindow*)ih->handle);
    return 1;
  }

  placement = iupAttribGet(ih, "PLACEMENT");
  if (!placement)
  {
    if (old_state == IUP_MAXIMIZE || old_state == IUP_MINIMIZE)
      ih->data->show_state = IUP_RESTORE;

    gtk_window_unmaximize((GtkWindow*)ih->handle);
    gtk_window_unminimize((GtkWindow*)ih->handle);

    if (iupAttribGetBoolean(ih, "CUSTOMFRAME") && iupDialogCustomFrameRestore(ih))
    {
      ih->data->show_state = IUP_RESTORE;
      return 1;
    }

    return 0;
  }

  if (iupAttribGetBoolean(ih, "CUSTOMFRAME") && iupStrEqualNoCase(placement, "MAXIMIZED"))
  {
    iupDialogCustomFrameMaximize(ih);
    iupAttribSet(ih, "PLACEMENT", NULL);
    ih->data->show_state = IUP_MAXIMIZE;
    return 1;
  }

  if (iupStrEqualNoCase(placement, "MINIMIZED"))
  {
    ih->data->show_state = IUP_MINIMIZE;
    gtk_window_minimize((GtkWindow*)ih->handle);
  }
  else if (iupStrEqualNoCase(placement, "MAXIMIZED"))
  {
    ih->data->show_state = IUP_MAXIMIZE;
    gtk_window_maximize((GtkWindow*)ih->handle);
  }
  else if (iupStrEqualNoCase(placement, "FULL"))
  {
    int width, height, x, y;
    int border, caption, menu;
    iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

    x = -(border);
    y = -(border + caption + menu);

    iupdrvGetFullSize(&width, &height);

    height += menu;

    iupdrvDialogSetPosition(ih, x, y);
    gtk_window_set_default_size((GtkWindow*)ih->handle, width, height);

    if (old_state == IUP_MAXIMIZE || old_state == IUP_MINIMIZE)
      ih->data->show_state = IUP_RESTORE;
  }

  iupAttribSet(ih, "PLACEMENT", NULL);
  return 1;
}

/****************************************************************
                     Callbacks and Events
****************************************************************/

IUP_DRV_API gboolean iupgtk4DialogCloseRequest(GtkWindow* window, Ihandle* ih)
{
  Icallback cb;
  (void)window;

  if (!iupdrvIsActive(ih))
    return TRUE;

  cb = IupGetCallback(ih, "CLOSE_CB");
  if (cb)
  {
    int ret = cb(ih);
    if (ret == IUP_IGNORE)
      return TRUE;
    if (ret == IUP_CLOSE)
      IupExitLoop();
  }

  IupHide(ih);

  return TRUE;
}

void gtk4DialogSizeAllocate(GtkWidget* widget, int width, int height, int baseline, Ihandle* ih)
{
  IFnii cb;
  int border, caption, menu;
  int client_width, client_height;
  (void)widget;
  (void)baseline;

  if (ih->data->ignore_resize)
    return;

  /* Get decoration sizes - we need menu height for total size calculation */
  iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

  ih->currentwidth = width;
  ih->currentheight = height + menu;

  client_width = ih->currentwidth - 2 * border;
  client_height = ih->currentheight;

  cb = (IFnii)IupGetCallback(ih, "RESIZE_CB");
  if (!cb || cb(ih, client_width, client_height - menu) != IUP_IGNORE)
  {
    ih->data->ignore_resize = 1;
    IupRefresh(ih);
    ih->data->ignore_resize = 0;
  }
}

static void gtk4DialogThemeChanged(GtkSettings* settings, GParamSpec* pspec, Ihandle* ih)
{
  int dark_mode;
  IFni cb;

  (void)settings;
  (void)pspec;

  if (!iupObjectCheck(ih))
    return;

  iupgtk4SetGlobalColors();

  dark_mode = iupgtk4IsSystemDarkMode();

  cb = (IFni)IupGetCallback(ih, "THEMECHANGED_CB");
  if (cb)
    cb(ih, dark_mode);
}

/****************************************************************
                     Idialog Methods
****************************************************************/

static void gtk4DialogSetChildrenPositionMethod(Ihandle* ih, int x, int y)
{
  if (ih->firstchild)
  {
    char* offset = iupAttribGet(ih, "CHILDOFFSET");

    x = 0;
    y = 0;

    if (offset) iupStrToIntInt(offset, &x, &y, 'x');

    y += gtk4DialogGetMenuSize(ih);

    if (ih->firstchild)
      iupBaseSetPosition(ih->firstchild, x, y);
  }
}

static void* gtk4DialogGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
  (void)child;
  return (void*)gtk_window_get_child((GtkWindow*)ih->handle);
}

static void gtk4DialogWindowStateChanged(GObject* window, GParamSpec* pspec, Ihandle* ih)
{
  int state = -1;
  gboolean is_maximized;
  (void)pspec;
  (void)window;

  is_maximized = gtk_window_is_maximized(GTK_WINDOW(ih->handle));

  iupAttribSet(ih, "MAXIMIZED", NULL);
  iupAttribSet(ih, "MINIMIZED", NULL);

  if (is_maximized)
  {
    state = IUP_MAXIMIZE;
    iupAttribSet(ih, "MAXIMIZED", "Yes");
  }
  else
  {
    /* If not maximized, it's either minimized or restored. Since this signal only notifies on 'maximized',
       a 'false' value means it's now restored. */
    state = IUP_RESTORE;
  }

  if (ih->data->show_state != state)
  {
    IFni cb;
    ih->data->show_state = state;

    cb = (IFni)IupGetCallback(ih, "SHOW_CB");
    if (cb && cb(ih, state) == IUP_CLOSE)
      IupExitLoop();
  }
}

static int gtk4DialogMapMethod(Ihandle* ih)
{
  InativeHandle* parent;
  GtkWidget* inner_parent;
  int has_titlebar = 0;

  ih->handle = gtk_window_new();
  if (!ih->handle)
    return IUP_ERROR;

  parent = iupDialogGetNativeParent(ih);
  if (parent)
  {
    gtk_window_set_transient_for((GtkWindow*)ih->handle, (GtkWindow*)parent);
    g_signal_connect(G_OBJECT(parent), "destroy", G_CALLBACK(gtk4DialogChildDestroyEvent), ih);
  }

  g_signal_connect(G_OBJECT(ih->handle), "close-request", G_CALLBACK(iupgtk4DialogCloseRequest), ih);

  {
    int init_w = ih->userwidth > 0 ? ih->userwidth : 100;
    int init_h = ih->userheight > 0 ? ih->userheight : 100;
    gtk_window_set_default_size((GtkWindow*)ih->handle, init_w, init_h);
  }

  if (iupAttribGetBoolean(ih, "DIALOGHINT"))
    gtk_window_set_modal(GTK_WINDOW(ih->handle), TRUE);

  if (iupAttribGetBoolean(ih, "CUSTOMFRAME"))
  {
    iupDialogCustomFrameSimulateCheckCallbacks(ih);
    iupAttribSet(ih, "HIDETITLEBAR", "Yes");
  }

  if (iupAttribGetBoolean(ih, "HIDETITLEBAR"))
    gtk_window_set_decorated((GtkWindow*)ih->handle, FALSE);

  inner_parent = iupgtk4NativeContainerNew();

  /* If dialog has menu, wrap inner_parent in a VBox to hold menu bar + content */
  if (ih->data->menu)
  {
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_set_spacing(GTK_BOX(vbox), 0);
    gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE);
    gtk_box_append(GTK_BOX(vbox), inner_parent);
    gtk_window_set_child((GtkWindow*)ih->handle, vbox);
    iupAttribSet(ih, "_IUPGTK4_MENU_BOX", (char*)vbox);
  }
  else
  {
    gtk_window_set_child((GtkWindow*)ih->handle, inner_parent);
  }

  /* Set dialog handle in container for resize notifications */
  iupgtk4NativeContainerSetIhandle(inner_parent, ih);

  /* Store inner_parent for menu bar installation */
  iupAttribSet(ih, "_IUPGTK4_INNER_PARENT", (char*)inner_parent);

  gtk_widget_realize(ih->handle);

  if (iupAttribGet(ih, "TITLE"))
    has_titlebar = 1;

  if (iupAttribGetBoolean(ih, "RESIZE"))
  {
    gtk_window_set_resizable((GtkWindow*)ih->handle, TRUE);
  }
  else
  {
    iupAttribSet(ih, "MAXBOX", "NO");
    gtk_window_set_resizable((GtkWindow*)ih->handle, FALSE);
  }

  if (iupAttribGetBoolean(ih, "MENUBOX") ||
      iupAttribGetBoolean(ih, "MAXBOX") ||
      iupAttribGetBoolean(ih, "MINBOX"))
    has_titlebar = 1;

  if (has_titlebar)
    gtk_window_set_title((GtkWindow*)ih->handle, "");
  else if (!iupAttribGetBoolean(ih, "BORDER") && !iupAttribGetBoolean(ih, "HIDETITLEBAR"))
    gtk_window_set_decorated((GtkWindow*)ih->handle, FALSE);

  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSet(ih, "DROPFILESTARGET", "YES");

  gtk4DialogSetMinMax(ih, 1, 1, 65535, 65535);

  iupAttribSet(ih, "VISIBLE", NULL);

  GtkSettings* settings = gtk_settings_get_default();
  gulong handler_id;

  handler_id = g_signal_connect(G_OBJECT(settings), "notify::gtk-theme-name", G_CALLBACK(gtk4DialogThemeChanged), ih);
  iupAttribSet(ih, "_IUPGTK4_THEME_HANDLER1", (char*)(uintptr_t)handler_id);

  handler_id = g_signal_connect(G_OBJECT(settings), "notify::gtk-application-prefer-dark-theme", G_CALLBACK(gtk4DialogThemeChanged), ih);
  iupAttribSet(ih, "_IUPGTK4_THEME_HANDLER2", (char*)(uintptr_t)handler_id);

  /* Monitor window state changes (maximize/minimize/restore) */
  handler_id = g_signal_connect(G_OBJECT(ih->handle), "notify::maximized", G_CALLBACK(gtk4DialogWindowStateChanged), ih);
  iupAttribSet(ih, "_IUPGTK4_MAXIMIZE_HANDLER", (char*)(uintptr_t)handler_id);

  /* Add key controller for DEFAULTENTER/DEFAULTESC support */
  {
    GtkEventController* key_controller = gtk_event_controller_key_new();
    g_signal_connect(key_controller, "key-pressed", G_CALLBACK(iupgtk4KeyPressEvent), ih);
    gtk_widget_add_controller(GTK_WIDGET(ih->handle), key_controller);
  }

  /* Build and install menu bar if present */
  if (ih->data->menu)
  {
    iupgtk4DialogSetMenuBar(ih, ih->data->menu);
  }

  return IUP_NOERROR;
}

static void gtk4DialogUnMapMethod(Ihandle* ih)
{
  if (ih->data->menu)
  {
    ih->data->menu->handle = NULL;
    IupDestroy(ih->data->menu);
    ih->data->menu = NULL;
  }

  gulong handler_id;
  GtkSettings* settings = gtk_settings_get_default();

  handler_id = (gulong)(uintptr_t)iupAttribGet(ih, "_IUPGTK4_THEME_HANDLER1");
  if (handler_id != 0)
    g_signal_handler_disconnect(G_OBJECT(settings), handler_id);

  handler_id = (gulong)(uintptr_t)iupAttribGet(ih, "_IUPGTK4_THEME_HANDLER2");
  if (handler_id != 0)
    g_signal_handler_disconnect(G_OBJECT(settings), handler_id);

  handler_id = (gulong)(uintptr_t)iupAttribGet(ih, "_IUPGTK4_MAXIMIZE_HANDLER");
  if (handler_id != 0)
    g_signal_handler_disconnect(G_OBJECT(ih->handle), handler_id);

  g_signal_handlers_disconnect_by_data(G_OBJECT(ih->handle), ih);

  InativeHandle* parent = iupDialogGetNativeParent(ih);
  if (parent)
    g_signal_handlers_disconnect_by_func(G_OBJECT(parent), gtk4DialogChildDestroyEvent, ih);

  if (!ih->handle || !GTK_IS_WIDGET(ih->handle))
    return;

  /* gtk_window_destroy() handles all child cleanup automatically.
   * Do not manually unrealize or unparent children - this causes crashes
   * as gtk_window_destroy() will try to clean them up again. */
  gtk_window_destroy((GtkWindow*)ih->handle);
}

static void gtk4DialogLayoutUpdateMethod(Ihandle* ih)
{
  int border, caption, menu;
  int width, height;

  if (ih->data->ignore_resize)
  {
    return;
  }

  ih->data->ignore_resize = 1;

  iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

  /* Check if we have CSD (Client-Side Decorations) */
  gboolean has_csd = gtk_widget_has_css_class(GTK_WIDGET(ih->handle), "csd");

  width = ih->currentwidth - 2 * border;

  /*
   * GTK4 HEIGHT calculation (titlebar):
   * The titlebar is a child widget of the window. GTK's measure function
   * calculates window height as: titlebar_height + content_height.
   *
   * - With SSD: No titlebar widget, caption=0. Window height = content height.
   * - With CSD: Titlebar widget exists. We must ADD caption to get total window height.
   */
  if (has_csd && caption > 0)
  {
    /* CSD with measured caption: Add caption to get correct window size */
    height = ih->currentheight + caption;
  }
  else if (has_csd && caption == 0)
  {
    /* CSD before window visible: Estimate caption */
    int estimated_caption = 37;
    height = ih->currentheight + estimated_caption;
  }
  else
  {
    height = ih->currentheight - caption;
  }

  if (width <= 0) width = 1;
  if (height <= 0) height = 1;

  /* Use gtk_window_set_default_size() to resize window */
  gtk_window_set_default_size((GtkWindow*)ih->handle, width, height);

  if (!iupAttribGetBoolean(ih, "RESIZE"))
  {
    int client_width, client_height;

    client_width = ih->currentwidth - 2 * border;
    client_height = ih->currentheight - caption;

    if (client_width <= 0) client_width = 1;
    if (client_height <= 0) client_height = 1;

    gtk_widget_set_size_request(ih->handle, client_width, client_height);
  }

  ih->data->ignore_resize = 0;
}

/****************************************************************************
                                   Attributes
****************************************************************************/

static void gtk4DialogSetMinMax(Ihandle* ih, int min_w, int min_h, int max_w, int max_h)
{
  int border = 0, caption = 0, menu = 0;
  int decorwidth, decorheight;

  (void)max_w;
  (void)max_h;

  iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

  decorwidth = 2 * border;
  decorheight = caption;

  if (min_w > decorwidth)
    min_w = min_w - decorwidth;
  else
    min_w = 1;

  if (min_h > decorheight)
    min_h = min_h - decorheight;
  else
    min_h = 1;

  /* GTK4 removed gtk_window_set_geometry_hints, max size constraints are not supported */
  gtk_widget_set_size_request(ih->handle, min_w, min_h);
}

static int gtk4DialogSetMinSizeAttrib(Ihandle* ih, const char* value)
{
  int min_w = 1, min_h = 1;
  int max_w = 65535, max_h = 65535;
  iupStrToIntInt(value, &min_w, &min_h, 'x');

  iupStrToIntInt(iupAttribGet(ih, "MAXSIZE"), &max_w, &max_h, 'x');

  gtk4DialogSetMinMax(ih, min_w, min_h, max_w, max_h);

  return iupBaseSetMinSizeAttrib(ih, value);
}

static int gtk4DialogSetMaxSizeAttrib(Ihandle* ih, const char* value)
{
  int min_w = 1, min_h = 1;
  int max_w = 65535, max_h = 65535;
  iupStrToIntInt(value, &max_w, &max_h, 'x');

  iupStrToIntInt(iupAttribGet(ih, "MINSIZE"), &min_w, &min_h, 'x');

  gtk4DialogSetMinMax(ih, min_w, min_h, max_w, max_h);

  return iupBaseSetMaxSizeAttrib(ih, value);
}

static int gtk4DialogSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    value = "";
  gtk_window_set_title((GtkWindow*)ih->handle, iupgtk4StrConvertToSystem(value));
  return 0;
}

static int gtk4DialogSetIconAttrib(Ihandle* ih, const char* value)
{
  if (!value)
  {
    gtk_window_set_icon_name(GTK_WINDOW(ih->handle), NULL);
    return 0;
  }

  /* GTK4 only supports named theme icons for window icons.
     Try it as a theme icon name. */
  gtk_window_set_icon_name(GTK_WINDOW(ih->handle), value);
  return 1;
}

static int gtk4DialogSetFullScreenAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
  {
    if (!iupAttribGet(ih, "_IUPGTK4_FS_STYLE"))
    {
      iupAttribSet(ih, "_IUPGTK4_FS_STYLE", "YES");
      gtk_window_fullscreen((GtkWindow*)ih->handle);
    }
  }
  else
  {
    iupAttribSet(ih, "_IUPGTK4_FS_STYLE", NULL);
    gtk_window_unfullscreen((GtkWindow*)ih->handle);
  }
  return 0;
}

static char* gtk4DialogGetClientSizeAttrib(Ihandle* ih)
{
  int width, height;
  int border, caption, menu;

  if (iupdrvIsVisible(ih))
  {
    width = gtk_widget_get_width(ih->handle);
    height = gtk_widget_get_height(ih->handle);
  }
  else
  {
    gtk_window_get_default_size((GtkWindow*)ih->handle, &width, &height);
  }

  iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

  width = width - 2 * border;
  height = height - caption - menu;

  if (width < 0) width = 0;
  if (height < 0) height = 0;

  return iupStrReturnIntInt(width, height, 'x');
}

static char* gtk4DialogGetClientOffsetAttrib(Ihandle* ih)
{
  int border, caption, menu;
  iupdrvDialogGetDecoration(ih, &border, &caption, &menu);
  return iupStrReturnIntInt(border, caption + menu, 'x');
}

static char* gtk4DialogGetActiveWindowAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(gtk_window_is_active(GTK_WINDOW(ih->handle)));
}


static int gtk4DialogSetOpacityAttrib(Ihandle* ih, const char* value)
{
  int opacity;
  if (!iupStrToInt(value, &opacity))
    return 0;

  gtk_widget_set_opacity(ih->handle, (double)opacity / 255.0);
  return 0;
}

/* Helper function to convert GdkTexture to base64-encoded PNG data URI for CSS */
static char* gtk4DialogTextureToDataURI(GdkTexture* texture)
{
  GBytes* bytes = gdk_texture_save_to_png_bytes(texture);
  if (!bytes)
    return NULL;

  gsize buffer_size;
  gconstpointer buffer = g_bytes_get_data(bytes, &buffer_size);

  gchar* base64_data = g_base64_encode((const guchar*)buffer, buffer_size);
  g_bytes_unref(bytes);

  gchar* data_uri = g_strdup_printf("data:image/png;base64,%s", base64_data);
  g_free(base64_data);

  return data_uri;
}

static int gtk4DialogSetBackgroundAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "DLGBGCOLOR"))
    return 0;

  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
  {
    if (iupAttribGet(ih, "_IUPGTK4_HAS_BG_IMAGE"))
    {
      iupgtk4CssSetWidgetCustom(ih->handle, "background-image", "none");
      iupAttribSet(ih, "_IUPGTK4_HAS_BG_IMAGE", NULL);
    }

    iupgtk4SetBgColor(ih->handle, r, g, b);
    gtk_widget_queue_draw(ih->handle);
    return 1;
  }
  else
  {
    GdkTexture* texture = (GdkTexture*)iupImageGetImage(value, ih, 0, NULL);
    if (texture)
    {
      /* Use CSS with data: URI for background image */
      gchar* data_uri = gtk4DialogTextureToDataURI(texture);
      if (data_uri)
      {
        gchar* css_value;
        if (iupAttribGetBoolean(ih, "BACKIMAGEZOOM"))
          css_value = g_strdup_printf("url(\"%s\"); background-size: 100%% 100%%; background-repeat: no-repeat", data_uri);
        else
          css_value = g_strdup_printf("url(\"%s\"); background-repeat: repeat", data_uri);
        iupgtk4CssSetWidgetCustom(ih->handle, "background-image", css_value);
        iupAttribSet(ih, "_IUPGTK4_HAS_BG_IMAGE", "1");

        g_free(css_value);
        g_free(data_uri);

        gtk_widget_queue_draw(ih->handle);

        return 1;
      }
    }
  }

  return 0;
}

static int gtk4DialogSetHideTitleBarAttrib(Ihandle* ih, const char* value)
{
  gtk_window_set_decorated((GtkWindow*)ih->handle, !iupStrBoolean(value));
  return 1;
}

static int gtk4DialogSetBackImageZoomAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  if (iupAttribGet(ih, "_IUPGTK4_HAS_BG_IMAGE"))
  {
    char* background = iupAttribGet(ih, "BACKGROUND");
    if (background)
      gtk4DialogSetBackgroundAttrib(ih, background);
  }
  return 1;
}

static int gtk4DialogSetBringFrontAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    gtk_window_present(GTK_WINDOW(ih->handle));
  return 0;
}

IUP_SDK_API void iupdrvDialogInitClass(Iclass* ic)
{
  ic->Map = gtk4DialogMapMethod;
  ic->UnMap = gtk4DialogUnMapMethod;
  ic->LayoutUpdate = gtk4DialogLayoutUpdateMethod;
  ic->GetInnerNativeContainerHandle = gtk4DialogGetInnerNativeContainerHandleMethod;
  ic->SetChildrenPosition = gtk4DialogSetChildrenPositionMethod;

  iupClassRegisterCallback(ic, "THEMECHANGED_CB", "i");

  iupClassRegisterAttribute(ic, iupgtk4GetNativeWindowHandleName(), iupgtk4GetNativeWindowHandleAttrib, NULL, NULL, NULL, IUPAF_NO_INHERIT | IUPAF_NO_STRING);

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, "DLGBGCOLOR", NULL, IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "CLIENTSIZE", gtk4DialogGetClientSizeAttrib, iupDialogSetClientSizeAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_SAVE | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTOFFSET", gtk4DialogGetClientOffsetAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_DEFAULTVALUE | IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "TITLE", NULL, gtk4DialogSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BACKGROUND", NULL, gtk4DialogSetBackgroundAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKIMAGEZOOM", NULL, gtk4DialogSetBackImageZoomAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ICON", NULL, gtk4DialogSetIconAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FULLSCREEN", NULL, gtk4DialogSetFullScreenAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MINSIZE", NULL, gtk4DialogSetMinSizeAttrib, IUPAF_SAMEASSYSTEM, "1x1", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAXSIZE", NULL, gtk4DialogSetMaxSizeAttrib, IUPAF_SAMEASSYSTEM, "65535x65535", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SAVEUNDER", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAXIMIZED", NULL, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ACTIVEWINDOW", gtk4DialogGetActiveWindowAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOPMOST", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DIALOGHINT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OPACITY", NULL, gtk4DialogSetOpacityAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OPACITYIMAGE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHAPEIMAGE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUSTOMFRAME", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "HIDETITLEBAR", NULL, gtk4DialogSetHideTitleBarAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BRINGFRONT", NULL, gtk4DialogSetBringFrontAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMPOSITED", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CONTROL", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HELPBUTTON", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOOLBOX", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDIFRAME", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDICLIENT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDIMENU", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDICHILD", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
}
