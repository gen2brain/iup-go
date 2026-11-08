//go:build !js

package iup

import (
	"slices"
	"strconv"
	"strings"
	"sync"
	"sync/atomic"
	"unsafe"
)

/*
#include "bind_callbacks.h"
*/
import "C"

var (
	globalIdleHandle  goHandle
	globalEntryHandle goHandle
	globalExitHandle  goHandle

	globalKeyPressHandle   goHandle
	globalButtonHandle     goHandle
	globalMotionHandle     goHandle
	globalWheelHandle      goHandle
	globalEnterModalHandle goHandle
	globalLeaveModalHandle goHandle
)

type goHandle uintptr

var (
	goHandles   sync.Map
	goHandleIdx atomic.Uintptr
)

func newGoHandle(v any) goHandle {
	h := goHandleIdx.Add(1)
	if h == 0 {
		panic("iup: ran out of handle space")
	}
	goHandles.Store(h, v)
	return goHandle(h)
}

func (h goHandle) Value() any {
	v, ok := goHandles.Load(uintptr(h))
	if !ok {
		panic("iup: misuse of an invalid handle")
	}
	return v
}

func (h goHandle) Delete() {
	if _, ok := goHandles.LoadAndDelete(uintptr(h)); !ok {
		panic("iup: misuse of an invalid handle")
	}
}

// Tracks which _IUPGO_* keys are set on an Ihandle. _IUP* names are skipped
// by IupGetAllAttributes, so we can't iterate them at destroy time without this list.
const iupgoRegistryAttr = "_IUPGO_REGISTRY"

func handleToStr(ch goHandle) string {
	return strconv.FormatUint(uint64(ch), 36)
}

func strToHandle(s string) goHandle {
	if s == "" {
		return 0
	}
	n, err := strconv.ParseUint(s, 36, 64)
	if err != nil {
		return 0
	}
	return goHandle(uintptr(n))
}

func storeCallback(ih Ihandle, key string, f any) {
	if old := strToHandle(ih.GetAttribute(key)); old != 0 {
		old.Delete()
	} else {
		list := ih.GetAttribute(iupgoRegistryAttr)
		switch {
		case list == "":
			ih.SetAttribute(iupgoRegistryAttr, key)
			C.goIupSetLDestroyFunc(ih.ptr())
		case !slices.Contains(strings.Split(list, ","), key):
			ih.SetAttribute(iupgoRegistryAttr, list+","+key)
		}
	}
	ih.SetAttribute(key, handleToStr(newGoHandle(f)))
}

func loadCallback(ih Ihandle, key string) goHandle {
	enterIupThread()
	return strToHandle(ih.GetAttribute(key))
}

func clearCallback(ih Ihandle, key string) {
	if old := strToHandle(ih.GetAttribute(key)); old != 0 {
		old.Delete()
		ih.SetAttribute(key, "")
	}
}

func setGlobalHandle(slot *goHandle, f any) {
	if *slot != 0 {
		slot.Delete()
	}
	*slot = newGoHandle(f)
}

func clearGlobalHandle(slot *goHandle) {
	if *slot != 0 {
		slot.Delete()
		*slot = 0
	}
}

//export goIupLDestroyCB
func goIupLDestroyCB(ih unsafe.Pointer) C.int {
	h := (Ihandle)(ih)
	releasePostPayloads(h)
	list := h.GetAttribute(iupgoRegistryAttr)
	if list == "" {
		return 0
	}
	for _, key := range strings.Split(list, ",") {
		if ch := strToHandle(h.GetAttribute(key)); ch != 0 {
			ch.Delete()
			h.SetAttribute(key, "")
		}
	}
	h.SetAttribute(iupgoRegistryAttr, "")
	return 0
}

//--------------------

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

//export goIupGlobalKeyPressCB
func goIupGlobalKeyPressCB(c, press C.int) C.int {
	f := globalKeyPressHandle.Value().(GlobalKeyPressFunc)
	return C.int(f(int(c), int(press)))
}

func setGlobalKeyPressFunc(f GlobalKeyPressFunc) {
	setGlobalHandle(&globalKeyPressHandle, f)
	C.goIupSetGlobalKeyPressFunc()
}

