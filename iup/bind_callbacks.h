/*
 * bind_callbacks.h
 *
 * This file contains all extern and static declarations for CGO callbacks.
 */

#ifndef BIND_CALLBACKS_H
#define BIND_CALLBACKS_H

#include <iup.h>

#include <stdlib.h>
#include "iup.h"

#if defined(_WIN32)
#define CGO_EXPORT __declspec(dllexport)
#else
#define CGO_EXPORT
#endif

CGO_EXPORT extern int goIupIdleCB();
static void goIupSetIdleFunc() {
	IupSetFunction("IDLE_ACTION", (Icallback) goIupIdleCB);
}

CGO_EXPORT extern void goIupEntryPointCB();
static void goIupSetEntryPointFunc() {
	IupSetFunction("ENTRY_POINT", (Icallback) goIupEntryPointCB);
}

CGO_EXPORT extern void goIupExitCB();
static void goIupSetExitFunc() {
	IupSetFunction("EXIT_CB", (Icallback) goIupExitCB);
}

CGO_EXPORT extern int goIupMapCB(void *);
static void goIupSetMapFunc(Ihandle *ih) {
	IupSetCallback(ih, "MAP_CB", (Icallback) goIupMapCB);
}

CGO_EXPORT extern int goIupUnmapCB(void *);
static void goIupSetUnmapFunc(Ihandle *ih) {
	IupSetCallback(ih, "UNMAP_CB", (Icallback) goIupUnmapCB);
}

CGO_EXPORT extern int goIupDestroyCB(void *);
static void goIupSetDestroyFunc(Ihandle *ih) {
	IupSetCallback(ih, "DESTROY_CB", (Icallback) goIupDestroyCB);
}

CGO_EXPORT extern int goIupGetFocusCB(void *);
static void goIupSetGetFocusFunc(Ihandle *ih) {
	IupSetCallback(ih, "GETFOCUS_CB", (Icallback) goIupGetFocusCB);
}

CGO_EXPORT extern int goIupKillFocusCB(void *);
static void goIupSetKillFocusFunc(Ihandle *ih) {
	IupSetCallback(ih, "KILLFOCUS_CB", (Icallback) goIupKillFocusCB);
}

CGO_EXPORT extern int goIupEnterWindowCB(void *);
static void goIupSetEnterWindowFunc(Ihandle *ih) {
	IupSetCallback(ih, "ENTERWINDOW_CB", (Icallback) goIupEnterWindowCB);
}

CGO_EXPORT extern int goIupLeaveWindowCB(void *);
static void goIupSetLeaveWindowFunc(Ihandle *ih) {
	IupSetCallback(ih, "LEAVEWINDOW_CB", (Icallback) goIupLeaveWindowCB);
}

CGO_EXPORT extern int goIupKAnyCB(void *, int c);
static void goIupSetKAnyFunc(Ihandle *ih) {
	IupSetCallback(ih, "K_ANY", (Icallback) goIupKAnyCB);
}

CGO_EXPORT extern int goIupHelpCB(void *);
static void goIupSetHelpFunc(Ihandle *ih) {
	IupSetCallback(ih, "HELP_CB", (Icallback) goIupHelpCB);
}

CGO_EXPORT extern int goIupActionCB(void *);
static void goIupSetActionFunc(Ihandle *ih) {
	IupSetCallback(ih, "ACTION", (Icallback) goIupActionCB);
}

CGO_EXPORT extern int goIupFlatActionCB(void *);
static void goIupSetFlatActionFunc(Ihandle *ih) {
	IupSetCallback(ih, "FLAT_ACTION", (Icallback) goIupFlatActionCB);
}

CGO_EXPORT extern int goIupButtonCB(void *, int button, int pressed, int x, int y, void *status);
static void goIupSetButtonFunc(Ihandle *ih) {
	IupSetCallback(ih, "BUTTON_CB", (Icallback) goIupButtonCB);
}

CGO_EXPORT extern int goIupDropFilesCB(void *, void *filename, int num, int x, int y);
static void goIupSetDropFilesFunc(Ihandle *ih) {
	IupSetCallback(ih, "DROPFILES_CB", (Icallback) goIupDropFilesCB);
}

CGO_EXPORT extern int goIupListActionCB(void *, void *text, int item, int state);
static void goIupSetListActionFunc(Ihandle *ih) {
	IupSetCallback(ih, "ACTION", (Icallback) goIupListActionCB);
}

CGO_EXPORT extern char* goIupListValueCB(void *, int pos);
static void goIupSetListValueFunc(Ihandle *ih) {
	IupSetCallback(ih, "VALUE_CB", (Icallback) goIupListValueCB);
}

CGO_EXPORT extern char* goIupListImageCB(void *, int pos);
static void goIupSetListImageFunc(Ihandle *ih) {
	IupSetCallback(ih, "IMAGE_CB", (Icallback) goIupListImageCB);
}

CGO_EXPORT extern int goIupFlatListActionCB(void *, void *text, int item, int state);
static void goIupSetFlatListActionFunc(Ihandle *ih) {
	IupSetCallback(ih, "FLAT_ACTION", (Icallback) goIupFlatListActionCB);
}

