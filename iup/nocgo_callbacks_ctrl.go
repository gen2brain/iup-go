//go:build !cgo && !js

package iup

import "github.com/ebitengine/purego"

type BusyFunc func(ih Ihandle, lin, col int, status string) int

var busyFuncCB = purego.NewCallback(func(ih uintptr, lin int32, col int32, status uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_BUSY_CB").(BusyFunc); ok {
		return f(Ihandle(ih), int(lin), int(col), goString(status))
	}
	return 0
})

func setBusyFunc(ih Ihandle, f BusyFunc) {
	storeCallback(ih, "_IUPGO_BUSY_CB", f)
	iupSetCallback(uintptr(ih), "BUSY_CB", busyFuncCB)
}

type CellsDrawFunc func(ih Ihandle, i, j, xmin, xmax, ymin, ymax int) int

var cellsDrawFuncCB = purego.NewCallback(func(ih uintptr, i int32, j int32, xmin int32, xmax int32, ymin int32, ymax int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_CELLS_DRAW_CB").(CellsDrawFunc); ok {
		return f(Ihandle(ih), int(i), int(j), int(xmin), int(xmax), int(ymin), int(ymax))
	}
	return 0
})

func setCellsDrawFunc(ih Ihandle, f CellsDrawFunc) {
	storeCallback(ih, "_IUPGO_CELLS_DRAW_CB", f)
	iupSetCallback(uintptr(ih), "DRAW_CB", cellsDrawFuncCB)
}

type ClickFunc func(ih Ihandle, lin, col int, status string) int

var clickFuncCB = purego.NewCallback(func(ih uintptr, lin int32, col int32, status uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_CLICK_CB").(ClickFunc); ok {
		return f(Ihandle(ih), int(lin), int(col), goString(status))
	}
	return 0
})

func setClickFunc(ih Ihandle, f ClickFunc) {
	storeCallback(ih, "_IUPGO_CLICK_CB", f)
	iupSetCallback(uintptr(ih), "CLICK_CB", clickFuncCB)
}

type ColResizeFunc func(ih Ihandle, col int) int

var colResizeFuncCB = purego.NewCallback(func(ih uintptr, col int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_COLRESIZE_CB").(ColResizeFunc); ok {
		return f(Ihandle(ih), int(col))
	}
	return 0
})

func setColResizeFunc(ih Ihandle, f ColResizeFunc) {
	storeCallback(ih, "_IUPGO_COLRESIZE_CB", f)
	iupSetCallback(uintptr(ih), "COLRESIZE_CB", colResizeFuncCB)
}

type DropCheckFunc func(ih Ihandle, lin, col int) int

var dropCheckFuncCB = purego.NewCallback(func(ih uintptr, lin int32, col int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_DROPCHECK_CB").(DropCheckFunc); ok {
		return f(Ihandle(ih), int(lin), int(col))
	}
	return 0
})

func setDropCheckFunc(ih Ihandle, f DropCheckFunc) {
	storeCallback(ih, "_IUPGO_DROPCHECK_CB", f)
	iupSetCallback(uintptr(ih), "DROPCHECK_CB", dropCheckFuncCB)
}

type DropSelectFunc func(ih Ihandle, lin, col int, drop Ihandle, text string, item, col2 int) int

var dropSelectFuncCB = purego.NewCallback(func(ih uintptr, lin int32, col int32, drop uintptr, text uintptr, item int32, col2 int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_DROPSELECT_CB").(DropSelectFunc); ok {
		return f(Ihandle(ih), int(lin), int(col), Ihandle(drop), goString(text), int(item), int(col2))
	}
	return 0
})

func setDropSelectFunc(ih Ihandle, f DropSelectFunc) {
	storeCallback(ih, "_IUPGO_DROPSELECT_CB", f)
	iupSetCallback(uintptr(ih), "DROPSELECT_CB", dropSelectFuncCB)
}

type EditBeginFunc func(ih Ihandle, lin, col int) int

var editBeginFuncCB = purego.NewCallback(func(ih uintptr, lin int32, col int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_EDITBEGIN_CB").(EditBeginFunc); ok {
		return f(Ihandle(ih), int(lin), int(col))
	}
	return 0
})

