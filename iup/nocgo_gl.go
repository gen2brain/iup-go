//go:build !cgo && !js && gl

package iup

import (
	"unsafe"

	"github.com/ebitengine/purego"
)

var (
	iupGLCanvasOpen     func()
	iupGLCanvas         func() uintptr
	iupGLBackgroundBox  func(child uintptr) uintptr
	iupGLMakeCurrent    func(ih uintptr)
	iupGLIsCurrent      func(ih uintptr) int32
	iupGLSwapBuffers    func(ih uintptr)
	iupGLPalette        func(ih uintptr, index int32, r, g, b float32)
	iupGLUseFont        func(ih uintptr, first, count, listBase int32)
	iupGLWait           func(gl int32)
	iupGLGetProcAddress func(name string) uintptr
)

func init() {
	ensureBase()
	lib := openLib("iupgl")
	if lib == 0 {
		panic("iup: cannot load libiupgl")
	}
	reg := func(fptr any, name string) { purego.RegisterLibFunc(fptr, lib, name) }
	reg(&iupGLCanvasOpen, "IupGLCanvasOpen")
	reg(&iupGLCanvas, "IupGLCanvas")
	reg(&iupGLBackgroundBox, "IupGLBackgroundBox")
	reg(&iupGLMakeCurrent, "IupGLMakeCurrent")
	reg(&iupGLIsCurrent, "IupGLIsCurrent")
	reg(&iupGLSwapBuffers, "IupGLSwapBuffers")
	reg(&iupGLPalette, "IupGLPalette")
	reg(&iupGLUseFont, "IupGLUseFont")
	reg(&iupGLWait, "IupGLWait")
	reg(&iupGLGetProcAddress, "IupGLGetProcAddress")
}

func GLCanvasOpen() { iupGLCanvasOpen() }

func GLCanvas() Ihandle { return mkih(iupGLCanvas()) }

func GLBackgroundBox(child Ihandle) Ihandle { return mkih(iupGLBackgroundBox(uintptr(child))) }

func GLMakeCurrent(ih Ihandle) { iupGLMakeCurrent(uintptr(ih)) }

func GLIsCurrent(ih Ihandle) bool { return iupGLIsCurrent(uintptr(ih)) != 0 }

func GLSwapBuffers(ih Ihandle) { iupGLSwapBuffers(uintptr(ih)) }

func GLPalette(ih Ihandle, index int, r, g, b float32) {
	iupGLPalette(uintptr(ih), int32(index), r, g, b)
}

func GLUseFont(ih Ihandle, first, count, listBase int) {
	iupGLUseFont(uintptr(ih), int32(first), int32(count), int32(listBase))
}

func GLWait(gl int) { iupGLWait(int32(gl)) }

func GLGetProcAddress(name string) unsafe.Pointer {
	return unsafe.Pointer(iupGLGetProcAddress(name))
}
