package main

import (
	"fmt"
	"image"
	"math"
	"os"
	"path/filepath"
	"slices"
	"strings"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

const title = "Image Viewer"

var extensions = []string{
	".png", ".jpg", ".jpeg", ".gif", ".bmp", ".tif", ".tiff", ".webp", ".svg",
	".ico", ".tga", ".heic", ".xpm", ".xbm", ".pbm", ".pgm", ".ppm",
}

var (
	files   []string
	current = -1
	imgName string
	img     iup.Ihandle
	imgW    int
	imgH    int
	zoom    = 1.0
	fit     = true

	dragging     bool
	dragX, dragY int
)

func main() {
	iup.Open()
	defer iup.Close()

	canvas := iup.Canvas().SetAttributes(`SCROLLBAR=YES, EXPAND=YES, BORDER=NO, BGCOLOR="64 64 64"`).SetHandle("iv_canvas")
	canvas.SetCallback("ACTION", iup.ActionFunc(canvasAction))
	canvas.SetCallback("RESIZE_CB", iup.ResizeFunc(func(iup.Ihandle, int, int) int { layoutImage(); return iup.DEFAULT }))
	canvas.SetCallback("SCROLL_CB", iup.ScrollFunc(func(ih iup.Ihandle, op int, posx, posy float64) int { iup.Update(ih); return iup.DEFAULT }))
	canvas.SetCallback("WHEEL_CB", iup.WheelFunc(canvasWheel))
	canvas.SetCallback("BUTTON_CB", iup.ButtonFunc(canvasButton))
	canvas.SetCallback("MOTION_CB", iup.MotionFunc(canvasMotion))
	canvas.SetCallback("K_ANY", iup.KAnyFunc(canvasKey))
	canvas.SetCallback("DROPFILES_CB", iup.DropFilesFunc(func(ih iup.Ihandle, name string, num, x, y int) int {
		if num == 0 {
			openFile(name)
		}
		return iup.DEFAULT
	}))

	fileLabel := iup.Label("Open an image, or drop one here").SetAttribute("EXPAND", "HORIZONTAL").SetHandle("iv_file")
	sizeLabel := iup.Label("").SetHandle("iv_size")
	zoomLabel := iup.Label("").SetHandle("iv_zoom")
	indexLabel := iup.Label("").SetHandle("iv_index")
	zoomSep := iup.Label("").SetAttribute("SEPARATOR", "VERTICAL").SetHandle("iv_zoomsep")
	indexSep := iup.Label("").SetAttribute("SEPARATOR", "VERTICAL").SetHandle("iv_indexsep")
	status := iup.Hbox(fileLabel, sizeLabel, zoomSep, zoomLabel, indexSep, indexLabel).SetAttributes("NGAP=8, NMARGIN=4x2, ALIGNMENT=ACENTER")
	status.SetHandle("imageviewer_status")

	slideTimer := iup.Timer().SetAttribute("TIME", 3000).SetHandle("iv_timer")
	slideTimer.SetCallback("ACTION_CB", iup.TimerActionFunc(func(iup.Ihandle) int {
		if len(files) > 1 {
			showIndex((current + 1) % len(files))
		}
		return iup.DEFAULT
	}))

	dlg := iup.Dialog(iup.Vbox(canvas, status)).SetHandle("iv_dlg").SetAttributes(map[string]string{
		"TITLE":     title,
		"MENU":      "imageviewer_menu",
		"PLACEMENT": "MAXIMIZED",
	})
	buildMenu()
	updateStatus()

	iup.Show(dlg)
	iup.SetFocus(canvas)

	if len(os.Args) > 1 {
		openFile(os.Args[1])
	}

	iup.MainLoop()
	slideTimer.Destroy()
}

func item(label string, f func()) iup.Ihandle {
	return iup.MenuItem(label).SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { f(); return iup.DEFAULT }))
}

