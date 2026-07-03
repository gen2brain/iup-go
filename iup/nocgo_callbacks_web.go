//go:build !cgo && !js

package iup

import "github.com/ebitengine/purego"

type CompletedFunc func(ih Ihandle, url string) int

var completedFuncCB = purego.NewCallback(func(ih uintptr, url uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_COMPLETED_CB").(CompletedFunc); ok {
		return f(Ihandle(ih), goString(url))
	}
	return 0
})

func setCompletedFunc(ih Ihandle, f CompletedFunc) {
	storeCallback(ih, "_IUPGO_COMPLETED_CB", f)
	iupSetCallback(uintptr(ih), "COMPLETED_CB", completedFuncCB)
}

type ErrorFunc func(ih Ihandle, url string) int

var errorFuncCB = purego.NewCallback(func(ih uintptr, url uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_ERROR_CB").(ErrorFunc); ok {
		return f(Ihandle(ih), goString(url))
	}
	return 0
})

func setErrorFunc(ih Ihandle, f ErrorFunc) {
	storeCallback(ih, "_IUPGO_ERROR_CB", f)
	iupSetCallback(uintptr(ih), "ERROR_CB", errorFuncCB)
}

type NavigateFunc func(ih Ihandle, url string) int

var navigateFuncCB = purego.NewCallback(func(ih uintptr, url uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_NAVIGATE_CB").(NavigateFunc); ok {
		return f(Ihandle(ih), goString(url))
	}
	return 0
})

func setNavigateFunc(ih Ihandle, f NavigateFunc) {
	storeCallback(ih, "_IUPGO_NAVIGATE_CB", f)
	iupSetCallback(uintptr(ih), "NAVIGATE_CB", navigateFuncCB)
}

type NewWindowFunc func(ih Ihandle, url string) int

var newWindowFuncCB = purego.NewCallback(func(ih uintptr, url uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_NEWWINDOW_CB").(NewWindowFunc); ok {
		return f(Ihandle(ih), goString(url))
	}
	return 0
})

func setNewWindowFunc(ih Ihandle, f NewWindowFunc) {
	storeCallback(ih, "_IUPGO_NEWWINDOW_CB", f)
	iupSetCallback(uintptr(ih), "NEWWINDOW_CB", newWindowFuncCB)
}
