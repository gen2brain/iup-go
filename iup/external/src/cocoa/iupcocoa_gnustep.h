/** \file
 * \brief GNUstep compatibility shim for the Cocoa driver.
 *
 * Included by iupcocoa_drv.h under #ifdef GNUSTEP. Provides enum/class renames,
 * missing typedef/#define constants, informal category declarations for AppKit
 * methods GNUstep's public headers don't export, and NSLog-based log macros.
 * Implementations (mostly no-ops) live in iupcocoa_common.m.
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUPCOCOA_GNUSTEP_H
#define __IUPCOCOA_GNUSTEP_H

#ifndef GNUSTEP
#error "iupcocoa_gnustep.h must only be included under #ifdef GNUSTEP"
#endif

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <CoreGraphics/CoreGraphics.h>


/* Enum renames: macOS 10.12+ modernized constants mapped to legacy GNUstep names. */

#define NSAlertStyleCritical                        NSCriticalAlertStyle
#define NSAlertStyleWarning                         NSWarningAlertStyle
#define NSAlertStyleInformational                   NSInformationalAlertStyle

#define NSBezelStyleRounded                         NSRoundedBezelStyle
#define NSBezelStyleRegularSquare                   NSRegularSquareBezelStyle
#define NSBezelStyleHelpButton                      NSHelpButtonBezelStyle

#define NSButtonTypeSwitch                          NSSwitchButton
#define NSButtonTypeRadio                           NSRadioButton
#define NSButtonTypePushOnPushOff                   NSPushOnPushOffButton
#define NSButtonTypeMomentaryPushIn                 NSMomentaryPushInButton
#define NSButtonTypeMomentaryChange                 NSMomentaryChangeButton

#define NSCompositingOperationSourceOver            NSCompositeSourceOver
#define NSCompositingOperationCopy                  NSCompositeCopy

#define NSEventMaskAny                              NSAnyEventMask
#define NSEventMaskLeftMouseDown                    NSLeftMouseDownMask
#define NSEventMaskLeftMouseUp                      NSLeftMouseUpMask
#define NSEventMaskLeftMouseDragged                 NSLeftMouseDraggedMask
#define NSEventMaskRightMouseDown                   NSRightMouseDownMask
#define NSEventMaskRightMouseUp                     NSRightMouseUpMask
#define NSEventMaskOtherMouseDown                   NSOtherMouseDownMask
#define NSEventMaskOtherMouseUp                     NSOtherMouseUpMask

#define NSBitmapImageFileTypePNG                    NSPNGFileType
#define NSBitmapImageFileTypeJPEG                   NSJPEGFileType
#define NSBitmapImageFileTypeBMP                    NSBMPFileType

#define NSProgressIndicatorStyleBar                 NSProgressIndicatorBarStyle

#define NSTickMarkPositionAbove                     NSTickMarkAbove
#define NSTickMarkPositionBelow                     NSTickMarkBelow
#define NSTickMarkPositionLeading                   NSTickMarkLeft
#define NSTickMarkPositionTrailing                  NSTickMarkRight

#define NSRectEdgeMinX                              NSMinXEdge
#define NSRectEdgeMinY                              NSMinYEdge
#define NSRectEdgeMaxX                              NSMaxXEdge
#define NSRectEdgeMaxY                              NSMaxYEdge

#define NSUnderlineStylePatternDot                  NSUnderlinePatternDot

#define NSDatePickerElementFlagYearMonthDay         NSYearMonthDayDatePickerElementFlag
#define NSDatePickerStyleClockAndCalendar           NSClockAndCalendarDatePickerStyle

#define NSControlStateValueOff                      NSOffState
#define NSControlStateValueOn                       NSOnState
#define NSControlStateValueMixed                    NSMixedState

#define NSBitmapFormatAlphaNonpremultiplied         NSAlphaNonpremultipliedBitmapFormat


/* Missing typedefs / constants. */

/* Pasteboard type alias (10.13+ on Apple; plain NSString* on GNUstep). */
typedef NSString* NSPasteboardType;

#ifndef NSPasteboardTypeURL
#define NSPasteboardTypeURL        NSURLPboardType
#endif
#ifndef NSPasteboardTypeFileURL
#define NSPasteboardTypeFileURL    NSFilenamesPboardType
#endif
#ifndef NSPasteboardNameDrag
#define NSPasteboardNameDrag       NSDragPboard
#endif

/* Menu validation protocol renamed 10.11+. */
#define NSMenuItemValidation       NSMenuValidation

/* Menu state image names; looked up at runtime, callers handle nil. */
#ifndef NSImageNameMenuOnStateTemplate
#define NSImageNameMenuOnStateTemplate    @"NSMenuCheckmark"
#endif
#ifndef NSImageNameMenuMixedStateTemplate
#define NSImageNameMenuMixedStateTemplate @"NSMenuMixedState"
#endif
#ifndef NSImageNameFolder
#define NSImageNameFolder          @"NSFolder"
#endif

/* Window title visibility values; the property isn't on GNUstep and sites are #ifndef-gated. */
#ifndef NSWindowTitleVisible
#define NSWindowTitleVisible       0
#endif
#ifndef NSWindowTitleHidden
#define NSWindowTitleHidden        1
#endif

