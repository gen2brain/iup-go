/** \file
 * \brief Draw Functions
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>

#include <Cocoa/Cocoa.h>

#include "iup.h"

#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_str.h"
#include "iup_object.h"
#include "iup_image.h"
#include "iup_drvdraw.h"
#include "iup_draw.h"
#include "iupcocoa_draw.h"
#include "iupcocoa_canvas.h"




static CGColorRef coregraphicsCreateAutoreleasedColor(unsigned char r, unsigned char g, unsigned char b, unsigned a)
{
	// What color space should I be using?
	//	CGColorSpaceRef color_space = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
	
	CGFloat inv_byte = 1.0/255.0;
	CGColorRef the_color = CGColorCreateGenericRGB(r*inv_byte, g*inv_byte, b*inv_byte, a*inv_byte);
	// Requires 10.9, 7.0
	CFAutorelease(the_color);
	return the_color;
}

static void coregraphicsSetLineStyle(IdrawCanvas* dc, int style)
{
	CGContextRef cg_context = dc->cgContext;

	if((IUP_DRAW_STROKE == style) || (IUP_DRAW_FILL==style))
	{
		CGContextSetLineDash(cg_context, 0, NULL, 0);
	}
	else
	{
		if(IUP_DRAW_STROKE_DASH == style)
		{
			CGFloat dashes[2] = { 6.0, 2.0 };
			CGContextSetLineDash(cg_context, 0, dashes, 2);
		}
		else // DOTS
		{
			CGFloat dots[2] = { 2.0, 2.0 };
			CGContextSetLineDash(cg_context, 0, dots, 2);
		}
	}
}

/*
I had a bunch of misunderstandings about the relationship between the IupCanvasView and the IdrawCanvas.
The two are intertwined to some degree.
The DC needs the CGContext, the width & height, and info about which focus ring to use.
But the View can get all sorts of notifications like view size changes, user configuration changes, or there could even possibly be OS level events which change the context.
So I've tried several different ways to keep the objects in-sync with each other.
I knew that the DC was created later than the View, which caused some syncing issues.
However, I just discovered that the DC actually gets created and destroyed on every user callback.
So this means I need to change my approach. The view should never keep a reference to the DC, and everything in the DC should pull on demand from the View.
Minor optimizations might allow for caching the variables in the DC if it does indeed just go through one frame without any possibility of system interruption (e.g. a singular call inside drawRect:)
If this is not true, then we will need to always pull from the NSView.
*/
IdrawCanvas* iupdrvDrawCreateCanvas(Ihandle* ih)
{
	IdrawCanvas* dc = calloc(1, sizeof(IdrawCanvas));


	dc->ih = ih;

	// We'll set the dc directly from here this time, but all other places will set the dc in the IupCanvasView
	IupCocoaCanvasView* canvas_view =(IupCocoaCanvasView*)ih->handle;
	CGRect frame_rect = [canvas_view frame];

	// Should we retain? It is implied these will outlive our dc, so we shouldn't need to.
	dc->canvasView = canvas_view;
	dc->graphicsContext = [canvas_view graphicsContext];
	dc->cgContext = [canvas_view CGContext];

	// [dc->canvasView retain];
	// [dc->graphicsContext retain];
	//	CGContextRetain(dc->cgContext);
	
	dc->w = frame_rect.size.width;
	dc->h = frame_rect.size.height;

//	dc->cgContext = (CGContextRef)IupGetAttribute(ih, "CGCONTEXT");
//	dc->cgContext = (CGContextRef)IupGetAttribute(ih, "DRAWABLE");



	NSCAssert(dc->cgContext != NULL, @"CGContextRef should not be NULL");

	
	return dc;
}

void iupdrvDrawKillCanvas(IdrawCanvas* dc)
{
	// We are no longer retaining the context
	//	CGContextRelease(dc->cgContext);
	// [dc->graphicsContext release];
	// [dc->canvasView release];

	// Set to NULL defensively in case something breaks my assumptions. I hope to generate an obvious crash.
	dc->canvasView = NULL;
	dc->graphicsContext = NULL;
	dc->cgContext = NULL;
	dc->w = 0;
	dc->h = 0;
	free(dc);
	dc = NULL;
}




