//go:build gl

package main

import (
	"fmt"
	"log"
	"math"
	"strings"
	"time"

	"github.com/gen2brain/iup-go/iup"
	"github.com/go-gl/gl/v3.2-core/gl"
)

var (
	initialized               bool
	startTime                 time.Time
	program                   uint32
	vao, vbo                  uint32
	timeUniform, transUniform int32
	animationTimer            iup.Ihandle
	resizing                  bool
	firstResize               bool = true
)

func main() {
	iup.Open()
	defer iup.Close()

	iup.GLCanvasOpen()

	canvas := iup.GLCanvas()
	canvas.SetAttribute("BUFFER", "DOUBLE")
	canvas.SetAttribute("RASTERSIZE", "640x480")

	canvas.SetAttribute("ARBCONTEXT", "YES")
	canvas.SetAttribute("CONTEXTVERSION", "3.2")
	canvas.SetAttribute("CONTEXTPROFILE", "CORE")

	canvas.SetCallback("MAP_CB", iup.MapFunc(func(ih iup.Ihandle) int {
		// Make context current first
		iup.GLMakeCurrent(ih)

		// Check for any errors from context creation
		if errStr := iup.GetAttribute(ih, "ERROR"); errStr != "" {
			log.Printf("GL Canvas Error: %s\n", errStr)
			return iup.DEFAULT
		}

		// Initialize OpenGL after the canvas is mapped and context is current
		if err := gl.Init(); err != nil {
			log.Printf("Failed to initialize OpenGL: %v\n", err)
			return iup.DEFAULT
		}

		log.Printf("OpenGL Version: %s\n", iup.GetGlobal("GL_VERSION"))
		log.Printf("OpenGL Vendor: %s\n", iup.GetGlobal("GL_VENDOR"))
		log.Printf("OpenGL Renderer: %s\n", iup.GetGlobal("GL_RENDERER"))

		// Initialize OpenGL resources
		var initSuccess bool
		program, vao, vbo, timeUniform, transUniform, initSuccess = initGL()
		if !initSuccess {
			log.Println("Failed to initialize OpenGL resources")
			return iup.DEFAULT
		}

		startTime = time.Now()
		initialized = true

		// Initial viewport setup
		size := iup.GetAttribute(ih, "RASTERSIZE")
		var w, h int32
		fmt.Sscanf(size, "%dx%d", &w, &h)
		gl.Viewport(0, 0, w, h)

		return iup.DEFAULT
	}))

	canvas.SetCallback("ACTION", iup.ActionFunc(redraw))

	canvas.SetCallback("POSTMESSAGE_CB", iup.PostMessageFunc(func(ih iup.Ihandle, s string, i int, d any) int {
		if s == "RESUME_ANIMATION" {
			resizing = false
			animationTimer.SetAttribute("RUN", "YES")
		}
		return iup.DEFAULT
	}))

	// Set up a timer for continuous redraw (60 FPS)
	animationTimer = iup.Timer()
	animationTimer.SetAttribute("TIME", "16") // ~60 FPS
	animationTimer.SetCallback("ACTION_CB", iup.TimerActionFunc(func(ih iup.Ihandle) int {
		if initialized {
			redraw(canvas)
		}
		return iup.DEFAULT
	}))

	hbox := iup.Hbox(
		iup.Fill(),
		canvas,
		iup.Fill(),
	)

	dlg := iup.Dialog(hbox)
	dlg.SetAttribute("TITLE", "Modern OpenGL Canvas")

	dlg.SetCallback("RESIZE_CB", iup.ResizeFunc(func(ih iup.Ihandle, width, height int) int {
		if initialized {
			iup.GLMakeCurrent(ih)
			gl.Viewport(0, 0, int32(width), int32(height))
		}

		// Skip pause logic on the initial resize (happens right after SHOW_CB)
		if firstResize {
			firstResize = false
			return iup.DEFAULT
		}

		// Pause animation during resize to allow geometry synchronization.
		// Resume after a short delay to let Qt/GTK commit the parent surface.
		if !resizing {
			animationTimer.SetAttribute("RUN", "NO")
			resizing = true

			// Resume animation after 100ms
			go func() {
				time.Sleep(100 * time.Millisecond)
				iup.PostMessage(canvas, "RESUME_ANIMATION", 0, 0)
			}()
		}

		return iup.DEFAULT
	}))

	dlg.SetCallback("SHOW_CB", iup.ShowFunc(func(ih iup.Ihandle, state int) int {
		if state == iup.SHOW {
			animationTimer.SetAttribute("RUN", "YES")
		} else if state == iup.MINIMIZE {
			animationTimer.SetAttribute("RUN", "NO")
		} else if state == iup.RESTORE {
			if !resizing {
				animationTimer.SetAttribute("RUN", "YES")
			}
		}
		return iup.DEFAULT
	}))

	dlg.SetCallback("CLOSE_CB", iup.CloseFunc(func(ih iup.Ihandle) int {
		animationTimer.SetAttribute("RUN", "NO")
		return iup.DEFAULT
	}))

	iup.Show(dlg)
	iup.MainLoop()
}

