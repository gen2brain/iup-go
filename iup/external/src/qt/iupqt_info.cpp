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
 * Platform-Specific Declarations for Input Simulation
 * Avoiding full headers - define only what we need
 ****************************************************************************/

#ifdef _WIN32
/* Windows SendInput API declarations */
extern "C" {
    typedef unsigned long DWORD;
    typedef unsigned short WORD;
    typedef long LONG;
    typedef void* LPARAM;

    typedef struct tagKEYBDINPUT {
        WORD wVk;
        WORD wScan;
        DWORD dwFlags;
        DWORD time;
        LPARAM dwExtraInfo;
    } KEYBDINPUT;

    typedef struct tagMOUSEINPUT {
        LONG dx;
        LONG dy;
        DWORD mouseData;
        DWORD dwFlags;
        DWORD time;
        LPARAM dwExtraInfo;
    } MOUSEINPUT;

    typedef struct tagINPUT {
        DWORD type;
        union {
            MOUSEINPUT mi;
            KEYBDINPUT ki;
        };
    } INPUT;

    #define INPUT_MOUSE 0
    #define INPUT_KEYBOARD 1
    #define KEYEVENTF_KEYUP 0x0002
    #define MOUSEEVENTF_MOVE 0x0001
    #define MOUSEEVENTF_LEFTDOWN 0x0002
    #define MOUSEEVENTF_LEFTUP 0x0004
    #define MOUSEEVENTF_RIGHTDOWN 0x0008
    #define MOUSEEVENTF_RIGHTUP 0x0010
    #define MOUSEEVENTF_MIDDLEDOWN 0x0020
    #define MOUSEEVENTF_MIDDLEUP 0x0040
    #define MOUSEEVENTF_XDOWN 0x0080
    #define MOUSEEVENTF_XUP 0x0100
    #define MOUSEEVENTF_WHEEL 0x0800
    #define MOUSEEVENTF_ABSOLUTE 0x8000
    #define XBUTTON1 0x0001
    #define XBUTTON2 0x0002
    #define MAPVK_VK_TO_VSC 0

    __declspec(dllimport) unsigned int __stdcall SendInput(unsigned int, INPUT*, int);
    __declspec(dllimport) WORD __stdcall MapVirtualKeyW(unsigned int, unsigned int);
    __declspec(dllimport) LPARAM __stdcall GetMessageExtraInfo(void);
}
#else
/* X11 minimal declarations for input simulation */
extern "C" {
    typedef struct _XDisplay Display;
    typedef unsigned long Window;
    typedef unsigned long XID;
    typedef unsigned long Time;
    typedef unsigned int KeyCode;
    typedef int Bool;

    #define KeyPress 2
    #define KeyRelease 3
    #define ButtonPress 4
    #define ButtonRelease 5
    #define X11_False 0
    #define X11_True 1

    #define KeyPressMask (1L<<0)
    #define KeyReleaseMask (1L<<1)
    #define ButtonPressMask (1L<<2)
    #define ButtonReleaseMask (1L<<3)

    #define InputFocus 1L
    #define PointerWindow 2L

    #define Button1 1
    #define Button2 2
    #define Button3 3
    #define Button4 4
    #define Button5 5
    #define Button1Mask (1<<8)
    #define Button2Mask (1<<9)
    #define Button3Mask (1<<10)
    #define Button4Mask (1<<11)
    #define Button5Mask (1<<12)

    typedef union _XEvent {
        int type;
        struct {
            int type;
            unsigned long serial;
            Bool send_event;
            Display* display;
            Window window;
            Window root;
            Window subwindow;
            Time time;
            int x, y;
            int x_root, y_root;
            unsigned int state;
            unsigned int keycode;
            Bool same_screen;
        } xkey;
        struct {
            int type;
            unsigned long serial;
            Bool send_event;
            Display* display;
            Window window;
            Window root;
            Window subwindow;
            Time time;
            int x, y;
            int x_root, y_root;
            unsigned int state;
            unsigned int button;
            Bool same_screen;
        } xbutton;
    } XEvent;

    /* dlopen/dlsym for dynamic loading */
    #define RTLD_LAZY 0x00001
    #define RTLD_NOW  0x00002
    void* dlopen(const char* filename, int flags);
    void* dlsym(void* handle, const char* symbol);
    int dlclose(void* handle);
    char* dlerror(void);
}

