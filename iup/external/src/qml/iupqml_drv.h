/** \file
 * \brief Qt Quick Driver - Common Function Declarations
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUPQML_DRV_H
#define __IUPQML_DRV_H

#ifndef __IUP_OBJECT_H
#include "iup_object.h"
#endif

#ifdef __cplusplus
#include <functional>
#include <QtCore/qnamespace.h>
#include <QString>
#include <QVariant>
class QObject;
class QQuickItem;
class QQuickWindow;
class QQmlEngine;
class QGuiApplication;
class QFont;
class QColor;
class QEvent;
class QKeyEvent;
class QMouseEvent;
class QPixmap;
class QPalette;
#else
typedef struct _QObject QObject;
typedef struct _QQuickItem QQuickItem;
typedef struct _QQuickWindow QQuickWindow;
typedef struct _QFont QFont;
typedef struct _QEvent QEvent;
typedef struct _QKeyEvent QKeyEvent;
typedef struct _QMouseEvent QMouseEvent;
#endif

#define IUPQML_IMPORTS "import QtQuick\nimport QtQuick.Controls\n"

#ifdef __cplusplus

/* Application and engine */
IUP_DRV_API QGuiApplication* iupqmlGetApplication(void);
IUP_DRV_API QQmlEngine* iupqmlGetEngine(void);
IUP_DRV_API void iupqmlLoopCleanup(void);
IUP_DRV_API int iupqmlStyleLocked(void);

/* Object creation */
IUP_DRV_API QObject* iupqmlCreateObject(const char* qml);
IUP_DRV_API QQuickItem* iupqmlCreateItem(const char* qml);
IUP_DRV_API QQuickItem* iupqmlTemplateItem(const char* qml);
IUP_DRV_API void iupqmlMeasureItem(QQuickItem* item);

/* Signals */
IUP_DRV_API QObject* iupqmlConnect(QObject* sender, const char* signal, std::function<void(void**)> func);

/* Properties */
IUP_DRV_API bool iupqmlSetProperty(QObject* obj, const char* name, const QVariant& value);
IUP_DRV_API QVariant iupqmlGetProperty(QObject* obj, const char* name);
IUP_DRV_API QQuickItem* iupqmlGetItemProperty(QObject* obj, const char* name);
IUP_DRV_API void iupqmlSetPaletteColor(QObject* obj, const char* role, const QColor& color);
IUP_DRV_API void iupqmlSetPaddings(QQuickItem* item, double left, double right, double top, double bottom);
IUP_DRV_API void iupqmlRestorePaddings(QQuickItem* item, QQuickItem* tpl);
IUP_DRV_API int iupqmlCallMethod(QObject* obj, const char* method, const QVariant& arg1 = QVariant(), const QVariant& arg2 = QVariant());

/* Images */
IUP_DRV_API QString iupqmlImageUrl(QPixmap* pixmap);
IUP_DRV_API void iupqmlImageRelease(QPixmap* pixmap);

/* Item helpers */
IUP_DRV_API QQuickItem* iupqmlGetItem(Ihandle* ih);
IUP_DRV_API Ihandle* iupqmlGetIhandle(QObject* obj);
IUP_DRV_API void iupqmlSetIhandle(QObject* obj, Ihandle* ih);
IUP_DRV_API void iupqmlAddToParent(Ihandle* ih);
IUP_DRV_API void iupqmlUnsetArrowCursors(QQuickItem* item);
IUP_DRV_API void iupqmlReleaseMouseGrab(void);
IUP_DRV_API int iupqmlMenuBarIsNative(void);
IUP_DRV_API QQuickItem* iupqmlButtonFrameInset(QQuickItem* button);
IUP_DRV_API QQuickItem* iupqmlCreateMeasureButton(Ihandle* ih, int checkable);
IUP_DRV_API QQuickItem* iupqmlButtonImageContent(QQuickItem* button);
IUP_DRV_API void iupqmlSetPosSize(QQuickItem* item, int x, int y, int width, int height);
IUP_DRV_API void iupqmlInstallFilter(Ihandle* ih, QQuickItem* item);
IUP_DRV_API void iupqmlRemoveFilter(QQuickItem* item);
IUP_DRV_API void iupqmlSetCanFocus(QQuickItem* item, int can);
IUP_DRV_API void iupqmlUpdateItemFont(Ihandle* ih, QObject* item);
IUP_DRV_API QQuickWindow* iupqmlGetParentWindow(Ihandle* ih);
IUP_DRV_API QQuickWindow* iupqmlDialogHostWindow(Ihandle* ih, int* owned);
IUP_DRV_API void iupqmlUpdateMnemonic(Ihandle* ih);
IUP_DRV_API int iupqmlSetMnemonicTitle(Ihandle* ih, QObject* item, const char* value);

