package main

import (
	"fmt"
	"math"

	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	cv := iup.Canvas().SetAttributes("RASTERSIZE=500x300")
	cv.SetCallback("ACTION", iup.ActionFunc(actionCb))

	dlg := iup.Dialog(cv).SetAttribute("TITLE", "Canvas")

	iup.Show(dlg)
	iup.MainLoop()
}

// actionCb is the callback function that handles the drawing on the canvas.
func actionCb(ih iup.Ihandle) int {
	iup.DrawBegin(ih)
	defer iup.DrawEnd(ih) // Ensure DrawEnd is always called.

	w, h := iup.DrawGetSize(ih)

	// Clear the background to white.
	ih.SetAttributes(`DRAWCOLOR="255 255 255", DRAWSTYLE=FILL`)
	iup.DrawRectangle(ih, 0, 0, w, h)

	// Define the data for our pie chart slices.
	data := []struct {
		label string
		value float64
		color string
	}{
		{"Marketing", 0.40, "255 105 97"}, // Pastel Red
		{"Sales", 0.30, "119 221 119"},    // Pastel Green
		{"R&D", 0.20, "137 207 240"},      // Pastel Blue
		{"Support", 0.10, "253 253 150"},  // Pastel Yellow
	}

	// Draw the pie chart on the left side of the canvas.
	xc := w / 3 // Center X of the pie.
	yc := h / 2 // Center Y of the pie.
	radius := int(math.Min(float64(w)/1.5, float64(h)) * 0.40)
	x1, y1 := xc-radius, yc-radius
	x2, y2 := xc+radius, yc+radius

	startAngle := 0.0
	ih.SetAttribute("DRAWSTYLE", "FILL")

	for _, slice := range data {
		sliceAngle := slice.value * 360.0
		endAngle := startAngle + sliceAngle

		ih.SetAttribute("DRAWCOLOR", slice.color)
		iup.DrawArc(ih, x1, y1, x2, y2, startAngle, endAngle)

		startAngle = endAngle
	}

	// Add a border around the pie for a cleaner look.
	ih.SetAttributes(`DRAWCOLOR="80 80 80", DRAWSTYLE=STROKE`)
	iup.DrawArc(ih, x1, y1, x2, y2, 0, 360)

	// Draw the legend on the right side of the canvas.
	legendX := w*2/3 - 40
	legendY := yc - (len(data)*25)/2 // Vertically center the legend block.
	boxSize := 15
	lineHeight := 25

	ih.SetAttribute("DRAWFONT", "Helvetica, 14")
	ih.SetAttribute("TEXTVERTICALALIGNMENT", "ACENTER") // Center text vertically against its y-coordinate.

	for i, slice := range data {
		currentY := legendY + i*lineHeight

		// Color box for the legend item.
		ih.SetAttributes(fmt.Sprintf(`DRAWCOLOR="%s", DRAWSTYLE=FILL`, slice.color))
		iup.DrawRectangle(ih, legendX, currentY, legendX+boxSize, currentY+boxSize)

		// Border for the color box.
		ih.SetAttributes(`DRAWCOLOR="80 80 80", DRAWSTYLE=STROKE`)
		iup.DrawRectangle(ih, legendX, currentY, legendX+boxSize, currentY+boxSize)

		// Text label for the legend item.
		label := fmt.Sprintf("%s (%.0f%%)", slice.label, slice.value*100)
		ih.SetAttribute("DRAWCOLOR", "50 50 50")
		iup.DrawText(ih, label, legendX+boxSize+10, currentY+boxSize/2, -1, -1)
	}

	return iup.DEFAULT
}
