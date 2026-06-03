//go:build js && wasm

package iup

func Append(ih, child Ihandle) Ihandle {
	return ccallHandle("IupAppend", []interface{}{"number", "number"}, []interface{}{int(ih), int(child)})
}

func Detach(child Ihandle) {
	ccall("IupDetach", "", []interface{}{"number"}, []interface{}{int(child)})
}

func GetBrother(ih Ihandle) Ihandle {
	return ccallHandle("IupGetBrother", []interface{}{"number"}, []interface{}{int(ih)})
}

func GetChildCount(ih Ihandle) int {
	return ccall("IupGetChildCount", "number", []interface{}{"number"}, []interface{}{int(ih)}).Int()
}

func GetDialog(ih Ihandle) Ihandle {
	return ccallHandle("IupGetDialog", []interface{}{"number"}, []interface{}{int(ih)})
}

func GetDialogChild(ih Ihandle, name string) Ihandle {
	return ccallHandle("IupGetDialogChild", []interface{}{"number", "string"}, []interface{}{int(ih), name})
}

func GetNextChild(ih, child Ihandle) Ihandle {
	return ccallHandle("IupGetNextChild", []interface{}{"number", "number"}, []interface{}{int(ih), int(child)})
}

func Insert(ih, refChild, child Ihandle) Ihandle {
	return ccallHandle("IupInsert", []interface{}{"number", "number", "number"}, []interface{}{int(ih), int(refChild), int(child)})
}

func Reparent(ih, newParent, refChild Ihandle) int {
	return ccall("IupReparent", "number", []interface{}{"number", "number", "number"}, []interface{}{int(ih), int(newParent), int(refChild)}).Int()
}
