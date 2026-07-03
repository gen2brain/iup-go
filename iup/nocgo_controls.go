//go:build !cgo && !js

package iup

func AnimatedLabel(animation Ihandle) Ihandle {
	return mkih(iupAnimatedLabel(uintptr(animation)))
}

func Button(title string) Ihandle {
	return mkih(iupButton(optCStr(title)))
}

func Calendar() Ihandle {
	return mkih(iupCalendar())
}

func Canvas() Ihandle {
	return mkih(iupCanvas())
}

func ColorBar() Ihandle {
	return mkih(iupColorbar())
}

func ColorBrowser() Ihandle {
	return mkih(iupColorBrowser())
}

func DatePick() Ihandle {
	return mkih(iupDatePick())
}

func Dial(orientation string) Ihandle {
	return mkih(iupDial(optCStr(orientation)))
}

func Label(title string) Ihandle {
	return mkih(iupLabel(optCStr(title)))
}

func Separator() Ihandle {
	return mkih(iupSeparator())
}

func Link(url, title string) Ihandle {
	return mkih(iupLink(optCStr(url), optCStr(title)))
}

func List() Ihandle {
	return mkih(iupList())
}

func ProgressBar() Ihandle {
	return mkih(iupProgressBar())
}

func Spin() Ihandle {
	return mkih(iupSpin())
}

func SpinBox(child Ihandle) Ihandle {
	return mkih(iupSpinbox(uintptr(child)))
}

func Text() Ihandle {
	return mkih(iupText())
}

func TextConvertLinColToPos(ih Ihandle, lin, col int) (pos int) {
	var p int32
	iupTextConvertLinColToPos(uintptr(ih), int32(lin), int32(col), &p)
	return int(p)
}

func TextConvertPosToLinCol(ih Ihandle, pos int) (lin, col int) {
	var l, c int32
	iupTextConvertPosToLinCol(uintptr(ih), int32(pos), &l, &c)
	return int(l), int(c)
}

func MultiLine() Ihandle {
	return mkih(iupMultiLine())
}

func Toggle(title string) Ihandle {
	return mkih(iupToggle(optCStr(title)))
}

func Tree() Ihandle {
	return mkih(iupTree())
}

func TreeSetAttributeHandle(ih Ihandle, name string, id int, ihNamed Ihandle) {
	iupTreeSetAttributeHandle(uintptr(ih), name, int32(id), uintptr(ihNamed))
}

func TreeSetUserId(ih Ihandle, id int, userid uintptr) int {
	return int(iupTreeSetUserId(uintptr(ih), int32(id), userid))
}

func TreeGetUserId(ih Ihandle, id int) uintptr {
	return iupTreeGetUserId(uintptr(ih), int32(id))
}

func TreeGetId(ih Ihandle, userid uintptr) int {
	return int(iupTreeGetId(uintptr(ih), userid))
}

func Table() Ihandle {
	return mkih(iupTable())
}

func Val(_type string) Ihandle {
	return mkih(iupVal(optCStr(_type)))
}

func Scrollbar(orientation string) Ihandle {
	return mkih(iupScrollbar(optCStr(orientation)))
}

func Popover(child Ihandle) Ihandle {
	return mkih(iupPopover(uintptr(child)))
}

func Timer() Ihandle {
	return mkih(iupTimer())
}
