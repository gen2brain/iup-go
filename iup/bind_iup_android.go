//go:build android

package iup

/*
#include "iup.h"
#include "iupandroid_drv.h"
*/
import "C"

import "os"

// Pre-open IUP so users can register ENTRY_POINT from a package
// init() before Java fires it.
func init() {
	Open()
}

// Open initializes the IUP toolkit. Repeat calls return NOERROR
// rather than OPENED so the same source works on desktop and Android.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_open.md
func Open() int {
	ret := int(C.iupAndroid_OpenOnce())
	SetGlobal("UTF8MODE", "YES")
	SetGlobal("UTF8MODE_FILE", "YES")
	if len(os.Args) > 0 {
		SetGlobal("ARGV0", os.Args[0])
	}
	if ret == OPENED {
		return NOERROR
	}
	return ret
}

// Close is a no-op on Android; the hosting Activity owns process teardown.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_close.md
func Close() {}

// EntryPoint registers entry as the ENTRY_POINT callback fired by
// IupLaunchActivity. Call from init() to keep a single main()
// portable across desktop and mobile.
//
// Typical usage:
//
//	func init() { iup.EntryPoint(main) }
//
//	func main() {
//	    iup.Open()
//	    defer iup.Close()
//	    iup.Show(iup.Dialog(...))
//	    iup.MainLoop()
//	}
func EntryPoint(entry func()) {
	SetFunction("ENTRY_POINT", EntryPointFunc(entry))
}
