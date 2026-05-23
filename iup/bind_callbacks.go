package iup

import (
	"runtime/cgo"
	"strconv"
	"strings"
	"unsafe"
)

/*
#include "bind_callbacks.h"
*/
import "C"

var (
	globalIdleHandle  cgo.Handle
	globalEntryHandle cgo.Handle
	globalExitHandle  cgo.Handle
)

// Tracks which _IUPGO_* keys are set on an Ihandle. _IUP* names are skipped
// by IupGetAllAttributes, so we can't iterate them at destroy time without this list.
const iupgoRegistryAttr = "_IUPGO_REGISTRY"

func handleToStr(ch cgo.Handle) string {
	return strconv.FormatUint(uint64(ch), 36)
}

func strToHandle(s string) cgo.Handle {
	if s == "" {
		return 0
	}
	n, err := strconv.ParseUint(s, 36, 64)
	if err != nil {
		return 0
	}
	return cgo.Handle(uintptr(n))
}

func storeCallback(ih Ihandle, key string, f any) {
	if old := strToHandle(ih.GetAttribute(key)); old != 0 {
		old.Delete()
	} else {
		list := ih.GetAttribute(iupgoRegistryAttr)
		if list == "" {
			ih.SetAttribute(iupgoRegistryAttr, key)
			C.goIupSetLDestroyFunc(ih.ptr())
		} else {
			ih.SetAttribute(iupgoRegistryAttr, list+","+key)
		}
	}
	ih.SetAttribute(key, handleToStr(cgo.NewHandle(f)))
}

func loadCallback(ih Ihandle, key string) cgo.Handle {
	return strToHandle(ih.GetAttribute(key))
}

func clearCallback(ih Ihandle, key string) {
	if old := strToHandle(ih.GetAttribute(key)); old != 0 {
		old.Delete()
		ih.SetAttribute(key, "")
	}
}

func setGlobalHandle(slot *cgo.Handle, f any) {
	if *slot != 0 {
		slot.Delete()
	}
	*slot = cgo.NewHandle(f)
}

func clearGlobalHandle(slot *cgo.Handle) {
	if *slot != 0 {
		slot.Delete()
		*slot = 0
	}
}

//export goIupLDestroyCB
func goIupLDestroyCB(ih unsafe.Pointer) C.int {
	h := (Ihandle)(ih)
	list := h.GetAttribute(iupgoRegistryAttr)
	if list == "" {
		return 0
	}
	for _, key := range strings.Split(list, ",") {
		if ch := strToHandle(h.GetAttribute(key)); ch != 0 {
			ch.Delete()
		}
	}
	return 0
}

//--------------------

// IdleFunc for IDLE_ACTION callback.
// generated when there are no events or messages to be processed. Often used to perform background operations.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/call/iup_idle_action.md
type IdleFunc func() int

//export goIupIdleCB
func goIupIdleCB() C.int {
	f := globalIdleHandle.Value().(IdleFunc)
	return C.int(f())
}

// setIdleFunc for IDLE_ACTION.
func setIdleFunc(f IdleFunc) {
	setGlobalHandle(&globalIdleHandle, f)
	C.goIupSetIdleFunc()
}

//--------------------

// EntryPointFunc for ENTRY_POINT callback.
// generated when there are no events or messages to be processed. Often used to perform background operations.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/call/iup_entry_point.md
type EntryPointFunc func()

//export goIupEntryPointCB
func goIupEntryPointCB() {
	f := globalEntryHandle.Value().(EntryPointFunc)
	f()
}

// setEntryPointFunc for ENTRY_POINT.
func setEntryPointFunc(f EntryPointFunc) {
	setGlobalHandle(&globalEntryHandle, f)
	C.goIupSetEntryPointFunc()
}

//--------------------

// ExitFunc for EXIT_CB callback.
// Global callback for an exit. Used when main is not possible, such as in iOS and Android systems.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/call/iup_exit_cb.md
type ExitFunc func()

//export goIupExitCB
func goIupExitCB() {
	f := globalExitHandle.Value().(ExitFunc)
	f()
}

// setExitFunc for EXIT_CB.
func setExitFunc(f ExitFunc) {
	setGlobalHandle(&globalExitHandle, f)
	C.goIupSetExitFunc()
}

//--------------------

// MapFunc for MAP_CB callback.
// Called right after an element is mapped and its attributes updated in Map.
// When the element is a dialog, it is called after the layout is updated. For all other elements is called before the layout is updated.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/call/iup_map_cb.md
type MapFunc func(Ihandle) int

//export goIupMapCB
func goIupMapCB(ih unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_MAP_CB").Value().(MapFunc)

	return C.int(f((Ihandle)(ih)))

}

// setMapFunc for MAP_CB.
func setMapFunc(ih Ihandle, f MapFunc) {
	storeCallback(ih, "_IUPGO_MAP_CB", f)

	C.goIupSetMapFunc(ih.ptr())
}

//--------------------

// UnmapFunc for UNMAP_CB callback.
// Called right before an element is unmapped.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/call/iup_unmap_cb.md
type UnmapFunc func(Ihandle) int

//export goIupUnmapCB
func goIupUnmapCB(ih unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_UNMAP_CB").Value().(UnmapFunc)

	return C.int(f((Ihandle)(ih)))
}

// setUnmapFunc for UNMAP_CB.
func setUnmapFunc(ih Ihandle, f UnmapFunc) {
	storeCallback(ih, "_IUPGO_UNMAP_CB", f)

	C.goIupSetUnmapFunc(ih.ptr())
}

//--------------------

// DestroyFunc for DESTROY_CB callback.
// Called right before an element is destroyed.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/call/iup_destroy_cb.md
type DestroyFunc func(Ihandle) int

//export goIupDestroyCB
func goIupDestroyCB(ih unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_DESTROY_CB").Value().(DestroyFunc)

	return C.int(f((Ihandle)(ih)))
}

// setDestroyFunc for DESTROY_CB.
func setDestroyFunc(ih Ihandle, f DestroyFunc) {
	storeCallback(ih, "_IUPGO_DESTROY_CB", f)

	C.goIupSetDestroyFunc(ih.ptr())
}

//--------------------

// GetFocusFunc for GETFOCUS_CB callback.
// Action generated when an element is given keyboard focus.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/call/iup_getfocus_cb.md
type GetFocusFunc func(Ihandle) int

//export goIupGetFocusCB
func goIupGetFocusCB(ih unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_GETFOCUS_CB").Value().(GetFocusFunc)

	return C.int(f((Ihandle)(ih)))
}

// setGetFocusFunc for GETFOCUS_CB.
func setGetFocusFunc(ih Ihandle, f GetFocusFunc) {
	storeCallback(ih, "_IUPGO_GETFOCUS_CB", f)

	C.goIupSetGetFocusFunc(ih.ptr())
}

//--------------------

// KillFocusFunc for KILLFOCUS_CB callback.
// Action generated when an element loses keyboard focus.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/call/iup_killfocus_cb.md
type KillFocusFunc func(Ihandle) int

//export goIupKillFocusCB
func goIupKillFocusCB(ih unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_KILLFOCUS_CB").Value().(KillFocusFunc)

	return C.int(f((Ihandle)(ih)))
}

// setKillFocusFunc for KILLFOCUS_CB.
func setKillFocusFunc(ih Ihandle, f KillFocusFunc) {
	storeCallback(ih, "_IUPGO_KILLFOCUS_CB", f)

	C.goIupSetKillFocusFunc(ih.ptr())
}

//--------------------

// EnterWindowFunc for ENTERWINDOW_CB callback.
// Action generated when the mouse enters the native element.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/call/iup_enterwindow_cb.md
type EnterWindowFunc func(Ihandle) int

//export goIupEnterWindowCB
func goIupEnterWindowCB(ih unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_ENTERWINDOW_CB").Value().(EnterWindowFunc)

	return C.int(f((Ihandle)(ih)))
}

// setEnterWindowFunc for ENTERWINDOW_CB.
func setEnterWindowFunc(ih Ihandle, f EnterWindowFunc) {
	storeCallback(ih, "_IUPGO_ENTERWINDOW_CB", f)

	C.goIupSetEnterWindowFunc(ih.ptr())
}

//--------------------

// LeaveWindowFunc for LEAVEWINDOW_CB callback.
// Action generated when the mouse leaves the native element.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/call/iup_leavewindow_cb.md
type LeaveWindowFunc func(Ihandle) int

//export goIupLeaveWindowCB
func goIupLeaveWindowCB(ih unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_LEAVEWINDOW_CB").Value().(LeaveWindowFunc)

	return C.int(f((Ihandle)(ih)))
}

// setLeaveWindowFunc for LEAVEWINDOW_CB.
func setLeaveWindowFunc(ih Ihandle, f LeaveWindowFunc) {
	storeCallback(ih, "_IUPGO_LEAVEWINDOW_CB", f)

	C.goIupSetLeaveWindowFunc(ih.ptr())
}

//--------------------

// KAnyFunc for K_ANY callback.
// Action generated when a keyboard event occurs.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/call/iup_k_any.md
type KAnyFunc func(Ihandle, int) int

//export goIupKAnyCB
func goIupKAnyCB(ih unsafe.Pointer, c C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_K_ANY").Value().(KAnyFunc)

	return C.int(f((Ihandle)(ih), int(c)))
}

// setKAnyFunc for K_ANY.
func setKAnyFunc(ih Ihandle, f KAnyFunc) {
	storeCallback(ih, "_IUPGO_K_ANY", f)

	C.goIupSetKAnyFunc(ih.ptr())
}

