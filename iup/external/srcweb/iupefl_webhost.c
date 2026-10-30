/** \file
 * \brief Web Browser Control - EFL host of the native web engine
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdint.h>
#include <string.h>

#include <Ecore_X.h>

#include "iupefl_drv.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_classbase.h"
#include "iup_focus.h"
#include "iup_drv.h"

#include "iupunix_x11.h"
#include "iupweb_host.h"


#ifndef _X11_XLIB_H_
#define KeyPress   2
#define KeyRelease 3
#endif

typedef struct _IeflWebHostKey
{
  int press;
  unsigned int keycode;
  unsigned int state;
  unsigned long time;
} IeflWebHostKey;

static Ecore_Event_Handler* efl_webhost_key_handler = NULL;
static IeflWebHostKey efl_webhost_key = {0, 0, 0, 0};

static Eina_Bool eflWebHostXEvent(void* data, int type, void* event)
{
  XEvent* xevent = (XEvent*)event;
  (void)data;
  (void)type;

  if (xevent->type == KeyPress || xevent->type == KeyRelease)
  {
    efl_webhost_key.press = (xevent->type == KeyPress);
    efl_webhost_key.keycode = xevent->xkey.keycode;
    efl_webhost_key.state = xevent->xkey.state;
    efl_webhost_key.time = xevent->xkey.time;
  }

  return ECORE_CALLBACK_PASS_ON;
}

static Eo* eflWebHostWindow(Ihandle* ih)
{
  Ihandle* dialog = IupGetDialog(ih);
  return dialog ? iupeflGetWidget(dialog) : NULL;
}

static void eflWebHostPlace(Ihandle* ih)
{
  Eo* widget = iupeflGetWidget(ih);
  Ihandle* parent;
  Eina_Rect geom;
  int x1, y1, x2, y2;
  int visible;

  if (!widget)
    return;

  geom = efl_gfx_entity_geometry_get(widget);
  x1 = geom.x; y1 = geom.y; x2 = geom.x + geom.w; y2 = geom.y + geom.h;
  visible = iupeflIsVisible(widget);

  for (parent = ih->parent; visible && parent && parent->iclass->nativetype != IUP_TYPEDIALOG; parent = parent->parent)
  {
    Eo* parent_widget = (Eo*)iupAttribGet(parent, "_IUP_EXTRAPARENT");
    Eina_Rect pgeom;

    if (!parent_widget)
      parent_widget = iupeflGetWidget(parent);
    if (parent->iclass->nativetype == IUP_TYPEVOID || !parent_widget)
      continue;

    if (!iupeflIsVisible(parent_widget))
      visible = 0;

    pgeom = efl_gfx_entity_geometry_get(parent_widget);
    if (pgeom.x > x1) x1 = pgeom.x;
    if (pgeom.y > y1) y1 = pgeom.y;
    if (pgeom.x + pgeom.w < x2) x2 = pgeom.x + pgeom.w;
    if (pgeom.y + pgeom.h < y2) y2 = pgeom.y + pgeom.h;
  }

  if (!visible || x2 <= x1 || y2 <= y1)
    iupwebHostSetBounds(ih, geom.x, geom.y, geom.w, geom.h, 0, 0, 0, 0);
  else
    iupwebHostSetBounds(ih, geom.x, geom.y, geom.w, geom.h, x1, y1, x2 - x1, y2 - y1);
}

static void eflWebHostChangedEvent(void* data, const Efl_Event* ev)
{
  (void)ev;
  eflWebHostPlace((Ihandle*)data);
}

static void eflWebHostFocusEvent(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  int focus = efl_ui_focus_object_focus_get(ev->object);

  if (!iupObjectCheck(ih))
    return;

  iupwebHostSetFocus(ih, focus);

  if (focus)
  {
    if (iupdrvIsActive(ih))
      iupCallGetFocusCb(ih);
  }
  else
    iupCallKillFocusCb(ih);
}

static void eflWebHostKeyEvent(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Efl_Input_Key* key_event = ev->info;
  int press = (ev->desc == EFL_EVENT_KEY_DOWN);

  if (efl_webhost_key.press == press && efl_webhost_key.keycode == (unsigned int)efl_input_key_code_get(key_event))
    iupwebHostKeyEvent(ih, press, efl_webhost_key.keycode, efl_webhost_key.state, efl_webhost_key.time);

  efl_input_processed_set(key_event, EINA_TRUE);
}

void* iupwebHostMap(Ihandle* ih)
{
  Eo* parent = iupeflGetParentWidget(ih);
  Eo* win = eflWebHostWindow(ih);
  Eo* widget;

  if (!parent || !win || !iupeflIsX11())
    return NULL;

  if (!efl_webhost_key_handler)
    efl_webhost_key_handler = ecore_event_handler_add(ECORE_X_EVENT_ANY, eflWebHostXEvent, NULL);

  widget = efl_add(EFL_UI_IMAGE_CLASS, parent);
  if (!widget)
    return NULL;

  elm_object_focus_allow_set(widget, iupAttribGetBoolean(ih, "CANFOCUS") ? EINA_TRUE : EINA_FALSE);
  iupeflSetVisible(widget, EINA_TRUE);

  ih->handle = (InativeHandle*)widget;

  efl_event_callback_add(widget, EFL_GFX_ENTITY_EVENT_POSITION_CHANGED, eflWebHostChangedEvent, ih);
  efl_event_callback_add(widget, EFL_GFX_ENTITY_EVENT_SIZE_CHANGED, eflWebHostChangedEvent, ih);
  efl_event_callback_add(widget, EFL_GFX_ENTITY_EVENT_VISIBILITY_CHANGED, eflWebHostChangedEvent, ih);
  efl_event_callback_add(widget, EFL_UI_FOCUS_OBJECT_EVENT_FOCUS_CHANGED, eflWebHostFocusEvent, ih);
  efl_event_callback_priority_add(widget, EFL_EVENT_KEY_DOWN, EFL_CALLBACK_PRIORITY_BEFORE, eflWebHostKeyEvent, ih);
  efl_event_callback_priority_add(widget, EFL_EVENT_KEY_UP, EFL_CALLBACK_PRIORITY_BEFORE, eflWebHostKeyEvent, ih);

  iupeflAddToParent(ih);

  ecore_x_sync();
  return (void*)(uintptr_t)elm_win_xwindow_get(win);
}

void iupwebHostLayoutUpdate(Ihandle* ih)
{
  iupdrvBaseLayoutUpdateMethod(ih);
  eflWebHostPlace(ih);
}

void iupwebHostUnMap(Ihandle* ih)
{
  Eo* widget = iupeflGetWidget(ih);
  if (!widget)
    return;

  efl_event_callback_del(widget, EFL_GFX_ENTITY_EVENT_POSITION_CHANGED, eflWebHostChangedEvent, ih);
  efl_event_callback_del(widget, EFL_GFX_ENTITY_EVENT_SIZE_CHANGED, eflWebHostChangedEvent, ih);
  efl_event_callback_del(widget, EFL_GFX_ENTITY_EVENT_VISIBILITY_CHANGED, eflWebHostChangedEvent, ih);
  efl_event_callback_del(widget, EFL_UI_FOCUS_OBJECT_EVENT_FOCUS_CHANGED, eflWebHostFocusEvent, ih);
  efl_event_callback_del(widget, EFL_EVENT_KEY_DOWN, eflWebHostKeyEvent, ih);
  efl_event_callback_del(widget, EFL_EVENT_KEY_UP, eflWebHostKeyEvent, ih);

  iupdrvBaseUnMapMethod(ih);
}
