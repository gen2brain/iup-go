package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	v1 := iup.Vbox(
		iup.Fill(),
		iup.Button("1"),
		iup.Button("22 wider"),
		iup.Button("333 widest button"),
		iup.Fill(),
	).SetAttributes(`ALIGNMENT=ALEFT, GAP=10`)
	fr1 := iup.Frame(
		iup.Hbox(
			iup.Fill(),
			v1,
			iup.Fill(),
		),
	).SetAttribute("TITLE", "ALIGNMENT=ALEFT, GAP=10")

	v2 := iup.Vbox(
		iup.Fill(),
		iup.Button("1"),
		iup.Button("22 wider"),
		iup.Button("333 widest button"),
		iup.Fill(),
	).SetAttributes(`ALIGNMENT=ACENTER, MARGIN=15`)
	fr2 := iup.Frame(
		iup.Hbox(
			iup.Fill(),
			v2,
			iup.Fill(),
		),
	).SetAttribute("TITLE", "ALIGNMENT=ACENTER, MARGIN=15")

	v3 := iup.Vbox(
		iup.Fill(),
		iup.Button("1"),
		iup.Button("22 wider"),
		iup.Button("333 widest button"),
		iup.Fill(),
	).SetAttributes(`ALIGNMENT=ARIGHT, SIZE=20`)
	fr3 := iup.Frame(
		iup.Hbox(
			iup.Fill(),
			v3,
			iup.Fill(),
		),
	).SetAttribute("TITLE", "ALIGNMENT=ARIGHT, SIZE=20")

	v4 := iup.Vbox(
		iup.Button("EXPANDWEIGHT=0.5").SetAttributes("EXPAND=VERTICAL, EXPANDWEIGHT=0.5"),
		iup.Button("EXPANDWEIGHT=1").SetAttributes("EXPAND=VERTICAL, EXPANDWEIGHT=1"),
		iup.Button("EXPANDWEIGHT=1.5").SetAttributes("EXPAND=VERTICAL, EXPANDWEIGHT=1.5"),
	).SetAttributes("NCGAP=1, NCMARGIN=2x1")
	fr4 := iup.Frame(v4).SetAttributes(`TITLE="EXPANDWEIGHT 0.5, 1, 1.5", EXPAND=VERTICAL`)

	v5 := iup.Vbox(
		iup.Button("CGAP=2"),
		iup.Button("CMARGIN=4x2"),
	).SetAttributes("CGAP=2, CMARGIN=4x2")
	fr5 := iup.Frame(v5).SetAttribute("TITLE", "CGAP=2, CMARGIN=4x2")

	dlg := iup.Dialog(
		iup.Hbox(
			iup.Vbox(fr1, fr2, fr3),
			fr4,
			fr5,
		),
	).SetAttribute("TITLE", "Vbox")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}
