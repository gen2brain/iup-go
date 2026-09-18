package main

import (
	"fmt"
	"sort"
	"strings"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

const (
	totalSamples = 50000
	maxVirtual   = 1000000
	maxDepth     = 32
)

type frame struct {
	name   string
	start  int
	total  int
	depth  int
	parent int
}

var (
	frames []frame
	levels [][]int

	rowH    = 18
	pps     float64
	hovered = -1
	root    = 0
	filter  string
	matches int

	pinchZoom   float64
	pinchSample float64

	canvas iup.Ihandle
	status iup.Ihandle
)

func main() {
	iup.Open()
	iup.SetGlobal("UTF8MODE", "YES")
	defer iup.Close()

	if phone() {
		rowH = 22
	}

	build()

	canvas = iup.Canvas().SetAttributes(map[string]string{
		"SCROLLBAR":  "YES",
		"RASTERSIZE": canvasSize(),
		"EXPAND":     "YES",
		"CANFOCUS":   "YES",
		"XAUTOHIDE":  "YES",
		"YAUTOHIDE":  "YES",
	})
	canvas.SetAttribute("DRAWTEXTELLIPSIS", "YES")

	canvas.SetCallback("ACTION", iup.ActionFunc(redraw))
	canvas.SetCallback("RESIZE_CB", iup.ResizeFunc(resized))
	canvas.SetCallback("SCROLL_CB", iup.ScrollFunc(scrolled))
	canvas.SetCallback("MOTION_CB", iup.MotionFunc(moved))
	canvas.SetCallback("BUTTON_CB", iup.ButtonFunc(clicked))
	canvas.SetCallback("WHEEL_CB", iup.WheelFunc(wheeled))
	canvas.SetCallback("GESTURE_CB", iup.GestureFunc(gestured))
	canvas.SetCallback("LEAVEWINDOW_CB", iup.LeaveWindowFunc(func(ih iup.Ihandle) int {
		hovered = -1
		iup.Update(ih)
		report()
		return iup.DEFAULT
	}))

	search := iup.Text().SetAttributes("VISIBLECOLUMNS=14, EXPAND=HORIZONTAL")
	search.SetAttribute("CUEBANNER", "Filter")
	search.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		filter = strings.ToLower(ih.GetAttribute("VALUE"))
		countMatches()
		iup.Update(canvas)
		report()
		return iup.DEFAULT
	}))

	tools := iup.Hbox(
		button("Zoom out", zoomOut),
		button("Reset", reset),
		search,
	).SetAttributes("NGAP=6, NMARGIN=6x6, ALIGNMENT=ACENTER")

	status = iup.Label("").SetAttributes("EXPAND=HORIZONTAL, PADDING=6x4")

	dlg := iup.Dialog(iup.Vbox(tools, canvas, status))
	dlg.SetAttribute("TITLE", "Flame Graph")
	dlg.SetCallback("K_ANY", iup.KAnyFunc(key))

	iup.Show(dlg)

	report()
	iup.MainLoop()
}

func canvasSize() string {
	if phone() {
		return "240x260"
	}
	return "700x420"
}

func phone() bool {
	switch iup.GetGlobal("SYSTEM") {
	case "Android", "iOS":
		return true
	}
	return false
}

type rng struct{ state uint64 }

func (r *rng) next() uint64 {
	r.state = r.state*6364136223846793005 + 1442695040888963407
	return r.state >> 33
}

func (r *rng) intn(n int) int {
	if n <= 0 {
		return 0
	}
	return int(r.next() % uint64(n))
}

