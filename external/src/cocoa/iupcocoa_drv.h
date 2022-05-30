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

#include <asl.h>
#if __OBJC__
	#import <Foundation/Foundation.h>
	#import "IupCocoaFont.h"
#endif

#include "iup_image.h"

// the point of this is we have a unique memory address for an identifier
IUP_DRV_API extern const void* IHANDLE_ASSOCIATED_OBJ_KEY;
// ih->handle is the root view, but often the main view is lower down, (e.g. scrollview contains the real widget).
// This pattern turned up enough that it merited adding this key.
// This was introduced late into the development process so a lot of code may continue to fetch this the hard way.
IUP_DRV_API extern const void* MAINVIEW_ASSOCIATED_OBJ_KEY;
// Formalize the rootview in case the root object is not a view
IUP_DRV_API extern const void* ROOTVIEW_ASSOCIATED_OBJ_KEY;


	
// TODO: Move to os_log, but requires Mac 10.12
#define iupAppleLog(...) asl_log(NULL, NULL, ASL_LEVEL_NOTICE, __VA_ARGS__)
#define iupAppleLogDebug(...) asl_log(NULL, NULL, ASL_LEVEL_DEBUG, __VA_ARGS__)
#define iupAppleLogInfo(...) asl_log(NULL, NULL, ASL_LEVEL_INFO, __VA_ARGS__)
#define iupAppleLogNotice(...) asl_log(NULL, NULL, ASL_LEVEL_NOTICE, __VA_ARGS__)
#define iupAppleLogWarning(...) asl_log(NULL, NULL, ASL_LEVEL_WARNING, __VA_ARGS__)
#define iupAppleLogError(...) asl_log(NULL, NULL, ASL_LEVEL_ERR, __VA_ARGS__)
#define iupAppleLogCritical(...) asl_log(NULL, NULL, ASL_LEVEL_CRIT, __VA_ARGS__)

#if __OBJC__
#define iupAppleNSLog(...) asl_log(NULL, NULL, ASL_LEVEL_INFO, "%s", [[NSString stringWithFormat:__VA_ARGS__] UTF8String])
#endif
	
IUP_DRV_API NSObject* iupCocoaGetRootObject(Ihandle* ih);
IUP_DRV_API NSView* iupCocoaGetRootView(Ihandle* ih);
IUP_DRV_API NSView* iupCocoaGetMainView(Ihandle* ih);
IUP_DRV_API void iupCocoaSetAssociatedViews(Ihandle* ih, NSView* main_view, NSView* root_view);

IUP_DRV_API void iupCocoaAddToParent(Ihandle* ih);
IUP_DRV_API void iupCocoaRemoveFromParent(Ihandle* ih);
	
NSView* iupCocoaCommonBaseLayoutGetParentView(Ihandle* ih);
NSView* iupCocoaCommonBaseLayoutGetChildView(Ihandle* ih);
NSRect iupCocoaCommonBaseLayoutComputeChildFrameRectFromParentRect(Ihandle* ih, NSRect parent_rect);
	
void iupCocoaCommonLoopCallExitCb(void);

	// Cocoa is in Cartesian (a.k.a. math book, aka OpenGL coordinates, aka y increases upwards), but Iup is y increases downwards.
int iupCocoaComputeCartesianScreenHeightFromIup(int iup_height);
int iupCocoaComputeIupScreenHeightFromCartesian(int cartesian_height);


int iupCocoaMenuIsApplicationBar(Ihandle* ih);
void iupCocoaMenuSetApplicationMenu(Ihandle* ih);
// Note: This only gets the user's Ihandle to the application menu. If the user doesn't set it, the default application will not be returned in its place. NULL will be returned instead.
Ihandle* iupCocoaMenuGetApplicationMenu(void);
// My current understanding is that IUP will not clean up our application menu Ihandles. So we need to do it ourselves.
void iupCocoaMenuCleanupApplicationMenu(void);


