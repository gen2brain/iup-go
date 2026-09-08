/** \file
 * \brief IupTable control internal definitions
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUP_TABLE_H
#define __IUP_TABLE_H

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup drvtable Driver Table Interface
 * \ingroup drv */


/* ========================================================================= */
/* IupTable Data Structure                                                   */
/* ========================================================================= */

struct _IcontrolData
{
  int num_lin;
  int num_col;

  int sortable;
  int allow_reorder;
  int user_resize;
  int stretch_last;

  int show_image;     /* set before map only */
  int fit_image;      /* default 1 */
  int show_dragdrop;  /* set before map only */

  void* native_data;
};

/* ========================================================================= */
/* Validation Functions (used by core and drivers)                          */
/* ========================================================================= */

int iupTableIsValid(Ihandle* ih);
int iupTableCheckCellPos(Ihandle* ih, int lin, int col);
char* iupTableGetCellImageCb(Ihandle* ih, int lin, int col);

int iupTableCallDragDropCb(Ihandle* ih, int drag_id, int drop_id, int *is_ctrl);
void iupTableMoveLinAttribs(Ihandle* ih, int from_lin, int to_lin);

/* ========================================================================= */
/* Driver Functions                                                         */
/* ========================================================================= */

/** \addtogroup drvtable
 * @{ */
/* Class initialization */
IUP_SDK_API void iupdrvTableInitClass(Iclass* ic);

/* Table structure */
IUP_SDK_API void iupdrvTableSetNumLin(Ihandle* ih, int num_lin);
IUP_SDK_API void iupdrvTableSetNumCol(Ihandle* ih, int num_col);
IUP_SDK_API void iupdrvTableAddLin(Ihandle* ih, int pos);
IUP_SDK_API void iupdrvTableDelLin(Ihandle* ih, int pos);
IUP_SDK_API void iupdrvTableAddCol(Ihandle* ih, int pos);
IUP_SDK_API void iupdrvTableDelCol(Ihandle* ih, int pos);

/* Cell operations */
IUP_SDK_API void iupdrvTableSetCellValue(Ihandle* ih, int lin, int col, const char* value);
IUP_SDK_API char* iupdrvTableGetCellValue(Ihandle* ih, int lin, int col);
IUP_SDK_API void iupdrvTableSetCellImage(Ihandle* ih, int lin, int col, const char* image);

/* Column operations */
IUP_SDK_API void iupdrvTableSetColTitle(Ihandle* ih, int col, const char* title);
IUP_SDK_API char* iupdrvTableGetColTitle(Ihandle* ih, int col);
IUP_SDK_API void iupdrvTableSetColWidth(Ihandle* ih, int col, int width);
IUP_SDK_API int iupdrvTableGetColWidth(Ihandle* ih, int col);

/* Selection */
IUP_SDK_API void iupdrvTableSetFocusCell(Ihandle* ih, int lin, int col);
IUP_SDK_API void iupdrvTableGetFocusCell(Ihandle* ih, int* lin, int* col);

/* Scrolling */
IUP_SDK_API void iupdrvTableScrollToCell(Ihandle* ih, int lin, int col);

/* Display */
IUP_SDK_API void iupdrvTableRedraw(Ihandle* ih);
IUP_SDK_API void iupdrvTableSetShowGrid(Ihandle* ih, int show);

/* Sizing */
IUP_SDK_API int iupdrvTableGetBorderWidth(Ihandle* ih);
IUP_SDK_API int iupdrvTableGetRowHeight(Ihandle* ih);
IUP_SDK_API int iupdrvTableGetHeaderHeight(Ihandle* ih);
IUP_SDK_API void iupdrvTableAddBorders(Ihandle* ih, int* w, int* h);
/** @} */


#ifdef __cplusplus
}
#endif

#endif
