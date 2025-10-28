/** \file
 * \brief WKWebView Web Browser Control
 *
 * See Copyright Notice in "iup.h"
 */

#import <Foundation/Foundation.h>
#import <WebKit/WKWebView.h>
#import <WebKit/WKUIDelegate.h>
#import <WebKit/WKNavigationAction.h>
#import <WebKit/WKNavigationDelegate.h>
#import <WebKit/WKUserContentController.h>
#import <WebKit/WKUserScript.h>
#import <WebKit/WKWebViewConfiguration.h>
#import <WebKit/WKScriptMessageHandler.h>

#import <objc/runtime.h>

#include <stdio.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_webbrowser.h"
#include "iup_drv.h"
#include "iup_drvfont.h"

static const void* IUP_APPLEWKWEBVIEW_WEBVIEW_DELEGATE_OBJ_KEY = @"IUP_APPLEWKWEBVIEW_WEBVIEW_DELEGATE_OBJ_KEY";

#import <AppKit/AppKit.h>
#include "iupcocoa_drv.h"
#define iupAppleWKAddToParent iupCocoaAddToParent
#define iupAppleWKRemoveFromParent iupCocoaRemoveFromParent
#define iupAppleWKBaseView NSView

static NSString* const IupUpdateCbMessageHandlerName = @"iupUpdateCb";
static NSString* const IupDirtyFlagMessageHandlerName = @"iupDirtyFlag";

static int cocoaWKWebBrowserSetValueAttrib(Ihandle* ih, const char* value);
static int cocoaWKWebBrowserSetEditableAttrib(Ihandle* ih, const char* value);
static void cocoaWKWebBrowserUpdateHistory(Ihandle* ih);

struct _IcontrolData
{
  int sb;
};

typedef NS_ENUM(NSInteger, IupAppleWKWebViewLoadStatus)
{
	IupAppleWKWebViewLoadStatusFinished,
	IupAppleWKWebViewLoadStatusFailed,
	IupAppleWKWebViewLoadStatusLoading
};

@interface IupAppleWKWebViewDelegate : NSObject <WKNavigationDelegate, WKUIDelegate, WKScriptMessageHandler>

@property(nonatomic, assign) Ihandle* ihandle;
@property(nonatomic, assign) IupAppleWKWebViewLoadStatus currentLoadStatus;
@end

@implementation IupAppleWKWebViewDelegate

- (void) webView:(WKWebView*)the_sender decidePolicyForNavigationAction:(WKNavigationAction*)navigation_action decisionHandler:(void (^)(WKNavigationActionPolicy))decision_handler
{
	Ihandle* ih = [self ihandle];

	if (iupAttribGet(ih, "_IUPWEB_IGNORE_NAVIGATE"))
	{
		decision_handler(WKNavigationActionPolicyAllow);
		return;
	}

	IFns cb = (IFns)IupGetCallback(ih, "NAVIGATE_CB");
	if (cb)
	{
		NSURL* ns_url = [[navigation_action request] URL];
		const char* c_url = [[ns_url absoluteString] UTF8String];

		if(cb(ih, (char*)c_url) == IUP_IGNORE)
		{
			decision_handler(WKNavigationActionPolicyCancel);
		}
		else
		{
			decision_handler(WKNavigationActionPolicyAllow);
		}
	}
	else
	{
		decision_handler(WKNavigationActionPolicyAllow);
	}
}

- (nullable WKWebView *)webView:(WKWebView*)web_view createWebViewWithConfiguration:(WKWebViewConfiguration*)webview_configuration forNavigationAction:(WKNavigationAction*)navigation_action windowFeatures:(WKWindowFeatures*)window_features
{
	Ihandle* ih = [self ihandle];

	IFns cb = (IFns)IupGetCallback(ih, "NEWWINDOW_CB");
	if (cb)
	{
		NSURL* ns_url = [[navigation_action request] URL];
		const char* c_url = [[ns_url absoluteString] UTF8String];

		cb(ih, (char*)c_url);
	}

	return nil;
}

- (void) webView:(WKWebView*)web_view didFailProvisionalNavigation:(WKNavigation*)navigation withError:(NSError*)the_error
{
	Ihandle* ih = [self ihandle];

	[self setCurrentLoadStatus:IupAppleWKWebViewLoadStatusFailed];

	cocoaWKWebBrowserUpdateHistory(ih);

	IFns cb = (IFns)IupGetCallback(ih, "ERROR_CB");
	if (cb)
	{
		if ([the_error code] != NSURLErrorCancelled)
		{
			const char* failed_url = [[[the_error userInfo] valueForKey:NSURLErrorFailingURLStringErrorKey] UTF8String];
			if (failed_url == NULL)
			{
				NSURL* current_url = [web_view URL];
				if (current_url)
					failed_url = [[current_url absoluteString] UTF8String];
			}
			if (failed_url)
				cb(ih, (char*)failed_url);
		}
	}
}

- (void) webView:(WKWebView*)web_view didFailNavigation:(WKNavigation*)navigation withError:(NSError*)the_error
{
	Ihandle* ih = [self ihandle];

	[self setCurrentLoadStatus:IupAppleWKWebViewLoadStatusFailed];

	cocoaWKWebBrowserUpdateHistory(ih);

	IFns cb = (IFns)IupGetCallback(ih, "ERROR_CB");
	if (cb)
	{
		if ([the_error code] != NSURLErrorCancelled)
		{
			const char* failed_url = [[[the_error userInfo] valueForKey:NSURLErrorFailingURLStringErrorKey] UTF8String];
			if (failed_url == NULL)
			{
				NSURL* current_url = [web_view URL];
				if (current_url)
					failed_url = [[current_url absoluteString] UTF8String];
			}
			if (failed_url)
				cb(ih, (char*)failed_url);
		}
	}
}

