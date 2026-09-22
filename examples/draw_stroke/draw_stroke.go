package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

const (
	cols     = 3
	rows     = 6
	cellW    = 180
	cellH    = 110
	cellPadX = 20
	cellPadY = 20
)

func cell(ih iup.Ihandle, col, row int, label string, draw func(x, y int)) {
	x := cellPadX + col*cellW
	y := cellPadY + row*cellH
	ih.SetAttribute("DRAWCOLOR", "120 120 120")
	ih.SetAttribute("DRAWFONT", "Sans, 8")
	iup.DrawText(ih, label, x, y, -1, -1)
	draw(x, y+24)
}

func reset(ih iup.Ihandle) {
	ih.SetAttribute("DRAWLINECAP", "BUTT")
	ih.SetAttribute("DRAWLINEJOIN", "MITER")
	ih.SetAttribute("DRAWDASH", "")
	ih.SetAttribute("DRAWDASHOFFSET", "0")
	ih.SetAttribute("DRAWSTYLE", "STROKE")
	ih.SetAttribute("DRAWLINEWIDTH", "12")
	ih.SetAttribute("DRAWCOLOR", "40 80 200")
}

func action(ih iup.Ihandle) int {
	iup.DrawBegin(ih)
	ih.SetAttribute("DRAWCOLOR", "255 255 255")
	ih.SetAttribute("DRAWSTYLE", "FILL")
	w, h := iup.DrawGetSize(ih)
	iup.DrawRectangle(ih, 0, 0, w, h)

	for i, lineCap := range []string{"BUTT", "ROUND", "SQUARE"} {
		cell(ih, i, 0, "CAP "+lineCap, func(x, y int) {
			reset(ih)
			ih.SetAttribute("DRAWLINECAP", lineCap)
			iup.DrawLine(ih, x+20, y+20, x+120, y+20)
			ih.SetAttribute("DRAWCOLOR", "255 255 255")
			ih.SetAttribute("DRAWLINEWIDTH", "1")
			iup.DrawLine(ih, x+20, y+10, x+20, y+30)
			iup.DrawLine(ih, x+120, y+10, x+120, y+30)
		})
	}

	for i, lineJoin := range []string{"MITER", "ROUND", "BEVEL"} {
		cell(ih, i, 1, "JOIN "+lineJoin, func(x, y int) {
			reset(ih)
			ih.SetAttribute("DRAWLINEJOIN", lineJoin)
			iup.DrawPathBegin(ih)
			iup.DrawPathMoveTo(ih, x+20, y+40)
			iup.DrawPathLineTo(ih, x+60, y+5)
			iup.DrawPathLineTo(ih, x+100, y+40)
			iup.DrawPathStroke(ih)
		})
	}

	styles := []string{"STROKE_DASH", "STROKE_DOT", "STROKE_DASH_DOT"}
	for i, style := range styles {
		cell(ih, i, 2, style, func(x, y int) {
			reset(ih)
			ih.SetAttribute("DRAWLINEWIDTH", "3")
			ih.SetAttribute("DRAWSTYLE", style)
			iup.DrawLine(ih, x+10, y+10, x+150, y+10)
			iup.DrawRectangle(ih, x+10, y+25, x+150, y+55)
		})
	}

	dashes := []string{"20 6", "2 6", "12 4 2 4"}
	for i, dash := range dashes {
		cell(ih, i, 3, "DASH "+dash, func(x, y int) {
			reset(ih)
			ih.SetAttribute("DRAWLINEWIDTH", "4")
			ih.SetAttribute("DRAWDASH", dash)
			iup.DrawLine(ih, x+10, y+10, x+150, y+10)
			ih.SetAttribute("DRAWLINECAP", "ROUND")
			iup.DrawArc(ih, x+40, y+25, x+110, y+75, 0, 360)
		})
	}

	for i, off := range []string{"0", "7", "14"} {
		cell(ih, i, 4, "OFFSET "+off, func(x, y int) {
			reset(ih)
			ih.SetAttribute("DRAWLINEWIDTH", "4")
			ih.SetAttribute("DRAWDASH", "14 6")
			ih.SetAttribute("DRAWDASHOFFSET", off)
			iup.DrawLine(ih, x+10, y+10, x+150, y+10)
		})
	}

	cell(ih, 0, 5, "SCALE 2 DASH 10 4", func(x, y int) {
		reset(ih)
		iup.DrawSave(ih)
		iup.DrawTranslate(ih, float64(x+10), float64(y+10))
		iup.DrawScale(ih, 2, 2)
		ih.SetAttribute("DRAWLINEWIDTH", "2")
		ih.SetAttribute("DRAWDASH", "10 4")
		iup.DrawLine(ih, 0, 0, 70, 0)
		iup.DrawRestore(ih)
	})

	cell(ih, 1, 5, "ROTATE 20 ROUND", func(x, y int) {
		reset(ih)
		iup.DrawSave(ih)
		iup.DrawTranslate(ih, float64(x+20), float64(y+20))
		iup.DrawRotate(ih, -20)
		ih.SetAttribute("DRAWLINECAP", "ROUND")
		ih.SetAttribute("DRAWLINEJOIN", "ROUND")
		ih.SetAttribute("DRAWLINEWIDTH", "10")
		iup.DrawPathBegin(ih)
		iup.DrawPathMoveTo(ih, 0, 30)
		iup.DrawPathLineTo(ih, 50, 0)
		iup.DrawPathLineTo(ih, 100, 30)
		iup.DrawPathStroke(ih)
		iup.DrawRestore(ih)
	})

	cell(ih, 2, 5, "POLYGON DASH", func(x, y int) {
		reset(ih)
		ih.SetAttribute("DRAWLINEWIDTH", "3")
		ih.SetAttribute("DRAWDASH", "8 4")
		iup.DrawPolygon(ih, []int{x + 20, y + 5, x + 130, y + 5, x + 75, y + 60}, 3)
	})

	iup.DrawEnd(ih)
	return iup.DEFAULT
}

func main() {
	iup.Open()
	defer iup.Close()

	canvas := iup.Canvas().SetAttributes(fmt.Sprintf("RASTERSIZE=%dx%d, BGCOLOR=255 255 255, BORDER=NO", cols*cellW+cellPadX, rows*cellH+cellPadY))
	canvas.SetCallback("ACTION", iup.ActionFunc(action))

	dlg := iup.Dialog(iup.Vbox(canvas)).SetAttributes("MARGIN=5x5").SetAttribute("TITLE", "IupDraw Stroke")
	iup.Show(dlg)
	iup.MainLoop()
}
