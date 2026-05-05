package iup

import (
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
	ch := loadCallback((Ihandle)(ih), "_IUPGO_LISTRELEASE_CB")
	if ch == 0 {
		return C.IUP_DEFAULT
	}

	f := ch.Value().(ListReleaseFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col), C.GoString(status)))
}

// setListReleaseFunc for LISTRELEASE_CB.
func setListReleaseFunc(ih Ihandle, f ListReleaseFunc) {
	storeCallback(ih, "_IUPGO_LISTRELEASE_CB", f)

	C.goIupSetListReleaseFunc(ih.ptr())
}

//--------------------

// ListInsertFunc for LISTINSERT_CB callback in MatrixList.
// Called when an item is inserted.
type ListInsertFunc func(ih Ihandle, pos int) int

//export goIupListInsertCB
func goIupListInsertCB(ih unsafe.Pointer, pos C.int) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_LISTINSERT_CB")
	if ch == 0 {
		return C.IUP_DEFAULT
	}

	f := ch.Value().(ListInsertFunc)

	return C.int(f((Ihandle)(ih), int(pos)))
}

// setListInsertFunc for LISTINSERT_CB.
func setListInsertFunc(ih Ihandle, f ListInsertFunc) {
	storeCallback(ih, "_IUPGO_LISTINSERT_CB", f)

	C.goIupSetListInsertFunc(ih.ptr())
}

//--------------------

// ListRemoveFunc for LISTREMOVE_CB callback in MatrixList.
// Called when an item is removed.
type ListRemoveFunc func(ih Ihandle, pos int) int

//export goIupListRemoveCB
func goIupListRemoveCB(ih unsafe.Pointer, pos C.int) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_LISTREMOVE_CB")
	if ch == 0 {
		return C.IUP_DEFAULT
	}

	f := ch.Value().(ListRemoveFunc)

	return C.int(f((Ihandle)(ih), int(pos)))
}

// setListRemoveFunc for LISTREMOVE_CB.
func setListRemoveFunc(ih Ihandle, f ListRemoveFunc) {
	storeCallback(ih, "_IUPGO_LISTREMOVE_CB", f)

	C.goIupSetListRemoveFunc(ih.ptr())
}

//--------------------

// ListEditionFunc for LISTEDITION_CB callback in MatrixList.
// Called during list item editing.
type ListEditionFunc func(ih Ihandle, lin, col, mode, update int) int

//export goIupListEditionCB
func goIupListEditionCB(ih unsafe.Pointer, lin, col, mode, update C.int) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_LISTEDITION_CB")
	if ch == 0 {
		return C.IUP_DEFAULT
	}

	f := ch.Value().(ListEditionFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col), int(mode), int(update)))
}

// setListEditionFunc for LISTEDITION_CB.
func setListEditionFunc(ih Ihandle, f ListEditionFunc) {
	storeCallback(ih, "_IUPGO_LISTEDITION_CB", f)

	C.goIupSetListEditionFunc(ih.ptr())
}

//--------------------

// ListDrawFunc for LISTDRAW_CB callback in MatrixList.
// Called to draw a list item (custom drawing).
type ListDrawFunc func(ih Ihandle, lin, x1, x2, y1, y2, canvas int) int

//export goIupListDrawCB
func goIupListDrawCB(ih unsafe.Pointer, lin, x1, x2, y1, y2, canvas C.int) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_LISTDRAW_CB")
	if ch == 0 {
		return C.IUP_DEFAULT
	}

	f := ch.Value().(ListDrawFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(x1), int(x2), int(y1), int(y2), int(canvas)))
}

// setListDrawFunc for LISTDRAW_CB.
func setListDrawFunc(ih Ihandle, f ListDrawFunc) {
	storeCallback(ih, "_IUPGO_LISTDRAW_CB", f)

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
	ch := loadCallback((Ihandle)(ih), "_IUPGO_BUSY_CB")
	if ch == 0 {
		return C.IUP_DEFAULT
	}

	f := ch.Value().(BusyFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col), C.GoString(status)))
}

// setBusyFunc for BUSY_CB.
func setBusyFunc(ih Ihandle, f BusyFunc) {
	storeCallback(ih, "_IUPGO_BUSY_CB", f)

	C.goIupSetBusyFunc(ih.ptr())
}

//--------------------

// MenuContextFunc for MENUCONTEXT_CB callback in MatrixEx.
// Called to show context menu for a cell.
type MenuContextFunc func(ih, menu Ihandle, lin, col int) int

//export goIupMenuContextCB
func goIupMenuContextCB(ih, menu unsafe.Pointer, lin, col C.int) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_MENUCONTEXT_CB")
	if ch == 0 {
		return C.IUP_DEFAULT
	}

	f := ch.Value().(MenuContextFunc)

	return C.int(f((Ihandle)(ih), (Ihandle)(menu), int(lin), int(col)))
}

// setMenuContextFunc for MENUCONTEXT_CB.
func setMenuContextFunc(ih Ihandle, f MenuContextFunc) {
	storeCallback(ih, "_IUPGO_MENUCONTEXT_CB", f)

	C.goIupSetMenuContextFunc(ih.ptr())
}

//--------------------

