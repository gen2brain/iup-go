package iup

import (
	"runtime"
	"unsafe"
)

/*
#include <stdlib.h>
#include "iup.h"
*/
import "C"

func init() {
	runtime.LockOSThread()
}

// Open initializes the IUP toolkit.
// Must be called before any other IUP function.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupopen.html
func Open() int {
	return int(C.IupOpen(nil, nil))
}

// Close ends the IUP toolkit and releases internal memory.
// It will also automatically destroy all dialogs and all elements that have names.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupclose.html
func Close() {
	C.IupClose()
}

// Version returns a string with the IUP version number.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupversion.html
func Version() string {
	return C.GoString(C.IupVersion())
}

// VersionDate returns a string with the IUP version date.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupversion.html
func VersionDate() string {
	return C.GoString(C.IupVersionDate())
}

// VersionNumber returns a string with the IUP version number.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupversion.html
func VersionNumber() int {
	return int(C.IupVersionNumber())
}

func bool2int(v bool) int {
	if v {
		return 1
	}
	return 0
}

func cStrOrNull(s string) *C.char {
	if len(s) == 0 {
		return nil
	}
	return C.CString(s)
}

func cStrFree(p *C.char) {
	if p != nil {
		C.free(unsafe.Pointer(p))
	}
}
