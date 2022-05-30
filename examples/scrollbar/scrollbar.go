package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	canvas := iup.Canvas().SetAttributes(map[string]string{
		"RASTERSIZE": "300x200",
		"SCROLLBAR":  "YES",
		"XMAX":       "599",
		"YMAX":       "399",
	})

	canvas.SetCallback("SCROLL_CB", iup.ScrollFunc(scrollCb))
	canvas.SetCallback("RESIZE_CB", iup.ResizeFunc(resizeCb))
	canvas.SetCallback("ACTION", iup.CanvasActionFunc(action))

	dlg := iup.Dialog(canvas).SetAttribute("TITLE", "Scrollbar")

	iup.Map(dlg)
	iup.SetAttribute(canvas, "RASTERSIZE", nil) // release the minimum limitation

	iup.Show(dlg)
	iup.MainLoop()
}

func scrollCb(ih iup.Ihandle, op int, posx, posy float64) int {
	action(ih, posx, posy)
	return iup.DEFAULT
}

func resizeCb(ih iup.Ihandle, width, height int) int {
	ih.SetAttribute("DX", width)
	ih.SetAttribute("DY", height)
	return iup.DEFAULT
}

func action(ih iup.Ihandle, posx, posy float64) int {
	iposx := int(posx)
	iposy := 399 - iup.GetInt(ih, "DY") - int(posy)

	iup.DrawBegin(ih)
	iup.DrawParentBackground(ih)
	iup.DrawLine(ih, 0-iposx, 0-iposy, 599-iposx, 399-iposy)
	iup.DrawLine(ih, 0-iposx, 399-iposy, 599-iposx, 0-iposy)
	iup.DrawEnd(ih)

	return iup.DEFAULT
}
