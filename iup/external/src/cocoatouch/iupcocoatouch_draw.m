/** \file
 * \brief iOS UIKit Draw Functions
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>

#import <UIKit/UIKit.h>

#include "iup.h"
#include "iup_attrib.h"
#include "iup_object.h"
#include "iup_image.h"
#include "iup_drvdraw.h"
#include "iup_draw.h"

#include "iupcocoatouch_drv.h"
#include "iupcocoatouch_draw.h"


IUP_DRV_API unsigned char* iupCocoaTouchCanvasEnsurePixels(Ihandle* ih, UIView* view, size_t* out_pixel_w, size_t* out_pixel_h, CGFloat* out_scale)
{
	if (!ih || !view) return NULL;

	CGSize view_size = [view bounds].size;
	CGFloat scale = [view contentScaleFactor];
	if (scale <= 0) scale = 1.0;

	size_t pixel_w = (size_t)(view_size.width  * scale);
	size_t pixel_h = (size_t)(view_size.height * scale);
	if (pixel_w == 0 || pixel_h == 0) return NULL;

	unsigned char* pixels = (unsigned char*)iupAttribGet(ih, "_IUPCOCOATOUCH_CANVAS_PIXELS");
	size_t cur_w = (size_t)iupAttribGetInt(ih, "_IUPCOCOATOUCH_CANVAS_PIXELS_W");
	size_t cur_h = (size_t)iupAttribGetInt(ih, "_IUPCOCOATOUCH_CANVAS_PIXELS_H");

	if (pixels && (cur_w != pixel_w || cur_h != pixel_h))
	{
		free(pixels);
		pixels = NULL;
		iupAttribSet(ih, "_IUPCOCOATOUCH_CANVAS_PIXELS", NULL);
	}

	if (!pixels)
	{
		pixels = (unsigned char*)calloc(pixel_w * pixel_h, 4);
		if (!pixels) return NULL;
		iupAttribSet(ih, "_IUPCOCOATOUCH_CANVAS_PIXELS", (char*)pixels);
		iupAttribSetInt(ih, "_IUPCOCOATOUCH_CANVAS_PIXELS_W", (int)pixel_w);
		iupAttribSetInt(ih, "_IUPCOCOATOUCH_CANVAS_PIXELS_H", (int)pixel_h);
	}

	if (out_pixel_w) *out_pixel_w = pixel_w;
	if (out_pixel_h) *out_pixel_h = pixel_h;
	if (out_scale)   *out_scale   = scale;
	return pixels;
}

IUP_DRV_API unsigned char* iupCocoaTouchCanvasGetPixels(Ihandle* ih, size_t* out_pixel_w, size_t* out_pixel_h, CGFloat* out_scale)
{
	if (!ih) return NULL;
	unsigned char* pixels = (unsigned char*)iupAttribGet(ih, "_IUPCOCOATOUCH_CANVAS_PIXELS");
	if (!pixels) return NULL;

	if (out_pixel_w) *out_pixel_w = (size_t)iupAttribGetInt(ih, "_IUPCOCOATOUCH_CANVAS_PIXELS_W");
	if (out_pixel_h) *out_pixel_h = (size_t)iupAttribGetInt(ih, "_IUPCOCOATOUCH_CANVAS_PIXELS_H");
	if (out_scale)
	{
		UIView* view = (UIView*)ih->handle;
		CGFloat scale = (view && [view isKindOfClass:[UIView class]]) ? [view contentScaleFactor] : 1.0;
		if (scale <= 0) scale = 1.0;
		*out_scale = scale;
	}
	return pixels;
}

IUP_DRV_API void iupCocoaTouchCanvasReleaseBuffer(Ihandle* ih)
{
	if (!ih) return;
	unsigned char* pixels = (unsigned char*)iupAttribGet(ih, "_IUPCOCOATOUCH_CANVAS_PIXELS");
	if (pixels) free(pixels);
	iupAttribSet(ih, "_IUPCOCOATOUCH_CANVAS_PIXELS", NULL);
	iupAttribSet(ih, "_IUPCOCOATOUCH_CANVAS_PIXELS_W", NULL);
	iupAttribSet(ih, "_IUPCOCOATOUCH_CANVAS_PIXELS_H", NULL);
}

static void cocoaTouchDrawSetStrokeColor(CGContextRef ctx, long color)
{
	CGContextSetRGBStrokeColor(ctx,
		iupDrawRed(color)   / 255.0,
		iupDrawGreen(color) / 255.0,
		iupDrawBlue(color)  / 255.0,
		iupDrawAlpha(color) / 255.0);
}

static void cocoaTouchDrawSetFillColor(CGContextRef ctx, long color)
{
	CGContextSetRGBFillColor(ctx,
		iupDrawRed(color)   / 255.0,
		iupDrawGreen(color) / 255.0,
		iupDrawBlue(color)  / 255.0,
		iupDrawAlpha(color) / 255.0);
}

static void cocoaTouchDrawSetLineStyle(CGContextRef ctx, int style)
{
	switch (style)
	{
		case IUP_DRAW_STROKE_DASH:
		{
			CGFloat dashes[2] = { 9.0, 3.0 };
			CGContextSetLineDash(ctx, 0, dashes, 2);
			break;
		}
		case IUP_DRAW_STROKE_DOT:
		{
			CGFloat dashes[2] = { 1.0, 2.0 };
			CGContextSetLineDash(ctx, 0, dashes, 2);
			break;
		}
		case IUP_DRAW_STROKE_DASH_DOT:
		{
			CGFloat dashes[4] = { 7.0, 3.0, 1.0, 3.0 };
			CGContextSetLineDash(ctx, 0, dashes, 4);
			break;
		}
		case IUP_DRAW_STROKE_DASH_DOT_DOT:
		{
			CGFloat dashes[6] = { 7.0, 3.0, 1.0, 3.0, 1.0, 3.0 };
			CGContextSetLineDash(ctx, 0, dashes, 6);
			break;
		}
		default:
			CGContextSetLineDash(ctx, 0, NULL, 0);
			break;
	}
}

IUP_SDK_API IdrawCanvas* iupdrvDrawCreateCanvas(Ihandle* ih)
{
	if (!ih || !ih->handle) return NULL;
	UIView* view = (UIView*)ih->handle;

	size_t pixel_w = 0, pixel_h = 0;
	CGFloat scale = 1.0;
	unsigned char* pixels = iupCocoaTouchCanvasEnsurePixels(ih, view, &pixel_w, &pixel_h, &scale);
	if (!pixels) return NULL;

	CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
	CGBitmapInfo info = kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big;
	CGContextRef ctx = CGBitmapContextCreate(pixels, pixel_w, pixel_h, 8, pixel_w * 4, cs, info);
	CGColorSpaceRelease(cs);
	if (!ctx) return NULL;

	CGContextTranslateCTM(ctx, 0.0, (CGFloat)pixel_h);
	CGContextScaleCTM(ctx, scale, -scale);
	CGContextSetLineCap(ctx, kCGLineCapButt);
	CGContextSetLineJoin(ctx, kCGLineJoinMiter);

	IdrawCanvas* dc = (IdrawCanvas*)calloc(1, sizeof(IdrawCanvas));
	if (!dc)
	{
		CGContextRelease(ctx);
		return NULL;
	}

	dc->ih = ih;
	dc->canvasView = view;
	dc->cgContext = ctx;
	dc->scale = scale;

	CGSize size = [view bounds].size;
	dc->w = (int)size.width;
	dc->h = (int)size.height;

	iupAttribSet(ih, "DRAWDRIVER", "COCOATOUCH");
	return dc;
}

IUP_SDK_API void iupdrvDrawKillCanvas(IdrawCanvas* dc)
{
	if (!dc) return;
	if (dc->clip_state)
	{
		CGContextRestoreGState(dc->cgContext);
		dc->clip_state = 0;
	}
	if (dc->cgContext) CGContextRelease(dc->cgContext);
	free(dc);
}

IUP_SDK_API void iupdrvDrawUpdateSize(IdrawCanvas* dc)
{
	if (!dc || !dc->canvasView) return;
	CGSize size = [(UIView*)dc->canvasView bounds].size;
	dc->w = (int)size.width;
	dc->h = (int)size.height;
}

IUP_SDK_API void iupdrvDrawFlush(IdrawCanvas* dc)
{
	if (!dc) return;

	if (dc->draw_focus)
	{
		CGContextSaveGState(dc->cgContext);
		CGFloat dashes[2] = { 1.0, 1.0 };
		CGContextSetLineDash(dc->cgContext, 0, dashes, 2);
		CGContextSetLineWidth(dc->cgContext, 1.0);
		CGContextSetRGBStrokeColor(dc->cgContext, 0, 0, 0, 1);
		int x1 = dc->focus_x1, y1 = dc->focus_y1;
		int x2 = dc->focus_x2, y2 = dc->focus_y2;
		CGRect r = CGRectMake(x1 + 0.5, y1 + 0.5, x2 - x1, y2 - y1);
		CGContextStrokeRect(dc->cgContext, r);
		CGContextRestoreGState(dc->cgContext);
		dc->draw_focus = 0;
	}

	CGContextFlush(dc->cgContext);
	iupAttribSet(dc->ih, "_IUPCOCOATOUCH_BUFFER_PENDING", "1");
	if (dc->canvasView)
	{
		[(UIView*)dc->canvasView setNeedsDisplay];
	}
}

IUP_SDK_API void iupdrvDrawGetSize(IdrawCanvas* dc, int* w, int* h)
{
	if (!dc) { if (w) *w = 0; if (h) *h = 0; return; }
	if (w) *w = dc->w;
	if (h) *h = dc->h;
}

IUP_SDK_API void iupdrvDrawSetClipRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
	if (!dc) return;
	if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0)
	{
		iupdrvDrawResetClip(dc);
		return;
	}

	iupDrawCheckSwapCoord(x1, x2);
	iupDrawCheckSwapCoord(y1, y2);

	iupdrvDrawResetClip(dc);
	CGContextSaveGState(dc->cgContext);
	dc->clip_state = 1;
	CGContextClipToRect(dc->cgContext, CGRectMake(x1, y1, x2 - x1 + 1, y2 - y1 + 1));

	dc->clip_x1 = x1; dc->clip_y1 = y1;
	dc->clip_x2 = x2; dc->clip_y2 = y2;
}

IUP_SDK_API void iupdrvDrawSetClipRoundedRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int corner_radius)
{
	if (!dc) return;
	if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0)
	{
		iupdrvDrawResetClip(dc);
		return;
	}

	iupDrawCheckSwapCoord(x1, x2);
	iupDrawCheckSwapCoord(y1, y2);

	CGFloat w = x2 - x1 + 1;
	CGFloat h = y2 - y1 + 1;
	CGFloat radius = (CGFloat)corner_radius;
	CGFloat max_radius = (w < h ? w : h) / 2.0;
	if (radius > max_radius) radius = max_radius;

	iupdrvDrawResetClip(dc);
	CGContextSaveGState(dc->cgContext);
	dc->clip_state = 1;

	CGRect rect = CGRectMake(x1, y1, w, h);
	CGPathRef path = CGPathCreateWithRoundedRect(rect, radius, radius, NULL);
	CGContextBeginPath(dc->cgContext);
	CGContextAddPath(dc->cgContext, path);
	CGContextClip(dc->cgContext);
	CGPathRelease(path);

	dc->clip_x1 = x1; dc->clip_y1 = y1;
	dc->clip_x2 = x2; dc->clip_y2 = y2;
}

IUP_SDK_API void iupdrvDrawResetClip(IdrawCanvas* dc)
{
	if (!dc) return;
	if (dc->clip_state)
	{
		CGContextRestoreGState(dc->cgContext);
		dc->clip_state = 0;
	}
	dc->clip_x1 = dc->clip_y1 = dc->clip_x2 = dc->clip_y2 = 0;
}

IUP_SDK_API void iupdrvDrawGetClipRect(IdrawCanvas* dc, int* x1, int* y1, int* x2, int* y2)
{
	if (!dc) return;
	if (x1) *x1 = dc->clip_x1;
	if (y1) *y1 = dc->clip_y1;
	if (x2) *x2 = dc->clip_x2;
	if (y2) *y2 = dc->clip_y2;
}

IUP_SDK_API void iupdrvDrawRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
	if (!dc) return;
	iupDrawCheckSwapCoord(x1, x2);
	iupDrawCheckSwapCoord(y1, y2);
	CGRect r = CGRectMake(x1, y1, x2 - x1 + 1, y2 - y1 + 1);

	if (style == IUP_DRAW_FILL)
	{
		cocoaTouchDrawSetFillColor(dc->cgContext, color);
		CGContextFillRect(dc->cgContext, r);
	}
	else
	{
		cocoaTouchDrawSetStrokeColor(dc->cgContext, color);
		CGContextSetLineWidth(dc->cgContext, (CGFloat)line_width);
		cocoaTouchDrawSetLineStyle(dc->cgContext, style);
		if (line_width == 1)
		{
			CGContextStrokeRect(dc->cgContext, CGRectInset(r, 0.5, 0.5));
		}
		else
		{
			CGContextStrokeRect(dc->cgContext, r);
		}
	}
}

IUP_SDK_API void iupdrvDrawLine(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
	if (!dc) return;
	cocoaTouchDrawSetStrokeColor(dc->cgContext, color);
	CGContextSetLineWidth(dc->cgContext, (CGFloat)line_width);
	cocoaTouchDrawSetLineStyle(dc->cgContext, style);

	CGContextBeginPath(dc->cgContext);
	if (line_width == 1)
	{
		if (x1 == x2)
		{
			iupDrawCheckSwapCoord(y1, y2);
			CGContextMoveToPoint(dc->cgContext, x1 + 0.5, y1);
			CGContextAddLineToPoint(dc->cgContext, x1 + 0.5, y2 + 1);
		}
		else if (y1 == y2)
		{
			iupDrawCheckSwapCoord(x1, x2);
			CGContextMoveToPoint(dc->cgContext, x1, y1 + 0.5);
			CGContextAddLineToPoint(dc->cgContext, x2 + 1, y1 + 0.5);
		}
		else
		{
			CGContextMoveToPoint(dc->cgContext, x1, y1);
			CGContextAddLineToPoint(dc->cgContext, x2, y2);
		}
	}
	else
	{
		CGContextMoveToPoint(dc->cgContext, x1, y1);
		CGContextAddLineToPoint(dc->cgContext, x2, y2);
	}
	CGContextStrokePath(dc->cgContext);
}

IUP_SDK_API void iupdrvDrawArc(IdrawCanvas* dc, int x1, int y1, int x2, int y2, double a1, double a2, long color, int style, int line_width)
{
	if (!dc) return;
	iupDrawCheckSwapCoord(x1, x2);
	iupDrawCheckSwapCoord(y1, y2);
	CGFloat w = x2 - x1 + 1;
	CGFloat h = y2 - y1 + 1;
	if (w <= 0 || h <= 0) return;

	CGFloat xc = x1 + w/2.0;
	CGFloat yc = y1 + h/2.0;
	if (a2 < a1) a2 += 360.0;
	CGFloat rad1 = -a1 * M_PI / 180.0;
	CGFloat rad2 = -a2 * M_PI / 180.0;

	if (style == IUP_DRAW_FILL)
	{
		cocoaTouchDrawSetFillColor(dc->cgContext, color);
		CGContextSaveGState(dc->cgContext);
		CGContextTranslateCTM(dc->cgContext, xc, yc);
		CGContextScaleCTM(dc->cgContext, w/2.0, h/2.0);
		CGContextBeginPath(dc->cgContext);
		CGContextMoveToPoint(dc->cgContext, 0, 0);
		CGContextAddArc(dc->cgContext, 0, 0, 1.0, rad1, rad2, 1);
		CGContextClosePath(dc->cgContext);
		CGContextFillPath(dc->cgContext);
		CGContextRestoreGState(dc->cgContext);
	}
	else
	{
		cocoaTouchDrawSetStrokeColor(dc->cgContext, color);
		CGContextSetLineWidth(dc->cgContext, (CGFloat)line_width);
		cocoaTouchDrawSetLineStyle(dc->cgContext, style);

		if (w == h)
		{
			CGContextBeginPath(dc->cgContext);
			CGContextAddArc(dc->cgContext, xc, yc, 0.5*w, rad1, rad2, 1);
			CGContextStrokePath(dc->cgContext);
		}
		else
		{
			CGMutablePathRef path = CGPathCreateMutable();
			CGAffineTransform t = CGAffineTransformMakeTranslation(xc, yc);
			t = CGAffineTransformScale(t, w/2.0, h/2.0);
			CGPathAddArc(path, &t, 0, 0, 1.0, rad1, rad2, 1);
			CGContextBeginPath(dc->cgContext);
			CGContextAddPath(dc->cgContext, path);
			CGPathRelease(path);
			CGContextStrokePath(dc->cgContext);
		}
	}
}

IUP_SDK_API void iupdrvDrawEllipse(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
	if (!dc) return;
	iupDrawCheckSwapCoord(x1, x2);
	iupDrawCheckSwapCoord(y1, y2);
	CGFloat w = x2 - x1 + 1;
	CGFloat h = y2 - y1 + 1;
	if (w <= 0 || h <= 0) return;

	CGRect rect = CGRectMake(x1, y1, w, h);
	CGContextBeginPath(dc->cgContext);
	CGContextAddEllipseInRect(dc->cgContext, rect);

	if (style == IUP_DRAW_FILL)
	{
		cocoaTouchDrawSetFillColor(dc->cgContext, color);
		CGContextFillPath(dc->cgContext);
	}
	else
	{
		cocoaTouchDrawSetStrokeColor(dc->cgContext, color);
		CGContextSetLineWidth(dc->cgContext, (CGFloat)line_width);
		cocoaTouchDrawSetLineStyle(dc->cgContext, style);
		CGContextStrokePath(dc->cgContext);
	}
}

IUP_SDK_API void iupdrvDrawPolygon(IdrawCanvas* dc, int* points, int count, long color, int style, int line_width)
{
	if (!dc || count <= 0) return;

	if (style == IUP_DRAW_FILL)
	{
		cocoaTouchDrawSetFillColor(dc->cgContext, color);
	}
	else
	{
		cocoaTouchDrawSetStrokeColor(dc->cgContext, color);
		CGContextSetLineWidth(dc->cgContext, (CGFloat)line_width);
		cocoaTouchDrawSetLineStyle(dc->cgContext, style);
	}

	CGContextBeginPath(dc->cgContext);
	CGContextMoveToPoint(dc->cgContext, (CGFloat)points[0], (CGFloat)points[1]);
	for (int i = 1; i < count; i++)
	{
		CGContextAddLineToPoint(dc->cgContext, (CGFloat)points[2*i], (CGFloat)points[2*i + 1]);
	}
	CGContextClosePath(dc->cgContext);

	if (style == IUP_DRAW_FILL)
		CGContextFillPath(dc->cgContext);
	else
		CGContextStrokePath(dc->cgContext);
}

IUP_SDK_API void iupdrvDrawPixel(IdrawCanvas* dc, int x, int y, long color)
{
	if (!dc) return;
	cocoaTouchDrawSetFillColor(dc->cgContext, color);
	CGContextFillRect(dc->cgContext, CGRectMake(x, y, 1, 1));
}

IUP_SDK_API void iupdrvDrawRoundedRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int corner_radius, long color, int style, int line_width)
{
	if (!dc) return;
	iupDrawCheckSwapCoord(x1, x2);
	iupDrawCheckSwapCoord(y1, y2);
	CGFloat w = x2 - x1 + 1;
	CGFloat h = y2 - y1 + 1;
	if (w <= 0 || h <= 0) return;

	CGFloat radius = (CGFloat)corner_radius;
	CGFloat max_radius = (w < h ? w : h) / 2.0;
	if (radius > max_radius) radius = max_radius;

	if (style == IUP_DRAW_FILL)
	{
		cocoaTouchDrawSetFillColor(dc->cgContext, color);
	}
	else
	{
		cocoaTouchDrawSetStrokeColor(dc->cgContext, color);
		CGContextSetLineWidth(dc->cgContext, (CGFloat)line_width);
		cocoaTouchDrawSetLineStyle(dc->cgContext, style);
	}

	CGRect rect = CGRectMake(x1, y1, w, h);
	CGPathRef path = CGPathCreateWithRoundedRect(rect, radius, radius, NULL);
	CGContextBeginPath(dc->cgContext);
	CGContextAddPath(dc->cgContext, path);
	CGPathRelease(path);

	if (style == IUP_DRAW_FILL)
		CGContextFillPath(dc->cgContext);
	else
		CGContextStrokePath(dc->cgContext);
}

IUP_SDK_API void iupdrvDrawBezier(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, long color, int style, int line_width)
{
	if (!dc) return;

	if (style == IUP_DRAW_FILL)
	{
		cocoaTouchDrawSetFillColor(dc->cgContext, color);
	}
	else
	{
		cocoaTouchDrawSetStrokeColor(dc->cgContext, color);
		CGContextSetLineWidth(dc->cgContext, (CGFloat)line_width);
		cocoaTouchDrawSetLineStyle(dc->cgContext, style);
	}

	CGContextBeginPath(dc->cgContext);
	CGContextMoveToPoint(dc->cgContext, x1, y1);
	CGContextAddCurveToPoint(dc->cgContext, x2, y2, x3, y3, x4, y4);

	if (style == IUP_DRAW_FILL)
		CGContextFillPath(dc->cgContext);
	else
		CGContextStrokePath(dc->cgContext);
}

IUP_SDK_API void iupdrvDrawQuadraticBezier(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int x3, int y3, long color, int style, int line_width)
{
	/* lift quadratic control point to cubic (c1 = q0 + 2/3 * (q1 - q0)) */
	int cx1 = x1 + ((2 * (x2 - x1)) / 3);
	int cy1 = y1 + ((2 * (y2 - y1)) / 3);
	int cx2 = x3 + ((2 * (x2 - x3)) / 3);
	int cy2 = y3 + ((2 * (y2 - y3)) / 3);
	iupdrvDrawBezier(dc, x1, y1, cx1, cy1, cx2, cy2, x3, y3, color, style, line_width);
}