- (void) webView:(WKWebView*)web_view didFinishNavigation:(WKNavigation*)navigation
{
	Ihandle* ih = [self ihandle];

	[self setCurrentLoadStatus:IupAppleWKWebViewLoadStatusFinished];

	if(iupAttribGet(ih, "_IUPWEB_EDITABLE"))
	{
		[web_view evaluateJavaScript:@"document.body.contentEditable = 'true';" completionHandler:nil];
	}
	else
	{
		[web_view evaluateJavaScript:@"document.body.contentEditable = 'false';" completionHandler:nil];
	}

	cocoaWKWebBrowserUpdateHistory(ih);

	IFns cb = (IFns)IupGetCallback(ih, "COMPLETED_CB");
	if (cb)
	{
		NSURL* current_url = [web_view URL];
		if (current_url)
		{
			const char* c_url = [[current_url absoluteString] UTF8String];
			cb(ih, (char*)c_url);
		}
	}

	IFn cb_update = (IFn)IupGetCallback(ih, "UPDATE_CB");
	if (cb_update)
	{
		cb_update(ih);
	}
}

- (void) webView:(WKWebView*)web_view didStartProvisionalNavigation:(WKNavigation*)navigation
{
	[self setCurrentLoadStatus:IupAppleWKWebViewLoadStatusLoading];
}

- (void)userContentController:(WKUserContentController *)userContentController didReceiveScriptMessage:(WKScriptMessage *)message
{
	if ([[message name] isEqualToString:IupUpdateCbMessageHandlerName])
	{
		Ihandle* ih = [self ihandle];
		IFn cb_update = (IFn)IupGetCallback(ih, "UPDATE_CB");
		if (cb_update)
		{
			cb_update(ih);
		}
	}
	else if ([[message name] isEqualToString:IupDirtyFlagMessageHandlerName])
	{
		Ihandle* ih = [self ihandle];
		iupAttribSet(ih, "_IUPWEB_DIRTY", "1");
	}
}

@end

static void cocoaWKWebBrowserUpdateHistory(Ihandle* ih)
{
	WKWebView* web_view = (WKWebView*)ih->handle;
	if (!web_view)
		return;

	BOOL can_go_back = [web_view canGoBack];
	BOOL can_go_forward = [web_view canGoForward];

	iupAttribSet(ih, "CANGOBACK", can_go_back ? "YES" : "NO");
	iupAttribSet(ih, "CANGOFORWARD", can_go_forward ? "YES" : "NO");
}

static NSString* cocoaWKWebBrowserEscapeJavaScript(const char* c_str)
{
	if (!c_str) return @"null";
	NSString* ns_str = [NSString stringWithUTF8String:c_str];

	NSArray* wrapper_array = [NSArray arrayWithObject:ns_str];
	NSData* data = [NSJSONSerialization dataWithJSONObject:wrapper_array options:0 error:nil];
	if (!data) return @"null";

	NSString* json_array_string = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];

	NSString* json_string;
	if ([json_array_string length] > 2)
	{
		json_string = [json_array_string substringWithRange:NSMakeRange(1, [json_array_string length] - 2)];
	}
	else
	{
		json_string = @"\"\"";
	}

	[json_array_string release];
	return json_string;
}

static char* cocoaWKWebBrowserRunJavaScriptSync(Ihandle* ih, NSString* js_string)
{
	WKWebView* web_view = (WKWebView*)ih->handle;
	__block char* ret_str = NULL;
	__block BOOL finished = NO;

	void (^run_js_and_wait)(void) = ^{
		[web_view evaluateJavaScript:js_string completionHandler:^(id result, NSError *error) {
			if (!error && result)
			{
				if ([result isKindOfClass:[NSString class]])
				{
					ret_str = iupStrReturnStr([(NSString*)result UTF8String]);
				}
				else if ([result isKindOfClass:[NSNumber class]])
				{
					NSNumber* num = (NSNumber*)result;
					const char* objc_type = [num objCType];
					if (strcmp(objc_type, @encode(BOOL)) == 0 ||
					    strcmp(objc_type, @encode(char)) == 0 ||
					    strcmp(objc_type, @encode(int)) == 0)
					{
						if ([num intValue] == 0 || [num intValue] == 1)
							ret_str = iupStrReturnBoolean([num boolValue]);
						else
							ret_str = iupStrReturnStr([[num stringValue] UTF8String]);
					}
					else
					{
						ret_str = iupStrReturnStr([[num stringValue] UTF8String]);
					}
				}
				else if (result == [NSNull null] || [result isKindOfClass:[NSNull class]])
				{
					ret_str = NULL;
				}
			}
			finished = YES;
		}];

		while (!finished)
		{
			[[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode
									 beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.01]];
		}
	};

	if ([NSThread isMainThread])
	{
		run_js_and_wait();
	}
	else
	{
		dispatch_sync(dispatch_get_main_queue(), run_js_and_wait);
	}

	return ret_str;
}

static int write_file(const char* filename, const char* str, int count)
{
  FILE* file = fopen(filename, "wb");
  if (!file)
    return 0;

  fwrite(str, 1, count, file);

  fclose(file);
  return 1;
}

static void cocoaWKWebBrowserExecCommand(Ihandle* ih, const char* cmd)
{
	WKWebView* web_view = (WKWebView*)ih->handle;

	NSString* cmd_js = cocoaWKWebBrowserEscapeJavaScript(cmd);
	NSString* js_cmd = [NSString stringWithFormat:@"window.iupRestoreSelection(); document.execCommand(%@, false, null);", cmd_js];

	if ([NSThread isMainThread])
	{
		[web_view.window makeFirstResponder:web_view];
		[web_view evaluateJavaScript:js_cmd completionHandler:nil];
	}
	else
	{
		dispatch_async(dispatch_get_main_queue(), ^{
			[web_view.window makeFirstResponder:web_view];
			[web_view evaluateJavaScript:js_cmd completionHandler:nil];
		});
	}
}

