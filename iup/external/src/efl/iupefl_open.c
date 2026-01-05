/** \file
 * \brief EFL Driver Core
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#ifdef HAVE_ECORE_X
#include <Ecore_X.h>
#endif

#ifdef HAVE_ECORE_WL2
#include <Ecore_Wl2.h>
#endif

#include "iup.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_object.h"
#include "iup_globalattrib.h"

#include "iupefl_drv.h"


static int efl_open_count = 0;
static Eo* iupefl_main_loop = NULL;
static int iupefl_backend_x11 = -1;
static int iupefl_backend_wayland = -1;

Eo* iupeflGetLoop(void)
{
  return iupefl_main_loop;
}

/****************************************************************************
 * Backend Detection (X11 vs Wayland)
 ****************************************************************************/

static void iupeflDetectBackend(void)
{
  Eo* temp_win;
  Evas* evas;
  Ecore_Evas* ee;
  const char* engine;

  if (iupefl_backend_x11 >= 0)
    return;

  iupefl_backend_x11 = 0;
  iupefl_backend_wayland = 0;

  temp_win = efl_add(EFL_UI_WIN_CLASS, efl_main_loop_get(), efl_ui_win_type_set(efl_added, EFL_UI_WIN_TYPE_BASIC));
  if (!temp_win)
    return;

  evas = evas_object_evas_get(temp_win);
  if (!evas)
  {
    efl_del(temp_win);
    return;
  }

  ee = ecore_evas_ecore_evas_get(evas);
  if (!ee)
  {
    efl_del(temp_win);
    return;
  }

  engine = ecore_evas_engine_name_get(ee);
  if (engine)
  {
    if (strncmp(engine, "wayland", 7) == 0)
    {
      iupefl_backend_wayland = 1;
      IupSetGlobal("WINDOWING", "WAYLAND");
    }
    else if (strstr(engine, "x11") != NULL)
    {
      iupefl_backend_x11 = 1;
      IupSetGlobal("WINDOWING", "X11");
    }
  }

  efl_del(temp_win);
}

int iupeflIsWayland(void)
{
  return iupefl_backend_wayland > 0;
}

int iupeflIsX11(void)
{
  return iupefl_backend_x11 > 0;
}

/****************************************************************************
 * Native Handle Access
 ****************************************************************************/

char* iupeflGetNativeWidgetHandle(Evas_Object* widget)
{
  Evas* evas;
  Ecore_Evas* ee;

  if (!widget)
    return NULL;

  evas = evas_object_evas_get(widget);
  if (!evas)
    return NULL;

  ee = ecore_evas_ecore_evas_get(evas);
  if (!ee)
    return NULL;

#ifdef HAVE_ECORE_X
  if (iupeflIsX11())
  {
    Ecore_X_Window xwin = ecore_evas_window_get(ee);
    if (xwin)
      return (char*)(uintptr_t)xwin;
  }
#endif

#ifdef HAVE_ECORE_WL2
  if (iupeflIsWayland())
  {
    Ecore_Wl2_Window* wl_win = ecore_evas_wayland2_window_get(ee);
    if (wl_win)
    {
      struct wl_surface* surface = ecore_wl2_window_surface_get(wl_win);
      return (char*)surface;
    }
  }
#endif

  return NULL;
}

const char* iupeflGetNativeWindowHandleName(void)
{
  if (iupeflIsX11())
    return "XWINDOW";

  if (iupeflIsWayland())
    return "WL_SURFACE";

  return "UNKNOWN";
}

char* iupeflGetNativeWindowHandleAttrib(Ihandle* ih)
{
  return iupeflGetNativeWidgetHandle((Evas_Object*)ih->handle);
}

/****************************************************************************
 * Global Attribute Setup
 ****************************************************************************/

static void eflSetGlobalAttrib(void)
{
#ifdef HAVE_ECORE_X
  if (iupeflIsX11())
  {
    void* display = ecore_x_display_get();
    if (display)
    {
      IupSetGlobal("XDISPLAY", (char*)display);
      IupSetGlobal("XSCREEN", "0");
    }
  }
#endif

#ifdef HAVE_ECORE_WL2
  if (iupeflIsWayland())
  {
    Ecore_Wl2_Display* wl2_display = ecore_wl2_connected_display_get(NULL);
    if (wl2_display)
    {
      struct wl_display* wl_display = ecore_wl2_display_get(wl2_display);
      if (wl_display)
        IupSetGlobal("WL_DISPLAY", (char*)wl_display);
    }
  }
#endif
}

