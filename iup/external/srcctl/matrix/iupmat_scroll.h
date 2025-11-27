/** \file
 * \brief iupmatrix control
 * scrolling.
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUPMAT_SCROLL_H 
#define __IUPMAT_SCROLL_H

#ifdef __cplusplus
extern "C" {
#endif

int  iupMatrixScroll_CB(Ihandle* ih, int action, float posx, float posy);

void iupMatrixScrollToVisible(Ihandle* ih, int lin, int col);

typedef void(*iupMatrixScrollMoveFunc)(Ihandle* ih, int mode, int m);
void  iupMatrixScrollMove(iupMatrixScrollMoveFunc func, Ihandle* ih, int mode, int m);

/* Used only by the macros below */
void iupMatrixScrollHomeFunc(Ihandle* ih, int, int);
void iupMatrixScrollEndFunc(Ihandle* ih, int, int);
void iupMatrixScrollLeftUpFunc(Ihandle* ih, int, int);
void iupMatrixScrollRightDownFunc(Ihandle* ih, int, int);
void iupMatrixScrollPgLeftUpFunc(Ihandle* ih, int, int);
void iupMatrixScrollPgRightDownFunc(Ihandle* ih, int, int);
void iupMatrixScrollPosFunc(Ihandle* ih, int, int);
void iupMatrixScrollCrFunc(Ihandle* ih, int, int);

/* Mode used to "walk" inside the matrix.
   It shows if the movement request was from the scrollbar or from a key.
   Possible values for the "mode" parameter of the iupMatrixScrollMove function.
 */
#define IMAT_SCROLLBAR    0
#define IMAT_SCROLLKEY    1

/* Macros to help during the call of iupMatrixScrollMove function */

/* used in the keyboard processing module */
#define iupMATRIX_ScrollKeyHome(ih)    iupMatrixScrollMove(iupMatrixScrollHomeFunc       , ih, IMAT_SCROLLKEY, 0)
#define iupMATRIX_ScrollKeyEnd(ih)     iupMatrixScrollMove(iupMatrixScrollEndFunc        , ih, IMAT_SCROLLKEY, 0)
#define iupMATRIX_ScrollKeyPgUp(ih)    iupMatrixScrollMove(iupMatrixScrollPgLeftUpFunc   , ih, IMAT_SCROLLKEY, IMAT_PROCESS_LIN)
#define iupMATRIX_ScrollKeyPgDown(ih)  iupMatrixScrollMove(iupMatrixScrollPgRightDownFunc, ih, IMAT_SCROLLKEY, IMAT_PROCESS_LIN)
#define iupMATRIX_ScrollKeyDown(ih)    iupMatrixScrollMove(iupMatrixScrollRightDownFunc  , ih, IMAT_SCROLLKEY, IMAT_PROCESS_LIN)
#define iupMATRIX_ScrollKeyRight(ih)   iupMatrixScrollMove(iupMatrixScrollRightDownFunc  , ih, IMAT_SCROLLKEY, IMAT_PROCESS_COL)
#define iupMATRIX_ScrollKeyUp(ih)      iupMatrixScrollMove(iupMatrixScrollLeftUpFunc     , ih, IMAT_SCROLLKEY, IMAT_PROCESS_LIN)
#define iupMATRIX_ScrollKeyLeft(ih)    iupMatrixScrollMove(iupMatrixScrollLeftUpFunc     , ih, IMAT_SCROLLKEY, IMAT_PROCESS_COL)
#define iupMATRIX_ScrollKeyCr(ih)      iupMatrixScrollMove(iupMatrixScrollCrFunc         , ih, IMAT_SCROLLKEY, 0)

/* Used by the scrollbar callback only */
#define iupMATRIX_ScrollUp(ih)         iupMatrixScrollMove(iupMatrixScrollLeftUpFunc     , ih, IMAT_SCROLLBAR, IMAT_PROCESS_LIN)
#define iupMATRIX_ScrollLeft(ih)       iupMatrixScrollMove(iupMatrixScrollLeftUpFunc     , ih, IMAT_SCROLLBAR, IMAT_PROCESS_COL)
#define iupMATRIX_ScrollDown(ih)       iupMatrixScrollMove(iupMatrixScrollRightDownFunc  , ih, IMAT_SCROLLBAR, IMAT_PROCESS_LIN)
#define iupMATRIX_ScrollRight(ih)      iupMatrixScrollMove(iupMatrixScrollRightDownFunc  , ih, IMAT_SCROLLBAR, IMAT_PROCESS_COL)
#define iupMATRIX_ScrollPgUp(ih)       iupMatrixScrollMove(iupMatrixScrollPgLeftUpFunc   , ih, IMAT_SCROLLBAR, IMAT_PROCESS_LIN)
#define iupMATRIX_ScrollPgLeft(ih)     iupMatrixScrollMove(iupMatrixScrollPgLeftUpFunc   , ih, IMAT_SCROLLBAR, IMAT_PROCESS_COL)
#define iupMATRIX_ScrollPgDown(ih)     iupMatrixScrollMove(iupMatrixScrollPgRightDownFunc, ih, IMAT_SCROLLBAR, IMAT_PROCESS_LIN)
#define iupMATRIX_ScrollPgRight(ih)    iupMatrixScrollMove(iupMatrixScrollPgRightDownFunc, ih, IMAT_SCROLLBAR, IMAT_PROCESS_COL)
#define iupMATRIX_ScrollPosVer(ih, y)  iupMatrixScrollMove(iupMatrixScrollPosFunc        , ih, IMAT_SCROLLBAR, IMAT_PROCESS_LIN)
#define iupMATRIX_ScrollPosHor(ih, x)  iupMatrixScrollMove(iupMatrixScrollPosFunc        , ih, IMAT_SCROLLBAR, IMAT_PROCESS_COL)


#ifdef __cplusplus
}
#endif

#endif