// MenuContextCloseFunc for MENUCONTEXTCLOSE_CB callback in MatrixEx.
// Called when context menu is closed.
type MenuContextCloseFunc func(ih, menu Ihandle, lin, col int) int

//export goIupMenuContextCloseCB
func goIupMenuContextCloseCB(ih, menu unsafe.Pointer, lin, col C.int) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_MENUCONTEXTCLOSE_CB")
	if ch == 0 {
		return C.IUP_DEFAULT
	}

	f := ch.Value().(MenuContextCloseFunc)

	return C.int(f((Ihandle)(ih), (Ihandle)(menu), int(lin), int(col)))
}

// setMenuContextCloseFunc for MENUCONTEXTCLOSE_CB.
func setMenuContextCloseFunc(ih Ihandle, f MenuContextCloseFunc) {
	storeCallback(ih, "_IUPGO_MENUCONTEXTCLOSE_CB", f)

	C.goIupSetMenuContextCloseFunc(ih.ptr())
}

//--------------------

// PasteSizeFunc for PASTESIZE_CB callback in MatrixEx.
// Called to determine paste area size.
type PasteSizeFunc func(ih Ihandle, numlin, numcol int) int

//export goIupPasteSizeCB
func goIupPasteSizeCB(ih unsafe.Pointer, numlin, numcol C.int) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_PASTESIZE_CB")
	if ch == 0 {
		return C.IUP_DEFAULT
	}

	f := ch.Value().(PasteSizeFunc)

	return C.int(f((Ihandle)(ih), int(numlin), int(numcol)))
}

// setPasteSizeFunc for PASTESIZE_CB.
func setPasteSizeFunc(ih Ihandle, f PasteSizeFunc) {
	storeCallback(ih, "_IUPGO_PASTESIZE_CB", f)

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
	ch := loadCallback((Ihandle)(ih), "_IUPGO_NUMERICGETVALUE_CB")
	if ch == 0 {
		return 0.0
	}

	f := ch.Value().(NumericGetValueFunc)

	return C.double(f((Ihandle)(ih), int(lin), int(col)))
}

// setNumericGetValueFunc for NUMERICGETVALUE_CB.
func setNumericGetValueFunc(ih Ihandle, f NumericGetValueFunc) {
	storeCallback(ih, "_IUPGO_NUMERICGETVALUE_CB", f)

	C.goIupSetNumericGetValueFunc(ih.ptr())
}

//--------------------

// NumericSetValueFunc for NUMERICSETVALUE_CB callback in Matrix.
// Called to set numeric value in a cell.
type NumericSetValueFunc func(ih Ihandle, lin, col int, value float64) int

//export goIupNumericSetValueCB
func goIupNumericSetValueCB(ih unsafe.Pointer, lin, col C.int, value C.double) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_NUMERICSETVALUE_CB")
	if ch == 0 {
		return C.IUP_DEFAULT
	}

	f := ch.Value().(NumericSetValueFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col), float64(value)))
}

// setNumericSetValueFunc for NUMERICSETVALUE_CB.
func setNumericSetValueFunc(ih Ihandle, f NumericSetValueFunc) {
	storeCallback(ih, "_IUPGO_NUMERICSETVALUE_CB", f)

	C.goIupSetNumericSetValueFunc(ih.ptr())
}

//--------------------

// SortColumnCompareFunc for SORTCOLUMNCOMPARE_CB callback in Matrix.
// Called to compare two rows during column sort.
type SortColumnCompareFunc func(ih Ihandle, lin1, lin2, col int) int

//export goIupSortColumnCompareCB
func goIupSortColumnCompareCB(ih unsafe.Pointer, lin1, lin2, col C.int) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_SORTCOLUMNCOMPARE_CB")
	if ch == 0 {
		return C.IUP_DEFAULT
	}

	f := ch.Value().(SortColumnCompareFunc)

	return C.int(f((Ihandle)(ih), int(lin1), int(lin2), int(col)))
}

// setSortColumnCompareFunc for SORTCOLUMNCOMPARE_CB.
func setSortColumnCompareFunc(ih Ihandle, f SortColumnCompareFunc) {
	storeCallback(ih, "_IUPGO_SORTCOLUMNCOMPARE_CB", f)

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
	f := loadCallback((Ihandle)(ih), "_IUPGO_CELLS_DRAW_CB").Value().(CellsDrawFunc)

	return C.int(f((Ihandle)(ih), int(i), int(j), int(xmin), int(xmax), int(ymin), int(ymax)))
}

// setCellsDrawFunc for DRAW_CB in Cells.
func setCellsDrawFunc(ih Ihandle, f CellsDrawFunc) {
	storeCallback(ih, "_IUPGO_CELLS_DRAW_CB", f)
	C.goIupSetCellsDrawFunc(ih.ptr())
}

//--------------------

// MouseClickFunc for MOUSECLICK_CB callback.
// Called when mouse is clicked in a cell.
type MouseClickFunc func(ih Ihandle, button, pressed, i, j, x, y int, status string) int

//export goIupMouseClickCB
func goIupMouseClickCB(ih unsafe.Pointer, button, pressed, i, j, x, y C.int, status unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_MOUSECLICK_CB").Value().(MouseClickFunc)

	goStatus := C.GoString((*C.char)(status))
	return C.int(f((Ihandle)(ih), int(button), int(pressed), int(i), int(j), int(x), int(y), goStatus))
}