func setEditBeginFunc(ih Ihandle, f EditBeginFunc) {
	storeCallback(ih, "_IUPGO_EDITBEGIN_CB", f)
	iupSetCallback(uintptr(ih), "EDITBEGIN_CB", editBeginFuncCB)
}

type EditClickFunc func(ih Ihandle, lin, col int, status string) int

var editClickFuncCB = purego.NewCallback(func(ih uintptr, lin int32, col int32, status uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_EDITCLICK_CB").(EditClickFunc); ok {
		return f(Ihandle(ih), int(lin), int(col), goString(status))
	}
	return 0
})

func setEditClickFunc(ih Ihandle, f EditClickFunc) {
	storeCallback(ih, "_IUPGO_EDITCLICK_CB", f)
	iupSetCallback(uintptr(ih), "EDITCLICK_CB", editClickFuncCB)
}

type EditEndFunc func(ih Ihandle, lin, col int, newValue string, apply int) int

var editEndFuncCB = purego.NewCallback(func(ih uintptr, lin int32, col int32, newValue uintptr, apply int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_EDITEND_CB").(EditEndFunc); ok {
		return f(Ihandle(ih), int(lin), int(col), goString(newValue), int(apply))
	}
	return 0
})

func setEditEndFunc(ih Ihandle, f EditEndFunc) {
	storeCallback(ih, "_IUPGO_EDITEND_CB", f)
	iupSetCallback(uintptr(ih), "EDITEND_CB", editEndFuncCB)
}

type EditionFunc func(ih Ihandle, lin, col, mode, update int) int

var editionFuncCB = purego.NewCallback(func(ih uintptr, lin int32, col int32, mode int32, update int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_EDITION_CB").(EditionFunc); ok {
		return f(Ihandle(ih), int(lin), int(col), int(mode), int(update))
	}
	return 0
})

func setEditionFunc(ih Ihandle, f EditionFunc) {
	storeCallback(ih, "_IUPGO_EDITION_CB", f)
	iupSetCallback(uintptr(ih), "EDITION_CB", editionFuncCB)
}

type EditMouseMoveFunc func(ih Ihandle, lin, col int) int

var editMouseMoveFuncCB = purego.NewCallback(func(ih uintptr, lin int32, col int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_EDITMOUSEMOVE_CB").(EditMouseMoveFunc); ok {
		return f(Ihandle(ih), int(lin), int(col))
	}
	return 0
})

func setEditMouseMoveFunc(ih Ihandle, f EditMouseMoveFunc) {
	storeCallback(ih, "_IUPGO_EDITMOUSEMOVE_CB", f)
	iupSetCallback(uintptr(ih), "EDITMOUSEMOVE_CB", editMouseMoveFuncCB)
}

type EditReleaseFunc func(ih Ihandle, lin, col int, status string) int

var editReleaseFuncCB = purego.NewCallback(func(ih uintptr, lin int32, col int32, status uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_EDITRELEASE_CB").(EditReleaseFunc); ok {
		return f(Ihandle(ih), int(lin), int(col), goString(status))
	}
	return 0
})

func setEditReleaseFunc(ih Ihandle, f EditReleaseFunc) {
	storeCallback(ih, "_IUPGO_EDITRELEASE_CB", f)
	iupSetCallback(uintptr(ih), "EDITRELEASE_CB", editReleaseFuncCB)
}

type EnterItemFunc func(ih Ihandle, lin, col int) int

var enterItemFuncCB = purego.NewCallback(func(ih uintptr, lin int32, col int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_ENTERITEM_CB").(EnterItemFunc); ok {
		return f(Ihandle(ih), int(lin), int(col))
	}
	return 0
})

func setEnterItemFunc(ih Ihandle, f EnterItemFunc) {
	storeCallback(ih, "_IUPGO_ENTERITEM_CB", f)
	iupSetCallback(uintptr(ih), "ENTERITEM_CB", enterItemFuncCB)
}

type FlatActionFunc func(ih Ihandle) int

var flatActionFuncCB = purego.NewCallback(func(ih uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_FLAT_ACTION").(FlatActionFunc); ok {
		return f(Ihandle(ih))
	}
	return 0
})

func setFlatActionFunc(ih Ihandle, f FlatActionFunc) {
	storeCallback(ih, "_IUPGO_FLAT_ACTION", f)
	iupSetCallback(uintptr(ih), "FLAT_ACTION", flatActionFuncCB)
}