//--------------------

//export goIupGlobalButtonCB
func goIupGlobalButtonCB(button, pressed, x, y C.int, status *C.char) C.int {
	f := globalButtonHandle.Value().(GlobalButtonFunc)
	return C.int(f(int(button), int(pressed), int(x), int(y), C.GoString(status)))
}

func setGlobalButtonFunc(f GlobalButtonFunc) {
	setGlobalHandle(&globalButtonHandle, f)
	C.goIupSetGlobalButtonFunc()
}

//--------------------

//export goIupGlobalMotionCB
func goIupGlobalMotionCB(x, y C.int, status *C.char) C.int {
	f := globalMotionHandle.Value().(GlobalMotionFunc)
	return C.int(f(int(x), int(y), C.GoString(status)))
}

func setGlobalMotionFunc(f GlobalMotionFunc) {
	setGlobalHandle(&globalMotionHandle, f)
	C.goIupSetGlobalMotionFunc()
}

//--------------------

//export goIupGlobalWheelCB
func goIupGlobalWheelCB(delta C.float, x, y C.int, status *C.char) C.int {
	f := globalWheelHandle.Value().(GlobalWheelFunc)
	return C.int(f(float64(delta), int(x), int(y), C.GoString(status)))
}

func setGlobalWheelFunc(f GlobalWheelFunc) {
	setGlobalHandle(&globalWheelHandle, f)
	C.goIupSetGlobalWheelFunc()
}

//--------------------

//export goIupGlobalEnterModalCB
func goIupGlobalEnterModalCB(level C.int) C.int {
	f := globalEnterModalHandle.Value().(GlobalEnterModalFunc)
	return C.int(f(int(level)))
}

func setGlobalEnterModalFunc(f GlobalEnterModalFunc) {
	setGlobalHandle(&globalEnterModalHandle, f)
	C.goIupSetGlobalEnterModalFunc()
}

//--------------------

//export goIupGlobalLeaveModalCB
func goIupGlobalLeaveModalCB(level C.int) C.int {
	f := globalLeaveModalHandle.Value().(GlobalLeaveModalFunc)
	return C.int(f(int(level)))
}

func setGlobalLeaveModalFunc(f GlobalLeaveModalFunc) {
	setGlobalHandle(&globalLeaveModalHandle, f)
	C.goIupSetGlobalLeaveModalFunc()
}

//--------------------

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

//export goIupTipsCB
func goIupTipsCB(ih unsafe.Pointer, x, y C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_TIPS_CB").Value().(TipsFunc)

	return C.int(f((Ihandle)(ih), int(x), int(y)))
}

// setTipsFunc for TIPS_CB.
func setTipsFunc(ih Ihandle, f TipsFunc) {
	storeCallback(ih, "_IUPGO_TIPS_CB", f)

	C.goIupSetTipsFunc(ih.ptr())
}

//--------------------

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

//export goIupGestureCB
func goIupGestureCB(ih unsafe.Pointer, gesture, state, x, y C.int, v1, v2 C.double) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_GESTURE_CB").Value().(GestureFunc)

	return C.int(f((Ihandle)(ih), int(gesture), int(state), int(x), int(y), float64(v1), float64(v2)))
}

// setGestureFunc for GESTURE_CB.
func setGestureFunc(ih Ihandle, f GestureFunc) {
	storeCallback(ih, "_IUPGO_GESTURE_CB", f)

	C.goIupSetGestureFunc(ih.ptr())
}

//--------------------

//export goIupTerminalInputCB
func goIupTerminalInputCB(ih, b unsafe.Pointer, length C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_TERMINAL_INPUT").Value().(TerminalInputFunc)
	return C.int(f((Ihandle)(ih), C.GoBytes(b, length)))
}

func setTerminalInputFunc(ih Ihandle, f TerminalInputFunc) {
	storeCallback(ih, "_IUPGO_TERMINAL_INPUT", f)
	C.goIupSetTerminalInputFunc(ih.ptr())
}

//--------------------

