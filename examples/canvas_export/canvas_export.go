package main

import (
	"fmt"
	"image/png"
	"math"
	"os"

	"github.com/gen2brain/iup-go/iup"
)

var canvas iup.Ihandle

func main() {
	iup.Open()
	defer iup.Close()

	canvas = iup.Canvas().SetAttributes("RASTERSIZE=600x400, BORDER=NO")
	canvas.SetCallback("ACTION", iup.ActionFunc(redraw))

	btnSave := iup.Button("Save as PNG...").SetAttributes("PADDING=10x4")
	btnSave.SetCallback("ACTION", iup.ActionFunc(onSave))

	dlg := iup.Dialog(
		iup.Vbox(canvas, btnSave).SetAttributes("MARGIN=10x10, GAP=8"),
	).SetAttribute("TITLE", "Canvas Export")

	iup.Show(dlg)
	iup.MainLoop()
}

func redraw(ih iup.Ihandle) int {
	iup.DrawBegin(ih)
	defer iup.DrawEnd(ih)

	drawScene(ih)

	return iup.DEFAULT
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
	iup.DrawPolygon(ih, []int{
		0, baseY,
		w / 5, baseY - h/4,
		w * 2 / 5, baseY,
	}, 3)

	ih.SetAttributes(`DRAWCOLOR="100 120 145", DRAWSTYLE=FILL`)
	iup.DrawPolygon(ih, []int{
		w / 4, baseY,
		w / 2, baseY - h/3,
		w * 3 / 4, baseY,
	}, 3)

	ih.SetAttributes(`DRAWCOLOR="140 155 170", DRAWSTYLE=FILL`)
	iup.DrawPolygon(ih, []int{
		w / 2, baseY,
		w * 3 / 4, baseY - h/5,
		w, baseY,
	}, 3)

	ih.SetAttributes(`DRAWCOLOR="255 255 255", DRAWSTYLE=FILL`)
	iup.DrawPolygon(ih, []int{
		w/2 - 20, baseY - h/3 + 15,
		w / 2, baseY - h/3,
		w/2 + 20, baseY - h/3 + 15,
	}, 3)
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
		iup.DrawPolygon(ih, []int{
			x - treeH/3, baseY - treeH/3,
			x, baseY - treeH,
			x + treeH/3, baseY - treeH/3,
		}, 3)
	}
}

func drawTitle(ih iup.Ihandle, w int) {
	ih.SetAttributes(`DRAWCOLOR="40 40 60", DRAWFONT="Helvetica, Bold 16", DRAWTEXTALIGNMENT=ACENTER`)
	iup.DrawText(ih, "Mountain Landscape", w/2, 10, -1, -1)
}

func onSave(ih iup.Ihandle) int {
	filedlg := iup.FileDlg().SetAttributes(`DIALOGTYPE=SAVE, TITLE="Save as PNG", FILTER="*.png", FILTERINFO="PNG Files", EXTDEFAULT="png"`)
	defer filedlg.Destroy()

	iup.Popup(filedlg, iup.CENTER, iup.CENTER)

	if filedlg.GetInt("STATUS") == -1 {
		return iup.DEFAULT
	}

	filename := filedlg.GetAttribute("VALUE")

	iup.DrawBegin(canvas)

	drawScene(canvas)

	img := iup.DrawGetImage(canvas)

	iup.DrawEnd(canvas)

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

	iup.Message("Success", fmt.Sprintf("Image saved to:\n%s\n\nSize: %dx%d", filename, goImg.Bounds().Dx(), goImg.Bounds().Dy()))

	return iup.DEFAULT
}

func drawScene(ih iup.Ihandle) {
	w, h := iup.DrawGetSize(ih)

	ih.SetAttributes(`DRAWCOLOR="245 245 250", DRAWSTYLE=FILL`)
	iup.DrawRectangle(ih, 0, 0, w, h)

	drawSky(ih, w, h)
	drawMountains(ih, w, h)
	drawSun(ih, w, h)
	drawTrees(ih, w, h)
	drawGround(ih, w, h)
	drawTitle(ih, w)
}
