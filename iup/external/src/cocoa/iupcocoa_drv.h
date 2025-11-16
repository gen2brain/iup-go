/** \file
 * \brief macOS Cocoa Driver
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUPMAC_DRV_H
#define __IUPMAC_DRV_H

#ifdef __cplusplus
extern "C" {
#endif

#include "iup_export.h"

#include <os/log.h>

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

#include "iup_image.h"


/* A unique memory address for an identifier. */
IUP_DRV_API extern const void* IHANDLE_ASSOCIATED_OBJ_KEY;
/* The main view within a root view (e.g., a scrollview contains the real widget). */
IUP_DRV_API extern const void* MAINVIEW_ASSOCIATED_OBJ_KEY;
/* The root view, in case the root object is not a view. */
IUP_DRV_API extern const void* ROOTVIEW_ASSOCIATED_OBJ_KEY;

#define iupcocoaLog(...) os_log(OS_LOG_DEFAULT, __VA_ARGS__)
#define iupcocoaLogDebug(...) os_log_debug(OS_LOG_DEFAULT, __VA_ARGS__)
#define iupcocoaLogInfo(...) os_log_info(OS_LOG_DEFAULT, __VA_ARGS__)
#define iupcocoaLogNotice(...) os_log(OS_LOG_DEFAULT, __VA_ARGS__)
#define iupcocoaLogWarning(...) os_log_error(OS_LOG_DEFAULT, __VA_ARGS__)
#define iupcocoaLogError(...) os_log_error(OS_LOG_DEFAULT, __VA_ARGS__)
#define iupcocoaLogCritical(...) os_log_fault(OS_LOG_DEFAULT, __VA_ARGS__)

#define iupcocoaNSLog(FORMAT, ...) os_log_info(OS_LOG_DEFAULT, "%{public}@", [NSString stringWithFormat:FORMAT, ##__VA_ARGS__])

IUP_DRV_API NSObject* iupcocoaGetRootObject(Ihandle* ih);
IUP_DRV_API NSView* iupcocoaGetRootView(Ihandle* ih);
IUP_DRV_API NSView* iupcocoaGetMainView(Ihandle* ih);
IUP_DRV_API void iupcocoaSetAssociatedViews(Ihandle* ih, NSView* main_view, NSView* root_view);

IUP_DRV_API void iupcocoaAddToParent(Ihandle* ih);
IUP_DRV_API void iupcocoaRemoveFromParent(Ihandle* ih);

NSView* iupcocoaCommonBaseLayoutGetParentView(Ihandle* ih);
NSView* iupcocoaCommonBaseLayoutGetChildView(Ihandle* ih);

void iupcocoaCommonLoopCallExitCb(void);

/* Coordinate conversion helpers: Cocoa uses Cartesian (y-up), IUP uses (y-down). */
int iupcocoaComputeCartesianScreenHeightFromIup(int iup_height);
int iupcocoaComputeIupScreenHeightFromCartesian(int cartesian_height);

/* Tooltip Functions */
IUP_DRV_API void cocoaUpdateTip(Ihandle* ih);
IUP_DRV_API void iupcocoaTipsDestroy(Ihandle* ih);

/* Menu Functions */
int iupcocoaMenuIsApplicationBar(Ihandle* ih);
void iupcocoaMenuSetApplicationMenu(Ihandle* ih);
Ihandle* iupcocoaMenuGetApplicationMenu(void);
void iupcocoaMenuCleanupApplicationMenu(void);
void iupcocoaEnsureDefaultApplicationMenu(void);

/* Helpers for mouse button and motion events. */
IUP_DRV_API int iupcocoaCommonBaseIupButtonForCocoaButton(NSInteger which_cocoa_button);
IUP_DRV_API bool iupcocoaCommonBaseHandleMouseButtonCallback(Ihandle* ih, NSEvent* the_event, NSView* represented_view, bool is_pressed);
IUP_DRV_API bool iupcocoaCommonBaseHandleMouseMotionCallback(Ihandle* ih, NSEvent* the_event, NSView* represented_view);

