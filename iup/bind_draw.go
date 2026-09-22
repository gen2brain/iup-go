//go:build !js

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
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawBegin(ih Ihandle) {
	C.IupDrawBegin(ih.ptr())
}

// DrawEnd terminates the drawing process and actually draw on screen..
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawEnd(ih Ihandle) {
	C.IupDrawEnd(ih.ptr())
}

// DrawSave saves the current drawing state.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawSave(ih Ihandle) { C.IupDrawSave(ih.ptr()) }

// DrawRestore restores the most recently saved drawing state.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawRestore(ih Ihandle) { C.IupDrawRestore(ih.ptr()) }

// DrawTransform multiplies the current drawing transform by the given matrix.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawTransform(ih Ihandle, a, b, c, d, e, f float64) {
	C.IupDrawTransform(ih.ptr(), C.double(a), C.double(b), C.double(c), C.double(d), C.double(e), C.double(f))
}

// DrawSetTransform replaces the current drawing transform.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawSetTransform(ih Ihandle, a, b, c, d, e, f float64) {
	C.IupDrawSetTransform(ih.ptr(), C.double(a), C.double(b), C.double(c), C.double(d), C.double(e), C.double(f))
}

// DrawResetTransform resets the current drawing transform to identity.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawResetTransform(ih Ihandle) { C.IupDrawResetTransform(ih.ptr()) }

// DrawGetTransform returns the current drawing transform.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawGetTransform(ih Ihandle) (a, b, c, d, e, f float64) {
	var ca, cb, cc, cd, ce, cf C.double
	C.IupDrawGetTransform(ih.ptr(), &ca, &cb, &cc, &cd, &ce, &cf)
	return float64(ca), float64(cb), float64(cc), float64(cd), float64(ce), float64(cf)
}

// DrawTranslate translates the current drawing transform.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawTranslate(ih Ihandle, tx, ty float64) {
	C.IupDrawTranslate(ih.ptr(), C.double(tx), C.double(ty))
}

// DrawScale scales the current drawing transform.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawScale(ih Ihandle, sx, sy float64) {
	C.IupDrawScale(ih.ptr(), C.double(sx), C.double(sy))
}

// DrawRotate rotates the current drawing transform counterclockwise in degrees.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawRotate(ih Ihandle, angle float64) { C.IupDrawRotate(ih.ptr(), C.double(angle)) }

// DrawSetClipRect defines a rectangular clipping region.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawSetClipRect(ih Ihandle, x1, y1, x2, y2 int) {
	C.IupDrawSetClipRect(ih.ptr(), C.int(x1), C.int(y1), C.int(x2), C.int(y2))
}

// DrawSetClipRoundedRect defines a rounded rectangular clipping region.
// This is useful for drawing gradients or other content with rounded corners.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawSetClipRoundedRect(ih Ihandle, x1, y1, x2, y2, cornerRadius int) {
	C.IupDrawSetClipRoundedRect(ih.ptr(), C.int(x1), C.int(y1), C.int(x2), C.int(y2), C.int(cornerRadius))
}

// DrawResetClip resets the clipping area to none.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawResetClip(ih Ihandle) {
	C.IupDrawResetClip(ih.ptr())
}

// DrawGetClipRect returns the previous rectangular clipping region set by DrawSetClipRect,
// if clipping was reset returns 0 in all values.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawGetClipRect(ih Ihandle) (x1, y1, x2, y2 int) {
	var cX1, cY1, cX2, cY2 C.int
	C.IupDrawGetClipRect(ih.ptr(), &cX1, &cY1, &cX2, &cY2)
	x1, y1, x2, y2 = int(cX1), int(cY1), int(cX2), int(cY2)
	return
}

// DrawParentBackground fills the canvas with the native parent background color.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawParentBackground(ih Ihandle) {
	C.IupDrawParentBackground(ih.ptr())
}

