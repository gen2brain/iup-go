/** \file
 * \brief Timer Resource (not exported API)
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUP_TIMER_H
#define __IUP_TIMER_H

#ifdef __cplusplus
extern "C" {
#endif


/** \addtogroup drv
 * @{ */
IUP_SDK_API void iupdrvTimerStop(Ihandle* ih);
IUP_SDK_API void iupdrvTimerRun(Ihandle* ih);
IUP_SDK_API void iupdrvTimerInitClass(Iclass* ic);
/** @} */

long long iupTimerGetLongLong(Ihandle* ih, const char* name);


#ifdef __cplusplus
}
#endif

#endif
