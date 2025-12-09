//go:build ctl

package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func leaveItemCb(ih iup.Ihandle, lin, col int) int {
	fmt.Printf("leaveItemCb(%d, %d)\n", lin, col)
	// if lin == 3 && col == 2 {
	// 	return iup.IGNORE // notice that this will lock the matrix in this cell
	// }
	return iup.DEFAULT
}

func valueCb(ih iup.Ihandle, lin, col int) string {
	return fmt.Sprintf("%d-%d", lin, col)
}

func enterItemCb(ih iup.Ihandle, lin, col int) int {
	fmt.Printf("enterItemCb(%d, %d)\n", lin, col)
	if lin == 2 && col == 2 {
		iup.GetHandle("mat1").SetAttribute("REDRAW", "ALL")
		iup.GetHandle("mat2").SetAttribute("REDRAW", "ALL")
		// iup.GetHandle("mat3").SetAttribute("REDRAW", "ALL")
		// iup.GetHandle("mat4").SetAttribute("REDRAW", "ALL")
		// iup.GetHandle("mat5").SetAttribute("REDRAW", "ALL")
		// iup.GetHandle("mat6").SetAttribute("REDRAW", "ALL")
	}
	return iup.DEFAULT
}

func dropSelectCb(ih iup.Ihandle, lin, col int, drop iup.Ihandle, t string, i, v int) int {
	fmt.Printf("dropSelectCb(%d, %d, %s, i=%d v=%d)\n", lin, col, t, i, v)
	return iup.CONTINUE
	// return iup.DEFAULT
}

func dropCheckCb(ih iup.Ihandle, lin, col int) int {
	if lin == 1 && col == 1 {
		return iup.DEFAULT
	}
	return iup.IGNORE
}

func clickCb(ih iup.Ihandle, lin, col int, status string) int {
	value := iup.GetAttribute(ih, fmt.Sprintf("%d:%d", lin, col))
	if value == "" {
		value = "NULL"
	}
	fmt.Printf("click_cb(%d, %d)\n", lin, col)
	fmt.Printf("  VALUE%d:%d = %s\n", lin, col, value)
	return iup.DEFAULT
}

func releaseCb(ih iup.Ihandle, lin, col int, status string) int {
	value := iup.GetAttribute(ih, fmt.Sprintf("%d:%d", lin, col))
	if value == "" {
		value = "NULL"
	}
	fmt.Printf("release_cb(%d, %d)\n", lin, col)
	fmt.Printf("  VALUE%d:%d = %s\n", lin, col, value)
	return iup.DEFAULT
}

func dropCb(ih iup.Ihandle, drop iup.Ihandle, lin, col int) int {
	fmt.Printf("dropCb(%d, %d)\n", lin, col)
	if lin == 1 && col == 1 {
		drop.SetAttribute("1", "A - Test of Very Big String for Dropdown!")
		drop.SetAttribute("2", "B")
		drop.SetAttribute("3", "C")
		drop.SetAttribute("4", "XXX")
		drop.SetAttribute("5", "5")
		drop.SetAttribute("6", "6")
		drop.SetAttribute("7", "7")
		drop.SetAttribute("8", "")
		drop.SetAttribute("VALUE", "")
		return iup.DEFAULT
	}
	return iup.IGNORE
}

func editionCb(ih iup.Ihandle, lin, col, mode, update int) int {
	fmt.Printf("editionCb(lin=%d, col=%d, mode=%d, update=%d)\n", lin, col, mode, update)
	if mode == 1 {
		ih.SetAttribute("CARET", "3")

		if lin == 3 && col == 2 {
			return iup.IGNORE
		}
	}

	// if lin == 1 && col == 1 && mode == 0 && mdrop == 1 {
	// 	mdrop = 0
	// 	ih.SetAttribute("EDITMODE", "NO")
	// 	ih.SetAttribute("EDITMODE", "YES")
	// 	return iup.IGNORE
	// }

	return iup.DEFAULT
}