static void cocoaWKWebBrowserExecCommandParam(Ihandle* ih, const char* cmd, const char* param)
{
	WKWebView* web_view = (WKWebView*)ih->handle;
	NSString* cmd_js = cocoaWKWebBrowserEscapeJavaScript(cmd);
	NSString* param_js = cocoaWKWebBrowserEscapeJavaScript(param);
	NSString* js_cmd = [NSString stringWithFormat:@"window.iupRestoreSelection(); document.execCommand(%@, false, %@);", cmd_js, param_js];

	if ([NSThread isMainThread])
	{
		[web_view.window makeFirstResponder:web_view];
		[web_view evaluateJavaScript:js_cmd completionHandler:nil];
	}
	else
	{
		dispatch_async(dispatch_get_main_queue(), ^{
			[web_view.window makeFirstResponder:web_view];
			[web_view evaluateJavaScript:js_cmd completionHandler:nil];
		});
	}
}

static int cocoaWKWebBrowserSetBackForwardAttrib(Ihandle* ih, const char* value)
{
	int val;
	if (iupStrToInt(value, &val))
	{
		WKWebView* web_view = (WKWebView*)ih->handle;
		if (val != 0)
		{
			WKBackForwardList* back_forward_list = [web_view backForwardList];
			WKBackForwardListItem* item = [back_forward_list itemAtIndex:val];
			if (item)
				[web_view goToBackForwardListItem:item];
		}
	}

	return 0;
}

static int cocoaWKWebBrowserSetStopAttrib(Ihandle* ih, const char* value)
{
	(void)value;

	WKWebView* web_view = (WKWebView*)ih->handle;
	[web_view stopLoading];

	return 0;
}

static int cocoaWKWebBrowserSetReloadAttrib(Ihandle* ih, const char* value)
{
	(void)value;

	WKWebView* web_view = (WKWebView*)ih->handle;
	[web_view reload];

	return 0;
}

static char* cocoaWKWebBrowserGetHTMLAttrib(Ihandle* ih)
{
	WKWebView* web_view = (WKWebView*)ih->handle;
	__block char* ret_str = NULL;
	__block BOOL finished = NO;

	void (^run_js_and_wait)(void) = ^{
		[web_view evaluateJavaScript:@"document.documentElement.outerHTML" completionHandler:^(id result, NSError *error) {
			if (!error && result)
			{
				if ([result isKindOfClass:[NSString class]])
				{
					NSString* html_string = (NSString*)result;
					const char* c_str = [html_string UTF8String];
					ret_str = iupStrReturnStr(c_str);
				}
			}
			finished = YES;
		}];

		while (!finished)
		{
			[[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode
									 beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.01]];
		}
	};

	if ([NSThread isMainThread])
	{
		run_js_and_wait();
	}
	else
	{
		dispatch_sync(dispatch_get_main_queue(), run_js_and_wait);
	}

	return ret_str;
}

static int cocoaWKWebBrowserSetHTMLAttrib(Ihandle* ih, const char* value)
{
	if (value)
	{
		WKWebView* web_view = (WKWebView*)ih->handle;
		NSString* html_string = [NSString stringWithUTF8String:value];
		iupAttribSet(ih, "_IUPWEB_DIRTY", NULL);
		iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", "1");
		[web_view loadHTMLString:html_string baseURL:nil];
		iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", NULL);
	}
	return 0;
}

static char* cocoaWKWebBrowserGetStatusAttrib(Ihandle* ih)
{
	WKWebView* web_view = (WKWebView*)ih->handle;
	IupAppleWKWebViewDelegate* delegate = (IupAppleWKWebViewDelegate*)objc_getAssociatedObject(web_view, IUP_APPLEWKWEBVIEW_WEBVIEW_DELEGATE_OBJ_KEY);

	if (delegate)
	{
		switch([delegate currentLoadStatus])
		{
			case IupAppleWKWebViewLoadStatusFinished:
				return "COMPLETED";
			case IupAppleWKWebViewLoadStatusFailed:
				return "FAILED";
			case IupAppleWKWebViewLoadStatusLoading:
				return "LOADING";
		}
	}

	return "COMPLETED";
}

static int cocoaWKWebBrowserSetPrintAttrib(Ihandle* ih, const char* value)
{
	(void)value;

	WKWebView* web_view = (WKWebView*)ih->handle;

	NSPrintInfo* print_info = [[NSPrintInfo sharedPrintInfo] copy];
	[print_info setHorizontalPagination:NSPrintingPaginationModeAutomatic];
	[print_info setVerticalPagination:NSPrintingPaginationModeAutomatic];
	[print_info setVerticallyCentered:NO];

	NSPrintOperation* print_operation = [NSPrintOperation printOperationWithView:web_view printInfo:print_info];
	[print_info release];

	[print_operation setShowsPrintPanel:YES];
	[print_operation setShowsProgressPanel:YES];

	[[print_operation printPanel] setOptions:NSPrintPanelShowsCopies | NSPrintPanelShowsPageRange | NSPrintPanelShowsPaperSize | NSPrintPanelShowsOrientation];

	Ihandle* dlg = IupGetDialog(ih);
	NSWindow* parent_window = NULL;
	if (dlg && dlg->handle)
	{
		parent_window = (NSWindow*)dlg->handle;
	}

	if (parent_window)
	{
		[print_operation runOperationModalForWindow:parent_window delegate:nil didRunSelector:NULL contextInfo:NULL];
	}
	else
	{
		[print_operation runOperation];
	}

	return 0;
}

