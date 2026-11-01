package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

const (
	numRows = 120
	margin  = 48
	rowH    = 22
	headerH = 70
	footerH = 30
)

var (
	columns     = []string{"ID", "Item", "Quantity", "Price"}
	columnX     = []float64{0, 0.12, 0.62, 0.82}
	rowsPerPage = 1
	pageCount   = 1
)

func init() { iup.EntryPoint(main) }

func cell(row, col int) string {
	switch col {
	case 0:
		return fmt.Sprintf("%d", row+1)
	case 1:
		return fmt.Sprintf("Item %03d", row+1)
	case 2:
		return fmt.Sprintf("%d", (row*7)%50+1)
	default:
		return fmt.Sprintf("%.2f", float64((row*37)%1000)/10+1)
	}
}

func main() {
	iup.Open()
	defer iup.Close()

	table := iup.Table().SetAttributes("VISIBLELINES=12, EXPAND=YES")
	table.SetAttribute("NUMCOL", len(columns))
	for col, title := range columns {
		table.SetAttribute(fmt.Sprintf("TITLE%d", col+1), title)
	}
	table.SetAttribute("NUMLIN", numRows)
	for row := 0; row < numRows; row++ {
		for col := range columns {
			iup.SetAttributeId2(table, "", row+1, col+1, cell(row, col))
		}
	}

	chart := iup.Canvas().SetAttributes("RASTERSIZE=x160, EXPAND=HORIZONTAL, BORDER=NO")
	chart.SetHandle("chart")
	chart.SetCallback("ACTION", iup.ActionFunc(drawChart))
	table.SetHandle("inventory")

	status := iup.Label("").SetAttributes("EXPAND=HORIZONTAL")

	btnPrint := iup.Button("Print...").SetAttributes("PADDING=10x4")
	btnPrint.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		runPrint(ih, status, func(dlg iup.Ihandle) {
			dlg.SetCallback("PAGINATE_CB", iup.PaginateFunc(paginate))
			dlg.SetCallback("PRINTPAGE_CB", iup.PrintPageFunc(printPage))
		})
		return iup.DEFAULT
	}))

	btnTable := iup.Button("Print Table").SetAttributes("PADDING=10x4")
	btnTable.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		runPrint(ih, status, func(dlg iup.Ihandle) {
			dlg.SetAttribute("ELEMENT", "inventory")
		})
		return iup.DEFAULT
	}))

	btnChart := iup.Button("Print Chart").SetAttributes("PADDING=10x4")
	btnChart.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		runPrint(ih, status, func(dlg iup.Ihandle) {
			dlg.SetAttribute("ELEMENT", "chart")
		})
		return iup.DEFAULT
	}))

	dlg := iup.Dialog(
		iup.Vbox(
			table,
			chart,
			iup.Hbox(btnPrint, btnTable, btnChart, status).SetAttributes("ALIGNMENT=ACENTER, GAP=10"),
		).SetAttributes("MARGIN=10x10, GAP=8"),
	).SetAttribute("TITLE", "Print Dialog")

	iup.Show(dlg)
	iup.MainLoop()
}

func runPrint(ih, status iup.Ihandle, setup func(dlg iup.Ihandle)) {
	dlg := iup.PrintDlg()
	defer dlg.Destroy()

	dlg.SetAttribute("TITLE", "Print Inventory")
	iup.SetAttributeHandle(dlg, "PARENTDIALOG", iup.GetDialog(ih))
	setup(dlg)

	iup.Popup(dlg, iup.CURRENT, iup.CURRENT)

	switch dlg.GetAttribute("STATUS") {
	case "1":
		status.SetAttribute("TITLE", fmt.Sprintf("Printed (%s, %s)", dlg.GetAttribute("PAGESIZE"), dlg.GetAttribute("ORIENTATION")))
	case "-1":
		status.SetAttribute("TITLE", "Printing failed or is not supported")
	default:
		status.SetAttribute("TITLE", "Canceled")
	}
}

func drawChart(ih iup.Ihandle) int {
	iup.DrawBegin(ih)
	defer iup.DrawEnd(ih)

	w, h := iup.DrawGetSize(ih)
	ih.SetAttribute("DRAWCOLOR", "255 255 255")
	ih.SetAttribute("DRAWSTYLE", "FILL")
	iup.DrawRectangle(ih, 0, 0, w-1, h-1)

	ih.SetAttribute("DRAWCOLOR", "0 0 0")
	ih.SetAttribute("DRAWFONT", "Sans, Bold 10")
	iup.DrawText(ih, "Quantity of the first 30 items", 8, 4, w-16, 20)

	bars := 30
	top, bottom := 28, h-8
	barW := (w - 16) / bars
	for i := 0; i < bars; i++ {
		var quantity int
		fmt.Sscanf(cell(i, 2), "%d", &quantity)
		x := 8 + i*barW
		y := bottom - (bottom-top)*quantity/50
		ih.SetAttribute("DRAWCOLOR", "70 120 200")
		iup.DrawRectangle(ih, x+2, y, x+barW-2, bottom)
	}

	ih.SetAttribute("DRAWCOLOR", "90 90 90")
	ih.SetAttribute("DRAWSTYLE", "STROKE")
	iup.DrawLine(ih, 8, bottom, w-8, bottom)
	return iup.DEFAULT
}

func paginate(ih iup.Ihandle, width, height int) int {
	rowsPerPage = (height - 2*margin - headerH - footerH) / rowH
	if rowsPerPage < 1 {
		rowsPerPage = 1
	}
	pageCount = (numRows + rowsPerPage - 1) / rowsPerPage
	return pageCount
}

func printPage(ih iup.Ihandle, page, width, height int) int {
	iup.DrawBegin(ih)
	defer iup.DrawEnd(ih)

	left, right := margin, width-margin
	tableW := right - left
	y := margin

	ih.SetAttribute("DRAWCOLOR", "0 0 0")
	ih.SetAttribute("DRAWFONT", "Sans, Bold 16")
	iup.DrawText(ih, "Inventory Report", left, y, tableW, 30)
	y += 40

	ih.SetAttribute("DRAWFONT", "Sans, Bold 10")
	for col, title := range columns {
		iup.DrawText(ih, title, left+int(columnX[col]*float64(tableW)), y, -1, rowH)
	}
	y += rowH + 4
	iup.DrawLine(ih, left, y-2, right, y-2)

	ih.SetAttribute("DRAWFONT", "Sans, 10")
	first := (page - 1) * rowsPerPage
	for row := first; row < first+rowsPerPage && row < numRows; row++ {
		if row%2 == 1 {
			ih.SetAttribute("DRAWCOLOR", "235 238 244")
			ih.SetAttribute("DRAWSTYLE", "FILL")
			iup.DrawRectangle(ih, left, y, right, y+rowH-1)
			ih.SetAttribute("DRAWSTYLE", "STROKE")
			ih.SetAttribute("DRAWCOLOR", "0 0 0")
		}
		for col := range columns {
			iup.DrawText(ih, cell(row, col), left+int(columnX[col]*float64(tableW))+4, y+3, -1, rowH)
		}
		y += rowH
	}

	ih.SetAttribute("DRAWCOLOR", "90 90 90")
	ih.SetAttribute("DRAWTEXTALIGNMENT", "ACENTER")
	iup.DrawText(ih, fmt.Sprintf("Page %d of %d", page, pageCount), left, height-margin-footerH+10, tableW, footerH)
	ih.SetAttribute("DRAWTEXTALIGNMENT", "ALEFT")

	return iup.DEFAULT
}
