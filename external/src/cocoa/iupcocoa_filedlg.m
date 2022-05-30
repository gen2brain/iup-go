/** \file
 * \brief IupFileDlg pre-defined dialog
 *
 * See Copyright Notice in "iup.h"
 */

#include <Cocoa/Cocoa.h>

#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drvinfo.h"
#include "iup_dialog.h"
#include "iup_strmessage.h"
#include "iup_array.h"
#include "iup_drvinfo.h"

#include "iupcocoa_drv.h"

#define MAX_FILENAME_SIZE PATH_MAX
#define IUP_PREVIEWCANVAS 3000

enum {IUP_DIALOGOPEN, IUP_DIALOGSAVE, IUP_DIALOGDIR};
                           
/*
static void macFileDlgGetFolder(Ihandle *ih)
{
  InativeHandle* parent = iupDialogGetNativeParent(ih);
//  BROWSEINFO browseinfo;
  char buffer[PATH_MAX];

  NSOpenPanel *op = [NSOpenPanel openPanel];
  [op setCanChooseFiles:NO];
  [op setCanChooseDirectories:YES];
  [op setPrompt:@"Choose folder"];
  [op setTitle: [NSString stringWithUTF8String:iupAttribGet(ih, "TITLE")]];

  if([op runModal] == NSOKButton) 
  {           
    strcpy(buffer,[[op filename] UTF8String]);                       
    iupAttribStoreStr(ih, "VALUE", buffer);
    iupAttribSetStr(ih, "STATUS", "0");	
  }  
  else 
  {
    iupAttribSetStr(ih, "VALUE", NULL);
    iupAttribSetStr(ih, "STATUS", "-1");	
  }  

  iupAttribSetStr(ih, "FILEEXIST", NULL);
  iupAttribSetStr(ih, "FILTERUSED", NULL);
}
*/
/*
- (BOOL)panel:(id)sender shouldShowFilename:(NSString*)filename
{    
	return YES;
}
 */


// FIXME: NOOVERWRITEPROMPT This is going to require a delegate
// FIXME: NOCHANGEDIR I don't know to support this. Apple manages this themselves. Maybe panel:didChangeToDirectoryURL:
// TODO: FILTERUSED FILTERINFO

@interface IupOpenFilePanelDelegate : NSObject <NSOpenSavePanelDelegate>

@property(nonatomic, assign) Ihandle* ihandle;

- (BOOL) panel:(id)the_sender shouldEnableURL:(NSURL*)file_url;

@end

@implementation IupOpenFilePanelDelegate

- (void) dealloc
{
	[self setIhandle:nil];
	[super dealloc];
}

