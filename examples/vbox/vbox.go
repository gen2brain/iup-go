package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	v1 := iup.Vbox(
		iup.Fill(),
		iup.Button("1").SetAttribute("SIZE", "20x30"),
		iup.Button("2").SetAttribute("SIZE", "30x30"),
		iup.Button("3").SetAttribute("SIZE", "40x30"),
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
		iup.Button("1").SetAttribute("SIZE", "20x30"),
		iup.Button("2").SetAttribute("SIZE", "30x30"),
		iup.Button("3").SetAttribute("SIZE", "40x30"),
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
		iup.Button("1").SetAttribute("SIZE", "20x30"),
		iup.Button("2").SetAttribute("SIZE", "30x30"),
		iup.Button("3").SetAttribute("SIZE", "40x30"),
		iup.Fill(),
	).SetAttributes(`ALIGNMENT=ARIGHT, SIZE=20`)
	fr3 := iup.Frame(
		iup.Hbox(
			iup.Fill(),
			v3,
			iup.Fill(),
		),
	).SetAttribute("TITLE", "ALIGNMENT=ARIGHT, SIZE=20")

	dlg := iup.Dialog(
		iup.Vbox(
			fr1,
			fr2,
			fr3,
		),
	).SetAttributes(`TITLE="Vbox", SIZE=QUARTER`)

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}