//export goIupTerminalTitleChangedCB
func goIupTerminalTitleChangedCB(ih, title unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_TITLECHANGED_CB").Value().(TerminalTitleChangedFunc)
	return C.int(f((Ihandle)(ih), C.GoString((*C.char)(title))))
}

func setTerminalTitleChangedFunc(ih Ihandle, f TerminalTitleChangedFunc) {
	storeCallback(ih, "_IUPGO_TITLECHANGED_CB", f)
	C.goIupSetTerminalTitleChangedFunc(ih.ptr())
}

//--------------------

//export goIupTerminalBellCB
func goIupTerminalBellCB(ih unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_TERMINAL_BELL").Value().(TerminalBellFunc)
	return C.int(f((Ihandle)(ih)))
}

func setTerminalBellFunc(ih Ihandle, f TerminalBellFunc) {
	storeCallback(ih, "_IUPGO_TERMINAL_BELL", f)
	C.goIupSetTerminalBellFunc(ih.ptr())
}

//--------------------

//export goIupTerminalSizeCB
func goIupTerminalSizeCB(ih unsafe.Pointer, cols, lines C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_TERMINAL_SIZE").Value().(TerminalSizeFunc)
	return C.int(f((Ihandle)(ih), int(cols), int(lines)))
}

func setTerminalSizeFunc(ih Ihandle, f TerminalSizeFunc) {
	storeCallback(ih, "_IUPGO_TERMINAL_SIZE", f)
	C.goIupSetTerminalSizeFunc(ih.ptr())
}

//--------------------

//export goIupTerminalExecExitCB
func goIupTerminalExecExitCB(ih unsafe.Pointer, status C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_EXECEXIT_CB").Value().(TerminalExecExitFunc)
	return C.int(f((Ihandle)(ih), int(status)))
}

func setTerminalExecExitFunc(ih Ihandle, f TerminalExecExitFunc) {
	storeCallback(ih, "_IUPGO_EXECEXIT_CB", f)
	C.goIupSetTerminalExecExitFunc(ih.ptr())
}

//--------------------

//export goIupTextInputCB
func goIupTextInputCB(ih, text unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_TEXTINPUT").Value().(TextInputFunc)
	return C.int(f((Ihandle)(ih), C.GoString((*C.char)(text))))
}

func setTextInputFunc(ih Ihandle, f TextInputFunc) {
	storeCallback(ih, "_IUPGO_TEXTINPUT", f)
	C.goIupSetTextInputFunc(ih.ptr())
}

//--------------------

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

//export goIupCopyDataCB
func goIupCopyDataCB(ih, cmdLine unsafe.Pointer, size C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_COPYDATA_CB").Value().(CopyDataFunc)

	return C.int(f((Ihandle)(ih), C.GoString((*C.char)(cmdLine)), int(size)))
}

// setCopyDataFunc for COPYDATA_CB.
func setCopyDataFunc(ih Ihandle, f CopyDataFunc) {
	storeCallback(ih, "_IUPGO_COPYDATA_CB", f)

	C.goIupSetCopyDataFunc(ih.ptr())
}

//--------------------

//export goIupPaginateCB
func goIupPaginateCB(ih unsafe.Pointer, width, height C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_PAGINATE_CB").Value().(PaginateFunc)

	return C.int(f((Ihandle)(ih), int(width), int(height)))
}

// setPaginateFunc for PAGINATE_CB.
func setPaginateFunc(ih Ihandle, f PaginateFunc) {
	storeCallback(ih, "_IUPGO_PAGINATE_CB", f)

	C.goIupSetPaginateFunc(ih.ptr())
}

//--------------------

//export goIupPrintPageCB
func goIupPrintPageCB(ih unsafe.Pointer, page, width, height C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_PRINTPAGE_CB").Value().(PrintPageFunc)

	return C.int(f((Ihandle)(ih), int(page), int(width), int(height)))
}

// setPrintPageFunc for PRINTPAGE_CB.
func setPrintPageFunc(ih Ihandle, f PrintPageFunc) {
	storeCallback(ih, "_IUPGO_PRINTPAGE_CB", f)

	C.goIupSetPrintPageFunc(ih.ptr())
}