// Helper functions for implementing the mouseDown/mouseUp, mouseDragged family of functions.
IUP_DRV_API int iupCocoaCommonBaseIupButtonForCocoaButton(NSInteger which_cocoa_button);
// mouseDown: and mouseUp: overrides can call this
// Returns a boolean specifying if the caller_should_not_propagate. (Trying to conform to the iupgtk counterpart.) So if false, call super, otherwise skip.
IUP_DRV_API bool iupCocoaCommonBaseHandleMouseButtonCallback(Ihandle* ih, NSEvent* the_event, NSView* represented_view, bool is_pressed);
// mouseDragged: overrides can call this
// Returns a boolean specifying if the caller_should_not_propagate. (Trying to conform to the iupgtk counterpart.) So if false, call super, otherwise skip.
IUP_DRV_API bool iupCocoaCommonBaseHandleMouseMotionCallback(Ihandle* ih, NSEvent* the_event, NSView* represented_view);

// WHEEL_CB: WARNING: IUP does not support delta-y axis.
IUP_DRV_API bool iupCocoaCommonBaseScrollWheelCallback(Ihandle* ih, NSEvent* the_event, NSView* represented_view);


// For Layer Backed Views
IUP_DRV_API int iupCocoaCommonBaseSetLayerBackedAttrib(Ihandle* ih, const char* value);
IUP_DRV_API char* iupCocoaCommonBaseGetLayerBackedAttrib(Ihandle* ih);

// This is for NSResponder context menus. (I expect new modules will want to call these.)
IUP_DRV_API void iupCocoaCommonBaseAppendMenuItems(NSMenu* dst_menu, NSMenu* src_menu);
IUP_DRV_API void iupCocoaCommonBaseAppendDefaultMenuItemsForClassType(NSMenu* dst_menu, Class class_of_widget);
IUP_DRV_API void iupCocoaCommonBaseSetContextMenuForWidget(Ihandle* ih, id ih_widget_to_attach_menu_to, Ihandle* menu_ih);
IUP_DRV_API int iupCocoaCommonBaseSetContextMenuAttrib(Ihandle* ih, const char* value);
IUP_DRV_API char* iupCocoaCommonBaseGetContextMenuAttrib(Ihandle* ih);

// Send an action through the responder chain.
// @param ih The ih belonging to the widget you want to send to. (IupCocoaGetMainView(ih) must be valid). If ih is NULL, this will send to the first responder (similar to what happens when you select a menu item).
// @param value Should be the selector name including the colon. Case sensitive. e.g. "undo:", "redo:", "cut:", "copy:", "paste:", "pasteAsPlainText:"
IUP_DRV_API int iupCocoaCommonBaseSetSendActionAttrib(Ihandle* ih, const char* value);


// All keyDown: and keyUp: overrides can call this method to handle K_ANY, the Canvas Keypress, and HELP_CB.
// Returns a boolean specifying if the caller_should_not_propagate. (Trying to conform to the iupgtk counterpart.) So if false, call super, otherwise skip.
IUP_DRV_API bool iupCocoaKeyEvent(Ihandle *ih, NSEvent* ns_event, int mac_key_code, bool is_pressed);
// All flagsChanged: overrides can call this function, which in turn calls iupCocoaKeyEvent() internally.
// Returns a boolean specifying if the caller_should_not_propagate. (Trying to conform to the iupgtk counterpart.) So if false, call super, otherwise skip.
IUP_DRV_API bool iupCocoaModifierEvent(Ihandle *ih, NSEvent* ns_event, int mac_key_code);

IUP_DRV_API void iupcocoaButtonKeySetStatus(NSEvent* ns_event, char* out_status);


IUP_DRV_API IupCocoaFont* iupCocoaGetFont(Ihandle* ih);


