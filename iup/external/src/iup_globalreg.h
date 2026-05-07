/** \file
 * \brief Registry of known IUP global attributes
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUP_GLOBALREG_H
#define __IUP_GLOBALREG_H

#include "iup.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _iGlobalRegEntry {
  const char* name;
  int flags;     /* IUPGF_* bits */
  int drivers;   /* IUPDRV_* mask */
} iGlobalRegEntry;

IUP_SDK_API int                     iupGlobalRegCount(void);
IUP_SDK_API const iGlobalRegEntry*  iupGlobalRegAt(int index);
IUP_SDK_API const iGlobalRegEntry*  iupGlobalRegFind(const char* name);

#ifdef __cplusplus
}
#endif

#endif