static char* cocoaWKWebBrowserGetZoomAttrib(Ihandle* ih)
{
	WKWebView* web_view = (WKWebView*)ih->handle;
	int zoom = (int)([web_view pageZoom] * 100);
	return iupStrReturnInt(zoom);
}

static int cocoaWKWebBrowserSetZoomAttrib(Ihandle* ih, const char* value)
{
	int zoom;
	if (iupStrToInt(value, &zoom))
	{
		WKWebView* web_view = (WKWebView*)ih->handle;
		[web_view setPageZoom:(CGFloat)zoom / 100.0];
	}
	return 0;
}

static int cocoaWKWebBrowserSetGoBackAttrib(Ihandle* ih, const char* value)
{
	(void)value;

	WKWebView* web_view = (WKWebView*)ih->handle;
	[web_view goBack];

	return 0;
}

static int cocoaWKWebBrowserSetGoForwardAttrib(Ihandle* ih, const char* value)
{
	(void)value;

	WKWebView* web_view = (WKWebView*)ih->handle;
	[web_view goForward];

	return 0;
}

static char* cocoaWKWebBrowserGetCanGoBackAttrib(Ihandle* ih)
{
	WKWebView* web_view = (WKWebView*)ih->handle;
	return iupStrReturnBoolean([web_view canGoBack]);
}

static char* cocoaWKWebBrowserGetCanGoForwardAttrib(Ihandle* ih)
{
	WKWebView* web_view = (WKWebView*)ih->handle;
	return iupStrReturnBoolean([web_view canGoForward]);
}

static int cocoaWKWebBrowserSetOpenAttrib(Ihandle* ih, const char* value)
{
	if (value)
	{
		char* url = iupStrFileMakeURL(value);
		iupAttribSet(ih, "_IUPWEB_DIRTY", NULL);
		cocoaWKWebBrowserSetValueAttrib(ih, url);
		free(url);
	}
	return 0;
}

static int cocoaWKWebBrowserSetNewAttrib(Ihandle* ih, const char* value)
{
	iupAttribSet(ih, "_IUPWEB_DIRTY", NULL);
	cocoaWKWebBrowserSetHTMLAttrib(ih, "<html><body></body></html>");
	cocoaWKWebBrowserSetEditableAttrib(ih, "Yes");
	(void)value;
	return 0;
}

static int cocoaWKWebBrowserSetSaveAttrib(Ihandle* ih, const char* value)
{
	if (value)
	{
		const char* html_value = cocoaWKWebBrowserGetHTMLAttrib(ih);
		if (html_value)
		{
			write_file(value, html_value, (int)strlen(html_value));
		}
	}
	return 0;
}

static int cocoaWKWebBrowserSetEditableAttrib(Ihandle* ih, const char* value)
{
	if (iupStrBoolean(value))
	{
		iupAttribSet(ih, "_IUPWEB_EDITABLE", "1");
	}
	else
	{
		iupAttribSet(ih, "_IUPWEB_EDITABLE", NULL);
	}

	WKWebView* web_view = (WKWebView*)ih->handle;
	if (iupAttribGet(ih, "_IUPWEB_EDITABLE"))
	{
		[web_view evaluateJavaScript:@"document.body.contentEditable = 'true';" completionHandler:nil];
	}
	else
	{
		[web_view evaluateJavaScript:@"document.body.contentEditable = 'false';" completionHandler:nil];
	}

	return 0;
}

static char* cocoaWKWebBrowserGetEditableAttrib(Ihandle* ih)
{
	return iupStrReturnBoolean(iupAttribGet(ih, "_IUPWEB_EDITABLE") != NULL);
}

static int cocoaWKWebBrowserSetCopyAttrib(Ihandle* ih, const char* value)
{
	cocoaWKWebBrowserExecCommand(ih, "copy");
	(void)value;
	return 0;
}

static int cocoaWKWebBrowserSetCutAttrib(Ihandle* ih, const char* value)
{
	cocoaWKWebBrowserExecCommand(ih, "cut");
	(void)value;
	return 0;
}

static int cocoaWKWebBrowserSetPasteAttrib(Ihandle* ih, const char* value)
{
	cocoaWKWebBrowserExecCommand(ih, "paste");
	(void)value;
	return 0;
}

static char* cocoaWKWebBrowserGetPasteAttrib(Ihandle* ih)
{
	char* result = cocoaWKWebBrowserRunJavaScriptSync(ih, @"document.queryCommandEnabled('paste');");
	if (result)
	{
		int enabled = (strcmp(result, "true") == 0 || strcmp(result, "1") == 0);
		return iupStrReturnBoolean(enabled);
	}
	return iupStrReturnBoolean(0);
}

static int cocoaWKWebBrowserSetSelectAllAttrib(Ihandle* ih, const char* value)
{
	cocoaWKWebBrowserExecCommand(ih, "selectAll");
	(void)value;
	return 0;
}

static int cocoaWKWebBrowserSetUndoAttrib(Ihandle* ih, const char* value)
{
	cocoaWKWebBrowserExecCommand(ih, "undo");
	(void)value;
	return 0;
}

static int cocoaWKWebBrowserSetRedoAttrib(Ihandle* ih, const char* value)
{
	cocoaWKWebBrowserExecCommand(ih, "redo");
	(void)value;
	return 0;
}

static int cocoaWKWebBrowserSetExecCommandAttrib(Ihandle* ih, const char* value)
{
	if (value)
		cocoaWKWebBrowserExecCommand(ih, value);
	return 0;
}

static int cocoaWKWebBrowserSetInsertImageAttrib(Ihandle* ih, const char* value)
{
	if (value)
		cocoaWKWebBrowserExecCommandParam(ih, "insertImage", value);
	return 0;
}

