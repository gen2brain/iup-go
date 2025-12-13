/** \file
 * \brief Cocoa Draw Functions
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>

#import <Cocoa/Cocoa.h>

#include "iup.h"
#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_str.h"
#include "iup_object.h"
#include "iup_image.h"
#include "iup_drvdraw.h"
#include "iup_draw.h"

#include "iupcocoa_drv.h"
#include "iupcocoa_draw.h"


static CGColorRef iupCocoaDrawCreateColor(long color)
{
  unsigned char r = iupDrawRed(color);
  unsigned char g = iupDrawGreen(color);
  unsigned char b = iupDrawBlue(color);
  unsigned char a = iupDrawAlpha(color);

  CGFloat inv_byte = 1.0/255.0;
  CGColorRef the_color = CGColorCreateGenericRGB(r*inv_byte, g*inv_byte, b*inv_byte, a*inv_byte);
  CFAutorelease(the_color);
  return the_color;
}

static void iupCocoaSetLineStyle(CGContextRef cg_context, int style)
{
  if (style == IUP_DRAW_STROKE || style == IUP_DRAW_FILL)
  {
    CGContextSetLineDash(cg_context, 0, NULL, 0);
  }
  else if (style == IUP_DRAW_STROKE_DASH)
  {
    CGFloat dashes[2] = { 9.0, 3.0 };
    CGContextSetLineDash(cg_context, 0, dashes, 2);
  }
  else if (style == IUP_DRAW_STROKE_DOT)
  {
    CGFloat dashes[2] = { 1.0, 2.0 };
    CGContextSetLineDash(cg_context, 0, dashes, 2);
  }
  else if (style == IUP_DRAW_STROKE_DASH_DOT)
  {
    CGFloat dashes[4] = { 7.0, 3.0, 1.0, 3.0 };
    CGContextSetLineDash(cg_context, 0, dashes, 4);
  }
  else if (style == IUP_DRAW_STROKE_DASH_DOT_DOT)
  {
    CGFloat dashes[6] = { 7.0, 3.0, 1.0, 3.0, 1.0, 3.0 };
    CGContextSetLineDash(cg_context, 0, dashes, 6);
  }
}

IdrawCanvas* iupdrvDrawCreateCanvas(Ihandle* ih)
{
  IdrawCanvas* dc = calloc(1, sizeof(IdrawCanvas));
  dc->ih = ih;
  dc->canvasView = iupcocoaGetMainView(ih);
  dc->release_context = 0;
  dc->draw_focus = 0;

  CGRect bounds_rect = [dc->canvasView bounds];
  dc->w = bounds_rect.size.width;
  dc->h = bounds_rect.size.height;

  NSGraphicsContext* graphicsContext = [NSGraphicsContext currentContext];

  /* Check if we're inside drawRect (ACTION callback) */
  if (graphicsContext && [graphicsContext isDrawingToScreen])
  {
    /* Inside drawRect - use current graphics context */
    dc->cgContext = [graphicsContext CGContext];
    dc->release_context = 0;
  }
  else
  {
    /* OpenGL canvases don't use IUP's draw buffer - OpenGL manages its own buffers */
    if (iupAttribGet(ih, "_IUP_GLCONTROLDATA"))
    {
      free(dc);
      return NULL;
    }

    /* Outside drawRect (e.g., SCROLL_CB) - use persistent buffer */
    NSBitmapImageRep* buffer = (NSBitmapImageRep*)iupAttribGet(ih, "_IUPCOCOA_CANVAS_BUFFER");

    if (buffer)
    {
      /* Check if size changed - recreate buffer if needed */
      if ([buffer pixelsWide] != (NSInteger)dc->w || [buffer pixelsHigh] != (NSInteger)dc->h)
      {
        [buffer release];
        buffer = nil;
      }
    }

    /* Create new buffer if needed */
    if (!buffer)
    {
      buffer = [[NSBitmapImageRep alloc]
        initWithBitmapDataPlanes:NULL
        pixelsWide:(NSInteger)dc->w
        pixelsHigh:(NSInteger)dc->h
        bitsPerSample:8
        samplesPerPixel:4
        hasAlpha:YES
        isPlanar:NO
        colorSpaceName:NSCalibratedRGBColorSpace
        bytesPerRow:0
        bitsPerPixel:0];

      iupAttribSet(ih, "_IUPCOCOA_CANVAS_BUFFER", (char*)buffer);
    }

    /* Create graphics context from bitmap with FLIPPED coordinate system
     * to match the flipped view coordinate system */
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();

    /* Use proper CGBitmapInfo for RGBA with premultiplied alpha */
    CGBitmapInfo bitmapInfo = kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big;

    CGContextRef cgContext = CGBitmapContextCreate(
      [buffer bitmapData],
      [buffer pixelsWide],
      [buffer pixelsHigh],
      [buffer bitsPerSample],
      [buffer bytesPerRow],
      colorSpace,
      bitmapInfo
    );
    CGColorSpaceRelease(colorSpace);

    if (!cgContext)
    {
      free(dc);
      return NULL;
    }

    graphicsContext = [NSGraphicsContext graphicsContextWithCGContext:cgContext flipped:YES];

    [NSGraphicsContext saveGraphicsState];
    [NSGraphicsContext setCurrentContext:graphicsContext];
    dc->cgContext = cgContext;
    dc->release_context = 1;
  }

  if (!dc->cgContext)
  {
    free(dc);
    iupcocoaLogError("Failed to get CGContextRef for drawing.");
    return NULL;
  }

  dc->cgLayer = CGLayerCreateWithContext(dc->cgContext, bounds_rect.size, NULL);
  dc->image_cgContext = CGLayerGetContext(dc->cgLayer);

  /* Coordinate system handling:
   * - DIRECT PATH: View is flipped, context is already top-down - no transform needed
   * - BUFFER PATH: Manually created CGContext is bottom-up - NEEDS Y-flip transform */
  if (dc->release_context)
  {
    /* Buffer path: Apply Y-flip to convert bottom-up CGContext to top-down */
    CGContextTranslateCTM(dc->image_cgContext, 0.0, dc->h);
    CGContextScaleCTM(dc->image_cgContext, 1.0, -1.0);
  }

  CGContextSetLineCap(dc->image_cgContext, kCGLineCapButt);
  CGContextSetLineJoin(dc->image_cgContext, kCGLineJoinMiter);

  dc->clip_state = 0;

  iupAttribSet(ih, "DRAWDRIVER", "COCOA");
  return dc;
}

