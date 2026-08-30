/** \file
 * \brief Canvas Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_canvas.h"
#include "iup_key.h"

#include "iupefl_drv.h"


static void eflCanvasScrollNotify(Ihandle* ih, int op)
{
  IFniff scroll_cb = (IFniff)IupGetCallback(ih, "SCROLL_CB");
  if (scroll_cb)
    scroll_cb(ih, op, (float)ih->data->posx, (float)ih->data->posy);
  else if (IupGetCallback(ih, "ACTION"))
    iupdrvRedrawNow(ih);
}

/* Evas has no expose event, so a burst of update requests collapses into one repaint here */
static void eflCanvasRenderPreCallback(void* data, Evas* e, void* event_info)
{
  Ihandle* ih = (Ihandle*)data;
  IFn cb;

  (void)e;
  (void)event_info;

  if (!iupAttribGet(ih, "_IUP_EFL_REDRAW_PENDING"))
    return;

  iupAttribSet(ih, "_IUP_EFL_REDRAW_PENDING", NULL);

  if (!ih->handle || !iupeflCanvasHasSize(ih))
    return;

  cb = (IFn)IupGetCallback(ih, "ACTION");
  if (cb)
  {
    char* updaterect = iupAttribGet(ih, "_IUP_EFL_UPDATERECT");
    if (updaterect)
      iupAttribSetStr(ih, "CLIPRECT", updaterect);
    else
    {
      Eina_Size2D size = efl_gfx_entity_size_get(iupeflGetWidget(ih));
      iupAttribSetStrf(ih, "CLIPRECT", "%d %d %d %d", 0, 0, size.w - 1, size.h - 1);
    }

    cb(ih);

    iupAttribSet(ih, "CLIPRECT", NULL);
    iupAttribSet(ih, "_IUP_EFL_UPDATERECT", NULL);
  }
}

IUP_DRV_API void iupeflRedrawSetPending(Ihandle* ih)
{
  iupAttribSet(ih, "_IUP_EFL_REDRAW_PENDING", "1");
}

IUP_DRV_API void iupeflRedrawClearPending(Ihandle* ih)
{
  iupAttribSet(ih, "_IUP_EFL_REDRAW_PENDING", NULL);
}

static void eflCanvasSliderChangedCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Eo* vsb = (Eo*)iupAttribGet(ih, "_IUP_EFL_VSB");
  double value = efl_ui_range_value_get(ev->object);

  if (ev->object == vsb)
  {
    if (ih->data->posy == value)
      return;
    ih->data->posy = value;
    eflCanvasScrollNotify(ih, IUP_SBPOSV);
  }
  else
  {
    if (ih->data->posx == value)
      return;
    ih->data->posx = value;
    eflCanvasScrollNotify(ih, IUP_SBPOSH);
  }
}

/* the range is in application units, the drawing surface is always the visible area */
static void eflCanvasUpdateScrollBar(Ihandle* ih, int horiz)
{
  Eo* sb = (Eo*)iupAttribGet(ih, horiz ? "_IUP_EFL_HSB" : "_IUP_EFL_VSB");
  double lo, hi, page, pos;

  if (!sb)
    return;

  lo = iupAttribGetDouble(ih, horiz ? "XMIN" : "YMIN");
  hi = iupAttribGetDouble(ih, horiz ? "XMAX" : "YMAX");
  page = iupAttribGetDouble(ih, horiz ? "DX" : "DY");
  pos = horiz ? ih->data->posx : ih->data->posy;

  if (page >= (hi - lo) || (hi - lo) <= 0)
  {
    /* the slider rejects an empty range, so keep one unit and take the thumb out of use */
    efl_ui_range_limits_set(sb, lo, lo + 1);
    efl_ui_range_value_set(sb, lo);
    efl_ui_widget_disabled_set(sb, EINA_TRUE);
    iupeflSetVisible(sb, !iupAttribGetBoolean(ih, horiz ? "XAUTOHIDE" : "YAUTOHIDE"));
    return;
  }

  efl_ui_widget_disabled_set(sb, EINA_FALSE);
  iupeflSetVisible(sb, EINA_TRUE);

  if (pos < lo) pos = lo;
  if (pos > hi - page) pos = hi - page;

  {
    double step = iupAttribGetDouble(ih, horiz ? "LINEX" : "LINEY");
    efl_ui_range_limits_set(sb, lo, hi - page);
    if (step > 0)
      efl_ui_range_step_set(sb, step);
    efl_ui_range_value_set(sb, pos);
  }
}

static int eflCanvasSetDXAttrib(Ihandle* ih, const char* value)
{
  double dx;

  if (!iupStrToDoubleDef(value, &dx, 0.1))
    return 1;

  iupAttribSetDouble(ih, "DX", dx);
  eflCanvasUpdateScrollBar(ih, 1);
  return 1;
}

