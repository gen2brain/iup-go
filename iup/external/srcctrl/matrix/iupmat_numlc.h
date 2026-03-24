/** \file
 * \brief iupmatrix. change number of collumns or lines.
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUPMAT_NUMLC_H 
#define __IUPMAT_NUMLC_H

#ifdef __cplusplus
extern "C" {
#endif

int iupMatrixGetStartEnd(const char* value, int *base, int *count, int max, int del);

int  iupMatrixSetAddLinAttrib(Ihandle* ih, const char* value);
int  iupMatrixSetDelLinAttrib(Ihandle* ih, const char* value);
int  iupMatrixSetAddColAttrib(Ihandle* ih, const char* value);
int  iupMatrixSetDelColAttrib(Ihandle* ih, const char* value);

int  iupMatrixSetNumLinAttrib(Ihandle* ih, const char* value);
int  iupMatrixSetNumColAttrib(Ihandle* ih, const char* value);

void iupMatrixCopyLinAttributes(Ihandle* ih, int lin1, int lin2);
void iupMatrixCopyColAttributes(Ihandle* ih, int col1, int col2);

#ifdef __cplusplus
}
#endif

#endif
