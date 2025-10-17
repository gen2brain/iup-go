//go:build gl

package iup

/*
#include <stdlib.h>
#include "iup.h"
#include "iupgl.h"
*/
import "C"

import (
	"github.com/google/uuid"
)

// GLCanvasOpen must be called after Open, so that the control can be used.
//
// https://www.tecgraf.puc-rio.br/iup/en/ctrl/iupglcanvas.html
func GLCanvasOpen() {
	C.IupGLCanvasOpen()
}

// GLCanvas creates an OpenGL canvas (drawing area for OpenGL). It inherits from Canvas.
//
// https://www.tecgraf.puc-rio.br/iup/en/ctrl/iupglcanvas.html
func GLCanvas() Ihandle {
	h := mkih(C.IupGLCanvas(nil))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// GLMakeCurrent activates the given canvas as the current OpenGL context.
// All subsequent OpenGL commands are directed to such canvas.
// The first call will set the global attributes GL_VERSION, GL_VENDOR and GL_RENDERER (since 3.16).
//
// https://www.tecgraf.puc-rio.br/iup/en/ctrl/iupglcanvas.html
func GLMakeCurrent(ih Ihandle) {
	C.IupGLMakeCurrent(ih.ptr())
}

// GLIsCurrent returns a non zero value if the given canvas is the current OpenGL context.
//
// https://www.tecgraf.puc-rio.br/iup/en/ctrl/iupglcanvas.html
func GLIsCurrent(ih Ihandle) bool {
	return int(C.IupGLIsCurrent(ih.ptr())) != 0
}

// GLSwapBuffers makes the BACK buffer visible. This function is necessary when a double buffer is used.
//
// https://www.tecgraf.puc-rio.br/iup/en/ctrl/iupglcanvas.html
func GLSwapBuffers(ih Ihandle) {
	C.IupGLSwapBuffers(ih.ptr())
}

// GLPalette defines a color in the color palette. This function is necessary when INDEX color is used.
//
// https://www.tecgraf.puc-rio.br/iup/en/ctrl/iupglcanvas.html
func GLPalette(ih Ihandle, index int, r, g, b float32) {
	C.IupGLPalette(ih.ptr(), C.int(index), C.float(r), C.float(g), C.float(b))
}

// GLUseFont creates a bitmap display list from the current FONT attribute.
// See the documentation of the wglUseFontBitmaps and glXUseXFont functions.
//
// https://www.tecgraf.puc-rio.br/iup/en/ctrl/iupglcanvas.html
func GLUseFont(ih Ihandle, first, count, listBase int) {
	C.IupGLUseFont(ih.ptr(), C.int(first), C.int(count), C.int(listBase))
}

// GLWait if gl is non zero it will call glFinish or glXWaitGL, else will call GdiFlush or glXWaitX.
//
// https://www.tecgraf.puc-rio.br/iup/en/ctrl/iupglcanvas.html
func GLWait(gl int) {
	C.IupGLWait(C.int(gl))
}

// GLBackgroundBox creates a simple native container with no decorations, but with OpenGL enabled. It inherits from GLCanvas.
//
// OBS: this is identical to the BackgroundBox element, but with OpenGL enabled.
//
// https://www.tecgraf.puc-rio.br/iup/en/ctrl/iupglbackgroundbox.html
func GLBackgroundBox(child Ihandle) Ihandle {
	h := mkih(C.IupGLBackgroundBox(child.ptr()))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}