// setMouseClickFunc for MOUSECLICK_CB.
func setMouseClickFunc(ih Ihandle, f MouseClickFunc) {
	storeCallback(ih, "_IUPGO_MOUSECLICK_CB", f)
	C.goIupSetMouseClickFunc(ih.ptr())
}

//--------------------

// MouseMotionFunc for MOUSEMOTION_CB callback.
// Called when mouse moves over a cell.
type MouseMotionFunc func(ih Ihandle, i, j, x, y int, status string) int

//export goIupMouseMotionCB
func goIupMouseMotionCB(ih unsafe.Pointer, i, j, x, y C.int, status unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_MOUSEMOTION_CB").Value().(MouseMotionFunc)

	goStatus := C.GoString((*C.char)(status))
	return C.int(f((Ihandle)(ih), int(i), int(j), int(x), int(y), goStatus))
}

// setMouseMotionFunc for MOUSEMOTION_CB in Cells.
func setMouseMotionFunc(ih Ihandle, f MouseMotionFunc) {
	storeCallback(ih, "_IUPGO_MOUSEMOTION_CB", f)
	C.goIupSetMouseMotionFunc(ih.ptr())
}

//--------------------

// ScrollingFunc for SCROLLING_CB callback.
// Called when cells are scrolled.
type ScrollingFunc func(ih Ihandle, i, j int) int

//export goIupScrollingCB
func goIupScrollingCB(ih unsafe.Pointer, i, j C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_SCROLLING_CB").Value().(ScrollingFunc)

	return C.int(f((Ihandle)(ih), int(i), int(j)))
}

// setScrollingFunc for SCROLLING_CB.
func setScrollingFunc(ih Ihandle, f ScrollingFunc) {
	storeCallback(ih, "_IUPGO_SCROLLING_CB", f)
	C.goIupSetScrollingFunc(ih.ptr())
}

//--------------------

// NColsFunc for NCOLS_CB callback.
// Called to get the number of columns.
type NColsFunc func(ih Ihandle) int

//export goIupNColsCB
func goIupNColsCB(ih unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_NCOLS_CB").Value().(NColsFunc)

	return C.int(f((Ihandle)(ih)))
}

// setNColsFunc for NCOLS_CB.
func setNColsFunc(ih Ihandle, f NColsFunc) {
	storeCallback(ih, "_IUPGO_NCOLS_CB", f)
	C.goIupSetNColsFunc(ih.ptr())
}

//--------------------

// NLinesFunc for NLINES_CB callback.
// Called to get the number of lines.
type NLinesFunc func(ih Ihandle) int

//export goIupNLinesCB
func goIupNLinesCB(ih unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_NLINES_CB").Value().(NLinesFunc)

	return C.int(f((Ihandle)(ih)))
}

// setNLinesFunc for NLINES_CB.
func setNLinesFunc(ih Ihandle, f NLinesFunc) {
	storeCallback(ih, "_IUPGO_NLINES_CB", f)
	C.goIupSetNLinesFunc(ih.ptr())
}

//--------------------

// HSpanFunc for HSPAN_CB callback.
// Called to get horizontal span of a cell.
type HSpanFunc func(ih Ihandle, i, j int) int

//export goIupHSpanCB
func goIupHSpanCB(ih unsafe.Pointer, i, j C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_HSPAN_CB").Value().(HSpanFunc)

	return C.int(f((Ihandle)(ih), int(i), int(j)))
}

// setHSpanFunc for HSPAN_CB.
func setHSpanFunc(ih Ihandle, f HSpanFunc) {
	storeCallback(ih, "_IUPGO_HSPAN_CB", f)
	C.goIupSetHSpanFunc(ih.ptr())
}

//--------------------

// VSpanFunc for VSPAN_CB callback.
// Called to get vertical span of a cell.
type VSpanFunc func(ih Ihandle, i, j int) int

//export goIupVSpanCB
func goIupVSpanCB(ih unsafe.Pointer, i, j C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_VSPAN_CB").Value().(VSpanFunc)

	return C.int(f((Ihandle)(ih), int(i), int(j)))
}

// setVSpanFunc for VSPAN_CB.
func setVSpanFunc(ih Ihandle, f VSpanFunc) {
	storeCallback(ih, "_IUPGO_VSPAN_CB", f)
	C.goIupSetVSpanFunc(ih.ptr())
}

//--------------------

// HeightFunc for HEIGHT_CB callback.
// Called to get the height of a line.
type HeightFunc func(ih Ihandle, i int) int

//export goIupHeightCB
func goIupHeightCB(ih unsafe.Pointer, i C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_HEIGHT_CB").Value().(HeightFunc)

	return C.int(f((Ihandle)(ih), int(i)))
}

// setHeightFunc for HEIGHT_CB.
func setHeightFunc(ih Ihandle, f HeightFunc) {
	storeCallback(ih, "_IUPGO_HEIGHT_CB", f)
	C.goIupSetHeightFunc(ih.ptr())
}

//--------------------

// WidthFunc for WIDTH_CB callback.
// Called to get the width of a column.
type WidthFunc func(ih Ihandle, j int) int

