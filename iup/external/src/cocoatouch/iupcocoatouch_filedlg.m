/** \file
 * \brief IupFileDlg (iOS UIKit)
 *
 * See Copyright Notice in "iup.h"
 */

#import <UIKit/UIKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import <CoreFoundation/CoreFoundation.h>
#import <objc/runtime.h>

#include <string.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_str.h"

#include "iupcocoatouch_drv.h"

#import "IupViewController.h"


#define IUPCOCOATOUCH_FILEDLG_PENDING (-1)
#define IUPCOCOATOUCH_FILEDLG_PICKED  (1)
#define IUPCOCOATOUCH_FILEDLG_CANCEL  (0)

@interface IupCocoaTouchFileDlgDelegate : NSObject <UIDocumentPickerDelegate>
@property(nonatomic, assign) int* stateOut;
@property(nonatomic, retain) NSArray<NSURL*>* pickedUrls;
@end

@implementation IupCocoaTouchFileDlgDelegate

- (void)documentPicker:(UIDocumentPickerViewController*)controller didPickDocumentsAtURLs:(NSArray<NSURL*>*)urls
{
	(void)controller;
	self.pickedUrls = urls;
	if (_stateOut) *_stateOut = IUPCOCOATOUCH_FILEDLG_PICKED;
}

- (void)documentPickerWasCancelled:(UIDocumentPickerViewController*)controller
{
	(void)controller;
	if (_stateOut) *_stateOut = IUPCOCOATOUCH_FILEDLG_CANCEL;
}

- (void)dealloc
{
	[_pickedUrls release];
	[super dealloc];
}

@end


static UIViewController* cocoaTouchFileDlgResolveHost(Ihandle* ih)
{
	Ihandle* parent_ih = IupGetAttributeHandle(ih, "PARENTDIALOG");
	if (parent_ih && parent_ih->handle && [(id)parent_ih->handle isKindOfClass:[IupViewController class]])
	{
		UIViewController* top = (IupViewController*)parent_ih->handle;
		while (top.presentedViewController && !top.presentedViewController.isBeingDismissed)
			top = top.presentedViewController;
		return top;
	}
	return iupCocoaTouchFindTopPresentedViewController();
}

static void cocoaTouchFileDlgPumpUntil(int* state)
{
	iupCocoaTouchModalBegin();
	while (*state == IUPCOCOATOUCH_FILEDLG_PENDING)
	{
		@autoreleasepool {
			CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.05, true);
		}
	}
}

static char* cocoaTouchFileDlgGetPatterns(Ihandle* ih, int* index)
{
	const char* p = iupAttribGet(ih, "EXTFILTER");
	int bars;

	*index = 0;
	if (!p)
		return iupStrDup(iupAttribGet(ih, "FILTER"));

	*index = iupAttribGetInt(ih, "FILTERUSED");
	if (*index < 1)
		*index = 1;

	for (bars = 2 * *index - 1; bars > 0; bars--)
	{
		p = strchr(p, '|');
		if (!p)
			return NULL;
		p++;
	}

	return strchr(p, '|') ? iupStrDupUntil(&p, '|') : iupStrDup(p);
}