void iupdrvDrawKillCanvas(IdrawCanvas* dc)
{
  if (!dc) return;

  CGLayerRelease(dc->cgLayer);

  if (dc->release_context)
  {
    [NSGraphicsContext restoreGraphicsState];
    /* Release manually created CGContext for buffer path */
    if (dc->cgContext)
    {
      CGContextRelease(dc->cgContext);
    }
  }

  free(dc);
}

void iupdrvDrawUpdateSize(IdrawCanvas* dc)
{
  CGRect bounds_rect = [dc->canvasView bounds];
  CGFloat w = bounds_rect.size.width;
  CGFloat h = bounds_rect.size.height;

  if (w != dc->w || h != dc->h)
  {
    dc->w = w;
    dc->h = h;

    iupdrvDrawResetClip(dc);

    CGLayerRelease(dc->cgLayer);
    dc->cgLayer = CGLayerCreateWithContext(dc->cgContext, bounds_rect.size, NULL);
    dc->image_cgContext = CGLayerGetContext(dc->cgLayer);

    CGContextSetLineCap(dc->image_cgContext, kCGLineCapButt);
    CGContextSetLineJoin(dc->image_cgContext, kCGLineJoinMiter);
  }
}

void iupdrvDrawFlush(IdrawCanvas* dc)
{
  /* NO transformation needed - view is already flipped, draw layer directly */
  CGContextDrawLayerAtPoint(dc->cgContext, CGPointZero, dc->cgLayer);

  if (dc->draw_focus)
  {
    /* Only create NSGraphicsContext when focus ring is actually needed */
    CGRect cocoa_rect = CGRectMake(dc->focus_x1, dc->h - dc->focus_y2 - 1,
                                   dc->focus_x2 - dc->focus_x1 + 1,
                                   dc->focus_y2 - dc->focus_y1 + 1);
    NSGraphicsContext* nsContext = [NSGraphicsContext graphicsContextWithCGContext:dc->cgContext flipped:NO];
    [NSGraphicsContext saveGraphicsState];
    [NSGraphicsContext setCurrentContext:nsContext];
    NSSetFocusRingStyle(NSFocusRingOnly);
    [[NSBezierPath bezierPathWithRect:cocoa_rect] fill];
    [NSGraphicsContext restoreGraphicsState];

    dc->draw_focus = 0;
  }

  CGContextFlush(dc->cgContext);

  /* If drawing to persistent buffer (outside drawRect), trigger widget repaint */
  if (dc->release_context)
  {
    NSBitmapImageRep* buffer = (NSBitmapImageRep*)iupAttribGet(dc->ih, "_IUPCOCOA_CANVAS_BUFFER");
    if (buffer)
    {
      /* Mark buffer as updated so drawRect knows to use it */
      iupAttribSet(dc->ih, "_IUPCOCOA_BUFFER_DIRTY", "1");
    }

    /* Trigger widget repaint to copy buffer to screen (in drawRect) */
    [dc->canvasView setNeedsDisplay:YES];
  }
}

