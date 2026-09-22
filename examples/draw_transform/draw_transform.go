package main

import (
	"fmt"
	"runtime"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

const cell = 150

var cols = func() int {
	if runtime.GOOS == "android" || runtime.GOOS == "ios" {
		return 2
	}
	return 5
}()

type demo struct {
	title string
	draw  func(ih iup.Ihandle, x, y int)
}

func circle(ih iup.Ihandle, cx, cy, r int) {
	iup.DrawPathMoveTo(ih, cx+r, cy)
	iup.DrawPathArcTo(ih, cx, cy, r, r, 0, 360)
	iup.DrawPathClose(ih)
}

func guide(ih iup.Ihandle, x1, y1, x2, y2 int) {
	iup.DrawSave(ih)
	iup.DrawResetTransform(ih)
	iup.DrawResetClip(ih)
	ih.SetAttribute("DRAWSTYLE", "STROKE")
	ih.SetAttribute("DRAWLINEWIDTH", "1")
	ih.SetAttribute("DRAWCOLOR", "255 0 0")
	iup.DrawRectangle(ih, x1, y1, x2, y2)
	iup.DrawRestore(ih)
}

var demos = []demo{
	{"even-odd ring", func(ih iup.Ihandle, x, y int) {
		iup.DrawPathBegin(ih)
		circle(ih, 75, 65, 50)
		circle(ih, 75, 65, 25)
		iup.DrawSetSourceSolid(ih, "40 110 220")
		iup.DrawPathFill(ih, iup.DRAW_RULE_EVENODD)
	}},
	{"even-odd clip", func(ih iup.Ihandle, x, y int) {
		iup.DrawPathBegin(ih)
		circle(ih, 75, 65, 50)
		circle(ih, 75, 65, 25)
		iup.DrawSetClipPath(ih, iup.DRAW_RULE_EVENODD)
		iup.DrawSetSourceLinearGradient(ih, 20, 10, 130, 120, 45, []string{"40 160 90", "220 240 120"}, nil)
		iup.DrawPathBegin(ih)
		iup.DrawPathMoveTo(ih, 0, 0)
		iup.DrawPathLineTo(ih, 149, 0)
		iup.DrawPathLineTo(ih, 149, 129)
		iup.DrawPathLineTo(ih, 0, 129)
		iup.DrawPathFill(ih, iup.DRAW_RULE_WINDING)
	}},
	{"dashed arc 0..270", func(ih iup.Ihandle, x, y int) {
		ih.SetAttribute("DRAWSTYLE", "STROKE_DASH")
		ih.SetAttribute("DRAWLINEWIDTH", "2")
		iup.DrawSetSourceSolid(ih, "120 40 200")
		iup.DrawPathBegin(ih)
		iup.DrawPathMoveTo(ih, 125, 65)
		iup.DrawPathArcTo(ih, 75, 65, 50, 50, 0, 270)
		iup.DrawPathStroke(ih)
	}},
	{"clip kept on translate", func(ih iup.Ihandle, x, y int) {
		iup.DrawTranslate(ih, 20, 20)
		iup.DrawSetClipRect(ih, 0, 0, 99, 99)
		iup.DrawTranslate(ih, 10, 0)
		ih.SetAttribute("DRAWSTYLE", "FILL")
		ih.SetAttribute("DRAWCOLOR", "230 230 150")
		iup.DrawRectangle(ih, -100, -100, 300, 300)
		ih.SetAttribute("DRAWCOLOR", "40 110 220")
		iup.DrawRectangle(ih, 0, 0, 29, 29)
		guide(ih, x+20, y+20, x+119, y+119)
	}},
	{"rotated clip", func(ih iup.Ihandle, x, y int) {
		iup.DrawTranslate(ih, 75, 65)
		iup.DrawRotate(ih, 30)
		iup.DrawSetClipRect(ih, -40, -40, 39, 39)
		iup.DrawResetTransform(ih)
		ih.SetAttribute("DRAWSTYLE", "FILL")
		ih.SetAttribute("DRAWCOLOR", "220 60 60")
		iup.DrawRectangle(ih, x, y, x+cell-1, y+cell-1)
	}},
	{"rotated text", func(ih iup.Ihandle, x, y int) {
		ih.SetAttribute("DRAWFONT", "Helvetica, 10")
		ih.SetAttribute("DRAWCOLOR", "0 0 150")
		ih.SetAttribute("DRAWTEXTORIENTATION", "-90")
		iup.DrawText(ih, "Rotated", 40, 10, -1, -1)
		iup.DrawTranslate(ih, 60, 20)
		iup.DrawScale(ih, 2, 2)
		ih.SetAttribute("DRAWFONT", "Helvetica, 8")
		ih.SetAttribute("DRAWCOLOR", "150 0 0")
		ih.SetAttribute("DRAWTEXTORIENTATION", "90")
		ih.SetAttribute("DRAWTEXTLAYOUTCENTER", "YES")
		iup.DrawText(ih, "Mid", 0, 0, 40, 40)
		guide(ih, x+60, y+20, x+139, y+99)
	}},
	{"scaled down", func(ih iup.Ihandle, x, y int) {
		iup.DrawScale(ih, 0.1, 0.1)
		ih.SetAttribute("DRAWSTYLE", "FILL")
		ih.SetAttribute("DRAWCOLOR", "90 180 90")
		iup.DrawRectangle(ih, 0, 0, 1349, 1199)
	}},
	{"text clip", func(ih iup.Ihandle, x, y int) {
		iup.DrawTranslate(ih, 20, 30)
		iup.DrawRotate(ih, 10)
		ih.SetAttribute("DRAWFONT", "Helvetica, 14")
		ih.SetAttribute("DRAWCOLOR", "0 0 0")
		ih.SetAttribute("DRAWTEXTCLIP", "YES")
		iup.DrawText(ih, "Clipped text", 0, 0, 80, 30)
		ih.SetAttribute("DRAWSTYLE", "STROKE")
		ih.SetAttribute("DRAWCOLOR", "255 0 0")
		iup.DrawRectangle(ih, 0, 0, 79, 29)
	}},
	{"nested save and restore", func(ih iup.Ihandle, x, y int) {
		iup.DrawSetClipRect(ih, 10, 10, 89, 89)
		iup.DrawSave(ih)
		iup.DrawTranslate(ih, 50, 50)
		iup.DrawRotate(ih, 45)
		iup.DrawSetClipRect(ih, -20, -20, 19, 19)
		iup.DrawRestore(ih)
		iup.DrawTranslate(ih, 30, 30)
		ih.SetAttribute("DRAWSTYLE", "FILL")
		ih.SetAttribute("DRAWCOLOR", "100 100 220")
		iup.DrawRectangle(ih, -100, -100, 300, 300)
		guide(ih, x+10, y+10, x+89, y+89)
	}},
	{"open and closed subpaths", func(ih iup.Ihandle, x, y int) {
		ih.SetAttribute("DRAWSTYLE", "STROKE")
		ih.SetAttribute("DRAWLINEWIDTH", "3")
		iup.DrawSetSourceSolid(ih, "0 0 0")
		iup.DrawPathBegin(ih)
		iup.DrawPathMoveTo(ih, 20, 20)
		iup.DrawPathLineTo(ih, 70, 20)
		iup.DrawPathLineTo(ih, 45, 60)
		iup.DrawPathClose(ih)
		iup.DrawPathMoveTo(ih, 20, 80)
		iup.DrawPathLineTo(ih, 75, 120)
		iup.DrawPathLineTo(ih, 130, 80)
		iup.DrawPathStroke(ih)
	}},
	{"radial source", func(ih iup.Ihandle, x, y int) {
		iup.DrawPathBegin(ih)
		iup.DrawPathMoveTo(ih, 10, 10)
		iup.DrawPathLineTo(ih, 140, 10)
		iup.DrawPathLineTo(ih, 140, 120)
		iup.DrawPathLineTo(ih, 10, 120)
		iup.DrawPathClose(ih)
		iup.DrawSetSourceRadialGradient(ih, 75, 65, 30, []string{"255 255 255", "200 40 40"}, nil)
		iup.DrawPathFill(ih, iup.DRAW_RULE_WINDING)
	}},
	{"gradient under clip", func(ih iup.Ihandle, x, y int) {
		iup.DrawSetClipRect(ih, 0, 0, 74, 129)
		iup.DrawPathBegin(ih)
		circle(ih, 75, 65, 55)
		iup.DrawSetSourceLinearGradient(ih, 20, 10, 130, 120, 0, []string{"40 160 90", "220 240 120", "40 60 200"}, nil)
		iup.DrawPathFill(ih, iup.DRAW_RULE_WINDING)
	}},
	{"skewed shapes", func(ih iup.Ihandle, x, y int) {
		iup.DrawTransform(ih, 1, 0, -0.4, 1, 50, 10)
		ih.SetAttribute("DRAWSTYLE", "FILL")
		ih.SetAttribute("DRAWCOLOR", "240 170 40")
		iup.DrawRectangle(ih, 10, 10, 90, 50)
		ih.SetAttribute("DRAWSTYLE", "STROKE")
		ih.SetAttribute("DRAWLINEWIDTH", "2")
		ih.SetAttribute("DRAWCOLOR", "60 60 200")
		iup.DrawEllipse(ih, 10, 60, 90, 110)
	}},
	{"thick stroke", func(ih iup.Ihandle, x, y int) {
		ih.SetAttribute("DRAWSTYLE", "STROKE")
		ih.SetAttribute("DRAWLINEWIDTH", "14")
		iup.DrawSetSourceSolid(ih, "60 60 200")
		iup.DrawPathBegin(ih)
		iup.DrawPathMoveTo(ih, 30, 110)
		iup.DrawPathLineTo(ih, 75, 30)
		iup.DrawPathLineTo(ih, 120, 110)
		iup.DrawPathStroke(ih)
	}},
	{"scaled path circle", func(ih iup.Ihandle, x, y int) {
		iup.DrawScale(ih, 3, 3)
		ih.SetAttribute("DRAWSTYLE", "STROKE")
		ih.SetAttribute("DRAWLINEWIDTH", "1")
		iup.DrawSetSourceSolid(ih, "0 0 0")
		iup.DrawPathBegin(ih)
		circle(ih, 25, 21, 18)
		iup.DrawPathStroke(ih)
	}},
	{"1px lines", func(ih iup.Ihandle, x, y int) {
		iup.DrawTranslate(ih, 10, 10)
		ih.SetAttribute("DRAWSTYLE", "STROKE")
		ih.SetAttribute("DRAWLINEWIDTH", "1")
		ih.SetAttribute("DRAWCOLOR", "0 0 0")
		for i := range 6 {
			iup.DrawLine(ih, 0, i*10, 120, i*10)
		}
		iup.DrawRectangle(ih, 0, 70, 120, 110)
	}},
	{"rotating spokes", func(ih iup.Ihandle, x, y int) {
		iup.DrawTranslate(ih, 75, 65)
		ih.SetAttribute("DRAWSTYLE", "STROKE")
		ih.SetAttribute("DRAWLINEWIDTH", "2")
		for i := range 12 {
			ih.SetAttribute("DRAWCOLOR", fmt.Sprintf("%d 80 %d", 40+i*18, 240-i*18))
			iup.DrawLine(ih, 15, 0, 50, 0)
			iup.DrawRotate(ih, 30)
		}
	}},
	{"focus rect", func(ih iup.Ihandle, x, y int) {
		iup.DrawTranslate(ih, 75, 65)
		iup.DrawRotate(ih, -15)
		iup.DrawFocusRect(ih, -50, -40, 49, 39)
	}},
}

func draw(ih iup.Ihandle) int {
	iup.DrawBegin(ih)
	w, h := iup.DrawGetSize(ih)
	ih.SetAttribute("DRAWSTYLE", "FILL")
	ih.SetAttribute("DRAWCOLOR", "255 255 255")
	iup.DrawRectangle(ih, 0, 0, w-1, h-1)

	for n, d := range demos {
		x, y := (n%cols)*cell, (n/cols)*cell
		iup.DrawSave(ih)
		iup.DrawTranslate(ih, float64(x), float64(y))
		d.draw(ih, x, y)
		iup.DrawRestore(ih)

		ih.SetAttribute("DRAWFONT", "Helvetica, 8")
		ih.SetAttribute("DRAWCOLOR", "0 0 0")
		ih.SetAttribute("DRAWSTYLE", "STROKE")
		ih.SetAttribute("DRAWLINEWIDTH", "1")
		iup.DrawText(ih, d.title, x+3, y+cell-14, -1, -1)
		ih.SetAttribute("DRAWCOLOR", "200 200 200")
		iup.DrawRectangle(ih, x, y, x+cell-1, y+cell-1)
	}

	iup.DrawEnd(ih)
	return iup.DEFAULT
}

func main() {
	iup.Open()
	defer iup.Close()

	rows := (len(demos) + cols - 1) / cols
	cv := iup.Canvas().SetAttributes(fmt.Sprintf("RASTERSIZE=%dx%d, BORDER=NO", cols*cell, rows*cell))
	cv.SetCallback("ACTION", iup.ActionFunc(draw))

	dlg := iup.Dialog(cv).SetAttribute("TITLE", "IupDraw Transforms")
	iup.Show(dlg)

	iup.MainLoop()
}
