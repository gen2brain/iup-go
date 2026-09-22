/** \file
 * \brief Driver Draw API.
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUP_DRVDRAW_H
#define __IUP_DRVDRAW_H

#ifdef __cplusplus
extern "C"
{
#endif

/** \defgroup drvdraw Driver Draw API
 * \par
 * See \ref iup_drvdraw.h
 * \ingroup util */



struct _IdrawCanvas;
typedef struct _IdrawCanvas IdrawCanvas;

typedef struct _IupDrawMatrix
{
  double a, b, c, d, e, f;
} IupDrawMatrix;

enum{ IUP_DRAW_FILL, IUP_DRAW_STROKE, IUP_DRAW_STROKE_DASH, IUP_DRAW_STROKE_DOT, IUP_DRAW_STROKE_DASH_DOT, IUP_DRAW_STROKE_DASH_DOT_DOT };

enum{ IUP_DRAW_IMAGE_NEAREST, IUP_DRAW_IMAGE_LINEAR };

#define IUP_DRAW_NO_TINT -1L  /* outside the iupDrawColor range */

/** Creates a draw canvas based on an IupCanvas.
 * This will create an image for offscreen drawing.
 * \ingroup drvdraw */
IUP_SDK_API IdrawCanvas* iupdrvDrawCreateCanvas(Ihandle* ih);

/** Destroys the IdrawCanvas.
 * \ingroup drvdraw */
IUP_SDK_API void iupdrvDrawKillCanvas(IdrawCanvas* dc);

/** Draws the offscreen image on the screen.
 * \ingroup drvdraw */
IUP_SDK_API void iupdrvDrawFlush(IdrawCanvas* dc);

/** Rebuild the offscreen image if the canvas size has changed.
 * Automatically done in iupdrvDrawCreateCanvas.
 * \ingroup drvdraw */
IUP_SDK_API void iupdrvDrawUpdateSize(IdrawCanvas* dc);

/** Returns the canvas size available for drawing.
 * \ingroup drvdraw */
IUP_SDK_API void iupdrvDrawGetSize(IdrawCanvas* dc, int* w, int* h);

IUP_SDK_API void iupdrvDrawSetTransform(IdrawCanvas* dc, const IupDrawMatrix* matrix);

/** Draws a line.
 * \ingroup drvdraw */
IUP_SDK_API void iupdrvDrawLine(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width);

/** Draws a filled/hollow rectangle.
 * \ingroup drvdraw */
IUP_SDK_API void iupdrvDrawRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width);

/** Draws a filled/hollow arc.
 * \ingroup drvdraw */
IUP_SDK_API void iupdrvDrawArc(IdrawCanvas* dc, int x1, int y1, int x2, int y2, double a1, double a2, long color, int style, int line_width);

/** Draws a filled/hollow ellipse inscribed in rectangle (x1,y1)-(x2,y2).
 * \ingroup drvdraw */
IUP_SDK_API void iupdrvDrawEllipse(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width);

/** Draws a filled/hollow polygon.
 * points are arranged xyxyxy...
 * \ingroup drvdraw */
IUP_SDK_API void iupdrvDrawPolygon(IdrawCanvas* dc, int* points, int count, long color, int style, int line_width);

/** Draws a single pixel.
 * \ingroup drvdraw */
IUP_SDK_API void iupdrvDrawPixel(IdrawCanvas* dc, int x, int y, long color);

/** Draws a filled/hollow rounded rectangle.
 * \ingroup drvdraw */
IUP_SDK_API void iupdrvDrawRoundedRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int corner_radius, long color, int style, int line_width);

/** Draws a cubic Bezier curve.
 * (x1,y1) = start point, (x2,y2) = first control point, (x3,y3) = second control point, (x4,y4) = end point
 * \ingroup drvdraw */
IUP_SDK_API void iupdrvDrawBezier(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, long color, int style, int line_width);

/** Draws a quadratic Bezier curve.
 * (x1,y1) = start point, (x2,y2) = control point, (x3,y3) = end point
 * \ingroup drvdraw */
IUP_SDK_API void iupdrvDrawQuadraticBezier(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int x3, int y3, long color, int style, int line_width);

#define IUP_GRADIENT_MAX_STOPS 64

/** Draws a linear gradient across count color stops (offsets in 0-1, ascending).
 * angle: 0=horizontal right, 90=vertical down, 180=horizontal left, 270=vertical up
 * \ingroup drvdraw */
IUP_SDK_API void iupdrvDrawLinearGradient(IdrawCanvas* dc, int x1, int y1, int x2, int y2, float angle, const long* colors, const float* offsets, int count);

/** Draws a radial gradient from center to edge across count color stops.
 * \ingroup drvdraw */
IUP_SDK_API void iupdrvDrawRadialGradient(IdrawCanvas* dc, int cx, int cy, int radius, const long* colors, const float* offsets, int count);