static int cocoaWKWebBrowserSetInsertImageFileAttrib(Ihandle* ih, const char* value)
{
	if (value)
	{
		char* url = iupStrFileMakeURL(value);
		if (url)
		{
			cocoaWKWebBrowserExecCommandParam(ih, "insertImage", url);
			free(url);
		}
	}
	return 0;
}

static int cocoaWKWebBrowserSetCreateLinkAttrib(Ihandle* ih, const char* value)
{
	if (value)
		cocoaWKWebBrowserExecCommandParam(ih, "createLink", value);
	return 0;
}

static int cocoaWKWebBrowserSetInsertTextAttrib(Ihandle* ih, const char* value)
{
	if (value)
		cocoaWKWebBrowserExecCommandParam(ih, "insertText", value);
	return 0;
}

static int cocoaWKWebBrowserSetInsertHtmlAttrib(Ihandle* ih, const char* value)
{
	if (value)
		cocoaWKWebBrowserExecCommandParam(ih, "insertHTML", value);
	return 0;
}

static int cocoaWKWebBrowserSetFontNameAttrib(Ihandle* ih, const char* value)
{
	if (value)
		cocoaWKWebBrowserExecCommandParam(ih, "fontName", value);
	return 0;
}

static char* cocoaWKWebBrowserGetFontNameAttrib(Ihandle* ih)
{
	return cocoaWKWebBrowserRunJavaScriptSync(ih, @"document.queryCommandValue('fontName');");
}

static int cocoaWKWebBrowserSetFontSizeAttrib(Ihandle* ih, const char* value)
{
	if (value)
		cocoaWKWebBrowserExecCommandParam(ih, "fontSize", value);
	return 0;
}

static char* cocoaWKWebBrowserGetFontSizeAttrib(Ihandle* ih)
{
	return cocoaWKWebBrowserRunJavaScriptSync(ih, @"document.queryCommandValue('fontSize');");
}

static int cocoaWKWebBrowserSetFormatBlockAttrib(Ihandle* ih, const char* value)
{
	if (value)
		cocoaWKWebBrowserExecCommandParam(ih, "formatBlock", value);
	return 0;
}

static char* cocoaWKWebBrowserGetFormatBlockAttrib(Ihandle* ih)
{
	return cocoaWKWebBrowserRunJavaScriptSync(ih, @"document.queryCommandValue('formatBlock');");
}

static int cocoaWKWebBrowserSetForeColorAttrib(Ihandle* ih, const char* value)
{
	if (value)
		cocoaWKWebBrowserExecCommandParam(ih, "foreColor", value);
	return 0;
}

static char* cocoaWKWebBrowserGetForeColorAttrib(Ihandle* ih)
{
	return cocoaWKWebBrowserRunJavaScriptSync(ih, @"document.queryCommandValue('foreColor');");
}

static int cocoaWKWebBrowserSetBackColorAttrib(Ihandle* ih, const char* value)
{
	if (value)
		cocoaWKWebBrowserExecCommandParam(ih, "backColor", value);
	return 0;
}

static char* cocoaWKWebBrowserGetBackColorAttrib(Ihandle* ih)
{
	return cocoaWKWebBrowserRunJavaScriptSync(ih, @"document.queryCommandValue('backColor');");
}

static char* cocoaWKWebBrowserGetCommandStateAttrib(Ihandle* ih)
{
	char* cmd = iupAttribGet(ih, "COMMAND");
	if (cmd)
	{
		NSString* cmd_js = cocoaWKWebBrowserEscapeJavaScript(cmd);
		NSString* js_query = [NSString stringWithFormat:@"document.queryCommandState(%@);", cmd_js];
		return cocoaWKWebBrowserRunJavaScriptSync(ih, js_query);
	}
	return NULL;
}

static char* cocoaWKWebBrowserGetCommandEnabledAttrib(Ihandle* ih)
{
	char* cmd = iupAttribGet(ih, "COMMAND");
	if (cmd)
	{
		NSString* cmd_js = cocoaWKWebBrowserEscapeJavaScript(cmd);
		NSString* js_query = [NSString stringWithFormat:@"document.queryCommandEnabled(%@);", cmd_js];
		return cocoaWKWebBrowserRunJavaScriptSync(ih, js_query);
	}
	return NULL;
}

static char* cocoaWKWebBrowserGetCommandTextAttrib(Ihandle* ih)
{
	char* cmd = iupAttribGet(ih, "COMMAND");
	if (cmd)
	{
		NSString* cmd_js = cocoaWKWebBrowserEscapeJavaScript(cmd);
		NSString* js_query = [NSString stringWithFormat:@"document.queryCommandText(%@);", cmd_js];
		return cocoaWKWebBrowserRunJavaScriptSync(ih, js_query);
	}
	return NULL;
}

static char* cocoaWKWebBrowserGetCommandValueAttrib(Ihandle* ih)
{
	char* cmd = iupAttribGet(ih, "COMMAND");
	if (cmd)
	{
		NSString* cmd_js = cocoaWKWebBrowserEscapeJavaScript(cmd);
		NSString* js_query = [NSString stringWithFormat:@"document.queryCommandValue(%@);", cmd_js];
		return cocoaWKWebBrowserRunJavaScriptSync(ih, js_query);
	}
	return NULL;
}

static char* cocoaWKWebBrowserGetValueAttrib(Ihandle* ih)
{
	WKWebView* web_view = (WKWebView*)ih->handle;
	NSURL* current_url = [web_view URL];
	if (current_url)
		return iupStrReturnStr([[current_url absoluteString] UTF8String]);
	return NULL;
}

