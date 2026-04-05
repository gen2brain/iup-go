/** \file
 * \brief Frame Control (not exported API)
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUP_FRAME_H
#define __IUP_FRAME_H

#ifdef __cplusplus
extern "C" {
#endif

/** \addtogroup drv
 * @{ */
IUP_SDK_API int iupdrvFrameHasClientOffset(Ihandle* ih);
IUP_SDK_API void iupdrvFrameInitClass(Iclass* ic);
/** Gets offset to client area. */
IUP_SDK_API void iupdrvFrameGetDecorOffset(Ihandle* ih, int *x, int *y);
/** Gets total decoration size. */
IUP_SDK_API int iupdrvFrameGetDecorSize(Ihandle* ih, int *w, int *h);
/** Gets title area height. */
IUP_SDK_API int iupdrvFrameGetTitleHeight(Ihandle* ih, int *h);
/** @} */

int iupFrameGetTitleHeight(Ihandle* ih);
char* iupFrameGetBgColorAttrib(Ihandle* ih);

#ifdef __cplusplus
}
#endif

#endif
