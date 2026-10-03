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

/* host toolkit: sets ih->handle, returns the native parent (HWND or NSView*) */
void* iupwebHostMap(Ihandle* ih);
void iupwebHostUnMap(Ihandle* ih);
void iupwebHostLayoutUpdate(Ihandle* ih);

/* web engine: bounds in the native parent, physical pixels on Windows, points on macOS */
void iupwebHostSetBounds(Ihandle* ih, int x, int y, int width, int height, int visible);

#ifdef __cplusplus
}
#endif

#endif
