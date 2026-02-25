/** \file
 * \brief IupTable control internal definitions
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUP_TABLECONTROL_H
#define __IUP_TABLECONTROL_H

#ifdef __cplusplus
extern "C" {
#endif


/* ========================================================================= */
/* IupTable Data Structure                                                   */
/* ========================================================================= */

struct _IcontrolData
{
  /* Table dimensions */
  int num_lin;  /* Number of rows */
  int num_col;  /* Number of columns */

  /* Table behavior */
  int sortable;       /* Enable/disable column sorting */
  int allow_reorder;  /* Enable/disable column reordering */
  int user_resize;    /* Enable/disable user column resizing */
  int stretch_last;   /* Enable/disable last column stretching to fill space */

  int show_image;     /* Enable image display, set before map only */
  int fit_image;      /* Scale images to fit row height, default 1 (YES) */

  /* Platform-specific data */
  void* native_data;  /* Platform-specific data (GtkTreeView, QTableWidget, etc.) */
};

/* ========================================================================= */
/* Validation Functions (used by core and drivers)                          */
/* ========================================================================= */

int iupTableIsValid(Ihandle* ih);
int iupTableCheckCellPos(Ihandle* ih, int lin, int col);
char* iupTableGetCellImageCb(Ihandle* ih, int lin, int col);

/* ========================================================================= */
/* Driver Functions                                                         */
/* ========================================================================= */

/* Class initialization */
void iupdrvTableInitClass(Iclass* ic);

/* Widget lifecycle */
int iupdrvTableMapMethod(Ihandle* ih);
void iupdrvTableUnMapMethod(Ihandle* ih);

/* Table structure */
void iupdrvTableSetNumLin(Ihandle* ih, int num_lin);
void iupdrvTableSetNumCol(Ihandle* ih, int num_col);
void iupdrvTableAddLin(Ihandle* ih, int pos);
void iupdrvTableDelLin(Ihandle* ih, int pos);
void iupdrvTableAddCol(Ihandle* ih, int pos);
void iupdrvTableDelCol(Ihandle* ih, int pos);

/* Cell operations */
void iupdrvTableSetCellValue(Ihandle* ih, int lin, int col, const char* value);
char* iupdrvTableGetCellValue(Ihandle* ih, int lin, int col);
void iupdrvTableSetCellImage(Ihandle* ih, int lin, int col, const char* image);

/* Column operations */
void iupdrvTableSetColTitle(Ihandle* ih, int col, const char* title);
char* iupdrvTableGetColTitle(Ihandle* ih, int col);
void iupdrvTableSetColWidth(Ihandle* ih, int col, int width);
int iupdrvTableGetColWidth(Ihandle* ih, int col);

/* Selection */
void iupdrvTableSetFocusCell(Ihandle* ih, int lin, int col);
void iupdrvTableGetFocusCell(Ihandle* ih, int* lin, int* col);

/* Scrolling */
void iupdrvTableScrollToCell(Ihandle* ih, int lin, int col);

/* Display */
void iupdrvTableRedraw(Ihandle* ih);
void iupdrvTableSetShowGrid(Ihandle* ih, int show);

/* Sizing */
int iupdrvTableGetBorderWidth(Ihandle* ih);
int iupdrvTableGetRowHeight(Ihandle* ih);
int iupdrvTableGetHeaderHeight(Ihandle* ih);
void iupdrvTableAddBorders(Ihandle* ih, int* w, int* h);


#ifdef __cplusplus
}
#endif

#endif
