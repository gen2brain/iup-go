//go:build !cgo && !js

package iup

import "unsafe"

func DrawBegin(ih Ihandle) { iupDrawBegin(uintptr(ih)) }

func DrawEnd(ih Ihandle) { iupDrawEnd(uintptr(ih)) }

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
	if n < 2 {
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
}

func DrawRadialGradientStops(ih Ihandle, cx, cy, radius int, colors []string, offsets []float32) {
	n := len(colors)
	if n < 2 {
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