/* Events */
IUP_DRV_API int iupqmlEnterLeaveEvent(QEvent* evt, Ihandle* ih);
IUP_DRV_API int iupqmlMouseMoveEvent(QMouseEvent* evt, Ihandle* ih);
IUP_DRV_API int iupqmlMouseButtonEvent(QMouseEvent* evt, Ihandle* ih);
IUP_DRV_API int iupqmlFocusInOutEvent(QEvent* evt, Ihandle* ih);
IUP_DRV_API int iupqmlFocusChanged(Ihandle* ih, int in);
IUP_DRV_API void iupqmlDialogSetFocus(Ihandle* ih);
IUP_DRV_API int iupqmlKeyPressEvent(QQuickItem* item, QKeyEvent* evt, Ihandle* ih);
IUP_DRV_API int iupqmlKeyReleaseEvent(QQuickItem* item, QKeyEvent* evt, Ihandle* ih);
IUP_DRV_API int iupqmlKeyDecode(QKeyEvent* evt);
IUP_DRV_API void iupqmlButtonKeySetStatus(Qt::KeyboardModifiers modifiers, Qt::MouseButtons buttons, int button, char* status, int doubleclick);
IUP_DRV_API int iupqmlMnemonicVisible(void);
IUP_DRV_API void iupqmlMnemonicUpdate(QObject* root);
IUP_DRV_API void iupqmlMnemonicRegister(Ihandle* ih, void (*refresh)(Ihandle*));

/* Fonts */
IUP_DRV_API QFont* iupqmlGetQFont(const char* value);
IUP_DRV_API QFont* iupqmlGetQFontLine(const char* value, int* ascent, int* charheight);
IUP_DRV_API QFont* iupqmlGetIhFont(Ihandle* ih);

/* Native handles and system */
IUP_DRV_API char* iupqmlGetNativeWindowHandle(QQuickWindow* window);
IUP_DRV_API char* iupqmlGetNativeWindowHandleAttrib(Ihandle* ih);
IUP_DRV_API const char* iupqmlGetNativeWindowHandleName(void);
IUP_DRV_API void iupqmlSetGlobalColors(void);
IUP_DRV_API void iupqmlUpdateSystemPalette(void);
IUP_DRV_API void iupqmlUpdateWindowPalette(QQuickWindow* window);
IUP_DRV_API int iupqmlSystemPaletteChanged(void);
IUP_DRV_API QPalette iupqmlGetPalette(void);
IUP_DRV_API int iupqmlStyleIsDark(void);

/* Dialog */
IUP_DRV_API QQuickWindow* iupqmlDialogGetWindow(Ihandle* ih);
IUP_DRV_API QQuickItem* iupqmlDialogGetContent(Ihandle* ih);
IUP_DRV_API int iupqmlDialogCloseEvent(Ihandle* ih);

/* Canvas */
IUP_DRV_API QQuickItem* iupqmlCanvasGetItem(Ihandle* ih);
IUP_DRV_API void iupqmlCanvasRedraw(Ihandle* ih, int now);
IUP_DRV_API void iupqmlCanvasFlush(Ihandle* ih);
IUP_DRV_API QPixmap* iupqmlCanvasCreateBuffer(Ihandle* ih, QQuickItem* item, int w, int h);
IUP_DRV_API int iupqmlCanvasBufferMatches(QPixmap* buffer, QQuickItem* item, int w, int h);

/* Tooltips and drag and drop */
IUP_DRV_API void iupqmlTipsDestroy(Ihandle* ih);
IUP_DRV_API void iupqmlDragDropCleanup(Ihandle* ih);

#endif

#endif
