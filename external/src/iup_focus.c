/** \file
 * \brief Keyboard Focus navigation
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_focus.h"
#include "iup_class.h"
#include "iup_assert.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_childtree.h"


IUP_SDK_API Ihandle* iupFocusNextInteractive(Ihandle *ih)
{
  Ihandle *c;

  if (!ih)
    return NULL;

  for (c = ih->brother; c; c = c->brother)
  {
    if (c->iclass->is_interactive)
      return c;
  }

  return NULL;
}

IUP_SDK_API int iupFocusCanAccept(Ihandle *ih)
{
  if (ih->iclass->is_interactive &&  /* interactive */
      iupAttribGetBoolean(ih, "CANFOCUS") &&   /* can receive focus */
      ih->handle &&                  /* mapped  */
      IupGetInt(ih, "ACTIVE") &&     /* active  */
      IupGetInt(ih, "VISIBLE"))      /* visible */
    return 1;
  else
    return 0;
}

static int iFocusCheckActiveRadio(Ihandle *ih)
{
  if (IupClassMatch(ih, "toggle") && 
      IupGetInt(ih, "RADIO") &&
      !IupGetInt(ih, "VALUE"))
    return 0;
  else
    return 1;
}

static Ihandle* iFocusFindAtBrothers(Ihandle *start, int checkradio)
{
  Ihandle *c;
  Ihandle *nf;

  for (c = start; c; c = c->brother)
  {
    /* check itself */
    if (iupFocusCanAccept(c) && (!checkradio || iFocusCheckActiveRadio(c)))
      return c;

    /* then check its children */
    nf = iFocusFindAtBrothers(c->firstchild, checkradio);
    if (nf)
      return nf;
  }

  return NULL;
}

static Ihandle* iFocusFindNext(Ihandle *ih, int checkradio)
{
  Ihandle *nf, *p;

  if (!ih)
    return NULL;

  /* look down in the child tree */
  if (ih->firstchild)
  {
    nf = iFocusFindAtBrothers(ih->firstchild, checkradio);
    if (nf) return nf;
  }

  /* look in the same level */
  if (ih->brother)
  {
    nf = iFocusFindAtBrothers(ih->brother, checkradio);
    if (nf) return nf;
  }

  /* look up in the brothers of the parent level */
  if (ih->parent)
  {
    for (p = ih->parent; p; p = p->parent)
    {
      if (p->brother)
      {
        nf = iFocusFindAtBrothers(p->brother, checkradio);
        if (nf) return nf;
      }
    }
  }

  return NULL;
}

IUP_API Ihandle* IupNextField(Ihandle *ih)
{
  Ihandle *ih_next;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return NULL;

  ih_next = iFocusFindNext(ih, 1);
  if (!ih_next)
  {
    /* not found after the element, then start over from the beginning,
       at the dialog. */
    ih_next = iFocusFindNext(IupGetDialog(ih), 1);
    if (ih_next == ih)
      return NULL;
  }

  if (ih_next)
  {
    iupdrvSetFocus(ih_next);
    return ih_next;
  }

  return NULL;
}

void iupFocusNext(Ihandle *ih)
{
  Ihandle *ih_next = iFocusFindNext(ih, 0);
  if (!ih_next)
  {
    /* not found after the element, then start over from the beginning,
       at the dialog. */
    ih_next = iFocusFindNext(IupGetDialog(ih), 0);
    if (ih_next == ih)
      return;
  }
  if (ih_next)
    iupdrvSetFocus(ih_next);
}

static int iFocusFindPrevious(Ihandle *parent, Ihandle **previous, Ihandle *ih, int checkradio)
{
  Ihandle *c;

  if (!parent)
    return 0;

  for (c = parent->firstchild; c; c = c->brother)
  {
    if (c == ih)
    {
      /* if found child, returns the current previous.
         but if previous is NULL, then keep searching until the last element. */
      if (*previous)
        return 1;
    }
    else
    {
      /* save the possible previous */
      if (iupFocusCanAccept(c) && (!checkradio || iFocusCheckActiveRadio(c)))
        *previous = c;
    }

    /* then check its children */
    if (iFocusFindPrevious(c, previous, ih, checkradio))
      return 1;
  }

  return 0;
}

