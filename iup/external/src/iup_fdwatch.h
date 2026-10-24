/** \file
 * \brief File Descriptor Watch Resource (not exported API)
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUP_FDWATCH_H
#define __IUP_FDWATCH_H

#ifdef __cplusplus
extern "C" {
#endif


#define IUP_FDWATCH_READ   1
#define IUP_FDWATCH_WRITE  2

IUP_SDK_API int iupFdWatchGetFd(Ihandle* ih);
IUP_SDK_API int iupFdWatchGetMode(Ihandle* ih);
IUP_SDK_API void iupFdWatchCallAction(Ihandle* ih);

/** \addtogroup drv
 * @{ */
IUP_SDK_API int iupdrvFdWatchStart(Ihandle* ih);
IUP_SDK_API void iupdrvFdWatchStop(Ihandle* ih);
IUP_SDK_API void iupdrvFdWatchInitClass(Iclass* ic);
/** @} */


#ifdef __cplusplus
}
#endif

#endif
