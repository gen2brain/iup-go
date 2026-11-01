/** \file
 * \brief IupPrintDlg pre-defined dialog
 *
 * See Copyright Notice in "iup.h"
 */

#import <UIKit/UIKit.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_stdcontrols.h"

#include "iupcocoatouch_drv.h"

#define IUPCOCOATOUCH_PRINT_SCALE (96.0 / 72.0)

@interface IupCocoaTouchPrintRenderer : UIPrintPageRenderer
{
@public
	Ihandle* ih;
	int width, height;
	BOOL canceled, landscape;
}
@end

@implementation IupCocoaTouchPrintRenderer

- (NSInteger)numberOfPages
{
	CGRect printable = [self printableRect];
	if (printable.size.width <= 0 || printable.size.height <= 0)
		return 0;
	width = (int)(printable.size.width * IUPCOCOATOUCH_PRINT_SCALE + 0.5);
	height = (int)(printable.size.height * IUPCOCOATOUCH_PRINT_SCALE + 0.5);
	landscape = self.paperRect.size.width > self.paperRect.size.height;
	return iupPrintDlgPaginate(ih, width, height);
}

- (void)drawPageAtIndex:(NSInteger)pageIndex inRect:(CGRect)printableRect
{
	CGContextRef ctx = UIGraphicsGetCurrentContext();
	if (!ctx || canceled)
		return;

	CGContextSaveGState(ctx);
	CGContextTranslateCTM(ctx, printableRect.origin.x, printableRect.origin.y);
	CGContextScaleCTM(ctx, 1.0 / IUPCOCOATOUCH_PRINT_SCALE, 1.0 / IUPCOCOATOUCH_PRINT_SCALE);
	if (iupPrintDlgDrawPage(ih, ctx, (int)pageIndex + 1, width, height) == IUP_CLOSE)
		canceled = YES;
	CGContextRestoreGState(ctx);
}

@end

static int cocoaTouchPrintDlgPopup(Ihandle* ih, int x, int y)
{
	__block int status = 2;
	UIPrintInteractionController* controller = [UIPrintInteractionController sharedPrintController];
	UIPrintInfo* info = [UIPrintInfo printInfo];
	IupCocoaTouchPrintRenderer* renderer = [[IupCocoaTouchPrintRenderer alloc] init];
	BOOL landscape = iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "LANDSCAPE");

	iupAttribSetInt(ih, "_IUPDLG_X", x);
	iupAttribSetInt(ih, "_IUPDLG_Y", y);

	renderer->ih = ih;

	info.jobName = [NSString stringWithUTF8String:iupPrintDlgGetJobName(ih)];
	info.outputType = UIPrintInfoOutputGeneral;
	info.orientation = landscape ? UIPrintInfoOrientationLandscape : UIPrintInfoOrientationPortrait;

	controller.printInfo = info;
	controller.printPageRenderer = renderer;

	[controller presentAnimated:YES completionHandler:^(UIPrintInteractionController* c, BOOL completed, NSError* error) {
		(void)c;
		status = error ? -1 : (completed ? 1 : 0);
	}];

	iupCocoaTouchModalBegin();
	while (status == 2)
	{
		@autoreleasepool {
			CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.05, true);
		}
	}

	if (status == 1 && renderer->canceled)
		status = 0;

	if (renderer->width > 0)
		iupAttribSet(ih, "ORIENTATION", renderer->landscape ? "LANDSCAPE" : "PORTRAIT");
	iupAttribSet(ih, "STATUS", status == 1 ? "1" : (status == 0 ? "0" : "-1"));

	controller.printPageRenderer = nil;
	[renderer release];
	return IUP_NOERROR;
}

IUP_SDK_API void iupdrvPrintDlgInitClass(Iclass* ic)
{
	ic->DlgPopup = cocoaTouchPrintDlgPopup;
}
