package main

import (
	"fmt"
	"image"
	"image/color"
	"image/png"
	"os"
	"path/filepath"

	"github.com/gen2brain/iup-go/iup"
)

const (
	spriteSize   = 16
	paletteSize  = 16
	transparent  = -1
	historyLimit = 64
)

type tool int

const (
	pencil tool = iota
	eraser
	fill
	picker
)

type snapshot struct {
	pixels  [spriteSize * spriteSize]int8
	palette [paletteSize]color.RGBA
}

type editor struct {
	snapshot
	undo, redo []snapshot
	before     *snapshot
	tool       tool
	primary    int
	secondary  int
	zoom       int
	grid       bool
	drawing    bool
	drawButton int
	lastX      int
	lastY      int
	canvas     iup.Ihandle
	bar        iup.Ihandle
	browser    iup.Ihandle
	preview    iup.Ihandle
	status     iup.Ihandle
	selected   iup.Ihandle
	dialog     iup.Ihandle
}

var palette = [paletteSize]color.RGBA{
	{28, 32, 48, 255}, {224, 68, 92, 255}, {255, 146, 164, 255}, {255, 208, 184, 255},
	{240, 154, 72, 255}, {249, 211, 95, 255}, {158, 206, 99, 255}, {55, 165, 130, 255},
	{68, 170, 216, 255}, {75, 111, 197, 255}, {133, 103, 202, 255}, {198, 107, 177, 255},
	{113, 91, 94, 255}, {151, 156, 170, 255}, {207, 210, 216, 255}, {255, 255, 255, 255},
}

var heart = [spriteSize]string{
	"................",
	"................",
	"................",
	".....rr...rr....",
	"....rrrr.rrrr...",
	"...rrrrrrrrrrr..",
	"...rrrpprrpprr..",
	"...rrrpprrpprr..",
	"....rrrrrrrrr...",
	".....rrrrrrr....",
	"......rrrrr.....",
	".......rrr......",
	"........r.......",
	"................",
	"................",
	"................",
}

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()
	iup.SetGlobal("UTF8MODE", "YES")

	app := &editor{tool: pencil, primary: 1, secondary: 15, zoom: 14, grid: true}
	app.palette = palette
	for i := range app.pixels {
		app.pixels[i] = transparent
	}
	for y, row := range heart {
		for x, pixel := range row {
			switch pixel {
			case 'r':
				app.pixels[y*spriteSize+x] = 1
			case 'p':
				app.pixels[y*spriteSize+x] = 2
			}
		}
	}

	app.build()
	app.refreshPreview()
	app.selectColor()
	if phone() {
		app.setStatus("Tap to draw. Swatches recolor the sprite.")
	} else {
		app.setStatus("Draw on the sprite. Select a swatch to recolor the art.")
	}
	iup.Show(app.dialog)
	iup.MainLoop()
}

func phone() bool {
	driver := iup.GetGlobal("DRIVER")
	return driver == "Android" || driver == "CocoaTouch"
}

