/** \file
 * \brief GTK Base Functions
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>              
#include <stdlib.h>
#include <string.h>             
#include <limits.h>             

#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupkey.h"

#include "iup_object.h"
#include "iup_childtree.h"
#include "iup_key.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_focus.h"
#include "iup_key.h"
#include "iup_image.h"
#include "iup_loop.h"
#include "iup_drv.h"

#include "iupcocoa_drv.h"


#if __has_feature(objc_arc)
#error "Cocoa backend for IUP does not use ARC. Compile with -fno-objc-arc"
#endif


const void* IHANDLE_ASSOCIATED_OBJ_KEY = @"IHANDLE_ASSOCIATED_OBJ_KEY"; // the point of this is we have a unique memory address for an identifier
const void* MAINVIEW_ASSOCIATED_OBJ_KEY = @"MAINVIEW_ASSOCIATED_OBJ_KEY"; // ih->handle is the root object, but often the main view is lower down, (e.g. scrollview contains the real widget).
const void* ROOTVIEW_ASSOCIATED_OBJ_KEY = @"ROOTVIEW_ASSOCIATED_OBJ_KEY"; // ih->handle is the root object, but in a few cases, this is not a View, e.g. NSWindow. Many times we want the root view.

NSObject* iupCocoaGetRootObject(Ihandle* ih)
{
	if(NULL == ih)
	{
		return nil;
	}
	return (NSObject*)ih->handle;
}

NSView* iupCocoaGetRootView(Ihandle* ih)
{
	if(NULL == ih)
	{
		return nil;
	}
	id root_object = (NSObject*)ih->handle;

	if(nil == root_object)
	{
		return nil;
	}
	
	NSView* main_view = (NSView*)objc_getAssociatedObject(root_object, ROOTVIEW_ASSOCIATED_OBJ_KEY);

	NSCAssert([main_view isKindOfClass:[NSView class]], @"Expected NSView");
	return main_view;
}

NSView* iupCocoaGetMainView(Ihandle* ih)
{
	if(NULL == ih)
	{
		return nil;
	}
	id root_object = (NSObject*)ih->handle;

	if(nil == root_object)
	{
		return nil;
	}
	
	NSView* main_view = (NSView*)objc_getAssociatedObject(root_object, MAINVIEW_ASSOCIATED_OBJ_KEY);

	NSCAssert([main_view isKindOfClass:[NSView class]], @"Expected NSView");
	return main_view;
}

void iupCocoaSetAssociatedViews(Ihandle* ih, NSView* main_view, NSView* root_view)
{
	NSCAssert(ih->handle, @"Expected ih->handle to be set");

	// Using assign because I don't want to accidentally create retain cycles.
	// I expect these values to always be available as long as the ih->handle is alive.
	objc_setAssociatedObject((id)ih->handle, MAINVIEW_ASSOCIATED_OBJ_KEY, main_view, OBJC_ASSOCIATION_ASSIGN);

	objc_setAssociatedObject((id)ih->handle, ROOTVIEW_ASSOCIATED_OBJ_KEY, root_view, OBJC_ASSOCIATION_ASSIGN);
}



void iupCocoaAddToParent(Ihandle* ih)
{
	id parent_native_handle = iupChildTreeGetNativeParentHandle(ih);
	
	id child_handle = ih->handle;
	if([child_handle isKindOfClass:[NSViewController class]])
	{
		child_handle = [child_handle view];
		// fall through to next block
	}
	
	
	if([child_handle isKindOfClass:[NSView class]])
	{
		NSView* the_view = (NSView*)child_handle;
		
		// From David Phillip Oster:
		/* 
		 now, when you resize the window, you see the hidden content. This makes the subviews preserve their relative x,y offset from the top left of the window, instead of the Mac default of the bottom left. It probably isn't the right way to do it, since there's probably some iup property that is specifying somethign more complex.
		 
		 
		 If I had set [the_view setAutoresizingMask:NSViewMinXMargin|NSViewWidthSizable|NSViewMaxXMargin];
		 
		 Then, as the window widens, that view widens along with it.
		 */
		NSAutoresizingMaskOptions current_mask = [the_view autoresizingMask];
		current_mask = current_mask & (NSViewWidthSizable | NSViewHeightSizable); // Preserve Sizable flags if set
		[the_view setAutoresizingMask:current_mask|NSViewMaxXMargin|NSViewMinYMargin];
//		[the_view setAutoresizingMask:NSViewMaxXMargin|NSViewMinYMargin];

		
		if([parent_native_handle isKindOfClass:[NSWindow class]])
		{
			NSWindow* parent_window = (NSWindow*)parent_native_handle;
			NSView* window_view = [parent_window contentView];
			[window_view addSubview:the_view];
//			[parent_window recalculateKeyViewLoop];

		}
		/*
		else if([parent_native_handle isKindOfClass:[NSBox class]])
		{
			NSBox* parent_view = (NSBox*)parent_native_handle;
			[[parent_view contentView] addSubview:the_view];

		}
		 */

		else if([parent_native_handle isKindOfClass:[NSView class]])
		{
			NSView* parent_view = (NSView*)parent_native_handle;
			
			[parent_view addSubview:the_view];
			
//			[[parent_view window] recalculateKeyViewLoop];
		}
		else if([parent_native_handle isKindOfClass:[NSViewController class]])
		{
			NSViewController* view_controller = (NSViewController*)parent_native_handle;
			NSView* parent_view = [view_controller view];
			[parent_view addSubview:the_view];

		}
		else
		{
			NSCAssert(1, @"Unexpected type for parent widget");
			@throw @"Unexpected type for parent widget";
		}
	