var packages = []struct {
	name  string
	funcs []string
}{
	{"main", []string{"run", "serve", "worker", "collect"}},
	{"net/http", []string{"(*conn).serve", "(*ServeMux).ServeHTTP", "(*Transport).roundTrip", "readRequest"}},
	{"encoding/json", []string{"(*decodeState).object", "(*decodeState).value", "Marshal", "(*encodeState).string"}},
	{"database/sql", []string{"(*DB).QueryContext", "(*Rows).Next", "(*Stmt).query"}},
	{"bytes", []string{"(*Buffer).Write", "(*Buffer).grow", "Join"}},
	{"strings", []string{"(*Builder).WriteString", "Split", "ToLower", "Index"}},
	{"sync", []string{"(*Mutex).Lock", "(*Pool).Get", "(*WaitGroup).Wait"}},
	{"runtime", []string{"mallocgc", "gcDrain", "scanobject", "memmove", "growslice", "mapassign"}},
	{"compress/gzip", []string{"(*Writer).Write", "(*Reader).Read"}},
	{"crypto/tls", []string{"(*Conn).Read", "(*halfConn).decrypt", "handshake"}},
}

func build() {
	r := &rng{state: 42}
	frames = append(frames, frame{name: "main.main", total: totalSamples, parent: -1})
	grow(r, 0, 0)

	for i := range frames {
		for len(levels) <= frames[i].depth {
			levels = append(levels, nil)
		}
		levels[frames[i].depth] = append(levels[frames[i].depth], i)
	}
	for _, level := range levels {
		sort.Slice(level, func(a, b int) bool { return frames[level[a]].start < frames[level[b]].start })
	}
}

func grow(r *rng, idx, depth int) {
	total := frames[idx].total
	start := frames[idx].start
	if total < 8 || depth >= maxDepth {
		return
	}

	count := 1 + r.intn(3)
	if depth > 3 {
		count = 1 + r.intn(2)
	}
	rest := total - total/10
	kids := make([]int, 0, count)

	for i := 0; i < count && rest > 2; i++ {
		share := rest
		if i < count-1 {
			share = rest/(count-i) + r.intn(rest/(2*(count-i))+1)
		}
		if share > rest {
			share = rest
		}

		frames = append(frames, frame{name: name(r, idx, depth), start: start, total: share, depth: depth + 1, parent: idx})
		kids = append(kids, len(frames)-1)
		start += share
		rest -= share
	}

	for _, kid := range kids {
		grow(r, kid, depth+1)
	}
}

func name(r *rng, parent, depth int) string {
	if depth > 3 && r.intn(8) == 0 {
		return frames[parent].name
	}
	pkg := packages[r.intn(len(packages))]
	if depth > 6 && r.intn(3) == 0 {
		pkg = packages[len(packages)-3+r.intn(3)]
	}
	return pkg.name + "." + pkg.funcs[r.intn(len(pkg.funcs))]
}

func countMatches() {
	matches = 0
	if filter == "" {
		return
	}
	for i := range frames {
		if strings.Contains(strings.ToLower(frames[i].name), filter) {
			matches++
		}
	}
}

func drawSize() (int, int) {
	_, w, h := iup.GetInt2(canvas, "DRAWSIZE")
	return w, h
}

func fitZoom() float64 {
	w, _ := drawSize()
	return float64(w) / float64(frames[root].total)
}

func clampZoom(z float64) float64 {
	fit := fitZoom()
	if z < fit {
		z = fit
	}
	if z*float64(totalSamples) > maxVirtual {
		z = maxVirtual / float64(totalSamples)
	}
	return z
}

func apply(posx float64) {
	width := pps * float64(totalSamples)
	height := float64(len(levels) * rowH)
	w, h := drawSize()

	canvas.SetAttribute("XMAX", fmt.Sprintf("%d", int(width)))
	canvas.SetAttribute("YMAX", fmt.Sprintf("%d", int(height)))
	canvas.SetAttribute("DX", fmt.Sprintf("%d", w))
	canvas.SetAttribute("DY", fmt.Sprintf("%d", h))
	canvas.SetAttribute("LINEX", fmt.Sprintf("%d", w/8))
	canvas.SetAttribute("LINEY", fmt.Sprintf("%d", rowH))

	if max := width - float64(w); posx > max {
		posx = max
	}
	if posx < 0 {
		posx = 0
	}
	canvas.SetAttribute("POSX", fmt.Sprintf("%f", posx))

	iup.Update(canvas)
	report()
}

