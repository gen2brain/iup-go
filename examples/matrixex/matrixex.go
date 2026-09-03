//go:build ctrl

package main

import (
	"fmt"
	"os"
	"path/filepath"

	"github.com/gen2brain/iup-go/iup"
)

var weights = map[int]float64{1: 72.5, 2: 8.25, 3: 1450, 4: 0.9, 5: 310}

func status(format string, args ...any) {
	text := fmt.Sprintf(format, args...)
	fmt.Println(text)
	iup.GetHandle("status").SetAttribute("TITLE", text)
}

func numericGetValueCb(ih iup.Ihandle, lin, col int) float64 {
	return weights[lin]
}

func numericSetValueCb(ih iup.Ihandle, lin, col int, value float64) int {
	weights[lin] = value
	status("NUMERICSETVALUE_CB line %d = %g kg", lin, value)
	return iup.DEFAULT
}

func sortCompareCb(ih iup.Ihandle, col, lin1, lin2 int) int {
	a := len(iup.GetAttributeId2(ih, "", lin1, col))
	b := len(iup.GetAttributeId2(ih, "", lin2, col))
	if a < b {
		return -1
	}
	if a > b {
		return 1
	}
	return 0
}

func busyCb(ih iup.Ihandle, status_, count int, name string) int {
	status("BUSY_CB status=%d count=%d %s", status_, count, name)
	return iup.DEFAULT
}

func pasteSizeCb(ih iup.Ihandle, numlin, numcol int) int {
	status("PASTESIZE_CB grows to %dx%d", numlin, numcol)
	ih.SetAttribute("NUMLIN", fmt.Sprint(numlin))
	ih.SetAttribute("NUMCOL", fmt.Sprint(numcol))
	return iup.DEFAULT
}

func menuContextCb(ih, menu iup.Ihandle, lin, col int) int {
	iup.Append(menu, iup.MenuSeparator())
	iup.Append(menu, iup.MenuItem(fmt.Sprintf("Clear %d:%d", lin, col)).SetCallback("ACTION", iup.ActionFunc(func(item iup.Ihandle) int {
		iup.SetAttributeId2(ih, "", lin, col, "")
		ih.SetAttribute("REDRAW", "ALL")
		return iup.DEFAULT
	})))
	return iup.DEFAULT
}

func menuContextCloseCb(ih, menu iup.Ihandle, lin, col int) int {
	status("MENUCONTEXTCLOSE_CB %d:%d", lin, col)
	return iup.DEFAULT
}

func createMatrix() iup.Ihandle {
	mat := iup.MatrixEx()
	iup.SetHandle("matrix", mat)

	mat.SetAttribute("NUMLIN", "5")
	mat.SetAttribute("NUMCOL", "4")
	mat.SetAttribute("NUMLIN_VISIBLE", "6")
	mat.SetAttribute("RESIZEMATRIX", "YES")
	mat.SetAttribute("MARKMODE", "CELL")
	mat.SetAttribute("MARKMULTIPLE", "YES")
	mat.SetAttribute("UNDOREDO", "YES")
	mat.SetAttribute("BUSYPROGRESS", "YES")
	mat.SetAttribute("MENUCONTEXT", "YES")
	mat.SetAttribute("FREEZECOLOR", "200 0 0")

	mat.SetAttribute("0:1", "Item")
	mat.SetAttribute("0:2", "Category")
	mat.SetAttribute("0:4", "Notes")
	for lin, item := range []string{"Bicycle", "Cat", "Piano", "Book", "Motorbike"} {
		mat.SetAttribute(fmt.Sprintf("%d:0", lin+1), fmt.Sprint(lin+1))
		mat.SetAttribute(fmt.Sprintf("%d:1", lin+1), item)
	}
	for lin, category := range []string{"vehicle", "animal", "instrument", "paper", "Vehicle"} {
		mat.SetAttribute(fmt.Sprintf("%d:2", lin+1), category)
	}

	mat.SetAttribute("NUMERICADDQUANTITY", "Weight")
	mat.SetAttribute("NUMERICADDUNIT", "kilogram")
	mat.SetAttribute("NUMERICADDUNITSYMBOL", "kg")
	mat.SetAttribute("NUMERICADDUNIT", "gram")
	mat.SetAttribute("NUMERICADDUNITFACTOR", "0.001")
	mat.SetAttribute("NUMERICADDUNITSYMBOL", "g")
	mat.SetAttribute("NUMERICADDUNIT", "pound")
	mat.SetAttribute("NUMERICADDUNITFACTOR", "0.45359237")
	mat.SetAttribute("NUMERICADDUNITSYMBOL", "lb")
	mat.SetAttribute("NUMERICUNITSPELL", "AMERICAN")
	mat.SetAttribute("NUMERICFORMATDEF", "%.1f")
	mat.SetAttribute("NUMERICQUANTITY3", "Weight")
	mat.SetAttribute("NUMERICUNIT3", "kilogram")
	mat.SetAttribute("NUMERICUNITSHOWN3", "pound")
	mat.SetAttribute("NUMERICFORMATPRECISION3", "2")
	mat.SetAttribute("NUMERICFORMATTITLE3", "%s (%s)")
	mat.SetAttribute("0:3", "Weight")

	mat.SetCallback("NUMERICGETVALUE_CB", iup.NumericGetValueFunc(numericGetValueCb))
	mat.SetCallback("NUMERICSETVALUE_CB", iup.NumericSetValueFunc(numericSetValueCb))
	mat.SetCallback("SORTCOLUMNCOMPARE_CB", iup.SortColumnCompareFunc(sortCompareCb))
	mat.SetCallback("BUSY_CB", iup.BusyFunc(busyCb))
	mat.SetCallback("PASTESIZE_CB", iup.PasteSizeFunc(pasteSizeCb))
	mat.SetCallback("MENUCONTEXT_CB", iup.MenuContextFunc(menuContextCb))
	mat.SetCallback("MENUCONTEXTCLOSE_CB", iup.MenuContextCloseFunc(menuContextCloseCb))

	return mat
}

