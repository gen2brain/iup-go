#ifndef __IUPCOCOA_DRAWCANVAS_H 
#define __IUPCOCOA_DRAWCANVAS_H

#include <stdbool.h>

@class IupCocoaCanvasView;
@class NSGraphicsContext;

struct _IdrawCanvas
{
	CGContextRef cgContext;
	IupCocoaCanvasView* canvasView;
	NSGraphicsContext* graphicsContext;
	Ihandle* ih;
	
	CGFloat w, h;
	bool useNativeFocusRing;
/*
	int draw_focus;
	int focus_x1;
	int focus_y1;
	int focus_x2;
	int focus_y2;
*/
	CGFloat clip_x1;
	CGFloat clip_y1;
	CGFloat clip_x2;
	CGFloat clip_y2;
};

#endif /* __IUPCOCOA_DRAWCANVAS_H */