/* X11 function pointers - loaded dynamically */
static void* x11_lib = NULL;
static int (*_XSendEvent)(Display*, Window, Bool, long, XEvent*) = NULL;
static int (*_XGetInputFocus)(Display*, Window*, int*) = NULL;
static Bool (*_XQueryPointer)(Display*, Window, Window*, Window*, int*, int*, int*, int*, unsigned int*) = NULL;
static Window (*_XDefaultRootWindow)(Display*) = NULL;

/* Load X11 functions dynamically */
static int x11_load_functions(void)
{
    if (x11_lib)
        return 1;  /* Already loaded */

    x11_lib = dlopen("libX11.so.6", RTLD_LAZY);
    if (!x11_lib)
        x11_lib = dlopen("libX11.so", RTLD_LAZY);

    if (!x11_lib)
        return 0;

    _XSendEvent = (int (*)(Display*, Window, Bool, long, XEvent*))dlsym(x11_lib, "XSendEvent");
    _XGetInputFocus = (int (*)(Display*, Window*, int*))dlsym(x11_lib, "XGetInputFocus");
    _XQueryPointer = (Bool (*)(Display*, Window, Window*, Window*, int*, int*, int*, int*, unsigned int*))dlsym(x11_lib, "XQueryPointer");
    _XDefaultRootWindow = (Window (*)(Display*))dlsym(x11_lib, "XDefaultRootWindow");

    if (!_XSendEvent || !_XGetInputFocus || !_XQueryPointer || !_XDefaultRootWindow)
    {
        dlclose(x11_lib);
        x11_lib = NULL;
        return 0;
    }

    return 1;
}
#endif


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

#ifdef _WIN32
static DWORD winGetButtonStatus(int bt, int status)
{
  /* Convert IUP button/status to Windows mouse event flags */
  DWORD flags = 0;

  switch(bt)
  {
  case IUP_BUTTON1:
    flags = (status == 1) ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
    break;
  case IUP_BUTTON2:
    flags = (status == 1) ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
    break;
  case IUP_BUTTON3:
    flags = (status == 1) ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
    break;
  case IUP_BUTTON4:
  case IUP_BUTTON5:
    flags = (status == 1) ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP;
    break;
  }

  return flags;
}
#endif

