/** \file
 * \brief EFL Driver
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef HAVE_ECORE_X
#include <Ecore_X.h>
#endif

#include "iup.h"
#include "iupcbs.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_layout.h"
#include "iup_class.h"
#include "iup_key.h"
#include "iup_childtree.h"
#include "iup_canvas.h"

#include "iupefl_drv.h"


/****************************************************************************
 * Color Management
 ****************************************************************************/

void iupeflColorSet(Eo* obj, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
  int pr = iupeflColorPremul(a, r);
  int pg = iupeflColorPremul(a, g);
  int pb = iupeflColorPremul(a, b);

  iupeflSetColor(obj, pr, pg, pb, a);
}

int iupeflSetBgColorAttrib(Ihandle* ih, const char* value)
{
  Eo* widget = iupeflGetWidget(ih);
  Eo* bg_rect;
  unsigned char r, g, b;

  if (!widget)
    return 0;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  bg_rect = (Eo*)iupAttribGet(ih, "_IUP_EFL_BGRECT");
  if (!bg_rect)
  {
    bg_rect = efl_add(EFL_CANVAS_RECTANGLE_CLASS, widget);
    iupAttribSet(ih, "_IUP_EFL_BGRECT", (char*)bg_rect);
    efl_gfx_entity_visible_set(bg_rect, EINA_TRUE);
  }

  efl_gfx_color_set(bg_rect, r, g, b, 255);

  return 1;
}

int iupeflSetFgColorAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 1;
}

/****************************************************************************
 * Widget Visibility
 ****************************************************************************/

int iupeflBaseSetVisibleAttrib(Ihandle* ih, const char* value)
{
  Eo* widget = iupeflGetWidget(ih);

  if (!widget)
    return 0;

  iupeflSetVisible(widget, iupStrBoolean(value) ? EINA_TRUE : EINA_FALSE);

  return 0;
}

/****************************************************************************
 * Widget Active/Enabled State
 ****************************************************************************/

int iupeflBaseSetActiveAttrib(Ihandle* ih, const char* value)
{
  Eo* widget = iupeflGetWidget(ih);

  if (!widget)
    return 0;

  iupeflSetDisabled(widget, iupStrBoolean(value) ? EINA_FALSE : EINA_TRUE);

  return 0;
}

char* iupeflBaseGetActiveAttrib(Ihandle* ih)
{
  Eo* widget = iupeflGetWidget(ih);

  if (!widget)
    return "YES";

  if (!efl_isa(widget, EFL_UI_WIDGET_CLASS))
    return "YES";

  if (iupeflGetDisabled(widget))
    return "NO";
  else
    return "YES";
}

/****************************************************************************
 * Position and Size
 ****************************************************************************/

void iupeflSetPosSize(Ihandle* ih, int x, int y, int width, int height)
{
  Eo* widget = iupeflGetWidget(ih);
  Eo* bg_rect;
  Ihandle* parent;
  int abs_x, abs_y;

  if (!widget)
    return;

  abs_x = x;
  abs_y = y;
  parent = ih->parent;

  while (parent)
  {
    if (parent->iclass->nativetype != IUP_TYPEVOID)
    {
      abs_x += parent->x;
      abs_y += parent->y;
    }
    parent = parent->parent;
  }

  iupeflSetPosition(widget, abs_x, abs_y);
  iupeflSetSize(widget, width, height);

  bg_rect = (Eo*)iupAttribGet(ih, "_IUP_EFL_BGRECT");
  if (bg_rect)
  {
    efl_gfx_entity_position_set(bg_rect, EINA_POSITION2D(abs_x, abs_y));
    efl_gfx_entity_size_set(bg_rect, EINA_SIZE2D(width, height));
    efl_gfx_stack_below(bg_rect, widget);
  }
}

/****************************************************************************
 * Parent/Child Management
 ****************************************************************************/

void iupeflAddToParent(Ihandle* ih)
{
  Eo* widget = iupeflGetWidget(ih);

  if (!ih->parent || !widget)
    return;
}

Eo* iupeflGetInnerContainer(Ihandle* ih)
{
  char* inner = iupAttribGet(ih, "_IUP_EFL_INNER");
  if (inner)
    return (Eo*)inner;

  return NULL;
}

Eo* iupeflGetParentWidget(Ihandle* ih)
{
  return (Eo*)iupChildTreeGetNativeParentHandle(ih);
}