static int cocoaWKWebBrowserSetInnerTextAttrib(Ihandle* ih, const char* value)
{
	if (value)
	{
		char* element_id = iupAttribGet(ih, "ELEMENT_ID");
		if (element_id)
		{
			WKWebView* web_view = (WKWebView*)ih->handle;
			NSString* element_id_js = cocoaWKWebBrowserEscapeJavaScript(element_id);
			NSString* value_js = cocoaWKWebBrowserEscapeJavaScript(value);
			NSString* js_cmd = [NSString stringWithFormat:@"document.getElementById(%@).innerText = %@;", element_id_js, value_js];
			[web_view evaluateJavaScript:js_cmd completionHandler:nil];
		}
	}
	return 0;
}

static char* cocoaWKWebBrowserGetInnerTextAttrib(Ihandle* ih)
{
	char* element_id = iupAttribGet(ih, "ELEMENT_ID");
	if (element_id)
	{
		NSString* element_id_js = cocoaWKWebBrowserEscapeJavaScript(element_id);
		NSString* js_cmd = [NSString stringWithFormat:@"document.getElementById(%@).innerText;", element_id_js];
		return cocoaWKWebBrowserRunJavaScriptSync(ih, js_cmd);
	}
	return NULL;
}

static int cocoaWKWebBrowserSetAttributeAttrib(Ihandle* ih, const char* value)
{
	if (value)
	{
		char* element_id = iupAttribGet(ih, "ELEMENT_ID");
		char* attribute_name = iupAttribGet(ih, "ATTRIBUTE_NAME");

		if (element_id && attribute_name)
		{
			WKWebView* web_view = (WKWebView*)ih->handle;
			NSString* element_id_js = cocoaWKWebBrowserEscapeJavaScript(element_id);
			NSString* attribute_name_js = cocoaWKWebBrowserEscapeJavaScript(attribute_name);
			NSString* value_js = cocoaWKWebBrowserEscapeJavaScript(value);
			NSString* js_cmd = [NSString stringWithFormat:@"document.getElementById(%@).setAttribute(%@, %@);", element_id_js, attribute_name_js, value_js];
			[web_view evaluateJavaScript:js_cmd completionHandler:nil];
		}
	}
	return 0;
}

static char* cocoaWKWebBrowserGetAttributeAttrib(Ihandle* ih)
{
	char* element_id = iupAttribGet(ih, "ELEMENT_ID");
	char* attribute_name = iupAttribGet(ih, "ATTRIBUTE_NAME");

	if (element_id && attribute_name)
	{
		NSString* element_id_js = cocoaWKWebBrowserEscapeJavaScript(element_id);
		NSString* attribute_name_js = cocoaWKWebBrowserEscapeJavaScript(attribute_name);
		NSString* js_cmd = [NSString stringWithFormat:@"document.getElementById(%@).getAttribute(%@);", element_id_js, attribute_name_js];
		return cocoaWKWebBrowserRunJavaScriptSync(ih, js_cmd);
	}
	return NULL;
}

static char* cocoaWKWebBrowserGetBackCountAttrib(Ihandle* ih)
{
	WKWebView* web_view = (WKWebView*)ih->handle;
	WKBackForwardList* list = [web_view backForwardList];
	NSArray* back_list = [list backList];
	return iupStrReturnInt((int)[back_list count]);
}

static char* cocoaWKWebBrowserGetForwardCountAttrib(Ihandle* ih)
{
	WKWebView* web_view = (WKWebView*)ih->handle;
	WKBackForwardList* list = [web_view backForwardList];
	NSArray* forward_list = [list forwardList];
	return iupStrReturnInt((int)[forward_list count]);
}

static char* cocoaWKWebBrowserGetItemHistoryAttrib(Ihandle* ih, int id)
{
	WKWebView* web_view = (WKWebView*)ih->handle;
	WKBackForwardList* list = [web_view backForwardList];
	WKBackForwardListItem* item = [list itemAtIndex:id];

	if (item)
	{
		NSURL* url = [item URL];
		if (url)
			return iupStrReturnStr([[url absoluteString] UTF8String]);
	}

	return NULL;
}

static char* cocoaWKWebBrowserGetDirtyAttrib(Ihandle* ih)
{
	if (iupAttribGet(ih, "_IUPWEB_DIRTY"))
		return "YES";

	WKWebView* web_view = (WKWebView*)ih->handle;
	char* result = cocoaWKWebBrowserRunJavaScriptSync(ih, @"window.iupGetDirtyFlag ? window.iupGetDirtyFlag() : false;");
	if (result)
	{
		int dirty = (strcmp(result, "true") == 0 || strcmp(result, "1") == 0);
		if (dirty)
		{
			iupAttribSet(ih, "_IUPWEB_DIRTY", "1");
			[web_view evaluateJavaScript:@"if (window.iupClearDirtyFlag) window.iupClearDirtyFlag();" completionHandler:nil];
		}
		return iupStrReturnBoolean(dirty);
	}
	return "NO";
}

static int cocoaWKWebBrowserSetFindAttrib(Ihandle* ih, const char* value)
{
	if (value)
	{
		WKWebView* web_view = (WKWebView*)ih->handle;
		NSString* value_js = cocoaWKWebBrowserEscapeJavaScript(value);
		NSString* js_cmd = [NSString stringWithFormat:@"window.find(%@);", value_js];
		[web_view evaluateJavaScript:js_cmd completionHandler:nil];
	}
	return 0;
}

static int cocoaWKWebBrowserSetPrintPreviewAttrib(Ihandle* ih, const char* value)
{
	(void)value;
	return cocoaWKWebBrowserSetPrintAttrib(ih, NULL);
}

static void cocoaWKWebBrowserLayoutUpdateMethod(Ihandle* ih)
{
	iupdrvBaseLayoutUpdateMethod(ih);
}