type FlatListActionFunc func(ih Ihandle, text string, item, state int) int

var flatListActionFuncCB = purego.NewCallback(func(ih uintptr, text uintptr, item int32, state int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_FLAT_LIST_ACTION").(FlatListActionFunc); ok {
		return f(Ihandle(ih), goString(text), int(item), int(state))
	}
	return 0
})

func setFlatListActionFunc(ih Ihandle, f FlatListActionFunc) {
	storeCallback(ih, "_IUPGO_FLAT_LIST_ACTION", f)
	iupSetCallback(uintptr(ih), "FLAT_ACTION", flatListActionFuncCB)
}

type FlatToggleActionFunc func(ih Ihandle, state int) int

var flatToggleActionFuncCB = purego.NewCallback(func(ih uintptr, state int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_FLAT_TOGGLE_ACTION").(FlatToggleActionFunc); ok {
		return f(Ihandle(ih), int(state))
	}
	return 0
})

func setFlatToggleActionFunc(ih Ihandle, f FlatToggleActionFunc) {
	storeCallback(ih, "_IUPGO_FLAT_TOGGLE_ACTION", f)
	iupSetCallback(uintptr(ih), "FLAT_ACTION", flatToggleActionFuncCB)
}

type HeightFunc func(ih Ihandle, i int) int

var heightFuncCB = purego.NewCallback(func(ih uintptr, i int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_HEIGHT_CB").(HeightFunc); ok {
		return f(Ihandle(ih), int(i))
	}
	return 0
})

func setHeightFunc(ih Ihandle, f HeightFunc) {
	storeCallback(ih, "_IUPGO_HEIGHT_CB", f)
	iupSetCallback(uintptr(ih), "HEIGHT_CB", heightFuncCB)
}

type HSpanFunc func(ih Ihandle, i, j int) int

var hSpanFuncCB = purego.NewCallback(func(ih uintptr, i int32, j int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_HSPAN_CB").(HSpanFunc); ok {
		return f(Ihandle(ih), int(i), int(j))
	}
	return 0
})

func setHSpanFunc(ih Ihandle, f HSpanFunc) {
	storeCallback(ih, "_IUPGO_HSPAN_CB", f)
	iupSetCallback(uintptr(ih), "HSPAN_CB", hSpanFuncCB)
}

type LeaveItemFunc func(ih Ihandle, lin, col int) int

var leaveItemFuncCB = purego.NewCallback(func(ih uintptr, lin int32, col int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_LEAVEITEM_CB").(LeaveItemFunc); ok {
		return f(Ihandle(ih), int(lin), int(col))
	}
	return 0
})

func setLeaveItemFunc(ih Ihandle, f LeaveItemFunc) {
	storeCallback(ih, "_IUPGO_LEAVEITEM_CB", f)
	iupSetCallback(uintptr(ih), "LEAVEITEM_CB", leaveItemFuncCB)
}

type ListDrawFunc func(ih Ihandle, lin, x1, x2, y1, y2, canvas int) int

var listDrawFuncCB = purego.NewCallback(func(ih uintptr, lin int32, x1 int32, x2 int32, y1 int32, y2 int32, canvas int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_LISTDRAW_CB").(ListDrawFunc); ok {
		return f(Ihandle(ih), int(lin), int(x1), int(x2), int(y1), int(y2), int(canvas))
	}
	return 0
})

func setListDrawFunc(ih Ihandle, f ListDrawFunc) {
	storeCallback(ih, "_IUPGO_LISTDRAW_CB", f)
	iupSetCallback(uintptr(ih), "LISTDRAW_CB", listDrawFuncCB)
}

type ListEditionFunc func(ih Ihandle, lin, col, mode, update int) int

var listEditionFuncCB = purego.NewCallback(func(ih uintptr, lin int32, col int32, mode int32, update int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_LISTEDITION_CB").(ListEditionFunc); ok {
		return f(Ihandle(ih), int(lin), int(col), int(mode), int(update))
	}
	return 0
})

func setListEditionFunc(ih Ihandle, f ListEditionFunc) {
	storeCallback(ih, "_IUPGO_LISTEDITION_CB", f)
	iupSetCallback(uintptr(ih), "LISTEDITION_CB", listEditionFuncCB)
}

type ListInsertFunc func(ih Ihandle, pos int) int

