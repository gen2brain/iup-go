/** \file
* \brief Progress bar Control
*
* See Copyright Notice in "iup.h"
*/

#include <Cocoa/Cocoa.h>

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
#include "iup_progressbar.h"
#include "iup_drv.h"

#include "iupcocoa_drv.h"

#import <QuartzCore/QuartzCore.h>



// TODO: API: I think we're going to need a separate start/stop key.
// Cocoa Indeterminate is for progresses you don't know the range for, but are still animated when in progress.

// TODO: FEATURE: Cocoa provides spinner style




/*
Apple Bug Radar: 32298779
Due to an Apple bug, we can't just use setFrameCenterRotation: to create a vertical progress bar.
The rendering is really screwed up under certain conditions. 
It seems that Layer Backed Views must be enabled,
and should be enabled immediately when created (and not enabled later) or the rendering is glitched.
As a workaround, we can create a container NSView which we rotate instead. 
And this doesn't require layer backed views to be enabled.
However, there seem to be other bugs with the container approach we also have to workaround.
Trying to move the NSProgressIndicator inside the view to compensate the position for the rotated transform brings back the corruption.
We can transform the container view, but now we have a coordinate mismatch between Cocoa and IUP.
So another workaround is to create another container view.
We translate the inner container view so the outer container view maps 1-to-1 with the IUP x,y expected values.
But I found some other rendering glitches with this.
First, don't use an NSView with clipping disabled. This causes all sorts of glitches (vertical pixels appearing nearby and flickering on resize).
Also, enabling layer backed mode would break everything and go back to the original corruption.
Without clipping, we must create NSViews large enough for the entire progressbar in either direction.
*/

static NSView* cocoaProgressBarGetRootView(Ihandle* ih)
{
	NSView* root_container_view = (NSView*)ih->handle;
	NSCAssert([root_container_view isKindOfClass:[NSView class]], @"Expected NSView");
	return root_container_view;
}

// This is the intermediate transform view
static NSView* cocoaProgressBarGetTransformView(Ihandle* ih)
{
	if(iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "VERTICAL"))
	{
		NSView* root_container_view = cocoaProgressBarGetRootView(ih);
		NSView* intermediate_transform_view = [[root_container_view subviews] firstObject];
		NSCAssert([intermediate_transform_view isKindOfClass:[NSView class]], @"Expected NSView");
		return intermediate_transform_view;
	}
	else
	{
		NSView* root_container_view = (NSView*)ih->handle;
		NSCAssert([root_container_view isKindOfClass:[NSView class]], @"Expected NSView");
		return root_container_view;
	}
}

static NSProgressIndicator* cocoaProgressBarGetProgressIndicator(Ihandle* ih)
{
	if(iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "VERTICAL"))
	{
		NSView* intermediate_transform_view = cocoaProgressBarGetTransformView(ih);
		NSProgressIndicator* progress_bar = [[intermediate_transform_view subviews] firstObject];
		NSCAssert([progress_bar isKindOfClass:[NSProgressIndicator class]], @"Expected NSProgressIndicator");
		return progress_bar;
	}
	else
	{
		NSProgressIndicator* root_container_view = (NSProgressIndicator*)ih->handle;
		NSCAssert([root_container_view isKindOfClass:[NSProgressIndicator class]], @"Expected NSProgressIndicator");
		return root_container_view;
	}
}





static int cocoaProgressBarSetValueAttrib(Ihandle* ih, const char* value)
{
	NSProgressIndicator* progress_bar = cocoaProgressBarGetProgressIndicator(ih);
	
	if (ih->data->marquee)
	{
		return 0;
	}
	
	if(!value)
	{
		ih->data->value = 0;
	}
	else
	{
		iupStrToDouble(value, &(ih->data->value));
	}

	iProgressBarCropValue(ih);

	[progress_bar setMinValue:ih->data->vmin];
	[progress_bar setMaxValue:ih->data->vmax];
	[progress_bar setDoubleValue:ih->data->value];

	//	[progress_bar setFrameCenterRotation:ih->data->value];
//	[progress_bar setFrameCenterRotation:M_PI/180.0 * ih->data->value];

	// Hack to test rotation
	// The problem with this technique is that the widget "snaps back" on window resize.
	// This is the typical problem of the layer going behind the view's back and the view eventually re-asserting itself.
#if 0
	CALayer* bar_layer = [progress_bar layer];
	
//	[[progress_bar superview] setWantsLayer:YES];
	[bar_layer setAnchorPoint:CGPointMake(0.5, 0.5)];
//	CGAffineTransform transform = progress_bar.layer.affineTransform;
	CGAffineTransform transform = CGAffineTransformIdentity;
//	transform = CGAffineTransformRotate(transform, M_PI/180.0 * ih->data->value);
	transform = CGAffineTransformRotate(transform, M_PI/180.0 * 90.0);
//	transform = CGAffineTransformRotate(transform, ih->data->value);
	progress_bar.layer.affineTransform = transform;
#endif
	
	// Not sure if I really need this, but
	// https://developer.apple.com/library/mac/qa/qa1473/_index.html
	[progress_bar displayIfNeeded];
	
	

//	gtk_progress_bar_set_fraction(pbar, (ih->data->value - ih->data->vmin) / (ih->data->vmax - ih->data->vmin));
	
	return 0;
}

