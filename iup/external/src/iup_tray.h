/** \file
 * \brief Tray Control
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUP_TRAY_H
#define __IUP_TRAY_H

#ifdef __cplusplus
extern "C" {
#endif

void iupdrvTrayInitClass(Iclass* ic);
int iupdrvTraySetVisible(Ihandle* ih, int visible);
int iupdrvTraySetImage(Ihandle* ih, const char* value);
int iupdrvTraySetTip(Ihandle* ih, const char* value);
int iupdrvTraySetMenu(Ihandle* ih, Ihandle* menu);
void iupdrvTrayDestroy(Ihandle* ih);
int iupdrvTrayIsAvailable(void);

/* Image conversion for cross-backend tray support (GTK/Qt on Win32/Cocoa/SNI).
   Converts backend image format to ARGB pixels. Caller must free(*pixels). */
int iupdrvGetIconPixels(Ihandle* ih, const char* value, int* width, int* height, unsigned char** pixels);

#ifdef __cplusplus
}
#endif

#endif