static NSArray<UTType*>* cocoaTouchFileDlgContentTypes(Ihandle* ih, int* filter_used)
{
	char* patterns = cocoaTouchFileDlgGetPatterns(ih, filter_used);
	NSMutableArray<UTType*>* types = [NSMutableArray array];

	if (patterns && *patterns)
	{
		for (NSString* p in [iupCocoaTouchStrToNSString(patterns) componentsSeparatedByString:@";"])
		{
			NSString* pattern = [p stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
			NSString* ext = [pattern hasPrefix:@"*."] ? [pattern substringFromIndex:2] : nil;
			UTType* type = nil;

			if (ext.length && [ext rangeOfCharacterFromSet:[NSCharacterSet characterSetWithCharactersInString:@"*?"]].location == NSNotFound)
				type = [UTType typeWithFilenameExtension:[ext lowercaseString]];

			if (!type || type.isDynamic)
			{
				[types removeAllObjects];
				break;
			}
			if (![types containsObject:type])
				[types addObject:type];
		}
	}

	free(patterns);
	return types.count ? types : @[UTTypeItem];
}

static void cocoaTouchFileDlgStoreFiles(Ihandle* ih, NSArray<NSURL*>* urls)
{
	NSString* dir = [[urls[0] path] stringByDeletingLastPathComponent];
	BOOL full_path = iupAttribGetBoolean(ih, "MULTIVALUEPATH");
	NSInteger i;

	iupAttribSetStr(ih, "DIRECTORY", [[dir stringByAppendingString:@"/"] UTF8String]);

	if (iupAttribGetBoolean(ih, "MULTIPLEFILES"))
	{
		iupAttribSetStrId(ih, "MULTIVALUE", 0, [[dir stringByAppendingString:@"/"] UTF8String]);
		for (i = 0; i < (NSInteger)urls.count; i++)
			iupAttribSetStrId(ih, "MULTIVALUE", (int)i + 1, [(full_path ? [urls[i] path] : [[urls[i] path] lastPathComponent]) UTF8String]);
		iupAttribSetInt(ih, "MULTIVALUECOUNT", (int)urls.count + 1);
	}

	if (urls.count == 1)
		iupAttribSetStr(ih, "VALUE", [[urls[0] path] UTF8String]);
	else
	{
		NSMutableString* value = [NSMutableString stringWithString:dir];
		for (NSURL* u in urls)
		{
			[value appendString:@"|"];
			[value appendString:full_path ? [u path] : [[u path] lastPathComponent]];
		}
		[value appendString:@"|"];
		iupAttribSetStr(ih, "VALUE", [value UTF8String]);
	}

	iupAttribSetStr(ih, "VALUE_URI", [[urls[0] absoluteString] UTF8String]);
}

static int cocoaTouchFileDlgPopup(Ihandle* ih, int x, int y)
{
	(void)x; (void)y;

	const char* type   = iupAttribGetStr(ih, "DIALOGTYPE");
	int is_save = iupStrEqualNoCase(type, "SAVE");
	int is_dir  = iupStrEqualNoCase(type, "DIR");

	/* folder picks return security-scoped URLs, not fopen paths; bail */
	if (is_dir)
	{
		iupAttribSet(ih, "STATUS", "-1");
		iupAttribSet(ih, "VALUE", NULL);
		return IUP_NOERROR;
	}

	UIViewController* host = cocoaTouchFileDlgResolveHost(ih);
	if (!host)
	{
		iupAttribSet(ih, "STATUS", "-1");
		return IUP_NOERROR;
	}

	int filter_used;
	NSArray<UTType*>* types = cocoaTouchFileDlgContentTypes(ih, &filter_used);

	UIDocumentPickerViewController* picker = nil;
	NSString* scratch_path = nil;

	if (is_save)
	{
		const char* file_name = iupAttribGet(ih, "FILE");
		if (!file_name || !*file_name) file_name = "untitled";
		scratch_path = [NSTemporaryDirectory() stringByAppendingPathComponent:
			[iupCocoaTouchStrToNSString(file_name) lastPathComponent]];
		[[NSData data] writeToFile:scratch_path atomically:YES];
		NSURL* scratch_url = [NSURL fileURLWithPath:scratch_path];
		picker = [[[UIDocumentPickerViewController alloc]
		    initForExportingURLs:@[scratch_url] asCopy:YES] autorelease];
	}
	else
	{
		picker = [[[UIDocumentPickerViewController alloc]
		    initForOpeningContentTypes:types asCopy:YES] autorelease];
		picker.allowsMultipleSelection = iupAttribGetBoolean(ih, "MULTIPLEFILES");
	}

	IupCocoaTouchFileDlgDelegate* delegate = [[[IupCocoaTouchFileDlgDelegate alloc] init] autorelease];
	__block int state = IUPCOCOATOUCH_FILEDLG_PENDING;
	delegate.stateOut = &state;
	picker.delegate = delegate;
	/* picker.delegate is __weak; pin the delegate so it survives pumpUntil's pool drain */
	static const void* kFileDlgDelegateKey = &kFileDlgDelegateKey;
	objc_setAssociatedObject(picker, kFileDlgDelegateKey, delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);

	[host presentViewController:picker animated:YES completion:nil];
	cocoaTouchFileDlgPumpUntil(&state);

	if (state == IUPCOCOATOUCH_FILEDLG_CANCEL || delegate.pickedUrls.count == 0)
	{
		if (scratch_path) [[NSFileManager defaultManager] removeItemAtPath:scratch_path error:nil];
		iupAttribSet(ih, "STATUS", "-1");
		iupAttribSet(ih, "FILTERUSED", NULL);
		iupAttribSet(ih, "VALUE", NULL);
		iupAttribSet(ih, "VALUE_URI", NULL);
		return IUP_NOERROR;
	}

	if (filter_used)
		iupAttribSetInt(ih, "FILTERUSED", filter_used);

	if (is_save)
	{
		/* caller writes to scratch; Destroy flushes scratch over the picker's destination */
		NSURL* dest_url = delegate.pickedUrls[0];
		iupAttribSetStr(ih, "VALUE", [scratch_path UTF8String]);
		iupAttribSetStr(ih, "DIRECTORY", [[[scratch_path stringByDeletingLastPathComponent] stringByAppendingString:@"/"] UTF8String]);
		iupAttribSetStr(ih, "VALUE_URI", [[dest_url absoluteString] UTF8String]);
		iupAttribSet(ih, "STATUS", "1");
		iupAttribSet(ih, "FILEEXIST", "NO");
		iupAttribSetStr(ih, "_IUPCOCOATOUCH_FILEDLG_SCRATCH", [scratch_path UTF8String]);
		iupAttribSetStr(ih, "_IUPCOCOATOUCH_FILEDLG_DEST", [[dest_url absoluteString] UTF8String]);
	}
	else
	{
		cocoaTouchFileDlgStoreFiles(ih, delegate.pickedUrls);
		iupAttribSet(ih, "STATUS", "0");
		iupAttribSet(ih, "FILEEXIST", "YES");
	}

	return IUP_NOERROR;
}

static void cocoaTouchFileDlgDestroyMethod(Ihandle* ih)
{
	const char* scratch_cstr = iupAttribGet(ih, "_IUPCOCOATOUCH_FILEDLG_SCRATCH");
	const char* dest_cstr    = iupAttribGet(ih, "_IUPCOCOATOUCH_FILEDLG_DEST");
	if (!scratch_cstr || !dest_cstr) return;

	@autoreleasepool {
		NSString* scratch = iupCocoaTouchStrToNSString(scratch_cstr);
		NSURL*    dest    = [NSURL URLWithString:iupCocoaTouchStrToNSString(dest_cstr)];
		NSFileManager* fm = [NSFileManager defaultManager];

		if (dest && [dest startAccessingSecurityScopedResource])
		{
			NSData* data = [NSData dataWithContentsOfFile:scratch];
			if (data) [data writeToURL:dest atomically:YES];
			[dest stopAccessingSecurityScopedResource];
		}

		[fm removeItemAtPath:scratch error:nil];
	}

	iupAttribSet(ih, "_IUPCOCOATOUCH_FILEDLG_SCRATCH", NULL);
	iupAttribSet(ih, "_IUPCOCOATOUCH_FILEDLG_DEST", NULL);
}

IUP_SDK_API void iupdrvFileDlgInitClass(Iclass* ic)
{
	ic->DlgPopup = cocoaTouchFileDlgPopup;
	ic->Destroy  = cocoaTouchFileDlgDestroyMethod;

	iupClassRegisterAttribute(ic, "EXTFILTER", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "FILTERINFO", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "FILTERUSED", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "MULTIPLEFILES", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "VALUE_URI", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}
