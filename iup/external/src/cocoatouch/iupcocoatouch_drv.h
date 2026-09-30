/** \file
 * \brief iOS UIKit Driver
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUPCOCOATOUCH_DRV_H
#define __IUPCOCOATOUCH_DRV_H

#ifdef __OBJC__
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#endif

#include "iup_export.h"
#include "iup.h"

#ifdef __cplusplus
extern "C" {
#endif

IUP_DRV_API extern const void* IHANDLE_ASSOCIATED_OBJ_KEY;

#ifdef __OBJC__

/* horizontal scroller around a WORDWRAP=NO multiline text view; the text view scrolls vertically */
@interface IupCocoaTouchTextScroll : UIScrollView
@property(nonatomic, retain) UITextView* textView;
- (void)revealRange:(NSRange)range;
@end

/* absolute-positioning client-area container; IupViewController reframes it to safe-area rect */
@interface IupCocoaTouchFixed : UIView
@property(nonatomic, assign) Ihandle* ihandle;
@end

/* cached UIFont + IUP sizing metrics, non-owning for callers */
@interface IupCocoaTouchFont : NSObject
@property(nonatomic, retain) UIFont* nativeFont;
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

IUP_DRV_API UIWindow* iupCocoaTouchFindCurrentWindow(void);
IUP_DRV_API UIViewController* iupCocoaTouchFindCurrentRootViewController(void);
IUP_DRV_API UIViewController* iupCocoaTouchFindTopPresentedViewController(void);

IUP_DRV_API bool iupCocoaTouchIsLaunchPlaceholder(UIViewController* vc);
IUP_DRV_API UIViewController* iupCocoaTouchNewLaunchPlaceholder(void);

/* client-area Fixed of a mapped Dialog, nil otherwise */
IUP_DRV_API IupCocoaTouchFixed* iupCocoaTouchDialogGetClientArea(Ihandle* dialog_ih);

IUP_DRV_API const char* iupCocoaTouchFontFaceName(const char* face);
IUP_DRV_API IupCocoaTouchFont* iupCocoaTouchFindFont(const char* iup_font_name);
IUP_DRV_API IupCocoaTouchFont* iupCocoaTouchGetFont(Ihandle* ih);

IUP_DRV_API UIColor* iupCocoaTouchToNativeColor(const char* color);
IUP_DRV_API char* iupCocoaTouchColorFromNative(UIColor* color);

IUP_DRV_API void iupCocoaTouchAddToParent(Ihandle* ih);
IUP_DRV_API void iupCocoaTouchRemoveFromParent(Ihandle* ih);

@interface UIResponder (IupCocoaTouchKeyCommand)
- (void)iupCocoaTouchKeyCommand:(UIKeyCommand*)command;
@end

/* returns true when handled (stops propagation) */
IUP_DRV_API bool iupCocoaTouchKeyEvent(Ihandle* ih, UIPress* press, bool is_pressed);
IUP_DRV_API bool iupCocoaTouchKeyPresses(Ihandle* ih, NSSet<UIPress*>* presses, bool is_pressed);
IUP_DRV_API void iupCocoaTouchKeyUpdateResponder(void);
IUP_DRV_API UIResponder* iupCocoaTouchKeyFirstResponder(void);

/* Tab, Shift+Tab and Esc go to the text input system before any press */
IUP_DRV_API NSArray<UIKeyCommand*>* iupCocoaTouchKeyCommands(void);
IUP_DRV_API bool iupCocoaTouchKeyCommandAllowed(UIResponder* responder);
IUP_DRV_API void iupCocoaTouchKeyCommandEvent(Ihandle* ih, UIKeyCommand* command, UIResponder* responder);

/* fills IUP status string (10 chars + NUL) */
IUP_DRV_API void iupCocoaTouchButtonKeySetStatus(UIEvent* event, UIKeyModifierFlags modifier_flags, int pressed_button, int doubleclick, char* out_status);

/* Pango subset -> NSAttributedString, nil if no markup */
IUP_DRV_API NSAttributedString* iupCocoaTouchParseMarkup(const char* raw, UIFont* base_font, UIColor* base_color);

/* theme-flip refresh bus: widgets attach a block, the trait observer walks every visible view on flip */
typedef void (^IupCocoaTouchThemeRefreshBlock)(UIView* view);
IUP_DRV_API void iupCocoaTouchRegisterThemeRefresh(UIView* view, IupCocoaTouchThemeRefreshBlock block);
IUP_DRV_API void iupCocoaTouchRefreshAllThemes(void);

IUP_DRV_API NSString* iupCocoaTouchDragTypeToUTI(const char* iup_type);
IUP_DRV_API NSArray<NSString*>* iupCocoaTouchDragParseTypes(const char* csv);

IUP_DRV_API void iupCocoaTouchHandleTraitFlip(void);

#endif /* __OBJC__ */

/* string-typed so it matches IattribSetFunc */
IUP_DRV_API int iupCocoaTouchSetBgColorAttrib(Ihandle* ih, const char* value);

/* IupAppDelegate flips this once ENTRY_POINT returns; before then IupMainLoop returns IUP_OPENED */
IUP_DRV_API void iupCocoaTouchMarkEntryFinished(void);

#define IUPCOCOATOUCH_UTI_PDF        @"com.adobe.pdf"
#define IUPCOCOATOUCH_UTI_PNG        @"public.png"
#define IUPCOCOATOUCH_UTI_JPEG       @"public.jpeg"
#define IUPCOCOATOUCH_UTI_TIFF       @"public.tiff"
#define IUPCOCOATOUCH_UTI_GIF        @"com.compuserve.gif"
#define IUPCOCOATOUCH_UTI_BMP        @"com.microsoft.bmp"
#define IUPCOCOATOUCH_UTI_HTML       @"public.html"
#define IUPCOCOATOUCH_UTI_PLAIN_TEXT @"public.utf8-plain-text"
#define IUPCOCOATOUCH_UTI_FILE_URL   @"public.file-url"

#ifdef __cplusplus
}
#endif

#endif