/* Fullscreen notification name placeholders; actual observer sites are gated. */
#ifndef NSWindowWillEnterFullScreenNotification
#define NSWindowWillEnterFullScreenNotification @"NSWindowWillEnterFullScreenNotification"
#endif
#ifndef NSWindowDidEnterFullScreenNotification
#define NSWindowDidEnterFullScreenNotification  @"NSWindowDidEnterFullScreenNotification"
#endif
#ifndef NSWindowDidExitFullScreenNotification
#define NSWindowDidExitFullScreenNotification   @"NSWindowDidExitFullScreenNotification"
#endif


/* Informal category declarations for methods GNUstep's public headers omit.
   Implementations (mostly no-ops) live in iupcocoa_common.m. */

@interface NSView (IupGnustepShim)
- (NSSize)fittingSize;
- (NSRect)convertRectToBacking:(NSRect)rect;
- (void)setClipsToBounds:(BOOL)flag;
- (void)updateTrackingAreas;
- (NSArray*)trackingAreas;
- (void)addTrackingArea:(NSTrackingArea*)area;
- (void)removeTrackingArea:(NSTrackingArea*)area;
- (void)setAccessibilityLabel:(NSString*)label;
- (void)viewWillDraw;
- (NSArray*)constraints;
- (void)removeConstraints:(NSArray*)constraints;
@end

@interface NSControl (IupGnustepShim)
- (void)setLineBreakMode:(NSLineBreakMode)mode;
- (void)setUsesSingleLineMode:(BOOL)flag;
- (void)setTruncatesLastVisibleLine:(BOOL)flag;
- (void)setControlSize:(NSInteger)size;
@end

@interface NSCell (IupGnustepShim)
- (void)setTextColor:(NSColor*)color;
@end

@interface NSSlider (IupGnustepShim)
- (void)setVertical:(BOOL)flag;
@end

@interface NSPasteboard (IupGnustepShim)
- (NSInteger)clearContents;
- (BOOL)writeObjects:(NSArray*)objects;
- (NSArray*)readObjectsForClasses:(NSArray*)classArray options:(NSDictionary*)options;
- (BOOL)canReadObjectForClasses:(NSArray*)classArray options:(NSDictionary*)options;
@end

@interface NSEvent (IupGnustepShim)
- (CGFloat)scrollingDeltaY;
@end

@interface NSBitmapImageRep (IupGnustepShim)
- (instancetype)initWithCGImage:(CGImageRef)cgImage;
@end

@interface NSImage (IupGnustepShim)
+ (NSArray*)imageTypes;
@end

@interface NSMenu (IupGnustepShim)
- (void)setMinimumWidth:(CGFloat)width;
- (NSSize)size;
@end

@interface NSOutlineView (IupGnustepShim)
- (void)setDraggingDestinationFeedbackStyle:(NSInteger)style;
- (void)setAllowsExpansionToolTips:(BOOL)flag;
- (void)removeItemsAtIndexes:(NSIndexSet*)indexes inParent:(id)parent withAnimation:(NSUInteger)options;
- (void)insertItemsAtIndexes:(NSIndexSet*)indexes inParent:(id)parent withAnimation:(NSUInteger)options;
- (void)moveItemAtIndex:(NSInteger)fromIndex inParent:(id)oldParent toIndex:(NSInteger)toIndex inParent:(id)newParent;
@end

@interface NSTableView (IupGnustepShim)
- (void)setDraggingDestinationFeedbackStyle:(NSInteger)style;
- (void)setRowSizeStyle:(NSInteger)style;
@end

@interface NSApplication (IupGnustepShim)
- (NSInteger)activationPolicy;
- (BOOL)setActivationPolicy:(NSInteger)activationPolicy;
- (void)setHelpMenu:(NSMenu*)helpMenu;
- (id)effectiveAppearance;
- (id)dockTile;
@end

@interface NSColor (IupGnustepShim)
+ (NSColor*)linkColor;
+ (NSColor*)selectedContentBackgroundColor;
+ (NSColor*)underPageBackgroundColor;
+ (NSColor*)separatorColor;
@end

@interface NSColorSpace (IupGnustepShim)
+ (NSColorSpace*)sRGBColorSpace;
@end

@interface NSFontManager (IupGnustepShim)
- (void)setTarget:(id)target;
@end

@interface NSTimer (IupGnustepShim)
- (void)setTolerance:(NSTimeInterval)tolerance;
@end

@interface NSTextView (IupGnustepShim)
- (void)insertText:(id)string replacementRange:(NSRange)range;
@end

@interface NSWindow (IupGnustepShim)
- (void)setStyleMask:(NSUInteger)styleMask;
- (void)toggleFullScreen:(id)sender;
+ (void)setAllowsAutomaticWindowTabbing:(BOOL)flag;
@end


/* Log macros: NSLog-based replacements for Apple's os_log family. */

#define iupcocoaLog(...)           NSLog(@__VA_ARGS__)
#define iupcocoaLogDebug(...)      ((void)0)
#define iupcocoaLogInfo(...)       NSLog(@__VA_ARGS__)
#define iupcocoaLogNotice(...)     NSLog(@__VA_ARGS__)
#define iupcocoaLogWarning(...)    NSLog(@__VA_ARGS__)
#define iupcocoaLogError(...)      NSLog(@__VA_ARGS__)
#define iupcocoaLogCritical(...)   NSLog(@__VA_ARGS__)
#define iupcocoaNSLog(FORMAT, ...) NSLog(FORMAT, ##__VA_ARGS__)


#endif /* __IUPCOCOA_GNUSTEP_H */
