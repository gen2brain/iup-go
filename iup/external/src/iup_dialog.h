/** \file
 * \brief Dialog (not exported API)
 *
 * See Copyright Notice in "iup.h"
 *
 */

#ifndef __IUP_DIALOG_H
#define __IUP_DIALOG_H

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup drvdialog Driver Dialog Interface
 * \ingroup drv */


/* PUBLIC */

/* Shows the dialog in the given position and disable interaction with other dialogs.
 * The element must be already mapped.
 * Must return IUP_ERROR or IUP_NOERROR.
 * Called only from IupPopup. */
int iupDialogPopup(Ihandle* ih, int x, int y);

/* Shows the dialog in the given position.
 * The dialog must be already mapped.
 * Must return IUP_ERROR or IUP_NOERROR.
 * Called only from IupShow and IupShowXY. */
int iupDialogShowXY(Ihandle* ih, int x, int y);

/* Hides the dialog.
 * Called only from IupHide. */
void iupDialogHide(Ihandle* ih);

/* Returns a unique number to be as child id. */
int iupDialogGetChildId(Ihandle* ih);
char* iupDialogGetChildIdStr(Ihandle* ih);

/** \addtogroup drvdialog
 * @{ */
/** Returns the size of the decoration. */
IUP_SDK_API void iupdrvDialogGetDecoration(Ihandle* ih, int *border, int *caption, int *menu);
/** Changes the parent dialog (works only if already set at map). */
IUP_SDK_API void iupdrvDialogSetParent(Ihandle* ih, InativeHandle* native_parent);
/** @} */

IUP_SDK_API InativeHandle* iupDialogGetNativeParent(Ihandle* ih);

/* Updates the dialog initial position from internal attributes.
   Used mostly by the native pre-defined dialogs. */
void iupDialogUpdatePosition(Ihandle* ih);

/* For external modal dialog stack control */
void iupDialogLeaveModal(int popup_level);
void iupDialogEnterModal(Ihandle* ih_popup, int popup_level);


/*********************************************************************/
                        /* PRIVATE */
/*********************************************************************/

#ifdef _IUPDLG_PRIVATE

/* retrieve the decorations size that offsets the window size of the client size. */
void iupDialogGetDecorSize(Ihandle* ih, int *decorwidth, int *decorheight);

void iupDialogCustomFrameSimulateCheckCallbacks(Ihandle* ih);
int  iupDialogCustomFrameRestore(Ihandle* ih);
void iupDialogCustomFrameMaximize(Ihandle* ih);

struct _IcontrolData 
{
  int show_state,     /* save the state to be used used in SHOW_CB */
    first_show,     /* boolean flag to indicate that the dialog was shown for the first time */
    ignore_resize,  /* flag to ignore the next resize */
    popup_level,    /* popup level of the dialog if IupPopup used */
    child_id,       /* serial number used by child controls */
    cmd_show;       /* parameters for ShowWindow in Windows driver */
  Ihandle* menu;
};


/******************************/
/* Driver dependent functions */
/******************************/

/** \addtogroup drvdialog
 * @{ */
/** Registers platform-specific Dialog attributes and methods. */
IUP_SDK_API void iupdrvDialogInitClass(Iclass* iclass);
/** Gets current dialog position (screen coordinates). */
IUP_SDK_API void iupdrvDialogGetPosition(Ihandle* ih, InativeHandle* handle, int *x, int *y);
/** Shows or hides dialog. */
IUP_SDK_API void iupdrvDialogSetVisible(Ihandle* ih, int visible);
/** Sets dialog placement (MINIMIZED, MAXIMIZED, FULLSCREEN, NORMAL). */
IUP_SDK_API int iupdrvDialogSetPlacement(Ihandle* ih);
/** Sets dialog position (screen coordinates). */
IUP_SDK_API void iupdrvDialogSetPosition(Ihandle *ih, int x, int y);
/** Gets current dialog size (including decorations). */
IUP_SDK_API void iupdrvDialogGetSize(Ihandle* ih, InativeHandle* handle, int *w, int *h);
/** Returns 1 if dialog is visible. */
IUP_SDK_API int iupdrvDialogIsVisible(Ihandle* ih);
/** @} */
int iupDialogSetClientSizeAttrib(Ihandle* ih, const char* value);
char* iupDialogGetClientSizeAttrib(Ihandle *ih);


#endif

#ifdef __cplusplus
}
#endif

#endif