static Eo* efl_main_window = NULL;

void iupeflSetMainWindow(Eo* win)
{
  efl_main_window = win;
}

Eo* iupeflGetMainWindow(void)
{
  return efl_main_window;
}

unsigned int iupeflGetDefaultSeat(Eo* widget)
{
  Eo* win = iupeflGetMainWindow();
  if (!win)
    win = efl_provider_find(widget, EFL_UI_WIN_CLASS);
  if (!win)
    return 1;

  Eo* seat = efl_canvas_scene_seat_default_get(win);
  if (!seat)
    return 1;

  return efl_input_device_seat_id_get(seat);
}

/****************************************************************************
 * Fixed Container (for absolute positioning)
 ****************************************************************************/

Eo* iupeflFixedContainerNew(Eo* parent)
{
  Eo* box = efl_add(EFL_UI_BOX_CLASS, parent);
  if (!box)
    return NULL;

  efl_gfx_hint_weight_set(box, EFL_GFX_HINT_EXPAND, EFL_GFX_HINT_EXPAND);
  efl_gfx_hint_fill_set(box, EINA_TRUE, EINA_TRUE);

  return box;
}

void iupeflFixedContainerMove(Eo* container, Eo* child, int x, int y)
{
  (void)container;
  iupeflSetPosition(child, x, y);
}

Eo* iupeflNativeContainerNew(Eo* parent)
{
  Eo* box = efl_add(EFL_UI_BOX_CLASS, parent);
  if (!box)
    return NULL;

  efl_gfx_hint_weight_set(box, EFL_GFX_HINT_EXPAND, EFL_GFX_HINT_EXPAND);
  efl_gfx_hint_fill_set(box, EINA_TRUE, EINA_TRUE);

  return box;
}

/****************************************************************************
 * Mnemonic Handling
 ****************************************************************************/

int iupeflSetMnemonicTitle(Ihandle* ih, Eo* widget, const char* value)
{
  char* str;
  char* markup = NULL;

  (void)ih;

  if (!widget)
    return 0;

  str = iupStrProcessMnemonic(value, NULL, 0);
  if (!str)
    str = (char*)(value ? value : "");

  if (efl_isa(widget, EFL_TEXT_INTERFACE))
  {
    efl_text_set(widget, str);
  }
  else
  {
    markup = evas_textblock_text_utf8_to_markup(NULL, str);
    if (markup)
    {
      elm_object_text_set(widget, markup);
      free(markup);
    }
    else
    {
      elm_object_text_set(widget, str);
    }
  }

  if (str && str != value)
    free(str);

  return 1;
}

void iupeflUpdateMnemonic(Ihandle* ih)
{
  (void)ih;
}

/****************************************************************************
 * Base Callbacks Registration
 ****************************************************************************/

void iupeflBaseAddCallbacks(Ihandle* ih, Eo* widget)
{
  efl_event_callback_add(widget, EFL_EVENT_POINTER_IN, iupeflPointerInEvent, ih);
  efl_event_callback_add(widget, EFL_EVENT_POINTER_OUT, iupeflPointerOutEvent, ih);
  efl_event_callback_add(widget, EFL_EVENT_POINTER_DOWN, iupeflPointerDownEvent, ih);
  efl_event_callback_add(widget, EFL_EVENT_POINTER_UP, iupeflPointerUpEvent, ih);
  efl_event_callback_add(widget, EFL_EVENT_POINTER_MOVE, iupeflPointerMoveEvent, ih);
  efl_event_callback_add(widget, EFL_EVENT_POINTER_WHEEL, iupeflPointerWheelEvent, ih);
  efl_event_callback_add(widget, EFL_EVENT_KEY_DOWN, iupeflKeyDownEvent, ih);
  efl_event_callback_add(widget, EFL_EVENT_KEY_UP, iupeflKeyUpEvent, ih);

  if (efl_isa(widget, EFL_UI_WIDGET_CLASS))
  {
    efl_event_callback_add(widget, EFL_EVENT_FOCUS_IN, iupeflFocusChangedEvent, ih);
    efl_event_callback_add(widget, EFL_EVENT_FOCUS_OUT, iupeflFocusChangedEvent, ih);
  }
}