func (app *editor) build() {
	app.canvas = iup.Canvas().SetAttributes("BORDER=NO, CANFOCUS=YES, EXPAND=YES, TOUCH=YES")
	app.canvas.SetAttribute("RASTERSIZE", fmt.Sprintf("%dx%d", spriteSize*18, spriteSize*18))
	app.canvas.SetCallback("ACTION", iup.ActionFunc(app.draw))
	app.canvas.SetCallback("BUTTON_CB", iup.ButtonFunc(app.button))
	app.canvas.SetCallback("MOTION_CB", iup.MotionFunc(app.motion))
	app.canvas.SetCallback("WHEEL_CB", iup.WheelFunc(app.wheel))
	app.canvas.SetCallback("K_ANY", iup.KAnyFunc(app.key))
	app.canvas.SetCallback("RESIZE_CB", iup.ResizeFunc(func(iup.Ihandle, int, int) int {
		app.updateZoom()
		return iup.DEFAULT
	}))

	app.bar = iup.ColorBar().SetAttributes("ORIENTATION=HORIZONTAL, NUM_CELLS=16, SHOW_SECONDARY=YES, SQUARED=NO, EXPAND=HORIZONTAL")
	app.bar.SetAttribute("RASTERSIZE", "272x32")
	app.bar.SetAttribute("PREVIEW_SIZE", "40")
	app.bar.SetAttribute("PRIMARY_CELL", app.primary)
	app.bar.SetAttribute("SECONDARY_CELL", app.secondary)
	for i, c := range app.palette {
		app.bar.SetAttributeId("CELL", i, rgb(c))
	}
	app.bar.SetCallback("SELECT_CB", iup.SelectFunc(func(_ iup.Ihandle, cell, kind int) int {
		if kind == iup.PRIMARY {
			app.primary = cell
			app.selectColor()
		} else {
			app.secondary = cell
			app.showColors()
		}
		return iup.DEFAULT
	}))
	app.bar.SetCallback("SWITCH_CB", iup.SwitchFunc(func(_ iup.Ihandle, primary, secondary int) int {
		app.primary, app.secondary = secondary, primary
		app.selectColor()
		return iup.DEFAULT
	}))

	app.browser = iup.ColorBrowser()
	app.browser.SetCallback("DRAG_CB", iup.DragFunc(func(_ iup.Ihandle, r, g, b uint8) int {
		app.changeColor(color.RGBA{r, g, b, 255})
		return iup.DEFAULT
	}))
	app.browser.SetCallback("CHANGE_CB", iup.ChangeFunc(func(_ iup.Ihandle, r, g, b uint8) int {
		app.changeColor(color.RGBA{r, g, b, 255})
		app.finish()
		return iup.DEFAULT
	}))

	app.preview = iup.Canvas().SetAttributes("BORDER=NO, CANFOCUS=NO, EXPAND=NO")
	app.preview.SetAttribute("RASTERSIZE", fmt.Sprintf("%dx%d", spriteSize*4, spriteSize*4))
	app.preview.SetCallback("ACTION", iup.ActionFunc(app.drawPreview))
	app.selected = iup.Label("").SetAttribute("EXPAND", "HORIZONTAL")
	app.status = iup.Label("").SetAttributes("EXPAND=HORIZONTAL, PADDING=6x4, WORDWRAP=YES")

	tools := iup.Hbox(
		app.action("Pencil", func() { app.choose(pencil) }),
		app.action("Erase", func() { app.choose(eraser) }),
		app.action("Fill", func() { app.choose(fill) }),
		app.action("Pick", func() { app.choose(picker) }),
	).SetAttributes("NGAP=5, ALIGNMENT=ACENTER")
	commands := []iup.Ihandle{
		app.action("Undo", app.undoStep),
		app.action("Redo", app.redoStep),
		app.action("-", func() { app.changeZoom(-2) }),
		app.action("+", func() { app.changeZoom(2) }),
		app.action("Save PNG", app.savePNG),
	}
	grid := iup.Toggle("Grid").SetAttribute("VALUE", "ON")
	grid.SetCallback("ACTION", iup.ToggleActionFunc(func(_ iup.Ihandle, state int) int {
		app.grid = state == 1
		iup.Update(app.canvas)
		return iup.DEFAULT
	}))

	zoomRow := []iup.Ihandle{grid, iup.Fill()}
	if phone() {
		zoomRow = append(zoomRow, commands[2], commands[3])
	}
	zoomRow = append(zoomRow, iup.Label("").SetHandle("pixelart_zoom"))
	art := iup.Vbox(app.canvas, iup.Hbox(zoomRow...).SetAttributes("NGAP=6, ALIGNMENT=ACENTER")).
		SetAttributes("EXPAND=YES, NGAP=5, TABTITLE=Artwork")
	colorHelp := "Left click: primary    Right click: secondary"
	if phone() {
		colorHelp = "Tap: primary    Long press: secondary"
	}
	colors := iup.Vbox(
		iup.Label("Palette").SetAttribute("FONTSTYLE", "Bold"),
		app.bar,
		app.selected,
		app.browser,
		iup.Hbox(iup.Label("Preview"), app.preview).SetAttributes("NGAP=8, ALIGNMENT=ACENTER"),
		iup.Label(colorHelp).SetAttribute("WORDWRAP", "YES"),
	).SetAttributes("NGAP=7, TABTITLE=Colors")

	var center iup.Ihandle
	var controls iup.Ihandle
	if phone() {
		center = iup.Tabs(art, colors).SetAttribute("EXPAND", "YES")
		controls = iup.Vbox(tools, iup.Hbox(commands[0], commands[1], commands[4]).SetAttributes("NGAP=5")).SetAttributes("NGAP=5")
	} else {
		center = iup.Hbox(art, colors).SetAttributes("NGAP=12, EXPAND=YES")
		controls = iup.Hbox(tools, iup.Fill(), iup.Hbox(commands...).SetAttributes("NGAP=5")).SetAttributes("NGAP=8")
	}
	app.dialog = iup.Dialog(iup.Vbox(controls, center, app.status).SetAttributes("NMARGIN=10x10, NGAP=8"))
	app.dialog.SetAttribute("TITLE", "Pixel Art Studio")
	app.updateZoom()
}

