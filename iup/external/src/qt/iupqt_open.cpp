/** \file
 * \brief Qt Driver Core - Initialization and Setup
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <clocale>

#if defined(__linux__)
#include <unistd.h>  /* for readlink() */
#endif

#include <QApplication>
#include <QWidget>
#include <QWindow>
#include <QPalette>
#include <QStyle>
#include <QStyleHints>
#include <QString>
#include <QGuiApplication>
#include <QScreen>
#include <QFileInfo>

/* Qt6 native interface handling */
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  /* Qt6: Native interface via qguiapplication_platform.h */
  #if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
    #include <QtGui/qguiapplication_platform.h>
  #endif

  /* Qt6.7+: QWaylandWindow available in private headers */
  #if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    #define IUP_QT_HAS_WAYLAND_WINDOW 1

    /* Forward declare wl_surface from Wayland client API */
    struct wl_surface;

    /* Forward declare QWaylandWindow native interface to avoid private header */
    QT_BEGIN_NAMESPACE
    namespace QNativeInterface {
      struct Q_GUI_EXPORT QWaylandWindow {
        QT_DECLARE_NATIVE_INTERFACE(QWaylandWindow, 1, QWindow)
        virtual struct wl_surface *surface() const = 0;
      };
    }
    QT_END_NAMESPACE
  #endif

  /* Qt6.5+: QWaylandApplication available */
  #if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    #define IUP_QT_HAS_WAYLAND_APP 1
  #endif
#endif

extern "C" {
#include "iup.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_object.h"
#include "iup_globalattrib.h"
}

#include "iupqt_drv.h"


static QApplication* qt_application = NULL;
static char* qt_str_buffer = NULL;

extern "C" void iupqtLoopCleanup(void);

/****************************************************************************
 * Native Handle Access
 ****************************************************************************/

extern "C" char* iupqtGetNativeWidgetHandle(QWidget *widget)
{
  if (!widget)
    return NULL;

  QWindow* window = widget->windowHandle();
  if (!window)
    return NULL;  /* Widget not yet realized */

  QString platform = QGuiApplication::platformName();

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  /* ========== Qt6 Implementation using QNativeInterface ========== */

  if (platform == "xcb")
  {
    /* X11/XCB: winId() returns the X Window ID (XID) */
    WId xid = window->winId();
    return (char*)(uintptr_t)xid;
  }
  else if (platform == "wayland")
  {
#ifdef IUP_QT_HAS_WAYLAND_WINDOW
    /* Qt 6.7+: Use QNativeInterface::QWaylandWindow to get wl_surface */
    auto *waylandWindow = window->nativeInterface<QNativeInterface::QWaylandWindow>();
    if (waylandWindow)
    {
      void* wl_surface = waylandWindow->surface();
      return (char*)wl_surface;
    }
#endif
    /* Fallback: Wayland native interface not available */
    return NULL;
  }
  else if (platform == "windows")
  {
    /* Windows: winId() returns HWND */
    WId hwnd = window->winId();
    return (char*)(void*)(uintptr_t)hwnd;
  }
  else if (platform == "cocoa")
  {
    /* macOS: winId() returns NSView* wrapped in WId */
    WId nsview = window->winId();
    return (char*)(uintptr_t)nsview;
  }

#else
  /* ========== Qt5 Implementation using winId() ========== */
  /* Qt5: winId() returns platform-specific native handle (public API)
   * - X11/XCB: returns XID (Window/xcb_window_t)
   * - Windows: returns HWND
   * - macOS: returns NSView*
   * - Wayland: returns wl_surface* (may be limited)
   */
  WId native_id = window->winId();
  return (char*)(uintptr_t)native_id;

#endif  /* Qt version check */

  /* Unknown platform */
  return NULL;
}

extern "C" const char* iupqtGetNativeWindowHandleName(void)
{
  const char* platform = QGuiApplication::platformName().toUtf8().constData();

  if (strcmp(platform, "xcb") == 0)
    return "XWINDOW";

  if (strcmp(platform, "wayland") == 0)
    return "WL_SURFACE";

  if (strcmp(platform, "windows") == 0)
    return "HWND";

  if (strcmp(platform, "cocoa") == 0)
    return "NSVIEW";

  return "UNKNOWN";
}