//--------------------

// HelpFunc for HELP_CB callback.
// Action generated when the user presses F1 at a control.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/call/iup_help_cb.md
type HelpFunc func(Ihandle) int

//export goIupHelpCB
func goIupHelpCB(ih unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_HELP_CB").Value().(HelpFunc)

	return C.int(f((Ihandle)(ih)))
}

// setHelpFunc for HELP_CB.
func setHelpFunc(ih Ihandle, f HelpFunc) {
	storeCallback(ih, "_IUPGO_HELP_CB", f)

	C.goIupSetHelpFunc(ih.ptr())
}

//--------------------

// ActionFunc for ACTION callback.
// Action generated when the element is activated. Affects each element differently.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/call/iup_action.md
type ActionFunc func(Ihandle) int

//export goIupActionCB
func goIupActionCB(ih unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_ACTION").Value().(ActionFunc)

	return C.int(f((Ihandle)(ih)))
}

// setActionFunc for ACTION.
func setActionFunc(ih Ihandle, f ActionFunc) {
	storeCallback(ih, "_IUPGO_ACTION", f)

	C.goIupSetActionFunc(ih.ptr())
}

//--------------------

// ButtonFunc for BUTTON_CB callback.
// Action generated when a mouse button is pressed or released.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/call/iup_button_cb.md
type ButtonFunc func(Ihandle, int, int, int, int, string) int

//export goIupButtonCB
func goIupButtonCB(ih unsafe.Pointer, button, pressed, x, y C.int, status unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_BUTTON_CB").Value().(ButtonFunc)

	goStatus := C.GoString((*C.char)(status))
	return C.int(f((Ihandle)(ih), int(button), int(pressed), int(x), int(y), goStatus))
}

// setButtonFunc for BUTTON_CB.
func setButtonFunc(ih Ihandle, f ButtonFunc) {
	storeCallback(ih, "_IUPGO_BUTTON_CB", f)

	C.goIupSetButtonFunc(ih.ptr())
}

//--------------------

// TouchFunc for TOUCH_CB callback.
// Action generated when a touch event occurred.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/elem/iup_canvas.md
type TouchFunc func(ih Ihandle, id, x, y int, state string) int

//export goIupTouchCB
func goIupTouchCB(ih unsafe.Pointer, id, x, y C.int, state unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_TOUCH_CB").Value().(TouchFunc)

	return C.int(f((Ihandle)(ih), int(id), int(x), int(y), C.GoString((*C.char)(state))))
}

// setTouchFunc for TOUCH_CB.
func setTouchFunc(ih Ihandle, f TouchFunc) {
	storeCallback(ih, "_IUPGO_TOUCH_CB", f)

	C.goIupSetTouchFunc(ih.ptr())
}

//--------------------

// MultiTouchFunc for MULTITOUCH_CB callback.
// Action generated when multiple touch events occurred.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/elem/iup_canvas.md
type MultiTouchFunc func(ih Ihandle, count int, pid, px, py, pstate []int) int

//export goIupMultiTouchCB
func goIupMultiTouchCB(ih unsafe.Pointer, count C.int, pid, px, py, pstate *C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_MULTITOUCH_CB").Value().(MultiTouchFunc)

	n := int(count)
	toGo := func(p *C.int) []int {
		out := make([]int, n)
		for i, v := range unsafe.Slice(p, n) {
			out[i] = int(v)
		}
		return out
	}

	return C.int(f((Ihandle)(ih), n, toGo(pid), toGo(px), toGo(py), toGo(pstate)))
}

// setMultiTouchFunc for MULTITOUCH_CB.
func setMultiTouchFunc(ih Ihandle, f MultiTouchFunc) {
	storeCallback(ih, "_IUPGO_MULTITOUCH_CB", f)

	C.goIupSetMultiTouchFunc(ih.ptr())
}

//--------------------

// DropFilesFunc for DROPFILES_CB callback.
// Action called when a file is "dropped" into control.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/call/iup_dropfiles_cb.md
type DropFilesFunc func(Ihandle, string, int, int, int) int

//export goIupDropFilesCB
func goIupDropFilesCB(ih, filename unsafe.Pointer, num, x, y C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_DROPFILES_CB").Value().(DropFilesFunc)

	goFilename := C.GoString((*C.char)(filename))
	return C.int(f((Ihandle)(ih), goFilename, int(num), int(x), int(y)))
}

// setDropFilesFunc for DROPFILE_CB.
func setDropFilesFunc(ih Ihandle, f DropFilesFunc) {
	storeCallback(ih, "_IUPGO_DROPFILES_CB", f)

	C.goIupSetDropFilesFunc(ih.ptr())
}

//--------------------

// ListActionFunc for List ACTION callback.
// Action generated when the state of an item in the list is changed.
type ListActionFunc func(ih Ihandle, text string, item, state int) int

//export goIupListActionCB
func goIupListActionCB(ih, text unsafe.Pointer, item, state C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_LIST_ACTION").Value().(ListActionFunc)

	goText := C.GoString((*C.char)(text))
	return C.int(f((Ihandle)(ih), goText, int(item), int(state)))
}

// setListActionFunc for List ACTION callback.
func setListActionFunc(ih Ihandle, f ListActionFunc) {
	storeCallback(ih, "_IUPGO_LIST_ACTION", f)

	C.goIupSetListActionFunc(ih.ptr())
}

//--------------------

// ListValueFunc for List VALUE_CB callback in virtual mode.
// Called to get the text value of an item when VIRTUALMODE=YES.
// pos is 1-based item index.
// Returns the string to display for the item.
type ListValueFunc func(ih Ihandle, pos int) string

//export goIupListValueCB
func goIupListValueCB(ih unsafe.Pointer, pos C.int) *C.char {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_LIST_VALUE_CB")
	if ch == 0 {
		return nil
	}
	f := ch.Value().(ListValueFunc)
	result := f((Ihandle)(ih), int(pos))
	if result == "" {
		return nil
	}
	return C.CString(result)
}

// setListValueFunc for List VALUE_CB callback.
func setListValueFunc(ih Ihandle, f ListValueFunc) {
	storeCallback(ih, "_IUPGO_LIST_VALUE_CB", f)
	C.goIupSetListValueFunc(ih.ptr())
}

//--------------------

// ListImageFunc for List IMAGE_CB callback in virtual mode.
// Called to get the image name for an item when VIRTUALMODE=YES and SHOWIMAGE=YES.
// Returns the image name to display for the item, or empty string for no image.
type ListImageFunc func(ih Ihandle, pos int) string

//export goIupListImageCB
func goIupListImageCB(ih unsafe.Pointer, pos C.int) *C.char {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_LIST_IMAGE_CB")
	if ch == 0 {
		return nil
	}
	f := ch.Value().(ListImageFunc)
	result := f((Ihandle)(ih), int(pos))
	if result == "" {
		return nil
	}
	return C.CString(result)
}

// setListImageFunc for List IMAGE_CB callback.
func setListImageFunc(ih Ihandle, f ListImageFunc) {
	storeCallback(ih, "_IUPGO_LIST_IMAGE_CB", f)
	C.goIupSetListImageFunc(ih.ptr())
}

//--------------------

// CaretFunc for CARET_CB callback.
// Action generated when the caret/cursor position is changed.
type CaretFunc func(ih Ihandle, lin, col, pos int) int

//export goIupCaretCB
func goIupCaretCB(ih unsafe.Pointer, lin, col, pos C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_CARET_CB").Value().(CaretFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col), int(pos)))
}

// setCaretFunc for CARET_CB.
func setCaretFunc(ih Ihandle, f CaretFunc) {
	storeCallback(ih, "_IUPGO_CARET_CB", f)

	C.goIupSetCaretFunc(ih.ptr())
}

//--------------------

// DblclickFunc for DBLCLICK_CB callback.
// Action generated when the user double click an item.
type DblclickFunc func(ih Ihandle, item int, text string) int

//export goIupDblclickCB
func goIupDblclickCB(ih unsafe.Pointer, item C.int, text unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_DBLCLICK_CB").Value().(DblclickFunc)

	goText := C.GoString((*C.char)(text))
	return C.int(f((Ihandle)(ih), int(item), goText))
}

// setDblclickFunc for DBLCLICK_CB.
func setDblclickFunc(ih Ihandle, f DblclickFunc) {
	storeCallback(ih, "_IUPGO_DBLCLICK_CB", f)

	C.goIupSetDblclickFunc(ih.ptr())
}

//--------------------

// EditFunc for EDIT_CB callback.
// Action generated when the user double click an item.
type EditFunc func(ih Ihandle, item int, text string) int

//export goIupEditCB
func goIupEditCB(ih unsafe.Pointer, item C.int, text unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_EDIT_CB").Value().(EditFunc)

	goText := C.GoString((*C.char)(text))
	return C.int(f((Ihandle)(ih), int(item), goText))
}

// setEditFunc for EDIT_CB.
func setEditFunc(ih Ihandle, f EditFunc) {
	storeCallback(ih, "_IUPGO_EDIT_CB", f)

	C.goIupSetEditFunc(ih.ptr())
}

//--------------------

// MotionFunc for MOTION_CB callback.
// Action generated when the mouse is moved over the list.
type MotionFunc func(ih Ihandle, x, y int, status string) int

//export goIupMotionCB
func goIupMotionCB(ih unsafe.Pointer, x, y C.int, status unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_MOTION_CB").Value().(MotionFunc)

	goStatus := C.GoString((*C.char)(status))
	return C.int(f((Ihandle)(ih), int(x), int(y), goStatus))
}

