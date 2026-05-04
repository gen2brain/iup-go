/** \file
 * \brief IupFontDlg (iOS UIKit)
 *
 * See Copyright Notice in "iup.h"
 */

#import <UIKit/UIKit.h>
#import <CoreFoundation/CoreFoundation.h>

#include <string.h>
#include <stdio.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drvfont.h"

#include "iupcocoatouch_drv.h"
#import "IupViewController.h"


#define IUPCOCOATOUCH_FONTDLG_PENDING (-1)
#define IUPCOCOATOUCH_FONTDLG_PICKED  (1)
#define IUPCOCOATOUCH_FONTDLG_CANCEL  (0)

@interface IupCocoaTouchFontPickerDelegate : NSObject <UIFontPickerViewControllerDelegate>
@property(nonatomic, assign) int* stateOut;
@property(nonatomic, retain) UIFontDescriptor* pickedDescriptor;
@end

@implementation IupCocoaTouchFontPickerDelegate

- (void)fontPickerViewControllerDidPickFont:(UIFontPickerViewController*)viewController
{
	self.pickedDescriptor = viewController.selectedFontDescriptor;
	[viewController dismissViewControllerAnimated:YES completion:^{
		if (_stateOut) *_stateOut = IUPCOCOATOUCH_FONTDLG_PICKED;
	}];
}

- (void)fontPickerViewControllerDidCancel:(UIFontPickerViewController*)viewController
{
	[viewController dismissViewControllerAnimated:YES completion:^{
		if (_stateOut) *_stateOut = IUPCOCOATOUCH_FONTDLG_CANCEL;
	}];
}

- (void)dealloc
{
	[_pickedDescriptor release];
	[super dealloc];
}

@end


static UIViewController* cocoaTouchFontDlgResolveHost(Ihandle* ih)
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

static void cocoaTouchFontDlgPumpUntil(int* state)
{
	while (*state == IUPCOCOATOUCH_FONTDLG_PENDING)
	{
		@autoreleasepool {
			CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.05, true);
		}
	}
}

static NSString* cocoaTouchFontDlgFaceString(UIFontDescriptor* descriptor)
{
	if (!descriptor) return @"";
	UIFontDescriptorSymbolicTraits traits = descriptor.symbolicTraits;
	BOOL bold = (traits & UIFontDescriptorTraitBold) != 0;
	BOOL italic = (traits & UIFontDescriptorTraitItalic) != 0;
	if (bold && italic) return @"Bold Italic";
	if (bold) return @"Bold";
	if (italic) return @"Italic";
	return @"";
}

static int cocoaTouchFontDlgShowSizeAlert(UIViewController* host, int initial_size, int* size_out)
{
	__block int state = IUPCOCOATOUCH_FONTDLG_PENDING;
	__block int picked_size = initial_size;

	UIAlertController* alert = [UIAlertController
	    alertControllerWithTitle:@"Size"
	                     message:nil
	              preferredStyle:UIAlertControllerStyleAlert];

	[alert addTextFieldWithConfigurationHandler:^(UITextField* tf) {
		tf.keyboardType = UIKeyboardTypeNumberPad;
		tf.text = [NSString stringWithFormat:@"%d", initial_size > 0 ? initial_size : 12];
		tf.clearButtonMode = UITextFieldViewModeWhileEditing;
	}];
	[alert addAction:[UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:^(UIAlertAction* a) {
		(void)a;
		state = IUPCOCOATOUCH_FONTDLG_CANCEL;
	}]];
	[alert addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:^(UIAlertAction* a) {
		(void)a;
		NSString* text = alert.textFields.firstObject.text;
		int sz = 0;
		if (text && iupStrToInt([text UTF8String], &sz) && sz > 0)
			picked_size = sz;
		state = IUPCOCOATOUCH_FONTDLG_PICKED;
	}]];

	[host presentViewController:alert animated:YES completion:nil];
	cocoaTouchFontDlgPumpUntil(&state);

	if (state == IUPCOCOATOUCH_FONTDLG_PICKED)
	{
		*size_out = picked_size;
		return 1;
	}
	return 0;
}

