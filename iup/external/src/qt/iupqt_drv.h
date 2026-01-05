/** \file
 * \brief Qt Driver - Common Function Declarations
 *
 * This header provides the interface for the Qt-based IUP driver.
 * All functions are MOC-free and use standard C linkage for compatibility.
 *
 * Minimum Requirements:
 * - Qt 5.0 or later (Qt 5.x and Qt 6.x supported)
 * - C++11 compiler (for lambda support)
 * - No MOC (Meta-Object Compiler) required
 *
 * Key features:
 * - MOC-free implementation (uses lambdas, no Q_OBJECT/signals/slots)
 * - UTF-8 string handling throughout
 * - Cross-platform Qt support (X11, Wayland, Windows, macOS)
 * - System icon theme support (FreeDesktop icons + Qt standard icons)
 * - Proper resource cleanup
 * - High-precision timer support (1ms resolution)
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUPQT_DRV_H
#define __IUPQT_DRV_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __IUP_OBJECT_H
#include "iup_object.h"  /* Defines InativeHandle and Ihandle */
#endif

/****************************************************************************
 * Macros and Type Definitions
 ****************************************************************************/

/* Color conversion macros */
#define iupqtColorFromDouble(_x) ((int)((_x)*255.0))
#define iupqtColorToDouble(_x) ((double)(_x)/255.0)

/* Forward declarations for Qt types */
#ifdef __cplusplus
#include <QtCore/qnamespace.h>  /* For Qt::KeyboardModifiers, Qt::MouseButtons */
class QWidget;
class QApplication;
class QFont;
class QColor;
class QEvent;
class QKeyEvent;
class QMouseEvent;
#else
typedef struct _QWidget QWidget;
typedef struct _QApplication QApplication;
typedef struct _QFont QFont;
typedef struct _QColor QColor;
typedef struct _QEvent QEvent;
typedef struct _QKeyEvent QKeyEvent;
typedef struct _QMouseEvent QMouseEvent;
#endif

/****************************************************************************
 * Event Handlers
 ****************************************************************************/

/* Mouse and keyboard events */
IUP_DRV_API int iupqtEnterLeaveEvent(QWidget *widget, QEvent *evt, Ihandle* ih);
int iupqtMouseMoveEvent(QWidget *widget, QEvent *evt, Ihandle *ih);
IUP_DRV_API int iupqtMouseButtonEvent(QWidget *widget, QEvent *evt, Ihandle *ih);
IUP_DRV_API int iupqtShowHelp(QWidget *widget, Ihandle* ih);

/****************************************************************************
 * Text and Mnemonic Handling
 ****************************************************************************/

int iupqtSetMnemonicTitle(Ihandle* ih, QWidget* widget, const char* value);
void iupqtUpdateMnemonic(Ihandle* ih);

/****************************************************************************
 * Color Management
 ****************************************************************************/

void iupqtColorSetRGB(QColor* color, unsigned char r, unsigned char g, unsigned char b);
void iupqtSetBgColor(InativeHandle* handle, unsigned char r, unsigned char g, unsigned char b);
void iupqtSetFgColor(InativeHandle* handle, unsigned char r, unsigned char g, unsigned char b);

/****************************************************************************
 * Widget Management
 ****************************************************************************/

IUP_DRV_API void iupqtAddToParent(Ihandle* ih);
const char* iupqtGetWidgetClassName(QWidget* widget);
void iupqtSetPosSize(QWidget* parent, QWidget* widget, int x, int y, int width, int height);
void* iupqtGetWindow(QWidget *widget);
void iupqtWindowGetPointer(void *window, int *x, int *y, int *mask);
int iupqtIsVisible(QWidget* widget);
void iupqtSetMargin(QWidget* widget, int horiz_padding, int vert_padding);

/****************************************************************************
 * Container Management
 ****************************************************************************/