CGO_EXPORT extern int goIupCaretCB(void *, int lin, int col, int pos);
static void goIupSetCaretFunc(Ihandle *ih) {
	IupSetCallback(ih, "CARET_CB", (Icallback) goIupCaretCB);
}

CGO_EXPORT extern int goIupDblclickCB(void *, int item, void *text);
static void goIupSetDblclickFunc(Ihandle *ih) {
	IupSetCallback(ih, "DBLCLICK_CB", (Icallback) goIupDblclickCB);
}

CGO_EXPORT extern int goIupEditCB(void *, int c, void *newValue);
static void goIupSetEditFunc(Ihandle *ih) {
	IupSetCallback(ih, "EDIT_CB", (Icallback) goIupEditCB);
}

CGO_EXPORT extern int goIupMotionCB(void *, int x, int y, void *status);
static void goIupSetMotionFunc(Ihandle *ih) {
	IupSetCallback(ih, "MOTION_CB", (Icallback) goIupMotionCB);
}

CGO_EXPORT extern int goIupMultiselectCB(void *, void *text);
static void goIupSetMultiselectFunc(Ihandle *ih) {
	IupSetCallback(ih, "MULTISELECT_CB", (Icallback) goIupMultiselectCB);
}

CGO_EXPORT extern int goIupValueChangedCB(void *);
static void goIupSetValueChangedFunc(Ihandle *ih) {
	IupSetCallback(ih, "VALUECHANGED_CB", (Icallback) goIupValueChangedCB);
}

CGO_EXPORT extern int goIupTextActionCB(void *ih, int ch, void *newValue);
static void goIupSetTextActionFunc(Ihandle *ih) {
	IupSetCallback(ih, "ACTION", (Icallback) goIupTextActionCB);
}

CGO_EXPORT extern int goIupToggleActionCB(void *, int state);
static void goIupSetToggleActionFunc(Ihandle *ih) {
	IupSetCallback(ih, "ACTION", (Icallback) goIupToggleActionCB);
}

CGO_EXPORT extern int goIupFlatToggleActionCB(void *, int state);
static void goIupSetFlatToggleActionFunc(Ihandle *ih) {
	IupSetCallback(ih, "FLAT_ACTION", (Icallback) goIupFlatToggleActionCB);
}

CGO_EXPORT extern int goIupTabChangeCB(void *ih, void *new_tab, void *old_tab);
static void goIupSetTabChangeFunc(Ihandle *ih) {
	IupSetCallback(ih, "TABCHANGE_CB", (Icallback) goIupTabChangeCB);
}

CGO_EXPORT extern int goIupTabChangePosCB(void *ih, int old_pos, int new_pos);
static void goIupSetTabChangePosFunc(Ihandle *ih) {
	IupSetCallback(ih, "TABCHANGEPOS_CB", (Icallback) goIupTabChangePosCB);
}

CGO_EXPORT extern int goIupSpinCB(void *ih, int inc);
static void goIupSetSpinFunc(Ihandle *ih) {
	IupSetCallback(ih, "SPIN_CB", (Icallback) goIupSpinCB);
}

CGO_EXPORT extern int goIupPostMessageCB(void *ih, void *s, int i, double d, void *p);
static void goIupSetPostMessageFunc(Ihandle *ih) {
	IupSetCallback(ih, "POSTMESSAGE_CB", (Icallback) goIupPostMessageCB);
}

CGO_EXPORT extern int goIupCloseCB(void *);
static void goIupSetCloseFunc(Ihandle *ih) {
	IupSetCallback(ih, "CLOSE_CB", (Icallback) goIupCloseCB);
}

CGO_EXPORT extern int goIupFocusCB(void *, int c);
static void goIupSetFocusFunc(Ihandle *ih) {
	IupSetCallback(ih, "FOCUS_CB", (Icallback) goIupFocusCB);
}

CGO_EXPORT extern int goIupMoveCB(void *, int x, int y);
static void goIupSetMoveFunc(Ihandle *ih) {
	IupSetCallback(ih, "MOVE_CB", (Icallback) goIupMoveCB);
}

CGO_EXPORT extern int goIupResizeCB(void *, int width, int height);
static void goIupSetResizeFunc(Ihandle *ih) {
	IupSetCallback(ih, "RESIZE_CB", (Icallback) goIupResizeCB);
}

CGO_EXPORT extern int goIupShowCB(void *ih, int state);
static void goIupSetShowFunc(Ihandle *ih) {
	IupSetCallback(ih, "SHOW_CB", (Icallback) goIupShowCB);
}

CGO_EXPORT extern int goIupChangeCB(void *, unsigned char r, unsigned char g, unsigned char b);
static void goIupSetChangeFunc(Ihandle *ih) {
	IupSetCallback(ih, "CHANGE_CB", (Icallback) goIupChangeCB);
}

CGO_EXPORT extern int goIupDragCB(void *, unsigned char r, unsigned char g, unsigned char b);
static void goIupSetDragFunc(Ihandle *ih) {
	IupSetCallback(ih, "DRAG_CB", (Icallback) goIupDragCB);
}