func buildMenu() {
	iup.Menu(
		iup.Submenu("&File", iup.Menu(
			item("&Open...\tCtrl+O", openDialog),
			item("&Save As...\tCtrl+S", saveDialog),
			item("&Close\tCtrl+W", closeImage),
			iup.MenuSeparator(),
			iup.MenuItem("E&xit").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { return iup.CLOSE })),
		)),
		iup.Submenu("&Edit", iup.Menu(
			item("&Copy\tCtrl+C", copyImage),
			item("&Paste\tCtrl+V", pasteImage),
		)),
		iup.Submenu("&View", iup.Menu(
			item("Zoom &In\tCtrl++", func() { zoomBy(1.25, -1, -1) }),
			item("Zoom &Out\tCtrl+-", func() { zoomBy(0.8, -1, -1) }),
			item("&Actual Size\tCtrl+0", func() { setZoom(1, -1, -1) }),
			item("&Fit to Window\tCtrl+F", fitWindow),
			iup.MenuSeparator(),
			iup.MenuItem("&Slideshow\tF5").SetHandle("imageviewer_slideshow").
				SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { toggleSlideshow(); return iup.DEFAULT })),
			item("F&ull Screen\tF11", toggleFullScreen),
		)),
		iup.Submenu("&Image", iup.Menu(
			item("Rotate &Left\tCtrl+L", func() { transform(rotateLeft) }),
			item("Rotate &Right\tCtrl+R", func() { transform(rotateRight) }),
			item("Flip &Horizontal", func() { transform(flipHorizontal) }),
			item("Flip &Vertical", func() { transform(flipVertical) }),
		)),
		iup.Submenu("&Go", iup.Menu(
			item("&Previous\tPgUp", func() { showIndex(current - 1) }),
			item("&Next\tPgDn", func() { showIndex(current + 1) }),
			item("&First\tHome", func() { showIndex(0) }),
			item("&Last\tEnd", func() { showIndex(len(files) - 1) }),
		)),
	).SetHandle("imageviewer_menu")
}

func openDialog() {
	fdlg := iup.FileDlg()
	defer fdlg.Destroy()
	fdlg.SetAttributes(map[string]string{
		"DIALOGTYPE": "OPEN",
		"TITLE":      "Open Image",
		"EXTFILTER":  "Images|*" + strings.Join(extensions, ";*") + "|All Files|*.*|",
	})
	if current >= 0 {
		fdlg.SetAttribute("DIRECTORY", filepath.Dir(files[current]))
	}
	iup.SetAttributeHandle(fdlg, "PARENTDIALOG", iup.GetHandle("iv_dlg"))
	iup.Popup(fdlg, iup.CENTERPARENT, iup.CENTERPARENT)
	if fdlg.GetInt("STATUS") != -1 {
		openFile(fdlg.GetAttribute("VALUE"))
	}
}

func openFile(path string) {
	path, err := filepath.Abs(path)
	if err != nil {
		iup.MessageError(iup.GetHandle("iv_dlg"), err.Error())
		return
	}
	files = listImages(filepath.Dir(path))
	current = slices.Index(files, path)
	if current < 0 {
		files = append(files, path)
		current = len(files) - 1
	}
	loadCurrent()
}

func listImages(dir string) []string {
	entries, err := os.ReadDir(dir)
	if err != nil {
		return nil
	}
	var list []string
	for _, e := range entries {
		if !e.IsDir() && slices.Contains(extensions, strings.ToLower(filepath.Ext(e.Name()))) {
			list = append(list, filepath.Join(dir, e.Name()))
		}
	}
	slices.SortFunc(list, func(a, b string) int { return strings.Compare(strings.ToLower(a), strings.ToLower(b)) })
	return list
}

func showIndex(i int) {
	if len(files) == 0 {
		return
	}
	i = max(0, min(i, len(files)-1))
	if i == current && img != 0 {
		return
	}
	current = i
	loadCurrent()
}

