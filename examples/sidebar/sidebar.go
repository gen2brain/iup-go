// Settings window with a sidebar: a search field over an icon list selects the page
// shown on the right, in the style of the macOS, GNOME and KDE settings panels.
package main

import (
	"fmt"
	"math"
	"strings"

	"github.com/gen2brain/iup-go/iup"
)

type category struct {
	name  string
	color string
	page  iup.Ihandle
}

var (
	categories []category
	visible    []int
	list       iup.Ihandle
	pages      iup.Ihandle
	title      iup.Ihandle
	summary    iup.Ihandle
)

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()
	iup.SetGlobal("UTF8MODE", "YES")

	categories = []category{
		{"General", "120 120 128", general()},
		{"Appearance", "40 40 40", appearance()},
		{"Displays", "0 122 255", displays()},
		{"Sound", "255 45 85", sound()},
		{"Network", "0 122 255", network()},
		{"Notifications", "255 59 48", notifications()},
		{"Privacy", "52 120 246", privacy()},
		{"Users", "48 176 199", users()},
		{"Keyboard", "120 120 128", keyboard()},
		{"Storage", "142 142 147", storage()},
	}
	makeIcons()

	search := iup.Text().SetAttributes("CUEBANNER=Search, EXPAND=HORIZONTAL")
	search.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		filter(ih.GetAttribute("VALUE"))
		return iup.DEFAULT
	}))

	list = iup.List().SetAttributes("SHOWIMAGE=YES, SPACING=3, VISIBLELINES=12, VISIBLECOLUMNS=14, EXPAND=VERTICAL")
	list.SetCallback("ACTION", iup.ListActionFunc(func(ih iup.Ihandle, text string, item, state int) int {
		if state == 1 {
			show(visible[item-1])
		}
		return iup.DEFAULT
	}))

	title = iup.Label("").SetAttributes("FONTSTYLE=Bold, FONTSIZE=16, EXPAND=HORIZONTAL")
	summary = iup.Label("").SetAttributes(`FGCOLOR="128 128 128", EXPAND=HORIZONTAL`)

	pageList := make([]iup.Ihandle, len(categories))
	for i, c := range categories {
		pageList[i] = c.page
	}
	pages = iup.Zbox(pageList...).SetAttributes("ALIGNMENT=NW, EXPAND=YES")

	sidebar := iup.Vbox(search, list).SetAttributes("GAP=6")
	content := iup.Vbox(title, summary, pages).SetAttributes("GAP=8, MARGIN=12x0, EXPAND=YES")

	var body iup.Ihandle
	if phone() {
		list.SetAttributes("VISIBLELINES=4, EXPAND=HORIZONTAL")
		body = iup.Vbox(sidebar, content)
	} else {
		body = iup.Hbox(sidebar, iup.Label("").SetAttribute("SEPARATOR", "VERTICAL"), content)
	}
	body.SetAttributes("NGAP=8, NMARGIN=10x10")

	dlg := iup.Dialog(body).SetAttribute("TITLE", "Settings")

	filter("")
	iup.Show(dlg)
	iup.MainLoop()
}

func filter(query string) {
	query = strings.ToLower(strings.TrimSpace(query))
	if list.GetInt("COUNT") > 0 {
		list.SetAttribute("REMOVEITEM", "ALL")
	}
	visible = visible[:0]
	for i, c := range categories {
		if query != "" && !strings.Contains(strings.ToLower(c.name), query) {
			continue
		}
		visible = append(visible, i)
		list.SetAttribute(fmt.Sprint(len(visible)), c.name)
		list.SetAttribute(fmt.Sprintf("IMAGE%d", len(visible)), "sidebar_"+strings.ToLower(c.name))
	}
	if len(visible) > 0 {
		list.SetAttribute("VALUE", "1")
		show(visible[0])
	}
}