int iupCocoaImageCaluclateBytesPerRow(int width, int bytes_per_pixel);
NSBitmapImageRep* iupCocoaImageNSBitmapImageRepFromPixels(int width, int height, int bpp, iupColor* colors, int colors_count, unsigned char *imgdata);
NSImage* iupCocoaImageNSImageFromPixels(int width, int height, int bpp, iupColor* colors, int colors_count, unsigned char *imgdata);



// Kind of a hack to handle IUP_CLOSE for modal dialogs
bool cocoaDialogExitModal(void);


#if 0
/* global variables, declared in iupmac_globalattrib.c */
extern int iupmac_utf8autoconvert;         


/* common */
gboolean iupmacEnterLeaveEvent(GtkWidget *widget, GdkEventCrossing *evt, Ihandle* ih);
gboolean iupmacShowHelp(GtkWidget *widget, GtkWidgetHelpType *arg1, Ihandle* ih);
GtkFixed* iupmacBaseGetFixed(Ihandle* ih);
void iupmacBaseAddToParent(Ihandle* ih);
void iupgdkColorSet(GdkColor* color, unsigned char r, unsigned char g, unsigned char b);
int iupmacSetDragDropAttrib(Ihandle* ih, const char* value);
int iupmacSetMnemonicTitle(Ihandle* ih, GtkLabel* label, const char* value);
char* iupmacStrConvertToUTF8(const char* str);
char* iupmacStrConvertFromUTF8(const char* str);
void iupmacReleaseConvertUTF8(void);
char* iupmacStrConvertFromFilename(const char* str);
char* iupmacStrConvertToFilename(const char* str);
void iupmacUpdateMnemonic(Ihandle* ih);
gboolean iupmacMotionNotifyEvent(GtkWidget *widget, GdkEventMotion *evt, Ihandle *ih);
gboolean iupmacButtonEvent(GtkWidget *widget, GdkEventButton *evt, Ihandle *ih);
void iupmacBaseSetBgColor(InativeHandle* handle, unsigned char r, unsigned char g, unsigned char b);
void iupmacBaseSetFgColor(InativeHandle* handle, unsigned char r, unsigned char g, unsigned char b);
void iupmacBaseSetFgGdkColor(InativeHandle* handle, GdkColor *color);


/* focus */
gboolean iupmacFocusInOutEvent(GtkWidget *widget, GdkEventFocus *evt, Ihandle* ih);


/* key */
gboolean iupmacKeyPressEvent(GtkWidget *widget, GdkEventKey *evt, Ihandle* ih);
gboolean iupmacKeyReleaseEvent(GtkWidget *widget, GdkEventKey *evt, Ihandle* ih);
void iupmacButtonKeySetStatus(guint state, unsigned int but, char* status, int doubleclick);
// original code used guint. Not sure what those are; changing to NSUInteger
// FIXME: file does not compile
//void iupmacKeyEncode(int key, NSUInteger *keyval, NSUInteger *state);


/* font */
char* iupmacGetPangoFontDescAttrib(Ihandle *ih);
char* iupmacGetFontIdAttrib(Ihandle *ih);
PangoFontDescription* iupmacGetPangoFontDesc(const char* value);
char* iupmacFindPangoFontDesc(PangoFontDescription* fontdesc);
void iupmacFontUpdatePangoLayout(Ihandle* ih, PangoLayout* layout);
void iupmacFontUpdateObjectPangoLayout(Ihandle* ih, gpointer object);


/* open */
char* iupmacGetNativeWindowHandle(Ihandle* ih);
void iupmacPushVisualAndColormap(void* visual, void* colormap);
void* iupmacGetNativeGraphicsContext(GtkWidget* widget);
void iupmacReleaseNativeGraphicsContext(GtkWidget* widget, void* gc);
void iupmacUpdateGlobalColors(GtkStyle* style);


/* dialog */
gboolean iupmacDialogDeleteEvent(GtkWidget *widget, GdkEvent *evt, Ihandle *ih);


#endif

#ifdef __cplusplus
}
#endif

#endif
