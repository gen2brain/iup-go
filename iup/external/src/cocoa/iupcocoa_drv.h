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

#ifndef GNUSTEP
#include <os/log.h>
#endif

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <objc/runtime.h>

#ifdef GNUSTEP
#include "iupcocoa_gnustep.h"
#endif

#include "iup_export.h"
#include "iup_image.h"


/* A unique memory address for an identifier. */
IUP_DRV_API extern const void* IHANDLE_ASSOCIATED_OBJ_KEY;
/* The main view within a root view (e.g., a scrollview contains the real widget). */
IUP_DRV_API extern const void* MAINVIEW_ASSOCIATED_OBJ_KEY;
/* The root view, in case the root object is not a view. */
IUP_DRV_API extern const void* ROOTVIEW_ASSOCIATED_OBJ_KEY;

#ifndef GNUSTEP
/* Apple os_log macros; GNUstep uses NSLog-based replacements from iupcocoa_gnustep.h. */
#define iupcocoaLog(...) os_log(OS_LOG_DEFAULT, __VA_ARGS__)
#define iupcocoaLogDebug(...) os_log_debug(OS_LOG_DEFAULT, __VA_ARGS__)
#define iupcocoaLogInfo(...) os_log_info(OS_LOG_DEFAULT, __VA_ARGS__)
#define iupcocoaLogNotice(...) os_log(OS_LOG_DEFAULT, __VA_ARGS__)
#define iupcocoaLogWarning(...) os_log_error(OS_LOG_DEFAULT, __VA_ARGS__)
#define iupcocoaLogError(...) os_log_error(OS_LOG_DEFAULT, __VA_ARGS__)
#define iupcocoaLogCritical(...) os_log_fault(OS_LOG_DEFAULT, __VA_ARGS__)
#define iupcocoaNSLog(FORMAT, ...) os_log_info(OS_LOG_DEFAULT, "%{public}@", [NSString stringWithFormat:FORMAT, ##__VA_ARGS__])
#endif /* !GNUSTEP */

IUP_DRV_API NSView* iupcocoaGetRootView(Ihandle* ih);
IUP_DRV_API NSView* iupcocoaGetMainView(Ihandle* ih);
IUP_DRV_API void iupcocoaSetAssociatedViews(Ihandle* ih, NSView* main_view, NSView* root_view);

IUP_DRV_API void iupcocoaAddToParent(Ihandle* ih);
IUP_DRV_API void iupcocoaRemoveFromParent(Ihandle* ih);

IUP_DRV_API NSView* iupcocoaCommonBaseLayoutGetParentView(Ihandle* ih);
IUP_DRV_API NSView* iupcocoaCommonBaseLayoutGetChildView(Ihandle* ih);

/* Coordinate conversion helpers: Cocoa uses Cartesian (y-up), IUP uses (y-down). */
IUP_DRV_API int iupcocoaComputeCartesianScreenHeightFromIup(int iup_height);
IUP_DRV_API int iupcocoaComputeIupScreenHeightFromCartesian(int cartesian_height);

/* Tooltip Functions */
IUP_DRV_API void iupcocoaUpdateTip(Ihandle* ih);
IUP_DRV_API void iupcocoaTipsDestroy(Ihandle* ih);

/* Menu Functions */
IUP_DRV_API int iupcocoaMenuIsApplicationBar(Ihandle* ih);
IUP_DRV_API void iupcocoaMenuSetApplicationMenu(Ihandle* ih);
IUP_DRV_API Ihandle* iupcocoaMenuGetApplicationMenu(void);
IUP_DRV_API void iupcocoaMenuCleanupApplicationMenu(void);
IUP_DRV_API void iupcocoaEnsureDefaultApplicationMenu(void);

/* Helpers for mouse button and motion events. */
IUP_DRV_API int iupcocoaCommonBaseIupButtonForCocoaButton(NSInteger which_cocoa_button);
IUP_DRV_API bool iupcocoaCommonBaseHandleMouseButtonCallback(Ihandle* ih, NSEvent* the_event, NSView* represented_view, bool is_pressed);
IUP_DRV_API bool iupcocoaCommonBaseHandleMouseMotionCallback(Ihandle* ih, NSEvent* the_event, NSView* represented_view);