func show(index int) {
	c := categories[index]
	pages.SetAttribute("VALUEPOS", fmt.Sprint(index))
	title.SetAttribute("TITLE", c.name)
	summary.SetAttribute("TITLE", summaries[c.name])
}

var summaries = map[string]string{
	"General":       "Updates, sharing and startup.",
	"Appearance":    "Theme, accent color and chrome.",
	"Displays":      "Resolution, scaling, night mode.",
	"Sound":         "Output device, volume, alerts.",
	"Network":       "Wi-Fi, proxies and limits.",
	"Notifications": "Banners, sounds, quiet hours.",
	"Privacy":       "Location, camera, microphone.",
	"Users":         "Accounts, login items, guests.",
	"Keyboard":      "Repeat rate, shortcuts, layouts.",
	"Storage":       "Disk usage and cleanup.",
}

func general() iup.Ihandle {
	return page(
		group("Software",
			row("Automatic updates", toggle("ON")),
			row("Update channel", dropdown("Stable", "Beta", "Nightly")),
		),
		group("Startup",
			row("Open at login", toggle("OFF")),
			row("Restore windows", toggle("ON")),
		),
	)
}

func appearance() iup.Ihandle {
	return page(
		group("Theme",
			row("Appearance", dropdown("System", "Light", "Dark")),
			row("Accent color", dropdown("Blue", "Purple", "Pink", "Red", "Orange", "Green", "Graphite")),
			row("Sidebar icon size", dropdown("Small", "Medium", "Large")),
		),
		group("Windows",
			row("Show scroll bars", dropdown("Automatically", "When scrolling", "Always")),
			row("Wallpaper tinting", toggle("ON")),
		),
	)
}

func displays() iup.Ihandle {
	return page(
		group("Built-in display",
			row("Resolution", dropdown("Default", "Larger text", "More space")),
			row("Brightness", slider(70)),
			row("Automatically adjust brightness", toggle("ON")),
		),
		group("Night mode",
			row("Schedule", dropdown("Off", "Sunset to sunrise", "Custom")),
			row("Warmth", slider(40)),
		),
	)
}

func sound() iup.Ihandle {
	return page(
		group("Output",
			row("Device", dropdown("Speakers", "Headphones", "HDMI")),
			row("Volume", slider(60)),
			row("Balance", slider(50)),
		),
		group("Alerts",
			row("Alert sound", dropdown("Boop", "Breeze", "Bubble", "Crystal")),
			row("Play sound on startup", toggle("ON")),
		),
	)
}

func network() iup.Ihandle {
	return page(
		group("Wi-Fi",
			row("Wi-Fi", toggle("ON")),
			row("Network", dropdown("Home", "Office", "Guest")),
			row("Ask to join networks", toggle("OFF")),
		),
		group("Proxy",
			row("Host", text("proxy.example.com")),
			row("Port", spin(3128, 1, 65535)),
		),
	)
}

func notifications() iup.Ihandle {
	return page(
		group("Banners",
			row("Allow notifications", toggle("ON")),
			row("Show previews", dropdown("Always", "When unlocked", "Never")),
			row("Play sound", toggle("ON")),
		),
		group("Quiet hours",
			row("Enabled", toggle("OFF")),
			row("From", spin(22, 0, 23)),
			row("To", spin(7, 0, 23)),
		),
	)
}

func privacy() iup.Ihandle {
	return page(
		group("Permissions",
			row("Location services", toggle("ON")),
			row("Camera", toggle("ON")),
			row("Microphone", toggle("OFF")),
		),
		group("Analytics",
			row("Share crash reports", toggle("ON")),
			row("Share usage data", toggle("OFF")),
		),
	)
}

func users() iup.Ihandle {
	return page(
		group("Current user",
			row("Full name", text("Milan Nikolic")),
			row("Password hint", text("")),
		),
		group("Login",
			row("Automatic login", toggle("OFF")),
			row("Guest user", toggle("ON")),
		),
	)
}

