/** \file
 * \brief Windows Driver
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUPWIN_DRV_H 
#define __IUPWIN_DRV_H

#ifdef __cplusplus
extern "C" {
#endif


/* global variables */
/* declared where they are initialized */
extern HINSTANCE iupwin_hinstance;      /* iupwin_open.c */
extern int       iupwin_comctl32ver6;   /* iupwin_open.c */
extern HINSTANCE iupwin_dll_hinstance;  /* iupwindows_main.c */
extern DWORD     iupwin_mainthreadid;   /* iupwin_open.c */
#ifdef USE_WINHOOKPOST
extern HHOOK     iupwin_threadmsghook;  /* iupwin_open.c */
#endif

/* open */
IUP_DRV_API void iupwinShowLastError(void);
IUP_DRV_API void iupwinSetInstance(HINSTANCE hInstance);

/* focus */
void iupwinWmSetFocus(Ihandle *ih);
int iupwinGetKeyBoardCues(void);

/* key */
int iupwinKeyEvent(Ihandle* ih, int wincode, int press);
void iupwinButtonKeySetStatus(WORD keys, char* status, int doubleclick);
int iupwinKeyDecode(int wincode);
void iupwinKeyInit(void);

/* tips */
void iupwinTipsGetDispInfo(LPARAM lp);
void iupwinTipsUpdateInfo(Ihandle* ih, HWND tips_hwnd);
void iupwinTipsDestroy(Ihandle* ih);

/* touch */
void iupwinTouchInit(void);
void iupwinTouchRegisterAttrib(Iclass* ic);
void iupwinTouchProcessInput(Ihandle* ih, int count, void* lp);

/* font */
char* iupwinGetHFontAttrib(Ihandle *ih);
HFONT iupwinGetHFont(const char* value);
char* iupwinFindHFont(HFONT hFont);

/* DnD */
int iupwinDragDetectStart(Ihandle* ih);
void iupwinDropFiles(HDROP hDrop, Ihandle *ih);
void iupwinDestroyDragDrop(Ihandle* ih);

/* menu */
void iupwinMenuDialogProc(Ihandle* ih, UINT msg, WPARAM wp, LPARAM lp);
Ihandle* iupwinMenuGetItemHandle(HMENU hmenu, int menuId);
Ihandle* iupwinMenuGetHandle(HMENU hMenu);

/* loop */
void iupwinSetCustomQuitMessage(int enable);


/***************************/
/* Procedures and Messages */
/***************************/

/* Definition of a callback used to return the background brush of controls called "_IUPWIN_CTLCOLOR_CB". */
typedef int (*IFctlColor)(Ihandle* ih, HDC hdc, LRESULT *result);

/* Definition of a callback used to draw custom controls called "_IUPWIN_DRAWITEM_CB". 
  drawitem is a pointer to a DRAWITEMSTRUCT struct. */
typedef void (*IFdrawItem)(Ihandle* ih, void* drawitem);

/* Definition of a callback used to notify custom controls called "_IUPWIN_NOTIFY_CB". 
  msg_info is a pointer to a NMHDR struct. */
typedef int (*IFnotify)(Ihandle* ih, void* msg_info, int *result);

/* Definition of a callback used to process WM_COMMAND messages called "_IUPWIN_COMMAND_CB". */
typedef int (*IFwmCommand)(Ihandle* ih, WPARAM wp, LPARAM lp);

/* Definition of callback used for custom Message Processing. Can return 0 or 1.
   0 = do default processing. 
   1 = ABORT default processing and the result value should be returned.
   NOT the same as a WndProc.
*/
typedef int (*IwinMsgProc)(Ihandle* ih, UINT msg, WPARAM wp, LPARAM lp, LRESULT *result);

/* Base IwinMsgProc callback used by native controls. */
IUP_DRV_API int iupwinBaseMsgProc(Ihandle* ih, UINT msg, WPARAM wp, LPARAM lp, LRESULT *result);

