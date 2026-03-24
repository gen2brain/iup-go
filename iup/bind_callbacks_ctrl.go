package iup

import (
	"runtime/cgo"
	"unsafe"
)

/*
#include "bind_callbacks.h"
*/
import "C"

// ListReleaseFunc for LISTRELEASE_CB callback in MatrixList.
// Called when mouse button is released over a list item.
type ListReleaseFunc func(ih Ihandle, lin, col int, status string) int

//export goIupListReleaseCB
func goIupListReleaseCB(ih unsafe.Pointer, lin, col C.int, status *C.char) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("LISTRELEASE_CB_" + uuid)
	if !ok {
		return C.IUP_DEFAULT
	}

	f := cgo.Handle(h.(cgo.Handle)).Value().(ListReleaseFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col), C.GoString(status)))
}

// setListReleaseFunc for LISTRELEASE_CB.
func setListReleaseFunc(ih Ihandle, f ListReleaseFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("LISTRELEASE_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetListReleaseFunc(ih.ptr())
}

//--------------------

// ListInsertFunc for LISTINSERT_CB callback in MatrixList.
// Called when an item is inserted.
type ListInsertFunc func(ih Ihandle, pos int) int

//export goIupListInsertCB
func goIupListInsertCB(ih unsafe.Pointer, pos C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("LISTINSERT_CB_" + uuid)
	if !ok {
		return C.IUP_DEFAULT
	}

	f := cgo.Handle(h.(cgo.Handle)).Value().(ListInsertFunc)

	return C.int(f((Ihandle)(ih), int(pos)))
}

// setListInsertFunc for LISTINSERT_CB.
func setListInsertFunc(ih Ihandle, f ListInsertFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("LISTINSERT_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetListInsertFunc(ih.ptr())
}

//--------------------

// ListRemoveFunc for LISTREMOVE_CB callback in MatrixList.
// Called when an item is removed.
type ListRemoveFunc func(ih Ihandle, pos int) int

//export goIupListRemoveCB
func goIupListRemoveCB(ih unsafe.Pointer, pos C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("LISTREMOVE_CB_" + uuid)
	if !ok {
		return C.IUP_DEFAULT
	}

	f := cgo.Handle(h.(cgo.Handle)).Value().(ListRemoveFunc)

	return C.int(f((Ihandle)(ih), int(pos)))
}

// setListRemoveFunc for LISTREMOVE_CB.
func setListRemoveFunc(ih Ihandle, f ListRemoveFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("LISTREMOVE_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetListRemoveFunc(ih.ptr())
}

//--------------------

// ListEditionFunc for LISTEDITION_CB callback in MatrixList.
// Called during list item editing.
type ListEditionFunc func(ih Ihandle, lin, col, mode, update int) int

//export goIupListEditionCB
func goIupListEditionCB(ih unsafe.Pointer, lin, col, mode, update C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("LISTEDITION_CB_" + uuid)
	if !ok {
		return C.IUP_DEFAULT
	}

	f := cgo.Handle(h.(cgo.Handle)).Value().(ListEditionFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col), int(mode), int(update)))
}

// setListEditionFunc for LISTEDITION_CB.
func setListEditionFunc(ih Ihandle, f ListEditionFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("LISTEDITION_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetListEditionFunc(ih.ptr())
}

//--------------------

// ListDrawFunc for LISTDRAW_CB callback in MatrixList.
// Called to draw a list item (custom drawing).
type ListDrawFunc func(ih Ihandle, lin, x1, x2, y1, y2, canvas int) int

//export goIupListDrawCB
func goIupListDrawCB(ih unsafe.Pointer, lin, x1, x2, y1, y2, canvas C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("LISTDRAW_CB_" + uuid)
	if !ok {
		return C.IUP_DEFAULT
	}

	f := cgo.Handle(h.(cgo.Handle)).Value().(ListDrawFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(x1), int(x2), int(y1), int(y2), int(canvas)))
}

// setListDrawFunc for LISTDRAW_CB.
func setListDrawFunc(ih Ihandle, f ListDrawFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("LISTDRAW_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetListDrawFunc(ih.ptr())
}

//--------------------
// MatrixEx Callbacks
//--------------------

// BusyFunc for BUSY_CB callback in MatrixEx.
// Called during busy/long operations.
type BusyFunc func(ih Ihandle, lin, col int, status string) int

//export goIupBusyCB
func goIupBusyCB(ih unsafe.Pointer, lin, col C.int, status *C.char) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("BUSY_CB_" + uuid)
	if !ok {
		return C.IUP_DEFAULT
	}

	f := cgo.Handle(h.(cgo.Handle)).Value().(BusyFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col), C.GoString(status)))
}