CGO_EXPORT extern int goIupDetachedCB(void *ih, void *newParent, int x, int y);
static void goIupSetDetachedFunc(Ihandle *ih) {
	IupSetCallback(ih, "DETACHED_CB", (Icallback) goIupDetachedCB);
}

CGO_EXPORT extern int goIupRestoredCB(void *ih, void *oldParent, int x, int y);
static void goIupSetRestoredFunc(Ihandle *ih) {
	IupSetCallback(ih, "RESTORED_CB", (Icallback) goIupRestoredCB);
}

CGO_EXPORT extern int goIupSwapBuffersCB(void *);
static void goIupSetSwapBuffersFunc(Ihandle *ih) {
	IupSetCallback(ih, "SWAPBUFFERS_CB", (Icallback) goIupSwapBuffersCB);
}

CGO_EXPORT extern int goIupCancelCB(void *);
static void goIupSetCancelFunc(Ihandle *ih) {
	IupSetCallback(ih, "CANCEL_CB", (Icallback) goIupCancelCB);
}

CGO_EXPORT extern int goIupTimerActionCB(void *);
static void goIupSetTimerActionFunc(Ihandle *ih) {
	IupSetCallback(ih, "ACTION_CB", (Icallback) goIupTimerActionCB);
}

CGO_EXPORT extern int goIupThreadCB(void *);
static void goIupSetThreadFunc(Ihandle *ih) {
	IupSetCallback(ih, "THREAD_CB", (Icallback) goIupThreadCB);
}

CGO_EXPORT extern int goIupScrollCB(void *, int op, float posx, float posy);
static void goIupSetScrollFunc(Ihandle *ih) {
	IupSetCallback(ih, "SCROLL_CB", (Icallback) goIupScrollCB);
}

CGO_EXPORT extern int goIupTrayClickCB(void *, int but, int pressed, int dclick);
static void goIupSetTrayClickFunc(Ihandle *ih) {
	IupSetCallback(ih, "TRAYCLICK_CB", (Icallback) goIupTrayClickCB);
}

CGO_EXPORT extern int goIupTabCloseCB(void *, int pos);
static void goIupSetTabCloseFunc(Ihandle *ih) {
	IupSetCallback(ih, "TABCLOSE_CB", (Icallback) goIupTabCloseCB);
}

CGO_EXPORT extern int goIupRightClickCB(void *, int pos);
static void goIupSetRightClickFunc(Ihandle *ih) {
	IupSetCallback(ih, "RIGHTCLICK_CB", (Icallback) goIupRightClickCB);
}

CGO_EXPORT extern int goIupExtraButtonCB(void *, int button, int pressed);
static void goIupSetExtraButtonFunc(Ihandle *ih) {
	IupSetCallback(ih, "EXTRABUTTON_CB", (Icallback) goIupExtraButtonCB);
}

CGO_EXPORT extern int goIupOpenCloseCB(void *, int state);
static void goIupSetOpenCloseFunc(Ihandle *ih) {
	IupSetCallback(ih, "OPENCLOSE_CB", (Icallback) goIupOpenCloseCB);
}

CGO_EXPORT extern int goIupValueChangingCB(void *, int start);
static void goIupSetValueChangingFunc(Ihandle *ih) {
	IupSetCallback(ih, "VALUECHANGING_CB", (Icallback) goIupValueChangingCB);
}

CGO_EXPORT extern int goIupDropDownCB(void *, int state);
static void goIupSetDropDownFunc(Ihandle *ih) {
	IupSetCallback(ih, "DROPDOWN_CB", (Icallback) goIupDropDownCB);
}

CGO_EXPORT extern int goIupDropShowCB(void *, int state);
static void goIupSetDropShowFunc(Ihandle *ih) {
	IupSetCallback(ih, "DROPSHOW_CB", (Icallback) goIupDropShowCB);
}

CGO_EXPORT extern int goIupButtonPressCB(void *, double angle);
static void goIupSetButtonPressFunc(Ihandle *ih) {
	IupSetCallback(ih, "BUTTON_PRESS_CB", (Icallback) goIupButtonPressCB);
}

CGO_EXPORT extern int goIupButtonReleaseCB(void *, double angle);
static void goIupSetButtonReleaseFunc(Ihandle *ih) {
	IupSetCallback(ih, "BUTTON_RELEASE_CB", (Icallback) goIupButtonReleaseCB);
}

CGO_EXPORT extern int goIupMouseMoveCB(void *, double angle);
static void goIupSetMouseMoveFunc(Ihandle *ih) {
	IupSetCallback(ih, "MOUSEMOVE_CB", (Icallback) goIupMouseMoveCB);
}

CGO_EXPORT extern int goIupKeyPressCB(void *ih, int c, int press);
static void goIupSetKeyPressFunc(Ihandle *ih) {
	IupSetCallback(ih, "KEYPRESS_CB", (Icallback) goIupKeyPressCB);
}

CGO_EXPORT extern int goIupCellCB(void *ih, int cell);
static void goIupSetCellFunc(Ihandle *ih) {
	IupSetCallback(ih, "CELL_CB", (Icallback) goIupCellCB);
}

