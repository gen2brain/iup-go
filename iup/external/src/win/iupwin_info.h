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
int   iupwinGetComCtl32Version(void);
char* iupwinGetSystemLanguage(void);
int   iupwinIsAppThemed(void);
int   iupwinIsVistaOrNew(void);
int   iupwinIsWinXPOrNew(void);
int   iupwinIsWin7OrNew(void);
int   iupwinIsWin8OrNew(void);
int   iupwinIsWin10OrNew(void);


#ifdef __cplusplus
}
#endif

#endif
