/** \file
 * \brief Cocoa Draw Functions
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUPCOCOA_DRAW_H
#define __IUPCOCOA_DRAW_H

#include "iup_export.h"

typedef struct _IdrawCanvas IdrawCanvas;

struct _IdrawCanvas
{
  Ihandle* ih;
  NSView* canvasView;

  CGContextRef cgContext;         /* bitmap buffer context (drawing target) */
  CGContextRef image_cgContext;   /* same as cgContext (kept for API symmetry with other drivers) */
  int release_context;
  CGFloat w, h;                   /* canvas size */

  /* clip region */
  CGFloat clip_x1, clip_y1, clip_x2, clip_y2;
  int clip_state;                 /* 0=no clip, 1=clip active */

  /* deferred focus rect drawing */
  int draw_focus;
  CGFloat focus_x1, focus_y1, focus_x2, focus_y2;
};

#endif /* __IUPCOCOA_DRAW_H */
