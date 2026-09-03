package main

import (
	"fmt"
	"math"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

type readings struct {
	gravityX, gravityY, gravityZ float64
	heading                      float64
	shake                        float64
	hasGravity, hasHeading       bool
}

var state readings

func main() {
	iup.Open()
	defer iup.Close()

	cv := iup.Canvas().SetAttributes("EXPAND=YES, RASTERSIZE=320x480")
	cv.SetCallback("ACTION", iup.ActionFunc(draw))

	gravity := iup.Sensor().SetAttributes("TYPE=GRAVITY, INTERVAL=50")
	gravity.SetCallback("SENSOR_CB", iup.SensorFunc(func(ih iup.Ihandle, x, y, z float64) int {
		state.gravityX, state.gravityY, state.gravityZ = x, y, z
		state.hasGravity = true
		return iup.DEFAULT
	}))

	compass := iup.Sensor().SetAttributes("TYPE=COMPASS, INTERVAL=100")
	compass.SetCallback("SENSOR_CB", iup.SensorFunc(func(ih iup.Ihandle, x, y, z float64) int {
		state.heading = x
		state.hasHeading = true
		return iup.DEFAULT
	}))

	motion := iup.Sensor().SetAttributes("TYPE=LINEARACCELERATION, INTERVAL=50")
	motion.SetCallback("SENSOR_CB", iup.SensorFunc(func(ih iup.Ihandle, x, y, z float64) int {
		magnitude := math.Sqrt(x*x + y*y + z*z)
		if magnitude > state.shake {
			state.shake = magnitude
		}
		return iup.DEFAULT
	}))

	for _, sensor := range []iup.Ihandle{gravity, compass, motion} {
		sensor.SetCallback("ERROR_CB", iup.ErrorFunc(func(ih iup.Ihandle, message string) int {
			iup.GetHandle("status").SetAttribute("TITLE", ih.GetAttribute("TYPE")+": "+message)
			return iup.DEFAULT
		}))
	}

	timer := iup.Timer().SetAttribute("TIME", "33")
	timer.SetCallback("ACTION_CB", iup.TimerActionFunc(func(ih iup.Ihandle) int {
		state.shake *= 0.92
		iup.Update(cv)
		return iup.DEFAULT
	}))

	active := iup.Toggle("Active")
	active.SetCallback("ACTION", iup.ToggleActionFunc(func(ih iup.Ihandle, on int) int {
		value := "NO"
		if on == 1 {
			value = "YES"
		}
		for _, sensor := range []iup.Ihandle{gravity, compass, motion} {
			sensor.SetAttribute("ACTIVE", value)
		}
		timer.SetAttribute("RUN", value)
		return iup.DEFAULT
	}))

	status := iup.Label(fmt.Sprintf("Gravity: %s, compass: %s, motion: %s",
		gravity.GetAttribute("AVAILABLE"), compass.GetAttribute("AVAILABLE"), motion.GetAttribute("AVAILABLE")))
	status.SetAttribute("EXPAND", "HORIZONTAL")
	status.SetHandle("status")

	dlg := iup.Dialog(
		iup.Vbox(active, cv, status).SetAttributes(`GAP=5, MARGIN=10x10`),
	).SetAttributes(`TITLE="Sensor"`)

	iup.Show(dlg)
	iup.MainLoop()
}

func draw(ih iup.Ihandle) int {
	iup.DrawBegin(ih)
	defer iup.DrawEnd(ih)

	w, h := iup.DrawGetSize(ih)
	ih.SetAttributes(`DRAWCOLOR="24 28 36", DRAWSTYLE=FILL`)
	iup.DrawRectangle(ih, 0, 0, w, h)

	size := w
	if h/2 < size {
		size = h / 2
	}
	radius := size*7/20 - 10

	drawLevel(ih, w/2, h/4, radius)
	drawCompass(ih, w/2, h*3/4, radius)
	drawShake(ih, w, h)

	return iup.DEFAULT
}

func drawLevel(ih iup.Ihandle, cx, cy, r int) {
	ih.SetAttributes(`DRAWCOLOR="40 46 58", DRAWSTYLE=FILL`)
	iup.DrawArc(ih, cx-r, cy-r, cx+r, cy+r, 0, 360)
	ih.SetAttributes(`DRAWCOLOR="90 100 120", DRAWSTYLE=STROKE, DRAWLINEWIDTH=2`)
	iup.DrawArc(ih, cx-r, cy-r, cx+r, cy+r, 0, 360)
	iup.DrawArc(ih, cx-r/4, cy-r/4, cx+r/4, cy+r/4, 0, 360)
	iup.DrawLine(ih, cx-r, cy, cx+r, cy)
	iup.DrawLine(ih, cx, cy-r, cx, cy+r)

	if !state.hasGravity {
		return
	}

	tiltX := state.gravityX / 9.81
	tiltY := state.gravityY / 9.81
	bx := cx + int(tiltX*float64(r))
	by := cy - int(tiltY*float64(r))
	br := r / 6
	color := "70 200 110"
	if math.Abs(tiltX) > 0.03 || math.Abs(tiltY) > 0.03 {
		color = "230 180 60"
	}
	ih.SetAttributes(fmt.Sprintf(`DRAWCOLOR="%s", DRAWSTYLE=FILL`, color))
	iup.DrawArc(ih, bx-br, by-br, bx+br, by+br, 0, 360)

	ih.SetAttributes(`DRAWCOLOR="220 225 235", DRAWFONT="Helvetica, Bold 14"`)
	text := fmt.Sprintf("%.1f° / %.1f°", math.Asin(clamp(tiltX))*180/math.Pi, math.Asin(clamp(tiltY))*180/math.Pi)
	tw, th := iup.DrawGetTextSize(ih, text)
	iup.DrawText(ih, text, cx-tw/2, cy+r+8, tw, th)
}

func drawCompass(ih iup.Ihandle, cx, cy, r int) {
	ih.SetAttributes(`DRAWCOLOR="40 46 58", DRAWSTYLE=FILL`)
	iup.DrawArc(ih, cx-r, cy-r, cx+r, cy+r, 0, 360)
	ih.SetAttributes(`DRAWCOLOR="90 100 120", DRAWSTYLE=STROKE, DRAWLINEWIDTH=2`)
	iup.DrawArc(ih, cx-r, cy-r, cx+r, cy+r, 0, 360)

	rotation := 0.0
	if state.hasHeading {
		rotation = -state.heading * math.Pi / 180
	}

	ih.SetAttributes(`DRAWCOLOR="220 225 235", DRAWFONT="Helvetica, Bold 16"`)
	for i, label := range []string{"N", "E", "S", "W"} {
		angle := rotation + float64(i)*math.Pi/2
		x := cx + int(float64(r-18)*math.Sin(angle))
		y := cy - int(float64(r-18)*math.Cos(angle))
		if i == 0 {
			ih.SetAttribute("DRAWCOLOR", "230 80 70")
		} else {
			ih.SetAttribute("DRAWCOLOR", "220 225 235")
		}
		tw, th := iup.DrawGetTextSize(ih, label)
		iup.DrawText(ih, label, x-tw/2, y-th/2, tw, th)
	}
	ih.SetAttribute("DRAWCOLOR", "90 100 120")
	for i := 0; i < 36; i++ {
		angle := rotation + float64(i)*math.Pi/18
		outer := r - 4
		inner := r - 10
		if i%9 == 0 {
			inner = r - 14
		}
		iup.DrawLine(ih, cx+int(float64(inner)*math.Sin(angle)), cy-int(float64(inner)*math.Cos(angle)),
			cx+int(float64(outer)*math.Sin(angle)), cy-int(float64(outer)*math.Cos(angle)))
	}

	needle := r * 6 / 10
	ih.SetAttributes(`DRAWCOLOR="230 80 70", DRAWSTYLE=FILL`)
	iup.DrawPolygon(ih, []int{cx, cy - needle, cx - 8, cy, cx + 8, cy}, 3)
	ih.SetAttributes(`DRAWCOLOR="220 225 235", DRAWSTYLE=FILL`)
	iup.DrawPolygon(ih, []int{cx, cy + needle, cx - 8, cy, cx + 8, cy}, 3)

	text := "no compass"
	if state.hasHeading {
		text = fmt.Sprintf("%.0f°", state.heading)
	}
	ih.SetAttributes(`DRAWCOLOR="220 225 235", DRAWFONT="Helvetica, Bold 14"`)
	tw, th := iup.DrawGetTextSize(ih, text)
	iup.DrawText(ih, text, cx-tw/2, cy+r+8, tw, th)
}

func drawShake(ih iup.Ihandle, w, h int) {
	level := clamp(state.shake / 15)
	bar := int(level * float64(w-40))
	ih.SetAttributes(`DRAWCOLOR="40 46 58", DRAWSTYLE=FILL`)
	iup.DrawRectangle(ih, 20, h-16, w-20, h-6)
	ih.SetAttributes(fmt.Sprintf(`DRAWCOLOR="%d %d 70", DRAWSTYLE=FILL`, 70+int(level*160), 200-int(level*130)))
	iup.DrawRectangle(ih, 20, h-16, 20+bar, h-6)
}

func clamp(v float64) float64 {
	return math.Max(-1, math.Min(1, v))
}