extern "C" const char* iupqtGetNativeFontIdName(void)
{
  const char* platform = QGuiApplication::platformName().toUtf8().constData();

  if (strcmp(platform, "xcb") == 0)
    return "XFONTID";

  if (strcmp(platform, "windows") == 0)
    return "HFONT";

  /* Wayland and macOS use Qt's font system */
  return NULL;
}

extern "C" char* iupqtGetNativeWindowHandleAttrib(Ihandle* ih)
{
  return iupqtGetNativeWidgetHandle((QWidget*)ih->handle);
}

extern "C" void* iupqtGetNativeGraphicsContext(QWidget* widget)
{
  /* Qt uses QPainter, not native graphics contexts directly */
  /* This would need platform-specific implementation if needed */
  (void)widget;
  return NULL;
}

extern "C" void iupqtReleaseNativeGraphicsContext(QWidget* widget, void* gc)
{
  (void)widget;
  (void)gc;
}

/****************************************************************************
 * String Conversion (UTF-8 handling)
 ****************************************************************************/

extern "C" void iupqtStrRelease(void)
{
  if (qt_str_buffer)
  {
    free(qt_str_buffer);
    qt_str_buffer = NULL;
  }
}

extern "C" char* iupqtStrConvertToSystem(const char* str)
{
  /* Qt uses UTF-8 internally, so no conversion needed */
  return (char*)str;
}

extern "C" char* iupqtStrConvertToSystemLen(const char* str, int *len)
{
  /* Qt uses UTF-8 internally */
  if (len)
    *len = (int)strlen(str);
  return (char*)str;
}

extern "C" char* iupqtStrConvertFromSystem(const char* str)
{
  /* Qt uses UTF-8 internally */
  return (char*)str;
}

extern "C" char* iupqtStrConvertFromFilename(const char* str)
{
  /* Qt handles filenames correctly across platforms */
  return (char*)str;
}

extern "C" char* iupqtStrConvertToFilename(const char* str)
{
  /* Qt handles filenames correctly across platforms */
  return (char*)str;
}

extern "C" void iupqtStrSetUTF8Mode(int utf8mode)
{
  /* Qt is always UTF-8 */
  (void)utf8mode;
}

extern "C" int iupqtStrGetUTF8Mode(void)
{
  /* Qt is always UTF-8 */
  return 1;
}

/****************************************************************************
 * Global Attributes
 ****************************************************************************/

static void qtSetGlobalColorAttrib(const char* name, const QColor &color)
{
  iupGlobalSetDefaultColorAttrib(name, color.red(), color.green(), color.blue());
}

static void qtSetGlobalColors(void)
{
  QWidget dialog;
  QPalette palette = dialog.palette();

  qtSetGlobalColorAttrib("DLGBGCOLOR", palette.color(QPalette::Window));
  qtSetGlobalColorAttrib("DLGFGCOLOR", palette.color(QPalette::WindowText));
  qtSetGlobalColorAttrib("TXTBGCOLOR", palette.color(QPalette::Base));
  qtSetGlobalColorAttrib("TXTFGCOLOR", palette.color(QPalette::Text));
  qtSetGlobalColorAttrib("TXTHLCOLOR", palette.color(QPalette::Highlight));
  qtSetGlobalColorAttrib("MENUBGCOLOR", palette.color(QPalette::Window));
  qtSetGlobalColorAttrib("MENUFGCOLOR", palette.color(QPalette::WindowText));
  qtSetGlobalColorAttrib("LINKFGCOLOR", palette.color(QPalette::Link));
}

