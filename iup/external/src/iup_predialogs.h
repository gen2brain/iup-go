/** \file
 * \brief IUP Core pre-defined dialogs (not exported API)
 *
 * See Copyright Notice in "iup.h"
 *
 */

#ifndef __IUP_PREDIAL_H
#define __IUP_PREDIAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Other functions declared in <iup.h> and implemented here.
IupListDialog
IupAlarm
IupMessagef
IupGetFile
IupGetText
*/

enum {
  IUP_BUTTON_ORDER_OK_FIRST = 0,    /* primary leftmost: Windows HIG */
  IUP_BUTTON_ORDER_CANCEL_FIRST = 1 /* primary rightmost: macOS / iOS / Linux DEs / Android HIG */
};
int iupDialogButtonOrder(void);

#ifdef __cplusplus
}
#endif

#endif
