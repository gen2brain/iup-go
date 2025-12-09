/** \file
 * \brief Clipboard for the Cocoa Driver.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#import <Cocoa/Cocoa.h>

#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iupcocoa_drv.h"



static int cocoaClipboardSetTextAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  NSPasteboard* paste_board = [NSPasteboard generalPasteboard];

  if (!value)
  {
    [paste_board clearContents];
    return 0;
  }

  NSString* ns_string = [NSString stringWithUTF8String:value];
  if (!ns_string)
  {
    ns_string = @"";
  }

  [paste_board clearContents];
  [paste_board writeObjects:@[ns_string]];

  return 0;
}

static char* cocoaClipboardGetTextAttrib(Ihandle* ih)
{
  (void)ih;
  NSPasteboard* paste_board = [NSPasteboard generalPasteboard];
  NSString* type_available = [paste_board availableTypeFromArray:@[NSPasteboardTypeString]];

  if (type_available)
  {
    NSString* pasteboard_string = [paste_board stringForType:NSPasteboardTypeString];
    return iupStrReturnStr([pasteboard_string UTF8String]);
  }

  return NULL;
}

static char* cocoaClipboardGetTextAvailableAttrib(Ihandle* ih)
{
  (void)ih;
  NSPasteboard* paste_board = [NSPasteboard generalPasteboard];
  NSString* type_available = [paste_board availableTypeFromArray:@[NSPasteboardTypeString]];
  return iupStrReturnBoolean(type_available != nil);
}

static int cocoaClipboardSetImageAttrib(Ihandle* ih, const char* value)
{
  NSPasteboard* paste_board = [NSPasteboard generalPasteboard];

  if (!value)
  {
    [paste_board clearContents];
    return 0;
  }

  NSImage* ns_image = (NSImage*)iupImageGetImage(value, ih, 0, NULL);
  if (ns_image)
  {
    [paste_board clearContents];
    [paste_board writeObjects:@[ns_image]];
  }

  return 0;
}

static int cocoaClipboardSetNativeImageAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  NSPasteboard* paste_board = [NSPasteboard generalPasteboard];
  [paste_board clearContents];

  if (value)
  {
    NSImage* ns_image = (NSImage*)value;
    [paste_board writeObjects:@[ns_image]];
  }

  return 0;
}

static char* cocoaClipboardGetNativeImageAttrib(Ihandle* ih)
{
  (void)ih;
  NSPasteboard* paste_board = [NSPasteboard generalPasteboard];
  NSArray* valid_classes = @[[NSImage class]];
  NSDictionary* options_dict = @{};

  if ([paste_board canReadObjectForClasses:valid_classes options:options_dict])
  {
    NSArray* array_of_objects = [paste_board readObjectsForClasses:valid_classes options:options_dict];
    if ([array_of_objects count] > 0)
    {
      NSImage* ns_image = (NSImage*)[array_of_objects firstObject];
      return (char*)ns_image;
    }
  }

  return NULL;
}

static char* cocoaClipboardGetImageAvailableAttrib(Ihandle* ih)
{
  (void)ih;
  NSPasteboard* paste_board = [NSPasteboard generalPasteboard];
  NSString* type_available = [paste_board availableTypeFromArray:[NSImage imageTypes]];
  return iupStrReturnBoolean(type_available != nil);
}

static int cocoaClipboardSetNativeVectorImageAttrib(Ihandle *ih, const char *value)
{
  NSPasteboard* paste_board = [NSPasteboard generalPasteboard];

  if (!value)
  {
    [paste_board clearContents];
    return 0;
  }

  int data_size = iupAttribGetInt(ih, "FORMATDATASIZE");
  if (data_size > 0)
  {
    [paste_board clearContents];
    NSData* pdf_data = [NSData dataWithBytes:value length:data_size];
    [paste_board setData:pdf_data forType:NSPasteboardTypePDF];
  }

  return 0;
}

static char* cocoaClipboardGetNativeVectorImageAttrib(Ihandle *ih)
{
  NSPasteboard* paste_board = [NSPasteboard generalPasteboard];
  NSData* pdf_data = [paste_board dataForType:NSPasteboardTypePDF];

  if (!pdf_data)
  {
    iupAttribSetInt(ih, "FORMATDATASIZE", 0);
    return NULL;
  }

  NSUInteger data_size = [pdf_data length];
  if (data_size == 0)
  {
    iupAttribSetInt(ih, "FORMATDATASIZE", 0);
    return NULL;
  }

  void* data_buffer = iupStrGetMemory((int)data_size);
  memcpy(data_buffer, [pdf_data bytes], data_size);

  iupAttribSetInt(ih, "FORMATDATASIZE", (int)data_size);
  return (char*)data_buffer;
}

static char* cocoaClipboardGetPDFAvailableAttrib(Ihandle* ih)
{
  (void)ih;
  NSPasteboard* paste_board = [NSPasteboard generalPasteboard];
  NSString* type_available = [paste_board availableTypeFromArray:@[NSPasteboardTypePDF]];
  return iupStrReturnBoolean(type_available != nil);
}

static int cocoaClipboardSetSaveNativeVectorImageAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  if (!value)
  {
    return 0;
  }

  NSPasteboard* paste_board = [NSPasteboard generalPasteboard];
  NSData* pdf_data = [paste_board dataForType:NSPasteboardTypePDF];

  if (pdf_data)
  {
    NSString* path = [NSString stringWithUTF8String:value];
    [pdf_data writeToFile:path atomically:NO];
  }

  return 0;
}

static int cocoaClipboardSetAddFormatAttrib(Ihandle* ih, const char* value)
{
  (void)ih;

  if (!value)
    return 0;

  /* Custom pasteboard types on macOS don't require explicit registration. */
  /* They are identified by strings, typically Uniform Type Identifiers (UTIs). */
  /* Validation occurs when the format is used in FORMATDATA operations. */

  return 0;
}