void iupeflBaseRemoveCallbacks(Ihandle* ih, Eo* widget)
{
  if (!widget)
    return;

  efl_event_callback_del(widget, EFL_EVENT_POINTER_IN, iupeflPointerInEvent, ih);
  efl_event_callback_del(widget, EFL_EVENT_POINTER_OUT, iupeflPointerOutEvent, ih);
  efl_event_callback_del(widget, EFL_EVENT_POINTER_DOWN, iupeflPointerDownEvent, ih);
  efl_event_callback_del(widget, EFL_EVENT_POINTER_UP, iupeflPointerUpEvent, ih);
  efl_event_callback_del(widget, EFL_EVENT_POINTER_MOVE, iupeflPointerMoveEvent, ih);
  efl_event_callback_del(widget, EFL_EVENT_POINTER_WHEEL, iupeflPointerWheelEvent, ih);
  efl_event_callback_del(widget, EFL_EVENT_KEY_DOWN, iupeflKeyDownEvent, ih);
  efl_event_callback_del(widget, EFL_EVENT_KEY_UP, iupeflKeyUpEvent, ih);

  if (efl_isa(widget, EFL_UI_WIDGET_CLASS))
  {
    efl_event_callback_del(widget, EFL_EVENT_FOCUS_IN, iupeflFocusChangedEvent, ih);
    efl_event_callback_del(widget, EFL_EVENT_FOCUS_OUT, iupeflFocusChangedEvent, ih);
  }
}

/****************************************************************************
 * Event Handlers
 ****************************************************************************/

void iupeflEnterLeaveEvent(void* data, Evas* e, Evas_Object* obj, void* event_info)
{
  Ihandle* ih = (Ihandle*)data;
  Evas_Event_Mouse_In* ev_in;
  Evas_Event_Mouse_Out* ev_out;
  IFn cb;

  (void)e;
  (void)obj;

  cb = (IFn)IupGetCallback(ih, "ENTERWINDOW_CB");
  if (cb)
  {
    ev_in = (Evas_Event_Mouse_In*)event_info;
    if (ev_in)
      cb(ih);
  }

  cb = (IFn)IupGetCallback(ih, "LEAVEWINDOW_CB");
  if (cb)
  {
    ev_out = (Evas_Event_Mouse_Out*)event_info;
    if (ev_out)
      cb(ih);
  }
}

void iupeflMouseMoveEvent(void* data, Evas* e, Evas_Object* obj, void* event_info)
{
  Ihandle* ih = (Ihandle*)data;
  Evas_Event_Mouse_Move* ev = (Evas_Event_Mouse_Move*)event_info;
  IFniis cb;

  (void)e;
  (void)obj;

  cb = (IFniis)IupGetCallback(ih, "MOTION_CB");
  if (cb)
  {
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    cb(ih, ev->cur.canvas.x, ev->cur.canvas.y, status);
  }
}

void iupeflMouseDownEvent(void* data, Evas* e, Evas_Object* obj, void* event_info)
{
  Ihandle* ih = (Ihandle*)data;
  Evas_Event_Mouse_Down* ev = (Evas_Event_Mouse_Down*)event_info;
  IFniiiis cb;
  char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
  int button;
  int doubleclick;

  (void)e;
  (void)obj;

  cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
  if (!cb)
    return;

  button = ev->button == 1 ? IUP_BUTTON1 :
           ev->button == 2 ? IUP_BUTTON2 :
           ev->button == 3 ? IUP_BUTTON3 : 0;
  doubleclick = (ev->flags & EVAS_BUTTON_DOUBLE_CLICK) ? 1 : 0;

  iupeflButtonKeySetStatus(ev->modifiers, ev->button, status, doubleclick);
  cb(ih, button, 1, ev->canvas.x, ev->canvas.y, status);
}

void iupeflMouseUpEvent(void* data, Evas* e, Evas_Object* obj, void* event_info)
{
  Ihandle* ih = (Ihandle*)data;
  Evas_Event_Mouse_Up* ev = (Evas_Event_Mouse_Up*)event_info;
  IFniiiis cb;
  char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
  int button;

  (void)e;
  (void)obj;

  cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
  if (!cb)
    return;

  button = ev->button == 1 ? IUP_BUTTON1 :
           ev->button == 2 ? IUP_BUTTON2 :
           ev->button == 3 ? IUP_BUTTON3 : 0;

  iupeflButtonKeySetStatus(ev->modifiers, ev->button, status, 0);
  cb(ih, button, 0, ev->canvas.x, ev->canvas.y, status);
}

