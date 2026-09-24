package main

import (
	"fmt"
	"math"
	"os"
	"strconv"
	"strings"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

const (
	nodeW = 172
	nodeH = 76
)

type node struct {
	id            int
	title, detail string
	shape, color  int
	x, y          int
}

type edge struct {
	from, to int
}

type colors struct {
	bg, panel, grid, text, dim, line, selection string
	fills, strokes                              []string
}

type pointer struct {
	x, y, panX, panY  float64
	node              int
	dragging, panning bool
}

var (
	nodes = []node{
		{1, "New request", "Customer submits a request", 2, 0, 40, 180},
		{2, "Validate", "Check required information", 0, 1, 285, 180},
		{3, "Complete?", "All fields are present", 1, 2, 535, 180},
		{4, "Ask for details", "Return to the customer", 0, 3, 535, 335},
		{5, "Review", "Assign an owner", 0, 4, 785, 180},
		{6, "Resolved", "Notify and archive", 2, 5, 1030, 180},
	}
	edges = []edge{{1, 2}, {2, 3}, {3, 4}, {4, 2}, {3, 5}, {5, 6}}

	nextID       = 7
	selected     = 3
	hovered      = -1
	connectFrom  = -1
	connectX     float64
	connectY     float64
	zoom         = 1.0
	panX         = 40.0
	panY         = 40.0
	fitted       = true
	showGrid     = true
	editing      bool
	editOriginal string
	editValue    string
	syncing      bool
	press        pointer
	pal          colors
	canvas       iup.Ihandle
)

var hitPath iup.Ihandle

var shapeNames = []string{"Process", "Decision", "Terminal"}
var colorNames = []string{"Blue", "Violet", "Amber", "Rose", "Teal", "Green"}

func main() {
	iup.Open()
	defer iup.Close()
	iup.SetGlobal("UTF8MODE", "YES")

	hitPath = iup.DrawPathCreate()

	setPalette()

	canvas = iup.Canvas().SetAttributes("BORDER=NO, EXPAND=YES, CANFOCUS=YES, TOUCH=YES").SetHandle("diagram_canvas")
	canvas.SetCallback("ACTION", iup.ActionFunc(draw))
	canvas.SetCallback("BUTTON_CB", iup.ButtonFunc(buttonEvent))
	canvas.SetCallback("MOTION_CB", iup.MotionFunc(motion))
	canvas.SetCallback("WHEEL_CB", iup.WheelFunc(wheel))
	canvas.SetCallback("GESTURE_CB", iup.GestureFunc(gesture))
	canvas.SetCallback("K_ANY", iup.KAnyFunc(key))
	canvas.SetCallback("TEXTINPUT_CB", iup.TextInputFunc(textInput))
	canvas.SetCallback("RESIZE_CB", iup.ResizeFunc(resize))

	mainArea := iup.Split(canvas, inspector()).SetAttributes("ORIENTATION=VERTICAL, VALUE=760, SHOWGRIP=YES")
	if phone() {
		mainArea = iup.Vbox(canvas, inspector()).SetAttributes("NGAP=0")
	}

	status := iup.Label("").SetAttributes("EXPAND=HORIZONTAL, PADDING=8x5").SetHandle("diagram_status")
	dlg := iup.Dialog(iup.Vbox(toolbar(), mainArea, status).SetAttributes("NGAP=0")).SetHandle("diagram_dlg")
	dlg.SetAttributes(map[string]string{"TITLE": "Diagram Editor", "PLACEMENT": "MAXIMIZED"})
	dlg.SetCallback("THEMECHANGED_CB", iup.ThemeChangedFunc(func(iup.Ihandle, int) int {
		setPalette()
		iup.Update(canvas)
		return iup.DEFAULT
	}))

	iup.Show(dlg)
	syncInspector()
	setStatus("Drag nodes to arrange them. Drag the blue port to connect. Wheel to zoom.")
	iup.MainLoop()
}

func phone() bool {
	switch iup.GetGlobal("SYSTEM") {
	case "Android", "iOS":
		return true
	}
	return false
}

func toolbar() iup.Ihandle {
	first := []iup.Ihandle{
		tool("Process", func() { addNode(0) }),
		tool("Decision", func() { addNode(1) }),
		tool("End", func() { addNode(2) }),
		tool("Connect", connect),
		tool("Clone", duplicate),
		tool("Delete", remove),
	}
	second := []iup.Ihandle{
		tool("-", func() { zoomAt(0.8, -1, -1) }),
		tool("Fit", fit),
		tool("+", func() { zoomAt(1.25, -1, -1) }),
		gridToggle(),
		tool("SVG", exportSVG),
		tool("Theme", toggleTheme),
	}
	if phone() {
		return iup.Vbox(
			iup.Hbox(first...).SetAttributes("NMARGIN=6x4, NGAP=4, ALIGNMENT=ACENTER"),
			iup.Hbox(second...).SetAttributes("NMARGIN=6x4, NGAP=4, ALIGNMENT=ACENTER"),
		)
	}
	return iup.Hbox(append(first, append([]iup.Ihandle{iup.Fill()}, second...)...)...).
		SetAttributes("NMARGIN=6x5, NGAP=4, ALIGNMENT=ACENTER")
}

func tool(title string, action func()) iup.Ihandle {
	b := iup.Button(title).SetAttributes("PADDING=5x3, CANFOCUS=NO")
	b.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		action()
		iup.SetFocus(canvas)
		return iup.DEFAULT
	}))
	return b
}

