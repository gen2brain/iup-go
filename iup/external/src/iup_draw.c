/** \file
 * \brief Canvas Control.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "iup.h"
#include "iupdraw.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_drvdraw.h"
#include "iup_draw.h"
#include "iup_draw_svg.h"
#include "iup_assert.h"
#include "iup_image.h"


#define IUP_SVG_GET(ih) ((iSvgCanvas*)iupAttribGet(ih, "_IUP_SVG_CANVAS"))

static void iSvgColorStr(long color, char* buf, int buf_size)
{
  unsigned char r = iupDrawRed(color);
  unsigned char g = iupDrawGreen(color);
  unsigned char b = iupDrawBlue(color);
  unsigned char a = iupDrawAlpha(color);
  if (a >= 255)
    snprintf(buf, buf_size, "%d %d %d", r, g, b);
  else
    snprintf(buf, buf_size, "%d %d %d %d", r, g, b, a);
}

typedef struct _IupDrawPathData
{
  IupPathSeg* segs;
  int count, cap;
  int has_current;
  int cur_x, cur_y;
  int sub_x, sub_y;
} IupDrawPathData;

enum { IUP_DRAW_CLIP_NONE, IUP_DRAW_CLIP_RECT, IUP_DRAW_CLIP_ROUNDED, IUP_DRAW_CLIP_PATH };

static const char* iDrawStateAttribNames[] =
{
  "DRAWFONT", "DRAWCOLOR", "DRAWSTYLE", "DRAWTEXTALIGNMENT",
  "DRAWTEXTWRAP", "DRAWTEXTELLIPSIS", "DRAWTEXTCLIP", "DRAWTEXTORIENTATION",
  "DRAWTEXTLAYOUTCENTER", "DRAWLINEWIDTH", "DRAWBGCOLOR", "DRAWMAKEINACTIVE",
  "DRAWIMAGETINT", "DRAWIMAGEOPACITY", "DRAWIMAGESRCRECT", "DRAWIMAGEQUALITY",
  "DRAWANTIALIAS", "DRAWLINECAP", "DRAWLINEJOIN", "DRAWDASH",
  "DRAWDASHOFFSET"
};

#define IUP_DRAW_STATE_ATTRIB_COUNT ((int)(sizeof(iDrawStateAttribNames) / sizeof(iDrawStateAttribNames[0])))

typedef struct _IupDrawClipData
{
  int type;
  int x1, y1, x2, y2;
  int corner_radius;
  IupPathSeg* segs;
  int count;
  int rule;
  IupDrawMatrix matrix;
} IupDrawClipData;

typedef struct _IupDrawStateStack
{
  IupDrawMatrix matrix;
  IupDrawClipData clip;
  IupDrawSource source;
  int has_source;
  char* attribs[IUP_DRAW_STATE_ATTRIB_COUNT];
  struct _IupDrawStateStack* next;
} IupDrawStateStack;

typedef struct _IupDrawState
{
  IupDrawMatrix matrix;
  IupDrawClipData clip;
  IupDrawStateStack* stack;
} IupDrawState;

static void iDrawMatrixIdentity(IupDrawMatrix* matrix)
{
  matrix->a = 1;
  matrix->b = 0;
  matrix->c = 0;
  matrix->d = 1;
  matrix->e = 0;
  matrix->f = 0;
}

static int iDrawMatrixValid(const IupDrawMatrix* matrix)
{
  double determinant;
  if (!isfinite(matrix->a) || !isfinite(matrix->b) || !isfinite(matrix->c) ||
      !isfinite(matrix->d) || !isfinite(matrix->e) || !isfinite(matrix->f))
    return 0;
  determinant = matrix->a * matrix->d - matrix->b * matrix->c;
  return isfinite(determinant) && determinant != 0;
}

static int iDrawMatrixMultiply(const IupDrawMatrix* left, const IupDrawMatrix* right, IupDrawMatrix* result)
{
  IupDrawMatrix value;
  value.a = left->a * right->a + left->c * right->b;
  value.b = left->b * right->a + left->d * right->b;
  value.c = left->a * right->c + left->c * right->d;
  value.d = left->b * right->c + left->d * right->d;
  value.e = left->a * right->e + left->c * right->f + left->e;
  value.f = left->b * right->e + left->d * right->f + left->f;
  if (!iDrawMatrixValid(&value))
    return 0;
  *result = value;
  return 1;
}

static IupDrawState* iDrawStateGet(Ihandle* ih)
{
  return (IupDrawState*)iupAttribGet(ih, "_IUPDRAW_STATE");
}

static void iDrawClipFree(IupDrawClipData* clip)
{
  free(clip->segs);
  memset(clip, 0, sizeof(IupDrawClipData));
}

static int iDrawClipCopy(IupDrawClipData* dst, const IupDrawClipData* src)
{
  *dst = *src;
  dst->segs = NULL;
  if (src->count)
  {
    dst->segs = (IupPathSeg*)malloc((size_t)src->count * sizeof(IupPathSeg));
    if (!dst->segs)
      return 0;
    memcpy(dst->segs, src->segs, (size_t)src->count * sizeof(IupPathSeg));
  }
  return 1;
}

static void iDrawStateStackFree(IupDrawStateStack* item)
{
  int i;
  iDrawClipFree(&item->clip);
  for (i = 0; i < IUP_DRAW_STATE_ATTRIB_COUNT; i++)
    free(item->attribs[i]);
  free(item);
}

static void iDrawStateFree(Ihandle* ih)
{
  IupDrawState* state = iDrawStateGet(ih);
  if (state)
  {
    IupDrawStateStack* item = state->stack;
    while (item)
    {
      IupDrawStateStack* next = item->next;
      iDrawStateStackFree(item);
      item = next;
    }
    iDrawClipFree(&state->clip);
    free(state);
    iupAttribSet(ih, "_IUPDRAW_STATE", NULL);
  }
}

static void iDrawSetDriverTransform(Ihandle* ih, const IupDrawMatrix* matrix)
{
  iSvgCanvas* svg = IUP_SVG_GET(ih);
  if (svg)
    iupSvgDrawSetTransform(svg, matrix);
  else
    iupdrvDrawSetTransform((IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC"), matrix);
}

static IupDrawState* iDrawStateCreate(Ihandle* ih)
{
  IupDrawState* state = (IupDrawState*)calloc(1, sizeof(IupDrawState));
  if (!state)
    return NULL;
  iDrawMatrixIdentity(&state->matrix);
  iDrawMatrixIdentity(&state->clip.matrix);
  iupAttribSet(ih, "_IUPDRAW_STATE", (char*)state);
  iDrawSetDriverTransform(ih, &state->matrix);
  return state;
}

static void iDrawReplayClip(Ihandle* ih, const IupDrawClipData* clip, const IupDrawMatrix* matrix)
{
  iSvgCanvas* svg = IUP_SVG_GET(ih);
  if (svg)
  {
    iupSvgDrawResetClip(svg);
    if (clip->type != IUP_DRAW_CLIP_NONE)
    {
      iupSvgDrawSetTransform(svg, &clip->matrix);
      if (clip->type == IUP_DRAW_CLIP_RECT)
        iupSvgDrawSetClipRect(svg, clip->x1, clip->y1, clip->x2, clip->y2);
      else if (clip->type == IUP_DRAW_CLIP_ROUNDED)
        iupSvgDrawSetClipRoundedRect(svg, clip->x1, clip->y1, clip->x2, clip->y2, clip->corner_radius);
      else
        iupSvgDrawSetClipPath(svg, clip->segs, clip->count, clip->rule);
    }
    iupSvgDrawSetTransform(svg, matrix);
  }
  else
  {
    IdrawCanvas* dc = (IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC");
    iupdrvDrawResetClip(dc);
    if (clip->type != IUP_DRAW_CLIP_NONE)
    {
      iupdrvDrawSetTransform(dc, &clip->matrix);
      if (clip->type == IUP_DRAW_CLIP_RECT)
        iupdrvDrawSetClipRect(dc, clip->x1, clip->y1, clip->x2, clip->y2);
      else if (clip->type == IUP_DRAW_CLIP_ROUNDED)
        iupdrvDrawSetClipRoundedRect(dc, clip->x1, clip->y1, clip->x2, clip->y2, clip->corner_radius);
      else
        iupdrvDrawSetClipPath(dc, clip->segs, clip->count, clip->rule);
    }
    iupdrvDrawSetTransform(dc, matrix);
  }
}

static void iDrawPathFree(Ihandle* ih)
{
  IupDrawPathData* path = (IupDrawPathData*)iupAttribGet(ih, "_IUPDRAW_PATH");
  if (path)
  {
    free(path->segs);
    free(path);
    iupAttribSet(ih, "_IUPDRAW_PATH", NULL);
  }
}

static IupDrawPathData* iDrawPathGet(Ihandle* ih)
{
  IupDrawPathData* path = (IupDrawPathData*)iupAttribGet(ih, "_IUPDRAW_PATH");
  if (!path)
  {
    path = (IupDrawPathData*)calloc(1, sizeof(IupDrawPathData));
    if (path)
      iupAttribSet(ih, "_IUPDRAW_PATH", (char*)path);
  }
  return path;
}

static int iDrawPathAppend(IupDrawPathData* path, IupPathSeg* seg)
{
  if (path->count == path->cap)
  {
    int cap = path->cap ? path->cap * 2 : 64;
    IupPathSeg* segs = (IupPathSeg*)realloc(path->segs, (size_t)cap * sizeof(IupPathSeg));
    if (!segs)
      return 0;
    path->segs = segs;
    path->cap = cap;
  }
  path->segs[path->count++] = *seg;
  return 1;
}

static void iDrawPathAdd(Ihandle* ih, int op, int x1, int y1, int x2, int y2, int x3, int y3, double a1, double a2)
{
  IupDrawPathData* path = (IupDrawPathData*)iupAttribGet(ih, "_IUPDRAW_PATH");
  IupPathSeg seg;

  if (!path)
    return;

  memset(&seg, 0, sizeof(seg));
  seg.op = (unsigned char)op;
  seg.x1 = x1; seg.y1 = y1;
  seg.x2 = x2; seg.y2 = y2;
  seg.x3 = x3; seg.y3 = y3;
  seg.a1 = a1; seg.a2 = a2;

  if (op == IUP_PATHSEG_MOVE_TO)
  {
    if (!iDrawPathAppend(path, &seg))
      return;
    path->has_current = 1;
    path->cur_x = x1; path->cur_y = y1;
    path->sub_x = x1; path->sub_y = y1;
  }
  else if (op == IUP_PATHSEG_CLOSE)
  {
    if (path->has_current)
    {
      if (!iDrawPathAppend(path, &seg))
        return;
      path->cur_x = path->sub_x;
      path->cur_y = path->sub_y;
    }
  }
  else
  {
    if (!path->has_current)
    {
      IupPathSeg mv;
      int ax = x1, ay = y1;

      if (op == IUP_PATHSEG_CURVE_TO) { ax = x3; ay = y3; }
      else if (op == IUP_PATHSEG_QUAD_TO) { ax = x2; ay = y2; }
      else if (op == IUP_PATHSEG_ARC_TO)
      {
        ax = iupROUND(x1 + x2 * cos(a1 * IUP_DEG2RAD));
        ay = iupROUND(y1 - y2 * sin(a1 * IUP_DEG2RAD));
      }

      memset(&mv, 0, sizeof(mv));
      mv.op = IUP_PATHSEG_MOVE_TO;
      mv.x1 = ax; mv.y1 = ay;
      if (!iDrawPathAppend(path, &mv))
        return;
      path->has_current = 1;
      path->cur_x = ax; path->cur_y = ay;
      path->sub_x = ax; path->sub_y = ay;
    }

    if (!iDrawPathAppend(path, &seg))
      return;

    if (op == IUP_PATHSEG_LINE_TO) { path->cur_x = x1; path->cur_y = y1; }
    else if (op == IUP_PATHSEG_CURVE_TO) { path->cur_x = x3; path->cur_y = y3; }
    else if (op == IUP_PATHSEG_QUAD_TO) { path->cur_x = x2; path->cur_y = y2; }
    else
    {
      path->cur_x = iupROUND(x1 + x2 * cos(a2 * IUP_DEG2RAD));
      path->cur_y = iupROUND(y1 - y2 * sin(a2 * IUP_DEG2RAD));
    }
  }
}

static void iDrawSourceFree(Ihandle* ih)
{
  IupDrawSource* src = (IupDrawSource*)iupAttribGet(ih, "_IUPDRAW_SOURCE");
  if (src)
  {
    free(src);
    iupAttribSet(ih, "_IUPDRAW_SOURCE", NULL);
  }
}

static IupDrawSource* iDrawSourceGet(Ihandle* ih)
{
  IupDrawSource* src = (IupDrawSource*)iupAttribGet(ih, "_IUPDRAW_SOURCE");
  if (!src)
  {
    src = (IupDrawSource*)calloc(1, sizeof(IupDrawSource));
    if (src)
      iupAttribSet(ih, "_IUPDRAW_SOURCE", (char*)src);
  }
  return src;
}

static const IupDrawSource* iDrawSourceCurrent(Ihandle* ih, IupDrawSource* solid)
{
  IupDrawSource* src = (IupDrawSource*)iupAttribGet(ih, "_IUPDRAW_SOURCE");
  if (src)
    return src;

  memset(solid, 0, sizeof(IupDrawSource));
  solid->type = IUP_SOURCE_SOLID;
  solid->color = iupDrawStrToColor(iupAttribGetStr(ih, "DRAWCOLOR"), 0);
  solid->colors[0] = solid->color;
  solid->offsets[0] = 0;
  solid->count = 1;
  return solid;
}


IUP_API void IupDrawBegin(Ihandle* ih)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  iDrawPathFree(ih);
  iDrawSourceFree(ih);
  iDrawStateFree(ih);

  if (IUP_SVG_GET(ih))
  {
    iupAttribSet(ih, "_IUP_DRAW_DC", (char*)1);
    (void)iDrawStateCreate(ih);
    return;
  }

  {
    IdrawCanvas* dc = iupdrvDrawCreateCanvas(ih);
    iupAttribSet(ih, "_IUP_DRAW_DC", (char*)dc);
    if (dc)
      (void)iDrawStateCreate(ih);
  }
}

IUP_API void IupDrawEnd(Ihandle* ih)
{
  IdrawCanvas* dc;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  iDrawPathFree(ih);
  iDrawSourceFree(ih);
  iDrawStateFree(ih);

  if (IUP_SVG_GET(ih))
  {
    iupAttribSet(ih, "_IUP_DRAW_DC", NULL);
    return;
  }

  dc = (IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC");
  if (!dc)
    return;

  iupdrvDrawFlush(dc);
  iupdrvDrawKillCanvas(dc);
  iupAttribSet(ih, "_IUP_DRAW_DC", NULL);
}

IUP_API void IupDrawSave(Ihandle* ih)
{
  IupDrawStateStack* item;
  IupDrawState* state;
  IupDrawSource* source;
  int i;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih) || !iupAttribGet(ih, "_IUP_DRAW_DC"))
    return;

  state = iDrawStateGet(ih);
  if (!state)
    return;

  item = (IupDrawStateStack*)calloc(1, sizeof(IupDrawStateStack));
  if (!item)
    return;

  item->matrix = state->matrix;
  if (!iDrawClipCopy(&item->clip, &state->clip))
  {
    iDrawStateStackFree(item);
    return;
  }

  source = (IupDrawSource*)iupAttribGet(ih, "_IUPDRAW_SOURCE");
  if (source)
  {
    item->source = *source;
    item->has_source = 1;
  }

  for (i = 0; i < IUP_DRAW_STATE_ATTRIB_COUNT; i++)
  {
    char* value = iupAttribGet(ih, iDrawStateAttribNames[i]);
    if (value)
    {
      item->attribs[i] = iupStrDup(value);
      if (!item->attribs[i])
      {
        iDrawStateStackFree(item);
        return;
      }
    }
  }

  item->next = state->stack;
  state->stack = item;
}

IUP_API void IupDrawRestore(Ihandle* ih)
{
  IupDrawStateStack* item;
  IupDrawState* state;
  int i;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih) || !iupAttribGet(ih, "_IUP_DRAW_DC"))
    return;

  state = iDrawStateGet(ih);
  if (!state || !state->stack)
    return;

  item = state->stack;
  state->stack = item->next;

  for (i = 0; i < IUP_DRAW_STATE_ATTRIB_COUNT; i++)
    iupAttribSetStr(ih, iDrawStateAttribNames[i], item->attribs[i]);

  iDrawSourceFree(ih);
  if (item->has_source)
  {
    IupDrawSource* source = iDrawSourceGet(ih);
    if (source)
      *source = item->source;
  }

  iDrawClipFree(&state->clip);
  state->clip = item->clip;
  item->clip.segs = NULL;
  state->matrix = item->matrix;
  iDrawReplayClip(ih, &state->clip, &state->matrix);
  iDrawStateStackFree(item);
}

IUP_API void IupDrawTransform(Ihandle* ih, double a, double b, double c, double d, double e, double f)
{
  IupDrawMatrix matrix, result;
  IupDrawState* state;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih) || !iupAttribGet(ih, "_IUP_DRAW_DC"))
    return;

  state = iDrawStateGet(ih);
  if (!state)
    return;

  matrix.a = a; matrix.b = b; matrix.c = c;
  matrix.d = d; matrix.e = e; matrix.f = f;
  if (!iDrawMatrixValid(&matrix) || !iDrawMatrixMultiply(&state->matrix, &matrix, &result))
    return;

  state->matrix = result;
  iDrawSetDriverTransform(ih, &state->matrix);
}

IUP_API void IupDrawSetTransform(Ihandle* ih, double a, double b, double c, double d, double e, double f)
{
  IupDrawMatrix matrix;
  IupDrawState* state;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih) || !iupAttribGet(ih, "_IUP_DRAW_DC"))
    return;

  matrix.a = a; matrix.b = b; matrix.c = c;
  matrix.d = d; matrix.e = e; matrix.f = f;
  if (!iDrawMatrixValid(&matrix))
    return;

  state = iDrawStateGet(ih);
  if (!state)
    return;

  state->matrix = matrix;
  iDrawSetDriverTransform(ih, &state->matrix);
}

IUP_API void IupDrawResetTransform(Ihandle* ih)
{
  IupDrawMatrix matrix;
  iDrawMatrixIdentity(&matrix);
  IupDrawSetTransform(ih, matrix.a, matrix.b, matrix.c, matrix.d, matrix.e, matrix.f);
}

IUP_API void IupDrawGetTransform(Ihandle* ih, double* a, double* b, double* c, double* d, double* e, double* f)
{
  IupDrawState* state;

  if (a) *a = 1;
  if (b) *b = 0;
  if (c) *c = 0;
  if (d) *d = 1;
  if (e) *e = 0;
  if (f) *f = 0;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih) || !iupAttribGet(ih, "_IUP_DRAW_DC"))
    return;

  state = iDrawStateGet(ih);
  if (!state)
    return;

  if (a) *a = state->matrix.a;
  if (b) *b = state->matrix.b;
  if (c) *c = state->matrix.c;
  if (d) *d = state->matrix.d;
  if (e) *e = state->matrix.e;
  if (f) *f = state->matrix.f;
}

IUP_API void IupDrawTranslate(Ihandle* ih, double tx, double ty)
{
  IupDrawTransform(ih, 1, 0, 0, 1, tx, ty);
}

IUP_API void IupDrawScale(Ihandle* ih, double sx, double sy)
{
  IupDrawTransform(ih, sx, 0, 0, sy, 0, 0);
}

IUP_API void IupDrawRotate(Ihandle* ih, double angle)
{
  double value, sine, cosine;
  if (!isfinite(angle))
    return;
  value = angle * IUP_DEG2RAD;
  sine = sin(value);
  cosine = cos(value);
  IupDrawTransform(ih, cosine, -sine, sine, cosine, 0, 0);
}

IUP_API void IupDrawGetSize(Ihandle* ih, int* w, int* h)
{
  iSvgCanvas* svg;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  svg = IUP_SVG_GET(ih);
  if (svg)
  {
    iupSvgDrawGetSize(svg, w, h);
    return;
  }

  {
    IdrawCanvas* dc = (IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC");
    if (!dc)
      return;
    iupdrvDrawGetSize(dc, w, h);
  }
}

IUP_API void IupDrawParentBackground(Ihandle* ih)
{
  IdrawCanvas* dc;
  IupDrawMatrix identity;
  IupDrawState* state;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (IUP_SVG_GET(ih))
    return;

  dc = (IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC");
  if (!dc)
    return;

  state = iDrawStateGet(ih);
  if (!state)
    return;

  iDrawMatrixIdentity(&identity);
  iupdrvDrawSetTransform(dc, &identity);
  iupDrawParentBackground(dc, ih);
  iupdrvDrawSetTransform(dc, &state->matrix);
}

static int iDrawGetStyle(Ihandle* ih)
{
  char* style = iupAttribGetStr(ih, "DRAWSTYLE");
  if (!style)
    return IUP_DRAW_STROKE;
  if (style[0] >= '0' && style[0] <= '5')
    return (int)(style[0] - '0');
  if (iupStrEqualNoCase(style, "FILL"))
    return IUP_DRAW_FILL;
  else if (iupStrEqualNoCase(style, "STROKE_DASH"))
    return IUP_DRAW_STROKE_DASH;
  else if (iupStrEqualNoCase(style, "STROKE_DOT"))
    return IUP_DRAW_STROKE_DOT;
  else if (iupStrEqualNoCase(style, "STROKE_DASH_DOT"))
    return IUP_DRAW_STROKE_DASH_DOT;
  else if (iupStrEqualNoCase(style, "STROKE_DASH_DOT_DOT"))
    return IUP_DRAW_STROKE_DASH_DOT_DOT;
  else
    return IUP_DRAW_STROKE;
}

static int iDrawGetCap(Ihandle* ih)
{
  char* value = iupAttribGetStr(ih, "DRAWLINECAP");
  if (iupStrEqualNoCase(value, "ROUND"))
    return IUP_DRAW_CAP_ROUND;
  else if (iupStrEqualNoCase(value, "SQUARE"))
    return IUP_DRAW_CAP_SQUARE;
  else
    return IUP_DRAW_CAP_BUTT;
}

static int iDrawGetJoin(Ihandle* ih)
{
  char* value = iupAttribGetStr(ih, "DRAWLINEJOIN");
  if (iupStrEqualNoCase(value, "ROUND"))
    return IUP_DRAW_JOIN_ROUND;
  else if (iupStrEqualNoCase(value, "BEVEL"))
    return IUP_DRAW_JOIN_BEVEL;
  else
    return IUP_DRAW_JOIN_MITER;
}

static int iDrawGetCustomDashes(Ihandle* ih, double* dashes)
{
  char* value = iupAttribGetStr(ih, "DRAWDASH");
  int count = 0;
  double total = 0;
  int i;
  if (!value)
    return 0;

  while (*value && count < IUP_DRAW_MAX_DASHES)
  {
    double len;
    char* end;
    while (*value == ' ' || *value == ',')
      value++;
    if (!*value)
      break;
    len = strtod(value, &end);
    if (end == value || !(len >= 0) || len > 1e6)
      return 0;
    dashes[count++] = len;
    value = end;
  }

  if (count < 2)
    return 0;

  for (i = 0; i < count; i++)
    total += dashes[i];
  if (total <= 0)
    return 0;

  if (count & 1)
  {
    if (count * 2 > IUP_DRAW_MAX_DASHES)
      count--;
    else
    {
      for (i = 0; i < count; i++)
        dashes[count + i] = dashes[i];
      count *= 2;
    }
  }

  return count;
}

static int iDrawGetStyleDashes(int style, double* dashes)
{
  static const double dash[] = { 9, 3 };
  static const double dot[] = { 1, 2 };
  static const double dash_dot[] = { 7, 3, 1, 3 };
  static const double dash_dot_dot[] = { 7, 3, 1, 3, 1, 3 };
  const double* pattern;
  int count, i;

  switch (style)
  {
  case IUP_DRAW_STROKE_DASH:         pattern = dash;         count = 2; break;
  case IUP_DRAW_STROKE_DOT:          pattern = dot;          count = 2; break;
  case IUP_DRAW_STROKE_DASH_DOT:     pattern = dash_dot;     count = 4; break;
  case IUP_DRAW_STROKE_DASH_DOT_DOT: pattern = dash_dot_dot; count = 6; break;
  default: return 0;
  }

  for (i = 0; i < count; i++)
    dashes[i] = pattern[i];
  return count;
}

IUP_SDK_API void iupDrawGetStroke(Ihandle* ih, int style, IupDrawStroke* stroke)
{
  memset(stroke, 0, sizeof(IupDrawStroke));
  stroke->cap = iDrawGetCap(ih);
  stroke->join = iDrawGetJoin(ih);
  stroke->dash_count = iDrawGetCustomDashes(ih, stroke->dashes);
  if (!stroke->dash_count)
    stroke->dash_count = iDrawGetStyleDashes(style, stroke->dashes);
  if (stroke->dash_count)
    stroke->dash_offset = iupAttribGetDouble(ih, "DRAWDASHOFFSET");
}

static int iDrawGetLineWidth(Ihandle* ih)
{
  int line_width = iupAttribGetInt(ih, "DRAWLINEWIDTH");
  if (line_width == 0)
    return 1;
  else
    return line_width;
}

IUP_API void IupDrawLine(Ihandle* ih, int x1, int y1, int x2, int y2)
{
  IdrawCanvas* dc;
  long color = 0;
  int style, line_width;
  iSvgCanvas* svg;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (!iupAttribGet(ih, "_IUP_DRAW_DC"))
    return;

  color = iupDrawStrToColor(iupAttribGetStr(ih, "DRAWCOLOR"), 0);
  line_width = iDrawGetLineWidth(ih);
  style = iDrawGetStyle(ih);

  svg = IUP_SVG_GET(ih);
  if (svg)
  {
    char c[32]; iSvgColorStr(color, c, sizeof(c));
    iupSvgDrawLine(svg, x1, y1, x2, y2, c, style, line_width);
    return;
  }

  dc = (IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC");
  iupdrvDrawLine(dc, x1, y1, x2, y2, color, style, line_width);
}

IUP_API void IupDrawRectangle(Ihandle* ih, int x1, int y1, int x2, int y2)
{
  IdrawCanvas* dc;
  long color;
  int style, line_width;
  iSvgCanvas* svg;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (!iupAttribGet(ih, "_IUP_DRAW_DC"))
    return;

  color = iupDrawStrToColor(iupAttribGetStr(ih, "DRAWCOLOR"), 0);
  line_width = iDrawGetLineWidth(ih);
  style = iDrawGetStyle(ih);

  svg = IUP_SVG_GET(ih);
  if (svg)
  {
    char c[32]; iSvgColorStr(color, c, sizeof(c));
    iupSvgDrawRectangle(svg, x1, y1, x2, y2, c, style, line_width);
    return;
  }

  dc = (IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC");
  iupdrvDrawRectangle(dc, x1, y1, x2, y2, color, style, line_width);
}

IUP_API void IupDrawArc(Ihandle* ih, int x1, int y1, int x2, int y2, double a1, double a2)
{
  long color = 0;
  int style, line_width;
  iSvgCanvas* svg;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (!iupAttribGet(ih, "_IUP_DRAW_DC"))
    return;

  color = iupDrawStrToColor(iupAttribGetStr(ih, "DRAWCOLOR"), 0);
  line_width = iDrawGetLineWidth(ih);
  style = iDrawGetStyle(ih);

  svg = IUP_SVG_GET(ih);
  if (svg)
  {
    char c[32]; iSvgColorStr(color, c, sizeof(c));
    iupSvgDrawArc(svg, x1, y1, x2, y2, a1, a2, c, style, line_width);
    return;
  }

  iupdrvDrawArc((IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC"), x1, y1, x2, y2, a1, a2, color, style, line_width);
}

IUP_API void IupDrawEllipse(Ihandle* ih, int x1, int y1, int x2, int y2)
{
  long color = 0;
  int style, line_width;
  iSvgCanvas* svg;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (!iupAttribGet(ih, "_IUP_DRAW_DC"))
    return;

  color = iupDrawStrToColor(iupAttribGetStr(ih, "DRAWCOLOR"), 0);
  line_width = iDrawGetLineWidth(ih);
  style = iDrawGetStyle(ih);

  svg = IUP_SVG_GET(ih);
  if (svg)
  {
    char c[32]; iSvgColorStr(color, c, sizeof(c));
    iupSvgDrawEllipse(svg, x1, y1, x2, y2, c, style, line_width);
    return;
  }

  iupdrvDrawEllipse((IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC"), x1, y1, x2, y2, color, style, line_width);
}

IUP_API void IupDrawPolygon(Ihandle* ih, int* points, int count)
{
  long color = 0;
  int style, line_width;
  iSvgCanvas* svg;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (!iupAttribGet(ih, "_IUP_DRAW_DC"))
    return;

  color = iupDrawStrToColor(iupAttribGetStr(ih, "DRAWCOLOR"), 0);
  line_width = iDrawGetLineWidth(ih);
  style = iDrawGetStyle(ih);

  svg = IUP_SVG_GET(ih);
  if (svg)
  {
    char c[32]; iSvgColorStr(color, c, sizeof(c));
    iupSvgDrawPolygon(svg, points, count, c, style, line_width);
    return;
  }

  iupdrvDrawPolygon((IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC"), points, count, color, style, line_width);
}

IUP_API void IupDrawPixel(Ihandle* ih, int x, int y)
{
  long color = 0;
  iSvgCanvas* svg;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (!iupAttribGet(ih, "_IUP_DRAW_DC"))
    return;

  color = iupDrawStrToColor(iupAttribGetStr(ih, "DRAWCOLOR"), 0);

  svg = IUP_SVG_GET(ih);
  if (svg)
  {
    char c[32]; iSvgColorStr(color, c, sizeof(c));
    iupSvgDrawPixel(svg, x, y, c);
    return;
  }

  iupdrvDrawPixel((IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC"), x, y, color);
}

IUP_API void IupDrawRoundedRectangle(Ihandle* ih, int x1, int y1, int x2, int y2, int corner_radius)
{
  long color = 0;
  int style, line_width;
  iSvgCanvas* svg;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (!iupAttribGet(ih, "_IUP_DRAW_DC"))
    return;

  color = iupDrawStrToColor(iupAttribGetStr(ih, "DRAWCOLOR"), 0);
  line_width = iDrawGetLineWidth(ih);
  style = iDrawGetStyle(ih);

  svg = IUP_SVG_GET(ih);
  if (svg)
  {
    char c[32]; iSvgColorStr(color, c, sizeof(c));
    iupSvgDrawRoundedRectangle(svg, x1, y1, x2, y2, corner_radius, c, style, line_width);
    return;
  }

  iupdrvDrawRoundedRectangle((IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC"), x1, y1, x2, y2, corner_radius, color, style, line_width);
}

IUP_API void IupDrawBezier(Ihandle* ih, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4)
{
  long color = 0;
  int style, line_width;
  iSvgCanvas* svg;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (!iupAttribGet(ih, "_IUP_DRAW_DC"))
    return;

  color = iupDrawStrToColor(iupAttribGetStr(ih, "DRAWCOLOR"), 0);
  line_width = iDrawGetLineWidth(ih);
  style = iDrawGetStyle(ih);

  svg = IUP_SVG_GET(ih);
  if (svg)
  {
    char c[32]; iSvgColorStr(color, c, sizeof(c));
    iupSvgDrawBezier(svg, x1, y1, x2, y2, x3, y3, x4, y4, c, style, line_width);
    return;
  }

  iupdrvDrawBezier((IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC"), x1, y1, x2, y2, x3, y3, x4, y4, color, style, line_width);
}

IUP_API void IupDrawQuadraticBezier(Ihandle* ih, int x1, int y1, int x2, int y2, int x3, int y3)
{
  long color = 0;
  int style, line_width;
  iSvgCanvas* svg;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (!iupAttribGet(ih, "_IUP_DRAW_DC"))
    return;

  color = iupDrawStrToColor(iupAttribGetStr(ih, "DRAWCOLOR"), 0);
  line_width = iDrawGetLineWidth(ih);
  style = iDrawGetStyle(ih);

  svg = IUP_SVG_GET(ih);
  if (svg)
  {
    char c[32]; iSvgColorStr(color, c, sizeof(c));
    iupSvgDrawQuadraticBezier(svg, x1, y1, x2, y2, x3, y3, c, style, line_width);
    return;
  }

  iupdrvDrawQuadraticBezier((IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC"), x1, y1, x2, y2, x3, y3, color, style, line_width);
}

static int iDrawBuildStops(const char** colors, const float* offsets, int count, long* out_colors, float* out_offsets)
{
  int i;

  if (!colors || count < 2 || count > IUP_GRADIENT_MAX_STOPS)
    return 0;

  for (i = 0; i < count; i++)
  {
    if (!colors[i])
      return 0;
    if (offsets && (!isfinite(offsets[i]) || offsets[i] < 0.0f || offsets[i] > 1.0f || (i > 0 && offsets[i] < offsets[i - 1])))
      return 0;
    out_colors[i] = iupDrawStrToColor(colors[i], 0);
    if (offsets)
      out_offsets[i] = offsets[i];
    else
      out_offsets[i] = (float)i / (float)(count - 1);
  }

  return count;
}

IUP_API void IupDrawLinearGradient(Ihandle* ih, int x1, int y1, int x2, int y2, float angle, const char* color1, const char* color2)
{
  const char* colors[2];
  colors[0] = color1;
  colors[1] = color2;
  IupDrawLinearGradientStops(ih, x1, y1, x2, y2, angle, colors, NULL, 2);
}

IUP_API void IupDrawLinearGradientStops(Ihandle* ih, int x1, int y1, int x2, int y2, float angle, const char** colors, const float* offsets, int count)
{
  iSvgCanvas* svg;
  long c[IUP_GRADIENT_MAX_STOPS];
  float o[IUP_GRADIENT_MAX_STOPS];

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (!iupAttribGet(ih, "_IUP_DRAW_DC") || !isfinite(angle))
    return;

  count = iDrawBuildStops(colors, offsets, count, c, o);
  if (!count)
    return;

  svg = IUP_SVG_GET(ih);
  if (svg)
  {
    iupSvgDrawLinearGradient(svg, x1, y1, x2, y2, angle, c, o, count);
    return;
  }

  iupdrvDrawLinearGradient((IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC"), x1, y1, x2, y2, angle, c, o, count);
}

IUP_API void IupDrawRadialGradient(Ihandle* ih, int cx, int cy, int radius, const char* colorCenter, const char* colorEdge)
{
  const char* colors[2];
  colors[0] = colorCenter;
  colors[1] = colorEdge;
  IupDrawRadialGradientStops(ih, cx, cy, radius, colors, NULL, 2);
}

IUP_API void IupDrawRadialGradientStops(Ihandle* ih, int cx, int cy, int radius, const char** colors, const float* offsets, int count)
{
  iSvgCanvas* svg;
  long c[IUP_GRADIENT_MAX_STOPS];
  float o[IUP_GRADIENT_MAX_STOPS];

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (!iupAttribGet(ih, "_IUP_DRAW_DC"))
    return;

  count = iDrawBuildStops(colors, offsets, count, c, o);
  if (!count)
    return;

  svg = IUP_SVG_GET(ih);
  if (svg)
  {
    iupSvgDrawRadialGradient(svg, cx, cy, radius, c, o, count);
    return;
  }

  iupdrvDrawRadialGradient((IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC"), cx, cy, radius, c, o, count);
}

IUP_API void IupDrawPathBegin(Ihandle* ih)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (!iupAttribGet(ih, "_IUP_DRAW_DC"))
    return;

  iDrawPathFree(ih);
  (void)iDrawPathGet(ih);
}

IUP_API void IupDrawPathMoveTo(Ihandle* ih, int x, int y)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (!iupAttribGet(ih, "_IUP_DRAW_DC"))
    return;

  if (!iDrawPathGet(ih))
    return;
  iDrawPathAdd(ih, IUP_PATHSEG_MOVE_TO, x, y, 0, 0, 0, 0, 0, 0);
}

IUP_API void IupDrawPathLineTo(Ihandle* ih, int x, int y)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (!iupAttribGet(ih, "_IUP_DRAW_DC"))
    return;

  if (!iDrawPathGet(ih))
    return;
  iDrawPathAdd(ih, IUP_PATHSEG_LINE_TO, x, y, 0, 0, 0, 0, 0, 0);
}

IUP_API void IupDrawPathCurveTo(Ihandle* ih, int x1, int y1, int x2, int y2, int x3, int y3)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (!iupAttribGet(ih, "_IUP_DRAW_DC"))
    return;

  if (!iDrawPathGet(ih))
    return;
  iDrawPathAdd(ih, IUP_PATHSEG_CURVE_TO, x1, y1, x2, y2, x3, y3, 0, 0);
}

IUP_API void IupDrawPathQuadTo(Ihandle* ih, int x1, int y1, int x2, int y2)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (!iupAttribGet(ih, "_IUP_DRAW_DC"))
    return;

  if (!iDrawPathGet(ih))
    return;
  iDrawPathAdd(ih, IUP_PATHSEG_QUAD_TO, x1, y1, x2, y2, 0, 0, 0, 0);
}

IUP_API void IupDrawPathArcTo(Ihandle* ih, int cx, int cy, int rx, int ry, double a1, double a2)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (!iupAttribGet(ih, "_IUP_DRAW_DC"))
    return;

  if (rx <= 0 || ry <= 0 || !isfinite(a1) || !isfinite(a2))
    return;

  if (!iDrawPathGet(ih))
    return;

  iDrawPathAdd(ih, IUP_PATHSEG_LINE_TO, iupROUND(cx + rx * cos(a1 * IUP_DEG2RAD)), iupROUND(cy - ry * sin(a1 * IUP_DEG2RAD)), 0, 0, 0, 0, 0, 0);
  iDrawPathAdd(ih, IUP_PATHSEG_ARC_TO, cx, cy, rx, ry, 0, 0, a1, a2);
}

IUP_API void IupDrawPathClose(Ihandle* ih)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (!iupAttribGet(ih, "_IUP_DRAW_DC"))
    return;

  if (!iDrawPathGet(ih))
    return;
  iDrawPathAdd(ih, IUP_PATHSEG_CLOSE, 0, 0, 0, 0, 0, 0, 0, 0);
}

IUP_API void IupDrawPathFill(Ihandle* ih, int rule)
{
  iSvgCanvas* svg;
  IupDrawPathData* path;
  IupDrawSource solid;
  const IupDrawSource* src;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (!iupAttribGet(ih, "_IUP_DRAW_DC") || (rule != IUP_DRAW_RULE_WINDING && rule != IUP_DRAW_RULE_EVENODD))
    return;

  path = (IupDrawPathData*)iupAttribGet(ih, "_IUPDRAW_PATH");
  if (!path || path->count == 0)
    return;

  src = iDrawSourceCurrent(ih, &solid);

  svg = IUP_SVG_GET(ih);
  if (svg)
  {
    iupSvgDrawPathFill(svg, path->segs, path->count, src, rule);
    return;
  }

  iupdrvDrawPathFill((IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC"), path->segs, path->count, src, rule);
}

IUP_API void IupDrawPathStroke(Ihandle* ih)
{
  iSvgCanvas* svg;
  IupDrawPathData* path;
  IupDrawSource solid;
  const IupDrawSource* src;
  int style, line_width;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (!iupAttribGet(ih, "_IUP_DRAW_DC"))
    return;

  path = (IupDrawPathData*)iupAttribGet(ih, "_IUPDRAW_PATH");
  if (!path || path->count == 0)
    return;

  src = iDrawSourceCurrent(ih, &solid);
  line_width = iDrawGetLineWidth(ih);
  style = iDrawGetStyle(ih);

  svg = IUP_SVG_GET(ih);
  if (svg)
  {
    iupSvgDrawPathStroke(svg, path->segs, path->count, src, style, line_width);
    return;
  }

  iupdrvDrawPathStroke((IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC"), path->segs, path->count, src, style, line_width);
}

IUP_API void IupDrawSetClipPath(Ihandle* ih, int rule)
{
  iSvgCanvas* svg;
  IupDrawPathData* path;
  IupPathSeg* segs;
  IupDrawState* state;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (!iupAttribGet(ih, "_IUP_DRAW_DC") || (rule != IUP_DRAW_RULE_WINDING && rule != IUP_DRAW_RULE_EVENODD))
    return;

  path = (IupDrawPathData*)iupAttribGet(ih, "_IUPDRAW_PATH");
  if (!path || path->count == 0)
    return;

  state = iDrawStateGet(ih);
  if (!state)
    return;

  segs = (IupPathSeg*)malloc((size_t)path->count * sizeof(IupPathSeg));
  if (!segs)
    return;
  memcpy(segs, path->segs, (size_t)path->count * sizeof(IupPathSeg));

  iDrawClipFree(&state->clip);
  state->clip.type = IUP_DRAW_CLIP_PATH;
  state->clip.segs = segs;
  state->clip.count = path->count;
  state->clip.rule = rule;
  state->clip.matrix = state->matrix;

  svg = IUP_SVG_GET(ih);
  if (svg)
  {
    iupSvgDrawSetClipPath(svg, path->segs, path->count, rule);
    return;
  }

  iupdrvDrawSetClipPath((IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC"), path->segs, path->count, rule);
}

IUP_API void IupDrawSetSourceSolid(Ihandle* ih, const char* color)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (!iupAttribGet(ih, "_IUP_DRAW_DC"))
    return;

  iDrawSourceFree(ih);
  if (color)
    iupAttribSetStr(ih, "DRAWCOLOR", color);
}

IUP_API void IupDrawSetSourceLinearGradient(Ihandle* ih, int x1, int y1, int x2, int y2, float angle, const char** colors, const float* offsets, int count)
{
  IupDrawSource value, *src;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (!iupAttribGet(ih, "_IUP_DRAW_DC") || !isfinite(angle))
    return;

  memset(&value, 0, sizeof(value));
  count = iDrawBuildStops(colors, offsets, count, value.colors, value.offsets);
  if (!count)
    return;

  src = iDrawSourceGet(ih);
  if (!src)
    return;

  value.type = IUP_SOURCE_LINEAR_GRADIENT;
  value.x1 = x1; value.y1 = y1;
  value.x2 = x2; value.y2 = y2;
  value.angle = angle;
  value.count = count;
  *src = value;
}

IUP_API void IupDrawSetSourceRadialGradient(Ihandle* ih, int cx, int cy, int radius, const char** colors, const float* offsets, int count)
{
  IupDrawSource value, *src;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (!iupAttribGet(ih, "_IUP_DRAW_DC") || radius <= 0)
    return;

  memset(&value, 0, sizeof(value));
  count = iDrawBuildStops(colors, offsets, count, value.colors, value.offsets);
  if (!count)
    return;

  src = iDrawSourceGet(ih);
  if (!src)
    return;

  value.type = IUP_SOURCE_RADIAL_GRADIENT;
  value.cx = cx; value.cy = cy;
  value.radius = radius;
  value.count = count;
  *src = value;
}

IUP_API void IupDrawResetSource(Ihandle* ih)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (!iupAttribGet(ih, "_IUP_DRAW_DC"))
    return;

  iDrawSourceFree(ih);
}


static void iDrawRotatePoint(int x, int y, int* rx, int* ry, double sin_theta, double cos_theta)
{
  double t;
  t = (x * cos_theta) - (y * sin_theta); *rx = iupROUND(t);
  t = (x * sin_theta) + (y * cos_theta); *ry = iupROUND(t);
}

static void iDrawGetTextBounds(int w, int h, double text_orientation, int* o_w, int* o_h)
{
  int xmin, xmax, ymin, ymax, x_r, y_r;

  double cos_theta = cos(text_orientation * IUP_DEG2RAD);
  double sin_theta = sin(text_orientation * IUP_DEG2RAD);

  iDrawRotatePoint(0, 0, &x_r, &y_r, sin_theta, cos_theta);
  xmax = xmin = x_r;
  ymax = ymin = y_r;
  iDrawRotatePoint(w - 1, 0, &x_r, &y_r, sin_theta, cos_theta);
  xmin = iupMIN(xmin, x_r);
  ymin = iupMIN(ymin, y_r);
  xmax = iupMAX(xmax, x_r);
  ymax = iupMAX(ymax, y_r);
  iDrawRotatePoint(w - 1, h - 1, &x_r, &y_r, sin_theta, cos_theta);
  xmin = iupMIN(xmin, x_r);
  ymin = iupMIN(ymin, y_r);
  xmax = iupMAX(xmax, x_r);
  ymax = iupMAX(ymax, y_r);
  iDrawRotatePoint(0, h - 1, &x_r, &y_r, sin_theta, cos_theta);
  xmin = iupMIN(xmin, x_r);
  ymin = iupMIN(ymin, y_r);
  xmax = iupMAX(xmax, x_r);
  ymax = iupMAX(ymax, y_r);

  if (o_w) *o_w = xmax - xmin + 1;
  if (o_h) *o_h = ymax - ymin + 1;
}

IUP_SDK_API char* iupDrawGetTextSize(Ihandle* ih, const char* text, int len, int* w, int* h, double text_orientation)
{
  char*font = iupAttribGetStr(ih, "DRAWFONT");
  if (!font)
    font = IupGetAttribute(ih, "FONT");

  if (!text)
  {
    if (w) *w = 0;
    if (h) *h = 0;
    return font;
  }

  if (len == 0 || len == -1)
    len = (int)strlen(text);

  if (len == 0)
  {
    if (w) *w = 0;
    if (h) *h = 0;
    return font;
  }

  if (text_orientation)
  {
    if (text_orientation == 90)
      iupdrvFontGetTextSize(font, text, len, h, w);
    else
    {
      int txt_w, txt_h;
      iupdrvFontGetTextSize(font, text, len, &txt_w, &txt_h);
      iDrawGetTextBounds(txt_w, txt_h, text_orientation, w, h);
    }
  }
  else
    iupdrvFontGetTextSize(font, text, len, w, h);

  return font;
}

IUP_SDK_API int iupDrawGetTextFlags(Ihandle* ih, const char* align_name, const char* wrap_name, const char* ellipsis_name)
{
  int flags = iupFlatGetHorizontalAlignment(iupAttribGetStr(ih, align_name));
  int wrap = iupAttribGetBoolean(ih, wrap_name);
  int ellipsis = iupAttribGetBoolean(ih, ellipsis_name);
  if (wrap)
    flags |= IUP_DRAW_WRAP;
  if (ellipsis)
    flags |= IUP_DRAW_ELLIPSIS;
  return flags;
}

IUP_API void IupDrawText(Ihandle* ih, const char* text, int len, int x, int y, int w, int h)
{
  long color = 0;
  int text_flags;
  double text_orientation;
  iSvgCanvas* svg;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  iupASSERT(text);
  if (!text || text[0] == 0)
    return;

  if (!iupAttribGet(ih, "_IUP_DRAW_DC"))
    return;

  color = iupDrawStrToColor(iupAttribGetStr(ih, "DRAWCOLOR"), 0);
  text_orientation = iupAttribGetDouble(ih, "DRAWTEXTORIENTATION");
  text_flags = iupDrawGetTextFlags(ih, "DRAWTEXTALIGNMENT", "DRAWTEXTWRAP", "DRAWTEXTELLIPSIS");
  if (iupAttribGetBoolean(ih, "DRAWTEXTCLIP"))
    text_flags |= IUP_DRAW_CLIP;
  if (iupAttribGetBoolean(ih, "DRAWTEXTLAYOUTCENTER"))
    text_flags |= IUP_DRAW_LAYOUTCENTER;

  if (len == 0 || len == -1)
    len = (int)strlen(text);

  if (len != 0)
  {
    int txt_w, txt_h;
    char* font = iupDrawGetTextSize(ih, text, len, &txt_w, &txt_h, text_orientation);
    if (w == -1 || w == 0) w = txt_w;
    if (h == -1 || h == 0) h = txt_h;

    svg = IUP_SVG_GET(ih);
    if (svg)
    {
      char c[32]; iSvgColorStr(color, c, sizeof(c));
      iupSvgDrawText(svg, text, len, x, y, w, h, c, font, text_flags, text_orientation);
      return;
    }

    iupdrvDrawText((IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC"), text, len, x, y, w, h, color, font, text_flags, text_orientation);
  }
}

IUP_API void IupDrawGetTextSize(Ihandle* ih, const char* text, int len, int* w, int* h)
{
  double text_orientation;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  iupASSERT(text);
  if (!text)
    return;

  text_orientation = iupAttribGetDouble(ih, "DRAWTEXTORIENTATION");

  iupDrawGetTextSize(ih, text, len, w, h, text_orientation);
}

IUP_API void IupDrawGetTextMetrics(Ihandle* ih, int* ascent, int* descent, int* line_height)
{
  char* font;
  int max_width, lh, asc, desc;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  font = iupAttribGetStr(ih, "DRAWFONT");
  if (!font)
    font = IupGetAttribute(ih, "FONT");

  iupdrvFontGetFontDim(font, &max_width, &lh, &asc, &desc);

  if (ascent) *ascent = asc;
  if (descent) *descent = desc;
  if (line_height) *line_height = lh;
}

IUP_API void IupDrawGetImageInfo(const char* name, int* w, int* h, int* bpp)
{
  iupImageGetInfo(name, w, h, bpp);
}

IUP_API Ihandle* IupDrawGetImage(Ihandle* ih)
{
  IdrawCanvas* dc;
  int w, h;
  unsigned char* data;
  Ihandle* image;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return NULL;

  dc = (IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC");
  if (dc)
  {
    iupdrvDrawGetSize(dc, &w, &h);
    if (w <= 0 || h <= 0 || w > 32767 || h > 32767)
      return NULL;

    data = (unsigned char*)malloc(w * h * 4);
    if (!data)
      return NULL;

    if (!iupdrvDrawGetImageData(dc, data))
    {
      free(data);
      return NULL;
    }
  }
  else
  {
    IupGetIntInt(ih, "DRAWSIZE", &w, &h);
    if (w <= 0 || h <= 0 || w > 32767 || h > 32767)
      return NULL;

    data = (unsigned char*)malloc(w * h * 4);
    if (!data)
      return NULL;

    if (!iupdrvCanvasGetImageData(ih, data, w, h))
    {
      free(data);
      return NULL;
    }
  }

  image = IupImageRGBA(w, h, data);
  free(data);
  return image;
}

IUP_API char* IupDrawGetSvg(Ihandle* ih)
{
  int w, h;
  iSvgCanvas* svg;
  const char* str;
  char* result;
  Icallback action_cb;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return NULL;

  if (iupAttribGet(ih, "_IUP_DRAW_DC"))
    return NULL;

  IupGetIntInt(ih, "DRAWSIZE", &w, &h);
  if (w <= 0 || h <= 0)
    return NULL;

  svg = iupSvgDrawCreateCanvas(ih, w, h);
  if (!svg)
    return NULL;

  iupAttribSet(ih, "_IUP_SVG_CANVAS", (char*)svg);

  action_cb = IupGetCallback(ih, "ACTION");
  if (action_cb)
    action_cb(ih);

  str = iupSvgDrawGetString(svg);
  if (str)
  {
    int len = (int)strlen(str);
    result = (char*)malloc(len + 1);
    if (result)
      memcpy(result, str, len + 1);
  }
  else
    result = NULL;

  iupAttribSet(ih, "_IUP_SVG_CANVAS", NULL);
  iupSvgDrawKillCanvas(svg);

  return result;
}

static void iDrawGetImageRGBA(const char* name, int make_inactive, const char* bgcolor, unsigned char** out_rgba, int* out_w, int* out_h)
{
  Ihandle* img_ih;

  *out_rgba = NULL;
  *out_w = 0;
  *out_h = 0;

  img_ih = iupImageGetImageFromName(name);
  if (!img_ih)
    return;

  *out_rgba = iupImageGetRGBAData(img_ih, make_inactive, bgcolor, out_w, out_h);
}

IUP_API void IupDrawImage(Ihandle* ih, const char* name, int x, int y, int w, int h)
{
  char* bgcolor;
  char* value;
  int make_inactive, quality, opacity;
  int sx = 0, sy = 0, sw = -1, sh = -1;
  long tint;
  iSvgCanvas* svg;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (!iupAttribGet(ih, "_IUP_DRAW_DC"))
    return;

  bgcolor = iupAttribGetStr(ih, "DRAWBGCOLOR");
  make_inactive = iupAttribGetInt(ih, "DRAWMAKEINACTIVE");
  tint = iupDrawStrToColor(iupAttribGetStr(ih, "DRAWIMAGETINT"), IUP_DRAW_NO_TINT);

  opacity = iupAttribGetInt(ih, "DRAWIMAGEOPACITY");
  if (opacity < 0) opacity = 0;
  if (opacity > 255) opacity = 255;

  quality = IUP_DRAW_IMAGE_LINEAR;
  if (iupStrEqualNoCase(iupAttribGetStr(ih, "DRAWIMAGEQUALITY"), "NEAREST"))
    quality = IUP_DRAW_IMAGE_NEAREST;

  value = iupAttribGetStr(ih, "DRAWIMAGESRCRECT");
  if (value && sscanf(value, "%d %d %d %d", &sx, &sy, &sw, &sh) == 4 && sw > 0 && sh > 0)
  {
    int img_w = 0, img_h = 0;
    iupImageGetInfo(name, &img_w, &img_h, NULL);
    if (img_w > 0 && img_h > 0)
    {
      if (sx < 0) sx = 0;
      if (sy < 0) sy = 0;
      if (sx > img_w - 1) sx = img_w - 1;
      if (sy > img_h - 1) sy = img_h - 1;
      if (sx + sw > img_w) sw = img_w - sx;
      if (sy + sh > img_h) sh = img_h - sy;
    }
  }
  else
  {
    sx = 0;
    sy = 0;
    sw = -1;
    sh = -1;
  }

  svg = IUP_SVG_GET(ih);
  if (svg)
  {
    unsigned char* rgba;
    int img_w, img_h;
    iDrawGetImageRGBA(name, make_inactive, bgcolor, &rgba, &img_w, &img_h);
    if (rgba)
    {
      if (sw > 0 && sh > 0 && sw <= img_w && sh <= img_h)
      {
        int line;
        for (line = 0; line < sh; line++)
          memmove(rgba + (size_t)line * sw * 4, rgba + ((size_t)(sy + line) * img_w + sx) * 4, (size_t)sw * 4);
        img_w = sw;
        img_h = sh;
      }

      if (tint != IUP_DRAW_NO_TINT || opacity < 255)
      {
        int i, count = img_w * img_h;
        int tint_on = (tint != IUP_DRAW_NO_TINT);
        unsigned char tr = iupDrawRed(tint), tg = iupDrawGreen(tint), tb = iupDrawBlue(tint), ta = iupDrawAlpha(tint);
        for (i = 0; i < count; i++)
        {
          if (tint_on)
          {
            rgba[i * 4 + 0] = tr;
            rgba[i * 4 + 1] = tg;
            rgba[i * 4 + 2] = tb;
            rgba[i * 4 + 3] = (unsigned char)((rgba[i * 4 + 3] * ta) / 255);
          }
          rgba[i * 4 + 3] = (unsigned char)((rgba[i * 4 + 3] * opacity) / 255);
        }
      }

      if (w == -1 || w == 0) w = img_w;
      if (h == -1 || h == 0) h = img_h;
      iupSvgDrawImageRGBA(svg, rgba, img_w, img_h, x, y, w, h, quality);
      free(rgba);
    }
    return;
  }

  iupdrvDrawImage((IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC"), name, make_inactive, bgcolor, tint, opacity, x, y, w, h, sx, sy, sw, sh, quality);
}

IUP_API void IupDrawSetClipRect(Ihandle* ih, int x1, int y1, int x2, int y2)
{
  iSvgCanvas* svg;
  IupDrawState* state;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (!iupAttribGet(ih, "_IUP_DRAW_DC"))
    return;

  if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0)
  {
    IupDrawResetClip(ih);
    return;
  }

  state = iDrawStateGet(ih);
  if (!state)
    return;
  iDrawClipFree(&state->clip);
  state->clip.type = IUP_DRAW_CLIP_RECT;
  state->clip.x1 = x1; state->clip.y1 = y1;
  state->clip.x2 = x2; state->clip.y2 = y2;
  state->clip.matrix = state->matrix;

  svg = IUP_SVG_GET(ih);
  if (svg)
  {
    iupSvgDrawSetClipRect(svg, x1, y1, x2, y2);
    return;
  }

  iupdrvDrawSetClipRect((IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC"), x1, y1, x2, y2);
}

IUP_API void IupDrawSetClipRoundedRect(Ihandle* ih, int x1, int y1, int x2, int y2, int corner_radius)
{
  iSvgCanvas* svg;
  IupDrawState* state;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (!iupAttribGet(ih, "_IUP_DRAW_DC"))
    return;

  if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0)
  {
    IupDrawResetClip(ih);
    return;
  }

  state = iDrawStateGet(ih);
  if (!state)
    return;
  iDrawClipFree(&state->clip);
  state->clip.type = IUP_DRAW_CLIP_ROUNDED;
  state->clip.x1 = x1; state->clip.y1 = y1;
  state->clip.x2 = x2; state->clip.y2 = y2;
  state->clip.corner_radius = corner_radius;
  state->clip.matrix = state->matrix;

  svg = IUP_SVG_GET(ih);
  if (svg)
  {
    iupSvgDrawSetClipRoundedRect(svg, x1, y1, x2, y2, corner_radius);
    return;
  }

  iupdrvDrawSetClipRoundedRect((IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC"), x1, y1, x2, y2, corner_radius);
}

IUP_API void IupDrawGetClipRect(Ihandle* ih, int* x1, int* y1, int* x2, int* y2)
{
  iSvgCanvas* svg;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (!iupAttribGet(ih, "_IUP_DRAW_DC"))
    return;

  svg = IUP_SVG_GET(ih);
  if (svg)
  {
    iupSvgDrawGetClipRect(svg, x1, y1, x2, y2);
    return;
  }

  iupdrvDrawGetClipRect((IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC"), x1, y1, x2, y2);
}

IUP_API void IupDrawResetClip(Ihandle* ih)
{
  iSvgCanvas* svg;
  IupDrawState* state;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (!iupAttribGet(ih, "_IUP_DRAW_DC"))
    return;

  state = iDrawStateGet(ih);
  if (!state)
    return;
  iDrawClipFree(&state->clip);
  iDrawMatrixIdentity(&state->clip.matrix);

  svg = IUP_SVG_GET(ih);
  if (svg)
  {
    iupSvgDrawResetClip(svg);
    return;
  }

  iupdrvDrawResetClip((IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC"));
}

IUP_API void IupDrawSelectRect(Ihandle* ih, int x1, int y1, int x2, int y2)
{
  iSvgCanvas* svg;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (!iupAttribGet(ih, "_IUP_DRAW_DC"))
    return;

  svg = IUP_SVG_GET(ih);
  if (svg)
  {
    iupSvgDrawSelectRect(svg, x1, y1, x2, y2);
    return;
  }

  iupdrvDrawSelectRect((IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC"), x1, y1, x2, y2);
}

IUP_API void IupDrawFocusRect(Ihandle* ih, int x1, int y1, int x2, int y2)
{
  iSvgCanvas* svg;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (!iupAttribGet(ih, "_IUP_DRAW_DC"))
    return;

  svg = IUP_SVG_GET(ih);
  if (svg)
  {
    iupSvgDrawFocusRect(svg, x1, y1, x2, y2);
    return;
  }

  iupdrvDrawFocusRect((IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC"), x1, y1, x2, y2);
}

/************************************************************************************************/

