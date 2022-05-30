/** \file
 * \brief Frame Control
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

#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_dialog.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_stdcontrols.h"
#include "iup_frame.h"

#include "iupcocoa_drv.h"



void iupdrvFrameGetDecorOffset(Ihandle* ih, int *x, int *y)
{
	// Drat: The padding will differ depending if there is a title or not (and where the title goes).
	*x = 0;
	*y = 0;
}

int iupdrvFrameHasClientOffset(Ihandle* ih)
{
	// If I don't set this to true, the y-padding in iupdrvFrameGetDecorOffset does nothing.
	// And without the padding, the first widget gets placed too high in the box and gets clipped.
	// Unless...I set the GetInnerNativeContainerHandle callback. Then it seems I can avoid needing iupdrvFrameGetDecorOffset
	return 0;
}

int iupdrvFrameGetTitleHeight(Ihandle* ih, int *h)
{
	// The title height uses a smaller font size than the normal system font,
	// so don't use iupdrvFontGetCharSize() since I don't think that font is stored in the ih. (Maybe that should be fixed.)
	// Also, it is offset differently depending on mode.
	// Also, the NSBox size gets bigger if there is no text. This is different than what IUP expects.
	// FIXME: Need to figure out different sizes. I'm pretty sure 0 is not correct.

/*
	if (iupAttribGet(ih, "_IUPFRAME_HAS_TITLE") || iupAttribGet(ih, "TITLE"))
	{
	
	}
	else
	{
		
	}
*/
	*h = 0;
  return 1;
}

int iupdrvFrameGetDecorSize(Ihandle* ih, int *w, int *h)
{
	// HACK: Need to make customizable based on whether title is shown or not.
/*
	if (iupAttribGet(ih, "_IUPFRAME_HAS_TITLE") || iupAttribGet(ih, "TITLE"))
	{
	
	}
	else
	{
		
	}
*/
	// FIXME: I put 14 as a first guess without measuring. It looked pretty good, but should be measured.
	*w = 14;
	// 22 seems to be okay. Testing with a multilist on the bottom of frame...19 clips the scrollbar at the bottom.
	// 20 doesn't have enough padding pixels under the scrollbar compared to other scrollbars I look at.
	*h = 22;
  return 1;
}


static void* cocoaFrameGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
	(void)child;
	
	NSBox* the_frame = ih->handle;
	
	return [the_frame contentView];
//	return (void*)gtk_bin_get_child((GtkBin*)ih->handle);
}

static int cocoaFrameSetTitleAttrib(Ihandle* ih, const char* title)
{
	NSBox* the_frame = (NSBox*)ih->handle;

	if(title && *title!=0)
	{
		NSString* ns_string = [NSString stringWithUTF8String:title];
		[the_frame setTitle:ns_string];
		[the_frame setTitlePosition:NSAtTop];

	}
	else
	{
		[the_frame setTitle:@""];
		[the_frame setTitlePosition:NSNoTitle];
		
	}
	
	
	iupdrvPostRedraw(ih);
	return 1;
}

static int cocoaFrameMapMethod(Ihandle* ih)
{

//	NSBox* the_frame = [[NSBox alloc] initWithFrame:NSZeroRect];
	NSBox* the_frame = [[NSBox alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)];

	{
		char* title;
		title = iupAttribGet(ih, "TITLE");
		if(title && *title!=0)
		{
			NSString* ns_string = [NSString stringWithUTF8String:title];
			[the_frame setTitle:ns_string];
		}
		else
		{
			[the_frame setTitle:@""];
			[the_frame setTitlePosition:NSNoTitle];

		}
	}
	
	
	
	
	ih->handle = the_frame;
	
	iupCocoaSetAssociatedViews(ih, the_frame, the_frame);

	iupCocoaAddToParent(ih);
	

	
	return IUP_NOERROR;
}


static void cocoaFrameUnMapMethod(Ihandle* ih)
{
	id the_frame = ih->handle;
	iupCocoaRemoveFromParent(ih);
	iupCocoaSetAssociatedViews(ih, nil, nil);
	[the_frame release];
	ih->handle = nil;
}

#if 0
/*
static void cocoaFrameComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
	(void)children_expand;
	
	*children_expand = 1;
	
	NSLog(@"cocoaFrameComputeNaturalSizeMethod: %d, %d, %d, %d", ih->currentwidth, ih->currentheight, *w, *h);
	
	NSBox* box_view = (NSBox*)ih->handle;
	NSView* box_content_view = [box_view contentView];
	
	CGFloat diff_width = NSWidth([box_view frame]) - NSWidth([box_content_view frame]);
	CGFloat diff_height = NSHeight([box_view frame]) - NSHeight([box_content_view frame]);
	
//	*w = *w - diff_width;
//	*h = *h - diff_height;

	*w = NSWidth([box_content_view frame]);
	*h = NSHeight([box_content_view frame]);

	
	//NSBox* the_box = ih->handle;
	//NSRect content_rect = [[the_box contentView] frame];
}


static void cocoaFrameLayoutUpdate(Ihandle* ih)
{

	NSLog(@"cocoaFrameLayoutUpdate: %d, %d", ih->currentwidth, ih->currentheight);
	
	NSBox* box_view = (NSBox*)ih->handle;
	NSView* box_content_view = [box_view contentView];
	
	CGFloat diff_width = NSWidth([box_view frame]) - NSWidth([box_content_view frame]);
	CGFloat diff_height = NSHeight([box_view frame]) - NSHeight([box_content_view frame]);
	
	//	*w = *w - diff_width;
	//	*h = *h - diff_height;

	
//	ih->currentwidth = ih->currentwidth - diff_width;
//	ih->currentheight = ih->currentheight - diff_height;
	iupdrvBaseLayoutUpdateMethod(ih);

}
*/
#endif


void iupdrvFrameInitClass(Iclass* ic)
{
	/* Driver Dependent Class functions */
	ic->Map = cocoaFrameMapMethod;
	ic->UnMap = cocoaFrameUnMapMethod;
	
	ic->GetInnerNativeContainerHandle = cocoaFrameGetInnerNativeContainerHandleMethod;

//	ic->ComputeNaturalSize = cocoaFrameComputeNaturalSizeMethod;
//	ic->LayoutUpdate = cocoaFrameLayoutUpdate;

#if 0
	
	/* Driver Dependent Attribute functions */
	
	/* Overwrite Common */
	iupClassRegisterAttribute(ic, "STANDARDFONT", NULL, gtkFrameSetStandardFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NO_SAVE|IUPAF_NOT_MAPPED);
	
	/* Visual */
	iupClassRegisterAttribute(ic, "BGCOLOR", iupFrameGetBgColorAttrib, gtkFrameSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
	iupClassRegisterAttribute(ic, "SUNKEN", NULL, gtkFrameSetSunkenAttrib, NULL, NULL, IUPAF_NO_INHERIT);
	
	/* Special */
	iupClassRegisterAttribute(ic, "FGCOLOR", NULL, gtkFrameSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
#endif
	iupClassRegisterAttribute(ic, "TITLE", NULL, cocoaFrameSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	
}
