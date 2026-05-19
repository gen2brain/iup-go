// goroutines2 is a responsiveness test: many background goroutines drive a
// wide variety of controls at modest rates. Sibling to goroutines (throughput).
package main

import (
	"fmt"
	"math"
	"math/rand/v2"
	"sync/atomic"
	"time"

	"github.com/gen2brain/iup-go/iup"
)

var (
	running     atomic.Bool
	totalUpdate atomic.Int64
	clickCount  atomic.Int64

	canvasPhase atomic.Uint64 // float64 bits
	ballX       atomic.Uint64 // float64 bits
	ballY       atomic.Uint64 // float64 bits
)

func storeF64(slot *atomic.Uint64, v float64) { slot.Store(math.Float64bits(v)) }
func loadF64(slot *atomic.Uint64) float64     { return math.Float64frombits(slot.Load()) }

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	status := iup.Label("Idle. Press Start to launch background updaters.").
		SetAttributes(`EXPAND=HORIZONTAL`)
	counter := iup.Label("Updates: 0").SetAttribute("SIZE", "70x")
	clicks := iup.Label("Clicks: 0").SetAttribute("SIZE", "55x")
	startBtn := iup.Button("Start").SetAttribute("PADDING", "DEFAULTBUTTONPADDING")
	clickMe := iup.Button("Click me!").SetAttribute("PADDING", "DEFAULTBUTTONPADDING")

	prog1 := iup.ProgressBar().SetAttribute("EXPAND", "HORIZONTAL")
	prog2 := iup.ProgressBar().SetAttribute("EXPAND", "HORIZONTAL").SetAttribute("DASHED", "YES")
	prog3 := iup.ProgressBar().SetAttribute("EXPAND", "HORIZONTAL").
		SetAttribute("BGCOLOR", "230 230 240").SetAttribute("FGCOLOR", "60 160 80")
	valH := iup.Val("HORIZONTAL").SetAttribute("EXPAND", "HORIZONTAL")
	valV := iup.Val("VERTICAL").SetAttribute("EXPAND", "VERTICAL")
	dial := iup.Dial("HORIZONTAL").SetAttribute("DENSITY", "0.4")
	spin := iup.Text().SetAttributes("SPIN=YES, SPINMAX=200, SPINMIN=0, VALUE=0, EXPAND=HORIZONTAL")
	toggle := iup.Toggle("Auto-toggling")
	tabs := iup.Tabs(
		iup.Vbox(iup.Label("Tab page A")).SetAttribute("TABTITLE", "Alpha"),
		iup.Vbox(iup.Label("Tab page B")).SetAttribute("TABTITLE", "Beta"),
		iup.Vbox(iup.Label("Tab page C")).SetAttribute("TABTITLE", "Gamma"),
	).SetAttribute("EXPAND", "YES")
	colorbr := iup.ColorBrowser().SetAttribute("EXPAND", "YES")

	cv := iup.Canvas().SetAttributes("RASTERSIZE=320x140, BORDER=NO")
	tree := iup.Tree().SetAttributes("EXPAND=YES, TITLE=Activity")
	listBox := iup.List().SetAttributes("EXPAND=YES, VISIBLELINES=6")
	logText := iup.Text().SetAttributes("MULTILINE=YES, READONLY=YES, EXPAND=YES, WORDWRAP=YES")

	iup.SetHandle("status", status)
	iup.SetHandle("counter", counter)
	iup.SetHandle("clicks", clicks)
	iup.SetHandle("startBtn", startBtn)
	iup.SetHandle("prog1", prog1)
	iup.SetHandle("prog2", prog2)
	iup.SetHandle("prog3", prog3)
	iup.SetHandle("valH", valH)
	iup.SetHandle("valV", valV)
	iup.SetHandle("dial", dial)
	iup.SetHandle("spin", spin)
	iup.SetHandle("toggle", toggle)
	iup.SetHandle("tabs", tabs)
	iup.SetHandle("colorbr", colorbr)
	iup.SetHandle("cv", cv)
	iup.SetHandle("tree", tree)
	iup.SetHandle("listBox", listBox)
	iup.SetHandle("logText", logText)

	// One coordinator dispatches by "kind" tag in s.
	status.SetCallback("POSTMESSAGE_CB", iup.PostMessageFunc(func(ih iup.Ihandle, s string, i int, p any) int {
		totalUpdate.Add(1)
		switch s {
		case "prog1", "prog2", "prog3":
			iup.GetHandle(s).SetAttribute("VALUE", fmt.Sprintf("%g", float64(i)/1000.0))
		case "valH", "valV":
			iup.GetHandle(s).SetAttribute("VALUE", fmt.Sprintf("%g", float64(i)/1000.0))
		case "dial":
			iup.GetHandle("dial").SetAttribute("VALUE", fmt.Sprintf("%g", float64(i)/1000.0))
		case "spin":
			iup.GetHandle("spin").SetAttribute("VALUE", fmt.Sprintf("%d", i))
		case "toggle":
			v := "OFF"
			if i == 1 {
				v = "ON"
			}
			iup.GetHandle("toggle").SetAttribute("VALUE", v)
		case "tabs":
			iup.GetHandle("tabs").SetAttribute("VALUEPOS", fmt.Sprintf("%d", i))
		case "color":
			rgb, _ := p.(string)
			iup.GetHandle("colorbr").SetAttribute("RGB", rgb)
		case "redraw":
			iup.Update(iup.GetHandle("cv"))
		case "tree":
			handleTree(p)
		case "list":
			line, _ := p.(string)
			lst := iup.GetHandle("listBox")
			n := lst.GetInt("COUNT")
			lst.SetAttribute(fmt.Sprintf("INSERTITEM%d", n+1), line)
			if n+1 > 50 {
				lst.SetAttribute("REMOVEITEM", "1")
			}
			lst.SetAttribute("TOPITEM", fmt.Sprintf("%d", lst.GetInt("COUNT")))
		case "log":
			line, _ := p.(string)
			tx := iup.GetHandle("logText")
			tx.SetAttribute("APPEND", line)
			if cur := tx.GetAttribute("VALUE"); len(cur) > 4096 {
				tx.SetAttribute("VALUE", cur[len(cur)-3072:])
			}
			tx.SetAttribute("CARETPOS", fmt.Sprintf("%d", len(tx.GetAttribute("VALUE"))))
		case "done":
			iup.GetHandle("status").SetAttribute("TITLE", "Stopped.")
			iup.GetHandle("startBtn").SetAttribute("TITLE", "Start")
			return iup.DEFAULT
		}
		iup.GetHandle("counter").SetAttribute("TITLE", fmt.Sprintf("Updates: %d", totalUpdate.Load()))
		return iup.DEFAULT
	}))

	startBtn.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		if running.Load() {
			running.Store(false)
			ih.SetAttribute("TITLE", "Start")
			iup.GetHandle("status").SetAttribute("TITLE", "Stopping...")
			return iup.DEFAULT
		}
		running.Store(true)
		totalUpdate.Store(0)
		ih.SetAttribute("TITLE", "Stop")
		iup.GetHandle("status").SetAttribute("TITLE",
			"Running. Try clicking, dragging the slider, switching tabs, resizing the window.")
		iup.GetHandle("counter").SetAttribute("TITLE", "Updates: 0")
		iup.GetHandle("logText").SetAttribute("VALUE", "")
		iup.GetHandle("listBox").SetAttribute("REMOVEITEM", "ALL")
		startWorkers(status)
		return iup.DEFAULT
	}))

	clickMe.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		n := clickCount.Add(1)
		iup.GetHandle("clicks").SetAttribute("TITLE", fmt.Sprintf("Clicks: %d", n))
		return iup.DEFAULT
	}))

	cv.SetCallback("ACTION", iup.ActionFunc(drawCanvas))

	leftCol := iup.Vbox(
		iup.Frame(iup.Vbox(prog1, prog2, prog3).SetAttributes("NMARGIN=8x8, NGAP=6")).
			SetAttribute("TITLE", "Progress bars"),
		iup.Frame(iup.Hbox(
			iup.Vbox(valH).SetAttributes("NMARGIN=4x4"),
			iup.Vbox(valV, dial).SetAttributes("NMARGIN=4x4, ALIGNMENT=ACENTER, NGAP=4"),
		).SetAttributes("NGAP=6")).SetAttribute("TITLE", "Vals + Dial (drag any of them!)"),
		iup.Frame(iup.Vbox(spin, toggle, tabs).SetAttributes("NMARGIN=8x8, NGAP=6")).
			SetAttribute("TITLE", "Spin / Toggle / Tabs"),
		iup.Frame(iup.Vbox(colorbr).SetAttributes("NMARGIN=4x4")).
			SetAttribute("TITLE", "ColorBrowser"),
	).SetAttributes("NGAP=8, EXPAND=YES")

	rightCol := iup.Vbox(
		iup.Frame(iup.Vbox(cv).SetAttributes("NMARGIN=4x4")).
			SetAttribute("TITLE", "Canvas"),
		iup.Frame(iup.Vbox(tree).SetAttributes("NMARGIN=4x4")).
			SetAttribute("TITLE", "Tree (nodes added/removed)"),
		iup.Frame(iup.Vbox(listBox).SetAttributes("NMARGIN=4x4")).
			SetAttribute("TITLE", "Streaming list"),
		iup.Frame(iup.Vbox(logText).SetAttributes("NMARGIN=4x4")).
			SetAttribute("TITLE", "Activity log"),
	).SetAttributes("NGAP=8, EXPAND=YES")

	dlg := iup.Dialog(iup.Vbox(
		iup.Hbox(status, clicks).SetAttributes("NGAP=10, ALIGNMENT=ACENTER"),
		iup.Hbox(counter, iup.Fill(), startBtn, clickMe).SetAttributes("NGAP=8, ALIGNMENT=ACENTER"),
		iup.Hbox(leftCol, rightCol).SetAttributes("NGAP=10"),
	).SetAttributes("NMARGIN=10x10, NGAP=8")).
		SetAttribute("TITLE", "Goroutines Responsiveness Test")

	dlg.SetCallback("CLOSE_CB", iup.CloseFunc(func(ih iup.Ihandle) int {
		running.Store(false)
		return iup.DEFAULT
	}))

	storeF64(&ballX, 30)
	storeF64(&ballY, 70)

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}

