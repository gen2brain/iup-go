/** \file
 * \brief IupDraw API
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUPDRAW_H
#define __IUPDRAW_H

#ifdef __cplusplus
extern "C" {
#endif

/* all functions can be used only in IUP canvas and inside the ACTION callback */

IUP_API void IupDrawBegin(Ihandle* ih);
IUP_API void IupDrawEnd(Ihandle* ih);

/* all functions can be called only between calls to Begin and End */

IUP_API void IupDrawSave(Ihandle* ih);
IUP_API void IupDrawRestore(Ihandle* ih);
IUP_API void IupDrawSaveLayer(Ihandle* ih, int alpha);
IUP_API void IupDrawTransform(Ihandle* ih, double a, double b, double c, double d, double e, double f);
IUP_API void IupDrawSetTransform(Ihandle* ih, double a, double b, double c, double d, double e, double f);
IUP_API void IupDrawResetTransform(Ihandle* ih);
IUP_API void IupDrawGetTransform(Ihandle* ih, double* a, double* b, double* c, double* d, double* e, double* f);
IUP_API void IupDrawTranslate(Ihandle* ih, double tx, double ty);
IUP_API void IupDrawScale(Ihandle* ih, double sx, double sy);
IUP_API void IupDrawRotate(Ihandle* ih, double angle);

IUP_API void IupDrawSetClipRect(Ihandle* ih, int x1, int y1, int x2, int y2);
IUP_API void IupDrawSetClipRoundedRect(Ihandle* ih, int x1, int y1, int x2, int y2, int corner_radius);
IUP_API void IupDrawGetClipRect(Ihandle* ih, int* x1, int* y1, int* x2, int* y2);
IUP_API void IupDrawResetClip(Ihandle* ih);

/* color controlled by the attribute DRAWCOLOR */
/* line style or fill controlled by the attribute DRAWSTYLE */

IUP_API void IupDrawParentBackground(Ihandle* ih);
IUP_API void IupDrawLine(Ihandle* ih, int x1, int y1, int x2, int y2);
IUP_API void IupDrawRectangle(Ihandle* ih, int x1, int y1, int x2, int y2);
IUP_API void IupDrawRoundedRectangle(Ihandle* ih, int x1, int y1, int x2, int y2, int corner_radius);
IUP_API void IupDrawArc(Ihandle* ih, int x1, int y1, int x2, int y2, double a1, double a2);
IUP_API void IupDrawEllipse(Ihandle* ih, int x1, int y1, int x2, int y2);
IUP_API void IupDrawPolygon(Ihandle* ih, int* points, int count);
IUP_API void IupDrawPixel(Ihandle* ih, int x, int y);
IUP_API void IupDrawBezier(Ihandle* ih, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4);
IUP_API void IupDrawQuadraticBezier(Ihandle* ih, int x1, int y1, int x2, int y2, int x3, int y3);

/* gradient colors are passed as parameters (not controlled by DRAWCOLOR) */
IUP_API void IupDrawLinearGradient(Ihandle* ih, int x1, int y1, int x2, int y2, float angle, const char* color1, const char* color2);
IUP_API void IupDrawRadialGradient(Ihandle* ih, int cx, int cy, int radius, const char* colorCenter, const char* colorEdge);
IUP_API void IupDrawLinearGradientStops(Ihandle* ih, int x1, int y1, int x2, int y2, float angle, const char** colors, const float* offsets, int count);
IUP_API void IupDrawRadialGradientStops(Ihandle* ih, int cx, int cy, int radius, const char** colors, const float* offsets, int count);

IUP_API void IupDrawPathBegin(Ihandle* ih);
IUP_API void IupDrawPathMoveTo(Ihandle* ih, int x, int y);
IUP_API void IupDrawPathLineTo(Ihandle* ih, int x, int y);
IUP_API void IupDrawPathCurveTo(Ihandle* ih, int x1, int y1, int x2, int y2, int x3, int y3);
IUP_API void IupDrawPathQuadTo(Ihandle* ih, int x1, int y1, int x2, int y2);
IUP_API void IupDrawPathArcTo(Ihandle* ih, int cx, int cy, int rx, int ry, double a1, double a2);
IUP_API void IupDrawPathClose(Ihandle* ih);

IUP_API void IupDrawPathMoveToF(Ihandle* ih, double x, double y);
IUP_API void IupDrawPathLineToF(Ihandle* ih, double x, double y);
IUP_API void IupDrawPathCurveToF(Ihandle* ih, double x1, double y1, double x2, double y2, double x3, double y3);
IUP_API void IupDrawPathQuadToF(Ihandle* ih, double x1, double y1, double x2, double y2);
IUP_API void IupDrawPathArcToF(Ihandle* ih, double cx, double cy, double rx, double ry, double a1, double a2);

IUP_API Ihandle* IupDrawPathCreate(void);
IUP_API void IupDrawPathClear(Ihandle* path);
IUP_API void IupDrawSetPath(Ihandle* ih, Ihandle* path);
IUP_API void IupDrawPathGetBounds(Ihandle* path, int* x1, int* y1, int* x2, int* y2);
IUP_API int IupDrawPathContains(Ihandle* path, int x, int y, int rule);
IUP_API int IupDrawPathSetSvg(Ihandle* path, const char* data);

#define IUP_DRAW_RULE_WINDING  0
#define IUP_DRAW_RULE_EVENODD 1

IUP_API void IupDrawPathFill(Ihandle* ih, int rule);
IUP_API void IupDrawPathStroke(Ihandle* ih);
IUP_API void IupDrawSetClipPath(Ihandle* ih, int rule);

IUP_API void IupDrawSetSourceSolid(Ihandle* ih, const char* color);
IUP_API void IupDrawSetSourceLinearGradient(Ihandle* ih, int x1, int y1, int x2, int y2, float angle, const char** colors, const float* offsets, int count);
IUP_API void IupDrawSetSourceRadialGradient(Ihandle* ih, int cx, int cy, int radius, const char** colors, const float* offsets, int count);
IUP_API void IupDrawResetSource(Ihandle* ih);

IUP_API void IupDrawText(Ihandle* ih, const char* text, int len, int x, int y, int w, int h);
IUP_API void IupDrawImage(Ihandle* ih, const char* name, int x, int y, int w, int h);
IUP_API void IupDrawSelectRect(Ihandle* ih, int x1, int y1, int x2, int y2);
IUP_API void IupDrawFocusRect(Ihandle* ih, int x1, int y1, int x2, int y2);

IUP_API void IupDrawGetSize(Ihandle* ih, int* w, int* h);
IUP_API void IupDrawGetTextSize(Ihandle* ih, const char* text, int len, int* w, int* h);
IUP_API void IupDrawGetTextMetrics(Ihandle* ih, int* ascent, int* descent, int* line_height);
IUP_API void IupDrawGetImageInfo(const char* name, int* w, int* h, int* bpp);

IUP_API Ihandle* IupDrawGetImage(Ihandle* ih);
IUP_API char* IupDrawGetSvg(Ihandle* ih);


#ifdef __cplusplus
}
#endif

#endif