func (app *editor) action(title string, run func()) iup.Ihandle {
	return iup.Button(title).SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		run()
		return iup.DEFAULT
	}))
}

func (app *editor) setStatus(text string) { app.status.SetAttribute("TITLE", text) }

func rgb(c color.RGBA) string { return fmt.Sprintf("%d %d %d", c.R, c.G, c.B) }

func (app *editor) selectColor() {
	app.browser.SetAttribute("RGB", rgb(app.palette[app.primary]))
	app.showColors()
}

func hex(c color.RGBA) string { return fmt.Sprintf("#%02X%02X%02X", c.R, c.G, c.B) }

func (app *editor) showColors() {
	app.selected.SetAttribute("TITLE", fmt.Sprintf("Primary %s    Secondary %s", hex(app.palette[app.primary]), hex(app.palette[app.secondary])))
}

func (app *editor) changeColor(c color.RGBA) {
	if app.palette[app.primary] == c {
		return
	}
	app.begin()
	app.palette[app.primary] = c
	app.bar.SetAttributeId("CELL", app.primary, rgb(c))
	app.selectColor()
	iup.Update(app.canvas)
	app.refreshPreview()
}

func (app *editor) begin() {
	if app.before == nil {
		original := app.snapshot
		app.before = &original
	}
}

func (app *editor) finish() {
	if app.before == nil {
		return
	}
	if *app.before != app.snapshot {
		app.undo = append(app.undo, *app.before)
		if len(app.undo) > historyLimit {
			app.undo = app.undo[1:]
		}
		app.redo = nil
	}
	app.before = nil
	app.refreshPreview()
}

func (app *editor) restore(s snapshot) {
	app.snapshot = s
	for i, c := range app.palette {
		app.bar.SetAttributeId("CELL", i, rgb(c))
	}
	app.selectColor()
	app.refreshPreview()
	iup.Update(app.canvas)
}

func (app *editor) undoStep() {
	app.finish()
	if len(app.undo) == 0 {
		return
	}
	app.redo = append(app.redo, app.snapshot)
	last := len(app.undo) - 1
	app.restore(app.undo[last])
	app.undo = app.undo[:last]
	app.setStatus("Undid edit")
}

func (app *editor) redoStep() {
	app.finish()
	if len(app.redo) == 0 {
		return
	}
	app.undo = append(app.undo, app.snapshot)
	last := len(app.redo) - 1
	app.restore(app.redo[last])
	app.redo = app.redo[:last]
	app.setStatus("Redid edit")
}

func (app *editor) choose(next tool) {
	app.finish()
	app.tool = next
	app.setStatus([]string{"Pencil", "Eraser", "Fill", "Eyedropper"}[next])
}

func (app *editor) geometry(w, h int) (x, y, cell int) {
	cell = app.zoom
	if cell > w/spriteSize {
		cell = w / spriteSize
	}
	if cell > h/spriteSize {
		cell = h / spriteSize
	}
	if cell < 1 {
		cell = 1
	}
	return (w - spriteSize*cell) / 2, (h - spriteSize*cell) / 2, cell
}

func (app *editor) point(x, y int) (int, int, bool) {
	var w, h int
	fmt.Sscanf(app.canvas.GetAttribute("DRAWSIZE"), "%dx%d", &w, &h)
	ox, oy, cell := app.geometry(w, h)
	x, y = x-ox, y-oy
	if x < 0 || y < 0 || x >= spriteSize*cell || y >= spriteSize*cell {
		return 0, 0, false
	}
	return x / cell, y / cell, true
}

