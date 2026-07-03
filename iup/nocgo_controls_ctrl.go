//go:build !cgo && !js && ctrl

package iup

import "github.com/ebitengine/purego"

var (
	iupControlsOpen  func() int32
	iupCells         func() uintptr
	iupMatrix        func() uintptr
	iupMatrixList    func() uintptr
	iupMatrixEx      func() uintptr
	iupFlatButton    func(title *byte) uintptr
	iupFlatLabel     func(title *byte) uintptr
	iupFlatToggle    func(title *byte) uintptr
	iupFlatList      func() uintptr
	iupFlatTree      func() uintptr
	iupFlatVal       func(orientation *byte) uintptr
	iupFlatFrame     func(child uintptr) uintptr
	iupFlatTabsv     func(children []uintptr) uintptr
	iupFlatScrollBox func(child uintptr) uintptr
	iupDropButton    func(dropChild uintptr) uintptr
	iupGauge         func() uintptr
)

func init() {
	ensureBase()
	lib := openLib("iupctrl")
	if lib == 0 {
		panic("iup: cannot load libiupctrl")
	}
	reg := func(fptr any, name string) { purego.RegisterLibFunc(fptr, lib, name) }
	reg(&iupControlsOpen, "IupControlsOpen")
	reg(&iupCells, "IupCells")
	reg(&iupMatrix, "IupMatrix")
	reg(&iupMatrixList, "IupMatrixList")
	reg(&iupMatrixEx, "IupMatrixEx")
	reg(&iupFlatButton, "IupFlatButton")
	reg(&iupFlatLabel, "IupFlatLabel")
	reg(&iupFlatToggle, "IupFlatToggle")
	reg(&iupFlatList, "IupFlatList")
	reg(&iupFlatTree, "IupFlatTree")
	reg(&iupFlatVal, "IupFlatVal")
	reg(&iupFlatFrame, "IupFlatFrame")
	reg(&iupFlatTabsv, "IupFlatTabsv")
	reg(&iupFlatScrollBox, "IupFlatScrollBox")
	reg(&iupDropButton, "IupDropButton")
	reg(&iupGauge, "IupGauge")
}

func ControlsOpen() int { return int(iupControlsOpen()) }

func Cells() Ihandle { return mkih(iupCells()) }

func Matrix() Ihandle { return mkih(iupMatrix()) }

func MatrixList() Ihandle { return mkih(iupMatrixList()) }

func MatrixEx() Ihandle { return mkih(iupMatrixEx()) }

func FlatButton(title string) Ihandle { return mkih(iupFlatButton(optCStr(title))) }

func FlatLabel(title string) Ihandle { return mkih(iupFlatLabel(optCStr(title))) }

func FlatToggle(title string) Ihandle { return mkih(iupFlatToggle(optCStr(title))) }

func FlatList() Ihandle { return mkih(iupFlatList()) }

func FlatTree() Ihandle { return mkih(iupFlatTree()) }

func FlatVal(orientation string) Ihandle { return mkih(iupFlatVal(optCStr(orientation))) }

func FlatFrame(child Ihandle) Ihandle { return mkih(iupFlatFrame(uintptr(child))) }

func FlatTabs(children ...Ihandle) Ihandle { return mkih(iupFlatTabsv(childrenArray(children))) }

func FlatScrollBox(child Ihandle) Ihandle { return mkih(iupFlatScrollBox(uintptr(child))) }

func DropButton(dropChild Ihandle) Ihandle { return mkih(iupDropButton(uintptr(dropChild))) }

func Gauge() Ihandle { return mkih(iupGauge()) }