//		[[the_view window] recalculateKeyViewLoop];

	}
	else if([child_handle isKindOfClass:[CALayer class]])
	{
		NSCAssert(1, @"CALayer not implemented");
		@throw @"CALayer not implemented";
	}
	else
	{
		NSCAssert(1, @"Unexpected type for parent widget");
		@throw @"Unexpected type for parent widget";
	}
	
}

void iupCocoaRemoveFromParent(Ihandle* ih)
{
	
	id child_handle = ih->handle;
	if([child_handle isKindOfClass:[NSView class]])
	{
		NSView* the_view = (NSView*)child_handle;
		NSWindow* parent_window = [the_view window];
		[the_view removeFromSuperview];
//		[parent_window recalculateKeyViewLoop];
	}
	else if([child_handle isKindOfClass:[NSViewController class]])
	{
		NSViewController* view_controller = (NSViewController*)child_handle;
		NSView* the_view = [view_controller view];
//		NSWindow* parent_window = [the_view window];
		[the_view removeFromSuperview];
//		[parent_window recalculateKeyViewLoop];
	}
	else if([child_handle isKindOfClass:[CALayer class]])
	{
		CALayer* the_layer = (CALayer*)child_handle;
		[the_layer removeFromSuperlayer];
	}
	else
	{
		NSCAssert(1, @"Unexpected type for widget");
		@throw @"Unexpected type for widget";
	}
}

int iupCocoaComputeCartesianScreenHeightFromIup(int iup_height)
{
	// Do I want the full screen height or the visible height?
	NSRect screen_rect = [[NSScreen mainScreen] frame];
//	NSRect screen_rect = [[NSScreen mainScreen] visibleFrame];
	CGFloat inverted_height = screen_rect.size.height - iup_height;
	return iupROUND(inverted_height);
}

int iupCocoaComputeIupScreenHeightFromCartesian(int cartesian_height)
{
	// Do I want the full screen height or the visible height?
	NSRect screen_rect = [[NSScreen mainScreen] frame];
//	NSRect screen_rect = [[NSScreen mainScreen] visibleFrame];
	CGFloat inverted_height = screen_rect.size.height - cartesian_height;
	return iupROUND(inverted_height);
}

void iupCocoaCommonLoopCallExitCb()
{
	static BOOL has_called_exit_cb = NO;
	if(NO == has_called_exit_cb)
	{
		has_called_exit_cb = YES;
		iupLoopCallExitCb();
	}
	else
	{
		// Drat. I don't think I have a way around this.
		// There seems to be at least 2 different possible exit triggers.
		// 1. Apple stopping the native event loop (cmd-Q)
		// 2. Iup manually terminating the event loop. (close/destroy last window or use IUP_CLOSE)
		// Because of that, when I tried to put the callback in only one location, I sometimes missed doing the callback.
		// But if I cover all the bases, I get multiple callbacks sometimes depending on the sequence of events.
		NSLog(@"Warning: iupCocoaCommonLoopCallExitCb has been already been called. Skipping callback.");
	}
}


void iupdrvActivate(Ihandle* ih)
{

}

void iupdrvReparent(Ihandle* ih)
{

	
}


// If you change these, make sure to update any overrides that copy/paste this pattern (e.g. cocoaButtonLayoutUpdateMethod)
// TODO: Consider CALayer
NSView* iupCocoaCommonBaseLayoutGetParentView(Ihandle* ih)
{
	id parent_native_handle = iupChildTreeGetNativeParentHandle(ih);
	NSView* parent_view = nil;
	
	if([parent_native_handle isKindOfClass:[NSWindow class]])
	{
		NSWindow* parent_window = (NSWindow*)parent_native_handle;
		parent_view = [parent_window contentView];
	}
	/* // Part of NSBox experiment
	 else if([parent_native_handle isKindOfClass:[NSBox class]])
	 {
		NSBox* box_view = (NSBox*)parent_native_handle;
		parent_view = [box_view contentView];
		
		CGFloat diff_width = NSWidth([box_view frame]) - NSWidth([parent_view frame]);
		CGFloat diff_height = NSHeight([box_view frame]) - NSHeight([parent_view frame]);
	 
		current_width = current_width - diff_width;
		current_height = current_height - diff_height;
	 }
	 */
	else if([parent_native_handle isKindOfClass:[NSView class]])
	{
		parent_view = (NSView*)parent_native_handle;
	}
	/*
	else if([parent_native_handle isKindOfClass:[NSTabViewController class]])
	{
		parent_view = [(NSTabViewController*)parent_native_handle tabView];
	}
	*/
	else if([parent_native_handle isKindOfClass:[NSViewController class]])
	{
		parent_view = [(NSViewController*)parent_native_handle view];
	}
	else
	{
		NSCAssert(1, @"Unexpected type for parent widget");
		@throw @"Unexpected type for parent widget";
	}
	
	return parent_view;
}

