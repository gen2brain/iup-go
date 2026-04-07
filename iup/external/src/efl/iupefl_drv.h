/** \file
 * \brief EFL Driver
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUPEFL_DRV_H
#define __IUPEFL_DRV_H

#include <Evas.h>
#include <Ecore.h>
#include <Elementary.h>
#include <Efl_Ui.h>

#include "iup.h"


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
#define iupeflSetSizeHintFill(obj, x, y) efl_gfx_hint_align_set(obj, (x) ? -1.0 : 0.5, (y) ? -1.0 : 0.5)

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
IUP_DRV_API Eo* iupeflGetLoop(void);

/* Called from iupefl_open.c during iupdrvClose() */
IUP_DRV_API void iupeflLoopCleanup(void);

/* Modal loop support - use instead of ecore_main_loop_begin/quit for dialogs.
   Pass the modal window to block input on all other windows, or NULL for no blocking. */
IUP_DRV_API void iupeflModalLoopRun(Eo* modal_win);
IUP_DRV_API void iupeflModalLoopQuit(void);
IUP_DRV_API void iupeflMessagePendingFlush(Eo *loop);

/****************************************************************************
 * Backend Detection (X11 vs Wayland)
 ****************************************************************************/

/* Runtime backend checks */
IUP_DRV_API int iupeflIsWayland(void);
IUP_DRV_API int iupeflIsX11(void);

/****************************************************************************
 * Event Handlers
 ****************************************************************************/

IUP_DRV_API void iupeflFocusChangedEvent(void *data, const Efl_Event *ev);
IUP_DRV_API void iupeflKeyDownEvent(void *data, const Efl_Event *ev);
IUP_DRV_API void iupeflKeyUpEvent(void *data, const Efl_Event *ev);
IUP_DRV_API void iupeflPointerMoveEvent(void *data, const Efl_Event *ev);
IUP_DRV_API void iupeflPointerDownEvent(void *data, const Efl_Event *ev);
IUP_DRV_API void iupeflPointerUpEvent(void *data, const Efl_Event *ev);
IUP_DRV_API void iupeflPointerWheelEvent(void *data, const Efl_Event *ev);
IUP_DRV_API void iupeflPointerInEvent(void *data, const Efl_Event *ev);
IUP_DRV_API void iupeflPointerOutEvent(void *data, const Efl_Event *ev);
IUP_DRV_API void iupeflManagerFocusChangedEvent(void *data, const Efl_Event *ev);

/****************************************************************************
 * Text and Mnemonic Handling
 ****************************************************************************/

IUP_DRV_API int iupeflSetMnemonicTitle(Ihandle* ih, Eo* widget, const char* value);
IUP_DRV_API void iupeflUpdateMnemonic(Ihandle* ih);

/****************************************************************************
 * Color Management
 ****************************************************************************/

IUP_DRV_API void iupeflColorSet(Eo* obj, unsigned char r, unsigned char g, unsigned char b, unsigned char a);
IUP_DRV_API int iupeflSetBgColorAttrib(Ihandle* ih, const char* value);
IUP_DRV_API int iupeflSetFgColorAttrib(Ihandle* ih, const char* value);

/****************************************************************************
 * Widget Management
 ****************************************************************************/

IUP_DRV_API void iupeflAddToParent(Ihandle* ih);
IUP_DRV_API void iupeflSetPosSize(Ihandle* ih, int x, int y, int width, int height);
IUP_DRV_API int iupeflIsInsideTabs(Ihandle* ih);

/* Base widget callbacks registration (EO API) */
IUP_DRV_API void iupeflBaseAddCallbacks(Ihandle* ih, Eo* widget);
IUP_DRV_API void iupeflBaseRemoveCallbacks(Ihandle* ih, Eo* widget);

