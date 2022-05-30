/** \file
 * \brief Canvas Control
 *
 * See Copyright Notice in "iup.h"
 */

#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>
#include <limits.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_dialog.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_drvfont.h"
#include "iup_canvas.h"
#include "iup_key.h"
#include "iup_class.h" // needed for iup_classbase.h
#include "iup_classbase.h" // iupROUND
#include "iup_focus.h"

#include "iupcocoa_draw.h" // struct _IdrawCanvas
#import "iupcocoa_canvas.h"
#include "iupcocoa_drv.h"
#import "iupcocoa_dragdrop.h"


@implementation IupCocoaCanvasView
@synthesize ih = _ih;
@synthesize dc = _dc;
@synthesize currentKeyWindow = _isCurrentKeyWindow;
@synthesize currentFirstResponder = _isCurrentFirstResponder;
@synthesize startedDrag = _startedDrag;

- (instancetype) initWithFrame:(NSRect)frame_rect ih:(Ihandle*)ih
{
	self = [super initWithFrame:frame_rect];
	if(self)
	{
		_ih = ih;
		_dc = NULL;

//		iupAttribSetDouble(ih, "_IUPAPPLE_CGWIDTH", frame_rect.size.width);
//		iupAttribSetDouble(ih, "_IUPAPPLE_CGHEIGHT", frame_rect.size.height);
		// Enabling layer backed views works around drawing corruption caused by native focus rings, but has all the consequences of using layer-backed views.
		// Apple Bug ID: 44545497
//		[self setWantsLayer:YES];
#if 1
		[self setPostsBoundsChangedNotifications:YES];

		// Surprisingly, NSView doesn't have a built in method for resize events, but instead we muse use NSNotificationCenter
		NSNotificationCenter* notification_center = [NSNotificationCenter defaultCenter];
		[notification_center addObserver:self
			selector:@selector(frameDidChangeNotification:)
			name:NSViewFrameDidChangeNotification
			object:self
		];
		[notification_center addObserver:self
			selector:@selector(windowDidBecomeKeyNotification:)
			name:NSWindowDidBecomeKeyNotification
			object:[self window]
		];
		[notification_center addObserver:self
			selector:@selector(windowDidResignKeyNotification:)
			name:NSWindowDidResignKeyNotification
			object:[self window]
		];

		[self setEnabled:YES];
#endif
		
		[self setBackgroundColor:[NSColor whiteColor]];

	}
	return self;
}

- (void) dealloc
{
	NSNotificationCenter* notification_center = [NSNotificationCenter defaultCenter];
	[notification_center removeObserver:self];
	[self setBackgroundColor:nil];
	[super dealloc];
}

- (NSGraphicsContext*) graphicsContext
{
	return [NSGraphicsContext currentContext];
}

- (CGContextRef) CGContext
{
	return [[NSGraphicsContext currentContext] CGContext];
}

- (void) drawRect:(NSRect)the_rect
{
	Ihandle* ih = _ih;

	[self lockFocus];

	// Obtain the Quartz context from the current NSGraphicsContext at the time the view's
	// drawRect method is called. This context is only appropriate for drawing in this invocation
	// of the drawRect method.
	// Interesting: graphicsPort is deprecated in 10.10
	// CGContextRef cg_context = (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];
	// Use [[NSGraphicsContext currentContext] CGContext] in 10.10+
	CGContextRef cg_context = [[NSGraphicsContext currentContext] CGContext];
	
	
	CGContextSaveGState(cg_context);

	// IUP has inverted y-coordinates compared to Cocoa (which uses Cartesian like OpenGL and your math books)
	// If we are clever, we can invert the transform matrix so that the rest of the drawing code doesn't need to care.
	{
		// just inverting the y-scale causes the rendered area to flip off-screen. So we need to shift the canvas so it is centered before inverting it.
		CGFloat translate_y = the_rect.size.height * 0.5;
		CGContextTranslateCTM(cg_context, 0.0, +translate_y);
		CGContextScaleCTM(cg_context, 1.0,  -1.0);
		CGContextTranslateCTM(cg_context, 0.0, -translate_y);
	}
	
	[[self backgroundColor] set];
    NSRectFill(the_rect);
	
	IFnff call_back = (IFnff)IupGetCallback(ih, "ACTION");
	if(call_back)
	{
		call_back(ih, ih->data->posx, ih->data->posy);
	}
	CGContextRestoreGState(cg_context);

	[self unlockFocus];
}



// Note: This also triggers when the view is moved, not just resize.
- (void) frameDidChangeNotification:(NSNotification*)the_notification
{
// This Notification does not provide a userInfo dictionary according to the docs
	NSRect view_frame = [self frame];
	Ihandle* ih = _ih;

//	struct _IdrawCanvas* dc = [self dc];
	CGSize previous_size = [self previousSize];
	
	CGFloat old_width = previous_size.width;
	CGFloat old_height = previous_size.height;

	if((old_width == view_frame.size.width) && (old_height == view_frame.size.height))
	{
		// Means we were moved, but not resized.
		return;
	}

	[self setPreviousSize:view_frame.size];
	
	IFnii call_back = (IFnii)IupGetCallback(ih, "RESIZE_CB");
	if(call_back)
	{
		call_back(ih, view_frame.size.width, view_frame.size.height);
	}
	

}


- (void) windowDidBecomeKeyNotification:(NSNotification*)the_notification
{
//		NSLog(@"Window became key: %@", [[self window] title]);
//	NSLog(@"window became key");
	[self setCurrentKeyWindow:true];
	[self updateFocus];

}
- (void) windowDidResignKeyNotification:(NSNotification*)the_notification
{
//		NSLog(@"Window resign key: %@", [[self window] title]);
//	NSLog(@"window resigned");
	[self setCurrentKeyWindow:false];
	[self updateFocus];
}


//////// Keyboard stuff

- (BOOL) acceptsFirstResponder
{
//	BOOL ret_flag = [super acceptsFirstResponder];
#if 1
	if([self isEnabled])
	{
//NSLog(@"acceptsFirstResponder:YES");
		return YES;
	}
	else
	{
//NSLog(@"acceptsFirstResponder:NO");
		return NO;
	}
#else
	return YES;
#endif
}

