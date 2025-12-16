/** \file
 * \brief Tray Control
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUP_TRAY_H
#define __IUP_TRAY_H

#ifdef __cplusplus
extern "C" {
#endif

void iupdrvTrayInitClass(Iclass* ic);
int iupdrvTraySetVisible(Ihandle* ih, int visible);
int iupdrvTraySetImage(Ihandle* ih, const char* value);
int iupdrvTraySetTip(Ihandle* ih, const char* value);
int iupdrvTraySetMenu(Ihandle* ih, Ihandle* menu);
void iupdrvTrayDestroy(Ihandle* ih);
int iupdrvTrayIsAvailable(void);

#ifdef __cplusplus
}
#endif

#endif
