/** \file
 * \brief Canvas Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_dialog.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_drvfont.h"
#include "iup_canvas.h"
#include "iup_key.h"

#include "iupefl_drv.h"


static void eflCanvasScrollChangedCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Eina_Position2D pos;
  IFniff scroll_cb;
  double old_posx, old_posy;
  int op;

  pos = efl_ui_scrollable_content_pos_get(ev->object);

  old_posx = ih->data->posx;
  old_posy = ih->data->posy;
  ih->data->posx = (double)pos.x;
  ih->data->posy = (double)pos.y;

  if (old_posx == ih->data->posx && old_posy == ih->data->posy)
    return;

  if (old_posx != ih->data->posx)
    op = IUP_SBPOSH;
  else
    op = IUP_SBPOSV;

  scroll_cb = (IFniff)IupGetCallback(ih, "SCROLL_CB");
  if (scroll_cb)
    scroll_cb(ih, op, (float)ih->data->posx, (float)ih->data->posy);
  else
  {
    IFn cb = (IFn)IupGetCallback(ih, "ACTION");
    if (cb)
      iupdrvRedrawNow(ih);
  }
}

static void eflCanvasUpdateScrollContentSize(Ihandle* ih)
{
  Eo* scroller = (Eo*)iupAttribGet(ih, "_IUP_EFL_SCROLLER");
  Eo* vg = iupeflGetWidget(ih);
  double xmin, xmax, ymin, ymax;
  int content_w, content_h;

  if (!scroller || !vg)
    return;

  xmin = iupAttribGetDouble(ih, "XMIN");
  xmax = iupAttribGetDouble(ih, "XMAX");
  ymin = iupAttribGetDouble(ih, "YMIN");
  ymax = iupAttribGetDouble(ih, "YMAX");

  content_w = (int)(xmax - xmin);
  content_h = (int)(ymax - ymin);

  if (content_w < 1) content_w = 1;
  if (content_h < 1) content_h = 1;

  efl_gfx_hint_size_min_set(vg, EINA_SIZE2D(content_w, content_h));
  efl_gfx_entity_size_set(vg, EINA_SIZE2D(content_w, content_h));
}

static int eflCanvasSetDXAttrib(Ihandle* ih, const char* value)
{
  Eo* scroller = (Eo*)iupAttribGet(ih, "_IUP_EFL_SCROLLER");
  double xmin, xmax, dx;

  if (!scroller)
    return 1;

  xmin = iupAttribGetDouble(ih, "XMIN");
  xmax = iupAttribGetDouble(ih, "XMAX");

  if (!iupStrToDoubleDef(value, &dx, 0.1))
    return 1;

  eflCanvasUpdateScrollContentSize(ih);

  if (iupAttribGetBoolean(ih, "XAUTOHIDE"))
  {
    Efl_Ui_Scrollbar_Mode h_mode, v_mode;
    efl_ui_scrollbar_bar_mode_get(scroller, &h_mode, &v_mode);

    if (dx >= (xmax - xmin))
      h_mode = EFL_UI_SCROLLBAR_MODE_OFF;
    else if (ih->data->sb & IUP_SB_HORIZ)
      h_mode = EFL_UI_SCROLLBAR_MODE_AUTO;

    efl_ui_scrollbar_bar_mode_set(scroller, h_mode, v_mode);
  }

  return 1;
}

static int eflCanvasSetDYAttrib(Ihandle* ih, const char* value)
{
  Eo* scroller = (Eo*)iupAttribGet(ih, "_IUP_EFL_SCROLLER");
  double ymin, ymax, dy;

  if (!scroller)
    return 1;

  ymin = iupAttribGetDouble(ih, "YMIN");
  ymax = iupAttribGetDouble(ih, "YMAX");

  if (!iupStrToDoubleDef(value, &dy, 0.1))
    return 1;

  eflCanvasUpdateScrollContentSize(ih);

  if (iupAttribGetBoolean(ih, "YAUTOHIDE"))
  {
    Efl_Ui_Scrollbar_Mode h_mode, v_mode;
    efl_ui_scrollbar_bar_mode_get(scroller, &h_mode, &v_mode);

    if (dy >= (ymax - ymin))
      v_mode = EFL_UI_SCROLLBAR_MODE_OFF;
    else if (ih->data->sb & IUP_SB_VERT)
      v_mode = EFL_UI_SCROLLBAR_MODE_AUTO;

    efl_ui_scrollbar_bar_mode_set(scroller, h_mode, v_mode);
  }

  return 1;
}

static int eflCanvasSetPosXAttrib(Ihandle* ih, const char* value)
{
  Eo* scroller = (Eo*)iupAttribGet(ih, "_IUP_EFL_SCROLLER");
  double posx, xmin, xmax, dx;

  if (!value)
    posx = 0;
  else
    iupStrToDouble(value, &posx);

  xmin = iupAttribGetDouble(ih, "XMIN");
  xmax = iupAttribGetDouble(ih, "XMAX");
  dx = iupAttribGetDouble(ih, "DX");

  if (posx < xmin) posx = xmin;
  if (posx > (xmax - dx)) posx = xmax - dx;

  ih->data->posx = posx;

  if (scroller)
  {
    Eina_Position2D pos = efl_ui_scrollable_content_pos_get(scroller);
    pos.x = (int)posx;
    efl_ui_scrollable_content_pos_set(scroller, pos);
  }

  return 1;
}

static int eflCanvasSetPosYAttrib(Ihandle* ih, const char* value)
{
  Eo* scroller = (Eo*)iupAttribGet(ih, "_IUP_EFL_SCROLLER");
  double posy, ymin, ymax, dy;

  if (!value)
    posy = 0;
  else
    iupStrToDouble(value, &posy);

  ymin = iupAttribGetDouble(ih, "YMIN");
  ymax = iupAttribGetDouble(ih, "YMAX");
  dy = iupAttribGetDouble(ih, "DY");

  if (posy < ymin) posy = ymin;
  if (posy > (ymax - dy)) posy = ymax - dy;

  ih->data->posy = posy;

  if (scroller)
  {
    Eina_Position2D pos = efl_ui_scrollable_content_pos_get(scroller);
    pos.y = (int)posy;
    efl_ui_scrollable_content_pos_set(scroller, pos);
  }

  return 1;
}

/****************************************************************
                     Callbacks
****************************************************************/

