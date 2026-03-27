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
IUP_DRV_API int iupqtMouseMoveEvent(QWidget *widget, QEvent *evt, Ihandle *ih);
IUP_DRV_API int iupqtMouseButtonEvent(QWidget *widget, QEvent *evt, Ihandle *ih);

/****************************************************************************
 * Text and Mnemonic Handling
 ****************************************************************************/

IUP_DRV_API int iupqtSetMnemonicTitle(Ihandle* ih, QWidget* widget, const char* value);
IUP_DRV_API void iupqtUpdateMnemonic(Ihandle* ih);

/****************************************************************************
 * Color Management
 ****************************************************************************/


/****************************************************************************
 * Widget Management
 ****************************************************************************/

IUP_DRV_API void iupqtAddToParent(Ihandle* ih);
IUP_DRV_API void iupqtSetPosSize(QWidget* parent, QWidget* widget, int x, int y, int width, int height);

/****************************************************************************
 * Container Management
 ****************************************************************************/

IUP_DRV_API QWidget* iupqtNativeContainerNew(int has_window);
IUP_DRV_API void iupqtNativeContainerAdd(QWidget* container, QWidget* widget);
IUP_DRV_API void iupqtNativeContainerMove(QWidget* container, QWidget* widget, int x, int y);

/****************************************************************************
 * String Conversion (UTF-8 Handling)
 ****************************************************************************/

/* Qt uses UTF-8 internally, so conversions are mostly pass-through */
IUP_DRV_API void iupqtStrRelease(void);
IUP_DRV_API char* iupqtStrConvertToSystem(const char* str);
IUP_DRV_API char* iupqtStrConvertToSystemLen(const char* str, int *len);
IUP_DRV_API char* iupqtStrConvertFromSystem(const char* str);
IUP_DRV_API char* iupqtStrConvertFromFilename(const char* str);
IUP_DRV_API char* iupqtStrConvertToFilename(const char* str);
IUP_DRV_API void iupqtStrSetUTF8Mode(int utf8mode);
IUP_DRV_API int iupqtStrGetUTF8Mode(void);

/****************************************************************************
 * Focus Management
 ****************************************************************************/

IUP_DRV_API int iupqtFocusInOutEvent(QWidget *widget, QEvent *evt, Ihandle* ih);
IUP_DRV_API void iupqtSetCanFocus(QWidget *widget, int can);
IUP_DRV_API void iupqtDialogSetFocus(Ihandle* ih);

/****************************************************************************
 * Key Input Handling
 ****************************************************************************/

IUP_DRV_API int iupqtKeyPressEvent(QWidget *widget, QKeyEvent *evt, Ihandle* ih);
IUP_DRV_API int iupqtKeyReleaseEvent(QWidget *widget, QKeyEvent *evt, Ihandle* ih);

#ifdef __cplusplus
IUP_DRV_API void iupqtButtonKeySetStatus(Qt::KeyboardModifiers modifiers, Qt::MouseButtons buttons, int button, char* status, int doubleclick);
#else
IUP_DRV_API void iupqtButtonKeySetStatus(int modifiers, int buttons, int button, char* status, int doubleclick);
#endif

/****************************************************************************
 * Font Management
 ****************************************************************************/

IUP_DRV_API QFont* iupqtGetQFont(const char* value);
IUP_DRV_API char* iupqtGetQFontAttrib(Ihandle *ih);
IUP_DRV_API void iupqtUpdateWidgetFont(Ihandle *ih, QWidget* widget);

/****************************************************************************
 * Native Handle Access
 ****************************************************************************/

/* Platform-specific native handles (X11, Wayland, Windows, macOS) */
IUP_DRV_API char* iupqtGetNativeWidgetHandle(QWidget *widget);
IUP_DRV_API char* iupqtGetNativeWindowHandleAttrib(Ihandle* ih);
IUP_DRV_API const char* iupqtGetNativeWindowHandleName(void);
IUP_DRV_API const char* iupqtGetNativeFontIdName(void);

/****************************************************************************
 * Dialog Management
 ****************************************************************************/

IUP_DRV_API int iupqtDialogCloseEvent(QWidget *widget, QEvent *evt, Ihandle *ih);
IUP_DRV_API QWidget* iupqtGetParentWidget(Ihandle* ih);

/****************************************************************************
 * System Utilities
 ****************************************************************************/

/* Dark mode detection */
IUP_DRV_API int iupqtIsSystemDarkMode(void);
IUP_DRV_API void iupqtSetGlobalColors(void);

/* Qt application instance (singleton) */
IUP_DRV_API QApplication* iupqtGetApplication(void);

/****************************************************************************
 * Canvas Support
 ****************************************************************************/

IUP_DRV_API QWidget* iupqtCanvasGetWidget(Ihandle* ih);

/****************************************************************************
 * Tooltip/Tips Management
 ****************************************************************************/

IUP_DRV_API void iupqtTipsDestroy(Ihandle* ih);
IUP_DRV_API void iupqtDragDropCleanup(Ihandle* ih);


#endif
