//go:build !cgo && !js

package iup

import (
	"unsafe"

	"github.com/ebitengine/purego"
)

// PlayEndFunc for PLAYEND_CB callback.
// Called when playback reaches the end of the file.
type PlayEndFunc func(ih Ihandle) int

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

// FrameFunc for FRAME_CB callback.
// Called for every captured frame, before it is drawn. The data slice holds packed RGB pixels and is valid only during the callback.
type FrameFunc func(ih Ihandle, width, height int, data []byte) int

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