/* Common attribute handlers */
IUP_DRV_API int iupeflBaseSetActiveAttrib(Ihandle* ih, const char* value);
IUP_DRV_API char* iupeflBaseGetActiveAttrib(Ihandle* ih);
IUP_DRV_API int iupeflBaseSetVisibleAttrib(Ihandle* ih, const char* value);

/****************************************************************************
 * Container Management
 ****************************************************************************/

/* Get the inner container for adding children */
IUP_DRV_API Eo* iupeflGetInnerContainer(Ihandle* ih);

/* Concrete Efl.Ui.Widget subclass with no automatic layout */
const Efl_Class* iupefl_fixed_class_get(void);

/* Fixed positioning container */
IUP_DRV_API Eo* iupeflFixedContainerNew(Eo* parent);
IUP_DRV_API void iupeflFixedContainerMove(Eo* container, Eo* child, int x, int y);

/* Native container for frame inner area */
IUP_DRV_API Eo* iupeflNativeContainerNew(Eo* parent);

/****************************************************************************
 * Focus Management
 ****************************************************************************/

IUP_DRV_API void iupeflSetCanFocus(Eo* widget, int can);
IUP_DRV_API void iupeflDialogSetFocus(Ihandle* ih);

/****************************************************************************
 * Font Management
 ****************************************************************************/

IUP_DRV_API void iupeflUpdateWidgetFont(Ihandle* ih, Eo* widget);
IUP_DRV_API void iupeflBuildTextStyle(Ihandle* ih, char* style, int style_size);
IUP_DRV_API void iupeflApplyTextStyle(Ihandle* ih, Eo* widget);
IUP_DRV_API void iupeflFontFree(Ihandle* ih);

/****************************************************************************
 * Textbox Border Measurement
 ****************************************************************************/

IUP_DRV_API void iupeflTextGetBorder(int* border_x, int* border_y);

/****************************************************************************
 * VG Image Data (used by canvas and draw)
 ****************************************************************************/

typedef struct _IeflVgImageData {
  Efl_VG* node;
  void* pixels;
  int w, h;
} IeflVgImageData;

/****************************************************************************
 * Image Management
 ****************************************************************************/

IUP_DRV_API Eo* iupeflImageGetImage(const char* name, Ihandle* ih, int make_inactive);
IUP_DRV_API Eo* iupeflImageGetImageForParent(const char* name, Ihandle* ih, int make_inactive, Eo* parent);
IUP_DRV_API int iupeflImageUpdateImage(Eo* elm_img, const char* name, Ihandle* ih, int make_inactive);

/****************************************************************************
 * Native Handle Access
 ****************************************************************************/

IUP_DRV_API char* iupeflGetNativeWidgetHandle(Eo* widget);
IUP_DRV_API char* iupeflGetNativeWindowHandleAttrib(Ihandle* ih);
IUP_DRV_API const char* iupeflGetNativeWindowHandleName(void);

/****************************************************************************
 * Dialog Management
 ****************************************************************************/

IUP_DRV_API Eo* iupeflGetParentWidget(Ihandle* ih);
IUP_DRV_API Eo* iupeflGetMainWindow(void);
IUP_DRV_API void iupeflSetMainWindow(Eo* win);
IUP_DRV_API unsigned int iupeflGetDefaultSeat(Eo* widget);

/****************************************************************************
 * Key Code Conversion
 ****************************************************************************/

IUP_DRV_API void iupeflKeyEncode(int key, const char** keyname, const char** keystr);
IUP_DRV_API void iupeflButtonKeySetStatus(Evas_Modifier* modifiers, unsigned int button, char* status, int doubleclick);

/****************************************************************************
 * Menu Mnemonic Support
 ****************************************************************************/

IUP_DRV_API Elm_Object_Item* iupeflMenuFindMnemonic(Ihandle* ih, char key);

/****************************************************************************
 * System Information
 ****************************************************************************/

IUP_DRV_API int iupeflIsSystemDarkMode(void);
IUP_DRV_API void iupeflSetGlobalColors(void);

#endif /* __IUPEFL_DRV_H */