static void eflCanvasResizeCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Eo* scroller = (Eo*)iupAttribGet(ih, "_IUP_EFL_SCROLLER");
  Eina_Size2D size;
  IFnii cb;

  (void)ev;

  if (scroller)
    size = efl_gfx_entity_size_get(scroller);
  else
    size = efl_gfx_entity_size_get(iupeflGetWidget(ih));

  cb = (IFnii)IupGetCallback(ih, "RESIZE_CB");
  if (cb)
    cb(ih, size.w, size.h);
}

static void eflCanvasActionCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Eo* scroller = (Eo*)iupAttribGet(ih, "_IUP_EFL_SCROLLER");
  IFn cb;
  Eina_Size2D size;

  (void)ev;

  if (scroller)
    size = efl_gfx_entity_size_get(scroller);
  else
    size = efl_gfx_entity_size_get(iupeflGetWidget(ih));

  if (size.w <= 0 || size.h <= 0)
    return;

  cb = (IFn)IupGetCallback(ih, "ACTION");
  if (cb)
    cb(ih);
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

static void eflCanvasKeyCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Efl_Input_Key* key_ev = ev->info;
  IFnii cb;
  IFni cb_any;
  int key;
  const char* keyname;

  keyname = efl_input_key_name_get(key_ev);
  if (!keyname)
    return;

  key = iupeflKeyDecode(NULL);
  if (key == 0)
    return;

  cb = (IFnii)IupGetCallback(ih, "KEYPRESS_CB");
  if (cb)
  {
    int ret = cb(ih, key, 1);
    if (ret == IUP_CLOSE)
      IupExitLoop();
    else if (ret == IUP_IGNORE)
      return;
  }

  cb_any = (IFni)IupGetCallback(ih, "K_ANY");
  if (cb_any)
  {
    int ret = cb_any(ih, key);
    if (ret == IUP_CLOSE)
      IupExitLoop();
  }
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
  Eo* scroller = (Eo*)iupAttribGet(ih, "_IUP_EFL_SCROLLER");
  Eina_Size2D size;

  if (scroller)
  {
    size = efl_gfx_entity_size_get(scroller);
  }
  else
  {
    Eo* vg = iupeflGetWidget(ih);
    if (!vg)
      return NULL;
    size = efl_gfx_entity_size_get(vg);
  }

  return iupStrReturnIntInt(size.w, size.h, 'x');
}

/****************************************************************
                     Methods
****************************************************************/

