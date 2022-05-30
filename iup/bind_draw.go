package iup

import (
	"unsafe"
)

/*
#include <stdlib.h>
#include <iup.h>
#include <iupdraw.h>
*/
import "C"

// DrawBegin initialize the drawing process.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawBegin(ih Ihandle) {
	C.IupDrawBegin(ih.ptr())
}

// DrawEnd terminates the drawing process and actually draw on screen..
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawEnd(ih Ihandle) {
	C.IupDrawEnd(ih.ptr())
}

// DrawSetClipRect defines a rectangular clipping region.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawSetClipRect(ih Ihandle, x1, y1, x2, y2 int) {
	C.IupDrawSetClipRect(ih.ptr(), C.int(x1), C.int(y1), C.int(x2), C.int(y2))
}

// DrawResetClip resets the clipping area to none.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawResetClip(ih Ihandle) {
	C.IupDrawResetClip(ih.ptr())
}

// DrawGetClipRect returns the previous rectangular clipping region set by DrawSetClipRect,
// if clipping was reset returns 0 in all values.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawGetClipRect(ih Ihandle) (x1, y1, x2, y2 int) {
	var cX1, cY1, cX2, cY2 C.int
	C.IupDrawGetClipRect(ih.ptr(), &cX1, &cY1, &cX2, &cY2)
	x1, y1, x2, y2 = int(cX1), int(cY1), int(cX2), int(cY2)
	return
}

// DrawParentBackground fills the canvas with the native parent background color.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawParentBackground(ih Ihandle) {
	C.IupDrawParentBackground(ih.ptr())
}

// DrawLine draws a line including start and end points.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawLine(ih Ihandle, x1, y1, x2, y2 int) {
	C.IupDrawLine(ih.ptr(), C.int(x1), C.int(y1), C.int(x2), C.int(y2))
}

// DrawRectangle draws a rectangle including start and end points.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawRectangle(ih Ihandle, x1, y1, x2, y2 int) {
	C.IupDrawRectangle(ih.ptr(), C.int(x1), C.int(y1), C.int(x2), C.int(y2))
}

// DrawArc draws an arc inside a rectangle between the two angles in degrees.
// When filled will draw a pie shape with the vertex at the center of the rectangle.
// Angles are counter-clock wise relative to the 3 o'clock position.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawArc(ih Ihandle, x1, y1, x2, y2 int, a1, a2 float64) {
	C.IupDrawArc(ih.ptr(), C.int(x1), C.int(y1), C.int(x2), C.int(y2), C.double(a1), C.double(a2))
}

// DrawPolygon draws a polygon.
// Coordinates are stored in the array in the sequence: x1, y1, x2, y2, ...
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawPolygon(ih Ihandle, points []int, count int) {
	C.IupDrawPolygon(ih.ptr(), (*C.int)(unsafe.Pointer(&points[0])), C.int(count))
}

// DrawText draws a text in the given position using the font defined by DRAWFONT, if not defined then use FONT.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawText(ih Ihandle, str string, x, y, w, h int) {
	cStr := C.CString(str)
	defer C.free(unsafe.Pointer(cStr))

	C.IupDrawText(ih.ptr(), cStr, C.int(len(str)), C.int(x), C.int(y), C.int(w), C.int(h))
}

// DrawImage draws an image given its name.
// The coordinates are relative the top-left corner of the image.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawImage(ih Ihandle, name string, x, y, w, h int) {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	C.IupDrawImage(ih.ptr(), cName, C.int(x), C.int(y), C.int(w), C.int(h))
}

// DrawSelectRect draws a selection rectangle.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawSelectRect(ih Ihandle, x1, y1, x2, y2 int) {
	C.IupDrawSelectRect(ih.ptr(), C.int(x1), C.int(y1), C.int(x2), C.int(y2))
}

// DrawFocusRect draws a focus rectangle.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawFocusRect(ih Ihandle, x1, y1, x2, y2 int) {
	C.IupDrawFocusRect(ih.ptr(), C.int(x1), C.int(y1), C.int(x2), C.int(y2))
}

// DrawGetSize returns the drawing area size.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawGetSize(ih Ihandle) (w, h int) {
	var cW, cH C.int
	C.IupDrawGetSize(ih.ptr(), &cW, &cH)
	w, h = int(cW), int(cH)
	return
}

// DrawGetTextSize returns the given text size using the font defined by DRAWFONT, if not defined then use FONT.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawGetTextSize(ih Ihandle, str string) (w, h int) {
	cStr := C.CString(str)
	defer C.free(unsafe.Pointer(cStr))

	var cW, cH C.int
	C.IupDrawGetTextSize(ih.ptr(), cStr, C.int(len(str)), &cW, &cH)
	w, h = int(cW), int(cH)
	return
}

// DrawGetImageInfo returns the given image size and bits per pixel.
// bpp can be 8, 24 or 32.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawGetImageInfo(name string) (w, h, bpp int) {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	var cW, cH, cBpp C.int
	C.IupDrawGetImageInfo(cName, &cW, &cH, &cBpp)
	w, h, bpp = int(cW), int(cH), int(cBpp)
	return
}
