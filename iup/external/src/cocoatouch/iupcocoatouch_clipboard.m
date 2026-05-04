/** \file
 * \brief Clipboard (iOS UIKit, UIPasteboard)
 *
 * See Copyright Notice in "iup.h"
 */

#import <UIKit/UIKit.h>

#include <string.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"

#include "iupcocoatouch_drv.h"


static UIPasteboard* cocoaTouchClipboardPB(void)
{
	return [UIPasteboard generalPasteboard];
}

static void cocoaTouchClipboardClear(UIPasteboard* pb)
{
	/* no -clearContents; setting items to @[] clears every type */
	[pb setItems:@[]];
}

static int cocoaTouchClipboardSetTextAttrib(Ihandle* ih, const char* value)
{
	(void)ih;
	UIPasteboard* pb = cocoaTouchClipboardPB();

	if (!value)
	{
		cocoaTouchClipboardClear(pb);
		return 0;
	}

	NSString* ns_string = [NSString stringWithUTF8String:value];
	[pb setString:ns_string ? ns_string : @""];
	return 0;
}

static char* cocoaTouchClipboardGetTextAttrib(Ihandle* ih)
{
	(void)ih;
	UIPasteboard* pb = cocoaTouchClipboardPB();
	if (![pb hasStrings])
	{
		return NULL;
	}
	NSString* s = [pb string];
	return s ? iupStrReturnStr([s UTF8String]) : NULL;
}

static char* cocoaTouchClipboardGetTextAvailableAttrib(Ihandle* ih)
{
	(void)ih;
	return iupStrReturnBoolean([cocoaTouchClipboardPB() hasStrings] ? 1 : 0);
}

static int cocoaTouchClipboardSetImageAttrib(Ihandle* ih, const char* value)
{
	UIPasteboard* pb = cocoaTouchClipboardPB();

	if (!value)
	{
		cocoaTouchClipboardClear(pb);
		return 0;
	}

	UIImage* image = (UIImage*)iupImageGetImage(value, ih, 0, NULL);
	if (image)
	{
		[pb setImage:image];
	}
	return 0;
}

static int cocoaTouchClipboardSetNativeImageAttrib(Ihandle* ih, const char* value)
{
	(void)ih;
	UIPasteboard* pb = cocoaTouchClipboardPB();

	if (!value)
	{
		cocoaTouchClipboardClear(pb);
		return 0;
	}

	UIImage* image = (UIImage*)value;
	if ([image isKindOfClass:[UIImage class]])
	{
		[pb setImage:image];
	}
	return 0;
}

static char* cocoaTouchClipboardGetNativeImageAttrib(Ihandle* ih)
{
	(void)ih;
	UIPasteboard* pb = cocoaTouchClipboardPB();
	if (![pb hasImages])
	{
		return NULL;
	}
	/* NATIVEIMAGE contract: caller releases, so balance the autoreleased -image */
	UIImage* image = [pb image];
	return image ? (char*)[image retain] : NULL;
}

static char* cocoaTouchClipboardGetImageAvailableAttrib(Ihandle* ih)
{
	(void)ih;
	return iupStrReturnBoolean([cocoaTouchClipboardPB() hasImages] ? 1 : 0);
}

static int cocoaTouchClipboardSetNativeVectorImageAttrib(Ihandle* ih, const char* value)
{
	UIPasteboard* pb = cocoaTouchClipboardPB();

	if (!value)
	{
		cocoaTouchClipboardClear(pb);
		return 0;
	}

	int data_size = iupAttribGetInt(ih, "FORMATDATASIZE");
	if (data_size > 0)
	{
		NSData* pdf = [NSData dataWithBytes:value length:(NSUInteger)data_size];
		[pb setData:pdf forPasteboardType:IUPCOCOATOUCH_UTI_PDF];
	}
	return 0;
}

static char* cocoaTouchClipboardGetNativeVectorImageAttrib(Ihandle* ih)
{
	NSData* pdf = [cocoaTouchClipboardPB() dataForPasteboardType:IUPCOCOATOUCH_UTI_PDF];
	if (!pdf || [pdf length] == 0)
	{
		iupAttribSetInt(ih, "FORMATDATASIZE", 0);
		return NULL;
	}

	NSUInteger size = [pdf length];
	void* buf = iupStrGetMemory((int)size);
	memcpy(buf, [pdf bytes], size);
	iupAttribSetInt(ih, "FORMATDATASIZE", (int)size);
	return (char*)buf;
}

static char* cocoaTouchClipboardGetPDFAvailableAttrib(Ihandle* ih)
{
	(void)ih;
	UIPasteboard* pb = cocoaTouchClipboardPB();
	BOOL has = [pb containsPasteboardTypes:@[IUPCOCOATOUCH_UTI_PDF]];
	return iupStrReturnBoolean(has ? 1 : 0);
}

static int cocoaTouchClipboardSetSaveNativeVectorImageAttrib(Ihandle* ih, const char* value)
{
	(void)ih;
	if (!value)
	{
		return 0;
	}

	NSData* pdf = [cocoaTouchClipboardPB() dataForPasteboardType:IUPCOCOATOUCH_UTI_PDF];
	if (pdf)
	{
		[pdf writeToFile:[NSString stringWithUTF8String:value] atomically:NO];
	}
	return 0;
}

