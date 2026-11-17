/** \file
 * \brief Cocoa IupMessageDlg pre-defined dialog
 *
 * See Copyright Notice in "iup.h"
 */

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_dialog.h"

#include "iupcocoa_drv.h"


#ifndef GNUSTEP
@interface IupCocoaMessageDlgDelegate : NSObject <NSAlertDelegate>
@property (nonatomic, assign) Ihandle* ih;
@end

@implementation IupCocoaMessageDlgDelegate
- (BOOL)alertShowHelp:(NSAlert*)alert
{
  Icallback cb = IupGetCallback(self.ih, "HELP_CB");
  if (cb && cb(self.ih) == IUP_CLOSE)
  {
    NSWindow* window = [alert window];
    if ([window sheetParent])
      [[window sheetParent] endSheet:window returnCode:NSModalResponseAbort];
    else
      [NSApp stopModalWithCode:NSModalResponseAbort];
  }
  return YES;
}
@end
#endif

static char* cocoaMessageDlgGetAutoModalAttrib(Ihandle* ih)
{
  InativeHandle* parent = iupDialogGetNativeParent(ih);
  int automodal = 1;
  if (parent)
    automodal = 0;
  return iupStrReturnBoolean(automodal);
}

static int cocoaMessageDlgPopup(Ihandle* ih, int x, int y)
{
  (void)x; /* Positioning is not supported by NSAlert. */
  (void)y;

  InativeHandle* parent = iupDialogGetNativeParent(ih);
  char* title_str = iupAttribGet(ih, "TITLE");
  char* value_str = iupAttribGet(ih, "VALUE");
  char* icon_str = iupAttribGetStr(ih, "DIALOGTYPE");
  char* buttons_str = iupAttribGetStr(ih, "BUTTONS");
  int button_def = iupAttribGetInt(ih, "BUTTONDEFAULT");
  int custom_count = iupMessageDlgCustomCount(ih);
  int cancel_idx = -1;

#ifdef GNUSTEP
  if (custom_count > 3)
    return iupMessageDlgCustomPopup(ih, x, y);
#endif

  NSWindow* parentWindow = nil;
  Ihandle* parent_ih = IupGetAttributeHandle(ih, "PARENTDIALOG");

  if (parent)
  {
    if ([(id)parent isKindOfClass:[NSWindow class]])
      parentWindow = (NSWindow*)parent;
    else if ([(id)parent isKindOfClass:[NSView class]])
      parentWindow = [(NSView*)parent window];

    if (parentWindow && !parent_ih)
      parent_ih = (Ihandle*)objc_getAssociatedObject(parentWindow, IHANDLE_ASSOCIATED_OBJ_KEY);
  }
  else if (parent_ih)
  {
    parentWindow = iupcocoaDialogGetWindow(parent_ih);
  }

  if (!parent_ih)
  {
    NSWindow* keyWindow = [NSApp keyWindow];
    if (keyWindow)
    {
      Ihandle* active_ih = (Ihandle*)objc_getAssociatedObject(keyWindow, IHANDLE_ASSOCIATED_OBJ_KEY);
      if (iupObjectCheck(active_ih) && iupdrvDialogIsVisible(active_ih))
      {
        parent_ih = active_ih;
        parentWindow = keyWindow;
      }
    }
  }

  if (parent_ih)
  {
    parent_ih = IupGetDialog(parent_ih);
  }

  NSInteger response;
  NSMutableDictionary* responseMap = [NSMutableDictionary dictionary];

  NSAlert* alert = [[NSAlert alloc] init];

  [alert setMessageText:iupcocoaStrToNSString(title_str ? title_str : "")];
  [alert setInformativeText:iupcocoaStrToNSString(value_str ? value_str : "")];

  if (iupStrEqualNoCase(icon_str, "ERROR"))
    [alert setAlertStyle:NSAlertStyleCritical];
  else if (iupStrEqualNoCase(icon_str, "WARNING"))
    [alert setAlertStyle:NSAlertStyleWarning];
  else
    [alert setAlertStyle:NSAlertStyleInformational];

  if (button_def == 0)
    button_def = 1;

  NSMutableArray* titles = [NSMutableArray array];
  NSMutableArray* responses = [NSMutableArray array];

  if (custom_count > 0)
  {
    for (int i = 1; i <= custom_count; i++)
    {
      const char* button_title = iupAttribGetId(ih, "BUTTONTITLE", i);
      char* str = iupStrProcessMnemonic(button_title, NULL, 0);
      [titles addObject:iupcocoaStrToNSString(str)];
      [responses addObject:[NSString stringWithFormat:@"%d", i]];
      if (str != button_title)
        free(str);
    }
    button_def = iupMessageDlgCustomDefault(ih);
    cancel_idx = iupMessageDlgCustomCancel(ih) - 1;
  }
  else if (iupStrEqualNoCase(buttons_str, "OKCANCEL"))
  {
    [titles addObject:@"OK"];     [responses addObject:@"1"];
    [titles addObject:@"Cancel"]; [responses addObject:@"2"];
  }
  else if (iupStrEqualNoCase(buttons_str, "RETRYCANCEL"))
  {
    const char* retry_cstr = IupGetLanguageString("IUP_RETRY");
    NSString* retry_str = retry_cstr ? iupcocoaStrToNSString(retry_cstr) : @"Retry";
    [titles addObject:retry_str]; [responses addObject:@"1"];
    [titles addObject:@"Cancel"]; [responses addObject:@"2"];
  }
  else if (iupStrEqualNoCase(buttons_str, "YESNO"))
  {
    [titles addObject:@"Yes"]; [responses addObject:@"1"];
    [titles addObject:@"No"];  [responses addObject:@"2"];
  }
  else if (iupStrEqualNoCase(buttons_str, "YESNOCANCEL"))
  {
    [titles addObject:@"Yes"];    [responses addObject:@"1"];
    [titles addObject:@"No"];     [responses addObject:@"2"];
    [titles addObject:@"Cancel"]; [responses addObject:@"3"];
  }
  else
  {
    [titles addObject:@"OK"]; [responses addObject:@"1"];
  }

  for (int i = 0; i < [titles count]; i++)
  {
      [alert addButtonWithTitle:titles[i]];
      [responseMap setObject:responses[i] forKey:@(NSAlertFirstButtonReturn + i)];
  }

  /* the first button added gets the Return key equivalent */
  int default_idx = button_def - 1;
  NSButton* esc_button = nil;
  if (default_idx >= 0 && default_idx < [[alert buttons] count])
  {
      NSButton* firstButton = [[alert buttons] objectAtIndex:0];

      if (default_idx != 0)
      {
          [firstButton setKeyEquivalent:@""];
          NSButton* defaultButton = [[alert buttons] objectAtIndex:default_idx];
          if ([[defaultButton keyEquivalent] isEqualToString:@"\033"])
            esc_button = defaultButton;
          [defaultButton setKeyEquivalent:@"\r"];
      }
      else
      {
          [firstButton setKeyEquivalent:@"\r"];
      }
  }

  if (cancel_idx >= 0)
  {
    for (int i = 0; i < (int)[[alert buttons] count]; i++)
    {
      NSButton* button = [[alert buttons] objectAtIndex:i];
      if (i != default_idx && [[button keyEquivalent] isEqualToString:@"\033"])
        [button setKeyEquivalent:@""];
    }

#ifdef GNUSTEP
    if (cancel_idx != default_idx)
      [[[alert buttons] objectAtIndex:cancel_idx] setKeyEquivalent:@"\033"];
#else
    esc_button = [[alert buttons] objectAtIndex:cancel_idx];
#endif
  }

#ifndef GNUSTEP
  IupCocoaMessageDlgDelegate* delegate = nil;
  if (IupGetCallback(ih, "HELP_CB"))
  {
    delegate = [[IupCocoaMessageDlgDelegate alloc] init];
    delegate.ih = ih;
    [alert setDelegate:delegate];
    [alert setShowsHelp:YES];
  }

  id esc_monitor = nil;
  if (esc_button)
  {
    esc_monitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyDown handler:^NSEvent*(NSEvent* event) {
      if ([[event charactersIgnoringModifiers] isEqualToString:@"\033"])
      {
        [esc_button performClick:nil];
        return nil;
      }
      return event;
    }];
  }