static CGGradientRef cocoaTouchDrawCreateGradient(long color1, long color2)
{
	CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
	CGFloat comps[] = {
		iupDrawRed(color1)/255.0, iupDrawGreen(color1)/255.0, iupDrawBlue(color1)/255.0, iupDrawAlpha(color1)/255.0,
		iupDrawRed(color2)/255.0, iupDrawGreen(color2)/255.0, iupDrawBlue(color2)/255.0, iupDrawAlpha(color2)/255.0
	};
	CGFloat locations[] = { 0.0, 1.0 };
	CGGradientRef gradient = CGGradientCreateWithColorComponents(cs, comps, locations, 2);
	CGColorSpaceRelease(cs);
	return gradient;
}

IUP_SDK_API void iupdrvDrawLinearGradient(IdrawCanvas* dc, int x1, int y1, int x2, int y2, float angle, long color1, long color2)
{
	if (!dc) return;
	iupDrawCheckSwapCoord(x1, x2);
	iupDrawCheckSwapCoord(y1, y2);

	CGGradientRef gradient = cocoaTouchDrawCreateGradient(color1, color2);
	if (!gradient) return;

	CGContextSaveGState(dc->cgContext);
	CGContextClipToRect(dc->cgContext, CGRectMake(x1, y1, x2 - x1 + 1, y2 - y1 + 1));

	CGFloat cx = x1 + (x2 - x1) / 2.0;
	CGFloat cy = y1 + (y2 - y1) / 2.0;
	CGFloat rad = angle * M_PI / 180.0;
	CGFloat dx = cos(rad), dy = sin(rad);
	CGFloat half_w = (x2 - x1) / 2.0;
	CGFloat half_h = (y2 - y1) / 2.0;
	CGFloat half_diag = sqrt(half_w * half_w + half_h * half_h);

	CGPoint start = CGPointMake(cx - dx * half_diag, cy - dy * half_diag);
	CGPoint end   = CGPointMake(cx + dx * half_diag, cy + dy * half_diag);
	CGContextDrawLinearGradient(dc->cgContext, gradient, start, end,
		kCGGradientDrawsBeforeStartLocation | kCGGradientDrawsAfterEndLocation);

	CGContextRestoreGState(dc->cgContext);
	CGGradientRelease(gradient);
}

