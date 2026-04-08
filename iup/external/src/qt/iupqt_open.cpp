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
#include <QString>
#include <QGuiApplication>

/* Qt6 native interface handling */
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  /* Qt6: Native interface via qguiapplication_platform.h */
  #if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
    #include <QtGui/qguiapplication_platform.h>
  #endif

  /* Qt6.7+: Forward declare QWaylandWindow to avoid private header dependency.
     Must be in QNativeInterface::Private and inherit QObject to match
     the real vtable layout from qplatformwindow_p.h. */
  #if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    #define IUP_QT_HAS_WAYLAND_WINDOW 1

    struct wl_surface;

    QT_BEGIN_NAMESPACE
    namespace QNativeInterface { namespace Private {
      struct Q_GUI_EXPORT QWaylandWindow : public QObject {
        QT_DECLARE_NATIVE_INTERFACE(QWaylandWindow, 1, QWindow)
        virtual struct wl_surface *surface() const = 0;
      };
    }}
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
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_globalattrib.h"
}

#include "iupqt_drv.h"

#ifdef IUPX11_USE_DLOPEN
#include "iupunix_x11.h"
#elif !defined(_WIN32) && !defined(__APPLE__)
#include <X11/Xlib.h>
#endif


static QApplication* qt_application = NULL;
static int qt_application_owned = 0;

IUP_DRV_API void iupqtLoopCleanup(void);

/****************************************************************************
 * Native Handle Access
 ****************************************************************************/

IUP_DRV_API char* iupqtGetNativeWidgetHandle(QWidget *widget)
{
  if (!widget)
    return NULL;

  if (!widget->windowHandle())
  {
    if (widget->isWindow() || widget->testAttribute(Qt::WA_NativeWindow))
    {
      widget->winId();
    }
  }

  QWindow* window = widget->windowHandle();
  if (!window)
    return NULL;

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
    {
      auto *waylandWindow = window->nativeInterface<QNativeInterface::Private::QWaylandWindow>();
      if (waylandWindow)
        return (char*)waylandWindow->surface();
    }
#endif
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
   */
  WId native_id = window->winId();
  return (char*)(uintptr_t)native_id;

#endif  /* Qt version check */

  /* Unknown platform */
  return NULL;
}

IUP_DRV_API const char* iupqtGetNativeWindowHandleName(void)
{
  QByteArray platformUtf8 = QGuiApplication::platformName().toUtf8();
  const char* platform = platformUtf8.constData();

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

IUP_DRV_API const char* iupqtGetNativeFontIdName(void)
{
  QByteArray platformUtf8 = QGuiApplication::platformName().toUtf8();
  const char* platform = platformUtf8.constData();

  if (strcmp(platform, "xcb") == 0)
    return "XFONTID";

  if (strcmp(platform, "windows") == 0)
    return "HFONT";

  /* Wayland and macOS use Qt's font system */
  return NULL;
}

IUP_DRV_API char* iupqtGetNativeWindowHandleAttrib(Ihandle* ih)
{
#ifndef __APPLE__
  QWidget* gl_canvas = (QWidget*)iupAttribGet(ih, "_IUPQT_CANVAS_WIDGET");
  if (gl_canvas)
    return iupqtGetNativeWidgetHandle(gl_canvas);
#endif

  return iupqtGetNativeWidgetHandle((QWidget*)ih->handle);
}

/****************************************************************************
 * String Conversion (UTF-8 handling)
 ****************************************************************************/

IUP_DRV_API void iupqtStrRelease(void)
{
}

IUP_DRV_API char* iupqtStrConvertToSystem(const char* str)
{
  /* Qt uses UTF-8 internally, so no conversion needed */
  return (char*)str;
}

IUP_DRV_API char* iupqtStrConvertToSystemLen(const char* str, int *len)
{
  /* Qt uses UTF-8 internally */
  if (len)
    *len = (int)strlen(str);
  return (char*)str;
}

IUP_DRV_API char* iupqtStrConvertFromSystem(const char* str)
{
  /* Qt uses UTF-8 internally */
  return (char*)str;
}

IUP_DRV_API char* iupqtStrConvertFromFilename(const char* str)
{
  /* Qt handles filenames correctly across platforms */
  return (char*)str;
}

IUP_DRV_API char* iupqtStrConvertToFilename(const char* str)
{
  /* Qt handles filenames correctly across platforms */
  return (char*)str;
}

IUP_DRV_API void iupqtStrSetUTF8Mode(int utf8mode)
{
  /* Qt is always UTF-8 */
  (void)utf8mode;
}

IUP_DRV_API int iupqtStrGetUTF8Mode(void)
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

IUP_DRV_API void iupqtSetGlobalColors(void)
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
    IupSetGlobal("WINDOWING", "X11");

#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0) && !defined(Q_OS_WIN) && !defined(Q_OS_MACOS)
    /* Qt 6.2+: Use QX11Application native interface */
    if (auto *x11App = qApp->nativeInterface<QNativeInterface::QX11Application>())
    {
      Display* xdisplay = (Display*)x11App->display();
      if (xdisplay)
      {
        IupSetGlobal("XDISPLAY", (char*)xdisplay);
#ifdef IUPX11_USE_DLOPEN
        if (iupX11Open())
#endif
        {
          IupSetGlobal("XSCREEN", (char*)(long)XDefaultScreen(xdisplay));
          IupSetGlobal("XSERVERVENDOR", XServerVendor(xdisplay));
          IupSetInt(NULL, "XVENDORRELEASE", XVendorRelease(xdisplay));
        }
      }
    }
#endif
  }
  else if (platform == "wayland")
  {
    IupSetGlobal("WINDOWING", "WAYLAND");

#if defined(IUP_QT_HAS_WAYLAND_APP) && !defined(Q_OS_WIN) && !defined(Q_OS_MACOS)
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
    IupSetGlobal("WINDOWING", "DWM");
  }
  else if (platform == "cocoa")
  {
    IupSetGlobal("WINDOWING", "QUARTZ");
  }
  else
  {
    /* Unknown platform - set platform name as-is */
    IupStoreGlobal("WINDOWING", platform.toUtf8().constData());
  }