func redraw(ih iup.Ihandle) int {
	if !initialized {
		return iup.DEFAULT
	}

	iup.GLMakeCurrent(ih)

	// Get canvas dimensions
	size := iup.GetAttribute(ih, "RASTERSIZE")
	var w, h int32
	fmt.Sscanf(size, "%dx%d", &w, &h)

	// Ensure viewport matches canvas size
	gl.Viewport(0, 0, w, h)

	// Create an animated gradient background
	elapsed := float32(time.Since(startTime).Seconds())
	r := (float32(math.Sin(float64(elapsed)*0.5)) + 1.0) * 0.1
	g := (float32(math.Cos(float64(elapsed)*0.3)) + 1.0) * 0.1
	b := 0.2 + (float32(math.Sin(float64(elapsed)*0.7))+1.0)*0.1
	gl.ClearColor(r, g, b, 1.0)
	gl.Clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT)

	// Use our shader program
	gl.UseProgram(program)

	// Pass time uniform for animation
	if timeUniform != -1 {
		gl.Uniform1f(timeUniform, elapsed)
	}

	// Create rotation matrix
	angle := elapsed * 0.5
	cos := float32(math.Cos(float64(angle)))
	sin := float32(math.Sin(float64(angle)))

	// Scale pulsing
	scale := float32(0.6 + 0.1*math.Sin(float64(elapsed)*2))

	// Combined transformation matrix (rotation + scale)
	transform := []float32{
		cos * scale, sin * scale, 0, 0,
		-sin * scale, cos * scale, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1,
	}

	if transUniform != -1 {
		gl.UniformMatrix4fv(transUniform, 1, false, &transform[0])
	}

	// Draw the star
	gl.BindVertexArray(vao)
	gl.DrawArrays(gl.TRIANGLE_FAN, 0, 25) // center + 24 points + 1 to close

	// Draw a second star rotating the opposite direction
	angle2 := -elapsed * 0.3
	cos2 := float32(math.Cos(float64(angle2)))
	sin2 := float32(math.Sin(float64(angle2)))
	scale2 := float32(0.4 + 0.05*math.Cos(float64(elapsed)*3))

	transform2 := []float32{
		cos2 * scale2, sin2 * scale2, 0, 0,
		-sin2 * scale2, cos2 * scale2, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1,
	}

	if transUniform != -1 {
		gl.UniformMatrix4fv(transUniform, 1, false, &transform2[0])
	}
	gl.DrawArrays(gl.TRIANGLE_FAN, 0, 25)

	gl.BindVertexArray(0)

	// Check for OpenGL errors
	if err := gl.GetError(); err != gl.NO_ERROR {
		log.Printf("OpenGL error during draw: 0x%x\n", err)
	}

	iup.GLSwapBuffers(ih)

	return iup.DEFAULT
}

