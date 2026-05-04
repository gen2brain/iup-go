/** \file
 * \brief IupHelp / IupExecute (iOS UIKit)
 *
 * See Copyright Notice in "iup.h"
 */

#import <UIKit/UIKit.h>

#include <string.h>

#include "iup.h"


/* http(s) gets percent-escaped; mailto/tel/itms-apps pass through */
static NSURL* cocoaTouchHelpBuildURL(const char* url)
{
	if (!url || !*url)
	{
		return nil;
	}
	NSString* raw = [NSString stringWithUTF8String:url];
	if (!raw)
	{
		return nil;
	}
	if ([raw hasPrefix:@"http://"] || [raw hasPrefix:@"https://"])
	{
		NSString* escaped = [raw stringByAddingPercentEncodingWithAllowedCharacters:
			[NSCharacterSet URLQueryAllowedCharacterSet]];
		return escaped ? [NSURL URLWithString:escaped] : nil;
	}
	return [NSURL URLWithString:raw];
}

static int cocoaTouchOpenURL(NSURL* ns_url)
{
	if (!ns_url) return -1;
	UIApplication* app = [UIApplication sharedApplication];
	if (![app canOpenURL:ns_url])
	{
		return -2;
	}
	[app openURL:ns_url options:@{} completionHandler:nil];
	return 1;
}



IUP_API int IupHelp(const char* url)
{
	return cocoaTouchOpenURL(cocoaTouchHelpBuildURL(url));
}

/* sandbox allows URL schemes only; parameters appended as query/path */
static int cocoaTouchExecute(const char* filename, const char* parameters)
{
	if (!filename || !*filename) return -1;

	NSString* base = [NSString stringWithUTF8String:filename];
	if (!base) return -1;

	if (parameters && *parameters)
	{
		NSString* ns_params = [NSString stringWithUTF8String:parameters];
		if (ns_params)
		{
			NSString* escaped = [ns_params stringByAddingPercentEncodingWithAllowedCharacters:
				[NSCharacterSet URLQueryAllowedCharacterSet]];
			if (!escaped) return -1;
			if ([base containsString:@"?"])
			{
				base = [base stringByAppendingFormat:@"&%@", escaped];
			}
			else
			{
				base = [base stringByAppendingFormat:@"?%@", escaped];
			}
		}
	}

	return cocoaTouchOpenURL(cocoaTouchHelpBuildURL([base UTF8String]));
}

IUP_API int IupExecute(const char* filename, const char* parameters)
{
	return cocoaTouchExecute(filename, parameters);
}

IUP_API int IupExecuteWait(const char* filename, const char* parameters)
{
	/* openURL: is fire-and-forget; no public API to wait, fall back to non-wait */
	return cocoaTouchExecute(filename, parameters);
}