// setMotionFunc for MOTION_CB.
func setMotionFunc(ih Ihandle, f MotionFunc) {
	storeCallback(ih, "_IUPGO_MOTION_CB", f)

	C.goIupSetMotionFunc(ih.ptr())
}

//--------------------

// MultiselectFunc for MULTISELECT_CB callback.
// Action generated when the state of an item in the multiple selection list is changed.
type MultiselectFunc func(ih Ihandle, text string) int

//export goIupMultiselectCB
func goIupMultiselectCB(ih, text unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_MULTISELECT_CB").Value().(MultiselectFunc)

	goText := C.GoString((*C.char)(text))
	return C.int(f((Ihandle)(ih), goText))
}

// setMultiselectFunc for MULTISELECT_CB.
func setMultiselectFunc(ih Ihandle, f MultiselectFunc) {
	storeCallback(ih, "_IUPGO_MULTISELECT_CB", f)

	C.goIupSetMultiselectFunc(ih.ptr())
}

//--------------------

// ValueChangedFunc for VALUECHANGED_CB callback.
// Called after the value was interactively changed by the user.
type ValueChangedFunc func(ih Ihandle) int

//export goIupValueChangedCB
func goIupValueChangedCB(ih unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_VALUECHANGED_CB").Value().(ValueChangedFunc)

	return C.int(f((Ihandle)(ih)))
}

// setValueChangedFunc for VALUECHANGED_CB.
func setValueChangedFunc(ih Ihandle, f ValueChangedFunc) {
	storeCallback(ih, "_IUPGO_VALUECHANGED_CB", f)

	C.goIupSetValueChangedFunc(ih.ptr())
}

//--------------------

// TextActionFunc for Text ACTION callback.
// Action generated when the text is edited, but before its value is actually changed.
type TextActionFunc func(ih Ihandle, ch int, newValue string) int

//export goIupTextActionCB
func goIupTextActionCB(ih unsafe.Pointer, c C.int, newValue unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_TEXT_ACTION").Value().(TextActionFunc)

	goNewValue := C.GoString((*C.char)(newValue))
	return C.int(f((Ihandle)(ih), int(c), goNewValue))
}

// setTextActionFunc for Text ACTION.
func setTextActionFunc(ih Ihandle, f TextActionFunc) {
	storeCallback(ih, "_IUPGO_TEXT_ACTION", f)

	C.goIupSetTextActionFunc(ih.ptr())
}

//--------------------

// ToggleActionFunc for Toggle ACTION callback.
// Action generated when the toggle's state (on/off) was changed.
type ToggleActionFunc func(ih Ihandle, state int) int

//export goIupToggleActionCB
func goIupToggleActionCB(ih unsafe.Pointer, state C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_TOGGLE_ACTION").Value().(ToggleActionFunc)

	return C.int(f((Ihandle)(ih), int(state)))
}

// setToggleActionFunc for Toggle ACTION.
func setToggleActionFunc(ih Ihandle, f ToggleActionFunc) {
	storeCallback(ih, "_IUPGO_TOGGLE_ACTION", f)

	C.goIupSetToggleActionFunc(ih.ptr())
}

//--------------------

// TabChangeFunc for TABCHANGE_CB callback.
type TabChangeFunc func(ih, newTab, oldTab Ihandle) int

//export goIupTabChangeCB
func goIupTabChangeCB(ih, newTab, oldTab unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_TABCHANGE_CB").Value().(TabChangeFunc)

	return C.int(f((Ihandle)(ih), (Ihandle)(newTab), (Ihandle)(oldTab)))
}

// setTabChangeFunc for TABCHANGE_CB.
func setTabChangeFunc(ih Ihandle, f TabChangeFunc) {
	storeCallback(ih, "_IUPGO_TABCHANGE_CB", f)

	C.goIupSetTabChangeFunc(ih.ptr())
}

//--------------------

// TabChangePosFunc for TABCHANGEPOS_CB callback.
type TabChangePosFunc func(ih Ihandle, newPos, oldPos int) int

//export goIupTabChangePosCB
func goIupTabChangePosCB(ih unsafe.Pointer, newPos, oldPos C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_TABCHANGEPOS_CB").Value().(TabChangePosFunc)

	return C.int(f((Ihandle)(ih), int(newPos), int(oldPos)))
}

// setTabChangePosFunc for TABCHANGEPOS_CB.
func setTabChangePosFunc(ih Ihandle, f TabChangePosFunc) {
	storeCallback(ih, "_IUPGO_TABCHANGEPOS_CB", f)

	C.goIupSetTabChangePosFunc(ih.ptr())
}

//--------------------

// ReorderFunc for REORDER_CB callback.
type ReorderFunc func(ih Ihandle, oldPos, newPos int) int

//export goIupReorderCB
func goIupReorderCB(ih unsafe.Pointer, oldPos, newPos C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_REORDER_CB").Value().(ReorderFunc)

	return C.int(f((Ihandle)(ih), int(oldPos), int(newPos)))
}

// setReorderFunc for REORDER_CB.
func setReorderFunc(ih Ihandle, f ReorderFunc) {
	storeCallback(ih, "_IUPGO_REORDER_CB", f)

	C.goIupSetReorderFunc(ih.ptr())
}

//--------------------

// SpinFunc for SPIN_CB callback.
type SpinFunc func(ih Ihandle, inc int) int

//export goIupSpinCB
func goIupSpinCB(ih unsafe.Pointer, inc C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_SPIN_CB").Value().(SpinFunc)

	return C.int(f((Ihandle)(ih), int(inc)))
}

// setSpinFunc for SPIN_CB.
func setSpinFunc(ih Ihandle, f SpinFunc) {
	storeCallback(ih, "_IUPGO_SPIN_CB", f)

	C.goIupSetSpinFunc(ih.ptr())
}

//--------------------

// PostMessageFunc for POSTMESSAGE_CB callback.
type PostMessageFunc func(Ihandle, string, int, any) int

//export goIupPostMessageCB
func goIupPostMessageCB(ih unsafe.Pointer, s unsafe.Pointer, i C.int, d C.double, p unsafe.Pointer) C.int {
	_ = d
	f := loadCallback((Ihandle)(ih), "_IUPGO_POSTMESSAGE_CB").Value().(PostMessageFunc)

	var payload any
	if p != nil {
		ph := cgo.Handle(uintptr(p))
		payload = ph.Value()
		ph.Delete()
	}

	return C.int(f((Ihandle)(ih), C.GoString((*C.char)(s)), int(i), payload))
}

// setPostMessageFunc for POSTMESSAGE_CB.
func setPostMessageFunc(ih Ihandle, f PostMessageFunc) {
	storeCallback(ih, "_IUPGO_POSTMESSAGE_CB", f)

	C.goIupSetPostMessageFunc(ih.ptr())
}

//--------------------

// CloseFunc for CLOSE_CB callback.
// Called just before a dialog is closed when the user clicks the close button
// of the title bar or an equivalent action.
type CloseFunc func(Ihandle) int

//export goIupCloseCB
func goIupCloseCB(ih unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_CLOSE_CB").Value().(CloseFunc)

	return C.int(f((Ihandle)(ih)))
}

// setCloseFunc for CLOSE_CB.
func setCloseFunc(ih Ihandle, f CloseFunc) {
	storeCallback(ih, "_IUPGO_CLOSE_CB", f)

	C.goIupSetCloseFunc(ih.ptr())
}

//--------------------

// FocusFunc for FOCUS_CB callback.
// Called when the dialog or any of its children gets the focus, or when
// another dialog or any control in another dialog gets the focus.
type FocusFunc func(Ihandle, int) int

//export goIupFocusCB
func goIupFocusCB(ih unsafe.Pointer, c C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_FOCUS_CB").Value().(FocusFunc)

	return C.int(f((Ihandle)(ih), int(c)))
}

// setFocusFunc for FOCUS_CB.
func setFocusFunc(ih Ihandle, f FocusFunc) {
	storeCallback(ih, "_IUPGO_FOCUS_CB", f)

	C.goIupSetFocusFunc(ih.ptr())
}

//--------------------

// MoveFunc for MOVE_CB callback.
// Called after the dialog was moved on screen.
type MoveFunc func(ih Ihandle, x, y int) int

//export goIupMoveCB
func goIupMoveCB(ih unsafe.Pointer, x, y C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_MOVE_CB").Value().(MoveFunc)

	return C.int(f((Ihandle)(ih), int(x), int(y)))
}

// setMoveFunc for MOVE_CB.
func setMoveFunc(ih Ihandle, f MoveFunc) {
	storeCallback(ih, "_IUPGO_MOVE_CB", f)

	C.goIupSetMoveFunc(ih.ptr())
}

//--------------------

// ResizeFunc for RESIZE_CB callback.
// Action generated when the canvas or dialog size is changed.
type ResizeFunc func(ih Ihandle, width, height int) int

//export goIupResizeCB
func goIupResizeCB(ih unsafe.Pointer, width, height C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_RESIZE_CB").Value().(ResizeFunc)

	return C.int(f((Ihandle)(ih), int(width), int(height)))
}

// setResizeFunc for RESIZE_CB.
func setResizeFunc(ih Ihandle, f ResizeFunc) {
	storeCallback(ih, "_IUPGO_RESIZE_CB", f)

	C.goIupSetResizeFunc(ih.ptr())
}

//--------------------