static int eflCanvasSetDYAttrib(Ihandle* ih, const char* value)
{
  double dy;

  if (!iupStrToDoubleDef(value, &dy, 0.1))
    return 1;

  iupAttribSetDouble(ih, "DY", dy);
  eflCanvasUpdateScrollBar(ih, 0);
  return 1;
}

static int eflCanvasSetPosXAttrib(Ihandle* ih, const char* value)
{
  double pos, lo, hi, page;

  if (!value)
    pos = 0;
  else
    iupStrToDouble(value, &pos);

  lo = iupAttribGetDouble(ih, "XMIN");
  hi = iupAttribGetDouble(ih, "XMAX");
  page = iupAttribGetDouble(ih, "DX");

  if (pos < lo) pos = lo;
  if (pos > (hi - page)) pos = hi - page;

  ih->data->posx = pos;
  eflCanvasUpdateScrollBar(ih, 1);

  return 1;
}

static int eflCanvasSetPosYAttrib(Ihandle* ih, const char* value)
{
  double pos, lo, hi, page;

  if (!value)
    pos = 0;
  else
    iupStrToDouble(value, &pos);

  lo = iupAttribGetDouble(ih, "YMIN");
  hi = iupAttribGetDouble(ih, "YMAX");
  page = iupAttribGetDouble(ih, "DY");

  if (pos < lo) pos = lo;
  if (pos > (hi - page)) pos = hi - page;

  ih->data->posy = pos;
  eflCanvasUpdateScrollBar(ih, 0);

  return 1;
}

/****************************************************************
                     Callbacks
****************************************************************/

static void eflCanvasResizeCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Eina_Size2D size;
  IFnii cb;

  (void)ev;

  size = efl_gfx_entity_size_get(iupeflGetWidget(ih));

  cb = (IFnii)IupGetCallback(ih, "RESIZE_CB");
  if (cb)
    cb(ih, size.w, size.h);
}

static void eflCanvasActionCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Eina_Size2D size;

  (void)ev;

  size = efl_gfx_entity_size_get(iupeflGetWidget(ih));

  if (size.w <= 0 || size.h <= 0)
    return;

  iupdrvRedrawNow(ih);
}

static void eflCanvasSetModifierStatus(Efl_Input_Pointer* pointer, char* status)
{
  if (efl_input_modifier_enabled_get(pointer, EFL_INPUT_MODIFIER_SHIFT, NULL))
    iupKEY_SETSHIFT(status);
  if (efl_input_modifier_enabled_get(pointer, EFL_INPUT_MODIFIER_CONTROL, NULL))
    iupKEY_SETCONTROL(status);
  if (efl_input_modifier_enabled_get(pointer, EFL_INPUT_MODIFIER_ALT, NULL))
    iupKEY_SETALT(status);
  if (efl_input_modifier_enabled_get(pointer, EFL_INPUT_MODIFIER_META, NULL) ||
      efl_input_modifier_enabled_get(pointer, EFL_INPUT_MODIFIER_SUPER, NULL))
    iupKEY_SETSYS(status);
}

static void eflCanvasMotionCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Efl_Input_Pointer* pointer = ev->info;
  IFniis cb;
  char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
  Eina_Position2D pos;
  Eina_Position2D canvas_pos;
  unsigned int pressed_buttons;

  pos = efl_input_pointer_position_get(pointer);
  canvas_pos = efl_gfx_entity_position_get(ev->object);

  eflCanvasSetModifierStatus(pointer, status);

  pressed_buttons = (unsigned int)efl_input_pointer_value_get(pointer, EFL_INPUT_VALUE_BUTTONS_PRESSED);
  if (pressed_buttons & (1 << 0))
    iupKEY_SETBUTTON1(status);
  if (pressed_buttons & (1 << 1))
    iupKEY_SETBUTTON2(status);
  if (pressed_buttons & (1 << 2))
    iupKEY_SETBUTTON3(status);

  cb = (IFniis)IupGetCallback(ih, "MOTION_CB");
  if (cb)
    cb(ih, pos.x - canvas_pos.x, pos.y - canvas_pos.y, status);
}

