/** \file
 * \brief Driver Interface. Each driver must export the symbols defined here.
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUP_DRV_H 
#define __IUP_DRV_H

#ifdef __cplusplus
extern "C" {
#endif

  
/** \defgroup drv Driver Interface 
 * \par
 * Each driver must export the symbols defined here.
 * \par
 * See \ref iup_drv.h 
 */


/** Sets a global environment attribute. Called from IupSetGlobal and IupStoreGlobal.
 * Must return 1 is process the attribute, or 0 is not. 
 * \ingroup drv */
IUP_SDK_API int iupdrvSetGlobal(const char* name, const char* value);

/** Returns a global environment attribute. Called from IupGetGlobal.
 * \ingroup drv */
IUP_SDK_API char* iupdrvGetGlobal(const char* name);

/** Changes the idle callback. Called from IupSetFunction.
 * \ingroup drv */
IUP_SDK_API void iupdrvSetIdleFunction(Icallback func);

/** Convert the coordinates from screen relative to client area.
 * \ingroup drv */
IUP_SDK_API void iupdrvScreenToClient(Ihandle* ih, int *x, int *y);

/** Convert the coordinates from relative client area to screen.
 * \ingroup drv */
IUP_SDK_API void iupdrvClientToScreen(Ihandle* ih, int *x, int *y);

/** Returns true if the element is visible.
 * \ingroup drv */
IUP_SDK_API int iupdrvIsVisible(Ihandle* ih);

/** Returns true if the element is active.
 * \ingroup drv */
IUP_SDK_API int iupdrvIsActive(Ihandle* ih);

/** Actually changes the focus to the given element.
 * \ingroup drv */
IUP_SDK_API void iupdrvSetFocus(Ihandle* ih);

/** Changes the visible state of an element. 
 * Not used for dialogs. 
 * \ingroup drv */
IUP_SDK_API void iupdrvSetVisible(Ihandle* ih, int enable);

/** Changes the active state of an element. 
 * \ingroup drv */
IUP_SDK_API void iupdrvSetActive(Ihandle* ih, int enable);

/** Post a redraw of a control and children.
 * \ingroup drv */
IUP_SDK_API void iupdrvPostRedraw(Ihandle *ih);

/** Force a redraw of a control and children.
 * \ingroup drv */
IUP_SDK_API void iupdrvRedrawNow(Ihandle *ih);

/** Reparent the native control.
 * \ingroup drv */
IUP_SDK_API void iupdrvReparent(Ihandle* ih);

/** Size of the scrollbar.
 * \ingroup drv */
IUP_SDK_API int iupdrvGetScrollbarSize(void);

/** Activates a button or toggle.
 * \ingroup drv */
IUP_SDK_API void iupdrvActivate(Ihandle* ih);

/** Returns the height of a menu bar.
 * \ingroup drv */
IUP_SDK_API int iupdrvMenuGetMenuBarSize(Ihandle* ih);

/** Sends a global keyboard message.
 * \ingroup drv */
IUP_SDK_API void iupdrvSendKey(int key, int press);

/** Sends a global mouse message.
 * status: 2=double pressed, 1=pressed, 0=released, -1=move
 * \ingroup drv */
IUP_SDK_API void iupdrvSendMouse(int x, int y, int bt, int status);

/** Moves the cursor on screen.
 * \ingroup drv */
IUP_SDK_API void iupdrvWarpPointer(int x, int y);

/** Translates an IUP key definition into a system definition.
 * \ingroup drv */
IUP_SDK_API void iupdrvKeyEncode(int key, unsigned int *keyval, unsigned int *state);

/** Suspends execution for the specified number of milliseconds.
 * \ingroup drv */
IUP_SDK_API void iupdrvSleep(int time);

/** Sets the accessibility text for screen readers.
 * \ingroup drv */
IUP_SDK_API void iupdrvSetAccessibleTitle(Ihandle *ih, const char* title);


/* Called only from IupOpen/IupClose. */
int iupdrvOpen(int *argc, char ***argv);
void iupdrvClose(void);


#ifdef __cplusplus
}
#endif

#endif
