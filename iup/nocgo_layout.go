//go:build !cgo && !js

package iup

func Append(ih, child Ihandle) Ihandle {
	return mkih(iupAppend(uintptr(ih), uintptr(child)))
}

func Detach(child Ihandle) {
	iupDetach(uintptr(child))
}

func Insert(ih, refChild, child Ihandle) Ihandle {
	return mkih(iupInsert(uintptr(ih), uintptr(refChild), uintptr(child)))
}

func Reparent(ih, newParent, refChild Ihandle) int {
	return int(iupReparent(uintptr(ih), uintptr(newParent), uintptr(refChild)))
}

func GetParent(ih Ihandle) Ihandle {
	return mkih(iupGetParent(uintptr(ih)))
}

func GetChild(ih Ihandle, pos int) Ihandle {
	return mkih(iupGetChild(uintptr(ih), int32(pos)))
}

func GetChildPos(ih, child Ihandle) int {
	return int(iupGetChildPos(uintptr(ih), uintptr(child)))
}

func GetChildCount(ih Ihandle) int {
	return int(iupGetChildCount(uintptr(ih)))
}

func GetNextChild(ih, child Ihandle) Ihandle {
	return mkih(iupGetNextChild(uintptr(ih), uintptr(child)))
}

func GetBrother(ih Ihandle) Ihandle {
	return mkih(iupGetBrother(uintptr(ih)))
}

func GetDialog(ih Ihandle) Ihandle {
	return mkih(iupGetDialog(uintptr(ih)))
}

func GetDialogChild(ih Ihandle, name string) Ihandle {
	return mkih(iupGetDialogChild(uintptr(ih), name))
}
