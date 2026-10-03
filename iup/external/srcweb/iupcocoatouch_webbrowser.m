/** \file
 * \brief WKWebView Web Browser Control (iOS UIKit shim).
 *
 * See Copyright Notice in "iup.h"
 */

#import <UIKit/UIKit.h>
#import <WebKit/WKWebView.h>

#include "iup.h"
#include "iup_object.h"
#include "iup_classbase.h"

#include "iupcocoatouch_drv.h"


#define iupAppleWKBaseView UIView

static inline int iupAppleWKAddToParent(Ihandle* ih, WKWebView* v)
{
	ih->handle = [v retain];
	iupCocoaTouchAddToParent(ih);
	return 1;
}

static inline void iupAppleWKRemoveFromParent(Ihandle* ih, WKWebView* v)
{
	(void)v;
	iupdrvBaseUnMapMethod(ih);
}

static inline void iupAppleWKLayoutUpdate(Ihandle* ih, WKWebView* v)
{
	(void)v;
	iupdrvBaseLayoutUpdateMethod(ih);
}

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
