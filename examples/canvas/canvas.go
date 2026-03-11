package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	cv := iup.Canvas().SetAttribute("SIZE", "300x300")
	cv.SetCallback("ACTION", iup.ActionFunc(actionCb))

	dlg := iup.Dialog(
		iup.Frame(cv),
	).SetAttribute("TITLE", "Canvas Example")

	iup.Show(dlg)
	iup.MainLoop()
}

// actionCb is the callback function that does the drawing on the canvas.
func actionCb(ih iup.Ihandle) int {
	iup.DrawBegin(ih)
	defer iup.DrawEnd(ih)

	// Enable anti-aliasing for smoother shapes
	ih.SetAttribute("ANTIALIAS", "YES")

	w, h := iup.DrawGetSize(ih)

	// Draw a light blue background
	ih.SetAttributes(`DRAWCOLOR="200 220 255", DRAWSTYLE=FILL`)
	iup.DrawRectangle(ih, 0, 0, w, h)

	// Define smiley dimensions, centered with padding
	cx, cy := w/2, h/2
	radius := (min(w, h) / 2) - 15

	// Draw the head
	// Yellow fill
	ih.SetAttributes(`DRAWCOLOR="255 255 0", DRAWSTYLE=FILL`)
	iup.DrawArc(ih, cx-radius, cy-radius, cx+radius, cy+radius, 0, 360)
	// Black outline
	ih.SetAttributes(`DRAWCOLOR="0 0 0", DRAWSTYLE=STROKE, DRAWLINEWIDTH=2`)
	iup.DrawArc(ih, cx-radius, cy-radius, cx+radius, cy+radius, 0, 360)

	// Draw the eyes
	eyeRadius := radius / 7
	eyeOffsetX := radius * 2 / 5
	eyeOffsetY := radius / 3
	ih.SetAttributes(`DRAWCOLOR="0 0 0", DRAWSTYLE=FILL`)
	// Left eye
	iup.DrawArc(ih, cx-eyeOffsetX-eyeRadius, cy-eyeOffsetY-eyeRadius, cx-eyeOffsetX+eyeRadius, cy-eyeOffsetY+eyeRadius, 0, 360)
	// Right eye
	iup.DrawArc(ih, cx+eyeOffsetX-eyeRadius, cy-eyeOffsetY-eyeRadius, cx+eyeOffsetX+eyeRadius, cy-eyeOffsetY+eyeRadius, 0, 360)

	// Draw the smile
	mouthRadius := radius * 2 / 3
	mouthOffsetY := radius / 4
	ih.SetAttributes(`DRAWCOLOR="0 0 0", DRAWSTYLE=STROKE, DRAWLINEWIDTH=4`)
	// Define the bounding box for the mouth arc
	x1, y1 := cx-mouthRadius, cy+mouthOffsetY-mouthRadius
	x2, y2 := cx+mouthRadius, cy+mouthOffsetY+mouthRadius
	// Create a wide smile
	iup.DrawArc(ih, x1, y1, x2, y2, 210, 330)

	return iup.DEFAULT
}