void iupdrvDrawGetSize(IdrawCanvas* dc, int *w, int *h)
{
  if (w) *w = iupROUND(dc->w);
  if (h) *h = iupROUND(dc->h);
}

void cocoaDrawParentBackground(IdrawCanvas* dc)
{
  char* color_str = iupBaseNativeParentGetBgColor(dc->ih);
  if (!color_str)
    color_str = "255 255 255";

  long color = iupDrawStrToColor(color_str, 0);

  CGContextRef cg_context = dc->image_cgContext;
  CGColorRef the_color = iupCocoaDrawCreateColor(color);

  CGContextSetFillColorWithColor(cg_context, the_color);
  CGContextFillRect(cg_context, CGRectMake(0, 0, dc->w, dc->h));
}

void iupdrvDrawRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  CGContextRef cg_context = dc->image_cgContext;
  CGColorRef the_color = iupCocoaDrawCreateColor(color);

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  CGRect iup_rect = CGRectMake(x1, y1, x2 - x1 + 1, y2 - y1 + 1);

  if (style == IUP_DRAW_FILL)
  {
    CGContextSetFillColorWithColor(cg_context, the_color);
    CGContextFillRect(cg_context, iup_rect);
  }
  else
  {
    CGContextSetStrokeColorWithColor(cg_context, the_color);
    CGContextSetLineWidth(cg_context, (CGFloat)line_width);
    iupCocoaSetLineStyle(cg_context, style);

    if (line_width == 1)
    {
      /* Use CGRectInset for pixel-perfect 1px lines */
      CGContextStrokeRect(cg_context, CGRectInset(iup_rect, 0.5, 0.5));
    }
    else
    {
      /* Use direct CGContextStrokeRect for wider lines - simpler and faster */
      CGContextStrokeRect(cg_context, iup_rect);
    }
  }
}

void iupdrvDrawLine(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  CGContextRef cg_context = dc->image_cgContext;
  CGColorRef the_color = iupCocoaDrawCreateColor(color);

  CGContextSetStrokeColorWithColor(cg_context, the_color);
  CGContextSetLineWidth(cg_context, (CGFloat)line_width);
  iupCocoaSetLineStyle(cg_context, style);

  CGContextBeginPath(cg_context);

  if (line_width == 1)
  {
    if (x1 == x2)
    {
      iupDrawCheckSwapCoord(y1, y2);
      CGContextMoveToPoint(cg_context, x1 + 0.5, y1);
      CGContextAddLineToPoint(cg_context, x1 + 0.5, y2 + 1);
    }
    else if (y1 == y2)
    {
      iupDrawCheckSwapCoord(x1, x2);
      CGContextMoveToPoint(cg_context, x1, y1 + 0.5);
      CGContextAddLineToPoint(cg_context, x2 + 1, y1 + 0.5);
    }
    else
    {
      CGContextMoveToPoint(cg_context, x1, y1);
      CGContextAddLineToPoint(cg_context, x2, y2);
    }
  }
  else
  {
    CGContextMoveToPoint(cg_context, x1, y1);
    CGContextAddLineToPoint(cg_context, x2, y2);
  }

  CGContextStrokePath(cg_context);
}

