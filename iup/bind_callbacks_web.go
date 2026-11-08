//go:build !js

package iup

import (
	"unsafe"
)

/*
#include "bind_callbacks.h"
*/
import "C"

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