IUP_SDK_API long iupDrawColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
  a = ~a;
  return (((unsigned long)a) << 24) |
         (((unsigned long)r) << 16) |
         (((unsigned long)g) << 8)  |
         (((unsigned long)b) << 0);
}

IUP_SDK_API long iupDrawColorMakeInactive(long color, long bgcolor)
{
  unsigned char r = iupDrawRed(color), g = iupDrawGreen(color), b = iupDrawBlue(color), a = iupDrawAlpha(color);
  unsigned char bg_r = iupDrawRed(bgcolor), bg_g = iupDrawGreen(bgcolor), bg_b = iupDrawBlue(bgcolor);
  iupImageColorMakeInactive(&r, &g, &b, bg_r, bg_g, bg_b);
  return iupDrawColor(r, g, b, a);
}

IUP_SDK_API long iupDrawStrToColor(const char* str, long c_def)
{
  unsigned char r, g, b, a;
  if (iupStrToRGBA(str, &r, &g, &b, &a))
    return iupDrawColor(r, g, b, a);
  else
    return c_def;
}

IUP_SDK_API int iupDrawPathArcToCurves(const IupPathSeg* seg, double* out)
{
  double cx = seg->x1, cy = seg->y1, rx = seg->x2, ry = seg->y2;
  double a1, span, step, k;
  int i, n;

  if (rx <= 0 || ry <= 0)
    return 0;

  span = seg->a2 - seg->a1;
  while (span < 0) span += 360.0;
  while (span > 360.0) span -= 360.0;
  if (span < 0.01)
    return 0;

  n = (int)ceil(span / 90.0);
  if (n > 4) n = 4;

  a1 = seg->a1 * IUP_DEG2RAD;
  step = span * IUP_DEG2RAD / n;
  k = (4.0 / 3.0) * tan(step / 4.0);

  for (i = 0; i < n; i++)
  {
    double t0 = a1 + i * step;
    double t1 = a1 + (i + 1) * step;
    double p0x = cx + rx * cos(t0), p0y = cy - ry * sin(t0);
    double p3x = cx + rx * cos(t1), p3y = cy - ry * sin(t1);
    double* c = out + i * 6;

    c[0] = p0x + k * (-rx * sin(t0));
    c[1] = p0y + k * (-ry * cos(t0));
    c[2] = p3x - k * (-rx * sin(t1));
    c[3] = p3y - k * (-ry * cos(t1));
    c[4] = p3x;
    c[5] = p3y;
  }

  return n;
}