// ShowFunc for SHOW_CB callback.
// Called right after the dialog is shown, hidden, maximized, minimized or restored from minimized/maximized.
type ShowFunc func(ih Ihandle, state int) int

//export goIupShowCB
func goIupShowCB(ih unsafe.Pointer, inc C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_SHOW_CB").Value().(ShowFunc)

	return C.int(f((Ihandle)(ih), int(inc)))
}

// setShowFunc for SHOW_CB.
func setShowFunc(ih Ihandle, f ShowFunc) {
	storeCallback(ih, "_IUPGO_SHOW_CB", f)

	C.goIupSetShowFunc(ih.ptr())
}

//--------------------

// ChangeFunc for CHANGE_CB callback.
// Called when the user releases the left mouse button over the control, defining the selected color.
type ChangeFunc func(ih Ihandle, r, g, b uint8) int

//export goIupChangeCB
func goIupChangeCB(ih unsafe.Pointer, r, g, b C.uchar) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_CHANGE_CB").Value().(ChangeFunc)

	return C.int(f((Ihandle)(ih), uint8(r), uint8(g), uint8(b)))
}

// setChangeFunc for CHANGE_CB.
func setChangeFunc(ih Ihandle, f ChangeFunc) {
	storeCallback(ih, "_IUPGO_CHANGE_CB", f)

	C.goIupSetChangeFunc(ih.ptr())
}

//--------------------

// DragFunc for DRAG_CB callback.
// Called several times while the color is being changed by dragging the mouse over the control.
type DragFunc func(ih Ihandle, r, g, b uint8) int

//export goIupDragCB
func goIupDragCB(ih unsafe.Pointer, r, g, b C.uchar) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_DRAG_CB").Value().(DragFunc)

	return C.int(f((Ihandle)(ih), uint8(r), uint8(g), uint8(b)))
}

// setDragFunc for DRAG_CB.
func setDragFunc(ih Ihandle, f DragFunc) {
	storeCallback(ih, "_IUPGO_DRAG_CB", f)

	C.goIupSetDragFunc(ih.ptr())
}

//--------------------

// DetachedFunc for DETACHED_CB callback.
type DetachedFunc func(Ihandle, Ihandle, int, int) int

//export goIupDetachedCB
func goIupDetachedCB(ih, newParent unsafe.Pointer, x, y C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_DETACHED_CB").Value().(DetachedFunc)

	return C.int(f((Ihandle)(ih), (Ihandle)(newParent), int(x), int(y)))
}

// setDetachedFunc for DETACHED_CB.
func setDetachedFunc(ih Ihandle, f DetachedFunc) {
	storeCallback(ih, "_IUPGO_DETACHED_CB", f)

	C.goIupSetDetachedFunc(ih.ptr())
}

//--------------------

// RestoredFunc for RESTORED_CB callback.
// Called when the box is restored if RESTOREWHENCLOSED=Yes.
type RestoredFunc func(Ihandle, Ihandle, int, int) int

//export goIupRestoredCB
func goIupRestoredCB(ih, oldParent unsafe.Pointer, x, y C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_RESTORED_CB").Value().(RestoredFunc)

	return C.int(f((Ihandle)(ih), (Ihandle)(oldParent), int(x), int(y)))
}

// setRestoredFunc for RESTORED_CB.
func setRestoredFunc(ih Ihandle, f RestoredFunc) {
	storeCallback(ih, "_IUPGO_RESTORED_CB", f)

	C.goIupSetRestoredFunc(ih.ptr())
}

//--------------------

// SwapBuffersFunc for SWAPBUFFERS_CB callback.
// Action generated when GLSwapBuffers is called.
type SwapBuffersFunc func(Ihandle) int

//export goIupSwapBuffersCB
func goIupSwapBuffersCB(ih unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_SWAPBUFFERS_CB").Value().(SwapBuffersFunc)

	return C.int(f((Ihandle)(ih)))
}

// setSwapBuffersFunc for SWAPBUFFERS_CB.
func setSwapBuffersFunc(ih Ihandle, f SwapBuffersFunc) {
	storeCallback(ih, "_IUPGO_SWAPBUFFERS_CB", f)

	C.goIupSetSwapBuffersFunc(ih.ptr())
}

//--------------------

// CancelFunc for CANCEL_CB callback.
// Action generated when the user clicked on the Cancel button.
type CancelFunc func(Ihandle) int

//export goIupCancelCB
func goIupCancelCB(ih unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_CANCEL_CB").Value().(CancelFunc)

	return C.int(f((Ihandle)(ih)))
}

// setCancelFunc for CANCEL_CB.
func setCancelFunc(ih Ihandle, f CancelFunc) {
	storeCallback(ih, "_IUPGO_CANCEL_CB", f)

	C.goIupSetCancelFunc(ih.ptr())
}

//--------------------

// TimerActionFunc for ACTION_CB callback.
// Called every time the defined time interval is reached.
// To stop the callback from being called, stop the timer with RUN=NO.
type TimerActionFunc func(ih Ihandle) int

//export goIupTimerActionCB
func goIupTimerActionCB(ih unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_TIMER_ACTION").Value().(TimerActionFunc)

	return C.int(f((Ihandle)(ih)))
}

// setTimerActionFunc for ACTION_CB callback.
func setTimerActionFunc(ih Ihandle, f TimerActionFunc) {
	storeCallback(ih, "_IUPGO_TIMER_ACTION", f)

	C.goIupSetTimerActionFunc(ih.ptr())
}

//--------------------

// ThreadFunc for THREAD_CB callback.
// Action generated when the thread is started.
// If this callback returns or does not exist, the thread is terminated.
type ThreadFunc func(ih Ihandle) int

//export goIupThreadCB
func goIupThreadCB(ih unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_THREAD_CB").Value().(ThreadFunc)

	return C.int(f((Ihandle)(ih)))
}

// setThreadFunc for THREAD_CB callback.
func setThreadFunc(ih Ihandle, f ThreadFunc) {
	storeCallback(ih, "_IUPGO_THREAD_CB", f)

	C.goIupSetThreadFunc(ih.ptr())
}

//--------------------

// ScrollFunc for SCROLL_CB callback.
// Called when the scrollbar is manipulated.
type ScrollFunc func(ih Ihandle, op int, posx, posy float64) int

//export goIupScrollCB
func goIupScrollCB(ih unsafe.Pointer, op C.int, posx, posy C.float) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_SCROLL_CB").Value().(ScrollFunc)

	return C.int(f((Ihandle)(ih), int(op), float64(posx), float64(posy)))
}

// setScrollFunc for SCROLL_CB callback.
func setScrollFunc(ih Ihandle, f ScrollFunc) {
	storeCallback(ih, "_IUPGO_SCROLL_CB", f)

	C.goIupSetScrollFunc(ih.ptr())
}

//--------------------

// TrayClickFunc for TRAYCLICK_CB callback.
// Called right after the mouse button is pressed or released over the tray icon.
type TrayClickFunc func(Ihandle, int, int, int) int

//export goIupTrayClickCB
func goIupTrayClickCB(ih unsafe.Pointer, but, pressed, dclick C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_TRAYCLICK_CB").Value().(TrayClickFunc)

	return C.int(f((Ihandle)(ih), int(but), int(pressed), int(dclick)))
}

// setTrayClickFunc for TRAYCLICK_CB.
func setTrayClickFunc(ih Ihandle, f TrayClickFunc) {
	storeCallback(ih, "_IUPGO_TRAYCLICK_CB", f)

	C.goIupSetTrayClickFunc(ih.ptr())
}

//--------------------

// TabCloseFunc for TABCLOSE_CB callback.
// Called when the user clicks on the close button.
type TabCloseFunc func(Ihandle, int) int

//export goIupTabCloseCB
func goIupTabCloseCB(ih unsafe.Pointer, pos C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_TABCLOSE_CB").Value().(TabCloseFunc)

	return C.int(f((Ihandle)(ih), int(pos)))
}

// setTabCloseFunc for TABCLOSE_CB.
func setTabCloseFunc(ih Ihandle, f TabCloseFunc) {
	storeCallback(ih, "_IUPGO_TABCLOSE_CB", f)

	C.goIupSetTabCloseFunc(ih.ptr())
}

//--------------------

// RightClickFunc for RIGHTCLICK_CB callback.
// Called when the user clicks on some tab using the right mouse button.
type RightClickFunc func(Ihandle, int) int

//export goIupRightClickCB
func goIupRightClickCB(ih unsafe.Pointer, pos C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_RIGHTCLICK_CB").Value().(RightClickFunc)

	return C.int(f((Ihandle)(ih), int(pos)))
}

// setRightClickFunc for RIGHTCLICK_CB.
func setRightClickFunc(ih Ihandle, f RightClickFunc) {
	storeCallback(ih, "_IUPGO_RIGHTCLICK_CB", f)

	C.goIupSetRightClickFunc(ih.ptr())
}

//--------------------

// ExtraButtonFunc for EXTRABUTTON_CB callback.
// Action generated when any mouse button is pressed or released.
type ExtraButtonFunc func(Ihandle, int, int) int

//export goIupExtraButtonCB
func goIupExtraButtonCB(ih unsafe.Pointer, button, pressed C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_EXTRABUTTON_CB").Value().(ExtraButtonFunc)

	return C.int(f((Ihandle)(ih), int(button), int(pressed)))
}

// setExtraButtonFunc for EXTRABUTTON_CB.
func setExtraButtonFunc(ih Ihandle, f ExtraButtonFunc) {
	storeCallback(ih, "_IUPGO_EXTRABUTTON_CB", f)

	C.goIupSetExtraButtonFunc(ih.ptr())
}

