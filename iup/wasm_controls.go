//go:build js && wasm

package iup

func AnimatedLabel(animation Ihandle) Ihandle {
	return ccallHandle("IupAnimatedLabel", []interface{}{"number"}, []interface{}{int(animation)})
}

func ColorBar() Ihandle {
	return ccallHandle("IupColorbar", nil, nil)
}

func DatePick() Ihandle {
	return ccallHandle("IupDatePick", nil, nil)
}

func Dial(orientation string) Ihandle {
	return ccallHandle("IupDial", []interface{}{"string"}, []interface{}{orientation})
}

func Link(url, title string) Ihandle {
	return ccallHandle("IupLink", []interface{}{"string", "string"}, []interface{}{url, title})
}

func Separator() Ihandle {
	return ccallHandle("IupSeparator", nil, nil)
}

func Spin() Ihandle {
	return ccallHandle("IupSpin", nil, nil)
}

func SpinBox(child Ihandle) Ihandle {
	return ccallHandle("IupSpinbox", []interface{}{"number"}, []interface{}{int(child)})
}

func TextConvertLinColToPos(ih Ihandle, lin, col int) (pos int) {
	p := wasmMalloc(4)
	defer wasmFree(p)
	ccall("IupTextConvertLinColToPos", "", []interface{}{"number", "number", "number", "number"}, []interface{}{int(ih), lin, col, p})
	return wasmGetI32(p)
}

func TextConvertPosToLinCol(ih Ihandle, pos int) (lin, col int) {
	pl, pc := wasmMalloc(4), wasmMalloc(4)
	defer wasmFree(pl)
	defer wasmFree(pc)
	ccall("IupTextConvertPosToLinCol", "", []interface{}{"number", "number", "number", "number"}, []interface{}{int(ih), pos, pl, pc})
	return wasmGetI32(pl), wasmGetI32(pc)
}

func TreeGetId(ih Ihandle, userid uintptr) int {
	return ccall("IupTreeGetId", "number", []interface{}{"number", "number"}, []interface{}{int(ih), int(userid)}).Int()
}

func TreeGetUserId(ih Ihandle, id int) uintptr {
	return uintptr(ccall("IupTreeGetUserId", "number", []interface{}{"number", "number"}, []interface{}{int(ih), id}).Int())
}

func TreeSetAttributeHandle(ih Ihandle, name string, id int, ihNamed Ihandle) {
	ccall("IupTreeSetAttributeHandle", "", []interface{}{"number", "string", "number", "number"}, []interface{}{int(ih), name, id, int(ihNamed)})
}

func TreeSetUserId(ih Ihandle, id int, userid uintptr) int {
	return ccall("IupTreeSetUserId", "number", []interface{}{"number", "number", "number"}, []interface{}{int(ih), id, int(userid)}).Int()
}
