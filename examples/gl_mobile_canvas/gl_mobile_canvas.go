//go:build (android && gl && egl) || (ios && gl && gles2)

// IupGLCanvas on mobile (Android/iOS): solid red quad drawn from ACTION on every layout.
package main

import (
	"log"
	"strings"

	"github.com/gen2brain/iup-go/iup"
	gl "github.com/go-gl/gl/v3.0/gles2"
)

const vertexSrc = `
attribute vec2 aPos;
void main() { gl_Position = vec4(aPos, 0.0, 1.0); }
` + "\x00"

const fragmentSrc = `
precision mediump float;
void main() { gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0); }
` + "\x00"

var (
	program     uint32
	vbo         uint32
	aPos        uint32
	initialized bool
	canvas      iup.Ihandle
	lastW       int32
	lastH       int32
)

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	initialized = false
	program, vbo, aPos = 0, 0, 0

	iup.GLCanvasOpen()

	canvas = iup.GLCanvas()
	canvas.SetAttribute("BUFFER", "DOUBLE")
	canvas.SetAttribute("CONTEXTVERSION", "3.0")
	canvas.SetAttribute("EXPAND", "YES")

	canvas.SetCallback("RESIZE_CB", iup.ResizeFunc(onResize))
	canvas.SetCallback("ACTION", iup.ActionFunc(redraw))

	dlg := iup.Dialog(canvas).SetAttribute("TITLE", "GLCanvas (red quad)")
	iup.Show(dlg)
	iup.MainLoop()
}

func onResize(ih iup.Ihandle, w, h int) int {
	lastW, lastH = int32(w), int32(h)
	if !initialized {
		iup.GLMakeCurrent(ih)
		if errStr := ih.GetAttribute("ERROR"); errStr != "" {
			log.Printf("GLCanvas error: %s", errStr)
			return iup.DEFAULT
		}
		if err := gl.Init(); err != nil {
			log.Printf("gl.Init: %v", err)
			return iup.DEFAULT
		}
		log.Printf("GL_VERSION: %s", iup.GetGlobal("GL_VERSION"))
		log.Printf("GL_RENDERER: %s", iup.GetGlobal("GL_RENDERER"))
		if !setupGL() {
			return iup.DEFAULT
		}
		initialized = true
	}
	return iup.DEFAULT
}

func setupGL() bool {
	vs := compileShader(gl.VERTEX_SHADER, vertexSrc)
	fs := compileShader(gl.FRAGMENT_SHADER, fragmentSrc)
	if vs == 0 || fs == 0 {
		return false
	}

	program = gl.CreateProgram()
	gl.AttachShader(program, vs)
	gl.AttachShader(program, fs)
	gl.LinkProgram(program)
	gl.DeleteShader(vs)
	gl.DeleteShader(fs)

	var status int32
	gl.GetProgramiv(program, gl.LINK_STATUS, &status)
	if status == gl.FALSE {
		var n int32
		gl.GetProgramiv(program, gl.INFO_LOG_LENGTH, &n)
		buf := strings.Repeat("\x00", int(n+1))
		gl.GetProgramInfoLog(program, n, nil, gl.Str(buf))
		log.Printf("link failed: %s", buf)
		return false
	}

	aPos = uint32(gl.GetAttribLocation(program, gl.Str("aPos\x00")))

	verts := []float32{
		-0.9, -0.9,
		0.9, -0.9,
		0.9, 0.9,
		-0.9, -0.9,
		0.9, 0.9,
		-0.9, 0.9,
	}
	gl.GenBuffers(1, &vbo)
	gl.BindBuffer(gl.ARRAY_BUFFER, vbo)
	gl.BufferData(gl.ARRAY_BUFFER, len(verts)*4, gl.Ptr(verts), gl.STATIC_DRAW)
	return true
}

func compileShader(kind uint32, src string) uint32 {
	s := gl.CreateShader(kind)
	csrc, free := gl.Strs(src)
	gl.ShaderSource(s, 1, csrc, nil)
	free()
	gl.CompileShader(s)

	var status int32
	gl.GetShaderiv(s, gl.COMPILE_STATUS, &status)
	if status == gl.FALSE {
		var n int32
		gl.GetShaderiv(s, gl.INFO_LOG_LENGTH, &n)
		buf := strings.Repeat("\x00", int(n+1))
		gl.GetShaderInfoLog(s, n, nil, gl.Str(buf))
		log.Printf("shader compile failed: %s", buf)
		gl.DeleteShader(s)
		return 0
	}
	return s
}

func redraw(ih iup.Ihandle) int {
	if !initialized {
		return iup.DEFAULT
	}
	iup.GLMakeCurrent(ih)

	gl.Viewport(0, 0, lastW, lastH)
	gl.ClearColor(1.0, 1.0, 1.0, 1.0)
	gl.Clear(gl.COLOR_BUFFER_BIT)

	gl.UseProgram(program)
	gl.BindBuffer(gl.ARRAY_BUFFER, vbo)
	gl.EnableVertexAttribArray(aPos)
	gl.VertexAttribPointer(aPos, 2, gl.FLOAT, false, 2*4, gl.PtrOffset(0))
	gl.DrawArrays(gl.TRIANGLES, 0, 6)

	iup.GLSwapBuffers(ih)
	return iup.DEFAULT
}