IUP_SDK_API int iupDrawPathArcToBeziers(const IupPathSeg* seg, IupPathSeg* out)
{
  double curves[24];
  int i, n = iupDrawPathArcToCurves(seg, curves);

  for (i = 0; i < n; i++)
  {
    const double* c = curves + i * 6;
    memset(&out[i], 0, sizeof(IupPathSeg));
    out[i].op = IUP_PATHSEG_CURVE_TO;
    out[i].x1 = iupROUND(c[0]);
    out[i].y1 = iupROUND(c[1]);
    out[i].x2 = iupROUND(c[2]);
    out[i].y2 = iupROUND(c[3]);
    out[i].x3 = iupROUND(c[4]);
    out[i].y3 = iupROUND(c[5]);
  }

  return n;
}

IUP_SDK_API void iupDrawPathGetBBox(const IupPathSeg* segs, int count, int* x1, int* y1, int* x2, int* y2)
{
  int i, min_x = 0, min_y = 0, max_x = 0, max_y = 0, has = 0;

  for (i = 0; i < count; i++)
  {
    int px[6], py[6], np = 0;

    switch (segs[i].op)
    {
    case IUP_PATHSEG_MOVE_TO:
    case IUP_PATHSEG_LINE_TO:
      px[0] = segs[i].x1; py[0] = segs[i].y1; np = 1;
      break;
    case IUP_PATHSEG_QUAD_TO:
      px[0] = segs[i].x1; py[0] = segs[i].y1;
      px[1] = segs[i].x2; py[1] = segs[i].y2; np = 2;
      break;
    case IUP_PATHSEG_CURVE_TO:
      px[0] = segs[i].x1; py[0] = segs[i].y1;
      px[1] = segs[i].x2; py[1] = segs[i].y2;
      px[2] = segs[i].x3; py[2] = segs[i].y3; np = 3;
      break;
    case IUP_PATHSEG_ARC_TO:
      px[0] = segs[i].x1 - segs[i].x2; py[0] = segs[i].y1 - segs[i].y2;
      px[1] = segs[i].x1 + segs[i].x2; py[1] = segs[i].y1 + segs[i].y2; np = 2;
      break;
    default:
      break;
    }

    while (np--)
    {
      if (!has)
      {
        min_x = max_x = px[np];
        min_y = max_y = py[np];
        has = 1;
      }
      else
      {
        if (px[np] < min_x) min_x = px[np];
        if (px[np] > max_x) max_x = px[np];
        if (py[np] < min_y) min_y = py[np];
        if (py[np] > max_y) max_y = py[np];
      }
    }
  }

  if (x1) *x1 = has ? min_x : 0;
  if (y1) *y1 = has ? min_y : 0;
  if (x2) *x2 = has ? max_x : 0;
  if (y2) *y2 = has ? max_y : 0;
}

