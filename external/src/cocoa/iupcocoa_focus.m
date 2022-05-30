/** \file
 * \brief MAC Focus
 *
 * See Copyright Notice in "iup.h"
 */

#include <Cocoa/Cocoa.h>

#include <stdio.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_focus.h"
#include "iup_attrib.h"
#include "iup_drv.h"
#include "iup_assert.h" 
#include "iup_drv.h" 

#include "iupcocoa_drv.h"


void iupdrvSetFocus(Ihandle *ih)
{               
//  [ih->handle makeFirstResponder: self];
	id the_object = ih->handle;
	if([the_object isKindOfClass:[NSWindow class]])
	{
		[(NSWindow*)the_object makeKeyAndOrderFront:nil];
	}
	else if([the_object isKindOfClass:[NSView class]])
	{
		[[(NSView*)the_object window] makeFirstResponder:the_object];
	}


	
}

// GTK uses this for the Canvas focus callbacks. 
/*
void iupcocoaFocusInOutEvent(Ihandle *ih)
{

  if (evt->in)
  {
    iupCallGetFocusCb(ih);
  }
  else
    iupCallKillFocusCb(ih);

  return FALSE; 
}
*/