static int cocoaProgressBarSetMarqueeAttrib(Ihandle* ih, const char* value)
{
	if (!ih->data->marquee)
	{
		return 0;
	}
	NSProgressIndicator* progress_bar = cocoaProgressBarGetProgressIndicator(ih);
	
	if (iupStrBoolean(value))
	{
		// FIXME: This feels like a hack
		[progress_bar startAnimation:nil];
		IupSetAttribute(ih->data->timer, "RUN", "YES");
	}
	else
	{
		// FIXME: This feels like a hack
		[progress_bar stopAnimation:nil];
	}
	return 1;
}

static int cocoaProgressBarSetContextMenuAttrib(Ihandle* ih, const char* value)
{
	Ihandle* menu_ih = (Ihandle*)value;
	NSProgressIndicator* progress_bar = cocoaProgressBarGetProgressIndicator(ih);
	iupCocoaCommonBaseSetContextMenuForWidget(ih, progress_bar, menu_ih);

	return 1;
}


static int cocoaProgressBarMapMethod(Ihandle* ih)
{
//	char* value;


	int initial_width = 200;
	int initial_height = 30;
	
//	woffset += 60;
//	hoffset += 10;
	//	ih->data->type = 0;
	
	// Due to an Apple bug, we can't just use setFrameCenterRotation: to create a vertical progress bar.
	// The rendering is really screwed up under certain conditions. It seems that Layer Backed Views must be enabled, and should be enabled immediately when created (and not enabled later) or the rendering is glitched.
	// As a workaround, we can create a container NSView which we rotate instead. And this doesn't even require layer backed views to be enabled.

	// IUP doc says 200x30 is the default
	// However, Mac always draws the bar 6 pixels thick. It seems to pad with empty space if you make it bigger. And Interface Builder says the height is always 20 (presuming padding around 6 pixels)
	
	IupGetIntInt(ih, "RASTERSIZE", &initial_width, &initial_height);
	if(0 == initial_width)
	{
		initial_width = 200;
	}
	if(0 == initial_height)
	{
		initial_height = 30;
	}
	
//	NSRect initial_rect = NSMakeRect(0, 0, 200, 30);
//	NSRect initial_rect = NSMakeRect(0, 0, 200, 20);
	NSRect initial_rect = NSMakeRect(0, 0, initial_width, initial_height);


	NSProgressIndicator* progress_indicator = [[NSProgressIndicator alloc] initWithFrame:initial_rect];
	NSView* container_view = nil;

	

	// Warning: Saw a claim that threaded animation breaks vertical
	[progress_indicator setUsesThreadedAnimation:YES];

	// FIXME: Iup doesn't seem to have explicit start/stop commands.
	// Cocoa Indeterminate is for progresses you don't know the range for, but are still animated when in progress.
	[progress_indicator startAnimation:nil];

	
	// Vertical mode is completely broken. This appears to be a Mac bug.
	if (iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "VERTICAL"))
	{
		
		int max_dim = iupMAX(initial_width, initial_height);
		// The container views must be large enough to hold the progressbar in either direction.
		NSRect container_rect = NSMakeRect(0, 0, max_dim, max_dim);
		
		container_view = [[NSView alloc] initWithFrame:container_rect];
		NSView* transform_view = [[NSView alloc] initWithFrame:container_rect];
		
		[container_view addSubview:transform_view];
		[transform_view addSubview:progress_indicator];
		[transform_view release];
		[progress_indicator release];
		
		
		
		NSRect widget_frame = [container_view frame];
		NSSize widget_size = widget_frame.size;
		

		// subtract the full_view_width and add back the thickness of the bar, minus 6 pixels for the actual bar (that way at say <0,0> won't clip half the bar)
		[transform_view setFrame:NSMakeRect(0.0-container_rect.size.width+(initial_rect.size.height-6.0), 0.0, widget_size.width, widget_size.height)];
//		[transform_view setFrame:NSMakeRect(0.0-200.0+(30.0-6.0), 0, 200, 200)];
		
		if (ih->userheight < ih->userwidth)
		{
			int tmp = ih->userheight;
			ih->userheight = ih->userwidth;
			ih->userwidth = tmp;
		}

		// do after setFrame: before setFrameCenterRotation: otherwise me must transform the values to include the rotation.
		[transform_view setFrameCenterRotation:90.0];
//		[container_view setWantsLayer:YES];

		// We must not allow IUP to EXPAND the width of the NSProgressIndicator so unset the bit flag if it is set.
		ih->expand = ih->expand & ~IUP_EXPAND_WIDTH;

		// Autosizing breaks vertical. We can't support EXPAND right now :(
		// And setting the frame on the progress indicator also breaks vertical so we can't manually resize either.
		//	[transform_view setAutoresizingMask:(NSViewWidthSizable|NSViewHeightSizable)];
		//	[progress_indicator setAutoresizingMask:(NSViewWidthSizable|NSViewHeightSizable)];
		//[transform_view setAutoresizingMask:(NSViewWidthSizable)];
		//[progress_indicator setAutoresizingMask:(NSViewWidthSizable)];

	}
	else
	{
//		[progress_indicator setWantsLayer:NO];

		
		container_view = progress_indicator;
		
		
		// Autosizing breaks vertical
		//	[transform_view setAutoresizingMask:(NSViewWidthSizable|NSViewHeightSizable)];
		//	[progress_indicator setAutoresizingMask:(NSViewWidthSizable|NSViewHeightSizable)];
		//[transform_view setAutoresizingMask:(NSViewWidthSizable)];
		[progress_indicator setAutoresizingMask:(NSViewWidthSizable)];
	
		
		// We must not allow IUP to EXPAND the height of the NSProgressIndicator so unset the bit flag if it is set.
		// Mac fixes the thickness to 6 pixels. Expanding causes the progress bar to become uncentered in the container view.
		// TODO: Maybe we should remove the container view for just the horizontal.
		ih->expand = ih->expand & ~IUP_EXPAND_HEIGHT;

		
	}
	
	if (iupAttribGetBoolean(ih, "MARQUEE"))
	{

		ih->data->marquee = 1;
		[progress_indicator setIndeterminate:YES];
		
		
	}
	else
	{
		ih->data->marquee = 0;
		[progress_indicator setIndeterminate:NO];

	}
	

	

	

	

	