func gridToggle() iup.Ihandle {
	t := iup.Toggle("Grid").SetAttributes("VALUE=ON, CANFOCUS=NO")
	t.SetCallback("ACTION", iup.ToggleActionFunc(func(_ iup.Ihandle, state int) int {
		showGrid = state == 1
		iup.Update(canvas)
		return iup.DEFAULT
	}))
	return t
}

func inspector() iup.Ihandle {
	name := iup.Text().SetAttributes("VISIBLECOLUMNS=20, EXPAND=HORIZONTAL").SetHandle("diagram_name")
	name.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		if n := selectedNode(); n != nil && !syncing {
			n.title = ih.GetAttribute("VALUE")
			iup.Update(canvas)
		}
		return iup.DEFAULT
	}))

	detail := iup.Text().SetAttributes("VISIBLECOLUMNS=20, EXPAND=HORIZONTAL").SetHandle("diagram_detail")
	detail.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		if n := selectedNode(); n != nil && !syncing {
			n.detail = ih.GetAttribute("VALUE")
			iup.Update(canvas)
		}
		return iup.DEFAULT
	}))

	shape := choice("diagram_shape", shapeNames, func(pos int) {
		if n := selectedNode(); n != nil {
			n.shape = pos
			iup.Update(canvas)
		}
	})
	color := choice("diagram_color", colorNames, func(pos int) {
		if n := selectedNode(); n != nil {
			n.color = pos
			iup.Update(canvas)
		}
	})

	fields := iup.Vbox(
		iup.Label("Title").SetAttribute("FONTSTYLE", "Bold"), name,
		iup.Label("Description").SetAttribute("FONTSTYLE", "Bold"), detail,
		iup.Label("Shape").SetAttribute("FONTSTYLE", "Bold"), shape,
		iup.Label("Color").SetAttribute("FONTSTYLE", "Bold"), color,
		iup.Label("").SetHandle("diagram_position"),
	).SetAttributes("NGAP=5")

	help := iup.Vbox(
		iup.Label("Canvas shortcuts").SetAttribute("FONTSTYLE", "Bold"),
		iup.Label("F2  Rename on canvas\nArrows  Nudge selection\nCtrl+C / Ctrl+V  Copy and paste\nDelete  Remove\nEsc  Cancel connection"),
	).SetAttributes("NGAP=5")

	return iup.Vbox(
		iup.Label("Inspector").SetAttributes("FONTSTYLE=Bold, PADDING=0x3"),
		fields,
		iup.Fill(),
		help,
	).SetAttributes("NMARGIN=12x10, NGAP=8, EXPAND=VERTICAL").SetHandle("diagram_inspector")
}

func choice(handle string, items []string, changed func(int)) iup.Ihandle {
	list := iup.List().SetAttributes("DROPDOWN=YES, EXPAND=HORIZONTAL").SetHandle(handle)
	for i, item := range items {
		iup.SetAttributeId(list, "", i+1, item)
	}
	list.SetCallback("ACTION", iup.ListActionFunc(func(_ iup.Ihandle, _ string, item, state int) int {
		if state == 1 && !syncing {
			changed(item - 1)
		}
		return iup.DEFAULT
	}))
	return list
}

func draw(ih iup.Ihandle) int {
	iup.DrawBegin(ih)
	defer iup.DrawEnd(ih)

	w, h := iup.DrawGetSize(ih)
	ih.SetAttributes("DRAWSTYLE=FILL, DRAWCOLOR=\"" + pal.bg + "\"")
	iup.DrawRectangle(ih, 0, 0, w, h)

	iup.DrawSave(ih)
	iup.DrawTranslate(ih, panX, panY)
	iup.DrawScale(ih, zoom, zoom)
	if showGrid {
		drawGrid(ih, w, h)
	}
	for _, e := range edges {
		drawEdge(ih, e.from, e.to, pal.line, 2)
	}
	if connectFrom >= 0 {
		drawLooseEdge(ih, connectFrom, connectX, connectY)
	}
	for i := range nodes {
		drawNode(ih, &nodes[i])
	}
	iup.DrawRestore(ih)

	drawMinimap(ih, w, h)
	return iup.DEFAULT
}

