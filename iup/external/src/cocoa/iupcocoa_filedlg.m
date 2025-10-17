/** \file
 * \brief IupFileDlg pre-defined dialog
 *
 * See Copyright Notice in "iup.h"
 */

#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>

#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_dialog.h"
#include "iup_key.h"

#include "iupcocoa_drv.h"


enum {IUP_DIALOGOPEN, IUP_DIALOGSAVE, IUP_DIALOGDIR};

/* Helper to get the next string in a list of null-separated strings. */
static char* iupCocoaFileDlgGetNextStr(char* str)
{
  int len = (int)strlen(str);
  return str + len + 1;
}

@interface IupPreviewCanvasView : NSView
@property(nonatomic, assign) Ihandle* ihandle;
@end


@implementation IupPreviewCanvasView

- (BOOL)isFlipped
{
  return YES;
}

- (void)drawRect:(NSRect)dirtyRect
{
  Ihandle* ih = [self ihandle];
  if (!ih) return;

  IFnss cb = (IFnss)IupGetCallback(ih, "FILE_CB");
  if (cb)
  {
    NSSavePanel* file_panel = (NSSavePanel*)[(NSView*)[self superview] window];
    NSString* path_str = [[file_panel URL] path];
    BOOL is_dir;

    if (path_str && [[NSFileManager defaultManager] fileExistsAtPath:path_str isDirectory:&is_dir] && !is_dir)
    {
      cb(ih, [path_str UTF8String], "PAINT");
    }
    else
    {
      cb(ih, NULL, "PAINT");
    }
  }
}

- (void)updateTrackingAreas
{
  [super updateTrackingAreas];
  NSTrackingArea* area = [[NSTrackingArea alloc] initWithRect:[self bounds]
                                                      options:(NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved | NSTrackingActiveInKeyWindow)
                                                        owner:self userInfo:nil];
  [self addTrackingArea:area];
  [area release];
}

- (void)mouseMoved:(NSEvent *)theEvent
{
  Ihandle* ih = [self ihandle];
  IFniis cb = (IFniis)IupGetCallback(ih, "MOTION_CB");
  if (cb)
  {
    NSPoint p = [self convertPoint:[theEvent locationInWindow] fromView:nil];
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    iupCocoaButtonKeySetStatus(theEvent, status);

    NSUInteger buttons = [NSEvent pressedMouseButtons];
    if (buttons & (1 << 0)) iupKEY_SETBUTTON1(status);
    if (buttons & (1 << 1)) iupKEY_SETBUTTON3(status);
    if (buttons & (1 << 2)) iupKEY_SETBUTTON2(status);

    cb(ih, (int)p.x, (int)p.y, status);
  }
}

- (void)mouseDown:(NSEvent *)theEvent
{
  Ihandle* ih = [self ihandle];
  IFniiiis cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
  if (cb)
  {
    NSPoint p = [self convertPoint:[theEvent locationInWindow] fromView:nil];
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    int doubleclick = ([theEvent clickCount] > 1) ? 1 : 0;
    int button = 0;

    switch ([theEvent buttonNumber])
    {
      case 0: button = IUP_BUTTON1; break;
      case 1: button = IUP_BUTTON3; break;
      case 2: button = IUP_BUTTON2; break;
      default: button = (int)[theEvent buttonNumber] + 1; break;
    }

    iupCocoaButtonKeySetStatus(theEvent, status);
    if(doubleclick) iupKEY_SETDOUBLE(status);

    switch(button)
    {
      case IUP_BUTTON1: iupKEY_SETBUTTON1(status); break;
      case IUP_BUTTON2: iupKEY_SETBUTTON2(status); break;
      case IUP_BUTTON3: iupKEY_SETBUTTON3(status); break;
      case IUP_BUTTON4: iupKEY_SETBUTTON4(status); break;
      case IUP_BUTTON5: iupKEY_SETBUTTON5(status); break;
    }

    int ret = cb(ih, button, 1, (int)p.x, (int)p.y, status);
    if (ret == IUP_CLOSE)
    {
      IupExitLoop();
    }
  }
}

