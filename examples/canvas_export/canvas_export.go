package main

import (
	"fmt"
	"image/png"
	"math"
	"os"
	"strings"

	"github.com/gen2brain/iup-go/iup"
)

var canvas iup.Ihandle

func main() {
	iup.Open()
	defer iup.Close()

	iup.SetHandle("HOUSE_IMG", createHouseImage())

	canvas = iup.Canvas().SetAttributes("RASTERSIZE=600x400, BORDER=NO")
	canvas.SetCallback("ACTION", iup.ActionFunc(redraw))

	btnSavePng := iup.Button("Save as PNG (Go)").SetAttributes("PADDING=10x4")
	btnSavePng.SetCallback("ACTION", iup.ActionFunc(onSavePng))

	btnSaveSvg := iup.Button("Save as SVG").SetAttributes("PADDING=10x4")
	btnSaveSvg.SetCallback("ACTION", iup.ActionFunc(onSaveSvg))

	btnNativePng := iup.Button("Save as PNG (Native)").SetAttributes("PADDING=10x4")
	btnNativePng.SetCallback("ACTION", iup.ActionFunc(onNativeSave("PNG")))

	btnNativeJpeg := iup.Button("Save as JPEG (Native)").SetAttributes("PADDING=10x4")
	btnNativeJpeg.SetCallback("ACTION", iup.ActionFunc(onNativeSave("JPEG")))

	btnNativeBmp := iup.Button("Save as BMP (Native)").SetAttributes("PADDING=10x4")
	btnNativeBmp.SetCallback("ACTION", iup.ActionFunc(onNativeSave("BMP")))

	btnBufferTest := iup.Button("Buffer Round-Trip Test").SetAttributes("PADDING=10x4")
	btnBufferTest.SetCallback("ACTION", iup.ActionFunc(onBufferTest))

	dlg := iup.Dialog(
		iup.Vbox(
			canvas,
			iup.Hbox(btnSavePng, btnSaveSvg).SetAttributes("GAP=8"),
			iup.Hbox(btnNativePng, btnNativeJpeg, btnNativeBmp, btnBufferTest).SetAttributes("GAP=8"),
		).SetAttributes("MARGIN=10x10, GAP=8"),
	).SetAttribute("TITLE", "Canvas Export")

	iup.Show(dlg)
	iup.MainLoop()
}

func redraw(ih iup.Ihandle) int {
	iup.DrawBegin(ih)
	defer iup.DrawEnd(ih)

	w, h := iup.DrawGetSize(ih)
	drawScene(ih, w, h)

	return iup.DEFAULT
}

func captureCanvas() iup.Ihandle {
	iup.DrawBegin(canvas)

	w, h := iup.DrawGetSize(canvas)
	drawScene(canvas, w, h)

	img := iup.DrawGetImage(canvas)

	iup.DrawEnd(canvas)

	return img
}

func onNativeSave(format string) func(iup.Ihandle) int {
	ext := strings.ToLower(format)
	if ext == "jpeg" {
		ext = "jpg"
	}

	return func(ih iup.Ihandle) int {
		filter := fmt.Sprintf("*.%s", ext)
		title := fmt.Sprintf("Save as %s (Native)", format)

		filedlg := iup.FileDlg().SetAttributes(fmt.Sprintf(`DIALOGTYPE=SAVE, TITLE="%s", FILTER="%s", EXTDEFAULT="%s"`, title, filter, ext))
		defer filedlg.Destroy()

		iup.Popup(filedlg, iup.CENTER, iup.CENTER)

		if filedlg.GetInt("STATUS") == -1 {
			return iup.DEFAULT
		}

		filename := filedlg.GetAttribute("VALUE")

		img := captureCanvas()
		if img == 0 {
			iup.Message("Error", "Failed to capture canvas image.")
			return iup.DEFAULT
		}
		defer img.Destroy()

		ret := iup.ImageSave(img, filename, format)
		if ret == 0 {
			iup.Message("Error", fmt.Sprintf("Failed to save %s file.", format))
			return iup.DEFAULT
		}

		w := iup.GetInt(img, "WIDTH")
		h := iup.GetInt(img, "HEIGHT")
		iup.Message("Success", fmt.Sprintf("Native %s saved to:\n%s\n\nSize: %dx%d", format, filename, w, h))

		return iup.DEFAULT
	}
}

