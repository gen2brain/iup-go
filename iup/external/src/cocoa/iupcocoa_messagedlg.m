/** \file
 * \brief Cocoa IupMessageDlg pre-defined dialog
 *
 * See Copyright Notice in "iup.h"
 */

#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_dialog.h"
#include "iup_strmessage.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"

#include "iupcocoa_drv.h"


#ifndef NSAlertHelpReturn
#define NSAlertHelpReturn 1003
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
    parentWindow = cocoaDialogGetWindow(parent_ih);
  }

  /* If no parent found, find the current key window that is an IUP dialog. */
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
  BOOL help_pressed;
  NSMutableDictionary* responseMap = nil;

  do {
    help_pressed = NO;
    NSAlert* alert = [[NSAlert alloc] init];

    [alert setMessageText:[NSString stringWithUTF8String:title_str ? title_str : ""]];
    [alert setInformativeText:[NSString stringWithUTF8String:value_str ? value_str : ""]];

    if (iupStrEqualNoCase(icon_str, "ERROR"))
      [alert setAlertStyle:NSAlertStyleCritical];
    else if (iupStrEqualNoCase(icon_str, "WARNING"))
      [alert setAlertStyle:NSAlertStyleWarning];
    else /* INFORMATION, QUESTION */
      [alert setAlertStyle:NSAlertStyleInformational];

    if (button_def == 0)
      button_def = 1;

    NSMutableArray* titles = [NSMutableArray array];
    NSMutableArray* responses = [NSMutableArray array];

    if (iupStrEqualNoCase(buttons_str, "OKCANCEL"))
    {
      [titles addObject:@"OK"];     [responses addObject:@"1"];
      [titles addObject:@"Cancel"]; [responses addObject:@"2"];
    }
    else if (iupStrEqualNoCase(buttons_str, "RETRYCANCEL"))
    {
      const char* retry_cstr = IupGetLanguageString("IUP_RETRY");
      NSString* retry_str = retry_cstr ? [NSString stringWithUTF8String:retry_cstr] : @"Retry";
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

    if(responseMap) [responseMap release];
    responseMap = [[NSMutableDictionary dictionary] retain];

    /* Add buttons in their natural IUP order (1, 2, 3...) */
    for (int i = 0; i < [titles count]; i++)
    {
        [alert addButtonWithTitle:titles[i]];
        /* Map the sequentially assigned Cocoa response code to our IUP response value. */
        [responseMap setObject:responses[i] forKey:@(NSAlertFirstButtonReturn + i)];
    }

    /* By default, the first button added gets the Return key equivalent. */
    /* We need to manually move this designation to the button specified by BUTTONDEFAULT. */
    int default_idx = button_def - 1;
    if (default_idx >= 0 && default_idx < [[alert buttons] count])
    {
        NSButton* firstButton = [[alert buttons] objectAtIndex:0]; /* The button added first. */

        if (default_idx != 0)
        {
            /* If the default is not the first button, remove the default status from the first button */
            [firstButton setKeyEquivalent:@""];
            /* And assign it to the correct button. */
            NSButton* defaultButton = [[alert buttons] objectAtIndex:default_idx];
            [defaultButton setKeyEquivalent:@"\r"];
        }
        else
        {
            /* If the default IS the first button, ensure it has the Return key equivalent. This is normally the default behavior. */
            [firstButton setKeyEquivalent:@"\r"];
        }
    }

    if (IupGetCallback(ih, "HELP_CB"))
    {
      [alert setShowsHelp:YES];
    }

    if (parentWindow) {
        [alert beginSheetModalForWindow:parentWindow completionHandler:^(NSModalResponse returnCode) {
            [NSApp stopModalWithCode:returnCode];
        }];
        response = [NSApp runModalForWindow:[alert window]];
    } else {
        response = [alert runModal];
    }

    [alert release];

    if (response == NSAlertHelpReturn)
    {
      help_pressed = YES;
      Icallback cb = IupGetCallback(ih, "HELP_CB");
      if (cb && cb(ih) == IUP_CLOSE)
      {
        if (iupStrEqualNoCase(buttons_str, "YESNOCANCEL"))
          iupAttribSet(ih, "BUTTONRESPONSE", "3");
        else if (iupStrEqualNoCase(buttons_str, "OK"))
          iupAttribSet(ih, "BUTTONRESPONSE", "1");
        else
          iupAttribSet(ih, "BUTTONRESPONSE", "2");

        if(responseMap) [responseMap release];
        return IUP_NOERROR;
      }
    }

  } while (help_pressed);

  NSString* iup_response = [responseMap objectForKey:@(response)];
  if (iup_response)
  {
    iupAttribSet(ih, "BUTTONRESPONSE", [iup_response UTF8String]);
  }
  else
  {
    /* This case occurs if the dialog is dismissed unexpectedly (e.g., window close button). */
    if (iupStrEqualNoCase(buttons_str, "YESNOCANCEL"))
      iupAttribSet(ih, "BUTTONRESPONSE", "3");
    else if (iupStrEqualNoCase(buttons_str, "OK"))
      iupAttribSet(ih, "BUTTONRESPONSE", "1");
    else
      iupAttribSet(ih, "BUTTONRESPONSE", "2");
  }

  if (parent_ih && iupObjectCheck(parent_ih))
  {
    IupSetFocus(parent_ih);
  }

  if(responseMap) [responseMap release];

  return IUP_NOERROR;
}

void iupdrvMessageDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = cocoaMessageDlgPopup;

  iupClassRegisterAttribute(ic, "AUTOMODAL", cocoaMessageDlgGetAutoModalAttrib, NULL, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED|IUPAF_READONLY|IUPAF_NO_INHERIT);
}
