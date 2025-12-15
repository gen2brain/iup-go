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


/** \defgroup popover IupPopover
 * \par
 * See \ref iup_popover.h
 * \ingroup ctrl */


/** Creates a popover container that displays content anchored to a widget.
 *
 * \ingroup popover */
IUP_API Ihandle* IupPopover(Ihandle* child);


/* Driver-specific functions that must be implemented by each platform */

void iupdrvPopoverInitClass(Iclass* ic);


#ifdef __cplusplus
}
#endif

#endif