// DrawLine draws a line including start and end points.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawLine(ih Ihandle, x1, y1, x2, y2 int) {
	C.IupDrawLine(ih.ptr(), C.int(x1), C.int(y1), C.int(x2), C.int(y2))
}

// DrawRectangle draws a rectangle including start and end points.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawRectangle(ih Ihandle, x1, y1, x2, y2 int) {
	C.IupDrawRectangle(ih.ptr(), C.int(x1), C.int(y1), C.int(x2), C.int(y2))
}

// DrawArc draws an arc inside a rectangle between the two angles in degrees.
// When filled will draw a pie shape with the vertex at the center of the rectangle.
// Angles are counter-clock wise relative to the 3 o'clock position.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawArc(ih Ihandle, x1, y1, x2, y2 int, a1, a2 float64) {
	C.IupDrawArc(ih.ptr(), C.int(x1), C.int(y1), C.int(x2), C.int(y2), C.double(a1), C.double(a2))
}

// DrawEllipse draws an ellipse inscribed in the rectangle (x1,y1)-(x2,y2).
// The ellipse is controlled by DRAWCOLOR, DRAWSTYLE, and DRAWLINEWIDTH attributes.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawEllipse(ih Ihandle, x1, y1, x2, y2 int) {
	C.IupDrawEllipse(ih.ptr(), C.int(x1), C.int(y1), C.int(x2), C.int(y2))
}

// DrawPolygon draws a polygon.
// Coordinates are stored in the array in the sequence: x1, y1, x2, y2, ...
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawPolygon(ih Ihandle, points []int, count int) {
	cPoints := make([]C.int, len(points))
	for i, v := range points {
		cPoints[i] = C.int(v)
	}
	C.IupDrawPolygon(ih.ptr(), &cPoints[0], C.int(count))
}

// DrawPixel draws a single pixel at the given position.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawPixel(ih Ihandle, x, y int) {
	C.IupDrawPixel(ih.ptr(), C.int(x), C.int(y))
}

// DrawRoundedRectangle draws a rectangle with rounded corners.
// The corner_radius parameter defines the radius of the corner arcs.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawRoundedRectangle(ih Ihandle, x1, y1, x2, y2, corner_radius int) {
	C.IupDrawRoundedRectangle(ih.ptr(), C.int(x1), C.int(y1), C.int(x2), C.int(y2), C.int(corner_radius))
}

// DrawText draws a text in the given position using the font defined by DRAWFONT, if not defined then use FONT.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawText(ih Ihandle, str string, x, y, w, h int) {
	cStr := C.CString(str)
	defer C.free(unsafe.Pointer(cStr))

	C.IupDrawText(ih.ptr(), cStr, C.int(len(str)), C.int(x), C.int(y), C.int(w), C.int(h))
}

// DrawImage draws an image given its name.
// The coordinates are relative the top-left corner of the image.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawImage(ih Ihandle, name string, x, y, w, h int) {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	C.IupDrawImage(ih.ptr(), cName, C.int(x), C.int(y), C.int(w), C.int(h))
}

// DrawSelectRect draws a selection rectangle.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawSelectRect(ih Ihandle, x1, y1, x2, y2 int) {
	C.IupDrawSelectRect(ih.ptr(), C.int(x1), C.int(y1), C.int(x2), C.int(y2))
}

// DrawFocusRect draws a focus rectangle.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawFocusRect(ih Ihandle, x1, y1, x2, y2 int) {
	C.IupDrawFocusRect(ih.ptr(), C.int(x1), C.int(y1), C.int(x2), C.int(y2))
}

// DrawBezier draws a cubic Bezier curve.
// (x1,y1) = start point, (x2,y2) = first control point,
// (x3,y3) = second control point, (x4,y4) = end point.
//
// The curve is controlled by DRAWCOLOR, DRAWSTYLE, and DRAWLINEWIDTH attributes.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawBezier(ih Ihandle, x1, y1, x2, y2, x3, y3, x4, y4 int) {
	C.IupDrawBezier(ih.ptr(), C.int(x1), C.int(y1), C.int(x2), C.int(y2),
		C.int(x3), C.int(y3), C.int(x4), C.int(y4))
}