static int cocoaTouchFontDlgPopup(Ihandle* ih, int x, int y)
{
	(void)x; (void)y;

	UIViewController* host = cocoaTouchFontDlgResolveHost(ih);
	if (!host)
	{
		iupAttribSet(ih, "STATUS", "-1");
		return IUP_NOERROR;
	}

	const char* value = iupAttribGet(ih, "VALUE");
	if (!value) value = IupGetGlobal("DEFAULTFONT");

	char initial_face[256] = "";
	int  initial_bold = 0, initial_italic = 0, initial_underline = 0, initial_strikeout = 0;
	int  initial_size = 12;
	if (value)
		iupGetFontInfo(value, initial_face, &initial_size, &initial_bold, &initial_italic, &initial_underline, &initial_strikeout);

	UIFontPickerViewControllerConfiguration* config = [[[UIFontPickerViewControllerConfiguration alloc] init] autorelease];
	config.includeFaces = YES;
	config.displayUsingSystemFont = NO;

	UIFontPickerViewController* picker = [[[UIFontPickerViewController alloc] initWithConfiguration:config] autorelease];

	IupCocoaTouchFontPickerDelegate* delegate = [[[IupCocoaTouchFontPickerDelegate alloc] init] autorelease];
	__block int state = IUPCOCOATOUCH_FONTDLG_PENDING;
	delegate.stateOut = &state;
	picker.delegate = delegate;

	if (initial_face[0])
	{
		UIFontDescriptorSymbolicTraits traits = 0;
		if (initial_bold)   traits |= UIFontDescriptorTraitBold;
		if (initial_italic) traits |= UIFontDescriptorTraitItalic;
		NSDictionary* attrs = @{
			UIFontDescriptorFamilyAttribute: [NSString stringWithUTF8String:initial_face],
			UIFontDescriptorTraitsAttribute: @{ UIFontSymbolicTrait: @(traits) }
		};
		picker.selectedFontDescriptor = [UIFontDescriptor fontDescriptorWithFontAttributes:attrs];
	}

	[host presentViewController:picker animated:YES completion:nil];
	cocoaTouchFontDlgPumpUntil(&state);

	if (state == IUPCOCOATOUCH_FONTDLG_CANCEL)
	{
		iupAttribSet(ih, "STATUS", "-1");
		return IUP_NOERROR;
	}

	UIFontDescriptor* picked = delegate.pickedDescriptor;
	NSString* family = picked ? [picked objectForKey:UIFontDescriptorFamilyAttribute] : nil;
	if (!family) family = picked ? picked.postscriptName : nil;
	if (!family) family = [NSString stringWithUTF8String:initial_face[0] ? initial_face : "System"];
	NSString* face = cocoaTouchFontDlgFaceString(picked);

	int final_size = initial_size;
	UIViewController* size_host = cocoaTouchFontDlgResolveHost(ih);
	if (!size_host) size_host = host;
	if (!cocoaTouchFontDlgShowSizeAlert(size_host, initial_size, &final_size))
	{
		iupAttribSet(ih, "STATUS", "-1");
		return IUP_NOERROR;
	}

	char font_value[512];
	if (face.length > 0)
		snprintf(font_value, sizeof(font_value), "%s, %s %d", [family UTF8String], [face UTF8String], final_size);
	else
		snprintf(font_value, sizeof(font_value), "%s, %d", [family UTF8String], final_size);

	iupAttribSetStr(ih, "VALUE", font_value);
	iupAttribSet(ih, "STATUS", "1");
	return IUP_NOERROR;
}

IUP_SDK_API void iupdrvFontDlgInitClass(Iclass* ic)
{
	ic->DlgPopup = cocoaTouchFontDlgPopup;
}