CGO_EXPORT extern int goIupExtendedCB(void *ih, int cell);
static void goIupSetExtendedFunc(Ihandle *ih) {
	IupSetCallback(ih, "EXTENDED_CB", (Icallback) goIupExtendedCB);
}

CGO_EXPORT extern int goIupSelectCB(void *ih, int cell, int type);
static void goIupSetSelectFunc(Ihandle *ih) {
	IupSetCallback(ih, "SELECT_CB", (Icallback) goIupSelectCB);
}

CGO_EXPORT extern int goIupSwitchCB(void *ih, int primCell, int secCell);
static void goIupSetSwitchFunc(Ihandle *ih) {
	IupSetCallback(ih, "SWITCH_CB", (Icallback) goIupSwitchCB);
}

CGO_EXPORT extern int goIupLinkActionCB(void *, void *url);
static void goIupSetLinkActionFunc(Ihandle *ih) {
	IupSetCallback(ih, "ACTION", (Icallback) goIupLinkActionCB);
}

CGO_EXPORT extern int goIupWheelCB(void *, float delta, int x, int y, void *status);
static void goIupSetWheelFunc(Ihandle *ih) {
	IupSetCallback(ih, "WHEEL_CB", (Icallback) goIupWheelCB);
}

CGO_EXPORT extern int goIupDragDropCB(void *, int dragId, int dropId, int isShift, int isControl);
static void goIupSetDragDropFunc(Ihandle *ih) {
	IupSetCallback(ih, "DRAGDROP_CB", (Icallback) goIupDragDropCB);
}

CGO_EXPORT extern int goIupDragBeginCB(void *, int x, int y);
static void goIupSetDragBeginFunc(Ihandle *ih) {
	IupSetCallback(ih, "DRAGBEGIN_CB", (Icallback) goIupDragBeginCB);
}

CGO_EXPORT extern int goIupDragDataSizeCB(void *, char *dragType);
static void goIupSetDragDataSizeFunc(Ihandle *ih) {
	IupSetCallback(ih, "DRAGDATASIZE_CB", (Icallback) goIupDragDataSizeCB);
}

CGO_EXPORT extern int goIupDragDataCB(void *, char *dragType, void *data, int size);
static void goIupSetDragDataFunc(Ihandle *ih) {
	IupSetCallback(ih, "DRAGDATA_CB", (Icallback) goIupDragDataCB);
}

CGO_EXPORT extern int goIupDragEndCB(void *, int action);
static void goIupSetDragEndFunc(Ihandle *ih) {
	IupSetCallback(ih, "DRAGEND_CB", (Icallback) goIupDragEndCB);
}

CGO_EXPORT extern int goIupDropDataCB(void *, char *dragType, void *data, int size, int x, int y);
static void goIupSetDropDataFunc(Ihandle *ih) {
	IupSetCallback(ih, "DROPDATA_CB", (Icallback) goIupDropDataCB);
}

CGO_EXPORT extern int goIupDropMotionCB(void *, int x, int y, char *status);
static void goIupSetDropMotionFunc(Ihandle *ih) {
	IupSetCallback(ih, "DROPMOTION_CB", (Icallback) goIupDropMotionCB);
}

CGO_EXPORT extern int goIupSelectionCB(void *ih, int id, int status);
static void goIupSetSelectionFunc(Ihandle *ih) {
	IupSetCallback(ih, "SELECTION_CB", (Icallback) goIupSelectionCB);
}

CGO_EXPORT extern int goIupBranchOpenCB(void *ih, int id);
static void goIupSetBranchOpenFunc(Ihandle *ih) {
	IupSetCallback(ih, "BRANCHOPEN_CB", (Icallback) goIupBranchOpenCB);
}

CGO_EXPORT extern int goIupBranchCloseCB(void *ih, int id);
static void goIupSetBranchCloseFunc(Ihandle *ih) {
	IupSetCallback(ih, "BRANCHCLOSE_CB", (Icallback) goIupBranchCloseCB);
}

CGO_EXPORT extern int goIupExecuteLeafCB(void *ih, int id);
static void goIupSetExecuteLeafFunc(Ihandle *ih) {
	IupSetCallback(ih, "EXECUTELEAF_CB", (Icallback) goIupExecuteLeafCB);
}

CGO_EXPORT extern int goIupExecuteBranchCB(void *ih, int id);
static void goIupSetExecuteBranchFunc(Ihandle *ih) {
	IupSetCallback(ih, "EXECUTEBRANCH_CB", (Icallback) goIupExecuteBranchCB);
}

CGO_EXPORT extern int goIupShowRenameCB(void *ih, int id);
static void goIupSetShowRenameFunc(Ihandle *ih) {
	IupSetCallback(ih, "SHOWRENAME_CB", (Icallback) goIupShowRenameCB);
}

CGO_EXPORT extern int goIupRenameCB(void *, int id, void *title);
static void goIupSetRenameFunc(Ihandle *ih) {
	IupSetCallback(ih, "RENAME_CB", (Icallback) goIupRenameCB);
}