func (app *editor) draw(ih iup.Ihandle) int {
	iup.DrawBegin(ih)
	defer iup.DrawEnd(ih)
	w, h := iup.DrawGetSize(ih)
	ih.SetAttributes(`DRAWSTYLE=FILL, DRAWCOLOR="55 58 67"`)
	iup.DrawRectangle(ih, 0, 0, w-1, h-1)
	ox, oy, cell := app.geometry(w, h)
	for y := 0; y < spriteSize; y++ {
		for x := 0; x < spriteSize; x++ {
			index := app.pixels[y*spriteSize+x]
			shade := "224 224 224"
			if (x/2+y/2)%2 == 0 {
				shade = "255 255 255"
			}
			if index != transparent {
				shade = rgb(app.palette[index])
			}
			ih.SetAttribute("DRAWCOLOR", shade)
			x0, y0 := ox+x*cell, oy+y*cell
			iup.DrawRectangle(ih, x0, y0, x0+cell-1, y0+cell-1)
		}
	}
	if app.grid && cell >= 6 {
		ih.SetAttributes(`DRAWSTYLE=STROKE, DRAWCOLOR="112 115 122 110"`)
		for n := 0; n <= spriteSize; n++ {
			iup.DrawLine(ih, ox+n*cell, oy, ox+n*cell, oy+spriteSize*cell)
			iup.DrawLine(ih, ox, oy+n*cell, ox+spriteSize*cell, oy+n*cell)
		}
	}
	return iup.DEFAULT
}

func (app *editor) drawPreview(ih iup.Ihandle) int {
	iup.DrawBegin(ih)
	defer iup.DrawEnd(ih)
	w, h := iup.DrawGetSize(ih)
	cell := min(w, h) / spriteSize
	if cell < 1 {
		return iup.DEFAULT
	}
	ox, oy := (w-spriteSize*cell)/2, (h-spriteSize*cell)/2
	ih.SetAttribute("DRAWSTYLE", "FILL")
	for y := 0; y < spriteSize; y++ {
		for x := 0; x < spriteSize; x++ {
			shade := "224 224 224"
			if (x/2+y/2)%2 == 0 {
				shade = "255 255 255"
			}
			if index := app.pixels[y*spriteSize+x]; index != transparent {
				shade = rgb(app.palette[index])
			}
			ih.SetAttribute("DRAWCOLOR", shade)
			x0, y0 := ox+x*cell, oy+y*cell
			iup.DrawRectangle(ih, x0, y0, x0+cell-1, y0+cell-1)
		}
	}
	return iup.DEFAULT
}

func (app *editor) paint(x, y, button int) {
	index := y*spriteSize + x
	value := int8(app.primary)
	if button == iup.BUTTON3 {
		value = int8(app.secondary)
	}
	if app.tool == eraser {
		value = transparent
	}
	if app.pixels[index] != value {
		app.begin()
		app.pixels[index] = value
		iup.Update(app.canvas)
	}
}

func (app *editor) stroke(x0, y0, x1, y1, button int) {
	dx, dy := abs(x1-x0), -abs(y1-y0)
	sx, sy := -1, -1
	if x0 < x1 {
		sx = 1
	}
	if y0 < y1 {
		sy = 1
	}
	err := dx + dy
	for {
		app.paint(x0, y0, button)
		if x0 == x1 && y0 == y1 {
			break
		}
		double := 2 * err
		if double >= dy {
			err += dy
			x0 += sx
		}
		if double <= dx {
			err += dx
			y0 += sy
		}
	}
}

func abs(n int) int {
	if n < 0 {
		return -n
	}
	return n
}

func (app *editor) flood(x, y, button int) {
	old := app.pixels[y*spriteSize+x]
	value := int8(app.primary)
	if button == iup.BUTTON3 {
		value = int8(app.secondary)
	}
	if old == value {
		return
	}
	app.begin()
	queue := []int{y*spriteSize + x}
	app.pixels[queue[0]] = value
	for len(queue) > 0 {
		index := queue[0]
		queue = queue[1:]
		px, py := index%spriteSize, index/spriteSize
		for _, next := range [][2]int{{px - 1, py}, {px + 1, py}, {px, py - 1}, {px, py + 1}} {
			if next[0] < 0 || next[0] >= spriteSize || next[1] < 0 || next[1] >= spriteSize {
				continue
			}
			neighbor := next[1]*spriteSize + next[0]
			if app.pixels[neighbor] == old {
				app.pixels[neighbor] = value
				queue = append(queue, neighbor)
			}
		}
	}
	iup.Update(app.canvas)
}