extern "C" IUP_SDK_API void iupdrvSendKey(int key, int press)
{
  QString platform = QGuiApplication::platformName();

#ifdef _WIN32
  if (platform == "windows")
  {
    unsigned int keyval, state;
    INPUT input[2];
    LPARAM extra_info;
    WORD state_scan = 0, key_scan;
    memset(input, 0, 2 * sizeof(INPUT));

    iupdrvKeyEncode(key, &keyval, &state);
    if (!keyval)
      return;

    extra_info = GetMessageExtraInfo();
    if (state)
      state_scan = MapVirtualKeyW(state, MAPVK_VK_TO_VSC);
    key_scan = MapVirtualKeyW(keyval, MAPVK_VK_TO_VSC);

    if (press & 0x01)  /* Key press */
    {
      if (state)
      {
        /* Modifier first */
        input[0].type = INPUT_KEYBOARD;
        input[0].ki.wVk = (WORD)state;
        input[0].ki.wScan = state_scan;
        input[0].ki.dwExtraInfo = extra_info;

        /* Key second */
        input[1].type = INPUT_KEYBOARD;
        input[1].ki.wVk = (WORD)keyval;
        input[1].ki.wScan = key_scan;
        input[1].ki.dwExtraInfo = extra_info;

        SendInput(2, input, sizeof(INPUT));
      }
      else
      {
        input[0].type = INPUT_KEYBOARD;
        input[0].ki.wVk = (WORD)keyval;
        input[0].ki.wScan = key_scan;
        input[0].ki.dwExtraInfo = extra_info;

        SendInput(1, input, sizeof(INPUT));
      }
    }

    if (press & 0x02)  /* Key release */
    {
      if (state)
      {
        /* Key first */
        input[0].type = INPUT_KEYBOARD;
        input[0].ki.wVk = (WORD)keyval;
        input[0].ki.wScan = key_scan;
        input[0].ki.dwFlags = KEYEVENTF_KEYUP;
        input[0].ki.dwExtraInfo = extra_info;

        /* Modifier second */
        input[1].type = INPUT_KEYBOARD;
        input[1].ki.wVk = (WORD)state;
        input[1].ki.wScan = state_scan;
        input[1].ki.dwFlags = KEYEVENTF_KEYUP;
        input[1].ki.dwExtraInfo = extra_info;

        SendInput(2, input, sizeof(INPUT));
      }
      else
      {
        input[0].type = INPUT_KEYBOARD;
        input[0].ki.wVk = (WORD)keyval;
        input[0].ki.wScan = key_scan;
        input[0].ki.dwFlags = KEYEVENTF_KEYUP;
        input[0].ki.dwExtraInfo = extra_info;

        SendInput(1, input, sizeof(INPUT));
      }
    }
  }
#else
  if (platform == "xcb")
  {
    /* Load X11 functions dynamically */
    if (!x11_load_functions())
      return;

    Display* display = (Display*)IupGetGlobal("XDISPLAY");
    if (!display)
      return;

    Window focus;
    int revert_to;
    XEvent evt;
    memset(&evt, 0, sizeof(XEvent));

    evt.xkey.display = display;
    evt.xkey.send_event = X11_True;
    evt.xkey.root = _XDefaultRootWindow(display);

    _XGetInputFocus(display, &focus, &revert_to);
    evt.xkey.window = focus;

    unsigned int keyval, state;
    iupdrvKeyEncode(key, &keyval, &state);
    if (!keyval)
      return;

    evt.xkey.keycode = keyval;
    evt.xkey.state = state;

    if (press & 0x01)
    {
      evt.type = KeyPress;
      evt.xkey.type = KeyPress;
      _XSendEvent(display, InputFocus, X11_False, KeyPressMask, &evt);
    }

    if (press & 0x02)
    {
      evt.type = KeyRelease;
      evt.xkey.type = KeyRelease;
      _XSendEvent(display, InputFocus, X11_False, KeyReleaseMask, &evt);
    }
  }
#endif

  /* Other platforms (Wayland, macOS): Not supported */
  (void)key;
  (void)press;
}

