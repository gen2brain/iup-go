// Capture-style toolbar: image and text buttons, separators, dropdowns and image-only
// buttons; the controls below rebuild the bar with another IMAGEPOSITION, FLAT or text setting.
package main

import (
	"fmt"
	"math"
	"strings"

	"github.com/gen2brain/iup-go/iup"
)

var (
	dlg      iup.Ihandle
	body     iup.Ihandle
	toolbar  iup.Ihandle
	controls iup.Ihandle
	log      iup.Ihandle

	position = "TOP"
	flat     = true
	showText = true
)

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	makeIcons()

	log = iup.Text().SetAttributes("MULTILINE=YES, READONLY=YES, EXPAND=YES, VISIBLELINES=10, VISIBLECOLUMNS=60")
	controls = buildControls()
	toolbar = buildToolbar()

	body = iup.Vbox(toolbar, controls, log).SetAttributes("MARGIN=8x8, GAP=8")

	dlg = iup.Dialog(body).SetAttribute("TITLE", "Toolbar")
	iup.Show(dlg)
	iup.MainLoop()
}

func buildToolbar() iup.Ihandle {
	hb := iup.Hbox(
		iconButton("stop", "Stop capture", func() { report("stop") }),
		separator(),
		modeButton("display", "Display"),
		modeButton("window", "Window"),
		modeButton("area", "Area"),
		modeButton("device", "Device"),
		separator(),
		textButton("camera", "No camera", func() { report("camera") }),
		iup.Label("").SetAttributes("IMAGE=toolbar_mic"),
		dropdown("mic", "Built-in microphone", "USB headset", "None"),
		iup.Label("").SetAttributes("IMAGE=toolbar_speaker"),
		dropdown("audio", "System audio", "Application only", "None"),
		iup.Fill(),
		iconButton("settings", "Settings", func() { report("settings") }),
	)
	hb.SetAttributes("GAP=4, ALIGNMENT=ACENTER")
	return hb
}

func buildControls() iup.Ihandle {
	positions := iup.Hbox()
	for _, p := range []string{"LEFT", "RIGHT", "TOP", "BOTTOM"} {
		t := iup.Toggle(p[:1] + strings.ToLower(p[1:]))
		if p == position {
			t.SetAttribute("VALUE", "ON")
		}
		t.SetCallback("ACTION", iup.ToggleActionFunc(func(ih iup.Ihandle, state int) int {
			if state == 1 {
				position = p
				rebuild()
			}
			return iup.DEFAULT
		}))
		iup.Append(positions, t)
	}
	positions.SetAttributes("GAP=6, ALIGNMENT=ACENTER")

	flatToggle := iup.Toggle("Flat").SetAttribute("VALUE", "ON")
	flatToggle.SetCallback("ACTION", iup.ToggleActionFunc(func(ih iup.Ihandle, state int) int {
		flat = state == 1
		rebuild()
		return iup.DEFAULT
	}))

	textToggle := iup.Toggle("Text").SetAttribute("VALUE", "ON")
	textToggle.SetCallback("ACTION", iup.ToggleActionFunc(func(ih iup.Ihandle, state int) int {
		showText = state == 1
		rebuild()
		return iup.DEFAULT
	}))

	return iup.Hbox(
		iup.Label("Image position:"),
		iup.Radio(positions),
		separator(),
		flatToggle,
		textToggle,
	).SetAttributes("GAP=8, ALIGNMENT=ACENTER")
}

func rebuild() {
	iup.Destroy(toolbar)
	toolbar = buildToolbar()
	iup.Insert(body, controls, toolbar)
	iup.Map(toolbar)
	dlg.SetAttribute("SIZE", "")
	iup.Refresh(dlg)

	report(fmt.Sprintf("IMAGEPOSITION=%s FLAT=%s TEXT=%s", position, yesNo(flat), yesNo(showText)))
}

func modeButton(icon, title string) iup.Ihandle {
	return textButton(icon, title, func() { report("mode " + title) })
}

func textButton(icon, title string, action func()) iup.Ihandle {
	if !showText {
		title = ""
	}
	b := iup.Button(title).SetAttributes(map[string]string{
		"IMAGE": "toolbar_" + icon, "IMAGEPOSITION": position, "TIP": title,
		"FLAT": yesNo(flat), "PADDING": "6x4", "CANFOCUS": "NO",
	})
	b.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		action()
		return iup.DEFAULT
	}))
	return b
}

