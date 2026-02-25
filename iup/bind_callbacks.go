package iup

import (
	"fmt"
	"runtime/cgo"
	"sync"
	"sync/atomic"
	"unsafe"
)

/*
#include "bind_callbacks.h"
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

// ListValueFunc for List VALUE_CB callback in virtual mode.
// Called to get the text value of an item when VIRTUALMODE=YES.
// pos is 1-based item index.
// Returns the string to display for the item.
type ListValueFunc func(ih Ihandle, pos int) string

//export goIupListValueCB
func goIupListValueCB(ih unsafe.Pointer, pos C.int) *C.char {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("LIST_VALUE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "LIST_VALUE_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(ListValueFunc)
	result := f((Ihandle)(ih), int(pos))
	if result == "" {
		return nil
	}
	return C.CString(result)
}

// setListValueFunc for List VALUE_CB callback.
func setListValueFunc(ih Ihandle, f ListValueFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("LIST_VALUE_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetListValueFunc(ih.ptr())
}

//--------------------

// ListImageFunc for List IMAGE_CB callback in virtual mode.
// Called to get the image name for an item when VIRTUALMODE=YES and SHOWIMAGE=YES.
// Returns the image name to display for the item, or empty string for no image.
type ListImageFunc func(ih Ihandle, pos int) string

//export goIupListImageCB
func goIupListImageCB(ih unsafe.Pointer, pos C.int) *C.char {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("LIST_IMAGE_CB_" + uuid)
	if !ok {
		return nil
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(ListImageFunc)
	result := f((Ihandle)(ih), int(pos))
	if result == "" {
		return nil
	}
	return C.CString(result)
}

// setListImageFunc for List IMAGE_CB callback.
func setListImageFunc(ih Ihandle, f ListImageFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("LIST_IMAGE_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetListImageFunc(ih.ptr())
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

// DragBeginFunc for DRAGBEGIN_CB callback.
// Called when drag operation begins.
type DragBeginFunc func(ih Ihandle, x, y int) int

//export goIupDragBeginCB
func goIupDragBeginCB(ih unsafe.Pointer, x, y C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("DRAGBEGIN_CB_" + uuid)
	if !ok {
		return C.IUP_DEFAULT
	}

	f := cgo.Handle(h.(cgo.Handle)).Value().(DragBeginFunc)

	return C.int(f((Ihandle)(ih), int(x), int(y)))
}

// setDragBeginFunc for DRAGBEGIN_CB.
func setDragBeginFunc(ih Ihandle, f DragBeginFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("DRAGBEGIN_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetDragBeginFunc(ih.ptr())
}

//--------------------

// DragDataSizeFunc for DRAGDATASIZE_CB callback.
// Called to query the size of drag data.
type DragDataSizeFunc func(ih Ihandle, dragType string) int

//export goIupDragDataSizeCB
func goIupDragDataSizeCB(ih unsafe.Pointer, dragType *C.char) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("DRAGDATASIZE_CB_" + uuid)
	if !ok {
		return C.IUP_DEFAULT
	}

	f := cgo.Handle(h.(cgo.Handle)).Value().(DragDataSizeFunc)

	return C.int(f((Ihandle)(ih), C.GoString(dragType)))
}

// setDragDataSizeFunc for DRAGDATASIZE_CB.
func setDragDataSizeFunc(ih Ihandle, f DragDataSizeFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("DRAGDATASIZE_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetDragDataSizeFunc(ih.ptr())
}

//--------------------

// DragDataFunc for DRAGDATA_CB callback.
// Called to get drag data.
type DragDataFunc func(ih Ihandle, dragType string, data unsafe.Pointer, size int) int

//export goIupDragDataCB
func goIupDragDataCB(ih unsafe.Pointer, dragType *C.char, data unsafe.Pointer, size C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("DRAGDATA_CB_" + uuid)
	if !ok {
		return C.IUP_DEFAULT
	}

	f := cgo.Handle(h.(cgo.Handle)).Value().(DragDataFunc)

	return C.int(f((Ihandle)(ih), C.GoString(dragType), data, int(size)))
}

// setDragDataFunc for DRAGDATA_CB.
func setDragDataFunc(ih Ihandle, f DragDataFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("DRAGDATA_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetDragDataFunc(ih.ptr())
}

//--------------------

// DragEndFunc for DRAGEND_CB callback.
// Called when drag operation ends.
type DragEndFunc func(ih Ihandle, action int) int

//export goIupDragEndCB
func goIupDragEndCB(ih unsafe.Pointer, action C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("DRAGEND_CB_" + uuid)
	if !ok {
		return C.IUP_DEFAULT
	}

	f := cgo.Handle(h.(cgo.Handle)).Value().(DragEndFunc)

	return C.int(f((Ihandle)(ih), int(action)))
}

// setDragEndFunc for DRAGEND_CB.
func setDragEndFunc(ih Ihandle, f DragEndFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("DRAGEND_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetDragEndFunc(ih.ptr())
}

//--------------------

// DropDataFunc for DROPDATA_CB callback.
// Called when data is dropped.
type DropDataFunc func(ih Ihandle, dragType string, data unsafe.Pointer, size, x, y int) int

//export goIupDropDataCB
func goIupDropDataCB(ih unsafe.Pointer, dragType *C.char, data unsafe.Pointer, size, x, y C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("DROPDATA_CB_" + uuid)
	if !ok {
		return C.IUP_DEFAULT
	}

	f := cgo.Handle(h.(cgo.Handle)).Value().(DropDataFunc)

	return C.int(f((Ihandle)(ih), C.GoString(dragType), data, int(size), int(x), int(y)))
}

// setDropDataFunc for DROPDATA_CB.
func setDropDataFunc(ih Ihandle, f DropDataFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("DROPDATA_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetDropDataFunc(ih.ptr())
}

//--------------------

// DropMotionFunc for DROPMOTION_CB callback.
// Called during drag motion over drop target.
type DropMotionFunc func(ih Ihandle, x, y int, status string) int

//export goIupDropMotionCB
func goIupDropMotionCB(ih unsafe.Pointer, x, y C.int, status *C.char) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("DROPMOTION_CB_" + uuid)
	if !ok {
		return C.IUP_DEFAULT
	}

	f := cgo.Handle(h.(cgo.Handle)).Value().(DropMotionFunc)

	return C.int(f((Ihandle)(ih), int(x), int(y), C.GoString(status)))
}

// setDropMotionFunc for DROPMOTION_CB.
func setDropMotionFunc(ih Ihandle, f DropMotionFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("DROPMOTION_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetDropMotionFunc(ih.ptr())
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

// TableEditionFunc for EDITION_CB callback.
// Called when a cell is being edited in a table/matrix control.
// update: The new value entered by the user.
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

// setTableEditionFunc for EDITION_CB (Table version).
func setTableEditionFunc(ih Ihandle, f TableEditionFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("EDITION_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetTableEditionFunc(ih.ptr())
}

//--------------------

// TableValueChangedFunc for VALUECHANGED_CB callback.
// Called after a cell value has been changed in a table/matrix control.
// Triggered by edits, paste operations, or programmatic value changes.
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

// setTableValueChangedFunc for VALUECHANGED_CB (Table version).
func setTableValueChangedFunc(ih Ihandle, f TableValueChangedFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("TABLEVALUECHANGED_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetTableValueChangedFunc(ih.ptr())
}

//--------------------

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

// TableImageFunc for IMAGE_CB callback.
// Called to get the image name for a cell in virtual mode when SHOWIMAGE=YES.
// Returns the image name to display in the cell, or empty string for no image.
type TableImageFunc func(ih Ihandle, lin, col int) string

//export goIupTableImageCB
func goIupTableImageCB(ih unsafe.Pointer, lin, col C.int) *C.char {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("TABLE_IMAGE_CB_" + uuid)
	if !ok {
		return nil
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(TableImageFunc)
	result := f((Ihandle)(ih), int(lin), int(col))
	if result == "" {
		return nil
	}
	return C.CString(result)
}

func setTableImageFunc(ih Ihandle, f TableImageFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("TABLE_IMAGE_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetTableImageFunc(ih.ptr())
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

// setTableSortFunc for SORT_CB (Table version).
func setTableSortFunc(ih Ihandle, f TableSortFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("SORT_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetTableSortFunc(ih.ptr())
}

//--------------------

// FileFunc for FILE_CB callback.
// Called when a file is selected in FileDlg.
// status: "OK", "NEW", or "CANCEL"
type FileFunc func(ih Ihandle, filename, status string) int

//export goIupFileCB
func goIupFileCB(ih unsafe.Pointer, filename, status *C.char) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("FILE_CB_" + uuid)
	if !ok {
		return C.IUP_DEFAULT
	}

	f := cgo.Handle(h.(cgo.Handle)).Value().(FileFunc)

	return C.int(f((Ihandle)(ih), C.GoString(filename), C.GoString(status)))
}

// setFileFunc for FILE_CB.
func setFileFunc(ih Ihandle, f FileFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("FILE_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetFileFunc(ih.ptr())
}

//--------------------

// LayoutUpdateFunc for LAYOUTUPDATE_CB callback.
// Called when layout is updated in FlatScrollBox or ScrollBox.
type LayoutUpdateFunc func(ih Ihandle) int

//export goIupLayoutUpdateCB
func goIupLayoutUpdateCB(ih unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("LAYOUTUPDATE_CB_" + uuid)
	if !ok {
		return C.IUP_DEFAULT
	}

	f := cgo.Handle(h.(cgo.Handle)).Value().(LayoutUpdateFunc)

	return C.int(f((Ihandle)(ih)))
}

// setLayoutUpdateFunc for LAYOUTUPDATE_CB.
func setLayoutUpdateFunc(ih Ihandle, f LayoutUpdateFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("LAYOUTUPDATE_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetLayoutUpdateFunc(ih.ptr())
}

//--------------------

// HighlightFunc for HIGHLIGHT_CB callback.
// Called when a menu item is highlighted.
type HighlightFunc func(ih Ihandle) int

//export goIupHighlightCB
func goIupHighlightCB(ih unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("HIGHLIGHT_CB_" + uuid)
	if !ok {
		return C.IUP_DEFAULT
	}

	f := cgo.Handle(h.(cgo.Handle)).Value().(HighlightFunc)

	return C.int(f((Ihandle)(ih)))
}

// setHighlightFunc for HIGHLIGHT_CB.
func setHighlightFunc(ih Ihandle, f HighlightFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("HIGHLIGHT_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetHighlightFunc(ih.ptr())
}

//--------------------

// MenuCloseFunc for MENUCLOSE_CB callback.
// Called when a menu is closed.
type MenuCloseFunc func(ih Ihandle) int

//export goIupMenuCloseCB
func goIupMenuCloseCB(ih unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("MENUCLOSE_CB_" + uuid)
	if !ok {
		return C.IUP_DEFAULT
	}

	f := cgo.Handle(h.(cgo.Handle)).Value().(MenuCloseFunc)

	return C.int(f((Ihandle)(ih)))
}

// setMenuCloseFunc for MENUCLOSE_CB.
func setMenuCloseFunc(ih Ihandle, f MenuCloseFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MENUCLOSE_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetMenuCloseFunc(ih.ptr())
}

//--------------------

// ColorUpdateFunc for COLORUPDATE_CB callback.
// Called when color is updated in ColorDlg.
type ColorUpdateFunc func(ih Ihandle) int

//export goIupColorUpdateCB
func goIupColorUpdateCB(ih unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("COLORUPDATE_CB_" + uuid)
	if !ok {
		return C.IUP_DEFAULT
	}

	f := cgo.Handle(h.(cgo.Handle)).Value().(ColorUpdateFunc)

	return C.int(f((Ihandle)(ih)))
}

// setColorUpdateFunc for COLORUPDATE_CB.
func setColorUpdateFunc(ih Ihandle, f ColorUpdateFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("COLORUPDATE_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetColorUpdateFunc(ih.ptr())
}

//--------------------

//##############################################################################
//##############################################################################
//##
//## MATRIX, CELLS, and MATRIXLIST CONTROL CALLBACKS
//##
//##############################################################################
//##############################################################################

//--------------------
// MatrixList Callbacks
//--------------------

// ListReleaseFunc for LISTRELEASE_CB callback in MatrixList.
// Called when mouse button is released over a list item.
type ListReleaseFunc func(ih Ihandle, lin, col int, status string) int

//export goIupListReleaseCB
func goIupListReleaseCB(ih unsafe.Pointer, lin, col C.int, status *C.char) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("LISTRELEASE_CB_" + uuid)
	if !ok {
		return C.IUP_DEFAULT
	}

	f := cgo.Handle(h.(cgo.Handle)).Value().(ListReleaseFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col), C.GoString(status)))
}

// setListReleaseFunc for LISTRELEASE_CB.
func setListReleaseFunc(ih Ihandle, f ListReleaseFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("LISTRELEASE_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetListReleaseFunc(ih.ptr())
}

//--------------------

// ListInsertFunc for LISTINSERT_CB callback in MatrixList.
// Called when an item is inserted.
type ListInsertFunc func(ih Ihandle, pos int) int

//export goIupListInsertCB
func goIupListInsertCB(ih unsafe.Pointer, pos C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("LISTINSERT_CB_" + uuid)
	if !ok {
		return C.IUP_DEFAULT
	}

	f := cgo.Handle(h.(cgo.Handle)).Value().(ListInsertFunc)

	return C.int(f((Ihandle)(ih), int(pos)))
}

// setListInsertFunc for LISTINSERT_CB.
func setListInsertFunc(ih Ihandle, f ListInsertFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("LISTINSERT_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetListInsertFunc(ih.ptr())
}

//--------------------

// ListRemoveFunc for LISTREMOVE_CB callback in MatrixList.
// Called when an item is removed.
type ListRemoveFunc func(ih Ihandle, pos int) int

//export goIupListRemoveCB
func goIupListRemoveCB(ih unsafe.Pointer, pos C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("LISTREMOVE_CB_" + uuid)
	if !ok {
		return C.IUP_DEFAULT
	}

	f := cgo.Handle(h.(cgo.Handle)).Value().(ListRemoveFunc)

	return C.int(f((Ihandle)(ih), int(pos)))
}

// setListRemoveFunc for LISTREMOVE_CB.
func setListRemoveFunc(ih Ihandle, f ListRemoveFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("LISTREMOVE_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetListRemoveFunc(ih.ptr())
}

//--------------------

// ListEditionFunc for LISTEDITION_CB callback in MatrixList.
// Called during list item editing.
type ListEditionFunc func(ih Ihandle, lin, col, mode, update int) int

//export goIupListEditionCB
func goIupListEditionCB(ih unsafe.Pointer, lin, col, mode, update C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("LISTEDITION_CB_" + uuid)
	if !ok {
		return C.IUP_DEFAULT
	}

	f := cgo.Handle(h.(cgo.Handle)).Value().(ListEditionFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col), int(mode), int(update)))
}

// setListEditionFunc for LISTEDITION_CB.
func setListEditionFunc(ih Ihandle, f ListEditionFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("LISTEDITION_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetListEditionFunc(ih.ptr())
}

//--------------------

// ListDrawFunc for LISTDRAW_CB callback in MatrixList.
// Called to draw a list item (custom drawing).
type ListDrawFunc func(ih Ihandle, lin, x1, x2, y1, y2, canvas int) int

//export goIupListDrawCB
func goIupListDrawCB(ih unsafe.Pointer, lin, x1, x2, y1, y2, canvas C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("LISTDRAW_CB_" + uuid)
	if !ok {
		return C.IUP_DEFAULT
	}

	f := cgo.Handle(h.(cgo.Handle)).Value().(ListDrawFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(x1), int(x2), int(y1), int(y2), int(canvas)))
}

// setListDrawFunc for LISTDRAW_CB.
func setListDrawFunc(ih Ihandle, f ListDrawFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("LISTDRAW_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetListDrawFunc(ih.ptr())
}

//--------------------
// MatrixEx Callbacks
//--------------------

// BusyFunc for BUSY_CB callback in MatrixEx.
// Called during busy/long operations.
type BusyFunc func(ih Ihandle, lin, col int, status string) int

//export goIupBusyCB
func goIupBusyCB(ih unsafe.Pointer, lin, col C.int, status *C.char) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("BUSY_CB_" + uuid)
	if !ok {
		return C.IUP_DEFAULT
	}

	f := cgo.Handle(h.(cgo.Handle)).Value().(BusyFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col), C.GoString(status)))
}

// setBusyFunc for BUSY_CB.
func setBusyFunc(ih Ihandle, f BusyFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("BUSY_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetBusyFunc(ih.ptr())
}

//--------------------

// MenuContextFunc for MENUCONTEXT_CB callback in MatrixEx.
// Called to show context menu for a cell.
type MenuContextFunc func(ih, menu Ihandle, lin, col int) int

//export goIupMenuContextCB
func goIupMenuContextCB(ih, menu unsafe.Pointer, lin, col C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("MENUCONTEXT_CB_" + uuid)
	if !ok {
		return C.IUP_DEFAULT
	}

	f := cgo.Handle(h.(cgo.Handle)).Value().(MenuContextFunc)

	return C.int(f((Ihandle)(ih), (Ihandle)(menu), int(lin), int(col)))
}

// setMenuContextFunc for MENUCONTEXT_CB.
func setMenuContextFunc(ih Ihandle, f MenuContextFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MENUCONTEXT_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetMenuContextFunc(ih.ptr())
}

//--------------------

// MenuContextCloseFunc for MENUCONTEXTCLOSE_CB callback in MatrixEx.
// Called when context menu is closed.
type MenuContextCloseFunc func(ih, menu Ihandle, lin, col int) int

//export goIupMenuContextCloseCB
func goIupMenuContextCloseCB(ih, menu unsafe.Pointer, lin, col C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("MENUCONTEXTCLOSE_CB_" + uuid)
	if !ok {
		return C.IUP_DEFAULT
	}

	f := cgo.Handle(h.(cgo.Handle)).Value().(MenuContextCloseFunc)

	return C.int(f((Ihandle)(ih), (Ihandle)(menu), int(lin), int(col)))
}

// setMenuContextCloseFunc for MENUCONTEXTCLOSE_CB.
func setMenuContextCloseFunc(ih Ihandle, f MenuContextCloseFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MENUCONTEXTCLOSE_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetMenuContextCloseFunc(ih.ptr())
}

//--------------------

// PasteSizeFunc for PASTESIZE_CB callback in MatrixEx.
// Called to determine paste area size.
type PasteSizeFunc func(ih Ihandle, numlin, numcol int) int

//export goIupPasteSizeCB
func goIupPasteSizeCB(ih unsafe.Pointer, numlin, numcol C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("PASTESIZE_CB_" + uuid)
	if !ok {
		return C.IUP_DEFAULT
	}

	f := cgo.Handle(h.(cgo.Handle)).Value().(PasteSizeFunc)

	return C.int(f((Ihandle)(ih), int(numlin), int(numcol)))
}

// setPasteSizeFunc for PASTESIZE_CB.
func setPasteSizeFunc(ih Ihandle, f PasteSizeFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("PASTESIZE_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetPasteSizeFunc(ih.ptr())
}

//--------------------
// Matrix Numeric Callbacks
//--------------------

// NumericGetValueFunc for NUMERICGETVALUE_CB callback in Matrix.
// Called to get numeric value from a cell (returns double).
type NumericGetValueFunc func(ih Ihandle, lin, col int) float64

//export goIupNumericGetValueCB
func goIupNumericGetValueCB(ih unsafe.Pointer, lin, col C.int) C.double {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("NUMERICGETVALUE_CB_" + uuid)
	if !ok {
		return 0.0
	}

	f := cgo.Handle(h.(cgo.Handle)).Value().(NumericGetValueFunc)

	return C.double(f((Ihandle)(ih), int(lin), int(col)))
}

// setNumericGetValueFunc for NUMERICGETVALUE_CB.
func setNumericGetValueFunc(ih Ihandle, f NumericGetValueFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("NUMERICGETVALUE_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetNumericGetValueFunc(ih.ptr())
}

//--------------------

// NumericSetValueFunc for NUMERICSETVALUE_CB callback in Matrix.
// Called to set numeric value in a cell.
type NumericSetValueFunc func(ih Ihandle, lin, col int, value float64) int

//export goIupNumericSetValueCB
func goIupNumericSetValueCB(ih unsafe.Pointer, lin, col C.int, value C.double) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("NUMERICSETVALUE_CB_" + uuid)
	if !ok {
		return C.IUP_DEFAULT
	}

	f := cgo.Handle(h.(cgo.Handle)).Value().(NumericSetValueFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col), float64(value)))
}

// setNumericSetValueFunc for NUMERICSETVALUE_CB.
func setNumericSetValueFunc(ih Ihandle, f NumericSetValueFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("NUMERICSETVALUE_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetNumericSetValueFunc(ih.ptr())
}

//--------------------

// SortColumnCompareFunc for SORTCOLUMNCOMPARE_CB callback in Matrix.
// Called to compare two rows during column sort.
type SortColumnCompareFunc func(ih Ihandle, lin1, lin2, col int) int

//export goIupSortColumnCompareCB
func goIupSortColumnCompareCB(ih unsafe.Pointer, lin1, lin2, col C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("SORTCOLUMNCOMPARE_CB_" + uuid)
	if !ok {
		return C.IUP_DEFAULT
	}

	f := cgo.Handle(h.(cgo.Handle)).Value().(SortColumnCompareFunc)

	return C.int(f((Ihandle)(ih), int(lin1), int(lin2), int(col)))
}

// setSortColumnCompareFunc for SORTCOLUMNCOMPARE_CB.
func setSortColumnCompareFunc(ih Ihandle, f SortColumnCompareFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("SORTCOLUMNCOMPARE_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetSortColumnCompareFunc(ih.ptr())
}

//--------------------
// Cells Control Callbacks
//--------------------

// CellsDrawFunc for DRAW_CB callback in Cells control.
// Called to draw a cell.
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

// setCellsDrawFunc for DRAW_CB in Cells.
func setCellsDrawFunc(ih Ihandle, f CellsDrawFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("CELLS_DRAW_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetCellsDrawFunc(ih.ptr())
}

//--------------------

// MouseClickFunc for MOUSECLICK_CB callback.
// Called when mouse is clicked in a cell.
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

// setMouseClickFunc for MOUSECLICK_CB.
func setMouseClickFunc(ih Ihandle, f MouseClickFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MOUSECLICK_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetMouseClickFunc(ih.ptr())
}

//--------------------

// MouseMotionFunc for MOUSEMOTION_CB callback.
// Called when mouse moves over a cell.
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

// setMouseMotionFunc for MOUSEMOTION_CB in Cells.
func setMouseMotionFunc(ih Ihandle, f MouseMotionFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MOUSEMOTION_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetMouseMotionFunc(ih.ptr())
}

//--------------------

// ScrollingFunc for SCROLLING_CB callback.
// Called when cells are scrolled.
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

// setScrollingFunc for SCROLLING_CB.
func setScrollingFunc(ih Ihandle, f ScrollingFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("SCROLLING_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetScrollingFunc(ih.ptr())
}

//--------------------

// NColsFunc for NCOLS_CB callback.
// Called to get the number of columns.
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

// setNColsFunc for NCOLS_CB.
func setNColsFunc(ih Ihandle, f NColsFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("NCOLS_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetNColsFunc(ih.ptr())
}

//--------------------

// NLinesFunc for NLINES_CB callback.
// Called to get the number of lines.
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

// setNLinesFunc for NLINES_CB.
func setNLinesFunc(ih Ihandle, f NLinesFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("NLINES_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetNLinesFunc(ih.ptr())
}

//--------------------

// HSpanFunc for HSPAN_CB callback.
// Called to get horizontal span of a cell.
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

// setHSpanFunc for HSPAN_CB.
func setHSpanFunc(ih Ihandle, f HSpanFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("HSPAN_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetHSpanFunc(ih.ptr())
}

//--------------------

// VSpanFunc for VSPAN_CB callback.
// Called to get vertical span of a cell.
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

// setVSpanFunc for VSPAN_CB.
func setVSpanFunc(ih Ihandle, f VSpanFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("VSPAN_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetVSpanFunc(ih.ptr())
}

//--------------------

// HeightFunc for HEIGHT_CB callback.
// Called to get the height of a line.
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

// setHeightFunc for HEIGHT_CB.
func setHeightFunc(ih Ihandle, f HeightFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("HEIGHT_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetHeightFunc(ih.ptr())
}

//--------------------

// WidthFunc for WIDTH_CB callback.
// Called to get the width of a column.
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

// setWidthFunc for WIDTH_CB.
func setWidthFunc(ih Ihandle, f WidthFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("WIDTH_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetWidthFunc(ih.ptr())
}

//--------------------

// MatrixDrawFunc for DRAW_CB callback in Matrix control.
// Called when a cell needs to be redrawn.

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

// setBgColorFunc for BGCOLOR_CB.
func setBgColorFunc(ih Ihandle, f BgColorFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("BGCOLOR_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetBgColorFunc(ih.ptr())
}

//--------------------

// ClickFunc for CLICK_CB callback.
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

// setClickFunc for CLICK_CB in Matrix.
func setClickFunc(ih Ihandle, f ClickFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("CLICK_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetClickFunc(ih.ptr())
}

//--------------------

// ColResizeFunc for COLRESIZE_CB callback.
// Called when a column is resized.
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

// setColResizeFunc for COLRESIZE_CB.
func setColResizeFunc(ih Ihandle, f ColResizeFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("COLRESIZE_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetColResizeFunc(ih.ptr())
}

//--------------------

// DropCheckFunc for DROPCHECK_CB callback.
// Called to check if dropdown should be shown.
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

// setDropCheckFunc for DROPCHECK_CB.
func setDropCheckFunc(ih Ihandle, f DropCheckFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("DROPCHECK_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetDropCheckFunc(ih.ptr())
}

//--------------------

// DropSelectFunc for DROPSELECT_CB callback.
// Called when dropdown item is selected.
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

// EditBeginFunc for EDITBEGIN_CB callback.
// Called when cell editing begins.
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

// setEditBeginFunc for EDITBEGIN_CB.
func setEditBeginFunc(ih Ihandle, f EditBeginFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("EDITBEGIN_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetEditBeginFunc(ih.ptr())
}

//--------------------

// EditClickFunc for EDITCLICK_CB callback.
// Called when edit field is clicked.
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

// EditEndFunc for EDITEND_CB callback.
// Called when cell editing ends.
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

// setEditEndFunc for EDITEND_CB.
func setEditEndFunc(ih Ihandle, f EditEndFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("EDITEND_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetEditEndFunc(ih.ptr())
}

//--------------------

// EditMouseMoveFunc for EDITMOUSEMOVE_CB callback.
// Called when mouse moves in edit field.
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

// EditReleaseFunc for EDITRELEASE_CB callback.
// Called when mouse button is released in edit field.
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

// EditionFunc for EDITION_CB callback.
// Called during cell editing.
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

// setEditionFunc for EDITION_CB.
func setEditionFunc(ih Ihandle, f EditionFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("EDITION_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetEditionFunc(ih.ptr())
}

//--------------------

// EnterItemFunc for ENTERITEM_CB callback.
// Called when a cell is entered.
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

// setEnterItemFunc for ENTERITEM_CB.
func setEnterItemFunc(ih Ihandle, f EnterItemFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("ENTERITEM_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetEnterItemFunc(ih.ptr())
}

//--------------------

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

// setFgColorFunc for FGCOLOR_CB.
func setFgColorFunc(ih Ihandle, f FgColorFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("FGCOLOR_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetFgColorFunc(ih.ptr())
}

//--------------------

// LeaveItemFunc for LEAVEITEM_CB callback.
// Called when a cell is left.
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

// setLeaveItemFunc for LEAVEITEM_CB.
func setLeaveItemFunc(ih Ihandle, f LeaveItemFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("LEAVEITEM_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetLeaveItemFunc(ih.ptr())
}

//--------------------

// MarkEditFunc for MARKEDIT_CB callback.
// Called when cell mark is edited.
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

// setMarkEditFunc for MARKEDIT_CB.
func setMarkEditFunc(ih Ihandle, f MarkEditFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MARKEDIT_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetMarkEditFunc(ih.ptr())
}

//--------------------

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

// setMarkFunc for MARK_CB.
func setMarkFunc(ih Ihandle, f MarkFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MARK_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetMarkFunc(ih.ptr())
}

//--------------------

// MatrixActionFunc for ACTION_CB callback in Matrix control.
// Called for keyboard actions in a cell.
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

// MatrixDrawFunc for DRAW_CB callback in Matrix control.
// Called to draw a cell.
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

// setMatrixDrawFunc for DRAW_CB in Matrix.
func setMatrixDrawFunc(ih Ihandle, f MatrixDrawFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MATRIX_DRAW_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetMatrixDrawFunc(ih.ptr())
}

//--------------------

// MatrixDropFunc for DROP_CB callback in Matrix control.
// Called when a dropdown is shown.
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

// MatrixFontFunc for FONT_CB callback.
// Called to get the font for a cell.
// Returns font string.
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

// MatrixMouseMoveFunc for MOUSEMOVE_CB callback in Matrix.
// Called when mouse moves over a cell.
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

// setMatrixMouseMoveFunc for MOUSEMOVE_CB in Matrix.
func setMatrixMouseMoveFunc(ih Ihandle, f MatrixMouseMoveFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MATRIX_MOUSEMOVE_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetMatrixMouseMoveFunc(ih.ptr())
}

//--------------------

// MatrixToggleValueFunc for TOGGLEVALUE_CB callback in Matrix control.
// Called when toggle value changes.
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

// MatrixTypeFunc for TYPE_CB callback.
// Called to get the type of a cell.
// Returns type string.
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

// MatrixValueFunc for VALUE_CB callback.
// Called to get the value of a cell.
// Returns value string.
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

// setTableValueFunc for VALUE_CB (Table version).
func setTableValueFunc(ih Ihandle, f TableValueFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("VALUE_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetTableValueFunc(ih.ptr())
}

//--------------------

// setMatrixValueFunc for VALUE_CB.
func setMatrixValueFunc(ih Ihandle, f MatrixValueFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("VALUE_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetMatrixValueFunc(ih.ptr())
}

//--------------------

// MenuDropFunc for MENUDROP_CB callback.
// Called to show dropdown menu for a cell.
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

// ReleaseFunc for RELEASE_CB callback.
// Called when mouse button is released over a cell.
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

// setReleaseFunc for RELEASE_CB in Matrix.
func setReleaseFunc(ih Ihandle, f ReleaseFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("RELEASE_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetReleaseFunc(ih.ptr())
}

//--------------------

// ResizeMatrixFunc for RESIZEMATRIX_CB callback.
// Called when the matrix is resized.
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

// setResizeMatrixFunc for RESIZEMATRIX_CB.
func setResizeMatrixFunc(ih Ihandle, f ResizeMatrixFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("RESIZEMATRIX_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetResizeMatrixFunc(ih.ptr())
}

//--------------------

// ScrollTopFunc for SCROLLTOP_CB callback.
// Called when scrolling to top.
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

// setScrollTopFunc for SCROLLTOP_CB.
func setScrollTopFunc(ih Ihandle, f ScrollTopFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("SCROLLTOP_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetScrollTopFunc(ih.ptr())
}

//--------------------

// TranslateValueFunc for TRANSLATEVALUE_CB callback.
// Called to translate cell value for display.
// Returns translated string.
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

// ValueEditFunc for VALUE_EDIT_CB callback.
// Called to edit cell value.
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

// setValueEditFunc for VALUE_EDIT_CB.
func setValueEditFunc(ih Ihandle, f ValueEditFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("VALUE_EDIT_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetValueEditFunc(ih.ptr())
}

//--------------------

// ColResizeFunc for COLRESIZE_CB callback.
// Called after a column is resized.

//--------------------
// MatrixList Control Callbacks (srcctl/iup_matrixlist.c)
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

//--------------------

//export goIupRecentCB
func goIupRecentCB(ih unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("RECENT_CB_" + uuid)
	if !ok {
		return C.int(DEFAULT)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(ActionFunc)
	return C.int(f((Ihandle)(ih)))
}

// setRecentFunc stores the callback for recent file selection.
// The callback is stored using the config handle's UUID.
func setRecentFunc(config, menuOrList Ihandle, f ActionFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("RECENT_CB_"+config.GetAttribute("UUID"), ch)
}

//--------------------

// NotifyFunc for NOTIFY_CB callback.
// Called when the notification is activated (clicked or action button pressed).
// action_id: 0=body clicked, 1-4=action buttons
type NotifyFunc func(ih Ihandle, actionId int) int

//export goIupNotifyCB
func goIupNotifyCB(ih unsafe.Pointer, actionId C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("NOTIFY_CB_" + uuid)
	if !ok {
		return C.int(DEFAULT)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(NotifyFunc)
	return C.int(f((Ihandle)(ih), int(actionId)))
}

// setNotifyFunc for NOTIFY_CB.
func setNotifyFunc(ih Ihandle, f NotifyFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("NOTIFY_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetNotifyFunc(ih.ptr())
}

//--------------------

// NotifyCloseFunc for CLOSE_CB callback on notification.
// Called when the notification is dismissed.
// reason: 1=expired, 2=user dismissed, 3=closed programmatically, 4=unknown
type NotifyCloseFunc func(ih Ihandle, reason int) int

//export goIupNotifyCloseCB
func goIupNotifyCloseCB(ih unsafe.Pointer, reason C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("NOTIFY_CLOSE_CB_" + uuid)
	if !ok {
		return C.int(DEFAULT)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(NotifyCloseFunc)
	return C.int(f((Ihandle)(ih), int(reason)))
}

// setNotifyCloseFunc for CLOSE_CB on notifications.
func setNotifyCloseFunc(ih Ihandle, f NotifyCloseFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("NOTIFY_CLOSE_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetNotifyCloseFunc(ih.ptr())
}
