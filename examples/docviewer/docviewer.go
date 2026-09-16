package main

import (
	"fmt"
	"image"
	"math"
	"os"
	"path/filepath"
	"strconv"

	"github.com/gen2brain/folio/doc"
	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

type entry struct {
	title    string
	page     int
	children []entry
}

type opened struct {
	path    string
	title   string
	kind    string
	pages   int
	outline []entry
	err     error
}

type rendered struct {
	page int
	zoom float64
	img  *image.RGBA
	err  error
}

type job struct {
	open   string
	page   int
	width  int
	zoom   float64
	serial int
}

var (
	jobs     = make(chan job, 64)
	serial   int
	pages    int
	current  int
	zoom     float64
	fitWidth = true
	pageImg  iup.Ihandle
	imgW     int
	imgH     int
	gotoEnd  bool
)

const pageMargin = 12

func main() {
	iup.Open()
	defer iup.Close()

	tree := iup.Tree().SetAttributes(`ADDROOT=NO, EXPAND=YES`).SetHandle("dv_tree")
	tree.SetCallback("SELECTION_CB", iup.SelectionFunc(treeSelection))

	canvas := iup.Canvas().SetAttributes(`SCROLLBAR=YES, EXPAND=YES, BORDER=NO, BGCOLOR="128 128 128"`).SetHandle("dv_canvas")
	canvas.SetCallback("ACTION", iup.ActionFunc(canvasAction))
	canvas.SetCallback("RESIZE_CB", iup.ResizeFunc(canvasResize))
	canvas.SetCallback("SCROLL_CB", iup.ScrollFunc(canvasScroll))
	canvas.SetCallback("WHEEL_CB", iup.WheelFunc(canvasWheel))
	canvas.SetCallback("K_ANY", iup.KAnyFunc(canvasKey))
	canvas.SetCallback("POSTMESSAGE_CB", iup.PostMessageFunc(workerResult))

	split := iup.Split(tree, canvas).SetAttributes("VALUE=250, AUTOHIDE=YES")

	pageText := iup.Text().SetAttributes("SPIN=YES, SPINMIN=1, SPINMAX=1, VISIBLECOLUMNS=4, ACTIVE=NO").SetHandle("dv_page")
	pageText.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		if n, err := strconv.Atoi(ih.GetAttribute("VALUE")); err == nil {
			showPage(n - 1)
		}
		return iup.DEFAULT
	}))
	pageCount := iup.Label("of 0").SetAttribute("EXPAND", "HORIZONTAL").SetHandle("dv_count")

	toolbar := iup.Hbox(
		iup.Button("Previous").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { showPage(current - 1); return iup.DEFAULT })),
		iup.Button("Next").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { showPage(current + 1); return iup.DEFAULT })),
		iup.Label("Page"),
		pageText,
		pageCount,
		iup.Button("Zoom Out").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { setZoom(0.8); return iup.DEFAULT })),
		iup.Button("Zoom In").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { setZoom(1.25); return iup.DEFAULT })),
		iup.Button("Fit Width").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { fitWidth = true; requestRender(); return iup.DEFAULT })),
	).SetAttributes("NGAP=4, ALIGNMENT=ACENTER")

	status := iup.Label("Open a PDF, EPUB, MOBI, CHM, FB2, DOCX or SVG file").SetAttribute("EXPAND", "HORIZONTAL").SetHandle("dv_status")

	dlg := iup.Dialog(iup.Vbox(toolbar, split, status).SetAttributes("NMARGIN=4x4, NGAP=4")).SetHandle("dv_dlg").SetAttributes(map[string]string{
		"TITLE":     "Document Viewer",
		"MENU":      "docviewer_menu",
		"PLACEMENT": "MAXIMIZED",
	})
	buildMenu()

	go worker(canvas)

	iup.Show(dlg)

	if len(os.Args) > 1 {
		openDocument(os.Args[1])
	}

	iup.MainLoop()
}

