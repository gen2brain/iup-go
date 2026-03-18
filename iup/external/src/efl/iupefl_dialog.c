/** \file
 * \brief IupDialog class for EFL
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef HAVE_ECORE_X
#include <Ecore_X.h>
#endif

#ifdef HAVE_ECORE_WL2
#include <Ecore_Wl2.h>
#endif

#ifdef HAVE_ECORE_WIN32
#include <Ecore_Win32.h>
#include <windows.h>
#endif

#ifdef HAVE_ECORE_COCOA
#include <Ecore_Cocoa.h>
#include <objc/runtime.h>
#include <objc/message.h>
#endif

#include <Ecore_Evas.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_class.h"
#include "iup_object.h"
#include "iup_layout.h"
#include "iup_dlglist.h"
#include "iup_attrib.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_drvinfo.h"
#include "iup_focus.h"
#include "iup_str.h"
#define _IUPDLG_PRIVATE
#include "iup_dialog.h"
#include "iup_image.h"

#include "iupefl_drv.h"


/****************************************************************
                     Callbacks
****************************************************************/

static void eflDialogCloseCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Icallback cb;

  (void)ev;

  if (!iupObjectCheck(ih))
    return;

  cb = IupGetCallback(ih, "CLOSE_CB");
  if (cb)
  {
    int ret = cb(ih);
    if (ret == IUP_IGNORE)
      return;
    if (ret == IUP_CLOSE)
      IupExitLoop();
  }

  IupHide(ih);
}

static void eflDialogStackBgBelow(Ihandle* ih, Eo* bg)
{
  Eo* inner = (Eo*)iupAttribGet(ih, "_IUP_EFL_INNER");
  if (inner)
  {
    Evas_Object* parent = evas_object_smart_parent_get(inner);
    if (parent)
      evas_object_smart_member_add(bg, parent);
    efl_gfx_stack_below(bg, inner);
  }
}

static void eflDialogFreeCanvasImage(Eo* img)
{
  if (!img)
    return;
  efl_unref(img);
}

static void eflDialogResizeCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Eina_Rect geometry;
  int w, h;
  IFnii cb;

  if (!iupObjectCheck(ih))
    return;

  geometry = iupeflGetGeometry(ev->object);
  w = geometry.w;
  h = geometry.h;

  if (ih->data->ignore_resize)
    return;

  if (w != ih->currentwidth || h != ih->currentheight)
  {
    ih->currentwidth = w;
    ih->currentheight = h;

    cb = (IFnii)IupGetCallback(ih, "RESIZE_CB");
    if (!cb || cb(ih, w, h) != IUP_IGNORE)
    {
      ih->data->ignore_resize = 1;
      IupRefresh(ih);
      ih->data->ignore_resize = 0;
    }
  }

  {
    Eo* bg_rect = (Eo*)iupAttribGet(ih, "_IUP_EFL_BGRECT");
    Eo* bg_img = (Eo*)iupAttribGet(ih, "_IUP_EFL_BACKGROUND_IMAGE");
    if (bg_rect)
    {
      efl_gfx_entity_size_set(bg_rect, EINA_SIZE2D(w, h));
      efl_gfx_entity_position_set(bg_rect, EINA_POSITION2D(0, 0));
      eflDialogStackBgBelow(ih, bg_rect);
    }
    if (bg_img)
    {
      efl_gfx_entity_size_set(bg_img, EINA_SIZE2D(w, h));
      efl_gfx_entity_position_set(bg_img, EINA_POSITION2D(0, 0));
      if (iupAttribGetBoolean(ih, "BACKIMAGEZOOM"))
        evas_object_image_fill_set(bg_img, 0, 0, w, h);
      eflDialogStackBgBelow(ih, bg_img);
    }
  }
}

static void eflDialogThemeChangedCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  int dark_mode;
  IFni cb;

  (void)ev;

  if (!iupObjectCheck(ih))
    return;

  iupeflSetGlobalColors();

  dark_mode = iupeflIsSystemDarkMode();

  cb = (IFni)IupGetCallback(ih, "THEMECHANGED_CB");
  if (cb)
    cb(ih, dark_mode);
}

static void eflDialogKeyDownCallback(void* data, const Efl_Event* event)
{
  Ihandle* ih = (Ihandle*)data;
  Eo* ev = event->info;
  const char* str;
  const char* keyname;

  if (!iupObjectCheck(ih))
    return;

  keyname = efl_input_key_name_get(ev);
  if (keyname)
  {
    if (strcmp(keyname, "Return") == 0 || strcmp(keyname, "KP_Enter") == 0)
    {
      Ihandle* bt = IupGetAttributeHandle(ih, "DEFAULTENTER");
      if (iupObjectCheck(bt))
      {
        iupdrvActivate(bt);
        return;
      }
    }
    else if (strcmp(keyname, "Escape") == 0)
    {
      Ihandle* bt = IupGetAttributeHandle(ih, "DEFAULTESC");
      if (iupObjectCheck(bt))
      {
        iupdrvActivate(bt);
        return;
      }
    }
  }

  if (efl_input_modifier_enabled_get(ev, EFL_INPUT_MODIFIER_ALT, NULL))
  {
    str = efl_input_key_string_get(ev);
    if (str && str[0] && !str[1])
    {
      char key = str[0];
      Elm_Object_Item* item = iupeflMenuFindMnemonic(ih, key);
      if (item)
        elm_menu_item_selected_set(item, EINA_TRUE);
    }
  }
}

