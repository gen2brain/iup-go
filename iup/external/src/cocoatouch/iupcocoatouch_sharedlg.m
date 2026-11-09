/** \file
 * \brief IupShareDlg pre-defined dialog
 *
 * See Copyright Notice in "iup.h"
 */

#import <UIKit/UIKit.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_stdcontrols.h"

#include "iupcocoatouch_drv.h"


static UIViewController* cocoaTouchShareDlgResolveHost(Ihandle* ih)
{
	Ihandle* parent_ih = IupGetAttributeHandle(ih, "PARENTDIALOG");
	if (parent_ih && parent_ih->handle && [(id)parent_ih->handle isKindOfClass:[UIViewController class]])
	{
		UIViewController* top = (UIViewController*)parent_ih->handle;
		while (top.presentedViewController && !top.presentedViewController.isBeingDismissed)
			top = top.presentedViewController;
		return top;
	}
	return iupCocoaTouchFindTopPresentedViewController();
}

static int cocoaTouchShareDlgPopup(Ihandle* ih, int x, int y)
{
	__block int status = 2;
	NSMutableArray* items = [NSMutableArray array];
	const char* text = iupAttribGet(ih, "TEXT");
	const char* url = iupAttribGet(ih, "URL");
	const char* file = iupAttribGet(ih, "FILE");
	UIViewController* host = cocoaTouchShareDlgResolveHost(ih);
	UIActivityViewController* controller;

	iupAttribSetInt(ih, "_IUPDLG_X", x);
	iupAttribSetInt(ih, "_IUPDLG_Y", y);

	if (text)
		[items addObject:[NSString stringWithUTF8String:text]];
	if (url)
	{
		NSURL* nsurl = [NSURL URLWithString:[NSString stringWithUTF8String:url]];
		if (nsurl)
			[items addObject:nsurl];
	}
	if (file)
		[items addObject:[NSURL fileURLWithPath:[NSString stringWithUTF8String:file]]];

	if ([items count] == 0 || !host)
	{
		iupAttribSet(ih, "STATUS", "-1");
		return IUP_ERROR;
	}

	controller = [[UIActivityViewController alloc] initWithActivityItems:items applicationActivities:nil];
	controller.completionWithItemsHandler = ^(UIActivityType type, BOOL completed, NSArray* returned, NSError* error) {
		(void)type; (void)returned;
		status = error ? -1 : (completed ? 1 : 0);
		iupCocoaTouchWakeWait();
	};

	if (controller.popoverPresentationController)
	{
		UIView* view = host.view;
		controller.popoverPresentationController.sourceView = view;
		controller.popoverPresentationController.sourceRect = CGRectMake(CGRectGetMidX(view.bounds), CGRectGetMidY(view.bounds), 0, 0);
		controller.popoverPresentationController.permittedArrowDirections = 0;
	}

	[host presentViewController:controller animated:YES completion:nil];

	iupCocoaTouchModalBegin();
	while (status == 2)
	{
		iupCocoaTouchWaitEvent();
	}

	iupAttribSet(ih, "STATUS", status == 1 ? "1" : (status == 0 ? "0" : "-1"));

	[controller release];
	return IUP_NOERROR;
}

IUP_SDK_API void iupdrvShareDlgInitClass(Iclass* ic)
{
	ic->DlgPopup = cocoaTouchShareDlgPopup;
}
