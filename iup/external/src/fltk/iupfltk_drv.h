/** \file
 * \brief FLTK Driver - Common Function Declarations
 *
 * This header provides the interface for the FLTK-based IUP driver.
 *
 * Minimum Requirements:
 * - FLTK 1.4.x
 * - C++17 compiler
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUPFLTK_DRV_H
#define __IUPFLTK_DRV_H

#ifndef __IUP_OBJECT_H
#include "iup_object.h"
#endif

/****************************************************************************
 * Forward Declarations for FLTK Types
 ****************************************************************************/

#ifdef __cplusplus
class Fl_Widget;
class Fl_Group;
class Fl_Window;
class Fl_Input_;
#else
typedef struct _Fl_Widget Fl_Widget;
typedef struct _Fl_Group Fl_Group;
typedef struct _Fl_Window Fl_Window;
typedef struct _Fl_Input_ Fl_Input_;
#endif

/****************************************************************************
 * Event Handlers
 ****************************************************************************/

IUP_DRV_API int iupfltkEnterLeaveEvent(Fl_Widget *widget, Ihandle* ih, int event);
IUP_DRV_API int iupfltkEditCheckMask(Ihandle* ih, Fl_Input_* input, int event, const char* cb_name, void* mask, int nc);
IUP_DRV_API int iupfltkMouseMoveEvent(Fl_Widget *widget, Ihandle *ih);
IUP_DRV_API int iupfltkMouseButtonEvent(Fl_Widget *widget, Ihandle *ih, int event);

/****************************************************************************
 * Widget Management
 ****************************************************************************/

IUP_DRV_API void iupfltkAddToParent(Ihandle* ih);
IUP_DRV_API void iupfltkSetPosSize(Fl_Widget* widget, int x, int y, int width, int height);

/****************************************************************************
 * Container Management
 ****************************************************************************/

IUP_DRV_API Fl_Group* iupfltkNativeContainerNew(void);
IUP_DRV_API void iupfltkNativeContainerAdd(Fl_Group* container, Fl_Widget* widget);

/****************************************************************************
 * Platform Detection (runtime)
 ****************************************************************************/

IUP_DRV_API int iupfltkIsX11(void);
IUP_DRV_API int iupfltkIsWayland(void);

/****************************************************************************
 * Focus Management
 ****************************************************************************/

IUP_DRV_API int iupfltkFocusInOutEvent(Fl_Widget *widget, Ihandle* ih, int event);
IUP_DRV_API void iupfltkSetCanFocus(Fl_Widget *widget, int can);

/****************************************************************************
 * Key Input Handling
 ****************************************************************************/

IUP_DRV_API int iupfltkKeyPressEvent(Fl_Widget *widget, Ihandle* ih);
IUP_DRV_API int iupfltkKeyReleaseEvent(Fl_Widget *widget, Ihandle* ih);
IUP_DRV_API void iupfltkButtonKeySetStatus(int state, int button, char* status, int doubleclick);

/****************************************************************************
 * Text and Mnemonic Handling
 ****************************************************************************/

IUP_DRV_API void iupfltkSetMnemonicTitle(Ihandle* ih, Fl_Widget* widget, const char* value);
IUP_DRV_API int iupfltkHandleDropFiles(Ihandle* ih);
IUP_DRV_API int iupfltkDragDropHandleEvent(Fl_Widget* widget, Ihandle* ih, int event);

/****************************************************************************
 * Font Management
 ****************************************************************************/

IUP_DRV_API int iupfltkGetFont(Ihandle* ih, int* fl_font, int* fl_size);
IUP_DRV_API int iupfltkMapFontFace(const char* typeface, int is_bold, int is_italic);

/****************************************************************************
 * Text Formatting
 ****************************************************************************/

IUP_DRV_API void iupfltkFormatInit(Ihandle* ih);
IUP_DRV_API void iupfltkFormatCleanup(Ihandle* ih);
IUP_DRV_API const char* iupfltkFormatGetLinkAtPos(Ihandle* ih, int pos);
IUP_DRV_API int iupfltkFormatSetRemoveFormattingAttrib(Ihandle* ih, const char* value);
IUP_DRV_API int iupfltkGetFontFromString(const char* font, int* fl_font, int* fl_size);
IUP_DRV_API void iupfltkUpdateWidgetFont(Ihandle *ih, Fl_Widget* widget);

/****************************************************************************
 * Native Handle Access
 ****************************************************************************/

IUP_DRV_API char* iupfltkGetNativeWindowHandle(Fl_Window* window);
IUP_DRV_API char* iupfltkGetNativeWindowHandleAttrib(Ihandle* ih);
IUP_DRV_API const char* iupfltkGetNativeWindowHandleName(void);
IUP_DRV_API const char* iupfltkGetNativeFontIdName(void);

/****************************************************************************
 * Dialog Management
 ****************************************************************************/

IUP_DRV_API Fl_Window* iupfltkGetParentWidget(Ihandle* ih);
IUP_DRV_API void iupfltkX11SetSkipTaskbar(Fl_Window* window);

/****************************************************************************
 * System Utilities
 ****************************************************************************/

IUP_DRV_API int iupfltkIsSystemDarkMode(void);
IUP_DRV_API void iupfltkSetGlobalColors(void);

/****************************************************************************
 * Cleanup
 ****************************************************************************/

IUP_DRV_API void iupfltkLoopCleanup(void);


#endif
