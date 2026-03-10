package iup

import (
	"runtime/cgo"
	"unsafe"
)

/*
#include "bind_callbacks.h"
*/
import "C"

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