//export goIupWidthCB
func goIupWidthCB(ih unsafe.Pointer, j C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_WIDTH_CB").Value().(WidthFunc)

	return C.int(f((Ihandle)(ih), int(j)))
}

// setWidthFunc for WIDTH_CB.
func setWidthFunc(ih Ihandle, f WidthFunc) {
	storeCallback(ih, "_IUPGO_WIDTH_CB", f)
	C.goIupSetWidthFunc(ih.ptr())
}

//--------------------

// BgColorFunc for BGCOLOR_CB callback.
// Called to determine the background color of a cell.
type BgColorFunc func(ih Ihandle, lin, col int) (r, g, b int, ret int)

//export goIupBgColorCB
func goIupBgColorCB(ih unsafe.Pointer, lin, col C.int, r, g, b *C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_BGCOLOR_CB").Value().(BgColorFunc)

	rr, gg, bb, ret := f((Ihandle)(ih), int(lin), int(col))
	*r = C.int(rr)
	*g = C.int(gg)
	*b = C.int(bb)
	return C.int(ret)
}

// setBgColorFunc for BGCOLOR_CB.
func setBgColorFunc(ih Ihandle, f BgColorFunc) {
	storeCallback(ih, "_IUPGO_BGCOLOR_CB", f)
	C.goIupSetBgColorFunc(ih.ptr())
}

//--------------------

// ClickFunc for CLICK_CB callback.
// Called when a cell is clicked.
type ClickFunc func(ih Ihandle, lin, col int, status string) int

//export goIupClickCB
func goIupClickCB(ih unsafe.Pointer, lin, col C.int, status unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_CLICK_CB").Value().(ClickFunc)

	goStatus := C.GoString((*C.char)(status))
	return C.int(f((Ihandle)(ih), int(lin), int(col), goStatus))
}

// setClickFunc for CLICK_CB in Matrix.
func setClickFunc(ih Ihandle, f ClickFunc) {
	storeCallback(ih, "_IUPGO_CLICK_CB", f)
	C.goIupSetClickFunc(ih.ptr())
}

//--------------------

// ColResizeFunc for COLRESIZE_CB callback.
// Called when a column is resized.
type ColResizeFunc func(ih Ihandle, col int) int

//export goIupColResizeCB
func goIupColResizeCB(ih unsafe.Pointer, col C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_COLRESIZE_CB").Value().(ColResizeFunc)

	return C.int(f((Ihandle)(ih), int(col)))
}

// setColResizeFunc for COLRESIZE_CB.
func setColResizeFunc(ih Ihandle, f ColResizeFunc) {
	storeCallback(ih, "_IUPGO_COLRESIZE_CB", f)
	C.goIupSetColResizeFunc(ih.ptr())
}

//--------------------

// DropCheckFunc for DROPCHECK_CB callback.
// Called to check if dropdown should be shown.
type DropCheckFunc func(ih Ihandle, lin, col int) int

//export goIupDropCheckCB
func goIupDropCheckCB(ih unsafe.Pointer, lin, col C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_DROPCHECK_CB").Value().(DropCheckFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col)))
}

// setDropCheckFunc for DROPCHECK_CB.
func setDropCheckFunc(ih Ihandle, f DropCheckFunc) {
	storeCallback(ih, "_IUPGO_DROPCHECK_CB", f)
	C.goIupSetDropCheckFunc(ih.ptr())
}

//--------------------

// DropSelectFunc for DROPSELECT_CB callback.
// Called when dropdown item is selected.
type DropSelectFunc func(ih Ihandle, lin, col int, drop Ihandle, text string, item, col2 int) int

//export goIupDropSelectCB
func goIupDropSelectCB(ih unsafe.Pointer, lin, col C.int, drop unsafe.Pointer, text *C.char, item, col2 C.int) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_DROPSELECT_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(DropSelectFunc)
	goText := C.GoString(text)
	return C.int(f((Ihandle)(ih), int(lin), int(col), (Ihandle)(drop), goText, int(item), int(col2)))
}

// setDropSelectFunc for DROPSELECT_CB.
func setDropSelectFunc(ih Ihandle, f DropSelectFunc) {
	storeCallback(ih, "_IUPGO_DROPSELECT_CB", f)
	C.goIupSetDropSelectFunc(ih.ptr())
}

//--------------------

// EditBeginFunc for EDITBEGIN_CB callback.
// Called when cell editing begins.
type EditBeginFunc func(ih Ihandle, lin, col int) int

//export goIupEditBeginCB
func goIupEditBeginCB(ih unsafe.Pointer, lin, col C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_EDITBEGIN_CB").Value().(EditBeginFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col)))
}

// setEditBeginFunc for EDITBEGIN_CB.
func setEditBeginFunc(ih Ihandle, f EditBeginFunc) {
	storeCallback(ih, "_IUPGO_EDITBEGIN_CB", f)
	C.goIupSetEditBeginFunc(ih.ptr())
}

//--------------------

// EditClickFunc for EDITCLICK_CB callback.
// Called when edit field is clicked.
type EditClickFunc func(ih Ihandle, lin, col int, status string) int

//export goIupEditClickCB
func goIupEditClickCB(ih unsafe.Pointer, lin, col C.int, status *C.char) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_EDITCLICK_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(EditClickFunc)
	goStatus := C.GoString(status)
	return C.int(f((Ihandle)(ih), int(lin), int(col), goStatus))
}

