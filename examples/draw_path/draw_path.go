package main

import (
	"math"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func label(ih iup.Ihandle, text string, x, y int) {
	ih.SetAttribute("DRAWFONT", "Helvetica, 9")
	ih.SetAttribute("DRAWCOLOR", "60 60 70")
	ih.SetAttribute("DRAWTEXTALIGNMENT", "ALEFT")
	iup.DrawText(ih, text, x, y, -1, -1)
}

func starPath(ih iup.Ihandle, cx, cy, r int) {
	iup.DrawPathBegin(ih)
	for i := 0; i < 5; i++ {
		a := (90.0 + float64(i)*144.0) * math.Pi / 180.0
		x := cx + int(float64(r)*math.Cos(a))
		y := cy - int(float64(r)*math.Sin(a))
		if i == 0 {
			iup.DrawPathMoveTo(ih, x, y)
		} else {
			iup.DrawPathLineTo(ih, x, y)
		}
	}
	iup.DrawPathClose(ih)
}

func nestedSquaresPath(ih iup.Ihandle, x, y, size, inset int) {
	iup.DrawPathBegin(ih)
	iup.DrawPathMoveTo(ih, x, y)
	iup.DrawPathLineTo(ih, x+size, y)
	iup.DrawPathLineTo(ih, x+size, y+size)
	iup.DrawPathLineTo(ih, x, y+size)
	iup.DrawPathClose(ih)
	iup.DrawPathMoveTo(ih, x+inset, y+inset)
	iup.DrawPathLineTo(ih, x+size-inset, y+inset)
	iup.DrawPathLineTo(ih, x+size-inset, y+size-inset)
	iup.DrawPathLineTo(ih, x+inset, y+size-inset)
	iup.DrawPathClose(ih)
}

func draw(ih iup.Ihandle) int {
	iup.DrawBegin(ih)
	w, h := iup.DrawGetSize(ih)

	ih.SetAttribute("DRAWSTYLE", "FILL")
	ih.SetAttribute("DRAWCOLOR", "255 255 255")
	iup.DrawRectangle(ih, 0, 0, w-1, h-1)

	ih.SetAttribute("DRAWFONT", "Helvetica, Bold 12")
	ih.SetAttribute("DRAWCOLOR", "30 30 40")
	iup.DrawText(ih, "Path model: IupDrawPathMoveTo / LineTo / CurveTo / QuadTo / ArcTo / Close", 20, 12, -1, -1)

	label(ih, "rounded card: gradient fill + solid stroke", 20, 44)
	iup.DrawPathBegin(ih)
	iup.DrawPathMoveTo(ih, 40, 74)
	iup.DrawPathLineTo(ih, 210, 74)
	iup.DrawPathQuadTo(ih, 230, 74, 230, 94)
	iup.DrawPathLineTo(ih, 230, 140)
	iup.DrawPathQuadTo(ih, 230, 160, 210, 160)
	iup.DrawPathLineTo(ih, 40, 160)
	iup.DrawPathQuadTo(ih, 20, 160, 20, 140)
	iup.DrawPathLineTo(ih, 20, 94)
	iup.DrawPathQuadTo(ih, 20, 74, 40, 74)
	iup.DrawPathClose(ih)
	iup.DrawSetSourceLinearGradient(ih, 20, 74, 230, 160, 45,
		[]string{"60 130 240", "130 90 220", "240 60 90"}, nil)
	iup.DrawPathFill(ih, iup.DRAW_RULE_WINDING)
	iup.DrawSetSourceSolid(ih, "40 40 60")
	ih.SetAttribute("DRAWLINEWIDTH", "2")
	iup.DrawPathStroke(ih)
	ih.SetAttribute("DRAWLINEWIDTH", "1")

	label(ih, "cubic + quadratic Beziers, dashed stroke", 310, 44)
	iup.DrawPathBegin(ih)
	iup.DrawPathMoveTo(ih, 310, 140)
	iup.DrawPathCurveTo(ih, 350, 60, 390, 60, 430, 140)
	iup.DrawPathCurveTo(ih, 470, 220, 510, 220, 550, 140)
	iup.DrawPathQuadTo(ih, 570, 110, 590, 140)
	ih.SetAttribute("DRAWCOLOR", "20 120 200")
	ih.SetAttribute("DRAWLINEWIDTH", "3")
	ih.SetAttribute("DRAWSTYLE", "STROKE_DASH")
	iup.DrawPathStroke(ih)
	ih.SetAttribute("DRAWSTYLE", "STROKE")
	ih.SetAttribute("DRAWLINEWIDTH", "1")

	label(ih, "even-odd star vs winding star", 20, 204)
	starPath(ih, 90, 320, 55)
	iup.DrawSetSourceRadialGradient(ih, 90, 320, 60,
		[]string{"255 220 80", "255 120 20"}, nil)
	iup.DrawPathFill(ih, iup.DRAW_RULE_EVENODD)

	starPath(ih, 230, 320, 55)
	iup.DrawSetSourceRadialGradient(ih, 230, 320, 60,
		[]string{"255 220 80", "255 120 20"}, nil)
	iup.DrawPathFill(ih, iup.DRAW_RULE_WINDING)

	label(ih, "path clip: content clipped to a circle", 340, 204)
	iup.DrawPathBegin(ih)
	iup.DrawPathMoveTo(ih, 485, 280)
	iup.DrawPathArcTo(ih, 430, 280, 55, 55, 0, 360)
	iup.DrawPathClose(ih)
	iup.DrawSetClipPath(ih, iup.DRAW_RULE_WINDING)
	iup.DrawSetSourceSolid(ih, "200 200 205")
	ih.SetAttribute("DRAWSTYLE", "FILL")
	iup.DrawRectangle(ih, 375, 175, 485, 335)
	ih.SetAttribute("DRAWSTYLE", "STROKE")
	iup.DrawSetSourceLinearGradient(ih, 375, 175, 485, 335, 60,
		[]string{"40 160 90", "220 240 120"}, nil)
	for y := 175; y <= 335; y += 8 {
		iup.DrawPathBegin(ih)
		iup.DrawPathMoveTo(ih, 375, y)
		iup.DrawPathLineTo(ih, 485, y)
		iup.DrawPathStroke(ih)
	}
	iup.DrawResetClip(ih)

	label(ih, "nested subpaths: even-odd hole vs winding fill", 20, 404)
	nestedSquaresPath(ih, 40, 426, 54, 16)
	iup.DrawSetSourceSolid(ih, "70 150 230")
	iup.DrawPathFill(ih, iup.DRAW_RULE_EVENODD)
	nestedSquaresPath(ih, 150, 426, 54, 16)
	iup.DrawSetSourceSolid(ih, "70 150 230")
	iup.DrawPathFill(ih, iup.DRAW_RULE_WINDING)

	label(ih, "saved state: translated, scaled, rotated, clipped", 280, 404)
	iup.DrawSave(ih)
	iup.DrawTranslate(ih, 420, 458)
	iup.DrawRotate(ih, 8)
	iup.DrawScale(ih, 0.72, 0.72)
	iup.DrawTranslate(ih, -420, -458)
	iup.DrawSetClipRoundedRect(ih, 300, 424, 540, 492, 12)
	iup.DrawSetSourceLinearGradient(ih, 300, 424, 540, 492, 0,
		[]string{"35 170 190", "100 70 210"}, nil)
	iup.DrawPathBegin(ih)
	iup.DrawPathMoveTo(ih, 300, 424)
	iup.DrawPathLineTo(ih, 540, 424)
	iup.DrawPathLineTo(ih, 540, 492)
	iup.DrawPathLineTo(ih, 300, 492)
	iup.DrawPathClose(ih)
	iup.DrawPathFill(ih, iup.DRAW_RULE_WINDING)
	iup.DrawSetSourceSolid(ih, "255 255 255")
	iup.DrawText(ih, "Affine transform", 358, 450, -1, -1)
	iup.DrawRestore(ih)
	iup.DrawSetSourceSolid(ih, "70 70 80")
	iup.DrawLine(ih, 280, 492, 560, 492)

	iup.DrawEnd(ih)
	return iup.DEFAULT
}

func main() {
	iup.Open()
	defer iup.Close()

	cv := iup.Canvas().SetAttributes(`RASTERSIZE=640x510, BORDER=NO`)
	cv.SetCallback("ACTION", iup.ActionFunc(draw))

	dlg := iup.Dialog(cv).SetAttribute("TITLE", "IupDraw Paths")
	iup.Show(dlg)

	iup.MainLoop()
}