//--------------------

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

// Payload handles of posted messages not yet delivered, per element with a Go POSTMESSAGE_CB.
var (
	postMu      sync.Mutex
	postPending = map[Ihandle]map[goHandle]struct{}{}
)

func newPostPayload(ih Ihandle, p any) goHandle {
	postMu.Lock()
	defer postMu.Unlock()
	pending, ok := postPending[ih]
	if !ok {
		return 0
	}
	h := newGoHandle(p)
	pending[h] = struct{}{}
	return h
}

func takePostPayload(ih Ihandle, h goHandle) (any, bool) {
	postMu.Lock()
	_, ok := postPending[ih][h]
	delete(postPending[ih], h)
	postMu.Unlock()
	if !ok {
		return nil, false
	}
	p := h.Value()
	h.Delete()
	return p, true
}

func releasePostPayloads(ih Ihandle) {
	postMu.Lock()
	defer postMu.Unlock()
	for h := range postPending[ih] {
		h.Delete()
	}
	delete(postPending, ih)
}

//export goIupPostMessageCB
func goIupPostMessageCB(ih unsafe.Pointer, s unsafe.Pointer, i C.int, d C.double, p unsafe.Pointer) C.int {
	_ = d
	f := loadCallback((Ihandle)(ih), "_IUPGO_POSTMESSAGE_CB").Value().(PostMessageFunc)

	var payload any
	if p != nil {
		var ok bool
		if payload, ok = takePostPayload((Ihandle)(ih), goHandle(uintptr(p))); !ok {
			return C.int(DEFAULT)
		}
	}

	return C.int(f((Ihandle)(ih), C.GoString((*C.char)(s)), int(i), payload))
}

// setPostMessageFunc for POSTMESSAGE_CB.
func setPostMessageFunc(ih Ihandle, f PostMessageFunc) {
	storeCallback(ih, "_IUPGO_POSTMESSAGE_CB", f)

	postMu.Lock()
	if postPending[ih] == nil {
		postPending[ih] = map[goHandle]struct{}{}
	}
	postMu.Unlock()

	C.goIupSetPostMessageFunc(ih.ptr())
}

//--------------------

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

//export goIupThreadCB
func goIupThreadCB(ih unsafe.Pointer) C.int {
	id := enterIupThread()
	defer leaveIupThread(id)

	f := loadCallback((Ihandle)(ih), "_IUPGO_THREAD_CB").Value().(ThreadFunc)

	return C.int(f((Ihandle)(ih)))
}

// setThreadFunc for THREAD_CB callback.
func setThreadFunc(ih Ihandle, f ThreadFunc) {
	storeCallback(ih, "_IUPGO_THREAD_CB", f)

	C.goIupSetThreadFunc(ih.ptr())
}

//--------------------

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

//export goIupOpenCloseCB
func goIupOpenCloseCB(ih unsafe.Pointer, state C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_OPENCLOSE_CB").Value().(OpenCloseFunc)

	return C.int(f((Ihandle)(ih), int(state)))
}

// setOpenCloseFunc for OPENCLOSE_CB.
func setOpenCloseFunc(ih Ihandle, f OpenCloseFunc) {
	storeCallback(ih, "_IUPGO_OPENCLOSE_CB", f)

	C.goIupSetOpenCloseFunc(ih.ptr())
}

//--------------------

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

//export goIupCellCB
func goIupCellCB(ih unsafe.Pointer, cell C.int) *C.char {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_CELL_CB")
	if ch == 0 {
		return nil
	}
	result := ch.Value().(CellFunc)((Ihandle)(ih), int(cell))
	if result == "" {
		return nil
	}
	return C.CString(result)
}

// setCellFunc for CELL_CB.
func setCellFunc(ih Ihandle, f CellFunc) {
	storeCallback(ih, "_IUPGO_CELL_CB", f)

	C.goIupSetCellFunc(ih.ptr())
}

//--------------------

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

//export goIupTextLinkCB
func goIupTextLinkCB(ih, url unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_LINK_CB").Value().(TextLinkFunc)

	goUrl := C.GoString((*C.char)(url))
	return C.int(f((Ihandle)(ih), goUrl))
}

