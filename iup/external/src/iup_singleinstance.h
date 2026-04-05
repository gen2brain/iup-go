/** \file
 * \brief Single Instance Support
 *
 * See Copyright Notice in "iup.h"
 *
 */

#ifndef __IUP_SINGLEINSTANCE_H
#define __IUP_SINGLEINSTANCE_H

#include "iup_export.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \addtogroup drv
 * @{ */
/** Sets up single instance detection.
 * Returns 1 if another instance already exists (caller should exit),
 * returns 0 if this is the first instance. */
IUP_SDK_API int iupdrvSingleInstanceSet(const char* name);
/** Cleans up single instance resources. */
IUP_SDK_API void iupdrvSingleInstanceClose(void);
/** @} */

#ifdef __cplusplus
}
#endif

#endif
