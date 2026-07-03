//go:build !cgo && !js

package iup

import "testing"

// nocgo-internal: callback registry clears on Destroy via the LDESTROY trampoline.
func TestNocgoCallbackRegistry(t *testing.T) {
	btn := Button("x")
	SetCallback(btn, "ACTION", ActionFunc(func(Ihandle) int { return DEFAULT }))
	SetCallback(btn, "BUTTON_CB", ButtonFunc(func(Ihandle, int, int, int, int, string) int { return DEFAULT }))
	SetCallback(btn, "MOTION_CB", MotionFunc(func(Ihandle, int, int, string) int { return DEFAULT }))
	SetCallback(btn, "K_ANY", KAnyFunc(func(Ihandle, int) int { return CONTINUE }))

	cbReg.Lock()
	n := len(cbReg.m)
	cbReg.Unlock()
	if n != 4 {
		t.Fatalf("callback registry size = %d, want 4", n)
	}

	Destroy(Dialog(Vbox(btn)))

	cbReg.Lock()
	n = len(cbReg.m)
	cbReg.Unlock()
	if n != 0 {
		t.Fatalf("registry not cleaned after Destroy: %d left (LDESTROY trampoline did not run)", n)
	}
}

// nocgo-internal: rotating buffer pool backing string-return callbacks round-trips.
func TestNocgoCReturnStr(t *testing.T) {
	if cReturnStr("") != 0 {
		t.Fatal("empty string should return NULL")
	}
	a := cReturnStr("Item 1")
	b := cReturnStr("Item 2")
	if goString(a) != "Item 1" || goString(b) != "Item 2" {
		t.Fatalf("pool round-trip failed: %q %q", goString(a), goString(b))
	}
}