// If you change these, make sure to update any overrides that copy/paste this pattern (e.g. cocoaButtonLayoutUpdateMethod)
// TODO: Consider CALayer
NSView* iupCocoaCommonBaseLayoutGetChildView(Ihandle* ih)
{
	id child_handle = ih->handle;
	NSView* the_view = nil;

	if([child_handle isKindOfClass:[NSViewController class]])
	{
		child_handle = [(NSViewController*)child_handle view];
		// fall through to next block
	}
	
	if([child_handle isKindOfClass:[NSView class]])
	{
		the_view = (NSView*)child_handle;
	}
	else if([child_handle isKindOfClass:[CALayer class]])
	{
		NSCAssert(1, @"CALayer not implemented");
		@throw @"CALayer not implemented";
	}
	else
	{
		NSCAssert(1, @"Unexpected type for child widget");
		@throw @"Unexpected type for child widget";
	}
	return the_view;
}

// If you change these, make sure to update any overrides that copy/paste this pattern (e.g. cocoaButtonLayoutUpdateMethod)
NSRect iupCocoaCommonBaseLayoutComputeChildFrameRectFromParentRect(Ihandle* ih, NSRect parent_rect)
{

	NSRect the_rect = NSMakeRect(
		ih->x,
		// Need to invert y-axis, and also need to shift/account for height of widget because that is also lower-left instead of upper-left.
		parent_rect.size.height - ih->y - ih->currentheight,
		ih->currentwidth,
		ih->currentheight
	);
//	the_rect.origin.y = the_rect.origin.y - 16.0*0.5; // I don't realy understand the logic of this offset, particularly the -1


	// for padding
	// drat, data is private and requires a per-widget header
	{

		char* padding_str = iupAttribGet(ih, "PADDING");
		if((NULL != padding_str) && (padding_str[0] != '\0'))
		{
			int horiz_padding = 0;
			int vert_padding = 0;
			iupStrToIntInt(padding_str, &horiz_padding, &vert_padding, 'x');
			
			NSRect old_frame = the_rect;
			NSRect new_frame;
			new_frame.origin.x = old_frame.origin.x + (CGFloat)horiz_padding*0.5;
			new_frame.origin.y = old_frame.origin.y + (CGFloat)vert_padding*0.5;
			new_frame.size.width = old_frame.size.width - (CGFloat)horiz_padding;
			new_frame.size.height = old_frame.size.height - (CGFloat)vert_padding;
			
			the_rect = new_frame;
		}
		
	}

//	NSLog(@"view %@, rect:%@", the_view, NSStringFromRect(the_rect));
	return the_rect;

}

// If you change these, make sure to update any overrides that copy/paste this pattern (e.g. cocoaButtonLayoutUpdateMethod)
void iupdrvBaseLayoutUpdateMethod(Ihandle *ih)
{

	NSView* parent_view = nil;
	NSView* child_view = nil;
	
	parent_view = iupCocoaCommonBaseLayoutGetParentView(ih);
	child_view = iupCocoaCommonBaseLayoutGetChildView(ih);
	
	
	
	
	NSRect parent_rect = [parent_view frame];

	
/*
#if 0 // experiment to try to handle NSBox directly instead of using cocoaFrameGetInnerNativeContainerHandleMethod. I think cocoaFrameGetInnerNativeContainerHandleMethod is better, but I haven't vetted everything.
	int current_width = ih->currentwidth;
	int current_height = ih->currentheight;

	if([parent_native_handle isKindOfClass:[NSBox class]])
	{
		NSBox* box_view = (NSBox*)parent_native_handle;
		NSView* box_content_view = [box_view contentView];
		
		CGFloat diff_width = NSWidth([box_view frame]) - NSWidth([box_content_view frame]);
		CGFloat diff_height = NSHeight([box_view frame]) - NSHeight([box_content_view frame]);

		current_width = current_width - diff_width;
		current_height = current_height - diff_height;
		
		NSRect the_rect = NSMakeRect(
		ih->x,
		// Need to invert y-axis, and also need to shift/account for height of widget because that is also lower-left instead of upper-left.
		parent_rect.size.height - ih->y - ih->currentheight,
		current_width,
		ih->currentheight
	);
	[the_view setFrame:the_rect];
//	[the_view setBounds:the_rect];
	}
	else
	{
		
	NSRect the_rect = NSMakeRect(
		ih->x,
		// Need to invert y-axis, and also need to shift/account for height of widget because that is also lower-left instead of upper-left.
		parent_rect.size.height - ih->y - ih->currentheight,
		ih->currentwidth,
		ih->currentheight
	);
	[the_view setFrame:the_rect];
//	[the_view setBounds:the_rect];
	}
#else
	
			
	NSRect the_rect = NSMakeRect(
		ih->x,
		// Need to invert y-axis, and also need to shift/account for height of widget because that is also lower-left instead of upper-left.
		parent_rect.size.height - ih->y - ih->currentheight,
		ih->currentwidth,
		ih->currentheight
	);
//	the_rect.origin.y = the_rect.origin.y - 16.0*0.5; // I don't realy understand the logic of this offset, particularly the -1


	// for padding
	// drat, data is private and requires a per-widget header
	{

		char* padding_str = iupAttribGet(ih, "PADDING");
		if((NULL != padding_str) && (padding_str[0] != '\0'))
		{
			int horiz_padding = 0;
			int vert_padding = 0;
			iupStrToIntInt(padding_str, &horiz_padding, &vert_padding, 'x');
			
			NSRect old_frame = the_rect;
			NSRect new_frame;
			new_frame.origin.x = old_frame.origin.x + (CGFloat)horiz_padding*0.5;
			new_frame.origin.y = old_frame.origin.y + (CGFloat)vert_padding*0.5;
			new_frame.size.width = old_frame.size.width - (CGFloat)horiz_padding;
			new_frame.size.height = old_frame.size.height - (CGFloat)vert_padding;
			
			the_rect = new_frame;
		}
		
	}

//	NSLog(@"view %@, rect:%@", the_view, NSStringFromRect(the_rect));
	[child_view setFrame:the_rect];
#endif
*/
	
	NSRect child_rect = iupCocoaCommonBaseLayoutComputeChildFrameRectFromParentRect(ih, parent_rect);
	[child_view setFrame:child_rect];
	

	
}

