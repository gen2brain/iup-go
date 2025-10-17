/** \file
 * \brief macOS Focus
 *
 * See Copyright Notice in "iup.h"
 */

#import <Cocoa/Cocoa.h>

#include <stdio.h>

#include "iup.h"
#include "iup_object.h"
#include "iup_focus.h"
#include "iup_attrib.h"
#include "iup_drv.h"
#include "iup_assert.h"

#include "iupcocoa_drv.h"


void iupCocoaSetCanFocus(Ihandle* ih, int can)
{
  if (!ih)
    return;

  /* Controls check this in acceptsFirstResponder */
  if (can)
    iupAttribSet(ih, "_IUPCOCOA_CANFOCUS", "YES");
  else
    iupAttribSet(ih, "_IUPCOCOA_CANFOCUS", "NO");
}

void iupdrvSetFocus(Ihandle *ih)
{
  if (!ih || !ih->handle)
  {
    return;
  }

  id native_handle = ih->handle;
  NSWindow* target_window = nil;
  NSView* view_to_focus = nil;

  if ([native_handle isKindOfClass:[NSWindow class]])
  {
    target_window = (NSWindow*)native_handle;
    view_to_focus = [target_window contentView];
  }
  else
  {
    view_to_focus = iupCocoaGetMainView(ih);
    if (view_to_focus)
    {
      target_window = [view_to_focus window];
    }
  }

  if (target_window && view_to_focus)
  {
    BOOL is_key = [target_window isKeyWindow];
    if (!is_key)
    {
      [target_window makeKeyAndOrderFront:nil];
    }

    BOOL accepts = [view_to_focus acceptsFirstResponder];
    if (accepts)
    {
      BOOL result = [target_window makeFirstResponder:view_to_focus];
      if (result)
      {
        iupCocoaFocusIn(ih);
      }
    }
  }
}

void iupCocoaFocusIn(Ihandle* ih)
{
  if (IupGetFocus() == ih)
  {
    return;
  }

  if (!iupObjectCheck(ih) || !iupdrvIsActive(ih))
  {
    return;
  }

  Ihandle* dialog = IupGetDialog(ih);
  if (ih != dialog)
  {
    iupAttribSet(dialog, "_IUPCOCOA_LASTFOCUS", (char*)ih);
  }
  else
  {
    Ihandle* last_focus = (Ihandle*)iupAttribGet(ih, "_IUPCOCOA_LASTFOCUS");

    if (iupObjectCheck(last_focus))
    {
      iupCallGetFocusCb(ih);

      if (!iupAttribGetBoolean(ih, "IGNORELASTFOCUS"))
      {
        IupSetFocus(last_focus);
      }

      return;
    }
  }

  iupCallGetFocusCb(ih);
}

void iupCocoaFocusOut(Ihandle* ih)
{
  if (!iupObjectCheck(ih))
  {
    return;
  }

  iupCallKillFocusCb(ih);
}
