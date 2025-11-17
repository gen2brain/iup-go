package iup

import (
	"unsafe"
)

/*
#include <stdlib.h>
#include "iup.h"
#include "iupdraw.h"
*/
import "C"

// DrawBegin initialize the drawing process.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawBegin(ih Ihandle) {
	C.IupDrawBegin(ih.ptr())
}

// DrawEnd terminates the drawing process and actually draw on screen..
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawEnd(ih Ihandle) {
	C.IupDrawEnd(ih.ptr())
}

// DrawSetClipRect defines a rectangular clipping region.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawSetClipRect(ih Ihandle, x1, y1, x2, y2 int) {
	C.IupDrawSetClipRect(ih.ptr(), C.int(x1), C.int(y1), C.int(x2), C.int(y2))
}

// DrawSetClipRoundedRect defines a rounded rectangular clipping region.
// This is useful for drawing gradients or other content with rounded corners.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawSetClipRoundedRect(ih Ihandle, x1, y1, x2, y2, cornerRadius int) {
	C.IupDrawSetClipRoundedRect(ih.ptr(), C.int(x1), C.int(y1), C.int(x2), C.int(y2), C.int(cornerRadius))
}

// DrawResetClip resets the clipping area to none.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawResetClip(ih Ihandle) {
	C.IupDrawResetClip(ih.ptr())
}

// DrawGetClipRect returns the previous rectangular clipping region set by DrawSetClipRect,
// if clipping was reset returns 0 in all values.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawGetClipRect(ih Ihandle) (x1, y1, x2, y2 int) {
	var cX1, cY1, cX2, cY2 C.int
	C.IupDrawGetClipRect(ih.ptr(), &cX1, &cY1, &cX2, &cY2)
	x1, y1, x2, y2 = int(cX1), int(cY1), int(cX2), int(cY2)
	return
}

// DrawParentBackground fills the canvas with the native parent background color.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawParentBackground(ih Ihandle) {
	C.IupDrawParentBackground(ih.ptr())
}

// DrawLine draws a line including start and end points.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawLine(ih Ihandle, x1, y1, x2, y2 int) {
	C.IupDrawLine(ih.ptr(), C.int(x1), C.int(y1), C.int(x2), C.int(y2))
}

// DrawRectangle draws a rectangle including start and end points.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawRectangle(ih Ihandle, x1, y1, x2, y2 int) {
	C.IupDrawRectangle(ih.ptr(), C.int(x1), C.int(y1), C.int(x2), C.int(y2))
}

// DrawArc draws an arc inside a rectangle between the two angles in degrees.
// When filled will draw a pie shape with the vertex at the center of the rectangle.
// Angles are counter-clock wise relative to the 3 o'clock position.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawArc(ih Ihandle, x1, y1, x2, y2 int, a1, a2 float64) {
	C.IupDrawArc(ih.ptr(), C.int(x1), C.int(y1), C.int(x2), C.int(y2), C.double(a1), C.double(a2))
}

// DrawEllipse draws an ellipse inscribed in the rectangle (x1,y1)-(x2,y2).
// The ellipse is controlled by DRAWCOLOR, DRAWSTYLE, and DRAWLINEWIDTH attributes.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawEllipse(ih Ihandle, x1, y1, x2, y2 int) {
	C.IupDrawEllipse(ih.ptr(), C.int(x1), C.int(y1), C.int(x2), C.int(y2))
}

// DrawPolygon draws a polygon.
// Coordinates are stored in the array in the sequence: x1, y1, x2, y2, ...
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawPolygon(ih Ihandle, points []int, count int) {
	cPoints := make([]C.int, len(points))
	for i, v := range points {
		cPoints[i] = C.int(v)
	}
	C.IupDrawPolygon(ih.ptr(), &cPoints[0], C.int(count))
}

// DrawPixel draws a single pixel at the given position.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawPixel(ih Ihandle, x, y int) {
	C.IupDrawPixel(ih.ptr(), C.int(x), C.int(y))
}

// DrawRoundedRectangle draws a rectangle with rounded corners.
// The corner_radius parameter defines the radius of the corner arcs.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawRoundedRectangle(ih Ihandle, x1, y1, x2, y2, corner_radius int) {
	C.IupDrawRoundedRectangle(ih.ptr(), C.int(x1), C.int(y1), C.int(x2), C.int(y2), C.int(corner_radius))
}