void iupdrvBaseUnMapMethod(Ihandle* ih)
{
	// Destroy the context menu ih it exists
	{
		Ihandle* context_menu_ih = (Ihandle*)iupCocoaCommonBaseGetContextMenuAttrib(ih);
		if(NULL != context_menu_ih)
		{
			IupDestroy(context_menu_ih);
		}
		iupCocoaCommonBaseSetContextMenuAttrib(ih, NULL);
	}
	
	// Why do I need this when everything else has its own UnMap method?
	NSLog(@"iupdrvBaseUnMapMethod not implemented. Might be leaking");
	id the_handle = ih->handle;
	iupCocoaRemoveFromParent(ih);
	[the_handle release];
}

static void iupCocoaDisplayUpdate(Ihandle *ih)
{
	id the_handle = ih->handle;
	
	if([the_handle isKindOfClass:[NSView class]])
	{
		NSView* the_view = (NSView*)the_handle;
		[the_view setNeedsDisplay:YES];
	}
	else if([the_handle isKindOfClass:[NSWindow class]])
	{
		// Cocoa generally does this automatically
//		[the_handle display];
	}
	else if([the_handle isKindOfClass:[CALayer class]])
	{
		CALayer* the_layer = (CALayer*)the_handle;
		[the_layer setNeedsDisplay];
	}
	else
	{
		NSCAssert(1, @"Unexpected type for widget");
		@throw @"Unexpected type for widget";
	}

}

void iupdrvDisplayRedraw(Ihandle *ih)
{
	iupCocoaDisplayUpdate(ih);
}

// Untested: I'm not sure what actually uses/needs this. (I see an MDI reference, but we MDI shouldn't exist on anything but Windows.)
void iupdrvScreenToClient(Ihandle* ih, int *x, int *y)
{
	// Do we need to invert the y?
	// And is it the mainScreen or a different one?
	NSRect main_screen = [[NSScreen mainScreen] frame];
	

	NSPoint screen_point = { *x, main_screen.size.height - *y };
	
	NSView* main_view = iupCocoaGetMainView(ih);
	NSWindow* the_window = [main_view window];
	NSRect screen_rect = NSMakeRect(screen_point.x, screen_point.y, 0, 0);
	NSRect window_rect = [the_window convertRectFromScreen:screen_rect];

	NSPoint window_point = window_rect.origin;
	NSPoint view_point = [main_view convertPoint:window_point fromView:nil];
	NSRect view_frame = [main_view frame];

	CGFloat inverted_y = view_frame.size.height - view_point.y;
	view_point.y = inverted_y;

	*x = view_point.x;
	*y = view_point.y;
}


void iupdrvClientToScreen(Ihandle* ih, int *x, int *y)
{
	
	NSView* main_view = iupCocoaGetMainView(ih);
	NSWindow* the_window = [main_view window];

	NSRect view_frame = [main_view frame];
	// Do we need to invert the y?
	NSPoint start_point = { *x, view_frame.size.height - *y };

	NSPoint window_point = [main_view convertPoint:start_point toView:nil];
	
	NSRect window_rect = NSMakeRect(window_point.x, window_point.y, 0, 0);
	NSRect screen_rect = [the_window convertRectToScreen:window_rect];

	
	// And is it the mainScreen or a different one?
	NSRect main_screen = [[NSScreen mainScreen] frame];
	
	*x = screen_rect.origin.x;
	*y = main_screen.size.height - screen_rect.origin.y;
}

