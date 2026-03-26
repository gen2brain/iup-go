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
void* iupdrvThreadStart(Ihandle* ih);
void  iupdrvThreadJoin(void* handle);
void  iupdrvThreadYield(void);
int   iupdrvThreadIsCurrent(void* handle);
void  iupdrvThreadExit(int code);
void  iupdrvThreadDestroy(void* handle);
void* iupdrvMutexCreate(void);
void  iupdrvMutexLock(void* handle);
void  iupdrvMutexUnlock(void* handle);
void  iupdrvMutexDestroy(void* handle);
/** @} */

#ifdef __cplusplus
}
#endif

#endif
