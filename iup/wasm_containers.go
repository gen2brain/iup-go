//go:build js && wasm

package iup

func BackgroundBox(child Ihandle) Ihandle {
	return ccallHandle("IupBackgroundBox", []interface{}{"number"}, []interface{}{int(child)})
}

func Cbox(children ...Ihandle) Ihandle { return newContainer("cbox", children) }

func DetachBox(child Ihandle) Ihandle {
	return ccallHandle("IupDetachBox", []interface{}{"number"}, []interface{}{int(child)})
}

func Expander(child Ihandle) Ihandle {
	return ccallHandle("IupExpander", []interface{}{"number"}, []interface{}{int(child)})
}

func GridBox(children ...Ihandle) Ihandle { return newContainer("gridbox", children) }

func MultiBox(children ...Ihandle) Ihandle { return newContainer("multibox", children) }

func Normalizer(ihList ...Ihandle) Ihandle {
	box := Create("normalizer")
	for _, c := range ihList {
		ccall("IupSetAttribute", "", []interface{}{"number", "string", "number"}, []interface{}{int(box), "ADDCONTROL_HANDLE", int(c)})
	}
	return box
}

func Sbox(child Ihandle) Ihandle {
	return ccallHandle("IupSbox", []interface{}{"number"}, []interface{}{int(child)})
}

func ScrollBox(child Ihandle) Ihandle {
	return ccallHandle("IupScrollBox", []interface{}{"number"}, []interface{}{int(child)})
}

func Space() Ihandle {
	return ccallHandle("IupSpace", nil, nil)
}

func Split(child1, child2 Ihandle) Ihandle {
	return ccallHandle("IupSplit", []interface{}{"number", "number"}, []interface{}{int(child1), int(child2)})
}

func Zbox(children ...Ihandle) Ihandle { return newContainer("zbox", children) }

func newContainer(class string, children []Ihandle) Ihandle {
	box := Create(class)
	for _, c := range children {
		Append(box, c)
	}
	return box
}