func keyboard() iup.Ihandle {
	return page(
		group("Typing",
			row("Key repeat rate", slider(80)),
			row("Delay until repeat", slider(30)),
			row("Autocorrect", toggle("ON")),
		),
		group("Input sources",
			row("Layout", dropdown("US", "US International", "Serbian Latin", "German")),
			row("Show input menu", toggle("ON")),
		),
	)
}

func storage() iup.Ihandle {
	return page(
		group("Usage",
			row("Used", iup.Label("312 GB of 512 GB")),
			row("Documents", iup.Label("84 GB")),
			row("Applications", iup.Label("61 GB")),
		),
		group("Cleanup",
			row("Empty trash automatically", toggle("ON")),
			row("Keep downloads for", dropdown("30 days", "90 days", "Forever")),
		),
	)
}

func page(groups ...iup.Ihandle) iup.Ihandle {
	return iup.Vbox(groups...).SetAttributes("GAP=12, EXPAND=YES")
}

func group(name string, rows ...[2]iup.Ihandle) iup.Ihandle {
	cells := make([]iup.Ihandle, 0, 2*len(rows))
	for _, r := range rows {
		cells = append(cells, r[0], r[1])
	}
	grid := iup.GridBox(cells...).SetAttributes("NUMDIV=2, SIZELIN=-1, SIZECOL=-1, GAPLIN=8, GAPCOL=24, ALIGNMENTLIN=ACENTER")
	return iup.Frame(iup.Hbox(grid, iup.Fill())).SetAttributes(fmt.Sprintf("TITLE=%q, MARGIN=10x8", name))
}

func row(label string, control iup.Ihandle) [2]iup.Ihandle {
	return [2]iup.Ihandle{iup.Label(label), control}
}

func toggle(value string) iup.Ihandle {
	return iup.Toggle("").SetAttribute("VALUE", value)
}

func dropdown(items ...string) iup.Ihandle {
	l := iup.List().SetAttributes("DROPDOWN=YES, VALUE=1")
	for i, it := range items {
		l.SetAttribute(fmt.Sprint(i+1), it)
	}
	return l
}

func slider(value int) iup.Ihandle {
	return iup.Val("HORIZONTAL").SetAttributes(fmt.Sprintf("MIN=0, MAX=100, VALUE=%d, EXPAND=HORIZONTAL", value))
}

func text(value string) iup.Ihandle {
	return iup.Text().SetAttributes(fmt.Sprintf("VALUE=%q, VISIBLECOLUMNS=16", value))
}

func spin(value, min, max int) iup.Ihandle {
	return iup.Text().SetAttributes(fmt.Sprintf("SPIN=YES, SPINMIN=%d, SPINMAX=%d, VALUE=%d, VISIBLECOLUMNS=6", min, max, value))
}

func phone() bool {
	switch iup.GetGlobal("SYSTEM") {
	case "Android", "iOS":
		return true
	}
	return false
}