func drawCb(ih iup.Ihandle, lin, col, xmin, xmax, ymin, ymax int) int {
	if lin < 5 || lin > 12 || col < 2 || col > 8 {
		return iup.IGNORE
	}

	// Draw red X across the cell
	ih.SetAttribute("DRAWCOLOR", "255 0 0")
	ih.SetAttribute("DRAWSTYLE", "STROKE")
	ih.SetAttribute("DRAWLINEWIDTH", "1")
	iup.DrawLine(ih, xmin, ymin, xmax, ymax)
	iup.DrawLine(ih, xmin, ymax, xmax, ymin)

	// Draw cell coordinates centered
	text := fmt.Sprintf("%d:%d", lin, col)
	ih.SetAttribute("DRAWCOLOR", "255 0 0")

	// Manual text centering (same as cells_numbering)
	txtW, txtH := iup.DrawGetTextSize(ih, text)
	textX := xmin + (xmax-xmin-txtW)/2
	textY := ymin + (ymax-ymin-txtH)/2

	iup.DrawText(ih, text, textX, textY, 0, 0)

	return iup.DEFAULT
}

func actionCb(ih iup.Ihandle, key, lin, col, edition int, status string) int {
	fmt.Printf("action_cb(key=%d, lin=%d, col=%d, edition=%d, status=%s)\n", key, lin, col, edition, status)

	if lin == 2 && col == 3 && edition == 1 && status != "" {
		str := status + "xxx"
		ih.SetAttribute("VALUE", str)
		ih.SetAttribute("CARET", "1")
		ih.SetAttribute("REDRAW", "ALL")
	}

	/*
		// Example of key filtering
		if key == 45 || (key >= 48 && key <= 57) || (key >= 65 && key <= 90) ||
			(key >= 97 && key <= 122) || key == 95 || key == 8 || key > 255 || key == 13 {
			fmt.Println("DEFAULT")
			return iup.DEFAULT
		}

		fmt.Println("IGNORE")
		return iup.IGNORE
	*/

	return iup.DEFAULT
}