int iupdrvBaseSetZorderAttrib(Ihandle* ih, const char* value)
{
  return 0;
}

void iupdrvSetVisible(Ihandle* ih, int visible)
{
	id the_object = ih->handle;
	if([the_object isKindOfClass:[NSWindow class]])
	{
		// NOT IMPLEMENTED
		NSLog(@"iupdrvSetVisible for NSWindow is not implemented");
	}
	else if([the_object isKindOfClass:[NSView class]])
	{
		NSView* the_view = (NSView*)the_object;
		bool is_hidden = !(bool)visible;
		[the_view setHidden:is_hidden];
	}
	else if([the_object isKindOfClass:[NSViewController class]])
	{
		NSViewController* the_viewcontroller = (NSViewController*)the_object;
		NSView* the_view = [the_viewcontroller view];
		bool is_hidden = !(bool)visible;
		[the_view setHidden:is_hidden];
	}
	
}

int iupdrvIsVisible(Ihandle* ih)
{
	id the_object = ih->handle;
	if([the_object isKindOfClass:[NSWindow class]])
	{
		NSWindow* the_window = (NSWindow*)ih->handle;
		return [the_window isVisible];
	}
	else if([the_object isKindOfClass:[NSView class]])
	{
		NSView* the_view = (NSView*)the_object;
		return [the_view isHidden] ? NO : YES;
	}
	else if([the_object isKindOfClass:[NSViewController class]])
	{
		NSViewController* the_viewcontroller = (NSViewController*)the_object;
		NSView* the_view = [the_viewcontroller view];
		return [the_view isHidden] ? NO : YES;
	}
	else
	{
		return 1;
	}
}

int iupdrvIsActive(Ihandle *ih)
{
	id the_object = ih->handle;
#if 0
	if([the_object isKindOfClass:[NSControl class]])
	{
		NSControl* the_control = (NSControl*)the_object;
		return [the_control isEnabled];
	}
#else

	// Note: NSViewController's contentView is probably never going to respond to the enabled property, so its handling is skipped.

	// Our custom CanvasView is going back and forth between subclassing NSView and NSControl.
	// Make sure to not implement any other NSViews that do something wonky with the enabled property.
	if([the_object respondsToSelector:@selector(isEnabled)])
	{
		// I forgot if there are rules about primitive types when there is no header definition.
		// So I'm going to pretend it is a NSControl even though it may not be
		NSControl* the_control = (NSControl*)the_object;
		return [the_control isEnabled];
	}
	else if([the_object isKindOfClass:[NSScrollView class]])
	{
		NSView* document_view = [the_object documentView];
		if([document_view respondsToSelector:@selector(isEnabled)])
		{
			// I forgot if there are rules about primitive types when there is no header definition.
			// So I'm going to pretend it is a NSControl even though it may not be
			NSControl* the_control = (NSControl*)document_view;
			return [the_control isEnabled];
		}
	}
#endif
	return 1;
}

void iupdrvSetActive(Ihandle* ih, int enable)
{
	id the_object = ih->handle;
#if 0
	if([the_object isKindOfClass:[NSControl class]])
	{
		NSControl* the_control = (NSControl*)the_object;
		[the_control setEnabled:enable];
	}
#else


	// Note: NSViewController's contentView is probably never going to respond to the enabled property, so its handling is skipped.


	// Our custom CanvasView is going back and forth between subclassing NSView and NSControl.
	// Make sure to not implement any other NSViews that do something wonky with the enabled property.
	if([the_object respondsToSelector:@selector(setEnabled:)])
	{
		// I forgot if there are rules about primitive types when there is no header definition.
		// So I'm going to pretend it is a NSControl even though it may not be
		NSControl* the_control = (NSControl*)the_object;
		[the_control setEnabled:enable];
	}
	else if([the_object isKindOfClass:[NSScrollView class]])
	{
		NSView* document_view = [the_object documentView];
		if([document_view respondsToSelector:@selector(setEnabled:)])
		{
			// I forgot if there are rules about primitive types when there is no header definition.
			// So I'm going to pretend it is a NSControl even though it may not be
			NSControl* the_control = (NSControl*)document_view;
			[the_control setEnabled:enable];
		}
	}
#endif
}

char* iupdrvBaseGetXAttrib(Ihandle *ih)
{
  return NULL;
}

char* iupdrvBaseGetYAttrib(Ihandle *ih)
{

  return NULL;
}

/*
char* iupdrvBaseGetClientSizeAttrib(Ihandle *ih)
{

    return NULL;

}
 */

// TODO: I don't know if anything actually uses this.
int iupdrvBaseSetBgColorAttrib(Ihandle* ih, const char* value)
{
	id the_object = ih->handle;

	// Our custom CanvasView is going back and forth between subclassing NSView and NSControl.
	// Make sure to not implement any other NSViews that do something wonky with the enabled property.
	if([the_object respondsToSelector:@selector(setBackgroundColor:)])
	{
		unsigned char r, g, b;
		if(iupStrToRGB(value, &r, &g, &b))
		{
			CGFloat red = r/255.0;
			CGFloat green = g/255.0;
			CGFloat blue = b/255.0;
			
			NSColor* the_color = [NSColor colorWithCalibratedRed:red green:green blue:blue alpha:1.0];
			[the_object setBackgroundColor:the_color];
		}
		else
		{
			[the_object setBackgroundColor:nil];
		}
		return 1;
	}
	
  return 0;
}