typedef struct _IupDrawFlatPath
{
  IupPathSeg* segs;
  int count, cap;
} IupDrawFlatPath;

static void iDrawFlatAppend(IupDrawFlatPath* fp, int op, double x, double y)
{
  if (fp->count == fp->cap)
  {
    int cap = fp->cap ? fp->cap * 2 : 64;
    IupPathSeg* segs = (IupPathSeg*)realloc(fp->segs, (size_t)cap * sizeof(IupPathSeg));
    if (!segs)
      return;
    fp->segs = segs;
    fp->cap = cap;
  }

  memset(&fp->segs[fp->count], 0, sizeof(IupPathSeg));
  fp->segs[fp->count].op = (unsigned char)op;
  fp->segs[fp->count].x1 = iupROUND(x);
  fp->segs[fp->count].y1 = iupROUND(y);
  fp->count++;
}

static void iDrawFlattenCubic(IupDrawFlatPath* fp, double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3, int depth)
{
  double dx = x3 - x0, dy = y3 - y0;
  double chord = sqrt(dx * dx + dy * dy);
  double tol = 0.5;

  if (depth >= 16 ||
      (chord < 0.001 &&
       (x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0) < tol * tol &&
       (x2 - x0) * (x2 - x0) + (y2 - y0) * (y2 - y0) < tol * tol) ||
      (chord >= 0.001 &&
       fabs((x1 - x0) * dy - (y1 - y0) * dx) <= tol * chord &&
       fabs((x2 - x0) * dy - (y2 - y0) * dx) <= tol * chord))
  {
    iDrawFlatAppend(fp, IUP_PATHSEG_LINE_TO, x3, y3);
    return;
  }

  {
    double ax1 = (x0 + x1) / 2, ay1 = (y0 + y1) / 2;
    double ax2 = (x1 + x2) / 2, ay2 = (y1 + y2) / 2;
    double ax3 = (x2 + x3) / 2, ay3 = (y2 + y3) / 2;
    double bx1 = (ax1 + ax2) / 2, by1 = (ay1 + ay2) / 2;
    double bx2 = (ax2 + ax3) / 2, by2 = (ay2 + ay3) / 2;
    double mx = (bx1 + bx2) / 2, my = (by1 + by2) / 2;

    iDrawFlattenCubic(fp, x0, y0, ax1, ay1, bx1, by1, mx, my, depth + 1);
    iDrawFlattenCubic(fp, mx, my, bx2, by2, ax3, ay3, x3, y3, depth + 1);
  }
}

