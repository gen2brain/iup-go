package iup

import (
	"runtime/cgo"
	"unsafe"
)

/*
#include <stdlib.h>
#include "iup.h"
*/
import "C"

// MainLoop executes the user interaction until a callback returns CLOSE, ExitLoop is called, or hiding the last visible dialog.
//
// If MainLoop is called without any visible dialogs and no active timers, the application will hang and will not be possible to close the main loop.
// The process will have to be interrupted by the system.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupmainloop.html
func MainLoop() (ret int) {
	return int(C.IupMainLoop())
}

// MainLoopLevel returns the current cascade level of MainLoop. When no calls were done, return value is 0.
//
// You can use this function to check if MainLoop was already called and avoid calling it again.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupmainlooplevel.html
func MainLoopLevel() (ret int) {
	return int(C.IupMainLoopLevel())
}

// LoopStep runs one iteration of the message loop.
// It returns immediately after processing any messages or if there are no messages to process.
//
// This function is useful for allowing a second message loop to be managed by the application itself.
// This means that messages can be intercepted and callbacks can be processed inside an application loop.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iuploopstep.html
func LoopStep() (ret int) {
	return int(C.IupLoopStep())
}

// LoopStepWait runs one iteration of the message loop.
//
// It puts the system in idle until a message is processed.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iuploopstep.html
func LoopStepWait() (ret int) {
	return int(C.IupLoopStepWait())
}

// ExitLoop terminates the current message loop. It has the same effect of a callback returning CLOSE.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupexitloop.html
func ExitLoop() {
	C.IupExitLoop()
}

// PostMessage sends data to an element, that will be received by a callback when the main loop regain control.
// It is expected to be thread safe.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iuppostmessage.html
func PostMessage(ih Ihandle, s string, i int, d float64, p cgo.Handle) {
	cS := C.CString(s)
	defer C.free(unsafe.Pointer(cS))

	C.IupPostMessage(ih.ptr(), cS, C.int(i), C.double(d), unsafe.Pointer(&p))
}

// Flush processes all pending messages in the message queue.
//
// When you change an attribute of a certain element, the change may not take place immediately.
// For this update to occur faster than usual, call Flush after the attribute is changed.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupflush.html
func Flush() {
	C.IupFlush()
}

// GetCallback returns the callback associated to an event.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetcallback.html
func GetCallback(ih Ihandle, name string) uintptr {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	return uintptr(unsafe.Pointer(C.IupGetCallback(ih.ptr(), cName)))
}