func drawGrid(ih iup.Ihandle, width, height int) {
	x0, y0 := screenToWorld(0, 0)
	x1, y1 := screenToWorld(width, height)
	step := 24
	startX := int(math.Floor(x0/float64(step))) * step
	startY := int(math.Floor(y0/float64(step))) * step
	ih.SetAttributes("DRAWSTYLE=FILL, DRAWCOLOR=\"" + pal.grid + "\"")
	for x := startX; float64(x) <= x1; x += step {
		for y := startY; float64(y) <= y1; y += step {
			iup.DrawEllipse(ih, x, y, x+1, y+1)
		}
	}
}

func drawNode(ih iup.Ihandle, n *node) {
	iup.DrawSave(ih)

	nodePath(ih, n, 4, 5)
	iup.DrawSetSourceSolid(ih, "0 0 0 38")
	iup.DrawPathFill(ih, iup.DRAW_RULE_WINDING)

	nodePath(ih, n, 0, 0)
	iup.DrawSetSourceLinearGradient(ih, n.x, n.y, n.x+nodeW, n.y+nodeH, 90,
		[]string{pal.fills[n.color], mix(pal.fills[n.color], pal.bg, 0.24)}, nil)
	iup.DrawPathFill(ih, iup.DRAW_RULE_WINDING)

	nodePath(ih, n, 0, 0)
	iup.DrawSetSourceSolid(ih, pal.strokes[n.color])
	ih.SetAttributes("DRAWSTYLE=STROKE, DRAWLINEWIDTH=2")
	iup.DrawPathStroke(ih)

	if n.id == selected {
		nodePath(ih, n, -4, -4)
		iup.DrawSetSourceSolid(ih, pal.selection)
		ih.SetAttributes("DRAWSTYLE=STROKE_DASH, DRAWLINEWIDTH=2")
		iup.DrawPathStroke(ih)
	}

	ih.SetAttributes("DRAWFONT=\"Helvetica, Bold 11\", DRAWCOLOR=\"" + pal.text + "\", DRAWTEXTALIGNMENT=ACENTER, DRAWTEXTELLIPSIS=YES")
	title := n.title
	if editing && n.id == selected {
		title = editValue + "|"
	}
	iup.DrawText(ih, title, n.x+16, n.y+17, nodeW-32, 20)
	ih.SetAttributes("DRAWFONT=\"Helvetica, 9\", DRAWCOLOR=\"" + pal.dim + "\"")
	iup.DrawText(ih, n.detail, n.x+16, n.y+43, nodeW-32, 18)

	if n.id == selected || n.id == hovered || n.id == connectFrom {
		cx, cy := outputPort(n)
		ih.SetAttributes("DRAWSTYLE=FILL, DRAWCOLOR=\"" + pal.bg + "\"")
		iup.DrawEllipse(ih, cx-7, cy-7, cx+7, cy+7)
		ih.SetAttribute("DRAWCOLOR", pal.selection)
		iup.DrawEllipse(ih, cx-4, cy-4, cx+4, cy+4)
	}

	iup.DrawRestore(ih)
}

func nodePath(ih iup.Ihandle, n *node, dx, dy int) {
	iup.DrawPathBegin(ih)
	nodeShape(ih, n, dx, dy)
}

func nodeShape(target iup.Ihandle, n *node, dx, dy int) {
	ih := target
	x, y := n.x+dx, n.y+dy
	w, h := nodeW-2*dx, nodeH-2*dy
	switch n.shape {
	case 1:
		iup.DrawPathMoveTo(ih, x+w/2, y)
		iup.DrawPathLineTo(ih, x+w, y+h/2)
		iup.DrawPathLineTo(ih, x+w/2, y+h)
		iup.DrawPathLineTo(ih, x, y+h/2)
		iup.DrawPathClose(ih)
	case 2:
		r := h / 2
		iup.DrawPathMoveTo(ih, x+r, y)
		iup.DrawPathLineTo(ih, x+w-r, y)
		iup.DrawPathCurveTo(ih, x+w-r/2, y, x+w, y+r/2, x+w, y+r)
		iup.DrawPathCurveTo(ih, x+w, y+h-r/2, x+w-r/2, y+h, x+w-r, y+h)
		iup.DrawPathLineTo(ih, x+r, y+h)
		iup.DrawPathCurveTo(ih, x+r/2, y+h, x, y+h-r/2, x, y+h-r)
		iup.DrawPathCurveTo(ih, x, y+r/2, x+r/2, y, x+r, y)
		iup.DrawPathClose(ih)
	default:
		r := 14
		iup.DrawPathMoveTo(ih, x+r, y)
		iup.DrawPathLineTo(ih, x+w-r, y)
		iup.DrawPathCurveTo(ih, x+w-5, y, x+w, y+5, x+w, y+r)
		iup.DrawPathLineTo(ih, x+w, y+h-r)
		iup.DrawPathCurveTo(ih, x+w, y+h-5, x+w-5, y+h, x+w-r, y+h)
		iup.DrawPathLineTo(ih, x+r, y+h)
		iup.DrawPathCurveTo(ih, x+5, y+h, x, y+h-5, x, y+h-r)
		iup.DrawPathLineTo(ih, x, y+r)
		iup.DrawPathCurveTo(ih, x, y+5, x+5, y, x+r, y)
		iup.DrawPathClose(ih)
	}
}

