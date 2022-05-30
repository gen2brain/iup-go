/** \file
 * \brief Clipboard for the GTK Driver.
 *
 * See Copyright Notice in "iup.h"
 */


#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#import <Cocoa/Cocoa.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"

#include "iupcocoa_drv.h"


// The only reason we have this class is  because ADDFORMAT requires us to carry state around.
// Cocoa doesn't have an ADD API, so we have to manually save/accumulate all the values.
@interface IupClipboardState : NSObject
@property(nonatomic, assign) Ihandle* ihandle;
@property(nonatomic, copy) NSArray* arrayOfTypes;
@end

@implementation IupClipboardState

- (void) dealloc
{
	[self setIhandle:NULL];
	[self setArrayOfTypes:nil];
	[super dealloc];
}

@end


static char* cocoaClipboardGetTextAvailableAttrib(Ihandle* ih)
{
	(void)ih;
	
	NSPasteboard* paste_board;
	NSString* type_available;

	paste_board = [NSPasteboard generalPasteboard];
	/* Ask if this kind of data is available on the paste board? */
	type_available = [paste_board availableTypeFromArray:[NSArray arrayWithObject:NSStringPboardType]];
	return iupStrReturnBoolean((int)type_available);

}

static int cocoaClipboardSetTextAttrib(Ihandle* ih, const char* value)
{
	NSPasteboard* paste_board;
	
	
	NSString* ns_string = nil;
	if(value)
	{
		// This will return nil if the string can't be converted.
		ns_string = [NSString stringWithUTF8String:value];
	}
	else
	{
		ns_string = @"";
	}
	
	paste_board = [NSPasteboard generalPasteboard];
	[paste_board declareTypes:[NSArray arrayWithObject:NSStringPboardType] owner:nil];
	
	/* Note: Assuming null terminated C string here. src_len is ignored. */
	[paste_board setString:ns_string forType:NSStringPboardType];
	
	return 0;
}

static char* cocoaClipboardGetTextAttrib(Ihandle* ih)
{
	NSPasteboard* paste_board;
	NSString* type_available;
	NSString* pasteboard_string = nil;

	paste_board = [NSPasteboard generalPasteboard];
	/* Ask if this kind of data is available on the paste board? */
	type_available = [paste_board availableTypeFromArray:[NSArray arrayWithObject:NSStringPboardType]];
	if(type_available)
//	if([type_available isEqualToString:NSStringPboardType])
	{
		/* Get the text string from the pasteboard */
		pasteboard_string = [paste_board stringForType:NSStringPboardType];
	}
	return iupStrReturnStr([pasteboard_string UTF8String]);
}


static char* cocoaClipboardGetImageAvailableAttrib(Ihandle* ih)
{
	(void)ih;
	
	NSPasteboard* paste_board;
	NSString* type_available;

	paste_board = [NSPasteboard generalPasteboard];
	/* Ask if this kind of data is available on the paste board? */
	type_available = [paste_board availableTypeFromArray:[NSArray arrayWithObjects:NSPasteboardTypeTIFF, NSPasteboardTypePNG, nil]];
	return iupStrReturnBoolean((int)type_available);

}

static int cocoaClipboardSetNativeImageAttrib(Ihandle* ih, const char* value)
{
	NSPasteboard* paste_board;
	paste_board = [NSPasteboard generalPasteboard];

	NSImage* ns_image = nil;
	if(value)
	{
		ns_image = (NSImage*)value;
		[paste_board writeObjects:@[ns_image]];
	}
	else
	{
		[paste_board clearContents];
	}
	return 0;
}

static char* cocoaClipboardGetNativeImageAttrib(Ihandle* ih)
{
	NSPasteboard* paste_board;
	paste_board = [NSPasteboard generalPasteboard];
	NSArray* valid_classes = @[[NSImage class]];
	NSDictionary* options_dict = @{};
	BOOL has_objects = [paste_board canReadObjectForClasses:valid_classes options:options_dict];
	if(has_objects)
	{
		NSArray* array_of_objects = [paste_board readObjectsForClasses:valid_classes options:options_dict];
		if([array_of_objects count] > 0)
		{
			NSImage* ns_image = (NSImage*)[array_of_objects firstObject];
			[ns_image retain]; // API says user needs to call release on this
			return (char*)ns_image;
		}
	}
	return NULL;
}

static int cocoaClipboardSetImageAttrib(Ihandle* ih, const char* value)
{
	NSPasteboard* paste_board = [NSPasteboard generalPasteboard];
	if(NULL == value)
	{
		[paste_board clearContents];
		return 0;
	}

	NSImage* ns_image = (NSImage*)iupImageGetImage(value, ih, 0, NULL);
	[paste_board writeObjects:@[ns_image]];
	return 0;
}

static int cocoaClipboardSetAddFormatAttrib(Ihandle* ih, const char* value)
{
	if(NULL == ih)
	{
		return 0;
	}
	if(NULL == value)
	{
		return 0;
	}
	
	NSString* new_type = [NSString stringWithUTF8String:value];
	
	IupClipboardState* iup_clipboard_state = ih->handle;
	NSArray* new_array = [[iup_clipboard_state arrayOfTypes] arrayByAddingObject:new_type];
	[iup_clipboard_state setArrayOfTypes:new_array];

	return 0;
}