void iupdrvDrawArc(IdrawCanvas* dc, int x1, int y1, int x2, int y2, double a1, double a2, long color, int style, int line_width)
{
  CGContextRef cg_context = dc->image_cgContext;
  CGColorRef the_color = iupCocoaDrawCreateColor(color);

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
  int is_clockwise = 1;

  if (w == h)
  {
    CGContextBeginPath(cg_context);

    if (style == IUP_DRAW_FILL)
    {
      CGContextSetFillColorWithColor(cg_context, the_color);
      CGContextMoveToPoint(cg_context, xc, yc);
      CGContextAddArc(cg_context, xc, yc, 0.5*w, rad1, rad2, is_clockwise);
      CGContextClosePath(cg_context);
      CGContextFillPath(cg_context);
    }
    else
    {
      CGContextSetStrokeColorWithColor(cg_context, the_color);
      CGContextSetLineWidth(cg_context, (CGFloat)line_width);
      iupCocoaSetLineStyle(cg_context, style);
      CGContextAddArc(cg_context, xc, yc, 0.5*w, rad1, rad2, is_clockwise);
      CGContextStrokePath(cg_context);
    }
  }
  else
  {
    if (style == IUP_DRAW_FILL)
    {
      CGContextSetFillColorWithColor(cg_context, the_color);

      CGContextSaveGState(cg_context);
      CGContextTranslateCTM(cg_context, xc, yc);
      CGContextScaleCTM(cg_context, w/2.0, h/2.0);

      CGContextBeginPath(cg_context);
      CGContextMoveToPoint(cg_context, 0, 0);
      CGContextAddArc(cg_context, 0, 0, 1.0, rad1, rad2, is_clockwise);
      CGContextClosePath(cg_context);
      CGContextFillPath(cg_context);

      CGContextRestoreGState(cg_context);
    }
    else
    {
      CGContextSetStrokeColorWithColor(cg_context, the_color);
      CGContextSetLineWidth(cg_context, (CGFloat)line_width);
      iupCocoaSetLineStyle(cg_context, style);

      CGMutablePathRef path = CGPathCreateMutable();
      CGAffineTransform transform = CGAffineTransformMakeTranslation(xc, yc);
      transform = CGAffineTransformScale(transform, w/2.0, h/2.0);

      CGPathAddArc(path, &transform, 0, 0, 1.0, rad1, rad2, is_clockwise);

      CGContextBeginPath(cg_context);
      CGContextAddPath(cg_context, path);
      CGPathRelease(path);

      CGContextStrokePath(cg_context);
    }
  }
}

void iupdrvDrawEllipse(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
  CGContextRef cg_context = dc->image_cgContext;
  CGColorRef the_color = iupCocoaDrawCreateColor(color);

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  CGFloat w = x2 - x1 + 1;
  CGFloat h = y2 - y1 + 1;
  if (w <= 0 || h <= 0) return;

  CGRect rect = CGRectMake((CGFloat)x1, (CGFloat)y1, w, h);

  CGContextBeginPath(cg_context);
  CGContextAddEllipseInRect(cg_context, rect);

  if (style == IUP_DRAW_FILL)
  {
    CGContextSetFillColorWithColor(cg_context, the_color);
    CGContextFillPath(cg_context);
  }
  else
  {
    CGContextSetStrokeColorWithColor(cg_context, the_color);
    CGContextSetLineWidth(cg_context, (CGFloat)line_width);
    iupCocoaSetLineStyle(cg_context, style);
    CGContextStrokePath(cg_context);
  }
}

void iupdrvDrawPolygon(IdrawCanvas* dc, int* points, int count, long color, int style, int line_width)
{
  CGContextRef cg_context = dc->image_cgContext;
  CGColorRef the_color = iupCocoaDrawCreateColor(color);

  if (style == IUP_DRAW_FILL)
    CGContextSetFillColorWithColor(cg_context, the_color);
  else
  {
    CGContextSetStrokeColorWithColor(cg_context, the_color);
    CGContextSetLineWidth(cg_context, (CGFloat)line_width);
    iupCocoaSetLineStyle(cg_context, style);
  }

  CGContextBeginPath(cg_context);
  CGContextMoveToPoint(cg_context, (CGFloat)points[0], (CGFloat)points[1]);

  /* Start at i=1 to avoid redundant line to first point */
  for (int i = 1; i < count; i++)
    CGContextAddLineToPoint(cg_context, (CGFloat)points[2*i], (CGFloat)points[2*i+1]);

  CGContextClosePath(cg_context);  /* Close polygon by connecting last point to first */

  if (style == IUP_DRAW_FILL)
    CGContextFillPath(cg_context);
  else
    CGContextStrokePath(cg_context);
}

void iupdrvDrawPixel(IdrawCanvas* dc, int x, int y, long color)
{
  CGContextRef cg_context = dc->image_cgContext;
  CGColorRef the_color = iupCocoaDrawCreateColor(color);

  CGContextSetFillColorWithColor(cg_context, the_color);
  CGContextFillRect(cg_context, CGRectMake((CGFloat)x, (CGFloat)y, 1.0, 1.0));
}

void iupdrvDrawRoundedRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int corner_radius, long color, int style, int line_width)
{
  CGContextRef cg_context = dc->image_cgContext;
  CGColorRef the_color = iupCocoaDrawCreateColor(color);
  CGFloat radius = (CGFloat)corner_radius;

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  /* Clamp radius to prevent oversized corners */
  CGFloat max_radius = ((x2 - x1) < (y2 - y1)) ? (CGFloat)(x2 - x1) / 2.0f : (CGFloat)(y2 - y1) / 2.0f;
  if (radius > max_radius)
    radius = max_radius;

  /* Set color and line properties */
  if (style == IUP_DRAW_FILL)
    CGContextSetFillColorWithColor(cg_context, the_color);
  else
  {
    CGContextSetStrokeColorWithColor(cg_context, the_color);
    CGContextSetLineWidth(cg_context, (CGFloat)line_width);
    iupCocoaSetLineStyle(cg_context, style);
  }

  /* Create rounded rectangle path */
  CGRect rect = CGRectMake((CGFloat)x1, (CGFloat)y1, (CGFloat)(x2 - x1 + 1), (CGFloat)(y2 - y1 + 1));
  CGPathRef path = CGPathCreateWithRoundedRect(rect, radius, radius, NULL);

  /* Draw the path */
  CGContextBeginPath(cg_context);
  CGContextAddPath(cg_context, path);

  if (style == IUP_DRAW_FILL)
    CGContextFillPath(cg_context);
  else
    CGContextStrokePath(cg_context);

  CGPathRelease(path);
}

void iupdrvDrawBezier(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, long color, int style, int line_width)
{
  CGContextRef cg_context = dc->image_cgContext;
  CGColorRef the_color = iupCocoaDrawCreateColor(color);

  /* Set color and line properties */
  if (style == IUP_DRAW_FILL)
    CGContextSetFillColorWithColor(cg_context, the_color);
  else
  {
    CGContextSetStrokeColorWithColor(cg_context, the_color);
    CGContextSetLineWidth(cg_context, (CGFloat)line_width);
    iupCocoaSetLineStyle(cg_context, style);
  }

  /* Create cubic Bezier path */
  CGContextBeginPath(cg_context);
  CGContextMoveToPoint(cg_context, (CGFloat)x1, (CGFloat)y1);
  CGContextAddCurveToPoint(cg_context,
                           (CGFloat)x2, (CGFloat)y2,  /* First control point */
                           (CGFloat)x3, (CGFloat)y3,  /* Second control point */
                           (CGFloat)x4, (CGFloat)y4); /* End point */

  /* Draw the path */
  if (style == IUP_DRAW_FILL)
    CGContextFillPath(cg_context);
  else
    CGContextStrokePath(cg_context);
}

void iupdrvDrawQuadraticBezier(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int x3, int y3, long color, int style, int line_width)
{
  /* Convert quadratic Bezier to cubic Bezier using the 2/3 formula:
   * Given quadratic: Q(t) with control points q0, q1, q2
   * Convert to cubic: C(t) with control points c0, c1, c2, c3
   *
   * c0 = q0                        (start point)
   * c1 = q0 + (2/3) * (q1 - q0)   (first control point)
   * c2 = q2 + (2/3) * (q1 - q2)   (second control point)
   * c3 = q2                        (end point)
   */
  int cx1, cy1, cx2, cy2;

  /* Calculate cubic control points from quadratic */
  cx1 = x1 + ((2 * (x2 - x1)) / 3);
  cy1 = y1 + ((2 * (y2 - y1)) / 3);
  cx2 = x3 + ((2 * (x2 - x3)) / 3);
  cy2 = y3 + ((2 * (y2 - y3)) / 3);

  /* Draw as cubic Bezier */
  iupdrvDrawBezier(dc, x1, y1, cx1, cy1, cx2, cy2, x3, y3, color, style, line_width);
}

void iupdrvDrawGetClipRect(IdrawCanvas* dc, int *x1, int *y1, int *x2, int *y2)
{
  if (x1) *x1 = (int)dc->clip_x1;
  if (y1) *y1 = (int)dc->clip_y1;
  if (x2) *x2 = (int)dc->clip_x2;
  if (y2) *y2 = (int)dc->clip_y2;
}

void iupdrvDrawSetClipRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0)
  {
    iupdrvDrawResetClip(dc);
    return;
  }

  if (x1 >= x2) x1 = x2;
  if (y1 >= y2) y1 = y2;

  iupdrvDrawResetClip(dc);

  CGContextSaveGState(dc->image_cgContext);
  dc->clip_state = 1;

  CGRect clip_rect = CGRectMake(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
  CGContextClipToRect(dc->image_cgContext, clip_rect);

  dc->clip_x1 = (CGFloat)x1;
  dc->clip_y1 = (CGFloat)y1;
  dc->clip_x2 = (CGFloat)x2;
  dc->clip_y2 = (CGFloat)y2;
}

void iupdrvDrawSetClipRoundedRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2, int corner_radius)
{
  CGFloat radius = (CGFloat)corner_radius;

  if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0)
  {
    iupdrvDrawResetClip(dc);
    return;
  }

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  /* IUP coordinates: use width + 1 and height + 1 */
  CGFloat w = (CGFloat)(x2 - x1 + 1);
  CGFloat h = (CGFloat)(y2 - y1 + 1);

  /* Clamp radius to prevent oversized corners */
  CGFloat max_radius = (w < h) ? w / 2.0f : h / 2.0f;
  if (radius > max_radius)
    radius = max_radius;

  iupdrvDrawResetClip(dc);

  CGContextSaveGState(dc->image_cgContext);
  dc->clip_state = 1;

  /* Create rounded rectangle path and use as clip */
  CGRect rect = CGRectMake((CGFloat)x1, (CGFloat)y1, w, h);
  CGPathRef path = CGPathCreateWithRoundedRect(rect, radius, radius, NULL);

  CGContextBeginPath(dc->image_cgContext);
  CGContextAddPath(dc->image_cgContext, path);
  CGContextClip(dc->image_cgContext);

  CGPathRelease(path);

  dc->clip_x1 = (CGFloat)x1;
  dc->clip_y1 = (CGFloat)y1;
  dc->clip_x2 = (CGFloat)x2;
  dc->clip_y2 = (CGFloat)y2;
}

void iupdrvDrawResetClip(IdrawCanvas* dc)
{
  if (dc->clip_state == 1)
  {
    CGContextRestoreGState(dc->image_cgContext);
    dc->clip_state = 0;
  }

  dc->clip_x1 = 0;
  dc->clip_y1 = 0;
  dc->clip_x2 = 0;
  dc->clip_y2 = 0;
}

void iupdrvDrawText(IdrawCanvas* dc, const char* text, int len, int x, int y, int w, int h, long color, const char* font, int flags, double text_orientation)
{
  if (!text || (len == 0 && text[0] == '\0'))
    return;

  @autoreleasepool {
    CGContextRef cg_context = dc->image_cgContext;
    CGContextSaveGState(cg_context);

    NSGraphicsContext* temp_ns_context = [NSGraphicsContext graphicsContextWithCGContext:cg_context flipped:YES];
    [NSGraphicsContext saveGraphicsState];
    [NSGraphicsContext setCurrentContext:temp_ns_context];

    IupCocoaFont* iupFont = (font && *font != '\0') ? iupcocoaFindFont(font) : iupcocoaGetFont(dc->ih);
    if (!iupFont)
    {
      [NSGraphicsContext restoreGraphicsState];
      CGContextRestoreGState(cg_context);
      return;
    }

    NSMutableDictionary* attributes = [[iupFont.attributeDictionary mutableCopy] autorelease];

    unsigned char r = iupDrawRed(color);
    unsigned char g = iupDrawGreen(color);
    unsigned char b = iupDrawBlue(color);
    unsigned char a = iupDrawAlpha(color);
    NSColor* ns_color = [NSColor colorWithCalibratedRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:a/255.0];
    [attributes setObject:ns_color forKey:NSForegroundColorAttributeName];

    NSMutableParagraphStyle* pStyle = [[[NSMutableParagraphStyle alloc] init] autorelease];
    if (flags & IUP_DRAW_RIGHT)
      pStyle.alignment = NSTextAlignmentRight;
    else if (flags & IUP_DRAW_CENTER)
      pStyle.alignment = NSTextAlignmentCenter;
    else
      pStyle.alignment = NSTextAlignmentLeft;

    if (flags & IUP_DRAW_WRAP)
      pStyle.lineBreakMode = NSLineBreakByWordWrapping;
    else if (flags & IUP_DRAW_ELLIPSIS)
      pStyle.lineBreakMode = NSLineBreakByTruncatingTail;
    else
      pStyle.lineBreakMode = NSLineBreakByClipping;

    [attributes setObject:pStyle forKey:NSParagraphStyleAttributeName];

    NSString* ns_string = len > 0 ?
      [[[NSString alloc] initWithBytes:text length:len encoding:NSUTF8StringEncoding] autorelease] :
      [NSString stringWithUTF8String:text];

    int layout_w = w;
    int layout_h = h;
    int layout_center = flags & IUP_DRAW_LAYOUTCENTER;

    if (text_orientation && layout_center)
      iupDrawGetTextSize(dc->ih, text, len, &layout_w, &layout_h, 0);

    NSRect text_rect = NSMakeRect(x, y, layout_w, layout_h);

    if (text_orientation != 0.0)
    {
      NSAffineTransform* transform = [NSAffineTransform transform];

      if (layout_center)
      {
        NSPoint center = NSMakePoint(x + layout_w / 2.0, y + layout_h / 2.0);
        [transform translateXBy:center.x yBy:center.y];
        [transform rotateByDegrees:-text_orientation];
        [transform translateXBy:-center.x yBy:-center.y];
        [transform translateXBy:(w - layout_w) / 2.0 yBy:(h - layout_h) / 2.0];
      }
      else
      {
        [transform translateXBy:x yBy:y];
        [transform rotateByDegrees:-text_orientation];
        [transform translateXBy:-x yBy:-y];
      }

      [transform concat];
    }

    if (flags & IUP_DRAW_CLIP)
    {
      [NSBezierPath clipRect:NSMakeRect(x, y, w, h)];
    }

    [ns_string drawInRect:text_rect withAttributes:attributes];

    [NSGraphicsContext restoreGraphicsState];
    CGContextRestoreGState(cg_context);
  }
}