// DrawQuadraticBezier draws a quadratic Bezier curve.
// (x1,y1) = start point, (x2,y2) = control point, (x3,y3) = end point.
//
// The curve is controlled by DRAWCOLOR, DRAWSTYLE, and DRAWLINEWIDTH attributes.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawQuadraticBezier(ih Ihandle, x1, y1, x2, y2, x3, y3 int) {
	C.IupDrawQuadraticBezier(ih.ptr(), C.int(x1), C.int(y1), C.int(x2), C.int(y2),
		C.int(x3), C.int(y3))
}

// DrawGetSize returns the drawing area size.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawGetSize(ih Ihandle) (w, h int) {
	var cW, cH C.int
	C.IupDrawGetSize(ih.ptr(), &cW, &cH)
	w, h = int(cW), int(cH)
	return
}

// DrawGetTextSize returns the given text size using the font defined by DRAWFONT, if not defined then use FONT.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawGetTextSize(ih Ihandle, str string) (w, h int) {
	cStr := C.CString(str)
	defer C.free(unsafe.Pointer(cStr))

	var cW, cH C.int
	C.IupDrawGetTextSize(ih.ptr(), cStr, C.int(len(str)), &cW, &cH)
	w, h = int(cW), int(cH)
	return
}

// DrawGetTextMetrics returns the font metrics for the font defined by DRAWFONT, if not defined then use FONT.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawGetTextMetrics(ih Ihandle) (ascent, descent, lineHeight int) {
	var cA, cD, cL C.int
	C.IupDrawGetTextMetrics(ih.ptr(), &cA, &cD, &cL)
	ascent, descent, lineHeight = int(cA), int(cD), int(cL)
	return
}

// DrawGetImageInfo returns the given image size and bits per pixel.
// bpp can be 8, 24 or 32.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawGetImageInfo(name string) (w, h, bpp int) {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	var cW, cH, cBpp C.int
	C.IupDrawGetImageInfo(cName, &cW, &cH, &cBpp)
	w, h, bpp = int(cW), int(cH), int(cBpp)
	return
}

// DrawGetImage returns an IupImageRGBA containing the current contents of the
// offscreen drawing buffer. Must be called between DrawBegin and DrawEnd.
// Returns 0 (nil handle) if the operation fails.
// The caller is responsible for destroying the returned image handle.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawGetImage(ih Ihandle) Ihandle {
	return mkih(C.IupDrawGetImage(ih.ptr()))
}

// DrawGetSvg repaints the control and returns the drawing as an SVG document, or an empty string on failure.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawGetSvg(ih Ihandle) string {
	cStr := C.IupDrawGetSvg(ih.ptr())
	if cStr == nil {
		return ""
	}
	defer C.free(unsafe.Pointer(cStr))
	return C.GoString(cStr)
}

// DrawLinearGradient draws a linear gradient between two colors.
// angle: 0=horizontal right, 90=vertical down, 180=horizontal left, 270=vertical up.
// color1 and color2 are color strings (e.g., "255 0 0" for red).
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawLinearGradient(ih Ihandle, x1, y1, x2, y2 int, angle float32, color1, color2 string) {
	cColor1 := C.CString(color1)
	defer C.free(unsafe.Pointer(cColor1))
	cColor2 := C.CString(color2)
	defer C.free(unsafe.Pointer(cColor2))

	C.IupDrawLinearGradient(ih.ptr(), C.int(x1), C.int(y1), C.int(x2), C.int(y2), C.float(angle), cColor1, cColor2)
}