QWidget* iupqtNativeContainerNew(int has_window);
void iupqtNativeContainerAdd(QWidget* container, QWidget* widget);
void iupqtNativeContainerMove(QWidget* container, QWidget* widget, int x, int y);

/****************************************************************************
 * String Conversion (UTF-8 Handling)
 ****************************************************************************/

/* Qt uses UTF-8 internally, so conversions are mostly pass-through */
void iupqtStrRelease(void);
IUP_DRV_API char* iupqtStrConvertToSystem(const char* str);
char* iupqtStrConvertToSystemLen(const char* str, int *len);
char* iupqtStrConvertFromSystem(const char* str);
char* iupqtStrConvertFromFilename(const char* str);
char* iupqtStrConvertToFilename(const char* str);
void iupqtStrSetUTF8Mode(int utf8mode);
int iupqtStrGetUTF8Mode(void);

/****************************************************************************
 * Focus Management
 ****************************************************************************/

IUP_DRV_API int iupqtFocusInOutEvent(QWidget *widget, QEvent *evt, Ihandle* ih);
IUP_DRV_API void iupqtSetCanFocus(QWidget *widget, int can);
void iupqtDialogSetFocus(Ihandle* ih);

/****************************************************************************
 * Key Input Handling
 ****************************************************************************/

IUP_DRV_API int iupqtKeyPressEvent(QWidget *widget, QKeyEvent *evt, Ihandle* ih);
int iupqtKeyReleaseEvent(QWidget *widget, QKeyEvent *evt, Ihandle* ih);

#ifdef __cplusplus
void iupqtButtonKeySetStatus(Qt::KeyboardModifiers modifiers, Qt::MouseButtons buttons, int button, char* status, int doubleclick);
#else
void iupqtButtonKeySetStatus(int modifiers, int buttons, int button, char* status, int doubleclick);
#endif

/****************************************************************************
 * Font Management
 ****************************************************************************/

QFont* iupqtGetQFont(const char* value);
char* iupqtGetQFontAttrib(Ihandle *ih);
void iupqtUpdateWidgetFont(Ihandle *ih, QWidget* widget);

/****************************************************************************
 * Native Handle Access
 ****************************************************************************/

/* Platform-specific native handles (X11, Wayland, Windows, macOS) */
char* iupqtGetNativeWidgetHandle(QWidget *widget);
char* iupqtGetNativeWindowHandleAttrib(Ihandle* ih);
const char* iupqtGetNativeWindowHandleName(void);
const char* iupqtGetNativeFontIdName(void);
void* iupqtGetNativeGraphicsContext(QWidget* widget);
void iupqtReleaseNativeGraphicsContext(QWidget* widget, void* gc);

/****************************************************************************
 * Dialog Management
 ****************************************************************************/

int iupqtDialogCloseEvent(QWidget *widget, QEvent *evt, Ihandle *ih);
QWidget* iupqtGetParentWidget(Ihandle* ih);

/****************************************************************************
 * System Tray Support
 ****************************************************************************/

int iupqtSetTrayAttrib(Ihandle *ih, const char *value);
int iupqtSetTrayTipAttrib(Ihandle *ih, const char *value);
int iupqtSetTrayImageAttrib(Ihandle *ih, const char *value);
int iupqtSetTrayMenuAttrib(Ihandle *ih, const char *value);
int iupqtSetTrayBalloonAttrib(Ihandle *ih, const char *value);
void iupqtTrayCleanup(Ihandle *ih);

/****************************************************************************
 * System Utilities
 ****************************************************************************/

/* Dark mode detection */
int iupqtIsSystemDarkMode(void);
void iupqtSetGlobalColors(void);

/* Qt application instance (singleton) */
QApplication* iupqtGetApplication(void);

/****************************************************************************
 * Tooltip/Tips Management
 ****************************************************************************/

void iupqtTipsDestroy(Ihandle* ih);


#ifdef __cplusplus
}
#endif

#endif
