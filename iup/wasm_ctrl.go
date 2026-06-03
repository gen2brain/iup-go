//go:build js && wasm && ctrl

package iup

func Cells() Ihandle {
	return ccallHandle("IupCells", nil, nil)
}

func ControlsOpen() int {
	return ccall("IupControlsOpen", "number", nil, nil).Int()
}

func DropButton(dropChild Ihandle) Ihandle {
	return ccallHandle("IupDropButton", []interface{}{"number"}, []interface{}{int(dropChild)})
}

func FlatButton(title string) Ihandle {
	return ccallHandle("IupFlatButton", []interface{}{"string"}, []interface{}{title})
}

func FlatFrame(child Ihandle) Ihandle {
	return ccallHandle("IupFlatFrame", []interface{}{"number"}, []interface{}{int(child)})
}

func FlatLabel(title string) Ihandle {
	return ccallHandle("IupFlatLabel", []interface{}{"string"}, []interface{}{title})
}

func FlatList() Ihandle {
	return ccallHandle("IupFlatList", nil, nil)
}

func FlatScrollBox(child Ihandle) Ihandle {
	return ccallHandle("IupFlatScrollBox", []interface{}{"number"}, []interface{}{int(child)})
}

func FlatTabs(children ...Ihandle) Ihandle { return newContainer("flattabs", children) }

func FlatToggle(title string) Ihandle {
	return ccallHandle("IupFlatToggle", []interface{}{"string"}, []interface{}{title})
}

func FlatTree() Ihandle {
	return ccallHandle("IupFlatTree", nil, nil)
}

func FlatVal(orientation string) Ihandle {
	return ccallHandle("IupFlatVal", []interface{}{"string"}, []interface{}{orientation})
}

func Gauge() Ihandle {
	return ccallHandle("IupGauge", nil, nil)
}

func Matrix() Ihandle {
	return ccallHandle("IupMatrix", nil, nil)
}

func MatrixEx() Ihandle {
	return ccallHandle("IupMatrixEx", nil, nil)
}

func MatrixList() Ihandle {
	return ccallHandle("IupMatrixList", nil, nil)
}