// DrawLinearGradientStops draws a linear gradient across count color stops.
// offsets are in the 0-1 range, ascending; if nil the stops are evenly spaced.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawLinearGradientStops(ih Ihandle, x1, y1, x2, y2 int, angle float32, colors []string, offsets []float32) {
	n := len(colors)
	if n < 2 || (len(offsets) != 0 && len(offsets) != n) {
		return
	}
	cColors := make([]*C.char, n)
	for i, c := range colors {
		cColors[i] = C.CString(c)
	}
	defer func() {
		for _, p := range cColors {
			C.free(unsafe.Pointer(p))
		}
	}()

	var cOffsets *C.float
	if len(offsets) >= n {
		cOffsets = (*C.float)(unsafe.Pointer(&offsets[0]))
	}

	C.IupDrawLinearGradientStops(ih.ptr(), C.int(x1), C.int(y1), C.int(x2), C.int(y2), C.float(angle), (**C.char)(unsafe.Pointer(&cColors[0])), cOffsets, C.int(n))
}

// DrawRadialGradientStops draws a radial gradient across count color stops.
// offsets are in the 0-1 range, ascending; if nil the stops are evenly spaced.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawRadialGradientStops(ih Ihandle, cx, cy, radius int, colors []string, offsets []float32) {
	n := len(colors)
	if n < 2 || (len(offsets) != 0 && len(offsets) != n) {
		return
	}
	cColors := make([]*C.char, n)
	for i, c := range colors {
		cColors[i] = C.CString(c)
	}
	defer func() {
		for _, p := range cColors {
			C.free(unsafe.Pointer(p))
		}
	}()

	var cOffsets *C.float
	if len(offsets) >= n {
		cOffsets = (*C.float)(unsafe.Pointer(&offsets[0]))
	}

	C.IupDrawRadialGradientStops(ih.ptr(), C.int(cx), C.int(cy), C.int(radius), (**C.char)(unsafe.Pointer(&cColors[0])), cOffsets, C.int(n))
}

// DrawRadialGradient draws a radial gradient from center to edge.
// colorCenter and colorEdge are color strings (e.g., "255 0 0" for red).
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawRadialGradient(ih Ihandle, cx, cy, radius int, colorCenter, colorEdge string) {
	cColorCenter := C.CString(colorCenter)
	defer C.free(unsafe.Pointer(cColorCenter))
	cColorEdge := C.CString(colorEdge)
	defer C.free(unsafe.Pointer(cColorEdge))

	C.IupDrawRadialGradient(ih.ptr(), C.int(cx), C.int(cy), C.int(radius), cColorCenter, cColorEdge)
}

// DrawPathBegin resets the current path.
// The path is kept until the next DrawPathBegin or DrawEnd.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawPathBegin(ih Ihandle) {
	C.IupDrawPathBegin(ih.ptr())
}

// DrawPathMoveTo starts a new subpath at the given position.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawPathMoveTo(ih Ihandle, x, y int) {
	C.IupDrawPathMoveTo(ih.ptr(), C.int(x), C.int(y))
}

// DrawPathLineTo adds a line to the given position.
// With no current point it starts a new subpath there.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawPathLineTo(ih Ihandle, x, y int) {
	C.IupDrawPathLineTo(ih.ptr(), C.int(x), C.int(y))
}

// DrawPathCurveTo adds a cubic Bezier curve with two control points.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawPathCurveTo(ih Ihandle, x1, y1, x2, y2, x3, y3 int) {
	C.IupDrawPathCurveTo(ih.ptr(), C.int(x1), C.int(y1), C.int(x2), C.int(y2), C.int(x3), C.int(y3))
}

// DrawPathQuadTo adds a quadratic Bezier curve with one control point.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawPathQuadTo(ih Ihandle, x1, y1, x2, y2 int) {
	C.IupDrawPathQuadTo(ih.ptr(), C.int(x1), C.int(y1), C.int(x2), C.int(y2))
}

// DrawPathArcTo adds an arc of the ellipse centered at (cx,cy) with radii
// rx,ry, from a1 to a2 degrees counter-clockwise, connecting the current point
// to the arc start with a line.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawPathArcTo(ih Ihandle, cx, cy, rx, ry int, a1, a2 float64) {
	C.IupDrawPathArcTo(ih.ptr(), C.int(cx), C.int(cy), C.int(rx), C.int(ry), C.double(a1), C.double(a2))
}

