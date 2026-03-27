/** \file
 * \brief GTK4 X11 Backend Support
 *
 * Centralizes all X11 functionality for the GTK4 driver.
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUPGTK4_X11_H
#define __IUPGTK4_X11_H

#ifdef GDK_WINDOWING_X11

#include "iup_export.h"

IUP_DRV_API int iupgtk4X11IsBackend(void);
IUP_DRV_API void* iupgtk4X11GetDisplay(void);
IUP_DRV_API unsigned long iupgtk4X11GetSurfaceXid(GdkSurface* surface);
IUP_DRV_API int iupgtk4X11IsSurface(GdkSurface* surface);

IUP_DRV_API int iupgtk4X11MoveWindow(GdkSurface* surface, int x, int y);
IUP_DRV_API int iupgtk4X11GetWindowPosition(GdkSurface* surface, int* x, int* y);
IUP_DRV_API int iupgtk4X11HideFromTaskbar(GdkSurface* surface);

IUP_DRV_API int iupgtk4X11QueryPointer(int* x, int* y);
IUP_DRV_API int iupgtk4X11WarpPointer(int x, int y);

IUP_DRV_API int iupgtk4X11GetDefaultScreen(void);
IUP_DRV_API char* iupgtk4X11GetServerVendor(void);
IUP_DRV_API int iupgtk4X11GetVendorRelease(void);

IUP_DRV_API int iupgtk4X11Sync(void);
IUP_DRV_API void iupgtk4X11Cleanup(void);

#endif

#endif