func zoomTo(idx int) {
	root = idx
	w, _ := drawSize()
	pps = clampZoom(float64(w) / float64(frames[idx].total))
	apply(float64(frames[idx].start) * pps)
}

func zoomOut() {
	if frames[root].parent >= 0 {
		zoomTo(frames[root].parent)
	} else {
		reset()
	}
}

func reset() {
	root = 0
	pps = fitZoom()
	apply(0)
}

func resized(ih iup.Ihandle, width, height int) int {
	if pps == 0 || pps <= fitZoom() {
		pps = fitZoom()
	}
	apply(float64(ih.GetFloat("POSX")))
	return iup.DEFAULT
}

func scrolled(ih iup.Ihandle, op int, posx, posy float64) int {
	iup.Update(ih)
	report()
	return iup.DEFAULT
}

func redraw(ih iup.Ihandle) int {
	posx := float64(ih.GetInt("POSX"))
	posy := ih.GetInt("POSY")

	iup.DrawBegin(ih)

	w, h := iup.DrawGetSize(ih)

	ih.SetAttributes("DRAWSTYLE=FILL, DRAWCOLOR=\"250 250 250\"")
	iup.DrawRectangle(ih, 0, 0, w, h)

	first := posy / rowH
	last := (posy + h) / rowH

	for depth := first; depth <= last && depth < len(levels); depth++ {
		y := depth*rowH - posy
		level := levels[depth]

		i := sort.Search(len(level), func(i int) bool {
			f := frames[level[i]]
			return float64(f.start+f.total)*pps > posx
		})

		for ; i < len(level); i++ {
			f := frames[level[i]]
			x1 := int(float64(f.start)*pps - posx)
			if x1 > w {
				break
			}

			x2 := int(float64(f.start+f.total)*pps - posx)
			if x2-x1 < 1 {
				continue
			}

			ih.SetAttributes("DRAWSTYLE=FILL, DRAWCOLOR=" + color(level[i]))
			iup.DrawRectangle(ih, x1, y, x2-2, y+rowH-2)

			tx, tw := x1, x2-x1
			if tx < 0 {
				tw, tx = tw+tx, 0
			}
			if tx+tw > w {
				tw = w - tx
			}

			if tw > 40 {
				iup.DrawSetClipRect(ih, tx, y, tx+tw-2, y+rowH-2)
				ih.SetAttribute("DRAWCOLOR", "25 25 25")
				iup.DrawText(ih, f.name, tx+4, y+2, tw-8, rowH-4)
				iup.DrawResetClip(ih)
			}
		}
	}

	if hovered >= 0 {
		f := frames[hovered]
		x1 := int(float64(f.start)*pps - posx)
		x2 := int(float64(f.start+f.total)*pps - posx)
		y := f.depth*rowH - posy
		ih.SetAttributes("DRAWSTYLE=STROKE, DRAWCOLOR=\"20 20 20\", DRAWLINEWIDTH=2")
		iup.DrawRectangle(ih, x1, y, x2-2, y+rowH-2)
		ih.SetAttribute("DRAWLINEWIDTH", "1")
	}

	iup.DrawEnd(ih)
	return iup.DEFAULT
}

func color(idx int) string {
	f := frames[idx]
	if filter != "" && strings.Contains(strings.ToLower(f.name), filter) {
		return "\"70 130 220\""
	}

	hash := 0
	for _, c := range f.name {
		hash = hash*31 + int(c)
	}
	if hash < 0 {
		hash = -hash
	}

	return fmt.Sprintf("\"%d %d %d\"", 215+hash%40, 90+(hash/40)%120, 45+(hash/13)%35)
}

func frameAt(x, y int) int {
	posx := float64(canvas.GetInt("POSX"))
	posy := canvas.GetInt("POSY")

	depth := (y + posy) / rowH
	if depth < 0 || depth >= len(levels) {
		return -1
	}

	sample := int((float64(x) + posx) / pps)
	level := levels[depth]

	i := sort.Search(len(level), func(i int) bool {
		f := frames[level[i]]
		return f.start+f.total > sample
	})
	if i < len(level) && frames[level[i]].start <= sample {
		return level[i]
	}
	return -1
}