func (app *editor) button(_ iup.Ihandle, button, pressed, x, y int, _ string) int {
	if button != iup.BUTTON1 && button != iup.BUTTON3 {
		return iup.DEFAULT
	}
	if pressed == 0 {
		if app.drawing {
			app.drawing = false
			app.finish()
		}
		return iup.DEFAULT
	}
	px, py, ok := app.point(x, y)
	if !ok {
		return iup.DEFAULT
	}
	iup.SetFocus(app.canvas)
	switch app.tool {
	case picker:
		if index := app.pixels[py*spriteSize+px]; index != transparent {
			if button == iup.BUTTON1 {
				app.primary = int(index)
				app.bar.SetAttribute("PRIMARY_CELL", app.primary)
				app.selectColor()
			} else {
				app.secondary = int(index)
				app.bar.SetAttribute("SECONDARY_CELL", app.secondary)
				app.showColors()
			}
		}
	case fill:
		app.flood(px, py, button)
		app.finish()
	default:
		app.drawing = true
		app.drawButton = button
		app.lastX, app.lastY = px, py
		app.paint(px, py, button)
	}
	return iup.DEFAULT
}

func (app *editor) motion(_ iup.Ihandle, x, y int, status string) int {
	if !app.drawing {
		return iup.DEFAULT
	}
	if app.drawButton == iup.BUTTON1 && !iup.IsButton1(status) || app.drawButton == iup.BUTTON3 && !iup.IsButton3(status) {
		return iup.DEFAULT
	}
	px, py, ok := app.point(x, y)
	if ok {
		app.stroke(app.lastX, app.lastY, px, py, app.drawButton)
		app.lastX, app.lastY = px, py
	}
	return iup.DEFAULT
}

func (app *editor) wheel(_ iup.Ihandle, delta float64, _, _ int, _ string) int {
	if delta > 0 {
		app.changeZoom(2)
	} else if delta < 0 {
		app.changeZoom(-2)
	}
	return iup.DEFAULT
}

func (app *editor) maxZoom() int {
	var w, h int
	fmt.Sscanf(app.canvas.GetAttribute("DRAWSIZE"), "%dx%d", &w, &h)
	if w <= 0 || h <= 0 {
		return 24
	}
	return max(6, min(24, min(w, h)/spriteSize))
}

func (app *editor) changeZoom(step int) {
	app.zoom = max(6, min(app.maxZoom(), app.zoom+step))
	app.updateZoom()
	iup.Update(app.canvas)
}

func (app *editor) updateZoom() {
	iup.GetHandle("pixelart_zoom").SetAttribute("TITLE", fmt.Sprintf("%dx", min(app.zoom, app.maxZoom())))
}

func (app *editor) key(_ iup.Ihandle, code int) int {
	if iup.IsCtrlXKey(code) {
		switch iup.XKeyBase(code) {
		case 'Z', 'z':
			app.undoStep()
			return iup.IGNORE
		case 'Y', 'y':
			app.redoStep()
			return iup.IGNORE
		}
	}
	return iup.DEFAULT
}

func (app *editor) sprite() *image.RGBA {
	img := image.NewRGBA(image.Rect(0, 0, spriteSize, spriteSize))
	for i, index := range app.pixels {
		if index != transparent {
			copy(img.Pix[i*4:i*4+4], []byte{app.palette[index].R, app.palette[index].G, app.palette[index].B, 255})
		}
	}
	return img
}

func (app *editor) refreshPreview() {
	iup.Update(app.preview)
}

func (app *editor) savePNG() {
	app.finish()
	dlg := iup.FileDlg().SetAttributes(`DIALOGTYPE=SAVE, TITLE="Export sprite", FILTER=*.png, EXTDEFAULT=png, FILE=sprite.png`)
	iup.SetAttributeHandle(dlg, "PARENTDIALOG", app.dialog)
	iup.Popup(dlg, iup.CENTERPARENT, iup.CENTERPARENT)
	defer dlg.Destroy()
	if dlg.GetInt("STATUS") == -1 {
		return
	}
	name := dlg.GetAttribute("VALUE")
	if name == "" {
		return
	}
	if browser() {
		if filepath.Ext(name) == "" {
			name += ".png"
		}
		img := iup.ImageFromImage(app.sprite())
		defer img.Destroy()
		if iup.ImageSave(img, name, "PNG") == 0 {
			app.setStatus("PNG download failed")
		} else {
			app.setStatus("Downloaded " + name)
		}
		return
	}
	file, err := os.Create(name)
	if err == nil {
		err = png.Encode(file, app.sprite())
		if closeErr := file.Close(); err == nil {
			err = closeErr
		}
	}
	if err != nil {
		app.setStatus("Export failed: " + err.Error())
	} else {
		app.setStatus("Saved " + name)
	}
}

func browser() bool {
	return iup.GetGlobal("DRIVER") == "WASM"
}
