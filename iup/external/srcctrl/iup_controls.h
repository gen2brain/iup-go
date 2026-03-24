/** \file
 * \brief Additional Controls Class Initialization functions.
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUP_CONTROLS_H 
#define __IUP_CONTROLS_H


#ifdef __cplusplus
extern "C" {
#endif


Iclass* iupCellsNewClass(void);
Iclass* iupMatrixNewClass(void);
Iclass* iupMatrixListNewClass(void);
Iclass* iupMatrixExNewClass(void);

char *iupControlBaseGetParentBgColor (Ihandle* ih);
char *iupControlBaseGetBgColorAttrib(Ihandle* ih);


#ifdef __cplusplus
}
#endif

#endif