func loadCurrent() {
	dlg := iup.GetHandle("iv_dlg")

	releaseImage()
	path := files[current]
	h := iup.ImageGetHandle(path)
	if h == 0 {
		iup.GetHandle("iv_file").SetAttribute("TITLE", "Cannot open "+filepath.Base(path))
		dlg.SetAttribute("TITLE", title)
		updateStatus()
		iup.Update(iup.GetHandle("iv_canvas"))
		return
	}
	setImage(h, path)
	fit = true
	dlg.SetAttribute("TITLE", filepath.Base(path)+" - "+title)
	layoutImage()
}

func setImage(h iup.Ihandle, name string) {
	img, imgName = h, name
	imgW, imgH = h.GetInt("WIDTH"), h.GetInt("HEIGHT")
}

func releaseImage() {
	if img != 0 {
		img.Destroy()
	}
	img, imgName, imgW, imgH = 0, "", 0, 0
}

func closeImage() {
	stopSlideshow()
	releaseImage()
	files, current = nil, -1
	iup.GetHandle("iv_dlg").SetAttribute("TITLE", title)
	iup.GetHandle("iv_file").SetAttribute("TITLE", "No image")
	layoutImage()
}

func canvasSize() (int, int) {
	var w, h int
	fmt.Sscanf(iup.GetHandle("iv_canvas").GetAttribute("DRAWSIZE"), "%dx%d", &w, &h)
	return w, h
}

func fitZoom() float64 {
	w, h := canvasSize()
	if imgW == 0 || imgH == 0 || w <= 0 || h <= 0 {
		return 1
	}
	return math.Min(1, math.Min(float64(w)/float64(imgW), float64(h)/float64(imgH)))
}

func scaled() (int, int) {
	return int(math.Round(float64(imgW) * zoom)), int(math.Round(float64(imgH) * zoom))
}

func layoutImage() {
	canvas := iup.GetHandle("iv_canvas")

	if fit {
		zoom = fitZoom()
	}
	w, h := canvasSize()
	sw, sh := scaled()
	canvas.SetAttributes(fmt.Sprintf("XMAX=%d, YMAX=%d, DX=%d, DY=%d", max(sw, w), max(sh, h), w, h))
	updateStatus()
	iup.Update(canvas)
}

func fitWindow() {
	fit = true
	layoutImage()
}

func zoomBy(factor float64, cx, cy int) {
	setZoom(zoom*factor, cx, cy)
}

func setZoom(z float64, cx, cy int) {
	if img == 0 {
		return
	}

	canvas := iup.GetHandle("iv_canvas")
	z = math.Max(0.02, math.Min(z, 32))
	w, h := canvasSize()
	if cx < 0 {
		cx, cy = w/2, h/2
	}
	ox, oy := imageOrigin()
	px := (float64(cx) - float64(ox)) / zoom
	py := (float64(cy) - float64(oy)) / zoom

	fit = false
	zoom = z
	layoutImage()

	sw, sh := scaled()
	if sw > w {
		canvas.SetAttribute("POSX", int(px*zoom)-cx)
	}
	if sh > h {
		canvas.SetAttribute("POSY", int(py*zoom)-cy)
	}
	iup.Update(canvas)
}

func imageOrigin() (int, int) {
	canvas := iup.GetHandle("iv_canvas")

	w, h := canvasSize()
	sw, sh := scaled()
	x := (w - sw) / 2
	if sw > w {
		x = -int(iup.GetFloat(canvas, "POSX"))
	}
	y := (h - sh) / 2
	if sh > h {
		y = -int(iup.GetFloat(canvas, "POSY"))
	}
	return x, y
}