/* WHEEL_CB: WARNING: IUP does not support delta-y axis. */
IUP_DRV_API bool iupcocoaCommonBaseScrollWheelCallback(Ihandle* ih, NSEvent* the_event, NSView* represented_view);

/* For Layer Backed Views. */
IUP_DRV_API int iupcocoaCommonBaseSetLayerBackedAttrib(Ihandle* ih, const char* value);
IUP_DRV_API char* iupCocoaCommonBaseGetLayerBackedAttrib(Ihandle* ih);

/* Helpers for NSResponder context menus. */
IUP_DRV_API void iupcocoaCommonBaseAppendMenuItems(NSMenu* dst_menu, NSMenu* src_menu);
IUP_DRV_API void iupcocoaCommonBaseAppendDefaultMenuItemsForClassType(NSMenu* dst_menu, Class class_of_widget);
IUP_DRV_API void iupcocoaCommonBaseSetContextMenuForWidget(Ihandle* ih, id ih_widget_to_attach_menu_to, Ihandle* menu_ih);
IUP_DRV_API int iupcocoaCommonBaseSetContextMenuAttrib(Ihandle* ih, const char* value);
IUP_DRV_API char* iupcocoaCommonBaseGetContextMenuAttrib(Ihandle* ih);

/* Send an action through the responder chain (e.g., "undo:", "copy:"). */
IUP_DRV_API int iupcocoaCommonBaseSetSendActionAttrib(Ihandle* ih, const char* value);

/* Helpers for keyboard events. */
IUP_DRV_API bool iupcocoaKeyEvent(Ihandle *ih, NSEvent* ns_event, int mac_key_code, bool is_pressed);
IUP_DRV_API bool iupcocoaModifierEvent(Ihandle *ih, NSEvent* ns_event, int mac_key_code);
IUP_DRV_API int iupcocoaKeyDecode(CGEventRef event);
IUP_DRV_API void iupcocoaButtonKeySetStatus(NSEvent* ns_event, char* out_status);

@interface IupCocoaFont : NSObject

@property(nonatomic, retain) NSFont* nativeFont;
@property(nonatomic, retain) NSString* iupFontName;
@property(nonatomic, retain) NSString* typeFace;
@property(nonatomic, retain) NSMutableDictionary* attributeDictionary;
@property(nonatomic, assign) BOOL usesAttributes;
@property(nonatomic, assign) int fontSize;
@property(nonatomic, assign) int charWidth;
@property(nonatomic, assign) int charHeight;
@property(nonatomic, assign) int maxWidth;
@property(nonatomic, assign) int ascent;
@property(nonatomic, assign) int descent;

@end

IUP_DRV_API IupCocoaFont* iupcocoaGetFont(Ihandle* ih);
IUP_DRV_API IupCocoaFont* iupcocoaFindFont(const char *iup_font_name);

IUP_DRV_API void iupdrvTextAddBorders(Ihandle* ih, int *x, int *y);

/* Image conversion helpers. */
int iupcocoaImageCalculateBytesPerRow(int width, int bytes_per_pixel);
NSBitmapImageRep* iupcocoaImageNSBitmapImageRepFromPixels(int width, int height, int bpp, iupColor* colors, int colors_count, unsigned char *imgdata);
NSImage* iupcocoaImageNSImageFromPixels(int width, int height, int bpp, iupColor* colors, int colors_count, unsigned char *imgdata);

/* Handles IUP_CLOSE for modal dialogs. */
bool cocoaDialogExitModal(Ihandle* ih);

/* Get dialog window. */
NSWindow* cocoaDialogGetWindow(Ihandle* ih);

IUP_DRV_API void iupdrvDialogSetVisible(Ihandle* ih, int visible);
IUP_DRV_API int iupdrvDialogIsVisible(Ihandle* ih);

/* Focus control. */
IUP_DRV_API void iupcocoaSetCanFocus(Ihandle* ih, int can);
IUP_DRV_API void iupcocoaFocusIn(Ihandle* ih);
IUP_DRV_API void iupcocoaFocusOut(Ihandle* ih);

/* System information */
IUP_DRV_API int iupcocoaIsSystemDarkMode(void);

#ifdef __cplusplus
}
#endif

#endif