/*
Apple doc:
The default value of this property is NO.
Subclasses can override this property and use their implementation to determine if the view requires its panel
to become the key window so that it can handle keyboard input and navigation.
Such a subclass should also override acceptsFirstResponder to return YES.
This property is also used in keyboard navigation.
It determines if a mouse click should give focus to a view—that is, make it the first responder).
Some views (for example, text fields) want to receive the keyboard focus when you click in them.
Other views (for example, buttons) receive focus only when you tab to them.
You wouldn't want focus to shift from a textfield that has editing in progress simply because you clicked on a check box.

Sooo... since IUP uses this mostly for buttons and not text entry, it seems like we should return NO.
But this means that the widgets will never get the focus ring.
*/
- (BOOL) needsPanelToBecomeKey
{
	// Should we also test [[NSApplication sharedApplication] isFullKeyboardAccessEnabled]?
//	BOOL ret_flag = [super needsPanelToBecomeKey];
//	return YES;

	// TODO: We should create a new ATTRIBUTE to distinguish different behavior modes, e.g.
	// FOCUSMODE=
	// BUTTON - returns no here so if the user is typing in another field and clicks this "button", the focus won't change
	// TEXTFIELD - text entry things are handled, but things like TAB to switch focus are passed up the responder chain
	// ALL - all entry is handled by the user

	Ihandle* ih = _ih;

	// FOR NOW: Hardcode/hack until I sort this out
/*
	if(IupClassMatch(ih, "flatbutton")
		|| IupClassMatch(ih, "flatseparator")
		|| IupClassMatch(ih, "dropbutton")
		|| IupClassMatch(ih, "flattoggle")
		|| IupClassMatch(ih, "flatlabel")
		|| IupClassMatch(ih, "colorbar")
		|| IupClassMatch(ih, "colorbrowser")
		|| IupClassMatch(ih, "dial")
		|| IupClassMatch(ih, "flatseparator")
		|| IupClassMatch(ih, "flatscrollbox")
		|| IupClassMatch(ih, "gauge")
		|| IupClassMatch(ih, "flatseparator")
		|| IupClassMatch(ih, "flatframe")
		|| IupClassMatch(ih, "flattabs")
	)
	{
		return NO;
	}
	else
	{
		return YES;
	}
*/
	if(IupClassMatch(ih, "canvas"))
	{
		return YES;
	}
	else
	{
		return NO;
	}

}

- (BOOL) canBecomeKeyView
{
//	BOOL ret_flag = [super canBecomeKeyView];
#if 1
	// Should we also test [[NSApplication sharedApplication] isFullKeyboardAccessEnabled]?
	//
	if([self isEnabled] && [[NSApplication sharedApplication] isFullKeyboardAccessEnabled])
	{
//NSLog(@"canBecomeKeyView:YES");
		return YES;
	}
	else
	{
//NSLog(@"canBecomeKeyView:NO");
		return NO;
	}
#else
	return YES;
#endif
}

- (BOOL) becomeFirstResponder
{
//NSLog(@"becomeFirstResponder");
#if 0

	BOOL ret_val = [super becomeFirstResponder];
	[self setCurrentFirstResponder:ret_val];
	[self updateFocus];
	return ret_val;
#else
	if([self isEnabled] && [[NSApplication sharedApplication] isFullKeyboardAccessEnabled])
	{
//NSLog(@"canBecomeKeyView:YES");
		[self setCurrentFirstResponder:true];
		[self updateFocus];
		return YES;
	}
	else
	{
		[self setCurrentFirstResponder:false];
		[self updateFocus];
		return NO;
	}
#endif
}

- (BOOL) resignFirstResponder
{
//NSLog(@"resignFirstResponder");
#if 1
	BOOL ret_val = [super resignFirstResponder];
	[self setCurrentFirstResponder:!ret_val];
	[self updateFocus];
	return ret_val;

#else
	[self setCurrentFirstResponder:false];

	[self updateFocus];
	return YES;
#endif



}

// 10.7 API for native focus ring
- (void) drawFocusRingMask
{
	bool should_use_native = [self useNativeFocusRing];
	if(!should_use_native)
	{
//		NSRectFill([self bounds]);
		NSRectFill(NSZeroRect);
		return;
	}
//	[self lockFocus];
//	[NSGraphicsContext currentContext];
	
    NSRectFill([self bounds]);
//	[self unlockFocus];
//	[[self window] setViewsNeedDisplay:YES];
}

// 10.7 API for native focus ring
- (NSRect) focusRingMaskBounds
{
	bool should_use_native = [self useNativeFocusRing];
	if(!should_use_native)
	{
	    //return [self bounds];
	    return NSZeroRect;
	}

    return [self bounds];
}

// helper API to notify IUP of focus state change
- (void) updateFocus
{
	Ihandle* ih = _ih;
	// BUG: I used to set my own variable in the key window notification callback.
	// But Apple was giving me multiple becomeKey callbacks or multiple resignKey callbacks, and I wasn't getting the counterparts.
	// So it appeared that I had multiple key windows and IUP focus rings were drawn in multiple windows.
	// I also tried the Main window callback, but got the same thing.
	// So instead, I query the keyWindow directly and that seems to solve the problem.
	
	
//	if([self isCurrentKeyWindow] && [self isCurrentFirstResponder])
//	if([self isCurrentKeyWindow] && [self isCurrentFirstResponder] && [self isCurrentMainWindowStatus])
	if([self isCurrentFirstResponder] && [[self window] isKeyWindow])
	{
//		NSLog(@"GetFocus ih:0x%p for View: %@ in Window: %@", ih, self, [self window]);
//		NSLog(@"GrabFocus ih:0x%p for View: %@ in Window: %@", ih, self, [[self window] title]);

		iupCallGetFocusCb(ih);
	}
	else
	{
//		NSLog(@"KillFocus ih:0x%p for View: %@ in Window: %@", ih, self, [[self window] title]);
		iupCallKillFocusCb(ih);

	}
	// Because IUP draws fake widgets, they may need to redraw to change focus rings or active-state theming
	[self setNeedsDisplay:YES]; // Cocoa seems to redraw without this. But it probably doesn't hurt.
}

