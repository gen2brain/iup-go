//go:build plot

package main

import (
	"fmt"
	"math/rand"
	"runtime"
	"sort"
	"strings"
	"sync/atomic"
	"time"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

const (
	samples  = 120
	interval = 500
)

type series struct {
	plot  string
	index int
	y     []float64
}

type worker struct {
	name  string
	iters atomic.Uint64
	bytes atomic.Uint64
	busy  atomic.Bool
}

var (
	workers       []*worker
	load          atomic.Int64
	rate          *series
	heapUsed      *series
	heapTotal     *series
	pause         *series
	lastAlloc     uint64
	lastPause     uint64
	started       = time.Now()
	paused        bool
	sortColumn    = 3
	sortAscending bool
	shown         []*worker
)

func main() {
	iup.Open()
	defer iup.Close()
	iup.PlotOpen()

	iup.SetGlobal("UTF8MODE", "YES")
	startWorkers(12)
	load.Store(20)

	activity := plot("Allocation rate").SetHandle("dash_activity")
	rate = addSeries("dash_activity", "MB/s", "70 140 220", "AREA")

	heap := plot("Heap").SetHandle("dash_heap")
	heapTotal = addSeries("dash_heap", "Reserved (MB)", "170 170 180", "AREA")
	heapUsed = addSeries("dash_heap", "In use (MB)", "80 170 110", "LINE")
	heap.SetAttributes("LEGEND=YES, LEGENDPOS=TOPLEFT")

	pauses := plot("Garbage collection").SetHandle("dash_pauses")
	pause = addSeries("dash_pauses", "Last pause (ms)", "220 130 60", "BAR")

	dlg := iup.Dialog(iup.Vbox(
		toolbar(),
		iup.Tabs(
			iup.Vbox(iup.Hbox(meters(), activity).SetAttributes("NGAP=8")).
				SetAttributes("TABTITLE=Overview, NMARGIN=8x8"),
			iup.Vbox(heap, pauses).SetAttributes("TABTITLE=Timeline, NMARGIN=8x8, NGAP=8"),
			workerTab(),
		).SetAttribute("EXPAND", "YES"),
		status(),
	)).SetHandle("dash_dlg")
	dlg.SetAttributes(map[string]string{
		"TITLE": "Dashboard",
		"MENU":  "dash_menu",
	})

	buildMenu()
	fillWorkers()
	loadChanged(iup.GetHandle("dash_load"))

	iup.Show(dlg)
	showAppearance()
	collect(dlg)
	iup.SetHandle("dash_timer", iup.Timer().SetAttributes(fmt.Sprintf("TIME=%d, RUN=YES", interval)))
	iup.GetHandle("dash_timer").SetCallback("ACTION_CB", iup.TimerActionFunc(collect))
	iup.MainLoop()
}

func plot(title string) iup.Ihandle {
	p := iup.Plot().SetAttributes(map[string]string{
		"EXPAND":       "YES",
		"TITLE":        title,
		"GRID":         "YES",
		"AXS_XAUTOMIN": "NO",
		"AXS_XAUTOMAX": "NO",
		"AXS_XMIN":     "0",
		"AXS_XMAX":     fmt.Sprint(samples - 1),
		"AXS_XTICK":    "NO",
		"AXS_YMIN":     "0",
		"AXS_YAUTOMIN": "NO",
		"MARGINLEFT":   "50",
		"MARGINBOTTOM": "15",
		"RASTERSIZE":   "460x240",
	})
	return p
}

func addSeries(handle, name, color, mode string) *series {
	p := iup.GetHandle(handle)
	iup.PlotBegin(p, 0)
	for i := 0; i < samples; i++ {
		iup.PlotAdd(p, float64(i), 0)
	}
	s := &series{plot: handle, index: iup.PlotEnd(p), y: make([]float64, samples)}
	p.SetAttributes(map[string]string{
		"DS_NAME":             name,
		"DS_LEGEND":           name,
		"DS_COLOR":            color,
		"DS_MODE":             mode,
		"DS_LINEWIDTH":        "2",
		"DS_AREATRANSPARENCY": "120",
	})
	return s
}

func meters() iup.Ihandle {
	heap := iup.ProgressBar().SetAttributes("EXPAND=HORIZONTAL, MAX=100").SetHandle("dash_heapbar")
	busy := iup.ProgressBar().SetAttributes("EXPAND=HORIZONTAL, MAX=100").SetHandle("dash_busybar")
	val := iup.Val("HORIZONTAL").SetAttributes("EXPAND=HORIZONTAL, MIN=0, MAX=100, VALUE=20").SetHandle("dash_load")
	val.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(loadChanged))

	return iup.Vbox(
		frame("Workload", iup.Vbox(val, meterLabel("dash_loadtext"))),
		frame("Heap in use", iup.Vbox(heap, meterLabel("dash_heaptext"))),
		frame("Workers busy", iup.Vbox(busy, meterLabel("dash_busytext"))),
		frame("Runtime", iup.Vbox(
			meterLabel("dash_gc"),
			meterLabel("dash_goroutines"),
			meterLabel("dash_uptime"),
		)),
		iup.Fill(),
	).SetAttributes("NGAP=8")
}