static int eflDialogGetMenuSize(Ihandle* ih)
{
  if (ih->data->menu)
    return iupdrvMenuGetMenuBarSize(ih->data->menu);
  return 0;
}

#ifdef HAVE_ECORE_X
#define ECORE_X_MWM_HINTS_FUNCTIONS   (1 << 0)
#define ECORE_X_MWM_HINTS_DECORATIONS (1 << 1)

#define ECORE_X_MWM_HINT_FUNC_ALL      (1 << 0)
#define ECORE_X_MWM_HINT_FUNC_RESIZE   (1 << 1)
#define ECORE_X_MWM_HINT_FUNC_MOVE     (1 << 2)
#define ECORE_X_MWM_HINT_FUNC_MINIMIZE (1 << 3)
#define ECORE_X_MWM_HINT_FUNC_MAXIMIZE (1 << 4)
#define ECORE_X_MWM_HINT_FUNC_CLOSE    (1 << 5)

#define ECORE_X_MWM_HINT_DECOR_ALL      (1 << 0)
#define ECORE_X_MWM_HINT_DECOR_BORDER   (1 << 1)
#define ECORE_X_MWM_HINT_DECOR_RESIZEH  (1 << 2)
#define ECORE_X_MWM_HINT_DECOR_TITLE    (1 << 3)
#define ECORE_X_MWM_HINT_DECOR_MENU     (1 << 4)
#define ECORE_X_MWM_HINT_DECOR_MINIMIZE (1 << 5)
#define ECORE_X_MWM_HINT_DECOR_MAXIMIZE (1 << 6)

static void eflDialogSetMwmHints(Ihandle* ih, Eo* win)
{
  Ecore_X_Window xwin;
  unsigned int functions = 0;
  unsigned int decorations = 0;
  unsigned int data[5] = {0, 0, 0, 0, 0};

  if (!iupeflIsX11())
    return;

  xwin = elm_win_xwindow_get(win);
  if (!xwin)
    return;

  functions = ECORE_X_MWM_HINT_FUNC_MOVE;
  decorations = ECORE_X_MWM_HINT_DECOR_BORDER | ECORE_X_MWM_HINT_DECOR_TITLE;

  if (iupAttribGetBoolean(ih, "RESIZE"))
  {
    functions |= ECORE_X_MWM_HINT_FUNC_RESIZE;
    decorations |= ECORE_X_MWM_HINT_DECOR_RESIZEH;
  }

  if (iupAttribGetBoolean(ih, "MENUBOX"))
  {
    functions |= ECORE_X_MWM_HINT_FUNC_CLOSE;
    decorations |= ECORE_X_MWM_HINT_DECOR_MENU;
  }

  if (iupAttribGetBoolean(ih, "MAXBOX"))
  {
    functions |= ECORE_X_MWM_HINT_FUNC_MAXIMIZE;
    decorations |= ECORE_X_MWM_HINT_DECOR_MAXIMIZE;
  }

  if (iupAttribGetBoolean(ih, "MINBOX"))
  {
    functions |= ECORE_X_MWM_HINT_FUNC_MINIMIZE;
    decorations |= ECORE_X_MWM_HINT_DECOR_MINIMIZE;
  }

  data[0] = ECORE_X_MWM_HINTS_FUNCTIONS | ECORE_X_MWM_HINTS_DECORATIONS;
  data[1] = functions;
  data[2] = decorations;

  ecore_x_window_prop_property_set(xwin, ECORE_X_ATOM_MOTIF_WM_HINTS, ECORE_X_ATOM_MOTIF_WM_HINTS, 32, (void *)data, 5);
}
#endif

static void eflDialogParentDestroyCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  (void)ev;

  if (IupMainLoopLevel() > 0 && iupObjectCheck(ih))
    IupDestroy(ih);
}

void iupdrvDialogSetParent(Ihandle* ih, InativeHandle* parent)
{
  Eo* win = iupeflGetWidget(ih);
  Eo* parent_win = (Eo*)parent;

  if (!win || !parent_win)
    return;

#ifdef HAVE_ECORE_X
  if (iupeflIsX11())
  {
    Ecore_X_Window xwin = elm_win_xwindow_get(win);
    Ecore_X_Window parent_xwin = elm_win_xwindow_get(parent_win);

    if (xwin && parent_xwin)
      ecore_x_icccm_transient_for_set(xwin, parent_xwin);
  }
#endif

#ifdef HAVE_ECORE_WIN32
  {
    Ecore_Win32_Window* win32_win = elm_win_win32_window_get(win);
    Ecore_Win32_Window* win32_parent = elm_win_win32_window_get(parent_win);

    if (win32_win && win32_parent)
    {
      HWND hwnd = (HWND)ecore_win32_window_hwnd_get(win32_win);
      HWND hwnd_parent = (HWND)ecore_win32_window_hwnd_get(win32_parent);

      if (hwnd && hwnd_parent)
        SetWindowLongPtr(hwnd, GWLP_HWNDPARENT, (LONG_PTR)hwnd_parent);
    }
  }
#endif

#ifdef HAVE_ECORE_COCOA
  {
    Ecore_Cocoa_Window* cocoa_win = elm_win_cocoa_window_get(win);
    Ecore_Cocoa_Window* cocoa_parent = elm_win_cocoa_window_get(parent_win);

    if (cocoa_win && cocoa_parent)
    {
      Ecore_Cocoa_Object* nswin = ecore_cocoa_window_get(cocoa_win);
      Ecore_Cocoa_Object* nsparent = ecore_cocoa_window_get(cocoa_parent);

      if (nswin && nsparent)
      {
        /* [parent addChildWindow:child ordered:NSWindowAbove] via runtime */
        SEL sel = sel_registerName("addChildWindow:ordered:");
        ((void (*)(id, SEL, id, long))objc_msgSend)((id)nsparent, sel, (id)nswin, 1);
      }
    }
  }
#endif

  efl_event_callback_add(parent_win, EFL_EVENT_DEL, eflDialogParentDestroyCallback, ih);
}

