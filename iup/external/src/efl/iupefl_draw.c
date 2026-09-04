/** \file
 * \brief Draw Functions
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "iupefl_drv.h"

#include "iup.h"
#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_str.h"
#include "iup_object.h"
#include "iup_image.h"
#include "iup_drvdraw.h"
#include "iup_draw.h"
#include "iup_drvfont.h"


struct _IdrawCanvas
{
  Ihandle* ih;
  int w, h;

  Eo* vg;                   /* the on-screen widget */
  Efl_VG* root;             /* shape container of the current layer */
  Eina_List* shapes;

  /* every primitive draws into this one buffer, in order, and it is rendered once at flush */
  Ecore_Evas* frame_ee;
  Evas* frame_evas;
  Eina_List* frame_objects;

  int clip_x1, clip_y1, clip_x2, clip_y2;
  int clip_corner_radius;
  Eo* clipper;

  Efl_VG* batch_shape;
  Efl_VG* batch_root;
  long batch_color;
  int batch_width;
};

/* Evas stacks later objects above earlier ones, so a new layer after each text or image keeps
   draw order */
static void iDrawNewLayer(IdrawCanvas* dc)
{
  Eo* layer = efl_add(EFL_CANVAS_VG_OBJECT_CLASS, dc->frame_evas);
  if (!layer)
    return;

  efl_gfx_entity_position_set(layer, EINA_POSITION2D(0, 0));
  efl_gfx_entity_size_set(layer, EINA_SIZE2D(dc->w, dc->h));
  efl_canvas_vg_object_viewbox_set(layer, EINA_RECT(0, 0, dc->w, dc->h));
  efl_canvas_vg_object_fill_mode_set(layer, EFL_CANVAS_VG_FILL_MODE_NONE);
  efl_gfx_entity_visible_set(layer, EINA_TRUE);
  if (dc->clipper)
    efl_canvas_object_clipper_set(layer, dc->clipper);

  dc->root = efl_add(EFL_CANVAS_VG_CONTAINER_CLASS, layer);
  efl_canvas_vg_object_root_node_set(layer, dc->root);

  dc->frame_objects = eina_list_append(dc->frame_objects, layer);
}

static void iDrawTrackObject(IdrawCanvas* dc, Eo* obj)
{
  if (!obj)
    return;
  if (dc->clipper)
    efl_canvas_object_clipper_set(obj, dc->clipper);
  dc->frame_objects = eina_list_append(dc->frame_objects, obj);
}

static void iDrawGetColor(long color, int* r, int* g, int* b, int* a)
{
  int red = iupDrawRed(color);
  int green = iupDrawGreen(color);
  int blue = iupDrawBlue(color);
  int alpha = iupDrawAlpha(color);

  /* EFL requires pre-multiplied colors */
  *r = (red * alpha) / 255;
  *g = (green * alpha) / 255;
  *b = (blue * alpha) / 255;
  *a = alpha;
}
static int iDrawHasClip(IdrawCanvas* dc)
{
  return (dc->clip_x1 > 0 || dc->clip_y1 > 0 || dc->clip_x2 < dc->w - 1 || dc->clip_y2 < dc->h - 1);
}

static int iDrawIsDashed(int style)
{
  return style == IUP_DRAW_STROKE_DASH || style == IUP_DRAW_STROKE_DOT ||
         style == IUP_DRAW_STROKE_DASH_DOT || style == IUP_DRAW_STROKE_DASH_DOT_DOT;
}

static void iDrawSetDash(Efl_VG* shape, int style)
{
  if (style == IUP_DRAW_STROKE_DASH)
  {
    Efl_Gfx_Dash dash[1] = { {6.0, 2.0} };
    efl_gfx_shape_stroke_dash_set(shape, dash, 1);
  }
  else if (style == IUP_DRAW_STROKE_DOT)
  {
    Efl_Gfx_Dash dash[1] = { {2.0, 2.0} };
    efl_gfx_shape_stroke_dash_set(shape, dash, 1);
  }
  else if (style == IUP_DRAW_STROKE_DASH_DOT)
  {
    Efl_Gfx_Dash dash[2] = { {6.0, 2.0}, {2.0, 2.0} };
    efl_gfx_shape_stroke_dash_set(shape, dash, 2);
  }
  else if (style == IUP_DRAW_STROKE_DASH_DOT_DOT)
  {
    Efl_Gfx_Dash dash[3] = { {6.0, 2.0}, {2.0, 2.0}, {2.0, 2.0} };
    efl_gfx_shape_stroke_dash_set(shape, dash, 3);
  }
}

static void iDrawRecycleFrame(Ihandle* ih)
{
  Eina_List* old_objects = (Eina_List*)iupAttribGet(ih, "_IUP_EFL_FRAME_OBJECTS");
  Eina_List* pool = (Eina_List*)iupAttribGet(ih, "_IUP_EFL_POOL_TEXT");
  Eina_List* l;
  Eo* obj;

  EINA_LIST_FOREACH(old_objects, l, obj)
  {
    const char* type = evas_object_type_get(obj);
    if (type && strcmp(type, "text") == 0)
    {
      efl_canvas_object_clipper_set(obj, NULL);
      efl_gfx_entity_visible_set(obj, EINA_FALSE);
      pool = eina_list_append(pool, obj);
    }
  }
  EINA_LIST_FREE(old_objects, obj)
  {
    const char* type = evas_object_type_get(obj);
    if (!type || strcmp(type, "text") != 0)
      efl_del(obj);
  }

  iupAttribSet(ih, "_IUP_EFL_FRAME_OBJECTS", NULL);
  iupAttribSet(ih, "_IUP_EFL_POOL_TEXT", (char*)pool);
}

