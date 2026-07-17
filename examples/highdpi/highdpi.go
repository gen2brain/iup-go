package main

import (
	"fmt"
	"image"
	"image/color"
	"math"
	"strconv"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

var uiScale = 1.0

func setStatus(s string) { iup.GetHandle("status").SetAttribute("TITLE", s) }

// icon builds a small emblem (blue disc, white ring and dot) to show off IMAGEAUTOSCALE.
func icon() image.Image {
	const s = 32
	img := image.NewRGBA(image.Rect(0, 0, s, s))
	blue := color.RGBA{50, 90, 220, 255}
	white := color.RGBA{255, 255, 255, 255}
	cx, cy := 15.5, 15.5
	for y := 0; y < s; y++ {
		for x := 0; x < s; x++ {
			d := math.Hypot(float64(x)-cx, float64(y)-cy)
			switch {
			case d <= 3, d >= 6 && d <= 10:
				img.Set(x, y, white)
			case d <= 15:
				img.Set(x, y, blue)
			}
		}
	}
	return img
}

// swatch shows the same base image scaled by AUTOSCALE, with the factor as caption.
func swatch(base image.Image, factor string) iup.Ihandle {
	name := "img" + factor
	iup.ImageFromImage(base).SetAttribute("AUTOSCALE", factor).SetHandle(name)
	pic := iup.Label("").SetAttribute("IMAGE", name)
	return iup.Vbox(pic, iup.Label("AUTOSCALE="+factor)).SetAttributes(`ALIGNMENT=ACENTER, NGAP=4`)
}

// canvas draws a ruler and shapes whose sizes all multiply by uiScale, the DPI-aware pattern.
func canvas() iup.Ihandle {
	cv := iup.Canvas().SetAttributes(`RASTERSIZE=x150, EXPAND=HORIZONTAL`).SetHandle("cv")
	cv.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		iup.DrawBegin(ih)
		w, h := iup.DrawGetSize(ih)
		s := uiScale
		ih.SetAttributes(`DRAWSTYLE=FILL, DRAWCOLOR="250 250 250"`)
		iup.DrawRectangle(ih, 0, 0, w, h)

		ih.SetAttributes(`DRAWSTYLE=STROKE, DRAWCOLOR="120 120 120"`)
		step := int(20 * s)
		for i, x := 0, 0; x < w; i, x = i+1, x+step {
			tall := 10
			if i%5 == 0 {
				tall = 18
			}
			iup.DrawLine(ih, x, 0, x, int(float64(tall)*s))
		}

		ih.SetAttributes(`DRAWSTYLE=FILL, DRAWCOLOR="50 90 220"`)
		iup.DrawRoundedRectangle(ih, int(20*s), int(48*s), int(120*s), int(108*s), int(12*s))
		ih.SetAttributes(`DRAWSTYLE=FILL, DRAWCOLOR="230 140 30"`)
		iup.DrawArc(ih, int(140*s), int(48*s), int(200*s), int(108*s), 0, 360)

		ih.SetAttribute("DRAWFONT", fmt.Sprintf("Sans %d", int(12*s)))
		ih.SetAttribute("DRAWCOLOR", "30 30 30")
		iup.DrawText(ih, fmt.Sprintf("scale %.0f%%", s*100), int(20*s), int(118*s), 0, 0)
		iup.DrawEnd(ih)
		return iup.DEFAULT
	}))
	return cv
}

func setScale(f float64) int {
	uiScale = f
	dlg := iup.GetHandle("dlg")
	dlg.SetAttribute("FONT", fmt.Sprintf("Sans %d", int(math.Round(10*f))))
	dlg.SetAttribute("SIZE", nil)
	cv := iup.GetHandle("cv")
	cv.SetAttribute("RASTERSIZE", fmt.Sprintf("x%d", int(150*f)))
	iup.Refresh(dlg)
	iup.Update(cv)
	setStatus(fmt.Sprintf("UI scale set to %.0f%% (dialog FONT rescaled)", f*100))
	return iup.DEFAULT
}

func scaleButton(label string, f float64) iup.Ihandle {
	return iup.Button(label).SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { return setScale(f) }))
}

func main() {
	iup.Open()

	dpi, ratio := iup.GetGlobal("SCREENDPI"), 1.0
	if v, err := strconv.ParseFloat(dpi, 64); err == nil && v > 0 {
		dpi, ratio = fmt.Sprintf("%.1f", v), v/96.0
	}
	imagesDPI := iup.GetGlobal("IMAGESDPI")
	if imagesDPI == "" {
		imagesDPI = "96 (default)"
	}
	screen := iup.Frame(iup.Vbox(
		iup.Label(fmt.Sprintf("SCREENDPI: %s  (system scale ~ %.2fx of 96 DPI)", dpi, ratio)),
		iup.Label("IMAGESDPI (base for IMAGEAUTOSCALE=DPI): "+imagesDPI),
	).SetAttributes(`NMARGIN=8x8, NGAP=4`)).SetAttribute("TITLE", "Screen")

	base := icon()
	images := iup.Frame(iup.Hbox(
		swatch(base, "1.0"), swatch(base, "1.5"), swatch(base, "2.0"),
	).SetAttributes(`NGAP=24, NMARGIN=10x10, ALIGNMENT=ACENTER`)).
		SetAttribute("TITLE", "Image auto-scaling (per-image AUTOSCALE)")

	draw := iup.Frame(iup.Vbox(canvas()).SetAttributes(`NMARGIN=6x6`)).
		SetAttribute("TITLE", "DPI-aware canvas drawing (sizes x uiScale)")

	scaleRow := iup.Frame(iup.Hbox(
		iup.Label("UI scale:"),
		scaleButton("100%", 1.0), scaleButton("150%", 1.5), scaleButton("200%", 2.0),
	).SetAttributes(`NMARGIN=8x8, NGAP=8, ALIGNMENT=ACENTER`)).
		SetAttribute("TITLE", "Global scale")

	status := iup.Label("Pick a UI scale to rescale the dialog font and the canvas drawing.").
		SetAttribute("EXPAND", "HORIZONTAL").SetHandle("status")

	dlg := iup.Dialog(
		iup.Vbox(screen, images, draw, scaleRow, status).SetAttributes(`NMARGIN=10x10, NGAP=8`),
	).SetAttribute("TITLE", "High DPI").SetHandle("dlg")

	iup.Show(dlg)
	iup.MainLoop()
	iup.Close()
}
