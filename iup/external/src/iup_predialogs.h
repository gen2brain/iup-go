/** \file
 * \brief IUP Core pre-defined dialogs (not exported API)
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUP_PREDIAL_H 
#define __IUP_PREDIAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Used by the IupScanf implementations in C and Lua */
IUP_SDK_API int iupDataEntry(int maxlin, int* maxcol, int* maxscr, char* title, char** text, char** data);

/* Other functions declared in <iup.h> and implemented here. 
IupListDialog
IupAlarm
IupMessagef
IupGetFile
IupGetText
*/

#ifdef __cplusplus
}
#endif

#endif
