/** \file
 * \brief Draw Functions
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
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


typedef struct _IeflVgImageData {
  Efl_VG* node;
  void* pixels;
} IeflVgImageData;

struct _IdrawCanvas
{
  Ihandle* ih;
  int w, h;

  Eo* vg;
  Efl_VG* root;
  Eina_List* shapes;
  Eina_List* evas_objects;  /* Track Evas objects (images) for cleanup */
  Eina_List* vg_images;     /* Track VG image nodes and pixel buffers */
  Efl_VG* clip_node;

  int clip_x1, clip_y1, clip_x2, clip_y2;
  int clip_corner_radius;
};

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

static void iDrawClearChildren(Efl_VG* container)
{
  const Eina_List* children;
  const Eina_List* l;
  Efl_VG* child;
  Eina_List* to_delete = NULL;

  children = efl_canvas_vg_container_children_direct_get(container);
  if (!children)
    return;

  EINA_LIST_FOREACH(children, l, child)
  {
    to_delete = eina_list_append(to_delete, child);
  }

  EINA_LIST_FREE(to_delete, child)
  {
    efl_del(child);
  }
}

static void iDrawClearEvasObjects(Eina_List** list)
{
  Eo* obj;

  if (!list || !*list)
    return;

  EINA_LIST_FREE(*list, obj)
  {
    efl_del(obj);
  }

  *list = NULL;
}

static void iDrawClearVgImages(Eina_List** list)
{
  IeflVgImageData* data;

  if (!list || !*list)
    return;

  EINA_LIST_FREE(*list, data)
  {
    if (data->node)
      efl_del(data->node);
    free(data->pixels);
    free(data);
  }

  *list = NULL;
}

