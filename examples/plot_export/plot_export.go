//go:build plot

package main

import (
	"fmt"
	"image/png"
	"math"
	"os"
	"strings"

	"github.com/gen2brain/iup-go/iup"
)

var plot iup.Ihandle

func main() {
	iup.Open()
	defer iup.Close()

	iup.PlotOpen()

	plot = iup.Plot().SetAttributes(`RASTERSIZE=640x480, MENUITEMPROPERTIES=NO,
		TITLE="Sine & Cosine", TITLEFONTSIZE=12,
		LEGENDSHOW=YES, LEGENDPOS=TOPRIGHT,
		GRID=YES, GRIDLINESTYLE=DOTTED, GRIDCOLOR="200 200 200",
		AXS_XLABEL="Angle (radians)", AXS_YLABEL="Amplitude",
		AXS_XCROSSORIGIN=YES, AXS_YCROSSORIGIN=YES,
		MARGINLEFT=AUTO, MARGINBOTTOM=AUTO`)

	addSineData()
	addCosineData()

	btnSavePng := iup.Button("Save as PNG...").SetAttributes("PADDING=10x4")
	btnSavePng.SetCallback("ACTION", iup.ActionFunc(onSavePng))

	btnSaveSvg := iup.Button("Save as SVG...").SetAttributes("PADDING=10x4")
	btnSaveSvg.SetCallback("ACTION", iup.ActionFunc(onSaveSvg))

	dlg := iup.Dialog(
		iup.Vbox(plot, iup.Hbox(btnSavePng, btnSaveSvg).SetAttributes("GAP=8")).SetAttributes("MARGIN=10x10, GAP=8"),
	).SetAttribute("TITLE", "Plot Export")

	iup.Show(dlg)
	iup.MainLoop()
}

func addSineData() {
	iup.PlotBegin(plot, 0)
	for i := 0; i <= 360; i += 2 {
		rad := float64(i) * math.Pi / 180
		iup.PlotAdd(plot, rad, math.Sin(rad))
	}
	iup.PlotEnd(plot)
	plot.SetAttribute("DS_LEGEND", "sin(x)")
	plot.SetAttribute("DS_LINEWIDTH", "2")
	plot.SetAttribute("DS_COLOR", "220 50 50")
}

func addCosineData() {
	iup.PlotBegin(plot, 0)
	for i := 0; i <= 360; i += 2 {
		rad := float64(i) * math.Pi / 180
		iup.PlotAdd(plot, rad, math.Cos(rad))
	}
	iup.PlotEnd(plot)
	plot.SetAttribute("DS_LEGEND", "cos(x)")
	plot.SetAttribute("DS_LINEWIDTH", "2")
	plot.SetAttribute("DS_COLOR", "50 50 220")
}

func onSavePng(ih iup.Ihandle) int {
	filedlg := iup.FileDlg().SetAttributes(`DIALOGTYPE=SAVE, TITLE="Save Plot as PNG", FILTER="*.png", FILTERINFO="PNG Files", EXTDEFAULT="png"`)
	defer filedlg.Destroy()

	iup.Popup(filedlg, iup.CENTER, iup.CENTER)

	if filedlg.GetInt("STATUS") == -1 {
		return iup.DEFAULT
	}

	filename := filedlg.GetAttribute("VALUE")

	img := iup.DrawGetImage(plot)
	if img == 0 {
		iup.Message("Error", "Failed to capture plot image.")
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

func onSaveSvg(ih iup.Ihandle) int {
	filedlg := iup.FileDlg().SetAttributes(`DIALOGTYPE=SAVE, TITLE="Save Plot as SVG", FILTER="*.svg", FILTERINFO="SVG Files", EXTDEFAULT="svg"`)
	defer filedlg.Destroy()

	iup.Popup(filedlg, iup.CENTER, iup.CENTER)

	if filedlg.GetInt("STATUS") == -1 {
		return iup.DEFAULT
	}

	filename := filedlg.GetAttribute("VALUE")
	if !strings.HasSuffix(strings.ToLower(filename), ".svg") {
		filename += ".svg"
	}

	svg := iup.DrawGetSvg(plot)
	if svg == "" {
		iup.Message("Error", "Failed to generate SVG.")
		return iup.DEFAULT
	}

	err := os.WriteFile(filename, []byte(svg), 0644)
	if err != nil {
		iup.Message("Error", fmt.Sprintf("Failed to write SVG: %s", err))
		return iup.DEFAULT
	}

	_, w, h := iup.GetInt2(plot, "DRAWSIZE")
	iup.Message("Success", fmt.Sprintf("SVG saved to:\n%s\n\nSize: %dx%d", filename, w, h))

	return iup.DEFAULT
}
