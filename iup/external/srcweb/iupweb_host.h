/** \file
 * \brief Web Browser Control - native engine hosted by another toolkit
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUPWEB_HOST_H
#define __IUPWEB_HOST_H

#include "iup.h"

#ifdef __cplusplus
extern "C" {
#endif

/* host toolkit: sets ih->handle, returns the native parent (HWND or NSView*),
   or NULL with ih->handle set when the parent comes later through iupwebHostSetParent */
void* iupwebHostMap(Ihandle* ih);
void iupwebHostUnMap(Ihandle* ih);
void iupwebHostLayoutUpdate(Ihandle* ih);

/* web engine: view and visible clip rectangles in the native parent, physical pixels on Windows,
   points on macOS; an empty clip hides the view */
void iupwebHostSetBounds(Ihandle* ih, int x, int y, int width, int height, int clip_x, int clip_y, int clip_width, int clip_height);
void iupwebHostSetParent(Ihandle* ih, void* parent);

#ifdef __cplusplus
}
#endif

#endif