static void iDrawFlattenSeg(IupDrawFlatPath* fp, const IupPathSeg* seg, double cur_x, double cur_y)
{
  if (seg->op == IUP_PATHSEG_CURVE_TO)
    iDrawFlattenCubic(fp, cur_x, cur_y, seg->x1, seg->y1, seg->x2, seg->y2, seg->x3, seg->y3, 0);
  else if (seg->op == IUP_PATHSEG_QUAD_TO)
  {
    double x1 = cur_x + (2.0 / 3.0) * (seg->x1 - cur_x);
    double y1 = cur_y + (2.0 / 3.0) * (seg->y1 - cur_y);
    double x2 = seg->x2 + (2.0 / 3.0) * (seg->x1 - seg->x2);
    double y2 = seg->y2 + (2.0 / 3.0) * (seg->y1 - seg->y2);
    iDrawFlattenCubic(fp, cur_x, cur_y, x1, y1, x2, y2, seg->x2, seg->y2, 0);
  }
  else
  {
    IupPathSeg bez[4];
    int i, n = iupDrawPathArcToBeziers(seg, bez);
    for (i = 0; i < n; i++)
    {
      iDrawFlattenCubic(fp, cur_x, cur_y, bez[i].x1, bez[i].y1, bez[i].x2, bez[i].y2, bez[i].x3, bez[i].y3, 0);
      cur_x = bez[i].x3;
      cur_y = bez[i].y3;
    }
  }
}