CGO_EXPORT extern int goIupToggleValueCB(void *ih, int id, int state);
static void goIupSetToggleValueFunc(Ihandle *ih) {
	IupSetCallback(ih, "TOGGLEVALUE_CB", (Icallback) goIupToggleValueCB);
}

CGO_EXPORT extern int goIupNodeRemovedCB(void *, void *userData);
static void goIupSetNodeRemovedFunc(Ihandle *ih) {
	IupSetCallback(ih, "NODEREMOVED_CB", (Icallback) goIupNodeRemovedCB);
}

CGO_EXPORT extern int goIupMultiSelectionCB(void *ih, int* ids, int n);
static void goIupSetMultiSelectionFunc(Ihandle *ih) {
	IupSetCallback(ih, "MULTISELECTION_CB", (Icallback) goIupMultiSelectionCB);
}

CGO_EXPORT extern int goIupMultiUnselectionCB(void *ih, int* ids, int n);
static void goIupSetMultiUnselectionFunc(Ihandle *ih) {
	IupSetCallback(ih, "MULTIUNSELECTION_CB", (Icallback) goIupMultiUnselectionCB);
}

CGO_EXPORT extern int goIupMenuOpenCB(void *);
static void goIupSetMenuOpenFunc(Ihandle *ih) {
	IupSetCallback(ih, "OPEN_CB", (Icallback) goIupMenuOpenCB);
}

CGO_EXPORT extern int goIupThemeChangedCB(void *, int darkMode);
static void goIupSetThemeChangedFunc(Ihandle *ih) {
	IupSetCallback(ih, "THEMECHANGED_CB", (Icallback) goIupThemeChangedCB);
}

CGO_EXPORT extern int goIupCompletedCB(void *, void *url);
static void goIupSetCompletedFunc(Ihandle *ih) {
	IupSetCallback(ih, "COMPLETED_CB", (Icallback) goIupCompletedCB);
}

CGO_EXPORT extern int goIupErrorCB(void *, void *url);
static void goIupSetErrorFunc(Ihandle *ih) {
	IupSetCallback(ih, "ERROR_CB", (Icallback) goIupErrorCB);
}

CGO_EXPORT extern int goIupNavigateCB(void *, void *url);
static void goIupSetNavigateFunc(Ihandle *ih) {
	IupSetCallback(ih, "NAVIGATE_CB", (Icallback) goIupNavigateCB);
}

CGO_EXPORT extern int goIupNewWindowCB(void *, void *url);
static void goIupSetNewWindowFunc(Ihandle *ih) {
	IupSetCallback(ih, "NEWWINDOW_CB", (Icallback) goIupNewWindowCB);
}

CGO_EXPORT extern int goIupUpdateCB(void *);
static void goIupSetUpdateFunc(Ihandle *ih) {
	IupSetCallback(ih, "UPDATE_CB", (Icallback) goIupUpdateCB);
}

// ============================================================================
// CELLS AND MATRIX CONTROL CALLBACKS
// ============================================================================

CGO_EXPORT extern int goIupCellsDrawCB(void *, int i, int j, int xmin, int xmax, int ymin, int ymax);
static void goIupSetCellsDrawFunc(Ihandle *ih) {
	IupSetCallback(ih, "DRAW_CB", (Icallback) goIupCellsDrawCB);
}

CGO_EXPORT extern int goIupMouseClickCB(void *, int button, int pressed, int i, int j, int x, int y, void *status);
static void goIupSetMouseClickFunc(Ihandle *ih) {
	IupSetCallback(ih, "MOUSECLICK_CB", (Icallback) goIupMouseClickCB);
}

CGO_EXPORT extern int goIupMouseMotionCB(void *, int i, int j, int x, int y, void *status);
static void goIupSetMouseMotionFunc(Ihandle *ih) {
	IupSetCallback(ih, "MOUSEMOTION_CB", (Icallback) goIupMouseMotionCB);
}

CGO_EXPORT extern int goIupScrollingCB(void *, int i, int j);
static void goIupSetScrollingFunc(Ihandle *ih) {
	IupSetCallback(ih, "SCROLLING_CB", (Icallback) goIupScrollingCB);
}

CGO_EXPORT extern int goIupNColsCB(void *);
static void goIupSetNColsFunc(Ihandle *ih) {
	IupSetCallback(ih, "NCOLS_CB", (Icallback) goIupNColsCB);
}

CGO_EXPORT extern int goIupNLinesCB(void *);
static void goIupSetNLinesFunc(Ihandle *ih) {
	IupSetCallback(ih, "NLINES_CB", (Icallback) goIupNLinesCB);
}

CGO_EXPORT extern int goIupHSpanCB(void *, int i, int j);
static void goIupSetHSpanFunc(Ihandle *ih) {
	IupSetCallback(ih, "HSPAN_CB", (Icallback) goIupHSpanCB);
}

CGO_EXPORT extern int goIupVSpanCB(void *, int i, int j);
static void goIupSetVSpanFunc(Ihandle *ih) {
	IupSetCallback(ih, "VSPAN_CB", (Icallback) goIupVSpanCB);
}

