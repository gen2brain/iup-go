/** \file
 * \brief Popover Control
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUP_POPOVER_H
#define __IUP_POPOVER_H

#ifdef __cplusplus
extern "C" {
#endif


/* Popover position constants */
#define IUP_POPOVER_BOTTOM        0
#define IUP_POPOVER_TOP           1
#define IUP_POPOVER_LEFT          2
#define IUP_POPOVER_RIGHT         3
#define IUP_POPOVER_BOTTOMLEFT    4
#define IUP_POPOVER_BOTTOMRIGHT   5
#define IUP_POPOVER_TOPLEFT       6
#define IUP_POPOVER_TOPRIGHT      7
#define IUP_POPOVER_LEFTBOTTOM    8
#define IUP_POPOVER_LEFTTOP       9
#define IUP_POPOVER_RIGHTBOTTOM  10
#define IUP_POPOVER_RIGHTTOP     11


/** \addtogroup drv
 * @{ */
IUP_SDK_API void iupdrvPopoverInitClass(Iclass* ic);

/** Returns the popover position enum from the POSITION attribute. */
IUP_SDK_API int iupPopoverGetPosition(Ihandle* ih);

/** Calculates screen position for a popover given anchor and popover geometry.
 *  Reads POSITION, OFFSETX, OFFSETY from ih.
 *  \param ih        the popover Ihandle
 *  \param ax,ay     anchor top-left in screen coordinates
 *  \param aw,ah     anchor size
 *  \param pw,ph     popover size
 *  \param x,y       [out] computed popover screen position */
IUP_SDK_API void iupPopoverCalcPosition(Ihandle* ih, int ax, int ay, int aw, int ah, int pw, int ph, int* x, int* y);
/** @} */


#ifdef __cplusplus
}
#endif

#endif