- (void)mouseUp:(NSEvent *)theEvent
{
  Ihandle* ih = [self ihandle];
  IFniiiis cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
  if (cb)
  {
    NSPoint p = [self convertPoint:[theEvent locationInWindow] fromView:nil];
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    int button = 0;

    switch ([theEvent buttonNumber])
    {
      case 0: button = IUP_BUTTON1; break;
      case 1: button = IUP_BUTTON3; break;
      case 2: button = IUP_BUTTON2; break;
      default: button = (int)[theEvent buttonNumber] + 1; break;
    }

    iupCocoaButtonKeySetStatus(theEvent, status);

    switch(button)
    {
      case IUP_BUTTON1: iupKEY_SETBUTTON1(status); break;
      case IUP_BUTTON2: iupKEY_SETBUTTON2(status); break;
      case IUP_BUTTON3: iupKEY_SETBUTTON3(status); break;
      case IUP_BUTTON4: iupKEY_SETBUTTON4(status); break;
      case IUP_BUTTON5: iupKEY_SETBUTTON5(status); break;
    }

    int ret = cb(ih, button, 0, (int)p.x, (int)p.y, status);
    if (ret == IUP_CLOSE)
    {
      IupExitLoop();
    }
  }
}

- (void)scrollWheel:(NSEvent *)theEvent
{
  Ihandle* ih = [self ihandle];
  IFnfiis cb = (IFnfiis)IupGetCallback(ih, "WHEEL_CB");
  if (cb)
  {
    NSPoint p = [self convertPoint:[theEvent locationInWindow] fromView:nil];
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    iupCocoaButtonKeySetStatus(theEvent, status);

    NSUInteger buttons = [NSEvent pressedMouseButtons];
    if (buttons & (1 << 0)) iupKEY_SETBUTTON1(status);
    if (buttons & (1 << 1)) iupKEY_SETBUTTON3(status);
    if (buttons & (1 << 2)) iupKEY_SETBUTTON2(status);

    float delta = [theEvent scrollingDeltaY];
    cb(ih, delta, (int)p.x, (int)p.y, status);
  }
}

@end


@interface IupFilePanelDelegate : NSObject <NSOpenSavePanelDelegate>
@property(nonatomic, assign) Ihandle* ihandle;
@end


@implementation IupFilePanelDelegate

- (void)dealloc
{
  Ihandle* ih = [self ihandle];
  if (ih)
  {
    [self setIhandle:nil];
  }
  [super dealloc];
}

- (BOOL)panel:(id)sender shouldEnableURL:(NSURL*)file_url
{
  Ihandle* ih = [self ihandle];
  char* value = iupAttribGet(ih, "_COCOA_ACTIVE_FILTER");
  if (NULL == value)
  {
    return YES;
  }

  BOOL is_directory;
  if ([[NSFileManager defaultManager] fileExistsAtPath:[file_url path] isDirectory:&is_directory] && is_directory)
  {
    return YES;
  }

  NSString* file_name = [[file_url path] lastPathComponent];
  NSString* semicolon_separated_string = [NSString stringWithUTF8String:value];
  NSArray* array_of_filters = [semicolon_separated_string componentsSeparatedByString:@";"];

  file_name = [file_name lowercaseString];
  for (NSString* match_pattern in array_of_filters)
  {
    NSString* lower_pattern = [match_pattern lowercaseString];
    NSPredicate* ns_predicate = [NSPredicate predicateWithFormat:@"self LIKE %@", lower_pattern];
    if ([ns_predicate evaluateWithObject:file_name])
    {
      return YES;
    }
  }
  return NO;
}

- (void)panelSelectionDidChange:(id)sender
{
  Ihandle* ih = [self ihandle];
  IFnss cb = (IFnss)IupGetCallback(ih, "FILE_CB");
  if (cb)
  {
    NSURL* url = [(NSSavePanel*)sender URL];
    const char* path_utf8 = [[url path] UTF8String];
    BOOL is_dir;
    if ([[NSFileManager defaultManager] fileExistsAtPath:[url path] isDirectory:&is_dir] && !is_dir)
    {
      cb(ih, path_utf8, "SELECT");
    }
    else
    {
      cb(ih, path_utf8, "OTHER");
    }
  }

  if (iupAttribGetBoolean(ih, "SHOWPREVIEW"))
  {
    NSSavePanel* file_panel = (NSSavePanel*)sender;
    IupPreviewCanvasView* preview_view = (IupPreviewCanvasView*)[file_panel accessoryView];
    if (preview_view)
    {
      [preview_view setNeedsDisplay:YES];
    }
  }
}