func canvasAction(ih iup.Ihandle) int {
	iup.DrawBegin(ih)
	w, h := iup.DrawGetSize(ih)
	ih.SetAttribute("DRAWCOLOR", ih.GetAttribute("BGCOLOR"))
	ih.SetAttribute("DRAWSTYLE", "FILL")
	iup.DrawRectangle(ih, 0, 0, w-1, h-1)
	if img != 0 {
		x, y := imageOrigin()
		sw, sh := scaled()
		drawChecker(ih, max(x, 0), max(y, 0), min(x+sw, w)-1, min(y+sh, h)-1)
		quality := "LINEAR"
		if zoom >= 4 {
			quality = "NEAREST"
		}
		ih.SetAttribute("DRAWIMAGEQUALITY", quality)
		iup.DrawImage(ih, imgName, x, y, sw, sh)
	}
	iup.DrawEnd(ih)
	return iup.DEFAULT
}

func drawChecker(ih iup.Ihandle, x1, y1, x2, y2 int) {
	const cell = 8
	ih.SetAttribute("DRAWCOLOR", "204 204 204")
	iup.DrawRectangle(ih, x1, y1, x2, y2)
	ih.SetAttribute("DRAWCOLOR", "153 153 153")
	for y := y1 - y1%cell; y <= y2; y += cell {
		for x := x1 - x1%cell; x <= x2; x += cell {
			if (x/cell+y/cell)%2 == 0 {
				iup.DrawRectangle(ih, max(x, x1), max(y, y1), min(x+cell-1, x2), min(y+cell-1, y2))
			}
		}
	}
}

func updateStatus() {
	fileLabel := iup.GetHandle("iv_file")
	sizeLabel := iup.GetHandle("iv_size")
	zoomLabel := iup.GetHandle("iv_zoom")

	index := ""
	if current >= 0 {
		index = fmt.Sprintf("%d / %d", current+1, len(files))
	}
	iup.GetHandle("iv_index").SetAttribute("TITLE", index)
	shown := img != 0 && !iup.GetHandle("iv_dlg").GetBool("FULLSCREEN")
	iup.GetHandle("iv_zoomsep").SetAttribute("VISIBLE", shown)
	iup.GetHandle("iv_indexsep").SetAttribute("VISIBLE", shown && index != "")
	if img == 0 {
		sizeLabel.SetAttribute("TITLE", "")
		zoomLabel.SetAttribute("TITLE", "")
		iup.Refresh(fileLabel)
		return
	}
	name := "Clipboard"
	if current >= 0 {
		name = filepath.Base(files[current])
		if info, err := os.Stat(files[current]); err == nil {
			name += ", " + fileSize(info.Size())
		}
	}
	fileLabel.SetAttribute("TITLE", name)
	sizeLabel.SetAttribute("TITLE", fmt.Sprintf("%d x %d", imgW, imgH))
	zoomLabel.SetAttribute("TITLE", fmt.Sprintf("%.0f%%", zoom*100))
	iup.Refresh(fileLabel)
}

func fileSize(n int64) string {
	switch {
	case n >= 1<<20:
		return fmt.Sprintf("%.1f MB", float64(n)/(1<<20))
	case n >= 1<<10:
		return fmt.Sprintf("%.0f KB", float64(n)/(1<<10))
	}
	return fmt.Sprintf("%d B", n)
}

func canvasWheel(ih iup.Ihandle, delta float64, x, y int, st string) int {
	if img == 0 {
		return iup.DEFAULT
	}
	if iup.IsControl(st) {
		if delta > 0 {
			zoomBy(1.25, x, y)
		} else if delta < 0 {
			zoomBy(0.8, x, y)
		}
		return iup.DEFAULT
	}
	w, h := canvasSize()
	sw, sh := scaled()
	switch {
	case sh > h:
		ih.SetAttribute("POSY", float64(iup.GetFloat(ih, "POSY"))-delta*float64(h)/8)
	case sw > w:
		ih.SetAttribute("POSX", float64(iup.GetFloat(ih, "POSX"))-delta*float64(w)/8)
	case delta < 0:
		showIndex(current + 1)
	case delta > 0:
		showIndex(current - 1)
	}
	iup.Update(ih)
	return iup.DEFAULT
}