// DrawText draws a text in the given position using the font defined by DRAWFONT, if not defined then use FONT.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawText(ih Ihandle, str string, x, y, w, h int) {
	cStr := C.CString(str)
	defer C.free(unsafe.Pointer(cStr))

	C.IupDrawText(ih.ptr(), cStr, C.int(len(str)), C.int(x), C.int(y), C.int(w), C.int(h))
}

// DrawImage draws an image given its name.
// The coordinates are relative the top-left corner of the image.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawImage(ih Ihandle, name string, x, y, w, h int) {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	C.IupDrawImage(ih.ptr(), cName, C.int(x), C.int(y), C.int(w), C.int(h))
}

// DrawSelectRect draws a selection rectangle.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawSelectRect(ih Ihandle, x1, y1, x2, y2 int) {
	C.IupDrawSelectRect(ih.ptr(), C.int(x1), C.int(y1), C.int(x2), C.int(y2))
}

// DrawFocusRect draws a focus rectangle.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawFocusRect(ih Ihandle, x1, y1, x2, y2 int) {
	C.IupDrawFocusRect(ih.ptr(), C.int(x1), C.int(y1), C.int(x2), C.int(y2))
}

// DrawBezier draws a cubic Bezier curve.
// (x1,y1) = start point, (x2,y2) = first control point,
// (x3,y3) = second control point, (x4,y4) = end point.
//
// The curve is controlled by DRAWCOLOR, DRAWSTYLE, and DRAWLINEWIDTH attributes.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawBezier(ih Ihandle, x1, y1, x2, y2, x3, y3, x4, y4 int) {
	C.IupDrawBezier(ih.ptr(), C.int(x1), C.int(y1), C.int(x2), C.int(y2),
		C.int(x3), C.int(y3), C.int(x4), C.int(y4))
}

// DrawQuadraticBezier draws a quadratic Bezier curve.
// (x1,y1) = start point, (x2,y2) = control point, (x3,y3) = end point.
//
// The curve is controlled by DRAWCOLOR, DRAWSTYLE, and DRAWLINEWIDTH attributes.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawQuadraticBezier(ih Ihandle, x1, y1, x2, y2, x3, y3 int) {
	C.IupDrawQuadraticBezier(ih.ptr(), C.int(x1), C.int(y1), C.int(x2), C.int(y2),
		C.int(x3), C.int(y3))
}

// DrawGetSize returns the drawing area size.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawGetSize(ih Ihandle) (w, h int) {
	var cW, cH C.int
	C.IupDrawGetSize(ih.ptr(), &cW, &cH)
	w, h = int(cW), int(cH)
	return
}

// DrawGetTextSize returns the given text size using the font defined by DRAWFONT, if not defined then use FONT.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
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
// https://www.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawGetImageInfo(name string) (w, h, bpp int) {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	var cW, cH, cBpp C.int
	C.IupDrawGetImageInfo(cName, &cW, &cH, &cBpp)
	w, h, bpp = int(cW), int(cH), int(cBpp)
	return
}

// DrawLinearGradient draws a linear gradient between two colors.
// angle: 0=horizontal right, 90=vertical down, 180=horizontal left, 270=vertical up.
// color1 and color2 are color strings (e.g., "255 0 0" for red).
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawLinearGradient(ih Ihandle, x1, y1, x2, y2 int, angle float32, color1, color2 string) {
	cColor1 := C.CString(color1)
	defer C.free(unsafe.Pointer(cColor1))
	cColor2 := C.CString(color2)
	defer C.free(unsafe.Pointer(cColor2))

	C.IupDrawLinearGradient(ih.ptr(), C.int(x1), C.int(y1), C.int(x2), C.int(y2), C.float(angle), cColor1, cColor2)
}

// DrawRadialGradient draws a radial gradient from center to edge.
// colorCenter and colorEdge are color strings (e.g., "255 0 0" for red).
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupdraw.html
func DrawRadialGradient(ih Ihandle, cx, cy, radius int, colorCenter, colorEdge string) {
	cColorCenter := C.CString(colorCenter)
	defer C.free(unsafe.Pointer(cColorCenter))
	cColorEdge := C.CString(colorEdge)
	defer C.free(unsafe.Pointer(cColorEdge))

	C.IupDrawRadialGradient(ih.ptr(), C.int(cx), C.int(cy), C.int(radius), cColorCenter, cColorEdge)
}
