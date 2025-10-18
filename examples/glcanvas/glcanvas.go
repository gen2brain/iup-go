package main

import (
	"fmt"
	"log"
	"math"

	"github.com/gen2brain/iup-go/iup"
	"github.com/go-gl/gl/v3.2-compatibility/gl"
)

var rotationAngle float32

func main() {
	iup.Open()
	defer iup.Close()

	iup.GLCanvasOpen()

	canvas := iup.GLCanvas()
	canvas.SetAttribute("BUFFER", "DOUBLE")
	canvas.SetAttribute("RASTERSIZE", "640x480")
	canvas.SetCallback("ACTION", iup.CanvasActionFunc(redraw))
	canvas.SetCallback("MAP_CB", iup.MapFunc(mapCb))

	hbox := iup.Hbox(
		iup.Fill(),
		canvas,
		iup.Fill(),
	)

	dlg := iup.Dialog(hbox).SetAttribute("TITLE", "OpenGL Canvas")

	iup.Map(dlg)

	iup.Show(dlg)
	iup.MainLoop()
}

// mapCb is called when the canvas is first displayed.
// It's the ideal place for one-time OpenGL initializations.
func mapCb(ih iup.Ihandle) int {
	iup.GLMakeCurrent(ih)

	if errStr := iup.GetAttribute(ih, "ERROR"); errStr != "" {
		log.Printf("GL Canvas Error: %s\n", errStr)
		return iup.DEFAULT
	}

	if err := gl.Init(); err != nil {
		log.Printf("Failed to initialize OpenGL: %v\n", err)
		return iup.DEFAULT
	}

	log.Printf("OpenGL Version: %s\n", iup.GetGlobal("GL_VERSION"))
	log.Printf("OpenGL Vendor: %s\n", iup.GetGlobal("GL_VENDOR"))
	log.Printf("OpenGL Renderer: %s\n", iup.GetGlobal("GL_RENDERER"))

	gl.ClearColor(0.0, 0.0, 0.0, 1.0)

	// Enable depth testing for proper 3D rendering
	gl.Enable(gl.DEPTH_TEST)

	return iup.DEFAULT
}

// redraw is called whenever the canvas needs to be repainted.
func redraw(ih iup.Ihandle, x, y float64) int {
	var w, h int32
	size := iup.GetAttribute(ih, "RASTERSIZE")
	fmt.Sscanf(size, "%dx%d", &w, &h)

	iup.GLMakeCurrent(ih)

	if errStr := iup.GetAttribute(ih, "ERROR"); errStr != "" {
		log.Printf("GL Canvas Error: %s\n", errStr)
		return iup.DEFAULT
	}

	// Set up the viewport and perspective
	gl.Viewport(0, 0, w, h)
	gl.MatrixMode(gl.PROJECTION)
	gl.LoadIdentity()

	// Set up a perspective projection matrix (field of view: 45 degrees)
	// This is a replacement for the old gluPerspective function
	aspect := float64(w) / float64(h)
	fovy := 45.0
	f := 1.0 / math.Tan(fovy*(math.Pi/180)/2.0)
	zNear := 1.0
	zFar := 500.0

	projMatrix := [16]float64{
		f / aspect, 0, 0, 0,
		0, f, 0, 0,
		0, 0, (zFar + zNear) / (zNear - zFar), -1,
		0, 0, (2 * zFar * zNear) / (zNear - zFar), 0,
	}
	gl.LoadMatrixd(&projMatrix[0])

	// Set up the model-view matrix (camera and object transformations)
	gl.MatrixMode(gl.MODELVIEW)
	gl.LoadIdentity()

	// Clear the color and depth buffers
	gl.Clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT)

	// Move the cube away from the camera so we can see it
	gl.Translatef(0, 0, -5)

	// Rotate the cube on two axes based on the rotationAngle
	gl.Rotatef(rotationAngle, 1.0, 1.0, 0.0)

	// Draw the object
	drawCube()

	// Swap the front and back buffers to display the rendered image
	iup.GLSwapBuffers(ih)

	// Prepare for the next frame
	rotationAngle += 0.5
	if rotationAngle > 360 {
		rotationAngle -= 360
	}

	// Request an immediate redraw to create animation
	iup.Update(ih)

	return iup.DEFAULT
}

// drawCube draws a cube with a different color for each face.
func drawCube() {
	gl.Begin(gl.QUADS)

	// Front Face (Red)
	gl.Color3f(1.0, 0.0, 0.0)
	gl.Vertex3f(-1.0, -1.0, 1.0)
	gl.Vertex3f(1.0, -1.0, 1.0)
	gl.Vertex3f(1.0, 1.0, 1.0)
	gl.Vertex3f(-1.0, 1.0, 1.0)

	// Back Face (Green)
	gl.Color3f(0.0, 1.0, 0.0)
	gl.Vertex3f(-1.0, -1.0, -1.0)
	gl.Vertex3f(-1.0, 1.0, -1.0)
	gl.Vertex3f(1.0, 1.0, -1.0)
	gl.Vertex3f(1.0, -1.0, -1.0)

	// Top Face (Blue)
	gl.Color3f(0.0, 0.0, 1.0)
	gl.Vertex3f(-1.0, 1.0, -1.0)
	gl.Vertex3f(-1.0, 1.0, 1.0)
	gl.Vertex3f(1.0, 1.0, 1.0)
	gl.Vertex3f(1.0, 1.0, -1.0)

	// Bottom Face (Yellow)
	gl.Color3f(1.0, 1.0, 0.0)
	gl.Vertex3f(-1.0, -1.0, -1.0)
	gl.Vertex3f(1.0, -1.0, -1.0)
	gl.Vertex3f(1.0, -1.0, 1.0)
	gl.Vertex3f(-1.0, -1.0, 1.0)

	// Right face (Cyan)
	gl.Color3f(0.0, 1.0, 1.0)
	gl.Vertex3f(1.0, -1.0, -1.0)
	gl.Vertex3f(1.0, 1.0, -1.0)
	gl.Vertex3f(1.0, 1.0, 1.0)
	gl.Vertex3f(1.0, -1.0, 1.0)

	// Left face (Magenta)
	gl.Color3f(1.0, 0.0, 1.0)
	gl.Vertex3f(-1.0, -1.0, -1.0)
	gl.Vertex3f(-1.0, -1.0, 1.0)
	gl.Vertex3f(-1.0, 1.0, 1.0)
	gl.Vertex3f(-1.0, 1.0, -1.0)

	gl.End()
}
