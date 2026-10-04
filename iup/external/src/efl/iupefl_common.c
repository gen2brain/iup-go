/** \file
 * \brief EFL Driver
 *
 * See Copyright Notice in "iup.h"
 */

#define EFL_UI_WIDGET_PROTECTED 1

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#ifndef _WIN32
#include <unistd.h>
#endif

#include "iup.h"
#include "iupcbs.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_key.h"
#include "iup_childtree.h"
#include "iup_markup.h"
#include "iup_image.h"

#include "iupefl_drv.h"

#ifdef HAVE_ECORE_X
#include <Ecore_X.h>
#endif

#ifdef HAVE_ECORE_WL2
#include <Ecore_Wl2.h>
#endif

#ifdef _WIN32
#include <windows.h>
#undef interface
#endif


/****************************************************************************
 * Color Management
 ****************************************************************************/

IUP_DRV_API void iupeflColorSet(Eo* obj, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
  int pr = iupeflColorPremul(a, r);
  int pg = iupeflColorPremul(a, g);
  int pb = iupeflColorPremul(a, b);

  iupeflSetColor(obj, pr, pg, pb, a);
}

static void eflBgRectDelCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;

  if (iupObjectCheck(ih) && iupAttribGet(ih, "_IUP_EFL_BGRECT") == (char*)ev->object)
    iupAttribSet(ih, "_IUP_EFL_BGRECT", NULL);
}

IUP_DRV_API int iupeflSetBgColorAttrib(Ihandle* ih, const char* value)
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
    Eina_Rect geom = efl_gfx_entity_geometry_get(widget);

    bg_rect = efl_add(EFL_CANVAS_RECTANGLE_CLASS, widget);
    iupAttribSet(ih, "_IUP_EFL_BGRECT", (char*)bg_rect);
    efl_event_callback_add(bg_rect, EFL_EVENT_DEL, eflBgRectDelCallback, ih);

    /* set after the layout ran there is no later update to place it, follow the widget now */
    if (geom.w > 0 && geom.h > 0)
    {
      iupeflAttachToContainer(ih, bg_rect);
      iupeflSetPosition(bg_rect, geom.x, geom.y);
      iupeflSetSize(bg_rect, geom.w, geom.h);
      efl_gfx_stack_below(bg_rect, widget);
      efl_gfx_entity_visible_set(bg_rect, efl_gfx_entity_visible_get(widget));
    }
    else
      efl_gfx_entity_visible_set(bg_rect, EINA_TRUE);
  }

  efl_gfx_color_set(bg_rect, r, g, b, 255);

  return 1;
}

IUP_DRV_API int iupeflSetFgColorAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 1;
}

/****************************************************************************
 * Widget Visibility
 ****************************************************************************/

IUP_DRV_API int iupeflBaseSetVisibleAttrib(Ihandle* ih, const char* value)
{
  Eo* widget = iupeflGetWidget(ih);
  Eo* bg_rect;
  Eina_Bool visible;

  if (!widget)
    return 0;

  visible = iupStrBoolean(value) ? EINA_TRUE : EINA_FALSE;
  iupeflSetVisible(widget, visible);

  bg_rect = (Eo*)iupAttribGet(ih, "_IUP_EFL_BGRECT");
  if (bg_rect)
    iupeflSetVisible(bg_rect, visible);

  return 0;
}

/****************************************************************************
 * Widget Active/Enabled State
 ****************************************************************************/

IUP_DRV_API int iupeflBaseSetActiveAttrib(Ihandle* ih, const char* value)
{
  Eo* widget = iupeflGetWidget(ih);

  if (!widget)
    return 0;

  iupeflSetDisabled(widget, iupStrBoolean(value) ? EINA_FALSE : EINA_TRUE);

  return 0;
}

IUP_DRV_API char* iupeflBaseGetActiveAttrib(Ihandle* ih)
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

IUP_DRV_API void iupeflGetOrigin(Ihandle* ih, int* x, int* y)
{
  Ihandle* parent;
  int abs_x = 0, abs_y = 0;

  /* a tab page child carries the container itself, ancestors carry it only for deeper children */
  Eo* own_box = (Eo*)iupAttribGet(ih, "_IUPTAB_CONTAINER");
  if (own_box)
  {
    Eina_Rect geom = efl_gfx_entity_geometry_get(own_box);
    *x = geom.x;
    *y = geom.y;
    return;
  }

  for (parent = ih->parent; parent; parent = parent->parent)
  {
    Eo* content_box = (Eo*)iupAttribGet(parent, "_IUPTAB_CONTAINER");
    if (content_box)
    {
      Eina_Rect geom = efl_gfx_entity_geometry_get(content_box);
      abs_x += geom.x;
      abs_y += geom.y;
      break;
    }

    if (parent->iclass->nativetype != IUP_TYPEVOID)
    {
      abs_x += parent->x;
      abs_y += parent->y;
    }
  }

  *x = abs_x;
  *y = abs_y;
}

IUP_DRV_API Eo* iupeflGetContainer(Ihandle* ih)
{
  Ihandle* parent;
  Eo* container = NULL;

  for (parent = ih->parent; parent; parent = parent->parent)
  {
    Eo* content_box = (Eo*)iupAttribGet(parent, "_IUPTAB_CONTAINER");
    if (content_box)
      return content_box;

    if (parent->iclass->nativetype == IUP_TYPEVOID || parent->iclass->nativetype == IUP_TYPECANVAS)
      continue;

    container = (Eo*)iupClassObjectGetInnerNativeContainerHandle(parent, ih);
    if (!container)
      container = (Eo*)parent->handle;
    break;
  }

  if (!container || !efl_isa(container, EFL_CANVAS_GROUP_CLASS) || efl_isa(container, EFL_UI_WIN_CLASS))
    return NULL;

  return container;
}

