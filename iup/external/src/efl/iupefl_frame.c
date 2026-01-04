/** \file
 * \brief Frame Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_frame.h"
#include "iup_drv.h"
#include "iup_drvfont.h"

#include "iupefl_drv.h"


/****************************************************************
                     Decoration Measurement
****************************************************************/

static void eflFrameMeasureDecorStyle(const char* style, int* decor_w, int* decor_h, int* offset_x, int* offset_y)
{
  Eo* temp_win;
  Eo* temp_frame;
  Eo* temp_child;
  int child_w = 100, child_h = 100;
  Eina_Size2D frame_min;

  temp_win = efl_add(EFL_UI_WIN_CLASS, iupeflGetLoop(),
    efl_ui_win_type_set(efl_added, EFL_UI_WIN_TYPE_BASIC),
    efl_text_set(efl_added, "iup_frame_measure"));
  if (!temp_win)
    return;

  /* Use legacy API which allows style changes after construction.
     The EO API loads the theme during constructor before style can be set. */
  temp_frame = elm_frame_add(temp_win);
  if (!temp_frame)
  {
    efl_del(temp_win);
    return;
  }

  if (style)
    elm_object_style_set(temp_frame, style);

  elm_frame_autocollapse_set(temp_frame, EINA_FALSE);

  temp_child = efl_add(EFL_UI_BOX_CLASS, temp_frame);
  efl_gfx_hint_size_min_set(temp_child, EINA_SIZE2D(child_w, child_h));
  efl_gfx_entity_size_set(temp_child, EINA_SIZE2D(child_w, child_h));
  efl_gfx_entity_visible_set(temp_child, EINA_TRUE);

  efl_content_set(temp_frame, temp_child);

  efl_gfx_entity_size_set(temp_frame, EINA_SIZE2D(200, 200));
  efl_gfx_entity_visible_set(temp_frame, EINA_TRUE);

  efl_canvas_group_calculate(temp_frame);

  frame_min = efl_gfx_hint_size_combined_min_get(temp_frame);

  *decor_w = frame_min.w - child_w;
  *decor_h = frame_min.h - child_h;

  if (*decor_w < 0) *decor_w = 0;
  if (*decor_h < 0) *decor_h = 0;

  {
    Eina_Rect child_geom = efl_gfx_entity_geometry_get(temp_child);
    Eina_Rect frame_geom = efl_gfx_entity_geometry_get(temp_frame);

    *offset_x = child_geom.x - frame_geom.x;
    *offset_y = child_geom.y - frame_geom.y;
    if (*offset_x < 0) *offset_x = 0;
    if (*offset_y < 0) *offset_y = 0;
  }

  efl_del(temp_win);
}

static int efl_frame_measured = 0;
static int efl_frame_title_decor_w = 0;
static int efl_frame_title_decor_h = 0;
static int efl_frame_title_offset_x = 0;
static int efl_frame_title_offset_y = 0;
static int efl_frame_notitle_decor_w = 0;
static int efl_frame_notitle_decor_h = 0;
static int efl_frame_notitle_offset_x = 0;
static int efl_frame_notitle_offset_y = 0;

static void eflFrameEnsureMeasured(void)
{
  if (!efl_frame_measured)
  {
    eflFrameMeasureDecorStyle(NULL, &efl_frame_title_decor_w, &efl_frame_title_decor_h,
                               &efl_frame_title_offset_x, &efl_frame_title_offset_y);
    eflFrameMeasureDecorStyle("outline", &efl_frame_notitle_decor_w, &efl_frame_notitle_decor_h,
                               &efl_frame_notitle_offset_x, &efl_frame_notitle_offset_y);
    efl_frame_measured = 1;
  }
}

static int eflFrameHasTitle(Ihandle* ih)
{
  char* title = iupAttribGet(ih, "TITLE");
  return (title && title[0]);
}

void iupdrvFrameGetDecorOffset(Ihandle* ih, int *x, int *y)
{
  eflFrameEnsureMeasured();
  if (eflFrameHasTitle(ih))
  {
    *x = efl_frame_title_offset_x;
    *y = efl_frame_title_offset_y;
  }
  else
  {
    *x = efl_frame_notitle_offset_x;
    *y = efl_frame_notitle_offset_y;
  }
}

int iupdrvFrameHasClientOffset(Ihandle* ih)
{
  (void)ih;
  return 1;
}

int iupdrvFrameGetTitleHeight(Ihandle* ih, int *h)
{
  (void)ih;
  *h = 0;
  return 1;
}

