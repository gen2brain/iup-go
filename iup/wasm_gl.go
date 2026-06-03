//go:build js && wasm && gl

package iup

import (
	"strconv"
	"syscall/js"
)

// GLCanvasOpen must be called after Open, so that the control can be used.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_glcanvas.md
func GLCanvasOpen() {
	ccall("IupGLCanvasOpen", "", nil, nil)
}

// GLCanvas creates an OpenGL canvas (drawing area for OpenGL). It inherits from Canvas.
// Obtain the WebGL rendering context with GLCanvasElement and drive it through syscall/js.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_glcanvas.md
func GLCanvas() Ihandle {
	return ccallHandle("IupGLCanvas", nil, nil)
}

// GLBackgroundBox creates a simple native container with no decorations, but with OpenGL enabled. It inherits from GLCanvas.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_glbackgroundbox.md
func GLBackgroundBox(child Ihandle) Ihandle {
	return ccallHandle("IupGLBackgroundBox", []interface{}{"number"}, []interface{}{int(child)})
}

// GLMakeCurrent activates the given canvas as the current OpenGL context.
// The first call sets the global attributes GL_VERSION, GL_VENDOR and GL_RENDERER.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_glcanvas.md
func GLMakeCurrent(ih Ihandle) {
	ccall("IupGLMakeCurrent", "", []interface{}{"number"}, []interface{}{int(ih)})
}

// GLIsCurrent returns true if the given canvas is the current OpenGL context.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_glcanvas.md
func GLIsCurrent(ih Ihandle) bool {
	return ccall("IupGLIsCurrent", "number", []interface{}{"number"}, []interface{}{int(ih)}).Int() != 0
}

// GLSwapBuffers makes the BACK buffer visible. Under WebGL the drawing buffer is
// presented automatically when control returns to the browser; this fires the
// SWAPBUFFERS_CB callback.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_glcanvas.md
func GLSwapBuffers(ih Ihandle) {
	ccall("IupGLSwapBuffers", "", []interface{}{"number"}, []interface{}{int(ih)})
}

// GLPalette defines a color in the color palette. Not supported under WebGL.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_glcanvas.md
func GLPalette(ih Ihandle, index int, r, g, b float32) {
	ccall("IupGLPalette", "", []interface{}{"number", "number", "number", "number", "number"}, []interface{}{int(ih), index, float64(r), float64(g), float64(b)})
}

// GLUseFont creates a bitmap display list from the current FONT attribute. Not supported under WebGL.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_glcanvas.md
func GLUseFont(ih Ihandle, first, count, listBase int) {
	ccall("IupGLUseFont", "", []interface{}{"number", "number", "number", "number"}, []interface{}{int(ih), first, count, listBase})
}

// GLWait calls glFinish when gl is non zero, otherwise glFlush.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_glcanvas.md
func GLWait(gl int) {
	ccall("IupGLWait", "", []interface{}{"number"}, []interface{}{gl})
}

// GLCanvasElement returns the worker-local OffscreenCanvas backing a GLCanvas, so
// the WebGL context can be obtained via getContext. Call GLMakeCurrent first.
// Null if ih is not mapped.
func GLCanvasElement(ih Ihandle) js.Value {
	id := ccall("iupwasmGLDomId", "number", []interface{}{"number"}, []interface{}{int(ih)}).Int()
	if id == 0 {
		return js.Null()
	}
	if local := js.Global().Get("__iupLocal"); local.Truthy() {
		if c := local.Get(strconv.Itoa(id)); c.Truthy() {
			return c
		}
	}
	return js.Null()
}
