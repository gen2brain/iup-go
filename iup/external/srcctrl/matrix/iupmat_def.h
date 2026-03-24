/** \file
 * \brief iupmatrix. definitions.
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUPMAT_DEF_H 
#define __IUPMAT_DEF_H

#ifdef __cplusplus
extern "C" {
#endif


#define IMAT_PROCESS_COL 1  /* Process the columns */
#define IMAT_PROCESS_LIN 2  /* Process the lines */

/***************************************************************************/
/* Decoration size in pixels                                               */
/***************************************************************************/
#define IMAT_PADDING_W   6
#define IMAT_PADDING_H   6
#define IMAT_FRAME_W   2
#define IMAT_FRAME_H   2

/* Cell/Column/Line flags */
#define IMAT_HAS_FONT    0x01     /* Has FONTL:C attribute */
#define IMAT_HAS_FGCOLOR 0x02     /* Has FGCOLORL:C attribute */
#define IMAT_HAS_BGCOLOR 0x04     /* Has BGCOLORL:C attribute */
#define IMAT_IS_MARKED   0x08     /* Is marked */
#define IMAT_HAS_FRAMEHORIZCOLOR 0x10  /* Has FRAMEHORIZCOLORL:C */
#define IMAT_HAS_FRAMEVERTCOLOR  0x20  /* Has FRAMEVERTCOLORL:C */
#define IMAT_HAS_TYPE     0x40     /* Has TYPEL:C attribute */

/* Numeric Column flags */
#define IMAT_IS_NUMERIC  1      /* Is numeric */
#define IMAT_HAS_FORMAT  2      /* has format for lin!= 0 */
#define IMAT_HAS_FORMATTITLE 4  /* has format for lin== 0 */

enum{IMAT_TYPE_TEXT,
     IMAT_TYPE_COLOR,
     IMAT_TYPE_IMAGE,
     IMAT_TYPE_FILL};

enum{IMAT_EDITNEXT_LIN, 
     IMAT_EDITNEXT_COL, 
     IMAT_EDITNEXT_LINCR, 
     IMAT_EDITNEXT_COLCR,
     IMAT_EDITNEXT_NONE};

/* Text Alignment that will be draw. Used by iMatrixDrawCellValue */
#define IMAT_ALIGN_CENTER  0
#define IMAT_ALIGN_START    1
#define IMAT_ALIGN_END   2

typedef double (*ImatNumericConvertFunc)(double number, int quantity, int src_units, int dst_units);

/***************************************************************************/
/* Structures stored in each matrix                                        */
/***************************************************************************/
typedef struct _ImatCell
{
  char *value;          /* Cell value */
  unsigned char flags;  /* Attribute flags for the cell */
} ImatCell;

typedef struct _ImatLinCol
{
  int size;             /* Width/height of the column/line */
  unsigned char flags;  /* Attribute flags for the column/line */
} ImatLinCol;

typedef struct _ImatLinColData
{
  ImatLinCol* dt;   /* columns/lines data (allocated after map)   */

  int num;          /* Number of columns/lines, default/minimum=1, always includes the non scrollable cells */
  int num_alloc;    /* Number of columns/lines allocated, default=5 */
  int num_noscroll; /* Number of non scrollable columns/lines, default/minimum=1 */

  int first_offset; /* Scroll offset of the first visible column/line from right to left 
                       (or the invisible part of the first visible cell) 
                       This is how the scrollbar controls scrolling of cells. */
  int first;        /* First visible column/line */
  int last;         /* Last visible column/line  */

  /* used to configure the scrollbar */
  int total_visible_size;   /* Sum of the widths/heights of the columns/lines, not including the non scrollable cells */
  int current_visible_size; /* Width/height of the visible window, not including the non scrollable cells */

  int total_size;   /* Sum of the widths/heights of all columns/lines */

  int focus_cell;   /* index of the current cell */
} ImatLinColData;

typedef struct _ImatNumericData
{
  unsigned char quantity;
  unsigned char unit, unit_shown;
  unsigned char flags;  
} ImatNumericData;

typedef struct _ImatMergedData
{
  int start_lin;
  int start_col;
  int end_lin;
  int end_col;
  unsigned char used;
} ImatMergedData;

