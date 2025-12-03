package iup

import (
	"fmt"
	"runtime/cgo"
	"sync"
	"sync/atomic"
	"unsafe"
)

/*
#include <stdlib.h>
#include "iup.h"

extern int goIupIdleCB();
static void goIupSetIdleFunc() {
	IupSetFunction("IDLE_ACTION", (Icallback) goIupIdleCB);
}

extern void goIupEntryPointCB();
static void goIupSetEntryPointFunc() {
	IupSetFunction("ENTRY_POINT", (Icallback) goIupEntryPointCB);
}

extern void goIupExitCB();
static void goIupSetExitFunc() {
	IupSetFunction("EXIT_CB", (Icallback) goIupExitCB);
}

extern int goIupMapCB(void *);
static void goIupSetMapFunc(Ihandle *ih) {
	IupSetCallback(ih, "MAP_CB", (Icallback) goIupMapCB);
}

extern int goIupUnmapCB(void *);
static void goIupSetUnmapFunc(Ihandle *ih) {
	IupSetCallback(ih, "UNMAP_CB", (Icallback) goIupUnmapCB);
}

extern int goIupDestroyCB(void *);
static void goIupSetDestroyFunc(Ihandle *ih) {
	IupSetCallback(ih, "DESTROY_CB", (Icallback) goIupDestroyCB);
}

extern int goIupGetFocusCB(void *);
static void goIupSetGetFocusFunc(Ihandle *ih) {
	IupSetCallback(ih, "GETFOCUS_CB", (Icallback) goIupGetFocusCB);
}

extern int goIupKillFocusCB(void *);
static void goIupSetKillFocusFunc(Ihandle *ih) {
	IupSetCallback(ih, "KILLFOCUS_CB", (Icallback) goIupKillFocusCB);
}

extern int goIupEnterWindowCB(void *);
static void goIupSetEnterWindowFunc(Ihandle *ih) {
	IupSetCallback(ih, "ENTERWINDOW_CB", (Icallback) goIupEnterWindowCB);
}

extern int goIupLeaveWindowCB(void *);
static void goIupSetLeaveWindowFunc(Ihandle *ih) {
	IupSetCallback(ih, "LEAVEWINDOW_CB", (Icallback) goIupLeaveWindowCB);
}

extern int goIupKAnyCB(void *, int c);
static void goIupSetKAnyFunc(Ihandle *ih) {
	IupSetCallback(ih, "K_ANY", (Icallback) goIupKAnyCB);
}

extern int goIupHelpCB(void *);
static void goIupSetHelpFunc(Ihandle *ih) {
	IupSetCallback(ih, "HELP_CB", (Icallback) goIupHelpCB);
}

extern int goIupActionCB(void *);
static void goIupSetActionFunc(Ihandle *ih) {
	IupSetCallback(ih, "ACTION", (Icallback) goIupActionCB);
}

extern int goIupFlatActionCB(void *);
static void goIupSetFlatActionFunc(Ihandle *ih) {
	IupSetCallback(ih, "FLAT_ACTION", (Icallback) goIupFlatActionCB);
}

extern int goIupButtonCB(void *, int button, int pressed, int x, int y, void *status);
static void goIupSetButtonFunc(Ihandle *ih) {
	IupSetCallback(ih, "BUTTON_CB", (Icallback) goIupButtonCB);
}

extern int goIupDropFilesCB(void *, void *filename, int num, int x, int y);
static void goIupSetDropFilesFunc(Ihandle *ih) {
	IupSetCallback(ih, "DROPFILES_CB", (Icallback) goIupDropFilesCB);
}

extern int goIupListActionCB(void *, void *text, int item, int state);
static void goIupSetListActionFunc(Ihandle *ih) {
	IupSetCallback(ih, "ACTION", (Icallback) goIupListActionCB);
}

extern int goIupFlatListActionCB(void *, void *text, int item, int state);
static void goIupSetFlatListActionFunc(Ihandle *ih) {
	IupSetCallback(ih, "FLAT_ACTION", (Icallback) goIupFlatListActionCB);
}

extern int goIupCaretCB(void *, int lin, int col, int pos);
static void goIupSetCaretFunc(Ihandle *ih) {
	IupSetCallback(ih, "CARET_CB", (Icallback) goIupCaretCB);
}

extern int goIupDblclickCB(void *, int item, void *text);
static void goIupSetDblclickFunc(Ihandle *ih) {
	IupSetCallback(ih, "DBLCLICK_CB", (Icallback) goIupDblclickCB);
}

extern int goIupEditCB(void *, int c, void *newValue);
static void goIupSetEditFunc(Ihandle *ih) {
	IupSetCallback(ih, "EDIT_CB", (Icallback) goIupEditCB);
}

extern int goIupMotionCB(void *, int x, int y, void *status);
static void goIupSetMotionFunc(Ihandle *ih) {
	IupSetCallback(ih, "MOTION_CB", (Icallback) goIupMotionCB);
}

extern int goIupMultiselectCB(void *, void *text);
static void goIupSetMultiselectFunc(Ihandle *ih) {
	IupSetCallback(ih, "MULTISELECT_CB", (Icallback) goIupMultiselectCB);
}

extern int goIupValueChangedCB(void *);
static void goIupSetValueChangedFunc(Ihandle *ih) {
	IupSetCallback(ih, "VALUECHANGED_CB", (Icallback) goIupValueChangedCB);
}

extern int goIupTextActionCB(void *ih, int ch, void *newValue);
static void goIupSetTextActionFunc(Ihandle *ih) {
	IupSetCallback(ih, "ACTION", (Icallback) goIupTextActionCB);
}

extern int goIupToggleActionCB(void *, int state);
static void goIupSetToggleActionFunc(Ihandle *ih) {
	IupSetCallback(ih, "ACTION", (Icallback) goIupToggleActionCB);
}

extern int goIupFlatToggleActionCB(void *, int state);
static void goIupSetFlatToggleActionFunc(Ihandle *ih) {
	IupSetCallback(ih, "FLAT_ACTION", (Icallback) goIupFlatToggleActionCB);
}

extern int goIupTabChangeCB(void *ih, void *new_tab, void *old_tab);
static void goIupSetTabChangeFunc(Ihandle *ih) {
	IupSetCallback(ih, "TABCHANGE_CB", (Icallback) goIupTabChangeCB);
}

extern int goIupTabChangePosCB(void *ih, int old_pos, int new_pos);
static void goIupSetTabChangePosFunc(Ihandle *ih) {
	IupSetCallback(ih, "TABCHANGEPOS_CB", (Icallback) goIupTabChangePosCB);
}

extern int goIupSpinCB(void *ih, int inc);
static void goIupSetSpinFunc(Ihandle *ih) {
	IupSetCallback(ih, "SPIN_CB", (Icallback) goIupSpinCB);
}

extern int goIupPostMessageCB(void *ih, void *s, int i, double d, void *p);
static void goIupSetPostMessageFunc(Ihandle *ih) {
	IupSetCallback(ih, "POSTMESSAGE_CB", (Icallback) goIupPostMessageCB);
}

extern int goIupCloseCB(void *);
static void goIupSetCloseFunc(Ihandle *ih) {
	IupSetCallback(ih, "CLOSE_CB", (Icallback) goIupCloseCB);
}

extern int goIupFocusCB(void *, int c);
static void goIupSetFocusFunc(Ihandle *ih) {
	IupSetCallback(ih, "FOCUS_CB", (Icallback) goIupFocusCB);
}

extern int goIupMoveCB(void *, int x, int y);
static void goIupSetMoveFunc(Ihandle *ih) {
	IupSetCallback(ih, "MOVE_CB", (Icallback) goIupMoveCB);
}

extern int goIupResizeCB(void *, int width, int height);
static void goIupSetResizeFunc(Ihandle *ih) {
	IupSetCallback(ih, "RESIZE_CB", (Icallback) goIupResizeCB);
}

extern int goIupShowCB(void *ih, int state);
static void goIupSetShowFunc(Ihandle *ih) {
	IupSetCallback(ih, "SHOW_CB", (Icallback) goIupShowCB);
}

extern int goIupChangeCB(void *, unsigned char r, unsigned char g, unsigned char b);
static void goIupSetChangeFunc(Ihandle *ih) {
	IupSetCallback(ih, "CHANGE_CB", (Icallback) goIupChangeCB);
}

extern int goIupDragCB(void *, unsigned char r, unsigned char g, unsigned char b);
static void goIupSetDragFunc(Ihandle *ih) {
	IupSetCallback(ih, "DRAG_CB", (Icallback) goIupDragCB);
}

extern int goIupDetachedCB(void *ih, void *newParent, int x, int y);
static void goIupSetDetachedFunc(Ihandle *ih) {
	IupSetCallback(ih, "DETACHED_CB", (Icallback) goIupDetachedCB);
}

extern int goIupRestoredCB(void *ih, void *oldParent, int x, int y);
static void goIupSetRestoredFunc(Ihandle *ih) {
	IupSetCallback(ih, "RESTORED_CB", (Icallback) goIupRestoredCB);
}

extern int goIupSwapBuffersCB(void *);
static void goIupSetSwapBuffersFunc(Ihandle *ih) {
	IupSetCallback(ih, "SWAPBUFFERS_CB", (Icallback) goIupSwapBuffersCB);
}

extern int goIupCancelCB(void *);
static void goIupSetCancelFunc(Ihandle *ih) {
	IupSetCallback(ih, "CANCEL_CB", (Icallback) goIupCancelCB);
}

extern int goIupTimerActionCB(void *);
static void goIupSetTimerActionFunc(Ihandle *ih) {
	IupSetCallback(ih, "ACTION_CB", (Icallback) goIupTimerActionCB);
}

extern int goIupThreadCB(void *);
static void goIupSetThreadFunc(Ihandle *ih) {
	IupSetCallback(ih, "THREAD_CB", (Icallback) goIupThreadCB);
}

extern int goIupScrollCB(void *, int op, float posx, float posy);
static void goIupSetScrollFunc(Ihandle *ih) {
	IupSetCallback(ih, "SCROLL_CB", (Icallback) goIupScrollCB);
}

extern int goIupTrayClickCB(void *, int but, int pressed, int dclick);
static void goIupSetTrayClickFunc(Ihandle *ih) {
	IupSetCallback(ih, "TRAYCLICK_CB", (Icallback) goIupTrayClickCB);
}

extern int goIupTabCloseCB(void *, int pos);
static void goIupSetTabCloseFunc(Ihandle *ih) {
	IupSetCallback(ih, "TABCLOSE_CB", (Icallback) goIupTabCloseCB);
}

extern int goIupRightClickCB(void *, int pos);
static void goIupSetRightClickFunc(Ihandle *ih) {
	IupSetCallback(ih, "RIGHTCLICK_CB", (Icallback) goIupRightClickCB);
}

extern int goIupExtraButtonCB(void *, int button, int pressed);
static void goIupSetExtraButtonFunc(Ihandle *ih) {
	IupSetCallback(ih, "EXTRABUTTON_CB", (Icallback) goIupExtraButtonCB);
}

extern int goIupOpenCloseCB(void *, int state);
static void goIupSetOpenCloseFunc(Ihandle *ih) {
	IupSetCallback(ih, "OPENCLOSE_CB", (Icallback) goIupOpenCloseCB);
}

extern int goIupValueChangingCB(void *, int start);
static void goIupSetValueChangingFunc(Ihandle *ih) {
	IupSetCallback(ih, "VALUECHANGING_CB", (Icallback) goIupValueChangingCB);
}

extern int goIupDropDownCB(void *, int state);
static void goIupSetDropDownFunc(Ihandle *ih) {
	IupSetCallback(ih, "DROPDOWN_CB", (Icallback) goIupDropDownCB);
}

extern int goIupDropShowCB(void *, int state);
static void goIupSetDropShowFunc(Ihandle *ih) {
	IupSetCallback(ih, "DROPSHOW_CB", (Icallback) goIupDropShowCB);
}

extern int goIupButtonPressCB(void *, double angle);
static void goIupSetButtonPressFunc(Ihandle *ih) {
	IupSetCallback(ih, "BUTTON_PRESS_CB", (Icallback) goIupButtonPressCB);
}

extern int goIupButtonReleaseCB(void *, double angle);
static void goIupSetButtonReleaseFunc(Ihandle *ih) {
	IupSetCallback(ih, "BUTTON_RELEASE_CB", (Icallback) goIupButtonReleaseCB);
}

extern int goIupMouseMoveCB(void *, double angle);
static void goIupSetMouseMoveFunc(Ihandle *ih) {
	IupSetCallback(ih, "MOUSEMOVE_CB", (Icallback) goIupMouseMoveCB);
}

extern int goIupKeyPressCB(void *ih, int c, int press);
static void goIupSetKeyPressFunc(Ihandle *ih) {
	IupSetCallback(ih, "KEYPRESS_CB", (Icallback) goIupKeyPressCB);
}

extern int goIupCellCB(void *ih, int cell);
static void goIupSetCellFunc(Ihandle *ih) {
	IupSetCallback(ih, "CELL_CB", (Icallback) goIupCellCB);
}

extern int goIupExtendedCB(void *ih, int cell);
static void goIupSetExtendedFunc(Ihandle *ih) {
	IupSetCallback(ih, "EXTENDED_CB", (Icallback) goIupExtendedCB);
}

extern int goIupSelectCB(void *ih, int cell, int type);
static void goIupSetSelectFunc(Ihandle *ih) {
	IupSetCallback(ih, "SELECT_CB", (Icallback) goIupSelectCB);
}

extern int goIupSwitchCB(void *ih, int primCell, int secCell);
static void goIupSetSwitchFunc(Ihandle *ih) {
	IupSetCallback(ih, "SWITCH_CB", (Icallback) goIupSwitchCB);
}

extern int goIupLinkActionCB(void *, void *url);
static void goIupSetLinkActionFunc(Ihandle *ih) {
	IupSetCallback(ih, "ACTION", (Icallback) goIupLinkActionCB);
}

extern int goIupWheelCB(void *, float delta, int x, int y, void *status);
static void goIupSetWheelFunc(Ihandle *ih) {
	IupSetCallback(ih, "WHEEL_CB", (Icallback) goIupWheelCB);
}

extern int goIupDragDropCB(void *, int dragId, int dropId, int isShift, int isControl);
static void goIupSetDragDropFunc(Ihandle *ih) {
	IupSetCallback(ih, "DRAGDROP_CB", (Icallback) goIupDragDropCB);
}

extern int goIupSelectionCB(void *ih, int id, int status);
static void goIupSetSelectionFunc(Ihandle *ih) {
	IupSetCallback(ih, "SELECTION_CB", (Icallback) goIupSelectionCB);
}

extern int goIupBranchOpenCB(void *ih, int id);
static void goIupSetBranchOpenFunc(Ihandle *ih) {
	IupSetCallback(ih, "BRANCHOPEN_CB", (Icallback) goIupBranchOpenCB);
}

extern int goIupBranchCloseCB(void *ih, int id);
static void goIupSetBranchCloseFunc(Ihandle *ih) {
	IupSetCallback(ih, "BRANCHCLOSE_CB", (Icallback) goIupBranchCloseCB);
}

extern int goIupExecuteLeafCB(void *ih, int id);
static void goIupSetExecuteLeafFunc(Ihandle *ih) {
	IupSetCallback(ih, "EXECUTELEAF_CB", (Icallback) goIupExecuteLeafCB);
}

extern int goIupExecuteBranchCB(void *ih, int id);
static void goIupSetExecuteBranchFunc(Ihandle *ih) {
	IupSetCallback(ih, "EXECUTEBRANCH_CB", (Icallback) goIupExecuteBranchCB);
}

extern int goIupShowRenameCB(void *ih, int id);
static void goIupSetShowRenameFunc(Ihandle *ih) {
	IupSetCallback(ih, "SHOWRENAME_CB", (Icallback) goIupShowRenameCB);
}

extern int goIupRenameCB(void *, int id, void *title);
static void goIupSetRenameFunc(Ihandle *ih) {
	IupSetCallback(ih, "RENAME_CB", (Icallback) goIupRenameCB);
}

extern int goIupToggleValueCB(void *ih, int id, int state);
static void goIupSetToggleValueFunc(Ihandle *ih) {
	IupSetCallback(ih, "TOGGLEVALUE_CB", (Icallback) goIupToggleValueCB);
}

extern int goIupNodeRemovedCB(void *, void *userData);
static void goIupSetNodeRemovedFunc(Ihandle *ih) {
	IupSetCallback(ih, "NODEREMOVED_CB", (Icallback) goIupNodeRemovedCB);
}

extern int goIupMultiSelectionCB(void *ih, int* ids, int n);
static void goIupSetMultiSelectionFunc(Ihandle *ih) {
	IupSetCallback(ih, "MULTISELECTION_CB", (Icallback) goIupMultiSelectionCB);
}

extern int goIupMultiUnselectionCB(void *ih, int* ids, int n);
static void goIupSetMultiUnselectionFunc(Ihandle *ih) {
	IupSetCallback(ih, "MULTIUNSELECTION_CB", (Icallback) goIupMultiUnselectionCB);
}

extern int goIupMenuOpenCB(void *);
static void goIupSetMenuOpenFunc(Ihandle *ih) {
	IupSetCallback(ih, "OPEN_CB", (Icallback) goIupMenuOpenCB);
}

extern int goIupThemeChangedCB(void *, int darkMode);
static void goIupSetThemeChangedFunc(Ihandle *ih) {
	IupSetCallback(ih, "THEMECHANGED_CB", (Icallback) goIupThemeChangedCB);
}

extern int goIupCompletedCB(void *, void *url);
static void goIupSetCompletedFunc(Ihandle *ih) {
	IupSetCallback(ih, "COMPLETED_CB", (Icallback) goIupCompletedCB);
}

extern int goIupErrorCB(void *, void *url);
static void goIupSetErrorFunc(Ihandle *ih) {
	IupSetCallback(ih, "ERROR_CB", (Icallback) goIupErrorCB);
}

extern int goIupNavigateCB(void *, void *url);
static void goIupSetNavigateFunc(Ihandle *ih) {
	IupSetCallback(ih, "NAVIGATE_CB", (Icallback) goIupNavigateCB);
}

extern int goIupNewWindowCB(void *, void *url);
static void goIupSetNewWindowFunc(Ihandle *ih) {
	IupSetCallback(ih, "NEWWINDOW_CB", (Icallback) goIupNewWindowCB);
}

extern int goIupUpdateCB(void *);
static void goIupSetUpdateFunc(Ihandle *ih) {
	IupSetCallback(ih, "UPDATE_CB", (Icallback) goIupUpdateCB);
}

// ============================================================================
// CELLS AND MATRIX CONTROL CALLBACKS
// ============================================================================

extern int goIupCellsDrawCB(void *, int i, int j, int xmin, int xmax, int ymin, int ymax);
static void goIupSetCellsDrawFunc(Ihandle *ih) {
	IupSetCallback(ih, "DRAW_CB", (Icallback) goIupCellsDrawCB);
}

extern int goIupMouseClickCB(void *, int button, int pressed, int i, int j, int x, int y, void *status);
static void goIupSetMouseClickFunc(Ihandle *ih) {
	IupSetCallback(ih, "MOUSECLICK_CB", (Icallback) goIupMouseClickCB);
}

extern int goIupMouseMotionCB(void *, int i, int j, int x, int y, void *status);
static void goIupSetMouseMotionFunc(Ihandle *ih) {
	IupSetCallback(ih, "MOUSEMOTION_CB", (Icallback) goIupMouseMotionCB);
}

extern int goIupScrollingCB(void *, int i, int j);
static void goIupSetScrollingFunc(Ihandle *ih) {
	IupSetCallback(ih, "SCROLLING_CB", (Icallback) goIupScrollingCB);
}

extern int goIupNColsCB(void *);
static void goIupSetNColsFunc(Ihandle *ih) {
	IupSetCallback(ih, "NCOLS_CB", (Icallback) goIupNColsCB);
}

extern int goIupNLinesCB(void *);
static void goIupSetNLinesFunc(Ihandle *ih) {
	IupSetCallback(ih, "NLINES_CB", (Icallback) goIupNLinesCB);
}

extern int goIupHSpanCB(void *, int i, int j);
static void goIupSetHSpanFunc(Ihandle *ih) {
	IupSetCallback(ih, "HSPAN_CB", (Icallback) goIupHSpanCB);
}

extern int goIupVSpanCB(void *, int i, int j);
static void goIupSetVSpanFunc(Ihandle *ih) {
	IupSetCallback(ih, "VSPAN_CB", (Icallback) goIupVSpanCB);
}

extern int goIupHeightCB(void *, int i);
static void goIupSetHeightFunc(Ihandle *ih) {
	IupSetCallback(ih, "HEIGHT_CB", (Icallback) goIupHeightCB);
}

extern int goIupWidthCB(void *, int j);
static void goIupSetWidthFunc(Ihandle *ih) {
	IupSetCallback(ih, "WIDTH_CB", (Icallback) goIupWidthCB);
}

extern int goIupMatrixDrawCB(void *, int lin, int col, int x1, int x2, int y1, int y2);
static void goIupSetMatrixDrawFunc(Ihandle *ih) {
	IupSetCallback(ih, "DRAW_CB", (Icallback) goIupMatrixDrawCB);
}

extern int goIupEnterItemCB(void *, int lin, int col);
static void goIupSetEnterItemFunc(Ihandle *ih) {
	IupSetCallback(ih, "ENTERITEM_CB", (Icallback) goIupEnterItemCB);
}

extern int goIupLeaveItemCB(void *, int lin, int col);
static void goIupSetLeaveItemFunc(Ihandle *ih) {
	IupSetCallback(ih, "LEAVEITEM_CB", (Icallback) goIupLeaveItemCB);
}

extern int goIupClickCB(void *, int lin, int col, void *status);
static void goIupSetClickFunc(Ihandle *ih) {
	IupSetCallback(ih, "CLICK_CB", (Icallback) goIupClickCB);
}

extern int goIupReleaseCB(void *, int lin, int col, void *status);
static void goIupSetReleaseFunc(Ihandle *ih) {
	IupSetCallback(ih, "RELEASE_CB", (Icallback) goIupReleaseCB);
}

extern int goIupMatrixMouseMoveCB(void *, int lin, int col);
static void goIupSetMatrixMouseMoveFunc(Ihandle *ih) {
	IupSetCallback(ih, "MOUSEMOVE_CB", (Icallback) goIupMatrixMouseMoveCB);
}

extern int goIupEditionCB(void *, int lin, int col, int mode, int update);
static void goIupSetEditionFunc(Ihandle *ih) {
	IupSetCallback(ih, "EDITION_CB", (Icallback) goIupEditionCB);
}

extern int goIupTableEditionCB(void *, int lin, int col, char* update);
static void goIupSetTableEditionFunc(Ihandle *ih) {
	IupSetCallback(ih, "EDITION_CB", (Icallback) goIupTableEditionCB);
}

extern int goIupEditBeginCB(void *, int lin, int col);
static void goIupSetEditBeginFunc(Ihandle *ih) {
	IupSetCallback(ih, "EDITBEGIN_CB", (Icallback) goIupEditBeginCB);
}

extern int goIupEditEndCB(void *, int lin, int col, char* newValue, int apply);
static void goIupSetEditEndFunc(Ihandle *ih) {
	IupSetCallback(ih, "EDITEND_CB", (Icallback) goIupEditEndCB);
}

extern int goIupTableValueChangedCB(void *, int lin, int col);
static void goIupSetTableValueChangedFunc(Ihandle *ih) {
	IupSetCallback(ih, "VALUECHANGED_CB", (Icallback) goIupTableValueChangedCB);
}

extern char* goIupTableValueCB(void *, int lin, int col);
static void goIupSetTableValueFunc(Ihandle *ih) {
	IupSetCallback(ih, "VALUE_CB", (Icallback) goIupTableValueCB);
}

extern int goIupTableSortCB(void *, int col);
static void goIupSetTableSortFunc(Ihandle *ih) {
	IupSetCallback(ih, "SORT_CB", (Icallback) goIupTableSortCB);
}

extern int goIupDropCheckCB(void *, int lin, int col);
static void goIupSetDropCheckFunc(Ihandle *ih) {
	IupSetCallback(ih, "DROPCHECK_CB", (Icallback) goIupDropCheckCB);
}

extern int goIupMarkCB(void *, int lin, int col);
static void goIupSetMarkFunc(Ihandle *ih) {
	IupSetCallback(ih, "MARK_CB", (Icallback) goIupMarkCB);
}

extern int goIupMarkEditCB(void *, int lin, int col, int marked);
static void goIupSetMarkEditFunc(Ihandle *ih) {
	IupSetCallback(ih, "MARKEDIT_CB", (Icallback) goIupMarkEditCB);
}

extern int goIupValueEditCB(void *, int lin, int col, void *newval);
static void goIupSetValueEditFunc(Ihandle *ih) {
	IupSetCallback(ih, "VALUE_EDIT_CB", (Icallback) goIupValueEditCB);
}

extern int goIupColResizeCB(void *, int col);
static void goIupSetColResizeFunc(Ihandle *ih) {
	IupSetCallback(ih, "COLRESIZE_CB", (Icallback) goIupColResizeCB);
}

extern int goIupResizeMatrixCB(void *, int width, int height);
static void goIupSetResizeMatrixFunc(Ihandle *ih) {
	IupSetCallback(ih, "RESIZEMATRIX_CB", (Icallback) goIupResizeMatrixCB);
}

extern int goIupScrollTopCB(void *, int lin, int col);
static void goIupSetScrollTopFunc(Ihandle *ih) {
	IupSetCallback(ih, "SCROLLTOP_CB", (Icallback) goIupScrollTopCB);
}

extern int goIupBgColorCB(void *, int lin, int col, int *r, int *g, int *b);
static void goIupSetBgColorFunc(Ihandle *ih) {
	IupSetCallback(ih, "BGCOLOR_CB", (Icallback) goIupBgColorCB);
}

extern int goIupFgColorCB(void *, int lin, int col, int *r, int *g, int *b);
static void goIupSetFgColorFunc(Ihandle *ih) {
	IupSetCallback(ih, "FGCOLOR_CB", (Icallback) goIupFgColorCB);
}

extern int goIupMatrixDropCB(void *, void *drop, int lin, int col);
static void goIupSetMatrixDropFunc(Ihandle *ih) {
	IupSetCallback(ih, "DROP_CB", (Icallback) goIupMatrixDropCB);
}

extern int goIupMatrixActionCB(void *, int key, int lin, int col, int edition, char *status);
static void goIupSetMatrixActionFunc(Ihandle *ih) {
	IupSetCallback(ih, "ACTION_CB", (Icallback) goIupMatrixActionCB);
}

extern int goIupEditClickCB(void *, int lin, int col, char *status);
static void goIupSetEditClickFunc(Ihandle *ih) {
	IupSetCallback(ih, "EDITCLICK_CB", (Icallback) goIupEditClickCB);
}

extern int goIupEditReleaseCB(void *, int lin, int col, char *status);
static void goIupSetEditReleaseFunc(Ihandle *ih) {
	IupSetCallback(ih, "EDITRELEASE_CB", (Icallback) goIupEditReleaseCB);
}

extern int goIupEditMouseMoveCB(void *, int lin, int col);
static void goIupSetEditMouseMoveFunc(Ihandle *ih) {
	IupSetCallback(ih, "EDITMOUSEMOVE_CB", (Icallback) goIupEditMouseMoveCB);
}

extern char* goIupMatrixFontCB(void *, int lin, int col);
static void goIupSetMatrixFontFunc(Ihandle *ih) {
	IupSetCallback(ih, "FONT_CB", (Icallback) goIupMatrixFontCB);
}

extern char* goIupMatrixTypeCB(void *, int lin, int col);
static void goIupSetMatrixTypeFunc(Ihandle *ih) {
	IupSetCallback(ih, "TYPE_CB", (Icallback) goIupMatrixTypeCB);
}

extern char* goIupTranslateValueCB(void *, int lin, int col, char *value);
static void goIupSetTranslateValueFunc(Ihandle *ih) {
	IupSetCallback(ih, "TRANSLATEVALUE_CB", (Icallback) goIupTranslateValueCB);
}

extern int goIupMenuDropCB(void *, void *menu, int lin, int col);
static void goIupSetMenuDropFunc(Ihandle *ih) {
	IupSetCallback(ih, "MENUDROP_CB", (Icallback) goIupMenuDropCB);
}

extern int goIupDropSelectCB(void *, int lin, int col, void *drop, char *text, int item, int col2);
static void goIupSetDropSelectFunc(Ihandle *ih) {
	IupSetCallback(ih, "DROPSELECT_CB", (Icallback) goIupDropSelectCB);
}

extern char* goIupMatrixValueCB(void *, int lin, int col);
static void goIupSetMatrixValueFunc(Ihandle *ih) {
	IupSetCallback(ih, "VALUE_CB", (Icallback) goIupMatrixValueCB);
}

extern int goIupMatrixToggleValueCB(void *, int lin, int col, int value);
static void goIupSetMatrixToggleValueFunc(Ihandle *ih) {
	IupSetCallback(ih, "TOGGLEVALUE_CB", (Icallback) goIupMatrixToggleValueCB);
}

extern int goIupMatrixListActionCB(void *, int item, int state);
static void goIupSetMatrixListActionFunc(Ihandle *ih, char *name) {
	IupSetCallback(ih, name, (Icallback) goIupMatrixListActionCB);
}

*/
import "C"