/* Base IwinMsgProc callback used by native containers. 
   Handle messages that are sent to the parent Window.  */
int iupwinBaseContainerMsgProc(Ihandle* ih, UINT msg, WPARAM wp, LPARAM lp, LRESULT *result);

/* Base WinProc used by all native elements. Configure base message handling 
   and custom IwinMsgProc using "_IUPWIN_CTRLMSGPROC_CB" callback. */
LRESULT CALLBACK iupwinBaseWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

void iupwinChangeWndProc(Ihandle *ih, WNDPROC newProc);

IUP_DRV_API int iupwinButtonUp(Ihandle* ih, UINT msg, WPARAM wp, LPARAM lp);
IUP_DRV_API int iupwinButtonDown(Ihandle* ih, UINT msg, WPARAM wp, LPARAM lp);
IUP_DRV_API int iupwinMouseMove(Ihandle* ih, UINT msg, WPARAM wp, LPARAM lp);
void iupwinTrackMouseLeave(Ihandle* ih);
void iupwinRefreshCursor(Ihandle* ih);
IUP_DRV_API void iupwinFlagButtonDown(Ihandle* ih, UINT msg);
IUP_DRV_API int iupwinFlagButtonUp(Ihandle* ih, UINT msg);

int iupwinListDND(Ihandle *ih, UINT uNotification, POINT pt);

#ifdef USE_WINHOOKPOST
LRESULT CALLBACK iupwinPostMessageFilterProc(int code, WPARAM wParam, LPARAM lParam);
#endif


/*********************/
/* Window Management */
/*********************/

HWND iupwinCreateWindowEx(HWND hParent, LPCTSTR lpClassName, DWORD dwExStyle, DWORD dwStyle, int serial, void* clientdata);

/* Creates the Window with native parent and child ID, associate HWND with Ihandle*, 
   and replace the WinProc by iupwinBaseWndProc */
IUP_DRV_API int iupwinCreateWindow(Ihandle* ih, LPCTSTR lpClassName, DWORD dwExStyle, DWORD dwStyle, void* clientdata);

void iupwinGetNativeParentStyle(Ihandle* ih, DWORD *dwExStyle, DWORD *dwStyle);
void iupwinMergeStyle(Ihandle* ih, DWORD old_mask, DWORD value);
void iupwinSetStyle(Ihandle* ih, DWORD value, int set);

int iupwinClassExist(const TCHAR* name);


/*********************/
/*      Utilities    */
/*********************/


int iupwinSetTitleAttrib(Ihandle* ih, const char* value);
TCHAR* iupwinGetWindowText(HWND hWnd);

HCURSOR iupwinGetCursor(Ihandle* ih, const char* name);

int iupwinGetColorRef(Ihandle *ih, char *name, COLORREF *color);
int iupwinGetParentBgColor(Ihandle* ih, COLORREF* cr);

int iupwinSetAutoRedrawAttrib(Ihandle* ih, const char* value);
void iupwinSetMnemonicTitle(Ihandle *ih, int pos, const char* value);

void iupwinDrawFocusRect(HDC hDC, int x, int y, int w, int h);

int iupwinGetScreenRes(void);
/* 1 point = 1/72 inch */
/* pixel = (point/72)*(pixel/inch) */
#define iupWIN_PT2PIXEL(_pt, _res)    MulDiv(_pt, _res, 72)     /* (((_pt)*(_res))/72)    */
#define iupWIN_PIXEL2PT(_pixel, _res) MulDiv(_pixel, 72, _res)  /* (((_pixel)*72)/(_res)) */

/* child window identifier of the first MDI child window created,
   should not conflict with any other command identifiers. */
#define IUP_MDI_FIRSTCHILD 100000000

#ifndef GET_X_LPARAM
/* Do not use the LOWORD or HIWORD for coordinates, because of
   incorrect results on systems with multiple monitors */
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif


#ifdef __cplusplus
}
#endif

#endif