struct _IcontrolData
{
  iupCanvas canvas; /* from IupCanvas (must reserve it) */

  ImatCell** cells; /* Cell value, this will be NULL if in callback mode (allocated after map) */

  Ihandle* texth;   /* Text handle                    */
  Ihandle* droph;   /* Dropdown handle                */
  Ihandle* datah;   /* Current active edition element, may be equal to texth or droph */
  int editing,
    edit_hide_onfocus, edit_lin, edit_col,
    edit_hidden_byfocus;  /* Used by MatrixList */

  ImatLinColData lines;
  ImatLinColData columns;
  int noscroll_as_title; /* Non scrollable columns/lines shown as title columns/lines, default=0 */

  /* State */
  int has_focus;
  int w, h;
  int callback_mode;
  int need_calcsize;
  int need_redraw;
  int inside_markedit_cb;   /* avoid recursion */
  int inside_scroll_update;  /* ignore SCROLL_CB when setting DX/DY programmatically */
  int last_sort_col;

  /* attributes */
  int mark_continuous, mark_mode, mark_multiple;
  int hidden_text_marks, editnext;
  int use_title_size;   /* use title contents when calculating cell size */
  int limit_expand; /* limit expand to maximum size */
  int undo_redo, 
      flat,
      show_fill_value;

  /* Mouse and Keyboard AUX */
  int button1edit, button1press;
  int homekeycount, endkeycount;  /* numbers of times that key was pressed */

  /* ColRes AUX */
  int colres_dragging,   /* indicates if it is being made a column resize  */
      colres_drag_col,   /* column being resized, handler is at right of the column */
      colres_drag_col_start_x; /* handler start position */
  long colres_color;
  int colres_feedback;   /* draw the colres feedback */
  int colres_x, colres_y1, colres_y2;
  int colres_drag;       /* draw while dragging */

  /* Mark AUX */
  int mark_lin1, mark_col1,  /* used to store the start cell when a block is being marked */
      mark_lin2, mark_col2,  /* used to store the end cell when a block was marked */
      mark_full1,            /* indicate if full lines or columns is being selected */
      mark_full2;
  int mark_block;            /* inside MarkBlockBegin/MarkBlockEnd */

  /* Draw AUX, valid only after iupMatrixPrepareDrawData */
  sIFnii font_cb, type_cb;
  IFniiIII fgcolor_cb;
  IFniiIII bgcolor_cb;
  char *bgcolor, *bgcolor_parent, *fgcolor, *font;  /* not need to free */

  /* RGB color components for IupDraw */
  unsigned char bgcolor_r, bgcolor_g, bgcolor_b;
  unsigned char fgcolor_r, fgcolor_g, fgcolor_b;

  /* Clipping AUX for cell  */
  int clip_x1, clip_x2, clip_y1, clip_y2;

  /* Numeric Columns */
  char numeric_buffer_get[80];
  char numeric_buffer_set[80];
  ImatNumericData* numeric_columns;   /* information for numeric columns (allocated after map) */
  ImatNumericConvertFunc numeric_convert_func;

  /* Column Sort */
  int* sort_line_index;     /* Remap index of the line */
  int sort_has_index;       /* has a remap index of columns/lines */

  /* merged ranges */
  ImatMergedData* merge_info;  /* must free if not NULL */
  int merge_info_max, merge_info_count;
};

int iupMatrixIsValid(Ihandle* ih, int check_cells);
void iupMatrixRegisterEx(Iclass* ic);

int iupMatrixIsCharacter(int c);

#define iupMATRIX_CHECK_COL(_ih, _col) ((_col >= 0) && (_col < (_ih)->data->columns.num))
#define iupMATRIX_CHECK_LIN(_ih, _lin) ((_lin >= 0) && (_lin < (_ih)->data->lines.num))

int iupMatrixGetScrollbar(Ihandle* ih);
int iupMatrixGetScrollbarSize(Ihandle* ih);
int iupMatrixGetWidth(Ihandle* ih);
int iupMatrixGetHeight(Ihandle* ih);


#ifdef __cplusplus
}
#endif

#endif