void iupeflMouseWheelEvent(void* data, Evas* e, Evas_Object* obj, void* event_info)
{
  Ihandle* ih = (Ihandle*)data;
  Evas_Event_Mouse_Wheel* ev = (Evas_Event_Mouse_Wheel*)event_info;
  IFnfiis cb;

  (void)e;
  (void)obj;

  cb = (IFnfiis)IupGetCallback(ih, "WHEEL_CB");
  if (cb)
  {
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    float delta = (float)ev->z;
    cb(ih, delta, ev->canvas.x, ev->canvas.y, status);
  }
}

/****************************************************************************
 * Event Handlers
 ****************************************************************************/

void iupeflPointerDownEvent(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Efl_Input_Pointer* pointer = ev->info;
  IFniiiis cb;
  char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
  Eina_Position2D pos;
  Eina_Position2D widget_pos;
  int button;
  unsigned int pressed_buttons;

  cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
  if (!cb)
    return;

  pos = efl_input_pointer_position_get(pointer);
  widget_pos = efl_gfx_entity_position_get(ev->object);
  button = efl_input_pointer_button_get(pointer);

  int iup_button = button;
  if (button == 1) iup_button = IUP_BUTTON1;
  else if (button == 2) iup_button = IUP_BUTTON2;
  else if (button == 3) iup_button = IUP_BUTTON3;

  if (efl_input_pointer_double_click_get(pointer))
    iupKEY_SETDOUBLE(status);

  if (efl_input_modifier_enabled_get(pointer, EFL_INPUT_MODIFIER_SHIFT, NULL))
    iupKEY_SETSHIFT(status);
  if (efl_input_modifier_enabled_get(pointer, EFL_INPUT_MODIFIER_CONTROL, NULL))
    iupKEY_SETCONTROL(status);
  if (efl_input_modifier_enabled_get(pointer, EFL_INPUT_MODIFIER_ALT, NULL))
    iupKEY_SETALT(status);
  if (efl_input_modifier_enabled_get(pointer, EFL_INPUT_MODIFIER_META, NULL) ||
      efl_input_modifier_enabled_get(pointer, EFL_INPUT_MODIFIER_SUPER, NULL))
    iupKEY_SETSYS(status);

  pressed_buttons = (unsigned int)efl_input_pointer_value_get(pointer, EFL_INPUT_VALUE_BUTTONS_PRESSED);
  if ((pressed_buttons & (1 << 0)) || button == 1)
    iupKEY_SETBUTTON1(status);
  if ((pressed_buttons & (1 << 1)) || button == 2)
    iupKEY_SETBUTTON2(status);
  if ((pressed_buttons & (1 << 2)) || button == 3)
    iupKEY_SETBUTTON3(status);

  cb(ih, iup_button, 1, pos.x - widget_pos.x, pos.y - widget_pos.y, status);
}

void iupeflPointerUpEvent(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Efl_Input_Pointer* pointer = ev->info;
  IFniiiis cb;
  char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
  Eina_Position2D pos;
  Eina_Position2D widget_pos;
  int button;
  unsigned int pressed_buttons;

  cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
  if (!cb)
    return;

  pos = efl_input_pointer_position_get(pointer);
  widget_pos = efl_gfx_entity_position_get(ev->object);
  button = efl_input_pointer_button_get(pointer);

  int iup_button = button;
  if (button == 1) iup_button = IUP_BUTTON1;
  else if (button == 2) iup_button = IUP_BUTTON2;
  else if (button == 3) iup_button = IUP_BUTTON3;

  if (efl_input_modifier_enabled_get(pointer, EFL_INPUT_MODIFIER_SHIFT, NULL))
    iupKEY_SETSHIFT(status);
  if (efl_input_modifier_enabled_get(pointer, EFL_INPUT_MODIFIER_CONTROL, NULL))
    iupKEY_SETCONTROL(status);
  if (efl_input_modifier_enabled_get(pointer, EFL_INPUT_MODIFIER_ALT, NULL))
    iupKEY_SETALT(status);
  if (efl_input_modifier_enabled_get(pointer, EFL_INPUT_MODIFIER_META, NULL) ||
      efl_input_modifier_enabled_get(pointer, EFL_INPUT_MODIFIER_SUPER, NULL))
    iupKEY_SETSYS(status);

  pressed_buttons = (unsigned int)efl_input_pointer_value_get(pointer, EFL_INPUT_VALUE_BUTTONS_PRESSED);
  if (pressed_buttons & (1 << 0))
    iupKEY_SETBUTTON1(status);
  if (pressed_buttons & (1 << 1))
    iupKEY_SETBUTTON2(status);
  if (pressed_buttons & (1 << 2))
    iupKEY_SETBUTTON3(status);

  cb(ih, iup_button, 0, pos.x - widget_pos.x, pos.y - widget_pos.y, status);
}

