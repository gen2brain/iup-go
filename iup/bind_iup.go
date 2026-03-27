package iup

import (
	"os"
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
// UTF8MODE is automatically enabled.
// To disable UTF8MODE, call SetGlobal("UTF8MODE", "NO") after Open().
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_open.md
func Open() int {
	ret := int(C.IupOpen(nil, nil))

	SetGlobal("UTF8MODE", "YES")
	SetGlobal("UTF8MODE_FILE", "YES")
	if len(os.Args) > 0 {
		SetGlobal("ARGV0", os.Args[0])
	}
	return ret
}

// Close ends the IUP toolkit and releases internal memory.
// It will also automatically destroy all dialogs and all elements that have names.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_close.md
func Close() {
	C.IupClose()
}

// Version returns a string with the IUP version number.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_version.md
func Version() string {
	return C.GoString(C.IupVersion())
}

// VersionDate returns a string with the IUP version date.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_version.md
func VersionDate() string {
	return C.GoString(C.IupVersionDate())
}

// VersionNumber returns a string with the IUP version number.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_version.md
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