IUP_SDK_API void iupdrvDrawRadialGradient(IdrawCanvas* dc, int cx, int cy, int radius, long colorCenter, long colorEdge)
{
	if (!dc || radius <= 0) return;

	CGGradientRef gradient = cocoaTouchDrawCreateGradient(colorCenter, colorEdge);
	if (!gradient) return;

	CGContextSaveGState(dc->cgContext);

	/* solid edge-color fill first for AA boundary; gradient doesn't AA */
	CGRect circle = CGRectMake(cx - radius, cy - radius, 2 * radius, 2 * radius);
	CGContextBeginPath(dc->cgContext);
	CGContextAddEllipseInRect(dc->cgContext, circle);
	cocoaTouchDrawSetFillColor(dc->cgContext, colorEdge);
	CGContextFillPath(dc->cgContext);

	CGContextDrawRadialGradient(dc->cgContext, gradient,
		CGPointMake(cx, cy), 0.0,
		CGPointMake(cx, cy), (CGFloat)radius,
		0);

	CGContextRestoreGState(dc->cgContext);
	CGGradientRelease(gradient);
}

IUP_SDK_API void iupdrvDrawText(IdrawCanvas* dc, const char* text, int len, int x, int y, int w, int h, long color, const char* font, int flags, double text_orientation)
{
	if (!dc || !text) return;
	if (len == 0 && text[0] == '\0') return;

	IupCocoaTouchFont* iup_font = (font && *font) ? iupCocoaTouchFindFont(font) : iupCocoaTouchGetFont(dc->ih);
	if (!iup_font)
	{
		return;
	}

	@autoreleasepool
	{
		NSString* ns_string;
		if (len > 0)
		{
			ns_string = [[[NSString alloc] initWithBytes:text length:len encoding:NSUTF8StringEncoding] autorelease];
		}
		else
		{
			ns_string = [NSString stringWithUTF8String:text];
		}
		if (!ns_string) return;

		NSMutableDictionary* attrs = [[[iup_font attributeDictionary] mutableCopy] autorelease];

		UIColor* ui_color = [UIColor colorWithRed:iupDrawRed(color)/255.0
		                                    green:iupDrawGreen(color)/255.0
		                                     blue:iupDrawBlue(color)/255.0
		                                    alpha:iupDrawAlpha(color)/255.0];
		[attrs setObject:ui_color forKey:NSForegroundColorAttributeName];

		NSMutableParagraphStyle* pstyle = [[[NSMutableParagraphStyle alloc] init] autorelease];
		if (flags & IUP_DRAW_RIGHT)       [pstyle setAlignment:NSTextAlignmentRight];
		else if (flags & IUP_DRAW_CENTER) [pstyle setAlignment:NSTextAlignmentCenter];
		else                              [pstyle setAlignment:NSTextAlignmentLeft];

		if (flags & IUP_DRAW_WRAP)          [pstyle setLineBreakMode:NSLineBreakByWordWrapping];
		else if (flags & IUP_DRAW_ELLIPSIS) [pstyle setLineBreakMode:NSLineBreakByTruncatingTail];
		else                                [pstyle setLineBreakMode:NSLineBreakByClipping];
		[attrs setObject:pstyle forKey:NSParagraphStyleAttributeName];

		int layout_w = w, layout_h = h;
		int layout_center = flags & IUP_DRAW_LAYOUTCENTER;
		if (text_orientation != 0.0)
		{
			iupDrawGetTextSize(dc->ih, text, len, &layout_w, &layout_h, 0);
		}

		CGContextSaveGState(dc->cgContext);
		UIGraphicsPushContext(dc->cgContext);

		if (flags & IUP_DRAW_CLIP)
		{
			CGContextClipToRect(dc->cgContext, CGRectMake(x, y, w, h));
		}

		if (text_orientation != 0.0)
		{
			CGAffineTransform t;
			if (layout_center)
			{
				t = CGAffineTransformMakeTranslation(x + w/2.0, y + h/2.0);
				t = CGAffineTransformRotate(t, -text_orientation * M_PI / 180.0);
				t = CGAffineTransformTranslate(t, -(x + w/2.0), -(y + h/2.0));
			}
			else
			{
				t = CGAffineTransformMakeTranslation(x, y);
				t = CGAffineTransformRotate(t, -text_orientation * M_PI / 180.0);
				t = CGAffineTransformTranslate(t, -x, -y);
			}
			CGContextConcatCTM(dc->cgContext, t);
		}

		if ((flags & IUP_DRAW_CLIP) || (flags & IUP_DRAW_WRAP) || (flags & IUP_DRAW_ELLIPSIS))
		{
			CGRect rect;
			if (text_orientation != 0.0 && layout_center)
			{
				rect = CGRectMake(x + (w - layout_w)/2.0, y + (h - layout_h)/2.0, layout_w, layout_h);
			}
			else
			{
				rect = CGRectMake(x, y, layout_w, layout_h);
			}
			[ns_string drawInRect:rect withAttributes:attrs];
		}
		else
		{
			CGPoint point;
			if (text_orientation != 0.0 && layout_center)
			{
				point = CGPointMake(x + (w - layout_w)/2.0, y + (h - layout_h)/2.0);
			}
			else
			{
				point = CGPointMake(x, y);
			}
			[ns_string drawAtPoint:point withAttributes:attrs];
		}

		UIGraphicsPopContext();
		CGContextRestoreGState(dc->cgContext);
	}
}