func createMat(mati int) iup.Ihandle {
	name := fmt.Sprintf("mat%d", mati)

	var mat iup.Ihandle
	if mati == 1 {
		mat = iup.MatrixEx()
		mat.SetAttribute("UNDOREDO", "Yes")
	} else {
		mat = iup.Matrix("")
	}

	iup.SetHandle(name, mat)

	mat.SetAttribute("NUMCOL", "15")
	mat.SetAttribute("NUMLIN", "18")

	mat.SetAttribute("NUMCOL_VISIBLE", "5")
	mat.SetAttribute("NUMLIN_VISIBLE", "8")

	// mat.SetAttribute("EXPAND", "NO")
	// mat.SetAttribute("SCROLLBAR", "NO")
	mat.SetAttribute("RESIZEMATRIX", "YES")

	mat.SetAttribute("MARKMODE", "CELL")
	// mat.SetAttribute("MARKMODE", "LINCOL")
	mat.SetAttribute("MARKMULTIPLE", "YES")
	mat.SetAttribute("MARKAREA", "NOT_CONTINUOUS")
	// mat.SetAttribute("MARKAREA", "CONTINUOUS")

	mat.SetAttribute("0:0", "Test")
	mat.SetAttribute("1:0", "Medicine")
	mat.SetAttribute("2:0", "Food")
	mat.SetAttribute("3:0", "Energy")
	mat.SetAttribute("0:1", "January 2000")
	mat.SetAttribute("0:2", "February 2000")
	mat.SetAttribute("1:1", "5.6")
	mat.SetAttribute("2:1", "2.2")
	mat.SetAttribute("3:1", "7.2")
	mat.SetAttribute("1:2", "4.5")
	mat.SetAttribute("2:2", "8.1")
	mat.SetAttribute("3:2", "3.4 (RO)")

	mat.SetAttribute("PASTEFILEAT", "1:1")

	mat.SetAttribute("NUMERICFORMATTITLE3", "%s (%s)")
	mat.SetAttribute("NUMERICQUANTITY3", "Time")
	mat.SetAttribute("NUMERICUNIT3", "day")
	mat.SetAttribute("NUMERICUNITSHOWN3", "day")
	mat.SetAttribute("0:3", "Time")
	mat.SetAttribute("1:3", "1")
	mat.SetAttribute("2:3", "1.5")
	mat.SetAttribute("3:3", "2")
	mat.SetAttribute("NUMERICDECIMALSYMBOL", ",")

	// mat.SetAttribute("BGCOLOR1:*", "255 128 0")
	mat.SetAttribute("BGCOLOR2:1", "255 128 0")
	mat.SetAttribute("FGCOLOR2:0", "255 0 128")
	// mat.SetAttribute("BGCOLOR0:*", "255 0 128")
	mat.SetAttribute("FGCOLOR1:1", "255 0 128")
	mat.SetAttribute("BGCOLOR3:*", "255 128 0")
	mat.SetAttribute("BGCOLOR*:4", "255 128 0")
	// mat.SetAttribute("FONT2:*", "Times:BOLD:8")
	// mat.SetAttribute("FONT*:2", "Courier::12")
	mat.SetAttribute("SORTSIGN1", "UP")
	// mat.SetAttribute("SORTSIGN2", "DOWN")
	mat.SetAttribute("FRAMEVERTCOLOR2:2", "255 255 255")
	mat.SetAttribute("CELLBYTITLE", "Yes")

	// mat.SetAttribute("MARKAREA", "NOT_CONTINUOUS")
	// mat.SetAttribute("MARKMULTIPLE", "YES")

	mat.SetCallback("LEAVEITEM_CB", iup.LeaveItemFunc(leaveItemCb))
	mat.SetCallback("ENTERITEM_CB", iup.EnterItemFunc(enterItemCb))
	mat.SetCallback("DROPSELECT_CB", iup.DropSelectFunc(dropSelectCb))
	mat.SetCallback("DROP_CB", iup.MatrixDropFunc(dropCb))
	mat.SetCallback("DROPCHECK_CB", iup.DropCheckFunc(dropCheckCb))
	mat.SetCallback("EDITION_CB", iup.EditionFunc(editionCb))
	mat.SetCallback("CLICK_CB", iup.ClickFunc(clickCb))
	mat.SetCallback("RELEASE_CB", iup.ReleaseFunc(releaseCb))
	mat.SetCallback("DRAW_CB", iup.CellsDrawFunc(drawCb))
	mat.SetCallback("ACTION_CB", iup.MatrixActionFunc(actionCb))

	// mat.SetCallback("VALUE_CB", iup.MatrixValueFunc(valueCb))
	// mat.SetAttribute("WIDTH0", "24")
	// mat.SetAttribute("HEIGHT0", "8")

	// mat.SetAttribute("FLATSCROLLBAR", "Yes")
	// mat.SetAttribute("SHOWFLOATING", "Yes")
	// mat.SetAttribute("SHOWTRANSPARENT", "Yes")

	return mat
}

func redrawCb(ih iup.Ihandle) int {
	iup.GetHandle("mat1").SetAttribute("REDRAW", "ALL")
	iup.GetHandle("mat2").SetAttribute("REDRAW", "ALL")
	// iup.GetHandle("mat3").SetAttribute("REDRAW", "ALL")
	// iup.GetHandle("mat4").SetAttribute("REDRAW", "ALL")
	// iup.GetHandle("mat5").SetAttribute("REDRAW", "ALL")
	// iup.GetHandle("mat6").SetAttribute("REDRAW", "ALL")

	// Example: Toggle matrix visibility
	// mat := iup.GetHandle("mat1")
	// if mat.GetInt("VISIBLE") != 0 {
	// 	mat.SetAttribute("VISIBLE", "NO")
	// 	mat.SetAttribute("OLD_SIZE", mat.GetAttribute("RASTERSIZE"))
	// 	mat.SetAttribute("RASTERSIZE", "1x1")
	// } else {
	// 	mat.SetAttribute("RASTERSIZE", mat.GetAttribute("OLD_SIZE"))
	// 	mat.SetAttribute("VISIBLE", "YES")
	// }

	return iup.DEFAULT
}

func removeLineCb(ih iup.Ihandle) int {
	// iup.GetHandle("mat1").SetAttribute("ORIGIN", "5:1")
	iup.GetHandle("mat1").SetAttribute("DELLIN", "1")
	// iup.GetHandle("mat1").SetAttribute("NUMLIN", "0")
	return iup.DEFAULT
}

