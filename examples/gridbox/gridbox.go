package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	fr := iup.Frame(
		iup.GridBox(
			iup.Label(""),
			iup.Label("col1").SetAttributes("FONTSTYLE=Bold"),
			iup.Label("col2").SetAttributes("FONTSTYLE=Bold"),

			iup.Label("lin1").SetAttributes("FONTSTYLE=Bold"),
			iup.Label("lbl").SetAttributes("XSIZE=50x12"),
			iup.Button("button").SetAttributes("XSIZE=50"),

			iup.Label("lin2").SetAttributes("FONTSTYLE=Bold"),
			iup.Label("label").SetAttributes("XSIZE=x12"),
			iup.Button("button").SetAttributes("XEXPAND=Horizontal"),

			iup.Label("lin3").SetAttributes("FONTSTYLE=Bold"),
			iup.Label("label large").SetAttributes("XSIZE=x12"),
			iup.Button("button large"),
		).SetAttributes(map[string]string{
			"SIZECOL": "2",
			"SIZELIN": "3",

			//"HOMOGENEOUSLIN": "Yes",
			//"HOMOGENEOUSCOL": "Yes",
			//"EXPANDCHILDREN": "HORIZONTAL",
			//"NORMALIZESIZE":  "BOTH",
			//"NORMALIZESIZE":  "HORIZONTAL",

			"NUMDIV": "3",

			//"ORIENTATION": "VERTICAL",
			//"NUMDIV": "2",
			//"NUMDIV": "AUTO",

			"ALIGNMENTLIN": "ACENTER",
			"ALIGNMENTCOL": "ARIGHT",
			"NCMARGIN":     "3x2",
			"NCGAPLIN":     "1",
			"NCGAPCOL":     "2",
		}),
	)
	gb := iup.GetChild(fr, 0)

	fit := iup.Button("FITTOCHILDREN C1 and L2")
	fit.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		gb.SetAttribute("FITTOCHILDREN", "C1")
		gb.SetAttribute("FITTOCHILDREN", "L2")
		iup.Refresh(gb)
		return iup.DEFAULT
	}))

	dlg := iup.Dialog(
		iup.Vbox(
			fr,
			fit,
		).SetAttributes("NMARGIN=5x5, NGAP=5"),
	)

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}