func makeIcons() {
	size := 2 * iup.GetGlobalInt("DEFAULTFONTSIZE")
	if size < 20 {
		size = 20
	}

	spokes := make([]sdf, 0, 8)
	for i := range 8 {
		a := float64(i) * math.Pi / 4
		spokes = append(spokes, bar(0.5+0.22*math.Cos(a), 0.5+0.22*math.Sin(a), 0.5+0.33*math.Cos(a), 0.5+0.33*math.Sin(a), 0.12))
	}

	glyphs := map[string]sdf{
		"general":       cut(union(ring(0.5, 0.5, 0.2, 0.12), union(spokes...)), disc(0.5, 0.5, 0.08)),
		"appearance":    union(ring(0.5, 0.5, 0.28, 0.08), cut(disc(0.5, 0.5, 0.28), box(0.5, 0, 1, 1))),
		"displays":      union(cut(box(0.18, 0.24, 0.82, 0.66), box(0.26, 0.32, 0.74, 0.58)), bar(0.5, 0.66, 0.5, 0.78, 0.08), bar(0.34, 0.8, 0.66, 0.8, 0.08)),
		"sound":         union(box(0.2, 0.42, 0.32, 0.58), wedge(0.32, 0.42, 0.5, 0.26, 0.5, 0.74), wedge(0.32, 0.58, 0.5, 0.26, 0.5, 0.74), arc(0.5, 0.5, 0.2, 0.08, -0.25*math.Pi, 0.25*math.Pi), arc(0.5, 0.5, 0.32, 0.08, -0.25*math.Pi, 0.25*math.Pi)),
		"network":       union(ring(0.5, 0.5, 0.3, 0.07), bar(0.2, 0.5, 0.8, 0.5, 0.07), bar(0.5, 0.2, 0.5, 0.8, 0.07), arc(0.5, 0.5, 0.3, 0.07, 0, 2*math.Pi)),
		"notifications": union(cut(union(disc(0.5, 0.44, 0.22), box(0.28, 0.44, 0.72, 0.66)), box(0, 0.66, 1, 1)), bar(0.22, 0.68, 0.78, 0.68, 0.07), disc(0.5, 0.78, 0.07)),
		"privacy":       union(cut(box(0.26, 0.42, 0.74, 0.78), box(0.34, 0.5, 0.66, 0.7)), arc(0.5, 0.42, 0.14, 0.08, math.Pi, 2*math.Pi)),
		"users":         union(disc(0.5, 0.36, 0.14), cut(disc(0.5, 0.78, 0.28), box(0, 0.78, 1, 1))),
		"keyboard":      union(cut(box(0.16, 0.3, 0.84, 0.7), box(0.24, 0.38, 0.76, 0.62)), bar(0.34, 0.5, 0.66, 0.5, 0.07)),
		"storage":       union(ring(0.5, 0.36, 0.22, 0.08), ring(0.5, 0.5, 0.22, 0.08), ring(0.5, 0.64, 0.22, 0.08)),
	}

	for _, c := range categories {
		key := strings.ToLower(c.name)
		r, g, b := rgb(c.color)
		img := iup.ImageRGBA(size, size, raster(size,
			layer{rbox(0.04, 0.04, 0.96, 0.96, 0.22), r, g, b},
			layer{glyphs[key], 255, 255, 255}))
		img.SetHandle("sidebar_" + key)
	}
}

type layer struct {
	shape   sdf
	r, g, b float64
}

func raster(size int, layers ...layer) []byte {
	pix := make([]byte, size*size*4)
	const ss = 3
	for y := range size {
		for x := range size {
			var cr, cg, cb, ca float64
			for _, l := range layers {
				hits := 0
				for sy := range ss {
					for sx := range ss {
						u := (float64(x) + (float64(sx)+0.5)/ss) / float64(size)
						v := (float64(y) + (float64(sy)+0.5)/ss) / float64(size)
						if l.shape(u, v) <= 0 {
							hits++
						}
					}
				}
				if hits == 0 {
					continue
				}
				a := float64(hits) / (ss * ss)
				cr = cr*(1-a) + l.r*a
				cg = cg*(1-a) + l.g*a
				cb = cb*(1-a) + l.b*a
				ca = ca + a*(1-ca)
			}
			i := (y*size + x) * 4
			pix[i], pix[i+1], pix[i+2], pix[i+3] = byte(cr), byte(cg), byte(cb), byte(255*ca)
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

func rbox(x0, y0, x1, y1, r float64) sdf {
	return func(x, y float64) float64 {
		cx, cy := (x0+x1)/2, (y0+y1)/2
		qx := math.Abs(x-cx) - ((x1-x0)/2 - r)
		qy := math.Abs(y-cy) - ((y1-y0)/2 - r)
		return math.Hypot(math.Max(qx, 0), math.Max(qy, 0)) + math.Min(math.Max(qx, qy), 0) - r
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