func drawEdge(ih iup.Ihandle, fromID, toID int, color string, width int) {
	from, to := nodeByID(fromID), nodeByID(toID)
	if from == nil || to == nil {
		return
	}
	sx, sy, ex, ey, c1x, c2x := edgePoints(from, to)
	ih.SetAttributes(fmt.Sprintf("DRAWSTYLE=STROKE, DRAWLINEWIDTH=%d", width))
	iup.DrawSetSourceSolid(ih, color)
	iup.DrawPathBegin(ih)
	iup.DrawPathMoveTo(ih, sx, sy)
	iup.DrawPathCurveTo(ih, c1x, sy, c2x, ey, ex, ey)
	iup.DrawPathStroke(ih)
	drawArrow(ih, ex, ey, ex-c2x, 0, color)
}

func drawLooseEdge(ih iup.Ihandle, fromID int, x, y float64) {
	from := nodeByID(fromID)
	if from == nil {
		return
	}
	sx, sy := outputPort(from)
	ex, ey := int(x), int(y)
	dx := ex - sx
	c := sx + dx/2
	ih.SetAttributes("DRAWSTYLE=STROKE_DASH, DRAWLINEWIDTH=2")
	iup.DrawSetSourceSolid(ih, pal.selection)
	iup.DrawPathBegin(ih)
	iup.DrawPathMoveTo(ih, sx, sy)
	iup.DrawPathCurveTo(ih, c, sy, c, ey, ex, ey)
	iup.DrawPathStroke(ih)
}

func edgePoints(from, to *node) (sx, sy, ex, ey, c1x, c2x int) {
	sy, ey = from.y+nodeH/2, to.y+nodeH/2
	if to.x >= from.x {
		sx, ex = from.x+nodeW, to.x
	} else {
		sx, ex = from.x, to.x+nodeW
	}
	d := abs(ex - sx)
	curve := max(60, d/2)
	if ex < sx {
		curve = -curve
	}
	c1x, c2x = sx+curve, ex-curve
	return
}

func drawArrow(ih iup.Ihandle, x, y, dx, dy int, color string) {
	angle := math.Atan2(float64(dy), float64(dx))
	size := 10.0
	points := []int{
		x, y,
		x + int(size*math.Cos(angle+2.55)), y + int(size*math.Sin(angle+2.55)),
		x + int(size*math.Cos(angle-2.55)), y + int(size*math.Sin(angle-2.55)),
	}
	ih.SetAttributes("DRAWSTYLE=FILL, DRAWCOLOR=\"" + color + "\"")
	iup.DrawPolygon(ih, points, 3)
}

func outputPort(n *node) (int, int) {
	return n.x + nodeW, n.y + nodeH/2
}

func drawMinimap(ih iup.Ihandle, width, height int) {
	if len(nodes) == 0 {
		return
	}
	x, y, w, h := minimapRect(width, height)
	ih.SetAttributes("DRAWSTYLE=FILL, DRAWCOLOR=\"" + pal.panel + " 235\"")
	iup.DrawRoundedRectangle(ih, x, y, x+w, y+h, 8)
	ih.SetAttributes("DRAWSTYLE=STROKE, DRAWLINEWIDTH=1, DRAWCOLOR=\"" + pal.grid + "\"")
	iup.DrawRoundedRectangle(ih, x, y, x+w, y+h, 8)

	minX, minY, maxX, maxY := sceneBounds()
	s := math.Min(float64(w-16)/float64(maxX-minX), float64(h-16)/float64(maxY-minY))
	ox := float64(x+8) + (float64(w-16)-float64(maxX-minX)*s)/2
	oy := float64(y+8) + (float64(h-16)-float64(maxY-minY)*s)/2
	mapPoint := func(wx, wy float64) (int, int) {
		return int(ox + (wx-float64(minX))*s), int(oy + (wy-float64(minY))*s)
	}
	ih.SetAttributes("DRAWSTYLE=STROKE, DRAWCOLOR=\"" + pal.line + "\"")
	for _, e := range edges {
		a, b := nodeByID(e.from), nodeByID(e.to)
		if a == nil || b == nil {
			continue
		}
		x1, y1 := mapPoint(float64(a.x+nodeW/2), float64(a.y+nodeH/2))
		x2, y2 := mapPoint(float64(b.x+nodeW/2), float64(b.y+nodeH/2))
		iup.DrawLine(ih, x1, y1, x2, y2)
	}
	for i := range nodes {
		n := &nodes[i]
		x1, y1 := mapPoint(float64(n.x), float64(n.y))
		x2, y2 := mapPoint(float64(n.x+nodeW), float64(n.y+nodeH))
		ih.SetAttributes("DRAWSTYLE=FILL, DRAWCOLOR=\"" + pal.strokes[n.color] + "\"")
		iup.DrawRectangle(ih, x1, y1, x2, y2)
	}
	vx1, vy1 := screenToWorld(0, 0)
	vx2, vy2 := screenToWorld(width, height)
	rx1, ry1 := mapPoint(vx1, vy1)
	rx2, ry2 := mapPoint(vx2, vy2)
	iup.DrawSave(ih)
	iup.DrawSetClipRoundedRect(ih, x, y, x+w, y+h, 8)
	ih.SetAttributes("DRAWSTYLE=STROKE, DRAWLINEWIDTH=2, DRAWCOLOR=\"" + pal.selection + "\"")
	iup.DrawRectangle(ih, rx1, ry1, rx2, ry2)
	iup.DrawRestore(ih)
}