enum { IUP_PATHSEG_MOVE_TO, IUP_PATHSEG_LINE_TO, IUP_PATHSEG_CURVE_TO, IUP_PATHSEG_QUAD_TO, IUP_PATHSEG_ARC_TO, IUP_PATHSEG_CLOSE };

/* ARC_TO: (x1,y1) = center, (x2,y2) = radius x, radius y, a1/a2 in degrees counterclockwise */
typedef struct _IupPathSeg
{
  unsigned char op;
  int x1, y1, x2, y2, x3, y3;
  double a1, a2;
} IupPathSeg;

enum { IUP_SOURCE_SOLID, IUP_SOURCE_LINEAR_GRADIENT, IUP_SOURCE_RADIAL_GRADIENT };

/* fill/stroke source; solid is a single color, gradients share the iupdrvDrawLinear/RadialGradient semantics */
typedef struct _IupDrawSource
{
  int type;
  long color;
  int x1, y1, x2, y2;
  float angle;
  int cx, cy, radius;
  long colors[IUP_GRADIENT_MAX_STOPS];
  float offsets[IUP_GRADIENT_MAX_STOPS];
  int count;
} IupDrawSource;

enum { IUP_PATH_RULE_WINDING, IUP_PATH_RULE_EVENODD };

/** Fills the path described by segs with the source.
 * The path is kept by the caller, drivers must not store it.
 * \ingroup drvdraw */
IUP_SDK_API void iupdrvDrawPathFill(IdrawCanvas* dc, const IupPathSeg* segs, int count, const IupDrawSource* src, int rule);

/** Strokes the path described by segs with the source.
 * \ingroup drvdraw */
IUP_SDK_API void iupdrvDrawPathStroke(IdrawCanvas* dc, const IupPathSeg* segs, int count, const IupDrawSource* src, int style, int line_width);

/** Sets the path as the clipping area, replacing the current clip.
 * \ingroup drvdraw */
IUP_SDK_API void iupdrvDrawSetClipPath(IdrawCanvas* dc, const IupPathSeg* segs, int count, int rule);

#define IUP_DRAW_LEFT     0x0000
#define IUP_DRAW_CENTER   0x0001
#define IUP_DRAW_RIGHT    0x0002
#define IUP_DRAW_WRAP     0x0004
#define IUP_DRAW_ELLIPSIS 0x0008
#define IUP_DRAW_CLIP     0x0010
#define IUP_DRAW_LAYOUTCENTER 0x0020

/** Draws a text.
 * x,y is at left,top corner of the text.
 * \ingroup drvdraw */
IUP_SDK_API void iupdrvDrawText(IdrawCanvas* dc, const char* text, int len, int x, int y, int w, int h, long color, const char* font, int flags, double text_orientation);

/** Draws an image. sw/sh -1 = whole image, w/h -1 = source size, tint IUP_DRAW_NO_TINT = none, opacity 0-255.
 * \ingroup drvdraw */
IUP_SDK_API void iupdrvDrawImage(IdrawCanvas* dc, const char* name, int make_inactive, const char* bgcolor, long tint, int opacity, int x, int y, int w, int h, int sx, int sy, int sw, int sh, int quality);

/** Sets a rectangle clipping area.
 * \ingroup drvdraw */
IUP_SDK_API void iupdrvDrawSetClipRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2);

/** Sets a rounded rectangle clipping area.
 * \ingroup drvdraw */
IUP_SDK_API void iupdrvDrawSetClipRoundedRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int corner_radius);

/** Removes clipping.
 * \ingroup drvdraw */
IUP_SDK_API void iupdrvDrawResetClip(IdrawCanvas* dc);

/** Returns the last rectangle set in iupdrvDrawSetClipRect.
* \ingroup drvdraw */
IUP_SDK_API void iupdrvDrawGetClipRect(IdrawCanvas* dc, int* x1, int* y1, int* x2, int* y2);

/** Draws a selection rectangle.
 * \ingroup drvdraw */
IUP_SDK_API void iupdrvDrawSelectRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2);

/** Draws a focus rectangle.
 * \ingroup drvdraw */
IUP_SDK_API void iupdrvDrawFocusRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2);

/** Extracts the offscreen buffer contents as RGBA pixel data (top-down, non-premultiplied).
 * Caller provides pre-allocated buffer of size w*h*4 bytes.
 * Returns 1 on success, 0 on failure.
 * \ingroup drvdraw */
IUP_SDK_API int iupdrvDrawGetImageData(IdrawCanvas* dc, unsigned char* data);

/** Extracts the canvas persistent buffer contents as RGBA pixel data (top-down, non-premultiplied).
 * Can be called outside DrawBegin/DrawEnd to read the last rendered frame.
 * Caller provides pre-allocated buffer of size w*h*4 bytes.
 * Returns 1 on success, 0 on failure.
 * \ingroup drvdraw */
IUP_SDK_API int iupdrvCanvasGetImageData(Ihandle* ih, unsigned char* data, int w, int h);


#ifdef __cplusplus
}
#endif

#endif
