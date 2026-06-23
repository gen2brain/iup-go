/** \file
 * \brief Windows Dark Mode support (undocumented uxtheme API)
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUPWIN_DARKMODE_H
#define __IUPWIN_DARKMODE_H

#ifdef __cplusplus
extern "C" {
#endif

IUP_DRV_API void iupwinDarkModeInit(void);     /* from iupdrvOpen, before iupwinSetGlobalColors */
IUP_DRV_API void iupwinDarkModeSetOptIn(int enable);  /* AUTODARKMODE global; off = classic light */
IUP_DRV_API int  iupwinDarkModeOptIn(void);
IUP_DRV_API int  iupwinDarkModeEnabled(void);  /* opt-in on, system dark, high contrast off */
IUP_DRV_API void iupwinDarkModeRefresh(void);  /* on ImmersiveColorSet */
IUP_DRV_API void iupwinDarkModeApplyToWindow(HWND hwnd);  /* per control, after creation */
IUP_DRV_API void iupwinDarkModeApplyToTree(HWND root);    /* on a live switch */
IUP_DRV_API void iupwinDarkModeSetNoHover(HWND hwnd);     /* grid list views (IupTable) */
IUP_DRV_API void iupwinDarkModeApplyTaskDialog(HWND root);  /* from TaskDialog TDN_CREATED */
IUP_DRV_API void iupwinDarkModeHookTaskDialog(int enable);  /* wrap TaskDialogIndirect */
IUP_DRV_API int  iupwinDarkModeMenuBarProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, LRESULT* result);

#ifdef __cplusplus
}
#endif

#endif