IUP_SDK_API void iupdrvDrawImage(IdrawCanvas* dc, const char* name, int make_inactive, const char* bgcolor, long tint, int opacity, int x, int y, int w, int h, int sx, int sy, int sw, int sh, int quality)
{
	if (!dc || !name) return;

	UIImage* ui_image = (UIImage*)iupImageGetImageTint(name, dc->ih, make_inactive, bgcolor, tint);
	if (!ui_image) return;

	@autoreleasepool
	{
		CGSize size = [ui_image size];
		if (sw <= 0 || sh <= 0)
		{
			sx = 0;
			sy = 0;
			sw = (int)size.width;
			sh = (int)size.height;
		}
		if (w <= 0) w = sw;
		if (h <= 0) h = sh;

		if (sx != 0 || sy != 0 || sw != (int)size.width || sh != (int)size.height)
		{
			CGImageRef sub_img = CGImageCreateWithImageInRect([ui_image CGImage], CGRectMake(sx, sy, sw, sh));
			if (sub_img)
			{
				ui_image = [UIImage imageWithCGImage:sub_img];
				CGImageRelease(sub_img);
			}
		}

		CGContextSaveGState(dc->cgContext);
		CGContextSetInterpolationQuality(dc->cgContext, quality == IUP_DRAW_IMAGE_NEAREST ? kCGInterpolationNone : kCGInterpolationDefault);
		UIGraphicsPushContext(dc->cgContext);
		[ui_image drawInRect:CGRectMake(x, y, w, h) blendMode:kCGBlendModeNormal alpha:opacity / 255.0];
		UIGraphicsPopContext();
		CGContextRestoreGState(dc->cgContext);
	}
}

