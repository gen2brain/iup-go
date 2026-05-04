/** \file
 * \brief WKWebView Web Browser Control (shared across macOS Cocoa and iOS CocoaTouch drivers).
 *
 * Included from iupcocoa_webbrowser.m and iupcocoatouch_webbrowser.m after
 * each platform shim defines the hook macros declared below.
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
#import <WebKit/WKBackForwardList.h>
#import <WebKit/WKBackForwardListItem.h>

#import <objc/runtime.h>

#include <stdio.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_webbrowser.h"
#include "iup_drvfont.h"


/* Required macros the including shim must define:
     iupAppleWKBaseView           - native base UIView/NSView type name
     iupAppleWKAddToParent(ih)    - add ih->handle to its parent
     iupAppleWKRemoveFromParent(ih)
     iupAppleWKApplyAutoresize(v) - apply full-bleed autoresize mask to v
     iupAppleWKFocusForExec(v)    - make v first responder (no-op on iOS)
     iupAppleWKSetAssociatedViews(ih, v) - track main/root views (no-op on iOS)
     iupAppleWKRunPrint(ih, v)    - platform print dialog
*/

static const void* IUP_APPLEWKWEBVIEW_WEBVIEW_DELEGATE_OBJ_KEY = @"IUP_APPLEWKWEBVIEW_WEBVIEW_DELEGATE_OBJ_KEY";

static NSString* const IupUpdateCbMessageHandlerName = @"iupUpdateCb";
static NSString* const IupDirtyFlagMessageHandlerName = @"iupDirtyFlag";