// setBusyFunc for BUSY_CB.
func setBusyFunc(ih Ihandle, f BusyFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("BUSY_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetBusyFunc(ih.ptr())
}

//--------------------

// MenuContextFunc for MENUCONTEXT_CB callback in MatrixEx.
// Called to show context menu for a cell.
type MenuContextFunc func(ih, menu Ihandle, lin, col int) int

//export goIupMenuContextCB
func goIupMenuContextCB(ih, menu unsafe.Pointer, lin, col C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("MENUCONTEXT_CB_" + uuid)
	if !ok {
		return C.IUP_DEFAULT
	}

	f := cgo.Handle(h.(cgo.Handle)).Value().(MenuContextFunc)

	return C.int(f((Ihandle)(ih), (Ihandle)(menu), int(lin), int(col)))
}

// setMenuContextFunc for MENUCONTEXT_CB.
func setMenuContextFunc(ih Ihandle, f MenuContextFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MENUCONTEXT_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetMenuContextFunc(ih.ptr())
}

//--------------------

// MenuContextCloseFunc for MENUCONTEXTCLOSE_CB callback in MatrixEx.
// Called when context menu is closed.
type MenuContextCloseFunc func(ih, menu Ihandle, lin, col int) int

//export goIupMenuContextCloseCB
func goIupMenuContextCloseCB(ih, menu unsafe.Pointer, lin, col C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("MENUCONTEXTCLOSE_CB_" + uuid)
	if !ok {
		return C.IUP_DEFAULT
	}

	f := cgo.Handle(h.(cgo.Handle)).Value().(MenuContextCloseFunc)

	return C.int(f((Ihandle)(ih), (Ihandle)(menu), int(lin), int(col)))
}

// setMenuContextCloseFunc for MENUCONTEXTCLOSE_CB.
func setMenuContextCloseFunc(ih Ihandle, f MenuContextCloseFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MENUCONTEXTCLOSE_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetMenuContextCloseFunc(ih.ptr())
}

//--------------------

// PasteSizeFunc for PASTESIZE_CB callback in MatrixEx.
// Called to determine paste area size.
type PasteSizeFunc func(ih Ihandle, numlin, numcol int) int

//export goIupPasteSizeCB
func goIupPasteSizeCB(ih unsafe.Pointer, numlin, numcol C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("PASTESIZE_CB_" + uuid)
	if !ok {
		return C.IUP_DEFAULT
	}

	f := cgo.Handle(h.(cgo.Handle)).Value().(PasteSizeFunc)

	return C.int(f((Ihandle)(ih), int(numlin), int(numcol)))
}

// setPasteSizeFunc for PASTESIZE_CB.
func setPasteSizeFunc(ih Ihandle, f PasteSizeFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("PASTESIZE_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetPasteSizeFunc(ih.ptr())
}

//--------------------
// Matrix Numeric Callbacks
//--------------------

// NumericGetValueFunc for NUMERICGETVALUE_CB callback in Matrix.
// Called to get numeric value from a cell (returns double).
type NumericGetValueFunc func(ih Ihandle, lin, col int) float64

//export goIupNumericGetValueCB
func goIupNumericGetValueCB(ih unsafe.Pointer, lin, col C.int) C.double {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("NUMERICGETVALUE_CB_" + uuid)
	if !ok {
		return 0.0
	}

	f := cgo.Handle(h.(cgo.Handle)).Value().(NumericGetValueFunc)

	return C.double(f((Ihandle)(ih), int(lin), int(col)))
}

// setNumericGetValueFunc for NUMERICGETVALUE_CB.
func setNumericGetValueFunc(ih Ihandle, f NumericGetValueFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("NUMERICGETVALUE_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetNumericGetValueFunc(ih.ptr())
}

//--------------------

// NumericSetValueFunc for NUMERICSETVALUE_CB callback in Matrix.
// Called to set numeric value in a cell.
type NumericSetValueFunc func(ih Ihandle, lin, col int, value float64) int

//export goIupNumericSetValueCB
func goIupNumericSetValueCB(ih unsafe.Pointer, lin, col C.int, value C.double) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("NUMERICSETVALUE_CB_" + uuid)
	if !ok {
		return C.IUP_DEFAULT
	}

	f := cgo.Handle(h.(cgo.Handle)).Value().(NumericSetValueFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col), float64(value)))
}