CGO_EXPORT extern int goIupHeightCB(void *, int i);
static void goIupSetHeightFunc(Ihandle *ih) {
	IupSetCallback(ih, "HEIGHT_CB", (Icallback) goIupHeightCB);
}

CGO_EXPORT extern int goIupWidthCB(void *, int j);
static void goIupSetWidthFunc(Ihandle *ih) {
	IupSetCallback(ih, "WIDTH_CB", (Icallback) goIupWidthCB);
}

CGO_EXPORT extern int goIupMatrixDrawCB(void *, int lin, int col, int x1, int x2, int y1, int y2);
static void goIupSetMatrixDrawFunc(Ihandle *ih) {
	IupSetCallback(ih, "DRAW_CB", (Icallback) goIupMatrixDrawCB);
}

CGO_EXPORT extern int goIupEnterItemCB(void *, int lin, int col);
static void goIupSetEnterItemFunc(Ihandle *ih) {
	IupSetCallback(ih, "ENTERITEM_CB", (Icallback) goIupEnterItemCB);
}

CGO_EXPORT extern int goIupLeaveItemCB(void *, int lin, int col);
static void goIupSetLeaveItemFunc(Ihandle *ih) {
	IupSetCallback(ih, "LEAVEITEM_CB", (Icallback) goIupLeaveItemCB);
}

CGO_EXPORT extern int goIupClickCB(void *, int lin, int col, void *status);
static void goIupSetClickFunc(Ihandle *ih) {
	IupSetCallback(ih, "CLICK_CB", (Icallback) goIupClickCB);
}

CGO_EXPORT extern int goIupReleaseCB(void *, int lin, int col, void *status);
static void goIupSetReleaseFunc(Ihandle *ih) {
	IupSetCallback(ih, "RELEASE_CB", (Icallback) goIupReleaseCB);
}

CGO_EXPORT extern int goIupMatrixMouseMoveCB(void *, int lin, int col);
static void goIupSetMatrixMouseMoveFunc(Ihandle *ih) {
	IupSetCallback(ih, "MOUSEMOVE_CB", (Icallback) goIupMatrixMouseMoveCB);
}

CGO_EXPORT extern int goIupEditionCB(void *, int lin, int col, int mode, int update);
static void goIupSetEditionFunc(Ihandle *ih) {
	IupSetCallback(ih, "EDITION_CB", (Icallback) goIupEditionCB);
}

CGO_EXPORT extern int goIupTableEditionCB(void *, int lin, int col, char* update);
static void goIupSetTableEditionFunc(Ihandle *ih) {
	IupSetCallback(ih, "EDITION_CB", (Icallback) goIupTableEditionCB);
}

CGO_EXPORT extern int goIupEditBeginCB(void *, int lin, int col);
static void goIupSetEditBeginFunc(Ihandle *ih) {
	IupSetCallback(ih, "EDITBEGIN_CB", (Icallback) goIupEditBeginCB);
}

CGO_EXPORT extern int goIupEditEndCB(void *, int lin, int col, char* newValue, int apply);
static void goIupSetEditEndFunc(Ihandle *ih) {
	IupSetCallback(ih, "EDITEND_CB", (Icallback) goIupEditEndCB);
}

CGO_EXPORT extern int goIupTableValueChangedCB(void *, int lin, int col);
static void goIupSetTableValueChangedFunc(Ihandle *ih) {
	IupSetCallback(ih, "VALUECHANGED_CB", (Icallback) goIupTableValueChangedCB);
}

CGO_EXPORT extern char* goIupTableValueCB(void *, int lin, int col);
static void goIupSetTableValueFunc(Ihandle *ih) {
	IupSetCallback(ih, "VALUE_CB", (Icallback) goIupTableValueCB);
}

CGO_EXPORT extern int goIupTableSortCB(void *, int col);
static void goIupSetTableSortFunc(Ihandle *ih) {
	IupSetCallback(ih, "SORT_CB", (Icallback) goIupTableSortCB);
}

CGO_EXPORT extern int goIupDropCheckCB(void *, int lin, int col);
static void goIupSetDropCheckFunc(Ihandle *ih) {
	IupSetCallback(ih, "DROPCHECK_CB", (Icallback) goIupDropCheckCB);
}

CGO_EXPORT extern int goIupMarkCB(void *, int lin, int col);
static void goIupSetMarkFunc(Ihandle *ih) {
	IupSetCallback(ih, "MARK_CB", (Icallback) goIupMarkCB);
}

CGO_EXPORT extern int goIupMarkEditCB(void *, int lin, int col, int marked);
static void goIupSetMarkEditFunc(Ihandle *ih) {
	IupSetCallback(ih, "MARKEDIT_CB", (Icallback) goIupMarkEditCB);
}

CGO_EXPORT extern int goIupValueEditCB(void *, int lin, int col, void *newval);
static void goIupSetValueEditFunc(Ihandle *ih) {
	IupSetCallback(ih, "VALUE_EDIT_CB", (Icallback) goIupValueEditCB);
}