IUP_SDK_API int iupDrawPathFlatten(const IupPathSeg* segs, int count, IupPathSeg** out_segs)
{
  IupDrawFlatPath fp;
  double cur_x = 0, cur_y = 0;
  int sub_x = 0, sub_y = 0;
  int i;

  memset(&fp, 0, sizeof(fp));

  for (i = 0; i < count; i++)
  {
    switch (segs[i].op)
    {
    case IUP_PATHSEG_ARC_TO:
      iDrawFlattenSeg(&fp, &segs[i], cur_x, cur_y);
      cur_x = iupROUND(segs[i].x1 + segs[i].x2 * cos(segs[i].a2 * IUP_DEG2RAD));
      cur_y = iupROUND(segs[i].y1 - segs[i].y2 * sin(segs[i].a2 * IUP_DEG2RAD));
      break;
    case IUP_PATHSEG_CURVE_TO:
      iDrawFlattenSeg(&fp, &segs[i], cur_x, cur_y);
      cur_x = segs[i].x3; cur_y = segs[i].y3;
      break;
    case IUP_PATHSEG_QUAD_TO:
      iDrawFlattenSeg(&fp, &segs[i], cur_x, cur_y);
      cur_x = segs[i].x2; cur_y = segs[i].y2;
      break;
    case IUP_PATHSEG_MOVE_TO:
      sub_x = segs[i].x1; sub_y = segs[i].y1;
      iDrawFlatAppend(&fp, IUP_PATHSEG_MOVE_TO, segs[i].x1, segs[i].y1);
      cur_x = segs[i].x1; cur_y = segs[i].y1;
      break;
    case IUP_PATHSEG_LINE_TO:
      iDrawFlatAppend(&fp, IUP_PATHSEG_LINE_TO, segs[i].x1, segs[i].y1);
      cur_x = segs[i].x1; cur_y = segs[i].y1;
      break;
    case IUP_PATHSEG_CLOSE:
      iDrawFlatAppend(&fp, IUP_PATHSEG_CLOSE, 0, 0);
      cur_x = sub_x; cur_y = sub_y;
      break;
    }
  }

  *out_segs = fp.segs;
  return fp.count;
}


IUP_SDK_API void iupDrawSetColor(Ihandle* ih, const char* name, long color)
{
  char value[60];
  unsigned char a = iupDrawAlpha(color);
  if (a < 255)
    snprintf(value, sizeof(value), "%d %d %d %d", (int)iupDrawRed(color), (int)iupDrawGreen(color), (int)iupDrawBlue(color), (int)a);
  else
    snprintf(value, sizeof(value), "%d %d %d", (int)iupDrawRed(color), (int)iupDrawGreen(color), (int)iupDrawBlue(color));
  iupAttribSetStr(ih, name, value);
}

IUP_SDK_API void iupDrawRaiseRect(Ihandle* ih, int x1, int y1, int x2, int y2, long light_shadow, long mid_shadow, long dark_shadow)
{
  iupDrawSetColor(ih, "DRAWCOLOR", light_shadow);
  IupDrawLine(ih, x1, y1, x1, y2);
  IupDrawLine(ih, x1, y1, x2, y1);

  iupDrawSetColor(ih, "DRAWCOLOR", mid_shadow);
  IupDrawLine(ih, x1 + 1, y2 - 1, x2 - 1, y2 - 1);
  IupDrawLine(ih, x2 - 1, y1 + 1, x2 - 1, y2 - 1);

  iupDrawSetColor(ih, "DRAWCOLOR", dark_shadow);
  IupDrawLine(ih, x1, y2, x2, y2);
  IupDrawLine(ih, x2, y1, x2, y2);
}

IUP_SDK_API void iupDrawVertSunkenMark(Ihandle* ih, int x, int y1, int y2, long light_shadow, long dark_shadow)
{
  iupDrawSetColor(ih, "DRAWCOLOR", dark_shadow);
  IupDrawLine(ih, x - 1, y1, x - 1, y2);
  iupDrawSetColor(ih, "DRAWCOLOR", light_shadow);
  IupDrawLine(ih, x, y1, x, y2);
}

IUP_SDK_API void iupDrawHorizSunkenMark(Ihandle* ih, int x1, int x2, int y, long light_shadow, long dark_shadow)
{
  iupDrawSetColor(ih, "DRAWCOLOR", dark_shadow);
  IupDrawLine(ih, x1, y - 1, x2, y - 1);
  iupDrawSetColor(ih, "DRAWCOLOR", light_shadow);
  IupDrawLine(ih, x1, y, x2, y);
}

IUP_SDK_API void iupDrawSunkenRect(Ihandle* ih, int x1, int y1, int x2, int y2, long light_shadow, long mid_shadow, long dark_shadow)
{
  iupDrawSetColor(ih, "DRAWCOLOR", mid_shadow);
  IupDrawLine(ih, x1, y1, x1, y2);
  IupDrawLine(ih, x1, y1, x2, y1);

  iupDrawSetColor(ih, "DRAWCOLOR", dark_shadow);
  IupDrawLine(ih, x1 + 1, y1 + 1, x1 + 1, y2 - 1);
  IupDrawLine(ih, x1 + 1, y1 + 1, x2 - 1, y1 + 1);

  iupDrawSetColor(ih, "DRAWCOLOR", light_shadow);
  IupDrawLine(ih, x1, y2, x2, y2);
  IupDrawLine(ih, x2, y1, x2, y2);
}

IUP_SDK_API void iupDrawCalcShadows(long bgcolor, long* light_shadow, long* mid_shadow, long* dark_shadow)
{
  int r, bg_r = iupDrawRed(bgcolor);
  int g, bg_g = iupDrawGreen(bgcolor);
  int b, bg_b = iupDrawBlue(bgcolor);

  /* light_shadow */

  r = bg_r + ((255 - bg_r) * 45) / 100;
  g = bg_g + ((255 - bg_g) * 45) / 100;
  b = bg_b + ((255 - bg_b) * 45) / 100;

  if (light_shadow) *light_shadow = iupDrawColor((unsigned char)r, (unsigned char)g, (unsigned char)b, 255);

  /* dark_shadow */
  r = (bg_r * 55) / 100;
  g = (bg_g * 55) / 100;
  b = (bg_b * 55) / 100;

  if (dark_shadow) *dark_shadow = iupDrawColor((unsigned char)r, (unsigned char)g, (unsigned char)b, 255);

  /*   mid_shadow = (dark_shadow+bgcolor)/2    */
  if (mid_shadow) *mid_shadow = iupDrawColor((unsigned char)((bg_r + r) / 2), (unsigned char)((bg_g + g) / 2), (unsigned char)((bg_b + b) / 2), 255);
}

IUP_SDK_API void iupDrawParentBackground(IdrawCanvas* dc, Ihandle* ih)
{
  long color;
  int w, h;
  char* color_str = iupBaseNativeParentGetBgColorAttrib(ih);
  color = iupDrawStrToColor(color_str, 0);
  iupdrvDrawGetSize(dc, &w, &h);
  iupdrvDrawRectangle(dc, 0, 0, w - 1, h - 1, color, IUP_DRAW_FILL, 1);
}

/***********************************************************************************************/

static long iFlatDrawColorMakeInactive(long color, const char* bgcolor)
{
  unsigned char bg_r = 0, bg_g = 0, bg_b = 0;
  unsigned char r = iupDrawRed(color), g = iupDrawGreen(color), b = iupDrawBlue(color), a = iupDrawAlpha(color);
  iupStrToRGB(bgcolor, &bg_r, &bg_g, &bg_b);
  iupImageColorMakeInactive(&r, &g, &b, bg_r, bg_g, bg_b);
  return iupDrawColor(r, g, b, a);
}

IUP_SDK_API void iupFlatDrawBorder(IdrawCanvas* dc, int xmin, int xmax, int ymin, int ymax, int border_width, const char* fgcolor, const char* bgcolor, int active)
{
  long color = 0;

  if (!fgcolor || border_width == 0 || xmin == xmax || ymin == ymax)
    return;

  iupDrawCheckSwapCoord(xmin, xmax);
  iupDrawCheckSwapCoord(ymin, ymax);

  color = iupDrawStrToColor(fgcolor, 0);
  if (!active)
    color = iFlatDrawColorMakeInactive(color, bgcolor);

  iupdrvDrawRectangle(dc, xmin, ymin, xmax, ymax, color, IUP_DRAW_STROKE, 1);
  while (border_width > 1)
  {
    border_width--;
    iupdrvDrawRectangle(dc, xmin + border_width,
                        ymin + border_width,
                        xmax - border_width,
                        ymax - border_width, color, IUP_DRAW_STROKE, 1);
  }
}

