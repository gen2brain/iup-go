/** \file
 * \brief WKWebView Web Browser Control (macOS Cocoa shim).
 *
 * See Copyright Notice in "iup.h"
 */

#import <AppKit/AppKit.h>
#import <WebKit/WKWebView.h>

#include "iup.h"
#include "iup_object.h"

#include "iupcocoa_drv.h"


#define iupAppleWKBaseView NSView
#define iupAppleWKAddToParent(ih)      iupcocoaAddToParent(ih)
#define iupAppleWKRemoveFromParent(ih) iupcocoaRemoveFromParent(ih)

static inline void iupAppleWKApplyAutoresize(WKWebView* v)
{
	[v setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
}

static inline void iupAppleWKFocusForExec(WKWebView* v)
{
	NSWindow* win = [v window];
	if (win) [win makeFirstResponder:v];
}

static inline void iupAppleWKSetAssociatedViews(Ihandle* ih, WKWebView* v)
{
	iupcocoaSetAssociatedViews(ih, v, v);
}

static inline void iupAppleWKRunPrint(Ihandle* ih, WKWebView* v)
{
	NSPrintInfo* print_info = [[NSPrintInfo sharedPrintInfo] copy];
	[print_info setHorizontalPagination:NSPrintingPaginationModeAutomatic];
	[print_info setVerticalPagination:NSPrintingPaginationModeAutomatic];
	[print_info setVerticallyCentered:NO];

	NSPrintOperation* print_operation = [NSPrintOperation printOperationWithView:v printInfo:print_info];
	[print_info release];

	[print_operation setShowsPrintPanel:YES];
	[print_operation setShowsProgressPanel:YES];

	[[print_operation printPanel] setOptions:NSPrintPanelShowsCopies | NSPrintPanelShowsPageRange | NSPrintPanelShowsPaperSize | NSPrintPanelShowsOrientation];

	Ihandle* dlg = IupGetDialog(ih);
	NSWindow* parent_window = (dlg && dlg->handle) ? (NSWindow*)dlg->handle : nil;

	if (parent_window)
		[print_operation runOperationModalForWindow:parent_window delegate:nil didRunSelector:NULL contextInfo:NULL];
	else
		[print_operation runOperation];
}


#include "iupapplewk_webbrowser.m"