- (BOOL) acceptsFirstMouse:(NSEvent *)theEvent
{
	return YES;
}
#if 1
- (void) flagsChanged:(NSEvent*)the_event
{
	// Don't respond if the control is inactive
	if(![self isEnabled])
	{
		return;
	}

//	NSLog(@"flagsChanged: %@", the_event);
//	NSLog(@"modifierFlags: 0x%X", [the_event modifierFlags]);
/*
    NSEventModifierFlagCapsLock           = 1 << 16, // Set if Caps Lock key is pressed.
    NSEventModifierFlagShift              = 1 << 17, // Set if Shift key is pressed.
    NSEventModifierFlagControl            = 1 << 18, // Set if Control key is pressed.
    NSEventModifierFlagOption             = 1 << 19, // Set if Option or Alternate key is pressed.
    NSEventModifierFlagCommand            = 1 << 20, // Set if Command key is pressed.
    NSEventModifierFlagNumericPad         = 1 << 21, // Set if any key in the numeric keypad is pressed.
    NSEventModifierFlagHelp               = 1 << 22, // Set if the Help key is pressed.
    NSEventModifierFlagFunction           = 1 << 23, // Set if any function key is pressed.
*/
	Ihandle* ih = [self ih];
    unsigned short mac_key_code = [the_event keyCode];
//    NSLog(@"mac_key_code : %d", mac_key_code);
	bool should_not_propagate = iupCocoaModifierEvent(ih, the_event, (int)mac_key_code);
	if(!should_not_propagate)
	{
		[super flagsChanged:the_event];
	}
}


//  Should we call this,
// [self setNeedsDisplay:YES];
// or force the user to call something if they need it?
- (void) keyDown:(NSEvent*)the_event
{
 	// Don't respond if the control is inactive
	if(![self isEnabled])
	{
		return;
	}
	   // gets ihandle
    Ihandle* ih = [self ih];
//	NSLog(@"keyDown: %@", the_event);
    unsigned short mac_key_code = [the_event keyCode];
//    NSLog(@"keydown string: %d", mac_key_code);

	bool should_not_propagate = iupCocoaKeyEvent(ih, the_event, (int)mac_key_code, true);
	if(!should_not_propagate)
	{
		[super keyDown:the_event];
	}
}

- (void) keyUp:(NSEvent*)the_event
{
	// Don't respond if the control is inactive
	if(![self isEnabled])
	{
		return;
	}
	
	Ihandle* ih = [self ih];
    unsigned short mac_key_code = [the_event keyCode];
	bool should_not_propagate = iupCocoaKeyEvent(ih, the_event, (int)mac_key_code, false);
	if(!should_not_propagate)
	{
		[super keyUp:the_event];
	}
}
#endif

//////// Mouse stuff

- (void) mouseDown:(NSEvent*)the_event
{
	// Don't respond if the control is inactive
	if(![self isEnabled])
	{
		return;
	}
	
	Ihandle* ih = _ih;
	bool should_not_propagate = iupCocoaCommonBaseHandleMouseButtonCallback(ih, the_event, self, true);
	if(!should_not_propagate)
	{
		[super mouseDown:the_event];
	}
}

// WARNING: This may be unsupportable.
// The currentEvent may not be the right kind ad will throw an exception when our code tries to call invalid methods for the wrong type.
// The one reason I think this may work is If-and-only-if the user calls this in the mouseDragged: callback (mouseDown: might also work),
// then the currentEvent should (I hope) be the event passed to mouseDragged:.
// If that is true, this should work.
// But calling anywhere else will probably not work.
static int cocoaCanvasSetBeginDragAttrib(Ihandle* ih, const char* value)
{
	IupSourceDragAssociatedData* drag_source_data = cocoaSourceDragGetAssociatedData(ih);

	if([drag_source_data isDragSourceEnabled])
	{
		NSDraggingItem* dragging_item = [drag_source_data defaultDraggingItem];

		NSView* main_view = [drag_source_data mainView];
		
		
		// Special case for Canvas. We want the default file promise action to write a png file of the snapshot.
		if([drag_source_data usesFilePromise] && ![drag_source_data hasFilePromiseCallback])
		{
			NSFilePromiseProvider* file_promise = (NSFilePromiseProvider*)[dragging_item item];
			// If the auto-generate drag setting was enabled, we already created an NSImage. So try reusing that.
			// This may also capture the manual drag image if the user set it.
			NSArray* images_array = [dragging_item imageComponents];
			if(images_array && ([images_array count] > 0))
			{
				NSDraggingImageComponent* image_component = [images_array objectAtIndex:0];
				id image_data = [image_component contents];
				[file_promise setUserInfo:image_data];
			}
			else
			{
				NSRect bounds_rect = [main_view bounds];
				NSData* pdf_data = [main_view dataWithPDFInsideRect:bounds_rect];
				NSImage* image_data = [[NSImage alloc] initWithData:pdf_data];
				[image_data autorelease];
				[file_promise setUserInfo:image_data];
			}
		} // end special case
		
		
		NSEvent* the_event = [[NSApplication sharedApplication] currentEvent];
		[main_view beginDraggingSessionWithItems:@[dragging_item] event:the_event source:drag_source_data];
	}

	return 0;
}

- (void) mouseDragged:(NSEvent*)the_event
{
	// Don't respond if the control is inactive
	if(![self isEnabled])
	{
		return;
	}
	
	Ihandle* ih = _ih;
	bool should_not_propagate = iupCocoaCommonBaseHandleMouseMotionCallback(ih, the_event, self);
	if(!should_not_propagate)
	{
		[super mouseDragged:the_event];
	}
	
	// Should this be before or after the user callback? Or should this only fire if super is allowed?
	// (The super reasoning is that other more complex widgets will invoke the drag in their super implementation.)
	// And if after, should we consider their return value?
	// Maybe the better thing to do is let the user directly invoke the drag?
	if(([the_event associatedEventsMask] & NSLeftMouseDragged) && ![self startedDrag])
	{
	
		IupSourceDragAssociatedData* drag_source_data = cocoaSourceDragGetAssociatedData(ih);
		
		if([drag_source_data isDragSourceEnabled] && [drag_source_data useAutoBeginDrag])
		{
#if 0
			NSDraggingItem* dragging_item = [drag_source_data defaultDraggingItem];
			if(nil != dragging_item)
			{
				// Special case for Canvas. We want the default file promise action to write a png file of the snapshot.
				if([drag_source_data usesFilePromise] && ![drag_source_data hasFilePromiseCallback])
				{
					NSFilePromiseProvider* file_promise = (NSFilePromiseProvider*)[dragging_item item];
					// If the auto-generate drag setting was enabled, we already created an NSImage. So try reusing that.
					// This may also capture the manual drag image if the user set it.
					NSArray* images_array = [dragging_item imageComponents];
					if(images_array && ([images_array count] > 0))
					{
						NSDraggingImageComponent* image_component = [images_array objectAtIndex:0];
						id image_data = [image_component contents];
						[file_promise setUserInfo:image_data];
					}
					else
					{
						NSRect bounds_rect = [self bounds];
						NSData* pdf_data = [self dataWithPDFInsideRect:bounds_rect];
						NSImage* image_data = [[NSImage alloc] initWithData:pdf_data];
						[image_data autorelease];
						[file_promise setUserInfo:image_data];
					}
				} // end special case


				[self beginDraggingSessionWithItems:@[dragging_item] event:the_event source:drag_source_data];
				[self setStartedDrag:true];
			}
#else
			cocoaCanvasSetBeginDragAttrib(ih, NULL);
#endif
		}

	}
	
}

