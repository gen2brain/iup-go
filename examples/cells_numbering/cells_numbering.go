//go:build ctl

package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func nlinesCb(ih iup.Ihandle) int {
	return 20
}

func ncolsCb(ih iup.Ihandle) int {
	return 50
}

func heightCb(ih iup.Ihandle, i int) int {
	return 30
}

func widthCb(ih iup.Ihandle, j int) int {
	return 70
}

func mouseclickCb(ih iup.Ihandle, button, pressed, i, j, x, y int, status string) int {
	msg := fmt.Sprintf("CLICK: %d: (%02d, %02d)", button, i, j)
	iup.Message("Hi!", msg)
	return iup.DEFAULT
}

func drawCb(ih iup.Ihandle, i, j, xmin, xmax, ymin, ymax int) int {
	// Set background color with gradient effect
	r := i * 20
	g := j * 100
	b := i + 100

	ih.SetAttribute("DRAWCOLOR", fmt.Sprintf("%d %d %d", r, g, b))

	// Draw filled rectangle
	ih.SetAttribute("DRAWSTYLE", "FILL")
	iup.DrawRectangle(ih, xmin, ymin, xmax, ymax)

	// Draw cell coordinates as text
	ih.SetAttribute("DRAWCOLOR", "0 0 0") // Black text
	text := fmt.Sprintf("(%02d, %02d)", i, j)

	// Calculate centered position
	txtW, txtH := iup.DrawGetTextSize(ih, text)
	textX := xmin + (xmax-xmin-txtW)/2
	textY := ymin + (ymax-ymin-txtH)/2

	iup.DrawText(ih, text, textX, textY, 0, 0)

	return iup.DEFAULT
}

func main() {
	iup.Open()
	iup.ControlsOpen()
	defer iup.Close()

	cells := iup.Cells()

	cells.SetCallback("MOUSECLICK_CB", iup.MouseClickFunc(mouseclickCb))
	cells.SetCallback("DRAW_CB", iup.CellsDrawFunc(drawCb))
	cells.SetCallback("WIDTH_CB", iup.WidthFunc(widthCb))
	cells.SetCallback("HEIGHT_CB", iup.HeightFunc(heightCb))
	cells.SetCallback("NLINES_CB", iup.NLinesFunc(nlinesCb))
	cells.SetCallback("NCOLS_CB", iup.NColsFunc(ncolsCb))
	cells.SetAttribute("BOXED", "NO")

	dlg := iup.Dialog(cells)
	dlg.SetAttribute("RASTERSIZE", "500x500")
	dlg.SetAttribute("TITLE", "IupCells - Numbering")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}