void iupeflDrawReleaseFrame(Ihandle* ih)
{
  Ecore_Evas* ee = (Ecore_Evas*)iupAttribGet(ih, "_IUP_EFL_FRAME_EE");
  Eina_List* objects = (Eina_List*)iupAttribGet(ih, "_IUP_EFL_FRAME_OBJECTS");
  Eina_List* pool = (Eina_List*)iupAttribGet(ih, "_IUP_EFL_POOL_TEXT");

  if (objects)
    eina_list_free(objects);
  if (pool)
    eina_list_free(pool);
  if (ee)
    ecore_evas_free(ee);

  iupAttribSet(ih, "_IUP_EFL_FRAME_OBJECTS", NULL);
  iupAttribSet(ih, "_IUP_EFL_POOL_TEXT", NULL);
  iupAttribSet(ih, "_IUP_EFL_FRAME_EE", NULL);
}

IUP_SDK_API IdrawCanvas* iupdrvDrawCreateCanvas(Ihandle* ih)
{
  IdrawCanvas* dc = calloc(1, sizeof(IdrawCanvas));
  Eo* vg;
  Eina_Size2D size;
  Ecore_Evas* ee;

  dc->ih = ih;

  vg = iupeflGetWidget(ih);
  if (!vg)
  {
    free(dc);
    return NULL;
  }

  size = efl_gfx_entity_size_get(vg);
  dc->w = size.w;
  dc->h = size.h;
  dc->vg = vg;

  if (dc->w < 1) dc->w = 1;
  if (dc->h < 1) dc->h = 1;

  ee = (Ecore_Evas*)iupAttribGet(ih, "_IUP_EFL_FRAME_EE");
  if (ee && (iupAttribGetInt(ih, "_IUP_EFL_FRAME_EE_W") != dc->w ||
             iupAttribGetInt(ih, "_IUP_EFL_FRAME_EE_H") != dc->h))
  {
    iupeflDrawReleaseFrame(ih);
    ee = NULL;
  }

  if (!ee)
  {
    ee = ecore_evas_buffer_new(dc->w, dc->h);
    if (!ee)
    {
      free(dc);
      return NULL;
    }
    ecore_evas_alpha_set(ee, EINA_TRUE);
    iupAttribSet(ih, "_IUP_EFL_FRAME_EE", (char*)ee);
    iupAttribSetInt(ih, "_IUP_EFL_FRAME_EE_W", dc->w);
    iupAttribSetInt(ih, "_IUP_EFL_FRAME_EE_H", dc->h);
  }
  else
    iDrawRecycleFrame(ih);

  dc->frame_ee = ee;
  dc->frame_evas = ecore_evas_get(ee);

  dc->clip_x1 = 0;
  dc->clip_y1 = 0;
  dc->clip_x2 = dc->w - 1;
  dc->clip_y2 = dc->h - 1;

  iDrawNewLayer(dc);

  iupAttribSet(ih, "DRAWDRIVER", "EFL_VG");

  return dc;
}

IUP_SDK_API void iupdrvDrawKillCanvas(IdrawCanvas* dc)
{
  if (dc->shapes)
    eina_list_free(dc->shapes);

  iupAttribSet(dc->ih, "_IUP_EFL_FRAME_OBJECTS", (char*)dc->frame_objects);

  free(dc);
}

IUP_SDK_API void iupdrvDrawUpdateSize(IdrawCanvas* dc)
{
  Eina_Size2D size;

  size = efl_gfx_entity_size_get(dc->vg);

  if (size.w != dc->w || size.h != dc->h)
  {
    dc->w = size.w;
    dc->h = size.h;
  }
}

/* one render for the whole frame; the same pixels are displayed and kept for readback */
IUP_SDK_API void iupdrvDrawFlush(IdrawCanvas* dc)
{
  const void* src;
  void* dst;
  char* updaterect;
  int x1 = 0, y1 = 0, x2 = dc->w - 1, y2 = dc->h - 1;
  int img_w = 0, img_h = 0;

  ecore_evas_manual_render(dc->frame_ee);

  src = ecore_evas_buffer_pixels_get(dc->frame_ee);
  if (!src)
    return;

  evas_object_image_size_get(dc->vg, &img_w, &img_h);

  updaterect = iupAttribGet(dc->ih, "_IUP_EFL_UPDATERECT");
  if (updaterect && img_w == dc->w && img_h == dc->h &&
      sscanf(updaterect, "%d %d %d %d", &x1, &y1, &x2, &y2) == 4)
  {
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 > dc->w - 1) x2 = dc->w - 1;
    if (y2 > dc->h - 1) y2 = dc->h - 1;
    if (x2 < x1 || y2 < y1)
      return;

    dst = evas_object_image_data_get(dc->vg, EINA_TRUE);
    if (dst)
    {
      int y, stride = evas_object_image_stride_get(dc->vg);
      for (y = y1; y <= y2; y++)
        memcpy((unsigned char*)dst + y * stride + x1 * sizeof(unsigned int),
               (const unsigned char*)src + ((size_t)y * dc->w + x1) * sizeof(unsigned int),
               (x2 - x1 + 1) * sizeof(unsigned int));
      evas_object_image_data_set(dc->vg, dst);
      evas_object_image_data_update_add(dc->vg, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
    }
    return;
  }

  evas_object_image_size_set(dc->vg, dc->w, dc->h);
  dst = evas_object_image_data_get(dc->vg, EINA_TRUE);
  if (dst)
  {
    memcpy(dst, src, (size_t)dc->w * dc->h * sizeof(unsigned int));
    evas_object_image_data_set(dc->vg, dst);
    evas_object_image_data_update_add(dc->vg, 0, 0, dc->w, dc->h);
  }

}

IUP_SDK_API void iupdrvDrawGetSize(IdrawCanvas* dc, int *w, int *h)
{
  if (w) *w = dc->w;
  if (h) *h = dc->h;
}