static int eflCanvasMapMethod(Ihandle* ih)
{
  Eo* parent;
  Eo* vg;
  Eo* scroller = NULL;
  Eo* vg_parent;
  Efl_VG* root;

  parent = iupeflGetParentWidget(ih);
  if (!parent)
    return IUP_ERROR;

  ih->data->sb = iupBaseGetScrollbar(ih);

  if (ih->data->sb)
  {
    Efl_Ui_Scrollbar_Mode h_mode = EFL_UI_SCROLLBAR_MODE_OFF;
    Efl_Ui_Scrollbar_Mode v_mode = EFL_UI_SCROLLBAR_MODE_OFF;

    scroller = efl_add(EFL_UI_SCROLLER_CLASS, parent,
      efl_gfx_entity_visible_set(efl_added, EINA_TRUE));
    if (!scroller)
      return IUP_ERROR;

    if (ih->data->sb & IUP_SB_HORIZ)
      h_mode = EFL_UI_SCROLLBAR_MODE_AUTO;
    if (ih->data->sb & IUP_SB_VERT)
      v_mode = EFL_UI_SCROLLBAR_MODE_AUTO;

    efl_ui_scrollbar_bar_mode_set(scroller, h_mode, v_mode);

    iupAttribSet(ih, "_IUP_EFL_SCROLLER", (char*)scroller);
    iupAttribSet(ih, "_IUP_EXTRAPARENT", (char*)scroller);

    efl_event_callback_add(scroller, EFL_UI_EVENT_SCROLL_CHANGED, eflCanvasScrollChangedCallback, ih);
  }

  vg_parent = scroller ? scroller : parent;

  {
    Evas* evas = evas_object_evas_get(vg_parent);
    vg = efl_add(EFL_CANVAS_VG_OBJECT_CLASS, evas,
      efl_canvas_object_key_focus_set(efl_added, EINA_TRUE));
  }

  if (!vg)
  {
    if (scroller)
      efl_del(scroller);
    return IUP_ERROR;
  }

  efl_canvas_object_pass_events_set(vg, EINA_FALSE);
  efl_canvas_object_repeat_events_set(vg, EINA_FALSE);

  ih->handle = (InativeHandle*)vg;

  if (scroller)
    efl_content_set(scroller, vg);

  root = efl_add(EFL_CANVAS_VG_CONTAINER_CLASS, vg);
  if (!root)
  {
    if (scroller)
      efl_del(scroller);
    efl_del(vg);
    ih->handle = NULL;
    return IUP_ERROR;
  }

  efl_canvas_vg_object_root_node_set(vg, root);
  iupAttribSet(ih, "_IUP_EFL_VG_ROOT", (char*)root);

  if (scroller)
  {
    efl_event_callback_add(scroller, EFL_GFX_ENTITY_EVENT_SIZE_CHANGED, eflCanvasActionCallback, ih);
    efl_event_callback_add(scroller, EFL_GFX_ENTITY_EVENT_SIZE_CHANGED, eflCanvasResizeCallback, ih);
  }
  else
  {
    efl_event_callback_add(vg, EFL_GFX_ENTITY_EVENT_SIZE_CHANGED, eflCanvasActionCallback, ih);
    efl_event_callback_add(vg, EFL_GFX_ENTITY_EVENT_SIZE_CHANGED, eflCanvasResizeCallback, ih);
  }
  efl_event_callback_add(vg, EFL_EVENT_POINTER_MOVE, eflCanvasMotionCallback, ih);
  efl_event_callback_add(vg, EFL_EVENT_POINTER_DOWN, eflCanvasButtonCallback, ih);
  efl_event_callback_add(vg, EFL_EVENT_POINTER_UP, eflCanvasButtonCallback, ih);
  efl_event_callback_add(vg, EFL_EVENT_POINTER_WHEEL, eflCanvasWheelCallback, ih);
  efl_event_callback_add(vg, EFL_EVENT_KEY_DOWN, eflCanvasKeyCallback, ih);
  efl_event_callback_add(vg, EFL_EVENT_POINTER_IN, iupeflPointerInEvent, ih);
  efl_event_callback_add(vg, EFL_EVENT_POINTER_OUT, iupeflPointerOutEvent, ih);

  eflCanvasSetDXAttrib(ih, NULL);
  eflCanvasSetDYAttrib(ih, NULL);

  return IUP_NOERROR;
}

typedef struct _IeflVgImageData {
  Efl_VG* node;
  void* pixels;
} IeflVgImageData;

