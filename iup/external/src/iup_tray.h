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

/** \addtogroup drv
 * @{ */
IUP_SDK_API void iupdrvTrayInitClass(Iclass* ic);
IUP_SDK_API int iupdrvTraySetVisible(Ihandle* ih, int visible);
IUP_SDK_API int iupdrvTraySetImage(Ihandle* ih, const char* value);
IUP_SDK_API int iupdrvTraySetTip(Ihandle* ih, const char* value);
IUP_SDK_API int iupdrvTraySetMenu(Ihandle* ih, Ihandle* menu);
IUP_SDK_API void iupdrvTrayDestroy(Ihandle* ih);
IUP_SDK_API int iupdrvTrayIsAvailable(void);
/** Converts backend image format to ARGB pixels. Caller must free(*pixels). */
IUP_SDK_API int iupdrvGetIconPixels(Ihandle* ih, const char* value, int* width, int* height, unsigned char** pixels);
/** @} */

#ifdef __cplusplus
}
#endif

#endif
