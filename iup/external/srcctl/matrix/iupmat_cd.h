/** \file
 * \brief iupmatrix. IupDraw help macros.
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUPMAT_CD_H
#define __IUPMAT_CD_H

#ifdef __cplusplus
extern "C" {
#endif

/* IupDraw-based drawing macros for matrix control */
/* Note: IupDraw uses top-down coordinates natively, no Y-axis inversion needed */

#define iupMATRIX_LINE(_ih,_x1,_y1,_x2,_y2) \
  IupDrawLine((_ih), (_x1), (_y1), (_x2), (_y2))

/* Matrix code uses (xmin, xmax, ymin, ymax) but IupDrawRectangle expects (x1, y1, x2, y2) */
/* So we need to reorder: (xmin, xmax, ymin, ymax) -> (xmin, ymin, xmax, ymax) */
#define iupMATRIX_BOX(_ih,_xmin,_xmax,_ymin,_ymax) do { \
  IupSetAttribute((_ih), "DRAWSTYLE", "FILL"); \
  IupDrawRectangle((_ih), (_xmin), (_ymin), (_xmax), (_ymax)); \
} while(0)

#define iupMATRIX_RECT(_ih,_xmin,_xmax,_ymin,_ymax) do { \
  IupSetAttribute((_ih), "DRAWSTYLE", "STROKE"); \
  IupDrawRectangle((_ih), (_xmin), (_ymin), (_xmax), (_ymax)); \
} while(0)

#define iupMATRIX_CLIPAREA(_ih,_xmin,_xmax,_ymin,_ymax) \
  IupDrawSetClipRect((_ih), (_xmin), (_ymin), (_xmax), (_ymax))

#define iupMATRIX_TEXT_CENTERED(_ih,_x1,_x2,_y1,_y2,_text) \
  IupDrawText((_ih), (_text), -1, (_x1), (_y1), (_x2)-(_x1), (_y2)-(_y1))

#define iupMATRIX_TEXT(_ih,_x,_y,_text) \
  IupDrawText((_ih), (_text), -1, (_x), (_y), 0, 0)

#ifdef __cplusplus
}
#endif

#endif