// SetCallback associates a callback to an event.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupsetcallback.html
func SetCallback(ih Ihandle, name string, fn interface{}) {
	if fn == nil {
		cName := C.CString(name)
		defer C.free(unsafe.Pointer(cName))

		_, ok := callbacks.Load(name + "_" + ih.GetAttribute("UUID"))
		if ok {
			callbacks.Delete(name)
		}

		C.IupSetCallback(ih.ptr(), cName, nil)
		return
	}

	switch name {
	case "MAP_CB":
		setMapFunc(ih, fn.(MapFunc))
	case "UNMAP_CB":
		setUnmapFunc(ih, fn.(UnmapFunc))
	case "DESTROY_CB":
		setDestroyFunc(ih, fn.(DestroyFunc))
	case "GETFOCUS_CB":
		setGetFocusFunc(ih, fn.(GetFocusFunc))
	case "KILLFOCUS_CB":
		setKillFocusFunc(ih, fn.(KillFocusFunc))
	case "ENTERWINDOW_CB":
		setEnterWindowFunc(ih, fn.(EnterWindowFunc))
	case "LEAVEWINDOW_CB":
		setLeaveWindowFunc(ih, fn.(LeaveWindowFunc))
	case "K_ANY":
		setKAnyFunc(ih, fn.(KAnyFunc))
	case "HELP_CB":
		setHelpFunc(ih, fn.(HelpFunc))
	case "ACTION", "FLAT_ACTION":
		switch v := fn.(type) {
		case ActionFunc:
			setActionFunc(ih, v)
		case FlatActionFunc:
			setFlatActionFunc(ih, v)
		case ListActionFunc:
			setListActionFunc(ih, v)
		case TextActionFunc:
			setTextActionFunc(ih, v)
		case ToggleActionFunc:
			setToggleActionFunc(ih, v)
		case CanvasActionFunc:
			setCanvasActionFunc(ih, v)
		case LinkActionFunc:
			setLinkActionFunc(ih, v)
		}
	case "BUTTON_CB":
		setButtonFunc(ih, fn.(ButtonFunc))
	case "DROPFILES_CB":
		setDropFilesFunc(ih, fn.(DropFilesFunc))
	case "CARET_CB":
		setCaretFunc(ih, fn.(CaretFunc))
	case "DBLCLICK_CB":
		setDblclickFunc(ih, fn.(DblclickFunc))
	case "EDIT_CB":
		setEditFunc(ih, fn.(EditFunc))
	case "MOTION_CB":
		setMotionFunc(ih, fn.(MotionFunc))
	case "MULTISELECT_CB":
		setMultiselectFunc(ih, fn.(MultiselectFunc))
	case "VALUECHANGED_CB":
		setValueChangedFunc(ih, fn.(ValueChangedFunc))
	case "TABCHANGE_CB":
		setTabChangeFunc(ih, fn.(TabChangeFunc))
	case "TABCHANGEPOS_CB":
		setTabChangePosFunc(ih, fn.(TabChangePosFunc))
	case "SPIN_CB":
		setSpinFunc(ih, fn.(SpinFunc))
	case "POSTMESSAGE_CB":
		setPostMessageFunc(ih, fn.(PostMessageFunc))
	case "CLOSE_CB":
		setCloseFunc(ih, fn.(CloseFunc))
	case "FOCUS_CB":
		setFocusFunc(ih, fn.(FocusFunc))
	case "MOVE_CB":
		setMoveFunc(ih, fn.(MoveFunc))
	case "RESIZE_CB":
		setResizeFunc(ih, fn.(ResizeFunc))
	case "SHOW_CB":
		setShowFunc(ih, fn.(ShowFunc))
	case "CHANGE_CB":
		setChangeFunc(ih, fn.(ChangeFunc))
	case "DRAG_CB":
		setDragFunc(ih, fn.(DragFunc))
	case "DETACHED_CB":
		setDetachedFunc(ih, fn.(DetachedFunc))
	case "RESTORED_CB":
		setRestoredFunc(ih, fn.(RestoredFunc))
	case "SWAPBUFFERS_CB":
		setSwapBuffersFunc(ih, fn.(SwapBuffersFunc))
	case "CANCEL_CB":
		setCancelFunc(ih, fn.(CancelFunc))
	case "ACTION_CB":
		setTimerActionFunc(ih, fn.(TimerActionFunc))
	case "THREAD_CB":
		setThreadFunc(ih, fn.(ThreadFunc))
	case "SCROLL_CB":
		setScrollFunc(ih, fn.(ScrollFunc))
	case "TRAYCLICK_CB":
		setTrayClickFunc(ih, fn.(TrayClickFunc))
	case "TABCLOSE_CB":
		setTabCloseFunc(ih, fn.(TabCloseFunc))
	case "RIGHTCLICK_CB":
		setRightClickFunc(ih, fn.(RightClickFunc))
	case "EXTRABUTTON_CB":
		setExtraButtonFunc(ih, fn.(ExtraButtonFunc))
	case "OPENCLOSE_CB":
		setOpenCloseFunc(ih, fn.(OpenCloseFunc))
	case "VALUECHANGING_CB":
		setValueChangingFunc(ih, fn.(ValueChangingFunc))
	case "DROPDOWN_CB":
		setDropDownFunc(ih, fn.(DropDownFunc))
	case "DROPSHOW_CB":
		setDropShowFunc(ih, fn.(DropShowFunc))
	case "BUTTON_PRESS_CB":
		setButtonPressFunc(ih, fn.(ButtonPressFunc))
	case "BUTTON_RELEASE_CB":
		setButtonReleaseFunc(ih, fn.(ButtonReleaseFunc))
	case "MOUSEMOVE_CB":
		setMouseMoveFunc(ih, fn.(MouseMoveFunc))
	case "KEYPRESS_CB":
		setKeyPressFunc(ih, fn.(KeyPressFunc))
	case "CELL_CB":
		setCellFunc(ih, fn.(CellFunc))
	case "EXTENDED_CB":
		setExtendedFunc(ih, fn.(ExtendedFunc))
	case "SELECT_CB":
		setSelectFunc(ih, fn.(SelectFunc))
	case "SWITCH_CB":
		setSwitchFunc(ih, fn.(SwitchFunc))
	case "WHEEL_CB":
		setWheelFunc(ih, fn.(WheelFunc))
	case "DRAGDROP_CB":
		setDragDropFunc(ih, fn.(DragDropFunc))
	case "SELECTION_CB":
		setSelectionFunc(ih, fn.(SelectionFunc))
	case "BRANCHOPEN_CB":
		setBranchOpenFunc(ih, fn.(BranchOpenFunc))
	case "BRANCHCLOSE_CB":
		setBranchCloseFunc(ih, fn.(BranchCloseFunc))
	case "EXECUTELEAF_CB":
		setExecuteLeafFunc(ih, fn.(ExecuteLeafFunc))
	case "EXECUTEBRANCH_CB":
		setExecuteBranchFunc(ih, fn.(ExecuteBranchFunc))
	case "SHOWRENAME_CB":
		setShowRenameFunc(ih, fn.(ShowRenameFunc))
	case "RENAME_CB":
		setRenameFunc(ih, fn.(RenameFunc))
	case "TOGGLEVALUE_CB":
		setToggleValueFunc(ih, fn.(ToggleValueFunc))
	case "NODEREMOVED_CB":
		setNodeRemovedFunc(ih, fn.(NodeRemovedFunc))
	case "MULTISELECTION_CB":
		setMultiSelectionFunc(ih, fn.(MultiSelectionFunc))
	case "MULTIUNSELECTION_CB":
		setMultiUnselectionFunc(ih, fn.(MultiUnselectionFunc))
	}
}