#else
  /* ========== Qt5 Implementation ========== */
  /* Qt5: Limited native interface support
   * QPlatformNativeInterface exists but requires qpa private headers which are
   * not included in standard pkg-config paths. Qt6 provides proper public API.
   */

  if (platform == "xcb")
  {
    IupSetGlobal("WINDOWING", "X11");
    /* Note: XDISPLAY not set in Qt5 - QPlatformNativeInterface requires private headers.
     * Applications can manually set XDISPLAY if needed using platform-specific code. */
  }
  else if (platform == "wayland")
  {
    IupSetGlobal("WINDOWING", "WAYLAND");
    /* Note: WL_DISPLAY not set in Qt5 (would require private headers) */
  }
  else if (platform == "windows")
  {
    IupSetGlobal("WINDOWING", "DWM");
  }
  else if (platform == "cocoa")
  {
    IupSetGlobal("WINDOWING", "QUARTZ");
  }
  else
  {
    /* Unknown platform - set platform name as-is */
    IupStoreGlobal("WINDOWING", platform.toUtf8().constData());
  }

#endif  /* Qt version check */
}

/****************************************************************************
 * Qt Application Management
 ****************************************************************************/

IUP_DRV_API QApplication* iupqtGetApplication(void)
{
  return qt_application;
}

/****************************************************************************
 * System Theme Detection
 ****************************************************************************/

IUP_DRV_API int iupqtIsSystemDarkMode(void)
{
  QPalette palette = QGuiApplication::palette();
  QColor bg = palette.color(QPalette::Window);
  QColor fg = palette.color(QPalette::WindowText);

  /* Calculate relative luminance using standard formula (ITU-R BT.709) */
  double bg_lum = 0.2126 * bg.redF() + 0.7152 * bg.greenF() + 0.0722 * bg.blueF();
  double fg_lum = 0.2126 * fg.redF() + 0.7152 * fg.greenF() + 0.0722 * fg.blueF();

  /* Dark theme has lower background luminance than foreground */
  return (bg_lum < fg_lum) ? 1 : 0;
}

/****************************************************************************
 * Driver Initialization
 ****************************************************************************/

extern "C" IUP_SDK_API int iupdrvOpen(int *argc, char ***argv)
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
        iupStrCopyN(exe_path, sizeof(exe_path), "iup-qt");
#else
      iupStrCopyN(exe_path, sizeof(exe_path), "iup-qt");
#endif
    }

    if (!argc || !argv || *argc == 0 || !(*argv))
    {
      default_argc = original_argc;
      argc = &default_argc;
      argv = &default_argv;
    }

    qt_application = new QApplication(*argc, *argv);
    qt_application_owned = 1;

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
  iupqtSetGlobalColors();

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

extern "C" IUP_SDK_API int iupdrvSetGlobalAppIDAttrib(const char* value)
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

extern "C" IUP_SDK_API int iupdrvSetGlobalAppNameAttrib(const char* value)
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

extern "C" IUP_SDK_API void iupdrvClose(void)
{
  iupqtStrRelease();
  iupqtLoopCleanup();

  if (qt_application_owned)
  {
    delete qt_application;
    qt_application = NULL;
    qt_application_owned = 0;
  }

#ifdef IUPX11_USE_DLOPEN
  iupX11Close();
#endif
}
