/** \file
 * \brief IupThread driver interface
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUP_THREAD_H
#define __IUP_THREAD_H

#ifdef __cplusplus
extern "C" {
#endif

/** \addtogroup drv
 * @{ */
IUP_SDK_API void* iupdrvThreadStart(Ihandle* ih);
IUP_SDK_API void  iupdrvThreadJoin(void* handle);
IUP_SDK_API void  iupdrvThreadYield(void);
IUP_SDK_API int   iupdrvThreadIsCurrent(void* handle);
IUP_SDK_API void  iupdrvThreadExit(int code);
IUP_SDK_API void  iupdrvThreadDestroy(void* handle);
IUP_SDK_API void* iupdrvMutexCreate(void);
IUP_SDK_API void  iupdrvMutexLock(void* handle);
IUP_SDK_API void  iupdrvMutexUnlock(void* handle);
IUP_SDK_API void  iupdrvMutexDestroy(void* handle);
/** @} */

#ifdef __cplusplus
}
#endif

#endif
