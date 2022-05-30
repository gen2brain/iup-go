/** \file
 * \brief Driver Information Functions
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUP_DRVINFO_H 
#define __IUP_DRVINFO_H

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup drvinfo Driver Information Interface 
 * \par
 * Each driver must export the symbols defined here. 
 * But in this case the functions are shared by different drivers in the same system.
 * \par
 * For example, the GTK driver and the Windows driver share the same implementation 
 * of these functions when the GTK driver is compiled in Windows. 
 * The GTK driver and the Motif driver share the same implementation 
 * of these functions when the GTK driver is compiled in UNIX. 
 * \par
 * See \ref iup_drvinfo.h 
 * \ingroup drv */


/** Retrieve the main desktop full size (size of the main monitor).
 * \ingroup drvinfo */
IUP_SDK_API void iupdrvGetFullSize(int *width, int *height);

/** Retrieve the main desktop available size (full size less taskbar/menubar space).
 * \ingroup drvinfo */
IUP_SDK_API void iupdrvGetScreenSize(int *width, int *height);

/** Adds the main desktop offset because of a taskbar/menubar positioning.
 * Only useful in Windows. In X-Windows the position of the origin 0,0 is already adjusted
 * to be after the taskbar/menubar.
 * \ingroup drvinfo */
IUP_SDK_API void iupdrvAddScreenOffset(int *x, int *y, int add);

/** Retrieve the default desktop bits per pixel.
 * \ingroup drvinfo */
IUP_SDK_API int iupdrvGetScreenDepth(void);

/** Retrieve the default desktop resolution in dpi (dots or pixels per inch).
 * \ingroup drvinfo */
IUP_SDK_API double iupdrvGetScreenDpi(void);

/** Returns a string with the system version number.
 * \ingroup drvinfo */
IUP_SDK_API char *iupdrvGetSystemVersion(void);

/** Returns a string with the system name.
 * \ingroup drvinfo */
IUP_SDK_API char* iupdrvGetSystemName(void);

/** Returns a string with the computer name.
 * \ingroup drvinfo */
IUP_SDK_API char* iupdrvGetComputerName(void);

/** Returns a string with the user name.
 * \ingroup drvinfo */
IUP_SDK_API char* iupdrvGetUserName(void);

/** Returns the key state for Shift, Ctrl, Alt and sYs, in this order.
 * Left and right keys are considered.
 * Should declare "char key[5]".
 * Values could be space (" ") or "SCAY".
 * \ingroup drvinfo */
IUP_SDK_API void iupdrvGetKeyState(char* key);

/** Returns the current position of the mouse cursor.
 * \ingroup drvinfo */
IUP_SDK_API void iupdrvGetCursorPos(int *x, int *y);

/** Returns the driver "Display" in UNIX and NULL in Windows.
 * Must be implemented somewhere else.
 * \ingroup drvinfo */
IUP_SDK_API void* iupdrvGetDisplay(void);

/** Returns the current locale name.
 * \ingroup drvinfo */
IUP_SDK_API char* iupdrvLocaleInfo(void);

/** Returns the path to the preference directory.
 * Path will always have a trailing slash.
 * Return 1 on success, 0 on failure.
 * \ingroup drvinfo */
IUP_SDK_API int iupdrvGetPreferencePath(char *filename, int use_system);

/** Changes the current directory.
* \ingroup drvinfo */
IUP_SDK_API int iupdrvSetCurrentDirectory(const char* dir);

/** Returns the current directory.
* \ingroup drvinfo */
IUP_SDK_API char* iupdrvGetCurrentDirectory(void);


#ifdef __cplusplus
}
#endif

#endif