// startWorkers launches the background goroutines; each stops when running flips false.
func startWorkers(target iup.Ihandle) {
	type spec struct {
		kind   string
		period time.Duration
		freq   float64
		offset float64
	}
	scalars := []spec{
		{"prog1", 40 * time.Millisecond, 0.7, 0},
		{"prog2", 60 * time.Millisecond, 1.1, 0.5},
		{"prog3", 80 * time.Millisecond, 1.7, 1.0},
		{"valH", 50 * time.Millisecond, 0.9, 0},
		{"valV", 70 * time.Millisecond, 1.3, 0.3},
		{"dial", 45 * time.Millisecond, 0.5, 0},
	}

	for _, s := range scalars {
		go func(s spec) {
			t := time.NewTicker(s.period)
			defer t.Stop()
			start := time.Now()
			for range t.C {
				if !running.Load() {
					return
				}
				dt := time.Since(start).Seconds()
				v := 0.5 + 0.5*math.Sin(dt*s.freq+s.offset)
				if s.kind == "dial" {
					v = math.Mod(dt*0.7, 2*math.Pi)
				}
				iup.PostMessage(target, s.kind, int(v*1000), nil)
			}
		}(s)
	}

	go func() { // spin: ramp 0..200

		t := time.NewTicker(60 * time.Millisecond)
		defer t.Stop()
		v := 0
		for range t.C {
			if !running.Load() {
				return
			}
			v = (v + 2) % 201
			iup.PostMessage(target, "spin", v, nil)
		}
	}()

	go func() { // toggle: periodic flip

		t := time.NewTicker(500 * time.Millisecond)
		defer t.Stop()
		on := 0
		for range t.C {
			if !running.Load() {
				return
			}
			on ^= 1
			iup.PostMessage(target, "toggle", on, nil)
		}
	}()

	go func() { // tabs: cycle active page

		t := time.NewTicker(800 * time.Millisecond)
		defer t.Stop()
		idx := 0
		for range t.C {
			if !running.Load() {
				return
			}
			idx = (idx + 1) % 3
			iup.PostMessage(target, "tabs", idx, nil)
		}
	}()

	go func() { // colorbrowser: rotate hue

		t := time.NewTicker(80 * time.Millisecond)
		defer t.Stop()
		start := time.Now()
		for range t.C {
			if !running.Load() {
				return
			}
			h := math.Mod(time.Since(start).Seconds()*0.15, 1.0)
			r, g, b := hsvToRGB(h, 0.8, 0.95)
			iup.PostMessage(target, "color", 0, fmt.Sprintf("%d %d %d", r, g, b))
		}
	}()

	go func() { // canvas: ~30 Hz redraw, phase advances here

		t := time.NewTicker(33 * time.Millisecond)
		defer t.Stop()
		for range t.C {
			if !running.Load() {
				return
			}
			storeF64(&canvasPhase, loadF64(&canvasPhase)+0.08)
			x := loadF64(&ballX) + 3
			y := loadF64(&ballY) + 2*math.Sin(loadF64(&canvasPhase))
			if x > 320 {
				x = 0
			}
			storeF64(&ballX, x)
			storeF64(&ballY, y)
			iup.PostMessage(target, "redraw", 0, nil)
		}
	}()

	go func() { // tree: append leaves

		t := time.NewTicker(250 * time.Millisecond)
		defer t.Stop()
		i := 0
		for range t.C {
			if !running.Load() {
				return
			}
			i++
			iup.PostMessage(target, "tree", 0, fmt.Sprintf("leaf #%03d", i))
		}
	}()

	go func() { // list: streaming items

		t := time.NewTicker(200 * time.Millisecond)
		defer t.Stop()
		i := 0
		for range t.C {
			if !running.Load() {
				return
			}
			i++
			iup.PostMessage(target, "list", 0,
				fmt.Sprintf("item #%04d  rnd=%.3f  ts=%s",
					i, rand.Float64(), time.Now().Format("15:04:05.000")))
		}
	}()

	go func() { // log: activity lines

		t := time.NewTicker(100 * time.Millisecond)
		defer t.Stop()
		ev := []string{"tick", "frame", "sample", "step", "pulse", "beat"}
		for range t.C {
			if !running.Load() {
				return
			}
			iup.PostMessage(target, "log", 0,
				fmt.Sprintf("[%s] %s\n",
					time.Now().Format("15:04:05.000"), ev[rand.IntN(len(ev))]))
		}
	}()

	go func() { // watchdog: notify coordinator when running flips false

		for running.Load() {
			time.Sleep(50 * time.Millisecond)
		}
		iup.PostMessage(target, "done", 0, nil)
	}()
}