- (void) mouseUp:(NSEvent*)the_event
{
	[self setStartedDrag:false];
	// Don't respond if the control is inactive
	if(![self isEnabled])
	{
		return;
	}
	
	Ihandle* ih = _ih;
	bool should_not_propagate = iupCocoaCommonBaseHandleMouseButtonCallback(ih, the_event, self, false);
	if(!should_not_propagate)
	{
		[super mouseUp:the_event];
	}
}

// I learned that if I don't call super, the context menu doesn't activate.
- (void) rightMouseDown:(NSEvent*)the_event
{
	// Don't respond if the control is inactive
	if(![self isEnabled])
	{
		return;
	}
	
	Ihandle* ih = _ih;
	bool should_not_propagate = iupCocoaCommonBaseHandleMouseButtonCallback(ih, the_event, self, true);
	if(!should_not_propagate)
	{
		[super rightMouseDown:the_event];
	}
}

- (void) rightMouseDragged:(NSEvent*)the_event
{
	// Don't respond if the control is inactive
	if(![self isEnabled])
	{
		return;
	}
	
	Ihandle* ih = _ih;
	bool should_not_propagate = iupCocoaCommonBaseHandleMouseMotionCallback(ih, the_event, self);
	if(!should_not_propagate)
	{
		[super rightMouseDragged:the_event];
	}
}

- (void) rightMouseUp:(NSEvent*)the_event
{
	// Don't respond if the control is inactive
	if(![self isEnabled])
	{
		return;
	}
	
	Ihandle* ih = _ih;
	bool should_not_propagate = iupCocoaCommonBaseHandleMouseButtonCallback(ih, the_event, self, false);
	if(!should_not_propagate)
	{
		[super rightMouseUp:the_event];
	}
}

- (void) otherMouseDown:(NSEvent*)the_event
{
	// Don't respond if the control is inactive
	if(![self isEnabled])
	{
		return;
	}
	
	Ihandle* ih = _ih;
	bool should_not_propagate = iupCocoaCommonBaseHandleMouseButtonCallback(ih, the_event, self, true);
	if(!should_not_propagate)
	{
		[super otherMouseDown:the_event];
	}
}

- (void) otherMouseDragged:(NSEvent*)the_event
{
	// Don't respond if the control is inactive
	if(![self isEnabled])
	{
		return;
	}
	
	Ihandle* ih = _ih;
	bool should_not_propagate = iupCocoaCommonBaseHandleMouseMotionCallback(ih, the_event, self);
	if(!should_not_propagate)
	{
		[super otherMouseDragged:the_event];
	}
}

- (void) otherMouseUp:(NSEvent*)the_event
{
	// Don't respond if the control is inactive
	if(![self isEnabled])
	{
		return;
	}
	
	Ihandle* ih = _ih;
	bool should_not_propagate = iupCocoaCommonBaseHandleMouseButtonCallback(ih, the_event, self, false);
	if(!should_not_propagate)
	{
		[super otherMouseUp:the_event];
	}
}

// WARNING: IUP WHEEL_CB does not support delta y-axis
- (void) scrollWheel:(NSEvent*)the_event
{
	// Don't respond if the control is inactive
	if(![self isEnabled])
	{
		return;
	}
	
	Ihandle* ih = _ih;
	bool should_not_propagate = iupCocoaCommonBaseScrollWheelCallback(ih, the_event, self);
	if(!should_not_propagate)
	{
		[super scrollWheel:the_event];
	}
}



/******* Begin Drag & Drop ************/

/*
- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender;
- (NSDragOperation)draggingUpdated:(id <NSDraggingInfo>)sender; // if the destination responded to draggingEntered: but not to draggingUpdated: the return value from draggingEntered: is used
- (void)draggingExited:(nullable id <NSDraggingInfo>)sender;
- (BOOL)prepareForDragOperation:(id <NSDraggingInfo>)sender;
- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender;
- (void)concludeDragOperation:(nullable id <NSDraggingInfo>)sender;
// draggingEnded: is implemented as of Mac OS 10.5
- (void)draggingEnded:(id<NSDraggingInfo>)sender;
*/


- (NSDragOperation) draggingEntered:(id<NSDraggingInfo>)the_sender
{
//	NSLog(@"%@, %@", NSStringFromSelector(_cmd), the_sender);
	
	Ihandle* ih = [self ih];
	IupTargetDropAssociatedData* target_drop_data = cocoaTargetDropGetAssociatedData(ih);
	NSArray* supported_types = [target_drop_data dropRegisteredTypes];

	NSPasteboard* paste_board = [the_sender draggingPasteboard];
//	NSString* available_type = [paste_board availableTypeFromArray:@[NSPasteboardTypeTIFF, NSPasteboardTypePNG, NSFilenamesPboardType]];
	NSString* available_type = [paste_board availableTypeFromArray:supported_types];
	if(available_type)
	{
	
		IFniis call_back = (IFniis)IupGetCallback(ih, "DROPMOTION_CB");
		if(NULL != call_back)
		{
			NSPoint window_point = [the_sender draggingLocation];
			NSPoint view_point = [self convertPoint:window_point fromView:nil];
			NSRect view_frame = [self frame];
			CGFloat inverted_y = view_frame.size.height - view_point.y;
			view_point.y = inverted_y;
	
			char mod_status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
			// We can't support this. The currentEvent may not be the right kind and will throw an exception when our code tries to call invalid methods for the wrong type.
//			NSEvent* the_event = [[NSApplication sharedApplication] currentEvent];
//			iupcocoaButtonKeySetStatus(the_event, mod_status);
			call_back(ih, view_point.x, view_point.y, mod_status);
		}
	
//		[self setNeedDisplay:YES];


		bool is_move = iupAttribGetBoolean(ih, "DRAGSOURCEMOVE");
		if(is_move)
		{
			return NSDragOperationMove;
		}
		else
		{
			return NSDragOperationCopy;
		}
	}
	return NSDragOperationNone;
}