void iupdrvDrawArc(IdrawCanvas* dc, int x1, int y1, int x2, int y2, double a1, double a2, long color, int style, int line_width)
{
	unsigned char r = iupDrawRed(color), g = iupDrawGreen(color), b = iupDrawBlue(color), a = iupDrawAlpha(color);
	CGContextRef cg_context = dc->cgContext;
	
	CGColorRef the_color = coregraphicsCreateAutoreleasedColor(r, g, b, a);

	if(IUP_DRAW_FILL == style)
	{
		CGContextSetFillColorWithColor(cg_context, the_color);
	}
	else
	{
		CGContextSetStrokeColorWithColor(cg_context, the_color);
		coregraphicsSetLineStyle(dc, style);
	}
	CGContextSetLineWidth(cg_context, (CGFloat)line_width);

	CGFloat w = x2-x1+1;
	CGFloat h = y2-y1+1;
	CGFloat xc = x1 + w/2;
	CGFloat yc = y1 + h/2;

	// Must convert degrees to radians
	CGFloat rad1 = a1/180.0*M_PI;
	CGFloat rad2 = a2/180.0*M_PI;

	if (w == h)
	{
		CGContextAddArc(cg_context, xc, yc, w, rad1, rad2, 1);
		if(IUP_DRAW_FILL == style)
		{
			CGContextFillPath(cg_context);
		}
		else
		{
			CGContextStrokePath(cg_context);
		}
	}
	else  /* Ellipse: change the scale to create from the circle */
	{
		/* save to use the local transform */
		CGContextSaveGState(cg_context);
		
		CGContextTranslateCTM(cg_context, xc, yc);
		CGContextScaleCTM(cg_context, w/h, 1.0);
		CGContextTranslateCTM(cg_context, -xc, -yc);
	
		CGContextAddArc(cg_context, xc, yc, 0.5*h, rad1, rad2, 1);
		
		if(IUP_DRAW_FILL == style)
		{
			CGContextFillPath(cg_context);
		}
		else
		{
			CGContextStrokePath(cg_context);
		}
		
		/* restore from local */
		CGContextRestoreGState(cg_context);
	}
}

void iupdrvDrawPolygon(IdrawCanvas* dc, int* points, int count, long color, int style, int line_width)
{
	unsigned char r = iupDrawRed(color), g = iupDrawGreen(color), b = iupDrawBlue(color), a = iupDrawAlpha(color);
	CGContextRef cg_context = dc->cgContext;
	
	CGColorRef the_color = coregraphicsCreateAutoreleasedColor(r, g, b, a);
	CGContextSetLineWidth(cg_context, (CGFloat)line_width);

	if(IUP_DRAW_FILL == style)
	{
		CGContextSetFillColorWithColor(cg_context, the_color);
	}
	else
	{
		CGContextSetStrokeColorWithColor(cg_context, the_color);
		coregraphicsSetLineStyle(dc, style);
	}
	
	CGContextMoveToPoint(cg_context, (CGFloat)points[0], (CGFloat)points[1]);
	
	for(int i=0; i<count; i++)
	{
		CGContextAddLineToPoint(cg_context, (CGFloat)points[2*i], (CGFloat)points[2*i+1]);
	}
	
	if(IUP_DRAW_FILL == style)
	{
		CGContextFillPath(cg_context);
	}
	else
	{
		CGContextStrokePath(cg_context);
	}
}



void iupdrvDrawFocusRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{

//	IupCocoaCanvasView* canvas_view =(IupCocoaCanvasView*)dc->ih->handle;
	IupCocoaCanvasView* canvas_view = dc->canvasView;
	if([canvas_view useNativeFocusRing])
	{
		return;
	}
#if 1


//	NSLog(@"iupdrvDrawFocusRect");
//		NSLog(@"DrawFocus ih:0x%p for View: %@", dc->ih, dc->canvasView);

	CGContextRef cg_context = dc->cgContext;

//	NSLog(@"draw rect: %d %d %d %d", x1, y1, x2, y2);
	CGRect the_rect = CGRectMake(x1, y1, x2-x1, y2-y1);
	// Do I need an inset?
//	the_rect = CGRectInset(the_rect, 4, 4);
//	NSLog(@"draw rect: %@", NSStringFromRect(the_rect));

	NSColor* focus_ring_color = [NSColor keyboardFocusIndicatorColor];
//	NSColor* focus_ring_color = [NSColor greenColor];
	CGColorRef cg_focus_ring_color = [focus_ring_color CGColor];
	CGContextSetStrokeColorWithColor(cg_context, cg_focus_ring_color);
	CGContextSetLineWidth(cg_context, (CGFloat)4.0);

	// Requires 10.9
	CGPathRef path_ref = CGPathCreateWithRoundedRect(the_rect, 4.0, 4.0, NULL);
	CGContextAddPath(cg_context, path_ref);
	CGContextStrokePath(cg_context);

	CGPathRelease(path_ref);
#endif

	
}

IUP_SDK_API void iupdrvDrawGetClipRect(IdrawCanvas* dc, int *x1, int *y1, int *x2, int *y2)
{
  if (x1) *x1 = (int)dc->clip_x1;
  if (y1) *y1 = (int)dc->clip_y1;
  if (x2) *x2 = (int)dc->clip_x2;
  if (y2) *y2 = (int)dc->clip_y2;
}


void iupdrvDrawSetClipRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
	// This gets called a lot, so print only once.
	static bool gave_warning = false;
	if(!gave_warning)
	{
		NSLog(@"iupdrvDrawSetClipRect not tested");
		gave_warning = true;
	}
#if 1
	CGContextRef cg_context = dc->cgContext;

	CGContextSaveGState(cg_context);

	CGRect clip_rect = CGRectMake(x1, y1, x2-x1, y2-y1);
	
	CGContextClipToRect(cg_context, clip_rect);
#endif
	dc->clip_x1 = (CGFloat)x1;
	dc->clip_y1 = (CGFloat)y1;
	dc->clip_x2 = (CGFloat)x2;
	dc->clip_y2 = (CGFloat)y2;
}

// This is supposed to remove the clipping.
void iupdrvDrawResetClip(IdrawCanvas* dc)
{
	CGContextRef cg_context = dc->cgContext;

	// Assumption1: The user activated clip and we saved (pushed) the GState.
	// Assumption2: No other API can save a GState without (popping) Restoring (balancing) it before returning.
	// QED: When we pop here, we are popping the activated clip, thus removing it.
	CGContextRestoreGState(cg_context);

}

void iupdrvDrawParentBackground(IdrawCanvas* dc)
{
	NSLog(@"iupdrvDrawParentBackground needs to be verified");
	// I don't know if this is correct
	
	CGContextRef cg_context = dc->cgContext;
	CGFloat context_width = dc->w;
	CGFloat context_height = dc->h;

	NSColor* the_color = [NSColor controlBackgroundColor];
//	NSColor* the_color = [NSColor greenColor];
	CGColorRef cg_color = [the_color CGColor];
//	CGContextSetStrokeColorWithColor(cg_context, cg_color);
	CGContextSetFillColorWithColor(cg_context, cg_color);
	
	CGRect the_rectangle = CGRectMake(0, 0, context_width, context_height);
	CGContextAddRect(cg_context, the_rectangle);
	
	CGContextFillRect(cg_context, the_rectangle);
}
// TODO: text_orientation was added after this implementation was done but before the merge. We need to implement it.
void iupdrvDrawText(IdrawCanvas* dc, const char* text, int len, int x, int y, int w, int h, long color, const char* font, int flags, double text_orientation)
{
	// FIXME: We need to properly implement the font system in order for us to implement this.
	// FIXME: Need to figure out how to size up the rendered string, and then align it within the specified draw box.
	// FIXME: I think text may have IUP attributes like bold, underline, etc. Need to properly render.
//	NSLog(@"iupdrvDrawText not fully implemented");
	unsigned char r = iupDrawRed(color), g = iupDrawGreen(color), b = iupDrawBlue(color), a = iupDrawAlpha(color);
	CGContextRef cg_context = dc->cgContext;
	CGColorRef the_color = coregraphicsCreateAutoreleasedColor(r, g, b, a);
	CGContextSetStrokeColorWithColor(cg_context, the_color);
	CGContextSetFillColorWithColor(cg_context, the_color);
//	NSLog(@"iupdrvDrawText text: %s, len:%d", text, len);
//	NSLog(@"iupdrvDrawText font: %s", font);

	// Ugh. The CGContext text functions are deprecated. Need to use CoreText or Cocoa.
//	CGContextSelectFont(cg_context, "Helvetica", 12.0, kCGEncodingMacRoman);
//    CGContextSetTextDrawingMode(cg_context, kCGTextFill);
//	CGContextShowTextAtPoint(cg_context, x, y, text, len);
NSGraphicsContext* nsgc = [NSGraphicsContext graphicsContextWithCGContext:cg_context flipped:YES];
 [NSGraphicsContext saveGraphicsState];
 [NSGraphicsContext setCurrentContext:nsgc];
	

    NSPoint start_point = { x, y };
 //   startPoint.x = bounds.origin.x + bounds.size.width / 2 - size.width / 2;
 //   startPoint.y = bounds.origin.y + bounds.size.height / 2 - size.height / 2;
	NSString* ns_string = [NSString stringWithUTF8String:text];
    [ns_string drawAtPoint:start_point withAttributes: nil];
	 [NSGraphicsContext restoreGraphicsState];
}
void iupdrvDrawSelectRect(IdrawCanvas* dc, int x1, int y1, int x2, int y2)
{
	NSLog(@"iupdrvDrawSelectRect not implemented");
	
	// I don't know what this function does. I'm guessing at the color
	
	CGContextRef cg_context = dc->cgContext;

//	NSLog(@"draw rect: %d %d %d %d", x1, y1, x2, y2);
	CGRect the_rect = CGRectMake(x1, y1, x2-x1, y2-y1);
	// Do I need an inset?
//	the_rect = CGRectInset(the_rect, 4, 4);
//	NSLog(@"draw rect: %@", NSStringFromRect(the_rect));

	NSColor* the_color = [NSColor selectedControlColor];
//	NSColor* the_color = [NSColor greenColor];
	CGColorRef cg_color = [the_color CGColor];
//	CGContextSetStrokeColorWithColor(cg_context, cg_color);
	CGContextSetFillColorWithColor(cg_context, cg_color);

	// Requires 10.9
	CGPathRef path_ref = CGPathCreateWithRoundedRect(the_rect, 4.0, 4.0, NULL);
	CGContextAddPath(cg_context, path_ref);
	CGContextFillPath(cg_context);

	CGPathRelease(path_ref);
}

