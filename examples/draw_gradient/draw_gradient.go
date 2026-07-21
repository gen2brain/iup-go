package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func label(ih iup.Ihandle, text string, x, y int) {
	ih.SetAttribute("DRAWFONT", "Helvetica, 9")
	ih.SetAttribute("DRAWCOLOR", "60 60 70")
	ih.SetAttribute("DRAWTEXTALIGNMENT", "ALEFT")
	iup.DrawText(ih, text, x, y, -1, -1)
}

func draw(ih iup.Ihandle) int {
	iup.DrawBegin(ih)
	w, _ := iup.DrawGetSize(ih)

	ih.SetAttribute("DRAWSTYLE", "FILL")
	ih.SetAttribute("DRAWCOLOR", "255 255 255")
	iup.DrawRectangle(ih, 0, 0, w-1, 9999)

	ih.SetAttribute("DRAWFONT", "Helvetica, Bold 12")
	ih.SetAttribute("DRAWCOLOR", "30 30 40")
	iup.DrawText(ih, "Multi-stop gradients: IupDrawLinearGradientStops / RadialGradientStops", 20, 12, -1, -1)

	rainbow := []string{"255 0 0", "255 165 0", "255 255 0", "0 200 0", "0 120 255", "75 0 130", "148 0 211"}

	label(ih, "2-stop (classic)", 20, 44)
	iup.DrawLinearGradient(ih, 20, 60, 360, 120, 0, "30 60 160", "220 80 60")

	label(ih, "7-stop rainbow, even spacing", 20, 140)
	iup.DrawLinearGradientStops(ih, 20, 156, 360, 216, 0, rainbow, nil)

	label(ih, "custom offsets (mid stop at 0.2)", 20, 236)
	iup.DrawLinearGradientStops(ih, 20, 252, 360, 312, 0,
		[]string{"30 60 160", "240 220 90", "200 40 40"},
		[]float32{0.0, 0.2, 1.0})

	label(ih, "vertical, translucent stops", 20, 332)
	iup.DrawLinearGradientStops(ih, 20, 348, 360, 408, 90,
		[]string{"255 0 0 255", "0 255 0 60", "0 0 255 255"}, nil)

	label(ih, "radial, 4-stop", 420, 44)
	iup.DrawRadialGradientStops(ih, 520, 130, 70,
		[]string{"255 255 210", "255 180 40", "200 40 40", "40 0 60"}, nil)

	label(ih, "radial rainbow", 420, 236)
	iup.DrawRadialGradientStops(ih, 520, 320, 70, rainbow, nil)

	iup.DrawEnd(ih)
	return iup.DEFAULT
}

func main() {
	iup.Open()
	defer iup.Close()

	cv := iup.Canvas().SetAttributes(`RASTERSIZE=640x430, BORDER=NO`)
	cv.SetCallback("ACTION", iup.ActionFunc(draw))

	dlg := iup.Dialog(cv).SetAttribute("TITLE", "IupDraw Gradients")
	iup.Show(dlg)

	iup.MainLoop()
}