IUP_SDK_API void iupFlatDrawBox(IdrawCanvas* dc, int xmin, int xmax, int ymin, int ymax, const char* fgcolor, const char* bgcolor, int active)
{
  long color;

  if (!fgcolor || xmin == xmax || ymin == ymax)
    return;

  iupDrawCheckSwapCoord(xmin, xmax);
  iupDrawCheckSwapCoord(ymin, ymax);

  color = iupDrawStrToColor(fgcolor, 0);
  if (!active)
    color = iFlatDrawColorMakeInactive(color, bgcolor);

  iupdrvDrawRectangle(dc, xmin, ymin, xmax, ymax, color, IUP_DRAW_FILL, 1);
}

IUP_SDK_API void iupFlatDrawRoundedBorder(IdrawCanvas* dc, int xmin, int xmax, int ymin, int ymax, int border_width, int corner_radius, const char* fgcolor, const char* bgcolor, int active)
{
  long color = 0;

  if (!fgcolor || border_width == 0 || xmin == xmax || ymin == ymax)
    return;

  iupDrawCheckSwapCoord(xmin, xmax);
  iupDrawCheckSwapCoord(ymin, ymax);

  color = iupDrawStrToColor(fgcolor, 0);
  if (!active)
    color = iFlatDrawColorMakeInactive(color, bgcolor);

  iupdrvDrawRoundedRectangle(dc, xmin, ymin, xmax, ymax, corner_radius, color, IUP_DRAW_STROKE, 1);
  while (border_width > 1)
  {
    border_width--;
    iupdrvDrawRoundedRectangle(dc, xmin + border_width,
                                ymin + border_width,
                                xmax - border_width,
                                ymax - border_width, corner_radius, color, IUP_DRAW_STROKE, 1);
  }
}

IUP_SDK_API void iupFlatDrawRoundedBox(IdrawCanvas* dc, int xmin, int xmax, int ymin, int ymax, int corner_radius, const char* fgcolor, const char* bgcolor, int active)
{
  long color;

  if (!fgcolor || xmin == xmax || ymin == ymax)
    return;

  iupDrawCheckSwapCoord(xmin, xmax);
  iupDrawCheckSwapCoord(ymin, ymax);

  color = iupDrawStrToColor(fgcolor, 0);
  if (!active)
    color = iFlatDrawColorMakeInactive(color, bgcolor);

  iupdrvDrawRoundedRectangle(dc, xmin, ymin, xmax, ymax, corner_radius, color, IUP_DRAW_FILL, 1);
}

IUP_SDK_API void iupFlatDrawGradientBox(IdrawCanvas* dc, int xmin, int xmax, int ymin, int ymax, int corner_radius, float angle, const char* color1, const char* color2, const char* bgcolor, int active)
{
  long c1, c2;

  if (!color1 || !color2 || xmin == xmax || ymin == ymax)
    return;

  iupDrawCheckSwapCoord(xmin, xmax);
  iupDrawCheckSwapCoord(ymin, ymax);

  c1 = iupDrawStrToColor(color1, 0);
  c2 = iupDrawStrToColor(color2, 0);
  if (!active)
  {
    c1 = iFlatDrawColorMakeInactive(c1, bgcolor);
    c2 = iFlatDrawColorMakeInactive(c2, bgcolor);
  }

  {
    long colors[2]; float offsets[2];
    colors[0] = c1; colors[1] = c2;
    offsets[0] = 0.0f; offsets[1] = 1.0f;

    if (corner_radius > 0)
    {
      iupdrvDrawSetClipRoundedRect(dc, xmin, ymin, xmax, ymax, corner_radius);
      iupdrvDrawLinearGradient(dc, xmin, ymin, xmax, ymax, angle, colors, offsets, 2);
      iupdrvDrawResetClip(dc);
    }
    else
      iupdrvDrawLinearGradient(dc, xmin, ymin, xmax, ymax, angle, colors, offsets, 2);
  }
}

static long iFlatDrawColorShade(long color, int shade)
{
  int r = iupDrawRed(color), g = iupDrawGreen(color), b = iupDrawBlue(color);
  unsigned char a = iupDrawAlpha(color);
  if (shade > 0)
  {
    r += (255 - r) * shade / 100;
    g += (255 - g) * shade / 100;
    b += (255 - b) * shade / 100;
  }
  else if (shade < 0)
  {
    r += r * shade / 100;
    g += g * shade / 100;
    b += b * shade / 100;
  }
  return iupDrawColor((unsigned char)r, (unsigned char)g, (unsigned char)b, a);
}

IUP_SDK_API void iupFlatDrawGradientBoxStops(IdrawCanvas* dc, int xmin, int xmax, int ymin, int ymax, int corner_radius, float angle, const char* gradient, const char* bgcolor, int active, int shade)
{
  long colors[IUP_GRADIENT_MAX_STOPS];
  float offsets[IUP_GRADIENT_MAX_STOPS];
  int i, count = 0;
  const char* p = gradient;

  if (!gradient || xmin == xmax || ymin == ymax)
    return;

  iupDrawCheckSwapCoord(xmin, xmax);
  iupDrawCheckSwapCoord(ymin, ymax);

  while (p && count < IUP_GRADIENT_MAX_STOPS)
  {
    char token[30];
    const char* sep = strchr(p, ':');
    int len = sep ? (int)(sep - p) : (int)strlen(p);
    if (len > (int)sizeof(token) - 1) len = (int)sizeof(token) - 1;
    memcpy(token, p, len);
    token[len] = 0;
    colors[count++] = iupDrawStrToColor(token, 0);
    if (!sep) break;
    p = sep + 1;
  }

  if (count == 0)
    return;

  if (count == 1)
    colors[count++] = iupDrawStrToColor(bgcolor, 0);

  if (shade != 0)
    for (i = 0; i < count; i++)
      colors[i] = iFlatDrawColorShade(colors[i], shade);

  if (!active)
    for (i = 0; i < count; i++)
      colors[i] = iFlatDrawColorMakeInactive(colors[i], bgcolor);

  for (i = 0; i < count; i++)
    offsets[i] = (float)i / (float)(count - 1);

  if (corner_radius > 0)
  {
    iupdrvDrawSetClipRoundedRect(dc, xmin, ymin, xmax, ymax, corner_radius);
    iupdrvDrawLinearGradient(dc, xmin, ymin, xmax, ymax, angle, colors, offsets, count);
    iupdrvDrawResetClip(dc);
  }
  else
    iupdrvDrawLinearGradient(dc, xmin, ymin, xmax, ymax, angle, colors, offsets, count);
}

static void iFlatDrawText(IdrawCanvas* dc, int x, int y, int w, int h, const char* str, const char* font, int text_flags, double text_orientation, const char* fgcolor, const char* bgcolor, int active)
{
  long color;

  if (!fgcolor || !str || str[0] == 0)
    return;

  color = iupDrawStrToColor(fgcolor, 0);
  if (!active)
    color = iFlatDrawColorMakeInactive(color, bgcolor);

  iupdrvDrawText(dc, str, (int)strlen(str), x, y, w, h, color, font, text_flags | IUP_DRAW_LAYOUTCENTER, text_orientation);  /* layout is always center here */
}

static void iFlatGetIconPosition(int icon_width, int icon_height, int* x, int* y, int width, int height, int horiz_alignment, int vert_alignment)
{
  if (horiz_alignment == IUP_ALIGN_ARIGHT)
    *x = icon_width - width;
  else if (horiz_alignment == IUP_ALIGN_ACENTER)
    *x = (icon_width - width) / 2;
  else  /* ALEFT */
    *x = 0;

  if (vert_alignment == IUP_ALIGN_ABOTTOM)
    *y = icon_height - height;
  else if (vert_alignment == IUP_ALIGN_ACENTER)
    *y = (icon_height - height) / 2;
  else  /* ATOP */
    *y = 0;
}

static void iFlatGetImageTextPosition(int x, int y, int img_position, int spacing,
                                        int img_width, int img_height, int txt_width, int txt_height,
                                        int* img_x, int* img_y, int* txt_x, int* txt_y)
{
  switch (img_position)
  {
  case IUP_IMGPOS_TOP:
    *img_y = y;
    *txt_y = y + img_height + spacing;
    if (img_width > txt_width)
    {
      *img_x = x;
      *txt_x = x + (img_width - txt_width) / 2;
    }
    else
    {
      *img_x = x + (txt_width - img_width) / 2;
      *txt_x = x;
    }
    break;
  case IUP_IMGPOS_BOTTOM:
    *img_y = y + txt_height + spacing;
    *txt_y = y;
    if (img_width > txt_width)
    {
      *img_x = x;
      *txt_x = x + (img_width - txt_width) / 2;
    }
    else
    {
      *img_x = x + (txt_width - img_width) / 2;
      *txt_x = x;
    }
    break;
  case IUP_IMGPOS_RIGHT:
    *img_x = x + txt_width + spacing;
    *txt_x = x;
    if (img_height > txt_height)
    {
      *img_y = y;
      *txt_y = y + (img_height - txt_height) / 2;
    }
    else
    {
      *img_y = y + (txt_height - img_height) / 2;
      *txt_y = y;
    }
    break;
  default: /* IUP_IMGPOS_LEFT (image at left of text) */
    *img_x = x;
    *txt_x = x + img_width + spacing;
    if (img_height > txt_height)
    {
      *img_y = y;
      *txt_y = y + (img_height - txt_height) / 2;
    }
    else
    {
      *img_y = y + (txt_height - img_height) / 2;
      *txt_y = y;
    }
    break;
  }
}

IUP_SDK_API void iupFlatDrawGetIconSize(Ihandle* ih, int img_position, int spacing, int horiz_padding, int vert_padding,
                            const char* imagename, const char* title, int* w, int* h, double text_orientation)
{
  if (imagename)
  {
    int img_width = 0, img_height = 0;
    iupImageGetInfo(imagename, &img_width, &img_height, NULL);

    if (title)
    {
      int txt_width, txt_height;
      iupDrawGetTextSize(ih, title, 0, &txt_width, &txt_height, text_orientation);

      if (img_position == IUP_IMGPOS_RIGHT || img_position == IUP_IMGPOS_LEFT)
      {
        *w = img_width + txt_width + spacing;
        *h = iupMAX(img_height, txt_height);
      }
      else
      {
        *w = iupMAX(img_width, txt_width);
        *h = img_height + txt_height + spacing;
      }
    }
    else
    {
      *w = img_width;
      *h = img_height;
    }
  }
  else if (title)
  {
    int txt_width, txt_height;
    iupDrawGetTextSize(ih, title, 0, &txt_width, &txt_height, text_orientation);

    *w = txt_width;
    *h = txt_height;
  }
  else
  {
    *w = 0;
    *h = 0;
  }

  *w += 2 * horiz_padding;
  *h += 2 * vert_padding;

  /* leave room for focus feedback */
  if (ih->iclass->is_interactive && iupAttribGetBoolean(ih, "CANFOCUS") && iupAttribGetBoolean(ih, "FOCUSFEEDBACK"))
  {
    *w += 2 * 2;  /* space between focus rect and contents */
    *h += 2 * 2;
  }
}

