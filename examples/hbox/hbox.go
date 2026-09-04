package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	h1 := iup.Hbox(
		iup.Fill(),
		iup.Button("1"),
		iup.Button("2\nlines"),
		iup.Button("3\nlines\nhigh"),
		iup.Fill(),
	).SetAttributes(`ALIGNMENT=ATOP, GAP=10, SIZE=200`)
	fr1 := iup.Frame(h1).SetAttribute("TITLE", "ALIGNMENT=ATOP, GAP=10, SIZE=200")

	h2 := iup.Hbox(
		iup.Fill(),
		iup.Button("1"),
		iup.Button("2\nlines"),
		iup.Button("3\nlines\nhigh"),
		iup.Fill(),
	).SetAttributes(`ALIGNMENT=ACENTER, GAP=20`)
	fr2 := iup.Frame(h2).SetAttribute("TITLE", "ALIGNMENT=ACENTER, GAP=20")

	h3 := iup.Hbox(
		iup.Fill(),
		iup.Button("1"),
		iup.Button("2\nlines"),
		iup.Button("3\nlines\nhigh"),
		iup.Fill(),
	).SetAttributes(`ALIGNMENT=ABOTTOM, SIZE=150`)
	fr3 := iup.Frame(h3).SetAttribute("TITLE", "ALIGNMENT=ABOTTOM, SIZE=150")

	h4 := iup.Hbox(
		iup.Button("EXPANDWEIGHT=0.5").SetAttributes("EXPAND=HORIZONTAL, EXPANDWEIGHT=0.5"),
		iup.Button("EXPANDWEIGHT=1").SetAttributes("EXPAND=HORIZONTAL, EXPANDWEIGHT=1"),
		iup.Button("EXPANDWEIGHT=1.5").SetAttributes("EXPAND=HORIZONTAL, EXPANDWEIGHT=1.5"),
	).SetAttributes("NCGAP=2, NCMARGIN=3x1")
	fr4 := iup.Frame(h4).SetAttributes(`TITLE="EXPANDWEIGHT 0.5, 1 and 1.5 with NCGAP=2, NCMARGIN=3x1", EXPAND=HORIZONTAL`)

	h5 := iup.Hbox(
		iup.Button("CGAP=4"),
		iup.Button("CMARGIN=6x2"),
	).SetAttributes("CGAP=4, CMARGIN=6x2")
	fr5 := iup.Frame(h5).SetAttribute("TITLE", "CGAP=4, CMARGIN=6x2")

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Hbox(fr1, fr2, fr3),
			fr4,
			fr5,
		),
	).SetAttribute("TITLE", "Hbox")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}