int iupdrvDialogIsVisible(Ihandle* ih)
{
  Eo* win = iupeflGetWidget(ih);
  if (!win)
    return 0;
  return iupeflIsVisible(win);
}

void iupdrvDialogGetSize(Ihandle* ih, InativeHandle* handle, int* w, int* h)
{
  int width = 0, height = 0;

  if (!handle)
    handle = ih->handle;

  if (handle)
  {
    Eina_Rect geometry = iupeflGetGeometry((Eo*)handle);
    width = geometry.w;
    height = geometry.h;
  }

  if (w) *w = width;
  if (h) *h = height;
}

void iupdrvDialogSetVisible(Ihandle* ih, int visible)
{
  Eo* win = iupeflGetWidget(ih);

  if (!win)
    return;

  iupeflSetVisible(win, visible ? EINA_TRUE : EINA_FALSE);
}

void iupdrvDialogGetPosition(Ihandle* ih, InativeHandle* handle, int* x, int* y)
{
  int gx = 0, gy = 0;

  if (!handle)
    handle = ih->handle;

  if (handle)
  {
    Eina_Rect geometry = iupeflGetGeometry((Eo*)handle);
    gx = geometry.x;
    gy = geometry.y;
  }

  if (x) *x = gx;
  if (y) *y = gy;
}

void iupdrvDialogSetPosition(Ihandle* ih, int x, int y)
{
  Eo* win = iupeflGetWidget(ih);
  if (!win)
    return;

  iupeflSetPosition(win, x, y);
}

void iupdrvDialogGetDecoration(Ihandle* ih, int* border, int* caption, int* menu)
{
  /* EFL windows handle decorations internally.
     Only menu bar height needs to be tracked since IUP positions content below it. */
  *menu = eflDialogGetMenuSize(ih);
  *border = 0;
  *caption = 0;
}

int iupdrvDialogSetPlacement(Ihandle* ih)
{
  char* placement;
  int old_state = ih->data->show_state;
  Eo* win = iupeflGetWidget(ih);
  ih->data->show_state = IUP_SHOW;

  if (!win)
    return 0;

  if (iupAttribGetBoolean(ih, "FULLSCREEN"))
  {
    efl_ui_win_fullscreen_set(win, EINA_TRUE);
    return 1;
  }

  if (iupAttribGetBoolean(ih, "CUSTOMFRAME") && iupDialogCustomFrameRestore(ih))
  {
    ih->data->show_state = IUP_RESTORE;
    return 1;
  }

  placement = iupAttribGet(ih, "PLACEMENT");
  if (!placement)
  {
    if (old_state == IUP_MAXIMIZE || old_state == IUP_MINIMIZE)
      ih->data->show_state = IUP_RESTORE;

    efl_ui_win_maximized_set(win, EINA_FALSE);
    return 0;
  }

  if (iupStrEqualNoCase(placement, "MINIMIZED"))
  {
    ih->data->show_state = IUP_MINIMIZE;
    efl_ui_win_minimized_set(win, EINA_TRUE);
  }
  else if (iupStrEqualNoCase(placement, "MAXIMIZED"))
  {
    ih->data->show_state = IUP_MAXIMIZE;
    efl_ui_win_maximized_set(win, EINA_TRUE);
  }
  else if (iupStrEqualNoCase(placement, "FULL"))
  {
    int width, height, x, y;
    int border, caption, menu;
    iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

    x = -(border);
    y = -(border + caption + menu);

    iupdrvGetFullSize(&width, &height);
    height += menu;

    iupdrvDialogSetPosition(ih, x, y);
    iupeflSetSize(win, width, height);

    if (old_state == IUP_MAXIMIZE || old_state == IUP_MINIMIZE)
      ih->data->show_state = IUP_RESTORE;
  }

  iupAttribSet(ih, "PLACEMENT", NULL);
  return 1;
}

static void eflDialogSetChildrenPositionMethod(Ihandle* ih, int x, int y)
{
  if (ih->firstchild)
  {
    char* offset = iupAttribGet(ih, "CHILDOFFSET");

    x = 0;
    y = 0;

    if (offset) iupStrToIntInt(offset, &x, &y, 'x');

    y += eflDialogGetMenuSize(ih);

    iupBaseSetPosition(ih->firstchild, x, y);
  }
}

static void* eflDialogGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
  char* inner = iupAttribGet(ih, "_IUP_EFL_INNER");
  (void)child;

  if (inner)
    return (void*)inner;

  return ih->handle;
}