IUP_SDK_API void iupdrvDrawLine(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  Efl_VG* shape;
  int r, g, b, a;

  if (!iDrawIsDashed(style) && iupDrawAlpha(color) == 255 && dc->batch_shape &&
      dc->batch_root == dc->root && dc->batch_color == color && dc->batch_width == line_width &&
      dc->batch_shape == eina_list_last_data_get(dc->shapes))
  {
    efl_gfx_path_append_move_to(dc->batch_shape, x1, y1);
    efl_gfx_path_append_line_to(dc->batch_shape, x2, y2);
    return;
  }

  iDrawGetColor(color, &r, &g, &b, &a);

  shape = efl_add(EFL_CANVAS_VG_SHAPE_CLASS, dc->root,
    efl_gfx_path_append_move_to(efl_added, x1, y1),
    efl_gfx_path_append_line_to(efl_added, x2, y2),
    efl_gfx_shape_stroke_color_set(efl_added, r, g, b, a),
    efl_gfx_shape_stroke_width_set(efl_added, line_width > 0 ? line_width : 1),
    efl_gfx_shape_stroke_cap_set(efl_added, EFL_GFX_CAP_BUTT));

  iDrawSetDash(shape, style);

  dc->shapes = eina_list_append(dc->shapes, shape);

  dc->batch_shape = (iDrawIsDashed(style) || iupDrawAlpha(color) != 255) ? NULL : shape;
  dc->batch_root = dc->root;
  dc->batch_color = color;
  dc->batch_width = line_width;
}

IUP_SDK_API void iupdrvDrawRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  Efl_VG* shape;
  int r, g, b, a;
  int rect_w, rect_h;

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  iDrawGetColor(color, &r, &g, &b, &a);

  rect_w = x2 - x1 + 1;
  rect_h = y2 - y1 + 1;

  if (style == IUP_DRAW_FILL)
  {
    shape = efl_add(EFL_CANVAS_VG_SHAPE_CLASS, dc->root,
      efl_gfx_path_append_rect(efl_added, x1, y1, rect_w, rect_h, 0, 0));
    efl_gfx_color_set(shape, r, g, b, a);
    efl_gfx_shape_stroke_color_set(shape, 0, 0, 0, 0);
  }
  else
  {
    double stroke_w = line_width > 0 ? line_width : 1;
    double offset = stroke_w / 2.0;
    shape = efl_add(EFL_CANVAS_VG_SHAPE_CLASS, dc->root,
      efl_gfx_path_append_rect(efl_added, x1 + offset, y1 + offset, rect_w - stroke_w, rect_h - stroke_w, 0, 0));
    efl_gfx_shape_stroke_color_set(shape, r, g, b, a);
    efl_gfx_shape_stroke_width_set(shape, stroke_w);
    iDrawSetDash(shape, style);
  }

  dc->shapes = eina_list_append(dc->shapes, shape);
}

static void iDrawArcToVg(Efl_VG* root, Eina_List** shapes, int x1, int y1, int x2, int y2, double a1, double a2, long color, int style, int line_width)
{
  Efl_VG* shape;
  int r, g, b, a;
  double cx, cy, rx, ry;
  double sweep = a2 - a1;

  iDrawGetColor(color, &r, &g, &b, &a);

  cx = (x1 + x2) / 2.0;
  cy = (y1 + y2) / 2.0;
  rx = (x2 - x1) / 2.0;
  ry = (y2 - y1) / 2.0;

  shape = efl_add(EFL_CANVAS_VG_SHAPE_CLASS, root);

  if (sweep >= 360.0 || sweep <= -360.0)
  {
    efl_gfx_path_append_circle(shape, cx, cy, rx);
    if (rx != ry)
    {
      Eina_Matrix3 m;
      eina_matrix3_identity(&m);
      eina_matrix3_translate(&m, cx, cy);
      eina_matrix3_scale(&m, 1.0, ry / rx);
      eina_matrix3_translate(&m, -cx, -cy);
      efl_canvas_vg_node_transformation_set(shape, &m);
    }
  }
  else
  {
    if (style == IUP_DRAW_FILL)
    {
      efl_gfx_path_append_move_to(shape, cx, cy);
      efl_gfx_path_append_arc(shape, x1, y1, x2 - x1, y2 - y1, a1, sweep);
      efl_gfx_path_append_close(shape);
    }
    else
    {
      efl_gfx_path_append_arc(shape, x1, y1, x2 - x1, y2 - y1, a1, sweep);
    }
  }

  if (style == IUP_DRAW_FILL)
  {
    efl_gfx_color_set(shape, r, g, b, a);
    efl_gfx_shape_stroke_color_set(shape, 0, 0, 0, 0);
  }
  else
  {
    efl_gfx_color_set(shape, 0, 0, 0, 0);
    efl_gfx_shape_stroke_color_set(shape, r, g, b, a);
    efl_gfx_shape_stroke_width_set(shape, line_width > 0 ? line_width : 1);
  }

  if (shapes)
    *shapes = eina_list_append(*shapes, shape);
}

IUP_SDK_API void iupdrvDrawArc(IdrawCanvas* dc, int x1, int y1, int x2, int y2, double a1, double a2, long color, int style, int line_width)
{
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  iDrawArcToVg(dc->root, &dc->shapes, x1, y1, x2, y2, a1, a2, color, style, line_width);
}

IUP_SDK_API void iupdrvDrawEllipse(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  Efl_VG* shape;
  int r, g, b, a;
  double cx, cy, rx, ry;

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  iDrawGetColor(color, &r, &g, &b, &a);

  cx = (x1 + x2) / 2.0;
  cy = (y1 + y2) / 2.0;
  rx = (x2 - x1) / 2.0;
  ry = (y2 - y1) / 2.0;

  shape = efl_add(EFL_CANVAS_VG_SHAPE_CLASS, dc->root,
    efl_gfx_path_append_circle(efl_added, cx, cy, rx));

  if (rx != ry)
  {
    Eina_Matrix3 m;
    eina_matrix3_identity(&m);
    eina_matrix3_translate(&m, cx, cy);
    eina_matrix3_scale(&m, 1.0, ry / rx);
    eina_matrix3_translate(&m, -cx, -cy);
    efl_canvas_vg_node_transformation_set(shape, &m);
  }

  if (style == IUP_DRAW_FILL)
  {
    efl_gfx_color_set(shape, r, g, b, a);
    efl_gfx_shape_stroke_color_set(shape, 0, 0, 0, 0);
  }
  else
  {
    efl_gfx_shape_stroke_color_set(shape, r, g, b, a);
    efl_gfx_shape_stroke_width_set(shape, line_width > 0 ? line_width : 1);
  }

  dc->shapes = eina_list_append(dc->shapes, shape);
}