static void eflCanvasUnMapMethod(Ihandle* ih)
{
  Eo* vg = iupeflGetWidget(ih);
  Eo* scroller = (Eo*)iupAttribGet(ih, "_IUP_EFL_SCROLLER");
  Efl_VG* root = (Efl_VG*)iupAttribGet(ih, "_IUP_EFL_VG_ROOT");
  Eina_List* evas_objects = (Eina_List*)iupAttribGet(ih, "_IUP_EFL_EVAS_OBJECTS");
  Eina_List* vg_images = (Eina_List*)iupAttribGet(ih, "_IUP_EFL_VG_IMAGES");


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

  if (scroller)
  {
    efl_event_callback_del(scroller, EFL_GFX_ENTITY_EVENT_SIZE_CHANGED, eflCanvasResizeCallback, ih);
    efl_event_callback_del(scroller, EFL_GFX_ENTITY_EVENT_SIZE_CHANGED, eflCanvasActionCallback, ih);
    efl_event_callback_del(scroller, EFL_UI_EVENT_SCROLL_CHANGED, eflCanvasScrollChangedCallback, ih);
  }

  if (vg)
  {
    if (!scroller)
    {
      efl_event_callback_del(vg, EFL_GFX_ENTITY_EVENT_SIZE_CHANGED, eflCanvasResizeCallback, ih);
      efl_event_callback_del(vg, EFL_GFX_ENTITY_EVENT_SIZE_CHANGED, eflCanvasActionCallback, ih);
    }
    efl_event_callback_del(vg, EFL_EVENT_POINTER_MOVE, eflCanvasMotionCallback, ih);
    efl_event_callback_del(vg, EFL_EVENT_POINTER_DOWN, eflCanvasButtonCallback, ih);
    efl_event_callback_del(vg, EFL_EVENT_POINTER_UP, eflCanvasButtonCallback, ih);
    efl_event_callback_del(vg, EFL_EVENT_POINTER_WHEEL, eflCanvasWheelCallback, ih);
    efl_event_callback_del(vg, EFL_EVENT_KEY_DOWN, eflCanvasKeyCallback, ih);

    if (root)
      efl_del(root);

    efl_del(vg);
  }

  if (scroller)
  {
    efl_del(scroller);
    iupAttribSet(ih, "_IUP_EFL_SCROLLER", NULL);
    iupAttribSet(ih, "_IUP_EXTRAPARENT", NULL);
  }

  iupAttribSet(ih, "_IUP_EFL_VG_ROOT", NULL);
  ih->handle = NULL;
}

static void eflCanvasLayoutUpdateMethod(Ihandle* ih)
{
  Eo* scroller = (Eo*)iupAttribGet(ih, "_IUP_EFL_SCROLLER");
  Eo* vg = iupeflGetWidget(ih);

  if (scroller)
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

    efl_gfx_entity_position_set(scroller, EINA_POSITION2D(abs_x, abs_y));
    efl_gfx_entity_size_set(scroller, EINA_SIZE2D(ih->currentwidth, ih->currentheight));
  }
  else if (vg)
  {
    iupeflSetPosSize(ih, ih->x, ih->y, ih->currentwidth, ih->currentheight);
  }
}

static void* eflCanvasGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
  (void)child;

  return iupeflGetParentWidget(ih);
}

void iupdrvCanvasInitClass(Iclass* ic)
{
  ic->Map = eflCanvasMapMethod;
  ic->UnMap = eflCanvasUnMapMethod;
  ic->LayoutUpdate = eflCanvasLayoutUpdateMethod;
  ic->GetInnerNativeContainerHandle = eflCanvasGetInnerNativeContainerHandleMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, eflCanvasSetBgColorAttrib, "255 255 255", NULL, IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "DRAWABLE", eflCanvasGetDrawableAttrib, NULL, NULL, NULL, IUPAF_NO_STRING | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAWSIZE", eflCanvasGetDrawSizeAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DX", NULL, eflCanvasSetDXAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DY", NULL, eflCanvasSetDYAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "POSX", iupCanvasGetPosXAttrib, eflCanvasSetPosXAttrib, "0", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "POSY", iupCanvasGetPosYAttrib, eflCanvasSetPosYAttrib, "0", NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "XMIN", NULL, NULL, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "XMAX", NULL, NULL, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "YMIN", NULL, NULL, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "YMAX", NULL, NULL, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BACKINGSTORE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOUCH", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLVISIBLE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
}