int iupdrvBaseSetCursorAttrib(Ihandle* ih, const char* value)
{

  return 0;
}


int iupdrvGetScrollbarSize(void)
{

  return 0;
}

/*
From Apple's quick documentation:
Do not include the accessibility element’s type in the label (for example, write Play not Play button.). If possible, use a single word. To help ensure that accessibility clients like VoiceOver read the label with the correct intonation, this label should start with a capital letter. Do not put a period at the end. Always localize the label.
*/
IUP_SDK_API void iupdrvSetAccessibleTitle(Ihandle *ih, const char* title)
{
	id the_object = ih->handle;

	// Our custom CanvasView is going back and forth between subclassing NSView and NSControl.
	// Make sure to not implement any other NSViews that do something wonky with the enabled property.
	if([the_object respondsToSelector:@selector(setAccessibilityLabel:)])
	{
		if(!title)
		{
			[the_object setAccessibilityLabel:nil];
		}
		else
		{
            NSString* ns_title = [NSString stringWithUTF8String:title];
			[the_object setAccessibilityLabel:ns_title];
		}
	}
}

void iupdrvBaseRegisterCommonAttrib(Iclass* ic)
{
	/*
#ifndef GTK_MAC
  #ifdef WIN32                                 
    iupClassRegisterAttribute(ic, "HFONT", iupgtkGetFontIdAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT|IUPAF_NO_STRING);
  #else
    iupClassRegisterAttribute(ic, "XFONTID", iupgtkGetFontIdAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT|IUPAF_NO_STRING);
  #endif
#endif
  iupClassRegisterAttribute(ic, "PANGOFONTDESC", iupgtkGetPangoFontDescAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT|IUPAF_NO_STRING);
*/
}

void iupdrvBaseRegisterVisualAttrib(Iclass* ic)
{
	
}


void iupdrvPostRedraw(Ihandle *ih)
{
	iupCocoaDisplayUpdate(ih);
}

void iupdrvRedrawNow(Ihandle *ih)
{
	iupCocoaDisplayUpdate(ih);
}
void iupdrvSendKey(int key, int press)
{
	
}
void iupdrvSendMouse(int x, int y, int bt, int status)
{
	
}
void iupdrvSleep(int time)
{
	
}
void iupdrvWarpPointer(int x, int y)
{
	
}

// This will copy all the menu items provided by src_menu and append them into dst_menu with a separator.
void iupCocoaCommonBaseAppendMenuItems(NSMenu* dst_menu, NSMenu* src_menu)
{
	if((src_menu != nil) && ([src_menu numberOfItems] > 0))
	{
		// Add a separator to separate the user's items from the default items
		[dst_menu addItem:[NSMenuItem separatorItem]];
		
		NSArray<NSMenuItem*>* item_array = [src_menu itemArray];
		for(NSMenuItem* a_default_item in item_array)
		{
			// We have to copy the item otherwise Cocoa will complain that the same menu item is used in multiple places.
			NSMenuItem* item_copy = [a_default_item copy];
			[dst_menu addItem:item_copy];
			[item_copy release];
		}
	}
}

// This will copy all the menu items provided by the class's defaultMenu and append them into dst_menu with a separator.
void iupCocoaCommonBaseAppendDefaultMenuItemsForClassType(NSMenu* dst_menu, Class class_of_widget)
{
	// If the class provides a defaultMenu, we should merge those items with our menu
	if([class_of_widget respondsToSelector:@selector(defaultMenu)])
	{
		NSMenu* default_menu = [class_of_widget defaultMenu];
		iupCocoaCommonBaseAppendMenuItems(dst_menu, default_menu);
	}
}

// Because we often have container views wrapping our core objects (e.g. scrollview wraps canvas, stackview wraps NSTextField)
// this helper function lets us split out the ih from the widget, so we don't have to assume the widget is ih->handle.
// So provide the ih, and provide the real core widget that provides [NSResponder setMenu:] that we should set.
// The menu should be in menu_ih.
void iupCocoaCommonBaseSetContextMenuForWidget(Ihandle* ih, id widget_to_attach_menu_to, Ihandle* menu_ih)
{
	// Save the menu Ihandle in this widget's Ihandle so we can GetContextMenuAttrib
	iupAttribSet(ih, "_COCOA_CONTEXT_MENU_IH", (const char*)menu_ih);


	// Unset the existing menu
	if(NULL == menu_ih)
	{
		if([widget_to_attach_menu_to respondsToSelector:@selector(setMenu:)])
		{
			[widget_to_attach_menu_to setMenu:nil];
		}
		return;
	}

	// FIXME: The Menu might not be IupMap'd yet. (Presumably because we do not attach it directly to a dialog in this case.)
	// I think calling IupMap() is the correct thing to do and fixes the problem.
	// But this should be reviewed.
	if(NULL == menu_ih->handle)
	{
		IupMap(menu_ih);
	}
	
	// Make sure we have an IupMenu
	if(menu_ih->iclass->nativetype != IUP_TYPEMENU)
	{
		// call IUPASSERT?
		return;
	}
	// Make sure we have a NSMenu
	if(![(id)menu_ih->handle isKindOfClass:[NSMenu class]])
	{
		// call IUPASSERT?
		return;
	}


	NSMenu* the_menu = (NSMenu*)menu_ih->handle;
	if([widget_to_attach_menu_to respondsToSelector:@selector(setMenu:)])
	{
		iupCocoaCommonBaseAppendDefaultMenuItemsForClassType(the_menu, [widget_to_attach_menu_to class]);
		[widget_to_attach_menu_to setMenu:the_menu];
	}

}


