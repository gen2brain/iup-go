package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	cv := iup.Canvas().SetAttributes("SIZE=300x100, XMIN=0, XMAX=99, POSX=0, DX=10")
	cv.SetCallback("ACTION", iup.CanvasActionFunc(actionCb))

	dlg := iup.Dialog(
		iup.Frame(cv),
	).SetAttribute("TITLE", "Canvas")

	iup.Show(dlg)
	iup.MainLoop()
}

func actionCb(ih iup.Ihandle, posx, posy float64) int {
	iup.DrawBegin(ih)

	w, h := iup.DrawGetSize(ih)

	ih.SetAttributes(`DRAWCOLOR="255 255 255", DRAWSTYLE=FILL`)
	iup.DrawRectangle(ih, 0, 0, w, h)

	ih.SetAttributes(`DRAWCOLOR="255 0 0"`)
	iup.DrawLine(ih, 0, 0, w-1, h-1)
	iup.DrawLine(ih, 0, h-1, w-1, 0)

	ih.SetAttributes(`DRAWCOLOR="0 0 0", DRAWFONT="Times, 28"`)
	iup.DrawText(ih, "This is a test", w/2, h/2, -1, -1)

	iup.DrawEnd(ih)
	return iup.DEFAULT
}