static int cocoaWKWebBrowserMapMethod(Ihandle* ih)
{
	WKWebViewConfiguration* web_config = [[WKWebViewConfiguration alloc] init];

	WKUserContentController* user_content_controller = [[WKUserContentController alloc] init];
	[web_config setUserContentController:user_content_controller];

	NSString* js_selection_script = [NSString stringWithFormat:
		 @"(function() {"
		 "  if (window.iupSelectionInitialized) return;"
		 "  window.iupSelectionInitialized = true;"
		 "  var iupSavedRange = null;"
		 "  var iupDirtyFlag = false;"
		 "  document.addEventListener('selectionchange', function() {"
		 "    if (document.body.contentEditable == 'true') {"
		 "      var sel = window.getSelection();"
		 "      if (sel && sel.rangeCount > 0) {"
		 "        iupSavedRange = sel.getRangeAt(0).cloneRange();"
		 "        window.webkit.messageHandlers.%@.postMessage(null);"
		 "      }"
		 "    }"
		 "  });"
		 "  document.addEventListener('input', function() {"
		 "    if (document.body.contentEditable == 'true' && !iupDirtyFlag) {"
		 "      iupDirtyFlag = true;"
		 "      window.webkit.messageHandlers.%@.postMessage(null);"
		 "    }"
		 "  });"
		 "  window.iupRestoreSelection = function() {"
		 "    if (document.body.contentEditable == 'true' && iupSavedRange) {"
		 "      try {"
		 "        var sel = window.getSelection();"
		 "        sel.removeAllRanges();"
		 "        sel.addRange(iupSavedRange);"
		 "      } catch (e) {}"
		 "    }"
		 "  };"
		 "  window.iupGetDirtyFlag = function() {"
		 "    return iupDirtyFlag;"
		 "  };"
		 "  window.iupClearDirtyFlag = function() {"
		 "    iupDirtyFlag = false;"
		 "  };"
		 "})();", IupUpdateCbMessageHandlerName, IupDirtyFlagMessageHandlerName];

	WKUserScript* user_script = [[WKUserScript alloc] initWithSource:js_selection_script
	                                                    injectionTime:WKUserScriptInjectionTimeAtDocumentEnd
	                                                 forMainFrameOnly:YES];
	[user_content_controller addUserScript:user_script];
	[user_script release];

	WKWebView* web_view = [[WKWebView alloc] initWithFrame:CGRectZero configuration:web_config];

	[web_config release];
	[user_content_controller release];

	if (!web_view)
		return IUP_ERROR;

	[web_view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];

	IupAppleWKWebViewDelegate* webview_delegate = [[IupAppleWKWebViewDelegate alloc] init];
	[webview_delegate setIhandle:ih];

	WKUserContentController* webview_user_content_controller = [[web_view configuration] userContentController];
	[webview_user_content_controller addScriptMessageHandler:webview_delegate name:IupUpdateCbMessageHandlerName];
	[webview_user_content_controller addScriptMessageHandler:webview_delegate name:IupDirtyFlagMessageHandlerName];

	objc_setAssociatedObject(web_view, IUP_APPLEWKWEBVIEW_WEBVIEW_DELEGATE_OBJ_KEY, (id)webview_delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
	[webview_delegate release];

	[web_view setNavigationDelegate:webview_delegate];
	[web_view setUIDelegate:webview_delegate];

	ih->handle = web_view;
	iupCocoaSetAssociatedViews(ih, web_view, web_view);

	iupAppleWKAddToParent(ih);

	[web_view release];

	return IUP_NOERROR;
}

static void cocoaWKWebBrowserUnMapMethod(Ihandle* ih)
{
	if (!ih || !ih->handle)
		return;

	WKWebView* web_view = (WKWebView*)ih->handle;

	if (web_view)
	{
		[web_view setNavigationDelegate:nil];
		[web_view setUIDelegate:nil];

		WKUserContentController* user_content_controller = [[web_view configuration] userContentController];
		[user_content_controller removeScriptMessageHandlerForName:IupUpdateCbMessageHandlerName];
		[user_content_controller removeScriptMessageHandlerForName:IupDirtyFlagMessageHandlerName];
	}

	iupdrvBaseUnMapMethod(ih);
}

static void cocoaWKWebBrowserComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  int natural_w = 0, natural_h = 0;
  (void)children_expand;

  iupdrvFontGetCharSize(ih, &natural_w, &natural_h);

  *w = natural_w;
  *h = natural_h;
}

static int cocoaWKWebBrowserCreateMethod(Ihandle* ih, void **params)
{
	(void)params;

	ih->data = iupALLOCCTRLDATA();
	ih->data->sb = IUP_SB_HORIZ | IUP_SB_VERT;

	ih->expand = IUP_EXPAND_BOTH;
	return IUP_NOERROR;
}

static int cocoaWKWebBrowserSetValueAttrib(Ihandle* ih, const char* value)
{
	if (value)
	{
		WKWebView* web_view = (WKWebView*)ih->handle;
		iupAttribSet(ih, "_IUPWEB_DIRTY", NULL);
		if (iupStrEqualPartial(value, "http://") || iupStrEqualPartial(value, "https://") ||
		    iupStrEqualPartial(value, "file://") || iupStrEqualPartial(value, "ftp://"))
		{
			NSString* url_string = [NSString stringWithUTF8String:value];
			NSURL* ns_url = [NSURL URLWithString:url_string];
			NSURLRequest* url_request = [NSURLRequest requestWithURL:ns_url];
			iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", "1");
			[web_view loadRequest:url_request];
			iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", NULL);
		}
		else
		{
			char* url = iupStrFileMakeURL(value);
			if (url)
			{
				NSURL* ns_url = [NSURL URLWithString:[NSString stringWithUTF8String:url]];
				NSURL* read_access_url = [ns_url URLByDeletingLastPathComponent];
				iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", "1");
				[web_view loadFileURL:ns_url allowingReadAccessToURL:read_access_url];
				iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", NULL);
				free(url);
			}
		}
	}
	return 0;
}

