//go:build ctrl

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

func init() { iup.EntryPoint(main) }

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
	cells.SetAttributes("BOXED=NO, CLIPPED=YES, NON_SCROLLABLE_LINES=1, NON_SCROLLABLE_COLS=1")
	cells.SetCallback("HSPAN_CB", iup.HSpanFunc(func(ih iup.Ihandle, i, j int) int {
		if i == 2 && j == 2 {
			return 3
		}
		return 1
	}))
	cells.SetCallback("VSPAN_CB", iup.VSpanFunc(func(ih iup.Ihandle, i, j int) int {
		if i == 5 && j == 1 {
			return 2
		}
		return 1
	}))
	cells.SetCallback("SCROLLING_CB", iup.ScrollingFunc(func(ih iup.Ihandle, i, j int) int {
		fmt.Printf("SCROLLING_CB %d:%d FIRST_LINE=%s FIRST_COL=%s\n", i, j, ih.GetAttribute("FIRST_LINE"), ih.GetAttribute("FIRST_COL"))
		return iup.DEFAULT
	}))
	cells.SetCallback("MOUSEMOTION_CB", iup.MouseMotionFunc(func(ih iup.Ihandle, i, j, x, y int, status string) int {
		fmt.Printf("MOUSEMOTION_CB %d:%d at %d,%d LIMITS=%s\n", i, j, x, y, iup.GetAttributeId2(ih, "LIMITS", i, j))
		return iup.DEFAULT
	}))

	show := iup.Button("FULL_VISIBLE=15:40")
	show.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		cells.SetAttribute("FULL_VISIBLE", "15:40")
		return iup.DEFAULT
	}))
	boxed := iup.Button("BUFFERIZE, toggle BOXED, then REPAINT")
	boxed.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		cells.SetAttribute("BUFFERIZE", "YES")
		if cells.GetAttribute("BOXED") == "YES" {
			cells.SetAttribute("BOXED", "NO")
		} else {
			cells.SetAttribute("BOXED", "YES")
		}
		cells.SetAttribute("BUFFERIZE", "NO")
		cells.SetAttribute("REPAINT", "YES")
		return iup.DEFAULT
	}))

	dlg := iup.Dialog(iup.Vbox(cells, iup.Hbox(show, boxed).SetAttribute("NGAP", "5")).SetAttributes("NMARGIN=5x5, NGAP=5"))
	dlg.SetAttribute("RASTERSIZE", "500x500")
	dlg.SetAttribute("TITLE", "IupCells - Numbering")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}