IUP_SDK_API void iupdrvDrawPolygon(IdrawCanvas* dc, int* points, int count, long color, int style, int line_width)
{
  Efl_VG* shape;
  int r, g, b, a;
  int i;

  if (count < 2)
    return;

  iDrawGetColor(color, &r, &g, &b, &a);

  shape = efl_add(EFL_CANVAS_VG_SHAPE_CLASS, dc->root,
    efl_gfx_path_append_move_to(efl_added, points[0], points[1]));

  for (i = 1; i < count; i++)
    efl_gfx_path_append_line_to(shape, points[i * 2], points[i * 2 + 1]);

  if (style == IUP_DRAW_FILL)
  {
    efl_gfx_path_append_close(shape);
    efl_gfx_color_set(shape, r, g, b, a);
    efl_gfx_shape_stroke_color_set(shape, 0, 0, 0, 0);
  }
  else
  {
    if (count > 2)
      efl_gfx_path_append_close(shape);

    efl_gfx_shape_stroke_color_set(shape, r, g, b, a);
    efl_gfx_shape_stroke_width_set(shape, line_width > 0 ? line_width : 1);
  }

  dc->shapes = eina_list_append(dc->shapes, shape);
}

IUP_SDK_API void iupdrvDrawPixel(IdrawCanvas* dc, int x, int y, long color)
{
  iupdrvDrawRectangle(dc, x, y, x, y, color, IUP_DRAW_FILL, 1);
}

IUP_SDK_API void iupdrvDrawRoundedRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int corner_radius, long color, int style, int line_width)
{
  Efl_VG* shape;
  int r, g, b, a;

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  if (corner_radius <= 0)
  {
    iupdrvDrawRectangle(dc, x1, y1, x2, y2, color, style, line_width);
    return;
  }

  iDrawGetColor(color, &r, &g, &b, &a);

  shape = efl_add(EFL_CANVAS_VG_SHAPE_CLASS, dc->root,
    efl_gfx_path_append_rect(efl_added, x1, y1, x2 - x1 + 1, y2 - y1 + 1, corner_radius, corner_radius));

  if (style == IUP_DRAW_FILL)
  {
    efl_gfx_color_set(shape, r, g, b, a);
    efl_gfx_shape_stroke_color_set(shape, 0, 0, 0, 0);
  }
  else
  {
    efl_gfx_shape_stroke_color_set(shape, r, g, b, a);
    efl_gfx_shape_stroke_width_set(shape, line_width > 0 ? line_width : 1);
  }

  dc->shapes = eina_list_append(dc->shapes, shape);
}

IUP_SDK_API void iupdrvDrawBezier(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, long color, int style, int line_width)
{
  Efl_VG* shape;
  int r, g, b, a;

  iDrawGetColor(color, &r, &g, &b, &a);

  shape = efl_add(EFL_CANVAS_VG_SHAPE_CLASS, dc->root,
    efl_gfx_path_append_move_to(efl_added, x1, y1),
    efl_gfx_path_append_cubic_to(efl_added, x2, y2, x3, y3, x4, y4));

  if (style == IUP_DRAW_FILL)
  {
    efl_gfx_path_append_close(shape);
    efl_gfx_color_set(shape, r, g, b, a);
    efl_gfx_shape_stroke_color_set(shape, 0, 0, 0, 0);
  }
  else
  {
    efl_gfx_shape_stroke_color_set(shape, r, g, b, a);
    efl_gfx_shape_stroke_width_set(shape, line_width > 0 ? line_width : 1);
    iDrawSetDash(shape, style);
  }

  dc->shapes = eina_list_append(dc->shapes, shape);
}

IUP_SDK_API void iupdrvDrawQuadraticBezier(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int x3, int y3, long color, int style, int line_width)
{
  int cx1 = x1 + 2 * (x2 - x1) / 3;
  int cy1 = y1 + 2 * (y2 - y1) / 3;
  int cx2 = x3 + 2 * (x2 - x3) / 3;
  int cy2 = y3 + 2 * (y2 - y3) / 3;

  iupdrvDrawBezier(dc, x1, y1, cx1, cy1, cx2, cy2, x3, y3, color, style, line_width);
}

IUP_SDK_API void iupdrvDrawLinearGradient(IdrawCanvas* dc, int x1, int y1, int x2, int y2, float angle, const long* colors, const float* offsets, int count)
{
  Efl_VG* shape;
  Efl_Canvas_Vg_Gradient* grad;
  Efl_Gfx_Gradient_Stop stops[IUP_GRADIENT_MAX_STOPS];
  int corner_radius = dc->clip_corner_radius;
  double rad, w, h, gx0, gy0, gx1, gy1;
  int i;

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  w = (double)(x2 - x1);
  h = (double)(y2 - y1);
  rad = angle * M_PI / 180.0;

  gx0 = x1 + w / 2.0 - (w * cos(rad)) / 2.0;
  gy0 = y1 + h / 2.0 - (h * sin(rad)) / 2.0;
  gx1 = x1 + w / 2.0 + (w * cos(rad)) / 2.0;
  gy1 = y1 + h / 2.0 + (h * sin(rad)) / 2.0;

  shape = efl_add(EFL_CANVAS_VG_SHAPE_CLASS, dc->root,
    efl_gfx_path_append_rect(efl_added, x1, y1, x2 - x1 + 1, y2 - y1 + 1, corner_radius, corner_radius));

  grad = efl_add(EFL_CANVAS_VG_GRADIENT_LINEAR_CLASS, dc->root,
    efl_gfx_gradient_linear_start_set(efl_added, gx0, gy0),
    efl_gfx_gradient_linear_end_set(efl_added, gx1, gy1));

  for (i = 0; i < count; i++)
  {
    int r, g, b, a;
    iDrawGetColor(colors[i], &r, &g, &b, &a);
    stops[i].offset = offsets[i];
    stops[i].r = r; stops[i].g = g; stops[i].b = b; stops[i].a = a;
  }
  efl_gfx_gradient_stop_set(grad, stops, count);

  efl_canvas_vg_shape_fill_set(shape, grad);

  dc->shapes = eina_list_append(dc->shapes, shape);
  dc->shapes = eina_list_append(dc->shapes, grad);
}