func minimapRect(width, height int) (x, y, w, h int) {
	w, h = nodeW, nodeH+32
	return width - w - 14, height - h - 14, w, h
}

func buttonEvent(ih iup.Ihandle, button, pressed, x, y int, status string) int {
	if button != iup.BUTTON1 {
		return iup.DEFAULT
	}
	iup.SetFocus(ih)
	if pressed == 0 {
		finishPointer(x, y)
		return iup.DEFAULT
	}

	w, h := drawSize()
	mx, my, mw, mh := minimapRect(w, h)
	if x >= mx && x <= mx+mw && y >= my && y <= my+mh {
		centerFromMinimap(x, y, mx, my, mw, mh)
		return iup.DEFAULT
	}

	wx, wy := screenToWorld(x, y)
	found := nodeAt(wx, wy)
	if found >= 0 && portHit(&nodes[found], wx, wy) {
		selected = nodes[found].id
		connectFrom = selected
		connectX, connectY = wx, wy
		syncInspector()
		setStatus("Release over another node to connect")
		iup.Update(ih)
		return iup.DEFAULT
	}
	if found >= 0 {
		selected = nodes[found].id
		if iup.IsDouble(status) {
			beginEdit()
		} else {
			press = pointer{x: wx - float64(nodes[found].x), y: wy - float64(nodes[found].y), node: selected, dragging: true}
		}
		syncInspector()
		setStatus(nodeSummary(&nodes[found]))
	} else {
		selected = -1
		cancelEdit(false)
		press = pointer{x: float64(x), y: float64(y), panX: panX, panY: panY, panning: true}
		syncInspector()
		setStatus("Drag the canvas to pan")
	}
	iup.Update(ih)
	return iup.DEFAULT
}

func finishPointer(x, y int) {
	if connectFrom >= 0 {
		wx, wy := screenToWorld(x, y)
		if at := nodeAt(wx, wy); at >= 0 && nodes[at].id != connectFrom {
			addEdge(connectFrom, nodes[at].id)
			selected = nodes[at].id
			setStatus("Connected " + nodeByID(connectFrom).title + " to " + nodes[at].title)
		}
		connectFrom = -1
	}
	press = pointer{}
	canvas.SetAttribute("CURSOR", "ARROW")
	syncInspector()
	iup.Update(canvas)
}

func motion(ih iup.Ihandle, x, y int, status string) int {
	wx, wy := screenToWorld(x, y)
	if connectFrom >= 0 {
		connectX, connectY = wx, wy
		iup.Update(ih)
		return iup.DEFAULT
	}
	if press.dragging && iup.IsButton1(status) {
		if n := nodeByID(press.node); n != nil {
			n.x = snap(int(math.Round(wx - press.x)))
			n.y = snap(int(math.Round(wy - press.y)))
			updatePosition(n)
			setStatus(nodeSummary(n))
		}
		ih.SetAttribute("CURSOR", "MOVE")
		iup.Update(ih)
		return iup.DEFAULT
	}
	if press.panning && iup.IsButton1(status) {
		fitted = false
		panX = press.panX + float64(x) - press.x
		panY = press.panY + float64(y) - press.y
		ih.SetAttribute("CURSOR", "MOVE")
		iup.Update(ih)
		return iup.DEFAULT
	}
	if found := nodeAt(wx, wy); found >= 0 {
		hovered = nodes[found].id
		if portHit(&nodes[found], wx, wy) {
			ih.SetAttribute("CURSOR", "CROSS")
		} else {
			ih.SetAttribute("CURSOR", "HAND")
		}
	} else {
		hovered = -1
		ih.SetAttribute("CURSOR", "ARROW")
	}
	iup.Update(ih)
	return iup.DEFAULT
}

func wheel(ih iup.Ihandle, delta float64, x, y int, status string) int {
	factor := 0.82
	if delta > 0 {
		factor = 1.22
	}
	zoomAt(factor, x, y)
	return iup.DEFAULT
}

var gestureZoom float64

