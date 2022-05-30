package main

import (
	"fmt"
	"log"

	"github.com/gen2brain/iup-go/iup"
	"github.com/go-gl/gl/v3.3-compatibility/gl"
)

func main() {
	iup.Open()
	defer iup.Close()

	iup.GLCanvasOpen()

	canvas := iup.GLCanvas()
	canvas.SetAttribute("BUFFER", "DOUBLE")
	canvas.SetAttribute("RASTERSIZE", "123x200")
	//canvas.SetAttribute("ARBCONTEXT", "YES")
	//canvas.SetAttribute("CONTEXTPROFILE", "COMPATIBILITY")
	//canvas.SetAttribute("CONTEXTVERSION", "3.3")
	canvas.SetCallback("ACTION", iup.CanvasActionFunc(redraw))
	canvas.SetCallback("SWAPBUFFERS_CB", iup.SwapBuffersFunc(swapBuffersCb))

	hbox := iup.Hbox(
		iup.Fill(),
		canvas,
		iup.Fill(),
	)

	dlg := iup.Dialog(hbox).SetAttribute("TITLE", "GLCanvas")

	iup.Map(dlg)

	iup.GLMakeCurrent(canvas)
	if err := gl.Init(); err != nil {
		log.Fatalln(err)
	}

	iup.Show(dlg)
	iup.MainLoop()
}

func swapBuffersCb(ih iup.Ihandle) int {
	//fmt.Println("SWAPBUFFERS")
	return iup.DEFAULT
}

func redraw(ih iup.Ihandle, x, y float64) int {
	//fmt.Println("REDRAW")
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