void iupeflPointerMoveEvent(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Efl_Input_Pointer* pointer = ev->info;
  IFniis cb;
  Eina_Position2D pos;
  Eina_Position2D widget_pos;

  cb = (IFniis)IupGetCallback(ih, "MOTION_CB");
  if (!cb)
    return;

  pos = efl_input_pointer_position_get(pointer);
  widget_pos = efl_gfx_entity_position_get(ev->object);

  char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
  cb(ih, pos.x - widget_pos.x, pos.y - widget_pos.y, status);
}

void iupeflPointerWheelEvent(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Efl_Input_Pointer* pointer = ev->info;
  IFnfiis cb;
  Eina_Position2D pos;
  Eina_Position2D widget_pos;

  cb = (IFnfiis)IupGetCallback(ih, "WHEEL_CB");
  if (!cb)
    return;

  pos = efl_input_pointer_position_get(pointer);
  widget_pos = efl_gfx_entity_position_get(ev->object);

  char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
  Eina_Bool is_horizontal = efl_input_pointer_wheel_horizontal_get(pointer);
  int wheel_delta = efl_input_pointer_wheel_delta_get(pointer);
  float delta = is_horizontal ? 0.0f : (float)wheel_delta;

  cb(ih, delta, pos.x - widget_pos.x, pos.y - widget_pos.y, status);
}

void iupeflPointerInEvent(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  IFn cb;

  (void)ev;

  cb = (IFn)IupGetCallback(ih, "ENTERWINDOW_CB");
  if (cb)
    cb(ih);
}

void iupeflPointerOutEvent(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  IFn cb;

  (void)ev;

  cb = (IFn)IupGetCallback(ih, "LEAVEWINDOW_CB");
  if (cb)
    cb(ih);
}

void iupeflFocusChangedEvent(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Eina_Bool focused = *((Eina_Bool*)ev->info);
  IFn cb;

  if (focused)
  {
    cb = (IFn)IupGetCallback(ih, "GETFOCUS_CB");
    if (cb)
      cb(ih);
  }
  else
  {
    cb = (IFn)IupGetCallback(ih, "KILLFOCUS_CB");
    if (cb)
      cb(ih);
  }
}

void iupeflManagerFocusChangedEvent(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Eina_Bool* prev_focused = (Eina_Bool*)iupAttribGet(ih, "_IUP_EFL_MANAGER_FOCUSED");
  Eina_Bool currently_focused = (efl_ui_focus_manager_focus_get(ev->object) != NULL);
  IFn cb;

  if (prev_focused && *prev_focused == currently_focused)
    return;

  if (!prev_focused)
  {
    prev_focused = malloc(sizeof(Eina_Bool));
    iupAttribSet(ih, "_IUP_EFL_MANAGER_FOCUSED", (char*)prev_focused);
  }
  *prev_focused = currently_focused;

  if (currently_focused)
  {
    cb = (IFn)IupGetCallback(ih, "GETFOCUS_CB");
    if (cb)
      cb(ih);
  }
  else
  {
    cb = (IFn)IupGetCallback(ih, "KILLFOCUS_CB");
    if (cb)
      cb(ih);
  }
}

/****************************************************************************
 * Driver Interface
 ****************************************************************************/

IUP_SDK_API void iupdrvSetActive(Ihandle* ih, int enable)
{
  Eo* widget = iupeflGetWidget(ih);
  if (!widget)
    return;

  if (efl_isa(widget, EFL_UI_WIDGET_CLASS))
    iupeflSetDisabled(widget, enable ? EINA_FALSE : EINA_TRUE);
}

IUP_SDK_API int iupdrvIsActive(Ihandle* ih)
{
  Eo* widget = iupeflGetWidget(ih);
  if (!widget)
    return 1;

  if (!efl_isa(widget, EFL_UI_WIDGET_CLASS))
    return 1;

  return !iupeflGetDisabled(widget);
}