// setNumericSetValueFunc for NUMERICSETVALUE_CB.
func setNumericSetValueFunc(ih Ihandle, f NumericSetValueFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("NUMERICSETVALUE_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetNumericSetValueFunc(ih.ptr())
}

//--------------------

// SortColumnCompareFunc for SORTCOLUMNCOMPARE_CB callback in Matrix.
// Called to compare two rows during column sort.
type SortColumnCompareFunc func(ih Ihandle, lin1, lin2, col int) int

//export goIupSortColumnCompareCB
func goIupSortColumnCompareCB(ih unsafe.Pointer, lin1, lin2, col C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("SORTCOLUMNCOMPARE_CB_" + uuid)
	if !ok {
		return C.IUP_DEFAULT
	}

	f := cgo.Handle(h.(cgo.Handle)).Value().(SortColumnCompareFunc)

	return C.int(f((Ihandle)(ih), int(lin1), int(lin2), int(col)))
}

// setSortColumnCompareFunc for SORTCOLUMNCOMPARE_CB.
func setSortColumnCompareFunc(ih Ihandle, f SortColumnCompareFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("SORTCOLUMNCOMPARE_CB_"+ih.GetAttribute("UUID"), ch)

	C.goIupSetSortColumnCompareFunc(ih.ptr())
}

//--------------------
// Cells Control Callbacks
//--------------------

// CellsDrawFunc for DRAW_CB callback in Cells control.
// Called to draw a cell.
type CellsDrawFunc func(ih Ihandle, i, j, xmin, xmax, ymin, ymax int) int

//export goIupCellsDrawCB
func goIupCellsDrawCB(ih unsafe.Pointer, i, j, xmin, xmax, ymin, ymax C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("CELLS_DRAW_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "CELLS_DRAW_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(CellsDrawFunc)

	return C.int(f((Ihandle)(ih), int(i), int(j), int(xmin), int(xmax), int(ymin), int(ymax)))
}

// setCellsDrawFunc for DRAW_CB in Cells.
func setCellsDrawFunc(ih Ihandle, f CellsDrawFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("CELLS_DRAW_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetCellsDrawFunc(ih.ptr())
}

//--------------------

// MouseClickFunc for MOUSECLICK_CB callback.
// Called when mouse is clicked in a cell.
type MouseClickFunc func(ih Ihandle, button, pressed, i, j, x, y int, status string) int

//export goIupMouseClickCB
func goIupMouseClickCB(ih unsafe.Pointer, button, pressed, i, j, x, y C.int, status unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("MOUSECLICK_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "MOUSECLICK_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(MouseClickFunc)

	goStatus := C.GoString((*C.char)(status))
	return C.int(f((Ihandle)(ih), int(button), int(pressed), int(i), int(j), int(x), int(y), goStatus))
}

// setMouseClickFunc for MOUSECLICK_CB.
func setMouseClickFunc(ih Ihandle, f MouseClickFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MOUSECLICK_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetMouseClickFunc(ih.ptr())
}

//--------------------

// MouseMotionFunc for MOUSEMOTION_CB callback.
// Called when mouse moves over a cell.
type MouseMotionFunc func(ih Ihandle, i, j, x, y int, status string) int

//export goIupMouseMotionCB
func goIupMouseMotionCB(ih unsafe.Pointer, i, j, x, y C.int, status unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("MOUSEMOTION_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "MOUSEMOTION_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(MouseMotionFunc)

	goStatus := C.GoString((*C.char)(status))
	return C.int(f((Ihandle)(ih), int(i), int(j), int(x), int(y), goStatus))
}

// setMouseMotionFunc for MOUSEMOTION_CB in Cells.
func setMouseMotionFunc(ih Ihandle, f MouseMotionFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MOUSEMOTION_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetMouseMotionFunc(ih.ptr())
}

//--------------------

// ScrollingFunc for SCROLLING_CB callback.
// Called when cells are scrolled.
type ScrollingFunc func(ih Ihandle, i, j int) int

//export goIupScrollingCB
func goIupScrollingCB(ih unsafe.Pointer, i, j C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("SCROLLING_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "SCROLLING_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(ScrollingFunc)

	return C.int(f((Ihandle)(ih), int(i), int(j)))
}

// setScrollingFunc for SCROLLING_CB.
func setScrollingFunc(ih Ihandle, f ScrollingFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("SCROLLING_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetScrollingFunc(ih.ptr())
}

//--------------------

// NColsFunc for NCOLS_CB callback.
// Called to get the number of columns.
type NColsFunc func(ih Ihandle) int

//export goIupNColsCB
func goIupNColsCB(ih unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("NCOLS_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "NCOLS_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(NColsFunc)

	return C.int(f((Ihandle)(ih)))
}

// setNColsFunc for NCOLS_CB.
func setNColsFunc(ih Ihandle, f NColsFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("NCOLS_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetNColsFunc(ih.ptr())
}

//--------------------

// NLinesFunc for NLINES_CB callback.
// Called to get the number of lines.
type NLinesFunc func(ih Ihandle) int

//export goIupNLinesCB
func goIupNLinesCB(ih unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("NLINES_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "NLINES_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(NLinesFunc)

	return C.int(f((Ihandle)(ih)))
}

// setNLinesFunc for NLINES_CB.
func setNLinesFunc(ih Ihandle, f NLinesFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("NLINES_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetNLinesFunc(ih.ptr())
}

//--------------------

// HSpanFunc for HSPAN_CB callback.
// Called to get horizontal span of a cell.
type HSpanFunc func(ih Ihandle, i, j int) int

//export goIupHSpanCB
func goIupHSpanCB(ih unsafe.Pointer, i, j C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("HSPAN_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "HSPAN_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(HSpanFunc)

	return C.int(f((Ihandle)(ih), int(i), int(j)))
}

// setHSpanFunc for HSPAN_CB.
func setHSpanFunc(ih Ihandle, f HSpanFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("HSPAN_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetHSpanFunc(ih.ptr())
}

//--------------------

// VSpanFunc for VSPAN_CB callback.
// Called to get vertical span of a cell.
type VSpanFunc func(ih Ihandle, i, j int) int

//export goIupVSpanCB
func goIupVSpanCB(ih unsafe.Pointer, i, j C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("VSPAN_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "VSPAN_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(VSpanFunc)

	return C.int(f((Ihandle)(ih), int(i), int(j)))
}

// setVSpanFunc for VSPAN_CB.
func setVSpanFunc(ih Ihandle, f VSpanFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("VSPAN_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetVSpanFunc(ih.ptr())
}

//--------------------

// HeightFunc for HEIGHT_CB callback.
// Called to get the height of a line.
type HeightFunc func(ih Ihandle, i int) int

//export goIupHeightCB
func goIupHeightCB(ih unsafe.Pointer, i C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("HEIGHT_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "HEIGHT_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(HeightFunc)

	return C.int(f((Ihandle)(ih), int(i)))
}

// setHeightFunc for HEIGHT_CB.
func setHeightFunc(ih Ihandle, f HeightFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("HEIGHT_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetHeightFunc(ih.ptr())
}

//--------------------

// WidthFunc for WIDTH_CB callback.
// Called to get the width of a column.
type WidthFunc func(ih Ihandle, j int) int

//export goIupWidthCB
func goIupWidthCB(ih unsafe.Pointer, j C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("WIDTH_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "WIDTH_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(WidthFunc)

	return C.int(f((Ihandle)(ih), int(j)))
}

// setWidthFunc for WIDTH_CB.
func setWidthFunc(ih Ihandle, f WidthFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("WIDTH_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetWidthFunc(ih.ptr())
}

//--------------------

// BgColorFunc for BGCOLOR_CB callback.
// Called to determine the background color of a cell.
type BgColorFunc func(ih Ihandle, lin, col int) (r, g, b int, ret int)

//export goIupBgColorCB
func goIupBgColorCB(ih unsafe.Pointer, lin, col C.int, r, g, b *C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("BGCOLOR_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "BGCOLOR_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(BgColorFunc)

	rr, gg, bb, ret := f((Ihandle)(ih), int(lin), int(col))
	*r = C.int(rr)
	*g = C.int(gg)
	*b = C.int(bb)
	return C.int(ret)
}

// setBgColorFunc for BGCOLOR_CB.
func setBgColorFunc(ih Ihandle, f BgColorFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("BGCOLOR_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetBgColorFunc(ih.ptr())
}

//--------------------

// ClickFunc for CLICK_CB callback.
// Called when a cell is clicked.
type ClickFunc func(ih Ihandle, lin, col int, status string) int

//export goIupClickCB
func goIupClickCB(ih unsafe.Pointer, lin, col C.int, status unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("CLICK_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "CLICK_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(ClickFunc)

	goStatus := C.GoString((*C.char)(status))
	return C.int(f((Ihandle)(ih), int(lin), int(col), goStatus))
}

// setClickFunc for CLICK_CB in Matrix.
func setClickFunc(ih Ihandle, f ClickFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("CLICK_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetClickFunc(ih.ptr())
}

//--------------------

// ColResizeFunc for COLRESIZE_CB callback.
// Called when a column is resized.
type ColResizeFunc func(ih Ihandle, col int) int

//export goIupColResizeCB
func goIupColResizeCB(ih unsafe.Pointer, col C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("COLRESIZE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "COLRESIZE_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(ColResizeFunc)

	return C.int(f((Ihandle)(ih), int(col)))
}

// setColResizeFunc for COLRESIZE_CB.
func setColResizeFunc(ih Ihandle, f ColResizeFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("COLRESIZE_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetColResizeFunc(ih.ptr())
}

//--------------------

// DropCheckFunc for DROPCHECK_CB callback.
// Called to check if dropdown should be shown.
type DropCheckFunc func(ih Ihandle, lin, col int) int

//export goIupDropCheckCB
func goIupDropCheckCB(ih unsafe.Pointer, lin, col C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("DROPCHECK_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "DROPCHECK_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(DropCheckFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col)))
}

// setDropCheckFunc for DROPCHECK_CB.
func setDropCheckFunc(ih Ihandle, f DropCheckFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("DROPCHECK_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetDropCheckFunc(ih.ptr())
}

//--------------------

// DropSelectFunc for DROPSELECT_CB callback.
// Called when dropdown item is selected.
type DropSelectFunc func(ih Ihandle, lin, col int, drop Ihandle, text string, item, col2 int) int

//export goIupDropSelectCB
func goIupDropSelectCB(ih unsafe.Pointer, lin, col C.int, drop unsafe.Pointer, text *C.char, item, col2 C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("DROPSELECT_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "DROPSELECT_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(DropSelectFunc)
	goText := C.GoString(text)
	return C.int(f((Ihandle)(ih), int(lin), int(col), (Ihandle)(drop), goText, int(item), int(col2)))
}

// setDropSelectFunc for DROPSELECT_CB.
func setDropSelectFunc(ih Ihandle, f DropSelectFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("DROPSELECT_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetDropSelectFunc(ih.ptr())
}

//--------------------

// EditBeginFunc for EDITBEGIN_CB callback.
// Called when cell editing begins.
type EditBeginFunc func(ih Ihandle, lin, col int) int

//export goIupEditBeginCB
func goIupEditBeginCB(ih unsafe.Pointer, lin, col C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("EDITBEGIN_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "EDITBEGIN_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(EditBeginFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col)))
}

// setEditBeginFunc for EDITBEGIN_CB.
func setEditBeginFunc(ih Ihandle, f EditBeginFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("EDITBEGIN_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetEditBeginFunc(ih.ptr())
}

//--------------------

// EditClickFunc for EDITCLICK_CB callback.
// Called when edit field is clicked.
type EditClickFunc func(ih Ihandle, lin, col int, status string) int

//export goIupEditClickCB
func goIupEditClickCB(ih unsafe.Pointer, lin, col C.int, status *C.char) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("EDITCLICK_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "EDITCLICK_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(EditClickFunc)
	goStatus := C.GoString(status)
	return C.int(f((Ihandle)(ih), int(lin), int(col), goStatus))
}

// setEditClickFunc for EDITCLICK_CB.
func setEditClickFunc(ih Ihandle, f EditClickFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("EDITCLICK_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetEditClickFunc(ih.ptr())
}

//--------------------

// EditEndFunc for EDITEND_CB callback.
// Called when cell editing ends.
type EditEndFunc func(ih Ihandle, lin, col int, newValue string, apply int) int

//export goIupEditEndCB
func goIupEditEndCB(ih unsafe.Pointer, lin, col C.int, newValue *C.char, apply C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("EDITEND_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "EDITEND_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(EditEndFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col), C.GoString(newValue), int(apply)))
}

// setEditEndFunc for EDITEND_CB.
func setEditEndFunc(ih Ihandle, f EditEndFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("EDITEND_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetEditEndFunc(ih.ptr())
}

//--------------------

// EditMouseMoveFunc for EDITMOUSEMOVE_CB callback.
// Called when mouse moves in edit field.
type EditMouseMoveFunc func(ih Ihandle, lin, col int) int

//export goIupEditMouseMoveCB
func goIupEditMouseMoveCB(ih unsafe.Pointer, lin, col C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("EDITMOUSEMOVE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "EDITMOUSEMOVE_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(EditMouseMoveFunc)
	return C.int(f((Ihandle)(ih), int(lin), int(col)))
}

// setEditMouseMoveFunc for EDITMOUSEMOVE_CB.
func setEditMouseMoveFunc(ih Ihandle, f EditMouseMoveFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("EDITMOUSEMOVE_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetEditMouseMoveFunc(ih.ptr())
}

//--------------------

// EditReleaseFunc for EDITRELEASE_CB callback.
// Called when mouse button is released in edit field.
type EditReleaseFunc func(ih Ihandle, lin, col int, status string) int

//export goIupEditReleaseCB
func goIupEditReleaseCB(ih unsafe.Pointer, lin, col C.int, status *C.char) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("EDITRELEASE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "EDITRELEASE_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(EditReleaseFunc)
	goStatus := C.GoString(status)
	return C.int(f((Ihandle)(ih), int(lin), int(col), goStatus))
}

// setEditReleaseFunc for EDITRELEASE_CB.
func setEditReleaseFunc(ih Ihandle, f EditReleaseFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("EDITRELEASE_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetEditReleaseFunc(ih.ptr())
}

//--------------------

// EditionFunc for EDITION_CB callback.
// Called during cell editing.
type EditionFunc func(ih Ihandle, lin, col, mode, update int) int

//export goIupEditionCB
func goIupEditionCB(ih unsafe.Pointer, lin, col, mode, update C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("EDITION_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "EDITION_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(EditionFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col), int(mode), int(update)))
}

// setEditionFunc for EDITION_CB.
func setEditionFunc(ih Ihandle, f EditionFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("EDITION_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetEditionFunc(ih.ptr())
}

//--------------------

// EnterItemFunc for ENTERITEM_CB callback.
// Called when a cell is entered.
type EnterItemFunc func(ih Ihandle, lin, col int) int

//export goIupEnterItemCB
func goIupEnterItemCB(ih unsafe.Pointer, lin, col C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("ENTERITEM_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "ENTERITEM_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(EnterItemFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col)))
}

// setEnterItemFunc for ENTERITEM_CB.
func setEnterItemFunc(ih Ihandle, f EnterItemFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("ENTERITEM_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetEnterItemFunc(ih.ptr())
}

//--------------------

// FgColorFunc for FGCOLOR_CB callback.
// Called to determine the foreground color of a cell.
type FgColorFunc func(ih Ihandle, lin, col int) (r, g, b int, ret int)

//export goIupFgColorCB
func goIupFgColorCB(ih unsafe.Pointer, lin, col C.int, r, g, b *C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("FGCOLOR_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "FGCOLOR_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(FgColorFunc)

	rr, gg, bb, ret := f((Ihandle)(ih), int(lin), int(col))
	*r = C.int(rr)
	*g = C.int(gg)
	*b = C.int(bb)
	return C.int(ret)
}

// setFgColorFunc for FGCOLOR_CB.
func setFgColorFunc(ih Ihandle, f FgColorFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("FGCOLOR_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetFgColorFunc(ih.ptr())
}

//--------------------

// LeaveItemFunc for LEAVEITEM_CB callback.
// Called when a cell is left.
type LeaveItemFunc func(ih Ihandle, lin, col int) int

//export goIupLeaveItemCB
func goIupLeaveItemCB(ih unsafe.Pointer, lin, col C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("LEAVEITEM_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "LEAVEITEM_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(LeaveItemFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col)))
}

// setLeaveItemFunc for LEAVEITEM_CB.
func setLeaveItemFunc(ih Ihandle, f LeaveItemFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("LEAVEITEM_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetLeaveItemFunc(ih.ptr())
}

//--------------------

// MarkEditFunc for MARKEDIT_CB callback.
// Called when cell mark is edited.
type MarkEditFunc func(ih Ihandle, lin, col, marked int) int

//export goIupMarkEditCB
func goIupMarkEditCB(ih unsafe.Pointer, lin, col, marked C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("MARKEDIT_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "MARKEDIT_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(MarkEditFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col), int(marked)))
}

// setMarkEditFunc for MARKEDIT_CB.
func setMarkEditFunc(ih Ihandle, f MarkEditFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MARKEDIT_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetMarkEditFunc(ih.ptr())
}

//--------------------

// MarkFunc for MARK_CB callback.
// Called when a cell is marked.
type MarkFunc func(ih Ihandle, lin, col int) int

//export goIupMarkCB
func goIupMarkCB(ih unsafe.Pointer, lin, col C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("MARK_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "MARK_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(MarkFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col)))
}

// setMarkFunc for MARK_CB.
func setMarkFunc(ih Ihandle, f MarkFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MARK_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetMarkFunc(ih.ptr())
}

//--------------------

// MatrixActionFunc for ACTION_CB callback in Matrix control.
// Called for keyboard actions in a cell.
type MatrixActionFunc func(ih Ihandle, key, lin, col, edition int, status string) int

//export goIupMatrixActionCB
func goIupMatrixActionCB(ih unsafe.Pointer, key, lin, col, edition C.int, status *C.char) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("MATRIX_ACTION_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "MATRIX_ACTION_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(MatrixActionFunc)
	goStatus := C.GoString(status)
	return C.int(f((Ihandle)(ih), int(key), int(lin), int(col), int(edition), goStatus))
}

// setMatrixActionFunc for ACTION_CB.
func setMatrixActionFunc(ih Ihandle, f MatrixActionFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MATRIX_ACTION_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetMatrixActionFunc(ih.ptr())
}

//--------------------

// MatrixDrawFunc for DRAW_CB callback in Matrix control.
// Called to draw a cell.
type MatrixDrawFunc func(ih Ihandle, lin, col, x1, x2, y1, y2 int) int

//export goIupMatrixDrawCB
func goIupMatrixDrawCB(ih unsafe.Pointer, lin, col, x1, x2, y1, y2 C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("MATRIX_DRAW_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "MATRIX_DRAW_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(MatrixDrawFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col), int(x1), int(x2), int(y1), int(y2)))
}

// setMatrixDrawFunc for DRAW_CB in Matrix.
func setMatrixDrawFunc(ih Ihandle, f MatrixDrawFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MATRIX_DRAW_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetMatrixDrawFunc(ih.ptr())
}

//--------------------

// MatrixDropFunc for DROP_CB callback in Matrix control.
// Called when a dropdown is shown.
type MatrixDropFunc func(ih, drop Ihandle, lin, col int) int

//export goIupMatrixDropCB
func goIupMatrixDropCB(ih, drop unsafe.Pointer, lin, col C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("DROP_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "DROP_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(MatrixDropFunc)
	return C.int(f((Ihandle)(ih), (Ihandle)(drop), int(lin), int(col)))
}

// setMatrixDropFunc for DROP_CB.
func setMatrixDropFunc(ih Ihandle, f MatrixDropFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("DROP_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetMatrixDropFunc(ih.ptr())
}

//--------------------

// MatrixFontFunc for FONT_CB callback.
// Called to get the font for a cell.
// Returns font string.
type MatrixFontFunc func(ih Ihandle, lin, col int) string

//export goIupMatrixFontCB
func goIupMatrixFontCB(ih unsafe.Pointer, lin, col C.int) *C.char {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("FONT_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "FONT_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(MatrixFontFunc)
	result := f((Ihandle)(ih), int(lin), int(col))
	if result == "" {
		return nil
	}
	return C.CString(result)
}

// setMatrixFontFunc for FONT_CB.
func setMatrixFontFunc(ih Ihandle, f MatrixFontFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("FONT_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetMatrixFontFunc(ih.ptr())
}

//--------------------

// MatrixMouseMoveFunc for MOUSEMOVE_CB callback in Matrix.
// Called when mouse moves over a cell.
type MatrixMouseMoveFunc func(ih Ihandle, lin, col int) int

//export goIupMatrixMouseMoveCB
func goIupMatrixMouseMoveCB(ih unsafe.Pointer, lin, col C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("MATRIX_MOUSEMOVE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "MATRIX_MOUSEMOVE_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(MatrixMouseMoveFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col)))
}

// setMatrixMouseMoveFunc for MOUSEMOVE_CB in Matrix.
func setMatrixMouseMoveFunc(ih Ihandle, f MatrixMouseMoveFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MATRIX_MOUSEMOVE_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetMatrixMouseMoveFunc(ih.ptr())
}

//--------------------

// MatrixToggleValueFunc for TOGGLEVALUE_CB callback in Matrix control.
// Called when toggle value changes.
type MatrixToggleValueFunc func(ih Ihandle, lin, col, value int) int

//export goIupMatrixToggleValueCB
func goIupMatrixToggleValueCB(ih unsafe.Pointer, lin, col, value C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("MATRIX_TOGGLEVALUE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "MATRIX_TOGGLEVALUE_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(MatrixToggleValueFunc)
	return C.int(f((Ihandle)(ih), int(lin), int(col), int(value)))
}

// setMatrixToggleValueFunc for TOGGLEVALUE_CB.
func setMatrixToggleValueFunc(ih Ihandle, f MatrixToggleValueFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MATRIX_TOGGLEVALUE_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetMatrixToggleValueFunc(ih.ptr())
}

//--------------------

// MatrixTypeFunc for TYPE_CB callback.
// Called to get the type of a cell.
// Returns type string.
type MatrixTypeFunc func(ih Ihandle, lin, col int) string

//export goIupMatrixTypeCB
func goIupMatrixTypeCB(ih unsafe.Pointer, lin, col C.int) *C.char {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("TYPE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "TYPE_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(MatrixTypeFunc)
	result := f((Ihandle)(ih), int(lin), int(col))
	if result == "" {
		return nil
	}
	return C.CString(result)
}

// setMatrixTypeFunc for TYPE_CB.
func setMatrixTypeFunc(ih Ihandle, f MatrixTypeFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("TYPE_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetMatrixTypeFunc(ih.ptr())
}

//--------------------

// MatrixValueFunc for VALUE_CB callback.
// Called to get the value of a cell.
// Returns value string.
type MatrixValueFunc func(ih Ihandle, lin, col int) string

//export goIupMatrixValueCB
func goIupMatrixValueCB(ih unsafe.Pointer, lin, col C.int) *C.char {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("VALUE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "VALUE_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(MatrixValueFunc)
	result := f((Ihandle)(ih), int(lin), int(col))
	if result == "" {
		return nil
	}
	return C.CString(result)
}

// setMatrixValueFunc for VALUE_CB.
func setMatrixValueFunc(ih Ihandle, f MatrixValueFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("VALUE_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetMatrixValueFunc(ih.ptr())
}

//--------------------

// MenuDropFunc for MENUDROP_CB callback.
// Called to show dropdown menu for a cell.
type MenuDropFunc func(ih, menu Ihandle, lin, col int) int

//export goIupMenuDropCB
func goIupMenuDropCB(ih, menu unsafe.Pointer, lin, col C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("MENUDROP_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "MENUDROP_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(MenuDropFunc)
	return C.int(f((Ihandle)(ih), (Ihandle)(menu), int(lin), int(col)))
}

// setMenuDropFunc for MENUDROP_CB.
func setMenuDropFunc(ih Ihandle, f MenuDropFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MENUDROP_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetMenuDropFunc(ih.ptr())
}

//--------------------

// ReleaseFunc for RELEASE_CB callback.
// Called when mouse button is released over a cell.
type ReleaseFunc func(ih Ihandle, lin, col int, status string) int

//export goIupReleaseCB
func goIupReleaseCB(ih unsafe.Pointer, lin, col C.int, status unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("RELEASE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "RELEASE_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(ReleaseFunc)

	goStatus := C.GoString((*C.char)(status))
	return C.int(f((Ihandle)(ih), int(lin), int(col), goStatus))
}

// setReleaseFunc for RELEASE_CB in Matrix.
func setReleaseFunc(ih Ihandle, f ReleaseFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("RELEASE_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetReleaseFunc(ih.ptr())
}

//--------------------

// ResizeMatrixFunc for RESIZEMATRIX_CB callback.
// Called when the matrix is resized.
type ResizeMatrixFunc func(ih Ihandle, width, height int) int

//export goIupResizeMatrixCB
func goIupResizeMatrixCB(ih unsafe.Pointer, width, height C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("RESIZEMATRIX_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "RESIZEMATRIX_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(ResizeMatrixFunc)

	return C.int(f((Ihandle)(ih), int(width), int(height)))
}

// setResizeMatrixFunc for RESIZEMATRIX_CB.
func setResizeMatrixFunc(ih Ihandle, f ResizeMatrixFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("RESIZEMATRIX_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetResizeMatrixFunc(ih.ptr())
}

//--------------------

// ScrollTopFunc for SCROLLTOP_CB callback.
// Called when scrolling to top.
type ScrollTopFunc func(ih Ihandle, lin, col int) int

//export goIupScrollTopCB
func goIupScrollTopCB(ih unsafe.Pointer, lin, col C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("SCROLLTOP_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "SCROLLTOP_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(ScrollTopFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col)))
}

// setScrollTopFunc for SCROLLTOP_CB.
func setScrollTopFunc(ih Ihandle, f ScrollTopFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("SCROLLTOP_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetScrollTopFunc(ih.ptr())
}

//--------------------

// TranslateValueFunc for TRANSLATEVALUE_CB callback.
// Called to translate cell value for display.
// Returns translated string.
type TranslateValueFunc func(ih Ihandle, lin, col int, value string) string

//export goIupTranslateValueCB
func goIupTranslateValueCB(ih unsafe.Pointer, lin, col C.int, value *C.char) *C.char {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("TRANSLATEVALUE_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "TRANSLATEVALUE_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(TranslateValueFunc)
	goValue := C.GoString(value)
	result := f((Ihandle)(ih), int(lin), int(col), goValue)
	if result == "" {
		return nil
	}
	return C.CString(result)
}

// setTranslateValueFunc for TRANSLATEVALUE_CB.
func setTranslateValueFunc(ih Ihandle, f TranslateValueFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("TRANSLATEVALUE_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetTranslateValueFunc(ih.ptr())
}

//--------------------

// ValueEditFunc for VALUE_EDIT_CB callback.
// Called to edit cell value.
type ValueEditFunc func(ih Ihandle, lin, col int, newval string) int

//export goIupValueEditCB
func goIupValueEditCB(ih unsafe.Pointer, lin, col C.int, newval unsafe.Pointer) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("VALUE_EDIT_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "VALUE_EDIT_CB_" + uuid)
	}

	ch := h.(cgo.Handle)
	f := ch.Value().(ValueEditFunc)

	goNewval := C.GoString((*C.char)(newval))
	return C.int(f((Ihandle)(ih), int(lin), int(col), goNewval))
}

// setValueEditFunc for VALUE_EDIT_CB.
func setValueEditFunc(ih Ihandle, f ValueEditFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("VALUE_EDIT_CB_"+ih.GetAttribute("UUID"), ch)
	C.goIupSetValueEditFunc(ih.ptr())
}

//--------------------

//--------------------
// MatrixList Control Callbacks (srcctrl/iup_matrixlist.c)
//--------------------

// MatrixListActionFunc for ACTION_CB and IMAGEVALUECHANGED_CB callbacks in MatrixList.
// Called when item selection or image value changes.
//
// f: func(ih Ihandle, item, state int) int
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_matrixlist.md
type MatrixListActionFunc func(ih Ihandle, item, state int) int

//export goIupMatrixListActionCB
func goIupMatrixListActionCB(ih unsafe.Pointer, item, state C.int) C.int {
	uuid := GetAttribute((Ihandle)(ih), "UUID")
	h, ok := callbacks.Load("MATRIXLIST_ACTION_CB_" + uuid)
	if !ok {
		panic("cannot load callback " + "MATRIXLIST_ACTION_CB_" + uuid)
	}
	ch := h.(cgo.Handle)
	f := ch.Value().(MatrixListActionFunc)
	return C.int(f((Ihandle)(ih), int(item), int(state)))
}

// setMatrixListActionFunc for ACTION_CB and IMAGEVALUECHANGED_CB.
func setMatrixListActionFunc(ih Ihandle, name string, f MatrixListActionFunc) {
	ch := cgo.NewHandle(f)
	callbacks.Store("MATRIXLIST_ACTION_CB_"+ih.GetAttribute("UUID"), ch)
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))
	C.goIupSetMatrixListActionFunc(ih.ptr(), cName)
}