func handleTree(p any) {
	title, _ := p.(string)
	tree := iup.GetHandle("tree")
	if tree.GetInt("COUNT") > 10 {
		tree.SetAttribute("DELNODE0", "CHILDREN")
	}
	tree.SetAttribute("ADDLEAF0", title)
}

func drawCanvas(ih iup.Ihandle) int {
	iup.DrawBegin(ih)
	defer iup.DrawEnd(ih)

	w, h := iup.DrawGetSize(ih)
	phase := loadF64(&canvasPhase)

	ih.SetAttributes(`DRAWCOLOR="20 20 40", DRAWSTYLE=FILL`)
	iup.DrawRectangle(ih, 0, 0, w, h)

	ih.SetAttribute("DRAWCOLOR", "40 40 60")
	for i := 1; i < 10; i++ {
		x := (w * i) / 10
		iup.DrawLine(ih, x, 0, x, h-1)
	}
	for i := 1; i < 4; i++ {
		y := (h * i) / 4
		iup.DrawLine(ih, 0, y, w-1, y)
	}

	midY := float64(h) / 2.0
	amp := float64(h) * 0.35
	freq := 2.5
	var px, py int
	for x := 0; x < w; x++ {
		angle := (float64(x)/float64(w))*2*math.Pi*freq + phase
		y := midY - math.Sin(angle)*amp
		if x > 0 {
			ih.SetAttribute("DRAWCOLOR", "50 220 100")
			iup.DrawLine(ih, px, py, x, int(y))
		}
		px, py = x, int(y)
	}

	bx := int(loadF64(&ballX))
	by := int(loadF64(&ballY))
	if bx >= 0 && bx < w && by >= 0 && by < h {
		ih.SetAttributes(`DRAWCOLOR="255 200 80", DRAWSTYLE=FILL`)
		iup.DrawArc(ih, bx-6, by-6, bx+6, by+6, 0, 360)
	}

	return iup.DEFAULT
}

func hsvToRGB(h, s, v float64) (uint8, uint8, uint8) {
	i := int(h*6) % 6
	f := h*6 - float64(int(h*6))
	p := v * (1 - s)
	q := v * (1 - f*s)
	t := v * (1 - (1-f)*s)
	var r, g, b float64
	switch i {
	case 0:
		r, g, b = v, t, p
	case 1:
		r, g, b = q, v, p
	case 2:
		r, g, b = p, v, t
	case 3:
		r, g, b = p, q, v
	case 4:
		r, g, b = t, p, v
	case 5:
		r, g, b = v, p, q
	}
	return uint8(r * 255), uint8(g * 255), uint8(b * 255)
}
