package main

import (
	"github.com/gen2brain/iup-go/iup"
)

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
			"MARGIN":       "10x10",
			"GAPLIN":       "5",
			"GAPCOL":       "5",
		}),
	)

	dlg := iup.Dialog(
		iup.Hbox(
			fr,
		),
	)

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}