void iupdrvDrawImage(IdrawCanvas* dc, const char* name, int make_inactive, const char* bgcolor, int x, int y, int w, int h)
{
  NSImage* user_image = (NSImage*)iupImageGetImage(name, dc->ih, make_inactive, bgcolor);
  if (!user_image)
    return;

  @autoreleasepool {
    NSSize image_size = [user_image size];
    if (w == -1 || w == 0) w = image_size.width;
    if (h == -1 || h == 0) h = image_size.height;

    CGContextRef cg_context = dc->image_cgContext;
    CGContextSaveGState(cg_context);

    /* Use flipped NSGraphicsContext to correctly draw image in y-down coordinate system */
    NSGraphicsContext* temp_ns_context = [NSGraphicsContext graphicsContextWithCGContext:cg_context flipped:YES];
    [NSGraphicsContext saveGraphicsState];
    [NSGraphicsContext setCurrentContext:temp_ns_context];

    CGRect target_rect = CGRectMake(x, y, w, h);
    [user_image drawInRect:target_rect];

    [NSGraphicsContext restoreGraphicsState];
    CGContextRestoreGState(cg_context);
  }
}

void iupdrvDrawSelectRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  CGRect iup_rect = CGRectMake(x1, y1, x2 - x1 + 1, y2 - y1 + 1);

  CGColorRef select_color = iupCocoaDrawCreateColor(iupDrawColor(0, 0, 255, 153));
  CGContextSetFillColorWithColor(dc->image_cgContext, select_color);
  CGContextFillRect(dc->image_cgContext, iup_rect);
}

void iupdrvDrawFocusRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  if (iupAttribGetBoolean(dc->ih, "NATIVEFOCUSRING"))
  {
    /* Defer focus ring drawing to flush for native macOS appearance */
    dc->draw_focus = 1;
    dc->focus_x1 = x1;
    dc->focus_y1 = y1;
    dc->focus_x2 = x2;
    dc->focus_y2 = y2;
  }
  else
  {
    /* Draw a simple dotted rectangle for focus indication */
    CGContextRef cg_context = dc->image_cgContext;
    CGContextSaveGState(cg_context);

    CGRect iup_rect = CGRectMake(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
    CGFloat dashes[2] = { 1.0, 1.0 };
    CGColorRef focus_color = iupCocoaDrawCreateColor(iupDrawColor(0, 0, 0, 224));

    CGContextSetStrokeColorWithColor(cg_context, focus_color);
    CGContextSetLineWidth(cg_context, 1.0);
    CGContextSetLineDash(cg_context, 0, dashes, 2);
    CGContextStrokeRect(cg_context, CGRectInset(iup_rect, 0.5, 0.5));

    CGContextRestoreGState(cg_context);
  }
}