var listInsertFuncCB = purego.NewCallback(func(ih uintptr, pos int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_LISTINSERT_CB").(ListInsertFunc); ok {
		return f(Ihandle(ih), int(pos))
	}
	return 0
})

func setListInsertFunc(ih Ihandle, f ListInsertFunc) {
	storeCallback(ih, "_IUPGO_LISTINSERT_CB", f)
	iupSetCallback(uintptr(ih), "LISTINSERT_CB", listInsertFuncCB)
}

type ListReleaseFunc func(ih Ihandle, lin, col int, status string) int

var listReleaseFuncCB = purego.NewCallback(func(ih uintptr, lin int32, col int32, status uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_LISTRELEASE_CB").(ListReleaseFunc); ok {
		return f(Ihandle(ih), int(lin), int(col), goString(status))
	}
	return 0
})

func setListReleaseFunc(ih Ihandle, f ListReleaseFunc) {
	storeCallback(ih, "_IUPGO_LISTRELEASE_CB", f)
	iupSetCallback(uintptr(ih), "LISTRELEASE_CB", listReleaseFuncCB)
}

type ListRemoveFunc func(ih Ihandle, pos int) int

var listRemoveFuncCB = purego.NewCallback(func(ih uintptr, pos int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_LISTREMOVE_CB").(ListRemoveFunc); ok {
		return f(Ihandle(ih), int(pos))
	}
	return 0
})

func setListRemoveFunc(ih Ihandle, f ListRemoveFunc) {
	storeCallback(ih, "_IUPGO_LISTREMOVE_CB", f)
	iupSetCallback(uintptr(ih), "LISTREMOVE_CB", listRemoveFuncCB)
}

type MarkEditFunc func(ih Ihandle, lin, col, marked int) int

var markEditFuncCB = purego.NewCallback(func(ih uintptr, lin int32, col int32, marked int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_MARKEDIT_CB").(MarkEditFunc); ok {
		return f(Ihandle(ih), int(lin), int(col), int(marked))
	}
	return 0
})

func setMarkEditFunc(ih Ihandle, f MarkEditFunc) {
	storeCallback(ih, "_IUPGO_MARKEDIT_CB", f)
	iupSetCallback(uintptr(ih), "MARKEDIT_CB", markEditFuncCB)
}

type MarkFunc func(ih Ihandle, lin, col int) int

var markFuncCB = purego.NewCallback(func(ih uintptr, lin int32, col int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_MARK_CB").(MarkFunc); ok {
		return f(Ihandle(ih), int(lin), int(col))
	}
	return 0
})

func setMarkFunc(ih Ihandle, f MarkFunc) {
	storeCallback(ih, "_IUPGO_MARK_CB", f)
	iupSetCallback(uintptr(ih), "MARK_CB", markFuncCB)
}

type MatrixActionFunc func(ih Ihandle, key, lin, col, edition int, status string) int

var matrixActionFuncCB = purego.NewCallback(func(ih uintptr, key int32, lin int32, col int32, edition int32, status uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_MATRIX_ACTION_CB").(MatrixActionFunc); ok {
		return f(Ihandle(ih), int(key), int(lin), int(col), int(edition), goString(status))
	}
	return 0
})

func setMatrixActionFunc(ih Ihandle, f MatrixActionFunc) {
	storeCallback(ih, "_IUPGO_MATRIX_ACTION_CB", f)
	iupSetCallback(uintptr(ih), "ACTION_CB", matrixActionFuncCB)
}

type MatrixDrawFunc func(ih Ihandle, lin, col, x1, x2, y1, y2 int) int

var matrixDrawFuncCB = purego.NewCallback(func(ih uintptr, lin int32, col int32, x1 int32, x2 int32, y1 int32, y2 int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_MATRIX_DRAW_CB").(MatrixDrawFunc); ok {
		return f(Ihandle(ih), int(lin), int(col), int(x1), int(x2), int(y1), int(y2))
	}
	return 0
})

func setMatrixDrawFunc(ih Ihandle, f MatrixDrawFunc) {
	storeCallback(ih, "_IUPGO_MATRIX_DRAW_CB", f)
	iupSetCallback(uintptr(ih), "DRAW_CB", matrixDrawFuncCB)
}

type MatrixDropFunc func(ih, drop Ihandle, lin, col int) int

