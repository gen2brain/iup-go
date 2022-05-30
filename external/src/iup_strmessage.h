/** \file
 * \brief Language Dependent String Messages 
 *
 * See Copyright Notice in "iup.h"
 */

 
#ifndef __IUP_STRMESSAGE_H 
#define __IUP_STRMESSAGE_H

#ifdef __cplusplus
extern "C" {
#endif


/* Called from iup_global */
void iupStrMessageUpdateLanguage(const char* language);

/* called only in IupOpen and IupClose */
void iupStrMessageInit(void);
void iupStrMessageFinish(void);


#ifdef __cplusplus
}
#endif

#endif
