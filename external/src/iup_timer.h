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


void iupdrvTimerStop(Ihandle* ih);
void iupdrvTimerRun(Ihandle* ih);
void iupdrvTimerInitClass(Iclass* ic);

long long iupTimerGetLongLong(Ihandle* ih, const char* name);


#ifdef __cplusplus
}
#endif

#endif