void iupdrvDrawLine(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
	unsigned char r = iupDrawRed(color), g = iupDrawGreen(color), b = iupDrawBlue(color), a = iupDrawAlpha(color);
	CGContextRef cg_context = dc->cgContext;
	
	CGColorRef the_color = coregraphicsCreateAutoreleasedColor(r, g, b, a);
	CGContextSetStrokeColorWithColor(cg_context, the_color);
	coregraphicsSetLineStyle(dc, style);
	CGContextSetLineWidth(cg_context, (CGFloat)line_width);

	CGContextMoveToPoint(cg_context, (CGFloat)x1, (CGFloat)y1);
	CGContextAddLineToPoint(cg_context, (CGFloat)x2, (CGFloat)y2);
	CGContextStrokePath(cg_context);
}


void iupdrvDrawRectangle(IdrawCanvas* dc, int x1, int y1, int x2, int y2, long color, int style, int line_width)
{
	unsigned char r = iupDrawRed(color), g = iupDrawGreen(color), b = iupDrawBlue(color), a = iupDrawAlpha(color);
	CGContextRef cg_context = dc->cgContext;
	
	CGColorRef the_color = coregraphicsCreateAutoreleasedColor(r, g, b, a);
	CGContextSetLineWidth(cg_context, (CGFloat)line_width);

	if(IUP_DRAW_FILL == style)
	{
		CGRect the_rectangle = CGRectMake(x1, y1, x2-x1, y2-y1);
		CGContextAddRect(cg_context, the_rectangle);
		
		CGContextSetFillColorWithColor(cg_context, the_color);
		CGContextFillRect(cg_context, the_rectangle);
	}
	else
	{
		// The other implementations make the line rect +1
		CGRect the_rectangle = CGRectMake(x1, y1, x2-x1+1, y2-y1+1);
		CGContextAddRect(cg_context, the_rectangle);
		
		CGContextSetStrokeColorWithColor(cg_context, the_color);
		coregraphicsSetLineStyle(dc, style);
		CGContextStrokePath(cg_context);
	}
}

void iupdrvDrawGetSize(IdrawCanvas* dc, int *w, int *h)
{
  if (w) *w = iupROUND(dc->w);
  if (h) *h = iupROUND(dc->h);
}

// NOTE: Searching through the code base, this never seems to get called by anything.
// So I don't know what this is supposed to do.
void iupdrvDrawUpdateSize(IdrawCanvas* dc)
{
//	NSLog(@"iupdrvDrawUpdateSize not implemented");

}

void iupdrvDrawFlush(IdrawCanvas* dc)
{
//	NSLog(@"iupdrvDrawFlush");
	// I don't think Apple gives us anything do anything here.
}

// TODO: w,h are new parameters added after this implementation, but before the merge. Need to test w,h implementation.
void iupdrvDrawImage(IdrawCanvas* dc, const char* name, int make_inactive, const char* bgcolor, int x, int y, int w, int h)
{
//	NSLog(@"iupdrvDrawImage not implemented");
	CGContextRef cg_context = dc->cgContext;
	NSImage* user_image = (NSImage*)iupImageGetImage(name, dc->ih, make_inactive, bgcolor);
//	[user_image autorelease]; // BAD: Iup is caching the value and returns the same pointer if cached. This results in a double autorelease.
//	NSImageRep* user_image_rep = nil;

//	NSSize image_size = [user_image size];
//	NSRect target_rect = NSMakeRect(x, y, image_size.width, image_size.height);
	NSRect target_rect = NSMakeRect(x, y, w, h);
	NSGraphicsContext* nsgc = [NSGraphicsContext graphicsContextWithCGContext:cg_context flipped:YES];
	[NSGraphicsContext saveGraphicsState];
	[NSGraphicsContext setCurrentContext:nsgc];
	
	[user_image drawInRect:target_rect];
	[NSGraphicsContext restoreGraphicsState];
	
}


