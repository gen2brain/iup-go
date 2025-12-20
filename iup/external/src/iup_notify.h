/** \file
 * \brief Notify Control - Desktop Notifications
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUP_NOTIFY_H
#define __IUP_NOTIFY_H

#ifdef __cplusplus
extern "C" {
#endif

void iupdrvNotifyInitClass(Iclass* ic);
int iupdrvNotifyShow(Ihandle* ih);
int iupdrvNotifyClose(Ihandle* ih);
void iupdrvNotifyDestroy(Ihandle* ih);
int iupdrvNotifyIsAvailable(void);

#ifdef __cplusplus
}
#endif

#endif