static void eflDialogSetMinMax(Ihandle* ih, int min_w, int min_h, int max_w, int max_h)
{
  Eo* win = iupeflGetWidget(ih);
  Ecore_Evas* ee;

  if (!win)
    return;

  ee = ecore_evas_object_ecore_evas_get(win);
  if (!ee)
    return;

  if (min_w < 1) min_w = 1;
  if (min_h < 1) min_h = 1;

  ecore_evas_size_min_set(ee, min_w, min_h);

  if (max_w > 0 && max_w < 65535 && max_h > 0 && max_h < 65535)
    ecore_evas_size_max_set(ee, max_w, max_h);
}

static int eflDialogSetMinSizeAttrib(Ihandle* ih, const char* value)
{
  int min_w = 1, min_h = 1;
  int max_w = 65535, max_h = 65535;
  iupStrToIntInt(value, &min_w, &min_h, 'x');

  iupStrToIntInt(iupAttribGet(ih, "MAXSIZE"), &max_w, &max_h, 'x');

  eflDialogSetMinMax(ih, min_w, min_h, max_w, max_h);

  return iupBaseSetMinSizeAttrib(ih, value);
}

static int eflDialogSetMaxSizeAttrib(Ihandle* ih, const char* value)
{
  int min_w = 1, min_h = 1;
  int max_w = 65535, max_h = 65535;
  iupStrToIntInt(value, &max_w, &max_h, 'x');

  iupStrToIntInt(iupAttribGet(ih, "MINSIZE"), &min_w, &min_h, 'x');

  eflDialogSetMinMax(ih, min_w, min_h, max_w, max_h);

  return iupBaseSetMaxSizeAttrib(ih, value);
}

static int eflDialogMapMethod(Ihandle* ih)
{
  Eo* win;
  Eo* parent;
  Eo* content_box;
  Efl_Ui_Win_Type win_type;
  char* title;
  int has_titlebar = 0;

  title = iupAttribGet(ih, "TITLE");
  parent = (Eo*)iupDialogGetNativeParent(ih);

  if (parent)
    win_type = EFL_UI_WIN_TYPE_DIALOG_BASIC;
  else
    win_type = EFL_UI_WIN_TYPE_BASIC;

  win = efl_add(EFL_UI_WIN_CLASS, parent ? parent : efl_main_loop_get(),
                efl_text_set(efl_added, title ? title : ""),
                efl_ui_win_type_set(efl_added, win_type));

  if (!win)
    return IUP_ERROR;

  ih->handle = (InativeHandle*)win;

  if (!iupeflGetMainWindow())
    iupeflSetMainWindow(win);

  efl_ui_win_autodel_set(win, EINA_FALSE);

  content_box = iupeflFixedContainerNew(win);
  if (content_box)
  {
    efl_content_set(win, content_box);
    efl_gfx_entity_visible_set(content_box, EINA_TRUE);
    iupAttribSet(ih, "_IUP_EFL_INNER", (char*)content_box);
  }
  else
  {
    iupAttribSet(ih, "_IUP_EFL_INNER", (char*)win);
  }

  efl_event_callback_add(win, EFL_UI_WIN_EVENT_DELETE_REQUEST, eflDialogCloseCallback, ih);
  efl_event_callback_add(win, EFL_GFX_ENTITY_EVENT_SIZE_CHANGED, eflDialogResizeCallback, ih);
  efl_event_callback_add(win, EFL_UI_WIN_EVENT_THEME_CHANGED, eflDialogThemeChangedCallback, ih);
  efl_event_callback_add(win, EFL_EVENT_KEY_DOWN, eflDialogKeyDownCallback, ih);

  if (iupAttribGet(ih, "TITLE"))
    has_titlebar = 1;

  if (!iupAttribGetBoolean(ih, "RESIZE"))
    iupAttribSet(ih, "MAXBOX", "NO");

  if (iupAttribGetBoolean(ih, "MENUBOX") ||
      iupAttribGetBoolean(ih, "MAXBOX") ||
      iupAttribGetBoolean(ih, "MINBOX"))
    has_titlebar = 1;

  if (iupAttribGetBoolean(ih, "CUSTOMFRAME"))
  {
    iupDialogCustomFrameSimulateCheckCallbacks(ih);
    has_titlebar = 0;
  }

  if (iupAttribGetBoolean(ih, "HIDETITLEBAR") || !has_titlebar)
  {
    efl_ui_win_borderless_set(win, EINA_TRUE);
  }
  else
  {
#ifdef HAVE_ECORE_X
    eflDialogSetMwmHints(ih, win);
#endif
  }

  if (parent)
    iupdrvDialogSetParent(ih, (InativeHandle*)parent);

  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSet(ih, "DROPFILESTARGET", "YES");

  eflDialogSetMinMax(ih, 1, 1, 65535, 65535);

  return IUP_NOERROR;
}