IUP_SDK_API void iupdrvSetVisible(Ihandle* ih, int visible)
{
  Eo* widget = iupeflGetWidget(ih);
  Eo* container = (Eo*)iupAttribGet(ih, "_IUP_EXTRAPARENT");

  if (container)
    iupeflSetVisible(container, visible ? EINA_TRUE : EINA_FALSE);

  if (widget && widget != (Eo*)-1)
    iupeflSetVisible(widget, visible ? EINA_TRUE : EINA_FALSE);
}

IUP_SDK_API int iupdrvIsVisible(Ihandle* ih)
{
  Eo* widget = iupeflGetWidget(ih);
  if (!widget)
    return 0;

  if (iupeflIsVisible(widget))
  {
    Ihandle* parent = ih->parent;
    while (parent)
    {
      if (parent->iclass->nativetype != IUP_TYPEVOID)
      {
        Eo* parent_widget = iupeflGetWidget(parent);
        if (!parent_widget || !iupeflIsVisible(parent_widget))
          return 0;
      }
      parent = parent->parent;
    }
    return 1;
  }
  else
    return 0;
}

IUP_SDK_API void iupdrvClientToScreen(Ihandle* ih, int* x, int* y)
{
  Eo* widget = iupeflGetWidget(ih);
  int widget_x = 0, widget_y = 0;
  int screen_x = 0, screen_y = 0;

  if (widget)
  {
    Eina_Rect geometry = iupeflGetGeometry(widget);
    widget_x = geometry.x;
    widget_y = geometry.y;

    Eo* win = iupeflGetMainWindow();
    if (win)
    {
      Evas* evas = evas_object_evas_get(win);
      if (evas)
      {
        Ecore_Evas* ee = ecore_evas_ecore_evas_get(evas);
        if (ee)
          ecore_evas_geometry_get(ee, &screen_x, &screen_y, NULL, NULL);
      }
    }
  }

  if (x) *x += widget_x + screen_x;
  if (y) *y += widget_y + screen_y;
}

IUP_SDK_API void iupdrvScreenToClient(Ihandle* ih, int* x, int* y)
{
  Eo* widget = iupeflGetWidget(ih);
  int widget_x = 0, widget_y = 0;
  int screen_x = 0, screen_y = 0;

  if (widget)
  {
    Eina_Rect geometry = iupeflGetGeometry(widget);
    widget_x = geometry.x;
    widget_y = geometry.y;

    Eo* win = iupeflGetMainWindow();
    if (win)
    {
      Evas* evas = evas_object_evas_get(win);
      if (evas)
      {
        Ecore_Evas* ee = ecore_evas_ecore_evas_get(evas);
        if (ee)
          ecore_evas_geometry_get(ee, &screen_x, &screen_y, NULL, NULL);
      }
    }
  }

  if (x) *x -= widget_x + screen_x;
  if (y) *y -= widget_y + screen_y;
}

IUP_SDK_API void iupdrvReparent(Ihandle* ih)
{
  (void)ih;
}

static void eflGetAbsolutePosition(Ihandle* ih, int* abs_x, int* abs_y)
{
  int x = ih->x;
  int y = ih->y;
  Ihandle* parent = ih->parent;

  while (parent)
  {
    if (parent->iclass->nativetype != IUP_TYPEVOID)
    {
      x += parent->x;
      y += parent->y;
    }
    parent = parent->parent;
  }

  *abs_x = x;
  *abs_y = y;
}

int iupeflIsInsideTabs(Ihandle* ih)
{
  Ihandle* parent = ih->parent;
  while (parent)
  {
    if (iupAttribGet(parent, "_IUPTAB_CONTAINER"))
      return 1;
    if (iupStrEqual(parent->iclass->name, "tabs"))
      return 1;
    parent = parent->parent;
  }
  return 0;
}

