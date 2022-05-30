//
//  IupAppDelegate.m
//  iup
//
//  Created by Eric Wing on 12/23/16.
//  Copyright © 2016 Tecgraf, PUC-Rio, Brazil. All rights reserved.
//

#import "IupAppDelegate.h"
#include <dlfcn.h>
#include "iup.h"
#include "iupcbs.h"
#include "iup_loop.h"
#include "iupcocoa_drv.h"

@implementation IupAppDelegate

// This is the callback for things like people double-clicking on an associated file, or dragging a file onto the Dock icon
// If the user opens the app via a file, this gets called before applicationDidFinishLaunching: (but after applicationWillFinishLaunching:)
/* We need to make some decisions on how to launch an IUP app supporting this kind of feature, and also URLs and Universal Deep App Links:
- Change ENTRYPOINT to pass back parameters (or set attributes the user can query).
	- Downsides: This means deferred actions, so if the method expects the user to return true/false, that is not possible.
			- Unless: We can trigger ENTRYPOINT with attributes at the correct time, and we need to change the ENTRYPOINT signature to return int to signal the caller.
		- Also, we still need a callback to handle the case where the app is already running.

- Add new specific callbacks (ENTRYPOINTFILE, ENTRYPOINTURL, ENTRYPOINTAPPLINK).
	- Downsides: Must decide if ENTRYPOINT is still called, and if so, decide if before or after.
	- User needs to be aware of more possible launch start points.
	- If the user put all their init code into ENTRYPOINT, they will probably have to do some annoying refactoring to figure out how to share code.
 
 
 - Since application:openFiles: doesn't have a return value, and application:openFile:'s return value doesn't seem to do anything,
	- it seems a separate callback that fires after ENTRYPOINT may be the easiest for the users.

- We should compare with other platforms before making a final decision.

// Update: This is also the callback Apple uses to open a file from the Recent File Menu.
// This adds an additional constraint because we cannot distinguish what action invoked this.

*/
// NOTE: application:openFiles: overrides application:openFile:.
// If application:openFiles: is defined, application:openFile: will never be invoked.
// Instead application:openFiles: will be invoked, but with only file in the array.
// And if multiple files are dragged when application:openFile: is ued, it just gets invoked multiple times.
// So it might be preferrable to use application:openFiles:
//
// NOTE: You must declare file types in your Info.plist for this to take effect.
#if 0
- (BOOL) application:(NSApplication *)the_sender openFile:(NSString*)file_name
{
    NSLog(@"application:openFile: %@", file_name);
	// For interest, I tried returning NO, but I did not see anything happen.
	// So maybe it is not really important, especially since application:openFile: lacks a return value.
    return YES;
}
#else
- (void) application:(NSApplication*)the_sender openFiles:(NSArray<NSString*>*)file_names
{
//    NSLog(@"application:openFiles: %@", file_names);

	if([self isInitialized])
	{
		[self doOpenFilesCallback:file_names];
	}
	else
	{
		// need to save array of files until after we finish initialization
		[self setDeferredOpenFiles:file_names];
	}
}
#endif

- (void) doOpenFilesCallback:(NSArray<NSString*>*)file_names
{
//	NSLog(@"doOpenFilesCallback: %@", file_names);
	// DROPFILES_CB is closest to this.
	// "When several files are dropped at once, the callback is called several times, once for each file."
	// typedef int (*IFnsiii)(Ihandle*, char*, int, int, int);  /* dropfiles_cb */
	// We probably should create a new signature that doesn't have the trailing x,y pos at the end.
	// Also, this is a global application callback which doesn't have a Ihandle*.
	//
	// Update: After the RECENT_CB problem, I decided to use globalmotion_cb, IFiis
	IFiis open_callback = (IFiis)IupGetFunction("OPENFILES_CB");
	if(open_callback)
	{
		int total_number_of_files = (int)[file_names count];
		int i = 0;
		for(NSString* ns_current_file in file_names)
		{
			const char* c_current_file = [ns_current_file fileSystemRepresentation];
			open_callback(i, total_number_of_files, (char*)c_current_file);
			i++;
		}
	}
}
- (void) doOpenURLCallback:(NSString*)url_string
{
//	NSLog(@"doOpenURLCallback: %@", url_string);
	// DROPFILES_CB is closest to this.
	// "When several files are dropped at once, the callback is called several times, once for each file."
	// typedef int (*IFnsiii)(Ihandle*, char*, int, int, int);  /* dropfiles_cb */
	// We probably should create a new signature that doesn't have the trailing x,y pos at the end.
	// Also, this is a global application callback which doesn't have a Ihandle*.
	//
	// Update: After the RECENT_CB problem, I decided to create a new type IFs.
	
	
	IFs open_callback = (IFs)IupGetFunction("OPENURL_CB");
	if(open_callback)
	{
		const char* c_url_string = [url_string UTF8String];
		open_callback((char*)c_url_string);
	}
}


/*
Look to TextMate, MacVim, and other good Mac apps for custom URL schemes.
https://macromates.com/blog/2007/the-textmate-url-scheme/
txmt://open?url=file:///etc/profile
NOTE: You must register your URL schemes you handle in your Info.plist (CFBundleURLSchemes) for this to take effect.
NOTE: If the application is not already open, opening the URL will launch your program and handleOpenURL: gets called before applicationDidFinishLaunching:
*/
- (void) handleOpenURL:(NSAppleEventDescriptor*)the_event replyEvent:(NSAppleEventDescriptor*)reply_event
{
	NSString* url_string = [[the_event paramDescriptorForKeyword:keyDirectObject] stringValue];
	// We could try to use NSURL to break up the url into components.
//    NSURL* ns_url = [NSURL URLWithString:url_string];
//	NSLog(@"handleOpenURL: %@", url_string);

	if([self isInitialized])
	{
		[self doOpenURLCallback:url_string];
	}
	else
	{
		// need to save array of files until after we finish initialization
		[self setDeferredOpenURL:url_string];
	}
	
}