Iclass* iupWebBrowserNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "webbrowser";
  ic->cons = "WebBrowser";
  ic->format = NULL;
  ic->nativetype  = IUP_TYPECONTROL;
  ic->childtype   = IUP_CHILDNONE;
  ic->is_interactive = 1;
  ic->has_attrib_id = 1;

  ic->New = NULL;
  ic->Create = cocoaWKWebBrowserCreateMethod;
  ic->Map = cocoaWKWebBrowserMapMethod;
  ic->UnMap = cocoaWKWebBrowserUnMapMethod;
  ic->ComputeNaturalSize = cocoaWKWebBrowserComputeNaturalSizeMethod;
  ic->LayoutUpdate = cocoaWKWebBrowserLayoutUpdateMethod;

  iupClassRegisterCallback(ic, "NEWWINDOW_CB", "s");
  iupClassRegisterCallback(ic, "NAVIGATE_CB", "s");
  iupClassRegisterCallback(ic, "ERROR_CB", "s");
  iupClassRegisterCallback(ic, "COMPLETED_CB", "s");
  iupClassRegisterCallback(ic, "UPDATE_CB", "");

  iupBaseRegisterCommonAttrib(ic);

  iupBaseRegisterVisualAttrib(ic);

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "VALUE", cocoaWKWebBrowserGetValueAttrib, cocoaWKWebBrowserSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKFORWARD", NULL, cocoaWKWebBrowserSetBackForwardAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GOBACK", NULL, cocoaWKWebBrowserSetGoBackAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GOFORWARD", NULL, cocoaWKWebBrowserSetGoForwardAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STOP", NULL, cocoaWKWebBrowserSetStopAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RELOAD", NULL, cocoaWKWebBrowserSetReloadAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HTML", cocoaWKWebBrowserGetHTMLAttrib, cocoaWKWebBrowserSetHTMLAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STATUS", cocoaWKWebBrowserGetStatusAttrib, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ZOOM", cocoaWKWebBrowserGetZoomAttrib, cocoaWKWebBrowserSetZoomAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PRINT", NULL, cocoaWKWebBrowserSetPrintAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CANGOBACK", cocoaWKWebBrowserGetCanGoBackAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CANGOFORWARD", cocoaWKWebBrowserGetCanGoForwardAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "EDITABLE", cocoaWKWebBrowserGetEditableAttrib, cocoaWKWebBrowserSetEditableAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NEW", NULL, cocoaWKWebBrowserSetNewAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OPENFILE", NULL, cocoaWKWebBrowserSetOpenAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SAVEFILE", NULL, cocoaWKWebBrowserSetSaveAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "UNDO", NULL, cocoaWKWebBrowserSetUndoAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "REDO", NULL, cocoaWKWebBrowserSetRedoAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COPY", NULL, cocoaWKWebBrowserSetCopyAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUT", NULL, cocoaWKWebBrowserSetCutAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PASTE", cocoaWKWebBrowserGetPasteAttrib, cocoaWKWebBrowserSetPasteAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTALL", NULL, cocoaWKWebBrowserSetSelectAllAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EXECCOMMAND", NULL, cocoaWKWebBrowserSetExecCommandAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTIMAGE", NULL, cocoaWKWebBrowserSetInsertImageAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTIMAGEFILE", NULL, cocoaWKWebBrowserSetInsertImageFileAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CREATELINK", NULL, cocoaWKWebBrowserSetCreateLinkAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTTEXT", NULL, cocoaWKWebBrowserSetInsertTextAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTHTML", NULL, cocoaWKWebBrowserSetInsertHtmlAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "FONTNAME", cocoaWKWebBrowserGetFontNameAttrib, cocoaWKWebBrowserSetFontNameAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FONTSIZE", cocoaWKWebBrowserGetFontSizeAttrib, cocoaWKWebBrowserSetFontSizeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATBLOCK", cocoaWKWebBrowserGetFormatBlockAttrib, cocoaWKWebBrowserSetFormatBlockAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORECOLOR", cocoaWKWebBrowserGetForeColorAttrib, cocoaWKWebBrowserSetForeColorAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKCOLOR", cocoaWKWebBrowserGetBackColorAttrib, cocoaWKWebBrowserSetBackColorAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "COMMAND", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDSHOWUI", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDSTATE", cocoaWKWebBrowserGetCommandStateAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDENABLED", cocoaWKWebBrowserGetCommandEnabledAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDTEXT", cocoaWKWebBrowserGetCommandTextAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDVALUE", cocoaWKWebBrowserGetCommandValueAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ELEMENT_ID", NULL, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INNERTEXT", cocoaWKWebBrowserGetInnerTextAttrib, cocoaWKWebBrowserSetInnerTextAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ATTRIBUTE_NAME", NULL, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ATTRIBUTE", cocoaWKWebBrowserGetAttributeAttrib, cocoaWKWebBrowserSetAttributeAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BACKCOUNT", cocoaWKWebBrowserGetBackCountAttrib, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORWARDCOUNT", cocoaWKWebBrowserGetForwardCountAttrib, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "ITEMHISTORY",  cocoaWKWebBrowserGetItemHistoryAttrib,  NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DIRTY", cocoaWKWebBrowserGetDirtyAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FIND", NULL, cocoaWKWebBrowserSetFindAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PRINTPREVIEW", NULL, cocoaWKWebBrowserSetPrintPreviewAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  return ic;
}
