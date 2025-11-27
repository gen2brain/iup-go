//go:build ctl

package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func nlinesCb(ih iup.Ihandle) int {
	return 7
}

func ncolsCb(ih iup.Ihandle) int {
	return 7
}

func heightCb(ih iup.Ihandle, i int) int {
	return int(30 + float64(i)*1.5)
}

func widthCb(ih iup.Ihandle, j int) int {
	return int(50 + float64(j)*1.5)
}

func mouseclickCb(ih iup.Ihandle, button, pressed, i, j, x, y int, status string) int {
	fmt.Printf("CLICK: %d: (%02d, %02d)\n", button, i, j)
	return iup.DEFAULT
}

func scrollingCb(ih iup.Ihandle, i, j int) int {
	fmt.Printf("SCROLL: (%02d, %02d)\n", i, j)
	return iup.DEFAULT
}

func vspanCb(ih iup.Ihandle, i, j int) int {
	if i == 1 && j == 1 {
		return 2
	}
	if i == 5 && j == 5 {
		return 2
	}
	return 1
}

func hspanCb(ih iup.Ihandle, i, j int) int {
	if i == 1 && j == 1 {
		return 2
	}
	if i == 5 && j == 5 {
		return 2
	}
	return 1
}

func drawCb(ih iup.Ihandle, i, j, xmin, xmax, ymin, ymax int) int {
	// Skip cells that are spanned
	if i == 1 && j == 2 {
		return iup.DEFAULT
	}
	if i == 2 && j == 1 {
		return iup.DEFAULT
	}
	if i == 2 && j == 2 {
		return iup.DEFAULT
	}
	if i == 5 && j == 6 {
		return iup.DEFAULT
	}
	if i == 6 && j == 5 {
		return iup.DEFAULT
	}
	if i == 6 && j == 6 {
		return iup.DEFAULT
	}

	// Set background color with gradient effect
	if i == 1 && j == 1 {
		ih.SetAttribute("DRAWCOLOR", "255 255 255") // White
	} else {
		r := i * 20
		g := j * 100
		b := i + 100
		ih.SetAttribute("DRAWCOLOR", fmt.Sprintf("%d %d %d", r, g, b))
	}

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
	cells.SetCallback("HSPAN_CB", iup.HSpanFunc(hspanCb))
	cells.SetCallback("VSPAN_CB", iup.VSpanFunc(vspanCb))
	cells.SetCallback("SCROLLING_CB", iup.ScrollingFunc(scrollingCb))
	cells.SetAttribute("BOXED", "NO")

	box := iup.Vbox(cells)
	box.SetAttribute("MARGIN", "10x10")

	dlg := iup.Dialog(box)
	dlg.SetAttribute("TITLE", "IupCells - Degrade")
	dlg.SetAttribute("RASTERSIZE", "350x250")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}