void iupeflSetGlobalColors(void)
{
  Eo* temp_win;
  Eo* widget;
  Eo* edje;
  Eo* part;
  int r, g, b, a;
  int bg_r = 240, bg_g = 240, bg_b = 240;
  int fg_r = 0, fg_g = 0, fg_b = 0;
  int txt_bg_r = 255, txt_bg_g = 255, txt_bg_b = 255;
  int hl_r = 51, hl_g = 153, hl_b = 255;

  temp_win = efl_add(EFL_UI_WIN_CLASS, efl_main_loop_get(),
                     efl_ui_win_type_set(efl_added, EFL_UI_WIN_TYPE_BASIC));
  if (!temp_win)
  {
    iupGlobalSetDefaultColorAttrib("DLGBGCOLOR", bg_r, bg_g, bg_b);
    iupGlobalSetDefaultColorAttrib("DLGFGCOLOR", fg_r, fg_g, fg_b);
    iupGlobalSetDefaultColorAttrib("TXTBGCOLOR", txt_bg_r, txt_bg_g, txt_bg_b);
    iupGlobalSetDefaultColorAttrib("TXTFGCOLOR", fg_r, fg_g, fg_b);
    iupGlobalSetDefaultColorAttrib("TXTHLCOLOR", hl_r, hl_g, hl_b);
    iupGlobalSetDefaultColorAttrib("MENUBGCOLOR", bg_r, bg_g, bg_b);
    iupGlobalSetDefaultColorAttrib("MENUFGCOLOR", fg_r, fg_g, fg_b);
    iupGlobalSetDefaultColorAttrib("LINKFGCOLOR", 0, 0, 238);
    return;
  }

  widget = elm_bg_add(temp_win);
  efl_gfx_entity_size_set(widget, EINA_SIZE2D(100, 100));
  efl_gfx_entity_visible_set(widget, EINA_TRUE);
  if (efl_isa(widget, EFL_UI_LAYOUT_BASE_CLASS))
  {
    edje = elm_layout_edje_get(widget);
    if (edje)
    {
      part = (Eo*)edje_object_part_object_get(edje, "base");
      if (part)
      {
        efl_gfx_color_get(part, &r, &g, &b, &a);
        bg_r = r; bg_g = g; bg_b = b;
      }
    }
  }

  widget = elm_button_add(temp_win);
  efl_text_set(efl_part(widget, "efl.text"), "X");
  efl_gfx_entity_size_set(widget, EINA_SIZE2D(100, 50));
  efl_gfx_entity_visible_set(widget, EINA_TRUE);
  if (efl_isa(widget, EFL_UI_LAYOUT_BASE_CLASS))
  {
    edje = elm_layout_edje_get(widget);
    if (edje)
    {
      part = (Eo*)edje_object_part_object_get(edje, "elm.text");
      if (part)
      {
        efl_gfx_color_get(part, &r, &g, &b, &a);
        if (a > 0)
        {
          fg_r = r; fg_g = g; fg_b = b;
        }
      }
    }
  }

  widget = elm_layout_add(temp_win);
  if (elm_layout_theme_set(widget, "scroller", "entry", "default"))
  {
    efl_gfx_entity_size_set(widget, EINA_SIZE2D(100, 50));
    efl_gfx_entity_visible_set(widget, EINA_TRUE);
    edje = elm_layout_edje_get(widget);
    if (edje)
    {
      part = (Eo*)edje_object_part_object_get(edje, "bg");
      if (part)
      {
        efl_gfx_color_get(part, &r, &g, &b, &a);
        if (a > 0)
        {
          txt_bg_r = r; txt_bg_g = g; txt_bg_b = b;
        }
      }
    }
  }

  widget = elm_layout_add(temp_win);
  if (elm_layout_theme_set(widget, "entry", "selection", "default"))
  {
    efl_gfx_entity_size_set(widget, EINA_SIZE2D(100, 50));
    efl_gfx_entity_visible_set(widget, EINA_TRUE);
    edje = elm_layout_edje_get(widget);
    if (edje)
    {
      part = (Eo*)edje_object_part_object_get(edje, "base");
      if (part)
      {
        efl_gfx_color_get(part, &r, &g, &b, &a);
        if (a > 0)
        {
          hl_r = r; hl_g = g; hl_b = b;
        }
      }
    }
  }

  efl_del(temp_win);

  iupGlobalSetDefaultColorAttrib("DLGBGCOLOR", bg_r, bg_g, bg_b);
  iupGlobalSetDefaultColorAttrib("DLGFGCOLOR", fg_r, fg_g, fg_b);
  iupGlobalSetDefaultColorAttrib("TXTBGCOLOR", txt_bg_r, txt_bg_g, txt_bg_b);
  iupGlobalSetDefaultColorAttrib("TXTFGCOLOR", fg_r, fg_g, fg_b);
  iupGlobalSetDefaultColorAttrib("TXTHLCOLOR", hl_r, hl_g, hl_b);
  iupGlobalSetDefaultColorAttrib("MENUBGCOLOR", bg_r, bg_g, bg_b);
  iupGlobalSetDefaultColorAttrib("MENUFGCOLOR", fg_r, fg_g, fg_b);
  iupGlobalSetDefaultColorAttrib("LINKFGCOLOR", 0, 0, 238);
}

/****************************************************************************
 * Driver Open/Close
 ****************************************************************************/

int iupdrvOpen(int* argc, char*** argv)
{
  char* value;

  if (efl_open_count > 0)
  {
    efl_open_count++;
    return IUP_NOERROR;
  }

  elm_init(*argc, *argv);

  iupefl_main_loop = efl_main_loop_get();

  setlocale(LC_NUMERIC, "C");

  elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_NONE);

#ifdef HAVE_ECORE_WL2
  if (getenv("WAYLAND_DISPLAY") && !getenv("ELM_DISPLAY") && !getenv("ELM_ENGINE"))
    setenv("ELM_DISPLAY", "wl", 0);
#endif

  iupeflDetectBackend();

  value = getenv("IUP_EFLTHEME");
  if (value && value[0])
    elm_theme_set(NULL, value);

  IupSetGlobal("DRIVER", "EFL");

  IupSetfAttribute(NULL, "EFLVERSION", "%d.%d.%d", ELM_VERSION_MAJOR, ELM_VERSION_MINOR, 0);

  if (argv && *argv && (*argv)[0] && (*argv)[0][0] != 0)
    IupStoreGlobal("ARGV0", (*argv)[0]);

  eflSetGlobalAttrib();
  iupeflSetGlobalColors();

  IupSetGlobal("SHOWMENUIMAGES", "YES");

  efl_open_count = 1;
  return IUP_NOERROR;
}

void iupdrvClose(void)
{
  if (efl_open_count == 0)
    return;

  efl_open_count--;

  if (efl_open_count > 0)
    return;

  iupeflLoopCleanup();

  iupefl_main_loop = NULL;
}