// This allows us to filter acceptable files.
// Apple BUG: I'm seeing something that looks like a race condition sometimes where the panel first opens,
// but this delegate callback does not get invoked.
// The early entries in the panel do not get filtered as a result.
// With both NSLog and breakpoints, I see the early entries do not go through this callback.
// Seen in 10.13.6. I've only noticed under sandboxing so far.
- (BOOL) panel:(id)the_sender shouldEnableURL:(NSURL*)file_url
{
	Ihandle* ih = [self ihandle];
	
	
	
	char* value = iupAttribGet(ih, "FILTER");
    if(NULL == value)
    {
		NSLog(@"Matched NULL == value");
		return YES;
	}
	
	
	// Check if the URL points to a directory.
	// We want to say yes to directories otherwise the user can't click on them to change directories.
	NSNumber* is_directory = nil;
	BOOL success = [file_url getResourceValue:&is_directory forKey:NSURLIsDirectoryKey error:nil];
	if(success && [is_directory boolValue])
	{
		
		// Make sure the directory is not a bundle.
		success = [file_url getResourceValue:&is_directory forKey:NSURLIsPackageKey error:nil];
		if(success && ![is_directory boolValue])
		{
			return YES;
		}
	}

	NSString* file_path = [file_url path];
	// We want to filter only against the file name, and not the path.
	NSString* file_name = [file_path lastPathComponent];
	
	NSString* semicolon_separated_string = [NSString stringWithUTF8String:value];
	// The Windows version doesn't worry about extra white space, so I won't either.
	NSArray* array_of_filters = [semicolon_separated_string componentsSeparatedByString:@";"];


#if 1 // Use NSPredicate
	// Usually these things are case-insensitive, especially on Mac with its default-case-insensitive file-system.
	file_name = [file_name lowercaseString];

	for(NSString* match_pattern in array_of_filters)
	{
		match_pattern = [match_pattern lowercaseString];

		NSPredicate* ns_predicate = [NSPredicate predicateWithFormat:@"self LIKE %@", match_pattern];
		BOOL is_match = [ns_predicate evaluateWithObject:file_name];
		if(YES == is_match)
		{
			// NSLog(@"Matched file: %@ to pattern: %@", file_path, match_pattern);
			return YES;
		}
		else
		{
			//NSLog(@"NoMatch file: %@ to pattern: %@", file_path, match_pattern);
		}
	}
	
#else
	// If the FILTER patterns are supposed to be regex, NSRegularExpression might map a little better than NSPredicate
	// Oops: I don't think this interpretation is right because . (dot) is supposed to be explicit and not a any-character symbol.
	// So I think NSPredicate is the correct solution.

	NSRange search_range = NSMakeRange(0, [file_name length]);
	NSError* ns_error = nil;


	for(NSString* match_pattern in array_of_filters)
	{
		NSRegularExpression* regex = [NSRegularExpression regularExpressionWithPattern:match_pattern options:NSRegularExpressionCaseInsensitive error:&ns_error];
		NSArray<NSTextCheckingResult*>* matches = [regex matchesInString:file_name options:0 range:search_range];
		for(NSTextCheckingResult* match_result in matches)
		{
			NSRange match_range = [match_result range];
			if(NSNotFound != match_range.location)
			{
				NSString* matched_text = [file_name substringWithRange:match_range];
//				NSLog(@"matched_text: %@", matched_text);
				return YES;
			}
		}

	}

#endif


	// NSLog(@"NoMatch for file: %@", file_path);

	return NO;
}

@end