static void eflDialogUnMapMethod(Ihandle* ih)
{
  Eo* win = iupeflGetWidget(ih);

  if (ih->data->menu)
  {
    ih->data->menu->handle = NULL;
    IupDestroy(ih->data->menu);
    ih->data->menu = NULL;
  }

  if (win)
  {
    InativeHandle* parent = iupDialogGetNativeParent(ih);
    if (parent)
      efl_event_callback_del((Eo*)parent, EFL_EVENT_DEL, eflDialogParentDestroyCallback, ih);

    efl_event_callback_del(win, EFL_UI_WIN_EVENT_DELETE_REQUEST, eflDialogCloseCallback, ih);
    efl_event_callback_del(win, EFL_GFX_ENTITY_EVENT_SIZE_CHANGED, eflDialogResizeCallback, ih);
    efl_event_callback_del(win, EFL_UI_WIN_EVENT_THEME_CHANGED, eflDialogThemeChangedCallback, ih);
    efl_event_callback_del(win, EFL_EVENT_KEY_DOWN, eflDialogKeyDownCallback, ih);

    {
      Eo* bg_rect = (Eo*)iupAttribGet(ih, "_IUP_EFL_BGRECT");
      Eo* bg_img = (Eo*)iupAttribGet(ih, "_IUP_EFL_BACKGROUND_IMAGE");
      Eo* op_img = (Eo*)iupAttribGet(ih, "_IUP_EFL_OPACITY_IMAGE");
      if (bg_rect)
        efl_unref(bg_rect);
      if (bg_img)
        eflDialogFreeCanvasImage(bg_img);
      if (op_img)
        eflDialogFreeCanvasImage(op_img);
      iupAttribSet(ih, "_IUP_EFL_BGRECT", NULL);
      iupAttribSet(ih, "_IUP_EFL_BACKGROUND_IMAGE", NULL);
      iupAttribSet(ih, "_IUP_EFL_OPACITY_IMAGE", NULL);
    }

    efl_del(win);
  }

  ih->handle = NULL;

  iupeflFontFree(ih);
}

static void eflDialogDeferredRefresh(void* data)
{
  Ihandle* ih = (Ihandle*)data;
  if (!iupObjectCheck(ih))
    return;
  if (ih->data->ignore_resize)
    return;

  ih->data->ignore_resize = 1;
  IupRefresh(ih);
  ih->data->ignore_resize = 0;
}

static void eflDialogLayoutUpdateMethod(Ihandle* ih)
{
  int width, height;
  Eo* win = iupeflGetWidget(ih);
  Eo* bg;

  if (ih->data->ignore_resize)
    return;

  if (!win)
    return;

  ih->data->ignore_resize = 1;

  width = ih->currentwidth;
  height = ih->currentheight;

  if (width <= 0) width = 1;
  if (height <= 0) height = 1;

  iupeflSetSize(win, width, height);

  bg = (Eo*)iupAttribGet(ih, "_IUP_EFL_BGRECT");
  if (bg)
  {
    efl_gfx_entity_size_set(bg, EINA_SIZE2D(width, height));
    efl_gfx_entity_position_set(bg, EINA_POSITION2D(0, 0));
    eflDialogStackBgBelow(ih, bg);
  }

  bg = (Eo*)iupAttribGet(ih, "_IUP_EFL_BACKGROUND_IMAGE");
  if (bg)
  {
    efl_gfx_entity_size_set(bg, EINA_SIZE2D(width, height));
    efl_gfx_entity_position_set(bg, EINA_POSITION2D(0, 0));
    if (iupAttribGetBoolean(ih, "BACKIMAGEZOOM"))
      evas_object_image_fill_set(bg, 0, 0, width, height);
    eflDialogStackBgBelow(ih, bg);
  }

  if (!iupAttribGetBoolean(ih, "RESIZE"))
  {
    Ecore_Evas* ee = ecore_evas_object_ecore_evas_get(win);
    if (ee)
    {
      ecore_evas_size_min_set(ee, width, height);
      ecore_evas_size_max_set(ee, width, height);
    }
  }

  ih->data->ignore_resize = 0;

  if (!iupAttribGetBoolean(ih, "_IUP_EFL_FIRST_REFRESH"))
  {
    iupAttribSet(ih, "_IUP_EFL_FIRST_REFRESH", "1");
    ecore_job_add(eflDialogDeferredRefresh, ih);
  }
}

/****************************************************************
                     Attributes
****************************************************************/

static int eflDialogSetTitleAttrib(Ihandle* ih, const char* value)
{
  Eo* win = iupeflGetWidget(ih);
  if (!win)
    return 0;

  iupeflSetText(win, value ? value : "");
  return 0;
}

static char* eflDialogGetTitleAttrib(Ihandle* ih)
{
  Eo* win = iupeflGetWidget(ih);
  if (!win)
    return NULL;

  return (char*)iupeflGetText(win);
}


static int eflDialogSetFullScreenAttrib(Ihandle* ih, const char* value)
{
  Eo* win = iupeflGetWidget(ih);
  if (!win)
    return 0;

  efl_ui_win_fullscreen_set(win, iupStrBoolean(value) ? EINA_TRUE : EINA_FALSE);

  return 1;
}

static int eflDialogSetMaximizedAttrib(Ihandle* ih, const char* value)
{
  Eo* win = iupeflGetWidget(ih);
  if (!win)
    return 0;

  if (iupStrBoolean(value))
  {
    efl_ui_win_maximized_set(win, EINA_TRUE);
    ih->data->show_state = IUP_MAXIMIZE;
  }
  else
  {
    efl_ui_win_maximized_set(win, EINA_FALSE);
    ih->data->show_state = IUP_RESTORE;
  }

  return 0;
}

