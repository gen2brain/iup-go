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

  Eo* vg;
  Efl_VG* root;
  Eina_List* shapes;
  Eina_List* evas_objects;  /* Track Evas objects (images) for cleanup */
  Eina_List* vg_images;     /* Track VG image nodes and pixel buffers */
  Efl_VG* clip_node;

  int clip_x1, clip_y1, clip_x2, clip_y2;
  int clip_corner_radius;

  Ecore_Evas* offscreen_ee;
  int offscreen_w, offscreen_h;
};

static Evas* iDrawGetOffscreen(IdrawCanvas* dc, int w, int h)
{
  if (!dc->offscreen_ee)
  {
    dc->offscreen_ee = ecore_evas_buffer_new(w, h);
    if (!dc->offscreen_ee)
      return NULL;
    ecore_evas_alpha_set(dc->offscreen_ee, EINA_TRUE);
    dc->offscreen_w = w;
    dc->offscreen_h = h;
  }
  else if (dc->offscreen_w != w || dc->offscreen_h != h)
  {
    ecore_evas_resize(dc->offscreen_ee, w, h);
    dc->offscreen_w = w;
    dc->offscreen_h = h;
  }
  return ecore_evas_get(dc->offscreen_ee);
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

static void iDrawFreeVgImagePixels(Eina_List** list)
{
  IeflVgImageData* data;

  if (!list || !*list)
    return;

  EINA_LIST_FREE(*list, data)
  {
    free(data->pixels);
    free(data);
  }

  *list = NULL;
}

static int iDrawHasClip(IdrawCanvas* dc)
{
  return (dc->clip_x1 > 0 || dc->clip_y1 > 0 || dc->clip_x2 < dc->w - 1 || dc->clip_y2 < dc->h - 1);
}

static Efl_VG* iDrawCloneTree(Efl_VG* node, Efl_VG* new_parent, Eina_List* vg_images)
{
  if (efl_isa(node, EFL_CANVAS_VG_IMAGE_CLASS))
  {
    Efl_VG* clone = efl_add(EFL_CANVAS_VG_IMAGE_CLASS, new_parent);
    const Eina_Matrix3* m = efl_canvas_vg_node_transformation_get(node);
    if (m)
      efl_canvas_vg_node_transformation_set(clone, m);
    {
      int r, g, b, a;
      efl_gfx_color_get(node, &r, &g, &b, &a);
      efl_gfx_color_set(clone, r, g, b, a);
    }
    efl_gfx_entity_visible_set(clone, efl_gfx_entity_visible_get(node));

    if (vg_images)
    {
      Eina_List* l;
      IeflVgImageData* img;
      EINA_LIST_FOREACH(vg_images, l, img)
      {
        if (img->node == node)
        {
          efl_canvas_vg_image_data_set(clone, img->pixels, EINA_SIZE2D(img->w, img->h));
          break;
        }
      }
    }
    return clone;
  }

  if (efl_isa(node, EFL_CANVAS_VG_CONTAINER_CLASS))
  {
    Efl_VG* clone = efl_add(EFL_CANVAS_VG_CONTAINER_CLASS, new_parent);
    const Eina_Matrix3* m = efl_canvas_vg_node_transformation_get(node);
    if (m)
      efl_canvas_vg_node_transformation_set(clone, m);

    {
      const Eina_List* children = efl_canvas_vg_container_children_direct_get(node);
      const Eina_List* l;
      Efl_VG* child;
      EINA_LIST_FOREACH(children, l, child)
        iDrawCloneTree(child, clone, vg_images);
    }
    return clone;
  }

  {
    Efl_VG* clone = efl_duplicate(node);
    efl_parent_set(clone, new_parent);
    return clone;
  }
}

static int iDrawRenderToBuffer(IdrawCanvas* dc, unsigned char* data)
{
  Ecore_Evas* export_ee;
  Evas* off_evas;
  Eo* off_vg;
  Efl_VG* cloned_root;
  const void* src_pixels;
  int x, y;

  export_ee = ecore_evas_buffer_new(dc->w, dc->h);
  if (!export_ee)
    return 0;
  ecore_evas_alpha_set(export_ee, EINA_TRUE);
  off_evas = ecore_evas_get(export_ee);

  off_vg = efl_add(EFL_CANVAS_VG_OBJECT_CLASS, off_evas);
  if (!off_vg)
  {
    ecore_evas_free(export_ee);
    return 0;
  }

  evas_object_move(off_vg, 0, 0);
  evas_object_resize(off_vg, dc->w, dc->h);
  efl_canvas_vg_object_viewbox_set(off_vg, EINA_RECT(0, 0, dc->w, dc->h));
  efl_canvas_vg_object_fill_mode_set(off_vg, EFL_CANVAS_VG_FILL_MODE_NONE);

  cloned_root = efl_add(EFL_CANVAS_VG_CONTAINER_CLASS, off_vg);
  {
    const Eina_Matrix3* m = efl_canvas_vg_node_transformation_get(dc->root);
    if (m)
      efl_canvas_vg_node_transformation_set(cloned_root, m);
  }

  {
    const Eina_List* children = efl_canvas_vg_container_children_direct_get(dc->root);
    const Eina_List* l;
    Efl_VG* child;
    EINA_LIST_FOREACH(children, l, child)
    {
      if (!efl_isa(child, EFL_CANVAS_VG_IMAGE_CLASS))
        iDrawCloneTree(child, cloned_root, dc->vg_images);
    }
  }

  efl_canvas_vg_object_root_node_set(off_vg, cloned_root);
  evas_object_show(off_vg);

  {
    Eina_List* l;
    IeflVgImageData* img;
    Eina_Matrix3 root_m;
    const Eina_Matrix3* rm = efl_canvas_vg_node_transformation_get(dc->root);
    if (rm)
      eina_matrix3_copy(&root_m, rm);
    else
      eina_matrix3_identity(&root_m);

    EINA_LIST_FOREACH(dc->vg_images, l, img)
    {
      Evas_Object* evas_img;
      const Eina_Matrix3* node_m;
      Eina_Matrix3 abs_m;
      double m00, m01, m02, m10, m11, m12, m20, m21, m22;
      double cx[4], cy[4];
      double sx[4] = {0, (double)img->w, (double)img->w, 0};
      double sy[4] = {0, 0, (double)img->h, (double)img->h};
      int i;
      Evas_Map* map;

      node_m = efl_canvas_vg_node_transformation_get(img->node);
      if (node_m)
        eina_matrix3_compose(&root_m, node_m, &abs_m);
      else
        eina_matrix3_copy(&abs_m, &root_m);

      eina_matrix3_values_get(&abs_m,
        &m00, &m01, &m02,
        &m10, &m11, &m12,
        &m20, &m21, &m22);

      for (i = 0; i < 4; i++)
      {
        cx[i] = sx[i] * m00 + sy[i] * m10 + m20;
        cy[i] = sx[i] * m01 + sy[i] * m11 + m21;
      }

      evas_img = evas_object_image_filled_add(off_evas);
      evas_object_image_alpha_set(evas_img, EINA_TRUE);
      evas_object_image_size_set(evas_img, img->w, img->h);
      {
        void* dst = evas_object_image_data_get(evas_img, EINA_TRUE);
        if (dst)
        {
          memcpy(dst, img->pixels, img->w * img->h * sizeof(unsigned int));
          evas_object_image_data_set(evas_img, dst);
          evas_object_image_data_update_add(evas_img, 0, 0, img->w, img->h);
        }
      }
      evas_object_resize(evas_img, img->w, img->h);

      map = evas_map_new(4);
      for (i = 0; i < 4; i++)
      {
        evas_map_point_coord_set(map, i, (int)(cx[i] + 0.5), (int)(cy[i] + 0.5), 0);
        evas_map_point_image_uv_set(map, i, sx[i], sy[i]);
      }
      evas_object_map_set(evas_img, map);
      evas_object_map_enable_set(evas_img, EINA_TRUE);
      evas_map_free(map);

      evas_object_show(evas_img);
    }
  }

  ecore_evas_manual_render(export_ee);

  src_pixels = ecore_evas_buffer_pixels_get(export_ee);

  if (!src_pixels)
  {
    ecore_evas_free(export_ee);
    return 0;
  }

  for (y = 0; y < dc->h; y++)
  {
    const unsigned int* src_line = (const unsigned int*)src_pixels + y * dc->w;
    unsigned char* dst_line = data + y * dc->w * 4;
    for (x = 0; x < dc->w; x++)
    {
      unsigned int pixel = src_line[x];
      unsigned char a = (unsigned char)((pixel >> 24) & 0xFF);
      unsigned char r = (unsigned char)((pixel >> 16) & 0xFF);
      unsigned char g = (unsigned char)((pixel >> 8) & 0xFF);
      unsigned char b = (unsigned char)((pixel >> 0) & 0xFF);

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

  ecore_evas_free(export_ee);

  return 1;
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

  /* Free deferred data from the previous-previous frame. */
  {
    Efl_VG* deferred_root = (Efl_VG*)iupAttribGet(ih, "_IUP_EFL_VG_ROOT_DEFERRED");
    Eina_List* deferred_vg_images = (Eina_List*)iupAttribGet(ih, "_IUP_EFL_VG_IMAGES_DEFERRED");
    Eina_List* deferred_evas_objects = (Eina_List*)iupAttribGet(ih, "_IUP_EFL_EVAS_OBJECTS_DEFERRED");

    if (deferred_root)
      efl_del(deferred_root);
    iDrawFreeVgImagePixels(&deferred_vg_images);
    iDrawClearEvasObjects(&deferred_evas_objects);

    iupAttribSet(ih, "_IUP_EFL_VG_ROOT_DEFERRED", NULL);
    iupAttribSet(ih, "_IUP_EFL_VG_IMAGES_DEFERRED", NULL);
    iupAttribSet(ih, "_IUP_EFL_EVAS_OBJECTS_DEFERRED", NULL);
  }

  /* Defer current scene graph data, the render thread might still be using it.
     VG image nodes and their ector buffers must stay alive until the render thread finishes. */
  {
    Efl_VG* old_root = (Efl_VG*)iupAttribGet(ih, "_IUP_EFL_VG_ROOT");
    Eina_List* old_vg_images = (Eina_List*)iupAttribGet(ih, "_IUP_EFL_VG_IMAGES");
    Eina_List* old_evas_objects = (Eina_List*)iupAttribGet(ih, "_IUP_EFL_EVAS_OBJECTS");

    iupAttribSet(ih, "_IUP_EFL_VG_ROOT_DEFERRED", (char*)old_root);
    iupAttribSet(ih, "_IUP_EFL_VG_IMAGES_DEFERRED", (char*)old_vg_images);
    iupAttribSet(ih, "_IUP_EFL_EVAS_OBJECTS_DEFERRED", (char*)old_evas_objects);

    iupAttribSet(ih, "_IUP_EFL_VG_ROOT", NULL);
    iupAttribSet(ih, "_IUP_EFL_VG_IMAGES", NULL);
    iupAttribSet(ih, "_IUP_EFL_EVAS_OBJECTS", NULL);
  }

  /* Create a new root container for this frame.
     The old root remains as a child of vg but is no longer the VG root,
     so it won't be rendered. It stays alive for the render thread. */
  root = efl_add(EFL_CANVAS_VG_CONTAINER_CLASS, vg);
  if (!root)
  {
    free(dc);
    return NULL;
  }
  efl_canvas_vg_object_root_node_set(vg, root);
  iupAttribSet(ih, "_IUP_EFL_VG_ROOT", (char*)root);

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

  if (dc->offscreen_ee)
    ecore_evas_free(dc->offscreen_ee);

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
  int isCanvas = iupClassMatch(dc->ih->iclass, "canvas");
  if (isCanvas)
  {
    unsigned char* old_buffer = (unsigned char*)iupAttribGet(dc->ih, "_IUP_EFL_CANVAS_BUFFER");
    unsigned char* buffer;
    int size = dc->w * dc->h * 4;

    if (old_buffer)
    {
      free(old_buffer);
      iupAttribSet(dc->ih, "_IUP_EFL_CANVAS_BUFFER", NULL);
    }

    buffer = (unsigned char*)malloc(size);
    if (buffer)
    {
      if (iDrawRenderToBuffer(dc, buffer))
      {
        iupAttribSet(dc->ih, "_IUP_EFL_CANVAS_BUFFER", (char*)buffer);
        iupAttribSetInt(dc->ih, "_IUP_EFL_CANVAS_BUFFER_W", dc->w);
        iupAttribSetInt(dc->ih, "_IUP_EFL_CANVAS_BUFFER_H", dc->h);
      }
      else
        free(buffer);
    }
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
    Evas* off_evas;
    Eo* vg;
    Efl_VG* root;
    Evas_Object* bg;
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

    off_evas = iDrawGetOffscreen(dc, dc->w, dc->h);
    if (!off_evas)
      return;

    bg = evas_object_rectangle_add(off_evas);
    evas_object_color_set(bg, 0, 0, 0, 0);
    evas_object_render_op_set(bg, EVAS_RENDER_COPY);
    evas_object_move(bg, 0, 0);
    evas_object_resize(bg, dc->w, dc->h);
    evas_object_show(bg);

    vg = evas_object_vg_add(off_evas);
    evas_object_resize(vg, dc->w, dc->h);
    evas_object_show(vg);

    root = evas_vg_container_add(vg);
    evas_object_vg_root_node_set(vg, root);

    iDrawArcToVg(root, NULL, x1, y1, x2, y2, a1, a2, color, style, line_width);

    ecore_evas_manual_render(dc->offscreen_ee);
    src_pixels = ecore_evas_buffer_pixels_get(dc->offscreen_ee);
    if (!src_pixels)
    {
      efl_del(vg);
      efl_del(bg);
      return;
    }

    pixels = (unsigned int*)malloc(vis_w * vis_h * sizeof(unsigned int));
    if (!pixels)
    {
      efl_del(vg);
      efl_del(bg);
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

    efl_del(vg);
    efl_del(bg);

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
    img_data->w = vis_w;
    img_data->h = vis_h;
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

IUP_SDK_API void iupdrvDrawLinearGradient(IdrawCanvas* dc, int x1, int y1, int x2, int y2, float angle, long color1, long color2)
{
  Efl_VG* shape;
  Efl_Canvas_Vg_Gradient* grad;
  Efl_Gfx_Gradient_Stop stops[2];
  int r1, g1, b1, a1;
  int r2, g2, b2, a2;
  int corner_radius = dc->clip_corner_radius;
  double rad, w, h, gx0, gy0, gx1, gy1;

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  iDrawGetColor(color1, &r1, &g1, &b1, &a1);
  iDrawGetColor(color2, &r2, &g2, &b2, &a2);

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

  off_evas = iDrawGetOffscreen(dc, buf_w, buf_h);
  if (!off_evas)
  {
    if (text_copy) free(text_copy);
    return;
  }

  {
    Evas_Object* bg = evas_object_rectangle_add(off_evas);
    evas_object_color_set(bg, 0, 0, 0, 0);
    evas_object_render_op_set(bg, EVAS_RENDER_COPY);
    evas_object_move(bg, 0, 0);
    evas_object_resize(bg, buf_w, buf_h);
    evas_object_show(bg);

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

    ecore_evas_manual_render(dc->offscreen_ee);

    src_pixels = ecore_evas_buffer_pixels_get(dc->offscreen_ee);
    if (!src_pixels)
    {
      efl_del(text_obj);
      efl_del(bg);
      if (text_copy) free(text_copy);
      return;
    }

    pixels = (unsigned int*)malloc(buf_w * buf_h * sizeof(unsigned int));
    if (!pixels)
    {
      efl_del(text_obj);
      efl_del(bg);
      if (text_copy) free(text_copy);
      return;
    }
    memcpy(pixels, src_pixels, buf_w * buf_h * sizeof(unsigned int));

    efl_del(text_obj);
    efl_del(bg);
  }

  draw_x = x;
  draw_y = y;

  if (layout_center && w > 0 && h > 0)
  {
    draw_x = x + (w - buf_w) / 2;
    draw_y = y + (h - buf_h) / 2;
  }

  if (text_orientation == 0)
  {
    int vis_x = (draw_x > dc->clip_x1) ? draw_x : dc->clip_x1;
    int vis_y = (draw_y > dc->clip_y1) ? draw_y : dc->clip_y1;
    int vis_w = (((draw_x + buf_w) < (dc->clip_x2 + 1)) ? (draw_x + buf_w) : (dc->clip_x2 + 1)) - vis_x;
    int vis_h = (((draw_y + buf_h) < (dc->clip_y2 + 1)) ? (draw_y + buf_h) : (dc->clip_y2 + 1)) - vis_y;

    if (vis_w <= 0 || vis_h <= 0)
    {
      free(pixels);
      if (text_copy) free(text_copy);
      return;
    }

    if (vis_x != draw_x || vis_y != draw_y || vis_w != buf_w || vis_h != buf_h)
    {
      unsigned int* clipped = (unsigned int*)malloc(vis_w * vis_h * sizeof(unsigned int));
      if (!clipped)
      {
        free(pixels);
        if (text_copy) free(text_copy);
        return;
      }
      {
        int sx = vis_x - draw_x;
        int sy = vis_y - draw_y;
        int py;
        for (py = 0; py < vis_h; py++)
          memcpy(clipped + py * vis_w, pixels + (sy + py) * buf_w + sx, vis_w * sizeof(unsigned int));
      }
      free(pixels);
      pixels = clipped;
      draw_x = vis_x;
      draw_y = vis_y;
      buf_w = vis_w;
      buf_h = vis_h;
    }
  }
  else
  {
    double angle_rad = -text_orientation * M_PI / 180.0;
    double ca = cos(angle_rad), sa = sin(angle_rad);
    double tx, ty;
    double bx[4], by[4];
    double min_x, min_y;
    int i;

    if (layout_center && w > 0 && h > 0)
    {
      double ctr_x = x + w / 2.0, ctr_y = y + h / 2.0;
      tx = ctr_x + ca * (-buf_w / 2.0) - sa * (-buf_h / 2.0);
      ty = ctr_y + sa * (-buf_w / 2.0) + ca * (-buf_h / 2.0);
    }
    else
    {
      tx = (double)draw_x;
      ty = (double)draw_y;
    }

    bx[0] = tx;                            by[0] = ty;
    bx[1] = buf_w * ca + tx;               by[1] = buf_w * (-sa) + ty;
    bx[2] = buf_h * sa + tx;               by[2] = buf_h * ca + ty;
    bx[3] = buf_w * ca + buf_h * sa + tx;  by[3] = buf_w * (-sa) + buf_h * ca + ty;

    min_x = bx[0]; min_y = by[0];
    for (i = 1; i < 4; i++)
    {
      if (bx[i] < min_x) min_x = bx[i];
      if (by[i] < min_y) min_y = by[i];
    }

    if (min_x < 0 || min_y < 0)
    {
      free(pixels);
      if (text_copy) free(text_copy);
      return;
    }
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
    if (text_orientation != 0)
    {
      double angle_rad = -text_orientation * M_PI / 180.0;
      double cos_a = cos(angle_rad);
      double sin_a = sin(angle_rad);

      if (layout_center && w > 0 && h > 0)
      {
        double cx = x + w / 2.0;
        double cy = y + h / 2.0;
        double ox = -buf_w / 2.0;
        double oy = -buf_h / 2.0;
        eina_matrix3_values_set(&m,
          cos_a, -sin_a, 0.0,
          sin_a,  cos_a, 0.0,
          cx + cos_a * ox - sin_a * oy,
          cy + sin_a * ox + cos_a * oy,
          1.0);
      }
      else
      {
        eina_matrix3_values_set(&m,
          cos_a, -sin_a, 0.0,
          sin_a,  cos_a, 0.0,
          (double)draw_x, (double)draw_y, 1.0);
      }
    }
    else
    {
      eina_matrix3_values_set(&m,
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
        (double)draw_x + 0.5, (double)draw_y + 0.5, 1.0);
    }
    efl_canvas_vg_node_transformation_set(vg_image, &m);
  }
  efl_canvas_vg_image_data_set(vg_image, pixels, EINA_SIZE2D(buf_w, buf_h));

  img_data = (IeflVgImageData*)malloc(sizeof(IeflVgImageData));
  img_data->node = vg_image;
  img_data->pixels = pixels;
  img_data->w = buf_w;
  img_data->h = buf_h;
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
  img_data->w = vis_w;
  img_data->h = vis_h;
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

IUP_SDK_API int iupdrvDrawGetImageData(IdrawCanvas* dc, unsigned char* data)
{
  return iDrawRenderToBuffer(dc, data);
}

IUP_SDK_API int iupdrvCanvasGetImageData(Ihandle* ih, unsigned char* data, int w, int h)
{
  unsigned char* buffer = (unsigned char*)iupAttribGet(ih, "_IUP_EFL_CANVAS_BUFFER");
  int buf_w, buf_h, copy_w, copy_h, y;

  if (!buffer)
    return 0;

  buf_w = iupAttribGetInt(ih, "_IUP_EFL_CANVAS_BUFFER_W");
  buf_h = iupAttribGetInt(ih, "_IUP_EFL_CANVAS_BUFFER_H");

  copy_w = (w < buf_w) ? w : buf_w;
  copy_h = (h < buf_h) ? h : buf_h;

  for (y = 0; y < copy_h; y++)
    memcpy(data + y * w * 4, buffer + y * buf_w * 4, copy_w * 4);

  return 1;
}