func meterLabel(handle string) iup.Ihandle {
	return iup.Label("").SetAttribute("EXPAND", "HORIZONTAL").SetHandle(handle)
}

func frame(title string, child iup.Ihandle) iup.Ihandle {
	return iup.Frame(child.SetAttributes("NMARGIN=4x4, NGAP=3")).SetAttribute("TITLE", title)
}

func workerTab() iup.Ihandle {
	filter := iup.Text().SetAttributes("EXPAND=HORIZONTAL, CUEBANNER=Filter").SetHandle("dash_filter")
	filter.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(iup.Ihandle) int {
		fillWorkers()
		return iup.DEFAULT
	}))

	table := iup.Table().SetAttributes(`EXPAND=YES, NUMCOL=4, SORTABLE=YES, USERRESIZE=YES,
		STRETCHLAST=YES, ALTERNATECOLOR=YES`).SetHandle("dash_table")
	table.SetAttributes(map[string]string{
		"TITLE1": "Worker", "TITLE2": "State", "TITLE3": "Iterations", "TITLE4": "Allocated",
		"ALIGNMENT3": "ARIGHT", "ALIGNMENT4": "ARIGHT",
	})
	table.SetCallback("SORT_CB", iup.TableSortFunc(sortRequested))
	table.SetCallback("CLICK_CB", iup.ClickFunc(clicked))

	return iup.Vbox(
		iup.Hbox(filter, button("Reset counters", resetCounters)).SetAttributes("NGAP=6"),
		table,
	).SetAttributes("TABTITLE=Workers, NMARGIN=8x8, NGAP=6")
}

func toolbar() iup.Ihandle {
	return iup.Hbox(
		button("Pause", togglePause).SetHandle("dash_pause"),
		button("Reset counters", resetCounters),
		iup.Label("").SetAttributes("EXPAND=HORIZONTAL, ALIGNMENT=ARIGHT").SetHandle("dash_rate"),
	).SetAttributes("NMARGIN=8x6, NGAP=6")
}

func status() iup.Ihandle {
	return iup.Hbox(
		iup.Label("").SetAttributes("EXPAND=HORIZONTAL").SetHandle("dash_status"),
		iup.Label(fmt.Sprintf("%d samples every %d ms", samples, interval)),
	).SetAttributes("NMARGIN=8x4, NGAP=8")
}

func button(title string, action func()) iup.Ihandle {
	b := iup.Button(title)
	b.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		action()
		return iup.DEFAULT
	}))
	return b
}

func buildMenu() {
	iup.SetHandle("dash_menu", iup.Menu(
		iup.Submenu("File", iup.Menu(
			item("Reset counters", resetCounters),
			iup.Separator(),
			item("Quit", func() { iup.ExitLoop() }),
		)),
		iup.Submenu("View", iup.Menu(
			item("Pause", togglePause).SetAttributes("AUTOTOGGLE=YES").SetHandle("dash_menupause"),
			item("Dark mode", toggleAppearance).SetAttributes("AUTOTOGGLE=YES").SetHandle("dash_dark"),
		)),
	))
}

func item(title string, action func()) iup.Ihandle {
	it := iup.MenuItem(title)
	it.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		action()
		return iup.DEFAULT
	}))
	return it
}

func startWorkers(n int) {
	for i := 0; i < n; i++ {
		w := &worker{name: fmt.Sprintf("worker-%02d", i+1)}
		workers = append(workers, w)
		go churn(w, int64(i))
	}
}

