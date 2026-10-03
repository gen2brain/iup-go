/** \file
 * \brief WKWebView Web Browser Control (macOS host shim for other toolkits).
 *
 * See Copyright Notice in "iup.h"
 */

#import <AppKit/AppKit.h>
#import <WebKit/WKWebView.h>

#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_classbase.h"

#include "iupweb_host.h"


#define iupAppleWKBaseView NSView

@interface IupWKHostClipView : NSView
@end

@implementation IupWKHostClipView
- (BOOL)isFlipped
{
	return YES;
}
@end

static NSView* iupWKHostClipView(Ihandle* ih)
{
	return (NSView*)iupAttribGet(ih, "_IUPWEB_CLIPVIEW");
}

static NSView* iupWKHostParentView(void* parent)
{
	id obj = (id)parent;
	if ([obj isKindOfClass:[NSWindow class]])
		return [(NSWindow*)obj contentView];
	return (NSView*)obj;
}

static inline int iupAppleWKAddToParent(Ihandle* ih, WKWebView* v)
{
	NSView* parent = iupWKHostParentView(iupwebHostMap(ih));
	IupWKHostClipView* clip;

	if (!parent && !ih->handle)
		return 0;

	clip = [[IupWKHostClipView alloc] initWithFrame:NSZeroRect];
	if ([clip respondsToSelector:@selector(setClipsToBounds:)])
		[clip setClipsToBounds:YES];
	[clip setWantsLayer:YES];
	[[clip layer] setZPosition:1];
	[clip setHidden:YES];
	[clip addSubview:v];
	iupAttribSet(ih, "_IUPWEB_CLIPVIEW", (char*)clip);

	if (parent)
		[parent addSubview:clip];
	return 1;
}

static inline void iupAppleWKRemoveFromParent(Ihandle* ih, WKWebView* v)
{
	NSView* clip = iupWKHostClipView(ih);
	iupAttribSet(ih, "_IUPWEB_CLIPVIEW", NULL);

	[v removeFromSuperview];
	[clip removeFromSuperview];
	[clip release];
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


void iupwebHostSetBounds(Ihandle* ih, int x, int y, int width, int height, int clip_x, int clip_y, int clip_width, int clip_height)
{
	WKWebView* v = ih->data ? ih->data->web_view : nil;
	NSView* clip = iupWKHostClipView(ih);
	if (!v || !clip)
		return;

	NSView* parent = [clip superview];
	int frame_y = clip_y;
	if (parent && ![parent isFlipped])
	{
		frame_y = (int)[parent bounds].size.height - clip_y - clip_height;
		[clip setAutoresizingMask:NSViewMinYMargin];
	}
	else
		[clip setAutoresizingMask:NSViewNotSizable];

	[clip setFrame:NSMakeRect(clip_x, frame_y, clip_width, clip_height)];
	[v setFrame:NSMakeRect(x - clip_x, y - clip_y, width, height)];
	[clip setHidden:(clip_width > 0 && clip_height > 0) ? NO : YES];
}

void iupwebHostSetParent(Ihandle* ih, void* parent)
{
	NSView* clip = iupWKHostClipView(ih);
	NSView* view = iupWKHostParentView(parent);
	if (!clip || [clip superview] == view)
		return;

	[clip removeFromSuperview];
	if (view)
		[view addSubview:clip];
}
