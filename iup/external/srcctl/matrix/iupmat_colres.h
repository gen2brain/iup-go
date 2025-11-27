/** \file
 * \brief iupmatrix column resize.
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUPMAT_COLRES_H 
#define __IUPMAT_COLRES_H

#ifdef __cplusplus
extern "C" {
#endif

int   iupMatrixColResStart       (Ihandle* ih, int x, int y);
void  iupMatrixColResFinish      (Ihandle* ih, int x);
void  iupMatrixColResMove        (Ihandle* ih, int x);
int   iupMatrixColResCheckChangeCursor(Ihandle* ih, int x, int y);
int   iupMatrixColResIsResizing  (Ihandle* ih);

#ifdef __cplusplus
}
#endif

#endif