func churn(w *worker, seed int64) {
	r := rand.New(rand.NewSource(seed))
	for {
		level := load.Load()
		if level == 0 {
			w.busy.Store(false)
			time.Sleep(100 * time.Millisecond)
			continue
		}
		w.busy.Store(true)
		buf := make([]byte, (1+r.Intn(int(level)))*2*1024)
		for i := range buf {
			buf[i] = byte(i)
		}
		w.iters.Add(1)
		w.bytes.Add(uint64(len(buf)))
		time.Sleep(time.Duration(300/level+2) * time.Millisecond)
	}
}

func collect(iup.Ihandle) int {
	if paused {
		return iup.DEFAULT
	}

	var m runtime.MemStats
	runtime.ReadMemStats(&m)

	push(rate, float64(m.TotalAlloc-lastAlloc)/(1<<20)/(float64(interval)/1000))
	lastAlloc = m.TotalAlloc
	push(heapUsed, float64(m.HeapAlloc)/(1<<20))
	push(heapTotal, float64(m.HeapSys)/(1<<20))
	push(pause, float64(m.PauseTotalNs-lastPause)/1e6)
	lastPause = m.PauseTotalNs

	for _, name := range []string{"dash_activity", "dash_heap", "dash_pauses"} {
		iup.GetHandle(name).SetAttribute("REDRAW", "YES")
	}

	updateMeters(&m)
	updateWorkers()
	updateStatus(&m)

	return iup.DEFAULT
}

func push(s *series, v float64) {
	copy(s.y, s.y[1:])
	s.y[samples-1] = v
	p := iup.GetHandle(s.plot)
	for i, y := range s.y {
		iup.PlotSetSample(p, s.index, i, float64(i), y)
	}
}

func updateMeters(m *runtime.MemStats) {
	used := float64(m.HeapAlloc) / (1 << 20)
	total := float64(m.HeapSys) / (1 << 20)
	percent := 0.0
	if total > 0 {
		percent = used / total * 100
	}
	iup.GetHandle("dash_heapbar").SetAttribute("VALUE", fmt.Sprintf("%g", percent))
	iup.GetHandle("dash_heaptext").SetAttribute("TITLE", fmt.Sprintf("%.1f of %.1f MB", used, total))

	busy := 0
	for _, w := range workers {
		if w.busy.Load() {
			busy++
		}
	}
	iup.GetHandle("dash_busybar").SetAttribute("VALUE", fmt.Sprintf("%g", float64(busy)/float64(len(workers))*100))
	iup.GetHandle("dash_busytext").SetAttribute("TITLE", fmt.Sprintf("%d of %d", busy, len(workers)))

	iup.GetHandle("dash_gc").SetAttribute("TITLE", fmt.Sprintf("GC cycles: %d", m.NumGC))
	iup.GetHandle("dash_goroutines").SetAttribute("TITLE", fmt.Sprintf("Goroutines: %d", runtime.NumGoroutine()))
	iup.GetHandle("dash_uptime").SetAttribute("TITLE", "Uptime: "+time.Since(started).Truncate(time.Second).String())
	iup.GetHandle("dash_rate").SetAttribute("TITLE", fmt.Sprintf("%.1f MB/s", rate.y[samples-1]))
}

func fillWorkers() {
	query := strings.ToLower(iup.GetHandle("dash_filter").GetAttribute("VALUE"))
	shown = nil
	for _, w := range workers {
		if query == "" || strings.Contains(w.name, query) {
			shown = append(shown, w)
		}
	}
	sortWorkers()

	table := iup.GetHandle("dash_table")
	table.SetAttribute("NUMLIN", len(shown))
	updateWorkers()
}

func updateWorkers() {
	table := iup.GetHandle("dash_table")
	if len(shown) == 0 {
		return
	}
	sortWorkers()
	for i, w := range shown {
		lin := i + 1
		state := "idle"
		if w.busy.Load() {
			state = "busy"
		}
		iup.SetAttributeId2(table, "", lin, 1, w.name)
		iup.SetAttributeId2(table, "", lin, 2, state)
		iup.SetAttributeId2(table, "", lin, 3, fmt.Sprint(w.iters.Load()))
		iup.SetAttributeId2(table, "", lin, 4, megabytes(w.bytes.Load()))
	}
	sign := "DOWN"
	if sortAscending {
		sign = "UP"
	}
	iup.SetAttributeId(table, "SORTSIGN", sortColumn, sign)
}