func buildMenu() {
	iup.Menu(
		iup.Submenu("&File", iup.Menu(
			iup.MenuItem("&Open...\tCtrl+O").SetCallback("ACTION", iup.ActionFunc(openAction)),
			iup.MenuItem("&Close\tCtrl+W").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { closeDocument(); return iup.DEFAULT })),
			iup.MenuSeparator(),
			iup.MenuItem("E&xit").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { return iup.CLOSE })),
		)),
		iup.Submenu("&View", iup.Menu(
			iup.MenuItem("Zoom &In\tCtrl++").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { setZoom(1.25); return iup.DEFAULT })),
			iup.MenuItem("Zoom &Out\tCtrl+-").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { setZoom(0.8); return iup.DEFAULT })),
			iup.MenuItem("&Fit Width").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { fitWidth = true; requestRender(); return iup.DEFAULT })),
		)),
		iup.Submenu("&Go", iup.Menu(
			iup.MenuItem("&Previous Page").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { showPage(current - 1); return iup.DEFAULT })),
			iup.MenuItem("&Next Page").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { showPage(current + 1); return iup.DEFAULT })),
			iup.MenuItem("&First Page").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { showPage(0); return iup.DEFAULT })),
			iup.MenuItem("&Last Page").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { showPage(pages - 1); return iup.DEFAULT })),
		)),
	).SetHandle("docviewer_menu")
}

func openAction(iup.Ihandle) int {
	fdlg := iup.FileDlg()
	defer fdlg.Destroy()
	fdlg.SetAttributes(map[string]string{
		"DIALOGTYPE": "OPEN",
		"TITLE":      "Open Document",
		"EXTFILTER":  "Documents|*.pdf;*.epub;*.mobi;*.azw3;*.chm;*.fb2;*.docx;*.pptx;*.xlsx;*.svg;*.html;*.txt|All Files|*.*|",
	})
	iup.SetAttributeHandle(fdlg, "PARENTDIALOG", iup.GetHandle("dv_dlg"))
	iup.Popup(fdlg, iup.CENTERPARENT, iup.CENTERPARENT)
	if fdlg.GetInt("STATUS") != -1 {
		openDocument(fdlg.GetAttribute("VALUE"))
	}
	return iup.DEFAULT
}

func openDocument(path string) {
	iup.GetHandle("dv_status").SetAttribute("TITLE", "Opening "+filepath.Base(path)+"...")
	serial++
	jobs <- job{open: path, serial: serial}
}

func closeDocument() {
	serial++
	jobs <- job{open: "", page: -1, serial: serial}
	resetView()
	iup.GetHandle("dv_dlg").SetAttribute("TITLE", "Document Viewer")
	iup.GetHandle("dv_status").SetAttribute("TITLE", "No document")
}

func resetView() {
	pages, current = 0, 0
	iup.GetHandle("dv_tree").SetAttribute("DELNODE", "ALL")
	iup.GetHandle("dv_page").SetAttributes("VALUE=1, SPINMAX=1, ACTIVE=NO")
	iup.GetHandle("dv_count").SetAttribute("TITLE", "of 0")
	setPageImage(nil)
}

func showPage(n int) {
	if pages == 0 {
		return
	}
	n = max(0, min(n, pages-1))
	if n == current && pageImg != 0 {
		return
	}
	current = n
	iup.GetHandle("dv_page").SetAttribute("VALUE", n+1)
	requestRender()
}

func setZoom(factor float64) {
	if pages == 0 {
		return
	}
	fitWidth = false
	zoom = math.Max(0.1, math.Min(zoom*factor, 8))
	requestRender()
}

func requestRender() {
	if pages == 0 {
		return
	}
	w, _ := canvasSize()
	z := zoom
	if fitWidth {
		z = 0
	}
	serial++
	jobs <- job{page: current, width: w - 2*pageMargin, zoom: z, serial: serial}
}

func canvasSize() (int, int) {
	var w, h int
	fmt.Sscanf(iup.GetHandle("dv_canvas").GetAttribute("DRAWSIZE"), "%dx%d", &w, &h)
	return w, h
}