static char* eflDialogGetMaximizedAttrib(Ihandle* ih)
{
  Eo* win = iupeflGetWidget(ih);
  if (!win)
    return "NO";

  return efl_ui_win_maximized_get(win) ? "YES" : "NO";
}

static int eflDialogSetMinimizedAttrib(Ihandle* ih, const char* value)
{
  Eo* win = iupeflGetWidget(ih);
  if (!win)
    return 0;

  if (iupStrBoolean(value))
  {
    efl_ui_win_minimized_set(win, EINA_TRUE);
    ih->data->show_state = IUP_MINIMIZE;
  }
  else
  {
    efl_ui_win_minimized_set(win, EINA_FALSE);
    ih->data->show_state = IUP_RESTORE;
  }

  return 0;
}

static char* eflDialogGetMinimizedAttrib(Ihandle* ih)
{
  Eo* win = iupeflGetWidget(ih);
  if (!win)
    return "NO";

  return efl_ui_win_minimized_get(win) ? "YES" : "NO";
}

static int eflDialogSetTopMostAttrib(Ihandle* ih, const char* value)
{
  Eo* win = iupeflGetWidget(ih);
  if (!win)
    return 0;

  if (iupStrBoolean(value))
    efl_gfx_stack_raise_to_top(win);

  return 1;
}

static int eflDialogSetBgColorAttrib(Ihandle* ih, const char* value)
{
  Eo* win = iupeflGetWidget(ih);
  Eo* bg_rect;
  unsigned char r, g, b;

  if (!win)
    return 0;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  bg_rect = (Eo*)iupAttribGet(ih, "_IUP_EFL_BGRECT");
  if (!bg_rect)
  {
    bg_rect = efl_add_ref(EFL_CANVAS_RECTANGLE_CLASS, win);
    iupAttribSet(ih, "_IUP_EFL_BGRECT", (char*)bg_rect);
    efl_gfx_entity_visible_set(bg_rect, EINA_TRUE);
  }

  efl_gfx_color_set(bg_rect, r, g, b, 255);
  efl_gfx_entity_size_set(bg_rect, EINA_SIZE2D(ih->currentwidth, ih->currentheight));
  efl_gfx_entity_position_set(bg_rect, EINA_POSITION2D(0, 0));
  eflDialogStackBgBelow(ih, bg_rect);

  return 1;
}

static int eflDialogSetBackgroundAttrib(Ihandle* ih, const char* value)
{
  if (eflDialogSetBgColorAttrib(ih, value))
  {
    Eo* old_bg_img = (Eo*)iupAttribGet(ih, "_IUP_EFL_BACKGROUND_IMAGE");
    if (old_bg_img)
    {
      eflDialogFreeCanvasImage(old_bg_img);
      iupAttribSet(ih, "_IUP_EFL_BACKGROUND_IMAGE", NULL);
    }
    return 1;
  }
  else
  {
    Eo* src_img = (Eo*)iupImageGetImage(value, ih, 0, NULL);
    if (src_img)
    {
      Eo* win = iupeflGetWidget(ih);
      Eo* bg_rect;
      Eo* bg_img;
      Eina_Size2D size;
      int w, h;
      int stride = 0;
      Eina_Rw_Slice mapped;

      if (!win)
        return 0;

      size = efl_gfx_buffer_size_get(src_img);
      w = size.w;
      h = size.h;
      if (w <= 0 || h <= 0)
        return 0;

      mapped = efl_gfx_buffer_map(src_img, EFL_GFX_BUFFER_ACCESS_MODE_READ,
                                   NULL, EFL_GFX_COLORSPACE_ARGB8888, 0, &stride);
      if (!mapped.mem)
        return 0;

      bg_rect = (Eo*)iupAttribGet(ih, "_IUP_EFL_BGRECT");
      if (bg_rect)
      {
        efl_unref(bg_rect);
        iupAttribSet(ih, "_IUP_EFL_BGRECT", NULL);
      }

      bg_img = (Eo*)iupAttribGet(ih, "_IUP_EFL_BACKGROUND_IMAGE");
      if (bg_img)
        eflDialogFreeCanvasImage(bg_img);

      bg_img = efl_add_ref(EFL_CANVAS_IMAGE_CLASS, win);
      iupAttribSet(ih, "_IUP_EFL_BACKGROUND_IMAGE", (char*)bg_img);

      evas_object_image_alpha_set(bg_img, efl_gfx_buffer_alpha_get(src_img));
      evas_object_image_size_set(bg_img, w, h);
      {
        unsigned int* dst = evas_object_image_data_get(bg_img, EINA_TRUE);
        if (dst)
        {
          int y;
          for (y = 0; y < h; y++)
            memcpy(dst + y * w, (unsigned char*)mapped.mem + y * stride, w * sizeof(unsigned int));
          evas_object_image_data_set(bg_img, dst);
        }
      }

      efl_gfx_buffer_unmap(src_img, mapped);

      if (iupAttribGetBoolean(ih, "BACKIMAGEZOOM"))
        evas_object_image_fill_set(bg_img, 0, 0, ih->currentwidth, ih->currentheight);
      else
        evas_object_image_fill_set(bg_img, 0, 0, w, h);

      efl_gfx_entity_size_set(bg_img, EINA_SIZE2D(ih->currentwidth, ih->currentheight));
      efl_gfx_entity_position_set(bg_img, EINA_POSITION2D(0, 0));
      eflDialogStackBgBelow(ih, bg_img);
      efl_gfx_entity_visible_set(bg_img, EINA_TRUE);

      return 1;
    }
  }

  return 0;
}