int iupCocoaCommonBaseIupButtonForCocoaButton(NSInteger which_cocoa_button)
{
	if(0 == which_cocoa_button)
	{
		return IUP_BUTTON1;
	}
	else if(1 == which_cocoa_button) // right
	{
		return IUP_BUTTON3;
	}
	else if(2 == which_cocoa_button) // middle
	{
		return IUP_BUTTON2;
	}
	else
	{
		// NOTE: IUP_BUTTON are ASCII values.
		return (int)(which_cocoa_button + 0x30);
	}
}

bool iupCocoaCommonBaseHandleMouseButtonCallback(Ihandle* ih, NSEvent* the_event, NSView* represented_view, bool is_pressed)
{
	IFniiiis callback_function;
	bool caller_should_propagate = true;

	callback_function = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
	if(callback_function)
	{
	    // We must convert the mouse event locations from the window coordinate system to the
		// local view coordinate system.
		NSPoint the_point = [the_event locationInWindow];
		NSPoint converted_point = [represented_view convertPoint:the_point fromView:nil];

		// We must flip the y to go from Cartesian to IUP
		NSRect view_frame = [represented_view frame];
		CGFloat inverted_y = view_frame.size.height - converted_point.y;

		// Button 0 is left
		// Button 1 is right
		// Button 2 is middle
		// Button 3 keeps going
		NSInteger which_cocoa_button = [the_event buttonNumber];
		char mod_status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
		iupcocoaButtonKeySetStatus(the_event, mod_status);

		if([the_event modifierFlags] & NSControlKeyMask)
		{
			// Should Ctrl-Left-click be a right click?
			if(0 == which_cocoa_button)
			{
				which_cocoa_button = 1; // make right button
			}
		}
		else if([the_event modifierFlags] & NSAlternateKeyMask)
		{

		}
		else if([the_event modifierFlags] & NSCommandKeyMask)
		{
		}
		else
		{
		}
		
		int which_iup_button = iupCocoaCommonBaseIupButtonForCocoaButton(which_cocoa_button);
	
		NSLog(@"Iup mouse button callback: <x,y>=<%f, %f, %f>, is_pressed=%d, button_num:%d", converted_point.x, converted_point.y, inverted_y, is_pressed, which_iup_button);

	
		int callback_result = callback_function(ih, which_iup_button, is_pressed, iupROUND(converted_point.x), iupROUND(inverted_y), mod_status);
		if(IUP_CLOSE == callback_result)
		{
			IupExitLoop();
			caller_should_propagate = false;
		}
		else if(IUP_IGNORE == callback_result)
		{
			caller_should_propagate = false;
		}
		else if(IUP_CONTINUE == callback_result)
		{
			caller_should_propagate = true;
		}
		else
		{
			caller_should_propagate = false;
		}
	}
	else
	{
		caller_should_propagate = true;
	}
	return !caller_should_propagate;
}

bool iupCocoaCommonBaseHandleMouseMotionCallback(Ihandle* ih, NSEvent* the_event, NSView* represented_view)
{
	bool caller_should_propagate = true;
	IFniis callback_function;
	callback_function = (IFniis)IupGetCallback(ih, "MOTION_CB");
	if(callback_function)
	{
	    // We must convert the mouse event locations from the window coordinate system to the
		// local view coordinate system.
		NSPoint the_point = [the_event locationInWindow];
		NSPoint converted_point = [represented_view convertPoint:the_point fromView:nil];
		
		// We must flip the y to go from Cartesian to IUP
		NSRect view_frame = [represented_view frame];
		CGFloat inverted_y = view_frame.size.height - converted_point.y;
		char mod_status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
		iupcocoaButtonKeySetStatus(the_event, mod_status);

	
		int callback_result = callback_function(ih,  iupROUND(converted_point.x), iupROUND(inverted_y), mod_status);
		if(IUP_CLOSE == callback_result)
		{
			IupExitLoop();
			caller_should_propagate = false;
		}
		else if(IUP_IGNORE == callback_result)
		{
			caller_should_propagate = false;
		}
		else if(IUP_CONTINUE == callback_result)
		{
			caller_should_propagate = true;
		}
		else
		{
			caller_should_propagate = false;
		}
	}
	else
	{
		caller_should_propagate = true;
	}
	return !caller_should_propagate;
}


