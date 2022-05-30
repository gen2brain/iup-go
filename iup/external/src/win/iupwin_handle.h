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

Ihandle* iupwinHandleGet(InativeHandle* handle);
void iupwinHandleAdd(Ihandle *ih, InativeHandle* handle);
void iupwinHandleRemove(InativeHandle* handle);
void iupwinHandleInit(void);
void iupwinHandleFinish(void);


#ifdef __cplusplus
}
#endif

#endif
