//go:build gl

package main

import (
	"fmt"
	"log"
	"math"

	"github.com/gen2brain/iup-go/iup"
	"github.com/go-gl/gl/v3.2-compatibility/gl"
)

var (
	initialized bool
	phase       float64
)

func init() { iup.EntryPoint(main) }

func setStatus(s string) { iup.GetHandle("status").SetAttribute("TITLE", s) }

func main() {
	iup.Open()
	defer iup.Close()
	iup.GLCanvasOpen()

	// Native controls hosted on top of the GL-drawn background.
	content := iup.Vbox(
		iup.Label("Native controls over an OpenGL background").SetAttributes(`FONT="Helvetica, Bold 11", FGCOLOR="255 255 255"`),
		iup.Button("Native button").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
			setStatus("native button clicked")
			return iup.DEFAULT
		})),
		iup.Toggle("Animate background").SetAttributes(`VALUE=ON, FGCOLOR="255 255 255"`).SetCallback("ACTION",
			iup.ToggleActionFunc(func(_ iup.Ihandle, state int) int {
				iup.GetHandle("timer").SetAttribute("RUN", map[bool]string{true: "YES", false: "NO"}[state == 1])
				setStatus(map[bool]string{true: "animating", false: "paused"}[state == 1])
				return iup.DEFAULT
			})),
		iup.Text().SetAttributes(`VISIBLECOLUMNS=22, CUEBANNER="a native text field"`),
		iup.Label("").SetHandle("status").SetAttributes(`EXPAND=HORIZONTAL, FGCOLOR="255 255 255"`),
	).SetAttributes(`NMARGIN=24x24, NGAP=14, ALIGNMENT=ACENTER`)

	glbox := iup.GLBackgroundBox(content)
	glbox.SetAttribute("BUFFER", "DOUBLE")
	glbox.SetAttribute("RASTERSIZE", "460x320")
	glbox.SetHandle("glbox")
	glbox.SetCallback("ACTION", iup.ActionFunc(redraw))
	glbox.SetCallback("MAP_CB", iup.MapFunc(func(ih iup.Ihandle) int {
		iup.GLMakeCurrent(ih)
		if err := gl.Init(); err != nil {
			log.Printf("gl init: %v", err)
			return iup.DEFAULT
		}
		initialized = true
		return iup.DEFAULT
	}))

	timer := iup.Timer().SetAttributes(`TIME=40`)
	timer.SetHandle("timer")
	timer.SetCallback("ACTION_CB", iup.TimerActionFunc(func(iup.Ihandle) int {
		phase += 0.03
		iup.Redraw(iup.GetHandle("glbox"), 0)
		return iup.DEFAULT
	}))
	timer.SetAttribute("RUN", "YES")

	dlg := iup.Dialog(glbox).SetAttribute("TITLE", "GL BackgroundBox")
	iup.Show(dlg)
	iup.MainLoop()
}

func redraw(ih iup.Ihandle) int {
	if !initialized {
		return iup.DEFAULT
	}

	var w, h int32
	fmt.Sscanf(iup.GetAttribute(ih, "RASTERSIZE"), "%dx%d", &w, &h)

	iup.GLMakeCurrent(ih)
	gl.Viewport(0, 0, w, h)
	gl.ClearColor(0, 0, 0, 1)
	gl.Clear(gl.COLOR_BUFFER_BIT)

	t := float32(0.5 + 0.5*math.Sin(phase))
	gl.Begin(gl.QUADS)
	gl.Color3f(0.10, 0.12, 0.32)
	gl.Vertex2f(-1, 1)
	gl.Color3f(0.10, 0.30+0.25*t, 0.55)
	gl.Vertex2f(1, 1)
	gl.Color3f(0.28, 0.10, 0.45)
	gl.Vertex2f(1, -1)
	gl.Color3f(0.06+0.30*t, 0.14, 0.34)
	gl.Vertex2f(-1, -1)
	gl.End()

	iup.GLSwapBuffers(ih)
	return iup.DEFAULT
}