CGO_EXPORT extern int goIupColResizeCB(void *, int col);
static void goIupSetColResizeFunc(Ihandle *ih) {
	IupSetCallback(ih, "COLRESIZE_CB", (Icallback) goIupColResizeCB);
}

CGO_EXPORT extern int goIupResizeMatrixCB(void *, int width, int height);
static void goIupSetResizeMatrixFunc(Ihandle *ih) {
	IupSetCallback(ih, "RESIZEMATRIX_CB", (Icallback) goIupResizeMatrixCB);
}

CGO_EXPORT extern int goIupScrollTopCB(void *, int lin, int col);
static void goIupSetScrollTopFunc(Ihandle *ih) {
	IupSetCallback(ih, "SCROLLTOP_CB", (Icallback) goIupScrollTopCB);
}

CGO_EXPORT extern int goIupBgColorCB(void *, int lin, int col, int *r, int *g, int *b);
static void goIupSetBgColorFunc(Ihandle *ih) {
	IupSetCallback(ih, "BGCOLOR_CB", (Icallback) goIupBgColorCB);
}

CGO_EXPORT extern int goIupFgColorCB(void *, int lin, int col, int *r, int *g, int *b);
static void goIupSetFgColorFunc(Ihandle *ih) {
	IupSetCallback(ih, "FGCOLOR_CB", (Icallback) goIupFgColorCB);
}

CGO_EXPORT extern int goIupMatrixDropCB(void *, void *drop, int lin, int col);
static void goIupSetMatrixDropFunc(Ihandle *ih) {
	IupSetCallback(ih, "DROP_CB", (Icallback) goIupMatrixDropCB);
}

CGO_EXPORT extern int goIupMatrixActionCB(void *, int key, int lin, int col, int edition, char *status);
static void goIupSetMatrixActionFunc(Ihandle *ih) {
	IupSetCallback(ih, "ACTION_CB", (Icallback) goIupMatrixActionCB);
}

CGO_EXPORT extern int goIupEditClickCB(void *, int lin, int col, char *status);
static void goIupSetEditClickFunc(Ihandle *ih) {
	IupSetCallback(ih, "EDITCLICK_CB", (Icallback) goIupEditClickCB);
}

CGO_EXPORT extern int goIupEditReleaseCB(void *, int lin, int col, char *status);
static void goIupSetEditReleaseFunc(Ihandle *ih) {
	IupSetCallback(ih, "EDITRELEASE_CB", (Icallback) goIupEditReleaseCB);
}

CGO_EXPORT extern int goIupEditMouseMoveCB(void *, int lin, int col);
static void goIupSetEditMouseMoveFunc(Ihandle *ih) {
	IupSetCallback(ih, "EDITMOUSEMOVE_CB", (Icallback) goIupEditMouseMoveCB);
}

CGO_EXPORT extern char* goIupMatrixFontCB(void *, int lin, int col);
static void goIupSetMatrixFontFunc(Ihandle *ih) {
	IupSetCallback(ih, "FONT_CB", (Icallback) goIupMatrixFontCB);
}

CGO_EXPORT extern char* goIupMatrixTypeCB(void *, int lin, int col);
static void goIupSetMatrixTypeFunc(Ihandle *ih) {
	IupSetCallback(ih, "TYPE_CB", (Icallback) goIupMatrixTypeCB);
}

CGO_EXPORT extern char* goIupTranslateValueCB(void *, int lin, int col, char *value);
static void goIupSetTranslateValueFunc(Ihandle *ih) {
	IupSetCallback(ih, "TRANSLATEVALUE_CB", (Icallback) goIupTranslateValueCB);
}

CGO_EXPORT extern int goIupMenuDropCB(void *, void *menu, int lin, int col);
static void goIupSetMenuDropFunc(Ihandle *ih) {
	IupSetCallback(ih, "MENUDROP_CB", (Icallback) goIupMenuDropCB);
}

CGO_EXPORT extern int goIupDropSelectCB(void *, int lin, int col, void *drop, char *text, int item, int col2);
static void goIupSetDropSelectFunc(Ihandle *ih) {
	IupSetCallback(ih, "DROPSELECT_CB", (Icallback) goIupDropSelectCB);
}

CGO_EXPORT extern char* goIupMatrixValueCB(void *, int lin, int col);
static void goIupSetMatrixValueFunc(Ihandle *ih) {
	IupSetCallback(ih, "VALUE_CB", (Icallback) goIupMatrixValueCB);
}

CGO_EXPORT extern int goIupMatrixToggleValueCB(void *, int lin, int col, int value);
static void goIupSetMatrixToggleValueFunc(Ihandle *ih) {
	IupSetCallback(ih, "TOGGLEVALUE_CB", (Icallback) goIupMatrixToggleValueCB);
}

CGO_EXPORT extern int goIupMatrixListActionCB(void *, int item, int state);
static void goIupSetMatrixListActionFunc(Ihandle *ih, char *name) {
	IupSetCallback(ih, name, (Icallback) goIupMatrixListActionCB);
}

CGO_EXPORT extern int goIupFileCB(void *, char *filename, char *status);
static void goIupSetFileFunc(Ihandle *ih) {
	IupSetCallback(ih, "FILE_CB", (Icallback) goIupFileCB);
}

