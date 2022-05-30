/** \file
 * \brief Button Control
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

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_button.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_image.h"
#include "iup_key.h"

#include "iupcocoa_drv.h"

static const CGFloat kIupCocoaDefaultWidthNSButton = 1.0;
static const CGFloat kIupCocoaDefaultHeightNSButton = 32.0;

// the point of this is we have a unique memory address for an identifier
static const void* IUP_COCOA_BUTTON_RECEIVER_OBJ_KEY = "IUP_COCOA_BUTTON_RECEIVER_OBJ_KEY";


@interface IupCocoaButtonReceiver : NSObject
- (IBAction) myButtonClickAction:(id)the_sender;
@end

@implementation IupCocoaButtonReceiver

/*
- (void) dealloc
{
	[super dealloc];
}
*/


- (IBAction) myButtonClickAction:(id)the_sender;
{
	Icallback callback_function;
	Ihandle* ih = (Ihandle*)objc_getAssociatedObject(the_sender, IHANDLE_ASSOCIATED_OBJ_KEY);

	// CONFLICT: Cocoa buttons don't normally do anything for non-primary click. (Second click is supposed to trigger the contextual menu.)
	// Also Cocoa doesn't normall give callbacks for both down and up
	/*
	callback_function = IupGetCallback(ih, "BUTTON_CB");
	if(callback_function)
	{
		if(callback_function(ih) == IUP_CLOSE)
		{
			IupExitLoop();
		}
		
	}
	 */
	
	callback_function = IupGetCallback(ih, "ACTION");
	if(callback_function)
	{
		if(callback_function(ih) == IUP_CLOSE)
		{
			IupExitLoop();
		}
	}
}

@end



void iupdrvButtonAddBorders(Ihandle* ih, int *x, int *y)
{
//	NSLog(@"iupdrvButtonAddBorders in <%d, %d>", *x, *y);
	
	
	if(*y < (int)kIupCocoaDefaultHeightNSButton)
	{
		*y = (int)kIupCocoaDefaultHeightNSButton;
//		*y = (int)22;

	}
//	*x += 4; // a regular label seems to get 2 padding on each size
//	*x += 36; // the difference between a label and push button is 36 in Interface Builder

	*x += 27;
	
	/*
	NSView* the_view = (NSView*)ih->handle;
	NSRect view_frame = [the_view frame];
	*x = view_frame.size.width;
	*y = view_frame.size.height;
	
	*/
//	NSLog(@"iupdrvButtonAddBorders frame <%d, %d>", *x, *y);

}

static int cocoaButtonSetTitleAttrib(Ihandle* ih, const char* value)
{
	id the_button = ih->handle;

	if (ih->data->type & IUP_BUTTON_TEXT)  /* text or both */
	{
		if(value && *value!=0)
		{
			char* stripped_str = iupStrProcessMnemonic(value, NULL, 0);   /* remove & */
			
			// This will return nil if the string can't be converted.
			NSString* ns_string = [NSString stringWithUTF8String:stripped_str];
			
			if(stripped_str && stripped_str != value)
			{
				free(stripped_str);
			}
			
			[the_button setTitle:ns_string];
			
			if(ih->data->type & IUP_BUTTON_IMAGE)
			{
				// TODO: FEATURE: Cocoa allows text to be placed in different positions
				// https://developer.apple.com/library/mac/documentation/Cocoa/Conceptual/Button/Tasks/SettingButtonImage.html
				[the_button setImagePosition:NSImageLeft];
			}
			else
			{
				//			[the_button setImagePosition:NSNoImage];
				
			}
			
			return 1;
		}
	}
	
	return 0;
}


// The reason we need a custom layout is because the button is being positioned too high compared to other widgets.
// I think the reason is because the NSButton has both a lot of invisible padding (officially 32 high, but only about 22 visible)
// and I think some of that padding is not completely centered.
// So when putting a button next to a label or textfield, the button looks too high up compared to standard Cocoa/IB layout.
void cocoaButtonLayoutUpdateMethod(Ihandle *ih)
{
	
	NSView* parent_view = nil;
	NSView* child_view = nil;
	
	parent_view = iupCocoaCommonBaseLayoutGetParentView(ih);
	child_view = iupCocoaCommonBaseLayoutGetChildView(ih);
	
	NSRect parent_rect = [parent_view frame];
	
	NSRect child_rect = iupCocoaCommonBaseLayoutComputeChildFrameRectFromParentRect(ih, parent_rect);

	
	// Experimentally, it looks like I just need to shift 1 pixel down to make it look right.
	child_rect.origin.y = child_rect.origin.y - 1.0;
	
	
	[child_view setFrame:child_rect];
	
	
	
}


