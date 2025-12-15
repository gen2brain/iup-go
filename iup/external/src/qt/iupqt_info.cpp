/** \file
 * \brief Qt System Information
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>

#include <QScreen>
#include <QGuiApplication>
#include <QCursor>
#include <QRect>
#include <QString>
#include <QSysInfo>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QWidget>
#include <QThread>

#ifndef _WIN32
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <langinfo.h>
#endif

#include "iup.h"
#include "iup_export.h"
#include "iup_str.h"
#include "iup_object.h"
#include "iup_drvinfo.h"
#include "iup_varg.h"

#include "iupqt_drv.h"

/* Forward declarations for IUP driver functions */
extern "C" {
    void iupdrvKeyEncode(int key, unsigned int *keyval, unsigned int *state);
    void iupdrvWarpPointer(int x, int y);
}

/****************************************************************************
 * Screen Information
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvAddScreenOffset(int *x, int *y, int add)
{
  /* Qt handles coordinates correctly, no offset needed */
  (void)x;
  (void)y;
  (void)add;
}

extern "C" IUP_SDK_API void iupdrvGetScreenSize(int *width, int *height)
{
  QScreen* screen = QGuiApplication::primaryScreen();

  if (screen)
  {
    QRect available_geom = screen->availableGeometry();
    *width = available_geom.width();
    *height = available_geom.height();
  }
  else
  {
    /* Fallback */
    *width = 800;
    *height = 600;
  }
}

extern "C" IUP_SDK_API void iupdrvGetFullSize(int *width, int *height)
{
  QScreen* screen = QGuiApplication::primaryScreen();

  if (screen)
  {
    QRect geom = screen->geometry();
    *width = geom.width();
    *height = geom.height();
  }
  else
  {
    /* Fallback */
    *width = 800;
    *height = 600;
  }
}

extern "C" IUP_SDK_API int iupdrvGetScreenDepth(void)
{
  QScreen* screen = QGuiApplication::primaryScreen();

  if (screen)
    return screen->depth();

  return 24;  /* Fallback */
}

extern "C" IUP_SDK_API double iupdrvGetScreenDpi(void)
{
  QScreen* screen = QGuiApplication::primaryScreen();

  if (screen)
  {
    /* Use logical DPI (like GTK) instead of physical DPI.
     * Qt's logicalDotsPerInch() returns the DPI setting (typically 96),
     * while physicalDotsPerInch() returns actual screen DPI (e.g., 264 on HiDPI).
     * IUP expects logical DPI for size calculations to match other platforms. */
    qreal dpi_x = screen->logicalDotsPerInchX();
    qreal dpi_y = screen->logicalDotsPerInchY();

    /* Return average */
    return (dpi_x + dpi_y) / 2.0;
  }

  return 96.0;  /* Fallback */
}

/****************************************************************************
 * Cursor and Keyboard State
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvGetCursorPos(int *x, int *y)
{
  QPoint pos = QCursor::pos();

  if (x) *x = pos.x();
  if (y) *y = pos.y();
}

extern "C" IUP_SDK_API void iupdrvGetKeyState(char* key)
{
  Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers();

  /* Shift */
  if (modifiers & Qt::ShiftModifier)
    key[0] = 'S';
  else
    key[0] = ' ';

  /* Control */
  if (modifiers & Qt::ControlModifier)
    key[1] = 'C';
  else
    key[1] = ' ';

  /* Alt */
  if (modifiers & Qt::AltModifier)
    key[2] = 'A';
  else
    key[2] = ' ';

  /* Meta/Super/Windows key */
  if (modifiers & Qt::MetaModifier)
    key[3] = 'Y';
  else
    key[3] = ' ';

  key[4] = 0;  /* NULL terminate */
}

/****************************************************************************
 * System Information
 ****************************************************************************/

extern "C" IUP_SDK_API char *iupdrvGetComputerName(void)
{
  QString hostname = QSysInfo::machineHostName();
  return iupStrReturnStr(hostname.toUtf8().constData());
}

extern "C" IUP_SDK_API char *iupdrvGetUserName(void)
{
#ifdef _WIN32
  /* Windows */
  QString username = qgetenv("USERNAME");
  if (username.isEmpty())
    username = qgetenv("USER");

  return iupStrReturnStr(username.toUtf8().constData());
#else
  /* Unix/Linux/macOS */
  const char* username = getenv("USER");

  if (!username)
  {
    /* Try getpwuid as fallback */
    struct passwd* pwd = getpwuid(getuid());
    if (pwd)
      username = pwd->pw_name;
  }

  if (username)
    return iupStrReturnStr(username);

  return iupStrReturnStr("unknown");
#endif
}

/****************************************************************************
 * Additional System Info
 ****************************************************************************/

extern "C" IUP_SDK_API char *iupdrvGetSystemName(void)
{
  /* Return OS name */
  QString os_name = QSysInfo::productType();
  return iupStrReturnStr(os_name.toUtf8().constData());
}

extern "C" IUP_SDK_API char *iupdrvGetSystemVersion(void)
{
  /* Return OS version */
  QString os_version = QSysInfo::productVersion();
  return iupStrReturnStr(os_version.toUtf8().constData());
}

/****************************************************************************
 * Keyboard/Mouse Input Simulation
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvSendKey(int key, int press)
{
  /* Not supported */
  (void)key;
  (void)press;
}

extern "C" IUP_SDK_API void iupdrvSendMouse(int x, int y, int bt, int status)
{
  /* Not supported */
  (void)x;
  (void)y;
  (void)bt;
  (void)status;
}

