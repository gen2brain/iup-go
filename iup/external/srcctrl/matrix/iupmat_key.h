/** \file
 * \brief iupmatrix. keyboard control.
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUPMAT_KEY_H
#define __IUPMAT_KEY_H

#ifdef __cplusplus
extern "C" {
#endif

int  iupMatrixKeyPress_CB(Ihandle* ih, int c, int press);
int  iupMatrixProcessKeyPress(Ihandle* ih, int c);
IFniiiis iupMatrixGetActionCb(Ihandle* ih);

void iupMatrixKeyResetHomeEndCount(Ihandle* ih);

#ifdef __cplusplus
}
#endif

#endif
