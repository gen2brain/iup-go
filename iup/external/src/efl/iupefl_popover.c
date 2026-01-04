/** \file
 * \brief Popover Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_childtree.h"
#include "iup_class.h"
#include "iup_register.h"
#include "iup_drvinfo.h"
#include "iup_popover.h"

#include "iupefl_drv.h"


static void eflPopoverFocusOutCb(void* data, const Efl_Event* ev);
static void eflPopoverAnchorDelCb(void* data, const Efl_Event* ev);

static int eflPopoverMapMethod(Ihandle* ih)
{
  Ihandle* anchor;
  Eo* anchor_widget;
  Eo* parent_win;
  Eo* popup_win;
  Eo* frame;

  anchor = (Ihandle*)iupAttribGet(ih, "_IUP_POPOVER_ANCHOR");
  if (!anchor || !anchor->handle)
    return IUP_ERROR;

  anchor_widget = iupeflGetWidget(anchor);
  if (!anchor_widget)
    return IUP_ERROR;

  parent_win = efl_provider_find(anchor_widget, EFL_UI_WIN_CLASS);
  if (!parent_win)
    parent_win = iupeflGetMainWindow();
  if (!parent_win)
    return IUP_ERROR;

  /* Create a borderless popup window */
  popup_win = efl_add(EFL_UI_WIN_CLASS, parent_win, efl_ui_win_type_set(efl_added, EFL_UI_WIN_TYPE_BASIC));
  if (!popup_win)
    return IUP_ERROR;

  ih->handle = (InativeHandle*)popup_win;

  /* Make it borderless and act as popup */
  efl_ui_win_borderless_set(popup_win, EINA_TRUE);

  /* Create a frame inside the window for visual border */
  frame = efl_add(EFL_UI_FRAME_CLASS, popup_win);
  if (frame)
  {
    /* Hide frame title, just use for border effect */
    efl_text_set(frame, "");
    efl_gfx_hint_weight_set(frame, EFL_GFX_HINT_EXPAND, EFL_GFX_HINT_EXPAND);
    efl_gfx_hint_fill_set(frame, EINA_TRUE, EINA_TRUE);
    efl_gfx_entity_visible_set(frame, EINA_TRUE);
    iupAttribSet(ih, "_IUP_EFL_FRAME", (char*)frame);
    iupAttribSet(ih, "_IUP_EFL_INNER", (char*)frame);
  }
  else
  {
    /* Fallback to window directly if frame creation fails */
    iupAttribSet(ih, "_IUP_EFL_INNER", (char*)popup_win);
  }

  /* Connect focus-out for autohide */
  if (iupAttribGetBoolean(ih, "AUTOHIDE"))
  {
    efl_event_callback_add(popup_win, EFL_EVENT_FOCUS_OUT, eflPopoverFocusOutCb, ih);
  }

  /* Track anchor deletion */
  efl_event_callback_add(anchor_widget, EFL_EVENT_DEL, eflPopoverAnchorDelCb, ih);

  return IUP_NOERROR;
}

static Eina_Bool eflPopoverIsCursorOverAnchor(Ihandle* ih)
{
  Ihandle* anchor;
  Eo* anchor_widget;
  Eo* anchor_win;
  Eina_Rect anchor_geom;
  int cursor_x, cursor_y;
  int win_x = 0, win_y = 0;

  anchor = (Ihandle*)iupAttribGet(ih, "_IUP_POPOVER_ANCHOR");
  if (!anchor || !anchor->handle)
    return EINA_FALSE;

  anchor_widget = iupeflGetWidget(anchor);
  if (!anchor_widget)
    return EINA_FALSE;

  iupdrvGetCursorPos(&cursor_x, &cursor_y);

  anchor_geom = efl_gfx_entity_geometry_get(anchor_widget);

  anchor_win = efl_provider_find(anchor_widget, EFL_UI_WIN_CLASS);
  if (anchor_win)
    elm_win_screen_position_get(anchor_win, &win_x, &win_y);

  anchor_geom.x += win_x;
  anchor_geom.y += win_y;

  if (cursor_x >= anchor_geom.x && cursor_x < anchor_geom.x + anchor_geom.w &&
      cursor_y >= anchor_geom.y && cursor_y < anchor_geom.y + anchor_geom.h)
    return EINA_TRUE;

  return EINA_FALSE;
}

static void eflPopoverFocusOutCb(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  IFni show_cb;

  (void)ev;

  if (!iupAttribGetBoolean(ih, "AUTOHIDE"))
    return;

  if (eflPopoverIsCursorOverAnchor(ih))
    return;

  efl_gfx_entity_visible_set(iupeflGetWidget(ih), EINA_FALSE);

  show_cb = (IFni)IupGetCallback(ih, "SHOW_CB");
  if (show_cb)
    show_cb(ih, IUP_HIDE);
}

static void eflPopoverAnchorDelCb(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;

  (void)ev;

  if (ih->handle)
  {
    iupeflDelete(iupeflGetWidget(ih));
    ih->handle = NULL;
  }
}