static int eflDialogSetOpacityAttrib(Ihandle* ih, const char* value)
{
  Eo* win = iupeflGetWidget(ih);
  Ecore_Evas* ee;
  int opacity = 255;

  if (!win)
    return 0;

  iupStrToInt(value, &opacity);
  if (opacity < 0) opacity = 0;
  if (opacity > 255) opacity = 255;

  ee = ecore_evas_object_ecore_evas_get(win);
  if (!ee)
    return 0;

  if (opacity < 255)
    ecore_evas_alpha_set(ee, EINA_TRUE);
  else
    ecore_evas_alpha_set(ee, EINA_FALSE);

  efl_gfx_color_set(win, opacity, opacity, opacity, opacity);
  return 1;
}

static int eflDialogSetShapeImageAttrib(Ihandle* ih, const char* value)
{
  Eo* win = iupeflGetWidget(ih);
  Ecore_Evas* ee;
  Ihandle* image;

  if (!win)
    return 0;

  ee = ecore_evas_object_ecore_evas_get(win);
  if (!ee)
    return 0;

  if (!value)
  {
    ecore_evas_shaped_set(ee, EINA_FALSE);
    return 1;
  }

  image = IupGetHandle(value);
  if (image)
  {
    unsigned char* imgdata = (unsigned char*)iupAttribGet(image, "WID");
    int channels = iupAttribGetInt(image, "CHANNELS");
    int w = image->currentwidth;
    int h = image->currentheight;

    if (imgdata && channels == 4)
    {
      Eo* mask_img = efl_add_ref(EFL_CANVAS_IMAGE_CLASS, win);
      int x, y;

      evas_object_image_alpha_set(mask_img, EINA_TRUE);
      evas_object_image_size_set(mask_img, w, h);
      {
        unsigned int* pixels = evas_object_image_data_get(mask_img, EINA_TRUE);
        if (pixels)
        {
          for (y = 0; y < h; y++)
          {
            for (x = 0; x < w; x++)
            {
              unsigned char a = imgdata[(y * w + x) * 4 + 3];
              if (a > 0)
                pixels[y * w + x] = 0xFFFFFFFF;
              else
                pixels[y * w + x] = 0x00000000;
            }
          }
          evas_object_image_data_set(mask_img, pixels);
        }
      }
      evas_object_image_filled_set(mask_img, EINA_TRUE);

      efl_gfx_entity_size_set(mask_img, EINA_SIZE2D(w, h));
      efl_gfx_entity_visible_set(mask_img, EINA_TRUE);

      ecore_evas_shaped_set(ee, EINA_TRUE);

      eflDialogFreeCanvasImage(mask_img);
      return 1;
    }
  }

  return 0;
}

static int eflDialogSetOpacityImageAttrib(Ihandle* ih, const char* value)
{
  Eo* win = iupeflGetWidget(ih);
  Ecore_Evas* ee;
  Ihandle* image;

  if (!win)
    return 0;

  ee = ecore_evas_object_ecore_evas_get(win);
  if (!ee)
    return 0;

  if (!value)
  {
    Eo* old_img = (Eo*)iupAttribGet(ih, "_IUP_EFL_OPACITY_IMAGE");
    if (old_img)
    {
      eflDialogFreeCanvasImage(old_img);
      iupAttribSet(ih, "_IUP_EFL_OPACITY_IMAGE", NULL);
    }
    ecore_evas_alpha_set(ee, EINA_FALSE);
    efl_gfx_color_set(win, 255, 255, 255, 255);
    return 1;
  }

  image = IupGetHandle(value);
  if (image)
  {
    unsigned char* imgdata = (unsigned char*)iupAttribGet(image, "WID");
    int channels = iupAttribGetInt(image, "CHANNELS");
    int w = image->currentwidth;
    int h = image->currentheight;

    if (imgdata && channels == 4)
    {
      Eo* old_img = (Eo*)iupAttribGet(ih, "_IUP_EFL_OPACITY_IMAGE");
      Eo* bg_img;
      unsigned int* pixels;
      int x, y;

      if (old_img)
        eflDialogFreeCanvasImage(old_img);

      bg_img = efl_add_ref(EFL_CANVAS_IMAGE_CLASS, win);
      iupAttribSet(ih, "_IUP_EFL_OPACITY_IMAGE", (char*)bg_img);

      evas_object_image_alpha_set(bg_img, EINA_TRUE);
      evas_object_image_size_set(bg_img, w, h);
      {
        unsigned int* dst = evas_object_image_data_get(bg_img, EINA_TRUE);
        if (dst)
        {
          for (y = 0; y < h; y++)
          {
            for (x = 0; x < w; x++)
            {
              int idx = (y * w + x) * 4;
              unsigned char r = imgdata[idx];
              unsigned char g = imgdata[idx + 1];
              unsigned char b = imgdata[idx + 2];
              unsigned char a = imgdata[idx + 3];
              dst[y * w + x] = (a << 24) | (r << 16) | (g << 8) | b;
            }
          }
          evas_object_image_data_set(bg_img, dst);
        }
      }
      evas_object_image_filled_set(bg_img, EINA_TRUE);

      ecore_evas_alpha_set(ee, EINA_TRUE);

      efl_gfx_entity_size_set(bg_img, EINA_SIZE2D(w, h));
      efl_gfx_entity_position_set(bg_img, EINA_POSITION2D(0, 0));
      efl_gfx_entity_visible_set(bg_img, EINA_TRUE);

      return 1;
    }
  }

  return 0;
}