// setEditClickFunc for EDITCLICK_CB.
func setEditClickFunc(ih Ihandle, f EditClickFunc) {
	storeCallback(ih, "_IUPGO_EDITCLICK_CB", f)
	C.goIupSetEditClickFunc(ih.ptr())
}

//--------------------

// EditEndFunc for EDITEND_CB callback.
// Called when cell editing ends.
type EditEndFunc func(ih Ihandle, lin, col int, newValue string, apply int) int

//export goIupEditEndCB
func goIupEditEndCB(ih unsafe.Pointer, lin, col C.int, newValue *C.char, apply C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_EDITEND_CB").Value().(EditEndFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col), C.GoString(newValue), int(apply)))
}

// setEditEndFunc for EDITEND_CB.
func setEditEndFunc(ih Ihandle, f EditEndFunc) {
	storeCallback(ih, "_IUPGO_EDITEND_CB", f)
	C.goIupSetEditEndFunc(ih.ptr())
}

//--------------------

// EditMouseMoveFunc for EDITMOUSEMOVE_CB callback.
// Called when mouse moves in edit field.
type EditMouseMoveFunc func(ih Ihandle, lin, col int) int

//export goIupEditMouseMoveCB
func goIupEditMouseMoveCB(ih unsafe.Pointer, lin, col C.int) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_EDITMOUSEMOVE_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(EditMouseMoveFunc)
	return C.int(f((Ihandle)(ih), int(lin), int(col)))
}

// setEditMouseMoveFunc for EDITMOUSEMOVE_CB.
func setEditMouseMoveFunc(ih Ihandle, f EditMouseMoveFunc) {
	storeCallback(ih, "_IUPGO_EDITMOUSEMOVE_CB", f)
	C.goIupSetEditMouseMoveFunc(ih.ptr())
}

//--------------------

// EditReleaseFunc for EDITRELEASE_CB callback.
// Called when mouse button is released in edit field.
type EditReleaseFunc func(ih Ihandle, lin, col int, status string) int

//export goIupEditReleaseCB
func goIupEditReleaseCB(ih unsafe.Pointer, lin, col C.int, status *C.char) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_EDITRELEASE_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(EditReleaseFunc)
	goStatus := C.GoString(status)
	return C.int(f((Ihandle)(ih), int(lin), int(col), goStatus))
}

// setEditReleaseFunc for EDITRELEASE_CB.
func setEditReleaseFunc(ih Ihandle, f EditReleaseFunc) {
	storeCallback(ih, "_IUPGO_EDITRELEASE_CB", f)
	C.goIupSetEditReleaseFunc(ih.ptr())
}

//--------------------

// EditionFunc for EDITION_CB callback.
// Called during cell editing.
type EditionFunc func(ih Ihandle, lin, col, mode, update int) int

//export goIupEditionCB
func goIupEditionCB(ih unsafe.Pointer, lin, col, mode, update C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_EDITION_CB").Value().(EditionFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col), int(mode), int(update)))
}

// setEditionFunc for EDITION_CB.
func setEditionFunc(ih Ihandle, f EditionFunc) {
	storeCallback(ih, "_IUPGO_EDITION_CB", f)
	C.goIupSetEditionFunc(ih.ptr())
}

//--------------------

// EnterItemFunc for ENTERITEM_CB callback.
// Called when a cell is entered.
type EnterItemFunc func(ih Ihandle, lin, col int) int

//export goIupEnterItemCB
func goIupEnterItemCB(ih unsafe.Pointer, lin, col C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_ENTERITEM_CB").Value().(EnterItemFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col)))
}

// setEnterItemFunc for ENTERITEM_CB.
func setEnterItemFunc(ih Ihandle, f EnterItemFunc) {
	storeCallback(ih, "_IUPGO_ENTERITEM_CB", f)
	C.goIupSetEnterItemFunc(ih.ptr())
}

//--------------------

// FgColorFunc for FGCOLOR_CB callback.
// Called to determine the foreground color of a cell.
type FgColorFunc func(ih Ihandle, lin, col int) (r, g, b int, ret int)

//export goIupFgColorCB
func goIupFgColorCB(ih unsafe.Pointer, lin, col C.int, r, g, b *C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_FGCOLOR_CB").Value().(FgColorFunc)

	rr, gg, bb, ret := f((Ihandle)(ih), int(lin), int(col))
	*r = C.int(rr)
	*g = C.int(gg)
	*b = C.int(bb)
	return C.int(ret)
}

// setFgColorFunc for FGCOLOR_CB.
func setFgColorFunc(ih Ihandle, f FgColorFunc) {
	storeCallback(ih, "_IUPGO_FGCOLOR_CB", f)
	C.goIupSetFgColorFunc(ih.ptr())
}

//--------------------

// LeaveItemFunc for LEAVEITEM_CB callback.
// Called when a cell is left.
type LeaveItemFunc func(ih Ihandle, lin, col int) int

//export goIupLeaveItemCB
func goIupLeaveItemCB(ih unsafe.Pointer, lin, col C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_LEAVEITEM_CB").Value().(LeaveItemFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col)))
}

