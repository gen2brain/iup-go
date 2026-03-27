/** \file
 * \brief Motif Driver 
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUPMOT_DRV_H 
#define __IUPMOT_DRV_H

#ifdef __cplusplus
extern "C" {
#endif


/* global variables, declared in iupmot_open.c */
extern Widget         iupmot_appshell;         
extern Display*       iupmot_display;          
extern int            iupmot_screen;           
extern XtAppContext   iupmot_appcontext;       
extern Visual*        iupmot_visual;
extern Atom           iupmot_wm_deletewindow;

/* dialog */
IUP_DRV_API void iupmotDialogSetVisual(Ihandle* ih, void* visual);
IUP_DRV_API void iupmotDialogResetVisual(Ihandle* ih);

/* focus */
IUP_DRV_API void iupmotFocusChangeEvent(Widget w, Ihandle *ih, XEvent *evt, Boolean *cont);

/* key */
IUP_DRV_API void iupmotCanvasKeyReleaseEvent(Widget w, Ihandle *ih, XEvent *evt, Boolean *cont);
IUP_DRV_API void iupmotKeyPressEvent(Widget w, Ihandle *ih, XEvent *evt, Boolean *cont);
IUP_DRV_API KeySym iupmotKeyCharToKeySym(char c);
IUP_DRV_API void iupmotButtonKeySetStatus(unsigned int state, unsigned int but, char* status, int doubleclick);
IUP_DRV_API int iupmotKeyDecode(XKeyEvent *evt);
IUP_DRV_API KeySym iupmotKeycodeToKeysym(XKeyEvent *evt);

/* font */
IUP_DRV_API char* iupmotGetFontListAttrib(Ihandle *ih);
IUP_DRV_API XmFontList iupmotGetFontList(const char* foundry, const char* value);
IUP_DRV_API XFontStruct* iupmotGetFontStruct(const char* value);
IUP_DRV_API char* iupmotFindFontList(XmFontList fontlist);
IUP_DRV_API char* iupmotGetFontStructAttrib(Ihandle *ih);
IUP_DRV_API char* iupmotGetFontIdAttrib(Ihandle *ih);
#ifdef IUP_USE_XFT
IUP_DRV_API void* iupmotGetXftFontAttrib(Ihandle *ih);
IUP_DRV_API void* iupmotGetXftFont(const char* value);
#endif

/* tips */
/* called from Enter/Leave events to check if a TIP is present. */
IUP_DRV_API void iupmotTipEnterNotify(Ihandle* ih);
IUP_DRV_API void iupmotTipLeaveNotify(void);
IUP_DRV_API void iupmotTipsFinish(void);

/* str */
IUP_DRV_API void iupmotSetXmString(Widget w, const char *resource, const char* value);
IUP_DRV_API char* iupmotGetXmString(XmString str);
IUP_DRV_API char* iupmotReturnXmString(XmString str);
IUP_DRV_API void iupmotSetMnemonicTitle(Ihandle *ih, Widget w, int pos, const char* value);
IUP_DRV_API void iupmotTextSetString(Widget w, const char *value);
IUP_DRV_API XmString iupmotStringCreate(const char *value);
IUP_DRV_API void iupmotSetTitle(Widget w, const char *value);
IUP_DRV_API void iupmotStrSetUTF8Mode(int utf8mode);
IUP_DRV_API void iupmotStrSetUTF8ModeFile(int utf8mode);
IUP_DRV_API int iupmotStrGetUTF8Mode(void);
IUP_DRV_API int iupmotStrGetUTF8ModeFile(void);
IUP_DRV_API char* iupmotStrConvertToSystem(const char* str);
IUP_DRV_API char* iupmotStrConvertFromSystem(const char* str);
IUP_DRV_API char* iupmotStrConvertToFilename(const char* str);
IUP_DRV_API char* iupmotStrConvertFromFilename(const char* str);
IUP_DRV_API void iupmotStrRelease(void);

/* common */
IUP_DRV_API void iupmotPointerMotionEvent(Widget w, Ihandle *ih, XEvent *evt, Boolean *cont);
IUP_DRV_API void iupmotDummyPointerMotionEvent(Widget w, XtPointer *data, XEvent *evt, Boolean *cont);
IUP_DRV_API void iupmotButtonPressReleaseEvent(Widget w, Ihandle* ih, XEvent* evt, Boolean* cont);
IUP_DRV_API void iupmotEnterLeaveWindowEvent(Widget w, Ihandle *ih, XEvent *evt, Boolean *cont);
IUP_DRV_API void iupmotHelpCallback(Widget w, Ihandle *ih, XtPointer call_data);
IUP_DRV_API void iupmotDisableDragSource(Widget w);
IUP_DRV_API void iupmotSetPixmap(Ihandle* ih, const char* name, const char* prop, int make_inactive);
IUP_DRV_API void iupmotSetGlobalColorAttrib(Widget w, const char* xmname, const char* name);
IUP_DRV_API void iupmotSetBgColor(Widget w, Pixel color);
IUP_DRV_API char* iupmotGetBgColorAttrib(Ihandle* ih);
IUP_DRV_API void iupmotScrolledWindowWheelEvent(Widget w, Ihandle *ih, XEvent *evt, Boolean *cont);

/* image */
IUP_DRV_API Pixmap iupmotImageGetMask(const char* name);

IUP_DRV_API void iupmotSetPosition(Widget widget, int x, int y);
IUP_DRV_API void iupmotGetWindowSize(Ihandle *ih, int *width, int *height);

IUP_DRV_API char* iupmotGetXWindowAttrib(Ihandle *ih);

#define iupMOT_SETARG(_a, _i, _n, _d) ((_a)[(_i)].name = (_n), (_a)[(_i)].value = (XtArgVal)(_d), (_i)++)

/* dark mode */
IUP_DRV_API int iupmotIsSystemDarkMode(void);


#ifdef __cplusplus
}
#endif

#endif