IUP_SDK_API void iupdrvDrawSelectRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
	if (!dc) return;
	iupDrawCheckSwapCoord(x1, x2);
	iupDrawCheckSwapCoord(y1, y2);

	CGContextSaveGState(dc->cgContext);
	/* translucent tint, blended so content shows through */
	CGContextSetRGBFillColor(dc->cgContext, 0.0, 0.47, 0.85, 0.25);
	CGContextFillRect(dc->cgContext, CGRectMake(x1, y1, x2 - x1 + 1, y2 - y1 + 1));
	CGContextRestoreGState(dc->cgContext);
}

IUP_SDK_API void iupdrvDrawFocusRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
	if (!dc) return;
	iupDrawCheckSwapCoord(x1, x2);
	iupDrawCheckSwapCoord(y1, y2);
	dc->draw_focus = 1;
	dc->focus_x1 = x1; dc->focus_y1 = y1;
	dc->focus_x2 = x2; dc->focus_y2 = y2;
}

/* w/h are logical points matching the w*h*4 buffer; step source by scale on Retina */
IUP_SDK_API int iupdrvCanvasGetImageData(Ihandle* ih, unsigned char* data, int w, int h)
{
	if (!ih || !data || w <= 0 || h <= 0) return 0;

	size_t buf_w = 0, buf_h = 0;
	CGFloat scale = 1.0;
	unsigned char* src = iupCocoaTouchCanvasGetPixels(ih, &buf_w, &buf_h, &scale);
	if (!src) return 0;

	size_t bpr = buf_w * 4;
	for (int y = 0; y < h; y++)
	{
		size_t src_y = (size_t)((double)y * scale);
		if (src_y >= buf_h) break;
		unsigned char* row = src + src_y * bpr;
		unsigned char* dst = data + (size_t)y * (size_t)w * 4;
		for (int x = 0; x < w; x++)
		{
			size_t src_x = (size_t)((double)x * scale);
			if (src_x >= buf_w) break;
			unsigned char r = row[src_x*4 + 0];
			unsigned char g = row[src_x*4 + 1];
			unsigned char b = row[src_x*4 + 2];
			unsigned char a = row[src_x*4 + 3];
			if (a > 0 && a < 255)
			{
				r = (unsigned char)(((int)r * 255) / a);
				g = (unsigned char)(((int)g * 255) / a);
				b = (unsigned char)(((int)b * 255) / a);
			}
			dst[x*4 + 0] = r;
			dst[x*4 + 1] = g;
			dst[x*4 + 2] = b;
			dst[x*4 + 3] = a;
		}
	}
	return 1;
}