//--------------------

// OpenCloseFunc for OPENCLOSE_CB callback.
// Action generated before the expander state is interactively changed.
type OpenCloseFunc func(ih Ihandle, state int) int

//export goIupOpenCloseCB
func goIupOpenCloseCB(ih unsafe.Pointer, state C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_OPENCLOSE_CB").Value().(OpenCloseFunc)

	return C.int(f((Ihandle)(ih), int(state)))
}

// setOpenCloseFunc for OPENCLOSE_CB.
func setOpenCloseFunc(ih Ihandle, f OpenCloseFunc) {
	storeCallback(ih, "_IUPGO_OPENCLOSE", f)

	C.goIupSetOpenCloseFunc(ih.ptr())
}

//--------------------

// ValueChangingFunc for VALUECHANGING_CB callback.
// Called when the value starts or ends to be interactively changed by the user.
type ValueChangingFunc func(ih Ihandle, start int) int

//export goIupValueChangingCB
func goIupValueChangingCB(ih unsafe.Pointer, start C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_VALUECHANGING_CB").Value().(ValueChangingFunc)

	return C.int(f((Ihandle)(ih), int(start)))
}

// setValueChangingFunc for VALUECHANGING_CB.
func setValueChangingFunc(ih Ihandle, f ValueChangingFunc) {
	storeCallback(ih, "_IUPGO_VALUECHANGING_CB", f)

	C.goIupSetValueChangingFunc(ih.ptr())
}

//--------------------

// DropDownFunc for DROPDOWN_CB callback.
// Action generated right before the drop child is shown or hidden.
type DropDownFunc func(ih Ihandle, state int) int

//export goIupDropDownCB
func goIupDropDownCB(ih unsafe.Pointer, state C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_DROPDOWN_CB").Value().(DropDownFunc)

	return C.int(f((Ihandle)(ih), int(state)))
}

// setDropDownFunc for DROPDOWN_CB.
func setDropDownFunc(ih Ihandle, f DropDownFunc) {
	storeCallback(ih, "_IUPGO_DROPDOWN_CB", f)

	C.goIupSetDropDownFunc(ih.ptr())
}

//--------------------

// DropShowFunc for DROPSHOW_CB callback.
// Action generated right after the drop child is shown or hidden.
type DropShowFunc func(ih Ihandle, state int) int

//export goIupDropShowCB
func goIupDropShowCB(ih unsafe.Pointer, state C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_DROPSHOW_CB").Value().(DropShowFunc)

	return C.int(f((Ihandle)(ih), int(state)))
}

// setDropShowFunc for DROPSHOW_CB.
func setDropShowFunc(ih Ihandle, f DropShowFunc) {
	storeCallback(ih, "_IUPGO_DROPSHOW_CB", f)

	C.goIupSetDropShowFunc(ih.ptr())
}

//--------------------

// ButtonPressFunc for BUTTON_PRESS_CB callback.
// Called when the user presses the left mouse button over the dial.
type ButtonPressFunc func(ih Ihandle, angle float64) int

//export goIupButtonPressCB
func goIupButtonPressCB(ih unsafe.Pointer, angle C.double) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_BUTTON_PRESS_CB").Value().(ButtonPressFunc)

	return C.int(f((Ihandle)(ih), float64(angle)))
}

// setButtonPressFunc for BUTTON_PRESS_CB.
func setButtonPressFunc(ih Ihandle, f ButtonPressFunc) {
	storeCallback(ih, "_IUPGO_BUTTON_PRESS_CB", f)

	C.goIupSetButtonPressFunc(ih.ptr())
}

//--------------------

// ButtonReleaseFunc for BUTTON_RELEASE_CB callback.
// Called when the user releases the left mouse button after pressing it over the dial.
type ButtonReleaseFunc func(ih Ihandle, angle float64) int

//export goIupButtonReleaseCB
func goIupButtonReleaseCB(ih unsafe.Pointer, angle C.double) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_BUTTON_RELEASE_CB").Value().(ButtonReleaseFunc)

	return C.int(f((Ihandle)(ih), float64(angle)))
}

// setButtonReleaseFunc for BUTTON_RELEASE_CB.
func setButtonReleaseFunc(ih Ihandle, f ButtonReleaseFunc) {
	storeCallback(ih, "_IUPGO_BUTTON_RELEASE_CB", f)

	C.goIupSetButtonReleaseFunc(ih.ptr())
}

//--------------------

// MouseMoveFunc for MOUSEMOVE_CB callback.
type MouseMoveFunc func(ih Ihandle, angle float64) int

//export goIupMouseMoveCB
func goIupMouseMoveCB(ih unsafe.Pointer, angle C.double) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_MOUSEMOVE_CB").Value().(MouseMoveFunc)

	return C.int(f((Ihandle)(ih), float64(angle)))
}

// setMouseMoveFunc for MOUSEMOVE_CB.
func setMouseMoveFunc(ih Ihandle, f MouseMoveFunc) {
	storeCallback(ih, "_IUPGO_MOUSEMOVE_CB", f)

	C.goIupSetMouseMoveFunc(ih.ptr())
}

//--------------------

// KeyPressFunc for KEYPRESS_CB callback.
// Action generated when a key is pressed or released.
type KeyPressFunc func(ih Ihandle, c, press int) int

//export goIupKeyPressCB
func goIupKeyPressCB(ih unsafe.Pointer, c, press C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_KEYPRESS_CB").Value().(KeyPressFunc)

	return C.int(f((Ihandle)(ih), int(c), int(press)))
}

// setKeyPressFunc for KEYPRESS_CB.
func setKeyPressFunc(ih Ihandle, f KeyPressFunc) {
	storeCallback(ih, "_IUPGO_KEYPRESS_CB", f)

	C.goIupSetKeyPressFunc(ih.ptr())
}

//--------------------

// CellFunc for CELL_CB callback.
// Called when the user double clicks a color cell to change its value.
type CellFunc func(ih Ihandle, cell int) int

//export goIupCellCB
func goIupCellCB(ih unsafe.Pointer, cell C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_CELL_CB").Value().(CellFunc)

	return C.int(f((Ihandle)(ih), int(cell)))
}

// setCellFunc for CELL_CB.
func setCellFunc(ih Ihandle, f CellFunc) {
	storeCallback(ih, "_IUPGO_CELL_CB", f)

	C.goIupSetCellFunc(ih.ptr())
}

//--------------------

// ExtendedFunc for EXTENDED_CB callback.
// Called when the user right click a cell with the Shift key pressed.
type ExtendedFunc func(ih Ihandle, cell int) int

//export goIupExtendedCB
func goIupExtendedCB(ih unsafe.Pointer, cell C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_EXTENDED_CB").Value().(ExtendedFunc)

	return C.int(f((Ihandle)(ih), int(cell)))
}

// setExtendedFunc for EXTENDED_CB.
func setExtendedFunc(ih Ihandle, f ExtendedFunc) {
	storeCallback(ih, "_IUPGO_EXTENDED_CB", f)

	C.goIupSetExtendedFunc(ih.ptr())
}

//--------------------

// SelectFunc for SELECT_CB callback.
// Called when a color is selected.
type SelectFunc func(ih Ihandle, cell, _type int) int

//export goIupSelectCB
func goIupSelectCB(ih unsafe.Pointer, cell, _type C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_SELECT_CB").Value().(SelectFunc)

	return C.int(f((Ihandle)(ih), int(cell), int(_type)))
}

// setSelectFunc for SELECT_CB.
func setSelectFunc(ih Ihandle, f SelectFunc) {
	storeCallback(ih, "_IUPGO_SELECT_CB", f)

	C.goIupSetSelectFunc(ih.ptr())
}

//--------------------

// SwitchFunc for SWITCH_CB callback.
// Called when the user double clicks the preview area outside the preview cells
// to switch the primary and secondary selections.
type SwitchFunc func(ih Ihandle, primCell, secCell int) int

//export goIupSwitchCB
func goIupSwitchCB(ih unsafe.Pointer, primCell, secCell C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_SWITCH_CB").Value().(SwitchFunc)

	return C.int(f((Ihandle)(ih), int(primCell), int(secCell)))
}

// setSwitchFunc for SWITCH_CB.
func setSwitchFunc(ih Ihandle, f SwitchFunc) {
	storeCallback(ih, "_IUPGO_SWITCH_CB", f)

	C.goIupSetSwitchFunc(ih.ptr())
}

//--------------------

// LinkActionFunc for Link ACTION callback.
type LinkActionFunc func(ih Ihandle, url string) int

//export goIupLinkActionCB
func goIupLinkActionCB(ih, url unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_LINK_ACTION").Value().(LinkActionFunc)

	goUrl := C.GoString((*C.char)(url))
	return C.int(f((Ihandle)(ih), goUrl))
}

// setLinkActionFunc for Link ACTION callback.
func setLinkActionFunc(ih Ihandle, f LinkActionFunc) {
	storeCallback(ih, "_IUPGO_LINK_ACTION", f)

	C.goIupSetLinkActionFunc(ih.ptr())
}

//--------------------

// TextLinkFunc for LINK_CB callback on multiline text controls.
// Action generated when a hyperlink in formatted text is clicked.
type TextLinkFunc func(ih Ihandle, url string) int

//export goIupTextLinkCB
func goIupTextLinkCB(ih, url unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_LINK_CB").Value().(TextLinkFunc)

	goUrl := C.GoString((*C.char)(url))
	return C.int(f((Ihandle)(ih), goUrl))
}