#else
  (void)esc_button;
#endif

  if (parentWindow) {
      [alert beginSheetModalForWindow:parentWindow completionHandler:^(NSModalResponse returnCode) {
          [NSApp stopModalWithCode:returnCode];
      }];
      response = [NSApp runModalForWindow:[alert window]];
  } else {
      response = [alert runModal];
  }

#ifndef GNUSTEP
  if (esc_monitor)
    [NSEvent removeMonitor:esc_monitor];
#endif

  [[alert window] orderOut:nil];
  [alert release];
#ifndef GNUSTEP
  [delegate release];
#endif

  NSString* iup_response = [responseMap objectForKey:@(response)];
  if (iup_response)
  {
    iupAttribSet(ih, "BUTTONRESPONSE", [iup_response UTF8String]);
  }
  else
  {
    iupMessageDlgSetCancelResponse(ih);
  }

  if (parent_ih && iupObjectCheck(parent_ih))
  {
    IupSetFocus(parent_ih);
  }

  return IUP_NOERROR;
}

IUP_SDK_API void iupdrvMessageDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = cocoaMessageDlgPopup;

  iupClassRegisterAttribute(ic, "AUTOMODAL", cocoaMessageDlgGetAutoModalAttrib, NULL, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED|IUPAF_READONLY|IUPAF_NO_INHERIT);
}