/*
- (NSDragOperation) draggingUpdated:(id<NSDraggingInfo>)the_sender
{
//	NSLog(@"%@, %@", NSStringFromSelector(_cmd), the_sender);
	Ihandle* ih = [self ih];
	IupTargetDropAssociatedData* target_drop_data = cocoaTargetDropGetAssociatedData(ih);
	NSArray* supported_types = [target_drop_data dropRegisteredTypes];
	
	NSPasteboard* paste_board = [the_sender draggingPasteboard];
//	NSString* available_type = [paste_board availableTypeFromArray:@[NSPasteboardTypeTIFF, NSPasteboardTypePNG, NSFilenamesPboardType]];
	NSString* available_type = [paste_board availableTypeFromArray:supported_types];
	if(available_type)
	{
//		[self setNeedDisplay:YES];
		return NSDragOperationCopy;
	}
	return NSDragOperationNone;
}
*/

- (BOOL) performDragOperation:(id<NSDraggingInfo>)the_sender
{
//	NSLog(@"%@, %@", NSStringFromSelector(_cmd), the_sender);
	
	NSPasteboard* paste_board = [the_sender draggingPasteboard];
	NSPoint drop_point = [the_sender draggingLocation];
	NSPoint converted_point = [self convertPoint:drop_point fromView:nil];
	NSRect view_frame = [self frame];
	CGFloat inverted_y = view_frame.size.height - converted_point.y;
	converted_point.y = inverted_y;

	Ihandle* ih = [self ih];
	cocoaTargetDropBasePerformDropCallback(ih, the_sender, paste_board, converted_point);

	return YES;
}

/*
- (BOOL) prepareForDragOperation:(id<NSDraggingInfo>)the_sender
{
	NSLog(@"%@, %@", NSStringFromSelector(_cmd), the_sender);
	[the_sender setAnimatesToDestination:YES];
	return YES;
}
*/
/*
- (void) concludeDragOperation:(id<NSDraggingInfo>)the_sender
{
	NSLog(@"%@, %@", NSStringFromSelector(_cmd), the_sender);
}

- (void) draggingEnded:(id<NSDraggingInfo>)the_sender
{
	NSLog(@"%@, %@", NSStringFromSelector(_cmd), the_sender);
}
*/
//- (void)updateDraggingItemsForDrag:(nullable id <NSDraggingInfo>)sender NS_AVAILABLE_MAC(10_7);

/*
- (NSDragOperation)draggingSession:(NSDraggingSession *)session sourceOperationMaskForDraggingContext:(NSDraggingContext)context
{
    if (context == NSDraggingContextOutsideApplication) {
        return NSDragOperationCopy;
    }
	
    return NSDragOperationNone;
}
*/
/*
- (NSDraggingSession*) beginDraggingSessionWithItems:(NSArray<NSDraggingItem*>*)items_array event:(NSEvent*)the_event source:(id<NSDraggingSource>)dragging_source
{
	NSLog(@"beginDraggingSessionWithItems:%@, event:%@, source:%@", items_array, the_event, dragging_source);
	NSDraggingSession* dragging_session = [super beginDraggingSessionWithItems:items_array event:the_event source:dragging_source];
	NSLog(@"dragging_session:%@", dragging_session);
	return dragging_session;
}
*/
//@protocol NSDraggingSource <NSObject>
/* Methods implemented by an object that initiates a drag session. The source application is sent these messages during dragging.  The first must be implemented, the others are sent if the source responds to them.
*/

//@required
/* Declares what types of operations the source allows to be performed. Apple may provide more specific "within" values in the future. To account for this, for unrecongized localities, return the operation mask for the most specific context that you are concerned with. For example:
    switch(context) {
        case NSDraggingContextOutsideApplication:
            return ...
            break;

        case NSDraggingContextWithinApplication:
        default:
            return ...
            break;
    }
*/


/******* End Drag & Drop ************/

/*
TODO: Menu Items / Responder Chain for Undo, Redo, Cut, Copy, Paste (and any other menu items)

I would like to allow our custom (Canvas) Views to respond to the standard menu items.
This means graying/not graying out, and being able to respond to the actions.

While I think it would be cool to provide direct integration with NSUndoManager for undo/redo,
I worry the semantics won't fit well with a cross-platform program in that the user will have to
write a lot more code to keep the undo manager aprised of the application state.
And this code will not be easily reusable with the other platforms, so nobody will want to write it.
(TODO: Re-evaluate other platforms and see if the modern APIs look closer to NSUndoManager.)

So instead of direct NSUndoManager integration, let's presume the user wrote their own
cross-platform manual undo system, with typical custom functions like myundo() and myredo().
(See 7GUI #6)

So instead, let's just override this View's responder to forward the menu events to the user's cross-platform
custom undo/redo system.

This will allow the user to reuse all their own code, and only require a little bit of additional code
to hook into it for Cocoa.


We can extend this idea to cut, copy, paste as well.

*/

/*
static BOOL iupCocoaValidateMenuItemFromActionCallback(Ihandle* ih, const char* callback_name, bool default_for_continue)
{
	Icallback action_callback = (Icallback)IupGetCallback(ih, callback_name);
	if(NULL != action_callback)
	{
		return YES;
	}
	return default_for_continue;
}
*/

static bool iupCocoaHelperHasCallback(Ihandle* ih, const char* callback_name)
{
	Icallback action_callback = (Icallback)IupGetCallback(ih, callback_name);
	if(NULL != action_callback)
	{
		return true;
	}
	else
	{
		return false;
	}
}

