/** \file
 * \brief macOS Focus
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>

#include "iup.h"
#include "iup_object.h"
#include "iup_focus.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"

#include "iupcocoa_drv.h"


IUP_DRV_API void iupcocoaSetCanFocus(Ihandle* ih, int can)
{
  if (!ih)
    return;

  /* Controls check this in acceptsFirstResponder */
  if (can)
    iupAttribSet(ih, "_IUPCOCOA_CANFOCUS", "YES");
  else
    iupAttribSet(ih, "_IUPCOCOA_CANFOCUS", "NO");
}

/* the system decides whether a click or Tab focuses the control, IupSetFocus always does */
IUP_DRV_API int iupcocoaAcceptsFirstResponder(Ihandle* ih, int super_accepts)
{
  const char* canfocus;

  if (!ih)
    return super_accepts;

  canfocus = iupAttribGet(ih, "_IUPCOCOA_CANFOCUS");
  if (!canfocus)
    canfocus = iupAttribGet(ih, "CANFOCUS");
  if (canfocus && !iupStrBoolean(canfocus))
    return 0;

  if (iupAttribGet(ih, "_IUPCOCOA_FOCUSREQUEST"))
    return 1;

  return super_accepts;
}

IUP_SDK_API void iupdrvSetFocus(Ihandle *ih)
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
    view_to_focus = iupcocoaGetMainView(ih);
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

    iupAttribSet(ih, "_IUPCOCOA_FOCUSREQUEST", "1");

    BOOL accepts = [view_to_focus acceptsFirstResponder];
    if (accepts)
    {
      BOOL result = [target_window makeFirstResponder:view_to_focus];
      if (result)
      {
        iupcocoaFocusIn(ih);
      }
    }

    iupAttribSet(ih, "_IUPCOCOA_FOCUSREQUEST", NULL);
  }
}

IUP_DRV_API void iupcocoaFocusIn(Ihandle* ih)
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
  if (!dialog)
    return;

  if (ih != dialog)
  {
    iupAttribSet(dialog, "_IUPCOCOA_LASTFOCUS", (char*)ih);
  }
  else
  {
    Ihandle* last_focus = (Ihandle*)iupAttribGet(dialog, "_IUPCOCOA_LASTFOCUS");

    if (iupObjectCheck(last_focus))
    {
      if (IupGetFocus() == last_focus)
        return;

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

IUP_DRV_API void iupcocoaFocusOut(Ihandle* ih)
{
  if (!iupObjectCheck(ih))
  {
    return;
  }

  iupCallKillFocusCb(ih);
}