// GetFunction returns the function associated to an action only when they were set by SetFunction.
// It will not work if SetCallback were used.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetfunction.html
func GetFunction(name string) uintptr {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	return uintptr(unsafe.Pointer(C.IupGetFunction(cName)))
}

// SetFunction associates a function to an action as a global callback.
//
// This function should not be used by new applications, use it only for global callbacks.
// For regular elements use SetCallback instead.
//
// Notice that the application or libraries may set the same name for two different functions by mistake.
// SetCallback does not depends on global names.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupsetfunction.html
func SetFunction(name string, fn interface{}) {
	if fn == nil {
		cName := C.CString(name)
		defer C.free(unsafe.Pointer(cName))

		_, ok := callbacks.Load(name)
		if ok {
			callbacks.Delete(name)
		}

		C.IupSetFunction(cName, nil)
		return
	}

	switch name {
	case "IDLE_ACTION":
		setIdleFunc(fn.(IdleFunc))
	case "ENTRY_POINT":
		setEntryPointFunc(fn.(EntryPointFunc))
	case "EXIT_CB":
		setExitFunc(fn.(ExitFunc))
	}
}

// RecordInput records all mouse and keyboard input in a file for later reproduction.
//
// Any existing file will be replaced. Must stop recording before exiting the application.
// If fileName is nil it will stop recording.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iuprecordinput.html
func RecordInput(fileName string, mode int) int {
	cFileName := C.CString(fileName)
	defer C.free(unsafe.Pointer(cFileName))

	return int(C.IupRecordInput(cFileName, C.int(mode)))
}

// PlayInput reproduces all mouse and keyboard input from a given file.
//
// The file must had been saved using the RecordInput function. Record mode will be automatically detected.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupplayinput.html
func PlayInput(fileName string) int {
	cFileName := C.CString(fileName)
	defer C.free(unsafe.Pointer(cFileName))

	return int(C.IupPlayInput(cFileName))
}