func addLineCb(ih iup.Ihandle) int {
	// iup.GetHandle("mat1").SetAttribute("NUMLIN", "5")
	iup.GetHandle("mat1").SetAttribute("ADDLIN", "0")
	// iup.GetHandle("mat1").SetAttribute("ADDLIN", "0-5")
	// iup.GetHandle("mat1").SetAttribute("NUMCOL_NOSCROLL", "2")
	// iup.GetHandle("mat1").SetAttribute("NUMLIN_NOSCROLL", "2")
	return iup.DEFAULT
}

func removeColCb(ih iup.Ihandle) int {
	iup.GetHandle("mat1").SetAttribute("DELCOL", "1")
	return iup.DEFAULT
}

func addColCb(ih iup.Ihandle) int {
	iup.GetHandle("mat1").SetAttribute("ADDCOL", "0")
	return iup.DEFAULT
}

func buttonCb(ih iup.Ihandle) int {
	fmt.Println("DEFAULTENTER")

	// iup.GetHandle("mat1").Hide()

	// Example: Clear various cell values
	// iup.GetHandle("mat1").SetAttribute("CLEARVALUE*:2", "1-10")
	// iup.GetHandle("mat1").SetAttribute("CLEARVALUE2:*", "1-10")
	// iup.GetHandle("mat1").SetAttribute("CLEARVALUE2:2", "10-10")
	// iup.GetHandle("mat1").SetAttribute("CLEARVALUE", "ALL")
	// iup.GetHandle("mat1").SetAttribute("CLEARVALUE", "CONTENTS")

	// Example: Clear attributes
	// iup.GetHandle("mat1").SetAttribute("CLEARATTRIB2:*", "ALL")
	// iup.GetHandle("mat1").SetAttribute("CLEARATTRIB3:*", "ALL")

	return iup.DEFAULT
}

func createMenu() {
	menu := iup.Menu(
		iup.Submenu("submenu", iup.Menu(
			iup.Item("item1"),
			iup.Item("item2"),
		)),
		iup.Item("remove line").SetCallback("ACTION", iup.ActionFunc(removeLineCb)),
		iup.Item("add line").SetCallback("ACTION", iup.ActionFunc(addLineCb)),
		iup.Item("remove col").SetCallback("ACTION", iup.ActionFunc(removeColCb)),
		iup.Item("add col").SetCallback("ACTION", iup.ActionFunc(addColCb)),
		iup.Item("redraw").SetCallback("ACTION", iup.ActionFunc(redrawCb)),
	)
	iup.SetHandle("mymenu", menu)
}

func main() {
	iup.Open()
	iup.ControlsOpen()
	defer iup.Close()

	createMenu()

	bt := iup.Button("Button")
	bt.SetCallback("ACTION", iup.ActionFunc(buttonCb))
	bt.SetAttribute("CANFOCUS", "NO")

	dlg := iup.Dialog(
		// iup.Zbox(
		iup.Tabs(
			iup.SetAttributes(
				iup.Vbox(
					createMat(1),
					bt,
					iup.Text(),
					iup.Label("Label Text"),
					iup.Frame(iup.Val("HORIZONTAL")),
				), "MARGIN=10x10, GAP=10, TABTITLE=Test1"),
			iup.SetAttributes(
				iup.Vbox(
					iup.Frame(createMat(2)),
					iup.Text(),
					iup.Label("Label Text"),
					iup.Val("HORIZONTAL"),
					// ), "BGCOLOR=\"0 255 255\", MARGIN=10x10, GAP=10, TABTITLE=Test2, FONT=HELVETICA_ITALIC_14"),
					// ), "FONT=HELVETICA_NORMAL_12, BGCOLOR=\"0 255 255\", MARGIN=10x10, GAP=10, TABTITLE=Test2"),
				), "BGCOLOR=\"0 255 255\", MARGIN=10x10, GAP=10, TABTITLE=Test2"),
		))

	dlg.SetAttribute("TITLE", "IupMatrix Callbacks")
	dlg.SetAttribute("MENU", "mymenu")
	iup.SetAttributeHandle(dlg, "DEFAULTENTER", bt)
	// dlg.SetAttribute("BGCOLOR", "255 0 255")

	// Example: Composited window with transparency
	// dlg.SetAttribute("COMPOSITED", "YES")
	// dlg.SetAttribute("OPACITY", "192")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}