// setLeaveItemFunc for LEAVEITEM_CB.
func setLeaveItemFunc(ih Ihandle, f LeaveItemFunc) {
	storeCallback(ih, "_IUPGO_LEAVEITEM_CB", f)
	C.goIupSetLeaveItemFunc(ih.ptr())
}

//--------------------

// MarkEditFunc for MARKEDIT_CB callback.
// Called when cell mark is edited.
type MarkEditFunc func(ih Ihandle, lin, col, marked int) int

//export goIupMarkEditCB
func goIupMarkEditCB(ih unsafe.Pointer, lin, col, marked C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_MARKEDIT_CB").Value().(MarkEditFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col), int(marked)))
}

// setMarkEditFunc for MARKEDIT_CB.
func setMarkEditFunc(ih Ihandle, f MarkEditFunc) {
	storeCallback(ih, "_IUPGO_MARKEDIT_CB", f)
	C.goIupSetMarkEditFunc(ih.ptr())
}

//--------------------

// MarkFunc for MARK_CB callback.
// Called when a cell is marked.
type MarkFunc func(ih Ihandle, lin, col int) int

//export goIupMarkCB
func goIupMarkCB(ih unsafe.Pointer, lin, col C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_MARK_CB").Value().(MarkFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col)))
}

// setMarkFunc for MARK_CB.
func setMarkFunc(ih Ihandle, f MarkFunc) {
	storeCallback(ih, "_IUPGO_MARK_CB", f)
	C.goIupSetMarkFunc(ih.ptr())
}

//--------------------

// MatrixActionFunc for ACTION_CB callback in Matrix control.
// Called for keyboard actions in a cell.
type MatrixActionFunc func(ih Ihandle, key, lin, col, edition int, status string) int

//export goIupMatrixActionCB
func goIupMatrixActionCB(ih unsafe.Pointer, key, lin, col, edition C.int, status *C.char) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_MATRIX_ACTION_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(MatrixActionFunc)
	goStatus := C.GoString(status)
	return C.int(f((Ihandle)(ih), int(key), int(lin), int(col), int(edition), goStatus))
}

// setMatrixActionFunc for ACTION_CB.
func setMatrixActionFunc(ih Ihandle, f MatrixActionFunc) {
	storeCallback(ih, "_IUPGO_MATRIX_ACTION_CB", f)
	C.goIupSetMatrixActionFunc(ih.ptr())
}

//--------------------

// MatrixDrawFunc for DRAW_CB callback in Matrix control.
// Called to draw a cell.
type MatrixDrawFunc func(ih Ihandle, lin, col, x1, x2, y1, y2 int) int

//export goIupMatrixDrawCB
func goIupMatrixDrawCB(ih unsafe.Pointer, lin, col, x1, x2, y1, y2 C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_MATRIX_DRAW_CB").Value().(MatrixDrawFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col), int(x1), int(x2), int(y1), int(y2)))
}

// setMatrixDrawFunc for DRAW_CB in Matrix.
func setMatrixDrawFunc(ih Ihandle, f MatrixDrawFunc) {
	storeCallback(ih, "_IUPGO_MATRIX_DRAW_CB", f)
	C.goIupSetMatrixDrawFunc(ih.ptr())
}

//--------------------

// MatrixDropFunc for DROP_CB callback in Matrix control.
// Called when a dropdown is shown.
type MatrixDropFunc func(ih, drop Ihandle, lin, col int) int

//export goIupMatrixDropCB
func goIupMatrixDropCB(ih, drop unsafe.Pointer, lin, col C.int) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_DROP_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(MatrixDropFunc)
	return C.int(f((Ihandle)(ih), (Ihandle)(drop), int(lin), int(col)))
}

// setMatrixDropFunc for DROP_CB.
func setMatrixDropFunc(ih Ihandle, f MatrixDropFunc) {
	storeCallback(ih, "_IUPGO_DROP_CB", f)
	C.goIupSetMatrixDropFunc(ih.ptr())
}

//--------------------

// MatrixFontFunc for FONT_CB callback.
// Called to get the font for a cell.
// Returns font string.
type MatrixFontFunc func(ih Ihandle, lin, col int) string

//export goIupMatrixFontCB
func goIupMatrixFontCB(ih unsafe.Pointer, lin, col C.int) *C.char {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_FONT_CB")
	if ch == 0 {
		return nil
	}
	f := ch.Value().(MatrixFontFunc)
	result := f((Ihandle)(ih), int(lin), int(col))
	if result == "" {
		return nil
	}
	return C.CString(result)
}

// setMatrixFontFunc for FONT_CB.
func setMatrixFontFunc(ih Ihandle, f MatrixFontFunc) {
	storeCallback(ih, "_IUPGO_FONT_CB", f)
	C.goIupSetMatrixFontFunc(ih.ptr())
}

//--------------------

// MatrixMouseMoveFunc for MOUSEMOVE_CB callback in Matrix.
// Called when mouse moves over a cell.
type MatrixMouseMoveFunc func(ih Ihandle, lin, col int) int

//export goIupMatrixMouseMoveCB
func goIupMatrixMouseMoveCB(ih unsafe.Pointer, lin, col C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_MATRIX_MOUSEMOVE_CB").Value().(MatrixMouseMoveFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col)))
}

