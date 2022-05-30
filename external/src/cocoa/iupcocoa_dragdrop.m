/** \file
 * \brief GTK Drag&Drop Functions
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

#include "iup_object.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_drv.h"
#include "iup_key.h"
#include "iup_image.h"

#include "iupcocoa_drv.h"
#import "iupcocoa_dragdrop.h"

// To avoid problems on pre-10.13, redefine the strings.
NSPasteboardType const kCompatNSPasteboardTypeFileURL = @"public.file-url";
NSPasteboardType const kCompatNSPasteboardTypeURL = @"public.url";


const void* IUPSOURCEDRAG_ASSOCIATED_OBJ_KEY = @"IUPSOURCEDRAG_ASSOCIATED_OBJ_KEY"; // the point of this is we have a unique memory address for an identifier
const void* IUPTARGETDROP_ASSOCIATED_OBJ_KEY = @"IUPTARGETDROP_ASSOCIATED_OBJ_KEY"; // the point of this is we have a unique memory address for an identifier


@implementation IupTargetDropAssociatedData

/*
- (instancetype) init
{
	self = [super init];
	if(nil != self)
	{
	
	}
	return self;
}
*/

- (void) dealloc
{
	[self setDropRegisteredTypes:nil];
	[self setIhandle:NULL];
	[self setDropTargetEnabled:false];
	[self setMainView:nil];
	[self setRootView:nil];
	[super dealloc];
}

/*
- (IupTargetDropAssociatedData*) iupTargetDropAssociatedData
{
	Ihandle* ih = [self ihandle];
	
	IupTargetDropAssociatedData* associated_data = (IupTargetDropAssociatedData*)objc_getAssociatedObject((id)ih->handle, IUPTARGETDROP_ASSOCIATED_OBJ_KEY);

	return associated_data;
}
*/
int cocoaTargetDropBasePerformDropCallback(Ihandle* ih, id<NSDraggingInfo> the_sender, NSPasteboard* paste_board, NSPoint drop_point)
{

//	NSPasteboard* paste_board = [the_sender draggingPasteboard];
//	NSPoint drop_point = [the_sender draggingLocation];
//	NSPoint converted_point = [self convertPoint:drop_point fromView:nil];

	IFnsViii drop_data_callback = (IFnsViii)IupGetCallback(ih, "DROPDATA_CB");
	if(NULL == drop_data_callback)
	{
		return 0;
	}

	IupTargetDropAssociatedData* associated_data = cocoaTargetDropGetAssociatedData(ih);
	NSArray* drop_types = [associated_data dropRegisteredTypes];

//	NSInteger number_of_files = [the_sender numberOfValidItemsForDrop];

//	NSString* best_type = [paste_board availableTypeFromArray:drop_types];


/* Classes in the provided array must implement the NSPasteboardReading protocol.
	Cocoa classes that implement this protocol include NSImage, NSString, NSURL, NSColor, NSAttributedString, and NSPasteboardItem.
	For every item on the pasteboard, each class in the provided array will be
	queried for the types it can read using -readableTypesForPasteboard:.
	An instance will be created of the first class found in the provided array
	whose readable types match a conforming type contained in that pasteboard item.
	Any instances that could be created from pasteboard item data is returned to the caller.
    Additional options, such as restricting the search to file URLs with particular content types,
    can be specified with an options dictionary.  See the comments for the option keys for a full description.
	Returns nil if there is an error in retrieving the requested items from the pasteboard
	or if no objects of the specified types can be created.

	Example: there are five items on the pasteboard, two contain TIFF data, two contain RTF data, one contains a private data type.  Calling -readObjectsForClasses: with just the NSImage class, will return an array containing two image objects.  Calling with just the NSAttributedString class, will return an array containing two attributed strings.  Calling with both classes will return two image objects and two attributed strings.  Note that in the above examples, the count of objects returned is less than the number of items on the pasteboard.  Only objects of the requested classes are returned.  You can always ensure to receive one object per item on the pasteboard by including the NSPasteboardItem class in the array of classes.  In this example, an array containing the NSImage, NSAttributedString, and NSSPasteboardItem classes will return an array with two images, two attributed strings, and one pasteboard item containing the private data type.
*/
	NSMutableArray* acceptable_classes = [NSMutableArray arrayWithCapacity:[drop_types count]];
//	NSArray* file_promise_types = [NSFilePromiseReceiver readableDraggedTypes];
	for(NSString* type_name in drop_types)
	{
	
		// Special case: Look for FILEPROMISE and replace with the real values.
		// UPDATE: I do not know how to make this work and I have not actually hit this case.
		// I am disabling for noww.
/*
		BOOL is_file_promise = [file_promise_types containsObject:type_name];
		if(is_file_promise)
		{
			// This case needs to come before the file-url case.
			[acceptable_classes addObject:[NSFilePromiseReceiver class]];
			continue;
		}
*/
		
		if([type_name isEqualToString:NSPasteboardTypeString])
		{
//			[acceptable_classes addObject:[NSAttributedString class]];
			[acceptable_classes addObject:[NSString class]];
		}
		else if([type_name isEqualToString:NSPasteboardTypeTIFF])
		{
			[acceptable_classes addObject:[NSImage class]];
		}
		else if([type_name isEqualToString:NSPasteboardTypePNG])
		{
			[acceptable_classes addObject:[NSImage class]];
		}
		else if([type_name isEqualToString:NSPasteboardTypeRTF])
		{
			[acceptable_classes addObject:[NSAttributedString class]];
//			[acceptable_classes addObject:[NSString class]];
		}
		else if([type_name isEqualToString:NSPasteboardTypeRTFD])
		{
			[acceptable_classes addObject:[NSAttributedString class]];
//			[acceptable_classes addObject:[NSString class]];
		}
		else if([type_name isEqualToString:NSPasteboardTypeHTML])
		{
			[acceptable_classes addObject:[NSAttributedString class]];
//			[acceptable_classes addObject:[NSString class]];
		}
		else if([type_name isEqualToString:NSPasteboardTypeTextFinderOptions])
		{
			[acceptable_classes addObject:[NSURL class]];
		}
		else if([type_name isEqualToString:kCompatNSPasteboardTypeURL])
		{
			[acceptable_classes addObject:[NSURL class]];
		}
		else if([type_name isEqualToString:kCompatNSPasteboardTypeFileURL])
		{
			[acceptable_classes addObject:[NSURL class]];
		}
		else
		{
			[acceptable_classes addObject:[NSPasteboardItem class]];
		}
	}
	
	// Grrr. There is a complication with choosing the "best" type.
	// I need to use NSPasteboardItem to handle custom types (and maybe fonts).
	// But if it comes in order before the built-in types,
	// the built-in types will be pre-empted and become a NSPasteboardItem.
	// For example, I have a custom type at the beginning (highest priority).
	// I drop a file-url onto the the target.
	// Instead of getting the [NSURL class], I get the NSPasteboardItem.
	// But if the item was at the end of the array, I would have gotten the [NSURL class].
	// This seems messier than it should be.

	/*
	NSPasteboardTypeString: public.utf8-plain-text
NSPasteboardTypePDF: com.adobe.pdf
NSPasteboardTypeTIFF: public.tiff
NSPasteboardTypePNG: public.png
NSPasteboardTypeRTF: public.rtf
NSPasteboardTypeRTFD: com.apple.flat-rtfd
NSPasteboardTypeHTML: public.html
NSPasteboardTypeTabularText: public.utf8-tab-separated-values-text
NSPasteboardTypeFont: com.apple.cocoa.pasteboard.character-formatting
NSPasteboardTypeRuler: com.apple.cocoa.pasteboard.paragraph-formatting
NSPasteboardTypeColor: com.apple.cocoa.pasteboard.color
NSPasteboardTypeSound: com.apple.cocoa.pasteboard.sound
NSPasteboardTypeMultipleTextSelection: com.apple.cocoa.pasteboard.multiple-text-selection
NSPasteboardTypeTextFinderOptions: com.apple.cocoa.pasteboard.find-panel-search-options
NSPasteboardTypeURL: public.url
NSPasteboardTypeFileURL: public.file-url
*/
//- (nullable NSArray *)readObjectsForClasses:(NSArray<Class> *)classArray options:(nullable NSDictionary<NSPasteboardReadingOptionKey, id> *)options NS_AVAILABLE_MAC(10_6);

	NSArray* acceptable_drop_items = [paste_board readObjectsForClasses:acceptable_classes options:nil];

	IFni drop_data_begin_callback = (IFni)IupGetCallback(ih, "DROPDATABEGIN_CB");
	if(NULL != drop_data_begin_callback)
	{
		drop_data_begin_callback(ih, (int)[acceptable_drop_items count]);
	}


	for(id drop_item in acceptable_drop_items)
	{
//		NSLog(@"drop_item:%@", drop_item);
		
		int ret_val = 0;
		
		if([drop_item isKindOfClass:[NSImage class]])
		{
			NSImage* ns_image = (NSImage*)drop_item;
			int w;
			int h;
			int bpp;
			iupdrvImageGetInfo(ns_image,&w, &h, &bpp);
			int bytes_per_row = iupCocoaImageCaluclateBytesPerRow(w, bpp/8);
			size_t buffer_size = bytes_per_row*h;
			unsigned char* img_data = malloc(buffer_size);
			iupdrvImageGetData(ns_image, img_data);
			char type_string[1024];
			snprintf(type_string, 1024, "IMAGE %d %d %d", w, h, bpp);
			ret_val = drop_data_callback(ih, type_string, (char*)img_data, (int)buffer_size, drop_point.x, drop_point.y);

			free(img_data);
		}
/*
		else if([drop_item isKindOfClass:[NSFilePromiseReceiver class]])
		{
			NSFilePromiseReceiver* file_promise_receiver = (NSFilePromiseReceiver*)drop_item;
			// WWDC 2018 Session 505 says to call this.
			// But I don't know what URL to use.
			// Example: I want to paste from our app to create/paste a file into the Finder.
			// I don't have sandbox permissions.
			// I don't think code works.
			// And I have not seen this case ever hit.
			NSURL* ns_url = [NSURL fileURLWithPath:@"MyFile"];
			[file_promise_receiver receivePromisedFilesAtDestination:ns_url options:@{} operationQueue:[NSOperationQueue mainQueue] reader:^(NSURL* file_url, NSError* error_or_nil)
				{
			
				}
			];
		}
*/
		else if([drop_item isKindOfClass:[NSURL class]])
		{
			NSURL* ns_url = (NSURL*)drop_item;
			
			if([ns_url isFileURL])
			{
				NSString* file_url = [ns_url path]; // still has file://
				const char* file_path = [file_url fileSystemRepresentation];
				size_t buffer_size = strlen(file_path) + 1;
				ret_val = drop_data_callback(ih, "FILE", (char*)file_path, (int)buffer_size, drop_point.x, drop_point.y);
			}
			else
			{
			
				NSString* ns_string = [ns_url absoluteString];
				const char* c_str = [ns_string UTF8String];
				size_t buffer_size = strlen(c_str) + 1;
				ret_val = drop_data_callback(ih, "URL", (char*)c_str, (int)buffer_size, drop_point.x, drop_point.y);
			}
		}
		else if([drop_item isKindOfClass:[NSString class]])
		{
			NSString* ns_string = (NSString*)drop_item;
			const char* c_str = [ns_string UTF8String];
			size_t buffer_size = strlen(c_str) + 1;
			ret_val = drop_data_callback(ih, "TEXT", (char*)c_str, (int)buffer_size, drop_point.x, drop_point.y);

		}
		else if([drop_item isKindOfClass:[NSAttributedString class]])
		{
			// Not supported.
		}
		else if([drop_item isKindOfClass:[NSColor class]])
		{
			NSColor* ns_color = (NSColor*)drop_item;
			NSColor* rgb_color = [ns_color colorUsingColorSpace:[NSColorSpace genericRGBColorSpace]];
			int r = iupROUND([rgb_color redComponent] * 255.0);
			int g = iupROUND([rgb_color greenComponent] * 255.0);
			int b = iupROUND([rgb_color blueComponent] * 255.0);
			int a = iupROUND([rgb_color alphaComponent] * 255.0);

			char color_string[20];
			snprintf(color_string, 20, "%d %d %d %d", r, g, b, a);

			size_t buffer_size = strlen(color_string) + 1;
			ret_val = drop_data_callback(ih, "COLOR", (char*)color_string, (int)buffer_size, drop_point.x, drop_point.y);
			
		}
 		// NSFont is not mentioned as one of the NSPasteboardReading classes, so this might never be called
 		else if([drop_item isKindOfClass:[NSFont class]])
		{
//			NSLog(@"got NSFont1");
			NSFont* ns_font = (NSFont*)drop_item;

			// FIXME: Does not include style
			NSString* ns_string = [NSString stringWithFormat:@"%@, %lf", [ns_font fontName], [ns_font pointSize]];
			const char* c_str = [ns_string UTF8String];
			size_t buffer_size = strlen(c_str) + 1;
			ret_val = drop_data_callback(ih, "FONT", (char*)c_str, (int)buffer_size, drop_point.x, drop_point.y);

		}
		else // NSPasteboardItem
		{
			NSPasteboardItem* pasteboard_item = (NSPasteboardItem*)drop_item;

			NSString* best_type = [pasteboard_item availableTypeFromArray:drop_types];
			if(nil == best_type)
			{
				continue;
			}
			NSData* ns_data = [pasteboard_item dataForType:best_type];
			if(nil == ns_data)
			{
				continue;
			}
			
			// Grrr. There is a complication with choosing the "best" type.
			// I need to use NSPasteboardItem to handle custom types (and maybe fonts).
			// But if it comes in order before the built-in types,
			// the built-in types will be pre-empted and become a NSPasteboardItem.
			// For example, I have a custom type at the beginning (highest priority).
			// I drop a file-url onto the the target.
			// Instead of getting the [NSURL class], I get the NSPasteboardItem.
			// But if the item was at the end of the array, I would have gotten the [NSURL class].
			// This seems messier than it should be.
			// So all the [Foo class] cases above are somewhat redundant with the below.
			// Maybe I should remove the above cases, but they have been mostly tested already.

			if([best_type isEqualToString:NSPasteboardTypePNG] || [best_type isEqualToString:NSPasteboardTypeTIFF])
			{
				NSImage* ns_image = [[[NSImage alloc] initWithData:ns_data] autorelease];
				
				int w;
				int h;
				int bpp;
				iupdrvImageGetInfo(ns_image,&w, &h, &bpp);
				int bytes_per_row = iupCocoaImageCaluclateBytesPerRow(w, bpp/8);
				size_t buffer_size = bytes_per_row*h;
				unsigned char* img_data = malloc(buffer_size);
				iupdrvImageGetData(ns_image, img_data);
				char type_string[1024];
				snprintf(type_string, 1024, "IMAGE %d %d %d", w, h, bpp);
				ret_val = drop_data_callback(ih, type_string, (char*)img_data, (int)buffer_size, drop_point.x, drop_point.y);

				free(img_data);
			}
			else if([best_type isEqualToString:kCompatNSPasteboardTypeFileURL])
			{
				NSString* ns_string = [[[NSString alloc] initWithData:ns_data encoding:NSUTF8StringEncoding] autorelease];
 				NSURL* ns_url = [[[NSURL alloc] initWithString:ns_string] autorelease];

				NSString* file_url = [ns_url path]; // still has file://
				const char* file_path = [file_url fileSystemRepresentation];
				size_t buffer_size = strlen(file_path) + 1;
				ret_val = drop_data_callback(ih, "FILE", (char*)file_path, (int)buffer_size, drop_point.x, drop_point.y);

			}
			else if([best_type isEqualToString:kCompatNSPasteboardTypeURL])
			{
				NSString* ns_string = [[[NSString alloc] initWithData:ns_data encoding:NSUTF8StringEncoding] autorelease];
 				NSURL* ns_url = [[[NSURL alloc] initWithString:ns_string] autorelease];

				NSString* ns_string2 = [ns_url absoluteString];
				const char* c_str = [ns_string2 UTF8String];
				size_t buffer_size = strlen(c_str) + 1;
				ret_val = drop_data_callback(ih, "URL", (char*)c_str, (int)buffer_size, drop_point.x, drop_point.y);
			}
			if([best_type isEqualToString:NSPasteboardTypeString]
				|| [best_type isEqualToString:NSPasteboardTypeHTML]
				|| [best_type isEqualToString:NSPasteboardTypeTabularText]
				|| [best_type isEqualToString:NSPasteboardTypeMultipleTextSelection]
			)
			{
				NSString* ns_string = [[[NSString alloc] initWithData:ns_data encoding:NSUTF8StringEncoding] autorelease];
				const char* c_str = [ns_string UTF8String];
				size_t buffer_size = strlen(c_str) + 1;
				ret_val = drop_data_callback(ih, "TEXT", (char*)c_str, (int)buffer_size, drop_point.x, drop_point.y);

			}
			else if([drop_item isKindOfClass:[NSColor class]])
			{
				NSColor* ns_color = (NSColor*)[NSUnarchiver unarchiveObjectWithData:ns_data];
				NSColor* rgb_color = [ns_color colorUsingColorSpace:[NSColorSpace genericRGBColorSpace]];
				int r = iupROUND([rgb_color redComponent] * 255.0);
				int g = iupROUND([rgb_color greenComponent] * 255.0);
				int b = iupROUND([rgb_color blueComponent] * 255.0);
				int a = iupROUND([rgb_color alphaComponent] * 255.0);

				char color_string[20];
				snprintf(color_string, 20, "%d %d %d %d", r, g, b, a);

				size_t buffer_size = strlen(color_string) + 1;
				ret_val = drop_data_callback(ih, "COLOR", (char*)color_string, (int)buffer_size, drop_point.x, drop_point.y);
				
			}
			if([best_type isEqualToString:NSPasteboardTypeFont])
			{
//				NSLog(@"got NSFont2");
				NSFont* ns_font = (NSFont*)[NSUnarchiver unarchiveObjectWithData:ns_data];

				// FIXME: Does not include style
				NSString* ns_string = [NSString stringWithFormat:@"%@, %lf", [ns_font fontName], [ns_font pointSize]];
				const char* c_str = [ns_string UTF8String];
				size_t buffer_size = strlen(c_str) + 1;
				ret_val = drop_data_callback(ih, "FONT", (char*)c_str, (int)buffer_size, drop_point.x, drop_point.y);

			}
			else
			{
				const void* data_buffer = [ns_data bytes];
				NSUInteger buffer_size = [ns_data length];
				const char* c_type_str = [best_type UTF8String];
				ret_val = drop_data_callback(ih, (char*)c_type_str, (char*)data_buffer, (int)buffer_size, drop_point.x, drop_point.y);
			}

		}
		
		
	}
	
	Icallback drop_data_end_callback = (Icallback)IupGetCallback(ih, "DROPDATAEND_CB");
	if(NULL != drop_data_end_callback)
	{
		drop_data_end_callback(ih);
	}

	
	// Should I auto-update, or require the user to call IupUpdate?
	// Only they know for sure if a redraw is actually needed.
	// [main_view setNeedsDisplay];
	
	return 0;
}