//	[progress_indicator sizeToFit];
	
	
	
//	ih->handle = progress_indicator;
	ih->handle = container_view;

	
	iupCocoaSetAssociatedViews(ih, progress_indicator, container_view);

	// All Cocoa views shoud call this to add the new view to the parent view.
	iupCocoaAddToParent(ih);
	

	
	
	
	
	
	return IUP_NOERROR;
}

static void cocoaProgressBarUnMapMethod(Ihandle* ih)
{
	id progress_bar = ih->handle;

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
	[progress_bar release];
	ih->handle = NULL;
	
}

void iupdrvProgressBarInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
	ic->Map = cocoaProgressBarMapMethod;
	ic->UnMap = cocoaProgressBarUnMapMethod;


  /* Driver Dependent Attribute functions */
  
  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  
  /* Special */
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);
#if 1

  /* IupProgressBar only */
  iupClassRegisterAttribute(ic, "VALUE",  iProgressBarGetValueAttrib,  cocoaProgressBarSetValueAttrib,  NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

	
  iupClassRegisterAttribute(ic, "ORIENTATION", NULL, NULL, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARQUEE",     NULL, cocoaProgressBarSetMarqueeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DASHED",      NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
#endif
	
	/* New API for view specific contextual menus (Mac only) */
	iupClassRegisterAttribute(ic, "CONTEXTMENU", iupCocoaCommonBaseGetContextMenuAttrib, cocoaProgressBarSetContextMenuAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

}
