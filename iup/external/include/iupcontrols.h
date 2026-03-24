/** \file
 * \brief Cells and Matrix controls.
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUPCONTROLS_H 
#define __IUPCONTROLS_H

#ifdef __cplusplus
extern "C" {
#endif


int  IupControlsOpen(void);

Ihandle* IupCells(void);
Ihandle* IupMatrix(const char *action);
Ihandle* IupMatrixList(void);
Ihandle* IupMatrixEx(void);


#ifdef __cplusplus
}
#endif

#endif
