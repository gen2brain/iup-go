/** \file
 * \brief WKWebView Web Browser Control (iOS UIKit shim).
 *
 * See Copyright Notice in "iup.h"
 */

#import <UIKit/UIKit.h>
#import <WebKit/WKWebView.h>

#include "iup.h"
#include "iup_object.h"

#include "iupcocoatouch_drv.h"


#define iupAppleWKBaseView UIView
#define iupAppleWKAddToParent(ih)      iupCocoaTouchAddToParent(ih)
#define iupAppleWKRemoveFromParent(ih) iupCocoaTouchRemoveFromParent(ih)

/* autoresize trips WKWebView's out-of-process renderer mid-rotation */
static inline void iupAppleWKApplyAutoresize(WKWebView* v)
{
	[v setAutoresizingMask:UIViewAutoresizingNone];
}

/* UIKit has no first-responder-equivalent for a WKWebView from the dialog
   side; touch input already drives focus. */
static inline void iupAppleWKFocusForExec(WKWebView* v)
{
	(void)v;
}

/* Cocoa driver tracks main+root views; cocoatouch driver has a single handle. */
static inline void iupAppleWKSetAssociatedViews(Ihandle* ih, WKWebView* v)
{
	(void)ih; (void)v;
}

static inline void iupAppleWKRunPrint(Ihandle* ih, WKWebView* v)
{
	(void)ih;
	UIPrintInteractionController* pic = [UIPrintInteractionController sharedPrintController];
	if (!pic) return;

	UIPrintInfo* info = [UIPrintInfo printInfo];
	info.outputType = UIPrintInfoOutputGeneral;

	NSURL* url = [v URL];
	if (url && [url absoluteString].length > 0)
		info.jobName = [url absoluteString];
	else
		info.jobName = @"IUP";

	pic.printInfo = info;
	pic.printFormatter = [v viewPrintFormatter];
	pic.showsNumberOfCopies = YES;
	pic.showsPaperSelectionForLoadedPapers = YES;

	[pic presentAnimated:YES completionHandler:nil];
}


#include "iupapplewk_webbrowser.m"
