//go:build !cgo && !js

package iup

func childrenArray(children []Ihandle) []uintptr {
	arr := make([]uintptr, len(children)+1)
	for i, c := range children {
		arr[i] = uintptr(c)
	}
	return arr
}

func Fill() Ihandle {
	return mkih(iupFill())
}

func Space() Ihandle {
	return mkih(iupSpace())
}

func Cbox(children ...Ihandle) Ihandle {
	return mkih(iupCboxv(childrenArray(children)))
}

func GridBox(children ...Ihandle) Ihandle {
	return mkih(iupGridBoxv(childrenArray(children)))
}

func MultiBox(children ...Ihandle) Ihandle {
	return mkih(iupMultiBoxv(childrenArray(children)))
}

func Hbox(children ...Ihandle) Ihandle {
	return mkih(iupHboxv(childrenArray(children)))
}

func Vbox(children ...Ihandle) Ihandle {
	return mkih(iupVboxv(childrenArray(children)))
}

func Zbox(children ...Ihandle) Ihandle {
	return mkih(iupZboxv(childrenArray(children)))
}

func Radio(child Ihandle) Ihandle {
	return mkih(iupRadio(uintptr(child)))
}

func Normalizer(ihList ...Ihandle) Ihandle {
	return mkih(iupNormalizerv(childrenArray(ihList)))
}

func Frame(child Ihandle) Ihandle {
	return mkih(iupFrame(uintptr(child)))
}

func Tabs(children ...Ihandle) Ihandle {
	return mkih(iupTabsv(childrenArray(children)))
}

func BackgroundBox(child Ihandle) Ihandle {
	return mkih(iupBackgroundBox(uintptr(child)))
}

func ScrollBox(child Ihandle) Ihandle {
	return mkih(iupScrollBox(uintptr(child)))
}

func DetachBox(child Ihandle) Ihandle {
	return mkih(iupDetachBox(uintptr(child)))
}

func Expander(child Ihandle) Ihandle {
	return mkih(iupExpander(uintptr(child)))
}

func Sbox(child Ihandle) Ihandle {
	return mkih(iupSbox(uintptr(child)))
}

func Split(child1, child2 Ihandle) Ihandle {
	return mkih(iupSplit(uintptr(child1), uintptr(child2)))
}