func initGL() (program uint32, vao uint32, vbo uint32, timeUniform int32, transUniform int32, success bool) {
	// Clear any previous OpenGL errors
	for gl.GetError() != gl.NO_ERROR {
	}

	// Try different shader versions based on what's available
	vertexSource, fragmentSource := getShaderSources()

	// Compile shaders
	vertexShader := compileShader(vertexSource, gl.VERTEX_SHADER)
	if vertexShader == 0 {
		// Try fallback shaders
		log.Println("Trying fallback shaders...")
		vertexSource, fragmentSource = getFallbackShaderSources()
		vertexShader = compileShader(vertexSource, gl.VERTEX_SHADER)
		if vertexShader == 0 {
			return 0, 0, 0, 0, 0, false
		}
	}
	defer gl.DeleteShader(vertexShader)

	fragmentShader := compileShader(fragmentSource, gl.FRAGMENT_SHADER)
	if fragmentShader == 0 {
		return 0, 0, 0, 0, 0, false
	}
	defer gl.DeleteShader(fragmentShader)

	// Create and link program
	program = gl.CreateProgram()
	gl.AttachShader(program, vertexShader)
	gl.AttachShader(program, fragmentShader)

	// Bind attribute locations before linking
	gl.BindAttribLocation(program, 0, gl.Str("position\x00"))
	gl.BindAttribLocation(program, 1, gl.Str("color\x00"))

	gl.LinkProgram(program)

	var status int32
	gl.GetProgramiv(program, gl.LINK_STATUS, &status)
	if status == gl.FALSE {
		var logLength int32
		gl.GetProgramiv(program, gl.INFO_LOG_LENGTH, &logLength)
		if logLength > 0 {
			logStr := strings.Repeat("\x00", int(logLength+1))
			gl.GetProgramInfoLog(program, logLength, nil, gl.Str(logStr))
			log.Printf("Failed to link program: %s\n", logStr)
		}
		return 0, 0, 0, 0, 0, false
	}

	// Get uniform locations
	timeUniform = gl.GetUniformLocation(program, gl.Str("time\x00"))
	transUniform = gl.GetUniformLocation(program, gl.Str("transform\x00"))

	if timeUniform == -1 {
		log.Println("Warning: 'time' uniform not found")
	}
	if transUniform == -1 {
		log.Println("Warning: 'transform' uniform not found")
	}

	// Create a colorful star/flower pattern
	vertices := createStarVertices(12, 0.4, 0.8)

	// Create and bind VAO
	gl.GenVertexArrays(1, &vao)
	gl.BindVertexArray(vao)

	// Create and bind VBO
	gl.GenBuffers(1, &vbo)
	gl.BindBuffer(gl.ARRAY_BUFFER, vbo)
	gl.BufferData(gl.ARRAY_BUFFER, len(vertices)*4, gl.Ptr(vertices), gl.STATIC_DRAW)

	// Set up vertex attributes
	// Position attribute (location = 0)
	gl.EnableVertexAttribArray(0)
	gl.VertexAttribPointer(0, 2, gl.FLOAT, false, 5*4, gl.PtrOffset(0))

	// Color attribute (location = 1)
	gl.EnableVertexAttribArray(1)
	gl.VertexAttribPointer(1, 3, gl.FLOAT, false, 5*4, gl.PtrOffset(2*4))

	// Unbind VAO
	gl.BindVertexArray(0)

	// Enable depth testing and blending
	gl.Enable(gl.DEPTH_TEST)
	gl.Enable(gl.BLEND)
	gl.BlendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA)

	// Check for any OpenGL errors
	if err := gl.GetError(); err != gl.NO_ERROR {
		log.Printf("OpenGL error after initialization: 0x%x\n", err)
		return 0, 0, 0, 0, 0, false
	}

	return program, vao, vbo, timeUniform, transUniform, true
}