/* IupDrawGetImage allocates dc->w * dc->h * 4 bytes (logical points). The CG bitmap is dc->w*scale by dc->h*scale (pixels) on Retina, so stride into source pixels at scale step. */
IUP_SDK_API int iupdrvDrawGetImageData(IdrawCanvas* dc, unsigned char* data)
{
	if (!dc || !data) return 0;

	size_t bpr = CGBitmapContextGetBytesPerRow(dc->cgContext);
	unsigned char* src = (unsigned char*)CGBitmapContextGetData(dc->cgContext);
	if (!src) return 0;

	size_t out_w = (size_t)dc->w;
	size_t out_h = (size_t)dc->h;
	if (out_w == 0 || out_h == 0) return 0;

	double scale = dc->scale > 0 ? dc->scale : 1.0;

	for (size_t y = 0; y < out_h; y++)
	{
		size_t src_y = (size_t)((double)y * scale);
		unsigned char* row = src + src_y * bpr;
		unsigned char* dst = data + y * out_w * 4;
		for (size_t x = 0; x < out_w; x++)
		{
			size_t src_x = (size_t)((double)x * scale);
			unsigned char r = row[src_x*4 + 0];
			unsigned char g = row[src_x*4 + 1];
			unsigned char b = row[src_x*4 + 2];
			unsigned char a = row[src_x*4 + 3];
			if (a != 0 && a != 255)
			{
				r = (unsigned char)((r * 255) / a);
				g = (unsigned char)((g * 255) / a);
				b = (unsigned char)((b * 255) / a);
			}
			dst[x*4 + 0] = r;
			dst[x*4 + 1] = g;
			dst[x*4 + 2] = b;
			dst[x*4 + 3] = a;
		}
	}
	return 1;
}
