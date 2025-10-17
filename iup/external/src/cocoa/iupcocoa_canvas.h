/** \file
 * \brief Canvas Control Custom View for Cocoa
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUPCOCOA_CANVASVIEW_H
#define __IUPCOCOA_CANVASVIEW_H

#import <Cocoa/Cocoa.h>

#include <stdbool.h>
#include "iup.h"

/**
 * @brief Custom NSControl subclass for IupCanvas.
 *
 * This view serves as the core drawing and event-handling surface for the IupCanvas element.
 * It is responsible for:
 * - Handling the ACTION callback for custom drawing, correctly flipping Cocoa's coordinate system to match IUP's top-left origin.
 * - Processing mouse, keyboard, and focus events and forwarding them to the appropriate IUP callbacks.
 * - Acting as a source and destination for drag-and-drop operations.
 * - Managing a native focus ring for accessibility.
 *
 * This view is designed to be used either standalone or as the `documentView` within an NSScrollView to support scrolling.
 * When used with an NSScrollView, it relies on NSViewBoundsDidChangeNotification from the parent NSClipView
 * to update IUP's scroll position attributes and trigger the SCROLL_CB callback.
 */
@interface IupCocoaCanvasView : NSControl

// The IUP handle (`Ihandle*`) associated with this view.
@property(nonatomic, assign) Ihandle* ih;

// State tracking for focus management.
@property(nonatomic, assign, getter=isCurrentKeyWindow) bool currentKeyWindow;
@property(nonatomic, assign, getter=isCurrentFirstResponder) bool currentFirstResponder;

// Flag to track drag-and-drop initiation.
@property(nonatomic, assign) bool startedDrag;

// Stores the previous size to detect resize events vs. move events.
@property(nonatomic, assign) CGSize previousSize;

// The background color to use when no ACTION callback is provided.
@property(nonatomic, copy) NSColor* backgroundColor;

// Determines whether to draw the native macOS focus ring.
@property(nonatomic, assign) bool useNativeFocusRing;


/**
 * @brief Initializes the canvas view.
 * @param frame_rect The initial frame for the view.
 * @param ih The associated IUP handle.
 * @return An initialized IupCocoaCanvasView object.
 */
- (instancetype) initWithFrame:(NSRect)frame_rect ih:(Ihandle*)ih;

/**
 * @brief Triggers the GETFOCUS_CB or KILLFOCUS_CB callback based on the view's focus state.
 */
- (void) updateFocus;

@end

#endif /* __IUPCOCOA_CANVASVIEW_H */