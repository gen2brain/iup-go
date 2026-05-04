/** \file
 * \brief Cross-file cached JNI refs.
 *
 * Classes and method IDs here are shared between translation units to avoid
 * duplicate caches. Per-file-only IDs should stay with IUPJNI_DECLARE_*_STATIC
 * in their owning file.
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUP_ANDROID_JNI_CACHEGLOBALS_H
#define __IUP_ANDROID_JNI_CACHEGLOBALS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "iupandroid_jnimacros.h"


IUPJNI_DECLARE_CLASS_EXTERN(IupCommon);

IUPJNI_DECLARE_METHOD_ID_EXTERN(IupCommon_addWidgetToParent);
IUPJNI_DECLARE_METHOD_ID_EXTERN(IupCommon_removeWidgetFromParent);
IUPJNI_DECLARE_METHOD_ID_EXTERN(IupCommon_setWidgetPosition);
IUPJNI_DECLARE_METHOD_ID_EXTERN(IupCommon_isActive);
IUPJNI_DECLARE_METHOD_ID_EXTERN(IupCommon_setActive);


#ifdef __cplusplus
}
#endif

#endif