func compileShader(source string, shaderType uint32) uint32 {
	shader := gl.CreateShader(shaderType)
	csources, free := gl.Strs(source)
	gl.ShaderSource(shader, 1, csources, nil)
	free()
	gl.CompileShader(shader)

	var status int32
	gl.GetShaderiv(shader, gl.COMPILE_STATUS, &status)
	if status == gl.FALSE {
		var logLength int32
		gl.GetShaderiv(shader, gl.INFO_LOG_LENGTH, &logLength)

		shaderTypeStr := "vertex"
		if shaderType == gl.FRAGMENT_SHADER {
			shaderTypeStr = "fragment"
		}

		if logLength > 0 {
			logStr := strings.Repeat("\x00", int(logLength+1))
			gl.GetShaderInfoLog(shader, logLength, nil, gl.Str(logStr))
			log.Printf("Failed to compile %s shader:\n%s", shaderTypeStr, logStr)
		} else {
			log.Printf("Failed to compile %s shader: no error log available", shaderTypeStr)
		}

		return 0
	}

	return shader
}

func createStarVertices(points int, innerRadius, outerRadius float32) []float32 {
	vertices := []float32{}

	// Center point (white)
	vertices = append(vertices, 0, 0, 1, 1, 1)

	angleStep := float32(2.0 * math.Pi / float64(points*2))

	for i := 0; i <= points*2; i++ {
		angle := float32(i) * angleStep
		radius := innerRadius
		if i%2 == 0 {
			radius = outerRadius
		}

		x := radius * float32(math.Cos(float64(angle)))
		y := radius * float32(math.Sin(float64(angle)))

		// Create a gradient of colors around the star
		hue := float32(i) / float32(points*2)
		r, g, b := hslToRgb(hue, 0.8, 0.6)

		vertices = append(vertices, x, y, r, g, b)
	}

	return vertices
}

func hslToRgb(h, s, l float32) (float32, float32, float32) {
	var r, g, b float32

	if s == 0 {
		r, g, b = l, l, l
	} else {
		var q float32
		if l < 0.5 {
			q = l * (1 + s)
		} else {
			q = l + s - l*s
		}

		p := 2*l - q
		r = hueToRgb(p, q, h+1.0/3.0)
		g = hueToRgb(p, q, h)
		b = hueToRgb(p, q, h-1.0/3.0)
	}

	return r, g, b
}

func hueToRgb(p, q, t float32) float32 {
	if t < 0 {
		t += 1
	}
	if t > 1 {
		t -= 1
	}
	if t < 1.0/6.0 {
		return p + (q-p)*6*t
	}
	if t < 1.0/2.0 {
		return q
	}
	if t < 2.0/3.0 {
		return p + (q-p)*(2.0/3.0-t)*6
	}

	return p
}

func getShaderSources() (vertex, fragment string) {
	// OpenGL 3.2 Core Profile shaders
	vertex = `
#version 150 core
in vec2 position;
in vec3 color;
out vec3 fragColor;
uniform float time;
uniform mat4 transform;

void main() {
    fragColor = color;
    gl_Position = transform * vec4(position, 0.0, 1.0);
}
` + "\x00"

	fragment = `
#version 150 core
in vec3 fragColor;
out vec4 outColor;
uniform float time;

void main() {
    // Add some pulsing effect to the colors
    float pulse = (sin(time * 2.0) + 1.0) * 0.5;
    vec3 modColor = fragColor * (0.7 + 0.3 * pulse);
    outColor = vec4(modColor, 1.0);
}
` + "\x00"

	return vertex, fragment
}

func getFallbackShaderSources() (vertex, fragment string) {
	// OpenGL 3.3 Core Profile shaders (slightly different)
	vertex = `
#version 330 core
layout (location = 0) in vec2 position;
layout (location = 1) in vec3 color;
out vec3 fragColor;
uniform float time;
uniform mat4 transform;

void main() {
    fragColor = color;
    gl_Position = transform * vec4(position, 0.0, 1.0);
}
` + "\x00"

	fragment = `
#version 330 core
in vec3 fragColor;
out vec4 FragColor;
uniform float time;

void main() {
    // Add some pulsing effect to the colors
    float pulse = (sin(time * 2.0) + 1.0) * 0.5;
    vec3 modColor = fragColor * (0.7 + 0.3 * pulse);
    FragColor = vec4(modColor, 1.0);
}
` + "\x00"

	return vertex, fragment
}
