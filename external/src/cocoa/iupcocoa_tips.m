/** \file
 * \brief MAC Driver TIPS management
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>

#import <Cocoa/Cocoa.h>

#include "iup.h" 

#include "iup_object.h" 
#include "iup_str.h" 
#include "iup_attrib.h" 

#include "iupcocoa_drv.h"

int iupdrvBaseSetTipAttrib(Ihandle* ih, const char* value)
{
	id widget_handle = ih->handle;
	NSView* the_view = nil;
	
	if([widget_handle isKindOfClass:[NSViewController class]])
	{
		widget_handle = [(NSViewController*)widget_handle view];
		// fall through to next block
	}
	
	if([widget_handle respondsToSelector:@selector(setToolTip:)])
	{
		the_view = (NSView*)widget_handle;
		
		NSString* tip_string = [NSString stringWithUTF8String:value];
		[the_view setToolTip:tip_string];
	}
	return 1;
}

int iupdrvBaseSetTipVisibleAttrib(Ihandle* ih, const char* value)
{
	return 0;
}

char* iupdrvBaseGetTipVisibleAttrib(Ihandle* ih)
{
	return NULL;
}
