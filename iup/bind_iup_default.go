//go:build !android && !ios && !js

package iup

/*
#include "iup.h"
*/
import "C"

// Open initializes the IUP toolkit.
// Must be called before any other IUP function.
//
// UTF8MODE is automatically enabled.
// To disable UTF8MODE, call SetGlobal("UTF8MODE", "NO") after Open().
//
// https://gen2brain.github.io/iup-go/func/iup_open.html
func Open() int {
	return openShared()
}

// Close ends the IUP toolkit and releases internal memory.
// It will also automatically destroy all dialogs and all elements that have names.
//
// https://gen2brain.github.io/iup-go/func/iup_close.html
func Close() {
	C.IupClose()
}

// EntryPoint is a portability shim. On desktop it is a no-op; on
// Android and iOS it registers entry as the ENTRY_POINT callback.
// Call from init() to keep a single main() portable.
func EntryPoint(entry func()) {}
