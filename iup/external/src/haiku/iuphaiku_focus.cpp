/** \file
 * \brief Haiku Focus Management
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstddef>

#include <TextControl.h>
#include <View.h>
#include <Window.h>

extern "C" {
#include "iup.h"
#include "iup_drv.h"
#include "iup_object.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_focus.h"
}

#include "iuphaiku_drv.h"


IUP_DRV_API void iuphaikuSetCanFocus(BView* widget, int can)
{
  if (!widget) return;
  LooperLockGuard guard(widget->Looper());
  uint32 flags = widget->Flags();
  if (can) flags |= B_NAVIGABLE;
  else     flags &= ~B_NAVIGABLE;
  widget->SetFlags(flags);
}

/* Fired from widget MakeFocus overrides (Canvas, Toggle, ...). */
IUP_DRV_API void iuphaikuFocusInOutEvent(Ihandle* ih, int focus_in)
{
  if (!ih) return;
  if (focus_in)
    iupCallGetFocusCb(ih);
  else
    iupCallKillFocusCb(ih);
}

extern "C" IUP_SDK_API void iupdrvSetFocus(Ihandle* ih)
{
  if (!ih || !ih->handle) return;
  if (!ih->iclass) return;

  /* TYPEVOID children (Vbox/Hbox/Fill/Space) have ih->handle = (void*)-1;
   * TYPEMENU items have no focusable native handle. */
  if (ih->iclass->nativetype == IUP_TYPEVOID ||
      ih->iclass->nativetype == IUP_TYPEMENU)
    return;

  if (ih->iclass->nativetype == IUP_TYPEDIALOG)
  {
    BWindow* win = (BWindow*)ih->handle;
    LooperLockGuard guard(win);
    win->Activate(true);
    return;
  }

  BView* view = (BView*)ih->handle;

  /* Ensure the owning window is active before grabbing focus, otherwise the
   * MakeFocus call gets queued until activation - matches GTK's pattern. */
  BWindow* win = view->Window();
  if (win)
  {
    LooperLockGuard guard(win);
    if (!win->IsActive()) win->Activate(true);
    /* find the inner focusable view inside wrap containers */
    BTextControl* tc = dynamic_cast<BTextControl*>(view);
    BView* inner = NULL;
    if (tc) inner = tc->TextView();
    else
    {
      inner = (BView*)iupAttribGet(ih, "_IUPHAIKU_LIST_INNER");
      if (!inner) inner = (BView*)iupAttribGet(ih, "_IUPHAIKU_TEXT_INNER");
    }
    (inner ? inner : view)->MakeFocus(true);
  }
  else
  {
    /* Pre-attach: cache CANFOCUS state, applied at AttachedToWindow. */
    iupAttribSet(ih, "_IUPHAIKU_PENDING_FOCUS", "1");
  }
}
