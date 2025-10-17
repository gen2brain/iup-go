package main

import (
	"math"

	"github.com/gen2brain/iup-go/iup"
)

// tm is used as a phase shift to animate the waves.
var tm float64

func main() {
	iup.Open()
	defer iup.Close()

	cv := iup.Canvas().SetAttributes("SIZE=500x200")
	cv.SetCallback("ACTION", iup.CanvasActionFunc(actionCb))

	// Set up a timer to periodically redraw the canvas, creating the animation.
	timer := iup.Timer().SetAttribute("TIME", "40") // Update roughly 25 times per second.
	timer.SetCallback("ACTION_CB", iup.TimerActionFunc(func(ih iup.Ihandle) int {
		// Increment the time variable to make the wave appear to move.
		tm += 0.1
		// Reset time to prevent the number from growing indefinitely.
		if tm > 2*math.Pi {
			tm -= 2 * math.Pi
		}
		iup.Update(cv) // Tell the canvas it needs to be redrawn.
		return iup.DEFAULT
	}))
	timer.SetAttribute("RUN", "YES")

	dlg := iup.Dialog(
		iup.Frame(cv),
	).SetAttribute("TITLE", "Animated Canvas")

	iup.Show(dlg)
	iup.MainLoop()
}

func actionCb(ih iup.Ihandle, posx, posy float64) int {
	iup.DrawBegin(ih)
	defer iup.DrawEnd(ih)

	w, h := iup.DrawGetSize(ih)

	// Draw a dark background, like an oscilloscope screen.
	ih.SetAttributes(`DRAWCOLOR="20 20 40", DRAWSTYLE=FILL`)
	iup.DrawRectangle(ih, 0, 0, w, h)

	// Draw a faint grid for reference.
	ih.SetAttribute("DRAWCOLOR", "40 40 60")
	// Vertical lines.
	for i := 1; i < 10; i++ {
		x := (w * i) / 10
		iup.DrawLine(ih, x, 0, x, h-1)
	}
	// Horizontal lines.
	for i := 1; i < 5; i++ {
		y := (h * i) / 5
		iup.DrawLine(ih, 0, y, w-1, y)
	}
	// A slightly brighter center line.
	ih.SetAttribute("DRAWCOLOR", "60 60 80")
	iup.DrawLine(ih, 0, h/2, w-1, h/2)

	// Define the properties of our two waves.
	midY := float64(h) / 2.0
	amplitude1 := float64(h) * 0.35
	amplitude2 := float64(h) * 0.25
	frequency1 := 2.5
	frequency2 := 4.0

	// Calculate points and draw waves segment by segment.
	var px int
	var p1y, p2y float64 // Previous y-coordinates for wave 1 and 2

	for x := 0; x < w; x++ {
		// Calculate current y for wave 1
		angle1 := (float64(x)/float64(w))*2*math.Pi*frequency1 + tm
		y1 := midY - math.Sin(angle1)*amplitude1

		// Calculate current y for wave 2
		angle2 := (float64(x)/float64(w))*2*math.Pi*frequency2 + tm*1.5
		y2 := midY - math.Cos(angle2)*amplitude2

		if x > 0 {
			// Draw segment for wave 1 (bright green).
			ih.SetAttribute("DRAWCOLOR", "50 255 100")
			iup.DrawLine(ih, px, int(p1y), x, int(y1))

			// Draw segment for wave 2 (light blue).
			ih.SetAttribute("DRAWCOLOR", "100 150 255")
			iup.DrawLine(ih, px, int(p2y), x, int(y2))
		}

		// Update previous points for the next iteration.
		px = x
		p1y = y1
		p2y = y2
	}

	return iup.DEFAULT
}