static char* eflDialogGetClientSizeAttrib(Ihandle* ih)
{
  Eo* win = iupeflGetWidget(ih);
  int width, height, menu_size;

  if (!win)
    return NULL;

  {
    Eina_Rect geom = iupeflGetGeometry(win);
    width = geom.w;
    height = geom.h;
  }

  menu_size = eflDialogGetMenuSize(ih);
  height -= menu_size;
  if (height < 0) height = 0;

  return iupStrReturnIntInt(width, height, 'x');
}

static char* eflDialogGetClientOffsetAttrib(Ihandle* ih)
{
  int menu_size = eflDialogGetMenuSize(ih);
  if (menu_size > 0)
    return iupStrReturnIntInt(0, -menu_size, 'x');
  return "0x0";
}

static char* eflDialogGetActiveWindowAttrib(Ihandle* ih)
{
  Eo* win = iupeflGetWidget(ih);
  if (!win)
    return "NO";
  return efl_ui_focus_object_focus_get(win) ? "YES" : "NO";
}

static int eflDialogSetIconAttrib(Ihandle* ih, const char* value)
{
  Eo* win = (Eo*)ih->handle;

  if (!value)
    efl_ui_win_icon_object_set(win, NULL);
  else
  {
    Evas_Object* icon = (Evas_Object*)iupImageGetIcon(value);
    if (icon)
    {
      int w = 0, h = 0;
      iupImageGetInfo(value, &w, &h, NULL);
      if (w > 0 && h > 0)
      {
        efl_gfx_hint_size_min_set(icon, EINA_SIZE2D(w, h));
        efl_gfx_hint_size_max_set(icon, EINA_SIZE2D(w, h));
      }
      efl_ui_win_icon_object_set(win, icon);
    }
  }

  return 1;
}

static int eflDialogSetBackImageZoomAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  if (iupAttribGet(ih, "_IUP_EFL_BACKGROUND_IMAGE"))
  {
    char* background = iupAttribGet(ih, "BACKGROUND");
    if (background)
      eflDialogSetBackgroundAttrib(ih, background);
  }
  return 1;
}

void iupdrvDialogInitClass(Iclass* ic)
{
  ic->Map = eflDialogMapMethod;
  ic->UnMap = eflDialogUnMapMethod;
  ic->LayoutUpdate = eflDialogLayoutUpdateMethod;
  ic->SetChildrenPosition = eflDialogSetChildrenPositionMethod;
  ic->GetInnerNativeContainerHandle = eflDialogGetInnerNativeContainerHandleMethod;

  iupClassRegisterCallback(ic, "THEMECHANGED_CB", "i");

  iupClassRegisterAttribute(ic, "TITLE", eflDialogGetTitleAttrib, eflDialogSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, eflDialogSetBgColorAttrib, "DLGBGCOLOR", NULL, IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "FULLSCREEN", NULL, eflDialogSetFullScreenAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAXIMIZED", eflDialogGetMaximizedAttrib, eflDialogSetMaximizedAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MINIMIZED", eflDialogGetMinimizedAttrib, eflDialogSetMinimizedAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOPMOST", NULL, eflDialogSetTopMostAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ICON", NULL, eflDialogSetIconAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MINSIZE", NULL, eflDialogSetMinSizeAttrib, IUPAF_SAMEASSYSTEM, "1x1", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAXSIZE", NULL, eflDialogSetMaxSizeAttrib, IUPAF_SAMEASSYSTEM, "65535x65535", IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BACKGROUND", NULL, eflDialogSetBackgroundAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKIMAGEZOOM", NULL, eflDialogSetBackImageZoomAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DIALOGHINT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUSTOMFRAME", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "OPACITY", NULL, eflDialogSetOpacityAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OPACITYIMAGE", NULL, eflDialogSetOpacityImageAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHAPEIMAGE", NULL, eflDialogSetShapeImageAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTSIZE", eflDialogGetClientSizeAttrib, iupDialogSetClientSizeAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_SAVE|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTOFFSET", eflDialogGetClientOffsetAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_DEFAULTVALUE|IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ACTIVEWINDOW", eflDialogGetActiveWindowAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "NATIVEPARENT", NULL, NULL, NULL, NULL, IUPAF_NO_STRING);
  iupClassRegisterAttribute(ic, iupeflGetNativeWindowHandleName(), iupeflGetNativeWindowHandleAttrib, NULL, NULL, NULL, IUPAF_NO_INHERIT|IUPAF_NO_STRING);

  iupClassRegisterAttribute(ic, "SAVEUNDER", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BRINGFRONT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMPOSITED", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CONTROL", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HELPBUTTON", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOOLBOX", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDIFRAME", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDICLIENT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDIMENU", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDICHILD", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
}