CGO_EXPORT extern int goIupLayoutUpdateCB(void *);
static void goIupSetLayoutUpdateFunc(Ihandle *ih) {
	IupSetCallback(ih, "LAYOUTUPDATE_CB", (Icallback) goIupLayoutUpdateCB);
}

CGO_EXPORT extern int goIupHighlightCB(void *);
static void goIupSetHighlightFunc(Ihandle *ih) {
	IupSetCallback(ih, "HIGHLIGHT_CB", (Icallback) goIupHighlightCB);
}

CGO_EXPORT extern int goIupMenuCloseCB(void *);
static void goIupSetMenuCloseFunc(Ihandle *ih) {
	IupSetCallback(ih, "MENUCLOSE_CB", (Icallback) goIupMenuCloseCB);
}

CGO_EXPORT extern int goIupColorUpdateCB(void *);
static void goIupSetColorUpdateFunc(Ihandle *ih) {
	IupSetCallback(ih, "COLORUPDATE_CB", (Icallback) goIupColorUpdateCB);
}

CGO_EXPORT extern int goIupListReleaseCB(void *, int lin, int col, char *status);
static void goIupSetListReleaseFunc(Ihandle *ih) {
	IupSetCallback(ih, "LISTRELEASE_CB", (Icallback) goIupListReleaseCB);
}

CGO_EXPORT extern int goIupListInsertCB(void *, int pos);
static void goIupSetListInsertFunc(Ihandle *ih) {
	IupSetCallback(ih, "LISTINSERT_CB", (Icallback) goIupListInsertCB);
}

CGO_EXPORT extern int goIupListRemoveCB(void *, int pos);
static void goIupSetListRemoveFunc(Ihandle *ih) {
	IupSetCallback(ih, "LISTREMOVE_CB", (Icallback) goIupListRemoveCB);
}

CGO_EXPORT extern int goIupListEditionCB(void *, int lin, int col, int mode, int update);
static void goIupSetListEditionFunc(Ihandle *ih) {
	IupSetCallback(ih, "LISTEDITION_CB", (Icallback) goIupListEditionCB);
}

CGO_EXPORT extern int goIupListDrawCB(void *, int lin, int x1, int x2, int y1, int y2, int canvas);
static void goIupSetListDrawFunc(Ihandle *ih) {
	IupSetCallback(ih, "LISTDRAW_CB", (Icallback) goIupListDrawCB);
}

CGO_EXPORT extern int goIupBusyCB(void *, int lin, int col, char *status);
static void goIupSetBusyFunc(Ihandle *ih) {
	IupSetCallback(ih, "BUSY_CB", (Icallback) goIupBusyCB);
}

CGO_EXPORT extern int goIupMenuContextCB(void *, void *menu, int lin, int col);
static void goIupSetMenuContextFunc(Ihandle *ih) {
	IupSetCallback(ih, "MENUCONTEXT_CB", (Icallback) goIupMenuContextCB);
}

CGO_EXPORT extern int goIupMenuContextCloseCB(void *, void *menu, int lin, int col);
static void goIupSetMenuContextCloseFunc(Ihandle *ih) {
	IupSetCallback(ih, "MENUCONTEXTCLOSE_CB", (Icallback) goIupMenuContextCloseCB);
}

CGO_EXPORT extern int goIupPasteSizeCB(void *, int numlin, int numcol);
static void goIupSetPasteSizeFunc(Ihandle *ih) {
	IupSetCallback(ih, "PASTESIZE_CB", (Icallback) goIupPasteSizeCB);
}

CGO_EXPORT extern double goIupNumericGetValueCB(void *, int lin, int col);
static void goIupSetNumericGetValueFunc(Ihandle *ih) {
	IupSetCallback(ih, "NUMERICGETVALUE_CB", (Icallback) goIupNumericGetValueCB);
}

CGO_EXPORT extern int goIupNumericSetValueCB(void *, int lin, int col, double value);
static void goIupSetNumericSetValueFunc(Ihandle *ih) {
	IupSetCallback(ih, "NUMERICSETVALUE_CB", (Icallback) goIupNumericSetValueCB);
}

CGO_EXPORT extern int goIupSortColumnCompareCB(void *, int lin1, int lin2, int col);
static void goIupSetSortColumnCompareFunc(Ihandle *ih) {
	IupSetCallback(ih, "SORTCOLUMNCOMPARE_CB", (Icallback) goIupSortColumnCompareCB);
}

CGO_EXPORT extern int goIupRecentCB(void *);

CGO_EXPORT extern int goIupNotifyCB(void *, int action_id);
static void goIupSetNotifyFunc(Ihandle *ih) {
	IupSetCallback(ih, "NOTIFY_CB", (Icallback) goIupNotifyCB);
}

CGO_EXPORT extern int goIupNotifyCloseCB(void *, int reason);
static void goIupSetNotifyCloseFunc(Ihandle *ih) {
	IupSetCallback(ih, "CLOSE_CB", (Icallback) goIupNotifyCloseCB);
}

#endif /* BIND_CALLBACKS_H */