var matrixDropFuncCB = purego.NewCallback(func(ih uintptr, drop uintptr, lin int32, col int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_DROP_CB").(MatrixDropFunc); ok {
		return f(Ihandle(ih), Ihandle(drop), int(lin), int(col))
	}
	return 0
})

func setMatrixDropFunc(ih Ihandle, f MatrixDropFunc) {
	storeCallback(ih, "_IUPGO_DROP_CB", f)
	iupSetCallback(uintptr(ih), "DROP_CB", matrixDropFuncCB)
}

type MatrixListActionFunc func(ih Ihandle, item, state int) int

var matrixListActionFuncCB = purego.NewCallback(func(ih uintptr, item int32, state int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_MATRIXLIST_ACTION_CB").(MatrixListActionFunc); ok {
		return f(Ihandle(ih), int(item), int(state))
	}
	return 0
})

func setMatrixListActionFunc(ih Ihandle, f MatrixListActionFunc) {
	storeCallback(ih, "_IUPGO_MATRIXLIST_ACTION_CB", f)
	iupSetCallback(uintptr(ih), "ACTION_CB", matrixListActionFuncCB)
}

type MatrixMouseMoveFunc func(ih Ihandle, lin, col int) int

var matrixMouseMoveFuncCB = purego.NewCallback(func(ih uintptr, lin int32, col int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_MATRIX_MOUSEMOVE_CB").(MatrixMouseMoveFunc); ok {
		return f(Ihandle(ih), int(lin), int(col))
	}
	return 0
})

func setMatrixMouseMoveFunc(ih Ihandle, f MatrixMouseMoveFunc) {
	storeCallback(ih, "_IUPGO_MATRIX_MOUSEMOVE_CB", f)
	iupSetCallback(uintptr(ih), "MOUSEMOVE_CB", matrixMouseMoveFuncCB)
}

type MatrixToggleValueFunc func(ih Ihandle, lin, col, value int) int

var matrixToggleValueFuncCB = purego.NewCallback(func(ih uintptr, lin int32, col int32, value int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_MATRIX_TOGGLEVALUE_CB").(MatrixToggleValueFunc); ok {
		return f(Ihandle(ih), int(lin), int(col), int(value))
	}
	return 0
})

func setMatrixToggleValueFunc(ih Ihandle, f MatrixToggleValueFunc) {
	storeCallback(ih, "_IUPGO_MATRIX_TOGGLEVALUE_CB", f)
	iupSetCallback(uintptr(ih), "TOGGLEVALUE_CB", matrixToggleValueFuncCB)
}

type MenuContextCloseFunc func(ih, menu Ihandle, lin, col int) int

var menuContextCloseFuncCB = purego.NewCallback(func(ih uintptr, menu uintptr, lin int32, col int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_MENUCONTEXTCLOSE_CB").(MenuContextCloseFunc); ok {
		return f(Ihandle(ih), Ihandle(menu), int(lin), int(col))
	}
	return 0
})

func setMenuContextCloseFunc(ih Ihandle, f MenuContextCloseFunc) {
	storeCallback(ih, "_IUPGO_MENUCONTEXTCLOSE_CB", f)
	iupSetCallback(uintptr(ih), "MENUCONTEXTCLOSE_CB", menuContextCloseFuncCB)
}

type MenuContextFunc func(ih, menu Ihandle, lin, col int) int

var menuContextFuncCB = purego.NewCallback(func(ih uintptr, menu uintptr, lin int32, col int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_MENUCONTEXT_CB").(MenuContextFunc); ok {
		return f(Ihandle(ih), Ihandle(menu), int(lin), int(col))
	}
	return 0
})

func setMenuContextFunc(ih Ihandle, f MenuContextFunc) {
	storeCallback(ih, "_IUPGO_MENUCONTEXT_CB", f)
	iupSetCallback(uintptr(ih), "MENUCONTEXT_CB", menuContextFuncCB)
}

type MenuDropFunc func(ih, menu Ihandle, lin, col int) int

var menuDropFuncCB = purego.NewCallback(func(ih uintptr, menu uintptr, lin int32, col int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_MENUDROP_CB").(MenuDropFunc); ok {
		return f(Ihandle(ih), Ihandle(menu), int(lin), int(col))
	}
	return 0
})