- (BOOL) validateMenuItem:(NSMenuItem*)menu_item
{
	// Instead of this, check for user callback, e.g. VALIDATEMENU_CB.
	// User should return YES/NO (I can't seem to call super.)
	// They should implement for all titles they need to support (Undo, Redo, Cut Copy Paste)
	// Not sure how to handle localized strings
	Ihandle* ih = _ih;
	NSString* menu_item_title = [menu_item title];
	const char* c_menu_item_title = [menu_item_title UTF8String];
	
	IFns validate_menu_callback = (IFns)IupGetCallback(ih, "VALIDATEMENU_CB");
	int ret_val = IUP_CONTINUE;
	if(NULL != validate_menu_callback)
	{
		ret_val = validate_menu_callback(ih, (char*)c_menu_item_title);
	}
	if(1 == ret_val)
	{
		return YES;
	}
	else if(0 == ret_val)
	{
		return NO;
	}
	else if(IUP_CONTINUE == ret_val)
	{
		if([menu_item_title isEqualTo:NSLocalizedString(@"Undo", @"Undo")])
		{
			return iupCocoaHelperHasCallback(ih, "UNDO_CB");
		}
		else if([menu_item_title isEqualTo:NSLocalizedString(@"Redo", @"Redo")])
		{
			return iupCocoaHelperHasCallback(ih, "REDO_CB");
		}
		else if([menu_item_title isEqualTo:NSLocalizedString(@"Cut", @"Cut")])
		{
		   	bool is_move = iupAttribGetBoolean(ih, "DRAGSOURCEMOVE");
			if(is_move)
			{
				IupSourceDragAssociatedData* drag_source_data = cocoaSourceDragGetAssociatedData(ih);
				NSArray* registered_types = [drag_source_data dragRegisteredTypes];
				if((registered_types != nil) && ([registered_types count] > 0))
				{
					return YES;
				}
			}
			return iupCocoaHelperHasCallback(ih, "CUT_CB");
		}
		else if([menu_item_title isEqualTo:NSLocalizedString(@"Copy", @"Copy")])
		{
		   	bool is_move = iupAttribGetBoolean(ih, "DRAGSOURCEMOVE");
			if(!is_move)
			{
				IupSourceDragAssociatedData* drag_source_data = cocoaSourceDragGetAssociatedData(ih);
				NSArray* registered_types = [drag_source_data dragRegisteredTypes];
				if((registered_types != nil) && ([registered_types count] > 0))
				{
					return YES;
				}
			}
			return iupCocoaHelperHasCallback(ih, "COPY_CB");
		}
		else if([menu_item_title isEqualTo:NSLocalizedString(@"Paste", @"Paste")])
		{
			IupTargetDropAssociatedData* target_drop_data = cocoaTargetDropGetAssociatedData(ih);
			NSArray* registered_types = [target_drop_data dropRegisteredTypes];
			if((registered_types != nil) && ([registered_types count] > 0))
			{
				return YES;
			}
			return iupCocoaHelperHasCallback(ih, "PASTE_CB");
		}
		else if([menu_item_title isEqualTo:NSLocalizedString(@"Paste and Match Style", @"Paste and Match Style")])
		{
			/*
			IupTargetDropAssociatedData* target_drop_data = cocoaTargetDropGetAssociatedData(ih);
			NSArray* registered_types = [target_drop_data dropRegisteredTypes];
			if((registered_types != nil) && ([registered_types count] > 0))
			{
				return YES;
			}
			*/
			return iupCocoaHelperHasCallback(ih, "PASTESTYLE_CB");
		}
		else
		{
			return NO;
		}
	}
	else
	{
		return NO;
	}
	

	return NO;
}

static int iupCocoaRunMenuItemActionFromCallback(Ihandle* ih, const char* callback_name)
{
	Icallback action_callback = (Icallback)IupGetCallback(ih, callback_name);
	int ret_val = IUP_CONTINUE;
	if(NULL != action_callback)
	{
		ret_val = action_callback(ih);
	}
	return ret_val;
}

- (void) undo:(id)the_sender
{
//	NSLog(@"Undo");
 	// Provide a new callback, e.g. MENU_CB or MENUACTION_CB or MENUITEM_CB
 	// We can provide pre-canned strings like "UNDO", "REDO", "CUT", "COPY", "PASTE"
 	// telling them which action they need to handle.
	Ihandle* ih = _ih;
	int ret_val = iupCocoaRunMenuItemActionFromCallback(ih, "UNDO_CB");
	if(IUP_CONTINUE == ret_val)
	{
	}
}
- (void) redo:(id)the_sender
{
	Ihandle* ih = _ih;
	int ret_val = iupCocoaRunMenuItemActionFromCallback(ih, "REDO_CB");
	if(IUP_CONTINUE == ret_val)
	{
	}
}
- (void) cut:(id)the_sender
{
	Ihandle* ih = _ih;
	int ret_val = iupCocoaRunMenuItemActionFromCallback(ih, "CUT_CB");
	if(IUP_CONTINUE == ret_val)
	{
	}
}
- (void) copy:(id)the_sender
{
	Ihandle* ih = _ih;
	int ret_val = iupCocoaRunMenuItemActionFromCallback(ih, "COPY_CB");
	if(IUP_CONTINUE == ret_val)
	{
		IupSourceDragAssociatedData* drag_source_data = cocoaSourceDragGetAssociatedData(ih);
		
		NSPasteboardItem* pasteboard_item = [drag_source_data defaultPasteboardItem];
		NSPasteboard* paste_board = [NSPasteboard generalPasteboard];

		[paste_board clearContents];
		[paste_board writeObjects:@[pasteboard_item]];
	}
}
- (void) paste:(id)the_sender
{
	Ihandle* ih = _ih;
	int ret_val = iupCocoaRunMenuItemActionFromCallback(ih, "PASTE_CB");
	if(IUP_CONTINUE == ret_val)
	{
		NSPasteboard* paste_board = [NSPasteboard generalPasteboard];
		// TODO: Provide user a way to change the drop point value for paste (so they can differentiate from a drop if needed)
		NSPoint drop_point = {0, 0};
		cocoaTargetDropBasePerformDropCallback(ih, the_sender, paste_board, drop_point);
	}
}

- (void) pasteAsPlainText:(id)the_sender
{
	Ihandle* ih = _ih;
	int ret_val = iupCocoaRunMenuItemActionFromCallback(ih, "PASTESTYLE_CB");
	if(IUP_CONTINUE == ret_val)
	{

	}
}



