//go:build !cgo && !js

package iup

import (
	"runtime"
	"unsafe"
)

func DrawBegin(ih Ihandle) { iupDrawBegin(uintptr(ih)) }

func DrawEnd(ih Ihandle) { iupDrawEnd(uintptr(ih)) }

func DrawSave(ih Ihandle) { iupDrawSave(uintptr(ih)) }

func DrawRestore(ih Ihandle) { iupDrawRestore(uintptr(ih)) }

func DrawTransform(ih Ihandle, a, b, c, d, e, f float64) {
	iupDrawTransform(uintptr(ih), a, b, c, d, e, f)
}

func DrawSetTransform(ih Ihandle, a, b, c, d, e, f float64) {
	iupDrawSetTransform(uintptr(ih), a, b, c, d, e, f)
}

func DrawResetTransform(ih Ihandle) { iupDrawResetTransform(uintptr(ih)) }

func DrawGetTransform(ih Ihandle) (a, b, c, d, e, f float64) {
	iupDrawGetTransform(uintptr(ih), &a, &b, &c, &d, &e, &f)
	return
}

func DrawTranslate(ih Ihandle, tx, ty float64) { iupDrawTranslate(uintptr(ih), tx, ty) }

func DrawScale(ih Ihandle, sx, sy float64) { iupDrawScale(uintptr(ih), sx, sy) }

func DrawRotate(ih Ihandle, angle float64) { iupDrawRotate(uintptr(ih), angle) }

func DrawSetClipRect(ih Ihandle, x1, y1, x2, y2 int) {
	iupDrawSetClipRect(uintptr(ih), int32(x1), int32(y1), int32(x2), int32(y2))
}

func DrawSetClipRoundedRect(ih Ihandle, x1, y1, x2, y2, cornerRadius int) {
	iupDrawSetClipRoundedRect(uintptr(ih), int32(x1), int32(y1), int32(x2), int32(y2), int32(cornerRadius))
}

func DrawResetClip(ih Ihandle) { iupDrawResetClip(uintptr(ih)) }

func DrawGetClipRect(ih Ihandle) (x1, y1, x2, y2 int) {
	var a, b, c, d int32
	iupDrawGetClipRect(uintptr(ih), &a, &b, &c, &d)
	return int(a), int(b), int(c), int(d)
}

func DrawParentBackground(ih Ihandle) { iupDrawParentBackground(uintptr(ih)) }

func DrawLine(ih Ihandle, x1, y1, x2, y2 int) {
	iupDrawLine(uintptr(ih), int32(x1), int32(y1), int32(x2), int32(y2))
}

func DrawRectangle(ih Ihandle, x1, y1, x2, y2 int) {
	iupDrawRectangle(uintptr(ih), int32(x1), int32(y1), int32(x2), int32(y2))
}

func DrawArc(ih Ihandle, x1, y1, x2, y2 int, a1, a2 float64) {
	iupDrawArc(uintptr(ih), int32(x1), int32(y1), int32(x2), int32(y2), a1, a2)
}

func DrawEllipse(ih Ihandle, x1, y1, x2, y2 int) {
	iupDrawEllipse(uintptr(ih), int32(x1), int32(y1), int32(x2), int32(y2))
}

func DrawPolygon(ih Ihandle, points []int, count int) {
	cp := make([]int32, len(points))
	for i, v := range points {
		cp[i] = int32(v)
	}
	iupDrawPolygon(uintptr(ih), cp, int32(count))
}

func DrawLinearGradientStops(ih Ihandle, x1, y1, x2, y2 int, angle float32, colors []string, offsets []float32) {
	n := len(colors)
	if n < 2 || (len(offsets) != 0 && len(offsets) != n) {
		return
	}
	bufs := make([][]byte, n)
	pColors := make([]uintptr, n)
	for i, s := range colors {
		bufs[i] = append([]byte(s), 0)
		pColors[i] = uintptr(unsafe.Pointer(&bufs[i][0]))
	}
	var pOff *float32
	if len(offsets) >= n {
		pOff = &offsets[0]
	}
	iupDrawLinearGradientStops(uintptr(ih), int32(x1), int32(y1), int32(x2), int32(y2), angle, &pColors[0], pOff, int32(n))
	runtime.KeepAlive(bufs)
}

