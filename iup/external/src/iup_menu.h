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

int iupdrvMenuPopup(Ihandle* ih, int x, int y);
void iupdrvSeparatorInitClass(Iclass* ic);
void iupdrvItemInitClass(Iclass* ic);
void iupdrvMenuInitClass(Iclass* ic);
void iupdrvSubmenuInitClass(Iclass* ic);

char* iupMenuProcessTitle(Ihandle* ih, const char* title);
int iupMenuGetChildId(Ihandle* ih);
char* iupMenuGetChildIdStr(Ihandle* ih);
int iupMenuIsMenuBar(Ihandle* ih);

#ifdef __cplusplus
}
#endif

#endif