IUP_SDK_API void iupdrvDrawRadialGradient(IdrawCanvas* dc, int cx, int cy, int radius, const long* colors, const float* offsets, int count)
{
  Efl_VG* shape;
  Efl_Canvas_Vg_Gradient* grad;
  Efl_Gfx_Gradient_Stop stops[IUP_GRADIENT_MAX_STOPS];
  int i;

  shape = efl_add(EFL_CANVAS_VG_SHAPE_CLASS, dc->root,
    efl_gfx_path_append_circle(efl_added, cx, cy, radius));

  grad = efl_add(EFL_CANVAS_VG_GRADIENT_RADIAL_CLASS, dc->root,
    efl_gfx_gradient_radial_center_set(efl_added, cx, cy),
    efl_gfx_gradient_radial_radius_set(efl_added, radius));

  for (i = 0; i < count; i++)
  {
    int r, g, b, a;
    iDrawGetColor(colors[i], &r, &g, &b, &a);
    stops[i].offset = offsets[i];
    stops[i].r = r; stops[i].g = g; stops[i].b = b; stops[i].a = a;
  }
  efl_gfx_gradient_stop_set(grad, stops, count);

  efl_canvas_vg_shape_fill_set(shape, grad);

  dc->shapes = eina_list_append(dc->shapes, shape);
  dc->shapes = eina_list_append(dc->shapes, grad);
}

/* the text object goes straight into the frame; its own geometry is the measurement */
IUP_SDK_API void iupdrvDrawText(IdrawCanvas* dc, const char* text, int len, int x, int y, int w, int h, long color, const char* font, int flags, double text_orientation)
{
  Evas_Object* text_obj;
  Eo* own_clip;
  int r, g, b, a;
  int fontsize = 12;
  int tw, th;
  int draw_x, draw_y;
  char typeface[256] = "Sans";
  char font_with_style[300];
  int bold = 0, italic = 0, underline = 0, strikeout = 0;
  int layout_center = flags & IUP_DRAW_LAYOUTCENTER;
  char* text_copy = NULL;

  if (!text || !text[0])
    return;

  iDrawGetColor(color, &r, &g, &b, &a);

  if (font)
    iupFontParsePango(font, typeface, &fontsize, &bold, &italic, &underline, &strikeout);

  if (fontsize <= 0)
    fontsize = 12;

  if (len > 0)
  {
    text_copy = (char*)malloc(len + 1);
    if (!text_copy)
      return;
    memcpy(text_copy, text, len);
    text_copy[len] = '\0';
    text = text_copy;
  }

  snprintf(font_with_style, sizeof(font_with_style), "%s%s%s",
      typeface, bold ? ":style=Bold" : "", italic ? ":style=Italic" : "");

  if ((flags & IUP_DRAW_WRAP) || (flags & IUP_DRAW_ELLIPSIS) || strchr(text, '\n'))
  {
    Evas_Textblock_Style* ts;
    char style[512];
    char* markup;
    const char* align_str = "left";
    int box_w = w > 0 ? w : dc->w;
    int box_h = h > 0 ? h : dc->h;

    if (flags & IUP_DRAW_RIGHT)
      align_str = "right";
    else if (flags & IUP_DRAW_CENTER)
      align_str = "center";

    text_obj = evas_object_textblock_add(dc->frame_evas);
    ts = evas_textblock_style_new();

    snprintf(style, sizeof(style),
        "DEFAULT='font=%s font_size=%d color=#%02X%02X%02X%02X wrap=%s ellipsis=%s align=%s'",
        font_with_style, fontsize, r, g, b, a,
        (flags & IUP_DRAW_WRAP) ? "word" : "none",
        (flags & IUP_DRAW_ELLIPSIS) ? "1.0" : "-1.0",
        align_str);

    evas_textblock_style_set(ts, style);
    evas_object_textblock_style_set(text_obj, ts);
    markup = evas_textblock_text_utf8_to_markup(text_obj, text);
    evas_object_textblock_text_markup_set(text_obj, markup);
    free(markup);
    evas_textblock_style_free(ts);

    efl_gfx_entity_size_set(text_obj, EINA_SIZE2D(box_w, box_h));
    evas_object_textblock_size_formatted_get(text_obj, &tw, &th);
    if ((flags & IUP_DRAW_WRAP) || (flags & IUP_DRAW_ELLIPSIS))
      tw = box_w;
    /* align is relative to the object width, so it has to stay the box */
    efl_gfx_entity_size_set(text_obj, EINA_SIZE2D(box_w, th));
  }
  else
  {
    Eina_Rect geom;

    Eina_List* pool = (Eina_List*)iupAttribGet(dc->ih, "_IUP_EFL_POOL_TEXT");
    if (pool)
    {
      text_obj = (Eo*)eina_list_data_get(pool);
      pool = eina_list_remove_list(pool, pool);
      iupAttribSet(dc->ih, "_IUP_EFL_POOL_TEXT", (char*)pool);
      evas_object_raise(text_obj);
    }
    else
      text_obj = evas_object_text_add(dc->frame_evas);
    evas_object_text_font_set(text_obj, font_with_style, fontsize);
    evas_object_text_text_set(text_obj, text);
    efl_gfx_color_set(text_obj, r, g, b, a);

    geom = efl_gfx_entity_geometry_get(text_obj);
    tw = geom.w;
    th = geom.h;

    if (w > 0)
    {
      if (flags & IUP_DRAW_RIGHT)
        x += w - tw;
      else if (flags & IUP_DRAW_CENTER)
        x += (w - tw) / 2;
    }
  }

  draw_x = x;
  draw_y = y;

  if (layout_center && text_orientation != 0 && w > 0 && h > 0)
  {
    draw_x = x + (w - tw) / 2;
    draw_y = y + (h - th) / 2;
  }

  efl_gfx_entity_position_set(text_obj, EINA_POSITION2D(draw_x, draw_y));
  efl_gfx_entity_visible_set(text_obj, EINA_TRUE);

  own_clip = NULL;
  if ((flags & IUP_DRAW_CLIP) && w > 0 && h > 0)
  {
    own_clip = efl_add(EFL_CANVAS_RECTANGLE_CLASS, dc->frame_evas);
    if (own_clip)
    {
      efl_gfx_entity_position_set(own_clip, EINA_POSITION2D(x, y));
      efl_gfx_entity_size_set(own_clip, EINA_SIZE2D(w, h));
      efl_gfx_color_set(own_clip, 255, 255, 255, 255);
      efl_gfx_entity_visible_set(own_clip, EINA_TRUE);
      /* chain under the canvas clip so both apply */
      if (dc->clipper)
        efl_canvas_object_clipper_set(own_clip, dc->clipper);
      efl_canvas_object_clipper_set(text_obj, own_clip);
      dc->frame_objects = eina_list_append(dc->frame_objects, own_clip);
    }
  }

  if (text_orientation != 0)
  {
    Evas_Map* map = evas_map_new(4);
    evas_map_util_points_populate_from_object(map, text_obj);
    if (layout_center)
      evas_map_util_rotate(map, -text_orientation, draw_x + tw / 2, draw_y + th / 2);
    else
      evas_map_util_rotate(map, -text_orientation, draw_x, draw_y);
    evas_object_map_set(text_obj, map);
    evas_object_map_enable_set(text_obj, EINA_TRUE);
    evas_map_free(map);
  }

  if (underline || strikeout)
  {
    int ly = underline ? (draw_y + th - 2) : (draw_y + th / 2);
    Evas_Object* line = efl_add(EFL_CANVAS_RECTANGLE_CLASS, dc->frame_evas);
    efl_gfx_color_set(line, r, g, b, a);
    efl_gfx_entity_position_set(line, EINA_POSITION2D(draw_x, ly));
    efl_gfx_entity_size_set(line, EINA_SIZE2D(tw, 1));
    efl_gfx_entity_visible_set(line, EINA_TRUE);
    iDrawTrackObject(dc, line);

    if (underline && strikeout)
    {
      Evas_Object* line2 = efl_add(EFL_CANVAS_RECTANGLE_CLASS, dc->frame_evas);
      efl_gfx_color_set(line2, r, g, b, a);
      efl_gfx_entity_position_set(line2, EINA_POSITION2D(draw_x, draw_y + th / 2));
      efl_gfx_entity_size_set(line2, EINA_SIZE2D(tw, 1));
      efl_gfx_entity_visible_set(line2, EINA_TRUE);
      iDrawTrackObject(dc, line2);
    }
  }

  if (own_clip)
    dc->frame_objects = eina_list_append(dc->frame_objects, text_obj);
  else
    iDrawTrackObject(dc, text_obj);
  iDrawNewLayer(dc);

  if (text_copy)
    free(text_copy);
}