extern "C" IUP_SDK_API void iupdrvWarpPointer(int x, int y)
{
  /* Move the mouse cursor to absolute screen coordinates */
  QCursor::setPos(x, y);
}

/****************************************************************************
 * Sleep Function
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvSleep(int time)
{
  QThread::msleep(time);
}

/****************************************************************************
 * Accessibility
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvSetAccessibleTitle(Ihandle *ih, const char* title)
{
  /* Set accessible title for screen readers */
  QWidget* widget = (QWidget*)ih->handle;

  if (widget && title)
  {
    widget->setAccessibleName(QString::fromUtf8(title));
  }
}

/****************************************************************************
 * Directory and File Functions
 ****************************************************************************/

extern "C" IUP_SDK_API char* iupdrvGetCurrentDirectory(void)
{
  QDir current = QDir::current();
  QByteArray path = current.absolutePath().toUtf8();
  char* buffer = (char*)iupStrGetMemory(path.length() + 1);
  strcpy(buffer, path.constData());
  return buffer;
}

extern "C" IUP_SDK_API int iupdrvSetCurrentDirectory(const char* dir)
{
  return QDir::setCurrent(QString::fromUtf8(dir)) ? 1 : 0;
}

extern "C" IUP_SDK_API int iupdrvGetPreferencePath(char *filename, const char *app_name, int use_system)
{
  if (!app_name || !app_name[0])
  {
    filename[0] = '\0';
    return 0;
  }

  if (use_system)
  {
    /* Use GenericConfigLocation for system config path:
     * - Linux: ~/.config/appname/config
     * - Windows: C:/Users/<USER>/AppData/Local/appname/config.cfg
     * - macOS: ~/Library/Application Support/appname/config */
    QString config_dir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    if (config_dir.isEmpty())
    {
      filename[0] = '\0';
      return 0;
    }

    QString app_dir = config_dir + "/" + QString::fromUtf8(app_name);

    /* Create app directory if needed */
    QDir dir(app_dir);
    if (!dir.exists())
      dir.mkpath(".");

#ifdef _WIN32
    QString config_file = app_dir + "/config.cfg";
#else
    QString config_file = app_dir + "/config";
#endif

    QByteArray path_bytes = config_file.toUtf8();
    strcpy(filename, path_bytes.constData());
    return 1;
  }
  else
  {
    /* Legacy: home directory */
    QString home_dir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    if (home_dir.isEmpty())
    {
      filename[0] = '\0';
      return 0;
    }

#ifdef _WIN32
    QString config_file = home_dir + "/" + QString::fromUtf8(app_name) + ".cfg";
#else
    QString config_file = home_dir + "/." + QString::fromUtf8(app_name);
#endif

    QByteArray path_bytes = config_file.toUtf8();
    strcpy(filename, path_bytes.constData());
    return 1;
  }
}

extern "C" int qtMakeDirectory(const char* name)
{
  QDir dir;
  return dir.mkpath(QString::fromUtf8(name)) ? 1 : 0;
}

extern "C" int qtIsFile(const char* name)
{
  QFileInfo info(QString::fromUtf8(name));
  return (info.exists() && info.isFile()) ? 1 : 0;
}

extern "C" int qtIsDirectory(const char* name)
{
  QFileInfo info(QString::fromUtf8(name));
  return (info.exists() && info.isDir()) ? 1 : 0;
}

extern "C" int qtGetWindowDecor(void* wnd, int *border, int *caption)
{
  (void)wnd;

  /* Qt doesn't provide a direct API to get window decoration sizes
   * These are typical values for most window managers */
  if (border)
    *border = 4;  /* Typical border width */

  if (caption)
    *caption = 30;  /* Typical title bar height */

  return 1;
}

/****************************************************************************
 * Locale Information
 ****************************************************************************/

extern "C" IUP_SDK_API char* iupdrvLocaleInfo(void)
{
#ifndef _WIN32
  /* Unix/Linux - use nl_langinfo */
  return iupStrReturnStr(nl_langinfo(CODESET));
#else
  /* Windows - Qt6 uses UTF-8 everywhere */
  return iupStrReturnStr("UTF-8");
#endif
}

/****************************************************************************
 * Logging Functions
 ****************************************************************************/

extern "C" IUP_API void IupLogV(const char* type, const char* format, va_list arglist)
{
  char buffer[2048];
  vsnprintf(buffer, sizeof(buffer), format, arglist);

  /* Use Qt's logging system */
  QtMsgType msg_type = QtDebugMsg;

  if (iupStrEqualNoCase(type, "DEBUG"))
    msg_type = QtDebugMsg;
  else if (iupStrEqualNoCase(type, "INFO"))
    msg_type = QtInfoMsg;
  else if (iupStrEqualNoCase(type, "WARNING"))
    msg_type = QtWarningMsg;
  else if (iupStrEqualNoCase(type, "ERROR"))
    msg_type = QtCriticalMsg;
  else if (iupStrEqualNoCase(type, "CRITICAL") || iupStrEqualNoCase(type, "ALERT") ||
           iupStrEqualNoCase(type, "EMERGENCY"))
    msg_type = QtFatalMsg;

  qt_message_output(msg_type, QMessageLogContext(), QString::fromUtf8(buffer));
}

extern "C" IUP_API void IupLog(const char* type, const char* format, ...)
{
  va_list arglist;
  va_start(arglist, format);
  IupLogV(type, format, arglist);
  va_end(arglist);
}