func workerResult(ih iup.Ihandle, kind string, s int, p any) int {
	if s != serial {
		return iup.DEFAULT
	}

	canvas := iup.GetHandle("dv_canvas")
	dlg := iup.GetHandle("dv_dlg")
	status := iup.GetHandle("dv_status")
	switch kind {
	case "opened":
		o := p.(opened)
		resetView()
		if o.err != nil {
			status.SetAttribute("TITLE", "Cannot open "+filepath.Base(o.path))
			iup.MessageError(dlg, o.err.Error())
			return iup.DEFAULT
		}
		pages = o.pages
		fitWidth, zoom = true, 1
		title := o.title
		if title == "" {
			title = filepath.Base(o.path)
		}
		dlg.SetAttribute("TITLE", title+" - Document Viewer")
		iup.GetHandle("dv_page").SetAttributes(fmt.Sprintf("SPINMAX=%d, ACTIVE=YES, VALUE=1", max(pages, 1)))
		iup.GetHandle("dv_count").SetAttribute("TITLE", fmt.Sprintf("of %d", pages))
		status.SetAttribute("TITLE", fmt.Sprintf("%s, %s, %d pages", filepath.Base(o.path), o.kind, pages))
		fillOutline(o.outline)
		current = 0
		requestRender()
	case "page":
		r := p.(rendered)
		if r.err != nil {
			status.SetAttribute("TITLE", fmt.Sprintf("Page %d: %v", r.page+1, r.err))
			return iup.DEFAULT
		}
		zoom = r.zoom
		setPageImage(r.img)
		posy := 0
		if gotoEnd {
			posy = imgH
			gotoEnd = false
		}
		canvas.SetAttribute("POSY", posy)
		canvas.SetAttribute("POSX", 0)
		iup.Update(canvas)
	}
	return iup.DEFAULT
}

func setPageImage(img *image.RGBA) {
	if pageImg != 0 {
		pageImg.Destroy()
		pageImg = 0
	}
	imgW, imgH = 0, 0
	if img != nil {
		pageImg = iup.ImageFromImage(img).SetHandle("docviewer_page")
		imgW, imgH = img.Bounds().Dx(), img.Bounds().Dy()
	}
	updateScrollbars()
	iup.Update(iup.GetHandle("dv_canvas"))
}

func updateScrollbars() {
	w, h := canvasSize()
	contentW := max(imgW+2*pageMargin, w)
	contentH := max(imgH+2*pageMargin, h)
	iup.GetHandle("dv_canvas").SetAttributes(fmt.Sprintf("XMAX=%d, YMAX=%d, DX=%d, DY=%d", contentW, contentH, w, h))
}

func fillOutline(entries []entry) {
	addEntries(entries, -1, true)
}

func addEntries(entries []entry, ref int, first bool) {
	tree := iup.GetHandle("dv_tree")

	for _, e := range entries {
		kind := "LEAF"
		if len(e.children) > 0 {
			kind = "BRANCH"
		}
		op := "ADD"
		if !first {
			op = "INSERT"
		}
		tree.SetAttribute(fmt.Sprintf("%s%s%d", op, kind, ref), e.title)
		id := tree.GetInt("LASTADDNODE")
		iup.TreeSetUserId(tree, id, uintptr(e.page+1))
		if len(e.children) > 0 {
			addEntries(e.children, id, true)
			tree.SetAttribute(fmt.Sprintf("STATE%d", id), "COLLAPSED")
		}
		ref, first = id, false
	}
}

func treeSelection(ih iup.Ihandle, id, state int) int {
	if state == 1 {
		if p := int(iup.TreeGetUserId(ih, id)); p > 0 {
			showPage(p - 1)
		}
	}
	return iup.DEFAULT
}

func canvasAction(ih iup.Ihandle) int {
	iup.DrawBegin(ih)
	iup.DrawParentBackground(ih)
	w, h := iup.DrawGetSize(ih)
	ih.SetAttribute("DRAWCOLOR", ih.GetAttribute("BGCOLOR"))
	ih.SetAttribute("DRAWSTYLE", "FILL")
	iup.DrawRectangle(ih, 0, 0, w-1, h-1)
	if pageImg != 0 {
		x := max((w-imgW)/2, pageMargin) - int(iup.GetFloat(ih, "POSX"))
		y := pageMargin - int(iup.GetFloat(ih, "POSY"))
		if imgH+2*pageMargin < h {
			y = (h - imgH) / 2
		}
		iup.DrawImage(ih, "docviewer_page", x, y, -1, -1)
	}
	iup.DrawEnd(ih)
	return iup.DEFAULT
}