typedef struct {
  unsigned char* imgdata;
  int img_w, bpp;
  iupColor* colors;
  int has_alpha, make_inactive;
  unsigned char bg_r, bg_g, bg_b;
  int tint_on, opacity;
  unsigned char tr, tg, tb, ta;
} IeflImageSampler;

static void eflDrawImageSample(const IeflImageSampler* s, int ix, int iy, int* pr, int* pg, int* pb, int* pa)
{
  unsigned char r, g, b, a;

  if (s->bpp == 8)
  {
    iupColor* c = &s->colors[s->imgdata[iy * s->img_w + ix]];
    r = c->r;
    g = c->g;
    b = c->b;
    a = c->a;
  }
  else
  {
    int channels = (s->bpp == 32) ? 4 : 3;
    unsigned char* p = s->imgdata + ((size_t)iy * s->img_w + ix) * channels;
    r = p[0];
    g = p[1];
    b = p[2];
    a = (s->bpp == 32) ? p[3] : 255;

    if (s->make_inactive)
    {
      if (a == 0 && s->has_alpha)
      {
        r = s->bg_r;
        g = s->bg_g;
        b = s->bg_b;
        a = 255;
      }
      iupImageColorMakeInactive(&r, &g, &b, s->bg_r, s->bg_g, s->bg_b);
    }
  }

  if (s->tint_on)
  {
    r = s->tr;
    g = s->tg;
    b = s->tb;
    a = (unsigned char)((a * s->ta) / 255);
  }

  if (s->opacity < 255)
    a = (unsigned char)((a * s->opacity) / 255);

  *pa = a;
  *pr = (r * a) / 255;
  *pg = (g * a) / 255;
  *pb = (b * a) / 255;
}