func setTextLinkFunc(ih Ihandle, f TextLinkFunc) {
	storeCallback(ih, "_IUPGO_LINK_CB", f)

	C.goIupSetTextLinkFunc(ih.ptr())
}

//--------------------

// MaskFailFunc for MASKFAIL_CB callback.
// Action generated when the new text fails at the mask check.
type MaskFailFunc func(ih Ihandle, newValue string) int

//export goIupMaskFailCB
func goIupMaskFailCB(ih, newValue unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_MASKFAIL_CB").Value().(MaskFailFunc)

	goNewValue := C.GoString((*C.char)(newValue))
	return C.int(f((Ihandle)(ih), goNewValue))
}

func setMaskFailFunc(ih Ihandle, f MaskFailFunc) {
	storeCallback(ih, "_IUPGO_MASKFAIL_CB", f)

	C.goIupSetMaskFailFunc(ih.ptr())
}

//--------------------

// WheelFunc for WHEEL_CB callback.
// Action generated when the mouse wheel is rotated.
type WheelFunc func(ih Ihandle, delta float64, x, y int, status string) int

//export goIupWheelCB
func goIupWheelCB(ih unsafe.Pointer, delta C.float, x, y C.int, status unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_WHEEL_CB").Value().(WheelFunc)

	goStatus := C.GoString((*C.char)(status))
	return C.int(f((Ihandle)(ih), float64(delta), int(x), int(y), goStatus))
}

// setWheelFunc for WHEEL_CB callback.
func setWheelFunc(ih Ihandle, f WheelFunc) {
	storeCallback(ih, "_IUPGO_WHEEL_CB", f)

	C.goIupSetWheelFunc(ih.ptr())
}

//--------------------

// DragDropFunc for DRAGDROP_CB callback.
// Action generated when an internal drag and drop is executed.
type DragDropFunc func(ih Ihandle, dragId, dropId, isShift, isControl int) int

//export goIupDragDropCB
func goIupDragDropCB(ih unsafe.Pointer, dragId, dropId, isShift, isControl C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_DRAGDROP_CB").Value().(DragDropFunc)

	return C.int(f((Ihandle)(ih), int(dragId), int(dropId), int(isShift), int(isControl)))
}

// setDragDropFunc for DRAGDROP_CB.
func setDragDropFunc(ih Ihandle, f DragDropFunc) {
	storeCallback(ih, "_IUPGO_DRAGDROP_CB", f)

	C.goIupSetDragDropFunc(ih.ptr())
}

//--------------------

// DragBeginFunc for DRAGBEGIN_CB callback.
// Called when drag operation begins.
type DragBeginFunc func(ih Ihandle, x, y int) int

//export goIupDragBeginCB
func goIupDragBeginCB(ih unsafe.Pointer, x, y C.int) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_DRAGBEGIN_CB")
	if ch == 0 {
		return C.IUP_DEFAULT
	}

	f := ch.Value().(DragBeginFunc)

	return C.int(f((Ihandle)(ih), int(x), int(y)))
}

// setDragBeginFunc for DRAGBEGIN_CB.
func setDragBeginFunc(ih Ihandle, f DragBeginFunc) {
	storeCallback(ih, "_IUPGO_DRAGBEGIN_CB", f)

	C.goIupSetDragBeginFunc(ih.ptr())
}

//--------------------

// DragDataSizeFunc for DRAGDATASIZE_CB callback.
// Called to query the size of drag data.
type DragDataSizeFunc func(ih Ihandle, dragType string) int

//export goIupDragDataSizeCB
func goIupDragDataSizeCB(ih unsafe.Pointer, dragType *C.char) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_DRAGDATASIZE_CB")
	if ch == 0 {
		return C.IUP_DEFAULT
	}

	f := ch.Value().(DragDataSizeFunc)

	return C.int(f((Ihandle)(ih), C.GoString(dragType)))
}

// setDragDataSizeFunc for DRAGDATASIZE_CB.
func setDragDataSizeFunc(ih Ihandle, f DragDataSizeFunc) {
	storeCallback(ih, "_IUPGO_DRAGDATASIZE_CB", f)

	C.goIupSetDragDataSizeFunc(ih.ptr())
}

//--------------------

// DragDataFunc for DRAGDATA_CB callback.
// Called to get drag data.
type DragDataFunc func(ih Ihandle, dragType string, data unsafe.Pointer, size int) int

//export goIupDragDataCB
func goIupDragDataCB(ih unsafe.Pointer, dragType *C.char, data unsafe.Pointer, size C.int) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_DRAGDATA_CB")
	if ch == 0 {
		return C.IUP_DEFAULT
	}

	f := ch.Value().(DragDataFunc)

	return C.int(f((Ihandle)(ih), C.GoString(dragType), data, int(size)))
}

// setDragDataFunc for DRAGDATA_CB.
func setDragDataFunc(ih Ihandle, f DragDataFunc) {
	storeCallback(ih, "_IUPGO_DRAGDATA_CB", f)

	C.goIupSetDragDataFunc(ih.ptr())
}

//--------------------

// DragEndFunc for DRAGEND_CB callback.
// Called when drag operation ends.
type DragEndFunc func(ih Ihandle, action int) int

//export goIupDragEndCB
func goIupDragEndCB(ih unsafe.Pointer, action C.int) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_DRAGEND_CB")
	if ch == 0 {
		return C.IUP_DEFAULT
	}

	f := ch.Value().(DragEndFunc)

	return C.int(f((Ihandle)(ih), int(action)))
}

// setDragEndFunc for DRAGEND_CB.
func setDragEndFunc(ih Ihandle, f DragEndFunc) {
	storeCallback(ih, "_IUPGO_DRAGEND_CB", f)

	C.goIupSetDragEndFunc(ih.ptr())
}

//--------------------

// DropDataFunc for DROPDATA_CB callback.
// Called when data is dropped.
type DropDataFunc func(ih Ihandle, dragType string, data unsafe.Pointer, size, x, y int) int

//export goIupDropDataCB
func goIupDropDataCB(ih unsafe.Pointer, dragType *C.char, data unsafe.Pointer, size, x, y C.int) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_DROPDATA_CB")
	if ch == 0 {
		return C.IUP_DEFAULT
	}

	f := ch.Value().(DropDataFunc)

	return C.int(f((Ihandle)(ih), C.GoString(dragType), data, int(size), int(x), int(y)))
}

// setDropDataFunc for DROPDATA_CB.
func setDropDataFunc(ih Ihandle, f DropDataFunc) {
	storeCallback(ih, "_IUPGO_DROPDATA_CB", f)

	C.goIupSetDropDataFunc(ih.ptr())
}

//--------------------

// DropMotionFunc for DROPMOTION_CB callback.
// Called during drag motion over drop target.
type DropMotionFunc func(ih Ihandle, x, y int, status string) int

//export goIupDropMotionCB
func goIupDropMotionCB(ih unsafe.Pointer, x, y C.int, status *C.char) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_DROPMOTION_CB")
	if ch == 0 {
		return C.IUP_DEFAULT
	}

	f := ch.Value().(DropMotionFunc)

	return C.int(f((Ihandle)(ih), int(x), int(y), C.GoString(status)))
}

// setDropMotionFunc for DROPMOTION_CB.
func setDropMotionFunc(ih Ihandle, f DropMotionFunc) {
	storeCallback(ih, "_IUPGO_DROPMOTION_CB", f)

	C.goIupSetDropMotionFunc(ih.ptr())
}

//--------------------

// SelectionFunc for SELECTION_CB callback.
// Action generated when a node is selected or deselected.
type SelectionFunc func(ih Ihandle, id, status int) int

//export goIupSelectionCB
func goIupSelectionCB(ih unsafe.Pointer, id, status C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_SELECTION_CB").Value().(SelectionFunc)

	return C.int(f((Ihandle)(ih), int(id), int(status)))
}

// setSelectionFunc for SELECTION_CB.
func setSelectionFunc(ih Ihandle, f SelectionFunc) {
	storeCallback(ih, "_IUPGO_SELECTION_CB", f)

	C.goIupSetSelectionFunc(ih.ptr())
}

//--------------------

// BranchOpenFunc for BRANCHOPEN_CB callback.
// Action generated when a branch is expanded.
type BranchOpenFunc func(ih Ihandle, id int) int

//export goIupBranchOpenCB
func goIupBranchOpenCB(ih unsafe.Pointer, id C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_BRANCHOPEN_CB").Value().(BranchOpenFunc)

	return C.int(f((Ihandle)(ih), int(id)))
}

// setBranchOpenFunc for BRANCHOPEN_CB.
func setBranchOpenFunc(ih Ihandle, f BranchOpenFunc) {
	storeCallback(ih, "_IUPGO_BRANCHOPEN_CB", f)

	C.goIupSetBranchOpenFunc(ih.ptr())
}

//--------------------

// BranchCloseFunc for BRANCHCLOSE_CB callback.
// Action generated when a branch is collapsed.
type BranchCloseFunc func(ih Ihandle, id int) int

//export goIupBranchCloseCB
func goIupBranchCloseCB(ih unsafe.Pointer, id C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_BRANCHCLOSE_CB").Value().(BranchCloseFunc)

	return C.int(f((Ihandle)(ih), int(id)))
}

// setBranchCloseFunc for BRANCHCLOSE_CB.
func setBranchCloseFunc(ih Ihandle, f BranchCloseFunc) {
	storeCallback(ih, "_IUPGO_BRANCHCLOSE_CB", f)

	C.goIupSetBranchCloseFunc(ih.ptr())
}