// DrawPathClose closes the current subpath with a line to its start.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawPathClose(ih Ihandle) {
	C.IupDrawPathClose(ih.ptr())
}

// DrawPathFill fills the current path with the current source.
// rule is DRAW_RULE_WINDING or DRAW_RULE_EVENODD.
// Open subpaths are closed before filling.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawPathFill(ih Ihandle, rule int) {
	C.IupDrawPathFill(ih.ptr(), C.int(rule))
}

// DrawPathStroke strokes the current path with the current source,
// controlled by the DRAWSTYLE and DRAWLINEWIDTH attributes.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawPathStroke(ih Ihandle) {
	C.IupDrawPathStroke(ih.ptr())
}

// DrawSetClipPath sets the current path as the clipping area, replacing the
// previous clip. rule is DRAW_RULE_WINDING or DRAW_RULE_EVENODD.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawSetClipPath(ih Ihandle, rule int) {
	C.IupDrawSetClipPath(ih.ptr(), C.int(rule))
}

// DrawSetSourceSolid sets a solid color as the current source, replacing any
// previous source. Same as setting the DRAWCOLOR attribute.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawSetSourceSolid(ih Ihandle, color string) {
	cColor := C.CString(color)
	defer C.free(unsafe.Pointer(cColor))

	C.IupDrawSetSourceSolid(ih.ptr(), cColor)
}

// DrawSetSourceLinearGradient sets a linear gradient as the current source.
// The source is used by DrawPathFill and DrawPathStroke until reset.
// offsets are in the 0-1 range, ascending; if nil the stops are evenly spaced.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawSetSourceLinearGradient(ih Ihandle, x1, y1, x2, y2 int, angle float32, colors []string, offsets []float32) {
	n := len(colors)
	if n < 2 || (len(offsets) != 0 && len(offsets) != n) {
		return
	}
	cColors := make([]*C.char, n)
	for i, c := range colors {
		cColors[i] = C.CString(c)
	}
	defer func() {
		for _, p := range cColors {
			C.free(unsafe.Pointer(p))
		}
	}()

	var cOffsets *C.float
	if len(offsets) >= n {
		cOffsets = (*C.float)(unsafe.Pointer(&offsets[0]))
	}

	C.IupDrawSetSourceLinearGradient(ih.ptr(), C.int(x1), C.int(y1), C.int(x2), C.int(y2), C.float(angle), (**C.char)(unsafe.Pointer(&cColors[0])), cOffsets, C.int(n))
}

// DrawSetSourceRadialGradient sets a radial gradient as the current source.
// The source is used by DrawPathFill and DrawPathStroke until reset.
// offsets are in the 0-1 range, ascending; if nil the stops are evenly spaced.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawSetSourceRadialGradient(ih Ihandle, cx, cy, radius int, colors []string, offsets []float32) {
	n := len(colors)
	if n < 2 || (len(offsets) != 0 && len(offsets) != n) {
		return
	}
	cColors := make([]*C.char, n)
	for i, c := range colors {
		cColors[i] = C.CString(c)
	}
	defer func() {
		for _, p := range cColors {
			C.free(unsafe.Pointer(p))
		}
	}()

	var cOffsets *C.float
	if len(offsets) >= n {
		cOffsets = (*C.float)(unsafe.Pointer(&offsets[0]))
	}

	C.IupDrawSetSourceRadialGradient(ih.ptr(), C.int(cx), C.int(cy), C.int(radius), (**C.char)(unsafe.Pointer(&cColors[0])), cOffsets, C.int(n))
}

// DrawResetSource resets the current source back to the DRAWCOLOR attribute.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_draw.md
func DrawResetSource(ih Ihandle) {
	C.IupDrawResetSource(ih.ptr())
}
