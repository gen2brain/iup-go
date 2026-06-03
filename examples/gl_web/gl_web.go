//go:build js && wasm && gl

// IupGLCanvas on WebAssembly: a colored triangle drawn with WebGL2 via syscall/js.
package main

import (
	"log"
	"syscall/js"

	"github.com/gen2brain/iup-go/iup"
)

const vertexSrc = `#version 300 es
in vec2 aPos;
in vec3 aColor;
out vec3 vColor;
void main() {
	vColor = aColor;
	gl_Position = vec4(aPos, 0.0, 1.0);
}`

const fragmentSrc = `#version 300 es
precision mediump float;
in vec3 vColor;
out vec4 fragColor;
void main() { fragColor = vec4(vColor, 1.0); }`

var (
	gl          js.Value
	program     js.Value
	initialized bool
	lastW       int
	lastH       int
)

func main() {
	iup.Open()
	defer iup.Close()

	iup.GLCanvasOpen()

	canvas := iup.GLCanvas()
	canvas.SetAttribute("BUFFER", "DOUBLE")
	canvas.SetAttribute("CONTEXTVERSION", "3.0")
	canvas.SetAttribute("EXPAND", "YES")
	canvas.SetAttribute("RASTERSIZE", "400x300")

	canvas.SetCallback("RESIZE_CB", iup.ResizeFunc(onResize))
	canvas.SetCallback("ACTION", iup.ActionFunc(redraw))

	dlg := iup.Dialog(canvas).SetAttribute("TITLE", "GLCanvas (WebGL2)")
	iup.Show(dlg)
	iup.MainLoop()
}

func onResize(ih iup.Ihandle, w, h int) int {
	lastW, lastH = w, h
	if initialized {
		return iup.DEFAULT
	}

	iup.GLMakeCurrent(ih)
	if e := ih.GetAttribute("ERROR"); e != "" {
		log.Printf("GLCanvas error: %s", e)
		return iup.DEFAULT
	}

	el := iup.GLCanvasElement(ih)
	if !el.Truthy() {
		log.Print("GLCanvas: no DOM element")
		return iup.DEFAULT
	}
	gl = el.Call("getContext", "webgl2")
	if !gl.Truthy() {
		log.Print("WebGL2 not available")
		return iup.DEFAULT
	}

	log.Printf("GL_VERSION: %s", iup.GetGlobal("GL_VERSION"))
	if !setupGL() {
		return iup.DEFAULT
	}
	initialized = true
	return iup.DEFAULT
}

func compileShader(kind, src string) js.Value {
	var k js.Value
	if kind == "vertex" {
		k = gl.Get("VERTEX_SHADER")
	} else {
		k = gl.Get("FRAGMENT_SHADER")
	}
	s := gl.Call("createShader", k)
	gl.Call("shaderSource", s, src)
	gl.Call("compileShader", s)
	if !gl.Call("getShaderParameter", s, gl.Get("COMPILE_STATUS")).Bool() {
		log.Printf("shader compile failed: %s", gl.Call("getShaderInfoLog", s).String())
		return js.Null()
	}
	return s
}

func setupGL() bool {
	vs := compileShader("vertex", vertexSrc)
	fs := compileShader("fragment", fragmentSrc)
	if !vs.Truthy() || !fs.Truthy() {
		return false
	}

	program = gl.Call("createProgram")
	gl.Call("attachShader", program, vs)
	gl.Call("attachShader", program, fs)
	gl.Call("linkProgram", program)
	if !gl.Call("getProgramParameter", program, gl.Get("LINK_STATUS")).Bool() {
		log.Printf("link failed: %s", gl.Call("getProgramInfoLog", program).String())
		return false
	}

	verts := []float32{
		0.0, 0.8, 1.0, 0.0, 0.0,
		-0.8, -0.8, 0.0, 1.0, 0.0,
		0.8, -0.8, 0.0, 0.0, 1.0,
	}
	arr := js.Global().Get("Float32Array").New(len(verts))
	for i, v := range verts {
		arr.SetIndex(i, v)
	}

	buf := gl.Call("createBuffer")
	gl.Call("bindBuffer", gl.Get("ARRAY_BUFFER"), buf)
	gl.Call("bufferData", gl.Get("ARRAY_BUFFER"), arr, gl.Get("STATIC_DRAW"))

	stride := 5 * 4
	gl.Call("useProgram", program)
	bindAttrib("aPos", 2, 0, stride)
	bindAttrib("aColor", 3, 2*4, stride)
	return true
}

func bindAttrib(name string, size, offset, stride int) {
	loc := gl.Call("getAttribLocation", program, name)
	gl.Call("enableVertexAttribArray", loc)
	gl.Call("vertexAttribPointer", loc, size, gl.Get("FLOAT"), false, stride, offset)
}

func redraw(ih iup.Ihandle) int {
	if !initialized {
		return iup.DEFAULT
	}
	iup.GLMakeCurrent(ih)

	gl.Call("viewport", 0, 0, lastW, lastH)
	gl.Call("clearColor", 0.12, 0.12, 0.16, 1.0)
	gl.Call("clear", gl.Get("COLOR_BUFFER_BIT"))

	gl.Call("useProgram", program)
	gl.Call("drawArrays", gl.Get("TRIANGLES"), 0, 3)

	iup.GLSwapBuffers(ih)
	return iup.DEFAULT
}