static void eflCanvasButtonCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Efl_Input_Pointer* pointer = ev->info;
  char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
  IFniiiis cb;
  Eina_Position2D pos;
  Eina_Position2D canvas_pos;
  int button;
  int pressed;
  unsigned int pressed_buttons;

  pos = efl_input_pointer_position_get(pointer);
  canvas_pos = efl_gfx_entity_position_get(ev->object);
  button = efl_input_pointer_button_get(pointer);
  pressed = (efl_input_pointer_action_get(pointer) == EFL_POINTER_ACTION_DOWN) ? 1 : 0;

  if (pressed && iupAttribGetBoolean(ih, "CANFOCUS"))
    efl_canvas_object_key_focus_set(ev->object, EINA_TRUE);

  cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
  if (cb)
  {
    int iup_button = button;
    int ret;
    if (button == 1) iup_button = IUP_BUTTON1;
    else if (button == 2) iup_button = IUP_BUTTON2;
    else if (button == 3) iup_button = IUP_BUTTON3;

    if (efl_input_pointer_double_click_get(pointer))
      iupKEY_SETDOUBLE(status);

    eflCanvasSetModifierStatus(pointer, status);

    pressed_buttons = (unsigned int)efl_input_pointer_value_get(pointer, EFL_INPUT_VALUE_BUTTONS_PRESSED);
    if ((pressed_buttons & (1 << 0)) || button == 1)
      iupKEY_SETBUTTON1(status);
    if ((pressed_buttons & (1 << 1)) || button == 2)
      iupKEY_SETBUTTON2(status);
    if ((pressed_buttons & (1 << 2)) || button == 3)
      iupKEY_SETBUTTON3(status);

    ret = cb(ih, iup_button, pressed, pos.x - canvas_pos.x, pos.y - canvas_pos.y, status);
    if (ret == IUP_CLOSE)
      IupExitLoop();
  }
}

static void eflCanvasWheelCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Efl_Input_Pointer* pointer = ev->info;
  IFnfiis cb;
  char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
  Eina_Position2D pos;
  Eina_Position2D canvas_pos;
  int wheel_delta;
  unsigned int pressed_buttons;

  pos = efl_input_pointer_position_get(pointer);
  canvas_pos = efl_gfx_entity_position_get(ev->object);
  wheel_delta = efl_input_pointer_wheel_delta_get(pointer);

  eflCanvasSetModifierStatus(pointer, status);

  pressed_buttons = (unsigned int)efl_input_pointer_value_get(pointer, EFL_INPUT_VALUE_BUTTONS_PRESSED);
  if (pressed_buttons & (1 << 0))
    iupKEY_SETBUTTON1(status);
  if (pressed_buttons & (1 << 1))
    iupKEY_SETBUTTON2(status);
  if (pressed_buttons & (1 << 2))
    iupKEY_SETBUTTON3(status);

  cb = (IFnfiis)IupGetCallback(ih, "WHEEL_CB");
  if (cb)
  {
    float delta = (float)wheel_delta;
    cb(ih, delta, pos.x - canvas_pos.x, pos.y - canvas_pos.y, status);
  }
}

/****************************************************************
                     Tooltip Support
****************************************************************/

