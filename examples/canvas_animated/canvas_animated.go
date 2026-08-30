package main

import (
	"math"

	"github.com/gen2brain/iup-go/iup"
)

// tm is used as a phase shift to animate the waves.
var tm float64

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	cv := iup.Canvas().SetAttributes("RASTERSIZE=600x350")
	cv.SetCallback("ACTION", iup.ActionFunc(actionCb))

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

	dlg := iup.Dialog(
		iup.Frame(cv),
	).SetAttribute("TITLE", "Animated Canvas")

	dlg.SetCallback("SHOW_CB", iup.ShowFunc(func(ih iup.Ihandle, state int) int {
		if state == iup.SHOW {
			timer.SetAttribute("RUN", "YES")
		} else if state == iup.MINIMIZE {
			timer.SetAttribute("RUN", "NO")
		} else if state == iup.RESTORE {
			timer.SetAttribute("RUN", "YES")
		}
		return iup.DEFAULT
	}))

	dlg.SetCallback("CLOSE_CB", iup.CloseFunc(func(ih iup.Ihandle) int {
		timer.SetAttribute("RUN", "NO")
		return iup.DEFAULT
	}))

	iup.Show(dlg)
	iup.MainLoop()
}

func actionCb(ih iup.Ihandle) int {
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

	// Draw each wave in a single pass, so its color is set once instead of once per segment.
	drawWave(ih, w, "50 255 100", func(t float64) float64 {
		return midY - math.Sin(t*2*math.Pi*frequency1+tm)*amplitude1
	})
	drawWave(ih, w, "100 150 255", func(t float64) float64 {
		return midY - math.Cos(t*2*math.Pi*frequency2+tm*1.5)*amplitude2
	})

	return iup.DEFAULT
}

// drawWave connects one sample per column into a polyline.
func drawWave(ih iup.Ihandle, w int, color string, y func(t float64) float64) {
	ih.SetAttribute("DRAWCOLOR", color)

	prev := y(0)
	for x := 1; x < w; x++ {
		cur := y(float64(x) / float64(w))
		iup.DrawLine(ih, x-1, int(prev), x, int(cur))
		prev = cur
	}
}
