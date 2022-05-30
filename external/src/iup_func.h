/** \file
 * \brief Global Function table (not exported API)
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUP_FUNC_H 
#define __IUP_FUNC_H

#ifdef __cplusplus
extern "C" {
#endif

/* called only in IupOpen and IupClose */
void iupFuncInit(void);
void iupFuncFinish(void);

char* iupGetCallbackName(Ihandle *ih, const char *name);
int iupGetFunctions(char** names, int n);

/* Other functions declared in <iup.h> and implemented here. 
IupGetFunction
IupSetFunction
*/

#ifdef __cplusplus
}
#endif

#endif