static int cocoaButtonMapMethod(Ihandle* ih)
{
	char* value;

	/*
	static int woffset = 0;
	static int hoffset = 0;
	
	woffset += 30;
	hoffset += 30;
//	ih->data->type = 0;
	
	 NSButton* the_button = [[NSButton alloc] initWithFrame:NSMakeRect(woffset, hoffset, 0, 0)];
	*/
	NSButton* the_button = [[NSButton alloc] initWithFrame:NSZeroRect];
	// I seem to be getting a default "Button" title for image button.
	[the_button setTitle:@""];
	

	value = iupAttribGet(ih, "IMAGE");
	if(value)
	{
		ih->data->type = IUP_BUTTON_IMAGE;
		
		const char* title = iupAttribGet(ih, "TITLE");
		if (title && *title!=0)
		{
			ih->data->type |= IUP_BUTTON_TEXT;
			
			
			char* stripped_str = iupStrProcessMnemonic(title, NULL, 0);   /* remove & */
			
			// This will return nil if the string can't be converted.
			NSString* ns_string = [NSString stringWithUTF8String:stripped_str];
			
			if(stripped_str && stripped_str != title)
			{
				free(stripped_str);
			}
			
			[the_button setTitle:ns_string];

			// TODO: FEATURE: Cocoa allows text to be placed in different positions
			// https://developer.apple.com/library/mac/documentation/Cocoa/Conceptual/Button/Tasks/SettingButtonImage.html
			[the_button setImagePosition:NSImageLeft];

		}
		else
		{
			// Explicitly set to NSImageOnly, otherwise expanding the image button does it in a off-centered way.
			[the_button setImagePosition:NSImageOnly];
		}
		
		
		[the_button setButtonType:NSMomentaryChangeButton];

		// I don't know what the style should be for images
		// https://mackuba.eu/2014/10/06/a-guide-to-nsbutton-styles/
//		[the_button setBezelStyle:NSRoundedBezelStyle];
		[the_button setBezelStyle:NSThickSquareBezelStyle];
//		[the_button setBezelStyle:NSShadowlessSquareBezelStyle];
//		[the_button setBezelStyle:NSTexturedSquareBezelStyle];
//		[the_button setBezelStyle:NSThickerSquareBezelStyle];

		
		NSImage* the_bitmap;
		int make_inactive = 0;
		

		if(iupAttribGet(ih, "IMINACTIVE"))
		{
			make_inactive = 1;
		}

		the_bitmap = iupImageGetImage(value, ih, make_inactive, NULL);
		[the_button setImage:the_bitmap];
		
		
		value = iupAttribGet(ih, "IMPRESS");
		if(value && *value!=0)
		{
			the_bitmap = iupImageGetImage(value, ih, make_inactive, NULL);
			[the_button setAlternateImage:the_bitmap];
		}
		
	}
	else
	{
		[the_button setButtonType:NSMomentaryLightButton];
		[the_button setBezelStyle:NSRoundedBezelStyle];
		
		ih->data->type = IUP_BUTTON_TEXT;

		

	}
	
	// Interface builder defaults to 13pt, but programmatic is smaller (12?). Setting the font fixes that difference.
	[the_button setFont:[NSFont systemFontOfSize:0]];

//	[the_button setButtonType:NSMomentaryLightButton];


	
//	[the_button sizeToFit];
	
	
	
	ih->handle = the_button;
	iupCocoaSetAssociatedViews(ih, the_button, the_button);

	// I'm using objc_setAssociatedObject/objc_getAssociatedObject because it allows me to avoid making subclasses just to hold ivars.
	objc_setAssociatedObject(the_button, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);
	// I also need to track the memory of the buttion action receiver.
	// I prefer to keep the Ihandle the actual NSView instead of the receiver because it makes the rest of the implementation easier if the handle is always an NSView (or very small set of things, e.g. NSWindow, NSView, CALayer).
	// So with only one pointer to deal with, this means we need our button to hold a reference to the receiver object.
	// This is generally not good Cocoa as buttons don't retain their receivers, but this seems like the best option.
	// Be careful of retain cycles.
	IupCocoaButtonReceiver* button_receiver = [[IupCocoaButtonReceiver alloc] init];
	[the_button setTarget:button_receiver];
	[the_button setAction:@selector(myButtonClickAction:)];
	// I *think* is we use RETAIN, the object will be released automatically when the button is freed.
	// However, the fact that this is tricky and I had to look up the rules (not to mention worrying about retain cycles)
	// makes me think I should just explicitly manage the memory so everybody is aware of what's going on.
	objc_setAssociatedObject(the_button, IUP_COCOA_BUTTON_RECEIVER_OBJ_KEY, (id)button_receiver, OBJC_ASSOCIATION_ASSIGN);

	
	// All Cocoa views shoud call this to add the new view to the parent view.
	iupCocoaAddToParent(ih);

	
	
	

	
	
//	gtk_widget_realize(ih->handle);
	
	/* update a mnemonic in a label if necessary */
//	iupgtkUpdateMnemonic(ih);
	
	return IUP_NOERROR;
}