func button(title string, action func(mat iup.Ihandle)) iup.Ihandle {
	return iup.Button(title).SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		action(iup.GetHandle("matrix"))
		return iup.DEFAULT
	}))
}

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	iup.ControlsOpen()
	defer iup.Close()

	mat := createMatrix()
	find := iup.Text().SetAttributes("VALUE=cat, VISIBLECOLUMNS=10")
	dialogs := iup.List().SetAttributes("DROPDOWN=YES, 1=FIND, 2=GOTO, 3=SORT, 4=SETTINGS, 5=EXPORT_TXT, 6=EXPORT_HTML, 7=IMPORT_TXT, 8=UNDOLIST, 9=COPYCOLTO_INTERVAL, VALUE=1")
	exportFile := filepath.Join(os.TempDir(), "matrixex.txt")

	status_ := iup.Label("").SetAttribute("EXPAND", "HORIZONTAL")
	status_.SetHandle("status")

	row1 := iup.Hbox(
		find,
		button("Find", func(mat iup.Ihandle) {
			mat.SetAttribute("FINDMATCHCASE", "NO")
			mat.SetAttribute("FINDMATCHWHOLECELL", "NO")
			mat.SetAttribute("FINDDIRECTION", "BOTTOMRIGHT")
			mat.SetAttribute("FIND", find.GetAttribute("VALUE"))
			status("FIND %q -> FOCUSCELL=%s", mat.GetAttribute("FIND"), mat.GetAttribute("FOCUSCELL"))
		}),
		button("Freeze 1:1", func(mat iup.Ihandle) { mat.SetAttribute("FREEZE", "1:1") }),
		button("Unfreeze", func(mat iup.Ihandle) { mat.SetAttribute("FREEZE", "NO") }),
		button("Sort by length", func(mat iup.Ihandle) {
			mat.SetAttribute("SORTCOLUMNORDER", "ASCENDING")
			mat.SetAttribute("SORTCOLUMN1", "ALL")
			status("LASTSORTCOLUMN=%s SORTSIGN1=%s", mat.GetAttribute("LASTSORTCOLUMN"), mat.GetAttribute("SORTSIGN1"))
		}),
		button("Sort category", func(mat iup.Ihandle) {
			mat.SetAttribute("SORTCOLUMNCASESENSITIVE", "NO")
			mat.SetAttribute("SORTCOLUMNORDER", "DESCENDING")
			mat.SetAttribute("SORTCOLUMNINTERVAL", "1-5")
			mat.SetAttribute("SORTCOLUMN2", "1-5")
			status("LASTSORTCOLUMN=%s SORTLINEINDEX1=%s", mat.GetAttribute("LASTSORTCOLUMN"), mat.GetAttribute("SORTLINEINDEX1"))
		}),
		button("Unsort", func(mat iup.Ihandle) { mat.SetAttribute("SORTCOLUMN", "RESET") }),
	).SetAttributes("NGAP=5")

	row2 := iup.Hbox(
		button("Copy marked", func(mat iup.Ihandle) { mat.SetAttribute("COPY", "MARKED") }),
		button("Paste at focus", func(mat iup.Ihandle) { mat.SetAttribute("PASTE", "FOCUS") }),
		button("Copy 1:2 down", func(mat iup.Ihandle) { mat.SetAttribute("COPYCOLTO1:2", "BOTTOM") }),
		button("Buffer round trip", func(mat iup.Ihandle) {
			mat.SetAttribute("COPYDATA", "ALL")
			data := mat.GetAttribute("COPYDATA")
			mat.SetAttribute("FOCUSCELL", "1:1")
			mat.SetAttribute("PASTEDATA", data)
			status("COPYDATA %d bytes pasted back", len(data))
		}),
		button("Export", func(mat iup.Ihandle) {
			mat.SetAttribute("FILEFORMAT", "TXT")
			mat.SetAttribute("SKIPLINES", "1")
			mat.SetAttribute("COPYFILE", exportFile)
			status("COPYFILE %s LASTERROR=%s", exportFile, mat.GetAttribute("LASTERROR"))
		}),
		button("Import", func(mat iup.Ihandle) {
			mat.SetAttribute("PASTEFILEAT", "2:1")
			mat.SetAttribute("PASTEFILE", exportFile)
			status("PASTEFILE LASTERROR=%s", mat.GetAttribute("LASTERROR"))
		}),
	).SetAttributes("NGAP=5")

	row3 := iup.Hbox(
		button("Undo", func(mat iup.Ihandle) {
			mat.SetAttribute("UNDO", "1")
			status("UNDOCOUNT=%s", mat.GetAttribute("UNDOCOUNT"))
		}),
		button("Redo", func(mat iup.Ihandle) { mat.SetAttribute("REDO", "1") }),
		button("Undo list", func(mat iup.Ihandle) {
			count := mat.GetInt("UNDOCOUNT")
			names := ""
			for i := 0; i < count; i++ {
				names += " " + iup.GetAttributeId(mat, "UNDONAME", i)
			}
			status("UNDOCOUNT=%d:%s", count, names)
		}),
		button("Clear undo", func(mat iup.Ihandle) { mat.SetAttribute("UNDOCLEAR", "YES") }),
		button("Hide line 3", func(mat iup.Ihandle) { mat.SetAttribute("VISIBLELIN3", "NO") }),
		button("Show line 3", func(mat iup.Ihandle) { mat.SetAttribute("VISIBLELIN3", "YES") }),
		button("Toggle col 4", func(mat iup.Ihandle) {
			visible := mat.GetAttribute("VISIBLECOL4") == "YES"
			mat.SetAttribute("VISIBLECOL4", map[bool]string{true: "NO", false: "YES"}[visible])
		}),
		button("Context 2:2", func(mat iup.Ihandle) {
			var x, y int
			fmt.Sscanf(mat.GetAttribute("CELLOFFSET2:2"), "%dx%d", &x, &y)
			iup.SetAttributeId2(mat, "SHOWMENUCONTEXT", 2, 2, fmt.Sprintf("%d,%d", mat.GetInt("X")+x, mat.GetInt("Y")+y))
		}),
		dialogs,
		button("Show dialog", func(mat iup.Ihandle) {
			mat.SetAttribute("SHOWDIALOG", dialogs.GetAttribute(dialogs.GetAttribute("VALUE")))
		}),
	).SetAttributes("NGAP=5")

	dlg := iup.Dialog(iup.Vbox(row1, row2, row3, mat, status_).SetAttributes("NMARGIN=10x10, NGAP=5"))
	dlg.SetAttribute("TITLE", "IupMatrixEx")

	iup.Show(dlg)
	status("Units: NUMERICUNITCOUNT3=%s shown as %s (%s)", mat.GetAttribute("NUMERICUNITCOUNT3"), mat.GetAttribute("NUMERICUNITSHOWN3"), mat.GetAttribute("NUMERICUNITSYMBOLSHOWN3"))
	iup.MainLoop()
}
