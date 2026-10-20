//go:build !js

package iup

import (
	"unsafe"
)

/*
#include "bind_callbacks.h"
*/
import "C"

// PlayEndFunc for PLAYEND_CB callback.
// Called when playback reaches the end of the file.
type PlayEndFunc func(ih Ihandle) int

// ReadyFunc for READY_CB callback.
// Called when the source is ready to play.
type ReadyFunc func(ih Ihandle) int

// StateChangedFunc for STATECHANGED_CB callback.
// Called when the playback state changes.
type StateChangedFunc func(ih Ihandle, state string) int

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

// FrameFunc for FRAME_CB callback.
// Called for every captured frame, before it is drawn. The data slice holds packed RGB pixels and is valid only during the callback.
type FrameFunc func(ih Ihandle, width, height int, data []byte) int

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

// SamplesFunc for SAMPLES_CB callback.
// Called for every block of captured audio. The samples slice holds 16-bit interleaved frames and is valid only during the callback.
type SamplesFunc func(ih Ihandle, frames, channels int, samples []int16) int

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