static Eo* eflCanvasGetTooltipWidget(Ihandle* ih)
{
  Eo* wrap = (Eo*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (wrap)
    return wrap;

  Eo* overlay = (Eo*)iupAttribGet(ih, "_IUP_EFL_TOOLTIP_OVERLAY");
  if (overlay)
    return overlay;

  Eo* parent = iupeflGetParentWidget(ih);
  if (!parent)
    return NULL;

  overlay = efl_add(EFL_UI_BG_CLASS, parent,
    efl_gfx_entity_visible_set(efl_added, EINA_TRUE));
  if (!overlay)
    return NULL;

  efl_gfx_color_set(overlay, 0, 0, 0, 0);
  efl_canvas_object_repeat_events_set(overlay, EINA_TRUE);

  Eo* vg = iupeflGetWidget(ih);
  if (vg)
  {
    Eina_Position2D pos = efl_gfx_entity_position_get(vg);
    Eina_Size2D size = efl_gfx_entity_size_get(vg);
    efl_gfx_entity_position_set(overlay, pos);
    efl_gfx_entity_size_set(overlay, size);
    efl_gfx_stack_above(overlay, vg);
  }

  iupAttribSet(ih, "_IUP_EFL_TOOLTIP_OVERLAY", (char*)overlay);
  return overlay;
}

static int eflCanvasSetTipAttrib(Ihandle* ih, const char* value)
{
  Eo* widget = eflCanvasGetTooltipWidget(ih);
  if (!widget)
    return 1;

  if (value && *value)
    elm_object_tooltip_text_set(widget, value);
  else
  {
    const char* old_tip = iupAttribGet(ih, "TIP");
    if (old_tip && *old_tip)
      elm_object_tooltip_unset(widget);
  }

  return 1;
}

static int eflCanvasSetTipVisibleAttrib(Ihandle* ih, const char* value)
{
  Eo* widget = eflCanvasGetTooltipWidget(ih);
  if (!widget)
    return 0;

  if (iupStrBoolean(value))
  {
    const char* tip = iupAttribGet(ih, "TIP");
    if (tip && *tip)
      elm_object_tooltip_show(widget);
  }
  else
    elm_object_tooltip_hide(widget);

  return 0;
}

static void eflCanvasFireGesture(Ihandle* ih, int gesture, int state, int x, int y, double v1, double v2)
{
  IFniiiidd cb = (IFniiiidd)IupGetCallback(ih, "GESTURE_CB");
  if (cb && cb(ih, gesture, state, x, y, v1, v2) == IUP_CLOSE)
    IupExitLoop();
}

/* zoom drives PINCH; EFL has no 2-finger pan gesture, so PAN is derived from the zoom center */
static void eflCanvasGestureZoom(Ihandle* ih, int state, void* event_info)
{
  Elm_Gesture_Zoom_Info* p = (Elm_Gesture_Zoom_Info*)event_info;
  double dx = 0, dy = 0;
  if (state == IUP_GESTURE_BEGIN)
  {
    iupAttribSetDouble(ih, "_IUPEFL_GESTURE_X0", p->x);
    iupAttribSetDouble(ih, "_IUPEFL_GESTURE_Y0", p->y);
  }
  else
  {
    dx = p->x - iupAttribGetDouble(ih, "_IUPEFL_GESTURE_X0");
    dy = p->y - iupAttribGetDouble(ih, "_IUPEFL_GESTURE_Y0");
  }
  eflCanvasFireGesture(ih, IUP_GESTURE_PINCH, state, p->x, p->y, p->zoom, 0);
  eflCanvasFireGesture(ih, IUP_GESTURE_PAN, state, p->x, p->y, dx, dy);
}

static Evas_Event_Flags eflCanvasZoomStart(void* d, void* ei) { eflCanvasGestureZoom((Ihandle*)d, IUP_GESTURE_BEGIN, ei);   return EVAS_EVENT_FLAG_NONE; }
static Evas_Event_Flags eflCanvasZoomMove(void* d, void* ei)  { eflCanvasGestureZoom((Ihandle*)d, IUP_GESTURE_CHANGED, ei); return EVAS_EVENT_FLAG_NONE; }
static Evas_Event_Flags eflCanvasZoomEnd(void* d, void* ei)   { eflCanvasGestureZoom((Ihandle*)d, IUP_GESTURE_END, ei);     return EVAS_EVENT_FLAG_NONE; }
static Evas_Event_Flags eflCanvasZoomAbort(void* d, void* ei) { eflCanvasGestureZoom((Ihandle*)d, IUP_GESTURE_CANCEL, ei);  return EVAS_EVENT_FLAG_NONE; }

static void eflCanvasGestureRotate(Ihandle* ih, int state, void* event_info)
{
  Elm_Gesture_Rotate_Info* p = (Elm_Gesture_Rotate_Info*)event_info;
  eflCanvasFireGesture(ih, IUP_GESTURE_ROTATE, state, p->x, p->y, p->angle, 0);
}

static Evas_Event_Flags eflCanvasRotateStart(void* d, void* ei) { eflCanvasGestureRotate((Ihandle*)d, IUP_GESTURE_BEGIN, ei);   return EVAS_EVENT_FLAG_NONE; }
static Evas_Event_Flags eflCanvasRotateMove(void* d, void* ei)  { eflCanvasGestureRotate((Ihandle*)d, IUP_GESTURE_CHANGED, ei); return EVAS_EVENT_FLAG_NONE; }
static Evas_Event_Flags eflCanvasRotateEnd(void* d, void* ei)   { eflCanvasGestureRotate((Ihandle*)d, IUP_GESTURE_END, ei);     return EVAS_EVENT_FLAG_NONE; }
static Evas_Event_Flags eflCanvasRotateAbort(void* d, void* ei) { eflCanvasGestureRotate((Ihandle*)d, IUP_GESTURE_CANCEL, ei);  return EVAS_EVENT_FLAG_NONE; }

static Evas_Event_Flags eflCanvasFlick(void* d, void* ei)
{
  Elm_Gesture_Line_Info* p = (Elm_Gesture_Line_Info*)ei;
  int dir;
  if (abs(p->momentum.mx) > abs(p->momentum.my)) dir = p->momentum.mx > 0 ? IUP_GESTURE_SWIPE_RIGHT : IUP_GESTURE_SWIPE_LEFT;
  else                                           dir = p->momentum.my > 0 ? IUP_GESTURE_SWIPE_DOWN : IUP_GESTURE_SWIPE_UP;
  eflCanvasFireGesture((Ihandle*)d, IUP_GESTURE_SWIPE, IUP_GESTURE_END, p->momentum.x2, p->momentum.y2, dir, 0);
  return EVAS_EVENT_FLAG_NONE;
}

static Evas_Event_Flags eflCanvasTap(void* d, void* ei)
{
  Elm_Gesture_Taps_Info* p = (Elm_Gesture_Taps_Info*)ei;
  eflCanvasFireGesture((Ihandle*)d, IUP_GESTURE_TAP, IUP_GESTURE_END, p->x, p->y, 1, 0);
  return EVAS_EVENT_FLAG_NONE;
}

static Evas_Event_Flags eflCanvasDoubleTap(void* d, void* ei)
{
  Elm_Gesture_Taps_Info* p = (Elm_Gesture_Taps_Info*)ei;
  eflCanvasFireGesture((Ihandle*)d, IUP_GESTURE_TAP, IUP_GESTURE_END, p->x, p->y, 2, 0);
  return EVAS_EVENT_FLAG_NONE;
}

static Evas_Event_Flags eflCanvasLongTap(void* d, void* ei)
{
  Elm_Gesture_Taps_Info* p = (Elm_Gesture_Taps_Info*)ei;
  eflCanvasFireGesture((Ihandle*)d, IUP_GESTURE_LONGPRESS, IUP_GESTURE_END, p->x, p->y, 0, 0);
  return EVAS_EVENT_FLAG_NONE;
}

static void eflCanvasSetupGestures(Ihandle* ih, Eo* parent, Eo* vg)
{
  Eo* gl = elm_gesture_layer_add(parent);
  if (!gl)
    return;
  elm_gesture_layer_attach(gl, vg);

  elm_gesture_layer_cb_set(gl, ELM_GESTURE_ZOOM, ELM_GESTURE_STATE_START, eflCanvasZoomStart, ih);
  elm_gesture_layer_cb_set(gl, ELM_GESTURE_ZOOM, ELM_GESTURE_STATE_MOVE,  eflCanvasZoomMove,  ih);
  elm_gesture_layer_cb_set(gl, ELM_GESTURE_ZOOM, ELM_GESTURE_STATE_END,   eflCanvasZoomEnd,   ih);
  elm_gesture_layer_cb_set(gl, ELM_GESTURE_ZOOM, ELM_GESTURE_STATE_ABORT, eflCanvasZoomAbort, ih);

  elm_gesture_layer_cb_set(gl, ELM_GESTURE_ROTATE, ELM_GESTURE_STATE_START, eflCanvasRotateStart, ih);
  elm_gesture_layer_cb_set(gl, ELM_GESTURE_ROTATE, ELM_GESTURE_STATE_MOVE,  eflCanvasRotateMove,  ih);
  elm_gesture_layer_cb_set(gl, ELM_GESTURE_ROTATE, ELM_GESTURE_STATE_END,   eflCanvasRotateEnd,   ih);
  elm_gesture_layer_cb_set(gl, ELM_GESTURE_ROTATE, ELM_GESTURE_STATE_ABORT, eflCanvasRotateAbort, ih);

  elm_gesture_layer_cb_set(gl, ELM_GESTURE_N_FLICKS, ELM_GESTURE_STATE_END, eflCanvasFlick, ih);
  elm_gesture_layer_cb_set(gl, ELM_GESTURE_N_TAPS, ELM_GESTURE_STATE_END, eflCanvasTap, ih);
  elm_gesture_layer_cb_set(gl, ELM_GESTURE_N_DOUBLE_TAPS, ELM_GESTURE_STATE_END, eflCanvasDoubleTap, ih);
  elm_gesture_layer_cb_set(gl, ELM_GESTURE_N_LONG_TAPS, ELM_GESTURE_STATE_END, eflCanvasLongTap, ih);

  iupAttribSet(ih, "_IUP_EFL_GESTURE_LAYER", (char*)gl);
}

/****************************************************************
                     Attributes
****************************************************************/

static int eflCanvasSetBgColorAttrib(Ihandle* ih, const char* value)
{
  Eo* vg = iupeflGetWidget(ih);
  unsigned char r, g, b;

  if (!vg)
    return 0;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupeflSetColor(vg, r, g, b, 255);

  return 1;
}

static char* eflCanvasGetDrawableAttrib(Ihandle* ih)
{
  return (char*)iupeflGetWidget(ih);
}

static char* eflCanvasGetDrawSizeAttrib(Ihandle* ih)
{
  Eo* vg = iupeflGetWidget(ih);
  Eina_Size2D size;

  if (!vg)
    return NULL;
  size = efl_gfx_entity_size_get(vg);

  return iupStrReturnIntInt(size.w, size.h, 'x');
}

/****************************************************************
                     Methods
****************************************************************/

static int eflCanvasMapMethod(Ihandle* ih)
{
  Eo* parent;
  Eo* vg;
  Eo* wrap = NULL;
  Eo* row = NULL;
  Eo* vg_parent;

  parent = iupeflGetParentWidget(ih);
  if (!parent)
    return IUP_ERROR;

  ih->data->sb = iupBaseGetScrollbar(ih);

  /* the range is in application units, so IUP owns the scrollbars and the surface is the viewport */
  if (ih->data->sb)
  {
    wrap = efl_add(EFL_UI_BOX_CLASS, parent, efl_gfx_entity_visible_set(efl_added, EINA_TRUE));
    if (!wrap)
      return IUP_ERROR;
    efl_ui_layout_orientation_set(wrap, EFL_UI_LAYOUT_ORIENTATION_VERTICAL);

    row = efl_add(EFL_UI_BOX_CLASS, wrap, efl_gfx_entity_visible_set(efl_added, EINA_TRUE));
    if (!row)
    {
      efl_del(wrap);
      return IUP_ERROR;
    }
    efl_ui_layout_orientation_set(row, EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL);
    efl_gfx_hint_weight_set(row, 1.0, 1.0);
    efl_pack(wrap, row);

    iupAttribSet(ih, "_IUP_EXTRAPARENT", (char*)wrap);
    iupAttribSet(ih, "_IUP_EFL_CANVAS_ROW", (char*)row);
  }

  vg_parent = row ? row : parent;

  {
    /* the frame is composited off-screen and pushed here as pixels */
    Evas* evas = evas_object_evas_get(vg_parent);
    vg = evas_object_image_filled_add(evas);
    if (vg)
    {
      evas_object_image_colorspace_set(vg, EVAS_COLORSPACE_ARGB8888);
      evas_object_image_alpha_set(vg, EINA_TRUE);
      efl_canvas_object_key_focus_set(vg, EINA_TRUE);
    }
  }

  if (!vg)
  {
    if (wrap)
      efl_del(wrap);
    return IUP_ERROR;
  }

  efl_canvas_object_pass_events_set(vg, EINA_FALSE);
  efl_canvas_object_repeat_events_set(vg, EINA_FALSE);

  ih->handle = (InativeHandle*)vg;

  evas_event_callback_add(evas_object_evas_get(vg), EVAS_CALLBACK_RENDER_PRE, eflCanvasRenderPreCallback, ih);

  if (row)
  {
    efl_gfx_hint_weight_set(vg, 1.0, 1.0);
    efl_pack(row, vg);

    if (ih->data->sb & IUP_SB_VERT)
    {
      Eo* vsb = efl_add(EFL_UI_SLIDER_CLASS, row, efl_gfx_entity_visible_set(efl_added, EINA_TRUE));
      efl_ui_layout_orientation_set(vsb, EFL_UI_LAYOUT_ORIENTATION_VERTICAL);
      efl_gfx_hint_weight_set(vsb, 0.0, 1.0);
      efl_event_callback_add(vsb, EFL_UI_RANGE_EVENT_CHANGED, eflCanvasSliderChangedCallback, ih);
      efl_pack(row, vsb);
      iupAttribSet(ih, "_IUP_EFL_VSB", (char*)vsb);
    }

    if (ih->data->sb & IUP_SB_HORIZ)
    {
      Eo* hsb = efl_add(EFL_UI_SLIDER_CLASS, wrap, efl_gfx_entity_visible_set(efl_added, EINA_TRUE));
      efl_ui_layout_orientation_set(hsb, EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL);
      efl_gfx_hint_weight_set(hsb, 1.0, 0.0);
      efl_event_callback_add(hsb, EFL_UI_RANGE_EVENT_CHANGED, eflCanvasSliderChangedCallback, ih);
      efl_pack(wrap, hsb);
      iupAttribSet(ih, "_IUP_EFL_HSB", (char*)hsb);
    }
  }
  else if (iupAttribGetBoolean(ih, "DROPTARGET"))
  {
    /* a raw vg has no smart parent so Efl.Ui.Dnd cannot reach it; wrap it in an Efl.Ui widget */
    Eo* dnd_wrap = efl_add(EFL_UI_SCROLLER_CLASS, parent, efl_gfx_entity_visible_set(efl_added, EINA_TRUE));
    efl_ui_scrollbar_bar_mode_set(dnd_wrap, EFL_UI_SCROLLBAR_MODE_OFF, EFL_UI_SCROLLBAR_MODE_OFF);
    efl_content_set(dnd_wrap, vg);
    iupAttribSet(ih, "_IUP_EXTRAPARENT", (char*)dnd_wrap);
  }

  efl_event_callback_add(vg, EFL_GFX_ENTITY_EVENT_SIZE_CHANGED, eflCanvasActionCallback, ih);
  efl_event_callback_add(vg, EFL_GFX_ENTITY_EVENT_SIZE_CHANGED, eflCanvasResizeCallback, ih);
  efl_event_callback_add(vg, EFL_EVENT_POINTER_MOVE, eflCanvasMotionCallback, ih);
  efl_event_callback_add(vg, EFL_EVENT_POINTER_DOWN, eflCanvasButtonCallback, ih);
  efl_event_callback_add(vg, EFL_EVENT_POINTER_UP, eflCanvasButtonCallback, ih);
  efl_event_callback_add(vg, EFL_EVENT_POINTER_WHEEL, eflCanvasWheelCallback, ih);
  efl_event_callback_add(vg, EFL_EVENT_KEY_DOWN, iupeflKeyDownEvent, ih);
  efl_event_callback_add(vg, EFL_EVENT_KEY_UP, iupeflKeyUpEvent, ih);
  efl_event_callback_add(vg, EFL_EVENT_POINTER_IN, iupeflPointerInEvent, ih);
  efl_event_callback_add(vg, EFL_EVENT_POINTER_OUT, iupeflPointerOutEvent, ih);

  eflCanvasSetupGestures(ih, parent, vg);

  eflCanvasSetDXAttrib(ih, NULL);
  eflCanvasSetDYAttrib(ih, NULL);

  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSet(ih, "DROPFILESTARGET", "YES");

  return IUP_NOERROR;
}

static void eflCanvasUnMapMethod(Ihandle* ih)
{
  Eo* vg = iupeflGetWidget(ih);
  Eo* wrap = (Eo*)iupAttribGet(ih, "_IUP_EXTRAPARENT");

  {
    Eo* gl = (Eo*)iupAttribGet(ih, "_IUP_EFL_GESTURE_LAYER");
    if (gl)
    {
      evas_object_del(gl);
      iupAttribSet(ih, "_IUP_EFL_GESTURE_LAYER", NULL);
    }
  }

  {
    Eo* clip = (Eo*)iupAttribGet(ih, "_IUP_EFL_CANVAS_CLIP");
    if (clip)
    {
      evas_object_del(clip);
      iupAttribSet(ih, "_IUP_EFL_CANVAS_CLIP", NULL);
    }
  }

  iupeflDrawReleaseFrame(ih);
  Eina_List* evas_objects = (Eina_List*)iupAttribGet(ih, "_IUP_EFL_EVAS_OBJECTS");
  Eina_List* vg_images = (Eina_List*)iupAttribGet(ih, "_IUP_EFL_VG_IMAGES");

  {
    Evas* evas = evas_object_evas_get(vg);
    if (evas)
      evas_sync(evas);
  }

  if (vg_images)
  {
    IeflVgImageData* data;
    EINA_LIST_FREE(vg_images, data)
    {
      if (data->node)
        efl_del(data->node);
      free(data->pixels);
      free(data);
    }
    iupAttribSet(ih, "_IUP_EFL_VG_IMAGES", NULL);
  }

  if (evas_objects)
  {
    Eo* obj;
    EINA_LIST_FREE(evas_objects, obj)
    {
      efl_del(obj);
    }
    iupAttribSet(ih, "_IUP_EFL_EVAS_OBJECTS", NULL);
  }

  if (vg)
  {
    evas_event_callback_del_full(evas_object_evas_get(vg), EVAS_CALLBACK_RENDER_PRE, eflCanvasRenderPreCallback, ih);
    iupeflRedrawClearPending(ih);
    efl_event_callback_del(vg, EFL_GFX_ENTITY_EVENT_SIZE_CHANGED, eflCanvasResizeCallback, ih);
    efl_event_callback_del(vg, EFL_GFX_ENTITY_EVENT_SIZE_CHANGED, eflCanvasActionCallback, ih);
    efl_event_callback_del(vg, EFL_EVENT_POINTER_MOVE, eflCanvasMotionCallback, ih);
    efl_event_callback_del(vg, EFL_EVENT_POINTER_DOWN, eflCanvasButtonCallback, ih);
    efl_event_callback_del(vg, EFL_EVENT_POINTER_UP, eflCanvasButtonCallback, ih);
    efl_event_callback_del(vg, EFL_EVENT_POINTER_WHEEL, eflCanvasWheelCallback, ih);
    efl_event_callback_del(vg, EFL_EVENT_KEY_DOWN, iupeflKeyDownEvent, ih);
    efl_event_callback_del(vg, EFL_EVENT_KEY_UP, iupeflKeyUpEvent, ih);

    efl_del(vg);
  }

  if (wrap)
  {
    efl_del(wrap);
    iupAttribSet(ih, "_IUP_EXTRAPARENT", NULL);
    iupAttribSet(ih, "_IUP_EFL_CANVAS_ROW", NULL);
    iupAttribSet(ih, "_IUP_EFL_VSB", NULL);
    iupAttribSet(ih, "_IUP_EFL_HSB", NULL);
  }

  {
    Eo* overlay = (Eo*)iupAttribGet(ih, "_IUP_EFL_TOOLTIP_OVERLAY");
    if (overlay)
    {
      efl_del(overlay);
      iupAttribSet(ih, "_IUP_EFL_TOOLTIP_OVERLAY", NULL);
    }
  }

  ih->handle = NULL;

  iupeflFontFree(ih);
}

static void eflCanvasLayoutUpdateMethod(Ihandle* ih)
{
  Eo* xparent = (Eo*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  Eo* vg = iupeflGetWidget(ih);

  if (!iupeflIsInsideTabs(ih))
  {
    if (xparent)
    {
      Ihandle* parent;
      int abs_x = ih->x;
      int abs_y = ih->y;

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

      efl_gfx_entity_position_set(xparent, EINA_POSITION2D(abs_x, abs_y));
      efl_gfx_entity_size_set(xparent, EINA_SIZE2D(ih->currentwidth, ih->currentheight));

      {
        Eo* clip = (Eo*)iupAttribGet(ih, "_IUP_EFL_CANVAS_CLIP");
        if (clip)
        {
          evas_object_move(clip, abs_x, abs_y);
          evas_object_resize(clip, ih->currentwidth, ih->currentheight);
        }
      }
    }
    else if (vg)
    {
      iupeflSetPosSize(ih, ih->x, ih->y, ih->currentwidth, ih->currentheight);
    }
  }

  {
    Eo* overlay = (Eo*)iupAttribGet(ih, "_IUP_EFL_TOOLTIP_OVERLAY");
    if (overlay && vg)
    {
      Eina_Position2D pos = efl_gfx_entity_position_get(vg);
      Eina_Size2D size = efl_gfx_entity_size_get(vg);
      efl_gfx_entity_position_set(overlay, pos);
      efl_gfx_entity_size_set(overlay, size);
      efl_gfx_stack_above(overlay, vg);
    }
  }
}

static void* eflCanvasGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
  (void)child;

  return iupeflGetParentWidget(ih);
}

static int eflCanvasSetUpdateRectAttrib(Ihandle* ih, const char* value)
{
  int x1, y1, x2, y2;
  Eo* widget = iupeflGetWidget(ih);
  if (widget && value && sscanf(value, "%d %d %d %d", &x1, &y1, &x2, &y2) == 4)
  {
    Evas* evas;
    char* pending = iupAttribGet(ih, "_IUP_EFL_UPDATERECT");
    if (pending)
    {
      int px1, py1, px2, py2;
      if (sscanf(pending, "%d %d %d %d", &px1, &py1, &px2, &py2) == 4)
      {
        if (px1 < x1) x1 = px1;
        if (py1 < y1) y1 = py1;
        if (px2 > x2) x2 = px2;
        if (py2 > y2) y2 = py2;
      }
    }
    iupAttribSetStrf(ih, "_IUP_EFL_UPDATERECT", "%d %d %d %d", x1, y1, x2, y2);
    iupeflRedrawSetPending(ih);

    evas = evas_object_evas_get(widget);
    if (evas)
      evas_damage_rectangle_add(evas, ih->x + x1, ih->y + y1, x2 - x1 + 1, y2 - y1 + 1);
  }
  else
    iupdrvPostRedraw(ih);
  return 0;
}

IUP_SDK_API void iupdrvCanvasInitClass(Iclass* ic)
{
  ic->Map = eflCanvasMapMethod;
  ic->UnMap = eflCanvasUnMapMethod;
  ic->LayoutUpdate = eflCanvasLayoutUpdateMethod;
  ic->GetInnerNativeContainerHandle = eflCanvasGetInnerNativeContainerHandleMethod;

  iupClassRegisterCallback(ic, "GESTURE_CB", "iiiidd");

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, eflCanvasSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "DRAWABLE", eflCanvasGetDrawableAttrib, NULL, NULL, NULL, IUPAF_NO_STRING | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAWSIZE", eflCanvasGetDrawSizeAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "UPDATERECT", NULL, eflCanvasSetUpdateRectAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DX", NULL, eflCanvasSetDXAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DY", NULL, eflCanvasSetDYAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "POSX", iupCanvasGetPosXAttrib, eflCanvasSetPosXAttrib, "0", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "POSY", iupCanvasGetPosYAttrib, eflCanvasSetPosYAttrib, "0", NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "XMIN", NULL, NULL, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "XMAX", NULL, NULL, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "YMIN", NULL, NULL, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "YMAX", NULL, NULL, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, iupeflGetNativeWindowHandleName(), iupeflGetNativeWindowHandleAttrib, NULL, NULL, NULL, IUPAF_NO_STRING|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BACKINGSTORE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOUCH", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLVISIBLE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "TIP", NULL, eflCanvasSetTipAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TIPVISIBLE", NULL, eflCanvasSetTipVisibleAttrib, NULL, NULL, IUPAF_NO_INHERIT);
}