- (BOOL)panel:(id)sender validateURL:(NSURL *)url error:(NSError **)outError
{
  Ihandle* ih = [self ihandle];
  IFnss cb = (IFnss)IupGetCallback(ih, "FILE_CB");
  if (cb)
  {
    const char* path_utf8 = [[url path] UTF8String];
    int ret = cb(ih, path_utf8, "OK");
    if (ret == IUP_IGNORE)
    {
      return NO;
    }
    else if (ret == IUP_CONTINUE)
    {
      char* value = iupAttribGet(ih, "FILE");
      if (value)
      {
        [(NSSavePanel*)sender setNameFieldStringValue:[NSString stringWithUTF8String:value]];
      }
      return NO;
    }
  }
  return YES;
}

- (void)panel:(id)sender userDidClickHelp:(id)help
{
  Ihandle* ih = [self ihandle];
  Icallback cb = (Icallback)IupGetCallback(ih, "HELP_CB");
  if (cb && cb(ih) == IUP_CLOSE)
  {
    [[NSApplication sharedApplication] stopModal];
  }
}

@end


static int cocoaFileDlgPopup(Ihandle *ih, int x, int y)
{
  (void)x;
  (void)y;

  char* value;
  int dialogtype;

  NSSavePanel* file_panel = nil;
  NSMutableArray* extention_array = nil;
  IupFilePanelDelegate* panel_delegate = nil;

  value = iupAttribGetStr(ih, "DIALOGTYPE");
  if (iupStrEqualNoCase(value, "SAVE"))
  {
    dialogtype = IUP_DIALOGSAVE;
    file_panel = [NSSavePanel savePanel];
  }
  else if (iupStrEqualNoCase(value, "DIR"))
  {
    dialogtype = IUP_DIALOGDIR;
    NSOpenPanel* open_panel = [NSOpenPanel openPanel];
    [open_panel setCanChooseDirectories:YES];
    [open_panel setCanChooseFiles:NO];
    [open_panel setResolvesAliases:NO];
    file_panel = open_panel;
  }
  else
  {
    dialogtype = IUP_DIALOGOPEN;
    NSOpenPanel* open_panel = [NSOpenPanel openPanel];
    [open_panel setCanChooseDirectories:NO];
    [open_panel setCanChooseFiles:YES];
    file_panel = open_panel;
  }

  IFnss file_cb = (IFnss)IupGetCallback(ih, "FILE_CB");
  Icallback help_cb = (Icallback)IupGetCallback(ih, "HELP_CB");

  if (file_cb || help_cb)
  {
    panel_delegate = [[IupFilePanelDelegate alloc] init];
    [panel_delegate setIhandle:ih];
    [file_panel setDelegate:panel_delegate];
  }

  if (file_cb)
  {
    file_cb(ih, NULL, "INIT");
  }

  if (help_cb)
  {
    [file_panel setShowsHelp:YES];
  }

  char* extfilter_val = iupAttribGet(ih, "EXTFILTER");
  char* filter_val = iupAttribGet(ih, "FILTER");
  iupAttribSet(ih, "_COCOA_ACTIVE_FILTER", NULL);
  if (extfilter_val)
  {
    char* filters_dup = iupStrDup(extfilter_val);
    int filter_count = iupStrReplace(filters_dup, '|', 0) / 2;
    int filter_index = iupAttribGetInt(ih, "FILTERUSED");
    if (filter_index == 0) filter_index = 1;

    if (filter_index > 0 && filter_index <= filter_count)
    {
      char* name = filters_dup;
      char* pattern = NULL;
      for (int i = 0; i < filter_index; i++)
      {
        pattern = iupCocoaFileDlgGetNextStr(name);
        if (i < filter_index - 1)
          name = iupCocoaFileDlgGetNextStr(pattern);
      }
      iupAttribSetStr(ih, "_COCOA_ACTIVE_FILTER", pattern);
    }
    free(filters_dup);
  }
  else if (filter_val)
  {
    iupAttribSetStr(ih, "_COCOA_ACTIVE_FILTER", filter_val);
  }

  char* active_filter = iupAttribGet(ih, "_COCOA_ACTIVE_FILTER");
  if (active_filter)
  {
    NSArray* arr = [[NSString stringWithUTF8String:active_filter] componentsSeparatedByString:@";"];
    extention_array = [NSMutableArray arrayWithCapacity:[arr count]];
    for (NSString* str in arr)
    {
      if ([str hasPrefix:@"*."])
      {
        [extention_array addObject:[str substringFromIndex:2]];
      }
    }
    if ([extention_array count] > 0)
    {
      [file_panel setAllowedFileTypes:extention_array];
    }
  }

  value = iupAttribGet(ih, "FILE");
  if (value && *value && strchr(value, '/'))
  {
    char* dir = iupStrFileGetPath(value);
    const char* filename = iupStrFileGetTitle(value);
    iupAttribSetStr(ih, "DIRECTORY", dir);
    iupAttribSetStr(ih, "FILE", filename);
    free(dir);
  }

  value = iupAttribGet(ih, "FILE");
  if (value && *value)
  {
    [file_panel setNameFieldStringValue:[NSString stringWithUTF8String:value]];
  }

  value = iupAttribGet(ih, "DIRECTORY");
  if (value && *value)
  {
    NSURL* ns_url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:value]];
    [file_panel setDirectoryURL:ns_url];
  }

  value = iupAttribGet(ih, "TITLE");
  if (value && *value)
  {
    [file_panel setTitle:[NSString stringWithUTF8String:value]];
  }

  if (iupAttribGetBoolean(ih, "SHOWHIDDEN"))
  {
    [file_panel setShowsHiddenFiles:YES];
  }

  if (iupAttribGet(ih, "ALLOWNEW"))
  {
    [file_panel setCanCreateDirectories:iupAttribGetBoolean(ih, "ALLOWNEW")];
  }

  if (iupAttribGetBoolean(ih, "SHOWPREVIEW") && file_cb)
  {
    int width = iupAttribGetInt(ih, "PREVIEWWIDTH");
    int height = iupAttribGetInt(ih, "PREVIEWHEIGHT");
    if (width <= 0) width = 200;
    if (height <= 0) height = 150;

    IupPreviewCanvasView* preview_view = [[IupPreviewCanvasView alloc] initWithFrame:NSMakeRect(0, 0, width, height)];
    [preview_view setIhandle:ih];
    [file_panel setAccessoryView:preview_view];
    iupAttribSetInt(ih, "PREVIEWWIDTH", width);
    iupAttribSetInt(ih, "PREVIEWHEIGHT", height);
    [preview_view release];
  }

  if (dialogtype != IUP_DIALOGSAVE && iupAttribGetBoolean(ih, "MULTIPLEFILES"))
  {
    [(NSOpenPanel*)file_panel setAllowsMultipleSelection:YES];
  }

  NSInteger response = [file_panel runModal];

  if (file_cb)
  {
    file_cb(ih, NULL, "FINISH");
  }

  iupAttribSet(ih, "FILTERUSED", NULL);

  if (response == NSModalResponseOK)
  {
    if (dialogtype == IUP_DIALOGSAVE)
    {
      NSURL* ns_url = [file_panel URL];
      NSString* path = [ns_url path];

      char* ext_default = iupAttribGet(ih, "EXTDEFAULT");
      if (ext_default && strlen(ext_default) > 0 && [[path pathExtension] length] == 0)
      {
        path = [path stringByAppendingPathExtension:[NSString stringWithUTF8String:ext_default]];
      }

      iupAttribSetStr(ih, "VALUE", [path UTF8String]);
      if ([[NSFileManager defaultManager] fileExistsAtPath:path])
      {
        iupAttribSet(ih, "STATUS", "0");
        iupAttribSet(ih, "FILEEXIST", "YES");
      }
      else
      {
        iupAttribSet(ih, "STATUS", "1");
        iupAttribSet(ih, "FILEEXIST", "NO");
      }
    }
    else
    {
      if (dialogtype != IUP_DIALOGDIR && iupAttribGetBoolean(ih, "MULTIPLEFILES"))
      {
        NSArray* array_of_urls = [(NSOpenPanel*)file_panel URLs];
        int count = (int)[array_of_urls count];

        if (count == 1)
        {
          NSURL* url = [array_of_urls firstObject];
          const char* filename_utf8 = [[url path] UTF8String];
          char* dir = iupStrFileGetPath(filename_utf8);

          iupAttribSetStr(ih, "VALUE", filename_utf8);
          iupAttribSetStr(ih, "DIRECTORY", dir);
          iupAttribSetStrId(ih, "MULTIVALUE", 0, dir);

          int dir_len = (int)strlen(dir);
          if (iupAttribGetBoolean(ih, "MULTIVALUEPATH"))
            dir_len = 0;

          iupAttribSetStrId(ih, "MULTIVALUE", 1, filename_utf8 + dir_len);
          iupAttribSetInt(ih, "MULTIVALUECOUNT", 2);
          free(dir);
        }
        else if (count > 1)
        {
          NSURL* first_url = [array_of_urls firstObject];
          char* dir_with_sep = iupStrFileGetPath([[first_url path] UTF8String]);

          iupAttribSetStr(ih, "DIRECTORY", dir_with_sep);
          iupAttribSetStrId(ih, "MULTIVALUE", 0, dir_with_sep);

          char* dir_no_sep = iupStrDup(dir_with_sep);
          int dir_len = (int)strlen(dir_no_sep);
          if (dir_len > 1 && dir_no_sep[dir_len - 1] == '/')
          {
            dir_no_sep[dir_len - 1] = '\0';
          }

          NSMutableString* value_str = [NSMutableString stringWithUTF8String:dir_no_sep];
          [value_str appendString:@"|"];
          free(dir_no_sep);

          BOOL use_full_path = iupAttribGetBoolean(ih, "MULTIVALUEPATH");
          int multivalue_idx = 1;

          for (NSURL* a_url in array_of_urls)
          {
            NSString* path_str = [a_url path];

            if (use_full_path)
            {
              /* Append the full path for both VALUE and MULTIVALUE. */
              [value_str appendString:path_str];
              [value_str appendString:@"|"];
              iupAttribSetStrId(ih, "MULTIVALUE", multivalue_idx, [path_str UTF8String]);
            }
            else
            {
              /* Append just the filename for both VALUE and MULTIVALUE. */
              NSString* filename_str = [path_str lastPathComponent];
              [value_str appendString:filename_str];
              [value_str appendString:@"|"];
              iupAttribSetStrId(ih, "MULTIVALUE", multivalue_idx, [filename_str UTF8String]);
            }
            multivalue_idx++;
          }

          iupAttribSetStr(ih, "VALUE", [value_str UTF8String]);
          iupAttribSetInt(ih, "MULTIVALUECOUNT", multivalue_idx);

          free(dir_with_sep);
        }
      }
      else
      {
        NSURL* url = [(NSOpenPanel*)file_panel URL];
        const char* path_utf8 = [[url path] UTF8String];
        iupAttribSetStr(ih, "VALUE", path_utf8);

        if (dialogtype == IUP_DIALOGDIR)
        {
          iupAttribSet(ih, "FILEEXIST", NULL);
          iupAttribSet(ih, "STATUS", "0");
        }
        else
        {
          if ([[NSFileManager defaultManager] fileExistsAtPath:[url path]])
          {
            iupAttribSet(ih, "FILEEXIST", "YES");
            iupAttribSet(ih, "STATUS", "0");
          }
          else
          {
            iupAttribSet(ih, "FILEEXIST", "NO");
            iupAttribSet(ih, "STATUS", "1");
          }
        }
      }
    }
  }
  else
  {
    iupAttribSet(ih, "VALUE", NULL);
    iupAttribSet(ih, "STATUS", "-1");
    iupAttribSet(ih, "FILEEXIST", NULL);
  }

  if (panel_delegate)
  {
    [panel_delegate release];
  }

  return IUP_NOERROR;
}

void iupdrvFileDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = cocoaFileDlgPopup;

  /* IupFileDialog common */
  iupClassRegisterAttribute(ic, "EXTFILTER", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILTERINFO", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILTERUSED", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  /* macOS and GTK only */
  iupClassRegisterAttribute(ic, "PREVIEWWIDTH", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PREVIEWHEIGHT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}
