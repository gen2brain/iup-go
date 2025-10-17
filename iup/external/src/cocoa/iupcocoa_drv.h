/** \file
 * \brief MAC Driver
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

#define iupAppleLog(...) os_log(OS_LOG_DEFAULT, __VA_ARGS__)
#define iupAppleLogDebug(...) os_log_debug(OS_LOG_DEFAULT, __VA_ARGS__)
#define iupAppleLogInfo(...) os_log_info(OS_LOG_DEFAULT, __VA_ARGS__)
#define iupAppleLogNotice(...) os_log(OS_LOG_DEFAULT, __VA_ARGS__)
#define iupAppleLogWarning(...) os_log_error(OS_LOG_DEFAULT, __VA_ARGS__)
#define iupAppleLogError(...) os_log_error(OS_LOG_DEFAULT, __VA_ARGS__)
#define iupAppleLogCritical(...) os_log_fault(OS_LOG_DEFAULT, __VA_ARGS__)

#define iupAppleNSLog(FORMAT, ...) os_log_info(OS_LOG_DEFAULT, "%{public}@", [NSString stringWithFormat:FORMAT, ##__VA_ARGS__])

IUP_DRV_API NSObject* iupCocoaGetRootObject(Ihandle* ih);
IUP_DRV_API NSView* iupCocoaGetRootView(Ihandle* ih);
IUP_DRV_API NSView* iupCocoaGetMainView(Ihandle* ih);
IUP_DRV_API void iupCocoaSetAssociatedViews(Ihandle* ih, NSView* main_view, NSView* root_view);

IUP_DRV_API void iupCocoaAddToParent(Ihandle* ih);
IUP_DRV_API void iupCocoaRemoveFromParent(Ihandle* ih);

NSView* iupCocoaCommonBaseLayoutGetParentView(Ihandle* ih);
NSView* iupCocoaCommonBaseLayoutGetChildView(Ihandle* ih);

void iupCocoaCommonLoopCallExitCb(void);

/* Coordinate conversion helpers: Cocoa uses Cartesian (y-up), IUP uses (y-down). */
int iupCocoaComputeCartesianScreenHeightFromIup(int iup_height);
int iupCocoaComputeIupScreenHeightFromCartesian(int cartesian_height);

/* Tooltip Functions */
IUP_DRV_API void iupdrvUpdateTip(Ihandle* ih);
IUP_DRV_API void iupCocoaTipsDestroy(Ihandle* ih);

/* Menu Functions */
int iupCocoaMenuIsApplicationBar(Ihandle* ih);
void iupCocoaMenuSetApplicationMenu(Ihandle* ih);
Ihandle* iupCocoaMenuGetApplicationMenu(void);
void iupCocoaMenuCleanupApplicationMenu(void);
void iupCocoaEnsureDefaultApplicationMenu(void);

/* Helpers for mouse button and motion events. */
IUP_DRV_API int iupCocoaCommonBaseIupButtonForCocoaButton(NSInteger which_cocoa_button);
IUP_DRV_API bool iupCocoaCommonBaseHandleMouseButtonCallback(Ihandle* ih, NSEvent* the_event, NSView* represented_view, bool is_pressed);
IUP_DRV_API bool iupCocoaCommonBaseHandleMouseMotionCallback(Ihandle* ih, NSEvent* the_event, NSView* represented_view);

/* WHEEL_CB: WARNING: IUP does not support delta-y axis. */
IUP_DRV_API bool iupCocoaCommonBaseScrollWheelCallback(Ihandle* ih, NSEvent* the_event, NSView* represented_view);

/* For Layer Backed Views. */
IUP_DRV_API int iupCocoaCommonBaseSetLayerBackedAttrib(Ihandle* ih, const char* value);
IUP_DRV_API char* iupCocoaCommonBaseGetLayerBackedAttrib(Ihandle* ih);

/* Helpers for NSResponder context menus. */
IUP_DRV_API void iupCocoaCommonBaseAppendMenuItems(NSMenu* dst_menu, NSMenu* src_menu);
IUP_DRV_API void iupCocoaCommonBaseAppendDefaultMenuItemsForClassType(NSMenu* dst_menu, Class class_of_widget);
IUP_DRV_API void iupCocoaCommonBaseSetContextMenuForWidget(Ihandle* ih, id ih_widget_to_attach_menu_to, Ihandle* menu_ih);
IUP_DRV_API int iupCocoaCommonBaseSetContextMenuAttrib(Ihandle* ih, const char* value);
IUP_DRV_API char* iupCocoaCommonBaseGetContextMenuAttrib(Ihandle* ih);

/* Send an action through the responder chain (e.g., "undo:", "copy:"). */
IUP_DRV_API int iupCocoaCommonBaseSetSendActionAttrib(Ihandle* ih, const char* value);

/* Helpers for keyboard events. */
IUP_DRV_API bool iupCocoaKeyEvent(Ihandle *ih, NSEvent* ns_event, int mac_key_code, bool is_pressed);
IUP_DRV_API bool iupCocoaModifierEvent(Ihandle *ih, NSEvent* ns_event, int mac_key_code);
IUP_DRV_API int iupCocoaKeyDecode(CGEventRef event);
IUP_DRV_API void iupCocoaButtonKeySetStatus(NSEvent* ns_event, char* out_status);

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

IUP_DRV_API IupCocoaFont* iupCocoaGetFont(Ihandle* ih);
IUP_DRV_API IupCocoaFont* iupCocoaFindFont(const char *iup_font_name);

IUP_DRV_API void iupdrvTextAddBorders(Ihandle* ih, int *x, int *y);

/* Image conversion helpers. */
int iupCocoaImageCalculateBytesPerRow(int width, int bytes_per_pixel);
NSBitmapImageRep* iupCocoaImageNSBitmapImageRepFromPixels(int width, int height, int bpp, iupColor* colors, int colors_count, unsigned char *imgdata);
NSImage* iupCocoaImageNSImageFromPixels(int width, int height, int bpp, iupColor* colors, int colors_count, unsigned char *imgdata);

/* Handles IUP_CLOSE for modal dialogs. */
bool cocoaDialogExitModal(Ihandle* ih);

/* Get dialog window. */
NSWindow* cocoaDialogGetWindow(Ihandle* ih);

IUP_DRV_API void iupdrvDialogSetVisible(Ihandle* ih, int visible);
IUP_DRV_API int iupdrvDialogIsVisible(Ihandle* ih);

/* Focus control. */
IUP_DRV_API void iupCocoaSetCanFocus(Ihandle* ih, int can);
IUP_DRV_API void iupCocoaFocusIn(Ihandle* ih);
IUP_DRV_API void iupCocoaFocusOut(Ihandle* ih);

#ifdef __cplusplus
}
#endif

#endif