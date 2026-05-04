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

// openShared is the platform-independent portion of Open.
// Per-platform wrappers live in bind_iup_default.go, bind_iup_android.go, and bind_iup_ios.go.
func openShared() int {
	ret := int(C.IupOpen(nil, nil))

	SetGlobal("UTF8MODE", "YES")
	SetGlobal("UTF8MODE_FILE", "YES")
	if len(os.Args) > 0 {
		SetGlobal("ARGV0", os.Args[0])
	}
	return ret
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