static void eflPopoverUnMapMethod(Ihandle* ih)
{
  Eo* popup_win = iupeflGetWidget(ih);
  Eo* frame = (Eo*)iupAttribGet(ih, "_IUP_EFL_FRAME");
  Ihandle* anchor = (Ihandle*)iupAttribGet(ih, "_IUP_POPOVER_ANCHOR");

  if (anchor && anchor->handle)
  {
    efl_event_callback_del(iupeflGetWidget(anchor), EFL_EVENT_DEL,
                           eflPopoverAnchorDelCb, ih);
  }

  if (popup_win)
  {
    efl_event_callback_del(popup_win, EFL_EVENT_FOCUS_OUT, eflPopoverFocusOutCb, ih);

    if (frame)
      efl_del(frame);

    iupeflDelete(popup_win);
  }

  iupAttribSet(ih, "_IUP_EFL_FRAME", NULL);
  iupAttribSet(ih, "_IUP_EFL_INNER", NULL);
  ih->handle = NULL;
}

static void eflPopoverLayoutUpdateMethod(Ihandle* ih)
{
  Eo* popup_win = iupeflGetWidget(ih);
  Eo* frame = (Eo*)iupAttribGet(ih, "_IUP_EFL_FRAME");

  if (!popup_win)
    return;

  /* Resize frame to fill window */
  if (frame)
    efl_gfx_entity_size_set(frame, EINA_SIZE2D(ih->currentwidth, ih->currentheight));

  if (ih->firstchild)
  {
    ih->iclass->SetChildrenPosition(ih, 0, 0);

    Ihandle* child;
    for (child = ih->firstchild; child; child = child->brother)
      iupLayoutUpdate(child);
  }
}

static void* eflPopoverGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
  (void)child;
  return iupAttribGet(ih, "_IUP_EFL_INNER");
}

static int eflPopoverSetVisibleAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
  {
    Eo* popup_win;
    Ihandle* anchor;
    Eo* anchor_widget;
    Eina_Rect anchor_geom;
    int x, y;
    const char* pos;

    anchor = (Ihandle*)iupAttribGet(ih, "_IUP_POPOVER_ANCHOR");
    if (!anchor || !anchor->handle)
      return 0;

    if (!ih->handle)
    {
      if (IupMap(ih) == IUP_ERROR)
        return 0;
    }

    popup_win = iupeflGetWidget(ih);
    anchor_widget = iupeflGetWidget(anchor);

    /* Compute layout */
    if (ih->firstchild)
      iupLayoutCompute(ih);

    /* Get anchor position in screen coordinates */
    {
      Eo* anchor_win;
      int win_x = 0, win_y = 0;

      anchor_geom = efl_gfx_entity_geometry_get(anchor_widget);

      /* Get anchor's parent window screen position */
      anchor_win = efl_provider_find(anchor_widget, EFL_UI_WIN_CLASS);
      if (anchor_win)
        elm_win_screen_position_get(anchor_win, &win_x, &win_y);

      /* Convert to screen coordinates */
      anchor_geom.x += win_x;
      anchor_geom.y += win_y;
    }

    pos = iupAttribGetStr(ih, "POSITION");

    if (iupStrEqualNoCase(pos, "TOP"))
    {
      x = anchor_geom.x;
      y = anchor_geom.y - ih->currentheight;
    }
    else if (iupStrEqualNoCase(pos, "LEFT"))
    {
      x = anchor_geom.x - ih->currentwidth;
      y = anchor_geom.y;
    }
    else if (iupStrEqualNoCase(pos, "RIGHT"))
    {
      x = anchor_geom.x + anchor_geom.w;
      y = anchor_geom.y;
    }
    else
    {
      x = anchor_geom.x;
      y = anchor_geom.y + anchor_geom.h;
    }

    /* Set window size and position */
    efl_gfx_entity_size_set(popup_win, EINA_SIZE2D(ih->currentwidth, ih->currentheight));
    efl_gfx_entity_position_set(popup_win, EINA_POSITION2D(x, y));

    /* Position children, they use window-relative coordinates (0,0 based) */
    if (ih->firstchild)
      eflPopoverLayoutUpdateMethod(ih);

    efl_gfx_entity_visible_set(popup_win, EINA_TRUE);

    /* Activate window for focus handling */
    efl_ui_win_activate(popup_win);

    {
      IFni show_cb = (IFni)IupGetCallback(ih, "SHOW_CB");
      if (show_cb)
        show_cb(ih, IUP_SHOW);
    }
  }
  else
  {
    if (ih->handle)
    {
      efl_gfx_entity_visible_set(iupeflGetWidget(ih), EINA_FALSE);

      {
        IFni show_cb = (IFni)IupGetCallback(ih, "SHOW_CB");
        if (show_cb)
          show_cb(ih, IUP_HIDE);
      }
    }
  }

  return 0;
}

static char* eflPopoverGetVisibleAttrib(Ihandle* ih)
{
  Eo* popup_win = iupeflGetWidget(ih);

  if (!popup_win)
    return "NO";

  if (iupeflIsVisible(popup_win))
    return "YES";

  return "NO";
}

void iupdrvPopoverInitClass(Iclass* ic)
{
  ic->Map = eflPopoverMapMethod;
  ic->UnMap = eflPopoverUnMapMethod;
  ic->LayoutUpdate = eflPopoverLayoutUpdateMethod;
  ic->GetInnerNativeContainerHandle = eflPopoverGetInnerNativeContainerHandleMethod;

  iupClassRegisterAttribute(ic, "VISIBLE", eflPopoverGetVisibleAttrib, eflPopoverSetVisibleAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
}