// setMatrixMouseMoveFunc for MOUSEMOVE_CB in Matrix.
func setMatrixMouseMoveFunc(ih Ihandle, f MatrixMouseMoveFunc) {
	storeCallback(ih, "_IUPGO_MATRIX_MOUSEMOVE_CB", f)
	C.goIupSetMatrixMouseMoveFunc(ih.ptr())
}

//--------------------

// MatrixToggleValueFunc for TOGGLEVALUE_CB callback in Matrix control.
// Called when toggle value changes.
type MatrixToggleValueFunc func(ih Ihandle, lin, col, value int) int

//export goIupMatrixToggleValueCB
func goIupMatrixToggleValueCB(ih unsafe.Pointer, lin, col, value C.int) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_MATRIX_TOGGLEVALUE_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(MatrixToggleValueFunc)
	return C.int(f((Ihandle)(ih), int(lin), int(col), int(value)))
}

// setMatrixToggleValueFunc for TOGGLEVALUE_CB.
func setMatrixToggleValueFunc(ih Ihandle, f MatrixToggleValueFunc) {
	storeCallback(ih, "_IUPGO_MATRIX_TOGGLEVALUE_CB", f)
	C.goIupSetMatrixToggleValueFunc(ih.ptr())
}

//--------------------

// MatrixTypeFunc for TYPE_CB callback.
// Called to get the type of a cell.
// Returns type string.
type MatrixTypeFunc func(ih Ihandle, lin, col int) string

//export goIupMatrixTypeCB
func goIupMatrixTypeCB(ih unsafe.Pointer, lin, col C.int) *C.char {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_TYPE_CB")
	if ch == 0 {
		return nil
	}
	f := ch.Value().(MatrixTypeFunc)
	result := f((Ihandle)(ih), int(lin), int(col))
	if result == "" {
		return nil
	}
	return C.CString(result)
}

// setMatrixTypeFunc for TYPE_CB.
func setMatrixTypeFunc(ih Ihandle, f MatrixTypeFunc) {
	storeCallback(ih, "_IUPGO_TYPE_CB", f)
	C.goIupSetMatrixTypeFunc(ih.ptr())
}

//--------------------

// MatrixValueFunc for VALUE_CB callback.
// Called to get the value of a cell.
// Returns value string.
type MatrixValueFunc func(ih Ihandle, lin, col int) string

//export goIupMatrixValueCB
func goIupMatrixValueCB(ih unsafe.Pointer, lin, col C.int) *C.char {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_VALUE_CB")
	if ch == 0 {
		return nil
	}
	f := ch.Value().(MatrixValueFunc)
	result := f((Ihandle)(ih), int(lin), int(col))
	if result == "" {
		return nil
	}
	return C.CString(result)
}

// setMatrixValueFunc for VALUE_CB.
func setMatrixValueFunc(ih Ihandle, f MatrixValueFunc) {
	storeCallback(ih, "_IUPGO_VALUE_CB", f)
	C.goIupSetMatrixValueFunc(ih.ptr())
}

//--------------------

// MenuDropFunc for MENUDROP_CB callback.
// Called to show dropdown menu for a cell.
type MenuDropFunc func(ih, menu Ihandle, lin, col int) int

//export goIupMenuDropCB
func goIupMenuDropCB(ih, menu unsafe.Pointer, lin, col C.int) C.int {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_MENUDROP_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(MenuDropFunc)
	return C.int(f((Ihandle)(ih), (Ihandle)(menu), int(lin), int(col)))
}

// setMenuDropFunc for MENUDROP_CB.
func setMenuDropFunc(ih Ihandle, f MenuDropFunc) {
	storeCallback(ih, "_IUPGO_MENUDROP_CB", f)
	C.goIupSetMenuDropFunc(ih.ptr())
}

//--------------------

// ReleaseFunc for RELEASE_CB callback.
// Called when mouse button is released over a cell.
type ReleaseFunc func(ih Ihandle, lin, col int, status string) int

//export goIupReleaseCB
func goIupReleaseCB(ih unsafe.Pointer, lin, col C.int, status unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_RELEASE_CB").Value().(ReleaseFunc)

	goStatus := C.GoString((*C.char)(status))
	return C.int(f((Ihandle)(ih), int(lin), int(col), goStatus))
}

// setReleaseFunc for RELEASE_CB in Matrix.
func setReleaseFunc(ih Ihandle, f ReleaseFunc) {
	storeCallback(ih, "_IUPGO_RELEASE_CB", f)
	C.goIupSetReleaseFunc(ih.ptr())
}

//--------------------

// ResizeMatrixFunc for RESIZEMATRIX_CB callback.
// Called when the matrix is resized.
type ResizeMatrixFunc func(ih Ihandle, width, height int) int

//export goIupResizeMatrixCB
func goIupResizeMatrixCB(ih unsafe.Pointer, width, height C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_RESIZEMATRIX_CB").Value().(ResizeMatrixFunc)

	return C.int(f((Ihandle)(ih), int(width), int(height)))
}

// setResizeMatrixFunc for RESIZEMATRIX_CB.
func setResizeMatrixFunc(ih Ihandle, f ResizeMatrixFunc) {
	storeCallback(ih, "_IUPGO_RESIZEMATRIX_CB", f)
	C.goIupSetResizeMatrixFunc(ih.ptr())
}

//--------------------