func DrawRadialGradientStops(ih Ihandle, cx, cy, radius int, colors []string, offsets []float32) {
	n := len(colors)
	if n < 2 || (len(offsets) != 0 && len(offsets) != n) {
		return
	}
	bufs := make([][]byte, n)
	pColors := make([]uintptr, n)
	for i, s := range colors {
		bufs[i] = append([]byte(s), 0)
		pColors[i] = uintptr(unsafe.Pointer(&bufs[i][0]))
	}
	var pOff *float32
	if len(offsets) >= n {
		pOff = &offsets[0]
	}
	iupDrawRadialGradientStops(uintptr(ih), int32(cx), int32(cy), int32(radius), &pColors[0], pOff, int32(n))
	runtime.KeepAlive(bufs)
}

func DrawPixel(ih Ihandle, x, y int) {
	iupDrawPixel(uintptr(ih), int32(x), int32(y))
}

func DrawRoundedRectangle(ih Ihandle, x1, y1, x2, y2, corner_radius int) {
	iupDrawRoundedRectangle(uintptr(ih), int32(x1), int32(y1), int32(x2), int32(y2), int32(corner_radius))
}

func DrawText(ih Ihandle, str string, x, y, w, h int) {
	iupDrawText(uintptr(ih), str, int32(len(str)), int32(x), int32(y), int32(w), int32(h))
}

func DrawImage(ih Ihandle, name string, x, y, w, h int) {
	iupDrawImage(uintptr(ih), name, int32(x), int32(y), int32(w), int32(h))
}

func DrawSelectRect(ih Ihandle, x1, y1, x2, y2 int) {
	iupDrawSelectRect(uintptr(ih), int32(x1), int32(y1), int32(x2), int32(y2))
}

func DrawFocusRect(ih Ihandle, x1, y1, x2, y2 int) {
	iupDrawFocusRect(uintptr(ih), int32(x1), int32(y1), int32(x2), int32(y2))
}

func DrawBezier(ih Ihandle, x1, y1, x2, y2, x3, y3, x4, y4 int) {
	iupDrawBezier(uintptr(ih), int32(x1), int32(y1), int32(x2), int32(y2), int32(x3), int32(y3), int32(x4), int32(y4))
}

func DrawQuadraticBezier(ih Ihandle, x1, y1, x2, y2, x3, y3 int) {
	iupDrawQuadraticBezier(uintptr(ih), int32(x1), int32(y1), int32(x2), int32(y2), int32(x3), int32(y3))
}

func DrawGetSize(ih Ihandle) (w, h int) {
	var cw, ch int32
	iupDrawGetSize(uintptr(ih), &cw, &ch)
	return int(cw), int(ch)
}

func DrawGetTextSize(ih Ihandle, str string) (w, h int) {
	var cw, ch int32
	iupDrawGetTextSize(uintptr(ih), str, int32(len(str)), &cw, &ch)
	return int(cw), int(ch)
}

func DrawGetTextMetrics(ih Ihandle) (ascent, descent, lineHeight int) {
	var ca, cd, cl int32
	iupDrawGetTextMetrics(uintptr(ih), &ca, &cd, &cl)
	return int(ca), int(cd), int(cl)
}

func DrawGetImageInfo(name string) (w, h, bpp int) {
	var cw, ch, cbpp int32
	iupDrawGetImageInfo(name, &cw, &ch, &cbpp)
	return int(cw), int(ch), int(cbpp)
}

func DrawGetImage(ih Ihandle) Ihandle {
	return mkih(iupDrawGetImage(uintptr(ih)))
}

func DrawGetSvg(ih Ihandle) string {
	return iupDrawGetSvg(uintptr(ih))
}

func DrawLinearGradient(ih Ihandle, x1, y1, x2, y2 int, angle float32, color1, color2 string) {
	iupDrawLinearGradient(uintptr(ih), int32(x1), int32(y1), int32(x2), int32(y2), angle, color1, color2)
}

func DrawRadialGradient(ih Ihandle, cx, cy, radius int, colorCenter, colorEdge string) {
	iupDrawRadialGradient(uintptr(ih), int32(cx), int32(cy), int32(radius), colorCenter, colorEdge)
}

func DrawPathBegin(ih Ihandle) { iupDrawPathBegin(uintptr(ih)) }

func DrawPathMoveTo(ih Ihandle, x, y int) {
	iupDrawPathMoveTo(uintptr(ih), int32(x), int32(y))
}

func DrawPathLineTo(ih Ihandle, x, y int) {
	iupDrawPathLineTo(uintptr(ih), int32(x), int32(y))
}

func DrawPathCurveTo(ih Ihandle, x1, y1, x2, y2, x3, y3 int) {
	iupDrawPathCurveTo(uintptr(ih), int32(x1), int32(y1), int32(x2), int32(y2), int32(x3), int32(y3))
}

