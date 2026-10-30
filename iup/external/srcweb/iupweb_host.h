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

/* host toolkit: sets ih->handle, returns the native parent (HWND, NSView*, NSWindow* or X11 Window),
   or NULL with ih->handle set when the parent comes later through iupwebHostSetParent */
void* iupwebHostMap(Ihandle* ih);
void iupwebHostUnMap(Ihandle* ih);
void iupwebHostLayoutUpdate(Ihandle* ih);

/* web engine: view and visible clip rectangles in the native parent, physical pixels on Windows and X11,
   points on macOS; an empty clip hides the view */
void iupwebHostSetBounds(Ihandle* ih, int x, int y, int width, int height, int clip_x, int clip_y, int clip_width, int clip_height);
void iupwebHostSetParent(Ihandle* ih, void* parent);
/* X11 only: the host control got or lost the keyboard focus, and forwards its key events while it has it */
void iupwebHostSetFocus(Ihandle* ih, int focus);
void iupwebHostKeyEvent(Ihandle* ih, int press, unsigned int keycode, unsigned int state, unsigned long time);

#ifdef __cplusplus
}
#endif

#endif