func gesture(ih iup.Ihandle, gesture, state, x, y int, v1, v2 float64) int {
	if gesture != iup.GESTURE_PINCH {
		return iup.DEFAULT
	}
	switch state {
	case iup.GESTURE_BEGIN:
		gestureZoom = zoom
	case iup.GESTURE_CHANGED:
		if gestureZoom > 0 && v1 > 0 {
			setZoom(gestureZoom*v1, x, y)
		}
	}
	return iup.DEFAULT
}

func key(ih iup.Ihandle, code int) int {
	if editing {
		switch code {
		case iup.K_CR:
			commitEdit()
		case iup.K_ESC:
			cancelEdit(true)
		case iup.K_BS:
			runes := []rune(editValue)
			if len(runes) > 0 {
				editValue = string(runes[:len(runes)-1])
				iup.Update(canvas)
			}
		default:
			return iup.CONTINUE
		}
		return iup.IGNORE
	}

	switch code {
	case iup.K_DEL:
		remove()
	case iup.K_F2:
		beginEdit()
	case iup.K_ESC:
		connectFrom = -1
		setStatus("Connection cancelled")
		iup.Update(canvas)
	case iup.K_LEFT:
		nudge(-8, 0)
	case iup.K_RIGHT:
		nudge(8, 0)
	case iup.K_UP:
		nudge(0, -8)
	case iup.K_DOWN:
		nudge(0, 8)
	case iup.K_plus, iup.K_equal:
		zoomAt(1.25, -1, -1)
	case iup.K_minus:
		zoomAt(0.8, -1, -1)
	case iup.XKeyCtrl(iup.K_c), iup.XKeyCtrl(iup.K_C):
		copyNode()
	case iup.XKeyCtrl(iup.K_v), iup.XKeyCtrl(iup.K_V):
		pasteNode()
	default:
		return iup.CONTINUE
	}
	return iup.IGNORE
}

func textInput(ih iup.Ihandle, text string) int {
	if !editing {
		return iup.DEFAULT
	}
	editValue += text
	iup.Update(ih)
	return iup.IGNORE
}

func resize(ih iup.Ihandle, width, height int) int {
	if fitted && width > 1 && height > 1 {
		fit()
	}
	return iup.DEFAULT
}

func addNode(shape int) {
	w, h := drawSize()
	wx, wy := screenToWorld(w/2, h/2)
	n := node{nextID, "New " + strings.ToLower(shapeNames[shape]), "Double click to rename", shape, (nextID - 1) % len(colorNames), snap(int(wx) - nodeW/2), snap(int(wy) - nodeH/2)}
	nextID++
	nodes = append(nodes, n)
	selected = n.id
	syncInspector()
	setStatus("Added " + n.title)
	iup.Update(canvas)
}

func connect() {
	if selectedNode() == nil {
		setStatus("Select a source node first")
		return
	}
	connectFrom = selected
	n := selectedNode()
	connectX, connectY = float64(n.x+nodeW+80), float64(n.y+nodeH/2)
	setStatus("Choose a target node")
	iup.Update(canvas)
}

func addEdge(from, to int) {
	for _, e := range edges {
		if e.from == from && e.to == to {
			return
		}
	}
	edges = append(edges, edge{from, to})
}

func duplicate() {
	n := selectedNode()
	if n == nil {
		return
	}
	clone := *n
	clone.id = nextID
	nextID++
	clone.title += " copy"
	clone.x += 32
	clone.y += 32
	nodes = append(nodes, clone)
	selected = clone.id
	syncInspector()
	setStatus("Duplicated " + n.title)
	iup.Update(canvas)
}

func remove() {
	if selected < 0 {
		return
	}
	name := "node"
	if n := selectedNode(); n != nil {
		name = n.title
	}
	kept := nodes[:0]
	for _, n := range nodes {
		if n.id != selected {
			kept = append(kept, n)
		}
	}
	nodes = kept
	keptEdges := edges[:0]
	for _, e := range edges {
		if e.from != selected && e.to != selected {
			keptEdges = append(keptEdges, e)
		}
	}
	edges = keptEdges
	selected, connectFrom = -1, -1
	cancelEdit(false)
	syncInspector()
	setStatus("Removed " + name)
	iup.Update(canvas)
}

func nudge(dx, dy int) {
	if n := selectedNode(); n != nil {
		n.x += dx
		n.y += dy
		updatePosition(n)
		iup.Update(canvas)
	}
}

func beginEdit() {
	n := selectedNode()
	if n == nil {
		return
	}
	editing = true
	editOriginal, editValue = n.title, n.title
	setStatus("Type a title, then press Enter")
	iup.Update(canvas)
}

func commitEdit() {
	if n := selectedNode(); n != nil && strings.TrimSpace(editValue) != "" {
		n.title = strings.TrimSpace(editValue)
	}
	editing = false
	syncInspector()
	setStatus("Title updated")
	iup.Update(canvas)
}

func cancelEdit(redraw bool) {
	if editing {
		editValue = editOriginal
	}
	editing = false
	if redraw {
		iup.Update(canvas)
	}
}