IUP_SDK_API void iupdrvBaseLayoutUpdateMethod(Ihandle* ih)
{
  Eo* widget;
  Eo* container = (Eo*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  int abs_x, abs_y;

  widget = container ? container : iupeflGetWidget(ih);
  if (!widget)
    return;

  if (iupeflIsInsideTabs(ih))
    return;

  eflGetAbsolutePosition(ih, &abs_x, &abs_y);

  iupeflSetPosition(widget, abs_x, abs_y);
  iupeflSetSize(widget, ih->currentwidth, ih->currentheight);

  {
    Eo* bg_rect = (Eo*)iupAttribGet(ih, "_IUP_EFL_BGRECT");
    if (bg_rect)
    {
      efl_gfx_entity_position_set(bg_rect, EINA_POSITION2D(abs_x, abs_y));
      efl_gfx_entity_size_set(bg_rect, EINA_SIZE2D(ih->currentwidth, ih->currentheight));
      efl_gfx_stack_below(bg_rect, widget);
    }
  }
}

IUP_SDK_API void iupdrvBaseUnMapMethod(Ihandle* ih)
{
  Eo* widget = (Eo*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (!widget)
    widget = iupeflGetWidget(ih);

  if (widget)
  {
    iupeflBaseRemoveCallbacks(ih, widget);
    iupeflDelete(widget);
  }

  ih->handle = NULL;
}

IUP_SDK_API int iupdrvBaseSetBgColorAttrib(Ihandle* ih, const char* value)
{
  Eo* widget = iupeflGetWidget(ih);
  unsigned char r, g, b;

  if (!widget)
    return 0;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupeflColorSet(widget, r, g, b, 255);
  return 1;
}

IUP_SDK_API int iupdrvBaseSetCursorAttrib(Ihandle* ih, const char* value)
{
  Eo* widget = iupeflGetWidget(ih);
  (void)value;

  if (!widget)
    return 0;

  return 0;
}

IUP_SDK_API int iupdrvBaseSetZorderAttrib(Ihandle* ih, const char* value)
{
  Eo* widget = iupeflGetWidget(ih);

  if (!widget)
    return 0;

  if (!iupdrvIsVisible(ih))
    return 0;

  if (iupStrEqualNoCase(value, "TOP"))
    efl_gfx_stack_raise_to_top(widget);
  else
    efl_gfx_stack_lower_to_bottom(widget);

  return 0;
}

IUP_SDK_API void iupdrvActivate(Ihandle* ih)
{
  Eo* widget = iupeflGetWidget(ih);
  if (widget)
  {
    if (efl_isa(widget, EFL_INPUT_CLICKABLE_MIXIN))
    {
      Efl_Input_Clickable_Clicked clicked = {0, 1};
      efl_event_callback_call(widget, EFL_INPUT_EVENT_CLICKED, &clicked);
    }
  }
}

IUP_SDK_API void iupdrvRedrawNow(Ihandle* ih)
{
  Eo* widget = iupeflGetWidget(ih);
  if (widget)
  {
    if (ih->iclass->nativetype == IUP_TYPECANVAS)
    {
      IFn cb = (IFn)IupGetCallback(ih, "ACTION");
      if (cb)
        cb(ih);
    }

    Evas* evas = evas_object_evas_get(widget);
    if (evas)
      evas_render(evas);
  }
}

IUP_SDK_API void iupdrvPostRedraw(Ihandle* ih)
{
  Eo* widget = iupeflGetWidget(ih);
  if (widget)
  {
    if (ih->iclass->nativetype == IUP_TYPECANVAS)
    {
      IFn cb = (IFn)IupGetCallback(ih, "ACTION");
      if (cb)
        cb(ih);
    }

    Evas* evas = evas_object_evas_get(widget);
    if (evas)
      evas_damage_rectangle_add(evas, ih->x, ih->y, ih->currentwidth, ih->currentheight);
  }
}

IUP_SDK_API void iupdrvBaseRegisterCommonAttrib(Iclass* ic)
{
  (void)ic;
}

IUP_SDK_API void iupdrvBaseRegisterVisualAttrib(Iclass* ic)
{
  (void)ic;
}

IUP_SDK_API void iupdrvSendKey(int key, int press)
{
  (void)key;
  (void)press;
}

IUP_SDK_API void iupdrvSendMouse(int x, int y, int bt, int status)
{
  (void)x;
  (void)y;
  (void)bt;
  (void)status;
}

IUP_SDK_API void iupdrvWarpPointer(int x, int y)
{
#ifdef HAVE_ECORE_X
  if (iupeflIsX11())
  {
    Ecore_X_Window root = ecore_x_window_root_first_get();
    if (root)
    {
      ecore_x_pointer_warp(root, x, y);
      return;
    }
  }
#endif
  (void)x;
  (void)y;
}

IUP_SDK_API void iupdrvSetAccessibleTitle(Ihandle* ih, const char* title)
{
  (void)ih;
  (void)title;
}

IUP_SDK_API void iupdrvSleep(int time)
{
#ifdef WIN32
  Sleep(time);
#else
  usleep((useconds_t)(time * 1000));
#endif
}
