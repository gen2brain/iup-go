/** \file
 * \brief Windows System Information
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUPWIN_INFO_H 
#define __IUPWIN_INFO_H

#ifdef __cplusplus
extern "C" {
#endif

/* system */
IUP_DRV_API int   iupwinGetComCtl32Version(void);
IUP_DRV_API char* iupwinGetSystemLanguage(void);
IUP_DRV_API int   iupwinCheckWindowsVersion(DWORD major, DWORD minor);
IUP_DRV_API DWORD iupwinGetBuildNumber(void);
IUP_DRV_API int   iupwinIsAppThemed(void);
IUP_DRV_API int   iupwinIsSystemDarkMode(void);
IUP_DRV_API int   iupwinIsVistaOrNew(void);
IUP_DRV_API int   iupwinIsWinXPOrNew(void);
IUP_DRV_API int   iupwinIsWin7OrNew(void);
IUP_DRV_API int   iupwinIsWin10OrNew(void);


#ifdef __cplusplus
}
#endif

#endif