func iconButton(icon, tip string, action func()) iup.Ihandle {
	b := iup.Button("").SetAttributes(map[string]string{
		"IMAGE": "toolbar_" + icon, "TIP": tip, "FLAT": yesNo(flat), "PADDING": "6x4", "CANFOCUS": "NO",
	})
	b.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		action()
		return iup.DEFAULT
	}))
	return b
}

func dropdown(name string, items ...string) iup.Ihandle {
	l := iup.List().SetAttributes("DROPDOWN=YES, VALUE=1")
	for i, it := range items {
		l.SetAttribute(fmt.Sprint(i+1), it)
	}
	l.SetCallback("ACTION", iup.ListActionFunc(func(ih iup.Ihandle, text string, item, state int) int {
		if state == 1 {
			report(name + " " + text)
		}
		return iup.DEFAULT
	}))
	return l
}

func separator() iup.Ihandle {
	return iup.Label("").SetAttribute("SEPARATOR", "VERTICAL")
}

func report(msg string) {
	log.SetAttribute("APPEND", msg)
}

func yesNo(b bool) string {
	if b {
		return "YES"
	}
	return "NO"
}

func makeIcons() {
	size := 2 * iup.GetGlobalInt("DEFAULTFONTSIZE")
	if size < 16 {
		size = 16
	}

	corners := union(
		bar(0.14, 0.24, 0.30, 0.24, 0.10), bar(0.14, 0.24, 0.14, 0.40, 0.10),
		bar(0.70, 0.24, 0.86, 0.24, 0.10), bar(0.86, 0.24, 0.86, 0.40, 0.10),
		bar(0.14, 0.60, 0.14, 0.76, 0.10), bar(0.14, 0.76, 0.30, 0.76, 0.10),
		bar(0.86, 0.60, 0.86, 0.76, 0.10), bar(0.70, 0.76, 0.86, 0.76, 0.10),
		bar(0.42, 0.24, 0.58, 0.24, 0.10), bar(0.42, 0.76, 0.58, 0.76, 0.10),
		bar(0.14, 0.44, 0.14, 0.56, 0.10), bar(0.86, 0.44, 0.86, 0.56, 0.10))

	spokes := make([]sdf, 0, 8)
	for i := range 8 {
		a := float64(i) * math.Pi / 4
		spokes = append(spokes, bar(0.5+0.28*math.Cos(a), 0.5+0.28*math.Sin(a), 0.5+0.40*math.Cos(a), 0.5+0.40*math.Sin(a), 0.14))
	}

	icons := map[string]sdf{
		"stop":     union(ring(0.5, 0.5, 0.38, 0.10), bar(0.36, 0.36, 0.64, 0.64, 0.10), bar(0.64, 0.36, 0.36, 0.64, 0.10)),
		"display":  union(cut(box(0.10, 0.20, 0.90, 0.70), box(0.19, 0.29, 0.81, 0.61)), bar(0.50, 0.70, 0.50, 0.82, 0.10), bar(0.32, 0.84, 0.68, 0.84, 0.09)),
		"window":   union(cut(box(0.12, 0.20, 0.88, 0.80), box(0.21, 0.36, 0.79, 0.71)), disc(0.24, 0.28, 0.035), disc(0.34, 0.28, 0.035)),
		"area":     corners,
		"device":   union(cut(box(0.32, 0.12, 0.68, 0.88), box(0.40, 0.22, 0.60, 0.74)), bar(0.45, 0.81, 0.55, 0.81, 0.05)),
		"camera":   union(cut(box(0.10, 0.30, 0.64, 0.70), box(0.19, 0.39, 0.55, 0.61)), wedge(0.66, 0.50, 0.90, 0.32, 0.90, 0.68)),
		"mic":      union(cut(union(disc(0.5, 0.32, 0.14), box(0.36, 0.32, 0.64, 0.52), disc(0.5, 0.52, 0.14)), union(disc(0.5, 0.32, 0.06), box(0.44, 0.32, 0.56, 0.52), disc(0.5, 0.52, 0.06))), arc(0.5, 0.52, 0.24, 0.09, 0, math.Pi), bar(0.5, 0.76, 0.5, 0.86, 0.09), bar(0.36, 0.86, 0.64, 0.86, 0.09)),
		"speaker":  union(box(0.12, 0.40, 0.28, 0.60), wedge(0.28, 0.40, 0.50, 0.20, 0.50, 0.80), wedge(0.28, 0.60, 0.50, 0.20, 0.50, 0.80), arc(0.50, 0.50, 0.20, 0.08, -0.25*math.Pi, 0.25*math.Pi), arc(0.50, 0.50, 0.34, 0.08, -0.25*math.Pi, 0.25*math.Pi)),
		"settings": cut(union(ring(0.5, 0.5, 0.26, 0.14), union(spokes...)), disc(0.5, 0.5, 0.10)),
	}

	r, g, b := rgb(iup.GetGlobal("DLGFGCOLOR"))
	for name, shape := range icons {
		iup.ImageRGBA(size, size, raster(size, shape, r, g, b)).SetHandle("toolbar_" + name)
	}
}

