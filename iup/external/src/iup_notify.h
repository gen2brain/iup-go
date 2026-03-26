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

/** \addtogroup drv
 * @{ */
IUP_SDK_API void iupdrvNotifyInitClass(Iclass* ic);
IUP_SDK_API int iupdrvNotifyShow(Ihandle* ih);
IUP_SDK_API int iupdrvNotifyClose(Ihandle* ih);
IUP_SDK_API void iupdrvNotifyDestroy(Ihandle* ih);
IUP_SDK_API int iupdrvNotifyIsAvailable(void);
/** @} */

#ifdef __cplusplus
}
#endif

#endif
