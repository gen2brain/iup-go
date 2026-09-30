//go:build android

package iup

/*
#include <stdlib.h>
#include <sys/system_properties.h>
#include "iup.h"
#include "iupandroid_drv.h"
*/
import "C"

import (
	"os"
	"time"
	"unsafe"
)

// Pre-open IUP so users can register ENTRY_POINT from a package
// init() before Java fires it.
func init() {
	Open()
	setTimeLocal()
}

// setTimeLocal loads the system time zone, an Android app has neither TZ nor /etc/localtime.
func setTimeLocal() {
	key := C.CString("persist.sys.timezone")
	defer C.free(unsafe.Pointer(key))
	var buf [C.PROP_VALUE_MAX]C.char
	n := C.__system_property_get(key, &buf[0])
	if n <= 0 {
		return
	}
	if loc, err := time.LoadLocation(C.GoStringN(&buf[0], n)); err == nil {
		time.Local = loc
	}
}

// Open initializes the IUP toolkit. Repeat calls return NOERROR
// rather than OPENED so the same source works on desktop and Android.
//
// https://gen2brain.github.io/iup-go/func/iup_open.html
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
// https://gen2brain.github.io/iup-go/func/iup_close.html
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
