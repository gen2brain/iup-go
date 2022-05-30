/** \file
 * \brief MAC IupMessageDlg pre-defined dialog
 *
 * See Copyright Notice in "iup.h"
 */

#import <Cocoa/Cocoa.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_dialog.h"       
#include "iup_strmessage.h"

#include "iupcocoa_drv.h"


static int macMessageDlgPopup(Ihandle* ih, int x, int y)
{
#if 1
//  InativeHandle* parent = iupDialogGetNativeParent(ih);
  char *icon, *buttons;
  NSInteger result;
  NSAlert *alert = [[NSAlert new] autorelease];  
  NSAlertStyle nsStyle = NSInformationalAlertStyle; 
  (void)x;
  (void)y;       

  icon = iupAttribGetStr(ih, "DIALOGTYPE");
  if (iupStrEqualNoCase(icon, "ERROR"))
  {
    nsStyle = NSCriticalAlertStyle;
  }
  else if (iupStrEqualNoCase(icon, "WARNING"))
  {
    nsStyle = NSWarningAlertStyle;
  }
  else if (iupStrEqualNoCase(icon, "INFORMATION"))
  {
    nsStyle = NSInformationalAlertStyle;
  }
  else if (iupStrEqualNoCase(icon, "QUESTION"))
  {
    nsStyle = NSInformationalAlertStyle;
  }
  [alert setAlertStyle:nsStyle];
   
  buttons = iupAttribGetStr(ih, "BUTTONS");     
  if (iupStrEqualNoCase(buttons, "OKCANCEL")) {  
	  [alert addButtonWithTitle:NSLocalizedString(@"OK", @"OK")];
	[alert addButtonWithTitle:NSLocalizedString(@"Cancel", @"Cancel")];
	  
  } else if (iupStrEqualNoCase(buttons, "YESNO")) {
	  [alert addButtonWithTitle:NSLocalizedString(@"Yes", @"Yes")];
	[alert addButtonWithTitle:NSLocalizedString(@"No", @"No")];
  } else
  {
	  [alert addButtonWithTitle:NSLocalizedString(@"OK", @"OK")];
//    num_but = 1;
  }       
  [alert setInformativeText: [NSString stringWithUTF8String:iupAttribGet(ih, "VALUE")]];
  [alert setMessageText:     [NSString stringWithUTF8String:iupAttribGet(ih, "TITLE")]];

  result = [alert runModal];
  if (result == 0)
  {
    iupAttribSetStr(ih, "BUTTONRESPONSE", NULL);
    return IUP_ERROR;
  }

  if (result == NSAlertFirstButtonReturn)
    iupAttribSetStr(ih, "BUTTONRESPONSE", "1");
  else if (result == NSAlertSecondButtonReturn) 
    iupAttribSetStr(ih, "BUTTONRESPONSE", "2");
#endif
  return IUP_NOERROR;
}

void iupdrvMessageDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = macMessageDlgPopup;
}