func setMenuDropFunc(ih Ihandle, f MenuDropFunc) {
	storeCallback(ih, "_IUPGO_MENUDROP_CB", f)
	iupSetCallback(uintptr(ih), "MENUDROP_CB", menuDropFuncCB)
}

type MouseClickFunc func(ih Ihandle, button, pressed, i, j, x, y int, status string) int

var mouseClickFuncCB = purego.NewCallback(func(ih uintptr, button int32, pressed int32, i int32, j int32, x int32, y int32, status uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_MOUSECLICK_CB").(MouseClickFunc); ok {
		return f(Ihandle(ih), int(button), int(pressed), int(i), int(j), int(x), int(y), goString(status))
	}
	return 0
})

func setMouseClickFunc(ih Ihandle, f MouseClickFunc) {
	storeCallback(ih, "_IUPGO_MOUSECLICK_CB", f)
	iupSetCallback(uintptr(ih), "MOUSECLICK_CB", mouseClickFuncCB)
}

type MouseMotionFunc func(ih Ihandle, i, j, x, y int, status string) int

var mouseMotionFuncCB = purego.NewCallback(func(ih uintptr, i int32, j int32, x int32, y int32, status uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_MOUSEMOTION_CB").(MouseMotionFunc); ok {
		return f(Ihandle(ih), int(i), int(j), int(x), int(y), goString(status))
	}
	return 0
})

func setMouseMotionFunc(ih Ihandle, f MouseMotionFunc) {
	storeCallback(ih, "_IUPGO_MOUSEMOTION_CB", f)
	iupSetCallback(uintptr(ih), "MOUSEMOTION_CB", mouseMotionFuncCB)
}

type NColsFunc func(ih Ihandle) int

var nColsFuncCB = purego.NewCallback(func(ih uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_NCOLS_CB").(NColsFunc); ok {
		return f(Ihandle(ih))
	}
	return 0
})

func setNColsFunc(ih Ihandle, f NColsFunc) {
	storeCallback(ih, "_IUPGO_NCOLS_CB", f)
	iupSetCallback(uintptr(ih), "NCOLS_CB", nColsFuncCB)
}

type NLinesFunc func(ih Ihandle) int

var nLinesFuncCB = purego.NewCallback(func(ih uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_NLINES_CB").(NLinesFunc); ok {
		return f(Ihandle(ih))
	}
	return 0
})

func setNLinesFunc(ih Ihandle, f NLinesFunc) {
	storeCallback(ih, "_IUPGO_NLINES_CB", f)
	iupSetCallback(uintptr(ih), "NLINES_CB", nLinesFuncCB)
}

type NumericSetValueFunc func(ih Ihandle, lin, col int, value float64) int

var numericSetValueFuncCB = purego.NewCallback(func(ih uintptr, lin int32, col int32, value float64) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_NUMERICSETVALUE_CB").(NumericSetValueFunc); ok {
		return f(Ihandle(ih), int(lin), int(col), value)
	}
	return 0
})

func setNumericSetValueFunc(ih Ihandle, f NumericSetValueFunc) {
	storeCallback(ih, "_IUPGO_NUMERICSETVALUE_CB", f)
	iupSetCallback(uintptr(ih), "NUMERICSETVALUE_CB", numericSetValueFuncCB)
}

type PasteSizeFunc func(ih Ihandle, numlin, numcol int) int

var pasteSizeFuncCB = purego.NewCallback(func(ih uintptr, numlin int32, numcol int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_PASTESIZE_CB").(PasteSizeFunc); ok {
		return f(Ihandle(ih), int(numlin), int(numcol))
	}
	return 0
})

func setPasteSizeFunc(ih Ihandle, f PasteSizeFunc) {
	storeCallback(ih, "_IUPGO_PASTESIZE_CB", f)
	iupSetCallback(uintptr(ih), "PASTESIZE_CB", pasteSizeFuncCB)
}

type ReleaseFunc func(ih Ihandle, lin, col int, status string) int

var releaseFuncCB = purego.NewCallback(func(ih uintptr, lin int32, col int32, status uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_RELEASE_CB").(ReleaseFunc); ok {
		return f(Ihandle(ih), int(lin), int(col), goString(status))
	}
	return 0
})