func onBufferTest(ih iup.Ihandle) int {
	img := captureCanvas()
	if img == 0 {
		iup.Message("Error", "Failed to capture canvas image.")
		return iup.DEFAULT
	}
	defer img.Destroy()

	results := ""

	for _, format := range []string{"PNG", "JPEG", "BMP"} {
		buf := iup.ImageSaveToBuffer(img, format)
		if buf == nil {
			results += fmt.Sprintf("%s: FAILED\n", format)
		} else {
			results += fmt.Sprintf("%s: %d bytes\n", format, len(buf))
		}
	}

	iup.Message("Buffer Round-Trip Results", results)

	return iup.DEFAULT
}

func onSavePng(ih iup.Ihandle) int {
	filedlg := iup.FileDlg().SetAttributes(`DIALOGTYPE=SAVE, TITLE="Save as PNG (Go)", FILTER="*.png", FILTERINFO="PNG Files", EXTDEFAULT="png"`)
	defer filedlg.Destroy()

	iup.Popup(filedlg, iup.CENTER, iup.CENTER)

	if filedlg.GetInt("STATUS") == -1 {
		return iup.DEFAULT
	}

	filename := filedlg.GetAttribute("VALUE")

	img := captureCanvas()
	if img == 0 {
		iup.Message("Error", "Failed to capture canvas image.")
		return iup.DEFAULT
	}
	defer img.Destroy()

	goImg := iup.ImageToImage(img)
	if goImg == nil {
		iup.Message("Error", "Failed to convert image.")
		return iup.DEFAULT
	}

	f, err := os.Create(filename)
	if err != nil {
		iup.Message("Error", fmt.Sprintf("Failed to create file: %s", err))
		return iup.DEFAULT
	}
	defer f.Close()

	if err := png.Encode(f, goImg); err != nil {
		iup.Message("Error", fmt.Sprintf("Failed to encode PNG: %s", err))
		return iup.DEFAULT
	}

	iup.Message("Success", fmt.Sprintf("Go PNG saved to:\n%s\n\nSize: %dx%d", filename, goImg.Bounds().Dx(), goImg.Bounds().Dy()))

	return iup.DEFAULT
}

func onSaveSvg(ih iup.Ihandle) int {
	filedlg := iup.FileDlg().SetAttributes(`DIALOGTYPE=SAVE, TITLE="Save as SVG", FILTER="*.svg", FILTERINFO="SVG Files", EXTDEFAULT="svg"`)
	defer filedlg.Destroy()

	iup.Popup(filedlg, iup.CENTER, iup.CENTER)

	if filedlg.GetInt("STATUS") == -1 {
		return iup.DEFAULT
	}

	filename := filedlg.GetAttribute("VALUE")
	if !strings.HasSuffix(strings.ToLower(filename), ".svg") {
		filename += ".svg"
	}

	svg := iup.DrawGetSvg(canvas)
	if svg == "" {
		iup.Message("Error", "Failed to generate SVG.")
		return iup.DEFAULT
	}

	err := os.WriteFile(filename, []byte(svg), 0644)
	if err != nil {
		iup.Message("Error", fmt.Sprintf("Failed to write SVG: %s", err))
		return iup.DEFAULT
	}

	_, w, h := iup.GetInt2(canvas, "DRAWSIZE")
	iup.Message("Success", fmt.Sprintf("SVG saved to:\n%s\n\nSize: %dx%d", filename, w, h))

	return iup.DEFAULT
}

func drawScene(ih iup.Ihandle, w, h int) {
	ih.SetAttributes(`DRAWCOLOR="245 245 250", DRAWSTYLE=FILL`)
	iup.DrawRectangle(ih, 0, 0, w, h)

	drawSky(ih, w, h)
	drawMountains(ih, w, h)
	drawSun(ih, w, h)
	drawTrees(ih, w, h)
	drawGround(ih, w, h)
	drawHouse(ih, w, h)
	drawTitle(ih, w)
}

func drawSky(ih iup.Ihandle, w, h int) {
	iup.DrawLinearGradient(ih, 0, 0, w, h*2/3, 90, "135 206 235", "200 230 255")
}

func drawSun(ih iup.Ihandle, w, h int) {
	cx := w - 80
	cy := 60
	iup.DrawRadialGradient(ih, cx, cy, 50, "255 255 180", "255 200 50")

	ih.SetAttributes(`DRAWCOLOR="255 220 80", DRAWSTYLE=STROKE, DRAWLINEWIDTH=2`)
	for i := 0; i < 8; i++ {
		angle := float64(i) * math.Pi / 4
		x1 := cx + int(55*math.Cos(angle))
		y1 := cy + int(55*math.Sin(angle))
		x2 := cx + int(70*math.Cos(angle))
		y2 := cy + int(70*math.Sin(angle))
		iup.DrawLine(ih, x1, y1, x2, y2)
	}
}