func DrawPathQuadTo(ih Ihandle, x1, y1, x2, y2 int) {
	iupDrawPathQuadTo(uintptr(ih), int32(x1), int32(y1), int32(x2), int32(y2))
}

func DrawPathArcTo(ih Ihandle, cx, cy, rx, ry int, a1, a2 float64) {
	iupDrawPathArcTo(uintptr(ih), int32(cx), int32(cy), int32(rx), int32(ry), a1, a2)
}

func DrawPathClose(ih Ihandle) { iupDrawPathClose(uintptr(ih)) }

func DrawPathMoveToF(ih Ihandle, x, y float64) {
	iupDrawPathMoveToF(uintptr(ih), x, y)
}

func DrawPathLineToF(ih Ihandle, x, y float64) {
	iupDrawPathLineToF(uintptr(ih), x, y)
}

func DrawPathCurveToF(ih Ihandle, x1, y1, x2, y2, x3, y3 float64) {
	iupDrawPathCurveToF(uintptr(ih), x1, y1, x2, y2, x3, y3)
}

func DrawPathQuadToF(ih Ihandle, x1, y1, x2, y2 float64) {
	iupDrawPathQuadToF(uintptr(ih), x1, y1, x2, y2)
}

func DrawPathArcToF(ih Ihandle, cx, cy, rx, ry, a1, a2 float64) {
	iupDrawPathArcToF(uintptr(ih), cx, cy, rx, ry, a1, a2)
}

func DrawPathCreate() Ihandle {
	return mkih(iupDrawPathCreate())
}

func DrawPathClear(path Ihandle) {
	iupDrawPathClear(uintptr(path))
}

func DrawSetPath(ih Ihandle, path Ihandle) {
	iupDrawSetPath(uintptr(ih), uintptr(path))
}

func DrawPathGetBounds(path Ihandle) (x1, y1, x2, y2 int) {
	var a, b, c, d int32
	iupDrawPathGetBounds(uintptr(path), &a, &b, &c, &d)
	return int(a), int(b), int(c), int(d)
}

func DrawPathContains(path Ihandle, x, y, rule int) bool {
	return iupDrawPathContains(uintptr(path), int32(x), int32(y), int32(rule)) != 0
}

func DrawPathSetSvg(path Ihandle, data string) bool {
	return iupDrawPathSetSvg(uintptr(path), data) != 0
}

func DrawPathFill(ih Ihandle, rule int) {
	iupDrawPathFill(uintptr(ih), int32(rule))
}

func DrawPathStroke(ih Ihandle) { iupDrawPathStroke(uintptr(ih)) }

func DrawSetClipPath(ih Ihandle, rule int) {
	iupDrawSetClipPath(uintptr(ih), int32(rule))
}

func DrawSetSourceSolid(ih Ihandle, color string) {
	iupDrawSetSourceSolid(uintptr(ih), color)
}

func DrawSetSourceLinearGradient(ih Ihandle, x1, y1, x2, y2 int, angle float32, colors []string, offsets []float32) {
	n := len(colors)
	if n < 2 || (len(offsets) != 0 && len(offsets) != n) {
		return
	}
	bufs := make([][]byte, n)
	pColors := make([]uintptr, n)
	for i, s := range colors {
		bufs[i] = append([]byte(s), 0)
		pColors[i] = uintptr(unsafe.Pointer(&bufs[i][0]))
	}
	var pOff *float32
	if len(offsets) >= n {
		pOff = &offsets[0]
	}
	iupDrawSetSourceLinearGradient(uintptr(ih), int32(x1), int32(y1), int32(x2), int32(y2), angle, &pColors[0], pOff, int32(n))
	runtime.KeepAlive(bufs)
}

func DrawSetSourceRadialGradient(ih Ihandle, cx, cy, radius int, colors []string, offsets []float32) {
	n := len(colors)
	if n < 2 || (len(offsets) != 0 && len(offsets) != n) {
		return
	}
	bufs := make([][]byte, n)
	pColors := make([]uintptr, n)
	for i, s := range colors {
		bufs[i] = append([]byte(s), 0)
		pColors[i] = uintptr(unsafe.Pointer(&bufs[i][0]))
	}
	var pOff *float32
	if len(offsets) >= n {
		pOff = &offsets[0]
	}
	iupDrawSetSourceRadialGradient(uintptr(ih), int32(cx), int32(cy), int32(radius), &pColors[0], pOff, int32(n))
	runtime.KeepAlive(bufs)
}

func DrawResetSource(ih Ihandle) { iupDrawResetSource(uintptr(ih)) }