func canvasResize(ih iup.Ihandle, w, h int) int {
	updateScrollbars()
	if fitWidth {
		requestRender()
	}
	return iup.DEFAULT
}

func canvasScroll(ih iup.Ihandle, op int, posx, posy float64) int {
	iup.Update(ih)
	return iup.DEFAULT
}

func canvasWheel(ih iup.Ihandle, delta float64, x, y int, st string) int {
	if pages == 0 {
		return iup.DEFAULT
	}
	if iup.IsControl(st) {
		if delta > 0 {
			setZoom(1.25)
		} else if delta < 0 {
			setZoom(0.8)
		}
		return iup.DEFAULT
	}
	_, h := canvasSize()
	maxY := float64(max(imgH+2*pageMargin-h, 0))
	posy := float64(iup.GetFloat(ih, "POSY"))
	switch {
	case delta < 0 && posy >= maxY && current < pages-1:
		showPage(current + 1)
	case delta > 0 && posy <= 0 && current > 0:
		gotoEnd = true
		showPage(current - 1)
	default:
		ih.SetAttribute("POSY", math.Max(0, math.Min(posy-delta*float64(h)/8, maxY)))
		iup.Update(ih)
	}
	return iup.DEFAULT
}

func canvasKey(ih iup.Ihandle, c int) int {
	switch c {
	case iup.K_PGDN, iup.K_RIGHT, iup.K_SP:
		showPage(current + 1)
	case iup.K_PGUP, iup.K_LEFT:
		showPage(current - 1)
	case iup.K_HOME:
		showPage(0)
	case iup.K_END:
		showPage(pages - 1)
	default:
		return iup.CONTINUE
	}
	return iup.IGNORE
}

func worker(canvas iup.Ihandle) {
	var d doc.Document
	for j := range jobs {
		if j.open != "" || j.page < 0 {
			if d != nil {
				d.Close()
				d = nil
			}
			if j.open == "" {
				continue
			}
			o := opened{path: j.open}
			d, o.err = doc.Open(j.open)
			if o.err == nil {
				o.pages = d.NumPages()
				o.title = d.Metadata().Title
				o.kind = d.Kind().String()
				o.outline = outline(d)
			}
			iup.PostMessage(canvas, "opened", j.serial, o)
			continue
		}

		for len(jobs) > 0 {
			next := <-jobs
			if next.open != "" || next.page < 0 {
				jobs <- next
				break
			}
			j = next
		}
		if d == nil {
			continue
		}

		r := rendered{page: j.page}
		var page doc.Page
		page, r.err = d.Page(j.page)
		if r.err == nil {
			dpi, base := pageDPI(d, page, j)
			r.zoom = dpi / base
			r.img, r.err = page.ImageDPI(dpi)
		}
		iup.PostMessage(canvas, "page", j.serial, r)
	}
}

func pageDPI(d doc.Document, page doc.Page, j job) (float64, float64) {
	base := 72.0
	if d.Kind() == doc.KindSVG {
		base = 96
	}
	if j.zoom > 0 {
		return base * j.zoom, base
	}
	b := page.Bounds().Normalized()
	pw := float64(b.X1 - b.X0)
	if pw <= 0 || j.width <= 0 {
		return base, base
	}
	return base * float64(j.width) / pw, base
}

func outline(d doc.Document) []entry {
	out := entries(d.Outline())
	if len(out) == 0 {
		for i := range d.NumPages() {
			out = append(out, entry{title: fmt.Sprintf("Page %d", i+1), page: i})
		}
	}
	return out
}

func entries(items []doc.Outline) []entry {
	var out []entry
	for _, it := range items {
		out = append(out, entry{title: it.Title, page: it.Page, children: entries(it.Children)})
	}
	return out
}