static int cocoaTouchClipboardSetAddFormatAttrib(Ihandle* ih, const char* value)
{
	(void)ih; (void)value;
	/* free-form UTIs need no registration; kept for IUP API compat */
	return 0;
}

static BOOL cocoaTouchClipboardIsValidUTI(NSString* s)
{
	NSUInteger n = [s length];
	if (n == 0 || n > 256) return NO;
	NSCharacterSet* allowed = [NSCharacterSet characterSetWithCharactersInString:
		@"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-_+"];
	for (NSUInteger i = 0; i < n; i++)
	{
		if (![allowed characterIsMember:[s characterAtIndex:i]]) return NO;
	}
	return YES;
}

static NSString* cocoaTouchClipboardFormatString(Ihandle* ih)
{
	const char* format = iupAttribGetStr(ih, "FORMAT");
	if (!format || !*format)
	{
		return nil;
	}
	NSString* s = [NSString stringWithUTF8String:format];
	if (!s || !cocoaTouchClipboardIsValidUTI(s)) return nil;
	return s;
}

static int cocoaTouchClipboardSetFormatDataAttrib(Ihandle* ih, const char* value)
{
	UIPasteboard* pb = cocoaTouchClipboardPB();

	if (!value)
	{
		cocoaTouchClipboardClear(pb);
		return 0;
	}

	NSString* format = cocoaTouchClipboardFormatString(ih);
	if (!format)
	{
		return 0;
	}

	int data_size = iupAttribGetInt(ih, "FORMATDATASIZE");
	if (data_size <= 0)
	{
		return 0;
	}

	NSData* data = [NSData dataWithBytes:value length:(NSUInteger)data_size];
	[pb setData:data forPasteboardType:format];
	return 0;
}

static char* cocoaTouchClipboardGetFormatDataAttrib(Ihandle* ih)
{
	NSString* format = cocoaTouchClipboardFormatString(ih);
	if (!format)
	{
		return NULL;
	}

	NSData* data = [cocoaTouchClipboardPB() dataForPasteboardType:format];
	if (!data || [data length] == 0)
	{
		iupAttribSetInt(ih, "FORMATDATASIZE", 0);
		return NULL;
	}

	NSUInteger size = [data length];
	void* buf = iupStrGetMemory((int)size + 1);
	memcpy(buf, [data bytes], size);
	iupAttribSetInt(ih, "FORMATDATASIZE", (int)size);
	return (char*)buf;
}

static char* cocoaTouchClipboardGetFormatAvailableAttrib(Ihandle* ih)
{
	NSString* format = cocoaTouchClipboardFormatString(ih);
	if (!format)
	{
		return iupStrReturnBoolean(0);
	}
	BOOL has = [cocoaTouchClipboardPB() containsPasteboardTypes:@[format]];
	return iupStrReturnBoolean(has ? 1 : 0);
}

static int cocoaTouchClipboardSetFormatDataStringAttrib(Ihandle* ih, const char* value)
{
	if (value)
	{
		iupAttribSetInt(ih, "FORMATDATASIZE", (int)strlen(value) + 1);
	}
	return cocoaTouchClipboardSetFormatDataAttrib(ih, value);
}

static char* cocoaTouchClipboardGetFormatDataStringAttrib(Ihandle* ih)
{
	char* data = cocoaTouchClipboardGetFormatDataAttrib(ih);
	if (!data)
	{
		return NULL;
	}
	int size = iupAttribGetInt(ih, "FORMATDATASIZE");
	data[size] = 0;
	return iupStrReturnStr(data);
}

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

	iupClassRegisterAttribute(ic, "TEXT", cocoaTouchClipboardGetTextAttrib, cocoaTouchClipboardSetTextAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "TEXTAVAILABLE", cocoaTouchClipboardGetTextAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

	iupClassRegisterAttribute(ic, "IMAGE", NULL, cocoaTouchClipboardSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "NATIVEIMAGE", cocoaTouchClipboardGetNativeImageAttrib, cocoaTouchClipboardSetNativeImageAttrib, NULL, NULL, IUPAF_NO_STRING|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "IMAGEAVAILABLE", cocoaTouchClipboardGetImageAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

	iupClassRegisterAttribute(ic, "NATIVEVECTORIMAGE", cocoaTouchClipboardGetNativeVectorImageAttrib, cocoaTouchClipboardSetNativeVectorImageAttrib, NULL, NULL, IUPAF_NO_STRING|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "PDFAVAILABLE", cocoaTouchClipboardGetPDFAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "SAVENATIVEVECTORIMAGE", NULL, cocoaTouchClipboardSetSaveNativeVectorImageAttrib,NULL, NULL, IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

	iupClassRegisterAttribute(ic, "ADDFORMAT", NULL, cocoaTouchClipboardSetAddFormatAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "FORMAT", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "FORMATAVAILABLE", cocoaTouchClipboardGetFormatAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "FORMATDATA", cocoaTouchClipboardGetFormatDataAttrib, cocoaTouchClipboardSetFormatDataAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "FORMATDATASTRING", cocoaTouchClipboardGetFormatDataStringAttrib, cocoaTouchClipboardSetFormatDataStringAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "FORMATDATASIZE", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

	return ic;
}