func moved(ih iup.Ihandle, x, y int, status string) int {
	found := frameAt(x, y)
	if found == hovered {
		return iup.DEFAULT
	}

	hovered = found
	if hovered >= 0 {
		f := frames[hovered]
		ih.SetAttribute("TIP", fmt.Sprintf("%s\n%d samples (%s)\ndepth %d", f.name, f.total, percent(f.total), f.depth))
	} else {
		ih.SetAttribute("TIP", "")
	}

	iup.Update(ih)
	report()
	return iup.DEFAULT
}

func clicked(ih iup.Ihandle, button, pressed, x, y int, status string) int {
	if pressed == 0 {
		return iup.DEFAULT
	}

	if iup.IsDouble(status) {
		reset()
		return iup.DEFAULT
	}

	switch button {
	case iup.BUTTON1:
		if found := frameAt(x, y); found >= 0 {
			zoomTo(found)
		}
	case iup.BUTTON3:
		zoomOut()
	}
	return iup.DEFAULT
}

func wheeled(ih iup.Ihandle, delta float64, x, y int, status string) int {
	if iup.IsControl(status) {
		sample := (float64(x) + float64(ih.GetInt("POSX"))) / pps
		if delta > 0 {
			pps = clampZoom(pps * 1.3)
		} else {
			pps = clampZoom(pps / 1.3)
		}
		apply(sample*pps - float64(x))
		return iup.DEFAULT
	}

	ih.SetAttribute("POSY", fmt.Sprintf("%d", ih.GetInt("POSY")-int(delta*float64(rowH*3))))
	iup.Update(ih)
	report()
	return iup.DEFAULT
}

func gestured(ih iup.Ihandle, gesture, state, x, y int, v1, v2 float64) int {
	if gesture != iup.GESTURE_PINCH {
		return iup.DEFAULT
	}

	switch state {
	case iup.GESTURE_BEGIN:
		pinchZoom = pps
		pinchSample = (float64(x) + float64(ih.GetInt("POSX"))) / pps
	case iup.GESTURE_CHANGED:
		if pinchZoom == 0 || v1 <= 0 {
			return iup.DEFAULT
		}
		pps = clampZoom(pinchZoom * v1)
		apply(pinchSample*pps - float64(x))
	}

	return iup.DEFAULT
}

func key(ih iup.Ihandle, c int) int {
	switch c {
	case iup.K_ESC:
		reset()
	case iup.K_plus, iup.K_equal:
		pps = clampZoom(pps * 1.3)
		apply(float64(canvas.GetFloat("POSX")))
	case iup.K_minus:
		pps = clampZoom(pps / 1.3)
		apply(float64(canvas.GetFloat("POSX")))
	case iup.K_BS:
		zoomOut()
	default:
		return iup.CONTINUE
	}
	return iup.DEFAULT
}

func percent(samples int) string {
	return fmt.Sprintf("%.2f%%", 100*float64(samples)/float64(totalSamples))
}

func report() {
	text := fmt.Sprintf("%d frames, %d samples", len(frames), totalSamples)
	if hovered >= 0 {
		f := frames[hovered]
		text = fmt.Sprintf("%s  %d samples, %s", f.name, f.total, percent(f.total))
	} else if filter != "" {
		text = fmt.Sprintf("%d frames match %q", matches, filter)
	}

	if phone() {
		status.SetAttribute("TITLE", text)
		return
	}

	status.SetAttribute("TITLE", fmt.Sprintf("%s   |   view %d px of %s, zoom %.3f px/sample",
		text, canvas.GetInt("POSX"), canvas.GetAttribute("XMAX"), pps))
}

func button(title string, action func()) iup.Ihandle {
	return iup.Button(title).SetAttributes("PADDING=10x4").
		SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
			action()
			return iup.DEFAULT
		}))
}
