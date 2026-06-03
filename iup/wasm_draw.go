//go:build js && wasm

package iup

func DrawGetClipRect(ih Ihandle) (x1, y1, x2, y2 int) {
	a, b, c, d := wasmMalloc(4), wasmMalloc(4), wasmMalloc(4), wasmMalloc(4)
	defer wasmFree(a)
	defer wasmFree(b)
	defer wasmFree(c)
	defer wasmFree(d)
	ccall("IupDrawGetClipRect", "", []interface{}{"number", "number", "number", "number", "number"}, []interface{}{int(ih), a, b, c, d})
	return wasmGetI32(a), wasmGetI32(b), wasmGetI32(c), wasmGetI32(d)
}

func DrawGetImage(ih Ihandle) Ihandle {
	return ccallHandle("IupDrawGetImage", []interface{}{"number"}, []interface{}{int(ih)})
}

func DrawGetImageInfo(name string) (w, h, bpp int) {
	pw, ph, pb := wasmMalloc(4), wasmMalloc(4), wasmMalloc(4)
	defer wasmFree(pw)
	defer wasmFree(ph)
	defer wasmFree(pb)
	ccall("IupDrawGetImageInfo", "", []interface{}{"string", "number", "number", "number"}, []interface{}{name, pw, ph, pb})
	return wasmGetI32(pw), wasmGetI32(ph), wasmGetI32(pb)
}

func DrawGetSvg(ih Ihandle) string {
	return ccall("IupDrawGetSvg", "string", []interface{}{"number"}, []interface{}{int(ih)}).String()
}