func copyNode() {
	n := selectedNode()
	if n == nil {
		return
	}
	clip := iup.Clipboard()
	clip.SetAttribute("TEXT", fmt.Sprintf("IUP-DIAGRAM\t%d\t%d\t%s\t%s", n.shape, n.color, clean(n.title), clean(n.detail)))
	iup.Destroy(clip)
	setStatus("Copied " + n.title)
}

func pasteNode() {
	clip := iup.Clipboard()
	text := clip.GetAttribute("TEXT")
	iup.Destroy(clip)
	parts := strings.Split(text, "\t")
	if len(parts) != 5 || parts[0] != "IUP-DIAGRAM" {
		setStatus("The clipboard does not contain a diagram node")
		return
	}
	shape, _ := strconv.Atoi(parts[1])
	color, _ := strconv.Atoi(parts[2])
	w, h := drawSize()
	wx, wy := screenToWorld(w/2, h/2)
	n := node{nextID, parts[3], parts[4], clamp(shape, 0, 2), clamp(color, 0, len(colorNames)-1), snap(int(wx) - nodeW/2), snap(int(wy) - nodeH/2)}
	nextID++
	nodes = append(nodes, n)
	selected = n.id
	syncInspector()
	setStatus("Pasted " + n.title)
	iup.Update(canvas)
}

func clean(s string) string {
	return strings.NewReplacer("\t", " ", "\n", " ", "\r", " ").Replace(s)
}

func exportSVG() {
	dlg := iup.FileDlg().SetAttributes("DIALOGTYPE=SAVE, TITLE=\"Export Diagram\", FILTER=*.svg, EXTDEFAULT=svg")
	dlg.SetAttributeHandle("PARENTDIALOG", iup.GetHandle("diagram_dlg"))
	iup.Popup(dlg, iup.CENTERPARENT, iup.CENTERPARENT)
	if dlg.GetInt("STATUS") != -1 {
		path := dlg.GetAttribute("VALUE")
		if !strings.HasSuffix(strings.ToLower(path), ".svg") {
			path += ".svg"
		}
		if svg := iup.DrawGetSvg(canvas); svg != "" {
			if err := os.WriteFile(path, []byte(svg), 0o644); err != nil {
				iup.Message("Export Diagram", err.Error())
			} else {
				setStatus("Exported " + path)
			}
		}
	}
	iup.Destroy(dlg)
}

func toggleTheme() {
	if iup.GetGlobalBool("DARKMODE") {
		iup.SetGlobal("APPEARANCE", "LIGHT")
	} else {
		iup.SetGlobal("APPEARANCE", "DARK")
	}
}

func zoomAt(factor float64, x, y int) {
	setZoom(zoom*factor, x, y)
}

func setZoom(value float64, x, y int) {
	value = math.Max(0.3, math.Min(value, 3.5))
	w, h := drawSize()
	if x < 0 {
		x, y = w/2, h/2
	}
	wx, wy := screenToWorld(x, y)
	zoom = value
	panX = float64(x) - wx*zoom
	panY = float64(y) - wy*zoom
	fitted = false
	iup.Update(canvas)
	setStatus(fmt.Sprintf("Zoom %.0f%%", zoom*100))
}

func fit() {
	if len(nodes) == 0 || canvas == 0 {
		return
	}
	w, h := drawSize()
	if w <= 1 || h <= 1 {
		return
	}
	minX, minY, maxX, maxY := sceneBounds()
	padding := 80.0
	zx := (float64(w) - padding) / float64(maxX-minX)
	zy := (float64(h) - padding) / float64(maxY-minY)
	zoom = math.Max(0.3, math.Min(1.35, math.Min(zx, zy)))
	panX = (float64(w)-float64(maxX-minX)*zoom)/2 - float64(minX)*zoom
	panY = (float64(h)-float64(maxY-minY)*zoom)/2 - float64(minY)*zoom
	fitted = true
	iup.Update(canvas)
	setStatus(fmt.Sprintf("Fit diagram at %.0f%%", zoom*100))
}

func centerFromMinimap(x, y, mx, my, mw, mh int) {
	minX, minY, maxX, maxY := sceneBounds()
	wx := float64(minX) + float64(x-mx)/float64(mw)*float64(maxX-minX)
	wy := float64(minY) + float64(y-my)/float64(mh)*float64(maxY-minY)
	w, h := drawSize()
	panX = float64(w)/2 - wx*zoom
	panY = float64(h)/2 - wy*zoom
	fitted = false
	iup.Update(canvas)
}

func sceneBounds() (minX, minY, maxX, maxY int) {
	if len(nodes) == 0 {
		return 0, 0, 1, 1
	}
	minX, minY = nodes[0].x, nodes[0].y
	maxX, maxY = nodes[0].x+nodeW, nodes[0].y+nodeH
	for _, n := range nodes[1:] {
		minX, minY = min(minX, n.x), min(minY, n.y)
		maxX, maxY = max(maxX, n.x+nodeW), max(maxY, n.y+nodeH)
	}
	minX, minY = minX-40, minY-40
	maxX, maxY = maxX+40, maxY+40
	return
}