func sortWorkers() {
	sort.SliceStable(shown, func(i, j int) bool {
		a, b := shown[i], shown[j]
		less := false
		switch sortColumn {
		case 1:
			less = a.name < b.name
		case 2:
			less = !a.busy.Load() && b.busy.Load()
		case 3:
			less = a.iters.Load() < b.iters.Load()
		case 4:
			less = a.bytes.Load() < b.bytes.Load()
		}
		if sortAscending {
			return less
		}
		return !less
	})
}

func sortRequested(ih iup.Ihandle, col int) int {
	if col == sortColumn {
		sortAscending = !sortAscending
	} else {
		sortColumn, sortAscending = col, true
	}
	updateWorkers()
	return iup.IGNORE
}

func clicked(ih iup.Ihandle, lin, col int, status string) int {
	if iup.IsButton3(status) && lin > 0 && lin <= len(shown) {
		w := shown[lin-1]
		menu := iup.Menu(
			item("Copy "+w.name, func() { copyWorker(w) }),
			item("Reset "+w.name, func() { reset(w); updateWorkers() }),
		)
		iup.Popup(menu, iup.MOUSEPOS, iup.MOUSEPOS)
		iup.Destroy(menu)
	}
	return iup.DEFAULT
}

func copyWorker(w *worker) {
	clip := iup.Clipboard()
	clip.SetAttribute("TEXT", fmt.Sprintf("%s\t%d\t%s", w.name, w.iters.Load(), megabytes(w.bytes.Load())))
	iup.Destroy(clip)
}

func megabytes(n uint64) string {
	return fmt.Sprintf("%.1f MB", float64(n)/(1<<20))
}

func resetCounters() {
	for _, w := range workers {
		reset(w)
	}
	updateWorkers()
}

func reset(w *worker) {
	w.iters.Store(0)
	w.bytes.Store(0)
}

func loadChanged(ih iup.Ihandle) int {
	load.Store(int64(ih.GetFloat("VALUE")))
	iup.GetHandle("dash_loadtext").SetAttribute("TITLE", fmt.Sprintf("%d %%", load.Load()))
	return iup.DEFAULT
}

func togglePause() {
	paused = !paused
	title := "Pause"
	if paused {
		title = "Resume"
	}
	iup.GetHandle("dash_pause").SetAttribute("TITLE", title)
	iup.GetHandle("dash_menupause").SetAttribute("VALUE", boolean(paused))
}

func updateStatus(m *runtime.MemStats) {
	iup.GetHandle("dash_status").SetAttribute("TITLE", fmt.Sprintf(
		"%d workers, %d GC cycles, %.1f MB allocated in total",
		len(workers), m.NumGC, float64(m.TotalAlloc)/(1<<20)))
}

func toggleAppearance() {
	if iup.GetGlobal("APPEARANCE") == "DARK" {
		iup.SetGlobal("APPEARANCE", "LIGHT")
	} else {
		iup.SetGlobal("APPEARANCE", "DARK")
	}
	showAppearance()
}

func showAppearance() {
	iup.GetHandle("dash_dark").SetAttribute("VALUE", boolean(iup.GetGlobal("APPEARANCE") == "DARK"))
	retheme()
}

func retheme() {
	back := global("DLGBGCOLOR", "240 240 240")
	fore := global("TXTFGCOLOR", "0 0 0")
	for _, name := range []string{"dash_activity", "dash_heap", "dash_pauses"} {
		iup.GetHandle(name).SetAttributes(map[string]string{
			"BGCOLOR":   back,
			"FGCOLOR":   fore,
			"BACKCOLOR": global("TXTBGCOLOR", back),
			"GRIDCOLOR": mix(fore, back, 0.75),
			"REDRAW":    "YES",
		})
	}
}

func boolean(b bool) string {
	if b {
		return "ON"
	}
	return "OFF"
}

func global(name, fallback string) string {
	if v := iup.GetGlobal(name); v != "" {
		return v
	}
	return fallback
}

func mix(a, b string, t float64) string {
	ar, ag, ab := rgb(a)
	br, bg, bb := rgb(b)
	return fmt.Sprintf("%d %d %d", int(ar+(br-ar)*t), int(ag+(bg-ag)*t), int(ab+(bb-ab)*t))
}

func rgb(c string) (float64, float64, float64) {
	var r, g, b int
	if strings.HasPrefix(c, "#") {
		fmt.Sscanf(c, "#%02x%02x%02x", &r, &g, &b)
	} else {
		fmt.Sscanf(c, "%d %d %d", &r, &g, &b)
	}
	return float64(r), float64(g), float64(b)
}