var (
	callbacks sync.Map
	messages  atomic.Int64
)

//--------------------

// IdleFunc for IDLE_ACTION callback.
// generated when there are no events or messages to be processed. Often used to perform background operations.
//
// https://www.tecgraf.puc-rio.br/iup/en/call/iup_idle_action.html
type IdleFunc func() int

//export goIupIdleCB
func goIupIdleCB() C.int {
	h, ok := callbacks.Load("IDLE_ACTION")
	if !ok {
		panic("cannot load callback " + "IDLE_ACTION")
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(IdleFunc)

	return C.int(f())
}

// setIdleFunc for IDLE_ACTION.
func setIdleFunc(f IdleFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("IDLE_ACTION", ch)

	C.goIupSetIdleFunc()
}

//--------------------

// EntryPointFunc for ENTRY_POINT callback.
// generated when there are no events or messages to be processed. Often used to perform background operations.
//
// https://www.tecgraf.puc-rio.br/iup/en/call/iup_entry_point.html
type EntryPointFunc func()

//export goIupEntryPointCB
func goIupEntryPointCB() {
	h, ok := callbacks.Load("ENTRY_POINT")
	if !ok {
		panic("cannot load callback " + "ENTRY_POINT")
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(EntryPointFunc)

	f()
}

// setEntryPointFunc for ENTRY_POINT.
func setEntryPointFunc(f EntryPointFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("ENTRY_POINT", ch)

	C.goIupSetEntryPointFunc()
}

//--------------------

// ExitFunc for EXIT_CB callback.
// Global callback for an exit. Used when main is not possible, such as in iOS and Android systems.
//
// https://www.tecgraf.puc-rio.br/iup/en/call/iup_exit_cb.html
type ExitFunc func()

//export goIupExitCB
func goIupExitCB() {
	h, ok := callbacks.Load("EXIT_CB")
	if !ok {
		panic("cannot load callback " + "EXIT_CB")
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(ExitFunc)

	f()
}

// setExitFunc for EXIT_CB.
func setExitFunc(f ExitFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("EXIT_CB", ch)

	C.goIupSetExitFunc()
}

//--------------------

// MapFunc for MAP_CB callback.
// Called right after an element is mapped and its attributes updated in Map.
// When the element is a dialog, it is called after the layout is updated. For all other elements is called before the layout is updated.
//
// https://www.tecgraf.puc-rio.br/iup/en/call/iup_map_cb.html
type MapFunc func(Ihandle) int

//export goIupMapCB
func goIupMapCB(ih unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("MAP_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "MAP_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(MapFunc)

	return C.int(f((Ihandle)(ih)))

}

// setMapFunc for MAP_CB.
func setMapFunc(ih Ihandle, f MapFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MAP_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetMapFunc(ih.ptr())
}

//--------------------

// UnmapFunc for UNMAP_CB callback.
// Called right before an element is unmapped.
//
// https://www.tecgraf.puc-rio.br/iup/en/call/iup_unmap_cb.html
type UnmapFunc func(Ihandle) int

//export goIupUnmapCB
func goIupUnmapCB(ih unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("UNMAP_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "UNMAP_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(UnmapFunc)

	return C.int(f((Ihandle)(ih)))
}

// setUnmapFunc for UNMAP_CB.
func setUnmapFunc(ih Ihandle, f UnmapFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("UNMAP_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetUnmapFunc(ih.ptr())
}

//--------------------

// DestroyFunc for DESTROY_CB callback.
// Called right before an element is destroyed.
//
// https://www.tecgraf.puc-rio.br/iup/en/call/iup_destroy_cb.html
type DestroyFunc func(Ihandle) int

//export goIupDestroyCB
func goIupDestroyCB(ih unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("DESTROY_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "DESTROY_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(DestroyFunc)

	return C.int(f((Ihandle)(ih)))
}

// setDestroyFunc for DESTROY_CB.
func setDestroyFunc(ih Ihandle, f DestroyFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("DESTROY_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetDestroyFunc(ih.ptr())
}

//--------------------

// GetFocusFunc for GETFOCUS_CB callback.
// Action generated when an element is given keyboard focus.
//
// https://www.tecgraf.puc-rio.br/iup/en/call/iup_getfocus_cb.html
type GetFocusFunc func(Ihandle) int

//export goIupGetFocusCB
func goIupGetFocusCB(ih unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("GETFOCUS_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "GETFOCUS_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(GetFocusFunc)

	return C.int(f((Ihandle)(ih)))
}

// setGetFocusFunc for GETFOCUS_CB.
func setGetFocusFunc(ih Ihandle, f GetFocusFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("GETFOCUS_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetGetFocusFunc(ih.ptr())
}

//--------------------

// KillFocusFunc for KILLFOCUS_CB callback.
// Action generated when an element loses keyboard focus.
//
// https://www.tecgraf.puc-rio.br/iup/en/call/iup_killfocus_cb.html
type KillFocusFunc func(Ihandle) int

//export goIupKillFocusCB
func goIupKillFocusCB(ih unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("KILLFOCUS_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "KILLFOCUS_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(KillFocusFunc)

	return C.int(f((Ihandle)(ih)))
}

// setKillFocusFunc for KILLFOCUS_CB.
func setKillFocusFunc(ih Ihandle, f KillFocusFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("KILLFOCUS_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetKillFocusFunc(ih.ptr())
}

//--------------------

// EnterWindowFunc for ENTERWINDOW_CB callback.
// Action generated when the mouse enters the native element.
//
// https://www.tecgraf.puc-rio.br/iup/en/call/iup_enterwindow_cb.html
type EnterWindowFunc func(Ihandle) int

//export goIupEnterWindowCB
func goIupEnterWindowCB(ih unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("ENTERWINDOW_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "ENTERWINDOW_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(EnterWindowFunc)

	return C.int(f((Ihandle)(ih)))
}

// setEnterWindowFunc for ENTERWINDOW_CB.
func setEnterWindowFunc(ih Ihandle, f EnterWindowFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("ENTERWINDOW_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetEnterWindowFunc(ih.ptr())
}

//--------------------

// LeaveWindowFunc for LEAVEWINDOW_CB callback.
// Action generated when the mouse leaves the native element.
//
// https://www.tecgraf.puc-rio.br/iup/en/call/iup_leavewindow_cb.html
type LeaveWindowFunc func(Ihandle) int

//export goIupLeaveWindowCB
func goIupLeaveWindowCB(ih unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("LEAVEWINDOW_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "LEAVEWINDOW_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(LeaveWindowFunc)

	return C.int(f((Ihandle)(ih)))
}

// setLeaveWindowFunc for LEAVEWINDOW_CB.
func setLeaveWindowFunc(ih Ihandle, f LeaveWindowFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("LEAVEWINDOW_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetLeaveWindowFunc(ih.ptr())
}

//--------------------

// KAnyFunc for K_ANY callback.
// Action generated when a keyboard event occurs.
//
// https://www.tecgraf.puc-rio.br/iup/en/call/iup_k_any.html
type KAnyFunc func(Ihandle, int) int

//export goIupKAnyCB
func goIupKAnyCB(ih unsafe.Pointer, c C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("K_ANY_" + uuid)
	if !ok {
		panic("cannot load callback " + "K_ANY_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(KAnyFunc)

	return C.int(f((Ihandle)(ih), int(c)))
}

// setKAnyFunc for K_ANY.
func setKAnyFunc(ih Ihandle, f KAnyFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("K_ANY_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetKAnyFunc(ih.ptr())
}

//--------------------

// HelpFunc for HELP_CB callback.
// Action generated when the user press F1 at a control.
//
// https://www.tecgraf.puc-rio.br/iup/en/call/iup_help_cb.html
type HelpFunc func(Ihandle) int

//export goIupHelpCB
func goIupHelpCB(ih unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("HELP_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "HELP_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(HelpFunc)

	return C.int(f((Ihandle)(ih)))
}

// setHelpFunc for HELP_CB.
func setHelpFunc(ih Ihandle, f HelpFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("HELP_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetHelpFunc(ih.ptr())
}

//--------------------

// ActionFunc for ACTION callback.
// Action generated when the element is activated. Affects each element differently.
//
// https://www.tecgraf.puc-rio.br/iup/en/call/iup_action.html
type ActionFunc func(Ihandle) int

//export goIupActionCB
func goIupActionCB(ih unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("ACTION_" + uuid)
	if !ok {
		panic("cannot load callback " + "ACTION_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(ActionFunc)

	return C.int(f((Ihandle)(ih)))
}

// setActionFunc for ACTION.
func setActionFunc(ih Ihandle, f ActionFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("ACTION_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetActionFunc(ih.ptr())
}

//--------------------

// FlatActionFunc for FLAT_ACTION callback.
// Action generated when the button 1 (usually left) is selected.
type FlatActionFunc func(ih Ihandle) int

//export goIupFlatActionCB
func goIupFlatActionCB(ih unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("FLAT_ACTION_" + uuid)
	if !ok {
		panic("cannot load callback " + "FLAT_ACTION_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(FlatActionFunc)

	return C.int(f((Ihandle)(ih)))
}

// setFlatActionFunc for FLAT_ACTION callback.
func setFlatActionFunc(ih Ihandle, f FlatActionFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("FLAT_ACTION_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetFlatActionFunc(ih.ptr())
}

//--------------------

// ButtonFunc for BUTTON_CB callback.
// Action generated when a mouse button is pressed or released.
//
// https://www.tecgraf.puc-rio.br/iup/en/call/iup_button_cb.html
type ButtonFunc func(Ihandle, int, int, int, int, string) int

//export goIupButtonCB
func goIupButtonCB(ih unsafe.Pointer, button, pressed, x, y C.int, status unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("BUTTON_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "BUTTON_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(ButtonFunc)

	goStatus := C.GoString((*C.char)(status))
	return C.int(f((Ihandle)(ih), int(button), int(pressed), int(x), int(y), goStatus))
}

// setButtonFunc for BUTTON_CB.
func setButtonFunc(ih Ihandle, f ButtonFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("BUTTON_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetButtonFunc(ih.ptr())
}

//--------------------

// DropFilesFunc for DROPFILES_CB callback.
// Action called when a file is "dropped" into the control.
//
// https://www.tecgraf.puc-rio.br/iup/en/call/iup_dropfiles_cb.html
type DropFilesFunc func(Ihandle, string, int, int, int) int

//export goIupDropFilesCB
func goIupDropFilesCB(ih, filename unsafe.Pointer, num, x, y C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("DROPFILES_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "DROPFILES_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(DropFilesFunc)

	goFilename := C.GoString((*C.char)(filename))
	return C.int(f((Ihandle)(ih), goFilename, int(num), int(x), int(y)))
}

// setDropFilesFunc for DROPFILE_CB.
func setDropFilesFunc(ih Ihandle, f DropFilesFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("DROPFILES_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetDropFilesFunc(ih.ptr())
}

//--------------------

// ListActionFunc for List ACTION callback.
// Action generated when the state of an item in the list is changed.
type ListActionFunc func(ih Ihandle, text string, item, state int) int

//export goIupListActionCB
func goIupListActionCB(ih, text unsafe.Pointer, item, state C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("LIST_ACTION_" + uuid)
	if !ok {
		panic("cannot load callback " + "LIST_ACTION_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(ListActionFunc)

	goText := C.GoString((*C.char)(text))
	return C.int(f((Ihandle)(ih), goText, int(item), int(state)))
}

// setListActionFunc for List ACTION callback.
func setListActionFunc(ih Ihandle, f ListActionFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("LIST_ACTION_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetListActionFunc(ih.ptr())
}

//--------------------

// FlatListActionFunc for FlatList FLAT_ACTION callback.
// Action generated when the state of an item in the list is interactively changed.
type FlatListActionFunc func(ih Ihandle, text string, item, state int) int

//export goIupFlatListActionCB
func goIupFlatListActionCB(ih, text unsafe.Pointer, item, state C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("FLAT_LIST_ACTION_" + uuid)
	if !ok {
		panic("cannot load callback " + "FLAT_LIST_ACTION_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(FlatListActionFunc)

	goText := C.GoString((*C.char)(text))
	return C.int(f((Ihandle)(ih), goText, int(item), int(state)))
}

// setFlatListActionFunc for FlatList FLAT_ACTION callback.
func setFlatListActionFunc(ih Ihandle, f FlatListActionFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("FLAT_LIST_ACTION_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetFlatListActionFunc(ih.ptr())
}

//--------------------

// CaretFunc for CARET_CB callback.
// Action generated when the caret/cursor position is changed.
type CaretFunc func(ih Ihandle, lin, col, pos int) int

//export goIupCaretCB
func goIupCaretCB(ih unsafe.Pointer, lin, col, pos C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("CARET_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "CARET_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(CaretFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col), int(pos)))
}

// setCaretFunc for CARET_CB.
func setCaretFunc(ih Ihandle, f CaretFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("CARET_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetCaretFunc(ih.ptr())
}

//--------------------

// DblclickFunc for DBLCLICK_CB callback.
// Action generated when the user double click an item.
type DblclickFunc func(ih Ihandle, item int, text string) int

//export goIupDblclickCB
func goIupDblclickCB(ih unsafe.Pointer, item C.int, text unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("DBLCLICK_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "DBLCLICK_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(DblclickFunc)

	goText := C.GoString((*C.char)(text))
	return C.int(f((Ihandle)(ih), int(item), goText))
}

// setDblclickFunc for DBLCLICK_CB.
func setDblclickFunc(ih Ihandle, f DblclickFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("DBLCLICK_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetDblclickFunc(ih.ptr())
}

//--------------------

// EditFunc for EDIT_CB callback.
// Action generated when the user double click an item.
type EditFunc func(ih Ihandle, item int, text string) int

//export goIupEditCB
func goIupEditCB(ih unsafe.Pointer, item C.int, text unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("EDIT_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "EDIT_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(EditFunc)

	goText := C.GoString((*C.char)(text))
	return C.int(f((Ihandle)(ih), int(item), goText))
}

// setEditFunc for EDIT_CB.
func setEditFunc(ih Ihandle, f EditFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("EDIT_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetEditFunc(ih.ptr())
}

//--------------------

// MotionFunc for MOTION_CB callback.
// Action generated when the mouse is moved over the list.
type MotionFunc func(ih Ihandle, x, y int, status string) int

//export goIupMotionCB
func goIupMotionCB(ih unsafe.Pointer, x, y C.int, status unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("MOTION_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "MOTION_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(MotionFunc)

	goStatus := C.GoString((*C.char)(status))
	return C.int(f((Ihandle)(ih), int(x), int(y), goStatus))
}

// setMotionFunc for MOTION_CB.
func setMotionFunc(ih Ihandle, f MotionFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MOTION_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetMotionFunc(ih.ptr())
}

//--------------------

// MultiselectFunc for MULTISELECT_CB callback.
// Action generated when the state of an item in the multiple selection list is changed.
type MultiselectFunc func(ih Ihandle, text string) int

//export goIupMultiselectCB
func goIupMultiselectCB(ih, text unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("MULTISELECT_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "MULTISELECT_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(MultiselectFunc)

	goText := C.GoString((*C.char)(text))
	return C.int(f((Ihandle)(ih), goText))
}

// setMultiselectFunc for MULTISELECT_CB.
func setMultiselectFunc(ih Ihandle, f MultiselectFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MULTISELECT_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetMultiselectFunc(ih.ptr())
}

//--------------------

// ValueChangedFunc for VALUECHANGED_CB callback.
// Called after the value was interactively changed by the user.
type ValueChangedFunc func(ih Ihandle) int

//export goIupValueChangedCB
func goIupValueChangedCB(ih unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("VALUECHANGED_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "VALUECHANGED_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(ValueChangedFunc)

	return C.int(f((Ihandle)(ih)))
}

// setValueChangedFunc for VALUECHANGED_CB.
func setValueChangedFunc(ih Ihandle, f ValueChangedFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("VALUECHANGED_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetValueChangedFunc(ih.ptr())
}

//--------------------

// TextActionFunc for Text ACTION callback.
// Action generated when the text is edited, but before its value is actually changed.
type TextActionFunc func(ih Ihandle, ch int, newValue string) int

//export goIupTextActionCB
func goIupTextActionCB(ih unsafe.Pointer, c C.int, newValue unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("TEXT_ACTION_" + uuid)
	if !ok {
		panic("cannot load callback " + "TEXT_ACTION_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(TextActionFunc)

	goNewValue := C.GoString((*C.char)(newValue))
	return C.int(f((Ihandle)(ih), int(c), goNewValue))
}

// setTextActionFunc for Text ACTION.
func setTextActionFunc(ih Ihandle, f TextActionFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("TEXT_ACTION_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetTextActionFunc(ih.ptr())
}

//--------------------

// ToggleActionFunc for Toggle ACTION callback.
// Action generated when the toggle's state (on/off) was changed.
type ToggleActionFunc func(ih Ihandle, state int) int

//export goIupToggleActionCB
func goIupToggleActionCB(ih unsafe.Pointer, state C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("TOGGLE_ACTION_" + uuid)
	if !ok {
		panic("cannot load callback " + "TOGGLE_ACTION_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(ToggleActionFunc)

	return C.int(f((Ihandle)(ih), int(state)))
}

// setToggleActionFunc for Toggle ACTION.
func setToggleActionFunc(ih Ihandle, f ToggleActionFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("TOGGLE_ACTION_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetToggleActionFunc(ih.ptr())
}

//--------------------

// FlatToggleActionFunc for FlatToggle FLAT_ACTION callback.
// Action generated when the toggle's state (on/off) was changed.
type FlatToggleActionFunc func(ih Ihandle, state int) int

//export goIupFlatToggleActionCB
func goIupFlatToggleActionCB(ih unsafe.Pointer, state C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("FLAT_TOGGLE_ACTION_" + uuid)
	if !ok {
		panic("cannot load callback " + "FLAT_TOGGLE_ACTION_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(FlatToggleActionFunc)

	return C.int(f((Ihandle)(ih), int(state)))
}

// setFlatToggleActionFunc for FlatToggle FLAT_ACTION.
func setFlatToggleActionFunc(ih Ihandle, f FlatToggleActionFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("FLAT_TOGGLE_ACTION_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetFlatToggleActionFunc(ih.ptr())
}

//--------------------

// TabChangeFunc for TABCHANGE_CB callback.
type TabChangeFunc func(ih, new_tab, old_tab Ihandle) int

//export goIupTabChangeCB
func goIupTabChangeCB(ih, new_tab, old_tab unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("TABCHANGE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "TABCHANGE_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(TabChangeFunc)

	return C.int(f((Ihandle)(ih), (Ihandle)(new_tab), (Ihandle)(old_tab)))
}

// setTabChangeFunc for TABCHANGE_CB.
func setTabChangeFunc(ih Ihandle, f TabChangeFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("TABCHANGE_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetTabChangeFunc(ih.ptr())
}

//--------------------

// TabChangePosFunc for TABCHANGEPOS_CB callback.
type TabChangePosFunc func(ih Ihandle, new_pos, old_pos int) int

//export goIupTabChangePosCB
func goIupTabChangePosCB(ih unsafe.Pointer, new_pos, old_pos C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("TABCHANGEPOS_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "TABCHANGEPOS_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(TabChangePosFunc)

	return C.int(f((Ihandle)(ih), int(new_pos), int(old_pos)))
}

// setTabChangePosFunc for TABCHANGEPOS_CB.
func setTabChangePosFunc(ih Ihandle, f TabChangePosFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("TABCHANGEPOS_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetTabChangePosFunc(ih.ptr())
}

//--------------------

// SpinFunc for SPIN_CB callback.
type SpinFunc func(ih Ihandle, inc int) int

//export goIupSpinCB
func goIupSpinCB(ih unsafe.Pointer, inc C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("SPIN_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "SPIN_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(SpinFunc)

	return C.int(f((Ihandle)(ih), int(inc)))
}

// setSpinFunc for SPIN_CB.
func setSpinFunc(ih Ihandle, f SpinFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("SPIN_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetSpinFunc(ih.ptr())
}

//--------------------

// PostMessageFunc for POSTMESSAGE_CB callback.
type PostMessageFunc func(Ihandle, string, int, any) int

//export goIupPostMessageCB
func goIupPostMessageCB(ih unsafe.Pointer, s unsafe.Pointer, i C.int, d C.double, p unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("POSTMESSAGE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "POSTMESSAGE_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(PostMessageFunc)

	m, ok := callbacks.LoadAndDelete(fmt.Sprintf("POSTMESSAGE_MSG_%s_%d", uuid, int(d)))
	if !ok {
		panic("cannot load and delete message " + fmt.Sprintf("POSTMESSAGE_MSG_%s_%d", uuid, int(d)))
	}

	return C.int(f((Ihandle)(ih), C.GoString((*C.char)(s)), int(i), m))
}

// setPostMessageFunc for POSTMESSAGE_CB.
func setPostMessageFunc(ih Ihandle, f PostMessageFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("POSTMESSAGE_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetPostMessageFunc(ih.ptr())
}

//--------------------

// CloseFunc for CLOSE_CB callback.
// Called just before a dialog is closed when the user clicks the close button
// of the title bar or an equivalent action.
type CloseFunc func(Ihandle) int

//export goIupCloseCB
func goIupCloseCB(ih unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("CLOSE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "CLOSE_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(CloseFunc)

	return C.int(f((Ihandle)(ih)))
}

// setCloseFunc for CLOSE_CB.
func setCloseFunc(ih Ihandle, f CloseFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("CLOSE_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetCloseFunc(ih.ptr())
}

//--------------------

// FocusFunc for FOCUS_CB callback.
// Called when the dialog or any of its children gets the focus, or when
// another dialog or any control in another dialog gets the focus.
type FocusFunc func(Ihandle, int) int

//export goIupFocusCB
func goIupFocusCB(ih unsafe.Pointer, c C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("FOCUS_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "FOCUS_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(FocusFunc)

	return C.int(f((Ihandle)(ih), int(c)))
}

// setFocusFunc for FOCUS_CB.
func setFocusFunc(ih Ihandle, f FocusFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("FOCUS_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetFocusFunc(ih.ptr())
}

//--------------------

// MoveFunc for MOVE_CB callback.
// Called after the dialog was moved on screen.
type MoveFunc func(ih Ihandle, x, y int) int

//export goIupMoveCB
func goIupMoveCB(ih unsafe.Pointer, x, y C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("MOVE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "MOVE_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(MoveFunc)

	return C.int(f((Ihandle)(ih), int(x), int(y)))
}

// setMoveFunc for MOVE_CB.
func setMoveFunc(ih Ihandle, f MoveFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MOVE_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetMoveFunc(ih.ptr())
}

//--------------------

// ResizeFunc for RESIZE_CB callback.
// Action generated when the canvas or dialog size is changed.
type ResizeFunc func(ih Ihandle, width, height int) int

//export goIupResizeCB
func goIupResizeCB(ih unsafe.Pointer, width, height C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("RESIZE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "RESIZE_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(ResizeFunc)

	return C.int(f((Ihandle)(ih), int(width), int(height)))
}

// setResizeFunc for RESIZE_CB.
func setResizeFunc(ih Ihandle, f ResizeFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("RESIZE_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetResizeFunc(ih.ptr())
}

//--------------------

// ShowFunc for SHOW_CB callback.
// Called right after the dialog is showed, hidden, maximized, minimized or restored from minimized/maximized.
type ShowFunc func(ih Ihandle, state int) int

//export goIupShowCB
func goIupShowCB(ih unsafe.Pointer, inc C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("SHOW_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "SHOW_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(ShowFunc)

	return C.int(f((Ihandle)(ih), int(inc)))
}

// setShowFunc for SHOW_CB.
func setShowFunc(ih Ihandle, f ShowFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("SHOW_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetShowFunc(ih.ptr())
}

//--------------------

// ChangeFunc for CHANGE_CB callback.
// Called when the user releases the left mouse button over the control, defining the selected color.
type ChangeFunc func(ih Ihandle, r, g, b uint8) int

//export goIupChangeCB
func goIupChangeCB(ih unsafe.Pointer, r, g, b C.uchar) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("CHANGE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "CHANGE_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(ChangeFunc)

	return C.int(f((Ihandle)(ih), uint8(r), uint8(g), uint8(b)))
}

// setChangeFunc for CHANGE_CB.
func setChangeFunc(ih Ihandle, f ChangeFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("CHANGE_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetChangeFunc(ih.ptr())
}

//--------------------

// DragFunc for DRAG_CB callback.
// Called several times while the color is being changed by dragging the mouse over the control.
type DragFunc func(ih Ihandle, r, g, b uint8) int

//export goIupDragCB
func goIupDragCB(ih unsafe.Pointer, r, g, b C.uchar) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("DRAG_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "DRAG_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(DragFunc)

	return C.int(f((Ihandle)(ih), uint8(r), uint8(g), uint8(b)))
}

// setDragFunc for DRAG_CB.
func setDragFunc(ih Ihandle, f DragFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("DRAG_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetDragFunc(ih.ptr())
}

//--------------------

// DetachedFunc for DETACHED_CB callback.
type DetachedFunc func(Ihandle, Ihandle, int, int) int

//export goIupDetachedCB
func goIupDetachedCB(ih, newParent unsafe.Pointer, x, y C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("DETACHED_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "DETACHED_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(DetachedFunc)

	return C.int(f((Ihandle)(ih), (Ihandle)(newParent), int(x), int(y)))
}

// setDetachedFunc for DETACHED_CB.
func setDetachedFunc(ih Ihandle, f DetachedFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("DETACHED_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetDetachedFunc(ih.ptr())
}

//--------------------

// RestoredFunc for RESTORED_CB callback.
// Called when the box is restored if RESTOREWHENCLOSED=Yes.
type RestoredFunc func(Ihandle, Ihandle, int, int) int

//export goIupRestoredCB
func goIupRestoredCB(ih, oldParent unsafe.Pointer, x, y C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("RESTORED_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "RESTORED_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(RestoredFunc)

	return C.int(f((Ihandle)(ih), (Ihandle)(oldParent), int(x), int(y)))
}

// setRestoredFunc for RESTORED_CB.
func setRestoredFunc(ih Ihandle, f RestoredFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("RESTORED_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetRestoredFunc(ih.ptr())
}

//--------------------

// SwapBuffersFunc for SWAPBUFFERS_CB callback.
// Action generated when GLSwapBuffers is called.
type SwapBuffersFunc func(Ihandle) int

//export goIupSwapBuffersCB
func goIupSwapBuffersCB(ih unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("SWAPBUFFERS_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "SWAPBUFFERS_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(SwapBuffersFunc)

	return C.int(f((Ihandle)(ih)))
}

// setSwapBuffersFunc for SWAPBUFFERS_CB.
func setSwapBuffersFunc(ih Ihandle, f SwapBuffersFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("SWAPBUFFERS_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetSwapBuffersFunc(ih.ptr())
}

//--------------------

// CancelFunc for CANCEL_CB callback.
// Action generated when the user clicked on the Cancel button.
type CancelFunc func(Ihandle) int

//export goIupCancelCB
func goIupCancelCB(ih unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("CANCEL_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "CANCEL_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(CancelFunc)

	return C.int(f((Ihandle)(ih)))
}

// setCancelFunc for CANCEL_CB.
func setCancelFunc(ih Ihandle, f CancelFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("CANCEL_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetCancelFunc(ih.ptr())
}

//--------------------

// TimerActionFunc for ACTION_CB callback.
// Called every time the defined time interval is reached.
// To stop the callback from being called simply stop the timer with RUN=NO.
type TimerActionFunc func(ih Ihandle) int

//export goIupTimerActionCB
func goIupTimerActionCB(ih unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("TIMER_ACTION_" + uuid)
	if !ok {
		panic("cannot load callback " + "TIMER_ACTION_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(TimerActionFunc)

	return C.int(f((Ihandle)(ih)))
}

// setTimerActionFunc for ACTION_CB callback.
func setTimerActionFunc(ih Ihandle, f TimerActionFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("TIMER_ACTION_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetTimerActionFunc(ih.ptr())
}

//--------------------

// ThreadFunc for THREAD_CB callback.
// Action generated when the thread is started.
// If this callback returns or does not exist the thread is terminated.
type ThreadFunc func(ih Ihandle) int

//export goIupThreadCB
func goIupThreadCB(ih unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("THREAD_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "THREAD_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(ThreadFunc)

	return C.int(f((Ihandle)(ih)))
}

// setThreadFunc for THREAD_CB callback.
func setThreadFunc(ih Ihandle, f ThreadFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("THREAD_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetThreadFunc(ih.ptr())
}

//--------------------

// ScrollFunc for SCROLL_CB callback.
// Called when the scrollbar is manipulated.
type ScrollFunc func(ih Ihandle, op int, posx, posy float64) int

//export goIupScrollCB
func goIupScrollCB(ih unsafe.Pointer, op C.int, posx, posy C.float) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("SCROLL_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "SCROLL_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(ScrollFunc)

	return C.int(f((Ihandle)(ih), int(op), float64(posx), float64(posy)))
}

// setScrollFunc for SCROLL_CB callback.
func setScrollFunc(ih Ihandle, f ScrollFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("SCROLL_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetScrollFunc(ih.ptr())
}

//--------------------

// TrayClickFunc for TRAYCLICK_CB callback.
// Called right after the mouse button is pressed or released over the tray icon.
type TrayClickFunc func(Ihandle, int, int, int) int

//export goIupTrayClickCB
func goIupTrayClickCB(ih unsafe.Pointer, but, pressed, dclick C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("TRAYCLICK_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "TRAYCLICK_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(TrayClickFunc)

	return C.int(f((Ihandle)(ih), int(but), int(pressed), int(dclick)))
}

// setTrayClickFunc for TRAYCLICK_CB.
func setTrayClickFunc(ih Ihandle, f TrayClickFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("TRAYCLICK_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetTrayClickFunc(ih.ptr())
}

//--------------------

// TabCloseFunc for TABCLOSE_CB callback.
// Called when the user clicks on the close button.
type TabCloseFunc func(Ihandle, int) int

//export goIupTabCloseCB
func goIupTabCloseCB(ih unsafe.Pointer, pos C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("TABCLOSE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "TABCLOSE_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(TabCloseFunc)

	return C.int(f((Ihandle)(ih), int(pos)))
}

// setTabCloseFunc for TABCLOSE_CB.
func setTabCloseFunc(ih Ihandle, f TabCloseFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("TABCLOSE_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetTabCloseFunc(ih.ptr())
}

//--------------------

// RightClickFunc for RIGHTCLICK_CB callback.
// Called when the user clicks on some tab using the right mouse button.
type RightClickFunc func(Ihandle, int) int

//export goIupRightClickCB
func goIupRightClickCB(ih unsafe.Pointer, pos C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("RIGHTCLICK_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "RIGHTCLICK_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(RightClickFunc)

	return C.int(f((Ihandle)(ih), int(pos)))
}

// setRightClickFunc for RIGHTCLICK_CB.
func setRightClickFunc(ih Ihandle, f RightClickFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("RIGHTCLICK_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetRightClickFunc(ih.ptr())
}

//--------------------

// ExtraButtonFunc for EXTRABUTTON_CB callback.
// Action generated when any mouse button is pressed or released.
type ExtraButtonFunc func(Ihandle, int, int) int

//export goIupExtraButtonCB
func goIupExtraButtonCB(ih unsafe.Pointer, button, pressed C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("EXTRABUTTON_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "EXTRABUTTON_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(ExtraButtonFunc)

	return C.int(f((Ihandle)(ih), int(button), int(pressed)))
}

// setExtraButtonFunc for EXTRABUTTON_CB.
func setExtraButtonFunc(ih Ihandle, f ExtraButtonFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("EXTRABUTTON_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetExtraButtonFunc(ih.ptr())
}

//--------------------

// OpenCloseFunc for OPENCLOSE_CB callback.
// Action generated before the expander state is interactively changed.
type OpenCloseFunc func(ih Ihandle, state int) int

//export goIupOpenCloseCB
func goIupOpenCloseCB(ih unsafe.Pointer, state C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("OPENCLOSE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "OPENCLOSE_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(OpenCloseFunc)

	return C.int(f((Ihandle)(ih), int(state)))
}

// setOpenCloseFunc for OPENCLOSE_CB.
func setOpenCloseFunc(ih Ihandle, f OpenCloseFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("OPENCLOSE_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetOpenCloseFunc(ih.ptr())
}

//--------------------

// ValueChangingFunc for VALUECHANGING_CB callback.
// Called when the value starts or ends to be interactively changed by the user.
type ValueChangingFunc func(ih Ihandle, start int) int

//export goIupValueChangingCB
func goIupValueChangingCB(ih unsafe.Pointer, start C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("VALUECHANGING_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "VALUECHANGING_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(ValueChangingFunc)

	return C.int(f((Ihandle)(ih), int(start)))
}

// setValueChangingFunc for VALUECHANGING_CB.
func setValueChangingFunc(ih Ihandle, f ValueChangingFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("VALUECHANGING_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetValueChangingFunc(ih.ptr())
}

//--------------------

// DropDownFunc for DROPDOWN_CB callback.
// Action generated right before the drop child is shown or hidden.
type DropDownFunc func(ih Ihandle, state int) int

//export goIupDropDownCB
func goIupDropDownCB(ih unsafe.Pointer, state C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("DROPDOWN_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "DROPDOWN_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(DropDownFunc)

	return C.int(f((Ihandle)(ih), int(state)))
}

// setDropDownFunc for DROPDOWN_CB.
func setDropDownFunc(ih Ihandle, f DropDownFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("DROPDOWN_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetDropDownFunc(ih.ptr())
}

//--------------------

// DropShowFunc for DROPSHOW_CB callback.
// Action generated right after the drop child is shown or hidden.
type DropShowFunc func(ih Ihandle, state int) int

//export goIupDropShowCB
func goIupDropShowCB(ih unsafe.Pointer, state C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("DROPSHOW_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "DROPSHOW_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(DropShowFunc)

	return C.int(f((Ihandle)(ih), int(state)))
}

// setDropShowFunc for DROPSHOW_CB.
func setDropShowFunc(ih Ihandle, f DropShowFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("DROPSHOW_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetDropShowFunc(ih.ptr())
}

//--------------------

// ButtonPressFunc for BUTTON_PRESS_CB callback.
// Called when the user presses the left mouse button over the dial.
type ButtonPressFunc func(ih Ihandle, angle float64) int

//export goIupButtonPressCB
func goIupButtonPressCB(ih unsafe.Pointer, angle C.double) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("BUTTON_PRESS_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "BUTTON_PRESS_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(ButtonPressFunc)

	return C.int(f((Ihandle)(ih), float64(angle)))
}

// setButtonPressFunc for BUTTON_PRESS_CB.
func setButtonPressFunc(ih Ihandle, f ButtonPressFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("BUTTON_PRESS_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetButtonPressFunc(ih.ptr())
}

//--------------------

// ButtonReleaseFunc for BUTTON_RELEASE_CB callback.
// Called when the user releases the left mouse button after pressing it over the dial.
type ButtonReleaseFunc func(ih Ihandle, angle float64) int

//export goIupButtonReleaseCB
func goIupButtonReleaseCB(ih unsafe.Pointer, angle C.double) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("BUTTON_RELEASE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "BUTTON_RELEASE_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(ButtonReleaseFunc)

	return C.int(f((Ihandle)(ih), float64(angle)))
}

// setButtonReleaseFunc for BUTTON_RELEASE_CB.
func setButtonReleaseFunc(ih Ihandle, f ButtonReleaseFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("BUTTON_RELEASE_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetButtonReleaseFunc(ih.ptr())
}

//--------------------

// MouseMoveFunc for MOUSEMOVE_CB callback.
type MouseMoveFunc func(ih Ihandle, angle float64) int

//export goIupMouseMoveCB
func goIupMouseMoveCB(ih unsafe.Pointer, angle C.double) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("MOUSEMOVE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "MOUSEMOVE_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(MouseMoveFunc)

	return C.int(f((Ihandle)(ih), float64(angle)))
}

// setMouseMoveFunc for MOUSEMOVE_CB.
func setMouseMoveFunc(ih Ihandle, f MouseMoveFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MOUSEMOVE_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetMouseMoveFunc(ih.ptr())
}

//--------------------

// KeyPressFunc for KEYPRESS_CB callback.
// Action generated when a key is pressed or released.
type KeyPressFunc func(ih Ihandle, c, press int) int

//export goIupKeyPressCB
func goIupKeyPressCB(ih unsafe.Pointer, c, press C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("KEYPRESS_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "KEYPRESS_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(KeyPressFunc)

	return C.int(f((Ihandle)(ih), int(c), int(press)))
}

// setKeyPressFunc for KEYPRESS_CB.
func setKeyPressFunc(ih Ihandle, f KeyPressFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("KEYPRESS_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetKeyPressFunc(ih.ptr())
}

//--------------------

// CellFunc for CELL_CB callback.
// Called when the user double clicks a color cell to change its value.
type CellFunc func(ih Ihandle, cell int) int

//export goIupCellCB
func goIupCellCB(ih unsafe.Pointer, cell C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("CELL_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "CELL_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(CellFunc)

	return C.int(f((Ihandle)(ih), int(cell)))
}

// setCellFunc for CELL_CB.
func setCellFunc(ih Ihandle, f CellFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("CELL_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetCellFunc(ih.ptr())
}

//--------------------

// ExtendedFunc for EXTENDED_CB callback.
// Called when the user right click a cell with the Shift key pressed.
type ExtendedFunc func(ih Ihandle, cell int) int

//export goIupExtendedCB
func goIupExtendedCB(ih unsafe.Pointer, cell C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("EXTENDED_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "EXTENDED_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(ExtendedFunc)

	return C.int(f((Ihandle)(ih), int(cell)))
}

// setExtendedFunc for EXTENDED_CB.
func setExtendedFunc(ih Ihandle, f ExtendedFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("EXTENDED_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetExtendedFunc(ih.ptr())
}

//--------------------

// SelectFunc for SELECT_CB callback.
// Called when a color is selected.
type SelectFunc func(ih Ihandle, cell, _type int) int

//export goIupSelectCB
func goIupSelectCB(ih unsafe.Pointer, cell, _type C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("SELECT_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "SELECT_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(SelectFunc)

	return C.int(f((Ihandle)(ih), int(cell), int(_type)))
}

// setSelectFunc for SELECT_CB.
func setSelectFunc(ih Ihandle, f SelectFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("SELECT_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetSelectFunc(ih.ptr())
}

//--------------------

// SwitchFunc for SWITCH_CB callback.
// Called when the user double clicks the preview area outside the preview cells
// to switch the primary and secondary selections.
type SwitchFunc func(ih Ihandle, primCell, secCell int) int

//export goIupSwitchCB
func goIupSwitchCB(ih unsafe.Pointer, primCell, secCell C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("SWITCH_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "SWITCH_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(SwitchFunc)

	return C.int(f((Ihandle)(ih), int(primCell), int(secCell)))
}

// setSwitchFunc for SWITCH_CB.
func setSwitchFunc(ih Ihandle, f SwitchFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("SWITCH_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetSwitchFunc(ih.ptr())
}

//--------------------

// LinkActionFunc for Link ACTION callback.
type LinkActionFunc func(ih Ihandle, url string) int

//export goIupLinkActionCB
func goIupLinkActionCB(ih, url unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("LINK_ACTION_" + uuid)
	if !ok {
		panic("cannot load callback " + "LINK_ACTION_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(LinkActionFunc)

	goUrl := C.GoString((*C.char)(url))
	return C.int(f((Ihandle)(ih), goUrl))
}

// setLinkActionFunc for Link ACTION callback.
func setLinkActionFunc(ih Ihandle, f LinkActionFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("LINK_ACTION_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetLinkActionFunc(ih.ptr())
}

//--------------------

// WheelFunc for WHEEL_CB callback.
// Action generated when the mouse wheel is rotated.
type WheelFunc func(ih Ihandle, delta float64, x, y int, status string) int

//export goIupWheelCB
func goIupWheelCB(ih unsafe.Pointer, delta C.float, x, y C.int, status unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("WHEEL_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "WHEEL_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(WheelFunc)

	goStatus := C.GoString((*C.char)(status))
	return C.int(f((Ihandle)(ih), float64(delta), int(x), int(y), goStatus))
}

// setWheelFunc for WHEEL_CB callback.
func setWheelFunc(ih Ihandle, f WheelFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("WHEEL_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetWheelFunc(ih.ptr())
}

//--------------------

// DragDropFunc for DRAGDROP_CB callback.
// Action generated when an internal drag and drop is executed.
type DragDropFunc func(ih Ihandle, dragId, dropId, isShift, isControl int) int

//export goIupDragDropCB
func goIupDragDropCB(ih unsafe.Pointer, dragId, dropId, isShift, isControl C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("DRAGDROP_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "DRAGDROP_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(DragDropFunc)

	return C.int(f((Ihandle)(ih), int(dragId), int(dropId), int(isShift), int(isControl)))
}

// setDragDropFunc for DRAGDROP_CB.
func setDragDropFunc(ih Ihandle, f DragDropFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("DRAGDROP_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetDragDropFunc(ih.ptr())
}

//--------------------

// SelectionFunc for SELECTION_CB callback.
// Action generated when a node is selected or deselected.
type SelectionFunc func(ih Ihandle, id, status int) int

//export goIupSelectionCB
func goIupSelectionCB(ih unsafe.Pointer, id, status C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("SELECTION_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "SELECTION_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(SelectionFunc)

	return C.int(f((Ihandle)(ih), int(id), int(status)))
}

// setSelectionFunc for SELECTION_CB.
func setSelectionFunc(ih Ihandle, f SelectionFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("SELECTION_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetSelectionFunc(ih.ptr())
}

//--------------------

// BranchOpenFunc for BRANCHOPEN_CB callback.
// Action generated when a branch is expanded.
type BranchOpenFunc func(ih Ihandle, id int) int

//export goIupBranchOpenCB
func goIupBranchOpenCB(ih unsafe.Pointer, id C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("BRANCHOPEN_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "BRANCHOPEN_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(BranchOpenFunc)

	return C.int(f((Ihandle)(ih), int(id)))
}

// setBranchOpenFunc for BRANCHOPEN_CB.
func setBranchOpenFunc(ih Ihandle, f BranchOpenFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("BRANCHOPEN_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetBranchOpenFunc(ih.ptr())
}

//--------------------

// BranchCloseFunc for BRANCHCLOSE_CB callback.
// Action generated when a branch is collapsed.
type BranchCloseFunc func(ih Ihandle, id int) int

//export goIupBranchCloseCB
func goIupBranchCloseCB(ih unsafe.Pointer, id C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("BRANCHCLOSE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "BRANCHCLOSE_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(BranchCloseFunc)

	return C.int(f((Ihandle)(ih), int(id)))
}

// setBranchCloseFunc for BRANCHCLOSE_CB.
func setBranchCloseFunc(ih Ihandle, f BranchCloseFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("BRANCHCLOSE_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetBranchCloseFunc(ih.ptr())
}

//--------------------

// ExecuteLeafFunc for EXECUTELEAF_CB callback.
// Action generated when a leaf is executed.
type ExecuteLeafFunc func(ih Ihandle, id int) int

//export goIupExecuteLeafCB
func goIupExecuteLeafCB(ih unsafe.Pointer, id C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("EXECUTELEAF_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "EXECUTELEAF_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(ExecuteLeafFunc)

	return C.int(f((Ihandle)(ih), int(id)))
}

// setExecuteLeafFunc for EXECUTELEAF_CB.
func setExecuteLeafFunc(ih Ihandle, f ExecuteLeafFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("EXECUTELEAF_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetExecuteLeafFunc(ih.ptr())
}

//--------------------

// ExecuteBranchFunc for EXECUTEBRANCH_CB callback.
// Action generated when a branch is executed.
type ExecuteBranchFunc func(ih Ihandle, id int) int

//export goIupExecuteBranchCB
func goIupExecuteBranchCB(ih unsafe.Pointer, id C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("EXECUTEBRANCH_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "EXECUTEBRANCH_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(ExecuteBranchFunc)

	return C.int(f((Ihandle)(ih), int(id)))
}

// setExecuteBranchFunc for EXECUTEBRANCH_CB.
func setExecuteBranchFunc(ih Ihandle, f ExecuteBranchFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("EXECUTEBRANCH_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetExecuteBranchFunc(ih.ptr())
}

//--------------------

// ShowRenameFunc for SHOWRENAME_CB callback.
// Action generated when a node is about to be renamed.
type ShowRenameFunc func(ih Ihandle, id int) int

//export goIupShowRenameCB
func goIupShowRenameCB(ih unsafe.Pointer, id C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("SHOWRENAME_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "SHOWRENAME_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(ShowRenameFunc)

	return C.int(f((Ihandle)(ih), int(id)))
}

// setShowRenameFunc for SHOWRENAME_CB.
func setShowRenameFunc(ih Ihandle, f ShowRenameFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("SHOWRENAME_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetShowRenameFunc(ih.ptr())
}

//--------------------

// RenameFunc for RENAME_CB callback.
// Action generated after a node was renamed in place.
type RenameFunc func(ih Ihandle, id int, title string) int

//export goIupRenameCB
func goIupRenameCB(ih unsafe.Pointer, id C.int, title unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("RENAME_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "RENAME_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(RenameFunc)

	goTitle := C.GoString((*C.char)(title))
	return C.int(f((Ihandle)(ih), int(id), goTitle))
}

// setRenameFunc for RENAME_CB.
func setRenameFunc(ih Ihandle, f RenameFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("RENAME_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetRenameFunc(ih.ptr())
}

//--------------------

// ToggleValueFunc for TOGGLEVALUE_CB callback.
// Action generated when the toggle's state was changed.
type ToggleValueFunc func(ih Ihandle, id, state int) int

//export goIupToggleValueCB
func goIupToggleValueCB(ih unsafe.Pointer, id, state C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("TOGGLEVALUE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "TOGGLEVALUE_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(ToggleValueFunc)

	return C.int(f((Ihandle)(ih), int(id), int(state)))
}

// setToggleValueFunc for TOGGLEVALUE_CB.
func setToggleValueFunc(ih Ihandle, f ToggleValueFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("TOGGLEVALUE_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetToggleValueFunc(ih.ptr())
}

//--------------------

// NodeRemovedFunc for NODEREMOVED_CB callback.
type NodeRemovedFunc func(ih Ihandle) int

//export goIupNodeRemovedCB
func goIupNodeRemovedCB(ih, userData unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("NODEREMOVED_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "NODEREMOVED_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(NodeRemovedFunc)

	return C.int(f((Ihandle)(ih)))
}

// setNodeRemovedFunc for NODEREMOVED_CB callback.
func setNodeRemovedFunc(ih Ihandle, f NodeRemovedFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("NODEREMOVED_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetNodeRemovedFunc(ih.ptr())
}

//--------------------

// MultiSelectionFunc for MULTISELECTION_CB callback.
// Action generated after a continuous range of nodes is selected in one single operation.
type MultiSelectionFunc func(ih Ihandle, ids []int, n int) int

//export goIupMultiSelectionCB
func goIupMultiSelectionCB(ih unsafe.Pointer, ids *C.int, n C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("MULTISELECTION_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "MULTISELECTION_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(MultiSelectionFunc)

	goIds := unsafe.Slice((*int)(unsafe.Pointer(ids)), n)

	return C.int(f((Ihandle)(ih), goIds, int(n)))
}

// setMultiSelectionFunc for MULTISELECTION_CB.
func setMultiSelectionFunc(ih Ihandle, f MultiSelectionFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MULTISELECTION_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetMultiSelectionFunc(ih.ptr())
}

//--------------------

// MultiUnselectionFunc for MULTIUNSELECTION_CB callback.
// Action generated before multiple nodes are unselected in one single operation.
type MultiUnselectionFunc func(ih Ihandle, ids []int, n int) int

//export goIupMultiUnselectionCB
func goIupMultiUnselectionCB(ih unsafe.Pointer, ids *C.int, n C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("MULTIUNSELECTION_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "MULTIUNSELECTION_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(MultiUnselectionFunc)

	goIds := unsafe.Slice((*int)(unsafe.Pointer(ids)), n)

	return C.int(f((Ihandle)(ih), goIds, int(n)))
}

// setMultiUnselectionFunc for MULTIUNSELECTION_CB.
func setMultiUnselectionFunc(ih Ihandle, f MultiUnselectionFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MULTIUNSELECTION_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetMultiUnselectionFunc(ih.ptr())
}

//--------------------

// MenuOpenFunc for OPEN_CB callback.
// Called just before the menu is opened.
//
// https://www.tecgraf.puc-rio.br/iup/en/call/iup_open_cb.html
type MenuOpenFunc func(ih Ihandle) int

//export goIupMenuOpenCB
func goIupMenuOpenCB(ih unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("OPEN_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "OPEN_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(MenuOpenFunc)

	return C.int(f((Ihandle)(ih)))
}

// setMenuOpenFunc for OPEN_CB.
func setMenuOpenFunc(ih Ihandle, f MenuOpenFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("OPEN_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetMenuOpenFunc(ih.ptr())
}

//--------------------

// ThemeChangedFunc for THEMECHANGED_CB callback.
// Action generated when the user changes the UI theme.
type ThemeChangedFunc func(ih Ihandle, darkMode int) int

//export goIupThemeChangedCB
func goIupThemeChangedCB(ih unsafe.Pointer, darkMode C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("THEMECHANGED_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "THEMECHANGED_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(ThemeChangedFunc)

	return C.int(f((Ihandle)(ih), int(darkMode)))
}

// setThemeChangedFunc for THEMECHANGED_CB callback.
func setThemeChangedFunc(ih Ihandle, f ThemeChangedFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("THEMECHANGED_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetThemeChangedFunc(ih.ptr())
}

//--------------------

// CompletedFunc for COMPLETED_CB callback.
// Action generated when a page is successfully completed.
type CompletedFunc func(ih Ihandle, url string) int

//export goIupCompletedCB
func goIupCompletedCB(ih unsafe.Pointer, url unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("COMPLETED_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "COMPLETED_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(CompletedFunc)

	goUrl := C.GoString((*C.char)(url))
	return C.int(f((Ihandle)(ih), goUrl))
}

// setCompletedFunc for COMPLETED_CB.
func setCompletedFunc(ih Ihandle, f CompletedFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("COMPLETED_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetCompletedFunc(ih.ptr())
}

//--------------------

// ErrorFunc for ERROR_CB callback.
// Action generated when a page load fails.
type ErrorFunc func(ih Ihandle, url string) int

//export goIupErrorCB
func goIupErrorCB(ih unsafe.Pointer, url unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("ERROR_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "ERROR_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(ErrorFunc)

	goUrl := C.GoString((*C.char)(url))
	return C.int(f((Ihandle)(ih), goUrl))
}

// setErrorFunc for ERROR_CB.
func setErrorFunc(ih Ihandle, f ErrorFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("ERROR_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetErrorFunc(ih.ptr())
}

//--------------------

// NavigateFunc for NAVIGATE_CB callback.
// Action generated when the browser requests a navigation to another page.
type NavigateFunc func(ih Ihandle, url string) int

//export goIupNavigateCB
func goIupNavigateCB(ih unsafe.Pointer, url unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("NAVIGATE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "NAVIGATE_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(NavigateFunc)

	goUrl := C.GoString((*C.char)(url))
	return C.int(f((Ihandle)(ih), goUrl))
}

// setNavigateFunc for NAVIGATE_CB.
func setNavigateFunc(ih Ihandle, f NavigateFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("NAVIGATE_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetNavigateFunc(ih.ptr())
}

//--------------------

// NewWindowFunc for NEWWINDOW_CB callback.
// Action generated when the browser requests a new window.
type NewWindowFunc func(ih Ihandle, url string) int

//export goIupNewWindowCB
func goIupNewWindowCB(ih unsafe.Pointer, url unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("NEWWINDOW_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "NEWWINDOW_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(NewWindowFunc)

	goUrl := C.GoString((*C.char)(url))
	return C.int(f((Ihandle)(ih), goUrl))
}

// setNewWindowFunc for NEWWINDOW_CB.
func setNewWindowFunc(ih Ihandle, f NewWindowFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("NEWWINDOW_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetNewWindowFunc(ih.ptr())
}

//--------------------

// UpdateFunc for UPDATE_CB callback.
// Action generated when the selection was changed and the editor interface needs an update.
type UpdateFunc func(ih Ihandle) int

//export goIupUpdateCB
func goIupUpdateCB(ih unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("UPDATE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "UPDATE_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(UpdateFunc)

	return C.int(f((Ihandle)(ih)))
}

// setUpdateFunc for UPDATE_CB.
func setUpdateFunc(ih Ihandle, f UpdateFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("UPDATE_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetUpdateFunc(ih.ptr())
}

//--------------------

// ============================================================================
// CELLS AND MATRIX CONTROL CALLBACKS
// ============================================================================

// CellsDrawFunc for DRAW_CB callback in Cells control.
// Called when a cell needs to be redrawn.
type CellsDrawFunc func(ih Ihandle, i, j, xmin, xmax, ymin, ymax int) int

//export goIupCellsDrawCB
func goIupCellsDrawCB(ih unsafe.Pointer, i, j, xmin, xmax, ymin, ymax C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("CELLS_DRAW_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "CELLS_DRAW_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(CellsDrawFunc)

	return C.int(f((Ihandle)(ih), int(i), int(j), int(xmin), int(xmax), int(ymin), int(ymax)))
}

// MouseClickFunc for MOUSECLICK_CB callback.
// Action generated when any mouse button is pressed or released.
type MouseClickFunc func(ih Ihandle, button, pressed, i, j, x, y int, status string) int

//export goIupMouseClickCB
func goIupMouseClickCB(ih unsafe.Pointer, button, pressed, i, j, x, y C.int, status unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("MOUSECLICK_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "MOUSECLICK_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(MouseClickFunc)

	goStatus := C.GoString((*C.char)(status))
	return C.int(f((Ihandle)(ih), int(button), int(pressed), int(i), int(j), int(x), int(y), goStatus))
}

// MouseMotionFunc for MOUSEMOTION_CB callback in Cells.
// Action generated when the mouse moves over the control.
type MouseMotionFunc func(ih Ihandle, i, j, x, y int, status string) int

//export goIupMouseMotionCB
func goIupMouseMotionCB(ih unsafe.Pointer, i, j, x, y C.int, status unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("MOUSEMOTION_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "MOUSEMOTION_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(MouseMotionFunc)

	goStatus := C.GoString((*C.char)(status))
	return C.int(f((Ihandle)(ih), int(i), int(j), int(x), int(y), goStatus))
}

// ScrollingFunc for SCROLLING_CB callback.
// Called when the user scrolls the view.
type ScrollingFunc func(ih Ihandle, i, j int) int

//export goIupScrollingCB
func goIupScrollingCB(ih unsafe.Pointer, i, j C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("SCROLLING_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "SCROLLING_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(ScrollingFunc)

	return C.int(f((Ihandle)(ih), int(i), int(j)))
}

// NColsFunc for NCOLS_CB callback.
// Returns the number of columns.
type NColsFunc func(ih Ihandle) int

//export goIupNColsCB
func goIupNColsCB(ih unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("NCOLS_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "NCOLS_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(NColsFunc)

	return C.int(f((Ihandle)(ih)))
}

// NLinesFunc for NLINES_CB callback.
// Returns the number of lines.
type NLinesFunc func(ih Ihandle) int

//export goIupNLinesCB
func goIupNLinesCB(ih unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("NLINES_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "NLINES_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(NLinesFunc)

	return C.int(f((Ihandle)(ih)))
}

// HSpanFunc for HSPAN_CB callback.
// Returns the horizontal span of a cell.
type HSpanFunc func(ih Ihandle, i, j int) int

//export goIupHSpanCB
func goIupHSpanCB(ih unsafe.Pointer, i, j C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("HSPAN_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "HSPAN_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(HSpanFunc)

	return C.int(f((Ihandle)(ih), int(i), int(j)))
}

// VSpanFunc for VSPAN_CB callback.
// Returns the vertical span of a cell.
type VSpanFunc func(ih Ihandle, i, j int) int

//export goIupVSpanCB
func goIupVSpanCB(ih unsafe.Pointer, i, j C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("VSPAN_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "VSPAN_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(VSpanFunc)

	return C.int(f((Ihandle)(ih), int(i), int(j)))
}

// HeightFunc for HEIGHT_CB callback.
// Returns the height of a line.
type HeightFunc func(ih Ihandle, i int) int

//export goIupHeightCB
func goIupHeightCB(ih unsafe.Pointer, i C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("HEIGHT_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "HEIGHT_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(HeightFunc)

	return C.int(f((Ihandle)(ih), int(i)))
}

// WidthFunc for WIDTH_CB callback.
// Returns the width of a column.
type WidthFunc func(ih Ihandle, j int) int

//export goIupWidthCB
func goIupWidthCB(ih unsafe.Pointer, j C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("WIDTH_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "WIDTH_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(WidthFunc)

	return C.int(f((Ihandle)(ih), int(j)))
}

// MatrixDrawFunc for DRAW_CB callback in Matrix control.
// Called when a cell needs to be redrawn.
type MatrixDrawFunc func(ih Ihandle, lin, col, x1, x2, y1, y2 int) int

//export goIupMatrixDrawCB
func goIupMatrixDrawCB(ih unsafe.Pointer, lin, col, x1, x2, y1, y2 C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("MATRIX_DRAW_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "MATRIX_DRAW_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(MatrixDrawFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col), int(x1), int(x2), int(y1), int(y2)))
}

// EnterItemFunc for ENTERITEM_CB callback.
// Called when a matrix cell is selected, becoming the current cell.
type EnterItemFunc func(ih Ihandle, lin, col int) int

//export goIupEnterItemCB
func goIupEnterItemCB(ih unsafe.Pointer, lin, col C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("ENTERITEM_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "ENTERITEM_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(EnterItemFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col)))
}

// LeaveItemFunc for LEAVEITEM_CB callback.
// Called when a cell is no longer the current cell.
type LeaveItemFunc func(ih Ihandle, lin, col int) int

//export goIupLeaveItemCB
func goIupLeaveItemCB(ih unsafe.Pointer, lin, col C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("LEAVEITEM_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "LEAVEITEM_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(LeaveItemFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col)))
}

// ClickFunc for CLICK_CB callback in Matrix.
// Called when a cell is clicked.
type ClickFunc func(ih Ihandle, lin, col int, status string) int

//export goIupClickCB
func goIupClickCB(ih unsafe.Pointer, lin, col C.int, status unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("CLICK_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "CLICK_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(ClickFunc)

	goStatus := C.GoString((*C.char)(status))
	return C.int(f((Ihandle)(ih), int(lin), int(col), goStatus))
}

// ReleaseFunc for RELEASE_CB callback in Matrix.
// Called when a mouse button is released over a cell.
type ReleaseFunc func(ih Ihandle, lin, col int, status string) int

//export goIupReleaseCB
func goIupReleaseCB(ih unsafe.Pointer, lin, col C.int, status unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("RELEASE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "RELEASE_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(ReleaseFunc)

	goStatus := C.GoString((*C.char)(status))
	return C.int(f((Ihandle)(ih), int(lin), int(col), goStatus))
}

// MatrixMouseMoveFunc for MOUSEMOVE_CB callback in Matrix.
// Called when the mouse moves over a cell.
type MatrixMouseMoveFunc func(ih Ihandle, lin, col int) int

//export goIupMatrixMouseMoveCB
func goIupMatrixMouseMoveCB(ih unsafe.Pointer, lin, col C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("MATRIX_MOUSEMOVE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "MATRIX_MOUSEMOVE_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(MatrixMouseMoveFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col)))
}

// EditionFunc for EDITION_CB callback (Matrix version).
// Called when a cell enters/leaves edition mode.
type EditionFunc func(ih Ihandle, lin, col, mode, update int) int

//export goIupEditionCB
func goIupEditionCB(ih unsafe.Pointer, lin, col, mode, update C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("EDITION_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "EDITION_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(EditionFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col), int(mode), int(update)))
}

// TableEditionFunc for EDITION_CB callback (Table version with string update).
// Called when a table cell is edited.
type TableEditionFunc func(ih Ihandle, lin, col int, update string) int

//export goIupTableEditionCB
func goIupTableEditionCB(ih unsafe.Pointer, lin, col C.int, update *C.char) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("EDITION_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "EDITION_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(TableEditionFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col), C.GoString(update)))
}

// EditBeginFunc for EDITBEGIN_CB callback.
// Called when table cell editing starts.
// Return IUP_DEFAULT to allow edit, IUP_IGNORE to block.
type EditBeginFunc func(ih Ihandle, lin, col int) int

//export goIupEditBeginCB
func goIupEditBeginCB(ih unsafe.Pointer, lin, col C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("EDITBEGIN_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "EDITBEGIN_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(EditBeginFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col)))
}

// EditEndFunc for EDITEND_CB callback.
// Called when table cell editing ends. Apply parameter: 1=accepted (Enter), 0=cancelled (ESC).
// Return IUP_DEFAULT to accept edit, IUP_IGNORE to reject.
type EditEndFunc func(ih Ihandle, lin, col int, newValue string, apply int) int

//export goIupEditEndCB
func goIupEditEndCB(ih unsafe.Pointer, lin, col C.int, newValue *C.char, apply C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("EDITEND_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "EDITEND_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(EditEndFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col), C.GoString(newValue), int(apply)))
}

// TableValueChangedFunc for VALUECHANGED_CB callback (Table version).
// Called when a table cell value is changed.
type TableValueChangedFunc func(ih Ihandle, lin, col int) int

//export goIupTableValueChangedCB
func goIupTableValueChangedCB(ih unsafe.Pointer, lin, col C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("TABLEVALUECHANGED_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "TABLEVALUECHANGED_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(TableValueChangedFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col)))
}

// TableValueFunc for VALUE_CB callback.
// Called to get the value of a cell in virtual mode.
// Returns the string value to display in the cell.
type TableValueFunc func(ih Ihandle, lin, col int) string

//export goIupTableValueCB
func goIupTableValueCB(ih unsafe.Pointer, lin, col C.int) *C.char {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("VALUE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "VALUE_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(TableValueFunc)
	result := f((Ihandle)(ih), int(lin), int(col))
	if result == "" {
		return nil
	}
	return C.CString(result)
}

// TableSortFunc for SORT_CB callback.
// Called when user clicks a column header to sort.
// Returns DEFAULT to proceed with sorting or IGNORE to cancel.
type TableSortFunc func(ih Ihandle, col int) int

//export goIupTableSortCB
func goIupTableSortCB(ih unsafe.Pointer, col C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("SORT_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "SORT_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(TableSortFunc)

	return C.int(f((Ihandle)(ih), int(col)))
}

// DropCheckFunc for DROPCHECK_CB callback.
// Called to determine if a cell can be a dropdown.
type DropCheckFunc func(ih Ihandle, lin, col int) int

//export goIupDropCheckCB
func goIupDropCheckCB(ih unsafe.Pointer, lin, col C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("DROPCHECK_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "DROPCHECK_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(DropCheckFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col)))
}

// MarkFunc for MARK_CB callback.
// Called when a cell is marked.
type MarkFunc func(ih Ihandle, lin, col int) int

//export goIupMarkCB
func goIupMarkCB(ih unsafe.Pointer, lin, col C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("MARK_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "MARK_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(MarkFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col)))
}

// MarkEditFunc for MARKEDIT_CB callback.
// Called when a marked cell is edited.
type MarkEditFunc func(ih Ihandle, lin, col, marked int) int

//export goIupMarkEditCB
func goIupMarkEditCB(ih unsafe.Pointer, lin, col, marked C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("MARKEDIT_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "MARKEDIT_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(MarkEditFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col), int(marked)))
}

// ValueEditFunc for VALUE_EDIT_CB callback.
// Called when the value of a cell is edited.
type ValueEditFunc func(ih Ihandle, lin, col int, newval string) int

//export goIupValueEditCB
func goIupValueEditCB(ih unsafe.Pointer, lin, col C.int, newval unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("VALUE_EDIT_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "VALUE_EDIT_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(ValueEditFunc)

	goNewval := C.GoString((*C.char)(newval))
	return C.int(f((Ihandle)(ih), int(lin), int(col), goNewval))
}

// ColResizeFunc for COLRESIZE_CB callback.
// Called after a column is resized.
type ColResizeFunc func(ih Ihandle, col int) int

//export goIupColResizeCB
func goIupColResizeCB(ih unsafe.Pointer, col C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("COLRESIZE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "COLRESIZE_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(ColResizeFunc)

	return C.int(f((Ihandle)(ih), int(col)))
}

// ResizeMatrixFunc for RESIZEMATRIX_CB callback.
// Called after the matrix size changes.
type ResizeMatrixFunc func(ih Ihandle, width, height int) int

//export goIupResizeMatrixCB
func goIupResizeMatrixCB(ih unsafe.Pointer, width, height C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("RESIZEMATRIX_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "RESIZEMATRIX_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(ResizeMatrixFunc)

	return C.int(f((Ihandle)(ih), int(width), int(height)))
}

// ScrollTopFunc for SCROLLTOP_CB callback.
// Called when the matrix is scrolled.
type ScrollTopFunc func(ih Ihandle, lin, col int) int

//export goIupScrollTopCB
func goIupScrollTopCB(ih unsafe.Pointer, lin, col C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("SCROLLTOP_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "SCROLLTOP_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(ScrollTopFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col)))
}

// BgColorFunc for BGCOLOR_CB callback.
// Called to determine the background color of a cell.
type BgColorFunc func(ih Ihandle, lin, col int) (r, g, b int, ret int)

//export goIupBgColorCB
func goIupBgColorCB(ih unsafe.Pointer, lin, col C.int, r, g, b *C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("BGCOLOR_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "BGCOLOR_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(BgColorFunc)

	rr, gg, bb, ret := f((Ihandle)(ih), int(lin), int(col))
	*r = C.int(rr)
	*g = C.int(gg)
	*b = C.int(bb)
	return C.int(ret)
}

// FgColorFunc for FGCOLOR_CB callback.
// Called to determine the foreground color of a cell.
type FgColorFunc func(ih Ihandle, lin, col int) (r, g, b int, ret int)

//export goIupFgColorCB
func goIupFgColorCB(ih unsafe.Pointer, lin, col C.int, r, g, b *C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("FGCOLOR_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "FGCOLOR_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(FgColorFunc)

	rr, gg, bb, ret := f((Ihandle)(ih), int(lin), int(col))
	*r = C.int(rr)
	*g = C.int(gg)
	*b = C.int(bb)
	return C.int(ret)
}


// setCellsDrawFunc for DRAW_CB in Cells.
func setCellsDrawFunc(ih Ihandle, f CellsDrawFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("CELLS_DRAW_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetCellsDrawFunc(ih.ptr())
}

// setMouseClickFunc for MOUSECLICK_CB.
func setMouseClickFunc(ih Ihandle, f MouseClickFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MOUSECLICK_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetMouseClickFunc(ih.ptr())
}

// setMouseMotionFunc for MOUSEMOTION_CB in Cells.
func setMouseMotionFunc(ih Ihandle, f MouseMotionFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MOUSEMOTION_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetMouseMotionFunc(ih.ptr())
}

// setScrollingFunc for SCROLLING_CB.
func setScrollingFunc(ih Ihandle, f ScrollingFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("SCROLLING_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetScrollingFunc(ih.ptr())
}

// setNColsFunc for NCOLS_CB.
func setNColsFunc(ih Ihandle, f NColsFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("NCOLS_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetNColsFunc(ih.ptr())
}

// setNLinesFunc for NLINES_CB.
func setNLinesFunc(ih Ihandle, f NLinesFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("NLINES_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetNLinesFunc(ih.ptr())
}

// setHSpanFunc for HSPAN_CB.
func setHSpanFunc(ih Ihandle, f HSpanFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("HSPAN_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetHSpanFunc(ih.ptr())
}

// setVSpanFunc for VSPAN_CB.
func setVSpanFunc(ih Ihandle, f VSpanFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("VSPAN_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetVSpanFunc(ih.ptr())
}

// setHeightFunc for HEIGHT_CB.
func setHeightFunc(ih Ihandle, f HeightFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("HEIGHT_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetHeightFunc(ih.ptr())
}

// setWidthFunc for WIDTH_CB.
func setWidthFunc(ih Ihandle, f WidthFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("WIDTH_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetWidthFunc(ih.ptr())
}

// setMatrixDrawFunc for DRAW_CB in Matrix.
func setMatrixDrawFunc(ih Ihandle, f MatrixDrawFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MATRIX_DRAW_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetMatrixDrawFunc(ih.ptr())
}

// setEnterItemFunc for ENTERITEM_CB.
func setEnterItemFunc(ih Ihandle, f EnterItemFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("ENTERITEM_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetEnterItemFunc(ih.ptr())
}

// setLeaveItemFunc for LEAVEITEM_CB.
func setLeaveItemFunc(ih Ihandle, f LeaveItemFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("LEAVEITEM_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetLeaveItemFunc(ih.ptr())
}

// setClickFunc for CLICK_CB in Matrix.
func setClickFunc(ih Ihandle, f ClickFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("CLICK_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetClickFunc(ih.ptr())
}

// setReleaseFunc for RELEASE_CB in Matrix.
func setReleaseFunc(ih Ihandle, f ReleaseFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("RELEASE_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetReleaseFunc(ih.ptr())
}

// setMatrixMouseMoveFunc for MOUSEMOVE_CB in Matrix.
func setMatrixMouseMoveFunc(ih Ihandle, f MatrixMouseMoveFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MATRIX_MOUSEMOVE_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetMatrixMouseMoveFunc(ih.ptr())
}

// setEditionFunc for EDITION_CB.
func setEditionFunc(ih Ihandle, f EditionFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("EDITION_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetEditionFunc(ih.ptr())
}

// setTableEditionFunc for EDITION_CB (Table version).
func setTableEditionFunc(ih Ihandle, f TableEditionFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("EDITION_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetTableEditionFunc(ih.ptr())
}

// setEditBeginFunc for EDITBEGIN_CB.
func setEditBeginFunc(ih Ihandle, f EditBeginFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("EDITBEGIN_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetEditBeginFunc(ih.ptr())
}

// setEditEndFunc for EDITEND_CB.
func setEditEndFunc(ih Ihandle, f EditEndFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("EDITEND_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetEditEndFunc(ih.ptr())
}

// setTableValueChangedFunc for VALUECHANGED_CB (Table version).
func setTableValueChangedFunc(ih Ihandle, f TableValueChangedFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("TABLEVALUECHANGED_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetTableValueChangedFunc(ih.ptr())
}

// setTableValueFunc for VALUE_CB (Table version).
func setTableValueFunc(ih Ihandle, f TableValueFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("VALUE_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetTableValueFunc(ih.ptr())
}

// setTableSortFunc for SORT_CB (Table version).
func setTableSortFunc(ih Ihandle, f TableSortFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("SORT_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetTableSortFunc(ih.ptr())
}

// setDropCheckFunc for DROPCHECK_CB.
func setDropCheckFunc(ih Ihandle, f DropCheckFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("DROPCHECK_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetDropCheckFunc(ih.ptr())
}

// setMarkFunc for MARK_CB.
func setMarkFunc(ih Ihandle, f MarkFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MARK_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetMarkFunc(ih.ptr())
}

// setMarkEditFunc for MARKEDIT_CB.
func setMarkEditFunc(ih Ihandle, f MarkEditFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MARKEDIT_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetMarkEditFunc(ih.ptr())
}

// setValueEditFunc for VALUE_EDIT_CB.
func setValueEditFunc(ih Ihandle, f ValueEditFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("VALUE_EDIT_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetValueEditFunc(ih.ptr())
}

// setColResizeFunc for COLRESIZE_CB.
func setColResizeFunc(ih Ihandle, f ColResizeFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("COLRESIZE_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetColResizeFunc(ih.ptr())
}

// setResizeMatrixFunc for RESIZEMATRIX_CB.
func setResizeMatrixFunc(ih Ihandle, f ResizeMatrixFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("RESIZEMATRIX_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetResizeMatrixFunc(ih.ptr())
}

// setScrollTopFunc for SCROLLTOP_CB.
func setScrollTopFunc(ih Ihandle, f ScrollTopFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("SCROLLTOP_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetScrollTopFunc(ih.ptr())
}

// setBgColorFunc for BGCOLOR_CB.
func setBgColorFunc(ih Ihandle, f BgColorFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("BGCOLOR_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetBgColorFunc(ih.ptr())
}

// setFgColorFunc for FGCOLOR_CB.
func setFgColorFunc(ih Ihandle, f FgColorFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("FGCOLOR_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetFgColorFunc(ih.ptr())
}

//--------------------

// MatrixDropFunc for DROP_CB callback in Matrix control.
// Called when a dropdown is created for a cell.
//
// f: func(ih, drop Ihandle, lin, col int) int
//
// https://www.tecgraf.puc-rio.br/iup/en/ctrl/iupmatrix.html
type MatrixDropFunc func(ih, drop Ihandle, lin, col int) int

//export goIupMatrixDropCB
func goIupMatrixDropCB(ih, drop unsafe.Pointer, lin, col C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("DROP_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "DROP_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(MatrixDropFunc)
	return C.int(f((Ihandle)(ih), (Ihandle)(drop), int(lin), int(col)))
}

// setMatrixDropFunc for DROP_CB.
func setMatrixDropFunc(ih Ihandle, f MatrixDropFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("DROP_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetMatrixDropFunc(ih.ptr())
}

//--------------------

// MatrixActionFunc for ACTION_CB callback in Matrix control.
// Called when a key is pressed in the matrix.
//
// f: func(ih Ihandle, key, lin, col, edition int, status string) int
//
// https://www.tecgraf.puc-rio.br/iup/en/ctrl/iupmatrix.html
type MatrixActionFunc func(ih Ihandle, key, lin, col, edition int, status string) int

//export goIupMatrixActionCB
func goIupMatrixActionCB(ih unsafe.Pointer, key, lin, col, edition C.int, status *C.char) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("MATRIX_ACTION_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "MATRIX_ACTION_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(MatrixActionFunc)
	goStatus := C.GoString(status)
	return C.int(f((Ihandle)(ih), int(key), int(lin), int(col), int(edition), goStatus))
}

// setMatrixActionFunc for ACTION_CB.
func setMatrixActionFunc(ih Ihandle, f MatrixActionFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MATRIX_ACTION_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetMatrixActionFunc(ih.ptr())
}

//--------------------

// EditClickFunc for EDITCLICK_CB callback.
// Called when a cell is clicked in edit mode.
//
// f: func(ih Ihandle, lin, col int, status string) int
//
// https://www.tecgraf.puc-rio.br/iup/en/ctrl/iupmatrix.html
type EditClickFunc func(ih Ihandle, lin, col int, status string) int

//export goIupEditClickCB
func goIupEditClickCB(ih unsafe.Pointer, lin, col C.int, status *C.char) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("EDITCLICK_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "EDITCLICK_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(EditClickFunc)
	goStatus := C.GoString(status)
	return C.int(f((Ihandle)(ih), int(lin), int(col), goStatus))
}

// setEditClickFunc for EDITCLICK_CB.
func setEditClickFunc(ih Ihandle, f EditClickFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("EDITCLICK_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetEditClickFunc(ih.ptr())
}

//--------------------

// EditReleaseFunc for EDITRELEASE_CB callback.
// Called when mouse button is released in edit mode.
//
// f: func(ih Ihandle, lin, col int, status string) int
//
// https://www.tecgraf.puc-rio.br/iup/en/ctrl/iupmatrix.html
type EditReleaseFunc func(ih Ihandle, lin, col int, status string) int

//export goIupEditReleaseCB
func goIupEditReleaseCB(ih unsafe.Pointer, lin, col C.int, status *C.char) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("EDITRELEASE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "EDITRELEASE_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(EditReleaseFunc)
	goStatus := C.GoString(status)
	return C.int(f((Ihandle)(ih), int(lin), int(col), goStatus))
}

// setEditReleaseFunc for EDITRELEASE_CB.
func setEditReleaseFunc(ih Ihandle, f EditReleaseFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("EDITRELEASE_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetEditReleaseFunc(ih.ptr())
}

//--------------------

// EditMouseMoveFunc for EDITMOUSEMOVE_CB callback.
// Called when mouse moves in edit mode.
//
// f: func(ih Ihandle, lin, col int) int
//
// https://www.tecgraf.puc-rio.br/iup/en/ctrl/iupmatrix.html
type EditMouseMoveFunc func(ih Ihandle, lin, col int) int

//export goIupEditMouseMoveCB
func goIupEditMouseMoveCB(ih unsafe.Pointer, lin, col C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("EDITMOUSEMOVE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "EDITMOUSEMOVE_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(EditMouseMoveFunc)
	return C.int(f((Ihandle)(ih), int(lin), int(col)))
}

// setEditMouseMoveFunc for EDITMOUSEMOVE_CB.
func setEditMouseMoveFunc(ih Ihandle, f EditMouseMoveFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("EDITMOUSEMOVE_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetEditMouseMoveFunc(ih.ptr())
}

//--------------------

// MatrixFontFunc for FONT_CB callback.
// Returns the font to be used for a cell.
//
// f: func(ih Ihandle, lin, col int) string
//
// https://www.tecgraf.puc-rio.br/iup/en/ctrl/iupmatrix.html
type MatrixFontFunc func(ih Ihandle, lin, col int) string

//export goIupMatrixFontCB
func goIupMatrixFontCB(ih unsafe.Pointer, lin, col C.int) *C.char {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("FONT_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "FONT_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(MatrixFontFunc)
	result := f((Ihandle)(ih), int(lin), int(col))
	if result == "" {
		return nil
	}
	return C.CString(result)
}

// setMatrixFontFunc for FONT_CB.
func setMatrixFontFunc(ih Ihandle, f MatrixFontFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("FONT_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetMatrixFontFunc(ih.ptr())
}

//--------------------

// MatrixTypeFunc for TYPE_CB callback.
// Returns the type to be used for a cell.
//
// f: func(ih Ihandle, lin, col int) string
//
// https://www.tecgraf.puc-rio.br/iup/en/ctrl/iupmatrix.html
type MatrixTypeFunc func(ih Ihandle, lin, col int) string

//export goIupMatrixTypeCB
func goIupMatrixTypeCB(ih unsafe.Pointer, lin, col C.int) *C.char {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("TYPE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "TYPE_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(MatrixTypeFunc)
	result := f((Ihandle)(ih), int(lin), int(col))
	if result == "" {
		return nil
	}
	return C.CString(result)
}

// setMatrixTypeFunc for TYPE_CB.
func setMatrixTypeFunc(ih Ihandle, f MatrixTypeFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("TYPE_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetMatrixTypeFunc(ih.ptr())
}

//--------------------

// TranslateValueFunc for TRANSLATEVALUE_CB callback.
// Translates a cell value for display.
//
// f: func(ih Ihandle, lin, col int, value string) string
//
// https://www.tecgraf.puc-rio.br/iup/en/ctrl/iupmatrix.html
type TranslateValueFunc func(ih Ihandle, lin, col int, value string) string

//export goIupTranslateValueCB
func goIupTranslateValueCB(ih unsafe.Pointer, lin, col C.int, value *C.char) *C.char {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("TRANSLATEVALUE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "TRANSLATEVALUE_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(TranslateValueFunc)
	goValue := C.GoString(value)
	result := f((Ihandle)(ih), int(lin), int(col), goValue)
	if result == "" {
		return nil
	}
	return C.CString(result)
}

// setTranslateValueFunc for TRANSLATEVALUE_CB.
func setTranslateValueFunc(ih Ihandle, f TranslateValueFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("TRANSLATEVALUE_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetTranslateValueFunc(ih.ptr())
}

//--------------------

// MenuDropFunc for MENUDROP_CB callback.
// Called when a menu dropdown is created for a cell.
//
// f: func(ih, menu Ihandle, lin, col int) int
//
// https://www.tecgraf.puc-rio.br/iup/en/ctrl/iupmatrix.html
type MenuDropFunc func(ih, menu Ihandle, lin, col int) int

//export goIupMenuDropCB
func goIupMenuDropCB(ih, menu unsafe.Pointer, lin, col C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("MENUDROP_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "MENUDROP_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(MenuDropFunc)
	return C.int(f((Ihandle)(ih), (Ihandle)(menu), int(lin), int(col)))
}

// setMenuDropFunc for MENUDROP_CB.
func setMenuDropFunc(ih Ihandle, f MenuDropFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MENUDROP_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetMenuDropFunc(ih.ptr())
}

//--------------------

// DropSelectFunc for DROPSELECT_CB callback.
// Called when an item is selected from dropdown.
//
// f: func(ih Ihandle, lin, col int, drop Ihandle, text string, item, col2 int) int
//
// https://www.tecgraf.puc-rio.br/iup/en/ctrl/iupmatrix.html
type DropSelectFunc func(ih Ihandle, lin, col int, drop Ihandle, text string, item, col2 int) int

//export goIupDropSelectCB
func goIupDropSelectCB(ih unsafe.Pointer, lin, col C.int, drop unsafe.Pointer, text *C.char, item, col2 C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("DROPSELECT_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "DROPSELECT_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(DropSelectFunc)
	goText := C.GoString(text)
	return C.int(f((Ihandle)(ih), int(lin), int(col), (Ihandle)(drop), goText, int(item), int(col2)))
}

// setDropSelectFunc for DROPSELECT_CB.
func setDropSelectFunc(ih Ihandle, f DropSelectFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("DROPSELECT_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetDropSelectFunc(ih.ptr())
}

//--------------------

// MatrixValueFunc for VALUE_CB callback.
// Returns the value to be displayed in a cell (callback mode).
//
// f: func(ih Ihandle, lin, col int) string
//
// https://www.tecgraf.puc-rio.br/iup/en/ctrl/iupmatrix.html
type MatrixValueFunc func(ih Ihandle, lin, col int) string

//export goIupMatrixValueCB
func goIupMatrixValueCB(ih unsafe.Pointer, lin, col C.int) *C.char {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("VALUE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "VALUE_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(MatrixValueFunc)
	result := f((Ihandle)(ih), int(lin), int(col))
	if result == "" {
		return nil
	}
	return C.CString(result)
}

// setMatrixValueFunc for VALUE_CB.
func setMatrixValueFunc(ih Ihandle, f MatrixValueFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("VALUE_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetMatrixValueFunc(ih.ptr())
}

//--------------------

// MatrixToggleValueFunc for TOGGLEVALUE_CB callback in Matrix control.
// Called when a toggle cell value changes.
//
// f: func(ih Ihandle, lin, col, value int) int
//
// https://www.tecgraf.puc-rio.br/iup/en/ctrl/iupmatrix.html
type MatrixToggleValueFunc func(ih Ihandle, lin, col, value int) int

//export goIupMatrixToggleValueCB
func goIupMatrixToggleValueCB(ih unsafe.Pointer, lin, col, value C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("MATRIX_TOGGLEVALUE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "MATRIX_TOGGLEVALUE_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(MatrixToggleValueFunc)
	return C.int(f((Ihandle)(ih), int(lin), int(col), int(value)))
}

// setMatrixToggleValueFunc for TOGGLEVALUE_CB.
func setMatrixToggleValueFunc(ih Ihandle, f MatrixToggleValueFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MATRIX_TOGGLEVALUE_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetMatrixToggleValueFunc(ih.ptr())
}

//--------------------

// MatrixListActionFunc for ACTION_CB and IMAGEVALUECHANGED_CB callbacks in MatrixList.
// Called when item selection or image value changes.
//
// f: func(ih Ihandle, item, state int) int
//
// https://www.tecgraf.puc-rio.br/iup/en/ctrl/iupmatrixlist.html
type MatrixListActionFunc func(ih Ihandle, item, state int) int

//export goIupMatrixListActionCB
func goIupMatrixListActionCB(ih unsafe.Pointer, item, state C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("MATRIXLIST_ACTION_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "MATRIXLIST_ACTION_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(MatrixListActionFunc)
	return C.int(f((Ihandle)(ih), int(item), int(state)))
}

// setMatrixListActionFunc for ACTION_CB and IMAGEVALUECHANGED_CB.
func setMatrixListActionFunc(ih Ihandle, name string, f MatrixListActionFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MATRIXLIST_ACTION_CB_"+ih.GetAttribute("UUID"), ch)
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))
	C.goIupSetMatrixListActionFunc(ih.ptr(), cName)
}

