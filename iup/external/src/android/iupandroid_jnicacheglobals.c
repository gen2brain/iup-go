/** \file
 * \brief Cross-file cached JNI refs (definitions).
 *
 * See iupandroid_jnicacheglobals.h for the rationale.
 *
 * See Copyright Notice in "iup.h"
 */

#include "iupandroid_jnimacros.h"


IUPJNI_DECLARE_CLASS_GLOBAL(IupCommon);

IUPJNI_DECLARE_METHOD_ID_GLOBAL(IupCommon_addWidgetToParent);
IUPJNI_DECLARE_METHOD_ID_GLOBAL(IupCommon_removeWidgetFromParent);
IUPJNI_DECLARE_METHOD_ID_GLOBAL(IupCommon_setWidgetPosition);
IUPJNI_DECLARE_METHOD_ID_GLOBAL(IupCommon_isActive);
IUPJNI_DECLARE_METHOD_ID_GLOBAL(IupCommon_setActive);