//--------------------

// ExecuteLeafFunc for EXECUTELEAF_CB callback.
// Action generated when a leaf is executed.
type ExecuteLeafFunc func(ih Ihandle, id int) int

//export goIupExecuteLeafCB
func goIupExecuteLeafCB(ih unsafe.Pointer, id C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_EXECUTELEAF_CB").Value().(ExecuteLeafFunc)

	return C.int(f((Ihandle)(ih), int(id)))
}

// setExecuteLeafFunc for EXECUTELEAF_CB.
func setExecuteLeafFunc(ih Ihandle, f ExecuteLeafFunc) {
	storeCallback(ih, "_IUPGO_EXECUTELEAF_CB", f)

	C.goIupSetExecuteLeafFunc(ih.ptr())
}

//--------------------

// ExecuteBranchFunc for EXECUTEBRANCH_CB callback.
// Action generated when a branch is executed.
type ExecuteBranchFunc func(ih Ihandle, id int) int

//export goIupExecuteBranchCB
func goIupExecuteBranchCB(ih unsafe.Pointer, id C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_EXECUTEBRANCH_CB").Value().(ExecuteBranchFunc)

	return C.int(f((Ihandle)(ih), int(id)))
}

// setExecuteBranchFunc for EXECUTEBRANCH_CB.
func setExecuteBranchFunc(ih Ihandle, f ExecuteBranchFunc) {
	storeCallback(ih, "_IUPGO_EXECUTEBRANCH_CB", f)

	C.goIupSetExecuteBranchFunc(ih.ptr())
}

//--------------------

// ShowRenameFunc for SHOWRENAME_CB callback.
// Action generated when a node is about to be renamed.
type ShowRenameFunc func(ih Ihandle, id int) int

//export goIupShowRenameCB
func goIupShowRenameCB(ih unsafe.Pointer, id C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_SHOWRENAME_CB").Value().(ShowRenameFunc)

	return C.int(f((Ihandle)(ih), int(id)))
}

// setShowRenameFunc for SHOWRENAME_CB.
func setShowRenameFunc(ih Ihandle, f ShowRenameFunc) {
	storeCallback(ih, "_IUPGO_SHOWRENAME_CB", f)

	C.goIupSetShowRenameFunc(ih.ptr())
}

//--------------------

// RenameFunc for RENAME_CB callback.
// Action generated after a node was renamed in place.
type RenameFunc func(ih Ihandle, id int, title string) int

//export goIupRenameCB
func goIupRenameCB(ih unsafe.Pointer, id C.int, title unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_RENAME_CB").Value().(RenameFunc)

	goTitle := C.GoString((*C.char)(title))
	return C.int(f((Ihandle)(ih), int(id), goTitle))
}

// setRenameFunc for RENAME_CB.
func setRenameFunc(ih Ihandle, f RenameFunc) {
	storeCallback(ih, "_IUPGO_RENAME_CB", f)

	C.goIupSetRenameFunc(ih.ptr())
}

//--------------------

// ToggleValueFunc for TOGGLEVALUE_CB callback.
// Action generated when the toggle's state was changed.
type ToggleValueFunc func(ih Ihandle, id, state int) int

//export goIupToggleValueCB
func goIupToggleValueCB(ih unsafe.Pointer, id, state C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_TOGGLEVALUE_CB").Value().(ToggleValueFunc)

	return C.int(f((Ihandle)(ih), int(id), int(state)))
}

// setToggleValueFunc for TOGGLEVALUE_CB.
func setToggleValueFunc(ih Ihandle, f ToggleValueFunc) {
	storeCallback(ih, "_IUPGO_TOGGLEVALUE_CB", f)

	C.goIupSetToggleValueFunc(ih.ptr())
}

//--------------------

// NodeRemovedFunc for NODEREMOVED_CB callback. userId is the USERDATA pointer
// that was set on the node via TreeSetUserId; useful for releasing whatever it
// pointed at (e.g. cgo.Handle(userId).Delete()).
type NodeRemovedFunc func(ih Ihandle, userId uintptr) int

//export goIupNodeRemovedCB
func goIupNodeRemovedCB(ih, userData unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_NODEREMOVED_CB").Value().(NodeRemovedFunc)

	return C.int(f((Ihandle)(ih), uintptr(userData)))
}

// setNodeRemovedFunc for NODEREMOVED_CB callback.
func setNodeRemovedFunc(ih Ihandle, f NodeRemovedFunc) {
	storeCallback(ih, "_IUPGO_NODEREMOVED_CB", f)

	C.goIupSetNodeRemovedFunc(ih.ptr())
}

//--------------------

// MultiSelectionFunc for MULTISELECTION_CB callback.
// Action generated after a continuous range of nodes is selected in one single operation.
type MultiSelectionFunc func(ih Ihandle, ids []int, n int) int

//export goIupMultiSelectionCB
func goIupMultiSelectionCB(ih unsafe.Pointer, ids *C.int, n C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_MULTISELECTION_CB").Value().(MultiSelectionFunc)

	goIds := unsafe.Slice((*int)(unsafe.Pointer(ids)), n)

	return C.int(f((Ihandle)(ih), goIds, int(n)))
}

// setMultiSelectionFunc for MULTISELECTION_CB.
func setMultiSelectionFunc(ih Ihandle, f MultiSelectionFunc) {
	storeCallback(ih, "_IUPGO_MULTISELECTION_CB", f)

	C.goIupSetMultiSelectionFunc(ih.ptr())
}

//--------------------

// MultiUnselectionFunc for MULTIUNSELECTION_CB callback.
// Action generated before multiple nodes are unselected in one single operation.
type MultiUnselectionFunc func(ih Ihandle, ids []int, n int) int

//export goIupMultiUnselectionCB
func goIupMultiUnselectionCB(ih unsafe.Pointer, ids *C.int, n C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_MULTIUNSELECTION_CB").Value().(MultiUnselectionFunc)

	goIds := unsafe.Slice((*int)(unsafe.Pointer(ids)), n)

	return C.int(f((Ihandle)(ih), goIds, int(n)))
}

// setMultiUnselectionFunc for MULTIUNSELECTION_CB.
func setMultiUnselectionFunc(ih Ihandle, f MultiUnselectionFunc) {
	storeCallback(ih, "_IUPGO_MULTIUNSELECTION_CB", f)

	C.goIupSetMultiUnselectionFunc(ih.ptr())
}

//--------------------

// MenuOpenFunc for MENUOPEN_CB callback.
// Called just before the menu is opened.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/call/iup_menuopen_cb.md
type MenuOpenFunc func(ih Ihandle) int

//export goIupMenuOpenCB
func goIupMenuOpenCB(ih unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_MENUOPEN_CB").Value().(MenuOpenFunc)

	return C.int(f((Ihandle)(ih)))
}

// setMenuOpenFunc for MENUOPEN_CB.
func setMenuOpenFunc(ih Ihandle, f MenuOpenFunc) {
	storeCallback(ih, "_IUPGO_MENUOPEN_CB", f)

	C.goIupSetMenuOpenFunc(ih.ptr())
}

//--------------------

// ThemeChangedFunc for THEMECHANGED_CB callback.
// Action generated when the user changes the UI theme.
type ThemeChangedFunc func(ih Ihandle, darkMode int) int

//export goIupThemeChangedCB
func goIupThemeChangedCB(ih unsafe.Pointer, darkMode C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_THEMECHANGED_CB").Value().(ThemeChangedFunc)

	return C.int(f((Ihandle)(ih), int(darkMode)))
}

// setThemeChangedFunc for THEMECHANGED_CB callback.
func setThemeChangedFunc(ih Ihandle, f ThemeChangedFunc) {
	storeCallback(ih, "_IUPGO_THEMECHANGED_CB", f)

	C.goIupSetThemeChangedFunc(ih.ptr())
}

//--------------------

//--------------------

// UpdateFunc for UPDATE_CB callback.
// Action generated when the selection was changed and the editor interface needs an update.
type UpdateFunc func(ih Ihandle) int

//export goIupUpdateCB
func goIupUpdateCB(ih unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_UPDATE_CB").Value().(UpdateFunc)

	return C.int(f((Ihandle)(ih)))
}

// setUpdateFunc for UPDATE_CB.
func setUpdateFunc(ih Ihandle, f UpdateFunc) {
	storeCallback(ih, "_IUPGO_UPDATE_CB", f)

	C.goIupSetUpdateFunc(ih.ptr())
}

//--------------------

// TableEditionFunc for EDITION_CB callback.
// Called when a cell is being edited in a table/matrix control.
// update: The new value entered by the user.
type TableEditionFunc func(ih Ihandle, lin, col int, update string) int

//export goIupTableEditionCB
func goIupTableEditionCB(ih unsafe.Pointer, lin, col C.int, update *C.char) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_EDITION_CB").Value().(TableEditionFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col), C.GoString(update)))
}

// setTableEditionFunc for EDITION_CB (Table version).
func setTableEditionFunc(ih Ihandle, f TableEditionFunc) {
	storeCallback(ih, "_IUPGO_EDITION_CB", f)
	C.goIupSetTableEditionFunc(ih.ptr())
}

//--------------------

// TableValueChangedFunc for VALUECHANGED_CB callback.
// Called after a cell value has been changed in a table/matrix control.
// Triggered by edits, paste operations, or programmatic value changes.
type TableValueChangedFunc func(ih Ihandle, lin, col int) int