func canvasButton(ih iup.Ihandle, button, pressed, x, y int, st string) int {
	if button != iup.BUTTON1 {
		return iup.DEFAULT
	}
	if pressed == 1 && iup.IsDouble(st) {
		if fit {
			setZoom(1, x, y)
		} else {
			fitWindow()
		}
		return iup.DEFAULT
	}
	dragging = pressed == 1
	dragX, dragY = x, y
	cursor := "ARROW"
	if dragging {
		cursor = "MOVE"
	}
	ih.SetAttribute("CURSOR", cursor)
	return iup.DEFAULT
}

func canvasMotion(ih iup.Ihandle, x, y int, st string) int {
	if !dragging || !iup.IsButton1(st) {
		return iup.DEFAULT
	}
	ih.SetAttribute("POSX", float64(iup.GetFloat(ih, "POSX"))-float64(x-dragX))
	ih.SetAttribute("POSY", float64(iup.GetFloat(ih, "POSY"))-float64(y-dragY))
	dragX, dragY = x, y
	iup.Update(ih)
	return iup.DEFAULT
}

func canvasKey(ih iup.Ihandle, c int) int {
	switch c {
	case iup.K_ESC:
		stopSlideshow()
		if iup.GetHandle("iv_dlg").GetBool("FULLSCREEN") {
			toggleFullScreen()
		}
	case iup.K_F11:
		toggleFullScreen()
	case iup.K_F5:
		toggleSlideshow()
	case iup.K_LEFT, iup.K_BS:
		showIndex(current - 1)
	case iup.K_RIGHT, iup.K_SP:
		showIndex(current + 1)
	case iup.K_plus, iup.K_equal:
		zoomBy(1.25, -1, -1)
	case iup.K_minus:
		zoomBy(0.8, -1, -1)
	case iup.K_0:
		setZoom(1, -1, -1)
	case iup.K_f:
		fitWindow()
	case iup.K_UP, iup.K_DOWN:
		_, h := canvasSize()
		step := float64(h) / 8
		if c == iup.K_UP {
			step = -step
		}
		ih.SetAttribute("POSY", float64(iup.GetFloat(ih, "POSY"))+step)
		iup.Update(ih)
	default:
		return iup.CONTINUE
	}
	return iup.IGNORE
}

func toggleFullScreen() {
	dlg := iup.GetHandle("iv_dlg")

	full := !dlg.GetBool("FULLSCREEN")
	status := iup.GetHandle("imageviewer_status")
	status.SetAttribute("FLOATING", full)
	status.SetAttribute("VISIBLE", !full)
	menu := "imageviewer_menu"
	if full {
		menu = ""
	}
	dlg.SetAttribute("MENU", menu)
	dlg.SetAttribute("FULLSCREEN", full)
	updateStatus()
	iup.Refresh(dlg)
	iup.SetFocus(iup.GetHandle("iv_canvas"))
}

func toggleSlideshow() {
	slideTimer := iup.GetHandle("iv_timer")

	if slideTimer.GetBool("RUN") {
		stopSlideshow()
		return
	}
	if len(files) > 1 {
		slideTimer.SetAttribute("RUN", "YES")
		iup.GetHandle("imageviewer_slideshow").SetAttribute("VALUE", "ON")
	}
}

func stopSlideshow() {
	iup.GetHandle("iv_timer").SetAttribute("RUN", "NO")
	iup.GetHandle("imageviewer_slideshow").SetAttribute("VALUE", "OFF")
}

func copyImage() {
	if img == 0 {
		return
	}
	clip := iup.Clipboard()
	clip.SetAttribute("IMAGE", imgName)
	clip.Destroy()
}