@end


static NSView* cocoaCanvasGetRootView(Ihandle* ih)
{
	NSView* root_container_view = (NSView*)ih->handle;
	NSCAssert([root_container_view isKindOfClass:[NSView class]], @"Expected NSView");
	return root_container_view;
}

static NSScrollView* cocoaCanvasGetScrollView(Ihandle* ih)
{
	if(iupAttribGetBoolean(ih, "_IUPCOCOA_CANVAS_HAS_SCROLLBAR"))
	{
		NSScrollView* scroll_view = (NSScrollView*)ih->handle;
		NSCAssert([scroll_view isKindOfClass:[NSScrollView class]], @"Expected NSScrollView");
		return scroll_view;
	}
	else
	{
		return nil;
	}
}

static IupCocoaCanvasView* cocoaCanvasGetCanvasView(Ihandle* ih)
{
	if(iupAttribGetBoolean(ih, "_IUPCOCOA_CANVAS_HAS_SCROLLBAR"))
	{
		NSScrollView* scroll_view = cocoaCanvasGetScrollView(ih);
		IupCocoaCanvasView* canvas_view = (IupCocoaCanvasView*)[scroll_view documentView];
		NSCAssert([canvas_view isKindOfClass:[IupCocoaCanvasView class]], @"Expected IupCocoaCanvasView");
		return canvas_view;
	}
	else
	{
		IupCocoaCanvasView* canvas_view = (IupCocoaCanvasView*)ih->handle;
		return canvas_view;
	}
}

static char* cocoaCanvasGetCGContextAttrib(Ihandle* ih)
{
	IupCocoaCanvasView* canvas_view = cocoaCanvasGetCanvasView(ih);
	CGContextRef cg_context = NULL;
//	[canvas_view lockFocus];
	// Interesting: graphicsPort is deprecated in 10.10
	// cg_context = (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];
	// Use [[NSGraphicsContext currentContext] CGContext] in 10.10+
	cg_context = [[NSGraphicsContext currentContext] CGContext];
//	[canvas_view unlockFocus];
	
	return (char*)cg_context;
}

static char* cocoaCanvasGetDrawableAttrib(Ihandle* ih)
{
	return (char*)cocoaCanvasGetCGContextAttrib(ih);
}

static char* cocoaCanvasGetDrawSizeAttrib(Ihandle *ih)
{
	int w, h;

	// scrollview or canvas view?
	IupCocoaCanvasView* canvas_view = cocoaCanvasGetCanvasView(ih);

	NSRect the_frame = [canvas_view frame];
	w = iupROUND(the_frame.size.width);
	h = iupROUND(the_frame.size.height);
	
	return iupStrReturnIntInt(w, h, 'x');
}

static char* cocoaCanvasGetNativeFocusRingAttrib(Ihandle* ih)
{
	IupCocoaCanvasView* canvas_view = cocoaCanvasGetCanvasView(ih);
	bool should_use_native = [canvas_view useNativeFocusRing];
/*
	struct _IdrawCanvas* dc = [canvas_view dc];
	if(dc)
	{
		should_use_native = dc->useNativeFocusRing;
	}
*/
	return iupStrReturnBoolean(should_use_native);
}

static int cocoaCanvasSetNativeFocusRingAttrib(Ihandle* ih, const char* value)
{
	IupCocoaCanvasView* canvas_view = cocoaCanvasGetCanvasView(ih);
	bool should_use_native = (bool)iupStrBoolean(value);

	[canvas_view setUseNativeFocusRing:should_use_native];
/*
	struct _IdrawCanvas* dc = [canvas_view dc];
	if(dc)
	{
		dc->useNativeFocusRing = should_use_native;
	}
*/
	return 1;
}

static int cocoaCanvasSetContextMenuAttrib(Ihandle* ih, const char* value)
{
	Ihandle* menu_ih = (Ihandle*)value;
 	IupCocoaCanvasView* canvas_view = cocoaCanvasGetCanvasView(ih);
	iupCocoaCommonBaseSetContextMenuForWidget(ih, canvas_view, menu_ih);

	return 1;
}



