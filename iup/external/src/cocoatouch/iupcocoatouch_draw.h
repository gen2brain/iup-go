/** \file
 * \brief iOS UIKit Draw Functions (IdrawCanvas)
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUPCOCOATOUCH_DRAW_H
#define __IUPCOCOATOUCH_DRAW_H

#include "iup_export.h"

#ifdef __OBJC__
#import <UIKit/UIKit.h>
#endif

typedef struct _IdrawCanvas IdrawCanvas;

struct _IdrawCanvas
{
	Ihandle* ih;
#ifdef __OBJC__
	UIView* canvasView;
#else
	void* canvasView;
#endif

	/* recycled across Draw cycles, owned by the canvas widget */
	CGContextRef cgContext;

	/* logical points; bitmap is (w*scale)x(h*scale) px, CTM absorbs the scale */
	int w, h;
	CGFloat scale;

	int clip_state;
	int clip_x1, clip_y1, clip_x2, clip_y2;

	/* queued during Draw, committed on Flush so it sits above primitives */
	int draw_focus;
	int focus_x1, focus_y1, focus_x2, focus_y2;
};

#ifdef __OBJC__

/* persistent offscreen pixel buffer sized to the view's bounds (point*scale); reallocs on bounds change */
IUP_DRV_API unsigned char* iupCocoaTouchCanvasEnsurePixels(Ihandle* ih, UIView* view, size_t* out_pixel_w, size_t* out_pixel_h, CGFloat* out_scale);

/* cached pixel buffer without resizing; NULL if not allocated */
IUP_DRV_API unsigned char* iupCocoaTouchCanvasGetPixels(Ihandle* ih, size_t* out_pixel_w, size_t* out_pixel_h, CGFloat* out_scale);

/* free the pixel buffer; called from layoutSubviews and UnMap */
IUP_DRV_API void iupCocoaTouchCanvasReleaseBuffer(Ihandle* ih);

#endif /* __OBJC__ */

#endif