static int cocoaFileDlgPopup(Ihandle *ih, int x, int y)
{

//  InativeHandle* parent = iupDialogGetNativeParent(ih);
  char* value;
  int dialogtype;
	NSInteger ret_val;
	
	// NSSavePanel is the base class for both save and open
	NSSavePanel* file_panel = nil;
//	NSSavePanel* save_panel = nil;
//	NSOpenPanel* open_panel = nil;

	NSMutableArray* extention_array = nil;


	NSObject* panel_delegate = nil;
	

  iupAttribSetInt(ih, "_IUPDLG_X", x);   /* used in iupDialogUpdatePosition */
  iupAttribSetInt(ih, "_IUPDLG_Y", y);

	// TODO: What does "DIR" mean in Cocoa???
	// ???  [open_panel setCanChooseDirectories:YES];

  value = iupAttribGetStr(ih, "DIALOGTYPE");
  if (iupStrEqualNoCase(value, "SAVE"))
  {
    dialogtype = IUP_DIALOGSAVE;
  }
  else if (iupStrEqualNoCase(value, "DIR"))
  {
	dialogtype = IUP_DIALOGDIR;
	NSOpenPanel* open_panel = [NSOpenPanel openPanel];
	  [open_panel setCanChooseDirectories:YES];
	  // ???
	  [open_panel setCanChooseFiles:NO];

	  // Symlinks automatically get resolved by Cocoa which can be nice, but this differs from the other implementions. So try to shut if off.
	  [open_panel setResolvesAliases:NO];
	  file_panel = open_panel;

		IupOpenFilePanelDelegate* iup_panel_delegate = [[IupOpenFilePanelDelegate alloc] init];
		[iup_panel_delegate setIhandle:ih];
		[open_panel setDelegate:iup_panel_delegate];
		panel_delegate = iup_panel_delegate;
  }
  else
  {
    dialogtype = IUP_DIALOGOPEN;
	  NSOpenPanel* open_panel = [NSOpenPanel openPanel];
	  [open_panel setCanChooseDirectories:NO];
	  [open_panel setCanChooseFiles:YES];
	  file_panel = open_panel;
		IupOpenFilePanelDelegate* iup_panel_delegate = [[IupOpenFilePanelDelegate alloc] init];
		[iup_panel_delegate setIhandle:ih];
		[open_panel setDelegate:iup_panel_delegate];
		panel_delegate = iup_panel_delegate;
  }
	
	
  value = iupAttribGet(ih, "EXTFILTER");
  if (value)
  {
	NSArray *arr = [[NSString stringWithUTF8String:value] componentsSeparatedByCharactersInSet:
		[NSCharacterSet characterSetWithCharactersInString:@"|;"]];  
	extention_array = [NSMutableArray arrayWithCapacity:[arr count]];
	for(NSString *str in arr)
	{
	  if([str hasPrefix:@"*."]){
		[extention_array addObject:[str substringFromIndex:2]];
	  }
    }
  }
  else 
  {
    value = iupAttribGet(ih, "FILTER");
    if (value)
    {
		NSArray *arr = [[NSString stringWithUTF8String:value] componentsSeparatedByCharactersInSet:
			[NSCharacterSet characterSetWithCharactersInString:@"|;"]];  
		extention_array = [NSMutableArray arrayWithCapacity:[arr count]];
		for(NSString *str in arr)
		{
		  if([str hasPrefix:@"*."]){
			[extention_array addObject:[str substringFromIndex:2]];
		  }
	    }
    }
  }

//  openfilename.lpstrFile = (char*)malloc(MAX_FILENAME_SIZE+1);
  value = iupAttribGet(ih, "FILE");
  if(value && 0!=*value)
  {
	[file_panel setNameFieldStringValue:[NSString stringWithUTF8String:value]];
  }

	value = iupAttribGet(ih, "DIRECTORY");
	if(value && 0!=*value)
	{
		NSString* ns_string = [NSString stringWithUTF8String:value];
		NSURL* ns_url = [NSURL URLWithString:ns_string];
		[file_panel setDirectoryURL:ns_url];
	}

	value = iupAttribGet(ih, "TITLE");
	if(value && 0!=*value)
	{
		NSString* ns_string = [NSString stringWithUTF8String:value];
		[file_panel setTitle:ns_string];
	}

	

	if(iupAttribGetBoolean(ih, "SHOWHIDDEN"))
	{
		[file_panel setShowsHiddenFiles:YES];

	}

  value = iupAttribGet(ih, "ALLOWNEW");
  if(value)
  {
	  
	  // Should we prevent this for openPanel? No. Example: BlurrrGenProj uses an Open panel to choose a directory to create a project in. Need to be able for the user to create new subdirectories.
	  // TODO: FIXME: We're going to have to revise the official documentation/API because it says this has no-effect when DIR, but we need it.
	  int allow_new = iupAttribGetBoolean(ih, "ALLOWNEW");
	  [file_panel setCanCreateDirectories:allow_new];

	  
  }

  if (iupAttribGetBoolean(ih, "MULTIPLEFILES"))
  {
	  // only valid for NSOpenPanel
	  if(iupStrEqualNoCase(value, "SAVE"))
	  {
		  
	  }
	  else
	  {
		  [(NSOpenPanel*)file_panel setAllowsMultipleSelection:YES];
		  
	  }

  }
	
	
	// FIXME: Supporting PARENTDIALOG might not work.
	// Iup assumes that this method blocks and is completely modal.
	// Cocoa doesn't really like modal windows at all.
	// The modal sheets will only block for a specific window, but in theory, other windows should still be interactive, plus the menu.
	// Cocoa wants to return immediately from creating a sheet which violates Iup assupmtions.
	// I have a hack in place to block the executtion flow, but I think it may break down in complex window states.
	// We may need to disable this feature.
	value = iupAttribGet(ih, "PARENTDIALOG");
	if(value)
	{
		InativeHandle* parent = iupDialogGetNativeParent(ih);
//		InativeHandle* parent = IupGetParent(ih);

		NSWindow* parent_window = nil;
		if([(id)parent isKindOfClass:[NSWindow class]])
		{
			parent_window = parent;
			
		}
		else if([(id)parent isKindOfClass:[NSView class]])
		{
			parent_window = [(id)parent window];
		}
		else
		{
		}
		__block NSInteger up_val_result = -1;
		__block BOOL up_val_did_complete = NO;

		[file_panel beginSheetModalForWindow:parent_window
			completionHandler:^(NSInteger the_result)
			{
				up_val_result = the_result;
				up_val_did_complete = YES;
			}
		 ];
		
		
		// HACK: Iup wants this method to block, but this method returns immediately (the completion handler blocks).
		// This spin-blocking might work for the basic case, but I'm worried it's going to be buggy in complex cases.
		// For example, what if other window buttons are still exposed; the user can interact with those.
		// Or the user can hit Quit in the menu.
		while(NO == up_val_did_complete)
		{
#if 0
			NSEvent* ns_event;
			ns_event = [NSApp
					 nextEventMatchingMask:NSAnyEventMask
					 untilDate:[NSDate dateWithTimeIntervalSinceNow:0.0]
					 inMode:NSDefaultRunLoopMode
					 dequeue:YES
			];
			[NSApp sendEvent:ns_event];
#else
			
			IupLoopStep();
#endif
		}
		

		
		
		 ret_val = up_val_result;
		
	}
	else
	{
		ret_val = [file_panel runModal];
	}
	
	if(ret_val == NSModalResponseOK)
	{
		
		// Slightly different things for save vs open, so let's split them up
		if(iupStrEqualNoCase(value, "SAVE"))
		{
			NSURL* ns_url = [file_panel URL];

			// For STATUS, we must return 1 for a new file, 0 for an existing file.
			// A delegate callback might allow us to handle this more directly.
			if([[NSFileManager defaultManager] fileExistsAtPath:[ns_url path]])
			{
				iupAttribSetInt(ih, "STATUS", 0);
				// TODO: maybe not set DIALOGTYPE=DIR or MULTIPLEFILES=YES
				iupAttribSetInt(ih, "FILEEXIST", 1);

				
			}
			else
			{
				iupAttribSetInt(ih, "STATUS", 1);
				// TODO: maybe not set DIALOGTYPE=DIR or MULTIPLEFILES=YES
				iupAttribSetInt(ih, "FILEEXIST", 0);
			}
			
			iupAttribSetStr(ih, "VALUE", [[[file_panel URL] path] UTF8String]);
			
		}
		else
		{
			if(iupAttribGetBoolean(ih, "MULTIPLEFILES") && !iupStrEqualNoCase(value, "SAVE"))
			{
				NSArray* array_of_urls = [(NSOpenPanel*)file_panel URLs];
				
				 
				NSMutableArray* array_of_strings = [NSMutableArray arrayWithCapacity:[array_of_urls count]];
				
				// TODO: implement MULTIVALUEid
				for(NSURL* a_url in array_of_urls)
				{
					[array_of_strings addObject:[a_url path]];
				}
				NSString* joined_path = [array_of_strings componentsJoinedByString:@"|"];
				joined_path = [joined_path stringByAppendingString:@"|"];

				// Should this be fileSystemRepresentation? Not sure it will work with the | separators.
				iupAttribSetStr(ih, "VALUE", [joined_path UTF8String]);
				iupAttribSetInt(ih, "MULTIVALUECOUNT", (int)[array_of_urls count]);
				
			}
			else
			{
				// Not using fileSystemRepresentation to be consistent with above
				iupAttribSetStr(ih, "VALUE", [[[file_panel URL] path] UTF8String]);
				
			}
			
			
			// TODO: FILTERUSED
			iupAttribSetStr(ih, "FILTERUSED", NULL);

		}
	}
	else // user cancelled
	{
		iupAttribSetStr(ih, "VALUE", NULL);
		iupAttribSetInt(ih, "STATUS", -1);
	}


	[panel_delegate release];
	
  return IUP_NOERROR;

}

void iupdrvFileDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = cocoaFileDlgPopup;

  iupClassRegisterAttribute(ic, "EXTFILTER", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILTERINFO", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILTERUSED", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MULTIPLEFILES", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}