func setReleaseFunc(ih Ihandle, f ReleaseFunc) {
	storeCallback(ih, "_IUPGO_RELEASE_CB", f)
	iupSetCallback(uintptr(ih), "RELEASE_CB", releaseFuncCB)
}

type ResizeMatrixFunc func(ih Ihandle, width, height int) int

var resizeMatrixFuncCB = purego.NewCallback(func(ih uintptr, width int32, height int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_RESIZEMATRIX_CB").(ResizeMatrixFunc); ok {
		return f(Ihandle(ih), int(width), int(height))
	}
	return 0
})

func setResizeMatrixFunc(ih Ihandle, f ResizeMatrixFunc) {
	storeCallback(ih, "_IUPGO_RESIZEMATRIX_CB", f)
	iupSetCallback(uintptr(ih), "RESIZEMATRIX_CB", resizeMatrixFuncCB)
}

type ScrollingFunc func(ih Ihandle, i, j int) int

var scrollingFuncCB = purego.NewCallback(func(ih uintptr, i int32, j int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_SCROLLING_CB").(ScrollingFunc); ok {
		return f(Ihandle(ih), int(i), int(j))
	}
	return 0
})

func setScrollingFunc(ih Ihandle, f ScrollingFunc) {
	storeCallback(ih, "_IUPGO_SCROLLING_CB", f)
	iupSetCallback(uintptr(ih), "SCROLLING_CB", scrollingFuncCB)
}

type ScrollTopFunc func(ih Ihandle, lin, col int) int

var scrollTopFuncCB = purego.NewCallback(func(ih uintptr, lin int32, col int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_SCROLLTOP_CB").(ScrollTopFunc); ok {
		return f(Ihandle(ih), int(lin), int(col))
	}
	return 0
})

func setScrollTopFunc(ih Ihandle, f ScrollTopFunc) {
	storeCallback(ih, "_IUPGO_SCROLLTOP_CB", f)
	iupSetCallback(uintptr(ih), "SCROLLTOP_CB", scrollTopFuncCB)
}

type SortColumnCompareFunc func(ih Ihandle, lin1, lin2, col int) int

var sortColumnCompareFuncCB = purego.NewCallback(func(ih uintptr, lin1 int32, lin2 int32, col int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_SORTCOLUMNCOMPARE_CB").(SortColumnCompareFunc); ok {
		return f(Ihandle(ih), int(lin1), int(lin2), int(col))
	}
	return 0
})

func setSortColumnCompareFunc(ih Ihandle, f SortColumnCompareFunc) {
	storeCallback(ih, "_IUPGO_SORTCOLUMNCOMPARE_CB", f)
	iupSetCallback(uintptr(ih), "SORTCOLUMNCOMPARE_CB", sortColumnCompareFuncCB)
}

type ValueEditFunc func(ih Ihandle, lin, col int, newval string) int

var valueEditFuncCB = purego.NewCallback(func(ih uintptr, lin int32, col int32, newval uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_VALUE_EDIT_CB").(ValueEditFunc); ok {
		return f(Ihandle(ih), int(lin), int(col), goString(newval))
	}
	return 0
})

func setValueEditFunc(ih Ihandle, f ValueEditFunc) {
	storeCallback(ih, "_IUPGO_VALUE_EDIT_CB", f)
	iupSetCallback(uintptr(ih), "VALUE_EDIT_CB", valueEditFuncCB)
}

type VSpanFunc func(ih Ihandle, i, j int) int

var vSpanFuncCB = purego.NewCallback(func(ih uintptr, i int32, j int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_VSPAN_CB").(VSpanFunc); ok {
		return f(Ihandle(ih), int(i), int(j))
	}
	return 0
})

func setVSpanFunc(ih Ihandle, f VSpanFunc) {
	storeCallback(ih, "_IUPGO_VSPAN_CB", f)
	iupSetCallback(uintptr(ih), "VSPAN_CB", vSpanFuncCB)
}

type WidthFunc func(ih Ihandle, j int) int

var widthFuncCB = purego.NewCallback(func(ih uintptr, j int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_WIDTH_CB").(WidthFunc); ok {
		return f(Ihandle(ih), int(j))
	}
	return 0
})

func setWidthFunc(ih Ihandle, f WidthFunc) {
	storeCallback(ih, "_IUPGO_WIDTH_CB", f)
	iupSetCallback(uintptr(ih), "WIDTH_CB", widthFuncCB)
}

