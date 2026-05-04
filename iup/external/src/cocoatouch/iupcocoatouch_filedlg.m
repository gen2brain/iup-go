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
	while (*state == IUPCOCOATOUCH_FILEDLG_PENDING)
	{
		@autoreleasepool {
			CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.05, true);
		}
	}
}

/* first "*.ext" pattern from EXTFILTER or FILTER */
static NSArray<UTType*>* cocoaTouchFileDlgContentTypes(Ihandle* ih)
{
	const char* pattern = NULL;
	const char* ext_filter = iupAttribGet(ih, "EXTFILTER");
	char buf[256];
	if (ext_filter && *ext_filter)
	{
		const char* first_bar = strchr(ext_filter, '|');
		if (first_bar)
		{
			const char* second_bar = strchr(first_bar + 1, '|');
			size_t len = second_bar ? (size_t)(second_bar - first_bar - 1) : strlen(first_bar + 1);
			if (len >= sizeof(buf)) len = sizeof(buf) - 1;
			memcpy(buf, first_bar + 1, len);
			buf[len] = '\0';
			pattern = buf;
		}
		else
		{
			pattern = ext_filter;
		}
	}
	else
	{
		pattern = iupAttribGet(ih, "FILTER");
	}

	if (!pattern || !*pattern) return @[UTTypeItem];

	const char* dot = strrchr(pattern, '.');
	if (!dot) return @[UTTypeItem];
	dot++;
	NSString* ext = [NSString stringWithUTF8String:dot];
	UTType* type = [UTType typeWithFilenameExtension:ext];
	return type ? @[type] : @[UTTypeItem];
}

/* multi-pick VALUE = "dir|name1|name2|..." per IUP convention */
static void cocoaTouchFileDlgStoreUrls(Ihandle* ih, NSArray<NSURL*>* urls)
{
	if (!urls || urls.count == 0) return;

	if (urls.count == 1)
	{
		iupAttribSetStr(ih, "VALUE", [[urls[0] path] UTF8String]);
		iupAttribSetStr(ih, "VALUE_URI", [[urls[0] absoluteString] UTF8String]);
		return;
	}

	NSString* dir = [[urls[0] path] stringByDeletingLastPathComponent];
	NSMutableString* value = [NSMutableString stringWithString:dir];
	for (NSURL* u in urls)
	{
		[value appendString:@"|"];
		[value appendString:[[u path] lastPathComponent]];
	}
	iupAttribSetStr(ih, "VALUE", [value UTF8String]);
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

	NSArray<UTType*>* types = cocoaTouchFileDlgContentTypes(ih);

	UIDocumentPickerViewController* picker = nil;
	NSString* scratch_path = nil;

	if (is_save)
	{
		const char* file_name = iupAttribGet(ih, "FILE");
		if (!file_name || !*file_name) file_name = "untitled";
		scratch_path = [NSTemporaryDirectory() stringByAppendingPathComponent:
			[NSString stringWithUTF8String:file_name]];
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
		iupAttribSet(ih, "VALUE", NULL);
		iupAttribSet(ih, "VALUE_URI", NULL);
		return IUP_NOERROR;
	}

	if (is_save)
	{
		/* caller writes to scratch; Destroy flushes scratch over the picker's destination */
		NSURL* dest_url = delegate.pickedUrls[0];
		iupAttribSetStr(ih, "VALUE", [scratch_path UTF8String]);
		iupAttribSetStr(ih, "VALUE_URI", [[dest_url absoluteString] UTF8String]);
		iupAttribSet(ih, "STATUS", "1");
		iupAttribSet(ih, "FILEEXIST", "NO");
		iupAttribSetStr(ih, "_IUPCOCOATOUCH_FILEDLG_SCRATCH", [scratch_path UTF8String]);
		iupAttribSetStr(ih, "_IUPCOCOATOUCH_FILEDLG_DEST", [[dest_url absoluteString] UTF8String]);
	}
	else
	{
		cocoaTouchFileDlgStoreUrls(ih, delegate.pickedUrls);
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
		NSString* scratch = [NSString stringWithUTF8String:scratch_cstr];
		NSURL*    dest    = [NSURL URLWithString:[NSString stringWithUTF8String:dest_cstr]];
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