static void cocoaButtonUnMapMethod(Ihandle* ih)
{
	id the_button = ih->handle;

	// Destroy the context menu ih it exists
	{
		Ihandle* context_menu_ih = (Ihandle*)iupCocoaCommonBaseGetContextMenuAttrib(ih);
		if(NULL != context_menu_ih)
		{
			IupDestroy(context_menu_ih);
		}
		iupCocoaCommonBaseSetContextMenuAttrib(ih, NULL);
	}
	
	id butten_receiver = objc_getAssociatedObject(the_button, IUP_COCOA_BUTTON_RECEIVER_OBJ_KEY);
	objc_setAssociatedObject(the_button, IUP_COCOA_BUTTON_RECEIVER_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);
	[butten_receiver release];
	
	iupCocoaRemoveFromParent(ih);
	iupCocoaSetAssociatedViews(ih, nil, nil);

	[the_button release];
	ih->handle = NULL;
	
}


void iupdrvButtonInitClass(Iclass* ic)
{
	/* Driver Dependent Class functions */
	ic->Map = cocoaButtonMapMethod;
	ic->UnMap = cocoaButtonUnMapMethod;
	

	ic->LayoutUpdate = cocoaButtonLayoutUpdateMethod;
#if 0
	
	/* Driver Dependent Attribute functions */
	
	/* Overwrite Common */
	iupClassRegisterAttribute(ic, "STANDARDFONT", NULL, gtkButtonSetStandardFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NO_SAVE|IUPAF_NOT_MAPPED);
	
	/* Overwrite Visual */
	iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, gtkButtonSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
	
	/* Visual */
	iupClassRegisterAttribute(ic, "BGCOLOR", NULL, gtkButtonSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
	
	/* Special */
	iupClassRegisterAttribute(ic, "FGCOLOR", NULL, gtkButtonSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
#endif

	iupClassRegisterAttribute(ic, "TITLE", NULL, cocoaButtonSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

#if 0
	/* IupButton only */
	iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, gtkButtonSetAlignmentAttrib, "ACENTER:ACENTER", NULL, IUPAF_NO_INHERIT);  /* force new default value */
	iupClassRegisterAttribute(ic, "IMAGE", NULL, gtkButtonSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, gtkButtonSetImInactiveAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "IMPRESS", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	
	iupClassRegisterAttribute(ic, "PADDING", iupButtonGetPaddingAttrib, gtkButtonSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
	iupClassRegisterAttribute(ic, "MARKUP", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);
#endif

	/* New API for view specific contextual menus (Mac only) */
	iupClassRegisterAttribute(ic, "CONTEXTMENU", iupCocoaCommonBaseGetContextMenuAttrib, iupCocoaCommonBaseSetContextMenuAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "LAYERBACKED", iupCocoaCommonBaseGetLayerBackedAttrib, iupCocoaCommonBaseSetLayerBackedAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE);

}
