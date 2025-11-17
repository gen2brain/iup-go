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

enum{ IUP_DRAW_FILL, IUP_DRAW_STROKE, IUP_DRAW_STROKE_DASH, IUP_DRAW_STROKE_DOT, IUP_DRAW_STROKE_DASH_DOT, IUP_DRAW_STROKE_DASH_DOT_DOT };

/** Creates a draw canvas based on an IupCanvas.
 * This will create an image for offscreen drawing.
 * \ingroup drvdraw */
IUP_SDK_API IdrawCanvas* iupdrvDrawCreateCanvas(Ihandle* ih);

/** Destroys the IdrawCanvas.
 * \ingroup drvdraw */
IUP_SDK_API void iupdrvDrawKillCanvas(IdrawCanvas* dc);

/** Draws the ofscreen image on the screen.
 * \ingroup drvdraw */
IUP_SDK_API void iupdrvDrawFlush(IdrawCanvas* dc);

/** Rebuild the offscreen image if the canvas size has changed.
 * Automatically done in iupdrvDrawCreateCanvas.
 * \ingroup drvdraw */
IUP_SDK_API void iupdrvDrawUpdateSize(IdrawCanvas* dc);

/** Returns the canvas size available for drawing.
 * \ingroup drvdraw */
IUP_SDK_API void iupdrvDrawGetSize(IdrawCanvas* dc, int *w, int *h);

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

/** Draws an image.
 * x,y is at left,top corner of the image.
 * \ingroup drvdraw */
IUP_SDK_API void iupdrvDrawImage(IdrawCanvas* dc, const char* name, int make_inactive, const char* bgcolor, int x, int y, int w, int h);

/** Sets a rectangle clipping area.
 * \ingroup drvdraw */
IUP_SDK_API void iupdrvDrawSetClipRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2);

/** Removes clipping.
 * \ingroup drvdraw */
IUP_SDK_API void iupdrvDrawResetClip(IdrawCanvas* dc);

/** Returns the last rectangle set in iupdrvDrawSetClipRect.
* \ingroup drvdraw */
IUP_SDK_API void iupdrvDrawGetClipRect(IdrawCanvas* dc, int *x1, int *y1, int *x2, int *y2);

/** Draws a selection rectangle.
 * \ingroup drvdraw */
IUP_SDK_API void iupdrvDrawSelectRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2);

/** Draws a focus rectangle.
 * \ingroup drvdraw */
IUP_SDK_API void iupdrvDrawFocusRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2);


#ifdef __cplusplus
}
#endif

#endif

