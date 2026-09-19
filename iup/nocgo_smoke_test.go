//go:build !cgo && !js

package iup

import (
	"testing"

	"github.com/ebitengine/purego"
)

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

// nocgo-internal: float arguments reach a callback from the float registers and from the stack.
func TestNocgoFloatCallbacks(t *testing.T) {
	canvas := Canvas()
	defer Destroy(canvas)

	var scroll struct {
		op         int
		posx, posy float64
	}
	SetCallback(canvas, "SCROLL_CB", ScrollFunc(func(_ Ihandle, op int, posx, posy float64) int {
		scroll.op, scroll.posx, scroll.posy = op, posx, posy
		return 11
	}))
	var callScroll func(ih uintptr, op int32, posx, posy float32) int
	purego.RegisterFunc(&callScroll, scrollCB())
	if ret := callScroll(uintptr(canvas), 3, 1.5, -2.25); ret != 11 || scroll.op != 3 || scroll.posx != 1.5 || scroll.posy != -2.25 {
		t.Errorf("SCROLL_CB: ret %d, got %+v", ret, scroll)
	}

	var sensor [3]float64
	SetCallback(canvas, "SENSOR_CB", SensorFunc(func(_ Ihandle, x, y, z float64) int {
		sensor = [3]float64{x, y, z}
		return 12
	}))
	var callSensor func(ih uintptr, x, y, z float64) int
	purego.RegisterFunc(&callSensor, sensorCB())
	if ret := callSensor(uintptr(canvas), 0.125, -9.81, 1e300); ret != 12 || sensor != [3]float64{0.125, -9.81, 1e300} {
		t.Errorf("SENSOR_CB: ret %d, got %v", ret, sensor)
	}

	var gesture struct {
		ints   [4]int
		v1, v2 float64
	}
	SetCallback(canvas, "GESTURE_CB", GestureFunc(func(_ Ihandle, g, state, x, y int, v1, v2 float64) int {
		gesture.ints, gesture.v1, gesture.v2 = [4]int{g, state, x, y}, v1, v2
		return 13
	}))
	var callGesture func(ih uintptr, g, state, x, y int32, v1, v2 float64) int
	purego.RegisterFunc(&callGesture, gestureCB())
	if ret := callGesture(uintptr(canvas), 1, 2, -30, 40, 2.5, -0.75); ret != 13 || gesture.ints != [4]int{1, 2, -30, 40} || gesture.v1 != 2.5 || gesture.v2 != -0.75 {
		t.Errorf("GESTURE_CB: ret %d, got %+v", ret, gesture)
	}

	callScroll(uintptr(canvas), 4, 8, 16)
	if scroll.op != 4 || scroll.posx != 8 || scroll.posy != 16 {
		t.Errorf("SCROLL_CB after other float callbacks: %+v", scroll)
	}
}