extern "C" IUP_SDK_API void iupdrvSendMouse(int x, int y, int bt, int status)
{
  QString platform = QGuiApplication::platformName();

#ifdef _WIN32
  if (platform == "windows")
  {
    INPUT input;
    memset(&input, 0, sizeof(INPUT));

    input.type = INPUT_MOUSE;
    input.mi.dx = x;
    input.mi.dy = y;
    input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE;
    input.mi.dwExtraInfo = GetMessageExtraInfo();

    if (bt != 'W' && status == -1)
    {
      /* Move only */
      input.mi.dwFlags |= MOUSEEVENTF_MOVE;
    }
    else
    {
      if (bt != 'W')
        input.mi.dwFlags |= winGetButtonStatus(bt, status);

      switch(bt)
      {
      case 'W':  /* Wheel */
        input.mi.mouseData = status * 120;
        input.mi.dwFlags |= MOUSEEVENTF_WHEEL;
        break;
      case IUP_BUTTON4:
        input.mi.mouseData = XBUTTON1;
        break;
      case IUP_BUTTON5:
        input.mi.mouseData = XBUTTON2;
        break;
      }
    }

    if (status == 2)  /* Double click */
    {
      SendInput(1, &input, sizeof(INPUT));  /* press */

      input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE;
      input.mi.dwFlags |= winGetButtonStatus(bt, 0);
      SendInput(1, &input, sizeof(INPUT));  /* release */

      input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE;
      input.mi.dwFlags |= winGetButtonStatus(bt, 1);
      SendInput(1, &input, sizeof(INPUT));  /* press again */
    }
    else
    {
      SendInput(1, &input, sizeof(INPUT));
    }
  }
#else
  if (platform == "xcb")
  {
    /* Load X11 functions dynamically */
    if (!x11_load_functions())
      return;

    Display* display = (Display*)IupGetGlobal("XDISPLAY");
    if (!display)
      return;

    /* Always update cursor position (like Motif implementation) */
    iupdrvWarpPointer(x, y);

    if (status != -1)
    {
      XEvent evt;
      memset(&evt, 0, sizeof(XEvent));

      evt.xbutton.display = display;
      evt.xbutton.send_event = X11_True;

      /* Query pointer to get current window under cursor */
      _XQueryPointer(display, _XDefaultRootWindow(display),
                    &evt.xbutton.root, &evt.xbutton.window,
                    &evt.xbutton.x_root, &evt.xbutton.y_root,
                    &evt.xbutton.x, &evt.xbutton.y, &evt.xbutton.state);

      /* Find deepest window at cursor position */
      evt.xbutton.subwindow = evt.xbutton.window;
      while (evt.xbutton.subwindow)
      {
        evt.xbutton.window = evt.xbutton.subwindow;
        _XQueryPointer(display, evt.xbutton.window, &evt.xbutton.root, &evt.xbutton.subwindow,
                      &evt.xbutton.x_root, &evt.xbutton.y_root,
                      &evt.xbutton.x, &evt.xbutton.y, &evt.xbutton.state);
      }

      evt.type = (status == 0) ? ButtonRelease : ButtonPress;
      evt.xbutton.type = evt.type;
      evt.xbutton.root = _XDefaultRootWindow(display);
      evt.xbutton.x = x;
      evt.xbutton.y = y;

      switch(bt)
      {
      case IUP_BUTTON1:
        evt.xbutton.state = Button1Mask;
        evt.xbutton.button = Button1;
        break;
      case IUP_BUTTON2:
        evt.xbutton.state = Button2Mask;
        evt.xbutton.button = Button2;
        break;
      case IUP_BUTTON3:
        evt.xbutton.state = Button3Mask;
        evt.xbutton.button = Button3;
        break;
      case IUP_BUTTON4:
        evt.xbutton.state = Button4Mask;
        evt.xbutton.button = Button4;
        break;
      case IUP_BUTTON5:
        evt.xbutton.state = Button5Mask;
        evt.xbutton.button = Button5;
        break;
      default:
        return;
      }

      _XSendEvent(display, PointerWindow, X11_False,
                 (status == 0) ? ButtonReleaseMask : ButtonPressMask, &evt);

      if (status == 2)  /* Double click */
      {
        evt.type = ButtonRelease;
        evt.xbutton.type = ButtonRelease;
        _XSendEvent(display, PointerWindow, X11_False, ButtonReleaseMask, &evt);

        evt.type = ButtonPress;
        evt.xbutton.type = ButtonPress;
        _XSendEvent(display, PointerWindow, X11_False, ButtonPressMask, &evt);
      }
    }
  }
#endif

  /* Other platforms (Wayland, macOS): Not supported */
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
  /* Sleep for specified milliseconds */
#ifdef _WIN32
  Sleep(time);
#else
  usleep(time * 1000);  /* usleep takes microseconds */
#endif
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

extern "C" IUP_SDK_API int iupdrvGetPreferencePath(char *filename, int use_system)
{
  (void)use_system;

  /* Use QStandardPaths to get the application data directory */
  QString app_name = QCoreApplication::applicationName();
  if (app_name.isEmpty())
    app_name = QCoreApplication::applicationFilePath().section('/', -1);

  QString config_path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

  if (config_path.isEmpty())
  {
    filename[0] = '\0';
    return 0;
  }

  /* Create directory if it doesn't exist */
  QDir dir(config_path);
  if (!dir.exists())
  {
    if (!dir.mkpath("."))
    {
      filename[0] = '\0';
      return 0;
    }
  }

  QByteArray path_bytes = config_path.toUtf8();
  strcpy(filename, path_bytes.constData());

  /* Ensure trailing separator */
  int len = (int)strlen(filename);
  if (len > 0 && filename[len-1] != '/' && filename[len-1] != '\\')
    strcat(filename, "/");

  return 1;
}

extern "C" int iupdrvMakeDirectory(const char* name)
{
  QDir dir;
  return dir.mkpath(QString::fromUtf8(name)) ? 1 : 0;
}

extern "C" int iupdrvIsFile(const char* name)
{
  QFileInfo info(QString::fromUtf8(name));
  return (info.exists() && info.isFile()) ? 1 : 0;
}

extern "C" int iupdrvIsDirectory(const char* name)
{
  QFileInfo info(QString::fromUtf8(name));
  return (info.exists() && info.isDir()) ? 1 : 0;
}

extern "C" int iupdrvGetWindowDecor(void* wnd, int *border, int *caption)
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
