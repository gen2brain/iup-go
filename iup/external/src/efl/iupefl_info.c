/** \file
 * \brief EFL System Information
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <unistd.h>
#include <pwd.h>
#endif

#include "iup.h"
#include "iup_export.h"
#include "iup_str.h"
#include "iup_drvinfo.h"

#include "iupefl_drv.h"

#ifdef _WIN32
#include <windows.h>
#undef interface
#endif


IUP_SDK_API void iupdrvAddScreenOffset(int *x, int *y, int add)
{
  (void)x;
  (void)y;
  (void)add;
}

IUP_SDK_API void iupdrvGetScreenSize(int *width, int *height)
{
  Ecore_Evas* ee = ecore_evas_new(NULL, 0, 0, 1, 1, NULL);
  if (ee)
  {
    ecore_evas_screen_geometry_get(ee, NULL, NULL, width, height);
    ecore_evas_free(ee);
  }
  else
  {
    *width = 1920;
    *height = 1080;
  }
}

IUP_SDK_API void iupdrvGetFullSize(int *width, int *height)
{
  iupdrvGetScreenSize(width, height);
}

IUP_SDK_API int iupdrvGetScreenDepth(void)
{
  return 32;
}

IUP_SDK_API double iupdrvGetScreenDpi(void)
{
  Ecore_Evas* ee = ecore_evas_new(NULL, 0, 0, 1, 1, NULL);
  if (ee)
  {
    int dpi_x, dpi_y;
    ecore_evas_screen_dpi_get(ee, &dpi_x, &dpi_y);
    ecore_evas_free(ee);
    return (double)dpi_x;
  }
  return 96.0;
}

IUP_SDK_API void iupdrvGetCursorPos(int *x, int *y)
{
  Eo* win = iupeflGetMainWindow();

  *x = 0;
  *y = 0;

  if (win)
  {
    Evas* evas = evas_object_evas_get(win);
    if (evas)
    {
      Ecore_Evas* ee = ecore_evas_ecore_evas_get(evas);
      if (ee)
      {
        int wx, wy;
        ecore_evas_geometry_get(ee, &wx, &wy, NULL, NULL);
        ecore_evas_pointer_xy_get(ee, x, y);
        *x += wx;
        *y += wy;
      }
    }
  }
}

IUP_SDK_API void iupdrvGetKeyState(char* key)
{
  Eo* win;

  key[0] = ' ';
  key[1] = ' ';
  key[2] = ' ';
  key[3] = ' ';
  key[4] = 0;

  win = iupeflGetMainWindow();
  if (!win)
    return;

  if (efl_input_modifier_enabled_get(win, EFL_INPUT_MODIFIER_SHIFT, NULL))
    key[0] = 'S';
  if (efl_input_modifier_enabled_get(win, EFL_INPUT_MODIFIER_CONTROL, NULL))
    key[1] = 'C';
  if (efl_input_modifier_enabled_get(win, EFL_INPUT_MODIFIER_ALT, NULL))
    key[2] = 'A';
  if (efl_input_modifier_enabled_get(win, EFL_INPUT_MODIFIER_META, NULL) ||
      efl_input_modifier_enabled_get(win, EFL_INPUT_MODIFIER_SUPER, NULL))
    key[3] = 'Y';
}

IUP_SDK_API char* iupdrvGetComputerName(void)
{
  char* str = iupStrGetMemory(100);
#ifdef _WIN32
  DWORD size = 100;
  GetComputerNameA(str, &size);
#else
  gethostname(str, 100);
#endif
  return str;
}

IUP_SDK_API char* iupdrvGetUserName(void)
{
  char* str = iupStrGetMemory(100);
#ifdef _WIN32
  DWORD size = 100;
  GetUserNameA(str, &size);
#else
  {
    struct passwd* pwd = getpwuid(getuid());
    if (!pwd)
      return NULL;
    iupStrCopyN(str, 100, pwd->pw_name);
  }
#endif
  return str;
}

void* iupdrvGetDisplay(void)
{
  return NULL;
}

IUP_SDK_API int iupdrvGetScrollbarSize(void)
{
  static int cached_size = 0;
  Eo* win;
  Evas* evas;
  Evas_Object* scroller;

  if (cached_size > 0)
    return cached_size;

  win = iupeflGetMainWindow();
  if (!win)
  {
    cached_size = 15;
    return cached_size;
  }

  evas = evas_object_evas_get(win);
  scroller = elm_scroller_add(win);
  if (scroller)
  {
    Evas_Object* content;
    int sw = 0, sh = 0, cw = 0, ch = 0;

    content = efl_add(EFL_CANVAS_RECTANGLE_CLASS, evas);
    efl_gfx_entity_size_set(content, EINA_SIZE2D(500, 500));
    elm_object_content_set(scroller, content);
    elm_scroller_policy_set(scroller, ELM_SCROLLER_POLICY_ON, ELM_SCROLLER_POLICY_ON);
    efl_gfx_entity_size_set(scroller, EINA_SIZE2D(200, 200));
    efl_gfx_entity_visible_set(scroller, EINA_TRUE);
    evas_smart_objects_calculate(evas);

    Eina_Rect scroller_geom = efl_gfx_entity_geometry_get(scroller);
    sw = scroller_geom.w;
    sh = scroller_geom.h;
    elm_scroller_region_get(scroller, NULL, NULL, &cw, &ch);

    cached_size = sw - cw;
    if (cached_size <= 0)
      cached_size = 15;

    efl_del(scroller);
  }
  else
    cached_size = 15;

  return cached_size;
}

