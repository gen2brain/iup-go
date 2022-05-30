//go:build darwin && gl

package iup

// GLCanvasOpen must be called after Open, so that the control can be used.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/ctrl/iupglcanvas.html
func GLCanvasOpen() {
	panic("not implemented")
}

// GLCanvas creates an OpenGL canvas (drawing area for OpenGL). It inherits from Canvas.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/ctrl/iupglcanvas.html
func GLCanvas() Ihandle {
	panic("not implemented")
}

// GLMakeCurrent activates the given canvas as the current OpenGL context.
// All subsequent OpenGL commands are directed to such canvas.
// The first call will set the global attributes GL_VERSION, GL_VENDOR and GL_RENDERER (since 3.16).
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/ctrl/iupglcanvas.html
func GLMakeCurrent(ih Ihandle) {
	panic("not implemented")
}

// GLIsCurrent returns a non zero value if the given canvas is the current OpenGL context.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/ctrl/iupglcanvas.html
func GLIsCurrent(ih Ihandle) bool {
	panic("not implemented")
}

// GLSwapBuffers makes the BACK buffer visible. This function is necessary when a double buffer is used.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/ctrl/iupglcanvas.html
func GLSwapBuffers(ih Ihandle) {
	panic("not implemented")
}

// GLPalette defines a color in the color palette. This function is necessary when INDEX color is used.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/ctrl/iupglcanvas.html
func GLPalette(ih Ihandle, index int, r, g, b float32) {
	panic("not implemented")
}

// GLUseFont creates a bitmap display list from the current FONT attribute.
// See the documentation of the wglUseFontBitmaps and glXUseXFont functions.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/ctrl/iupglcanvas.html
func GLUseFont(ih Ihandle, first, count, listBase int) {
	panic("not implemented")
}

// GLWait if gl is non zero it will call glFinish or glXWaitGL, else will call GdiFlush or glXWaitX.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/ctrl/iupglcanvas.html
func GLWait(gl int) {
	panic("not implemented")
}

// GLBackgroundBox creates a simple native container with no decorations, but with OpenGL enabled. It inherits from GLCanvas.
//
// OBS: this is identical to the BackgroundBox element, but with OpenGL enabled.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/ctrl/iupglbackgroundbox.html
func GLBackgroundBox(child Ihandle) Ihandle {
	panic("not implemented")
}