void iupdrvDrawLinearGradient(IdrawCanvas* dc, int x1, int y1, int x2, int y2, float angle, long color1, long color2)
{
  CGContextRef cg_context = dc->image_cgContext;
  CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();

  iupDrawCheckSwapCoord(x1, x2);
  iupDrawCheckSwapCoord(y1, y2);

  /* Create gradient colors */
  CGFloat components[8] = {
    iupDrawRed(color1) / 255.0f, iupDrawGreen(color1) / 255.0f, iupDrawBlue(color1) / 255.0f, iupDrawAlpha(color1) / 255.0f,
    iupDrawRed(color2) / 255.0f, iupDrawGreen(color2) / 255.0f, iupDrawBlue(color2) / 255.0f, iupDrawAlpha(color2) / 255.0f
  };
  CGFloat locations[2] = { 0.0, 1.0 };

  CGGradientRef gradient = CGGradientCreateWithColorComponents(colorSpace, components, locations, 2);

  /* Calculate gradient endpoints based on angle */
  /* 0 = left to right, 90 = top to bottom, 180 = right to left, 270 = bottom to top */
  CGFloat rad = angle * M_PI / 180.0f;

  /* IUP coordinates: use width + 1 and height + 1 */
  CGFloat w = (CGFloat)(x2 - x1 + 1);
  CGFloat h = (CGFloat)(y2 - y1 + 1);
  CGFloat cx = x1 + w / 2.0f;
  CGFloat cy = y1 + h / 2.0f;

  CGPoint start = CGPointMake(cx - (w * cos(rad)) / 2.0f, cy - (h * sin(rad)) / 2.0f);
  CGPoint end = CGPointMake(cx + (w * cos(rad)) / 2.0f, cy + (h * sin(rad)) / 2.0f);

  /* Clip to rectangle */
  CGContextSaveGState(cg_context);
  CGContextClipToRect(cg_context, CGRectMake(x1, y1, w, h));

  /* Draw gradient */
  /* Use kCGGradientDrawsBeforeStartLocation | kCGGradientDrawsAfterEndLocation
     to ensure the gradient fills the entire clipped area, otherwise corners that fall
     outside the projected start/end planes (common in diagonal gradients) will be cut off. */
  CGContextDrawLinearGradient(cg_context, gradient, start, end, kCGGradientDrawsBeforeStartLocation | kCGGradientDrawsAfterEndLocation);

  CGContextRestoreGState(cg_context);
  CGGradientRelease(gradient);
  CGColorSpaceRelease(colorSpace);
}

void iupdrvDrawRadialGradient(IdrawCanvas* dc, int cx, int cy, int radius, long colorCenter, long colorEdge)
{
  CGContextRef cg_context = dc->image_cgContext;
  CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();

  /* Create gradient colors */
  CGFloat components[8] = {
    iupDrawRed(colorCenter) / 255.0f, iupDrawGreen(colorCenter) / 255.0f, iupDrawBlue(colorCenter) / 255.0f, iupDrawAlpha(colorCenter) / 255.0f,
    iupDrawRed(colorEdge) / 255.0f, iupDrawGreen(colorEdge) / 255.0f, iupDrawBlue(colorEdge) / 255.0f, iupDrawAlpha(colorEdge) / 255.0f
  };
  CGFloat locations[2] = { 0.0, 1.0 };

  CGGradientRef gradient = CGGradientCreateWithColorComponents(colorSpace, components, locations, 2);

  CGPoint center = CGPointMake(cx, cy);
  CGRect circleRect = CGRectMake(cx - radius, cy - radius, 2 * radius, 2 * radius);

  CGContextSaveGState(cg_context);

  /* First draw the circle with solid fill to get anti-aliased edges
   * Gradients don't anti-alias their edges, but filled paths do.
   * Use the edge color as the fill color for the circle boundary. */
  CGContextBeginPath(cg_context);
  CGContextAddEllipseInRect(cg_context, circleRect);
  CGContextSetRGBFillColor(cg_context,
                           iupDrawRed(colorEdge) / 255.0f,
                           iupDrawGreen(colorEdge) / 255.0f,
                           iupDrawBlue(colorEdge) / 255.0f,
                           iupDrawAlpha(colorEdge) / 255.0f);
  CGContextFillPath(cg_context);

  /* Now draw the gradient on top - it will have smooth edges from the solid fill underneath */
  CGContextDrawRadialGradient(cg_context, gradient, center, 0, center, radius, 0);

  CGContextRestoreGState(cg_context);

  CGGradientRelease(gradient);
  CGColorSpaceRelease(colorSpace);
}