IUP_SDK_API void iupFlatDrawIcon(Ihandle* ih, IdrawCanvas* dc, int icon_x, int icon_y, int icon_width, int icon_height,
                     int img_position, int spacing, int horiz_alignment, int vert_alignment, int horiz_padding, int vert_padding,
                     const char* imagename, int make_inactive, const char* title, int text_flags, double text_orientation, const char* fgcolor, const char* bgcolor, int active)
{
  int x, y, width, height;
  int txt_width, txt_height;
  char* font;
  int clip_x1, clip_y1, clip_x2, clip_y2;

  iupdrvDrawGetClipRect(dc, &clip_x1, &clip_y1, &clip_x2, &clip_y2);
  if (clip_x1 != 0 || clip_y1 != 0 || clip_x2 != 0 || clip_y2)
    iupdrvDrawSetClipRect(dc, iupMAX(icon_x, clip_x1), iupMAX(icon_y, clip_y1), iupMIN(icon_x + icon_width, clip_x2), iupMIN(icon_y + icon_height, clip_y2));  /* intersect */
  else
    iupdrvDrawSetClipRect(dc, icon_x, icon_y, icon_x + icon_width, icon_y + icon_height);

  /* clipping allows the padding area to be drawn */
  icon_width -= 2 * horiz_padding;
  icon_height -= 2 * vert_padding;
  icon_x += horiz_padding;
  icon_y += vert_padding;

  if (imagename)
  {
    int img_width = 0, img_height = 0;
    iupImageGetInfo(imagename, &img_width, &img_height, NULL);

    if (title)
    {
      int img_x, img_y, txt_x, txt_y;

      font = iupDrawGetTextSize(ih, title, 0, &txt_width, &txt_height, text_orientation);

      /* first combine image and text */
      if (img_position == IUP_IMGPOS_RIGHT || img_position == IUP_IMGPOS_LEFT)
      {
        int max_txt_width = icon_width - img_width - spacing;
        if (max_txt_width < 0) max_txt_width = 0;

        /* the text can be larger than the icon space, so wrap and ellipsis can work */
        txt_width = iupMIN(max_txt_width, txt_width);
        if (text_flags & IUP_DRAW_WRAP)
          txt_height = icon_height;
        else
          txt_height = iupMIN(icon_height, txt_height);

        width = img_width + txt_width + spacing;
        height = iupMAX(img_height, txt_height);
      }
      else
      {
        int max_txt_height = icon_height - img_height - spacing;
        if (max_txt_height < 0) max_txt_height = 0;

        /* the text can be larger than the icon space, so wrap and ellipsis can work */
        txt_width = iupMIN(icon_width, txt_width);
        if (text_flags & IUP_DRAW_WRAP)
          txt_height = max_txt_height;
        else
          txt_height = iupMIN(max_txt_height, txt_height);

        width = iupMAX(img_width, txt_width);
        height = img_height + txt_height + spacing;
      }

      iFlatGetIconPosition(icon_width, icon_height, &x, &y, width, height, horiz_alignment, vert_alignment);

      iFlatGetImageTextPosition(x, y, img_position, spacing,
                                img_width, img_height, txt_width, txt_height,
                                &img_x, &img_y, &txt_x, &txt_y);

      iupdrvDrawImage(dc, imagename, make_inactive, bgcolor, IUP_DRAW_NO_TINT, 255, icon_x + img_x, icon_y + img_y, img_width, img_height, 0, 0, -1, -1, IUP_DRAW_IMAGE_LINEAR);  /* no zoom */
      iFlatDrawText(dc, icon_x + txt_x, icon_y + txt_y, txt_width, txt_height, title, font, text_flags, text_orientation, fgcolor, bgcolor, active);
    }
    else
    {
      /* if image is larger than the icon space, then the position can be negative, clipping will crop the result */
      width = img_width;
      height = img_height;

      iFlatGetIconPosition(icon_width, icon_height, &x, &y, width, height, horiz_alignment, vert_alignment);

      iupdrvDrawImage(dc, imagename, make_inactive, bgcolor, IUP_DRAW_NO_TINT, 255, icon_x + x, icon_y + y, img_width, img_height, 0, 0, -1, -1, IUP_DRAW_IMAGE_LINEAR);  /* no zoom */
    }
  }
  else if (title)
  {
    font = iupDrawGetTextSize(ih, title, 0, &txt_width, &txt_height, text_orientation);

    /* the text can be larger than the icon space, so wrap and ellipsis can work */
    width = iupMIN(icon_width, txt_width);
    if (text_flags & IUP_DRAW_WRAP)
      height = icon_height;
    else
      height = iupMIN(icon_height, txt_height);

    iFlatGetIconPosition(icon_width, icon_height, &x, &y, width, height, horiz_alignment, vert_alignment);

    iFlatDrawText(dc, icon_x + x, icon_y + y, width, height, title, font, text_flags, text_orientation, fgcolor, bgcolor, active);
  }

  iupdrvDrawSetClipRect(dc, clip_x1, clip_y1, clip_x2, clip_y2);

  if (title && !iupAttribGet(ih, "ACCESSIBLETITLE") && !iupAttribGet(ih, "SECONDARYTITLE"))
    iupdrvSetAccessibleTitle(ih, title);  /* for accessibility */
}

IUP_SDK_API int iupFlatGetHorizontalAlignment(const char* value)
{
  int horiz_alignment = IUP_ALIGN_ACENTER;  /* default always "ACENTER" */
  if (iupStrEqualNoCase(value, "ARIGHT"))
    horiz_alignment = IUP_ALIGN_ARIGHT;
  else if (iupStrEqualNoCase(value, "ALEFT"))
    horiz_alignment = IUP_ALIGN_ALEFT;
  return horiz_alignment;
}

IUP_SDK_API int iupFlatGetVerticalAlignment(const char* value)
{
  int vert_alignment = IUP_ALIGN_ACENTER;  /* default always "ACENTER" */
  if (iupStrEqualNoCase(value, "ABOTTOM"))
    vert_alignment = IUP_ALIGN_ABOTTOM;
  else if (iupStrEqualNoCase(value, "ATOP"))
    vert_alignment = IUP_ALIGN_ATOP;
  return vert_alignment;
}

IUP_SDK_API int iupFlatGetImagePosition(const char* value)
{
  int img_position = IUP_IMGPOS_LEFT; /* default always "LEFT" */
  if (iupStrEqualNoCase(value, "RIGHT"))
    img_position = IUP_IMGPOS_RIGHT;
  else if (iupStrEqualNoCase(value, "BOTTOM"))
    img_position = IUP_IMGPOS_BOTTOM;
  else if (iupStrEqualNoCase(value, "TOP"))
    img_position = IUP_IMGPOS_TOP;
  return img_position;
}

IUP_SDK_API void iupFlatDrawArrow(IdrawCanvas* dc, int x, int y, int size, const char* color_str, const char* bgcolor, int active, int dir)
{
  int points[6];

  int off1 = iupRound((double)size * 0.13);
  int off2 = iupRound((double)size * 0.87);
  int half = size / 2;

  long color = iupDrawStrToColor(color_str, 0);
  if (!active)
    color = iFlatDrawColorMakeInactive(color, bgcolor);

  switch (dir)
  {
  case IUPDRAW_ARROW_LEFT:  /* arrow points left */
    points[0] = x + off2;
    points[1] = y;
    points[2] = x + off2;
    points[3] = y + size;
    points[4] = x + off1;
    points[5] = y + half;
    break;
  case IUPDRAW_ARROW_TOP:    /* arrow points top */
    points[0] = x;
    points[1] = y + off2;
    points[2] = x + size;
    points[3] = y + off2;
    points[4] = x + half;
    points[5] = y + off1;
    break;
  case IUPDRAW_ARROW_RIGHT:  /* arrow points right */
    points[0] = x + off1;
    points[1] = y;
    points[2] = x + off1;
    points[3] = y + size;
    points[4] = x + size - off1;
    points[5] = y + half;
    break;
  case IUPDRAW_ARROW_BOTTOM:  /* arrow points bottom */
    points[0] = x;
    points[1] = y + off1;
    points[2] = x + size;
    points[3] = y + off1;
    points[4] = x + half;
    points[5] = y + size - off1;
    break;
  }

  iupdrvDrawPolygon(dc, points, 3, color, IUP_DRAW_FILL, 1);
  iupdrvDrawPolygon(dc, points, 3, color, IUP_DRAW_STROKE, 1);
}

IUP_SDK_API void iupFlatDrawCheckMark(IdrawCanvas* dc, int xmin, int xmax, int ymin, int ymax, const char* color_str, const char* bgcolor, int active)
{
  int points[6];

  long color = iupDrawStrToColor(color_str, 0);
  if (!active)
    color = iFlatDrawColorMakeInactive(color, bgcolor);

  points[0] = xmin;
  points[1] = (ymax + ymin) / 2;
  points[2] = (xmax + xmin) / 2;
  points[3] = ymax;
  points[4] = xmax;
  points[5] = ymin;

  iupdrvDrawPolygon(dc, points, 3, color, IUP_DRAW_STROKE, 2);
}

IUP_SDK_API void iupFlatDrawDrawCircle(IdrawCanvas* dc, int xc, int yc, int radius, int fill, int line_width, char* fgcolor, char* bgcolor, int active)
{
  int x1, y1, x2, y2;
  int style = (fill) ? IUP_DRAW_FILL : IUP_DRAW_STROKE;

  long color = iupDrawStrToColor(fgcolor, 0);
  if (!active)
    color = iFlatDrawColorMakeInactive(color, bgcolor);

  x1 = xc - radius;
  y1 = yc - radius;
  x2 = xc + radius;
  y2 = yc + radius;

  iupdrvDrawArc(dc, x1, y1, x2, y2, 0.0, 360, color, style, line_width);
}

static char* iFlatDrawGetImageName(Ihandle* ih, const char* baseattrib, const char* state)
{
  char attrib[1024];
  snprintf(attrib, sizeof(attrib), "%s%s", baseattrib, state);
  return iupAttribGetStr(ih, attrib);
}

IUP_SDK_API const char* iupFlatGetImageName(Ihandle* ih, const char* baseattrib, const char* basevalue, int press, int highlight, int active, int* make_inactive)
{
  const char* imagename = NULL;

  *make_inactive = 0;

  if (active)
  {
    if (press)
      imagename = iFlatDrawGetImageName(ih, baseattrib, "PRESS");
    else
    {
      if (highlight)
        imagename = iFlatDrawGetImageName(ih, baseattrib, "HIGHLIGHT");
    }
  }
  else
  {
    imagename = iFlatDrawGetImageName(ih, baseattrib, "INACTIVE");
    if (!imagename)
      *make_inactive = 1;
  }

  if (!imagename)
  {
    if (!basevalue)
      basevalue = iupAttribGetStr(ih, baseattrib);

    imagename = basevalue;
  }

  return imagename;
}

static char* iFlatDrawGetImageNameId(Ihandle* ih, const char* baseattrib, const char* state, int id)
{
  char attrib[1024];
  snprintf(attrib, sizeof(attrib), "%s%s", baseattrib, state);
  return iupAttribGetId(ih, attrib, id);
}

IUP_SDK_API const char* iupFlatGetImageNameId(Ihandle* ih, const char* baseattrib, int id, const char* basevalue, int press, int highlight, int active, int* make_inactive)
{
  const char* imagename = NULL;

  *make_inactive = 0;

  if (active)
  {
    if (press == id)
      imagename = iFlatDrawGetImageNameId(ih, baseattrib, "PRESS", id);
    else
    {
      if (highlight == id)
        imagename = iFlatDrawGetImageNameId(ih, baseattrib, "HIGHLIGHT", id);
    }
  }
  else
  {
    imagename = iFlatDrawGetImageNameId(ih, baseattrib, "INACTIVE", id);
    if (!imagename)
      *make_inactive = 1;
  }

  if (!imagename)
  {
    if (!basevalue)
      basevalue = iupAttribGetId(ih, baseattrib, id);

    imagename = basevalue;
  }

  return imagename;
}

IUP_SDK_API char* iupFlatGetDarkerBgColor(Ihandle* ih)
{
  char* value = iupAttribGet(ih, "DARK_DLGBGCOLOR");
  if (!value)
  {
    unsigned char r, g, b;
    iupStrToRGB(IupGetGlobal("DLGBGCOLOR"), &r, &g, &b);
    r = (r * 90) / 100;
    g = (g * 90) / 100;
    b = (b * 90) / 100;
    iupAttribSetStrf(ih, "DARK_DLGBGCOLOR", "%d %d %d", r, g, b);
    return iupAttribGet(ih, "DARK_DLGBGCOLOR");
  }
  else
    return value;
}

IUP_SDK_API int iupFlatSetActiveAttrib(Ihandle* ih, const char* value)
{
  iupBaseSetActiveAttrib(ih, value);
  iupdrvRedrawNow(ih);
  return 0;
}

static void iFlatItemSetTipVisible(Ihandle* ih, const char* tip)
{
  int visible = IupGetInt(ih, "TIPVISIBLE");

  /* do not call IupSetAttribute */
  iupAttribSetStr(ih, "TIP", tip);
  iupdrvBaseSetTipAttrib(ih, tip);

  if (visible)
  {
    IupSetAttribute(ih, "TIPVISIBLE", "No");
    if (tip)
      IupSetAttribute(ih, "TIPVISIBLE", "Yes");
  }
}

static int iFlatItemCheckTip(Ihandle* ih, const char* new_tip)
{
  char* tip = iupAttribGet(ih, "TIP");
  if (!tip && !new_tip)
    return 1;
  if (iupStrEqual(tip, new_tip))
    return 1;
  return 0;
}

IUP_SDK_API void iupFlatItemResetTip(Ihandle* ih)
{
  char* tip = iupAttribGet(ih, "_IUP_FLATITEM_TIP");
  if (!iFlatItemCheckTip(ih, tip))
    iFlatItemSetTipVisible(ih, tip);
}

IUP_SDK_API void iupFlatItemSetTip(Ihandle* ih, const char* tip)
{
  if (!iFlatItemCheckTip(ih, tip))
    iFlatItemSetTipVisible(ih, tip);
}

IUP_SDK_API int iupFlatItemSetTipAttrib(Ihandle* ih, const char* value)
{
  iupAttribSetStr(ih, "_IUP_FLATITEM_TIP", value);
  return iupdrvBaseSetTipAttrib(ih, value);
}
