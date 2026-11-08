//go:build !cgo && !js

package iup

import (
	"unsafe"

	"github.com/ebitengine/purego"
)

var readyCB = purego.NewCallback(func(ih uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_READY_CB").(ReadyFunc); ok {
		return f(Ihandle(ih))
	}
	return DEFAULT
})

var stateChangedCB = purego.NewCallback(func(ih, state uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_STATECHANGED_CB").(StateChangedFunc); ok {
		return f(Ihandle(ih), goString(state))
	}
	return DEFAULT
})

func setReadyFunc(ih Ihandle, f ReadyFunc) {
	storeCallback(ih, "_IUPGO_READY_CB", f)
	iupSetCallback(uintptr(ih), "READY_CB", readyCB)
}

func setStateChangedFunc(ih Ihandle, f StateChangedFunc) {
	storeCallback(ih, "_IUPGO_STATECHANGED_CB", f)
	iupSetCallback(uintptr(ih), "STATECHANGED_CB", stateChangedCB)
}

var playEndCB = purego.NewCallback(func(ih uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_PLAYEND_CB").(PlayEndFunc); ok {
		return f(Ihandle(ih))
	}
	return 0
})

func setPlayEndFunc(ih Ihandle, f PlayEndFunc) {
	storeCallback(ih, "_IUPGO_PLAYEND_CB", f)
	iupSetCallback(uintptr(ih), "PLAYEND_CB", playEndCB)
}

var frameCB = purego.NewCallback(func(ih uintptr, width, height int32, data uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_FRAME_CB").(FrameFunc); ok {
		return f(Ihandle(ih), int(width), int(height), unsafe.Slice((*byte)(goPtr(data)), int(width)*int(height)*3))
	}
	return 0
})

func setFrameFunc(ih Ihandle, f FrameFunc) {
	storeCallback(ih, "_IUPGO_FRAME_CB", f)
	iupSetCallback(uintptr(ih), "FRAME_CB", frameCB)
}

var samplesCB = purego.NewCallback(func(ih uintptr, frames, channels int32, samples uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_SAMPLES_CB").(SamplesFunc); ok {
		return f(Ihandle(ih), int(frames), int(channels), unsafe.Slice((*int16)(goPtr(samples)), int(frames)*int(channels)))
	}
	return 0
})

func setSamplesFunc(ih Ihandle, f SamplesFunc) {
	storeCallback(ih, "_IUPGO_SAMPLES_CB", f)
	iupSetCallback(uintptr(ih), "SAMPLES_CB", samplesCB)
}
