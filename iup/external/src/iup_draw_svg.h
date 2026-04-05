/** \file
 * \brief SVG Draw API.
 *
 * Provides an SVG drawing canvas that mirrors the IUP draw API.
 * Instead of rendering to a native widget, all drawing operations
 * build an SVG string in memory that can be retrieved as text.
 *
 * See Copyright Notice in "iup.h"
 *
 */

#ifndef __IUP_DRAW_SVG_H
#define __IUP_DRAW_SVG_H

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _iSvgCanvas iSvgCanvas;

iSvgCanvas* iupSvgDrawCreateCanvas(int w, int h);
void        iupSvgDrawKillCanvas(iSvgCanvas* dc);

const char* iupSvgDrawGetString(iSvgCanvas* dc);
int         iupSvgDrawGetStringLength(iSvgCanvas* dc);
void        iupSvgDrawGetSize(iSvgCanvas* dc, int* w, int* h);

/* style: IUP_DRAW_FILL, IUP_DRAW_STROKE, IUP_DRAW_STROKE_DASH, etc. */
/* color is passed as "r g b" or "r g b a" string */

void iupSvgDrawLine(iSvgCanvas* dc, int x1, int y1, int x2, int y2, const char* color, int style, int line_width);
void iupSvgDrawRectangle(iSvgCanvas* dc, int x1, int y1, int x2, int y2, const char* color, int style, int line_width);
void iupSvgDrawRoundedRectangle(iSvgCanvas* dc, int x1, int y1, int x2, int y2, int corner_radius, const char* color, int style, int line_width);
void iupSvgDrawArc(iSvgCanvas* dc, int x1, int y1, int x2, int y2, double a1, double a2, const char* color, int style, int line_width);
void iupSvgDrawEllipse(iSvgCanvas* dc, int x1, int y1, int x2, int y2, const char* color, int style, int line_width);
void iupSvgDrawPolygon(iSvgCanvas* dc, int* points, int count, const char* color, int style, int line_width);
void iupSvgDrawPixel(iSvgCanvas* dc, int x, int y, const char* color);
void iupSvgDrawBezier(iSvgCanvas* dc, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, const char* color, int style, int line_width);
void iupSvgDrawQuadraticBezier(iSvgCanvas* dc, int x1, int y1, int x2, int y2, int x3, int y3, const char* color, int style, int line_width);
void iupSvgDrawText(iSvgCanvas* dc, const char* text, int len, int x, int y, int w, int h, const char* color, const char* font, int flags, double text_orientation);

void iupSvgDrawLinearGradient(iSvgCanvas* dc, int x1, int y1, int x2, int y2, float angle, const char* color1, const char* color2);
void iupSvgDrawRadialGradient(iSvgCanvas* dc, int cx, int cy, int radius, const char* color_center, const char* color_edge);

void iupSvgDrawSetClipRect(iSvgCanvas* dc, int x1, int y1, int x2, int y2);
void iupSvgDrawSetClipRoundedRect(iSvgCanvas* dc, int x1, int y1, int x2, int y2, int corner_radius);
void iupSvgDrawResetClip(iSvgCanvas* dc);
void iupSvgDrawGetClipRect(iSvgCanvas* dc, int* x1, int* y1, int* x2, int* y2);

void iupSvgDrawImageRGBA(iSvgCanvas* dc, const unsigned char* rgba, int img_w, int img_h, int x, int y, int w, int h);

void iupSvgDrawSelectRect(iSvgCanvas* dc, int x1, int y1, int x2, int y2);
void iupSvgDrawFocusRect(iSvgCanvas* dc, int x1, int y1, int x2, int y2);

#ifdef __cplusplus
}
#endif

#endif
