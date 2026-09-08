/** \file
 * \brief global attributes environment (not exported API)
 *
 * See Copyright Notice in "iup.h"
 *
 */

#ifndef __IUP_GLOBALATTRIB_H
#define __IUP_GLOBALATTRIB_H

#ifdef __cplusplus
extern "C" {
#endif

/* called only in IupOpen and IupClose */
void iupGlobalAttribInit(void);
void iupGlobalAttribFinish(void);

IUP_SDK_API int iupGlobalIsPointer(const char* name);

/* The APPEARANCE global, one of IUP_APPEARANCE_*. */
IUP_SDK_API int iupGlobalGetAppearance(void);

IUP_SDK_API int iupGlobalIsDarkMode(void);

/* Seeds the palette for a forced appearance, for drivers with no native theme switch. */
IUP_SDK_API void iupGlobalSetAppearanceColors(int dark);

/* Re-applies the global palette to every mapped element that did not set its own color. */
IUP_SDK_API void iupGlobalUpdateThemeColors(void);

/* Calls THEMECHANGED_CB on every dialog, unless the palette and the dark mode are unchanged. */
IUP_SDK_API void iupGlobalNotifyThemeChanged(void);

/* Records the current palette and dark mode as the baseline. */
IUP_SDK_API void iupGlobalSeedThemeState(void);

int iupGlobalDefaultColorChanged(const char *name);    /* check if user changed */
void iupGlobalSetDefaultColorAttrib(const char* name, int r, int g, int b);  /* internal change method */

int iupGetGlobalAttributes(char** names, int n);

/* Other functions declared in <iup.h> and implemented here.
IupSetGlobal
IupStoreGlobal
IupGetGlobal
*/

#ifdef __cplusplus
}
#endif

#endif
