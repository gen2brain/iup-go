//go:build ctl

package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func nlinesCb(ih iup.Ihandle) int {
	return 8
}

func ncolsCb(ih iup.Ihandle) int {
	return 8
}

func heightCb(ih iup.Ihandle, i int) int {
	return 50
}

func widthCb(ih iup.Ihandle, j int) int {
	return 50
}

func drawCb(ih iup.Ihandle, i, j, xmin, xmax, ymin, ymax int) int {
	// Checkboard pattern: alternate black and white squares
	if ((i%2 == 1) && (j%2 == 1)) || ((i%2 == 0) && (j%2 == 0)) {
		ih.SetAttribute("DRAWCOLOR", "255 255 255") // White
	} else {
		ih.SetAttribute("DRAWCOLOR", "0 0 0") // Black
	}

	// Draw filled rectangle
	ih.SetAttribute("DRAWSTYLE", "FILL")
	iup.DrawRectangle(ih, xmin, ymin, xmax, ymax)

	return iup.DEFAULT
}

func main() {
	iup.Open()
	iup.ControlsOpen()
	defer iup.Close()

	cells := iup.Cells()

	cells.SetCallback("DRAW_CB", iup.CellsDrawFunc(drawCb))
	cells.SetCallback("WIDTH_CB", iup.WidthFunc(widthCb))
	cells.SetCallback("HEIGHT_CB", iup.HeightFunc(heightCb))
	cells.SetCallback("NLINES_CB", iup.NLinesFunc(nlinesCb))
	cells.SetCallback("NCOLS_CB", iup.NColsFunc(ncolsCb))

	dlg := iup.Dialog(cells)
	dlg.SetAttribute("TITLE", "IupCells - Checkboard")
	dlg.SetAttribute("RASTERSIZE", "440x480")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}
