/** \file
 * \brief IUP Menu Class (not exported API)
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUP_MENU_H 
#define __IUP_MENU_H

#ifdef __cplusplus
extern "C" {
#endif

/* Shows a popup menu in the given position.
* Must return IUP_ERROR or IUP_NOERROR.
* Called only from IupPopup.
*/
int iupMenuPopup(Ihandle* ih, int x, int y);

/** \addtogroup drv
 * @{ */
IUP_SDK_API int iupdrvMenuPopup(Ihandle* ih, int x, int y);
IUP_SDK_API void iupdrvSeparatorInitClass(Iclass* ic);
IUP_SDK_API void iupdrvItemInitClass(Iclass* ic);
IUP_SDK_API void iupdrvMenuInitClass(Iclass* ic);
IUP_SDK_API void iupdrvSubmenuInitClass(Iclass* ic);
/** @} */

char* iupMenuProcessTitle(Ihandle* ih, const char* title);
int iupMenuGetChildId(Ihandle* ih);
char* iupMenuGetChildIdStr(Ihandle* ih);
int iupMenuIsMenuBar(Ihandle* ih);

#ifdef __cplusplus
}
#endif

#endif
