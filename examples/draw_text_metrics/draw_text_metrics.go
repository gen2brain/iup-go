package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

var fonts = []string{"Helvetica, 12", "Times, Italic 20", "Courier, Bold 28", "Helvetica, 40"}

func hline(ih iup.Ihandle, x1, x2, y int, color string) {
	ih.SetAttribute("DRAWCOLOR", color)
	ih.SetAttribute("DRAWSTYLE", "STROKE_DASH")
	iup.DrawLine(ih, x1, y, x2, y)
	ih.SetAttribute("DRAWSTYLE", "STROKE")
}

func box(ih iup.Ihandle, x, y, w, h int) {
	ih.SetAttribute("DRAWCOLOR", "245 245 248")
	ih.SetAttribute("DRAWSTYLE", "FILL")
	iup.DrawRectangle(ih, x, y, x+w, y+h)
	ih.SetAttribute("DRAWCOLOR", "185 185 195")
	ih.SetAttribute("DRAWSTYLE", "STROKE")
	iup.DrawRectangle(ih, x, y, x+w, y+h)
}

func row(ih iup.Ihandle, y, boxH int, font string) int {
	const naiveX = 30
	const metricX = 190
	boxW := 150

	ih.SetAttribute("DRAWFONT", font)
	ascent, descent, lineHeight := iup.DrawGetTextMetrics(ih)
	_, textH := iup.DrawGetTextSize(ih, "Ag")

	box(ih, naiveX, y, boxW, boxH)
	ih.SetAttribute("DRAWFONT", font)
	ih.SetAttribute("DRAWCOLOR", "205 110 110")
	iup.DrawText(ih, "Ag", naiveX+14, y+(boxH-textH)/2, boxW, textH)

	box(ih, metricX, y, boxW, boxH)
	glyphH := ascent + descent
	top := y + (boxH-glyphH)/2
	hline(ih, metricX+1, metricX+boxW-1, top, "150 195 150")
	hline(ih, metricX+1, metricX+boxW-1, top+ascent, "70 155 70")
	hline(ih, metricX+1, metricX+boxW-1, top+glyphH, "150 195 150")
	ih.SetAttribute("DRAWFONT", font)
	ih.SetAttribute("DRAWCOLOR", "45 95 45")
	iup.DrawText(ih, "Ag", metricX+14, top, boxW, glyphH)

	ih.SetAttribute("DRAWFONT", "Helvetica, 9")
	ih.SetAttribute("DRAWCOLOR", "90 90 100")
	iup.DrawText(ih, fmt.Sprintf("%s\nascent %d  descent %d  lineHeight %d", font, ascent, descent, lineHeight), metricX+boxW+20, y+(boxH-24)/2, 300, 40)

	return y + boxH + 14
}

func draw(ih iup.Ihandle) int {
	iup.DrawBegin(ih)
	w, _ := iup.DrawGetSize(ih)

	ih.SetAttribute("DRAWSTYLE", "FILL")
	ih.SetAttribute("DRAWCOLOR", "255 255 255")
	iup.DrawRectangle(ih, 0, 0, w-1, 9999)

	ih.SetAttribute("DRAWTEXTALIGNMENT", "ALEFT")
	ih.SetAttribute("DRAWFONT", "Helvetica, Bold 12")
	ih.SetAttribute("DRAWCOLOR", "30 30 40")
	iup.DrawText(ih, "IupDrawGetTextMetrics: baseline-accurate vertical centering", 30, 12, -1, -1)

	ih.SetAttribute("DRAWFONT", "Helvetica, 9")
	ih.SetAttribute("DRAWCOLOR", "170 100 100")
	iup.DrawText(ih, "naive (text height)", 30, 36, -1, -1)
	ih.SetAttribute("DRAWCOLOR", "70 150 70")
	iup.DrawText(ih, "metric (ascent/baseline/descent)", 190, 36, -1, -1)

	y := 62
	for i, f := range fonts {
		y = row(ih, y, 40+i*16, f)
	}

	iup.DrawEnd(ih)
	return iup.DEFAULT
}

func main() {
	iup.Open()
	defer iup.Close()

	cv := iup.Canvas().SetAttributes(`RASTERSIZE=720x384, BORDER=NO`)
	cv.SetCallback("ACTION", iup.ActionFunc(draw))

	dlg := iup.Dialog(cv).SetAttribute("TITLE", "IupDrawGetTextMetrics")
	iup.Show(dlg)

	iup.MainLoop()
}
