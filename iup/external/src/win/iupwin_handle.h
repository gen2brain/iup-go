/** \file
 * \brief HWND to Ihandle* table
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUPWIN_HANDLE_H 
#define __IUPWIN_HANDLE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Returns the IUP handle given the Windows handle. */

IUP_DRV_API Ihandle* iupwinHandleGet(InativeHandle* handle);
IUP_DRV_API void iupwinHandleAdd(Ihandle *ih, InativeHandle* handle);
IUP_DRV_API void iupwinHandleRemove(InativeHandle* handle);
IUP_DRV_API void iupwinHandleInit(void);
IUP_DRV_API void iupwinHandleFinish(void);


#ifdef __cplusplus
}
#endif

#endif