static void qtSetGlobalAttrib(void)
{
  QString platform = QGuiApplication::platformName();

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  /* ========== Qt6 Implementation using QNativeInterface ========== */

  if (platform == "xcb")
  {
    IupSetGlobal("QTPLATFORM", "X11");

#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0) && defined(Q_OS_LINUX)
    /* Qt 6.2+: Use QX11Application native interface (Linux only) */
    if (auto *x11App = qApp->nativeInterface<QNativeInterface::QX11Application>())
    {
      /* Get X Display for use with Xlib */
      void* xdisplay = x11App->display();
      if (xdisplay)
      {
        IupSetGlobal("XDISPLAY", (char*)xdisplay);
      }

      /* Also available: x11App->connection() returns xcb_connection_t* for XCB */
    }
#endif
  }
  else if (platform == "wayland")
  {
    IupSetGlobal("QTPLATFORM", "WAYLAND");

#if defined(IUP_QT_HAS_WAYLAND_APP) && defined(Q_OS_LINUX)
    /* Qt 6.5+: Use QWaylandApplication native interface (Linux only) */
    if (auto *waylandApp = qApp->nativeInterface<QNativeInterface::QWaylandApplication>())
    {
      /* Get Wayland display (wl_display*) */
      void* wl_display = waylandApp->display();
      if (wl_display)
      {
        IupSetGlobal("WL_DISPLAY", (char*)wl_display);
      }
    }
#endif
  }
  else if (platform == "windows")
  {
    IupSetGlobal("QTPLATFORM", "WIN32");
  }
  else if (platform == "cocoa")
  {
    IupSetGlobal("QTPLATFORM", "COCOA");
  }
  else
  {
    /* Unknown platform - set platform name as-is */
    IupStoreGlobal("QTPLATFORM", platform.toUtf8().constData());
  }

#else
  /* ========== Qt5 Implementation ========== */
  /* Qt5: Limited native interface support
   * QPlatformNativeInterface exists but requires qpa private headers which are
   * not included in standard pkg-config paths. Qt6 provides proper public API.
   */

  if (platform == "xcb")
  {
    IupSetGlobal("QTPLATFORM", "X11");
    /* Note: XDISPLAY not set in Qt5 - QPlatformNativeInterface requires private headers.
     * Applications can manually set XDISPLAY if needed using platform-specific code. */
  }
  else if (platform == "wayland")
  {
    IupSetGlobal("QTPLATFORM", "WAYLAND");
    /* Note: WL_DISPLAY not set in Qt5 (would require private headers) */
  }
  else if (platform == "windows")
  {
    IupSetGlobal("QTPLATFORM", "WIN32");
  }
  else if (platform == "cocoa")
  {
    IupSetGlobal("QTPLATFORM", "COCOA");
  }
  else
  {
    /* Unknown platform - set platform name as-is */
    IupStoreGlobal("QTPLATFORM", platform.toUtf8().constData());
  }

#endif  /* Qt version check */
}

/****************************************************************************
 * Qt Application Management
 ****************************************************************************/

extern "C" QApplication* iupqtGetApplication(void)
{
  return qt_application;
}

/****************************************************************************
 * System Theme Detection
 ****************************************************************************/

extern "C" int iupqtIsSystemDarkMode(void)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
  /* Qt 6.5+ has proper dark mode detection */
  QStyleHints* hints = QGuiApplication::styleHints();
  return (hints->colorScheme() == Qt::ColorScheme::Dark) ? 1 : 0;
#else
  /* For older versions, check palette */
  QPalette palette = QGuiApplication::palette();
  QColor bg = palette.color(QPalette::Window);

  /* Simple heuristic: dark mode if background is darker than foreground */
  int brightness = (bg.red() + bg.green() + bg.blue()) / 3;
  return brightness < 128 ? 1 : 0;
#endif
}

/****************************************************************************
 * Screen DPI Information
 ****************************************************************************/

static void qtSetScreenInfo(void)
{
  QScreen* primary_screen = QGuiApplication::primaryScreen();
  if (primary_screen)
  {
    qreal dpi_x = primary_screen->logicalDotsPerInchX();
    qreal dpi_y = primary_screen->logicalDotsPerInchY();

    IupSetfAttribute(NULL, "SCREENDPI", "%g", dpi_x);
    IupSetfAttribute(NULL, "SCREENDPI_X", "%g", dpi_x);
    IupSetfAttribute(NULL, "SCREENDPI_Y", "%g", dpi_y);

    QSizeF physical_size = primary_screen->physicalSize();
    IupSetfAttribute(NULL, "SCREENPHYSICALSIZE", "%gx%g", physical_size.width(), physical_size.height());

    qreal pixel_density = primary_screen->devicePixelRatio();
    IupSetfAttribute(NULL, "SCREENPIXELRATIO", "%g", pixel_density);
  }
}

/****************************************************************************
 * Driver Initialization
 ****************************************************************************/

