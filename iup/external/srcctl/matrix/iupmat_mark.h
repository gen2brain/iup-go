/** \file
 * \brief iupmatrix. cell selection.
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUPMAT_MARK_H 
#define __IUPMAT_MARK_H

#ifdef __cplusplus
extern "C" {
#endif

/* Used to mark (or not mark) cells */
#define IMAT_MARK_NO      0
#define IMAT_MARK_LIN     1
#define IMAT_MARK_COL     2
#define IMAT_MARK_LINCOL  3
#define IMAT_MARK_CELL    4

int iupMatrixSetMarkedAttrib(Ihandle* ih, const char* value);
char* iupMatrixGetMarkedAttrib(Ihandle* ih);
char* iupMatrixGetMarkAttrib(Ihandle* ih, int lin, int col);
int iupMatrixSetMarkAttrib(Ihandle* ih, int lin, int col, const char* value);

void iupMatrixMarkClearAll(Ihandle* ih, int check);

int iupMatrixGetMark(Ihandle* ih, int lin, int col, IFnii mark_cb);

int iupMatrixColumnIsMarked(Ihandle* ih, int col);
int iupMatrixLineIsMarked  (Ihandle* ih, int lin);

void iupMatrixMarkBlockSet(Ihandle* ih, int ctrl, int lin1, int col1);
void iupMatrixMarkBlockInc(Ihandle* ih, int lin2, int col2);
void iupMatrixMarkBlockReset(Ihandle* ih);


#ifdef __cplusplus
}
#endif

#endif