static int iDrawHasClip(IdrawCanvas* dc)
{
  return (dc->clip_x1 > 0 || dc->clip_y1 > 0 || dc->clip_x2 < dc->w - 1 || dc->clip_y2 < dc->h - 1);
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

IUP_SDK_API IdrawCanvas* iupdrvDrawCreateCanvas(Ihandle* ih)
{
  IdrawCanvas* dc = calloc(1, sizeof(IdrawCanvas));
  Eo* vg;
  Efl_VG* root;
  Eina_Size2D size;
  Eina_List* old_evas_objects;
  Eina_List* old_vg_images;

  dc->ih = ih;

  vg = iupeflGetWidget(ih);
  root = (Efl_VG*)iupAttribGet(ih, "_IUP_EFL_VG_ROOT");

  if (!vg || !root)
  {
    free(dc);
    return NULL;
  }

  size = efl_gfx_entity_size_get(vg);
  dc->w = size.w;
  dc->h = size.h;
  dc->vg = vg;
  dc->root = root;
  dc->shapes = NULL;
  dc->evas_objects = NULL;
  dc->vg_images = NULL;
  dc->clip_node = NULL;

  if (iupAttribGet(ih, "_IUP_EFL_SCROLLER"))
  {
    Eina_Matrix3 m;
    double posx = IupGetDouble(ih, "POSX");
    double posy = IupGetDouble(ih, "POSY");
    eina_matrix3_identity(&m);
    eina_matrix3_translate(&m, posx, posy);
    efl_canvas_vg_node_transformation_set(root, &m);
  }

  efl_canvas_vg_object_viewbox_set(vg, EINA_RECT(0, 0, dc->w, dc->h));
  efl_canvas_vg_object_fill_mode_set(vg, EFL_CANVAS_VG_FILL_MODE_NONE);

  old_vg_images = (Eina_List*)iupAttribGet(ih, "_IUP_EFL_VG_IMAGES");
  iDrawClearVgImages(&old_vg_images);
  iupAttribSet(ih, "_IUP_EFL_VG_IMAGES", NULL);

  iDrawClearChildren(root);

  old_evas_objects = (Eina_List*)iupAttribGet(ih, "_IUP_EFL_EVAS_OBJECTS");
  iDrawClearEvasObjects(&old_evas_objects);
  iupAttribSet(ih, "_IUP_EFL_EVAS_OBJECTS", NULL);

  dc->clip_x1 = 0;
  dc->clip_y1 = 0;
  dc->clip_x2 = dc->w - 1;
  dc->clip_y2 = dc->h - 1;

  iupAttribSet(ih, "DRAWDRIVER", "EFL_VG");

  return dc;
}

IUP_SDK_API void iupdrvDrawKillCanvas(IdrawCanvas* dc)
{
  if (dc->shapes)
    eina_list_free(dc->shapes);

  if (dc->evas_objects)
    iupAttribSet(dc->ih, "_IUP_EFL_EVAS_OBJECTS", (char*)dc->evas_objects);

  if (dc->vg_images)
    iupAttribSet(dc->ih, "_IUP_EFL_VG_IMAGES", (char*)dc->vg_images);

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

IUP_SDK_API void iupdrvDrawFlush(IdrawCanvas* dc)
{
  (void)dc;
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

  iDrawGetColor(color, &r, &g, &b, &a);

  shape = efl_add(EFL_CANVAS_VG_SHAPE_CLASS, dc->root,
    efl_gfx_path_append_move_to(efl_added, x1, y1),
    efl_gfx_path_append_line_to(efl_added, x2, y2),
    efl_gfx_shape_stroke_color_set(efl_added, r, g, b, a),
    efl_gfx_shape_stroke_width_set(efl_added, line_width > 0 ? line_width : 1),
    efl_gfx_shape_stroke_cap_set(efl_added, EFL_GFX_CAP_BUTT));

  iDrawSetDash(shape, style);

  dc->shapes = eina_list_append(dc->shapes, shape);
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

  if (iDrawHasClip(dc))
  {
    Ecore_Evas* ee;
    Evas* off_evas;
    Eo* vg;
    Efl_VG* root;
    Efl_VG* vg_image;
    IeflVgImageData* img_data;
    const void* src_pixels;
    unsigned int* pixels;
    int vis_x, vis_y, vis_w, vis_h;
    int px, py;

    vis_x = dc->clip_x1;
    vis_y = dc->clip_y1;
    vis_w = dc->clip_x2 - dc->clip_x1 + 1;
    vis_h = dc->clip_y2 - dc->clip_y1 + 1;

    if (vis_w <= 0 || vis_h <= 0)
      return;

    ee = ecore_evas_buffer_new(dc->w, dc->h);
    if (!ee)
      return;
    ecore_evas_alpha_set(ee, EINA_TRUE);

    off_evas = ecore_evas_get(ee);
    vg = evas_object_vg_add(off_evas);
    evas_object_resize(vg, dc->w, dc->h);
    evas_object_show(vg);

    root = evas_vg_container_add(vg);
    evas_object_vg_root_node_set(vg, root);

    iDrawArcToVg(root, NULL, x1, y1, x2, y2, a1, a2, color, style, line_width);

    ecore_evas_manual_render(ee);
    src_pixels = ecore_evas_buffer_pixels_get(ee);
    if (!src_pixels)
    {
      ecore_evas_free(ee);
      return;
    }

    pixels = (unsigned int*)malloc(vis_w * vis_h * sizeof(unsigned int));
    if (!pixels)
    {
      ecore_evas_free(ee);
      return;
    }

    for (py = 0; py < vis_h; py++)
    {
      const unsigned int* src_line = (const unsigned int*)src_pixels + (vis_y + py) * dc->w;
      unsigned int* dst_line = pixels + py * vis_w;
      for (px = 0; px < vis_w; px++)
      {
        dst_line[px] = src_line[vis_x + px];
      }
    }

    ecore_evas_free(ee);

    vg_image = efl_add(EFL_CANVAS_VG_IMAGE_CLASS, dc->root);
    if (!vg_image)
    {
      free(pixels);
      return;
    }

    {
      Eina_Matrix3 m;
      eina_matrix3_values_set(&m,
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
        (double)vis_x + 0.5, (double)vis_y + 0.5, 1.0);
      efl_canvas_vg_node_transformation_set(vg_image, &m);
    }
    efl_canvas_vg_image_data_set(vg_image, pixels, EINA_SIZE2D(vis_w, vis_h));

    img_data = (IeflVgImageData*)malloc(sizeof(IeflVgImageData));
    img_data->node = vg_image;
    img_data->pixels = pixels;
    dc->vg_images = eina_list_append(dc->vg_images, img_data);
  }
  else
  {
    iDrawArcToVg(dc->root, &dc->shapes, x1, y1, x2, y2, a1, a2, color, style, line_width);
  }
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
  iupdrvDrawRectangle(dc, x, y, x + 1, y + 1, color, IUP_DRAW_FILL, 1);
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

  (void)style;

  iDrawGetColor(color, &r, &g, &b, &a);

  shape = efl_add(EFL_CANVAS_VG_SHAPE_CLASS, dc->root,
    efl_gfx_path_append_move_to(efl_added, x1, y1),
    efl_gfx_path_append_cubic_to(efl_added, x2, y2, x3, y3, x4, y4),
    efl_gfx_shape_stroke_color_set(efl_added, r, g, b, a),
    efl_gfx_shape_stroke_width_set(efl_added, line_width > 0 ? line_width : 1));

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

IUP_SDK_API void iupdrvDrawLinearGradient(IdrawCanvas* dc, int x1, int y1, int x2, int y2, float angle, long color1, long color2)
{
  Efl_VG* shape;
  Efl_Canvas_Vg_Gradient* grad;
  Efl_Gfx_Gradient_Stop stops[2];
  int r1, g1, b1, a1;
  int r2, g2, b2, a2;
  int corner_radius = dc->clip_corner_radius;

  (void)angle;

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  iDrawGetColor(color1, &r1, &g1, &b1, &a1);
  iDrawGetColor(color2, &r2, &g2, &b2, &a2);

  shape = efl_add(EFL_CANVAS_VG_SHAPE_CLASS, dc->root,
    efl_gfx_path_append_rect(efl_added, x1, y1, x2 - x1 + 1, y2 - y1 + 1, corner_radius, corner_radius));

  grad = efl_add(EFL_CANVAS_VG_GRADIENT_LINEAR_CLASS, dc->root,
    efl_gfx_gradient_linear_start_set(efl_added, x1, y1),
    efl_gfx_gradient_linear_end_set(efl_added, x2, y2));

  stops[0].offset = 0.0;
  stops[0].r = r1; stops[0].g = g1; stops[0].b = b1; stops[0].a = a1;
  stops[1].offset = 1.0;
  stops[1].r = r2; stops[1].g = g2; stops[1].b = b2; stops[1].a = a2;
  efl_gfx_gradient_stop_set(grad, stops, 2);

  efl_canvas_vg_shape_fill_set(shape, grad);

  dc->shapes = eina_list_append(dc->shapes, shape);
  dc->shapes = eina_list_append(dc->shapes, grad);
}

IUP_SDK_API void iupdrvDrawRadialGradient(IdrawCanvas* dc, int cx, int cy, int radius, long colorCenter, long colorEdge)
{
  Efl_VG* shape;
  Efl_Canvas_Vg_Gradient* grad;
  Efl_Gfx_Gradient_Stop stops[2];
  int r1, g1, b1, a1;
  int r2, g2, b2, a2;

  iDrawGetColor(colorCenter, &r1, &g1, &b1, &a1);
  iDrawGetColor(colorEdge, &r2, &g2, &b2, &a2);

  shape = efl_add(EFL_CANVAS_VG_SHAPE_CLASS, dc->root,
    efl_gfx_path_append_circle(efl_added, cx, cy, radius));

  grad = efl_add(EFL_CANVAS_VG_GRADIENT_RADIAL_CLASS, dc->root,
    efl_gfx_gradient_radial_center_set(efl_added, cx, cy),
    efl_gfx_gradient_radial_radius_set(efl_added, radius));

  stops[0].offset = 0.0;
  stops[0].r = r1; stops[0].g = g1; stops[0].b = b1; stops[0].a = a1;
  stops[1].offset = 1.0;
  stops[1].r = r2; stops[1].g = g2; stops[1].b = b2; stops[1].a = a2;
  efl_gfx_gradient_stop_set(grad, stops, 2);

  efl_canvas_vg_shape_fill_set(shape, grad);

  dc->shapes = eina_list_append(dc->shapes, shape);
  dc->shapes = eina_list_append(dc->shapes, grad);
}

IUP_SDK_API void iupdrvDrawText(IdrawCanvas* dc, const char* text, int len, int x, int y, int w, int h, long color, const char* font, int flags, double text_orientation)
{
  Ecore_Evas* ee;
  Evas* off_evas;
  Evas_Object* text_obj;
  Efl_VG* vg_image;
  IeflVgImageData* img_data;
  const void* src_pixels;
  unsigned int* pixels;
  int r, g, b, a;
  int fontsize = 12;
  int tw, th;
  int buf_w, buf_h;
  int draw_x, draw_y;
  int text_x;
  char typeface[256] = "Sans";
  int bold = 0, italic = 0, underline = 0, strikeout = 0;
  int layout_center = flags & IUP_DRAW_LAYOUTCENTER;
  char* text_copy = NULL;

  (void)text_orientation;

  if (!text || !text[0])
    return;

  iDrawGetColor(color, &r, &g, &b, &a);

  if (font)
    iupFontParsePango(font, typeface, &fontsize, &bold, &italic, &underline, &strikeout);

  if (fontsize <= 0)
    fontsize = 12;

  if (len > 0 && len < (int)strlen(text))
  {
    text_copy = (char*)malloc(len + 1);
    memcpy(text_copy, text, len);
    text_copy[len] = '\0';
    text = text_copy;
  }

  if ((flags & IUP_DRAW_WRAP) || (flags & IUP_DRAW_ELLIPSIS))
  {
    buf_w = w > 0 ? w : 1000;
    buf_h = h > 0 ? h : 1000;
  }
  else
  {
    iupdrvFontGetTextSize(font, text, len, &tw, &th);
    if (tw <= 0 || th <= 0)
    {
      if (text_copy) free(text_copy);
      return;
    }

    if ((flags & IUP_DRAW_CLIP) && w > 0 && h > 0)
    {
      buf_w = w;
      buf_h = h;
    }
    else
    {
      buf_w = tw;
      buf_h = th;
    }
  }

  if (buf_w <= 0 || buf_h <= 0)
  {
    if (text_copy) free(text_copy);
    return;
  }

  ee = ecore_evas_buffer_new(buf_w, buf_h);
  if (!ee)
  {
    if (text_copy) free(text_copy);
    return;
  }
  ecore_evas_alpha_set(ee, EINA_TRUE);

  off_evas = ecore_evas_get(ee);

  if ((flags & IUP_DRAW_WRAP) || (flags & IUP_DRAW_ELLIPSIS))
  {
    Evas_Textblock_Style* ts;
    char style[512];
    const char* align_str = "left";

    if (flags & IUP_DRAW_RIGHT)
      align_str = "right";
    else if (flags & IUP_DRAW_CENTER)
      align_str = "center";

    text_obj = evas_object_textblock_add(off_evas);
    ts = evas_textblock_style_new();

    snprintf(style, sizeof(style),
        "DEFAULT='font=%s%s%s font_size=%d color=#%02X%02X%02X%02X "
        "wrap=%s ellipsis=%s align=%s'",
        typeface,
        bold ? ":style=Bold" : "",
        italic ? ":style=Italic" : "",
        fontsize,
        r, g, b, a,
        (flags & IUP_DRAW_WRAP) ? "word" : "none",
        (flags & IUP_DRAW_ELLIPSIS) ? "1.0" : "-1.0",
        align_str);

    evas_textblock_style_set(ts, style);
    evas_object_textblock_style_set(text_obj, ts);
    evas_object_textblock_text_markup_set(text_obj, text);
    evas_object_resize(text_obj, buf_w, buf_h);
    evas_object_move(text_obj, 0, 0);
    evas_object_show(text_obj);

    evas_textblock_style_free(ts);
  }
  else
  {
    char font_with_style[300];

    text_obj = evas_object_text_add(off_evas);

    snprintf(font_with_style, sizeof(font_with_style), "%s%s%s",
        typeface,
        bold ? ":style=Bold" : "",
        italic ? ":style=Italic" : "");

    evas_object_text_font_set(text_obj, font_with_style, fontsize);
    evas_object_text_text_set(text_obj, text);
    evas_object_color_set(text_obj, r, g, b, a);

    evas_object_geometry_get(text_obj, NULL, NULL, &tw, &th);

    text_x = 0;
    if (flags & IUP_DRAW_RIGHT)
      text_x = buf_w - tw;
    else if (flags & IUP_DRAW_CENTER)
      text_x = (buf_w - tw) / 2;

    evas_object_move(text_obj, text_x, 0);
    evas_object_show(text_obj);
  }

  ecore_evas_manual_render(ee);

  src_pixels = ecore_evas_buffer_pixels_get(ee);
  if (!src_pixels)
  {
    ecore_evas_free(ee);
    if (text_copy) free(text_copy);
    return;
  }

  pixels = (unsigned int*)malloc(buf_w * buf_h * sizeof(unsigned int));
  if (!pixels)
  {
    ecore_evas_free(ee);
    if (text_copy) free(text_copy);
    return;
  }
  memcpy(pixels, src_pixels, buf_w * buf_h * sizeof(unsigned int));

  ecore_evas_free(ee);

  draw_x = x;
  draw_y = y;

  if (layout_center && w > 0 && h > 0)
  {
    draw_x = x + (w - buf_w) / 2;
    draw_y = y + (h - buf_h) / 2;
  }

  vg_image = efl_add(EFL_CANVAS_VG_IMAGE_CLASS, dc->root);
  if (!vg_image)
  {
    free(pixels);
    if (text_copy) free(text_copy);
    return;
  }

  {
    Eina_Matrix3 m;
    eina_matrix3_values_set(&m,
      1.0, 0.0, 0.0,
      0.0, 1.0, 0.0,
      (double)draw_x + 0.5, (double)draw_y + 0.5, 1.0);
    efl_canvas_vg_node_transformation_set(vg_image, &m);
  }
  efl_canvas_vg_image_data_set(vg_image, pixels, EINA_SIZE2D(buf_w, buf_h));

  img_data = (IeflVgImageData*)malloc(sizeof(IeflVgImageData));
  img_data->node = vg_image;
  img_data->pixels = pixels;
  dc->vg_images = eina_list_append(dc->vg_images, img_data);

  if (text_copy)
    free(text_copy);
}

IUP_SDK_API void iupdrvDrawImage(IdrawCanvas* dc, const char* name, int make_inactive, const char* bgcolor, int x, int y, int w, int h)
{
  Efl_VG* vg_image;
  IeflVgImageData* img_data;
  Ihandle* img_ih;
  unsigned char* imgdata;
  unsigned int* pixels;
  int img_w, img_h, bpp;
  int colors_count = 0, has_alpha = 0;
  iupColor colors[256];
  unsigned char bg_r = 0, bg_g = 0, bg_b = 0;
  int vis_x, vis_y, vis_w, vis_h;
  double scale_x, scale_y;

  img_ih = iupImageGetImageFromName(name);
  if (!img_ih)
    return;

  img_w = img_ih->currentwidth;
  img_h = img_ih->currentheight;
  bpp = iupAttribGetInt(img_ih, "BPP");

  if (bpp == 8)
    has_alpha = iupImageInitColorTable(img_ih, colors, &colors_count);
  else if (bpp == 32)
    has_alpha = 1;

  if (make_inactive && bgcolor)
    iupStrToRGB(bgcolor, &bg_r, &bg_g, &bg_b);

  if (w <= 0) w = img_w;
  if (h <= 0) h = img_h;

  vis_x = (x > dc->clip_x1) ? x : dc->clip_x1;
  vis_y = (y > dc->clip_y1) ? y : dc->clip_y1;
  vis_w = (((x + w) < (dc->clip_x2 + 1)) ? (x + w) : (dc->clip_x2 + 1)) - vis_x;
  vis_h = (((y + h) < (dc->clip_y2 + 1)) ? (y + h) : (dc->clip_y2 + 1)) - vis_y;

  if (vis_w <= 0 || vis_h <= 0)
    return;

  scale_x = (double)img_w / (double)w;
  scale_y = (double)img_h / (double)h;

  pixels = (unsigned int*)malloc(vis_w * vis_h * sizeof(unsigned int));
  if (!pixels)
    return;

  imgdata = (unsigned char*)iupAttribGetStr(img_ih, "WID");

  if (bpp == 8)
  {
    int px, py;

    if (make_inactive)
    {
      int i;
      for (i = 0; i < colors_count; i++)
      {
        if (colors[i].a == 0)
        {
          colors[i].r = bg_r;
          colors[i].g = bg_g;
          colors[i].b = bg_b;
          colors[i].a = 255;
        }
        iupImageColorMakeInactive(&colors[i].r, &colors[i].g, &colors[i].b, bg_r, bg_g, bg_b);
      }
    }

    for (py = 0; py < vis_h; py++)
    {
      unsigned int* pix_line = pixels + py * vis_w;
      int src_y = (int)((vis_y - y + py) * scale_y);
      if (src_y >= img_h) src_y = img_h - 1;

      for (px = 0; px < vis_w; px++)
      {
        int src_x = (int)((vis_x - x + px) * scale_x);
        unsigned char index;
        iupColor* c;
        unsigned char pa, pr, pg, pb;

        if (src_x >= img_w) src_x = img_w - 1;

        index = imgdata[src_y * img_w + src_x];
        c = &colors[index];
        pa = c->a;
        pr = (c->r * pa) / 255;
        pg = (c->g * pa) / 255;
        pb = (c->b * pa) / 255;
        pix_line[px] = (pa << 24) | (pr << 16) | (pg << 8) | pb;
      }
    }
  }
  else
  {
    int channels = (bpp == 32) ? 4 : 3;
    int px, py;

    for (py = 0; py < vis_h; py++)
    {
      unsigned int* pix_line = pixels + py * vis_w;
      int src_y = (int)((vis_y - y + py) * scale_y);
      if (src_y >= img_h) src_y = img_h - 1;

      for (px = 0; px < vis_w; px++)
      {
        int src_x = (int)((vis_x - x + px) * scale_x);
        unsigned char r, g, b, a;
        unsigned char pr, pg, pb;

        if (src_x >= img_w) src_x = img_w - 1;

        r = imgdata[(src_y * img_w + src_x) * channels];
        g = imgdata[(src_y * img_w + src_x) * channels + 1];
        b = imgdata[(src_y * img_w + src_x) * channels + 2];
        a = (bpp == 32) ? imgdata[(src_y * img_w + src_x) * channels + 3] : 255;

        if (make_inactive)
        {
          if (a == 0 && has_alpha)
          {
            r = bg_r;
            g = bg_g;
            b = bg_b;
            a = 255;
          }
          iupImageColorMakeInactive(&r, &g, &b, bg_r, bg_g, bg_b);
        }

        pr = (r * a) / 255;
        pg = (g * a) / 255;
        pb = (b * a) / 255;
        pix_line[px] = (a << 24) | (pr << 16) | (pg << 8) | pb;
      }
    }
  }

  vg_image = efl_add(EFL_CANVAS_VG_IMAGE_CLASS, dc->root);
  if (!vg_image)
  {
    free(pixels);
    return;
  }

  {
    Eina_Matrix3 m;
    eina_matrix3_values_set(&m,
      1.0, 0.0, 0.0,
      0.0, 1.0, 0.0,
      (double)vis_x + 0.5, (double)vis_y + 0.5, 1.0);
    efl_canvas_vg_node_transformation_set(vg_image, &m);
  }
  efl_canvas_vg_image_data_set(vg_image, pixels, EINA_SIZE2D(vis_w, vis_h));

  img_data = (IeflVgImageData*)malloc(sizeof(IeflVgImageData));
  img_data->node = vg_image;
  img_data->pixels = pixels;
  dc->vg_images = eina_list_append(dc->vg_images, img_data);
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
