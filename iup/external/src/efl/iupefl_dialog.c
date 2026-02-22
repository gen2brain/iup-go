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

  if (iupObjectCheck(ih))
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

    if (ih->firstchild)
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
    elm_win_resize_object_add(win, content_box);
    evas_object_show(content_box);
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
    efl_del(win);
  }

  ih->handle = NULL;
}

static void eflDialogLayoutUpdateMethod(Ihandle* ih)
{
  int width, height;
  Eo* win = iupeflGetWidget(ih);

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

static int eflDialogSetSizeAttrib(Ihandle* ih, const char* value)
{
  if (!value)
  {
    ih->userwidth = 0;
    ih->userheight = 0;
  }
  else
  {
    int s = 0, d = 0;
    iupStrToIntInt(value, &s, &d, 'x');
    if (s > 0)
    {
      int charwidth, charheight;
      iupdrvFontGetCharSize(ih, &charwidth, &charheight);
      ih->userwidth = iupWIDTH2RASTER(s, charwidth);
    }
    else
      ih->userwidth = 0;
    if (d > 0)
    {
      int charwidth, charheight;
      iupdrvFontGetCharSize(ih, &charwidth, &charheight);
      ih->userheight = iupHEIGHT2RASTER(d, charheight);
    }
    else
      ih->userheight = 0;
  }

  iupLayoutCompute(ih);

  if (!ih->handle)
    return 0;

  iupLayoutUpdate(ih);

  return 0;
}

static int eflDialogSetRasterSizeAttrib(Ihandle* ih, const char* value)
{
  if (!value)
  {
    ih->userwidth = 0;
    ih->userheight = 0;
  }
  else
  {
    int w = 0, h = 0;
    iupStrToIntInt(value, &w, &h, 'x');
    ih->userwidth = w;
    ih->userheight = h;
  }

  iupLayoutCompute(ih);

  if (!ih->handle)
    return 0;

  iupLayoutUpdate(ih);

  return 0;
}

static int eflDialogSetVisibleAttrib(Ihandle* ih, const char* value)
{
  Eo* win = iupeflGetWidget(ih);
  if (!win)
    return 0;

  iupeflSetVisible(win, iupStrBoolean(value) ? EINA_TRUE : EINA_FALSE);

  return 0;
}

static char* eflDialogGetVisibleAttrib(Ihandle* ih)
{
  Eo* win = iupeflGetWidget(ih);
  if (!win)
    return "NO";

  return iupeflIsVisible(win) ? "YES" : "NO";
}

static int eflDialogSetFullScreenAttrib(Ihandle* ih, const char* value)
{
  Eo* win = iupeflGetWidget(ih);
  if (!win)
    return 0;

  efl_ui_win_fullscreen_set(win, iupStrBoolean(value) ? EINA_TRUE : EINA_FALSE);

  return 1;
}

static char* eflDialogGetFullScreenAttrib(Ihandle* ih)
{
  Eo* win = iupeflGetWidget(ih);
  if (!win)
    return "NO";

  return efl_ui_win_fullscreen_get(win) ? "YES" : "NO";
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

static void eflDialogSetChildrenCurrentSizeMethod(Ihandle* ih, int shrink)
{
  int decorwidth, decorheight, client_width, client_height;

  (void)shrink;

  client_width = ih->currentwidth;
  client_height = ih->currentheight;

  iupDialogGetDecorSize(ih, &decorwidth, &decorheight);

  client_width  -= decorwidth;
  client_height -= decorheight;
  if (client_width < 0) client_width = 0;
  if (client_height < 0) client_height = 0;

  iupBaseSetCurrentSize(ih->firstchild, client_width, client_height, 1);
}

void iupdrvDialogInitClass(Iclass* ic)
{
  ic->Map = eflDialogMapMethod;
  ic->UnMap = eflDialogUnMapMethod;
  ic->LayoutUpdate = eflDialogLayoutUpdateMethod;
  ic->SetChildrenCurrentSize = eflDialogSetChildrenCurrentSizeMethod;
  ic->SetChildrenPosition = eflDialogSetChildrenPositionMethod;
  ic->GetInnerNativeContainerHandle = eflDialogGetInnerNativeContainerHandleMethod;

  iupClassRegisterCallback(ic, "THEMECHANGED_CB", "i");

  iupClassRegisterAttribute(ic, "TITLE", eflDialogGetTitleAttrib, eflDialogSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VISIBLE", eflDialogGetVisibleAttrib, eflDialogSetVisibleAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "FULLSCREEN", eflDialogGetFullScreenAttrib, eflDialogSetFullScreenAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAXIMIZED", eflDialogGetMaximizedAttrib, eflDialogSetMaximizedAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MINIMIZED", eflDialogGetMinimizedAttrib, eflDialogSetMinimizedAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOPMOST", NULL, eflDialogSetTopMostAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ICON", NULL, eflDialogSetIconAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "OPACITY", NULL, eflDialogSetOpacityAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTSIZE", eflDialogGetClientSizeAttrib, iupDialogSetClientSizeAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_SAVE|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTOFFSET", eflDialogGetClientOffsetAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_DEFAULTVALUE|IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ACTIVEWINDOW", eflDialogGetActiveWindowAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "NATIVEPARENT", NULL, NULL, NULL, NULL, IUPAF_NO_STRING);
  iupClassRegisterAttribute(ic, "NATIVEWINDOWHANDLE", iupeflGetNativeWindowHandleAttrib, NULL, NULL, NULL, IUPAF_NO_INHERIT | IUPAF_NO_STRING | IUPAF_READONLY);

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
