package iup

import (
	"runtime/cgo"
	"sync"
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

extern int goIupCanvasActionCB(void *, float posx, float posy);
static void goIupSetCanvasActionFunc(Ihandle *ih) {
	IupSetCallback(ih, "ACTION", (Icallback) goIupCanvasActionCB);
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
*/
import "C"

var (
	callbacks sync.Map
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
type PostMessageFunc func(Ihandle, string, int, float64, *cgo.Handle) int

//export goIupPostMessageCB
func goIupPostMessageCB(ih unsafe.Pointer, s unsafe.Pointer, i C.int, d C.double, p unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("POSTMESSAGE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "POSTMESSAGE_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(PostMessageFunc)

	goS := C.GoString((*C.char)(s))
	goP := (*cgo.Handle)(p)

	return C.int(f((Ihandle)(ih), goS, int(i), float64(d), goP))
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

// CanvasActionFunc for Canvas ACTION callback.
// Action generated when the canvas needs to be redrawn.
type CanvasActionFunc func(ih Ihandle, posx, posy float64) int

//export goIupCanvasActionCB
func goIupCanvasActionCB(ih unsafe.Pointer, posx, posy C.float) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("CANVAS_ACTION_" + uuid)
	if !ok {
		panic("cannot load callback " + "CANVAS_ACTION_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(CanvasActionFunc)

	return C.int(f((Ihandle)(ih), float64(posx), float64(posy)))
}

// setCanvasActionFunc for Canvas ACTION callback.
func setCanvasActionFunc(ih Ihandle, f CanvasActionFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("CANVAS_ACTION_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetCanvasActionFunc(ih.ptr())
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