IUP_DRV_API void iupeflAttachToContainer(Ihandle* ih, Eo* obj)
{
  Eo* container;

  if (!obj || evas_object_smart_parent_get(obj))
    return;

  container = iupeflGetContainer(ih);
  if (!container || container == obj)
    return;

  evas_object_smart_member_add(obj, container);
}

IUP_DRV_API void iupeflSetPosSize(Ihandle* ih, int x, int y, int width, int height)
{
  Eo* widget = (Eo*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  Eo* bg_rect;
  int abs_x, abs_y;

  if (!widget)
    widget = iupeflGetWidget(ih);

  if (!widget)
    return;

  iupeflAttachToContainer(ih, widget);

  iupeflGetOrigin(ih, &abs_x, &abs_y);
  abs_x += x;
  abs_y += y;

  iupeflSetPosition(widget, abs_x, abs_y);
  iupeflSetSize(widget, width, height);

  bg_rect = (Eo*)iupAttribGet(ih, "_IUP_EFL_BGRECT");
  if (bg_rect)
  {
    iupeflAttachToContainer(ih, bg_rect);
    efl_gfx_entity_position_set(bg_rect, EINA_POSITION2D(abs_x, abs_y));
    efl_gfx_entity_size_set(bg_rect, EINA_SIZE2D(width, height));
    efl_gfx_stack_below(bg_rect, widget);
  }

  {
    Ihandle* p = ih->parent;
    while (p)
    {
      Eo* clip = (Eo*)iupAttribGet(p, "_IUP_EFL_CANVAS_CLIP");
      if (clip)
      {
        evas_object_clip_set(widget, clip);
        if (bg_rect)
          evas_object_clip_set(bg_rect, clip);
        break;
      }
      if (p->iclass->nativetype != IUP_TYPEVOID)
        break;
      p = p->parent;
    }
  }
}

/****************************************************************************
 * Parent/Child Management
 ****************************************************************************/

IUP_DRV_API void iupeflAddToParent(Ihandle* ih)
{
  Eo* widget = iupeflGetWidget(ih);

  if (!ih->parent || !widget)
    return;
}

IUP_DRV_API Eo* iupeflGetInnerContainer(Ihandle* ih)
{
  char* inner = iupAttribGet(ih, "_IUP_EFL_INNER");
  if (inner)
    return (Eo*)inner;

  return NULL;
}

IUP_DRV_API Eo* iupeflGetParentWidget(Ihandle* ih)
{
  return (Eo*)iupChildTreeGetNativeParentHandle(ih);
}

static Eo* efl_main_window = NULL;

IUP_DRV_API void iupeflSetMainWindow(Eo* win)
{
  efl_main_window = win;
}

IUP_DRV_API Eo* iupeflGetMainWindow(void)
{
  return efl_main_window;
}

IUP_DRV_API unsigned int iupeflGetDefaultSeat(Eo* widget)
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
 *
 ****************************************************************************/

static void eflFixedGroupMemberAdd(Eo* obj, void* pd, Eo* member)
{
  (void)pd;
  efl_canvas_group_member_add(efl_super(obj, EFL_UI_WIDGET_CLASS), member);
}

static Eo* eflFixedFinalize(Eo* obj, void* pd)
{
  (void)pd;
  obj = efl_finalize(efl_super(obj, iupefl_fixed_class_get()));
  if (obj)
    efl_ui_widget_focus_allow_set(obj, EINA_FALSE);
  return obj;
}

static Eina_Bool eflFixedClassInitializer(Efl_Class* klass)
{
  EFL_OPS_DEFINE(ops,
                 EFL_OBJECT_OP_FUNC(efl_canvas_group_member_add, eflFixedGroupMemberAdd),
                 EFL_OBJECT_OP_FUNC(efl_finalize, eflFixedFinalize));
  return efl_class_functions_set(klass, &ops, NULL);
}

static const Efl_Class_Description _iup_fixed_class_desc = {
  EO_VERSION,
  "Iup.Fixed",
  EFL_CLASS_TYPE_REGULAR,
  0, eflFixedClassInitializer, NULL, NULL
};

EFL_DEFINE_CLASS(iupefl_fixed_class_get, &_iup_fixed_class_desc, EFL_UI_WIDGET_CLASS, NULL)

IUP_DRV_API Eo* iupeflFixedContainerNew(Eo* parent)
{
  Eo* fixed = efl_add(iupefl_fixed_class_get(), parent);
  if (!fixed)
    return NULL;

  efl_gfx_hint_weight_set(fixed, EFL_GFX_HINT_EXPAND, EFL_GFX_HINT_EXPAND);
  efl_gfx_hint_align_set(fixed, -1.0, -1.0);

  return fixed;
}

IUP_DRV_API void iupeflFixedContainerMove(Eo* container, Eo* child, int x, int y)
{
  (void)container;
  iupeflSetPosition(child, x, y);
}

IUP_DRV_API Eo* iupeflNativeContainerNew(Eo* parent)
{
  Eo* fixed = efl_add(iupefl_fixed_class_get(), parent);
  if (!fixed)
    return NULL;

  efl_gfx_hint_weight_set(fixed, EFL_GFX_HINT_EXPAND, EFL_GFX_HINT_EXPAND);
  efl_gfx_hint_align_set(fixed, -1.0, -1.0);

  return fixed;
}

/****************************************************************************
 * Mnemonic Handling
 ****************************************************************************/

IUP_DRV_API int iupeflSetMnemonicTitle(Ihandle* ih, Eo* widget, const char* value)
{
  char* str;
  char* efl_markup = NULL;

  if (!widget)
    return 0;

  if (!value)
    value = "";

  if (iupAttribGetBoolean(ih, "MARKUP"))
  {
    efl_markup = iupMarkupToEfl(value);

    if (efl_isa(widget, EFL_TEXT_MARKUP_INTERFACE))
      efl_text_markup_set(widget, efl_markup);
    else
      efl_text_markup_set(efl_part(widget, "efl.text"), efl_markup);

    free(efl_markup);
    return 1;
  }

  {
    char c = 0;
    str = iupStrProcessMnemonic(value, &c, -1);
    if (c)
      iupKeySetMnemonic(ih, c, -1);
  }

  if (efl_isa(widget, EFL_TEXT_INTERFACE))
  {
    efl_text_set(widget, str);
  }
  else
  {
    efl_markup = evas_textblock_text_utf8_to_markup(NULL, str);
    if (efl_markup)
    {
      elm_object_text_set(widget, efl_markup);
      free(efl_markup);
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

IUP_DRV_API void iupeflUpdateMnemonic(Ihandle* ih)
{
  (void)ih;
}

/****************************************************************************
 * Base Callbacks Registration
 ****************************************************************************/

static Eo* eflBaseKeyTarget(Eo* widget)
{
  if (efl_isa(widget, EFL_UI_TEXTBOX_CLASS))
  {
    Eo* text_obj = efl_text_cursor_object_text_object_get(efl_text_interactive_main_cursor_get(widget));
    if (text_obj)
      return text_obj;
  }
  return widget;
}

IUP_DRV_API void iupeflBaseAddCallbacks(Ihandle* ih, Eo* widget)
{
  Eo* key_target = eflBaseKeyTarget(widget);

  efl_event_callback_add(widget, EFL_EVENT_POINTER_IN, iupeflPointerInEvent, ih);
  efl_event_callback_add(widget, EFL_EVENT_POINTER_OUT, iupeflPointerOutEvent, ih);
  efl_event_callback_add(widget, EFL_EVENT_POINTER_DOWN, iupeflPointerDownEvent, ih);
  efl_event_callback_add(widget, EFL_EVENT_POINTER_UP, iupeflPointerUpEvent, ih);
  efl_event_callback_add(widget, EFL_EVENT_POINTER_MOVE, iupeflPointerMoveEvent, ih);
  efl_event_callback_add(widget, EFL_EVENT_POINTER_WHEEL, iupeflPointerWheelEvent, ih);
  iupeflKeySetTarget(key_target, ih);
  efl_event_callback_priority_add(key_target, EFL_EVENT_KEY_DOWN, EFL_CALLBACK_PRIORITY_BEFORE, iupeflKeyDownEvent, ih);
  efl_event_callback_priority_add(key_target, EFL_EVENT_KEY_UP, EFL_CALLBACK_PRIORITY_BEFORE, iupeflKeyUpEvent, ih);

  if (efl_isa(widget, EFL_UI_WIDGET_CLASS))
    efl_event_callback_add(widget, EFL_UI_FOCUS_OBJECT_EVENT_FOCUS_CHANGED, iupeflFocusChangedEvent, ih);

  if (efl_isa(widget, EFL_UI_FOCUS_COMPOSITION_MIXIN))
    efl_event_callback_add(widget, EFL_UI_FOCUS_OBJECT_EVENT_CHILD_FOCUS_CHANGED, iupeflChildFocusChangedEvent, ih);
}

IUP_DRV_API void iupeflBaseRemoveCallbacks(Ihandle* ih, Eo* widget)
{
  if (!widget)
    return;

  efl_event_callback_del(widget, EFL_EVENT_POINTER_IN, iupeflPointerInEvent, ih);
  efl_event_callback_del(widget, EFL_EVENT_POINTER_OUT, iupeflPointerOutEvent, ih);
  efl_event_callback_del(widget, EFL_EVENT_POINTER_DOWN, iupeflPointerDownEvent, ih);
  efl_event_callback_del(widget, EFL_EVENT_POINTER_UP, iupeflPointerUpEvent, ih);
  efl_event_callback_del(widget, EFL_EVENT_POINTER_MOVE, iupeflPointerMoveEvent, ih);
  efl_event_callback_del(widget, EFL_EVENT_POINTER_WHEEL, iupeflPointerWheelEvent, ih);
  efl_event_callback_del(eflBaseKeyTarget(widget), EFL_EVENT_KEY_DOWN, iupeflKeyDownEvent, ih);
  efl_event_callback_del(eflBaseKeyTarget(widget), EFL_EVENT_KEY_UP, iupeflKeyUpEvent, ih);

  iupeflKeyImfDestroy(ih);

  if (efl_isa(widget, EFL_UI_WIDGET_CLASS))
    efl_event_callback_del(widget, EFL_UI_FOCUS_OBJECT_EVENT_FOCUS_CHANGED, iupeflFocusChangedEvent, ih);

  if (efl_isa(widget, EFL_UI_FOCUS_COMPOSITION_MIXIN))
    efl_event_callback_del(widget, EFL_UI_FOCUS_OBJECT_EVENT_CHILD_FOCUS_CHANGED, iupeflChildFocusChangedEvent, ih);
}

/****************************************************************************
 * Cursor
 ****************************************************************************/

#ifdef HAVE_ECORE_X
#define EFL_XSHAPE(_s) ECORE_X_CURSOR_##_s
#else
#define EFL_XSHAPE(_s) 0
#endif

static const struct {
  const char* iupname;
  int hide;
  int xshape;
  const char* xname;
  const char* wlgroup;
} efl_cursors[] = {
  { "NONE",           1, 0,                               NULL,                  NULL },
  { "NULL",           1, 0,                               NULL,                  NULL },
  { "ARROW",          0, EFL_XSHAPE(LEFT_PTR),            "left_ptr",            "elm/pointer/base/default" },
  { "BUSY",           0, EFL_XSHAPE(WATCH),               "watch",               NULL },
  { "CROSS",          0, EFL_XSHAPE(CROSSHAIR),           "crosshair",           NULL },
  { "HAND",           0, EFL_XSHAPE(HAND2),               "hand2",               "elm/pointer/base/hand1" },
  { "HELP",           0, EFL_XSHAPE(QUESTION_ARROW),      "question_arrow",      NULL },
  { "IUP",            0, EFL_XSHAPE(QUESTION_ARROW),      "question_arrow",      NULL },
  { "MOVE",           0, EFL_XSHAPE(FLEUR),               "fleur",               NULL },
  { "PEN",            0, EFL_XSHAPE(PENCIL),              "pencil",              NULL },
  { "RESIZE_N",       0, EFL_XSHAPE(TOP_SIDE),            "top_side",            "elm/pointer/base/top_side" },
  { "RESIZE_S",       0, EFL_XSHAPE(BOTTOM_SIDE),         "bottom_side",         "elm/pointer/base/bottom_side" },
  { "RESIZE_NS",      0, EFL_XSHAPE(SB_V_DOUBLE_ARROW),   "sb_v_double_arrow",   NULL },
  { "SPLITTER_HORIZ", 0, EFL_XSHAPE(SB_V_DOUBLE_ARROW),   "sb_v_double_arrow",   NULL },
  { "RESIZE_W",       0, EFL_XSHAPE(LEFT_SIDE),           "left_side",           "elm/pointer/base/left_side" },
  { "RESIZE_E",       0, EFL_XSHAPE(RIGHT_SIDE),          "right_side",          "elm/pointer/base/right_side" },
  { "RESIZE_WE",      0, EFL_XSHAPE(SB_H_DOUBLE_ARROW),   "sb_h_double_arrow",   NULL },
  { "SPLITTER_VERT",  0, EFL_XSHAPE(SB_H_DOUBLE_ARROW),   "sb_h_double_arrow",   NULL },
  { "RESIZE_NE",      0, EFL_XSHAPE(TOP_RIGHT_CORNER),    "top_right_corner",    "elm/pointer/base/top_right_corner" },
  { "RESIZE_SE",      0, EFL_XSHAPE(BOTTOM_RIGHT_CORNER), "bottom_right_corner", "elm/pointer/base/bottom_right_corner" },
  { "RESIZE_NW",      0, EFL_XSHAPE(TOP_LEFT_CORNER),     "top_left_corner",     "elm/pointer/base/top_left_corner" },
  { "RESIZE_SW",      0, EFL_XSHAPE(BOTTOM_LEFT_CORNER),  "bottom_left_corner",  "elm/pointer/base/bottom_left_corner" },
  { "TEXT",           0, EFL_XSHAPE(XTERM),               "xterm",               "elm/pointer/base/xterm" },
  { "UPARROW",        0, EFL_XSHAPE(CENTER_PTR),          "center_ptr",          NULL }
};

#define EFL_CURSOR_COUNT ((int)(sizeof(efl_cursors) / sizeof(efl_cursors[0])))

static Ihandle* efl_cursor_hover_ih = NULL;
static Eo* efl_cursor_hover_obj = NULL;

static int eflCursorFindIndex(const char* name)
{
  int i;
  for (i = 0; i < EFL_CURSOR_COUNT; i++)
  {
    if (iupStrEqualNoCase(name, efl_cursors[i].iupname))
      return i;
  }
  return -1;
}

static unsigned int* eflCursorImagePixels(const char* name, int* w, int* h, int* hx, int* hy)
{
  Eo* img = (Eo*)iupImageGetCursor(name);
  Ihandle* image_ih = iupImageGetImageFromName(name);

  if (!img || !image_ih)
    return NULL;

  evas_object_image_size_get(img, w, h);
  *hx = 0;
  *hy = 0;
  iupStrToIntInt(iupAttribGet(image_ih, "HOTSPOT"), hx, hy, ':');
  return (unsigned int*)evas_object_image_data_get(img, EINA_FALSE);
}

#ifdef HAVE_ECORE_X
static void eflCursorApplyX11(Ecore_X_Window xwin, const char* name)
{
  static Ecore_X_Cursor shapes[EFL_CURSOR_COUNT];
  int i = name ? eflCursorFindIndex(name) : -1;

  if (i >= 0 && efl_cursors[i].hide)
  {
    ecore_x_window_cursor_set(xwin, 0);
    ecore_x_window_cursor_show(xwin, EINA_FALSE);
    return;
  }

  ecore_x_window_cursor_show(xwin, EINA_TRUE);

  if (i >= 0)
  {
    if (!shapes[i])
      shapes[i] = ecore_x_cursor_shape_get(efl_cursors[i].xshape);
    ecore_x_window_cursor_set(xwin, shapes[i]);
    return;
  }

  if (name)
  {
    Ihandle* image_ih = iupImageGetImageFromName(name);
    Ecore_X_Cursor cursor = image_ih ? (Ecore_X_Cursor)(uintptr_t)iupAttribGet(image_ih, "_IUPEFL_XCURSOR") : 0;
    if (!cursor)
    {
      int w, h, hx, hy;
      unsigned int* pixels = eflCursorImagePixels(name, &w, &h, &hx, &hy);
      if (pixels)
      {
        cursor = ecore_x_cursor_new(xwin, (int*)pixels, w, h, hx, hy);
        iupAttribSet(image_ih, "_IUPEFL_XCURSOR", (char*)(uintptr_t)cursor);
      }
    }
    if (cursor)
    {
      ecore_x_window_cursor_set(xwin, cursor);
      return;
    }
  }

  ecore_x_window_cursor_set(xwin, 0);
}
#endif

#ifdef HAVE_ECORE_WL2
static void eflCursorWlPointerSet(Ecore_Wl2_Display* display, struct wl_surface* surface, int hx, int hy)
{
  Eina_Iterator* it = ecore_wl2_display_inputs_get(display);
  Ecore_Wl2_Input* input;

  if (!it)
    return;

  EINA_ITERATOR_FOREACH(it, input)
    ecore_wl2_input_pointer_set(input, surface, hx, hy);
  eina_iterator_free(it);
}

static Ecore_Evas* efl_cursor_wl_ee = NULL;
static Evas_Object* efl_cursor_wl_edje = NULL;
static Evas_Object* efl_cursor_wl_image = NULL;
static Ecore_Wl2_Display* efl_cursor_wl_display = NULL;
static int efl_cursor_wl_hx = 0, efl_cursor_wl_hy = 0;
static int efl_cursor_wl_rendered = 0, efl_cursor_wl_pending = 0;

static void eflCursorWlShow(void)
{
  efl_cursor_wl_pending = 0;
  eflCursorWlPointerSet(efl_cursor_wl_display, ecore_wl2_window_surface_get(ecore_evas_wayland2_window_get(efl_cursor_wl_ee)), efl_cursor_wl_hx, efl_cursor_wl_hy);
}

static void eflCursorWlRenderPost(void* data, Evas* e, void* event_info)
{
  (void)data;
  (void)e;
  (void)event_info;
  efl_cursor_wl_rendered = 1;
  if (efl_cursor_wl_pending)
    eflCursorWlShow();
}

static void eflCursorWlCreate(void)
{
  Evas* evas;

  efl_cursor_wl_ee = ecore_evas_wayland_shm_new(NULL, 0, 0, 0, 32, 32, 0);
  if (!efl_cursor_wl_ee)
    return;

  ecore_evas_alpha_set(efl_cursor_wl_ee, EINA_TRUE);
  ecore_wl2_window_type_set(ecore_evas_wayland2_window_get(efl_cursor_wl_ee), ECORE_WL2_WINDOW_TYPE_NONE);
  evas = ecore_evas_get(efl_cursor_wl_ee);
  evas_event_callback_add(evas, EVAS_CALLBACK_RENDER_POST, eflCursorWlRenderPost, NULL);
  efl_cursor_wl_edje = edje_object_add(evas);
  efl_cursor_wl_image = evas_object_image_filled_add(evas);
  evas_object_image_alpha_set(efl_cursor_wl_image, EINA_TRUE);
  ecore_evas_show(efl_cursor_wl_ee);
}

static void eflCursorApplyWayland(Ecore_Wl2_Window* wl_win, const char* name)
{
  Ecore_Wl2_Display* display = ecore_wl2_window_display_get(wl_win);
  unsigned int* pixels = NULL;
  const char* group = NULL;
  const char* file = NULL;
  char theme_group[128];
  int i, w = 0, h = 0, hx = 0, hy = 0;

  if (!display)
    return;

  i = name ? eflCursorFindIndex(name) : -1;
  if (i >= 0 && efl_cursors[i].hide)
  {
    eflCursorWlPointerSet(display, NULL, 0, 0);
    return;
  }

  if (!efl_cursor_wl_ee)
    eflCursorWlCreate();
  if (!efl_cursor_wl_ee)
    return;

  if (name)
  {
    snprintf(theme_group, sizeof(theme_group), "elm/cursor/%s/default", i >= 0 ? efl_cursors[i].xname : name);
    group = theme_group;
    file = elm_theme_group_path_find(NULL, group);
  }

  if (!file && i < 0 && name)
    pixels = eflCursorImagePixels(name, &w, &h, &hx, &hy);

  if (pixels)
  {
    evas_object_image_size_set(efl_cursor_wl_image, w, h);
    evas_object_image_data_copy_set(efl_cursor_wl_image, pixels);
    evas_object_image_data_update_add(efl_cursor_wl_image, 0, 0, w, h);
    evas_object_geometry_set(efl_cursor_wl_image, 0, 0, w, h);
    evas_object_hide(efl_cursor_wl_edje);
    evas_object_show(efl_cursor_wl_image);
  }
  else
  {
    if (!file && i >= 0 && efl_cursors[i].wlgroup)
    {
      group = efl_cursors[i].wlgroup;
      file = elm_theme_group_path_find(NULL, group);
    }
    if (!file)
    {
      group = "elm/pointer/base/default";
      file = elm_theme_group_path_find(NULL, group);
    }
    if (!file || !edje_object_file_set(efl_cursor_wl_edje, file, group))
      return;

    edje_object_size_min_get(efl_cursor_wl_edje, &w, &h);
    if (w < 32 || h < 32)
    {
      w = 32;
      h = 32;
    }
    evas_object_geometry_set(efl_cursor_wl_edje, 0, 0, w, h);
    edje_object_calc_force(efl_cursor_wl_edje);
    edje_object_part_geometry_get(efl_cursor_wl_edje, "elm.swallow.hotspot", &hx, &hy, NULL, NULL);
    evas_object_hide(efl_cursor_wl_image);
    evas_object_show(efl_cursor_wl_edje);
  }

  efl_cursor_wl_display = display;
  efl_cursor_wl_hx = hx;
  efl_cursor_wl_hy = hy;
  efl_cursor_wl_pending = 1;

  ecore_evas_resize(efl_cursor_wl_ee, w, h);
  ecore_evas_manual_render(efl_cursor_wl_ee);
  if (efl_cursor_wl_rendered && efl_cursor_wl_pending)
    eflCursorWlShow();
}
#endif

static int efl_cursor_applied = 0;

IUP_DRV_API void iupeflCursorInit(void)
{
#ifdef HAVE_ECORE_WL2
  if (iupeflIsWayland() && !efl_cursor_wl_ee)
    eflCursorWlCreate();
#endif
}

static void eflCursorApply(Ihandle* ih, const char* name)
{
  Eo* widget = iupeflGetWidget(ih);
  Ecore_Evas* ee;

  if (!widget || (!name && !efl_cursor_applied))
    return;

  efl_cursor_applied = name ? 1 : 0;

  ee = ecore_evas_ecore_evas_get(evas_object_evas_get(widget));
  if (!ee)
    return;

#ifdef HAVE_ECORE_X
  if (iupeflIsX11())
  {
    Ecore_X_Window xwin = (Ecore_X_Window)ecore_evas_window_get(ee);
    if (xwin)
      eflCursorApplyX11(xwin, name);
  }
#endif

#ifdef HAVE_ECORE_WL2
  if (iupeflIsWayland())
  {
    Ecore_Wl2_Window* wl_win = ecore_evas_wayland2_window_get(ee);
    if (wl_win)
      eflCursorApplyWayland(wl_win, name);
  }
#endif
}

static const char* eflCursorFind(Ihandle* ih, Ihandle* changed, const char* value)
{
  for (; ih; ih = ih->parent)
  {
    const char* cursor = (ih == changed) ? value : iupAttribGet(ih, "CURSOR");
    if (cursor)
      return cursor;
  }
  return NULL;
}

static Ecore_Job* efl_cursor_job = NULL;

static void eflCursorApplyJob(void* data)
{
  (void)data;
  efl_cursor_job = NULL;
  if (efl_cursor_hover_obj)
    eflCursorApply(efl_cursor_hover_ih, eflCursorFind(efl_cursor_hover_ih, NULL, NULL));
}

static int eflCursorIsAncestor(Ihandle* ancestor, Ihandle* ih)
{
  for (; ih; ih = ih->parent)
  {
    if (ih == ancestor)
      return 1;
  }
  return 0;
}

static void eflCursorSetHover(Ihandle* ih, Eo* obj)
{
  if (efl_cursor_hover_obj)
    efl_wref_del(efl_cursor_hover_obj, &efl_cursor_hover_obj);

  efl_cursor_hover_ih = ih;
  efl_cursor_hover_obj = obj;

  if (obj)
    efl_wref_add(obj, &efl_cursor_hover_obj);
}

IUP_SDK_API int iupdrvBaseSetCursorAttrib(Ihandle* ih, const char* value)
{
  if (efl_cursor_hover_obj && eflCursorIsAncestor(ih, efl_cursor_hover_ih))
    eflCursorApply(efl_cursor_hover_ih, eflCursorFind(efl_cursor_hover_ih, ih, value));
  return 1;
}

/****************************************************************************
 * Event Handlers
 ****************************************************************************/

IUP_DRV_API void iupeflPointerDownEvent(void* data, const Efl_Event* ev)
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

IUP_DRV_API void iupeflPointerUpEvent(void* data, const Efl_Event* ev)
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

IUP_DRV_API void iupeflPointerMoveEvent(void* data, const Efl_Event* ev)
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

  {
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    unsigned int pressed_buttons;

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

    cb(ih, pos.x - widget_pos.x, pos.y - widget_pos.y, status);
  }
}

IUP_DRV_API void iupeflPointerWheelEvent(void* data, const Efl_Event* ev)
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

  {
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    Eina_Bool is_horizontal = efl_input_pointer_wheel_horizontal_get(pointer);
    int wheel_delta = efl_input_pointer_wheel_delta_get(pointer);
    float delta = is_horizontal ? 0.0f : (float)-wheel_delta;

    if (efl_input_modifier_enabled_get(pointer, EFL_INPUT_MODIFIER_SHIFT, NULL))
      iupKEY_SETSHIFT(status);
    if (efl_input_modifier_enabled_get(pointer, EFL_INPUT_MODIFIER_CONTROL, NULL))
      iupKEY_SETCONTROL(status);
    if (efl_input_modifier_enabled_get(pointer, EFL_INPUT_MODIFIER_ALT, NULL))
      iupKEY_SETALT(status);
    if (efl_input_modifier_enabled_get(pointer, EFL_INPUT_MODIFIER_META, NULL) ||
        efl_input_modifier_enabled_get(pointer, EFL_INPUT_MODIFIER_SUPER, NULL))
      iupKEY_SETSYS(status);

    cb(ih, delta, pos.x - widget_pos.x, pos.y - widget_pos.y, status);
  }
}

IUP_DRV_API void iupeflPointerInEvent(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  IFn cb;

  if (!efl_cursor_hover_obj || !eflCursorIsAncestor(ih, efl_cursor_hover_ih))
    eflCursorSetHover(ih, ev->object);
  if (!efl_cursor_job)
    efl_cursor_job = ecore_job_add(eflCursorApplyJob, NULL);

  cb = (IFn)IupGetCallback(ih, "ENTERWINDOW_CB");
  if (cb)
    cb(ih);
}

IUP_DRV_API void iupeflPointerOutEvent(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  IFn cb;

  if (efl_cursor_hover_obj == ev->object)
  {
    Ihandle* parent = ih->parent;
    Eo* parent_obj = NULL;
    while (parent && (parent->iclass->nativetype == IUP_TYPEVOID || !(parent_obj = iupeflGetWidget(parent))))
      parent = parent->parent;
    eflCursorSetHover(parent, parent_obj);
    if (parent_obj)
      eflCursorApply(parent, eflCursorFind(parent, NULL, NULL));
  }

  cb = (IFn)IupGetCallback(ih, "LEAVEWINDOW_CB");
  if (cb)
    cb(ih);
}

/****************************************************************************
 * Driver Interface
 ****************************************************************************/

IUP_SDK_API void iupdrvSetActive(Ihandle* ih, int enable)
{
  Eo* widget = iupeflGetWidget(ih);
  if (!widget)
    return;

  if (efl_isa(widget, EFL_UI_WIN_CLASS))
  {
    Evas* evas = evas_object_evas_get(widget);
    iupAttribSet(ih, "_IUPEFL_INACTIVE", enable ? NULL : "1");
    if (!enable)
    {
      Eo* blocker = (Eo*)iupAttribGet(ih, "_IUP_EFL_MODAL_BLOCKER");
      if (!blocker)
      {
        Eo* inner = (Eo*)iupAttribGet(ih, "_IUP_EFL_INNER");
        Evas_Object* parent = inner ? evas_object_smart_parent_get(inner) : NULL;
        if (parent && evas)
        {
          blocker = efl_add(EFL_CANVAS_RECTANGLE_CLASS, evas);
          efl_gfx_color_set(blocker, 0, 0, 0, 64);
          evas_object_smart_member_add(blocker, parent);
          efl_gfx_entity_size_set(blocker, EINA_SIZE2D(ih->currentwidth, ih->currentheight));
          efl_gfx_entity_position_set(blocker, EINA_POSITION2D(0, 0));
          efl_gfx_stack_above(blocker, inner);
          efl_gfx_entity_visible_set(blocker, EINA_TRUE);
          iupAttribSet(ih, "_IUP_EFL_MODAL_BLOCKER", (char*)blocker);
        }
      }
      if (evas)
        evas_event_freeze(evas);
    }
    else
    {
      if (evas)
        evas_event_thaw(evas);
      {
        Eo* blocker = (Eo*)iupAttribGet(ih, "_IUP_EFL_MODAL_BLOCKER");
        if (blocker)
        {
          efl_del(blocker);
          iupAttribSet(ih, "_IUP_EFL_MODAL_BLOCKER", NULL);
        }
      }
    }
    return;
  }

  if (efl_isa(widget, EFL_UI_WIDGET_CLASS))
    iupeflSetDisabled(widget, enable ? EINA_FALSE : EINA_TRUE);
  else
  {
    /* no native disabled state; track it for iupdrvIsActive and ignore pointer input */
    iupAttribSet(ih, "_IUPEFL_INACTIVE", enable ? NULL : "1");
    evas_object_pass_events_set(widget, enable ? EINA_FALSE : EINA_TRUE);
  }
}

IUP_SDK_API int iupdrvIsActive(Ihandle* ih)
{
  Eo* widget = iupeflGetWidget(ih);
  if (!widget)
    return 1;

  if (!efl_isa(widget, EFL_UI_WIDGET_CLASS) || efl_isa(widget, EFL_UI_WIN_CLASS))
    return iupAttribGet(ih, "_IUPEFL_INACTIVE") ? 0 : 1;

  return !iupeflGetDisabled(widget);
}

IUP_SDK_API void iupdrvSetVisible(Ihandle* ih, int enable)
{
  Eo* widget = iupeflGetWidget(ih);
  Eo* container = (Eo*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  Eo* bg_rect = (Eo*)iupAttribGet(ih, "_IUP_EFL_BGRECT");

  iupAttribSet(ih, "_IUPEFL_HIDDEN", enable ? NULL : "1");

  if (container)
    iupeflSetVisible(container, enable ? EINA_TRUE : EINA_FALSE);

  if (widget && widget != (Eo*)-1)
    iupeflSetVisible(widget, enable ? EINA_TRUE : EINA_FALSE);

  if (bg_rect)
    iupeflSetVisible(bg_rect, enable ? EINA_TRUE : EINA_FALSE);

  if (ih->iclass->nativetype == IUP_TYPECANVAS)
    iupeflCanvasSetScrollBarsVisible(ih, enable);
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

IUP_SDK_API void iupdrvReparent(Ihandle* ih)
{
  Eo* new_parent = (Eo*)iupChildTreeGetNativeParentHandle(ih);
  Eo* widget = (Eo*)iupAttribGet(ih, "_IUP_EXTRAPARENT");

  if (!widget)
    widget = iupeflGetWidget(ih);

  if (!widget || !new_parent || efl_parent_get(widget) == new_parent)
    return;

  /* an Evas object cannot move to another canvas, so another window needs a new widget */
  if (evas_object_evas_get(widget) != evas_object_evas_get(new_parent))
  {
    int old_visible = IupGetInt(ih, "VISIBLE");

    if (old_visible)
      IupSetAttribute(ih, "VISIBLE", "NO");

    IupUnmap(ih);
    IupMap(ih);

    if (old_visible)
      IupSetAttribute(ih, "VISIBLE", "YES");

    return;
  }

  {
    Eina_Bool visible = efl_gfx_entity_visible_get(widget);

    if (efl_isa(widget, EFL_UI_WIDGET_CLASS) && efl_isa(new_parent, EFL_UI_WIDGET_CLASS))
      efl_ui_widget_sub_object_add(new_parent, widget);
    efl_parent_set(widget, new_parent);
    evas_object_raise(widget);
    efl_gfx_entity_visible_set(widget, visible);
  }
}

IUP_SDK_API void iupdrvBaseLayoutUpdateMethod(Ihandle* ih)
{
  iupeflSetPosSize(ih, ih->x, ih->y, ih->currentwidth, ih->currentheight);
}

IUP_SDK_API void iupdrvBaseUnMapMethod(Ihandle* ih)
{
  Eo* widget = (Eo*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (!widget)
    widget = iupeflGetWidget(ih);

  if (widget)
  {
    iupeflBaseRemoveCallbacks(ih, widget);
    if (efl_parent_get(widget))
      efl_del(widget);
    else
      efl_unref(widget);
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
  Eo* check_widget = (Eo*)iupAttribGet(ih, "_IUP_EFL_CHECK");
  if (check_widget)
  {
    efl_ui_selectable_selected_set(check_widget, !efl_ui_selectable_selected_get(check_widget));
    return;
  }

  Eo* widget = iupeflGetWidget(ih);
  if (widget && efl_isa(widget, EFL_INPUT_CLICKABLE_MIXIN))
  {
    Efl_Input_Clickable_Clicked clicked = {0, 1};
    efl_event_callback_call(widget, EFL_INPUT_EVENT_CLICKED, &clicked);
  }
}

IUP_DRV_API int iupeflCanvasHasSize(Ihandle* ih)
{
  Eina_Size2D size = efl_gfx_entity_size_get(iupeflGetWidget(ih));
  return (size.w > 1 && size.h > 1);
}

static int eflWindowCanRender(Eo* win)
{
  if (!efl_gfx_entity_visible_get(win))
    return 0;

#ifdef HAVE_ECORE_WL2
  if (iupeflIsWayland())
  {
    Ecore_Evas* ee = ecore_evas_ecore_evas_get(evas_object_evas_get(win));
    Ecore_Wl2_Window* wl_win = ee ? ecore_evas_wayland2_window_get(ee) : NULL;
    if (wl_win && !ecore_wl2_window_shell_surface_exists(wl_win))
      return 0;
  }
#endif

  return 1;
}

IUP_SDK_API void iupdrvRedrawNow(Ihandle* ih)
{
  Ihandle* dialog;
  Eo* win;
  Eo* widget = iupeflGetWidget(ih);

  if (!widget)
    return;

  dialog = IupGetDialog(ih);
  win = dialog? iupeflGetWidget(dialog): NULL;

  /* Wayland rejects a buffer attached before the surface has a role */
  if (win && !eflWindowCanRender(win))
  {
    if (ih->iclass->nativetype == IUP_TYPECANVAS)
      iupeflRedrawSetPending(ih);
    return;
  }

  if (ih->iclass->nativetype == IUP_TYPECANVAS && iupeflCanvasHasSize(ih))
  {
    IFn cb = (IFn)IupGetCallback(ih, "ACTION");
    iupeflRedrawClearPending(ih);
    if (cb)
      cb(ih);
  }

  {
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
      iupeflRedrawSetPending(ih);

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
  iupClassRegisterAttribute(ic, "TIPICON", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "TIPMARKUP", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TIPRECT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "ACCESSIBLETITLE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "ACCESSIBLEDESCRIPTION", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_DEFAULT);
}

IUP_SDK_API void iupdrvSetAccessibleTitle(Ihandle* ih, const char* title)
{
  (void)ih;
  (void)title;
}

IUP_SDK_API void iupdrvSetAccessibleDescription(Ihandle* ih, const char* description)
{
  (void)ih;
  (void)description;
}

static void eflGetWidgetScreenPos(Ihandle* ih, int* widget_x, int* widget_y)
{
  Eo* widget = iupeflGetWidget(ih);
  Eo* win;

  *widget_x = 0;
  *widget_y = 0;

  if (!widget)
    return;

  {
    Eina_Rect geometry = iupeflGetGeometry(widget);
    *widget_x = geometry.x;
    *widget_y = geometry.y;
  }

  win = iupeflGetMainWindow();
  if (win)
  {
    Evas* evas = evas_object_evas_get(win);
    if (evas)
    {
      Ecore_Evas* ee = ecore_evas_ecore_evas_get(evas);
      if (ee)
      {
        int screen_x = 0, screen_y = 0;
        ecore_evas_geometry_get(ee, &screen_x, &screen_y, NULL, NULL);
        *widget_x += screen_x;
        *widget_y += screen_y;
      }
    }
  }
}

IUP_SDK_API void iupdrvClientToScreen(Ihandle* ih, int* x, int* y)
{
  int wx, wy;
  eflGetWidgetScreenPos(ih, &wx, &wy);
  if (x) *x += wx;
  if (y) *y += wy;
}

IUP_SDK_API void iupdrvScreenToClient(Ihandle* ih, int* x, int* y)
{
  int wx, wy;
  eflGetWidgetScreenPos(ih, &wx, &wy);
  if (x) *x -= wx;
  if (y) *y -= wy;
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

IUP_SDK_API void iupdrvSleep(int time)
{
#ifdef _WIN32
  Sleep(time);
#else
  usleep((useconds_t)(time * 1000));
#endif
}

IUP_SDK_API unsigned int iupdrvGetTickCount(void)
{
  return (unsigned int)(unsigned long long)(ecore_time_get() * 1000.0);
}
