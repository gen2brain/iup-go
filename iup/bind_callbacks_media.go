//go:build !js

package iup

import (
	"unsafe"
)

/*
#include "bind_callbacks.h"
*/
import "C"

//export goIupReadyCB
func goIupReadyCB(ih unsafe.Pointer) C.int {
	f := loadCallback(Ihandle(ih), "_IUPGO_READY_CB").Value().(ReadyFunc)
	return C.int(f(Ihandle(ih)))
}

func setReadyFunc(ih Ihandle, f ReadyFunc) {
	storeCallback(ih, "_IUPGO_READY_CB", f)
	C.goIupSetReadyFunc(ih.ptr())
}

//export goIupStateChangedCB
func goIupStateChangedCB(ih, state unsafe.Pointer) C.int {
	f := loadCallback(Ihandle(ih), "_IUPGO_STATECHANGED_CB").Value().(StateChangedFunc)
	return C.int(f(Ihandle(ih), C.GoString((*C.char)(state))))
}

func setStateChangedFunc(ih Ihandle, f StateChangedFunc) {
	storeCallback(ih, "_IUPGO_STATECHANGED_CB", f)
	C.goIupSetStateChangedFunc(ih.ptr())
}

//export goIupPlayEndCB
func goIupPlayEndCB(ih unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_PLAYEND_CB").Value().(PlayEndFunc)

	return C.int(f((Ihandle)(ih)))
}

// setPlayEndFunc for PLAYEND_CB.
func setPlayEndFunc(ih Ihandle, f PlayEndFunc) {
	storeCallback(ih, "_IUPGO_PLAYEND_CB", f)

	C.goIupSetPlayEndFunc(ih.ptr())
}

//export goIupFrameCB
func goIupFrameCB(ih unsafe.Pointer, width, height C.int, data unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_FRAME_CB").Value().(FrameFunc)

	return C.int(f((Ihandle)(ih), int(width), int(height), unsafe.Slice((*byte)(data), int(width)*int(height)*3)))
}

// setFrameFunc for FRAME_CB.
func setFrameFunc(ih Ihandle, f FrameFunc) {
	storeCallback(ih, "_IUPGO_FRAME_CB", f)

	C.goIupSetFrameFunc(ih.ptr())
}

//export goIupSamplesCB
func goIupSamplesCB(ih unsafe.Pointer, frames, channels C.int, samples unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_SAMPLES_CB").Value().(SamplesFunc)

	return C.int(f((Ihandle)(ih), int(frames), int(channels), unsafe.Slice((*int16)(samples), int(frames)*int(channels))))
}

// setSamplesFunc for SAMPLES_CB.
func setSamplesFunc(ih Ihandle, f SamplesFunc) {
	storeCallback(ih, "_IUPGO_SAMPLES_CB", f)

	C.goIupSetSamplesFunc(ih.ptr())
}
