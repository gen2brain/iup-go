//go:build gl

package main

import (
	"fmt"
	"log"

	"github.com/gen2brain/iup-go/iup"
	"github.com/go-gl/gl/v3.2-compatibility/gl"
)

var initialized bool

func main() {
	iup.Open()
	defer iup.Close()

	iup.GLCanvasOpen()

	canvas := iup.GLCanvas()
	canvas.SetAttribute("BUFFER", "DOUBLE")
	canvas.SetAttribute("RASTERSIZE", "240x320")

	canvas.SetCallback("ACTION", iup.ActionFunc(redraw))
	canvas.SetCallback("MAP_CB", iup.MapFunc(func(ih iup.Ihandle) int {
		iup.GLMakeCurrent(ih)
		if err := gl.Init(); err != nil {
			log.Printf("Failed to initialize OpenGL: %v\n", err)
			return iup.DEFAULT
		}
		initialized = true
		return iup.DEFAULT
	}))

	hbox := iup.Hbox(
		iup.Fill(),
		canvas,
		iup.Fill(),
	)

	dlg := iup.Dialog(hbox).SetAttribute("TITLE", "GLCanvas")

	iup.Show(dlg)
	iup.MainLoop()
}

func redraw(ih iup.Ihandle) int {
	if !initialized {
		return iup.DEFAULT
	}

	var w, h int32
	size := iup.GetAttribute(ih, "RASTERSIZE")
	fmt.Sscanf(size, "%dx%d", &w, &h)

	iup.GLMakeCurrent(ih)

	gl.Viewport(0, 0, w, h)

	gl.ClearColor(1.0, 1.0, 1.0, 1.0)
	gl.Clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT)

	gl.Color3f(1.0, 0, 0)
	gl.Begin(gl.QUADS)
	gl.Vertex2f(0.9, 0.9)
	gl.Vertex2f(0.9, -0.9)
	gl.Vertex2f(-0.9, -0.9)
	gl.Vertex2f(-0.9, 0.9)
	gl.End()

	iup.GLSwapBuffers(ih)

	return iup.DEFAULT
}
