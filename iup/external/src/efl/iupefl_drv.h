/** \file
 * \brief EFL Driver
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUPEFL_DRV_H
#define __IUPEFL_DRV_H

#include <Efl.h>
#include <Evas.h>
#include <Ecore.h>

#ifdef HAVE_ECORE_WL2
#include <Ecore_Wl2.h>
#endif

#include <Ecore_Evas.h>
#include <Edje.h>
#include <Elementary.h>
#include <Efl_Ui.h>

#include "iup.h"
#include "iup_object.h"


/* Get the Eo object from an Ihandle */
#define iupeflGetWidget(ih) ((Eo*)(ih)->handle)

/* Color conversion macros */
#define iupeflColorFromDouble(_x) ((int)((_x)*255.0))
#define iupeflColorToDouble(_x) ((double)(_x)/255.0)

/* EFL uses ARGB premultiplied colors */
#define iupeflColorPremul(_a, _c) (((_a) * (_c)) / 255)

/* EO API convenience macros */
#define iupeflGetMainLoop() efl_main_loop_get()

/* Visibility */
#define iupeflSetVisible(obj, visible) efl_gfx_entity_visible_set(obj, visible)
#define iupeflIsVisible(obj) efl_gfx_entity_visible_get(obj)

/* EO geometry operations using EINA structs */
#define iupeflSetPosition(obj, x, y) efl_gfx_entity_position_set(obj, EINA_POSITION2D(x, y))
#define iupeflSetSize(obj, w, h) efl_gfx_entity_size_set(obj, EINA_SIZE2D(w, h))
#define iupeflGetGeometry(obj) efl_gfx_entity_geometry_get(obj)

/* EO size hints */
#define iupeflSetSizeHintMin(obj, w, h) efl_gfx_hint_size_min_set(obj, EINA_SIZE2D(w, h))
#define iupeflSetSizeHintWeight(obj, x, y) efl_gfx_hint_weight_set(obj, x, y)
#define iupeflSetSizeHintAlign(obj, x, y) efl_gfx_hint_align_set(obj, x, y)
#define iupeflSetSizeHintFill(obj, x, y) efl_gfx_hint_fill_set(obj, x, y)

/* EO text operations */
#define iupeflSetText(obj, text) efl_text_set(obj, text)
#define iupeflGetText(obj) efl_text_get(obj)

/* EO widget disabled state */
#define iupeflSetDisabled(obj, disabled) efl_ui_widget_disabled_set(obj, disabled)
#define iupeflGetDisabled(obj) efl_ui_widget_disabled_get(obj)

/* EO content operations for parts */
#define iupeflSetPartContent(obj, part, content) efl_content_set(efl_part(obj, part), content)
#define iupeflGetPartContent(obj, part) efl_content_get(efl_part(obj, part))

/* EO color operations */
#define iupeflSetColor(obj, r, g, b, a) efl_gfx_color_set(obj, r, g, b, a)
#define iupeflGetColor(obj, r, g, b, a) efl_gfx_color_get(obj, r, g, b, a)

/* EO object deletion */
#define iupeflDelete(obj) efl_del(obj)

/****************************************************************************
 * Driver Initialization and Cleanup
 ****************************************************************************/

/* Get the main EFL loop (set during iupdrvOpen) */
Eo* iupeflGetLoop(void);

/* Called from iupefl_open.c during iupdrvClose() */
void iupeflLoopCleanup(void);

/* Modal loop support - use instead of ecore_main_loop_begin/quit for dialogs */
void iupeflModalLoopRun(void);
void iupeflModalLoopQuit(void);
void iupeflMessagePendingFlush(Eo *loop);

/****************************************************************************
 * Backend Detection (X11 vs Wayland)
 ****************************************************************************/

/* Runtime backend checks */
int iupeflIsWayland(void);
int iupeflIsX11(void);

/****************************************************************************
 * Event Handlers
 ****************************************************************************/

void iupeflClickedEvent(void *data, const Efl_Event *ev);
void iupeflPressedEvent(void *data, const Efl_Event *ev);
void iupeflUnpressedEvent(void *data, const Efl_Event *ev);
void iupeflLongpressedEvent(void *data, const Efl_Event *ev);
void iupeflFocusChangedEvent(void *data, const Efl_Event *ev);
void iupeflKeyDownEvent(void *data, const Efl_Event *ev);
void iupeflKeyUpEvent(void *data, const Efl_Event *ev);
void iupeflPointerMoveEvent(void *data, const Efl_Event *ev);
void iupeflPointerDownEvent(void *data, const Efl_Event *ev);
void iupeflPointerUpEvent(void *data, const Efl_Event *ev);
void iupeflPointerWheelEvent(void *data, const Efl_Event *ev);
void iupeflPointerInEvent(void *data, const Efl_Event *ev);
void iupeflPointerOutEvent(void *data, const Efl_Event *ev);
void iupeflManagerFocusChangedEvent(void *data, const Efl_Event *ev);