func raster(size int, shape sdf, r, g, b float64) []byte {
	pix := make([]byte, size*size*4)
	const ss = 3
	for y := range size {
		for x := range size {
			hits := 0
			for sy := range ss {
				for sx := range ss {
					u := (float64(x) + (float64(sx)+0.5)/ss) / float64(size)
					v := (float64(y) + (float64(sy)+0.5)/ss) / float64(size)
					if shape(u, v) <= 0 {
						hits++
					}
				}
			}
			if hits == 0 {
				continue
			}
			i := (y*size + x) * 4
			pix[i], pix[i+1], pix[i+2] = byte(r), byte(g), byte(b)
			pix[i+3] = byte(255 * hits / (ss * ss))
		}
	}
	return pix
}

func rgb(c string) (float64, float64, float64) {
	var r, g, b int
	fmt.Sscanf(c, "%d %d %d", &r, &g, &b)
	return float64(r), float64(g), float64(b)
}

type sdf func(x, y float64) float64

func union(fs ...sdf) sdf {
	return func(x, y float64) float64 {
		d := math.MaxFloat64
		for _, f := range fs {
			d = math.Min(d, f(x, y))
		}
		return d
	}
}

func cut(a, b sdf) sdf {
	return func(x, y float64) float64 { return math.Max(a(x, y), -b(x, y)) }
}

func box(x0, y0, x1, y1 float64) sdf {
	return func(x, y float64) float64 {
		return math.Max(math.Max(x0-x, x-x1), math.Max(y0-y, y-y1))
	}
}

func disc(cx, cy, r float64) sdf {
	return func(x, y float64) float64 { return math.Hypot(x-cx, y-cy) - r }
}

func ring(cx, cy, r, t float64) sdf {
	return func(x, y float64) float64 { return math.Abs(math.Hypot(x-cx, y-cy)-r) - t/2 }
}

func bar(x0, y0, x1, y1, t float64) sdf {
	return func(x, y float64) float64 {
		dx, dy := x1-x0, y1-y0
		l := dx*dx + dy*dy
		u := 0.0
		if l > 0 {
			u = math.Max(0, math.Min(1, ((x-x0)*dx+(y-y0)*dy)/l))
		}
		return math.Hypot(x-(x0+u*dx), y-(y0+u*dy)) - t/2
	}
}

func arc(cx, cy, r, t, a0, a1 float64) sdf {
	band := ring(cx, cy, r, t)
	return func(x, y float64) float64 {
		a := math.Atan2(y-cy, x-cx)
		for a < a0 {
			a += 2 * math.Pi
		}
		if a <= a1 {
			return band(x, y)
		}
		return math.Min(
			math.Hypot(x-(cx+r*math.Cos(a0)), y-(cy+r*math.Sin(a0))),
			math.Hypot(x-(cx+r*math.Cos(a1)), y-(cy+r*math.Sin(a1)))) - t/2
	}
}

func wedge(x0, y0, x1, y1, x2, y2 float64) sdf {
	side := func(ax, ay, bx, by, px, py float64) float64 {
		return (bx-ax)*(py-ay) - (by-ay)*(px-ax)
	}
	return func(x, y float64) float64 {
		a := side(x0, y0, x1, y1, x, y)
		b := side(x1, y1, x2, y2, x, y)
		c := side(x2, y2, x0, y0, x, y)
		if (a >= 0 && b >= 0 && c >= 0) || (a <= 0 && b <= 0 && c <= 0) {
			return -1
		}
		return 1
	}
}