static int cocoaCanvasMapMethod(Ihandle* ih)
{
	NSView* root_view = nil;
	IupCocoaCanvasView* canvas_view = [[IupCocoaCanvasView alloc] initWithFrame:NSZeroRect ih:ih];
	
	if(iupAttribGetBoolean(ih, "SCROLLBAR"))
	{
		NSScrollView* scroll_view = [[NSScrollView alloc] initWithFrame:NSZeroRect];
		[scroll_view setDocumentView:canvas_view];
		[canvas_view release];
		[scroll_view setHasVerticalScroller:YES];
	
		[scroll_view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
		[canvas_view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
		
		root_view = scroll_view;
		iupAttribSet(ih, "_IUPCOCOA_CANVAS_HAS_SCROLLBAR", "1");
	}
	else
	{
		root_view = canvas_view;
	}
	
	
	ih->handle = root_view;

	
	iupCocoaSetAssociatedViews(ih, canvas_view, root_view);

	// All Cocoa views shoud call this to add the new view to the parent view.
	iupCocoaAddToParent(ih);
	
	
	IupSourceDragAssociatedData* source_drag_associated_data = cocoaSourceDragCreateAssociatedData(ih, canvas_view, root_view);
	IupTargetDropAssociatedData* target_drop_associated_data = cocoaTargetDropCreateAssociatedData(ih,  canvas_view, root_view);

	[source_drag_associated_data setDefaultFilePromiseName:@"IupCanvas.png"];
//	[source_drag_associated_data setUseAutoBeginDrag:true];
	
	
	// Ideally, we want IUP to automatically do the thing that gives users the best experience because devs may not know enough to turn on the right features.
	// Native Focus Rings are one of those things that seem to provide the best user experience.
	// But due to an Apple bug, the native focus ring causes draw corruption unless layerbacked is enabled.
	// Historically, enabling layer backed views has been known to cause various problems.
	// And even today, layer backed views are still off by default.
	// So I'm hesitent to turn them on by default.
	// Additionally, the Canvas drawing is classic CoreGrpahics, CPU based, so this is not necessarily the greatest performance path to take.
	// But layer backed seems to be getting better over time, and this is a case where we need it on in order to work correctly.
	// So for the fake widget case, I think we should turn on native focus rings.
	// For regular, I'm less certain.
	// But maybe we should only activate if needsPanelToBecomeKey (FOCUSMODE) is true, because that is the only time we care about the focus ring.
/*
	if(IupClassMatch(ih, "canvas"))
	{
		IupSetAttribute(ih, "LAYERBACKED", "YES");
		IupSetAttribute(ih, "NATIVEFOCUSRING", "YES");
	}
	else
	{
		IupSetAttribute(ih, "LAYERBACKED", "YES");
		IupSetAttribute(ih, "NATIVEFOCUSRING", "YES");
	}
*/

	
	return IUP_NOERROR;
}

static void cocoaCanvasUnMapMethod(Ihandle* ih)
{
	id root_view = ih->handle;
	
	
	cocoaTargetDropDestroyAssociatedData(ih);
	cocoaSourceDragDestroyAssociatedData(ih);
	
	// Destroy the context menu ih it exists
	{
		Ihandle* context_menu_ih = (Ihandle*)iupCocoaCommonBaseGetContextMenuAttrib(ih);
		if(NULL != context_menu_ih)
		{
			IupDestroy(context_menu_ih);
		}
		iupCocoaCommonBaseSetContextMenuAttrib(ih, NULL);
	}
	
	iupCocoaRemoveFromParent(ih);
	iupCocoaSetAssociatedViews(ih, nil, nil);
	[root_view release];
	ih->handle = NULL;
}




void iupdrvCanvasInitClass(Iclass* ic)
{
	/* Driver Dependent Class functions */
	ic->Map = cocoaCanvasMapMethod;
	ic->UnMap = cocoaCanvasUnMapMethod;
#if 0
	ic->LayoutUpdate = gtkCanvasLayoutUpdateMethod;
	
	/* Driver Dependent Attribute functions */
#endif
	/* Visual */
	iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, "255 255 255", NULL, IUPAF_DEFAULT);  /* force new default value */

#if 0
	/* IupCanvas only */
#endif
	iupClassRegisterAttribute(ic, "DRAWSIZE", cocoaCanvasGetDrawSizeAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
	
#if 0
	iupClassRegisterAttribute(ic, "DX", NULL, gtkCanvasSetDXAttrib, NULL, NULL, IUPAF_NO_INHERIT);  /* force new default value */
	iupClassRegisterAttribute(ic, "DY", NULL, gtkCanvasSetDYAttrib, NULL, NULL, IUPAF_NO_INHERIT);  /* force new default value */
	iupClassRegisterAttribute(ic, "POSX", iupCanvasGetPosXAttrib, gtkCanvasSetPosXAttrib, "0", NULL, IUPAF_NO_INHERIT);  /* force new default value */
	iupClassRegisterAttribute(ic, "POSY", iupCanvasGetPosYAttrib, gtkCanvasSetPosYAttrib, "0", NULL, IUPAF_NO_INHERIT);  /* force new default value */
#endif
	
	// TODO: Returns the CGContext. Is this the right thing, or should it be the NSGraphicsContext? Or should it be the NSView?
	iupClassRegisterAttribute(ic, "DRAWABLE", cocoaCanvasGetDrawableAttrib, NULL, NULL, NULL, IUPAF_NO_STRING);

	// Private helper, used by iupdrvDrawCreateCanvas and currently cocoaCanvasGetDrawableAttrib calls this.
	// Do not start with an underscore, because I need this to trigger the function
	iupClassRegisterAttribute(ic, "CGCONTEXT", cocoaCanvasGetCGContextAttrib, NULL, NULL, NULL, IUPAF_NO_STRING);

#if 0
	/* IupCanvas Windows or X only */
#ifndef GTK_MAC
#ifdef WIN32
	iupClassRegisterAttribute(ic, "HWND", iupgtkGetNativeWindowHandle, NULL, NULL, NULL, IUPAF_NO_STRING|IUPAF_NO_INHERIT);
#else
	iupClassRegisterAttribute(ic, "XWINDOW", iupgtkGetNativeWindowHandle, NULL, NULL, NULL, IUPAF_NO_INHERIT|IUPAF_NO_STRING);
	iupClassRegisterAttribute(ic, "XDISPLAY", (IattribGetFunc)iupdrvGetDisplay, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT|IUPAF_NO_STRING);
#endif
#endif
	
	/* Not Supported */
	iupClassRegisterAttribute(ic, "BACKINGSTORE", NULL, NULL, "YES", NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
#endif

//	TODO:
//	iupClassRegisterAttribute(ic, "TOUCH", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);

	/* New API for view specific contextual menus (Mac only) */
	iupClassRegisterAttribute(ic, "CONTEXTMENU", iupCocoaCommonBaseGetContextMenuAttrib, cocoaCanvasSetContextMenuAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE);

	iupClassRegisterAttribute(ic, "NATIVEFOCUSRING", cocoaCanvasGetNativeFocusRingAttrib, cocoaCanvasSetNativeFocusRingAttrib, NULL, "NO", IUPAF_DEFAULT);

	// TODO: We need a layer backed API for everything. But especially for here to workaround the native focus ring rendering corruption.
	iupClassRegisterAttribute(ic, "LAYERBACKED", iupCocoaCommonBaseGetLayerBackedAttrib, iupCocoaCommonBaseSetLayerBackedAttrib, NULL,  NULL, IUPAF_NO_DEFAULTVALUE);


	iupClassRegisterCallback(ic, "VALIDATEMENU_CB", "s");
	iupClassRegisterCallback(ic, "UNDO_CB", "");
	iupClassRegisterCallback(ic, "REDO_CB", "");
	iupClassRegisterCallback(ic, "CUT_CB", "");
	iupClassRegisterCallback(ic, "COPY_CB", "");
	iupClassRegisterCallback(ic, "PASTE_CB", "");
	iupClassRegisterCallback(ic, "PASTESTYLE_CB", "");
	
	iupClassRegisterAttribute(ic, "SENDACTION", NULL, iupCocoaCommonBaseSetSendActionAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE);

	// EXPERIMENTAL: May not work because it uses currentEvent. This is an override of cocoaSourceDragSetBeginDragAttrib
  iupClassRegisterAttribute(ic, "DRAGINITIATE", NULL, cocoaCanvasSetBeginDragAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE);

}