// TODO: Support dropping data onto Dock Icon
// NSFilesPromisePboardType/kPasteboardTypeFileURLPromise
// https://stackoverflow.com/questions/33793386/dropping-promised-files-on-to-application-icon-in-dock

- (void) applicationWillFinishLaunching:(NSNotification*)a_notification
{
//	NSLog(@"applicationWillFinishLaunching:");
	
	// Register for custom URL scheme handling
	[[NSAppleEventManager sharedAppleEventManager]
		setEventHandler:self
		andSelector:@selector(handleOpenURL:replyEvent:)
		forEventClass:kInternetEventClass
		andEventID:kAEGetURL
	];
	
}

- (void) applicationDidFinishLaunching:(NSNotification*)a_notification
{
//	NSLog(@"applicationDidFinishLaunching:");
	// Invoke the IupEntry callback function to start the user code.
	iupLoopCallEntryCb();

	[self setInitialized:true];


	if(nil != [self deferredOpenFiles])
	{
		// Do callback
		NSArray* file_names = [self deferredOpenFiles];
		[self doOpenFilesCallback:file_names];
		[self setDeferredOpenFiles:nil];
	}
	
	if(nil != [self deferredOpenURL])
	{
		// Do callback
		[self doOpenURLCallback:[self deferredOpenURL]];
		[self setDeferredOpenURL:nil];
	}
}

// Drat: Due to the way IupExitLoop() can be called to quit a program,
// applicationWillTerminate: is not guaranteed to be invoked depending on the quit.
// It seems that this will be called when using Cmd-Q (Menu quit)
// But this will not be called when you use IUP_CLOSE or close the last window (IUP will automatically shutdown. We do not use applicationShouldTerminateAfterLastWindowClosed: because of that.)
// Similatly, quitting the other way will skip this.
- (void) applicationWillTerminate:(NSNotification*)a_notification
{
	// BUG: NSPasteboard copy may invoke pastebord:provideDataForType: on Quit/terminate: if there is lazy data in the clipboard.
	// But Apple seems to invoke this after this method returns.
	// But by that point, we have already shutdown IUP and freed a lot of objects, which may lead to a crash in pasteboard:provideDataForType:
	// Unfortunately, I don't know how to fix this.
	// We have two options:
	// 1) Clear out all pasteboard pointers when we tear down everything. But this will prevent the final copy of data being sent to the pasteboard before quit, so the data will not be pastable after this.
	// 2) Don't call IupClose() in the IupExit callback.
	// I'm starting to think in the loop redesign,
	// we should not call IupClose and maybe the platform takes responsbility for calling it.
	// In the Mac/Cocoa case, I don't think it is necessary to call IupClose at all which might solve this problem.

	// Invoke the IupExit callback function to start the user shutdown code.
	iupCocoaCommonLoopCallExitCb();
}

@end



#include "iup_config.h"
// We need to override the built-in IUP implementation and use the native Cocoa API.
// 1) Sandboxing requires we use the native API, otherwise we won't have permission to access files between launches.
// (We could try to use Security Scoped Bookmarks directly, but because they are not referenced counted, we have to be careful if the user still wants a reference.
// Also, Iup doesn't provide us a convenient API to work with that maps to SSB. It really would benefit from having an entire Ihandle* object, like IupClipboard does.)
//
// 2) Apple updates at least both the Main Menu Recent Files and also the Dock contextual menu. (Are there others?).
//
// 3) The standard .XIB Main Menu file has special hooks for Recent Files so NSDocumentController can modify it.
// If we muck with it so we can manually modify it, we break that. We must pick one or the other.
// In early versions, I went the manual modify route, but was not thinking about the Sandbox or Dock menu yet.
// I reverted to the standard XIB because it is the most correct thing to do.
//
// For our override, we pretty much ignore EVERYTHING IUP normally does.
// menu is useless to us.
// recent_cb won't work for us using the legacy signature because ours must be shared with OPENFILES_CB due to how the native API works.
// max_recent is useless to us because we can't programatically set the value. (Users set it in their System Preferences.)
// ih is useless to use because we don't write any of these values to IupConfig and all our interaction is done through NSDocuementController.
#if 0
// NOTE: This got moved to iup_config.c
void IupConfigRecentInit(Ihandle* ih, Ihandle* menu, Icallback recent_cb, int max_recent)
{
/*
  char* recent_name = IupGetAttribute(ih, "RECENTNAME");
  const char* group_name = recent_name;
  if (!group_name) group_name = "Recent";

  IupSetAttribute(ih, iConfigGetRecentAttribName(recent_name, "RECENTMENU"), (char*)menu);
  IupSetCallback(ih, iConfigGetRecentAttribName(recent_name, "RECENT_CB"), recent_cb);
  IupSetInt(ih, iConfigGetRecentAttribName(recent_name, "RECENTMAX"), max_recent);
*/
//	NSDocumentController* document_controller = [NSDocumentController sharedDocumentController];
	// maximumRecentDocumentCount is read-only
}

void IupConfigRecentUpdate(Ihandle* ih, const char* filename)
{
	if(NULL == filename)
	{
		return;
	}
	NSString* ns_string = [NSString stringWithUTF8String:filename];
	NSURL* file_url = [NSURL fileURLWithPath:ns_string];
	
	NSDocumentController* document_controller = [NSDocumentController sharedDocumentController];
	[document_controller noteNewRecentDocumentURL:file_url];
}
#endif