static int appleWKWebBrowserSetValueAttrib(Ihandle* ih, const char* value);
static int appleWKWebBrowserSetEditableAttrib(Ihandle* ih, const char* value);
static void appleWKWebBrowserUpdateHistory(Ihandle* ih);

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
	NSURL* ns_url = [[navigation_action request] URL];
	NSString* scheme = [[ns_url scheme] lowercaseString];

	static NSSet* allowed_schemes = nil;
	static dispatch_once_t once;
	dispatch_once(&once, ^{
		allowed_schemes = [[NSSet alloc] initWithObjects:
			@"http", @"https", @"file", @"about", @"data", @"blob", nil];
	});
	if (scheme && ![allowed_schemes containsObject:scheme])
	{
		decision_handler(WKNavigationActionPolicyCancel);
		return;
	}

	if (!ih || !iupObjectCheck(ih) || iupAttribGet(ih, "_IUPWEB_IGNORE_NAVIGATE"))
	{
		decision_handler(WKNavigationActionPolicyAllow);
		return;
	}

	IFns cb = (IFns)IupGetCallback(ih, "NAVIGATE_CB");
	if (cb)
	{
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

- (WKWebView *)webView:(WKWebView*)web_view createWebViewWithConfiguration:(WKWebViewConfiguration*)webview_configuration forNavigationAction:(WKNavigationAction*)navigation_action windowFeatures:(WKWindowFeatures*)window_features
{
	Ihandle* ih = [self ihandle];
	if (!ih || !iupObjectCheck(ih)) return nil;

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
	if (!ih || !iupObjectCheck(ih)) return;

	[self setCurrentLoadStatus:IupAppleWKWebViewLoadStatusFailed];

	appleWKWebBrowserUpdateHistory(ih);

	IFns cb = (IFns)IupGetCallback(ih, "ERROR_CB");
	if (cb)
	{
		if ([the_error code] != NSURLErrorCancelled)
		{
			NSURL* error_url = [[the_error userInfo] valueForKey:NSURLErrorFailingURLErrorKey];
			if (!error_url)
				error_url = [web_view URL];
			if (error_url)
				cb(ih, (char*)[[error_url absoluteString] UTF8String]);
		}
	}
}

- (void) webView:(WKWebView*)web_view didFailNavigation:(WKNavigation*)navigation withError:(NSError*)the_error
{
	Ihandle* ih = [self ihandle];
	if (!ih || !iupObjectCheck(ih)) return;

	[self setCurrentLoadStatus:IupAppleWKWebViewLoadStatusFailed];

	appleWKWebBrowserUpdateHistory(ih);

	IFns cb = (IFns)IupGetCallback(ih, "ERROR_CB");
	if (cb)
	{
		if ([the_error code] != NSURLErrorCancelled)
		{
			NSURL* error_url = [[the_error userInfo] valueForKey:NSURLErrorFailingURLErrorKey];
			if (!error_url)
				error_url = [web_view URL];
			if (error_url)
				cb(ih, (char*)[[error_url absoluteString] UTF8String]);
		}
	}
}

- (void) webView:(WKWebView*)web_view didFinishNavigation:(WKNavigation*)navigation
{
	Ihandle* ih = [self ihandle];
	if (!ih || !iupObjectCheck(ih)) return;

	[self setCurrentLoadStatus:IupAppleWKWebViewLoadStatusFinished];

	if(iupAttribGet(ih, "_IUPWEB_EDITABLE"))
	{
		[web_view evaluateJavaScript:@"document.body.contentEditable = 'true';" completionHandler:nil];
	}
	else
	{
		[web_view evaluateJavaScript:@"document.body.contentEditable = 'false';" completionHandler:nil];
	}

	appleWKWebBrowserUpdateHistory(ih);

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
	/* KVC avoids a Mac Catalyst header clash where NSLayoutAnchor.name is
	   marked unavailable, poisoning static resolution of -name. */
	NSString* msg_name = [message valueForKey:@"name"];
	Ihandle* ih = [self ihandle];
	if (!ih || !iupObjectCheck(ih)) return;
	if ([msg_name isEqualToString:IupUpdateCbMessageHandlerName])
	{
		IFn cb_update = (IFn)IupGetCallback(ih, "UPDATE_CB");
		if (cb_update)
		{
			cb_update(ih);
		}
	}
	else if ([msg_name isEqualToString:IupDirtyFlagMessageHandlerName])
	{
		iupAttribSet(ih, "_IUPWEB_DIRTY", "1");
	}
}

@end

static void appleWKWebBrowserUpdateHistory(Ihandle* ih)
{
	WKWebView* web_view = (WKWebView*)ih->handle;
	if (!web_view)
		return;

	BOOL can_go_back = [web_view canGoBack];
	BOOL can_go_forward = [web_view canGoForward];

	iupAttribSet(ih, "CANGOBACK", can_go_back ? "YES" : "NO");
	iupAttribSet(ih, "CANGOFORWARD", can_go_forward ? "YES" : "NO");
}

static NSString* appleWKWebBrowserEscapeJavaScript(const char* c_str)
{
	if (!c_str) return @"null";
	NSString* ns_str = [NSString stringWithUTF8String:c_str];
	if (!ns_str) return @"null";

	NSArray* wrapper_array = [NSArray arrayWithObject:ns_str];
	NSData* data = [NSJSONSerialization dataWithJSONObject:wrapper_array options:0 error:nil];
	if (!data) return @"null";

	NSString* json_array_string = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
	if (!json_array_string) return @"null";

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

static char* appleWKWebBrowserRunJavaScriptSync(Ihandle* ih, NSString* js_string)
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

static char* appleWKWebBrowserGetMimeType(const char* filename)
{
  const char* ext = strrchr(filename, '.');
  if (!ext)
    return iupStrDup("image/png");

  ext++;
  if (iupStrEqualNoCase(ext, "png"))
    return iupStrDup("image/png");
  else if (iupStrEqualNoCase(ext, "jpg") || iupStrEqualNoCase(ext, "jpeg"))
    return iupStrDup("image/jpeg");
  else if (iupStrEqualNoCase(ext, "gif"))
    return iupStrDup("image/gif");
  else if (iupStrEqualNoCase(ext, "svg"))
    return iupStrDup("image/svg+xml");
  else if (iupStrEqualNoCase(ext, "webp"))
    return iupStrDup("image/webp");
  else if (iupStrEqualNoCase(ext, "bmp"))
    return iupStrDup("image/bmp");

  return iupStrDup("image/png");
}

static char* appleWKWebBrowserFileToDataURI(const char* filename)
{
  if (!filename) return NULL;
  NSString* path = [NSString stringWithUTF8String:filename];
  if (!path) return NULL;

  NSError* err = nil;
  NSDictionary* attrs = [[NSFileManager defaultManager] attributesOfItemAtPath:path error:&err];
  if (err || !attrs) return NULL;
  unsigned long long sz = [attrs fileSize];
  if (sz == 0 || sz > (10ULL * 1024 * 1024)) return NULL;

  NSData* data = [NSData dataWithContentsOfFile:path options:NSDataReadingMappedIfSafe error:&err];
  if (err || !data) return NULL;

  NSString* base64 = [data base64EncodedStringWithOptions:0];
  if (!base64) return NULL;

  char* mime_type = appleWKWebBrowserGetMimeType(filename);
  if (!mime_type) return NULL;
  const char* base64_cstr = [base64 UTF8String];
  if (!base64_cstr) { free(mime_type); return NULL; }

  size_t data_uri_len = strlen(mime_type) + strlen(base64_cstr) + 50;
  char* data_uri = (char*)malloc(data_uri_len);
  if (!data_uri) { free(mime_type); return NULL; }

  snprintf(data_uri, data_uri_len, "data:%s;base64,%s", mime_type, base64_cstr);
  free(mime_type);
  return data_uri;
}

static void appleWKWebBrowserExecCommand(Ihandle* ih, const char* cmd)
{
	WKWebView* web_view = (WKWebView*)ih->handle;

	NSString* cmd_js = appleWKWebBrowserEscapeJavaScript(cmd);
	NSString* js_cmd = [NSString stringWithFormat:@"document.body.focus(); if (window.iupRestoreSelection) window.iupRestoreSelection(); document.execCommand(%@, false, null);", cmd_js];

	if ([NSThread isMainThread])
	{
		iupAppleWKFocusForExec(web_view);
		[web_view evaluateJavaScript:js_cmd completionHandler:nil];
	}
	else
	{
		dispatch_async(dispatch_get_main_queue(), ^{
			iupAppleWKFocusForExec(web_view);
			[web_view evaluateJavaScript:js_cmd completionHandler:nil];
		});
	}
}

static void appleWKWebBrowserExecCommandParam(Ihandle* ih, const char* cmd, const char* param)
{
	WKWebView* web_view = (WKWebView*)ih->handle;
	NSString* cmd_js = appleWKWebBrowserEscapeJavaScript(cmd);
	NSString* param_js = appleWKWebBrowserEscapeJavaScript(param);
	NSString* js_cmd = [NSString stringWithFormat:@"document.body.focus(); if (window.iupRestoreSelection) window.iupRestoreSelection(); document.execCommand(%@, false, %@);", cmd_js, param_js];

	if ([NSThread isMainThread])
	{
		iupAppleWKFocusForExec(web_view);
		[web_view evaluateJavaScript:js_cmd completionHandler:nil];
	}
	else
	{
		dispatch_async(dispatch_get_main_queue(), ^{
			iupAppleWKFocusForExec(web_view);
			[web_view evaluateJavaScript:js_cmd completionHandler:nil];
		});
	}
}

static int appleWKWebBrowserSetBackForwardAttrib(Ihandle* ih, const char* value)
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

static int appleWKWebBrowserSetStopAttrib(Ihandle* ih, const char* value)
{
	(void)value;

	WKWebView* web_view = (WKWebView*)ih->handle;
	[web_view stopLoading];

	return 0;
}

static int appleWKWebBrowserSetReloadAttrib(Ihandle* ih, const char* value)
{
	(void)value;

	WKWebView* web_view = (WKWebView*)ih->handle;
	[web_view reload];

	return 0;
}

static char* appleWKWebBrowserGetHTMLAttrib(Ihandle* ih)
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

static int appleWKWebBrowserSetHTMLAttrib(Ihandle* ih, const char* value)
{
	if (value)
	{
		WKWebView* web_view = (WKWebView*)ih->handle;
		NSString* html_string = [NSString stringWithUTF8String:value];
		if (!html_string)
			return 0;
		iupAttribSet(ih, "_IUPWEB_DIRTY", NULL);
		iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", "1");
		[web_view loadHTMLString:html_string baseURL:nil];
		iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", NULL);
	}
	return 0;
}

static char* appleWKWebBrowserGetStatusAttrib(Ihandle* ih)
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

static int appleWKWebBrowserSetPrintAttrib(Ihandle* ih, const char* value)
{
	(void)value;
	WKWebView* web_view = (WKWebView*)ih->handle;
	iupAppleWKRunPrint(ih, web_view);
	return 0;
}

static char* appleWKWebBrowserGetZoomAttrib(Ihandle* ih)
{
	WKWebView* web_view = (WKWebView*)ih->handle;
	int zoom = (int)([web_view pageZoom] * 100);
	return iupStrReturnInt(zoom);
}

static int appleWKWebBrowserSetZoomAttrib(Ihandle* ih, const char* value)
{
	int zoom;
	if (iupStrToInt(value, &zoom))
	{
		WKWebView* web_view = (WKWebView*)ih->handle;
		[web_view setPageZoom:(CGFloat)zoom / 100.0];
	}
	return 0;
}

static int appleWKWebBrowserSetGoBackAttrib(Ihandle* ih, const char* value)
{
	(void)value;

	WKWebView* web_view = (WKWebView*)ih->handle;
	[web_view goBack];

	return 0;
}

static int appleWKWebBrowserSetGoForwardAttrib(Ihandle* ih, const char* value)
{
	(void)value;

	WKWebView* web_view = (WKWebView*)ih->handle;
	[web_view goForward];

	return 0;
}

static char* appleWKWebBrowserGetCanGoBackAttrib(Ihandle* ih)
{
	WKWebView* web_view = (WKWebView*)ih->handle;
	return iupStrReturnBoolean([web_view canGoBack]);
}

static char* appleWKWebBrowserGetCanGoForwardAttrib(Ihandle* ih)
{
	WKWebView* web_view = (WKWebView*)ih->handle;
	return iupStrReturnBoolean([web_view canGoForward]);
}

static int appleWKWebBrowserSetOpenAttrib(Ihandle* ih, const char* value)
{
	if (value)
	{
		char* url = iupStrFileMakeURL(value);
		iupAttribSet(ih, "_IUPWEB_DIRTY", NULL);
		appleWKWebBrowserSetValueAttrib(ih, url);
		free(url);
	}
	return 0;
}

static int appleWKWebBrowserSetNewAttrib(Ihandle* ih, const char* value)
{
	iupAttribSet(ih, "_IUPWEB_DIRTY", NULL);
	appleWKWebBrowserSetHTMLAttrib(ih, "<html><body></body></html>");
	appleWKWebBrowserSetEditableAttrib(ih, "Yes");
	(void)value;
	return 0;
}

static int appleWKWebBrowserSetSaveAttrib(Ihandle* ih, const char* value)
{
	if (value)
	{
		const char* html_value = appleWKWebBrowserGetHTMLAttrib(ih);
		if (html_value)
		{
			write_file(value, html_value, (int)strlen(html_value));
		}
	}
	return 0;
}

static int appleWKWebBrowserSetEditableAttrib(Ihandle* ih, const char* value)
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

static char* appleWKWebBrowserGetEditableAttrib(Ihandle* ih)
{
	return iupStrReturnBoolean(iupAttribGet(ih, "_IUPWEB_EDITABLE") != NULL);
}

static int appleWKWebBrowserSetCopyAttrib(Ihandle* ih, const char* value)
{
	appleWKWebBrowserExecCommand(ih, "copy");
	(void)value;
	return 0;
}

static int appleWKWebBrowserSetCutAttrib(Ihandle* ih, const char* value)
{
	appleWKWebBrowserExecCommand(ih, "cut");
	(void)value;
	return 0;
}

static int appleWKWebBrowserSetPasteAttrib(Ihandle* ih, const char* value)
{
	appleWKWebBrowserExecCommand(ih, "paste");
	(void)value;
	return 0;
}

static char* appleWKWebBrowserGetPasteAttrib(Ihandle* ih)
{
	char* result = appleWKWebBrowserRunJavaScriptSync(ih, @"document.queryCommandEnabled('paste');");
	if (result)
	{
		int enabled = (strcmp(result, "true") == 0 || strcmp(result, "1") == 0);
		return iupStrReturnBoolean(enabled);
	}
	return iupStrReturnBoolean(0);
}

static int appleWKWebBrowserSetSelectAllAttrib(Ihandle* ih, const char* value)
{
	appleWKWebBrowserExecCommand(ih, "selectAll");
	(void)value;
	return 0;
}

static int appleWKWebBrowserSetUndoAttrib(Ihandle* ih, const char* value)
{
	appleWKWebBrowserExecCommand(ih, "undo");
	(void)value;
	return 0;
}

static int appleWKWebBrowserSetRedoAttrib(Ihandle* ih, const char* value)
{
	appleWKWebBrowserExecCommand(ih, "redo");
	(void)value;
	return 0;
}

static int appleWKWebBrowserSetExecCommandAttrib(Ihandle* ih, const char* value)
{
	if (value)
		appleWKWebBrowserExecCommand(ih, value);
	return 0;
}

static int appleWKWebBrowserSetInsertImageAttrib(Ihandle* ih, const char* value)
{
	if (value)
		appleWKWebBrowserExecCommandParam(ih, "insertImage", value);
	return 0;
}

static int appleWKWebBrowserSetInsertImageFileAttrib(Ihandle* ih, const char* value)
{
	if (!value)
		return 0;

	char* data_uri = appleWKWebBrowserFileToDataURI(value);
	if (data_uri)
	{
		appleWKWebBrowserSetInsertImageAttrib(ih, data_uri);
		free(data_uri);
	}

	return 0;
}

static int appleWKWebBrowserSetCreateLinkAttrib(Ihandle* ih, const char* value)
{
	if (value)
		appleWKWebBrowserExecCommandParam(ih, "createLink", value);
	return 0;
}

static int appleWKWebBrowserSetInsertTextAttrib(Ihandle* ih, const char* value)
{
	if (value)
		appleWKWebBrowserExecCommandParam(ih, "insertText", value);
	return 0;
}

static int appleWKWebBrowserSetInsertHtmlAttrib(Ihandle* ih, const char* value)
{
	if (value)
		appleWKWebBrowserExecCommandParam(ih, "insertHTML", value);
	return 0;
}

static int appleWKWebBrowserSetFontNameAttrib(Ihandle* ih, const char* value)
{
	if (value)
		appleWKWebBrowserExecCommandParam(ih, "fontName", value);
	return 0;
}

static char* appleWKWebBrowserGetFontNameAttrib(Ihandle* ih)
{
	return appleWKWebBrowserRunJavaScriptSync(ih, @"document.queryCommandValue('fontName');");
}

static int appleWKWebBrowserSetFontSizeAttrib(Ihandle* ih, const char* value)
{
	if (value)
		appleWKWebBrowserExecCommandParam(ih, "fontSize", value);
	return 0;
}

static char* appleWKWebBrowserGetFontSizeAttrib(Ihandle* ih)
{
	return appleWKWebBrowserRunJavaScriptSync(ih, @"document.queryCommandValue('fontSize');");
}

static int appleWKWebBrowserSetFormatBlockAttrib(Ihandle* ih, const char* value)
{
	if (value)
		appleWKWebBrowserExecCommandParam(ih, "formatBlock", value);
	return 0;
}

static char* appleWKWebBrowserGetFormatBlockAttrib(Ihandle* ih)
{
	return appleWKWebBrowserRunJavaScriptSync(ih, @"document.queryCommandValue('formatBlock');");
}

static int appleWKWebBrowserSetForeColorAttrib(Ihandle* ih, const char* value)
{
	if (value)
	{
		unsigned char r, g, b;
		if (iupStrToRGB(value, &r, &g, &b))
		{
			char rgb_color[32];
			snprintf(rgb_color, sizeof(rgb_color), "rgb(%d,%d,%d)", r, g, b);
			appleWKWebBrowserExecCommandParam(ih, "foreColor", rgb_color);
		}
	}
	return 0;
}

static char* appleWKWebBrowserGetForeColorAttrib(Ihandle* ih)
{
	return appleWKWebBrowserRunJavaScriptSync(ih, @"document.queryCommandValue('foreColor');");
}

static int appleWKWebBrowserSetBackColorAttrib(Ihandle* ih, const char* value)
{
	if (value)
	{
		unsigned char r, g, b;
		if (iupStrToRGB(value, &r, &g, &b))
		{
			char rgb_color[32];
			snprintf(rgb_color, sizeof(rgb_color), "rgb(%d,%d,%d)", r, g, b);
			appleWKWebBrowserExecCommandParam(ih, "backColor", rgb_color);
		}
	}
	return 0;
}

static char* appleWKWebBrowserGetBackColorAttrib(Ihandle* ih)
{
	return appleWKWebBrowserRunJavaScriptSync(ih, @"document.queryCommandValue('backColor');");
}

static char* appleWKWebBrowserGetCommandStateAttrib(Ihandle* ih)
{
	char* cmd = iupAttribGet(ih, "COMMAND");
	if (cmd)
	{
		NSString* cmd_js = appleWKWebBrowserEscapeJavaScript(cmd);
		NSString* js_query = [NSString stringWithFormat:@"document.queryCommandState(%@);", cmd_js];
		return appleWKWebBrowserRunJavaScriptSync(ih, js_query);
	}
	return NULL;
}

static char* appleWKWebBrowserGetCommandEnabledAttrib(Ihandle* ih)
{
	char* cmd = iupAttribGet(ih, "COMMAND");
	if (cmd)
	{
		NSString* cmd_js = appleWKWebBrowserEscapeJavaScript(cmd);
		NSString* js_query = [NSString stringWithFormat:@"document.queryCommandEnabled(%@);", cmd_js];
		return appleWKWebBrowserRunJavaScriptSync(ih, js_query);
	}
	return NULL;
}

static char* appleWKWebBrowserGetCommandTextAttrib(Ihandle* ih)
{
	char* cmd = iupAttribGet(ih, "COMMAND");
	if (cmd)
	{
		NSString* cmd_js = appleWKWebBrowserEscapeJavaScript(cmd);
		NSString* js_query = [NSString stringWithFormat:@"document.queryCommandText(%@);", cmd_js];
		return appleWKWebBrowserRunJavaScriptSync(ih, js_query);
	}
	return NULL;
}

static char* appleWKWebBrowserGetCommandValueAttrib(Ihandle* ih)
{
	char* cmd = iupAttribGet(ih, "COMMAND");
	if (cmd)
	{
		NSString* cmd_js = appleWKWebBrowserEscapeJavaScript(cmd);
		NSString* js_query = [NSString stringWithFormat:@"document.queryCommandValue(%@);", cmd_js];
		return appleWKWebBrowserRunJavaScriptSync(ih, js_query);
	}
	return NULL;
}

static char* appleWKWebBrowserGetValueAttrib(Ihandle* ih)
{
	WKWebView* web_view = (WKWebView*)ih->handle;
	NSURL* current_url = [web_view URL];
	if (current_url)
		return iupStrReturnStr([[current_url absoluteString] UTF8String]);
	return NULL;
}

static int appleWKWebBrowserSetInnerTextAttrib(Ihandle* ih, const char* value)
{
	if (value)
	{
		char* element_id = iupAttribGet(ih, "ELEMENT_ID");
		if (element_id)
		{
			WKWebView* web_view = (WKWebView*)ih->handle;
			NSString* element_id_js = appleWKWebBrowserEscapeJavaScript(element_id);
			NSString* value_js = appleWKWebBrowserEscapeJavaScript(value);
			NSString* js_cmd = [NSString stringWithFormat:@"document.getElementById(%@).innerText = %@;", element_id_js, value_js];
			[web_view evaluateJavaScript:js_cmd completionHandler:nil];
		}
	}
	return 0;
}

static char* appleWKWebBrowserGetInnerTextAttrib(Ihandle* ih)
{
	char* element_id = iupAttribGet(ih, "ELEMENT_ID");
	if (element_id)
	{
		NSString* element_id_js = appleWKWebBrowserEscapeJavaScript(element_id);
		NSString* js_cmd = [NSString stringWithFormat:@"document.getElementById(%@).innerText;", element_id_js];
		return appleWKWebBrowserRunJavaScriptSync(ih, js_cmd);
	}
	return NULL;
}

static char* appleWKWebBrowserGetJavascriptAttrib(Ihandle* ih)
{
	return iupAttribGet(ih, "_IUPWEB_JS_RESULT");
}

static int appleWKWebBrowserSetJavascriptAttrib(Ihandle* ih, const char* value)
{
	iupAttribSet(ih, "_IUPWEB_JS_RESULT", NULL);
	if (!value)
		return 0;

	NSString* js_string = [NSString stringWithUTF8String:value];
	if (!js_string)
		return 0;
	char* result = appleWKWebBrowserRunJavaScriptSync(ih, js_string);
	if (result)
		iupAttribSetStr(ih, "_IUPWEB_JS_RESULT", result);
	return 0;
}

static int appleWKWebBrowserSetAttributeAttrib(Ihandle* ih, const char* value)
{
	if (value)
	{
		char* element_id = iupAttribGet(ih, "ELEMENT_ID");
		char* attribute_name = iupAttribGet(ih, "ATTRIBUTE_NAME");

		if (element_id && attribute_name)
		{
			WKWebView* web_view = (WKWebView*)ih->handle;
			NSString* element_id_js = appleWKWebBrowserEscapeJavaScript(element_id);
			NSString* attribute_name_js = appleWKWebBrowserEscapeJavaScript(attribute_name);
			NSString* value_js = appleWKWebBrowserEscapeJavaScript(value);
			NSString* js_cmd = [NSString stringWithFormat:@"document.getElementById(%@).setAttribute(%@, %@);", element_id_js, attribute_name_js, value_js];
			[web_view evaluateJavaScript:js_cmd completionHandler:nil];
		}
	}
	return 0;
}

static char* appleWKWebBrowserGetAttributeAttrib(Ihandle* ih)
{
	char* element_id = iupAttribGet(ih, "ELEMENT_ID");
	char* attribute_name = iupAttribGet(ih, "ATTRIBUTE_NAME");

	if (element_id && attribute_name)
	{
		NSString* element_id_js = appleWKWebBrowserEscapeJavaScript(element_id);
		NSString* attribute_name_js = appleWKWebBrowserEscapeJavaScript(attribute_name);
		NSString* js_cmd = [NSString stringWithFormat:@"document.getElementById(%@).getAttribute(%@);", element_id_js, attribute_name_js];
		return appleWKWebBrowserRunJavaScriptSync(ih, js_cmd);
	}
	return NULL;
}

static char* appleWKWebBrowserGetBackCountAttrib(Ihandle* ih)
{
	WKWebView* web_view = (WKWebView*)ih->handle;
	WKBackForwardList* list = [web_view backForwardList];
	NSArray* back_list = [list backList];
	return iupStrReturnInt((int)[back_list count]);
}

static char* appleWKWebBrowserGetForwardCountAttrib(Ihandle* ih)
{
	WKWebView* web_view = (WKWebView*)ih->handle;
	WKBackForwardList* list = [web_view backForwardList];
	NSArray* forward_list = [list forwardList];
	return iupStrReturnInt((int)[forward_list count]);
}

static char* appleWKWebBrowserGetItemHistoryAttrib(Ihandle* ih, int id)
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

static char* appleWKWebBrowserGetDirtyAttrib(Ihandle* ih)
{
	if (iupAttribGet(ih, "_IUPWEB_DIRTY"))
		return "YES";

	WKWebView* web_view = (WKWebView*)ih->handle;
	char* result = appleWKWebBrowserRunJavaScriptSync(ih, @"window.iupGetDirtyFlag ? window.iupGetDirtyFlag() : false;");
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

static int appleWKWebBrowserSetFindAttrib(Ihandle* ih, const char* value)
{
	if (value)
	{
		WKWebView* web_view = (WKWebView*)ih->handle;
		NSString* value_js = appleWKWebBrowserEscapeJavaScript(value);
		NSString* js_cmd = [NSString stringWithFormat:@"window.find(%@);", value_js];
		[web_view evaluateJavaScript:js_cmd completionHandler:nil];
	}
	return 0;
}

static int appleWKWebBrowserSetPrintPreviewAttrib(Ihandle* ih, const char* value)
{
	(void)value;
	return appleWKWebBrowserSetPrintAttrib(ih, NULL);
}

static void appleWKWebBrowserLayoutUpdateMethod(Ihandle* ih)
{
	iupdrvBaseLayoutUpdateMethod(ih);
}

static int appleWKWebBrowserMapMethod(Ihandle* ih)
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

	iupAppleWKApplyAutoresize(web_view);

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
	iupAppleWKSetAssociatedViews(ih, web_view);

	iupAppleWKAddToParent(ih);

	[web_view release];

	return IUP_NOERROR;
}

static void appleWKWebBrowserUnMapMethod(Ihandle* ih)
{
	if (!ih || !ih->handle)
		return;

	WKWebView* web_view = (WKWebView*)ih->handle;

	if (web_view)
	{
		IupAppleWKWebViewDelegate* delegate = (IupAppleWKWebViewDelegate*)objc_getAssociatedObject(web_view, IUP_APPLEWKWEBVIEW_WEBVIEW_DELEGATE_OBJ_KEY);
		if (delegate)
			[delegate setIhandle:NULL];

		[web_view setNavigationDelegate:nil];
		[web_view setUIDelegate:nil];
		[web_view stopLoading];

		WKUserContentController* user_content_controller = [[web_view configuration] userContentController];
		[user_content_controller removeScriptMessageHandlerForName:IupUpdateCbMessageHandlerName];
		[user_content_controller removeScriptMessageHandlerForName:IupDirtyFlagMessageHandlerName];
	}

	iupdrvBaseUnMapMethod(ih);
}

static void appleWKWebBrowserComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  int natural_w = 0, natural_h = 0;
  (void)children_expand;

  iupdrvFontGetCharSize(ih, &natural_w, &natural_h);

  *w = natural_w;
  *h = natural_h;
}

static int appleWKWebBrowserCreateMethod(Ihandle* ih, void **params)
{
	(void)params;

	ih->data = iupALLOCCTRLDATA();
	ih->data->sb = IUP_SB_HORIZ | IUP_SB_VERT;

	ih->expand = IUP_EXPAND_BOTH;
	return IUP_NOERROR;
}

static int appleWKWebBrowserSetValueAttrib(Ihandle* ih, const char* value)
{
	if (value)
	{
		WKWebView* web_view = (WKWebView*)ih->handle;
		iupAttribSet(ih, "_IUPWEB_DIRTY", NULL);
		if (iupStrEqualPartial(value, "http://") || iupStrEqualPartial(value, "https://") ||
		    iupStrEqualPartial(value, "ftp://"))
		{
			NSString* url_string = [NSString stringWithUTF8String:value];
			if (url_string)
			{
				NSURL* ns_url = [NSURL URLWithString:url_string];
				if (ns_url)
				{
					NSURLRequest* url_request = [NSURLRequest requestWithURL:ns_url];
					iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", "1");
					[web_view loadRequest:url_request];
					iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", NULL);
				}
			}
		}
		else
		{
			char* url = iupStrEqualPartial(value, "file://") ? iupStrDup(value) : iupStrFileMakeURL(value);
			if (url)
			{
				NSString* url_string = [NSString stringWithUTF8String:url];
				if (url_string)
				{
					NSURL* ns_url = [NSURL URLWithString:url_string];
					if (ns_url && [ns_url isFileURL])
					{
						NSURL* read_access_url = [ns_url URLByDeletingLastPathComponent];
						if (!read_access_url)
							read_access_url = ns_url;
						iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", "1");
						[web_view loadFileURL:ns_url allowingReadAccessToURL:read_access_url];
						iupAttribSet(ih, "_IUPWEB_IGNORE_NAVIGATE", NULL);
					}
				}
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
  ic->Create = appleWKWebBrowserCreateMethod;
  ic->Map = appleWKWebBrowserMapMethod;
  ic->UnMap = appleWKWebBrowserUnMapMethod;
  ic->ComputeNaturalSize = appleWKWebBrowserComputeNaturalSizeMethod;
  ic->LayoutUpdate = appleWKWebBrowserLayoutUpdateMethod;

  iupClassRegisterCallback(ic, "NEWWINDOW_CB", "s");
  iupClassRegisterCallback(ic, "NAVIGATE_CB", "s");
  iupClassRegisterCallback(ic, "ERROR_CB", "s");
  iupClassRegisterCallback(ic, "COMPLETED_CB", "s");
  iupClassRegisterCallback(ic, "UPDATE_CB", "");

  iupBaseRegisterCommonAttrib(ic);

  iupBaseRegisterVisualAttrib(ic);

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "VALUE", appleWKWebBrowserGetValueAttrib, appleWKWebBrowserSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKFORWARD", NULL, appleWKWebBrowserSetBackForwardAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GOBACK", NULL, appleWKWebBrowserSetGoBackAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GOFORWARD", NULL, appleWKWebBrowserSetGoForwardAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STOP", NULL, appleWKWebBrowserSetStopAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RELOAD", NULL, appleWKWebBrowserSetReloadAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HTML", appleWKWebBrowserGetHTMLAttrib, appleWKWebBrowserSetHTMLAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STATUS", appleWKWebBrowserGetStatusAttrib, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ZOOM", appleWKWebBrowserGetZoomAttrib, appleWKWebBrowserSetZoomAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PRINT", NULL, appleWKWebBrowserSetPrintAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CANGOBACK", appleWKWebBrowserGetCanGoBackAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CANGOFORWARD", appleWKWebBrowserGetCanGoForwardAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "EDITABLE", appleWKWebBrowserGetEditableAttrib, appleWKWebBrowserSetEditableAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NEW", NULL, appleWKWebBrowserSetNewAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OPENFILE", NULL, appleWKWebBrowserSetOpenAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SAVEFILE", NULL, appleWKWebBrowserSetSaveAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "UNDO", NULL, appleWKWebBrowserSetUndoAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "REDO", NULL, appleWKWebBrowserSetRedoAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COPY", NULL, appleWKWebBrowserSetCopyAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUT", NULL, appleWKWebBrowserSetCutAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PASTE", appleWKWebBrowserGetPasteAttrib, appleWKWebBrowserSetPasteAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTALL", NULL, appleWKWebBrowserSetSelectAllAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EXECCOMMAND", NULL, appleWKWebBrowserSetExecCommandAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTIMAGE", NULL, appleWKWebBrowserSetInsertImageAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTIMAGEFILE", NULL, appleWKWebBrowserSetInsertImageFileAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CREATELINK", NULL, appleWKWebBrowserSetCreateLinkAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTTEXT", NULL, appleWKWebBrowserSetInsertTextAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTHTML", NULL, appleWKWebBrowserSetInsertHtmlAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "FONTNAME", appleWKWebBrowserGetFontNameAttrib, appleWKWebBrowserSetFontNameAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FONTSIZE", appleWKWebBrowserGetFontSizeAttrib, appleWKWebBrowserSetFontSizeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATBLOCK", appleWKWebBrowserGetFormatBlockAttrib, appleWKWebBrowserSetFormatBlockAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORECOLOR", appleWKWebBrowserGetForeColorAttrib, appleWKWebBrowserSetForeColorAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKCOLOR", appleWKWebBrowserGetBackColorAttrib, appleWKWebBrowserSetBackColorAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "COMMAND", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDSHOWUI", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDSTATE", appleWKWebBrowserGetCommandStateAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDENABLED", appleWKWebBrowserGetCommandEnabledAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDTEXT", appleWKWebBrowserGetCommandTextAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDVALUE", appleWKWebBrowserGetCommandValueAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ELEMENT_ID", NULL, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INNERTEXT", appleWKWebBrowserGetInnerTextAttrib, appleWKWebBrowserSetInnerTextAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ATTRIBUTE_NAME", NULL, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ATTRIBUTE", appleWKWebBrowserGetAttributeAttrib, appleWKWebBrowserSetAttributeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "JAVASCRIPT", appleWKWebBrowserGetJavascriptAttrib, appleWKWebBrowserSetJavascriptAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BACKCOUNT", appleWKWebBrowserGetBackCountAttrib, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORWARDCOUNT", appleWKWebBrowserGetForwardCountAttrib, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "ITEMHISTORY",  appleWKWebBrowserGetItemHistoryAttrib,  NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DIRTY", appleWKWebBrowserGetDirtyAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FIND", NULL, appleWKWebBrowserSetFindAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PRINTPREVIEW", NULL, appleWKWebBrowserSetPrintPreviewAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  return ic;
}
