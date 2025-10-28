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

  NSGraphicsContext* graphicsContext = [NSGraphicsContext currentContext];

  if (graphicsContext && [graphicsContext isDrawingToScreen])
  {
    dc->cgContext = [graphicsContext CGContext];
  }
  else
  {
    [dc->canvasView lockFocus];
    dc->release_context = 1;
    graphicsContext = [NSGraphicsContext currentContext];
    dc->cgContext = [graphicsContext CGContext];
  }

  if (!dc->cgContext)
  {
    if (dc->release_context) [dc->canvasView unlockFocus];
    free(dc);
    iupcocoaLogError("Failed to get CGContextRef for drawing.");
    return NULL;
  }

  CGRect bounds_rect = [dc->canvasView bounds];
  dc->w = bounds_rect.size.width;
  dc->h = bounds_rect.size.height;

  dc->cgLayer = CGLayerCreateWithContext(dc->cgContext, bounds_rect.size, NULL);
  dc->image_cgContext = CGLayerGetContext(dc->cgLayer);

  CGContextTranslateCTM(dc->image_cgContext, 0.0, dc->h);
  CGContextScaleCTM(dc->image_cgContext, 1.0, -1.0);

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
    [dc->canvasView unlockFocus];
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

    CGContextTranslateCTM(dc->image_cgContext, 0.0, dc->h);
    CGContextScaleCTM(dc->image_cgContext, 1.0, -1.0);

    CGContextSetLineCap(dc->image_cgContext, kCGLineCapButt);
    CGContextSetLineJoin(dc->image_cgContext, kCGLineJoinMiter);
  }
}

void iupdrvDrawFlush(IdrawCanvas* dc)
{
  CGContextSaveGState(dc->cgContext);

  CGContextTranslateCTM(dc->cgContext, 0.0, dc->h);
  CGContextScaleCTM(dc->cgContext, 1.0, -1.0);

  CGContextDrawLayerAtPoint(dc->cgContext, CGPointZero, dc->cgLayer);

  CGContextRestoreGState(dc->cgContext);

  if (dc->draw_focus)
  {
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
}

void iupdrvDrawGetSize(IdrawCanvas* dc, int *w, int *h)
{
  if (w) *w = iupROUND(dc->w);
  if (h) *h = iupROUND(dc->h);
}

void iupdrvDrawParentBackground(IdrawCanvas* dc)
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
      CGContextStrokeRect(cg_context, CGRectInset(iup_rect, 0.5, 0.5));
    }
    else
    {
      CGContextBeginPath(cg_context);
      CGContextMoveToPoint(cg_context, x1, y1);
      CGContextAddLineToPoint(cg_context, x2, y1);
      CGContextAddLineToPoint(cg_context, x2, y2);
      CGContextAddLineToPoint(cg_context, x1, y2);
      CGContextClosePath(cg_context);
      CGContextStrokePath(cg_context);
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

  for (int i = 1; i < count; i++)
    CGContextAddLineToPoint(cg_context, (CGFloat)points[2*i], (CGFloat)points[2*i+1]);

  if (style == IUP_DRAW_FILL)
    CGContextFillPath(cg_context);
  else
    CGContextStrokePath(cg_context);
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
