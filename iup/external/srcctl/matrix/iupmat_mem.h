/** \file
 * \brief iupmatrix control
 * memory allocation.
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUPMAT_MEM_H 
#define __IUPMAT_MEM_H

#ifdef __cplusplus
extern "C" {
#endif

void iupMatrixMemAlloc(Ihandle* ih);
void iupMatrixMemRelease(Ihandle* ih);
void iupMatrixMemReAllocLines  (Ihandle* ih, int old_num, int num, int base);
void iupMatrixMemReAllocColumns(Ihandle* ih, int old_num, int num, int base);

#ifdef __cplusplus
}
#endif

#endif