//export goIupTableValueChangedCB
func goIupTableValueChangedCB(ih unsafe.Pointer, lin, col C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_TABLEVALUECHANGED_CB").Value().(TableValueChangedFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col)))
}

// setTableValueChangedFunc for VALUECHANGED_CB (Table version).
func setTableValueChangedFunc(ih Ihandle, f TableValueChangedFunc) {
	storeCallback(ih, "_IUPGO_TABLEVALUECHANGED_CB", f)
	C.goIupSetTableValueChangedFunc(ih.ptr())
}

//--------------------

// TableValueFunc for VALUE_CB callback.
// Called to get the value of a cell in virtual mode.
// Returns the string value to display in the cell.
type TableValueFunc func(ih Ihandle, lin, col int) string

//export goIupTableValueCB
func goIupTableValueCB(ih unsafe.Pointer, lin, col C.int) *C.char {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_VALUE_CB")
	if ch == 0 {
		return nil
	}
	f := ch.Value().(TableValueFunc)
	result := f((Ihandle)(ih), int(lin), int(col))
	if result == "" {
		return nil
	}
	return C.CString(result)
}

// setTableValueFunc for VALUE_CB (Table version).
func setTableValueFunc(ih Ihandle, f TableValueFunc) {
	storeCallback(ih, "_IUPGO_VALUE_CB", f)
	C.goIupSetTableValueFunc(ih.ptr())
}

//--------------------

// TableImageFunc for IMAGE_CB callback.
// Called to get the image name for a cell in virtual mode when SHOWIMAGE=YES.
// Returns the image name to display in the cell, or empty string for no image.
type TableImageFunc func(ih Ihandle, lin, col int) string

//export goIupTableImageCB
func goIupTableImageCB(ih unsafe.Pointer, lin, col C.int) *C.char {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_TABLE_IMAGE_CB")
	if ch == 0 {
		return nil
	}
	f := ch.Value().(TableImageFunc)
	result := f((Ihandle)(ih), int(lin), int(col))
	if result == "" {
		return nil
	}
	return C.CString(result)
}

func setTableImageFunc(ih Ihandle, f TableImageFunc) {
	storeCallback(ih, "_IUPGO_TABLE_IMAGE_CB", f)
	C.goIupSetTableImageFunc(ih.ptr())
}

// TableSortFunc for SORT_CB callback.
// Called when user clicks a column header to sort.
// Returns DEFAULT to proceed with sorting or IGNORE to cancel.
type TableSortFunc func(ih Ihandle, col int) int

//export goIupTableSortCB
func goIupTableSortCB(ih unsafe.Pointer, col C.int) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_SORT_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(TableSortFunc)

	return C.int(f((Ihandle)(ih), int(col)))
}

// setTableSortFunc for SORT_CB (Table version).
func setTableSortFunc(ih Ihandle, f TableSortFunc) {
	storeCallback(ih, "_IUPGO_SORT_CB", f)
	C.goIupSetTableSortFunc(ih.ptr())
}

//--------------------

// FileFunc for FILE_CB callback.
// Called when a file is selected in FileDlg.
// status: "OK", "NEW", or "CANCEL"
type FileFunc func(ih Ihandle, filename, status string) int

//export goIupFileCB
func goIupFileCB(ih unsafe.Pointer, filename, status *C.char) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_FILE_CB")
	if ch == 0 {
		return C.IUP_DEFAULT
	}

	f := ch.Value().(FileFunc)

	return C.int(f((Ihandle)(ih), C.GoString(filename), C.GoString(status)))
}

// setFileFunc for FILE_CB.
func setFileFunc(ih Ihandle, f FileFunc) {
	storeCallback(ih, "_IUPGO_FILE_CB", f)

	C.goIupSetFileFunc(ih.ptr())
}

//--------------------

// LayoutUpdateFunc for LAYOUTUPDATE_CB callback.
// Called when layout is updated in FlatScrollBox or ScrollBox.
type LayoutUpdateFunc func(ih Ihandle) int

//export goIupLayoutUpdateCB
func goIupLayoutUpdateCB(ih unsafe.Pointer) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_LAYOUTUPDATE_CB")
	if ch == 0 {
		return C.IUP_DEFAULT
	}

	f := ch.Value().(LayoutUpdateFunc)

	return C.int(f((Ihandle)(ih)))
}

// setLayoutUpdateFunc for LAYOUTUPDATE_CB.
func setLayoutUpdateFunc(ih Ihandle, f LayoutUpdateFunc) {
	storeCallback(ih, "_IUPGO_LAYOUTUPDATE_CB", f)

	C.goIupSetLayoutUpdateFunc(ih.ptr())
}

//--------------------

// HighlightFunc for HIGHLIGHT_CB callback.
// Called when a menu item is highlighted.
type HighlightFunc func(ih Ihandle) int

//export goIupHighlightCB
func goIupHighlightCB(ih unsafe.Pointer) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_HIGHLIGHT_CB")
	if ch == 0 {
		return C.IUP_DEFAULT
	}

	f := ch.Value().(HighlightFunc)

	return C.int(f((Ihandle)(ih)))
}

// setHighlightFunc for HIGHLIGHT_CB.
func setHighlightFunc(ih Ihandle, f HighlightFunc) {
	storeCallback(ih, "_IUPGO_HIGHLIGHT_CB", f)

	C.goIupSetHighlightFunc(ih.ptr())
}

//--------------------

// MenuCloseFunc for MENUCLOSE_CB callback.
// Called when a menu is closed.
type MenuCloseFunc func(ih Ihandle) int

//export goIupMenuCloseCB
func goIupMenuCloseCB(ih unsafe.Pointer) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_MENUCLOSE_CB")
	if ch == 0 {
		return C.IUP_DEFAULT
	}

	f := ch.Value().(MenuCloseFunc)

	return C.int(f((Ihandle)(ih)))
}

// setMenuCloseFunc for MENUCLOSE_CB.
func setMenuCloseFunc(ih Ihandle, f MenuCloseFunc) {
	storeCallback(ih, "_IUPGO_MENUCLOSE_CB", f)

	C.goIupSetMenuCloseFunc(ih.ptr())
}

//--------------------

// ColorUpdateFunc for COLORUPDATE_CB callback.
// Called when color is updated in ColorDlg.
type ColorUpdateFunc func(ih Ihandle) int

//export goIupColorUpdateCB
func goIupColorUpdateCB(ih unsafe.Pointer) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_COLORUPDATE_CB")
	if ch == 0 {
		return C.IUP_DEFAULT
	}

	f := ch.Value().(ColorUpdateFunc)

	return C.int(f((Ihandle)(ih)))
}

// setColorUpdateFunc for COLORUPDATE_CB.
func setColorUpdateFunc(ih Ihandle, f ColorUpdateFunc) {
	storeCallback(ih, "_IUPGO_COLORUPDATE_CB", f)

	C.goIupSetColorUpdateFunc(ih.ptr())
}

//--------------------

//export goIupRecentCB
func goIupRecentCB(ih unsafe.Pointer) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_RECENT_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(ActionFunc)
	return C.int(f((Ihandle)(ih)))
}

// setRecentFunc stores the callback for recent file selection.
// The callback is stored on the config handle.
func setRecentFunc(config Ihandle, f ActionFunc) {
	storeCallback(config, "_IUPGO_RECENT_CB", f)
}

//--------------------

// NotifyFunc for NOTIFY_CB callback.
// Called when the notification is activated (clicked or action button pressed).
// action_id: 0=body clicked, 1-4=action buttons
type NotifyFunc func(ih Ihandle, actionId int) int

//export goIupNotifyCB
func goIupNotifyCB(ih unsafe.Pointer, actionId C.int) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_NOTIFY_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(NotifyFunc)
	return C.int(f((Ihandle)(ih), int(actionId)))
}

// setNotifyFunc for NOTIFY_CB.
func setNotifyFunc(ih Ihandle, f NotifyFunc) {
	storeCallback(ih, "_IUPGO_NOTIFY_CB", f)
	C.goIupSetNotifyFunc(ih.ptr())
}

//--------------------

// NotifyCloseFunc for CLOSE_CB callback on notification.
// Called when the notification is dismissed.
// reason: 1=expired, 2=user dismissed, 3=closed programmatically, 4=unknown
type NotifyCloseFunc func(ih Ihandle, reason int) int

//export goIupNotifyCloseCB
func goIupNotifyCloseCB(ih unsafe.Pointer, reason C.int) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_NOTIFY_CLOSE_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(NotifyCloseFunc)
	return C.int(f((Ihandle)(ih), int(reason)))
}

// setNotifyCloseFunc for CLOSE_CB on notifications.
func setNotifyCloseFunc(ih Ihandle, f NotifyCloseFunc) {
	storeCallback(ih, "_IUPGO_NOTIFY_CLOSE_CB", f)
	C.goIupSetNotifyCloseFunc(ih.ptr())
}

//--------------------

// GetParamFunc is the callback for GetParam dialog.
// It is called when a parameter value changes, a button is pressed, or the dialog is mapped/initialized.
// The paramIndex is >= 0 for parameter changes, or one of the GETPARAM_* constants for events.
// Return 1 to accept the change or 0 to reject it.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/dlg/iup_getparam.md
type GetParamFunc func(dialog Ihandle, paramIndex int) int

//export goIupGetParamCB
func goIupGetParamCB(dialog unsafe.Pointer, paramIndex C.int, userData unsafe.Pointer) C.int {
	ch := cgo.Handle(userData)
	f := ch.Value().(GetParamFunc)
	return C.int(f(Ihandle(dialog), int(paramIndex)))
}