func screenToWorld(x, y int) (float64, float64) {
	return (float64(x) - panX) / zoom, (float64(y) - panY) / zoom
}

func nodeAt(x, y float64) int {
	for i := len(nodes) - 1; i >= 0; i-- {
		n := &nodes[i]
		if x < float64(n.x) || x > float64(n.x+nodeW) || y < float64(n.y) || y > float64(n.y+nodeH) {
			continue
		}
		iup.DrawPathClear(hitPath)
		nodeShape(hitPath, n, 0, 0)
		if iup.DrawPathContains(hitPath, int(x), int(y), iup.DRAW_RULE_WINDING) {
			return i
		}
	}
	return -1
}

func portHit(n *node, x, y float64) bool {
	px, py := outputPort(n)
	return math.Hypot(x-float64(px), y-float64(py)) <= 12/zoom
}

func nodeByID(id int) *node {
	for i := range nodes {
		if nodes[i].id == id {
			return &nodes[i]
		}
	}
	return nil
}

func selectedNode() *node {
	return nodeByID(selected)
}

func syncInspector() {
	if iup.GetHandle("diagram_name") == 0 {
		return
	}
	syncing = true
	n := selectedNode()
	active := "NO"
	name, detail, shape, color, position := "", "", 0, 0, "No selection"
	if n != nil {
		active = "YES"
		name, detail, shape, color = n.title, n.detail, n.shape, n.color
		position = fmt.Sprintf("Position  %d, %d", n.x, n.y)
	}
	iup.GetHandle("diagram_name").SetAttributes(map[string]string{"VALUE": name, "ACTIVE": active})
	iup.GetHandle("diagram_detail").SetAttributes(map[string]string{"VALUE": detail, "ACTIVE": active})
	iup.GetHandle("diagram_shape").SetAttributes(map[string]string{"VALUE": strconv.Itoa(shape + 1), "ACTIVE": active})
	iup.GetHandle("diagram_color").SetAttributes(map[string]string{"VALUE": strconv.Itoa(color + 1), "ACTIVE": active})
	iup.GetHandle("diagram_position").SetAttribute("TITLE", position)
	syncing = false
}

func updatePosition(n *node) {
	if n.id == selected {
		iup.GetHandle("diagram_position").SetAttribute("TITLE", fmt.Sprintf("Position  %d, %d", n.x, n.y))
	}
}

func nodeSummary(n *node) string {
	return fmt.Sprintf("%s  |  %s  |  %d, %d", n.title, shapeNames[n.shape], n.x, n.y)
}

func drawSize() (int, int) {
	_, w, h := iup.GetInt2(canvas, "DRAWSIZE")
	return w, h
}

func snap(v int) int {
	const grid = 8
	return int(math.Round(float64(v)/grid)) * grid
}

func setStatus(text string) {
	if status := iup.GetHandle("diagram_status"); status != 0 {
		status.SetAttribute("TITLE", text)
	}
}

func setPalette() {
	bg := global("TXTBGCOLOR", "250 250 252")
	panel := global("DLGBGCOLOR", "240 240 242")
	text := global("TXTFGCOLOR", "30 32 38")
	dark := iup.GetGlobalBool("DARKMODE")
	weight := 0.18
	if dark {
		weight = 0.34
	}
	accents := []string{"62 120 224", "132 92 210", "220 148 42", "210 82 116", "36 156 154", "66 158 92"}
	pal = colors{
		bg: bg, panel: panel, text: text,
		grid: mix(bg, text, 0.13), dim: mix(text, bg, 0.42), line: mix(text, bg, 0.35),
		selection: global("ACCENTCOLOR", accents[0]),
	}
	for _, accent := range accents {
		pal.fills = append(pal.fills, mix(bg, accent, weight))
		pal.strokes = append(pal.strokes, mix(accent, text, 0.12))
	}
}

func global(name, fallback string) string {
	if value := iup.GetGlobal(name); value != "" {
		return value
	}
	return fallback
}

func mix(a, b string, amount float64) string {
	ar, ag, ab := rgb(a)
	br, bg, bb := rgb(b)
	return fmt.Sprintf("%d %d %d", int(ar+(br-ar)*amount), int(ag+(bg-ag)*amount), int(ab+(bb-ab)*amount))
}

func rgb(value string) (float64, float64, float64) {
	fields := strings.Fields(value)
	if len(fields) < 3 {
		return 0, 0, 0
	}
	r, _ := strconv.ParseFloat(fields[0], 64)
	g, _ := strconv.ParseFloat(fields[1], 64)
	b, _ := strconv.ParseFloat(fields[2], 64)
	return r, g, b
}

func clamp(value, low, high int) int {
	return max(low, min(value, high))
}

func abs(value int) int {
	if value < 0 {
		return -value
	}
	return value
}