func pasteImage() {
	dlg := iup.GetHandle("iv_dlg")

	clip := iup.Clipboard()
	defer clip.Destroy()
	if !clip.GetBool("IMAGEAVAILABLE") {
		iup.GetHandle("iv_file").SetAttribute("TITLE", "No image in the clipboard")
		return
	}
	h := iup.ImageFromHandle(iup.GetPtr(clip, "NATIVEIMAGE"))
	if h == 0 {
		iup.MessageError(dlg, "The clipboard image cannot be read")
		return
	}
	stopSlideshow()
	releaseImage()
	files, current = nil, -1
	setImage(h.SetHandle("imageviewer_clipboard"), "imageviewer_clipboard")
	fit = true
	dlg.SetAttribute("TITLE", "Clipboard - "+title)
	layoutImage()
}

func saveDialog() {
	if img == 0 {
		return
	}

	dlg := iup.GetHandle("iv_dlg")
	fdlg := iup.FileDlg()
	defer fdlg.Destroy()
	base := "clipboard"
	if current >= 0 {
		base = strings.TrimSuffix(filepath.Base(files[current]), filepath.Ext(files[current]))
		fdlg.SetAttribute("DIRECTORY", filepath.Dir(files[current]))
	}
	fdlg.SetAttributes(map[string]string{
		"DIALOGTYPE": "SAVE",
		"TITLE":      "Save Image As",
		"EXTFILTER":  "PNG|*.png|JPEG|*.jpg;*.jpeg|BMP|*.bmp|",
		"FILE":       base + ".png",
	})
	iup.SetAttributeHandle(fdlg, "PARENTDIALOG", dlg)
	iup.Popup(fdlg, iup.CENTERPARENT, iup.CENTERPARENT)
	if fdlg.GetInt("STATUS") == -1 {
		return
	}
	path := fdlg.GetAttribute("VALUE")
	if filepath.Ext(path) == "" {
		path += []string{".png", ".jpg", ".bmp"}[max(fdlg.GetInt("FILTERUSED"), 1)-1]
	}
	if iup.ImageSave(img, path, "") == 0 {
		iup.MessageError(dlg, "Cannot save "+path)
	}
}

func transform(f func(src *image.RGBA) *image.RGBA) {
	if img == 0 {
		return
	}
	src := iup.ImageToImage(img)
	if src == nil {
		iup.MessageError(iup.GetHandle("iv_dlg"), "This image format cannot be transformed")
		return
	}
	name := imgName
	img.Destroy()
	setImage(iup.ImageFromImage(f(src)).SetHandle(name), name)
	layoutImage()
}

func rotateRight(src *image.RGBA) *image.RGBA {
	w, h := src.Rect.Dx(), src.Rect.Dy()
	dst := image.NewRGBA(image.Rect(0, 0, h, w))
	for y := range h {
		for x := range w {
			copy(dst.Pix[dst.PixOffset(h-1-y, x):], src.Pix[src.PixOffset(x, y):src.PixOffset(x, y)+4])
		}
	}
	return dst
}

func rotateLeft(src *image.RGBA) *image.RGBA {
	w, h := src.Rect.Dx(), src.Rect.Dy()
	dst := image.NewRGBA(image.Rect(0, 0, h, w))
	for y := range h {
		for x := range w {
			copy(dst.Pix[dst.PixOffset(y, w-1-x):], src.Pix[src.PixOffset(x, y):src.PixOffset(x, y)+4])
		}
	}
	return dst
}

func flipHorizontal(src *image.RGBA) *image.RGBA {
	w, h := src.Rect.Dx(), src.Rect.Dy()
	dst := image.NewRGBA(src.Rect)
	for y := range h {
		for x := range w {
			copy(dst.Pix[dst.PixOffset(w-1-x, y):], src.Pix[src.PixOffset(x, y):src.PixOffset(x, y)+4])
		}
	}
	return dst
}

func flipVertical(src *image.RGBA) *image.RGBA {
	w, h := src.Rect.Dx(), src.Rect.Dy()
	dst := image.NewRGBA(src.Rect)
	for y := range h {
		copy(dst.Pix[dst.PixOffset(0, h-1-y):dst.PixOffset(0, h-1-y)+4*w], src.Pix[src.PixOffset(0, y):src.PixOffset(0, y)+4*w])
	}
	return dst
}