type BgColorFunc func(ih Ihandle, lin, col int) (r, g, b int, ret int)

var bgColorCB = purego.NewCallback(func(ih uintptr, lin, col int32, r, g, b *int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_BGCOLOR_CB").(BgColorFunc); ok {
		rr, gg, bb, ret := f(Ihandle(ih), int(lin), int(col))
		*r, *g, *b = int32(rr), int32(gg), int32(bb)
		return ret
	}
	return 0
})

func setBgColorFunc(ih Ihandle, f BgColorFunc) {
	storeCallback(ih, "_IUPGO_BGCOLOR_CB", f)
	iupSetCallback(uintptr(ih), "BGCOLOR_CB", bgColorCB)
}

type FgColorFunc func(ih Ihandle, lin, col int) (r, g, b int, ret int)

var fgColorCB = purego.NewCallback(func(ih uintptr, lin, col int32, r, g, b *int32) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_FGCOLOR_CB").(FgColorFunc); ok {
		rr, gg, bb, ret := f(Ihandle(ih), int(lin), int(col))
		*r, *g, *b = int32(rr), int32(gg), int32(bb)
		return ret
	}
	return 0
})

func setFgColorFunc(ih Ihandle, f FgColorFunc) {
	storeCallback(ih, "_IUPGO_FGCOLOR_CB", f)
	iupSetCallback(uintptr(ih), "FGCOLOR_CB", fgColorCB)
}

type MatrixValueFunc func(ih Ihandle, lin, col int) string

var matrixValueCB = purego.NewCallback(func(ih uintptr, lin, col int32) uintptr {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_VALUE_CB").(MatrixValueFunc); ok {
		return cReturnStr(f(Ihandle(ih), int(lin), int(col)))
	}
	return 0
})

func setMatrixValueFunc(ih Ihandle, f MatrixValueFunc) {
	storeCallback(ih, "_IUPGO_VALUE_CB", f)
	iupSetCallback(uintptr(ih), "VALUE_CB", matrixValueCB)
}

type MatrixTypeFunc func(ih Ihandle, lin, col int) string

var matrixTypeCB = purego.NewCallback(func(ih uintptr, lin, col int32) uintptr {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_TYPE_CB").(MatrixTypeFunc); ok {
		return cReturnStr(f(Ihandle(ih), int(lin), int(col)))
	}
	return 0
})

func setMatrixTypeFunc(ih Ihandle, f MatrixTypeFunc) {
	storeCallback(ih, "_IUPGO_TYPE_CB", f)
	iupSetCallback(uintptr(ih), "TYPE_CB", matrixTypeCB)
}

type MatrixFontFunc func(ih Ihandle, lin, col int) string

var matrixFontCB = purego.NewCallback(func(ih uintptr, lin, col int32) uintptr {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_FONT_CB").(MatrixFontFunc); ok {
		return cReturnStr(f(Ihandle(ih), int(lin), int(col)))
	}
	return 0
})

func setMatrixFontFunc(ih Ihandle, f MatrixFontFunc) {
	storeCallback(ih, "_IUPGO_FONT_CB", f)
	iupSetCallback(uintptr(ih), "FONT_CB", matrixFontCB)
}

type TranslateValueFunc func(ih Ihandle, lin, col int, value string) string

var translateValueCB = purego.NewCallback(func(ih uintptr, lin, col int32, value uintptr) uintptr {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_TRANSLATEVALUE_CB").(TranslateValueFunc); ok {
		return cReturnStr(f(Ihandle(ih), int(lin), int(col), goString(value)))
	}
	return 0
})

func setTranslateValueFunc(ih Ihandle, f TranslateValueFunc) {
	storeCallback(ih, "_IUPGO_TRANSLATEVALUE_CB", f)
	iupSetCallback(uintptr(ih), "TRANSLATEVALUE_CB", translateValueCB)
}

type NumericGetValueFunc func(ih Ihandle, lin, col int) float64

// NUMERICGETVALUE_CB returns a C double; purego.NewCallback cannot return a
// floating-point value (result comes back in an integer register), so the
// native callback cannot be installed without cgo. Store-only no-op.
func setNumericGetValueFunc(ih Ihandle, f NumericGetValueFunc) {
	storeCallback(ih, "_IUPGO_NUMERICGETVALUE_CB", f)
}