/* WHEEL_CB: WARNING: IUP does not support delta-y axis. */
IUP_DRV_API bool iupcocoaCommonBaseScrollWheelCallback(Ihandle* ih, NSEvent* the_event, NSView* represented_view);

/* Helpers for NSResponder context menus. */
IUP_DRV_API void iupcocoaCommonBaseAppendMenuItems(NSMenu* dst_menu, NSMenu* src_menu);
IUP_DRV_API void iupcocoaCommonBaseAppendDefaultMenuItemsForClassType(NSMenu* dst_menu, Class class_of_widget);
IUP_DRV_API void iupcocoaCommonBaseSetContextMenuForWidget(Ihandle* ih, id ih_widget_to_attach_menu_to, Ihandle* menu_ih);
IUP_DRV_API int iupcocoaCommonBaseSetContextMenuAttrib(Ihandle* ih, const char* value);
IUP_DRV_API char* iupcocoaCommonBaseGetContextMenuAttrib(Ihandle* ih);

/* Helpers for keyboard events. */
IUP_DRV_API bool iupcocoaKeyEvent(Ihandle *ih, NSEvent* ns_event, int mac_key_code, bool is_pressed);
IUP_DRV_API bool iupcocoaModifierEvent(Ihandle *ih, NSEvent* ns_event, int mac_key_code);
#ifndef GNUSTEP
IUP_DRV_API int iupcocoaKeyDecode(CGEventRef event);
#endif
IUP_DRV_API void iupcocoaButtonKeySetStatus(NSEvent* ns_event, char* out_status);

#ifdef GNUSTEP
/* Apply the handful of tweaks every NSTableView/NSOutlineView needs on GNUstep:
   drawsGrid=NO (setGridStyleMask is a no-op and _drawsGrid defaults YES),
   autoresizesAllColumnsToFit=YES (setColumnAutoresizingStyle is also a no-op). */
IUP_DRV_API void iupcocoaGnustepConfigureTableView(NSTableView* tableView);
/* Intrinsic content height with fallbacks (NSView -intrinsicContentSize is (-1,-1) on GNUstep):
   prefer -sizeToFit, then font metrics, and never return below `minimum`. */
IUP_DRV_API int iupcocoaGnustepIntrinsicHeight(NSControl* control, int minimum);
/* Fill a cellView's background under GNUstep so its transparent textField doesn't stamp
   text over prior pixels. */
IUP_DRV_API void iupcocoaGnustepFillCellRect(NSView* cellView, NSRect dirtyRect, NSColor* customBackgroundColor);
/* Clamp negative width/height to zero (Opal/cairo locks on zero-determinant CTMs). */
static inline NSRect iupcocoaClampRect(NSRect r)
{
  if (r.size.width  < 0) r.size.width  = 0;
  if (r.size.height < 0) r.size.height = 0;
  return r;
}
#endif

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

IUP_DRV_API NSMutableAttributedString* iupcocoaBuildMarkupAttributedString(Ihandle* ih, const char* value);

/* Image conversion helpers. */
IUP_DRV_API int iupcocoaImageCalculateBytesPerRow(int width, int bytes_per_pixel);

/* Handles IUP_CLOSE for modal dialogs. */
IUP_DRV_API bool iupcocoaDialogExitModal(Ihandle* ih);

/* Get dialog window. */
IUP_DRV_API NSWindow* iupcocoaDialogGetWindow(Ihandle* ih);

/* Focus control. */
IUP_DRV_API void iupcocoaSetCanFocus(Ihandle* ih, int can);
IUP_DRV_API void iupcocoaFocusIn(Ihandle* ih);
IUP_DRV_API void iupcocoaFocusOut(Ihandle* ih);

/* System information */
IUP_DRV_API int iupcocoaIsSystemDarkMode(void);
IUP_DRV_API void iupcocoaSetGlobalColors(void);

/* Tray support */
IUP_DRV_API int iupcocoaTrayGetLastButton(void);
IUP_DRV_API int iupcocoaTrayGetLastDclick(void);

#ifdef __cplusplus
}
#endif

#endif