// setTextLinkFunc for LINK_CB.
func setTextLinkFunc(ih Ihandle, f TextLinkFunc) {
	storeCallback(ih, "_IUPGO_LINK_CB", f)

	C.goIupSetTextLinkFunc(ih.ptr())
}

//--------------------

//export goIupMaskFailCB
func goIupMaskFailCB(ih, newValue unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_MASKFAIL_CB").Value().(MaskFailFunc)

	goNewValue := C.GoString((*C.char)(newValue))
	return C.int(f((Ihandle)(ih), goNewValue))
}

// setMaskFailFunc for MASKFAIL_CB.
func setMaskFailFunc(ih Ihandle, f MaskFailFunc) {
	storeCallback(ih, "_IUPGO_MASKFAIL_CB", f)

	C.goIupSetMaskFailFunc(ih.ptr())
}

//--------------------

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

//export goIupDragBeginCB
func goIupDragBeginCB(ih unsafe.Pointer, x, y C.int) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_DRAGBEGIN_CB")
	if ch == 0 {
		return C.int(DEFAULT)
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

//export goIupDragDataSizeCB
func goIupDragDataSizeCB(ih unsafe.Pointer, dragType *C.char) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_DRAGDATASIZE_CB")
	if ch == 0 {
		return C.int(DEFAULT)
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

//export goIupDragDataCB
func goIupDragDataCB(ih unsafe.Pointer, dragType *C.char, data unsafe.Pointer, size C.int) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_DRAGDATA_CB")
	if ch == 0 {
		return C.int(DEFAULT)
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

//export goIupDragEndCB
func goIupDragEndCB(ih unsafe.Pointer, action C.int) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_DRAGEND_CB")
	if ch == 0 {
		return C.int(DEFAULT)
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

//export goIupDropDataCB
func goIupDropDataCB(ih unsafe.Pointer, dragType *C.char, data unsafe.Pointer, size, x, y C.int) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_DROPDATA_CB")
	if ch == 0 {
		return C.int(DEFAULT)
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

//export goIupDropMotionCB
func goIupDropMotionCB(ih unsafe.Pointer, x, y C.int, status *C.char) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_DROPMOTION_CB")
	if ch == 0 {
		return C.int(DEFAULT)
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

//export goIupMultiSelectionCB
func goIupMultiSelectionCB(ih unsafe.Pointer, ids *C.int, n C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_MULTISELECTION_CB").Value().(MultiSelectionFunc)

	cIds := unsafe.Slice(ids, n)
	goIds := make([]int, n)
	for i, id := range cIds {
		goIds[i] = int(id)
	}

	return C.int(f((Ihandle)(ih), goIds, int(n)))
}

// setMultiSelectionFunc for MULTISELECTION_CB.
func setMultiSelectionFunc(ih Ihandle, f MultiSelectionFunc) {
	storeCallback(ih, "_IUPGO_MULTISELECTION_CB", f)

	C.goIupSetMultiSelectionFunc(ih.ptr())
}

//--------------------

//export goIupMultiUnselectionCB
func goIupMultiUnselectionCB(ih unsafe.Pointer, ids *C.int, n C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_MULTIUNSELECTION_CB").Value().(MultiUnselectionFunc)

	cIds := unsafe.Slice(ids, n)
	goIds := make([]int, n)
	for i, id := range cIds {
		goIds[i] = int(id)
	}

	return C.int(f((Ihandle)(ih), goIds, int(n)))
}

// setMultiUnselectionFunc for MULTIUNSELECTION_CB.
func setMultiUnselectionFunc(ih Ihandle, f MultiUnselectionFunc) {
	storeCallback(ih, "_IUPGO_MULTIUNSELECTION_CB", f)

	C.goIupSetMultiUnselectionFunc(ih.ptr())
}

//--------------------

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

//export goIupAppearanceChangedCB
func goIupAppearanceChangedCB(ih unsafe.Pointer, darkMode C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_APPEARANCECHANGED_CB").Value().(AppearanceChangedFunc)

	return C.int(f((Ihandle)(ih), int(darkMode)))
}

// setAppearanceChangedFunc for APPEARANCECHANGED_CB callback.
func setAppearanceChangedFunc(ih Ihandle, f AppearanceChangedFunc) {
	storeCallback(ih, "_IUPGO_APPEARANCECHANGED_CB", f)

	C.goIupSetAppearanceChangedFunc(ih.ptr())
}

//--------------------

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

//export goIupTableEditChangedCB
func goIupTableEditChangedCB(ih unsafe.Pointer, lin, col C.int, update *C.char) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_EDITCHANGED_CB").Value().(TableEditChangedFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col), C.GoString(update)))
}

// setTableEditChangedFunc for EDITCHANGED_CB.
func setTableEditChangedFunc(ih Ihandle, f TableEditChangedFunc) {
	storeCallback(ih, "_IUPGO_EDITCHANGED_CB", f)
	C.goIupSetTableEditChangedFunc(ih.ptr())
}

//--------------------

//export goIupTableRightClickCB
func goIupTableRightClickCB(ih unsafe.Pointer, lin, col C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_RIGHTCLICK_CB").Value().(TableRightClickFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col)))
}

// setTableRightClickFunc for RIGHTCLICK_CB (Table version).
func setTableRightClickFunc(ih Ihandle, f TableRightClickFunc) {
	storeCallback(ih, "_IUPGO_RIGHTCLICK_CB", f)
	C.goIupSetTableRightClickFunc(ih.ptr())
}

//--------------------

//export goIupCellSelectionCB
func goIupCellSelectionCB(ih unsafe.Pointer, lin1, col1, lin2, col2 C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_CELLSELECTION_CB").Value().(CellSelectionFunc)

	return C.int(f((Ihandle)(ih), int(lin1), int(col1), int(lin2), int(col2)))
}

// setCellSelectionFunc for CELLSELECTION_CB.
func setCellSelectionFunc(ih Ihandle, f CellSelectionFunc) {
	storeCallback(ih, "_IUPGO_CELLSELECTION_CB", f)
	C.goIupSetCellSelectionFunc(ih.ptr())
}

//--------------------

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

// setTableImageFunc for IMAGE_CB (Table version).
func setTableImageFunc(ih Ihandle, f TableImageFunc) {
	storeCallback(ih, "_IUPGO_TABLE_IMAGE_CB", f)
	C.goIupSetTableImageFunc(ih.ptr())
}

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

//export goIupFileCB
func goIupFileCB(ih unsafe.Pointer, filename, status *C.char) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_FILE_CB")
	if ch == 0 {
		return C.int(DEFAULT)
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

//export goIupLayoutUpdateCB
func goIupLayoutUpdateCB(ih unsafe.Pointer) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_LAYOUTUPDATE_CB")
	if ch == 0 {
		return C.int(DEFAULT)
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

//export goIupHighlightCB
func goIupHighlightCB(ih unsafe.Pointer) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_HIGHLIGHT_CB")
	if ch == 0 {
		return C.int(DEFAULT)
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

//export goIupMenuCloseCB
func goIupMenuCloseCB(ih unsafe.Pointer) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_MENUCLOSE_CB")
	if ch == 0 {
		return C.int(DEFAULT)
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

//export goIupColorUpdateCB
func goIupColorUpdateCB(ih unsafe.Pointer) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_COLORUPDATE_CB")
	if ch == 0 {
		return C.int(DEFAULT)
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

//export goIupNotifyDismissCB
func goIupNotifyDismissCB(ih unsafe.Pointer, reason C.int) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_DISMISS_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(NotifyDismissFunc)
	return C.int(f((Ihandle)(ih), int(reason)))
}

// setNotifyDismissFunc for DISMISS_CB on notifications.
func setNotifyDismissFunc(ih Ihandle, f NotifyDismissFunc) {
	storeCallback(ih, "_IUPGO_DISMISS_CB", f)
	C.goIupSetNotifyDismissFunc(ih.ptr())
}

//--------------------

//export goIupLocationCB
func goIupLocationCB(ih unsafe.Pointer, latitude, longitude C.double) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_LOCATION_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(LocationFunc)
	return C.int(f((Ihandle)(ih), float64(latitude), float64(longitude)))
}

// setLocationFunc for LOCATION_CB.
func setLocationFunc(ih Ihandle, f LocationFunc) {
	storeCallback(ih, "_IUPGO_LOCATION_CB", f)
	C.goIupSetLocationFunc(ih.ptr())
}

//--------------------

//export goIupSensorCB
func goIupSensorCB(ih unsafe.Pointer, x, y, z C.double) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_SENSOR_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(SensorFunc)
	return C.int(f((Ihandle)(ih), float64(x), float64(y), float64(z)))
}

// setSensorFunc for SENSOR_CB.
func setSensorFunc(ih Ihandle, f SensorFunc) {
	storeCallback(ih, "_IUPGO_SENSOR_CB", f)
	C.goIupSetSensorFunc(ih.ptr())
}

//--------------------

//export goIupPermissionCB
func goIupPermissionCB(ih unsafe.Pointer, granted C.int) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_PERMISSION_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(PermissionFunc)
	return C.int(f((Ihandle)(ih), int(granted)))
}

// setPermissionFunc for PERMISSION_CB.
func setPermissionFunc(ih Ihandle, f PermissionFunc) {
	storeCallback(ih, "_IUPGO_PERMISSION_CB", f)
	C.goIupSetPermissionFunc(ih.ptr())
}

//--------------------

//export goIupGetParamCB
func goIupGetParamCB(dialog unsafe.Pointer, paramIndex C.int, userData unsafe.Pointer) C.int {
	ch := goHandle(userData)
	f := ch.Value().(GetParamFunc)
	return C.int(f(Ihandle(dialog), int(paramIndex)))
}

//--------------------

//export goIupParamCB
func goIupParamCB(ih unsafe.Pointer, paramIndex C.int, userData unsafe.Pointer) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_PARAM_CB")
	if ch == 0 {
		return C.int(1)
	}
	f := ch.Value().(ParamFunc)
	return C.int(f((Ihandle)(ih), int(paramIndex)))
}

// setParamFunc for PARAM_CB on ParamBox.
func setParamFunc(ih Ihandle, f ParamFunc) {
	storeCallback(ih, "_IUPGO_PARAM_CB", f)
	C.goIupSetParamFunc(ih.ptr())
}

//--------------------

//export goIupCustomFrameDrawCB
func goIupCustomFrameDrawCB(ih unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_CUSTOMFRAMEDRAW_CB").Value().(CustomFrameDrawFunc)
	return C.int(f((Ihandle)(ih)))
}

// setCustomFrameDrawFunc for CUSTOMFRAMEDRAW_CB.
func setCustomFrameDrawFunc(ih Ihandle, f CustomFrameDrawFunc) {
	storeCallback(ih, "_IUPGO_CUSTOMFRAMEDRAW_CB", f)
	C.goIupSetCustomFrameDrawFunc(ih.ptr())
}

//--------------------

//export goIupCustomFrameActivateCB
func goIupCustomFrameActivateCB(ih unsafe.Pointer, active C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_CUSTOMFRAMEACTIVATE_CB").Value().(CustomFrameActivateFunc)
	return C.int(f((Ihandle)(ih), int(active)))
}

// setCustomFrameActivateFunc for CUSTOMFRAMEACTIVATE_CB.
func setCustomFrameActivateFunc(ih Ihandle, f CustomFrameActivateFunc) {
	storeCallback(ih, "_IUPGO_CUSTOMFRAMEACTIVATE_CB", f)
	C.goIupSetCustomFrameActivateFunc(ih.ptr())
}

//--------------------

//export goIupWomCB
func goIupWomCB(ih unsafe.Pointer, state C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_WOM_CB").Value().(WomFunc)
	return C.int(f((Ihandle)(ih), int(state)))
}

// setWomFunc for WOM_CB.
func setWomFunc(ih Ihandle, f WomFunc) {
	storeCallback(ih, "_IUPGO_WOM_CB", f)
	C.goIupSetWomFunc(ih.ptr())
}
