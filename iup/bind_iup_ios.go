//go:build ios

package iup

// Pre-open IUP so users can register ENTRY_POINT from a package
// init() before UIKit fires it.
func init() {
	Open()
}

// Open initializes the IUP toolkit. Repeat calls return NOERROR
// rather than OPENED so the same source works on desktop and iOS.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_open.md
func Open() int {
	ret := openShared()
	if ret == OPENED {
		return NOERROR
	}
	return ret
}

// Close is a no-op on iOS; UIKit owns process lifetime.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_close.md
func Close() {}

// EntryPoint registers entry as the ENTRY_POINT callback fired by
// UIApplicationMain after didFinishLaunching. Call from init() to
// keep a single main() portable across desktop and mobile.
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