/****************************************************************************
 * Text and Mnemonic Handling
 ****************************************************************************/

int iupeflSetMnemonicTitle(Ihandle* ih, Eo* widget, const char* value);
void iupeflUpdateMnemonic(Ihandle* ih);

/****************************************************************************
 * String Conversion (UTF-8 Handling)
 ****************************************************************************/

/* EFL uses UTF-8 internally, so conversions are mostly pass-through */
char* iupeflStrConvertToSystem(const char* str);
char* iupeflStrConvertFromSystem(const char* str);
char* iupeflStrConvertFromFilename(const char* str);
char* iupeflStrConvertToFilename(const char* str);

/****************************************************************************
 * Color Management
 ****************************************************************************/

void iupeflColorSet(Eo* obj, unsigned char r, unsigned char g, unsigned char b, unsigned char a);
int iupeflSetBgColorAttrib(Ihandle* ih, const char* value);
int iupeflSetFgColorAttrib(Ihandle* ih, const char* value);

/****************************************************************************
 * Widget Management
 ****************************************************************************/

void iupeflAddToParent(Ihandle* ih);
void iupeflSetPosSize(Ihandle* ih, int x, int y, int width, int height);

/* Base widget callbacks registration (EO API) */
void iupeflBaseAddCallbacks(Ihandle* ih, Eo* widget);
void iupeflBaseRemoveCallbacks(Ihandle* ih, Eo* widget);

/* Common attribute handlers */
int iupeflBaseSetActiveAttrib(Ihandle* ih, const char* value);
char* iupeflBaseGetActiveAttrib(Ihandle* ih);
int iupeflBaseSetVisibleAttrib(Ihandle* ih, const char* value);

/****************************************************************************
 * Container Management
 ****************************************************************************/

/* Get the inner container for adding children */
Eo* iupeflGetInnerContainer(Ihandle* ih);

/* Fixed positioning container */
Eo* iupeflFixedContainerNew(Eo* parent);
void iupeflFixedContainerMove(Eo* container, Eo* child, int x, int y);

/* Native container using EFL_UI_BOX_CLASS */
Eo* iupeflNativeContainerNew(Eo* parent);

/****************************************************************************
 * Focus Management
 ****************************************************************************/

void iupeflSetCanFocus(Eo* widget, int can);
void iupeflDialogSetFocus(Ihandle* ih);

/****************************************************************************
 * Font Management
 ****************************************************************************/

void iupeflUpdateWidgetFont(Ihandle* ih, Eo* widget);
void iupeflBuildTextStyle(Ihandle* ih, char* style, int style_size);
void iupeflApplyTextStyle(Ihandle* ih, Eo* widget);

/****************************************************************************
 * Textbox Border Measurement
 ****************************************************************************/

void iupeflTextGetBorder(int* border_x, int* border_y);

/****************************************************************************
 * Image Management
 ****************************************************************************/

Eo* iupeflImageGetImage(const char* name, Ihandle* ih, int make_inactive);
Eo* iupeflImageGetImageForParent(const char* name, Ihandle* ih, int make_inactive, Eo* parent);
int iupeflImageUpdateImage(Eo* elm_img, const char* name, Ihandle* ih, int make_inactive);
void iupeflImageDestroy(Eo* image);

/****************************************************************************
 * Native Handle Access
 ****************************************************************************/

char* iupeflGetNativeWidgetHandle(Eo* widget);
char* iupeflGetNativeWindowHandleAttrib(Ihandle* ih);
const char* iupeflGetNativeWindowHandleName(void);

/****************************************************************************
 * Dialog Management
 ****************************************************************************/

void iupeflDialogCloseCallback(void* data, const Efl_Event* ev);
Eo* iupeflGetParentWidget(Ihandle* ih);
Eo* iupeflGetMainWindow(void);
void iupeflSetMainWindow(Eo* win);
unsigned int iupeflGetDefaultSeat(Eo* widget);

/****************************************************************************
 * Key Code Conversion
 ****************************************************************************/

int iupeflKeyDecode(Evas_Event_Key_Down* ev);
void iupeflKeyEncode(int key, const char** keyname, const char** keystr);
void iupeflButtonKeySetStatus(Evas_Modifier* modifiers, unsigned int button, char* status, int doubleclick);

/****************************************************************************
 * Menu Mnemonic Support
 ****************************************************************************/

Elm_Object_Item* iupeflMenuFindMnemonic(Ihandle* ih, char key);

/****************************************************************************
 * System Information
 ****************************************************************************/

int iupeflIsSystemDarkMode(void);
void iupeflSetGlobalColors(void);

#endif /* __IUPEFL_DRV_H */