IUP_SDK_API void iupdrvDrawImage(IdrawCanvas* dc, const char* name, int make_inactive, const char* bgcolor, long tint, int opacity, int x, int y, int w, int h, int sx, int sy, int sw, int sh, int quality)
{
  Ihandle* img_ih;
  unsigned int* pixels;
  int img_w, img_h, bpp;
  int colors_count = 0;
  iupColor colors[256];
  int vis_x, vis_y, vis_w, vis_h;
  int px, py, bilinear;
  double scale_x, scale_y;
  IeflImageSampler s;

  img_ih = iupImageGetImageFromName(name);
  if (!img_ih)
    return;

  img_w = img_ih->currentwidth;
  img_h = img_ih->currentheight;
  bpp = iupAttribGetInt(img_ih, "BPP");

  memset(&s, 0, sizeof(s));
  s.img_w = img_w;
  s.bpp = bpp;
  s.colors = colors;
  s.make_inactive = make_inactive;
  s.opacity = opacity;

  if (bpp == 8)
    s.has_alpha = iupImageInitColorTable(img_ih, colors, &colors_count);
  else if (bpp == 32)
    s.has_alpha = 1;

  if (make_inactive && bgcolor)
    iupStrToRGB(bgcolor, &s.bg_r, &s.bg_g, &s.bg_b);

  if (tint != IUP_DRAW_NO_TINT)
  {
    s.tint_on = 1;
    s.tr = iupDrawRed(tint);
    s.tg = iupDrawGreen(tint);
    s.tb = iupDrawBlue(tint);
    s.ta = iupDrawAlpha(tint);
  }

  if (sw <= 0 || sh <= 0)
  {
    sx = 0;
    sy = 0;
    sw = img_w;
    sh = img_h;
  }
  if (w <= 0) w = sw;
  if (h <= 0) h = sh;

  vis_x = (x > dc->clip_x1) ? x : dc->clip_x1;
  vis_y = (y > dc->clip_y1) ? y : dc->clip_y1;
  vis_w = (((x + w) < (dc->clip_x2 + 1)) ? (x + w) : (dc->clip_x2 + 1)) - vis_x;
  vis_h = (((y + h) < (dc->clip_y2 + 1)) ? (y + h) : (dc->clip_y2 + 1)) - vis_y;

  if (vis_w <= 0 || vis_h <= 0)
    return;

  scale_x = (double)sw / (double)w;
  scale_y = (double)sh / (double)h;
  bilinear = (quality != IUP_DRAW_IMAGE_NEAREST) && (w != sw || h != sh);

  pixels = (unsigned int*)malloc((size_t)vis_w * vis_h * sizeof(unsigned int));
  if (!pixels)
    return;

  s.imgdata = (unsigned char*)iupAttribGetStr(img_ih, "WID");

  if (bpp == 8 && make_inactive)
  {
    int i;
    for (i = 0; i < colors_count; i++)
    {
      if (colors[i].a == 0)
      {
        colors[i].r = s.bg_r;
        colors[i].g = s.bg_g;
        colors[i].b = s.bg_b;
        colors[i].a = 255;
      }
      iupImageColorMakeInactive(&colors[i].r, &colors[i].g, &colors[i].b, s.bg_r, s.bg_g, s.bg_b);
    }
  }

  for (py = 0; py < vis_h; py++)
  {
    unsigned int* pix_line = pixels + py * vis_w;

    for (px = 0; px < vis_w; px++)
    {
      int pr, pg, pb, pa;

      if (bilinear)
      {
        double fx = sx + ((vis_x - x + px) + 0.5) * scale_x - 0.5;
        double fy = sy + ((vis_y - y + py) + 0.5) * scale_y - 0.5;
        int x0 = (int)floor(fx);
        int y0 = (int)floor(fy);
        double u = fx - x0, v = fy - y0;
        int x1 = x0 + 1, y1 = y0 + 1;
        int r00, g00, b00, a00, r10, g10, b10, a10;
        int r01, g01, b01, a01, r11, g11, b11, a11;

        if (x0 < sx) x0 = sx;
        if (y0 < sy) y0 = sy;
        if (x0 > sx + sw - 1) x0 = sx + sw - 1;
        if (y0 > sy + sh - 1) y0 = sy + sh - 1;
        if (x1 < sx) x1 = sx;
        if (y1 < sy) y1 = sy;
        if (x1 > sx + sw - 1) x1 = sx + sw - 1;
        if (y1 > sy + sh - 1) y1 = sy + sh - 1;

        eflDrawImageSample(&s, x0, y0, &r00, &g00, &b00, &a00);
        eflDrawImageSample(&s, x1, y0, &r10, &g10, &b10, &a10);
        eflDrawImageSample(&s, x0, y1, &r01, &g01, &b01, &a01);
        eflDrawImageSample(&s, x1, y1, &r11, &g11, &b11, &a11);

        pr = (int)((1 - u) * (1 - v) * r00 + u * (1 - v) * r10 + (1 - u) * v * r01 + u * v * r11 + 0.5);
        pg = (int)((1 - u) * (1 - v) * g00 + u * (1 - v) * g10 + (1 - u) * v * g01 + u * v * g11 + 0.5);
        pb = (int)((1 - u) * (1 - v) * b00 + u * (1 - v) * b10 + (1 - u) * v * b01 + u * v * b11 + 0.5);
        pa = (int)((1 - u) * (1 - v) * a00 + u * (1 - v) * a10 + (1 - u) * v * a01 + u * v * a11 + 0.5);
      }
      else
      {
        int ix = sx + (int)((vis_x - x + px) * scale_x);
        int iy = sy + (int)((vis_y - y + py) * scale_y);
        if (ix > sx + sw - 1) ix = sx + sw - 1;
        if (iy > sy + sh - 1) iy = sy + sh - 1;

        eflDrawImageSample(&s, ix, iy, &pr, &pg, &pb, &pa);
      }

      pix_line[px] = (pa << 24) | (pr << 16) | (pg << 8) | pb;
    }
  }

  {
    Evas_Object* img = evas_object_image_filled_add(dc->frame_evas);
    void* dst;

    evas_object_image_colorspace_set(img, EVAS_COLORSPACE_ARGB8888);
    evas_object_image_alpha_set(img, EINA_TRUE);
    evas_object_image_size_set(img, vis_w, vis_h);

    dst = evas_object_image_data_get(img, EINA_TRUE);
    if (dst)
    {
      memcpy(dst, pixels, (size_t)vis_w * vis_h * sizeof(unsigned int));
      evas_object_image_data_set(img, dst);
      evas_object_image_data_update_add(img, 0, 0, vis_w, vis_h);
    }

    efl_gfx_entity_position_set(img, EINA_POSITION2D(vis_x, vis_y));
    efl_gfx_entity_size_set(img, EINA_SIZE2D(vis_w, vis_h));
    efl_gfx_entity_visible_set(img, EINA_TRUE);

    iDrawTrackObject(dc, img);
    iDrawNewLayer(dc);
  }

  free(pixels);
}

