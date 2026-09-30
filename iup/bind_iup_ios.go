//go:build ios

package iup

/*
#include <Foundation/Foundation.h>

static void iupGoSystemTimeZone(char* name, int size, int* offset)
{
	@autoreleasepool
	{
		NSTimeZone* tz = [NSTimeZone systemTimeZone];
		*offset = (int)[tz secondsFromGMT];
		if (![[tz name] getCString:name maxLength:size encoding:NSUTF8StringEncoding])
			name[0] = 0;
	}
}
*/
import "C"

import "time"

// Pre-open IUP so users can register ENTRY_POINT from a package
// init() before UIKit fires it.
func init() {
	Open()
	setTimeLocal()
}

// setTimeLocal loads the system time zone, an iOS app has neither TZ nor a readable /etc/localtime.
func setTimeLocal() {
	var buf [128]C.char
	var offset C.int
	C.iupGoSystemTimeZone(&buf[0], C.int(len(buf)), &offset)
	name := C.GoString(&buf[0])
	if loc, err := time.LoadLocation(name); err == nil {
		time.Local = loc
	} else {
		time.Local = time.FixedZone(name, int(offset))
	}
}

// Open initializes the IUP toolkit. Repeat calls return NOERROR
// rather than OPENED so the same source works on desktop and iOS.
//
// https://gen2brain.github.io/iup-go/func/iup_open.html
func Open() int {
	ret := openShared()
	if ret == OPENED {
		return NOERROR
	}
	return ret
}

// Close is a no-op on iOS; UIKit owns process lifetime.
//
// https://gen2brain.github.io/iup-go/func/iup_close.html
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
