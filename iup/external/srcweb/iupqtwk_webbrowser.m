/** \file
 * \brief WKWebView Web Browser Control (macOS Qt and Qt Quick shim).
 *
 * See Copyright Notice in "iup.h"
 */

#import <AppKit/AppKit.h>
#import <WebKit/WKWebView.h>

#include "iup.h"
#include "iup_object.h"
#include "iup_classbase.h"

#include "iupweb_host.h"


#define iupAppleWKBaseView NSView

static inline int iupAppleWKAddToParent(Ihandle* ih, WKWebView* v)
{
	NSView* parent = (NSView*)iupwebHostMap(ih);
	if (!parent)
	{
		iupwebHostUnMap(ih);
		return 0;
	}

	[v setHidden:YES];
	[parent addSubview:v];
	return 1;
}

static inline void iupAppleWKRemoveFromParent(Ihandle* ih, WKWebView* v)
{
	[v removeFromSuperview];
	iupwebHostUnMap(ih);
}

static inline void iupAppleWKLayoutUpdate(Ihandle* ih, WKWebView* v)
{
	(void)v;
	iupwebHostLayoutUpdate(ih);
}

static inline void iupAppleWKApplyAutoresize(WKWebView* v)
{
	[v setAutoresizingMask:NSViewNotSizable];
}

static inline void iupAppleWKFocusForExec(WKWebView* v)
{
	NSWindow* win = [v window];
	if (win) [win makeFirstResponder:v];
}

static inline void iupAppleWKRunPrint(Ihandle* ih, WKWebView* v)
{
	(void)ih;
	NSPrintInfo* print_info = [[NSPrintInfo sharedPrintInfo] copy];
	[print_info setHorizontalPagination:NSPrintingPaginationModeAutomatic];
	[print_info setVerticalPagination:NSPrintingPaginationModeAutomatic];
	[print_info setVerticallyCentered:NO];

	NSPrintOperation* print_operation = [NSPrintOperation printOperationWithView:v printInfo:print_info];
	[print_info release];

	[print_operation setShowsPrintPanel:YES];
	[print_operation setShowsProgressPanel:YES];

	[[print_operation printPanel] setOptions:NSPrintPanelShowsCopies | NSPrintPanelShowsPageRange | NSPrintPanelShowsPaperSize | NSPrintPanelShowsOrientation];

	NSWindow* parent_window = [v window];
	if (parent_window)
		[print_operation runOperationModalForWindow:parent_window delegate:nil didRunSelector:NULL contextInfo:NULL];
	else
		[print_operation runOperation];
}


#include "iupapplewk_webbrowser.m"


void iupwebHostSetBounds(Ihandle* ih, int x, int y, int width, int height, int visible)
{
	WKWebView* v = ih->data ? ih->data->web_view : nil;
	if (!v)
		return;

	[v setFrame:NSMakeRect(x, y, width, height)];
	[v setHidden:visible ? NO : YES];
}