static char* cocoaClipboardGetFormatAvailableAttrib(Ihandle* ih)
{
	NSPasteboard* paste_board;
	NSString* type_available;

	IupClipboardState* iup_clipboard_state = ih->handle;

	paste_board = [NSPasteboard generalPasteboard];
	/* Ask if this kind of data is available on the paste board? */
	type_available = [paste_board availableTypeFromArray:[iup_clipboard_state arrayOfTypes]];
	return iupStrReturnBoolean((int)type_available);
}


static int cocoaClipboardSetFormatDataAttrib(Ihandle* ih, const char* value)
{
	if(NULL == ih)
	{
		return 0;
	}
	NSPasteboard* paste_board = [NSPasteboard generalPasteboard];
	if(NULL == value)
	{
		[paste_board clearContents];
		return 0;
	}


	int data_size = iupAttribGetInt(ih, "FORMATDATASIZE");
	if(data_size <= 0)
	{
		return 0;
	}

	IupClipboardState* iup_clipboard_state = ih->handle;
	[paste_board declareTypes:[iup_clipboard_state arrayOfTypes] owner:nil];
	
	NSData* ns_data = [[NSData alloc] initWithBytes:value length:data_size];
	[ns_data autorelease];
	
	// I'm not sure how to use writeObjects in a way that preserves the type mapping.
	// So doing it the slow, legacy way.
	// Remember, this is copying the data now, instead of lazily, unlike the new way.
	for(NSString* a_type in [iup_clipboard_state arrayOfTypes])
	{
		[paste_board setData:ns_data forType:a_type];
	}

	return 0;
}

static char* cocoaClipboardGetFormatDataAttrib(Ihandle* ih)
{
	if(NULL == ih)
	{
		return 0;
	}
	NSPasteboard* paste_board = [NSPasteboard generalPasteboard];

	IupClipboardState* iup_clipboard_state = ih->handle;
	
	NSData* ns_data = nil;
	for(NSString* a_type in [iup_clipboard_state arrayOfTypes])
	{
		ns_data = [paste_board dataForType:a_type];
		if(nil != ns_data)
		{
			break;
		}
	}

	if(nil == ns_data)
	{
		iupAttribSetInt(ih, "FORMATDATASIZE", 0);
		return NULL;
	}


	NSUInteger data_size = [ns_data length];
	void* data_buffer = iupStrGetMemory((int)data_size);

	memcpy(data_buffer, [ns_data bytes], data_size);

	iupAttribSetInt(ih, "FORMATDATASIZE", (int)data_size);
	return data_buffer;
}

/*
static int cocoaClipboardSetClearAttrib(Ihandle* ih, const char* value)
{
	NSPasteboard* paste_board = [NSPasteboard generalPasteboard];
	[paste_board clearContents];
	return 0;
}
*/


/******************************************************************************/

Ihandle* IupClipboard(void)
{
  return IupCreate("clipboard");
}

// We need Create/Destroy because ADDFORMAT requires us to carry state around. Cocoa doesn't have an ADD API, so we have to manually save/accumulate all the values.
static int cocoaClipboardCreateMethod(Ihandle* ih, void** params)
{
	IupClipboardState* iup_clipboard_state = [[IupClipboardState alloc] init];
	
	ih->handle = iup_clipboard_state;
	[iup_clipboard_state setIhandle:ih];
	
	
	return IUP_NOERROR;
}

static void cocoaClipboardDestroyMethod(Ihandle* ih)
{
	IupClipboardState* iup_clipboard_state = ih->handle;
	[iup_clipboard_state release];
	ih->handle = nil;
}

Iclass* iupClipboardNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  // We need Create/Destroy because ADDFORMAT requires us to carry state around. Cocoa doesn't have an ADD API, so we have to manually save/accumulate all the values.
  ic->Create = cocoaClipboardCreateMethod;
  ic->Destroy = cocoaClipboardDestroyMethod;
  ic->name = "clipboard";
  ic->format = NULL;  /* no parameters */
  ic->nativetype = IUP_TYPECONTROL;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 0;

  ic->New = iupClipboardNewClass;

  /* Attribute functions */
  iupClassRegisterAttribute(ic, "TEXT", cocoaClipboardGetTextAttrib, cocoaClipboardSetTextAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TEXTAVAILABLE", cocoaClipboardGetTextAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "NATIVEIMAGE", cocoaClipboardGetNativeImageAttrib, cocoaClipboardSetNativeImageAttrib, NULL, NULL, IUPAF_NO_STRING|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, cocoaClipboardSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEAVAILABLE", cocoaClipboardGetImageAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ADDFORMAT", NULL, cocoaClipboardSetAddFormatAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMAT", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATAVAILABLE", cocoaClipboardGetFormatAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATA", cocoaClipboardGetFormatDataAttrib, cocoaClipboardSetFormatDataAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATASIZE", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

	// New API: Do we need this, or is it intended that value==NULL for above APIs invokes clear?
//	iupClassRegisterAttribute(ic, "CLEAR", NULL, cocoaClipboardSetClearAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  return ic;
}