IUP_API Ihandle* IupPreviousField(Ihandle *ih)
{
  Ihandle *previous = NULL;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return NULL;

  /* search from the dialog down to the element */
  iFocusFindPrevious(IupGetDialog(ih), &previous, ih, 1);
  
  if (previous)
  {
    iupdrvSetFocus(previous);
    return previous;
  }

  return NULL;
}

void iupFocusPrevious(Ihandle *ih)
{
  Ihandle *previous = NULL;

  /* search from the dialog down to the element */
  iFocusFindPrevious(IupGetDialog(ih), &previous, ih, 0);
  
  if (previous)
    iupdrvSetFocus(previous);
}

/* local variables */
static Ihandle* iup_current_focus = NULL;
static Ihandle* iup_current_dialog_focus = NULL;

IUP_API Ihandle* IupGetFocus(void)
{
  return iup_current_focus;
}

void iupSetCurrentFocus(Ihandle *ih)
{
  iup_current_focus = ih;

  if (ih)
  {
    Ihandle* dialog = IupGetDialog(ih);
    Ihandle* current_dialog = iup_current_dialog_focus;

    if (current_dialog != dialog)
    {
      IFni cb;

      /* change it before calling the callbacks 
         because focus can be changed again from inside the callbacks. */
      iup_current_dialog_focus = dialog;

      if (iupObjectCheck(current_dialog)) /* can be NULL at start or can be destroyed */
      {
        cb = (IFni)IupGetCallback(current_dialog, "FOCUS_CB");
        if (cb) cb(current_dialog, 0);
      }

      cb = (IFni)IupGetCallback(iup_current_dialog_focus, "FOCUS_CB");
      if (cb) cb(iup_current_dialog_focus, 1);
    }
  }
}

void iupResetCurrentFocus(Ihandle *destroyed_ih)
{
  if (iup_current_focus == destroyed_ih)
    iup_current_focus = NULL;
  if (iup_current_dialog_focus == destroyed_ih)
    iup_current_dialog_focus = NULL;
}

IUP_API Ihandle *IupSetFocus(Ihandle *ih)
{
  Ihandle* old_focus = IupGetFocus();

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return old_focus;

  /* Current focus is NOT set here, 
     only in the iupCallGetFocusCb */

  if (iupFocusCanAccept(ih))  
    iupdrvSetFocus(ih);

  return old_focus;
}

IUP_SDK_API void iupCallGetFocusCb(Ihandle *ih)
{
  Icallback cb;

  if (ih == IupGetFocus())  /* avoid duplicate messages */
    return;

  cb = (Icallback)IupGetCallback(ih, "GETFOCUS_CB");
  if (cb) cb(ih);

  if (ih->iclass->nativetype == IUP_TYPECANVAS)
  {
    IFni cb2 = (IFni)IupGetCallback(ih, "FOCUS_CB");
    if (cb2) cb2(ih, 1);
  }

  iupSetCurrentFocus(ih);

  if (iupAttribGetBoolean(ih, "PROPAGATEFOCUS"))
  {
    Ihandle* parent = iupChildTreeGetNativeParent(ih);
    while (parent)
    {
      IFni focus_cb = (IFni)IupGetCallback(parent, "FOCUS_CB");
      if (focus_cb)
      {
        focus_cb(parent, 1);
        break;
      }

      parent = iupChildTreeGetNativeParent(parent);
    }
  }
}

IUP_SDK_API void iupCallKillFocusCb(Ihandle *ih)
{
  Icallback cb;

  if (ih != IupGetFocus())  /* avoid duplicate messages */
    return;

  cb = IupGetCallback(ih, "KILLFOCUS_CB");
  if (cb) cb(ih);

  if (iupObjectCheck(ih) && ih->iclass->nativetype == IUP_TYPECANVAS)
  {
    IFni cb2 = (IFni)IupGetCallback(ih, "FOCUS_CB");
    if (cb2) cb2(ih, 0);
  }

  if (iupObjectCheck(ih) && iupAttribGetBoolean(ih, "PROPAGATEFOCUS"))
  {
    Ihandle* parent = iupChildTreeGetNativeParent(ih);
    while (parent)
    {
      IFni focus_cb = (IFni)IupGetCallback(parent, "FOCUS_CB");
      if (focus_cb)
      {
        focus_cb(parent, 0);
        break;
      }

      parent = iupChildTreeGetNativeParent(parent);
    }
  }

  iupSetCurrentFocus(NULL);
}