/* a clipper applies to the objects it is set on, so the clip starts a fresh layer */
static void iDrawApplyClip(IdrawCanvas* dc)
{
  int w = dc->clip_x2 - dc->clip_x1 + 1;
  int h = dc->clip_y2 - dc->clip_y1 + 1;

  dc->clipper = NULL;

  if (dc->clip_x1 > 0 || dc->clip_y1 > 0 || w < dc->w || h < dc->h)
  {
    Eo* clipper = efl_add(EFL_CANVAS_RECTANGLE_CLASS, dc->frame_evas);
    if (clipper)
    {
      efl_gfx_entity_position_set(clipper, EINA_POSITION2D(dc->clip_x1, dc->clip_y1));
      efl_gfx_entity_size_set(clipper, EINA_SIZE2D(w > 0 ? w : 0, h > 0 ? h : 0));
      efl_gfx_color_set(clipper, 255, 255, 255, 255);
      efl_gfx_entity_visible_set(clipper, EINA_TRUE);
      dc->frame_objects = eina_list_append(dc->frame_objects, clipper);
      dc->clipper = clipper;
    }
  }

  iDrawNewLayer(dc);
}

IUP_SDK_API void iupdrvDrawSetClipRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  dc->clip_x1 = x1;
  dc->clip_y1 = y1;
  dc->clip_x2 = x2;
  dc->clip_y2 = y2;
  dc->clip_corner_radius = 0;

  iDrawApplyClip(dc);
}

IUP_SDK_API void iupdrvDrawSetClipRoundedRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int corner_radius)
{
  iupdrvDrawSetClipRect(dc, x1, y1, x2, y2);
  dc->clip_corner_radius = corner_radius;
}

IUP_SDK_API void iupdrvDrawResetClip(IdrawCanvas* dc)
{
  dc->clip_x1 = 0;
  dc->clip_y1 = 0;
  dc->clip_x2 = dc->w - 1;
  dc->clip_y2 = dc->h - 1;
  dc->clip_corner_radius = 0;

  iDrawApplyClip(dc);
}

IUP_SDK_API void iupdrvDrawGetClipRect(IdrawCanvas* dc, int *x1, int *y1, int *x2, int *y2)
{
  if (x1) *x1 = dc->clip_x1;
  if (y1) *y1 = dc->clip_y1;
  if (x2) *x2 = dc->clip_x2;
  if (y2) *y2 = dc->clip_y2;
}

IUP_SDK_API void iupdrvDrawSelectRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  iupdrvDrawRectangle(dc, x1, y1, x2, y2, iupDrawColor(0, 0, 255, 128), IUP_DRAW_FILL, 1);
}

IUP_SDK_API void iupdrvDrawFocusRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  iupdrvDrawRectangle(dc, x1, y1, x2, y2, iupDrawColor(0, 0, 0, 255), IUP_DRAW_STROKE_DOT, 1);
}

/* the frame is premultiplied ARGB; IUP wants packed straight RGBA */
static void iDrawUnpackFrame(const unsigned int* src, int src_w, unsigned char* data, int dst_w, int copy_w, int copy_h)
{
  int x, y;

  for (y = 0; y < copy_h; y++)
  {
    const unsigned int* src_line = src + (size_t)y * src_w;
    unsigned char* dst_line = data + (size_t)y * dst_w * 4;

    for (x = 0; x < copy_w; x++)
    {
      unsigned int pixel = src_line[x];
      unsigned char a = (unsigned char)((pixel >> 24) & 0xFF);
      unsigned char r = (unsigned char)((pixel >> 16) & 0xFF);
      unsigned char g = (unsigned char)((pixel >> 8) & 0xFF);
      unsigned char b = (unsigned char)(pixel & 0xFF);

      if (a != 0 && a != 255)
      {
        r = (unsigned char)((r * 255) / a);
        g = (unsigned char)((g * 255) / a);
        b = (unsigned char)((b * 255) / a);
      }

      dst_line[x * 4 + 0] = r;
      dst_line[x * 4 + 1] = g;
      dst_line[x * 4 + 2] = b;
      dst_line[x * 4 + 3] = a;
    }
  }
}

IUP_SDK_API int iupdrvDrawGetImageData(IdrawCanvas* dc, unsigned char* data)
{
  const void* src;

  ecore_evas_manual_render(dc->frame_ee);

  src = ecore_evas_buffer_pixels_get(dc->frame_ee);
  if (!src)
    return 0;

  iDrawUnpackFrame((const unsigned int*)src, dc->w, data, dc->w, dc->w, dc->h);
  return 1;
}

IUP_SDK_API int iupdrvCanvasGetImageData(Ihandle* ih, unsigned char* data, int w, int h)
{
  Ecore_Evas* ee = (Ecore_Evas*)iupAttribGet(ih, "_IUP_EFL_FRAME_EE");
  const void* src;
  int buf_w, buf_h, copy_w, copy_h;

  if (!ee)
    return 0;

  ecore_evas_manual_render(ee);
  src = ecore_evas_buffer_pixels_get(ee);
  if (!src)
    return 0;

  buf_w = iupAttribGetInt(ih, "_IUP_EFL_FRAME_EE_W");
  buf_h = iupAttribGetInt(ih, "_IUP_EFL_FRAME_EE_H");

  copy_w = (w < buf_w) ? w : buf_w;
  copy_h = (h < buf_h) ? h : buf_h;

  iDrawUnpackFrame((const unsigned int*)src, buf_w, data, w, copy_w, copy_h);
  return 1;
}