func drawMountains(ih iup.Ihandle, w, h int) {
	baseY := h * 2 / 3

	ih.SetAttributes(`DRAWCOLOR="120 140 160", DRAWSTYLE=FILL`)
	iup.DrawPolygon(ih, []int{0, baseY, w / 5, baseY - h/4, w * 2 / 5, baseY}, 3)

	ih.SetAttributes(`DRAWCOLOR="100 120 145", DRAWSTYLE=FILL`)
	iup.DrawPolygon(ih, []int{w / 4, baseY, w / 2, baseY - h/3, w * 3 / 4, baseY}, 3)

	ih.SetAttributes(`DRAWCOLOR="140 155 170", DRAWSTYLE=FILL`)
	iup.DrawPolygon(ih, []int{w / 2, baseY, w * 3 / 4, baseY - h/5, w, baseY}, 3)

	ih.SetAttributes(`DRAWCOLOR="255 255 255", DRAWSTYLE=FILL`)
	iup.DrawPolygon(ih, []int{w/2 - 20, baseY - h/3 + 15, w / 2, baseY - h/3, w/2 + 20, baseY - h/3 + 15}, 3)
}

func drawGround(ih iup.Ihandle, w, h int) {
	groundY := h * 2 / 3
	iup.DrawLinearGradient(ih, 0, groundY, w, h, 90, "100 180 80", "60 130 50")
}

func drawTrees(ih iup.Ihandle, w, h int) {
	baseY := h * 2 / 3
	positions := []int{80, 200, 350, 480}

	for i, x := range positions {
		treeH := 40 + (i%3)*15
		trunkW := 6

		ih.SetAttributes(`DRAWCOLOR="100 70 40", DRAWSTYLE=FILL`)
		iup.DrawRectangle(ih, x-trunkW/2, baseY-treeH/3, x+trunkW/2, baseY)

		green := fmt.Sprintf("%d %d %d", 30+i*15, 120+i*10, 30)
		ih.SetAttribute("DRAWCOLOR", green)
		ih.SetAttribute("DRAWSTYLE", "FILL")
		iup.DrawPolygon(ih, []int{x - treeH/3, baseY - treeH/3, x, baseY - treeH, x + treeH/3, baseY - treeH/3}, 3)
	}
}

func drawHouse(ih iup.Ihandle, w, h int) {
	groundY := h * 2 / 3
	imgW, imgH := 48, 48
	x := w/2 - imgW/2
	y := groundY - imgH + 5

	iup.DrawImage(ih, "HOUSE_IMG", x, y, imgW, imgH)
}

func createHouseImage() iup.Ihandle {
	const size = 48
	pixels := make([]byte, size*size*4)

	setPixel := func(x, y int, r, g, b, a byte) {
		if x >= 0 && x < size && y >= 0 && y < size {
			off := (y*size + x) * 4
			pixels[off] = r
			pixels[off+1] = g
			pixels[off+2] = b
			pixels[off+3] = a
		}
	}

	fillRect := func(x1, y1, x2, y2 int, r, g, b, a byte) {
		for y := y1; y <= y2; y++ {
			for x := x1; x <= x2; x++ {
				setPixel(x, y, r, g, b, a)
			}
		}
	}

	// Roof (triangle)
	for y := 8; y <= 22; y++ {
		halfW := (y - 8) * 24 / 14
		cx := 24
		for x := cx - halfW; x <= cx+halfW; x++ {
			setPixel(x, y, 180, 60, 40, 255)
		}
	}

	// Walls
	fillRect(6, 22, 42, 44, 220, 190, 140, 255)

	// Door
	fillRect(19, 30, 28, 44, 120, 70, 30, 255)

	// Doorknob
	setPixel(26, 37, 255, 215, 0, 255)

	// Left window
	fillRect(9, 26, 16, 32, 160, 210, 240, 255)
	fillRect(12, 26, 13, 32, 100, 70, 50, 255) // cross vertical
	fillRect(9, 29, 16, 29, 100, 70, 50, 255)  // cross horizontal

	// Right window
	fillRect(31, 26, 38, 32, 160, 210, 240, 255)
	fillRect(34, 26, 35, 32, 100, 70, 50, 255)
	fillRect(31, 29, 38, 29, 100, 70, 50, 255)

	// Chimney
	fillRect(33, 8, 38, 18, 150, 50, 35, 255)

	return iup.ImageRGBA(size, size, pixels)
}

func drawTitle(ih iup.Ihandle, w int) {
	ih.SetAttributes(`DRAWCOLOR="40 40 60", DRAWFONT="Helvetica, Bold 16", DRAWTEXTALIGNMENT=ACENTER`)
	iup.DrawText(ih, "Mountain Landscape", w/2, 10, -1, -1)
}