@end

@implementation IupSourceDragAssociatedData


- (instancetype) init
{
	self = [super init];
	if(nil != self)
	{
 		[self setDefaultFilePromiseName:@"IupDragDrop.iup"];
 		[self setUseAutoBeginDrag:true];
	}
	return self;
}


- (void) dealloc
{
	[self setIhandle:NULL];
	[self setDragSourceEnabled:false];
	[self setUseAutoBeginDrag:false];
	[self setDragRegisteredTypes:nil];
	[self setUseAutoGenerateDragImage:false];
	[self setDefaultFilePromiseName:nil];
	[self setMainView:nil];
	[self setRootView:nil];

	[super dealloc];
}

- (NSDragOperation) draggingSession:(NSDraggingSession*)dragging_session sourceOperationMaskForDraggingContext:(NSDraggingContext)dragging_context
{
    if(NSDraggingContextOutsideApplication == dragging_context)
    {
        return NSDragOperationCopy;
    }
	
    // NSDraggingContextWithinApplication
   	Ihandle* ih = [self ihandle];
   	// FIXME/TODO:
   	// 1. The user may need finer grain control on when to set copy or move
   	// (i.e. which widget is targeted, which key modifier is pressed).
	// 2. The user may need the other types
	// e.g. NSDragOperationLink, NSDragOperationNone
	
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


static void cocoaSourceDragProvideDataForTypeDefault(Ihandle* ih, NSPasteboard* paste_board, NSPasteboardItem* pasteboard_item, NSPasteboardType type_name, NSView* main_view, NSView* root_view)
{
		if([type_name isEqualToString:NSPasteboardTypeTIFF])
		{
			
			NSRect bounds_rect = [main_view bounds];
#if 1
			NSData* pdf_data = [main_view dataWithPDFInsideRect:bounds_rect];
			NSImage* image_data = [[NSImage alloc] initWithData:pdf_data];
			[image_data autorelease];
			NSData* tiff_data = [image_data TIFFRepresentation];
			[pasteboard_item setData:tiff_data forType:type_name];
#else
			// this doesn't work
			NSBitmapImageRep* bitmap_rep = [main_view bitmapImageRepForCachingDisplayInRect:bounds_rect];
			NSData* tiff_data = [bitmap_rep representationUsingType:NSBitmapImageFileTypeTIFF properties:@{}];
			[pasteboard_item setData:tiff_data forType:type_name];
#endif


//			NSLog(@"Type:%@, value:%@", type_name, pdf_data);

		}
		else if([type_name isEqualToString:NSPasteboardTypePNG])
		{
			NSRect bounds_rect = [main_view bounds];
			NSData* pdf_data = [main_view dataWithPDFInsideRect:bounds_rect];
			NSImage* image_data = [[NSImage alloc] initWithData:pdf_data];
			[image_data autorelease];
			NSData* tiff_data = [image_data TIFFRepresentation];
			NSBitmapImageRep* bitmap_rep = [[NSBitmapImageRep alloc] initWithData:tiff_data];
			[bitmap_rep autorelease];
			NSData* png_rep = [bitmap_rep representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
			
			[pasteboard_item setData:png_rep forType:type_name];
//			NSLog(@"Type:%@, value:%@", type_name, pdf_data);

		}
		else if([type_name isEqualToString:NSPasteboardTypeRTF])
		{
			if([main_view respondsToSelector:@selector(attributedStringValue)])
			{
				NSAttributedString* attr_str = [main_view attributedStringValue];
				NSData* ns_data = [NSKeyedArchiver archivedDataWithRootObject:attr_str];
				[pasteboard_item setData:ns_data forType:type_name];

//				NSLog(@"Type:%@, value:%@", type_name, attr_str);
			}
		}
		else if([type_name isEqualToString:NSPasteboardTypeRTFD])
		{
			if([main_view respondsToSelector:@selector(attributedStringValue)])
			{
				NSAttributedString* attr_str = [main_view attributedStringValue];
				NSData* ns_data = [NSKeyedArchiver archivedDataWithRootObject:attr_str];
				[pasteboard_item setData:ns_data forType:type_name];

//				NSLog(@"Type:%@, value:%@", type_name, attr_str);
			}
		}
		else if([type_name isEqualToString:NSPasteboardTypeHTML])
		{
		/*
			if([main_view respondsToSelector:@selector(attributedStringValue)])
			{
				NSAttributedString* attr_str = [main_view attributedStringValue];
				NSData* ns_data = [NSKeyedArchiver archivedDataWithRootObject:attr_str];
				[pasteboard_item setData:ns_data forType:type_name];

				NSLog(@"Type:%@, value:%@", type_name, attr_str);
			}
		*/
			if([main_view respondsToSelector:@selector(stringValue)])
			{
				NSString* ns_string = [main_view stringValue];
				[pasteboard_item setString:ns_string forType:type_name];
//				NSLog(@"Type:%@, value:%@", type_name, ns_string);
			}
		}
		// I don't understand what Apple intends this to be.
		else if([type_name isEqualToString:NSPasteboardTypeTextFinderOptions])
		{
#if 0
			if([self respondsToSelector:@selector(stringValue)])
			{
				NSString* ns_str = [main_view stringValue];
				NSURL* ns_url = [NSURL URLWithString:ns_str];
				NSData* ns_data = [NSData dataWithContentsOfURL:ns_url];
				[pasteboard_item setData:ns_data forType:type_name];

//				NSLog(@"Type:%@, value:%@", type_name, ns_str);
			}
#else
			if([main_view respondsToSelector:@selector(stringValue)])
			{
				NSString* ns_string = [main_view stringValue];
				[pasteboard_item setString:ns_string forType:type_name];
//				NSLog(@"Type:%@, value:%@", type_name, ns_string);
			}
#endif

		}
		else if([type_name isEqualToString:kCompatNSPasteboardTypeFileURL])
		{
			// This one is extra tricky.
			// If the user wants to provide a file path, then we get to do the easy thing.
			// But if the user has data (like an image), and wants to automatically create a file from it,
			// we have to use the NSFilePromise APIs and require the user to deal with the associated callbacks for that.
			// But we need to be able to distinguish which case the user wants.
		
			if([main_view respondsToSelector:@selector(stringValue)])
			{
				NSString* ns_str = [main_view stringValue];
				NSURL* ns_url = [NSURL URLWithString:ns_str];
				NSData* ns_data = [NSData dataWithContentsOfURL:ns_url];
				[pasteboard_item setData:ns_data forType:type_name];

//				NSLog(@"Type:%@, value:%@", type_name, ns_str);
			}
		}
		else if([type_name isEqualToString:NSPasteboardTypeURL])
		{
			if([main_view respondsToSelector:@selector(stringValue)])
			{
				NSString* ns_str = [main_view stringValue];
				NSURL* ns_url = [NSURL URLWithString:ns_str];
				NSData* ns_data = [NSData dataWithContentsOfURL:ns_url];
				[pasteboard_item setData:ns_data forType:type_name];

//				NSLog(@"Type:%@, value:%@", type_name, ns_str);
			}

			
		}

		else if([type_name isEqualToString:NSPasteboardTypeTabularText])
		{

			if([main_view respondsToSelector:@selector(stringValue)])
			{
				NSString* ns_string = [main_view stringValue];
				[pasteboard_item setString:ns_string forType:type_name];
//				NSLog(@"Type:%@, value:%@", type_name, ns_string);
			}
		}
		else if([type_name isEqualToString:NSPasteboardTypeString])
		{
			if([main_view respondsToSelector:@selector(stringValue)])
			{
				NSString* ns_string = [main_view stringValue];
				[pasteboard_item setString:ns_string forType:type_name];
//				NSLog(@"Type:%@, value:%@", type_name, ns_string);
			}
		}
	
		else if([type_name isEqualToString:NSPasteboardTypePDF])
		{
			NSRect bounds_rect = [main_view bounds];
			NSData* pdf_data = [main_view dataWithPDFInsideRect:bounds_rect];
			[pasteboard_item setData:pdf_data forType:type_name];
//			NSLog(@"Type:%@, value:%@", type_name, pdf_data);
		}
		else if([type_name isEqualToString:NSPasteboardTypeFont])
		{
			//
			NSData* ns_data = [pasteboard_item dataForType:type_name];
//			NSLog(@"Type:%@, value:%@", type_name, ns_data);
		}
		else if([type_name isEqualToString:NSPasteboardTypeRuler])
		{
			if([main_view respondsToSelector:@selector(attributedStringValue)])
			{
				NSAttributedString* attr_str = [main_view attributedStringValue];
				NSData* ns_data = [NSKeyedArchiver archivedDataWithRootObject:attr_str];
				[pasteboard_item setData:ns_data forType:type_name];

//				NSLog(@"Type:%@, value:%@", type_name, attr_str);
			}
		}
		else if([type_name isEqualToString:NSPasteboardTypeColor])
		{
			if([main_view respondsToSelector:@selector(color)])
			{
				NSColor* the_color = [main_view color];
				NSData* ns_data = [NSKeyedArchiver archivedDataWithRootObject:the_color];
				[pasteboard_item setData:ns_data forType:type_name];

//				NSLog(@"Type:%@, value:%@", type_name, the_color);
			}
			else if([main_view respondsToSelector:@selector(foregroundColor)])
			{
				NSColor* the_color = [main_view foregroundColor];
				NSData* ns_data = [NSKeyedArchiver archivedDataWithRootObject:the_color];
				[pasteboard_item setData:ns_data forType:type_name];

//				NSLog(@"Type:%@, value:%@", type_name, the_color);
			}
			else if([main_view respondsToSelector:@selector(backgroundColor)])
			{
				NSColor* the_color = [main_view backgroundColor];
				NSData* ns_data = [NSKeyedArchiver archivedDataWithRootObject:the_color];
				[pasteboard_item setData:ns_data forType:type_name];

//				NSLog(@"Type:%@, value:%@", type_name, the_color);
			}
		}
		else if([type_name isEqualToString:NSPasteboardTypeSound])
		{

			NSData* ns_data = [pasteboard_item dataForType:type_name];
//			NSLog(@"Type:%@, value:%@", type_name, ns_data);
		}
		else if([type_name isEqualToString:NSPasteboardTypeMultipleTextSelection])
		{
			if([main_view respondsToSelector:@selector(stringValue)])
			{
				NSString* ns_string = [main_view stringValue];
				[pasteboard_item setString:ns_string forType:type_name];
//				NSLog(@"Type:%@, value:%@", type_name, ns_string);
			}
		}

		else
		{
//			NSData* ns_data = [pasteboard_item dataForType:type_name];
//			NSLog(@"Type:%@, value:%@", type_name, ns_data);
		}

}

static void cocoaSourceDragProvideDataForTypeUser(Ihandle* ih, NSPasteboard* paste_board, NSPasteboardItem* pasteboard_item, NSPasteboardType type_name, NSView* main_view, NSView* root_view, void* data_buffer, int data_size)
{

		if([type_name isEqualToString:NSPasteboardTypeTIFF])
		{
			// I'm not thrilled with this, but our options aren't great.

			// In order to construct a usable image, I need not only the pixel data,
			// but also the width, height, and bit depth.
			// I would have to invent a new format for the user to provide this information in the buffer, which I don't want to do.

			// The more robust thing to do is just accept an IupImage which has all this information.
			// However, the Iup callback interface isn't well designed for this.
			// So this means the user will need to convert their IupFont Ihandle* to an intptr_t,
			// write the intptr_t value into the buffer (using sizeof(intptr_t)),
			// and we have to convert it back to a pointer here.
			// This will work fine as C99 allows this sort of thing if the platform provides intptr_t.
			// But those binding from other languages will have to pay special attention here.


			// Late Entry: Case 1: This is what IupClipboard does. It tries the associated string name for the loaded image.
			bool found_image = false;
			NSString* ns_string = [[[NSString alloc] initWithBytes:data_buffer length:data_size encoding:NSUTF8StringEncoding] autorelease];
			if(nil != ns_string)
			{
				// This is an Iup mapped name, not a file name. So we use UTF8String, not fileSystemRepresentation
				const char* c_string = [ns_string UTF8String];
				NSImage* ns_image = (NSImage*)iupImageGetImage(c_string, ih, 0, NULL);
				NSBitmapImageRep* bitmap_image = nil;
				if([[ns_image representations] count] > 0)
				{
					bitmap_image = (NSBitmapImageRep*)[[ns_image representations] objectAtIndex:0];
				}

				if(nil != bitmap_image)
				{
					found_image = true;
					
					NSData* ns_data_image = [bitmap_image TIFFRepresentation];
					[pasteboard_item setData:ns_data_image forType:type_name];
				}
			}

			if(!found_image)
			{
			
				// Because of the IupClipboard 'name' technique, I don't need the IupImage intptr_t hack.
				// There are things I still kind of like about it, but because the string length above
				// could be the same as sizeof(intptr_t),
				// if the name resolution fails for some reason,
				// we end up in this block and crash on the IupGetInt(iup_image, "WIDTH").
				// So it is safer to remove this hack.
/*
				if(data_size == sizeof(intptr_t))
				{
					intptr_t int_ptr_for_iupimage = 0;
					memcpy(&int_ptr_for_iupimage, data_buffer, sizeof(intptr_t));
					//NSLog(@"int_ptr_for_iupfont: %zu\n", int_ptr_for_iupfont);
					Ihandle* iup_image = (Ihandle*)int_ptr_for_iupimage;
					
					int height = IupGetInt(iup_image, "WIDTH");
					int width = IupGetInt(iup_image, "HEIGHT");
					int bpp = IupGetInt(iup_image, "BPP");
					const char* pixel_data = IupGetAttribute(iup_image, "WID");

					NSBitmapImageRep* bitmap_image = iupCocoaImageNSBitmapImageRepFromPixels(width, height, bpp, NULL, 0, (unsigned char*)pixel_data);
					NSData* ns_data_image = [bitmap_image TIFFRepresentation];
					[pasteboard_item setData:ns_data_image forType:type_name];
				}
				else
				{
*/
					// Assume the user has provided a proper TIFF buffer
					NSData* ns_data = [NSData dataWithBytes:data_buffer length:data_size];
					[pasteboard_item setData:ns_data forType:type_name];
/*
				}
*/
			}

		}
		else if([type_name isEqualToString:NSPasteboardTypePNG])
		{
			// I'm not thrilled with this, but our options aren't great.

			// In order to construct a usable image, I need not only the pixel data,
			// but also the width, height, and bit depth.
			// I would have to invent a new format for the user to provide this information in the buffer, which I don't want to do.

			// The more robust thing to do is just accept an IupImage which has all this information.
			// However, the Iup callback interface isn't well designed for this.
			// So this means the user will need to convert their IupFont Ihandle* to an intptr_t,
			// write the intptr_t value into the buffer (using sizeof(intptr_t)),
			// and we have to convert it back to a pointer here.
			// This will work fine as C99 allows this sort of thing if the platform provides intptr_t.
			// But those binding from other languages will have to pay special attention here.
			
			// Late Entry: Case 1: This is what IupClipboard does. It tries the associated string name for the loaded image.
			bool found_image = false;
			NSString* ns_string = [[[NSString alloc] initWithBytes:data_buffer length:data_size encoding:NSUTF8StringEncoding] autorelease];
			if(nil != ns_string)
			{
				// This is an Iup mapped name, not a file name. So we use UTF8String, not fileSystemRepresentation
				const char* c_string = [ns_string UTF8String];
				NSImage* ns_image = (NSImage*)iupImageGetImage(c_string, ih, 0, NULL);
				NSBitmapImageRep* bitmap_image = nil;
				if([[ns_image representations] count] > 0)
				{
					bitmap_image = (NSBitmapImageRep*)[[ns_image representations] objectAtIndex:0];
				}

				if(nil != bitmap_image)
				{
					found_image = true;
					
					NSData* ns_data_image = [bitmap_image representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
					[pasteboard_item setData:ns_data_image forType:type_name];
				}
			}

			if(!found_image)
			{
				// Because of the IupClipboard 'name' technique, I don't need the IupImage intptr_t hack.
				// There are things I still kind of like about it, but because the string length above
				// could be the same as sizeof(intptr_t),
				// if the name resolution fails for some reason,
				// we end up in this block and crash on the IupGetInt(iup_image, "WIDTH").
				// So it is safer to remove this hack.
/*
				if(data_size == sizeof(intptr_t))
				{
					intptr_t int_ptr_for_iupimage = 0;
					memcpy(&int_ptr_for_iupimage, data_buffer, sizeof(intptr_t));
					//NSLog(@"int_ptr_for_iupfont: %zu\n", int_ptr_for_iupfont);
					Ihandle* iup_image = (Ihandle*)int_ptr_for_iupimage;
					
					int height = IupGetInt(iup_image, "WIDTH");
					int width = IupGetInt(iup_image, "HEIGHT");
					int bpp = IupGetInt(iup_image, "BPP");
					const char* pixel_data = IupGetAttribute(iup_image, "WID");

					NSBitmapImageRep* bitmap_image = iupCocoaImageNSBitmapImageRepFromPixels(width, height, bpp, NULL, 0, (unsigned char*)pixel_data);
					NSData* ns_data_image = [bitmap_image representationUsingType:NSBitmapImageFileTypePNG properties:@{}];

					[pasteboard_item setData:ns_data_image forType:type_name];
				}
				else
				{
*/
					// Assume the user has provided a proper PNG buffer
					NSData* ns_data = [NSData dataWithBytes:data_buffer length:data_size];
					[pasteboard_item setData:ns_data forType:type_name];
/*
				}
*/
			}

		}
		else if([type_name isEqualToString:NSPasteboardTypeRTF])
		{
			// I don't what to do
			NSData* ns_data = [NSData dataWithBytes:data_buffer length:data_size];
			[pasteboard_item setData:ns_data forType:type_name];
		}
		else if([type_name isEqualToString:NSPasteboardTypeRTFD])
		{
			// I don't what to do
			NSData* ns_data = [NSData dataWithBytes:data_buffer length:data_size];
			[pasteboard_item setData:ns_data forType:type_name];
		}
		else if([type_name isEqualToString:NSPasteboardTypeHTML])
		{
			// I'm guessing the user can enter plain text HTML data (including tags)
			NSString* ns_string = [[NSString alloc] initWithBytes:data_buffer length:data_size encoding:NSUTF8StringEncoding];
			[ns_string autorelease];
			[pasteboard_item setString:ns_string forType:type_name];
		}
		// I don't understand what Apple intends this to be.
		else if([type_name isEqualToString:NSPasteboardTypeTextFinderOptions])
		{
			NSString* ns_string = [[NSString alloc] initWithBytes:data_buffer length:data_size encoding:NSUTF8StringEncoding];
			[ns_string autorelease];
			[pasteboard_item setString:ns_string forType:type_name];
		}
		else if([type_name isEqualToString:kCompatNSPasteboardTypeFileURL])
		{
			// This one is extra tricky.
			// If the user wants to provide a file path, then we get to do the easy thing.
			// But if the user has data (like an image), and wants to automatically create a file from it,
			// we have to use the NSFilePromise APIs and require the user to deal with the associated callbacks for that.
			// But we need to be able to distinguish which case the user wants.
		
			NSString* ns_string = [[NSString alloc] initWithBytes:data_buffer length:data_size encoding:NSUTF8StringEncoding];
			[ns_string autorelease];
			NSURL* ns_url = [NSURL fileURLWithPath:ns_string];
			NSData* ns_data = [NSData dataWithContentsOfURL:ns_url];
			[pasteboard_item setData:ns_data forType:type_name];

		}
		else if([type_name isEqualToString:kCompatNSPasteboardTypeURL])
		{
			NSString* ns_string = [[NSString alloc] initWithBytes:data_buffer length:data_size encoding:NSUTF8StringEncoding];
			[ns_string autorelease];
			NSURL* ns_url = [NSURL URLWithString:ns_string];
			NSData* ns_data = [NSData dataWithContentsOfURL:ns_url];
			[pasteboard_item setData:ns_data forType:type_name];
		}

		else if([type_name isEqualToString:NSPasteboardTypeTabularText])
		{
			NSString* ns_string = [[NSString alloc] initWithBytes:data_buffer length:data_size encoding:NSUTF8StringEncoding];
			[ns_string autorelease];
			[pasteboard_item setString:ns_string forType:type_name];
		}
		else if([type_name isEqualToString:NSPasteboardTypeString])
		{
			NSString* ns_string = [[NSString alloc] initWithBytes:data_buffer length:data_size encoding:NSUTF8StringEncoding];
			[ns_string autorelease];
			[pasteboard_item setString:ns_string forType:type_name];
		}
	
		else if([type_name isEqualToString:NSPasteboardTypePDF])
		{
			// I don't know what to do. Trust that the user gave me a valid PDF, and that Apple knows what to do with it when put in an NSData?
			NSData* ns_data = [NSData dataWithBytes:data_buffer length:data_size];
			[pasteboard_item setData:ns_data forType:type_name];
		}
		else if([type_name isEqualToString:NSPasteboardTypeFont])
		{
			// I'm not thrilled with this, but our options aren't great.
			// Should we support taking an IupFont pointer (check for sizeof pointer)?
			// Or do we require a string description?
			
			// I can try to parse a Iup font string, but at the moment of this writing,
			// the Iup font string format is pretty lose as each platform can define it slightly differently,
			// and I haven't really decided if I need to invent a new variation for Cocoa.
			// The more robust thing to do is just accept an IupFont, so if we change the string format,
			// this code doesn't need to care.
			// However, the Iup callback interface isn't well designed for this.
			// So this means the user will need to convert their IupFont Ihandle* to an intptr_t,
			// write the intptr_t value into the buffer (using sizeof(intptr_t)),
			// and we have to convert it back to a pointer here.
			// This will work fine as C99 allows this sort of thing if the platform provides intptr_t.
			// But those binding from other languages will have to pay special attention here.
			
			if(data_size >= sizeof(intptr_t))
			{
				intptr_t int_ptr_for_iupfont = 0;
				memcpy(&int_ptr_for_iupfont, data_buffer, sizeof(intptr_t));
				//NSLog(@"int_ptr_for_iupfont: %zu\n", int_ptr_for_iupfont);
				Ihandle* iup_font = (Ihandle*)int_ptr_for_iupfont;
				
				// Get the internal Cocoa representation backing the iup_font
				IupCocoaFont* iup_cocoa_font = iupCocoaGetFont(iup_font);
				// Get the native Cocoa representation backing the IupCocoaFont
				NSFont* cocoa_font = [iup_cocoa_font nativeFont];
				NSData* ns_data = [NSKeyedArchiver archivedDataWithRootObject:cocoa_font];
				[pasteboard_item setData:ns_data forType:type_name];
			}
		}
		else if([type_name isEqualToString:NSPasteboardTypeRuler])
		{
			// I don't know what to do. Maybe somehow integrate with IupText???
			NSData* ns_data = [NSData dataWithBytes:data_buffer length:data_size];
			[pasteboard_item setData:ns_data forType:type_name];
		}
		else if([type_name isEqualToString:NSPasteboardTypeColor])
		{
			unsigned char r = 0;
			unsigned char g = 0;
			unsigned char b = 0;
			unsigned char a = 0;

			int is_valid = iupStrToRGBA(data_buffer, &r, &g, &b, &a);
			if(1 != is_valid)
			{
				is_valid = iupStrToRGB(data_buffer, &r, &g, &b);
				a = 255;
			}
			if(is_valid)
			{
				NSColor* the_color = [NSColor colorWithCalibratedRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:a/255.0];
				NSData* ns_data = [NSKeyedArchiver archivedDataWithRootObject:the_color];
				[pasteboard_item setData:ns_data forType:type_name];
			}
		}
		else if([type_name isEqualToString:NSPasteboardTypeSound])
		{
			// No idea. NSSound doesn't provide APIs for raw PCM data.
			NSData* ns_data = [NSData dataWithBytes:data_buffer length:data_size];
			[pasteboard_item setData:ns_data forType:type_name];
		}
		else if([type_name isEqualToString:NSPasteboardTypeMultipleTextSelection])
		{
			NSString* ns_string = [[NSString alloc] initWithBytes:data_buffer length:data_size encoding:NSUTF8StringEncoding];
			[ns_string autorelease];
			[pasteboard_item setString:ns_string forType:type_name];
		}

		else
		{
			NSData* ns_data = [NSData dataWithBytes:data_buffer length:data_size];
			[pasteboard_item setData:ns_data forType:type_name];
		}

}

- (bool) usesFilePromise
{
	Ihandle* ih = [self ihandle];
	Icallback file_promise_name_callback = IupGetCallback(ih, "DRAGFILECREATENAME_CB");
	if(nil != file_promise_name_callback)
	{
		return true;
	}

	// Check to see if the file promise types are in the registered list.
	NSArray* registered_types = [self dragRegisteredTypes];
	NSArray* file_promise_types = [NSFilePromiseReceiver readableDraggedTypes];

	NSSet* file_promise_set = [NSSet setWithArray:file_promise_types];
	for(NSString* a_type in registered_types)
	{
		if([file_promise_set containsObject:a_type])
		{
			return true;
		}
	}
	return false;
}

- (bool) hasFilePromiseCallback
{
	Ihandle* ih = [self ihandle];
	Icallback file_promise_callback = IupGetCallback(ih, "DRAGFILECREATE_CB");
	return(file_promise_callback);
}


- (void) pasteboard:(nullable NSPasteboard*)paste_board item:(NSPasteboardItem*)pasteboard_item provideDataForType:(NSPasteboardType)type_name
{
//	NSLog(@"%@, %@, %@", NSStringFromSelector(_cmd), pasteboard_item, type_name);

	// Because Cocoa
	// 1. supports so many types
	// 2. users expect these things to work
	// 3. Iup lacks equivalent 1-to-1 easily mappable data types to the native Cocoa types
	// 4. It is extra work to try to handle all these types manually
	// I am trying to supply a way to automatically handle common cases without user intervention.
	// So I am going to look at both
	// 1. whether the user defined a callback
	// 2. What return value did they hand back.
	// Depending on these factors, I will call a default routine.
	// Otherwise, I will try to use their data, but they must supply me data in a particular way depending on the type.
	Ihandle* ih = [self ihandle];



	// BUG: NSPasteboard copy may invoke pastebord:provideDataForType: on Quit/terminate: if there is lazy data in the clipboard.
	// But Apple seems to invoke this after this method returns.
	// But by that point, we have already shutdown IUP and freed a lot of objects, which may lead to a crash in pasteboard:provideDataForType:
	// Unfortunately, I don't know how to fix this.
	// We have two options:
	// 1) Clear out all pasteboard pointers when we tear down everything. But this will prevent the final copy of data being sent to the pasteboard before quit, so the data will not be pastable after this.
	// 2) Don't call IupClose() in the IupExit callback.
	// I'm starting to think in the loop redesign,
	// we should not call IupClose and maybe the platform takes responsbility for calling it.
	// In the Mac/Cocoa case, I don't think it is necessary to call IupClose at all which might solve this problem.
	// For now, IupClose is assumed and there is explicit clearing.
	// The delegate on the NSPasteboardItem is still set because I don't have access to it.
	// But assuming we don't crash and still reach here, bail out if ih is NULL.
	if(nil == ih)
	{
//		NSLog(@"Warnign: ih is NULL in %@", NSStringFromSelector(_cmd));
		return;
	}
	
	bool use_default_implementation = true;
	IFnsVi drag_data_callback = (IFnsVi)IupGetCallback(ih, "DRAGDATA_CB");
	IFns drag_data_size_callback = (IFns)IupGetCallback(ih, "DRAGDATASIZE_CB");
	void* data_buffer = NULL;
	if(drag_data_callback && drag_data_size_callback)
	{
		const char* c_type_name = [type_name UTF8String];
		int data_size = drag_data_size_callback(ih, (char*)c_type_name);
	    if(data_size > 0)
	    {
			data_buffer = malloc(data_size);
			// fill data
			int ret_val = drag_data_callback(ih, (char*)c_type_name, data_buffer, data_size);
			/*
			Which value should we use to represent what?
			- For legacy code, most people will probably be returning IUP_DEFAULT already.
			- Do we assume if they implemented this, this will do the correct thing?
			- If so, then IUP_DEFAULT should mean use the user's implementation (not the built-in-automatic-default-implementation.
			- So for the built-in-automatic-default-implementation, that suggests IUP_IGNORE or IUP_CONTINUE.
			- Do I want to reserve IUP_IGNORE to say skip entirely (i.e. don't do anything)?
				- It seems like they should avoid registering the data type instead of relying on this,
				- but maybe there is an edge case???
			- So if we reserve that, then IUP_CONTINUE would be the logicial remainder. (I don't think IUP_CLOSE makes sense.)
			*/
			
			if(IUP_IGNORE == ret_val)
			{
				free(data_buffer);
				data_buffer = NULL;
			}
			else if(IUP_CONTINUE == ret_val)
			{
				free(data_buffer);
				data_buffer = NULL;
				use_default_implementation = true;
			}
			else
			{
				use_default_implementation = false;
			}
			
			
			if(!use_default_implementation)
			{
				NSView* main_view = [self mainView];
				NSView* root_view = [self rootView];
				cocoaSourceDragProvideDataForTypeUser(ih, paste_board, pasteboard_item, type_name, main_view, root_view, data_buffer, data_size);

				IFns drag_data_end_callback = (IFns)IupGetCallback(ih, "DRAGDATAEND_CB");
				if(NULL != drag_data_end_callback)
				{
					drag_data_end_callback(ih, (char*)c_type_name);
				}

				free(data_buffer);
				data_buffer = NULL;
				return;
			}
			else
			{
				NSView* main_view = [self mainView];
				NSView* root_view = [self rootView];
				cocoaSourceDragProvideDataForTypeDefault(ih, paste_board, pasteboard_item, type_name, main_view, root_view);
	
	
				IFns drag_data_end_callback = (IFns)IupGetCallback(ih, "DRAGDATAEND_CB");
				if(NULL != drag_data_end_callback)
				{
					drag_data_end_callback(ih, (char*)c_type_name);
				}

				return;
			}

		}
		else
		{
			// Special case for file promise
			//IFnsVi drag_data_callback = (IFnsVi)IupGetCallback(ih, "DRAGFILECREATE_CB");

		}
		
	}
	

	NSView* main_view = [self mainView];
	NSView* root_view = [self rootView];
	cocoaSourceDragProvideDataForTypeDefault(ih, paste_board, pasteboard_item, type_name, main_view, root_view);
	

}



- (NSPasteboardItem*) defaultPasteboardItem
{
	IupSourceDragAssociatedData* drag_source_data = self;

	NSArray* registered_types = [drag_source_data dragRegisteredTypes];
	// Because copy/paste & drag/drop are interconnected, we want to allow this to work for copy/paste even when drag is disabled
//	if([drag_source_data isDragSourceEnabled] && ([registered_types count] > 0))
	if([registered_types count] > 0)
	{

		
		
		
	//		DRAGDATA_CB: request for drag data from source. It is called when the data is dropped.
//int function(Ihandle* ih, char* type, void* data, int size) [in C]

		// If not DRAGDATA_CB, try to do automatically?
		// Else if DRAGDATA_CB, then
		//	text, let the user fill it out
		// 		if an image type, do automatically, or force them to provide RGBA? or IupImage?
		



		// We are using the new APIs
		// https://stackoverflow.com/questions/7243668/nspasteboard-and-simple-custom-data
		// The downside is that in the drop callback, we can't get the specific original type.
		// But the major upside is that it allows us to specify multiple possible types for the source,
		// so the user can drag it onto different things.
//			[pasteboard_item setDataProvider:drag_source_data forTypes:@[NSPasteboardTypeTIFF, NSPasteboardTypePNG, NSPasteboardTypeFileURL]];


		// I need to use NSFilePromiseProvider to allow generating a file from memory.
		// Example: I drag an image from Safari to the desktop and it writes out a file.
		// In this example, the type was a file URL. But there is no actual file URL since the image is loaded in memory from a website.
		// So using NSFilePromiseProvider, we can get an addition series of callbacks to write a file when needed.
	
		bool wants_file_promise = [drag_source_data usesFilePromise];
		//NSDraggingItem* dragging_item = nil;
		id return_item = nil;
		if(wants_file_promise)
		{
			//		NSFilePromiseProvider* promise_provider = [[NSFilePromiseProvider alloc] initWithFileType:kUTTypePNG delegate:self];
			NSFilePromiseProvider* promise_provider = [[NSFilePromiseProvider alloc] initWithFileType:(NSString*)kUTTypeData delegate:self];
			[promise_provider autorelease];
//			dragging_item = [[NSDraggingItem alloc] initWithPasteboardWriter:promise_provider];
//			[dragging_item autorelease];
			// [promise_provider setDataProvider:drag_source_data forTypes:@[NSPasteboardTypeTIFF, NSPasteboardTypePNG, NSPasteboardTypeFileURL]];
			// I'm a little worried about this, since NSFilePromiseProvider is not a NSPasteboardItem.
			// But it seems to work, and I can't figure any other way of doing this.
			[promise_provider setDataProvider:drag_source_data forTypes:registered_types];
			return_item = promise_provider;
		}
		else
		{
			NSPasteboardItem* pasteboard_item = [[NSPasteboardItem alloc] init];
			[pasteboard_item autorelease];
			[pasteboard_item setDataProvider:drag_source_data forTypes:registered_types];
//			dragging_item = [[NSDraggingItem alloc] initWithPasteboardWriter:pasteboard_item];
//			[dragging_item autorelease];
			return_item = pasteboard_item;

		}


		
		// [self beginDraggingSessionWithItems:@[dragging_item, dragging_promise_item] event:the_event source:self];

		return return_item;

	}
	return nil;
}

- (NSDraggingItem*) defaultDraggingItem
{
	IupSourceDragAssociatedData* drag_source_data = self;
	NSView* main_view = [self mainView];

	NSDraggingItem* dragging_item = nil;
	NSPasteboardItem* pasteboard_item = [self defaultPasteboardItem];

	if(nil != pasteboard_item)
	{
		dragging_item = [[NSDraggingItem alloc] initWithPasteboardWriter:pasteboard_item];
		[dragging_item autorelease];


		if([drag_source_data useAutoGenerateDragImage])
		{
			NSRect bounds_rect = [main_view bounds];
			NSData* pdf_data = [main_view dataWithPDFInsideRect:bounds_rect];
			NSImage* image_data = [[NSImage alloc] initWithData:pdf_data];
			[image_data autorelease];
			
			//[dragging_item setDraggingFrame:bounds_rect contents:image_data];
			[dragging_item setDraggingFrame:bounds_rect contents:image_data];
		}
		
		// [self beginDraggingSessionWithItems:@[dragging_item, dragging_promise_item] event:the_event source:self];

		return dragging_item;

	}
	return nil;
}

- (NSString *) filePromiseProvider:(NSFilePromiseProvider*)file_promise_provider fileNameForType:(NSString*)file_type
{
	Ihandle* ih = [self ihandle];
	IFnssi file_promise_name_callback = (IFnssi)IupGetCallback(ih, "DRAGFILECREATENAME_CB");
	if(NULL == file_promise_name_callback)
	{
		NSString* default_file_name = [self defaultFilePromiseName];
		return default_file_name;
	}
	
	char file_buffer[PATH_MAX] = { '\0' };
	const char* c_file_type = [file_type UTF8String];
	int ret_val = file_promise_name_callback(ih, (char*)c_file_type, file_buffer, PATH_MAX);
	
	// user wants to fallback to default routine
	if(IUP_CONTINUE == ret_val)
	{
		NSString* default_file_name = [self defaultFilePromiseName];
		return default_file_name;
	}
	
	NSString* ret_string = [NSString stringWithUTF8String:file_buffer];
	
	return ret_string;
//	return @"MyFile123.png";
}

// TODO: allow optional opt-in for writing files on background thread.
// User's callback must be thread-safe, which includes any language bindings and VM.
/*
- (NSOperationQueue *)operationQueueForFilePromiseProvider:(NSFilePromiseProvider*)filePromiseProvider
{
}
*/

static bool cocoaSourceDragDoDefaultFileCreate(NSFilePromiseProvider* file_promise_provider, NSURL* write_url)
{
	// Convention: If userInfo is not nil, then that contains the data we need to write to disk.
	id the_object = [file_promise_provider userInfo];
	bool did_handle = false;
	if(the_object != nil)
	{
		did_handle = true;

		if([the_object isKindOfClass:[NSImage class]])
		{
			NSImage* the_image = the_object;
			NSData* tiff_rep = [the_image TIFFRepresentation];
			NSBitmapImageRep* bitmap_rep = [[NSBitmapImageRep alloc] initWithData:tiff_rep];
			[bitmap_rep autorelease];
			NSData* png_rep = [bitmap_rep representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
			[png_rep writeToURL:write_url atomically:NO];
		}
		else if([the_object isKindOfClass:[NSURL class]])
		{
			NSURL* ns_url = (NSURL*)the_object;
			NSString* ns_string = [ns_url absoluteString];
			[ns_string writeToURL:write_url atomically:NO encoding:NSUTF8StringEncoding error:nil];
		}
		else if([the_object isKindOfClass:[NSAttributedString class]])
		{
			NSAttributedString* ns_attributed_string = (NSAttributedString*)the_object;
			NSData* ns_data = [ns_attributed_string RTFFromRange:NSMakeRange(0, [ns_attributed_string length]) documentAttributes:@{}];
			[ns_data writeToURL:write_url atomically:NO];
		}
		else if([the_object isKindOfClass:[NSString class]])
		{
			NSString* ns_string = (NSString*)the_object;
			[ns_string writeToURL:write_url atomically:NO encoding:NSUTF8StringEncoding error:nil];
		}
		else if([the_object isKindOfClass:[NSData class]])
		{
			NSData* ns_data = (NSData*)the_object;
			[ns_data writeToURL:write_url atomically:NO];
		}
		else if([the_object isKindOfClass:[NSFont class]])
		{
			NSFont* ns_font = (NSFont*)the_object;
			// requires 10.13. Probably better to write custom text file any way.
//			NSData* ns_data = [NSKeyedArchiver archiveRootObject:ns_font requiringSecureCoding:YES error:nil];
			//+ (BOOL)archiveRootObject:(id)rootObject toFile:(NSString *)path
			//[ns_data writeToURL:write_url atomically:NO];

			// FIXME: Does not include style
			NSString* ns_string = [NSString stringWithFormat:@"%@, %lf", [ns_font fontName], [ns_font pointSize]];
			[ns_string writeToURL:write_url atomically:NO encoding:NSUTF8StringEncoding error:nil];
		}
		else if([the_object isKindOfClass:[NSColor class]])
		{
			NSColor* ns_color = (NSColor*)the_object;
			NSColor* rgb_color = [ns_color colorUsingColorSpace:[NSColorSpace genericRGBColorSpace]];
			int r = iupROUND([rgb_color redComponent] * 255.0);
			int g = iupROUND([rgb_color greenComponent] * 255.0);
			int b = iupROUND([rgb_color blueComponent] * 255.0);
			int a = iupROUND([rgb_color alphaComponent] * 255.0);
			NSString* ns_string = [NSString stringWithFormat:@"%d %d %d %d", r, g, b, a];
			[ns_string writeToURL:write_url atomically:NO encoding:NSUTF8StringEncoding error:nil];
		}
		else
		{
			did_handle = false;
		}
	}
	return did_handle;
}

- (void) filePromiseProvider:(NSFilePromiseProvider*)file_promise_provider writePromiseToURL:(NSURL*)write_url completionHandler:(void (^)(NSError * __nullable errorOrNil))completion_handler;
{
//	NSLog(@"%@, %@", NSStringFromSelector(_cmd), write_url);
	

#if 0
	// This doesn't work. File promise provider isn't a NSPasteboardItem.
	// I don't see any way to fetch an alternative data type from here.
	// So unfortunately, the user will have to re-implement all that for the file write.
	NSData* ns_data = [file_promise_provider dataForType:NSPasteboardTypePNG];
//	NSData* ns_data = [self pasteboard:nil item:file_promise_provider provideDataForType:NSPasteboardTypePNG];

	[ns_data writeToURL:write_url atomically:NO];
#else

	Ihandle* ih = [self ihandle];
	IFns file_create_callback = (IFns)IupGetCallback(ih, "DRAGFILECREATE_CB");
	
	if(NULL == file_create_callback)
	{
		cocoaSourceDragDoDefaultFileCreate(file_promise_provider, write_url);
		completion_handler(nil);
		return;
	}
	
	NSString* file_url = [write_url path]; // still has file://
	const char* file_path = [file_url fileSystemRepresentation];
	// User gets to write the file here.
	int ret_val = file_create_callback(ih, (char*)file_path);

	// user wants to fallback to default routine
	if(IUP_CONTINUE == ret_val)
	{
		cocoaSourceDragDoDefaultFileCreate(file_promise_provider, write_url);
		completion_handler(nil);
		return;
	}


#endif
}

//- (BOOL) ignoreModifierKeysForDraggingSession:(NSDraggingSession *)session;

- (void) draggingSession:(NSDraggingSession*)dragging_session willBeginAtPoint:(NSPoint)screen_point
{
//	NSLog(@"%@, %@, %@", NSStringFromSelector(_cmd), dragging_session, NSStringFromPoint(screen_point));

	Ihandle* ih = [self ihandle];
	IFnii call_back = (IFnii)IupGetCallback(ih, "DRAGBEGIN_CB");
	if(NULL != call_back)
	{
		NSView* main_view = [self mainView];
		NSWindow* the_window = [main_view window];
		NSRect screen_rect = NSMakeRect(screen_point.x, screen_point.y, 0, 0);
		NSRect window_rect = [the_window convertRectFromScreen:screen_rect];
		
		NSPoint window_point = window_rect.origin;
		NSPoint view_point = [main_view convertPoint:window_point fromView:nil];
		NSRect view_frame = [main_view frame];
		CGFloat inverted_y = view_frame.size.height - view_point.y;
		view_point.y = inverted_y;
		call_back(ih, view_point.x, view_point.y);
	}
}
/*
- (void) draggingSession:(NSDraggingSession*)dragging_session movedToPoint:(NSPoint)screen_point
{
//	NSLog(@"%@, %@, %@", NSStringFromSelector(_cmd), dragging_session, NSStringFromPoint(screen_point));
//	[super draggingSession:dragging_session movedToPoint:screen_point];
}
*/

- (void) draggingSession:(NSDraggingSession*)dragging_session endedAtPoint:(NSPoint)screen_point operation:(NSDragOperation)drag_operation
{
//	NSLog(@"%@, %@, %@, %d", NSStringFromSelector(_cmd), dragging_session, NSStringFromPoint(screen_point), drag_operation);
	Ihandle* ih = [self ihandle];
	IFni call_back = (IFni)IupGetCallback(ih, "DRAGEND_CB");
	if(NULL != call_back)
	{
		NSView* main_view = [self mainView];
		NSWindow* the_window = [main_view window];
		NSRect screen_rect = NSMakeRect(screen_point.x, screen_point.y, 0, 0);
		NSRect window_rect = [the_window convertRectFromScreen:screen_rect];
		
		NSPoint window_point = window_rect.origin;
		NSPoint view_point = [main_view convertPoint:window_point fromView:nil];
		NSRect view_frame = [main_view frame];
		CGFloat inverted_y = view_frame.size.height - view_point.y;
		view_point.y = inverted_y;
		// action: action performed by the operation (1 = move, 0 = copy, -1 = drag failed or aborted)
/*
    NSDragOperationNone		= 0,
    NSDragOperationCopy		= 1,
    NSDragOperationLink		= 2,
    NSDragOperationGeneric	= 4,
    NSDragOperationPrivate	= 8,
    NSDragOperationMove		= 16,
    NSDragOperationDelete	= 32,
*/
		int action_val = 0;
		if(NSDragOperationNone == drag_operation)
		{
			action_val = -1;
		}
		else if(NSDragOperationCopy == drag_operation)
		{
			action_val = 0;
		}
		else if(NSDragOperationMove == drag_operation)
		{
			action_val = 1;
		}
		else
		{
			// No idea what to do.
			action_val = (int)drag_operation;
		}
		call_back(ih, action_val);
	}
}


@end

IupTargetDropAssociatedData* cocoaTargetDropCreateAssociatedData(Ihandle* ih, NSView* main_view, NSView* root_view)
{
	if(!ih || !ih->handle)
	{
		return nil;
	}
	IupTargetDropAssociatedData* drag_drop_data = [[IupTargetDropAssociatedData alloc] init];
	[drag_drop_data autorelease];
	objc_setAssociatedObject(ih->handle, IUPTARGETDROP_ASSOCIATED_OBJ_KEY, (id)drag_drop_data, OBJC_ASSOCIATION_RETAIN_NONATOMIC);

	[drag_drop_data setIhandle:ih];
	[drag_drop_data setMainView:main_view];
	[drag_drop_data setRootView:root_view];
	
	return drag_drop_data;
}

void cocoaTargetDropDestroyAssociatedData(Ihandle* ih)
{
	if(!ih || !ih->handle)
	{
		return;
	}
	IupTargetDropAssociatedData* drag_drop_data = cocoaTargetDropGetAssociatedData(ih);
	[drag_drop_data setIhandle:nil];
	[drag_drop_data setMainView:nil];
	[drag_drop_data setRootView:nil];

	objc_setAssociatedObject(ih->handle, IUPTARGETDROP_ASSOCIATED_OBJ_KEY, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

IupTargetDropAssociatedData* cocoaTargetDropGetAssociatedData(Ihandle* ih)
{
	if(!ih || !ih->handle)
	{
		return nil;
	}
	IupTargetDropAssociatedData* drag_drop_data = (IupTargetDropAssociatedData*)objc_getAssociatedObject((id)ih->handle, IUPTARGETDROP_ASSOCIATED_OBJ_KEY);
	return drag_drop_data;
}
/*
static IupTargetDropAssociatedData* cocoaTargetDropGetOrCreateAssociatedData(Ihandle* ih)
{

	IupTargetDropAssociatedData* drag_drop_data = cocoaTargetDropGetAssociatedData(ih);
	if(nil == drag_drop_data)
	{
		drag_drop_data = cocoaTargetDropCreateAssociatedData(ih);
	}
	return drag_drop_data;
}
*/

static int cocoaTargetDropSetDropTargetAttrib(Ihandle* ih, const char* value)
{
//	iupAttribSet(ih, "_IUPCOCOA_DROPTARGET", value);
	id the_object = ih->handle;
	IupTargetDropAssociatedData* drag_drop_data = cocoaTargetDropGetAssociatedData(ih);
	if(iupStrBoolean(value))
	{
		[drag_drop_data setDropTargetEnabled:true];
		if([the_object respondsToSelector:@selector(registerForDraggedTypes:)])
		{
			NSArray* array_of_types = [drag_drop_data dropRegisteredTypes];
			[the_object registerForDraggedTypes:array_of_types];
		}
	}
	else
	{
		[drag_drop_data setDropTargetEnabled:false];
		if([the_object respondsToSelector:@selector(unregisterDraggedTypes)])
		{
			[the_object unregisterDraggedTypes];
		}
	}

	return 0;

}

static int cocoaTargetDropSetDropTypesAttrib(Ihandle* ih, const char* value)
{
	
	IupTargetDropAssociatedData* drag_drop_data = cocoaTargetDropGetAssociatedData(ih);
//- (void)registerForDraggedTypes:(NSArray<NSPasteboardType> *)newTypes;
//- (void)unregisterDraggedTypes;
	id the_object = ih->handle;

	if(NULL != value)
	{
		NSString* comma_separated_string = [NSString stringWithUTF8String:value];
		// The Windows version doesn't worry about extra white space, so I won't either.
		NSArray* array_of_types = [comma_separated_string componentsSeparatedByString:@","];
		
		// TODO: Introduce generic strings like TEXT, IMAGE, and map them to the native type, to allow easier cross-platform code.

		// Special case: Look for FILEPROMISE and replace with the real values.
		NSUInteger file_promise_index = [array_of_types indexOfObject:@"FILEPROMISE"];
		if(NSNotFound != file_promise_index)
		{
			NSMutableArray* mutable_array_of_types = [array_of_types mutableCopy];
			[mutable_array_of_types autorelease];
			NSRange replace_range = NSMakeRange(file_promise_index, 1);
			[mutable_array_of_types replaceObjectsInRange:replace_range withObjectsFromArray:[NSFilePromiseReceiver readableDraggedTypes]];
			array_of_types = mutable_array_of_types;
		}

		[drag_drop_data setDropRegisteredTypes:array_of_types];

		if([drag_drop_data isDropTargetEnabled])
		{
			if([the_object respondsToSelector:@selector(registerForDraggedTypes:)])
			{
				[the_object registerForDraggedTypes:array_of_types];
			}
		}
	}
	else
	{
		if([the_object respondsToSelector:@selector(unregisterDraggedTypes)])
		{
			[the_object unregisterDraggedTypes];
		}
		[drag_drop_data setDropRegisteredTypes:nil];
	}

	return 0;
}

static int cocoaSourceDropSetPasteFromPasteboardAttrib(Ihandle* ih, const char* value)
{
	bool is_triggered = iupStrBoolean(value);
	if(!is_triggered)
	{
		return 0;
	}
	
	NSPasteboard* paste_board = [NSPasteboard generalPasteboard];
	// TODO: Provide user a way to change the drop point value for paste (so they can differentiate from a drop if needed)
	NSPoint drop_point = {0, 0};

	IupTargetDropAssociatedData* drag_drop_data = cocoaTargetDropGetAssociatedData(ih);
	id sender_object = [drag_drop_data mainView];
	cocoaTargetDropBasePerformDropCallback(ih, sender_object, paste_board, drop_point);
	return 0;
}


IupSourceDragAssociatedData* cocoaSourceDragCreateAssociatedData(Ihandle* ih, NSView* main_view, NSView* root_view)
{
	if(!ih || !ih->handle)
	{
		return nil;
	}
	IupSourceDragAssociatedData* drag_drop_data = [[IupSourceDragAssociatedData alloc] init];
	[drag_drop_data autorelease];
	objc_setAssociatedObject(ih->handle, IUPSOURCEDRAG_ASSOCIATED_OBJ_KEY, (id)drag_drop_data, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
	[drag_drop_data setIhandle:ih];
	[drag_drop_data setMainView:main_view];
	[drag_drop_data setRootView:root_view];
	return drag_drop_data;
}

void cocoaSourceDragDestroyAssociatedData(Ihandle* ih)
{
	if(!ih || !ih->handle)
	{
		return;
	}
	IupSourceDragAssociatedData* drag_drop_data = cocoaSourceDragGetAssociatedData(ih);
	[drag_drop_data setIhandle:nil];
	[drag_drop_data setMainView:nil];
	[drag_drop_data setRootView:nil];

	objc_setAssociatedObject(ih->handle, IUPSOURCEDRAG_ASSOCIATED_OBJ_KEY, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}


IupSourceDragAssociatedData* cocoaSourceDragGetAssociatedData(Ihandle* ih)
{
	if(!ih || !ih->handle)
	{
		return nil;
	}
	IupSourceDragAssociatedData* drag_drop_data = (IupSourceDragAssociatedData*)objc_getAssociatedObject((id)ih->handle, IUPSOURCEDRAG_ASSOCIATED_OBJ_KEY);
	return drag_drop_data;
}

/*
static IupSourceDragAssociatedData* cocoaSourceDragGetOrCreateAssociatedData(Ihandle* ih)
{
	IupSourceDragAssociatedData* drag_drop_data = cocoaSourceDragGetAssociatedData(ih);
	if(nil == drag_drop_data)
	{
		drag_drop_data = cocoaSourceDragCreateAssociatedData(ih);
	}
	return drag_drop_data;
}
*/

static int cocoaSourceDragSetDragSourceAttrib(Ihandle* ih, const char* value)
{
//	iupAttribSet(ih, "_IUPCOCOA_DROPTARGET", value);
	IupSourceDragAssociatedData* drag_drop_data = cocoaSourceDragGetAssociatedData(ih);
	if(iupStrBoolean(value))
	{
		[drag_drop_data setDragSourceEnabled:true];
		/*
		if([the_object respondsToSelector:@selector(registerForDraggedTypes:)])
		{
			NSArray* array_of_types = [drag_drop_data dropRegisteredTypes];
			[the_object registerForDraggedTypes:array_of_types];
		}
		*/
	}
	else
	{
		[drag_drop_data setDragSourceEnabled:false];
		/*
		if([the_object respondsToSelector:@selector(unregisterDraggedTypes)])
		{
			[the_object unregisterDraggedTypes];
		}
		*/
	}

	return 0;

}

static int cocoaSourceDragSetDragTypesAttrib(Ihandle* ih, const char* value)
{
//	iupAttribSet(ih, "_IUPCOCOA_DROPTARGET", value);

	IupSourceDragAssociatedData* drag_drop_data = cocoaSourceDragGetAssociatedData(ih);
	if(value)
	{
		NSString* comma_separated_string = [NSString stringWithUTF8String:value];
		// The Windows version doesn't worry about extra white space, so I won't either.
		NSArray* array_of_types = [comma_separated_string componentsSeparatedByString:@","];
		
		// TODO: Introduce generic strings like TEXT, IMAGE, and map them to the native type, to allow easier cross-platform code.

		// Special case: Look for FILEPROMISE and replace with the real values.
		NSUInteger file_promise_index = [array_of_types indexOfObject:@"FILEPROMISE"];
		if(NSNotFound != file_promise_index)
		{
			NSMutableArray* mutable_array_of_types = [array_of_types mutableCopy];
			[mutable_array_of_types autorelease];
			NSRange replace_range = NSMakeRange(file_promise_index, 1);
			[mutable_array_of_types replaceObjectsInRange:replace_range withObjectsFromArray:[NSFilePromiseReceiver readableDraggedTypes]];
			array_of_types = mutable_array_of_types;
		}
		
		[drag_drop_data setDragRegisteredTypes:array_of_types];

	}
	else
	{
		[drag_drop_data setDragRegisteredTypes:nil];
	}

	return 0;

}

static int cocoaSourceDragSetDragAutoImageAttrib(Ihandle* ih, const char* value)
{
	IupSourceDragAssociatedData* drag_drop_data = cocoaSourceDragGetAssociatedData(ih);
	bool is_enabled = iupStrBoolean(value);
	[drag_drop_data setUseAutoGenerateDragImage:is_enabled];
	return 0;
}
char* cocoaSourceDragGetDragAutoImageAttrib(Ihandle* ih)
{
	IupSourceDragAssociatedData* drag_drop_data = cocoaSourceDragGetAssociatedData(ih);
	bool is_enabled = [drag_drop_data useAutoGenerateDragImage];
	return iupStrReturnBoolean(is_enabled);
}

/*
static int cocoaSourceDragSetDragWantsFileCreateAttrib(Ihandle* ih, const char* value)
{
	IupSourceDragAssociatedData* drag_drop_data = cocoaSourceDragGetAssociatedData(ih);
	bool is_enabled = iupStrBoolean(value);
	[drag_drop_data setWantsFilePromise:is_enabled];
	return 0;
}
*/


static int cocoaSourceDragSetAutoBeginDragAttrib(Ihandle* ih, const char* value)
{
	IupSourceDragAssociatedData* drag_drop_data = cocoaSourceDragGetAssociatedData(ih);
	bool is_enabled = iupStrBoolean(value);
	[drag_drop_data setUseAutoBeginDrag:is_enabled];
	return 0;
}
char* cocoaSourceDragGetAutoBeginDragAttrib(Ihandle* ih)
{
	IupSourceDragAssociatedData* drag_drop_data = cocoaSourceDragGetAssociatedData(ih);
	bool is_enabled = [drag_drop_data useAutoBeginDrag];
	return iupStrReturnBoolean(is_enabled);
}


// WARNING: This may be unsupportable.
// The currentEvent may not be the right kind ad will throw an exception when our code tries to call invalid methods for the wrong type.
// The one reason I think this may work is If-and-only-if the user calls this in the mouseDragged: callback (mouseDown: might also work),
// then the currentEvent should (I hope) be the event passed to mouseDragged:.
// If that is true, this should work.
// But calling anywhere else will probably not work.
static int cocoaSourceDragSetBeginDragAttrib(Ihandle* ih, const char* value)
{
	IupSourceDragAssociatedData* drag_source_data = cocoaSourceDragGetAssociatedData(ih);

	if([drag_source_data isDragSourceEnabled])
	{
		NSDraggingItem* dragging_item = [drag_source_data defaultDraggingItem];

		NSView* main_view = [drag_source_data mainView];

		NSEvent* the_event = [[NSApplication sharedApplication] currentEvent];
		[main_view beginDraggingSessionWithItems:@[dragging_item] event:the_event source:drag_source_data];
	}

	return 0;
}



static int cocoaSourceDragSetCopyToPasteboardAttrib(Ihandle* ih, const char* value)
{
	bool is_triggered = iupStrBoolean(value);
	if(!is_triggered)
	{
		return 0;
	}
	
	IupSourceDragAssociatedData* drag_source_data = cocoaSourceDragGetAssociatedData(ih);

	NSPasteboardItem* pasteboard_item = [drag_source_data defaultPasteboardItem];
	NSPasteboard* paste_board = [NSPasteboard generalPasteboard];

	[paste_board clearContents];
	[paste_board writeObjects:@[pasteboard_item]];
	return 0;
}


void iupdrvRegisterDragDropAttrib(Iclass* ic)
{
  iupClassRegisterCallback(ic, "DRAGBEGIN_CB", "ii");
  iupClassRegisterCallback(ic, "DRAGEND_CB", "i");
  iupClassRegisterCallback(ic, "DRAGDATASIZE_CB", "s");
  iupClassRegisterCallback(ic, "DRAGDATA_CB", "sCi");
	
//	  iupClassRegisterCallback(ic, "DROPFILES_CB", "siii");

  iupClassRegisterCallback(ic, "DROPDATA_CB", "sCiii");
  iupClassRegisterCallback(ic, "DROPMOTION_CB", "iis"); // This must be


  iupClassRegisterAttribute(ic, "DRAGTYPES",  NULL, cocoaSourceDragSetDragTypesAttrib,  NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DRAGSOURCE", NULL, cocoaSourceDragSetDragSourceAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGSOURCEMOVE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);


	// NEW API:
  iupClassRegisterAttribute(ic, "DRAGAUTOIMAGE", cocoaSourceDragGetDragAutoImageAttrib, cocoaSourceDragSetDragAutoImageAttrib, NULL, NULL, IUPAF_NO_INHERIT);

	// EXPERIMENTAL: This may need to exist on a per-widget basis.
  iupClassRegisterAttribute(ic, "DRAGAUTOINITIATE", cocoaSourceDragGetAutoBeginDragAttrib, cocoaSourceDragSetAutoBeginDragAttrib, "YES", NULL, IUPAF_NO_INHERIT);

	// EXPERIMENTAL: May not work because it uses currentEvent
  iupClassRegisterAttribute(ic, "DRAGINITIATE", NULL, cocoaSourceDragSetBeginDragAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE);


//  iupClassRegisterAttribute(ic, "DRAGFILECREATE", NULL, cocoaSourceDragSetDragWantsFileCreateAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterCallback(ic, "DRAGFILECREATE_CB", "s");
  iupClassRegisterCallback(ic, "DRAGFILECREATENAME_CB", "ssi");


	// Because we may need to create an IupImage in the DRAGDATA_CB, we need a place to free it.
	// The DRAGEND_CB is less than ideal because it will not fire for copy/paste and it is kind of ambiguous for multiple-drag.
	// TODO: Need to add to other platforms
	iupClassRegisterCallback(ic, "DRAGDATAEND_CB", "s");


	// Because DROPDATA_CB will fire off multiple callbacks if multiple items are dropped at the same time,
	// we want a way to know when this starts, and how many items there are.
	// And when the callbacks stop.
	// The passed number is a hint. If the data type cannot be supported, a callback may skip firing.
	// Use the DROPDATAEND_CB to know for sure when you are done and don't rely on the count.
  iupClassRegisterCallback(ic, "DROPDATABEGIN_CB", "i");
  iupClassRegisterCallback(ic, "DROPDATAEND_CB", "");


	// Copy & Paste to Clipboard (Pasteboard)
	// DD is for Drag & Drop.
	// We need an API that the user can call to invoke/trigger copying or pasting an item.
	// Since the data mechanisms are driven by the drag and drop implementation, everything else works through the drag & drop after the trigger.
	iupClassRegisterAttribute(ic, "DRAGCOPY", NULL, cocoaSourceDragSetCopyToPasteboardAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE);
	iupClassRegisterAttribute(ic, "DROPPASTE", NULL, cocoaSourceDropSetPasteFromPasteboardAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE);



  iupClassRegisterAttribute(ic, "DROPTARGET", NULL, cocoaTargetDropSetDropTargetAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPTYPES",  NULL, cocoaTargetDropSetDropTypesAttrib,  NULL, NULL, IUPAF_NO_INHERIT);
//  iupClassRegisterAttribute(ic, "DRAGDROP", NULL, cocoaDragDropSetDropFilesTargetAttrib, NULL, NULL, IUPAF_NO_INHERIT);
//  iupClassRegisterAttribute(ic, "DROPFILESTARGET", NULL, cocoaDragDropSetDropFilesTargetAttrib, NULL, NULL, IUPAF_NO_INHERIT);



#if 0
	NSLog(@"NSPasteboardTypeString: %@", NSPasteboardTypeString);
	NSLog(@"NSPasteboardTypePDF: %@", NSPasteboardTypePDF);
	NSLog(@"NSPasteboardTypeTIFF: %@", NSPasteboardTypeTIFF);
	NSLog(@"NSPasteboardTypePNG: %@", NSPasteboardTypePNG);
	NSLog(@"NSPasteboardTypeRTF: %@", NSPasteboardTypeRTF);
	NSLog(@"NSPasteboardTypeRTFD: %@", NSPasteboardTypeRTFD);
	NSLog(@"NSPasteboardTypeHTML: %@", NSPasteboardTypeHTML);
	NSLog(@"NSPasteboardTypeTabularText: %@", NSPasteboardTypeTabularText);
	NSLog(@"NSPasteboardTypeFont: %@", NSPasteboardTypeFont);
	NSLog(@"NSPasteboardTypeRuler: %@", NSPasteboardTypeRuler);
	NSLog(@"NSPasteboardTypeColor: %@", NSPasteboardTypeColor);
	NSLog(@"NSPasteboardTypeSound: %@", NSPasteboardTypeSound);
	NSLog(@"NSPasteboardTypeMultipleTextSelection: %@", NSPasteboardTypeMultipleTextSelection);
	NSLog(@"NSPasteboardTypeTextFinderOptions: %@", NSPasteboardTypeTextFinderOptions);
	NSLog(@"NSPasteboardTypeURL: %@", NSPasteboardTypeURL);
	NSLog(@"NSPasteboardTypeFileURL: %@", NSPasteboardTypeFileURL);

	for(NSString* file_promise_type in [NSFilePromiseReceiver readableDraggedTypes])
	{
		NSLog(@"readableDraggedTypes: %@", file_promise_type);
	}
#endif


/*
NSPasteboardTypeString: public.utf8-plain-text
NSPasteboardTypePDF: com.adobe.pdf
NSPasteboardTypeTIFF: public.tiff
NSPasteboardTypePNG: public.png
NSPasteboardTypeRTF: public.rtf
NSPasteboardTypeRTFD: com.apple.flat-rtfd
NSPasteboardTypeHTML: public.html
NSPasteboardTypeTabularText: public.utf8-tab-separated-values-text
NSPasteboardTypeFont: com.apple.cocoa.pasteboard.character-formatting
NSPasteboardTypeRuler: com.apple.cocoa.pasteboard.paragraph-formatting
NSPasteboardTypeColor: com.apple.cocoa.pasteboard.color
NSPasteboardTypeSound: com.apple.cocoa.pasteboard.sound
NSPasteboardTypeMultipleTextSelection: com.apple.cocoa.pasteboard.multiple-text-selection
NSPasteboardTypeTextFinderOptions: com.apple.cocoa.pasteboard.find-panel-search-options
NSPasteboardTypeURL: public.url
NSPasteboardTypeFileURL: public.file-url
*/


	
}