extern "C" int iupdrvOpen(int *argc, char ***argv)
{
  if (!QApplication::instance())
  {
    /* QApplication requires valid argc/argv */
    /* Qt WebEngine (Chromium) requires a proper executable path in argv[0] */
    /* NOTE: QApplication modifies argc, so we keep separate variables */
    static int original_argc = 1;   /* Never modified */
    static int default_argc = 1;    /* May be modified by QApplication */
    static char exe_path[4096] = {0};
    static char* default_argv_data[2] = { exe_path, NULL };
    static char** default_argv = default_argv_data;

    if (exe_path[0] == 0)
    {
#if defined(__linux__)
      ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
      if (len > 0)
        exe_path[len] = '\0';
      else
        strcpy(exe_path, "iup-qt");
#else
      strcpy(exe_path, "iup-qt");
#endif
    }

    if (!argc || !argv || *argc == 0 || !(*argv))
    {
      default_argc = original_argc;
      argc = &default_argc;
      argv = &default_argv;
    }

    qt_application = new QApplication(*argc, *argv);

    if (!qt_application)
      return IUP_ERROR;

    /* Disable automatic quit on last window close
     * IUP manually controls quit via IupExitLoop() in processEvents loop
     * NOTE: lastWindowClosed signal is NOT emitted when using processEvents(),
     * only when using exec(), so signal-based approaches won't work */
    qt_application->setQuitOnLastWindowClosed(false);
  }
  else
  {
    /* QApplication already exists (created by user) */
    qt_application = qobject_cast<QApplication*>(QApplication::instance());

    if (!qt_application)
      return IUP_ERROR;

    qt_application->setQuitOnLastWindowClosed(false);
  }

  setlocale(LC_NUMERIC, "C");

  IupSetGlobal("DRIVER", "Qt");

  IupSetfAttribute(NULL, "QTVERSION", "%s", qVersion());
  IupSetfAttribute(NULL, "QTDEVVERSION", "%d.%d.%d", QT_VERSION_MAJOR, QT_VERSION_MINOR, QT_VERSION_PATCH);

#ifdef QT_DEBUG
  IupSetGlobal("QTBUILDTYPE", "Debug");
#else
  IupSetGlobal("QTBUILDTYPE", "Release");
#endif

  if (argv && *argv && (*argv)[0] && (*argv)[0][0] != 0)
  {
    IupStoreGlobal("ARGV0", (*argv)[0]);
  }

  QString locale = QLocale::system().name();
  IupStoreGlobal("SYSTEMLANGUAGE", locale.toUtf8().constData());

  qtSetGlobalAttrib();
  qtSetGlobalColors();
  qtSetScreenInfo();

  IupSetGlobal("SHOWMENUIMAGES", "YES");

  if (iupqtIsSystemDarkMode())
    IupSetGlobal("DARKMODE", "YES");

#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
  IupSetGlobal("HIGHDPI_AWARE", "YES");
#endif

  QString style_name = QApplication::style()->objectName();
  IupStoreGlobal("QTSTYLE", style_name.toUtf8().constData());

  return IUP_NOERROR;
}

extern "C" int iupdrvSetGlobalAppIDAttrib(const char* value)
{
  static int appid_set = 0;
  if (appid_set || !value || !value[0])
    return 0;

  QApplication* app = iupqtGetApplication();
  if (!app)
    return 0;

  QString appid = QString::fromUtf8(value);

  if (!appid.endsWith(".desktop"))
    appid += ".desktop";

  app->setDesktopFileName(appid);
  appid_set = 1;
  return 1;
}

extern "C" int iupdrvSetGlobalAppNameAttrib(const char* value)
{
  static int appname_set = 0;
  if (appname_set || !value || !value[0])
    return 0;

  QApplication* app = iupqtGetApplication();
  if (!app)
    return 0;

  app->setApplicationName(QString::fromUtf8(value));
  appname_set = 1;
  return 1;
}

extern "C" void iupdrvClose(void)
{
  iupqtStrRelease();
  iupqtLoopCleanup();

  /* Note: We don't delete qt_application here because:
   * - It might have been created by user code
   * - QApplication should live for the entire program lifetime
   * - Deleting it can cause issues with Qt's event loop
   */
}