int iupdrvFrameGetDecorSize(Ihandle* ih, int *w, int *h)
{
  eflFrameEnsureMeasured();
  if (eflFrameHasTitle(ih))
  {
    *w = efl_frame_title_decor_w;
    *h = efl_frame_title_decor_h;
  }
  else
  {
    *w = efl_frame_notitle_decor_w;
    *h = efl_frame_notitle_decor_h;
  }
  return 1;
}

/****************************************************************
                     Attributes
****************************************************************/

static void eflFrameApplyFont(Ihandle* ih)
{
  Eo* frame = iupeflGetWidget(ih);
  if (frame && iupAttribGetStr(ih, "_IUPFRAME_HAS_TITLE"))
    iupeflApplyTextStyle(ih, frame);
}

static int eflFrameSetTitleAttrib(Ihandle* ih, const char* value)
{
  Eo* frame = iupeflGetWidget(ih);

  if (!frame)
    return 0;

  if (iupAttribGetStr(ih, "_IUPFRAME_HAS_TITLE"))
  {
    iupeflSetText(frame, iupeflStrConvertToSystem(value ? value : ""));
    eflFrameApplyFont(ih);
    return 1;
  }

  return 0;
}

static int eflFrameSetSunkenAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 1;
}

static int eflFrameSetBgColorAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 1;
}

static int eflFrameSetFgColorAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 1;
}

static int eflFrameSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  eflFrameApplyFont(ih);
  return 1;
}

/****************************************************************
                     Methods
****************************************************************/

static void* eflFrameGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
  Eo* frame = iupeflGetWidget(ih);
  (void)child;

  if (!frame)
    return NULL;

  return (void*)iupAttribGet(ih, "_IUP_EFL_INNER");
}

static int eflFrameMapMethod(Ihandle* ih)
{
  Eo* parent;
  Eo* frame;
  Eo* inner_parent;
  char* title;

  parent = iupeflGetParentWidget(ih);
  if (!parent)
    return IUP_ERROR;

  title = iupAttribGet(ih, "TITLE");

  if (title && title[0])
  {
    /* Frame with title, use default style */
    frame = efl_add(EFL_UI_FRAME_CLASS, parent);
    if (!frame)
      return IUP_ERROR;

    ih->handle = (InativeHandle*)frame;

    efl_ui_frame_autocollapse_set(frame, EINA_FALSE);

    iupAttribSet(ih, "_IUPFRAME_HAS_TITLE", "1");
    iupeflSetText(frame, iupeflStrConvertToSystem(title));
    eflFrameApplyFont(ih);
  }
  else
  {
    /* Frame without title, use legacy API with "outline" style.
       Legacy API allows style changes after construction. */
    frame = elm_frame_add(parent);
    if (!frame)
      return IUP_ERROR;

    ih->handle = (InativeHandle*)frame;

    elm_object_style_set(frame, "outline");
    elm_frame_autocollapse_set(frame, EINA_FALSE);

    if (iupAttribGet(ih, "BGCOLOR") || iupAttribGet(ih, "BACKCOLOR"))
      iupAttribSet(ih, "_IUPFRAME_HAS_BGCOLOR", "1");
  }

  inner_parent = iupeflNativeContainerNew(frame);
  if (inner_parent)
  {
    efl_gfx_entity_visible_set(inner_parent, EINA_TRUE);
    efl_content_set(frame, inner_parent);
    iupAttribSet(ih, "_IUP_EFL_INNER", (char*)inner_parent);
  }

  iupeflBaseAddCallbacks(ih, frame);

  iupeflAddToParent(ih);

  return IUP_NOERROR;
}

static void eflFrameUnMapMethod(Ihandle* ih)
{
  Eo* frame = iupeflGetWidget(ih);

  if (frame)
    iupeflDelete(frame);

  iupAttribSet(ih, "_IUP_EFL_INNER", NULL);
}

static void eflFrameComputeNaturalSizeMethod(Ihandle* ih, int* w, int* h, int* children_expand)
{
  int decor_w = 8, decor_h = 8;
  Ihandle* child = ih->firstchild;

  (void)children_expand;

  iupdrvFrameGetDecorSize(ih, &decor_w, &decor_h);

  *w = decor_w;
  *h = decor_h;

  if (child)
  {
    *w += child->naturalwidth;
    *h += child->naturalheight;
  }
}

void iupdrvFrameInitClass(Iclass* ic)
{
  ic->Map = eflFrameMapMethod;
  ic->UnMap = eflFrameUnMapMethod;
  ic->GetInnerNativeContainerHandle = eflFrameGetInnerNativeContainerHandleMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, eflFrameSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "BGCOLOR", iupFrameGetBgColorAttrib, eflFrameSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BACKCOLOR", iupFrameGetBgColorAttrib, eflFrameSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SUNKEN", NULL, eflFrameSetSunkenAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, eflFrameSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TITLE", NULL, eflFrameSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
}
