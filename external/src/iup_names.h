/** \file
 * \brief Ihandle <-> Name table manager (not exported API).
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUP_NAMES_H 
#define __IUP_NAMES_H

#ifdef __cplusplus
extern "C" {
#endif

/* called only in IupOpen and IupClose */
void iupNamesInit(void);
void iupNamesFinish(void);
void iupNamesDestroyHandles(void);

/* called from IupDestroy */
void iupRemoveNames(Ihandle* ih);

int iupNamesFindAll(Ihandle *ih, char** names, int n);

/* Other functions declared in <iup.h> and implemented here.
IupGetName
IupGetAllDialogs
IupGetAllNames
IupSetHandle
IupGetHandle
*/

#ifdef __cplusplus
}
#endif

#endif
