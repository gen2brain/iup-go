package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func label(ih iup.Ihandle, text string, x, y int) {
	ih.SetAttribute("DRAWFONT", "Sans, 8")
	ih.SetAttribute("DRAWCOLOR", "60 60 70")
	iup.DrawText(ih, text, x, y, -1, -1)
}

func circles(ih iup.Ihandle, x, y int, alpha string) {
	ih.SetAttribute("DRAWSTYLE", "FILL")
	ih.SetAttribute("DRAWCOLOR", "220 40 40"+alpha)
	iup.DrawEllipse(ih, x, y, x+70, y+70)
	ih.SetAttribute("DRAWCOLOR", "40 90 220"+alpha)
	iup.DrawEllipse(ih, x+40, y, x+110, y+70)
	ih.SetAttribute("DRAWCOLOR", "40 170 70"+alpha)
	iup.DrawEllipse(ih, x+20, y+35, x+90, y+105)
}

func action(ih iup.Ihandle) int {
	iup.DrawBegin(ih)
	w, h := iup.DrawGetSize(ih)

	ih.SetAttribute("DRAWSTYLE", "FILL")
	ih.SetAttribute("DRAWCOLOR", "255 255 255")
	iup.DrawRectangle(ih, 0, 0, w, h)
	ih.SetAttribute("DRAWCOLOR", "225 225 230")
	for x := 0; x < w; x += 20 {
		iup.DrawRectangle(ih, x, 0, x+9, h)
	}

	label(ih, "per-shape alpha: overlaps double-blend", 20, 10)
	circles(ih, 30, 30, " 128")

	label(ih, "layer alpha 128: one uniform group", 220, 10)
	iup.DrawSaveLayer(ih, 128)
	circles(ih, 230, 30, "")
	iup.DrawRestore(ih)

	label(ih, "nested: 200 outer, 100 inner (right)", 420, 10)
	iup.DrawSaveLayer(ih, 200)
	ih.SetAttribute("DRAWSTYLE", "FILL")
	ih.SetAttribute("DRAWCOLOR", "120 60 160")
	iup.DrawRectangle(ih, 430, 30, 500, 130)
	iup.DrawSaveLayer(ih, 100)
	ih.SetAttribute("DRAWCOLOR", "120 60 160")
	iup.DrawRectangle(ih, 510, 30, 580, 130)
	iup.DrawRestore(ih)
	iup.DrawRestore(ih)

	label(ih, "clip outside, new clip inside the layer", 20, 160)
	iup.DrawSetClipRect(ih, 20, 180, 170, 280)
	iup.DrawSaveLayer(ih, 150)
	ih.SetAttribute("DRAWSTYLE", "FILL")
	ih.SetAttribute("DRAWCOLOR", "230 140 30")
	iup.DrawRectangle(ih, 0, 170, 200, 300)
	iup.DrawSetClipRect(ih, 60, 200, 130, 260)
	ih.SetAttribute("DRAWCOLOR", "30 30 30")
	iup.DrawRectangle(ih, 0, 170, 200, 300)
	iup.DrawRestore(ih)
	ih.SetAttribute("DRAWSTYLE", "STROKE")
	ih.SetAttribute("DRAWCOLOR", "0 0 0")
	iup.DrawRectangle(ih, 0, 170, 200, 300)
	iup.DrawResetClip(ih)

	label(ih, "rotated, text inside a layer 140", 220, 160)
	iup.DrawSave(ih)
	iup.DrawTranslate(ih, 300, 230)
	iup.DrawRotate(ih, 15)
	iup.DrawSaveLayer(ih, 140)
	ih.SetAttribute("DRAWSTYLE", "FILL")
	ih.SetAttribute("DRAWCOLOR", "20 120 160")
	iup.DrawRectangle(ih, -70, -35, 70, 35)
	ih.SetAttribute("DRAWFONT", "Sans, Bold 14")
	ih.SetAttribute("DRAWCOLOR", "255 255 255")
	iup.DrawText(ih, "Layer", -28, -12, -1, -1)
	iup.DrawRestore(ih)
	iup.DrawRestore(ih)

	label(ih, "faded panel: shapes and text as one", 420, 160)
	iup.DrawSaveLayer(ih, 110)
	ih.SetAttribute("DRAWSTYLE", "FILL")
	ih.SetAttribute("DRAWCOLOR", "40 40 50")
	iup.DrawRoundedRectangle(ih, 430, 180, 580, 280, 12)
	ih.SetAttribute("DRAWCOLOR", "250 200 60")
	iup.DrawEllipse(ih, 445, 195, 485, 235)
	ih.SetAttribute("DRAWFONT", "Sans, Bold 11")
	ih.SetAttribute("DRAWCOLOR", "255 255 255")
	iup.DrawText(ih, "Paused", 495, 205, -1, -1)
	ih.SetAttribute("DRAWFONT", "Sans, 9")
	iup.DrawText(ih, "Tap to resume", 445, 250, -1, -1)
	iup.DrawRestore(ih)

	iup.DrawEnd(ih)
	return iup.DEFAULT
}

func main() {
	iup.Open()
	defer iup.Close()

	cv := iup.Canvas().SetAttributes("RASTERSIZE=600x300, BORDER=NO")
	cv.SetCallback("ACTION", iup.ActionFunc(action))

	dlg := iup.Dialog(cv).SetAttribute("TITLE", "IupDraw Layers")
	iup.Show(dlg)
	iup.MainLoop()
}