static int cocoaClipboardSetFormatDataAttrib(Ihandle* ih, const char* value)
{
  if (!ih)
  {
    return 0;
  }

  NSPasteboard* paste_board = [NSPasteboard generalPasteboard];

  if (!value)
  {
    [paste_board clearContents];
    return 0;
  }

  char* format = iupAttribGetStr(ih, "FORMAT");
  if (!format)
  {
    return 0;
  }

  int data_size = iupAttribGetInt(ih, "FORMATDATASIZE");
  if (data_size <= 0)
  {
    return 0;
  }

  NSString* format_string = [NSString stringWithUTF8String:format];
  if (!format_string)
  {
    return 0;
  }

  NSData* ns_data = [[NSData alloc] initWithBytes:value length:data_size];
  [paste_board clearContents];
  [paste_board setData:ns_data forType:format_string];
  [ns_data release];

  return 0;
}

static char* cocoaClipboardGetFormatDataAttrib(Ihandle* ih)
{
  if (!ih)
  {
    return NULL;
  }

  char* format = iupAttribGetStr(ih, "FORMAT");
  if (!format)
  {
    return NULL;
  }

  NSString* format_string = [NSString stringWithUTF8String:format];
  if (!format_string)
  {
    return NULL;
  }

  NSPasteboard* paste_board = [NSPasteboard generalPasteboard];
  NSData* ns_data = [paste_board dataForType:format_string];

  if (!ns_data)
  {
    iupAttribSetInt(ih, "FORMATDATASIZE", 0);
    return NULL;
  }

  NSUInteger data_size = [ns_data length];
  if (data_size == 0)
  {
    iupAttribSetInt(ih, "FORMATDATASIZE", 0);
    return NULL;
  }

  void* data_buffer = iupStrGetMemory((int)data_size + 1);
  memcpy(data_buffer, [ns_data bytes], data_size);

  iupAttribSetInt(ih, "FORMATDATASIZE", (int)data_size);
  return (char*)data_buffer;
}

static char* cocoaClipboardGetFormatAvailableAttrib(Ihandle* ih)
{
  if (!ih)
  {
    return NULL;
  }

  char* format = iupAttribGetStr(ih, "FORMAT");
  if (!format)
  {
    return iupStrReturnBoolean(0);
  }

  NSString* format_string = [NSString stringWithUTF8String:format];
  if (!format_string)
  {
    return iupStrReturnBoolean(0);
  }

  NSPasteboard* paste_board = [NSPasteboard generalPasteboard];
  NSString* type_available = [paste_board availableTypeFromArray:@[format_string]];
  return iupStrReturnBoolean(type_available != nil);
}

static int cocoaClipboardSetFormatDataStringAttrib(Ihandle* ih, const char* value)
{
  if (value)
  {
    int len = (int)strlen(value);
    iupAttribSetInt(ih, "FORMATDATASIZE", len + 1); /* include the terminator */
    return cocoaClipboardSetFormatDataAttrib(ih, value);
  }
  else
  {
    return cocoaClipboardSetFormatDataAttrib(ih, NULL);
  }
}

static char* cocoaClipboardGetFormatDataStringAttrib(Ihandle* ih)
{
  char* data = cocoaClipboardGetFormatDataAttrib(ih);
  if (!data)
  {
    return NULL;
  }

  int size = iupAttribGetInt(ih, "FORMATDATASIZE");
  data[size] = 0;
  return iupStrReturnStr(data);
}

/******************************************************************************/

IUP_API Ihandle* IupClipboard(void)
{
  return IupCreate("clipboard");
}

Iclass* iupClipboardNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "clipboard";
  ic->format = NULL;
  ic->nativetype = IUP_TYPEOTHER;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 0;

  iupClassRegisterAttribute(ic, "TEXT", cocoaClipboardGetTextAttrib, cocoaClipboardSetTextAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TEXTAVAILABLE", cocoaClipboardGetTextAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "IMAGE", NULL, cocoaClipboardSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NATIVEIMAGE", cocoaClipboardGetNativeImageAttrib, cocoaClipboardSetNativeImageAttrib, NULL, NULL, IUPAF_NO_STRING|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEAVAILABLE", cocoaClipboardGetImageAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "NATIVEVECTORIMAGE", cocoaClipboardGetNativeVectorImageAttrib, cocoaClipboardSetNativeVectorImageAttrib, NULL, NULL, IUPAF_NO_STRING|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PDFAVAILABLE", cocoaClipboardGetPDFAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SAVENATIVEVECTORIMAGE", NULL, cocoaClipboardSetSaveNativeVectorImageAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ADDFORMAT", NULL, cocoaClipboardSetAddFormatAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMAT", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATAVAILABLE", cocoaClipboardGetFormatAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATA", cocoaClipboardGetFormatDataAttrib, cocoaClipboardSetFormatDataAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATASTRING", cocoaClipboardGetFormatDataStringAttrib, cocoaClipboardSetFormatDataStringAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATASIZE", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  return ic;
}