// TODO: IUP doesn't support y-axis scroll events. We need to add a new API.
bool iupCocoaCommonBaseScrollWheelCallback(Ihandle* ih, NSEvent* the_event, NSView* represented_view)
{
	IFnfiis callback_function;
	bool caller_should_propagate = true;

	callback_function = (IFnfiis)IupGetCallback(ih, "WHEEL_CB");
	if(callback_function)
	{
	    // We must convert the mouse event locations from the window coordinate system to the
		// local view coordinate system.
		NSPoint the_point = [the_event locationInWindow];
		NSPoint converted_point = [represented_view convertPoint:the_point fromView:nil];

		CGFloat delta_x = -[the_event deltaX];
		CGFloat delta_y = [the_event deltaY];

		// We must flip the y to go from Cartesian to IUP
		NSRect view_frame = [represented_view frame];
		CGFloat inverted_y = view_frame.size.height - converted_point.y;

		char mod_status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
		iupcocoaButtonKeySetStatus(the_event, mod_status);

		int callback_result = callback_function(ih, delta_x, iupROUND(converted_point.x), iupROUND(inverted_y), mod_status);
		if(IUP_CLOSE == callback_result)
		{
			IupExitLoop();
			caller_should_propagate = false;
		}
		else if(IUP_IGNORE == callback_result)
		{
			caller_should_propagate = false;
		}
		else if(IUP_CONTINUE == callback_result)
		{
			caller_should_propagate = true;
		}
		else
		{
			caller_should_propagate = false;
		}
	}
	else
	{
		caller_should_propagate = true;
	}
	return !caller_should_propagate;
}


int iupCocoaCommonBaseSetLayerBackedAttrib(Ihandle* ih, const char* value)
{
	id the_object = ih->handle;
	
	NSView* main_view = iupCocoaGetMainView(ih);
	if(nil != main_view)
	{
		BOOL should_enable = (BOOL)iupStrBoolean(value);
		[main_view setWantsLayer:should_enable];
		NSView* root_view = iupCocoaGetMainView(ih);
		if(root_view != main_view)
		{
			[root_view setWantsLayer:should_enable];
		}
	}
	else if([the_object respondsToSelector:@selector(setWantsLayer:)])
	{
		BOOL should_enable = (BOOL)iupStrBoolean(value);
		[the_object setWantsLayer:should_enable];
	}
	else if([the_object isKindOfClass:[NSViewController class]])
	{
		NSView* the_view = [(NSViewController*)the_object view];
		BOOL should_enable = (BOOL)iupStrBoolean(value);
		[the_view setWantsLayer:should_enable];
	}
	return 0;
}

char* iupCocoaCommonBaseGetLayerBackedAttrib(Ihandle* ih)
{
	id the_object = ih->handle;
	NSView* main_view = iupCocoaGetRootView(ih);
	if(nil != main_view)
	{
		BOOL is_enabled = [main_view wantsLayer];
		return iupStrReturnBoolean(is_enabled);
	}
	else if([the_object respondsToSelector:@selector(wantsLayer)])
	{
		BOOL is_enabled = [the_object wantsLayer];
		return iupStrReturnBoolean(is_enabled);
	}
	else if([the_object isKindOfClass:[NSViewController class]])
	{
		NSView* the_view = [(NSViewController*)the_object view];
		BOOL is_enabled = [the_view wantsLayer];
		return iupStrReturnBoolean(is_enabled);
	}
	return iupStrReturnBoolean(false);
}

int iupCocoaCommonBaseSetContextMenuAttrib(Ihandle* ih, const char* value)
{
	Ihandle* menu_ih = (Ihandle*)value;
//	id widget_to_attach_menu_to = ih->handle;
	id widget_to_attach_menu_to = iupCocoaGetMainView(ih);

	iupCocoaCommonBaseSetContextMenuForWidget(ih, widget_to_attach_menu_to, menu_ih);
	
	return 1;
}

char* iupCocoaCommonBaseGetContextMenuAttrib(Ihandle* ih)
{
	return (char*)iupAttribGet(ih, "_COCOA_CONTEXT_MENU_IH");
}

int iupCocoaCommonBaseSetSendActionAttrib(Ihandle* ih, const char* value)
{
	if(NULL == value)
	{
		return 0;
	}
	
	NSView* target_view = nil;
	id sender_object = nil;
	if(NULL == ih)
	{
		// Send through the normal responder chain starting at the first responder
		target_view = nil;
		sender_object = nil;
	}
	else
	{
	 	target_view = iupCocoaGetMainView(ih);
	 	sender_object = target_view;
	}
	
	// TODO: Create well know aliases.
	// For now, user must put the exact selector name, with the colon
	// undo: redo: cut: copy: paste: pasteAsPlainText:
	SEL the_selector = sel_registerName(value);
	
	[[NSApplication sharedApplication] sendAction:the_selector to:target_view from:sender_object];

	return 0;
}

