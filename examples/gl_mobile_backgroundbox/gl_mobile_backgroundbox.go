//go:build (android && gl && egl) || (ios && gl && gles2)

// IupGLBackgroundBox on mobile (Android/iOS): native controls over a GLES 3.0 animated background.
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
uniform float uPhase;
varying vec3 vColor;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    float t = 0.5 + 0.5 * sin(uPhase);
    vColor = mix(aColor, aColor.bgr, t);
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
	uPhase       int32
	initialized  bool
	startTime    time.Time
	lastW, lastH int32
)

func init() { iup.EntryPoint(main) }

func setStatus(s string) { iup.GetHandle("status").SetAttribute("TITLE", s) }

func main() {
	iup.Open()
	defer iup.Close()

	// Reset per-context state: the .so outlives the Activity, the EGL context does not.
	initialized = false
	program, vbo, aPos, aColor, uPhase = 0, 0, 0, 0, -1

	iup.GLCanvasOpen()

	content := iup.Vbox(
		iup.Label("Native controls over a GLES background").SetAttributes(`FGCOLOR="255 255 255"`),
		iup.Button("Native button").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
			setStatus("native button clicked")
			return iup.DEFAULT
		})),
		iup.Toggle("Animate background").SetAttributes(`VALUE=ON, FGCOLOR="255 255 255"`).SetCallback("ACTION",
			iup.ToggleActionFunc(func(_ iup.Ihandle, state int) int {
				running := state == 1
				iup.GetHandle("timer").SetAttribute("RUN", map[bool]string{true: "YES", false: "NO"}[running])
				setStatus(map[bool]string{true: "animating", false: "paused"}[running])
				return iup.DEFAULT
			})),
		iup.Text().SetAttributes(`VISIBLECOLUMNS=18, CUEBANNER="a native text field"`),
		iup.Label("").SetHandle("status").SetAttributes(`EXPAND=HORIZONTAL, FGCOLOR="255 255 255"`),
		iup.Fill(),
	).SetAttributes(`NMARGIN=20x20, NGAP=12, ALIGNMENT=ACENTER`)

	glbox := iup.GLBackgroundBox(content)
	glbox.SetAttribute("BUFFER", "DOUBLE")
	glbox.SetAttribute("CONTEXTVERSION", "3.0")
	glbox.SetAttribute("EXPAND", "YES")
	glbox.SetHandle("glbox")
	glbox.SetCallback("RESIZE_CB", iup.ResizeFunc(onResize))
	glbox.SetCallback("ACTION", iup.ActionFunc(redraw))

	timer := iup.Timer().SetAttributes(`TIME=32`)
	timer.SetHandle("timer")
	timer.SetCallback("ACTION_CB", iup.TimerActionFunc(func(iup.Ihandle) int {
		if initialized {
			redraw(iup.GetHandle("glbox"))
		}
		return iup.DEFAULT
	}))
	timer.SetAttribute("RUN", "YES")

	dlg := iup.Dialog(glbox).SetAttribute("TITLE", "GL BackgroundBox (GLES)")
	iup.Show(dlg)
	iup.MainLoop()
}

func onResize(ih iup.Ihandle, w, h int) int {
	lastW, lastH = int32(w), int32(h)
	if !initialized {
		iup.GLMakeCurrent(ih)
		if errStr := ih.GetAttribute("ERROR"); errStr != "" {
			log.Printf("GLBackgroundBox error: %s", errStr)
			return iup.DEFAULT
		}
		if err := gl.Init(); err != nil {
			log.Printf("gl.Init: %v", err)
			return iup.DEFAULT
		}
		log.Printf("GL_VERSION: %s", iup.GetGlobal("GL_VERSION"))
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
	uPhase = gl.GetUniformLocation(program, gl.Str("uPhase\x00"))

	// full-viewport quad as a triangle strip, one corner colour each
	verts := []float32{
		-1, 1, 0.10, 0.12, 0.32,
		1, 1, 0.10, 0.30, 0.55,
		-1, -1, 0.06, 0.14, 0.34,
		1, -1, 0.28, 0.10, 0.45,
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
	gl.ClearColor(0, 0, 0, 1)
	gl.Clear(gl.COLOR_BUFFER_BIT)
	gl.UseProgram(program)
	gl.Uniform1f(uPhase, float32(time.Since(startTime).Seconds()))

	gl.BindBuffer(gl.ARRAY_BUFFER, vbo)
	gl.EnableVertexAttribArray(aPos)
	gl.VertexAttribPointer(aPos, 2, gl.FLOAT, false, 5*4, gl.PtrOffset(0))
	gl.EnableVertexAttribArray(aColor)
	gl.VertexAttribPointer(aColor, 3, gl.FLOAT, false, 5*4, gl.PtrOffset(2*4))

	gl.DrawArrays(gl.TRIANGLE_STRIP, 0, 4)

	iup.GLSwapBuffers(ih)
	return iup.DEFAULT
}