// ScrollTopFunc for SCROLLTOP_CB callback.
// Called when scrolling to top.
type ScrollTopFunc func(ih Ihandle, lin, col int) int

//export goIupScrollTopCB
func goIupScrollTopCB(ih unsafe.Pointer, lin, col C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_SCROLLTOP_CB").Value().(ScrollTopFunc)

	return C.int(f((Ihandle)(ih), int(lin), int(col)))
}

// setScrollTopFunc for SCROLLTOP_CB.
func setScrollTopFunc(ih Ihandle, f ScrollTopFunc) {
	storeCallback(ih, "_IUPGO_SCROLLTOP_CB", f)
	C.goIupSetScrollTopFunc(ih.ptr())
}

//--------------------

// TranslateValueFunc for TRANSLATEVALUE_CB callback.
// Called to translate cell value for display.
// Returns translated string.
type TranslateValueFunc func(ih Ihandle, lin, col int, value string) string

//export goIupTranslateValueCB
func goIupTranslateValueCB(ih unsafe.Pointer, lin, col C.int, value *C.char) *C.char {
	ch := loadCallback((Ihandle)(ih), "_IUPGO_TRANSLATEVALUE_CB")
	if ch == 0 {
		return nil
	}
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
	storeCallback(ih, "_IUPGO_TRANSLATEVALUE_CB", f)
	C.goIupSetTranslateValueFunc(ih.ptr())
}

//--------------------

// ValueEditFunc for VALUE_EDIT_CB callback.
// Called to edit cell value.
type ValueEditFunc func(ih Ihandle, lin, col int, newval string) int

//export goIupValueEditCB
func goIupValueEditCB(ih unsafe.Pointer, lin, col C.int, newval unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_VALUE_EDIT_CB").Value().(ValueEditFunc)

	goNewval := C.GoString((*C.char)(newval))
	return C.int(f((Ihandle)(ih), int(lin), int(col), goNewval))
}

// setValueEditFunc for VALUE_EDIT_CB.
func setValueEditFunc(ih Ihandle, f ValueEditFunc) {
	storeCallback(ih, "_IUPGO_VALUE_EDIT_CB", f)
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
	ch := loadCallback((Ihandle)(ih), "_IUPGO_MATRIXLIST_ACTION_CB")
	if ch == 0 {
		return C.int(DEFAULT)
	}
	f := ch.Value().(MatrixListActionFunc)
	return C.int(f((Ihandle)(ih), int(item), int(state)))
}

// setMatrixListActionFunc for ACTION_CB and IMAGEVALUECHANGED_CB.
func setMatrixListActionFunc(ih Ihandle, name string, f MatrixListActionFunc) {
	storeCallback(ih, "_IUPGO_MATRIXLIST_ACTION_CB", f)
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))
	C.goIupSetMatrixListActionFunc(ih.ptr(), cName)
}

//--------------------

// FlatActionFunc for FLAT_ACTION callback.
// Action generated when the button 1 (usually left) is selected.
type FlatActionFunc func(ih Ihandle) int

//export goIupFlatActionCB
func goIupFlatActionCB(ih unsafe.Pointer) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_FLAT_ACTION").Value().(FlatActionFunc)

	return C.int(f((Ihandle)(ih)))
}

// setFlatActionFunc for FLAT_ACTION callback.
func setFlatActionFunc(ih Ihandle, f FlatActionFunc) {
	storeCallback(ih, "_IUPGO_FLAT_ACTION", f)

	C.goIupSetFlatActionFunc(ih.ptr())
}

//--------------------

// FlatListActionFunc for FlatList FLAT_ACTION callback.
// Action generated when the state of an item in the list is interactively changed.
type FlatListActionFunc func(ih Ihandle, text string, item, state int) int

//export goIupFlatListActionCB
func goIupFlatListActionCB(ih, text unsafe.Pointer, item, state C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_FLAT_LIST_ACTION").Value().(FlatListActionFunc)

	goText := C.GoString((*C.char)(text))
	return C.int(f((Ihandle)(ih), goText, int(item), int(state)))
}

// setFlatListActionFunc for FlatList FLAT_ACTION callback.
func setFlatListActionFunc(ih Ihandle, f FlatListActionFunc) {
	storeCallback(ih, "_IUPGO_FLAT_LIST_ACTION", f)

	C.goIupSetFlatListActionFunc(ih.ptr())
}

//--------------------

// FlatToggleActionFunc for FlatToggle FLAT_ACTION callback.
// Action generated when the toggle's state (on/off) was changed.
type FlatToggleActionFunc func(ih Ihandle, state int) int

//export goIupFlatToggleActionCB
func goIupFlatToggleActionCB(ih unsafe.Pointer, state C.int) C.int {
	f := loadCallback((Ihandle)(ih), "_IUPGO_FLAT_TOGGLE_ACTION").Value().(FlatToggleActionFunc)

	return C.int(f((Ihandle)(ih), int(state)))
}

// setFlatToggleActionFunc for FlatToggle FLAT_ACTION.
func setFlatToggleActionFunc(ih Ihandle, f FlatToggleActionFunc) {
	storeCallback(ih, "_IUPGO_FLAT_TOGGLE_ACTION", f)

	C.goIupSetFlatToggleActionFunc(ih.ptr())
}
