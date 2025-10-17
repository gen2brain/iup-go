/** \file
 * \brief Drag&Drop Functions
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


const void* IUPSOURCEDRAG_ASSOCIATED_OBJ_KEY = @"IUPSOURCEDRAG_ASSOCIATED_OBJ_KEY";
const void* IUPTARGETDROP_ASSOCIATED_OBJ_KEY = @"IUPTARGETDROP_ASSOCIATED_OBJ_KEY";


@implementation IupTargetDropAssociatedData

- (void) dealloc
{
  [self setDropRegisteredTypes:nil];
  [self setIhandle:NULL];
  [self setDropTargetEnabled:false];
  [self setMainView:nil];
  [self setRootView:nil];
  [super dealloc];
}

@end

NSDragOperation cocoaTargetDropBaseDraggingUpdated(Ihandle* ih, id<NSDraggingInfo> the_sender)
{
  IFniis cbDropMotion = (IFniis)IupGetCallback(ih, "DROPMOTION_CB");
  if(cbDropMotion)
  {
    IupTargetDropAssociatedData* associated_data = cocoaTargetDropGetAssociatedData(ih);
    NSView* main_view = [associated_data mainView];
    if (!main_view)
    {
      return NSDragOperationNone;
    }

    NSWindow* the_window = [main_view window];
    NSPoint screen_point = [the_sender draggingLocation];
    NSRect screen_rect = NSMakeRect(screen_point.x, screen_point.y, 0, 0);
    NSRect window_rect = [the_window convertRectFromScreen:screen_rect];
    NSPoint window_point = window_rect.origin;
    NSPoint view_point = [main_view convertPoint:window_point fromView:nil];

    if (![main_view isFlipped])
    {
      NSRect view_bounds = [main_view bounds];
      view_point.y = view_bounds.size.height - view_point.y;
    }

    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;

    NSEventModifierFlags modifier_flags = [[NSApp currentEvent] modifierFlags];
    if (modifier_flags & NSEventModifierFlagShift)
      iupKEY_SETSHIFT(status);
    if (modifier_flags & NSEventModifierFlagControl)
      iupKEY_SETCONTROL(status);
    if (modifier_flags & NSEventModifierFlagOption)
      iupKEY_SETALT(status);
    if (modifier_flags & NSEventModifierFlagCommand)
      iupKEY_SETSYS(status);

    if (CGEventSourceButtonState(kCGEventSourceStateHIDSystemState, kCGMouseButtonLeft))
      iupKEY_SETBUTTON1(status);
    if (CGEventSourceButtonState(kCGEventSourceStateHIDSystemState, kCGMouseButtonRight))
      iupKEY_SETBUTTON3(status);
    if (CGEventSourceButtonState(kCGEventSourceStateHIDSystemState, kCGMouseButtonCenter))
      iupKEY_SETBUTTON2(status);

    cbDropMotion(ih, (int)view_point.x, (int)view_point.y, status);

    return [the_sender draggingSourceOperationMask];
  }

  return [the_sender draggingSourceOperationMask];
}

// Helper to convert NSFont to an IUP font string, preserving style.
static NSString* iupCocoaGetFontStringFromFont(NSFont* ns_font)
{
  if (!ns_font)
  {
    return @"";
  }

  NSFontManager* font_manager = [NSFontManager sharedFontManager];
  NSFontTraitMask traits = [font_manager traitsOfFont:ns_font];

  NSMutableString* style_string = [NSMutableString string];
  if ((traits & NSBoldFontMask))
  {
    [style_string appendString:@"Bold "];
  }
  if ((traits & NSItalicFontMask))
  {
    [style_string appendString:@"Italic "];
  }

  // Trim trailing space
  NSString* final_style = [style_string stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];

  NSString* family_name = [ns_font familyName];
  double point_size = [ns_font pointSize];

  if ([final_style length] > 0)
  {
    return [NSString stringWithFormat:@"%@, %@ %g", family_name, final_style, point_size];
  }
  else
  {
    return [NSString stringWithFormat:@"%@, %g", family_name, point_size];
  }
}

int cocoaTargetDropBasePerformDropCallback(Ihandle* ih, id<NSDraggingInfo> the_sender, NSPasteboard* paste_board, NSPoint drop_point)
{
  IFnsViii drop_data_callback = (IFnsViii)IupGetCallback(ih, "DROPDATA_CB");
  IFnsiii drop_files_callback = (IFnsiii)IupGetCallback(ih, "DROPFILES_CB");

  if(!drop_data_callback && !drop_files_callback)
  {
    return 0;
  }

  IupTargetDropAssociatedData* associated_data = cocoaTargetDropGetAssociatedData(ih);
  NSArray* drop_types = [associated_data dropRegisteredTypes];

  NSView* main_view = [associated_data mainView];
  NSPoint drop_point_iup = drop_point;
  if (main_view)
  {
    if (![main_view isFlipped])
    {
      drop_point_iup.y = [main_view bounds].size.height - drop_point.y;
    }
  }

  NSMutableArray* acceptable_classes = [NSMutableArray arrayWithCapacity:[drop_types count]];
  for(NSString* type_name in drop_types)
  {
    if([type_name isEqualToString:NSPasteboardTypeString])
    {
      [acceptable_classes addObject:[NSString class]];
    }
    else if([type_name isEqualToString:NSPasteboardTypeTIFF] || [type_name isEqualToString:NSPasteboardTypePNG])
    {
      [acceptable_classes addObject:[NSImage class]];
    }
    else if([type_name isEqualToString:NSPasteboardTypeRTF] || [type_name isEqualToString:NSPasteboardTypeRTFD] || [type_name isEqualToString:NSPasteboardTypeHTML])
    {
      [acceptable_classes addObject:[NSAttributedString class]];
    }
    else if([type_name isEqualToString:NSPasteboardTypeURL] || [type_name isEqualToString:NSPasteboardTypeFileURL])
    {
      [acceptable_classes addObject:[NSURL class]];
    }
    else
    {
      [acceptable_classes addObject:[NSPasteboardItem class]];
    }
  }

  NSArray* acceptable_drop_items = [paste_board readObjectsForClasses:acceptable_classes options:nil];

  if(drop_files_callback)
  {
    NSMutableArray* file_urls = [NSMutableArray array];
    for(id drop_item in acceptable_drop_items)
    {
      if([drop_item isKindOfClass:[NSURL class]])
      {
        NSURL* ns_url = (NSURL*)drop_item;
        if([ns_url isFileURL])
        {
          [file_urls addObject:ns_url];
        }
      }
    }

    if([file_urls count] > 0)
    {
      int total_files = (int)[file_urls count];
      for(int i = 0; i < total_files; i++)
      {
        NSURL* ns_url = file_urls[i];
        NSString* file_url_string = [ns_url path];
        const char* file_path = [file_url_string fileSystemRepresentation];
        if (drop_files_callback(ih, (char*)file_path, total_files - i - 1, (int)drop_point_iup.x, (int)drop_point_iup.y) == IUP_IGNORE)
        {
          break;
        }
      }
      return 0;
    }
  }

  if(!drop_data_callback)
  {
    return 0;
  }

  for(id drop_item in acceptable_drop_items)
  {
    int ret_val = IUP_DEFAULT;

    if([drop_item isKindOfClass:[NSImage class]])
    {
      NSImage* ns_image = (NSImage*)drop_item;
      int w, h, bpp;
      iupdrvImageGetInfo(ns_image, &w, &h, &bpp);
      int bytes_per_row = iupCocoaImageCalculateBytesPerRow(w, bpp/8);
      size_t buffer_size = bytes_per_row * h;
      unsigned char* img_data = malloc(buffer_size);
      iupdrvImageGetData(ns_image, img_data);

      char type_string[1024];
      snprintf(type_string, 1024, "IMAGE %d %d %d", w, h, bpp);
      ret_val = drop_data_callback(ih, type_string, (void*)img_data, (int)buffer_size, (int)drop_point_iup.x, (int)drop_point_iup.y);

      free(img_data);
    }
    else if([drop_item isKindOfClass:[NSURL class]])
    {
      NSURL* ns_url = (NSURL*)drop_item;

      if([ns_url isFileURL])
      {
        NSString* file_url = [ns_url path];
        const char* file_path = [file_url fileSystemRepresentation];
        size_t buffer_size = strlen(file_path) + 1;
        ret_val = drop_data_callback(ih, "FILE", (void*)file_path, (int)buffer_size, (int)drop_point_iup.x, (int)drop_point_iup.y);
      }
      else
      {
        NSString* ns_string = [ns_url absoluteString];
        const char* c_str = [ns_string UTF8String];
        size_t buffer_size = strlen(c_str) + 1;
        ret_val = drop_data_callback(ih, "URL", (void*)c_str, (int)buffer_size, (int)drop_point_iup.x, (int)drop_point_iup.y);
      }
    }
    else if([drop_item isKindOfClass:[NSString class]])
    {
      NSString* ns_string = (NSString*)drop_item;
      const char* c_str = [ns_string UTF8String];
      size_t buffer_size = strlen(c_str) + 1;
      ret_val = drop_data_callback(ih, "TEXT", (void*)c_str, (int)buffer_size, (int)drop_point_iup.x, (int)drop_point_iup.y);
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
      ret_val = drop_data_callback(ih, "COLOR", (void*)color_string, (int)buffer_size, (int)drop_point_iup.x, (int)drop_point_iup.y);
    }
    else if([drop_item isKindOfClass:[NSFont class]])
    {
      NSFont* ns_font = (NSFont*)drop_item;
      NSString* ns_string = iupCocoaGetFontStringFromFont(ns_font);
      const char* c_str = [ns_string UTF8String];
      size_t buffer_size = strlen(c_str) + 1;
      ret_val = drop_data_callback(ih, "FONT", (void*)c_str, (int)buffer_size, (int)drop_point_iup.x, (int)drop_point_iup.y);
    }
    else
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

      if([best_type isEqualToString:NSPasteboardTypePNG] || [best_type isEqualToString:NSPasteboardTypeTIFF])
      {
        NSImage* ns_image = [[[NSImage alloc] initWithData:ns_data] autorelease];
        int w, h, bpp;
        iupdrvImageGetInfo(ns_image,&w, &h, &bpp);
        int bytes_per_row = iupCocoaImageCalculateBytesPerRow(w, bpp/8);
        size_t buffer_size = bytes_per_row*h;
        unsigned char* img_data = malloc(buffer_size);
        iupdrvImageGetData(ns_image, img_data);

        char type_string[1024];
        snprintf(type_string, 1024, "IMAGE %d %d %d", w, h, bpp);
        ret_val = drop_data_callback(ih, type_string, (void*)img_data, (int)buffer_size, (int)drop_point_iup.x, (int)drop_point_iup.y);
        free(img_data);
      }
      else if([best_type isEqualToString:NSPasteboardTypeFileURL])
      {
        NSString* ns_string = [[[NSString alloc] initWithData:ns_data encoding:NSUTF8StringEncoding] autorelease];
        NSURL* ns_url = [[[NSURL alloc] initWithString:ns_string] autorelease];
        NSString* file_url = [ns_url path];
        const char* file_path = [file_url fileSystemRepresentation];
        size_t buffer_size = strlen(file_path) + 1;
        ret_val = drop_data_callback(ih, "FILE", (void*)file_path, (int)buffer_size, (int)drop_point_iup.x, (int)drop_point_iup.y);
      }
      else if([best_type isEqualToString:NSPasteboardTypeURL])
      {
        NSString* ns_string = [[[NSString alloc] initWithData:ns_data encoding:NSUTF8StringEncoding] autorelease];
        NSURL* ns_url = [[[NSURL alloc] initWithString:ns_string] autorelease];
        NSString* ns_string2 = [ns_url absoluteString];
        const char* c_str = [ns_string2 UTF8String];
        size_t buffer_size = strlen(c_str) + 1;
        ret_val = drop_data_callback(ih, "URL", (void*)c_str, (int)buffer_size, (int)drop_point_iup.x, (int)drop_point_iup.y);
      }
      else if([best_type isEqualToString:NSPasteboardTypeString] || [best_type isEqualToString:NSPasteboardTypeHTML] ||
              [best_type isEqualToString:NSPasteboardTypeTabularText] || [best_type isEqualToString:NSPasteboardTypeMultipleTextSelection])
      {
        NSString* ns_string = [[[NSString alloc] initWithData:ns_data encoding:NSUTF8StringEncoding] autorelease];
        const char* c_str = [ns_string UTF8String];
        size_t buffer_size = strlen(c_str) + 1;
        ret_val = drop_data_callback(ih, "TEXT", (void*)c_str, (int)buffer_size, (int)drop_point_iup.x, (int)drop_point_iup.y);
      }
      else if([best_type isEqualToString:NSPasteboardTypeColor])
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
        ret_val = drop_data_callback(ih, "COLOR", (void*)color_string, (int)buffer_size, (int)drop_point_iup.x, (int)drop_point_iup.y);
      }
      else if([best_type isEqualToString:NSPasteboardTypeFont])
      {
        NSFont* ns_font = (NSFont*)[NSUnarchiver unarchiveObjectWithData:ns_data];
        NSString* ns_string = iupCocoaGetFontStringFromFont(ns_font);
        const char* c_str = [ns_string UTF8String];
        size_t buffer_size = strlen(c_str) + 1;
        ret_val = drop_data_callback(ih, "FONT", (void*)c_str, (int)buffer_size, (int)drop_point_iup.x, (int)drop_point_iup.y);
      }
      else
      {
        const void* data_buffer = [ns_data bytes];
        NSUInteger buffer_size = [ns_data length];
        const char* c_type_str = [best_type UTF8String];
        ret_val = drop_data_callback(ih, (char*)c_type_str, (void*)data_buffer, (int)buffer_size, (int)drop_point_iup.x, (int)drop_point_iup.y);
      }
    }

    if (ret_val == IUP_IGNORE)
      break;
  }

  return 0;
}

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

  Ihandle* ih = [self ihandle];
  bool is_move = iupAttribGetBoolean(ih, "DRAGSOURCEMOVE");
  if(is_move)
  {
    return NSDragOperationMove | NSDragOperationCopy;
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
    NSData* pdf_data = [main_view dataWithPDFInsideRect:bounds_rect];
    NSImage* image_data = [[NSImage alloc] initWithData:pdf_data];
    [image_data autorelease];
    NSData* tiff_data = [image_data TIFFRepresentation];
    [pasteboard_item setData:tiff_data forType:type_name];
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
  }
  else if([type_name isEqualToString:NSPasteboardTypeRTF] || [type_name isEqualToString:NSPasteboardTypeRTFD])
  {
    if([main_view respondsToSelector:@selector(attributedStringValue)])
    {
      NSAttributedString* attr_str = [main_view attributedStringValue];
      NSData* ns_data = [NSKeyedArchiver archivedDataWithRootObject:attr_str];
      [pasteboard_item setData:ns_data forType:type_name];
    }
  }
  else if([type_name isEqualToString:NSPasteboardTypeHTML])
  {
    if([main_view respondsToSelector:@selector(stringValue)])
    {
      NSString* ns_string = [main_view stringValue];
      [pasteboard_item setString:ns_string forType:type_name];
    }
  }
  else if([type_name isEqualToString:NSPasteboardTypeFileURL] || [type_name isEqualToString:NSPasteboardTypeURL])
  {
    // Providing a file or URL requires a string value from the view, which is then converted to a URL.
    if([main_view respondsToSelector:@selector(stringValue)])
    {
      NSString* ns_str = [main_view stringValue];
      NSURL* ns_url = [NSURL URLWithString:ns_str];
      NSData* ns_data = [NSData dataWithContentsOfURL:ns_url];
      [pasteboard_item setData:ns_data forType:type_name];
    }
  }
  else if([type_name isEqualToString:NSPasteboardTypeTabularText] || [type_name isEqualToString:NSPasteboardTypeString] || [type_name isEqualToString:NSPasteboardTypeMultipleTextSelection])
  {
    if([main_view respondsToSelector:@selector(stringValue)])
    {
      NSString* ns_string = [main_view stringValue];
      [pasteboard_item setString:ns_string forType:type_name];
    }
  }
  else if([type_name isEqualToString:NSPasteboardTypePDF])
  {
    NSRect bounds_rect = [main_view bounds];
    NSData* pdf_data = [main_view dataWithPDFInsideRect:bounds_rect];
    [pasteboard_item setData:pdf_data forType:type_name];
  }
  else if([type_name isEqualToString:NSPasteboardTypeColor])
  {
    NSColor* the_color = nil;
    if([main_view respondsToSelector:@selector(color)])
    {
      the_color = [main_view color];
    }
    else if([main_view respondsToSelector:@selector(foregroundColor)])
    {
      the_color = [main_view foregroundColor];
    }
    else if([main_view respondsToSelector:@selector(backgroundColor)])
    {
      the_color = [main_view backgroundColor];
    }

    if(the_color)
    {
      NSData* ns_data = [NSKeyedArchiver archivedDataWithRootObject:the_color];
      [pasteboard_item setData:ns_data forType:type_name];
    }
  }
}

static void cocoaSourceDragProvideDataForTypeUser(Ihandle* ih, NSPasteboard* paste_board, NSPasteboardItem* pasteboard_item, NSPasteboardType type_name, NSView* main_view, NSView* root_view, void* data_buffer, int data_size)
{
  if([type_name isEqualToString:NSPasteboardTypeTIFF] || [type_name isEqualToString:NSPasteboardTypePNG])
  {
    // For images, the user can provide either an IUP image name or raw image data.
    bool found_image = false;
    NSString* ns_string = [[[NSString alloc] initWithBytes:data_buffer length:data_size encoding:NSUTF8StringEncoding] autorelease];
    if(nil != ns_string)
    {
      // Attempt to resolve as an IUP image name first.
      const char* c_string = [ns_string UTF8String];
      NSImage* ns_image = (NSImage*)iupImageGetImage(c_string, ih, 0, NULL);
      NSBitmapImageRep* bitmap_image = nil;
      if(ns_image && [[ns_image representations] count] > 0)
      {
        bitmap_image = (NSBitmapImageRep*)[[ns_image representations] objectAtIndex:0];
      }

      if(nil != bitmap_image)
      {
        found_image = true;
        NSData* ns_data_image = nil;
        if([type_name isEqualToString:NSPasteboardTypePNG])
        {
          ns_data_image = [bitmap_image representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
        }
        else
        {
          ns_data_image = [bitmap_image TIFFRepresentation];
        }
        [pasteboard_item setData:ns_data_image forType:type_name];
      }
    }

    if(!found_image)
    {
      // If not a known image name, assume the buffer contains raw TIFF or PNG data.
      NSData* ns_data = [NSData dataWithBytes:data_buffer length:data_size];
      [pasteboard_item setData:ns_data forType:type_name];
    }
  }
  else if([type_name isEqualToString:NSPasteboardTypeRTF] || [type_name isEqualToString:NSPasteboardTypeRTFD])
  {
    NSData* ns_data = [NSData dataWithBytes:data_buffer length:data_size];
    [pasteboard_item setData:ns_data forType:type_name];
  }
  else if([type_name isEqualToString:NSPasteboardTypeHTML] || [type_name isEqualToString:NSPasteboardTypeTabularText] || [type_name isEqualToString:NSPasteboardTypeString] || [type_name isEqualToString:NSPasteboardTypeMultipleTextSelection])
  {
    NSString* ns_string = [[NSString alloc] initWithBytes:data_buffer length:data_size encoding:NSUTF8StringEncoding];
    [ns_string autorelease];
    [pasteboard_item setString:ns_string forType:type_name];
  }
  else if([type_name isEqualToString:NSPasteboardTypeFileURL])
  {
    // User provides a file path string.
    NSString* ns_string = [[NSString alloc] initWithBytes:data_buffer length:data_size encoding:NSUTF8StringEncoding];
    [ns_string autorelease];
    NSURL* ns_url = [NSURL fileURLWithPath:ns_string];
    NSData* ns_data = [NSData dataWithContentsOfURL:ns_url]; // This is inefficient but necessary for some contexts.
    [pasteboard_item setData:ns_data forType:type_name];
  }
  else if([type_name isEqualToString:NSPasteboardTypeURL])
  {
    // User provides a URL string.
    NSString* ns_string = [[NSString alloc] initWithBytes:data_buffer length:data_size encoding:NSUTF8StringEncoding];
    [ns_string autorelease];
    NSURL* ns_url = [NSURL URLWithString:ns_string];
    NSData* ns_data = [NSData dataWithContentsOfURL:ns_url];
    [pasteboard_item setData:ns_data forType:type_name];
  }
  else if([type_name isEqualToString:NSPasteboardTypePDF])
  {
    NSData* ns_data = [NSData dataWithBytes:data_buffer length:data_size];
    [pasteboard_item setData:ns_data forType:type_name];
  }
  else if([type_name isEqualToString:NSPasteboardTypeFont])
  {
    // WARNING: This relies on passing an Ihandle* as a pointer in the data buffer.
    if(data_size >= sizeof(intptr_t))
    {
      intptr_t int_ptr_for_iupfont = 0;
      memcpy(&int_ptr_for_iupfont, data_buffer, sizeof(intptr_t));
      Ihandle* iup_font = (Ihandle*)int_ptr_for_iupfont;

      IupCocoaFont* iup_cocoa_font = iupCocoaGetFont(iup_font);
      NSFont* cocoa_font = [iup_cocoa_font nativeFont];
      NSData* ns_data = [NSKeyedArchiver archivedDataWithRootObject:cocoa_font];
      [pasteboard_item setData:ns_data forType:type_name];
    }
  }
  else if([type_name isEqualToString:NSPasteboardTypeColor])
  {
    unsigned char r = 0, g = 0, b = 0, a = 255;
    int is_valid = iupStrToRGBA(data_buffer, &r, &g, &b, &a);
    if(1 != is_valid)
    {
      is_valid = iupStrToRGB(data_buffer, &r, &g, &b);
    }

    if(is_valid)
    {
      NSColor* the_color = [NSColor colorWithCalibratedRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:a/255.0];
      NSData* ns_data = [NSKeyedArchiver archivedDataWithRootObject:the_color];
      [pasteboard_item setData:ns_data forType:type_name];
    }
  }
  else // Handle as custom binary data.
  {
    NSData* ns_data = [NSData dataWithBytes:data_buffer length:data_size];
    [pasteboard_item setData:ns_data forType:type_name];
  }

}

- (bool) usesFilePromise
{
  Ihandle* ih = [self ihandle];
  if(IupGetCallback(ih, "DRAGFILECREATENAME_CB"))
  {
    return true;
  }

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
  return (IupGetCallback(ih, "DRAGFILECREATE_CB") != NULL);
}

- (void) pasteboard:(nullable NSPasteboard*)paste_board item:(NSPasteboardItem*)pasteboard_item provideDataForType:(NSPasteboardType)type_name
{
  Ihandle* ih = [self ihandle];
  if(nil == ih)
  {
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
      int ret_val = drag_data_callback(ih, (char*)c_type_name, data_buffer, data_size);

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

        free(data_buffer);
        data_buffer = NULL;
      }
    }
  }

  if(use_default_implementation)
  {
    NSView* main_view = [self mainView];
    NSView* root_view = [self rootView];
    cocoaSourceDragProvideDataForTypeDefault(ih, paste_board, pasteboard_item, type_name, main_view, root_view);
  }
}

- (NSPasteboardItem*) defaultPasteboardItem
{
  IupSourceDragAssociatedData* drag_source_data = self;
  NSArray* registered_types = [drag_source_data dragRegisteredTypes];

  if([registered_types count] > 0)
  {
    // NSFilePromiseProvider is used to generate file data on-demand, for example,
    // when dragging an in-memory image to the Finder to create a new file.
    bool wants_file_promise = [drag_source_data usesFilePromise];
    id return_item = nil;
    if(wants_file_promise)
    {
      NSFilePromiseProvider* promise_provider = [[NSFilePromiseProvider alloc] initWithFileType:(NSString*)kUTTypeData delegate:self];
      [promise_provider autorelease];
      [promise_provider setDataProvider:drag_source_data forTypes:registered_types];
      return_item = promise_provider;
    }
    else
    {
      NSPasteboardItem* pasteboard_item = [[NSPasteboardItem alloc] init];
      [pasteboard_item autorelease];
      [pasteboard_item setDataProvider:drag_source_data forTypes:registered_types];
      return_item = pasteboard_item;
    }
    return return_item;
  }
  return nil;
}

- (NSDraggingItem*) defaultDraggingItem
{
  IupSourceDragAssociatedData* drag_source_data = self;
  NSView* main_view = [self mainView];
  Ihandle* ih = [self ihandle];

  NSDraggingItem* dragging_item = nil;
  NSPasteboardItem* pasteboard_item = [self defaultPasteboardItem];

  if(nil != pasteboard_item)
  {
    dragging_item = [[NSDraggingItem alloc] initWithPasteboardWriter:pasteboard_item];
    [dragging_item autorelease];

    NSImage* drag_image = nil;

    bool is_move = iupAttribGetBoolean(ih, "DRAGSOURCEMOVE");
    char* drag_cursor_name = NULL;

    if (!is_move)
    {
      drag_cursor_name = iupAttribGet(ih, "DRAGCURSORCOPY");
    }

    if (!drag_cursor_name)
    {
      drag_cursor_name = iupAttribGet(ih, "DRAGCURSOR");
    }

    if(drag_cursor_name)
    {
      drag_image = (NSImage*)iupImageGetImage(drag_cursor_name, ih, 0, NULL);
    }

    if(drag_image)
    {
      NSRect image_rect = NSMakeRect(0, 0, [drag_image size].width, [drag_image size].height);
      [dragging_item setDraggingFrame:image_rect contents:drag_image];
    }
    else if([drag_source_data useAutoGenerateDragImage])
    {
      NSRect bounds_rect = [main_view bounds];
      NSData* pdf_data = [main_view dataWithPDFInsideRect:bounds_rect];
      NSImage* image_data = [[NSImage alloc] initWithData:pdf_data];
      [image_data autorelease];

      [dragging_item setDraggingFrame:bounds_rect contents:image_data];
    }
  }
  return dragging_item;
}

- (NSString *) filePromiseProvider:(NSFilePromiseProvider*)file_promise_provider fileNameForType:(NSString*)file_type
{
  Ihandle* ih = [self ihandle];
  IFnssi file_promise_name_callback = (IFnssi)IupGetCallback(ih, "DRAGFILECREATENAME_CB");
  if(NULL == file_promise_name_callback)
  {
    return [self defaultFilePromiseName];
  }

  char file_buffer[PATH_MAX] = { '\0' };
  const char* c_file_type = [file_type UTF8String];
  int ret_val = file_promise_name_callback(ih, (char*)c_file_type, file_buffer, PATH_MAX);

  if(IUP_CONTINUE == ret_val)
  {
    return [self defaultFilePromiseName];
  }

  return [NSString stringWithUTF8String:file_buffer];
}

static bool cocoaSourceDragDoDefaultFileCreate(NSFilePromiseProvider* file_promise_provider, NSURL* write_url)
{
  // Convention: If userInfo contains data, we can use it to write the file.
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
      NSString* ns_string = iupCocoaGetFontStringFromFont(ns_font);
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
  Ihandle* ih = [self ihandle];
  IFns file_create_callback = (IFns)IupGetCallback(ih, "DRAGFILECREATE_CB");

  if(NULL == file_create_callback)
  {
    cocoaSourceDragDoDefaultFileCreate(file_promise_provider, write_url);
    completion_handler(nil);
    return;
  }

  NSString* file_url = [write_url path];
  const char* file_path = [file_url fileSystemRepresentation];
  int ret_val = file_create_callback(ih, (char*)file_path);

  if(IUP_CONTINUE == ret_val)
  {
    cocoaSourceDragDoDefaultFileCreate(file_promise_provider, write_url);
  }

  completion_handler(nil);
}

- (void) draggingSession:(NSDraggingSession*)dragging_session willBeginAtPoint:(NSPoint)screen_point
{
  // This method is part of the NSDraggingSource protocol and is called when a drag session begins.
  // At this point, the drag has already been initiated and any pre-drag callbacks
  // (like DRAGBEGIN_CB) should have been called before beginDraggingSessionWithItems.
  // No additional setup is needed here.

  (void)dragging_session;
  (void)screen_point;
}

- (void) draggingSession:(NSDraggingSession*)dragging_session endedAtPoint:(NSPoint)screen_point operation:(NSDragOperation)drag_operation
{
  Ihandle* ih = [self ihandle];
  IFni call_back = (IFni)IupGetCallback(ih, "DRAGEND_CB");
  if(NULL != call_back)
  {
    // The action parameter signifies the result of the drag operation.
    // 1 = move, 0 = copy, -1 = drag failed or was aborted.
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
      // Unknown or combined operation, report as is.
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

static int cocoaTargetDropSetDropTypesAttrib(Ihandle* ih, const char* value)
{
  IupTargetDropAssociatedData* drag_drop_data = cocoaTargetDropGetAssociatedData(ih);
  id the_object = ih->handle;

  if(NULL != value)
  {
    NSMutableArray* array_of_types = [NSMutableArray array];
    char value_copy[256];
    char value_temp1[256];
    char value_temp2[256];

    strcpy(value_copy, value);
    while (iupStrToStrStr(value_copy, value_temp1, value_temp2, ',') > 0)
    {
      NSString* type_string = [NSString stringWithUTF8String:value_temp1];
      [array_of_types addObject:type_string];

      if (iupStrEqualNoCase(value_temp2, value_temp1))
        break;

      strcpy(value_copy, value_temp2);
    }

    if ([array_of_types count] > 0)
    {
      [drag_drop_data setDropRegisteredTypes:array_of_types];

      if([drag_drop_data isDropTargetEnabled])
      {
        if([the_object respondsToSelector:@selector(registerForDraggedTypes:)])
        {
          [the_object registerForDraggedTypes:array_of_types];
        }
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
  return 1;
}

static int cocoaTargetDropSetDropTargetAttrib(Ihandle* ih, const char* value)
{
  id the_object = ih->handle;
  IupTargetDropAssociatedData* drag_drop_data = cocoaTargetDropGetAssociatedData(ih);
  if(iupStrBoolean(value))
  {
    [drag_drop_data setDropTargetEnabled:true];
    if([the_object respondsToSelector:@selector(registerForDraggedTypes:)])
    {
      NSArray* array_of_types = [drag_drop_data dropRegisteredTypes];
      if (array_of_types && [array_of_types count] > 0)
      {
        [the_object registerForDraggedTypes:array_of_types];
      }
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
  return 1;
}

static int cocoaSetDropFilesTargetAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
  {
    // Set DROPTYPES to handle files.
    cocoaTargetDropSetDropTypesAttrib(ih, (const char*)NSPasteboardTypeFileURL.UTF8String);
    // Enable DROPTARGET.
    cocoaTargetDropSetDropTargetAttrib(ih, "YES");
  }
  else
  {
    cocoaTargetDropSetDropTargetAttrib(ih, "NO");
  }
  return 1;
}

static int cocoaSourceDropSetPasteFromPasteboardAttrib(Ihandle* ih, const char* value)
{
  if(!iupStrBoolean(value))
  {
    return 0;
  }

  NSPasteboard* paste_board = [NSPasteboard generalPasteboard];
  NSPoint drop_point = {0, 0}; // Paste operations have no specific drop point.

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

static int cocoaSourceDragSetDragSourceAttrib(Ihandle* ih, const char* value)
{
  IupSourceDragAssociatedData* drag_drop_data = cocoaSourceDragGetAssociatedData(ih);
  if(iupStrBoolean(value))
  {
    [drag_drop_data setDragSourceEnabled:true];
  }
  else
  {
    [drag_drop_data setDragSourceEnabled:false];
  }
  return 0;
}

static int cocoaSourceDragSetDragTypesAttrib(Ihandle* ih, const char* value)
{
  IupSourceDragAssociatedData* drag_drop_data = cocoaSourceDragGetAssociatedData(ih);
  if(value)
  {
    NSMutableArray* array_of_types = [NSMutableArray array];
    char value_copy[256];
    char value_temp1[256];
    char value_temp2[256];

    strcpy(value_copy, value);
    while (iupStrToStrStr(value_copy, value_temp1, value_temp2, ',') > 0)
    {
      NSString* type_string = [NSString stringWithUTF8String:value_temp1];

      if ([type_string isEqualToString:@"FILEPROMISE"])
      {
        NSArray* file_promise_types = [NSFilePromiseReceiver readableDraggedTypes];
        [array_of_types addObjectsFromArray:file_promise_types];
      }
      else
      {
        [array_of_types addObject:type_string];
      }

      if (iupStrEqualNoCase(value_temp2, value_temp1))
        break;

      strcpy(value_copy, value_temp2);
    }

    if ([array_of_types count] > 0)
    {
      [drag_drop_data setDragRegisteredTypes:array_of_types];
    }
  }
  else
  {
    [drag_drop_data setDragRegisteredTypes:nil];
  }
  return 1;
}

static int cocoaSourceDragSetDragAutoImageAttrib(Ihandle* ih, const char* value)
{
  IupSourceDragAssociatedData* drag_drop_data = cocoaSourceDragGetAssociatedData(ih);
  [drag_drop_data setUseAutoGenerateDragImage:iupStrBoolean(value)];
  return 0;
}

char* cocoaSourceDragGetDragAutoImageAttrib(Ihandle* ih)
{
  IupSourceDragAssociatedData* drag_drop_data = cocoaSourceDragGetAssociatedData(ih);
  return iupStrReturnBoolean([drag_drop_data useAutoGenerateDragImage]);
}

static int cocoaSourceDragSetAutoInitiateAttrib(Ihandle* ih, const char* value)
{
  IupSourceDragAssociatedData* drag_drop_data = cocoaSourceDragGetAssociatedData(ih);
  [drag_drop_data setUseAutoBeginDrag:iupStrBoolean(value)];
  return 0;
}

char* cocoaSourceDragGetAutoInitiateAttrib(Ihandle* ih)
{
  IupSourceDragAssociatedData* drag_drop_data = cocoaSourceDragGetAssociatedData(ih);
  return iupStrReturnBoolean([drag_drop_data useAutoBeginDrag]);
}

static int cocoaSourceDragSetDragStartAttrib(Ihandle* ih, const char* value)
{
  int x, y;

  if (!iupStrToIntInt(value, &x, &y, ','))
    return 0;

  IupSourceDragAssociatedData* drag_source_data = cocoaSourceDragGetAssociatedData(ih);
  if (![drag_source_data isDragSourceEnabled])
    return 0;

  NSView* main_view = [drag_source_data mainView];
  if (!main_view)
    return 0;

  IFnii cbDragBegin = (IFnii)IupGetCallback(ih, "DRAGBEGIN_CB");
  if (cbDragBegin)
  {
    if (cbDragBegin(ih, x, y) == IUP_IGNORE)
      return 0;
  }

  NSDraggingItem* dragging_item = [drag_source_data defaultDraggingItem];
  if (!dragging_item)
    return 0;

  NSPoint view_point = NSMakePoint(x, y);

  if (![main_view isFlipped])
  {
    NSRect view_bounds = [main_view bounds];
    view_point.y = view_bounds.size.height - y;
  }

  NSPoint window_point = [main_view convertPoint:view_point toView:nil];

  NSEvent* the_event = [[NSApplication sharedApplication] currentEvent];

  if (!the_event ||
      (the_event.type != NSEventTypeLeftMouseDown &&
       the_event.type != NSEventTypeLeftMouseDragged &&
       the_event.type != NSEventTypeRightMouseDown &&
       the_event.type != NSEventTypeRightMouseDragged))
  {
    NSWindow* window = [main_view window];
    if (!window)
      return 0;

    NSTimeInterval timestamp = [[NSProcessInfo processInfo] systemUptime];

    the_event = [NSEvent mouseEventWithType:NSEventTypeLeftMouseDragged
                                   location:window_point
                              modifierFlags:0
                                  timestamp:timestamp
                               windowNumber:[window windowNumber]
                                    context:nil
                                eventNumber:0
                                 clickCount:1
                                   pressure:1.0];
  }

  [main_view beginDraggingSessionWithItems:@[dragging_item] event:the_event source:drag_source_data];

  return 0;
}

static int cocoaSourceDragSetCopyToPasteboardAttrib(Ihandle* ih, const char* value)
{
  if(!iupStrBoolean(value))
  {
    return 0;
  }

  IupSourceDragAssociatedData* drag_source_data = cocoaSourceDragGetAssociatedData(ih);
  NSArray* registered_types = [drag_source_data dragRegisteredTypes];

  if (!registered_types || [registered_types count] == 0)
  {
    return 0;
  }

  NSPasteboard* paste_board = [NSPasteboard generalPasteboard];
  NSPasteboardItem* pasteboard_item = [[NSPasteboardItem alloc] init];
  [pasteboard_item autorelease];

  [paste_board clearContents];

  for (NSString* type_name in registered_types)
  {
    [drag_source_data pasteboard:paste_board item:pasteboard_item provideDataForType:type_name];
  }

  [paste_board writeObjects:@[pasteboard_item]];
  return 0;
}

void iupCocoaDestroyDragDrop(Ihandle* ih)
{
  if (!ih || !ih->handle)
    return;

  id the_object = ih->handle;

  IupTargetDropAssociatedData* drop_data = cocoaTargetDropGetAssociatedData(ih);
  if (drop_data && [drop_data isDropTargetEnabled])
  {
    if ([the_object respondsToSelector:@selector(unregisterDraggedTypes)])
    {
      [the_object unregisterDraggedTypes];
    }
  }

  cocoaTargetDropDestroyAssociatedData(ih);
  cocoaSourceDragDestroyAssociatedData(ih);
}

void iupdrvRegisterDragDropAttrib(Iclass* ic)
{
  iupClassRegisterCallback(ic, "DRAGBEGIN_CB", "ii");
  iupClassRegisterCallback(ic, "DRAGDATA_CB", "sVi");
  iupClassRegisterCallback(ic, "DRAGDATASIZE_CB", "s");
  iupClassRegisterCallback(ic, "DRAGEND_CB", "i");
  iupClassRegisterCallback(ic, "DRAGFILECREATE_CB", "s");
  iupClassRegisterCallback(ic, "DRAGFILECREATENAME_CB", "ssi");

  iupClassRegisterCallback(ic, "DROPFILES_CB", "siii");
  iupClassRegisterCallback(ic, "DROPDATA_CB", "sViii");
  iupClassRegisterCallback(ic, "DROPMOTION_CB", "iis");

  iupClassRegisterAttribute(ic, "DRAGAUTOIMAGE", cocoaSourceDragGetDragAutoImageAttrib, cocoaSourceDragSetDragAutoImageAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGAUTOINITIATE", cocoaSourceDragGetAutoInitiateAttrib, cocoaSourceDragSetAutoInitiateAttrib, "YES", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGCURSOR", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGCURSORCOPY", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGSTART", NULL, cocoaSourceDragSetDragStartAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGSOURCE", NULL, cocoaSourceDragSetDragSourceAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGSOURCEMOVE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGTYPES", NULL, cocoaSourceDragSetDragTypesAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DRAGDROP", NULL, cocoaSetDropFilesTargetAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPFILESTARGET", NULL, cocoaSetDropFilesTargetAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPTARGET", NULL, cocoaTargetDropSetDropTargetAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPTYPES", NULL, cocoaTargetDropSetDropTypesAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DRAGCOPY", NULL, cocoaSourceDragSetCopyToPasteboardAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPPASTE", NULL, cocoaSourceDropSetPasteFromPasteboardAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
}
