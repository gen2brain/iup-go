//go:build !js

package iup

import (
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
	f := loadCallback((Ihandle)(ih), "_IUPGO_COMPLETED_CB").Value().(CompletedFunc)

	goUrl := C.GoString((*C.char)(url))
	return C.int(f((Ihandle)(ih), goUrl))
}

// setCompletedFunc for COMPLETED_CB.
func setCompletedFunc(ih Ihandle, f CompletedFunc) {
	storeCallback(ih, "_IUPGO_COMPLETED_CB", f)

	C.goIupSetCompletedFunc(ih.ptr())
}

//--------------------

// ErrorFunc for ERROR_CB callback.
// Action generated when a page load fails.
type ErrorFunc func(ih Ihandle, url string) int

//export goIupErrorCB
func goIupErrorCB(ih unsafe.Pointer, url unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_ERROR_CB").Value().(ErrorFunc)

	goUrl := C.GoString((*C.char)(url))
	return C.int(f((Ihandle)(ih), goUrl))
}

// setErrorFunc for ERROR_CB.
func setErrorFunc(ih Ihandle, f ErrorFunc) {
	storeCallback(ih, "_IUPGO_ERROR_CB", f)

	C.goIupSetErrorFunc(ih.ptr())
}

//--------------------

// NavigateFunc for NAVIGATE_CB callback.
// Action generated when the browser requests a navigation to another page.
type NavigateFunc func(ih Ihandle, url string) int

//export goIupNavigateCB
func goIupNavigateCB(ih unsafe.Pointer, url unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_NAVIGATE_CB").Value().(NavigateFunc)

	goUrl := C.GoString((*C.char)(url))
	return C.int(f((Ihandle)(ih), goUrl))
}

// setNavigateFunc for NAVIGATE_CB.
func setNavigateFunc(ih Ihandle, f NavigateFunc) {
	storeCallback(ih, "_IUPGO_NAVIGATE_CB", f)

	C.goIupSetNavigateFunc(ih.ptr())
}

//--------------------

// NewWindowFunc for NEWWINDOW_CB callback.
// Action generated when the browser requests a new window.
type NewWindowFunc func(ih Ihandle, url string) int

//export goIupNewWindowCB
func goIupNewWindowCB(ih unsafe.Pointer, url unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_NEWWINDOW_CB").Value().(NewWindowFunc)

	goUrl := C.GoString((*C.char)(url))
	return C.int(f((Ihandle)(ih), goUrl))
}

// setNewWindowFunc for NEWWINDOW_CB.
func setNewWindowFunc(ih Ihandle, f NewWindowFunc) {
	storeCallback(ih, "_IUPGO_NEWWINDOW_CB", f)

	C.goIupSetNewWindowFunc(ih.ptr())
}
