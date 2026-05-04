//go:build (android && gl && egl) || (ios && gl && gles2)

// IupGLCanvas on mobile (Android/iOS): GLES 3.0 rotating colored triangle.
package main

import (
	"log"
	"strings"
	"time"

	"github.com/gen2brain/iup-go/iup"
	gl "github.com/go-gl/gl/v3.0/gles2"
)

const vertexSrc = `
attribute vec2 aPos;
attribute vec3 aColor;
uniform float uAngle;
varying vec3 vColor;
void main() {
    float c = cos(uAngle), s = sin(uAngle);
    vec2 p = vec2(c*aPos.x - s*aPos.y, s*aPos.x + c*aPos.y);
    gl_Position = vec4(p, 0.0, 1.0);
    vColor = aColor;
}
` + "\x00"

const fragmentSrc = `
precision mediump float;
varying vec3 vColor;
void main() { gl_FragColor = vec4(vColor, 1.0); }
` + "\x00"

var (
	program      uint32
	vbo          uint32
	aPos, aColor uint32
	uAngle       int32
	initialized  bool
	startTime    time.Time
	canvas       iup.Ihandle
	lastW, lastH int32
)

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	// Reset per-context state: .so outlives Activity, EGL context does not.
	initialized = false
	program, vbo, aPos, aColor, uAngle = 0, 0, 0, 0, -1

	iup.GLCanvasOpen()

	canvas = iup.GLCanvas()
	canvas.SetAttribute("BUFFER", "DOUBLE")
	canvas.SetAttribute("CONTEXTVERSION", "3.0")
	canvas.SetAttribute("EXPAND", "YES")

	canvas.SetCallback("RESIZE_CB", iup.ResizeFunc(onResize))
	canvas.SetCallback("ACTION", iup.ActionFunc(draw))

	timer := iup.Timer()
	timer.SetAttribute("TIME", "16")
	timer.SetCallback("ACTION_CB", iup.TimerActionFunc(func(ih iup.Ihandle) int {
		if initialized {
			draw(canvas)
		}
		return iup.DEFAULT
	}))
	timer.SetAttribute("RUN", "YES")

	dlg := iup.Dialog(canvas).SetAttribute("TITLE", "IupGLCanvas (GLES)")
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
		startTime = time.Now()
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
	aColor = uint32(gl.GetAttribLocation(program, gl.Str("aColor\x00")))
	uAngle = gl.GetUniformLocation(program, gl.Str("uAngle\x00"))

	verts := []float32{
		0.00, 0.75, 1.0, 0.2, 0.2,
		-0.75, -0.50, 0.2, 1.0, 0.2,
		0.75, -0.50, 0.2, 0.4, 1.0,
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

func draw(ih iup.Ihandle) int {
	if !initialized {
		return iup.DEFAULT
	}
	iup.GLMakeCurrent(ih)

	elapsed := float32(time.Since(startTime).Seconds())

	gl.Viewport(0, 0, lastW, lastH)
	gl.ClearColor(0.08, 0.08, 0.12, 1.0)
	gl.Clear(gl.COLOR_BUFFER_BIT)
	gl.UseProgram(program)
	gl.Uniform1f(uAngle, elapsed)

	gl.BindBuffer(gl.ARRAY_BUFFER, vbo)
	gl.EnableVertexAttribArray(aPos)
	gl.VertexAttribPointer(aPos, 2, gl.FLOAT, false, 5*4, gl.PtrOffset(0))
	gl.EnableVertexAttribArray(aColor)
	gl.VertexAttribPointer(aColor, 3, gl.FLOAT, false, 5*4, gl.PtrOffset(2*4))

	gl.DrawArrays(gl.TRIANGLES, 0, 3)

	iup.GLSwapBuffers(ih)
	return iup.DEFAULT
}
