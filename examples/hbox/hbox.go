package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	h1 := iup.Hbox(
		iup.Fill(),
		iup.Button("1").SetAttribute("SIZE", "30x30"),
		iup.Button("2").SetAttribute("SIZE", "30x40"),
		iup.Button("3").SetAttribute("SIZE", "30x50"),
		iup.Fill(),
	).SetAttributes(`ALIGNMENT=ATOP, GAP=10, SIZE=200`)
	fr1 := iup.Frame(h1).SetAttribute("TITLE", "ALIGNMENT=ATOP, GAP=10, SIZE=200")

	h2 := iup.Hbox(
		iup.Fill(),
		iup.Button("1").SetAttribute("SIZE", "30x30"),
		iup.Button("2").SetAttribute("SIZE", "30x40"),
		iup.Button("3").SetAttribute("SIZE", "30x50"),
		iup.Fill(),
	).SetAttributes(`ALIGNMENT=ACENTER, GAP=20`)
	fr2 := iup.Frame(h2).SetAttribute("TITLE", "ALIGNMENT=ACENTER, GAP=20")

	h3 := iup.Hbox(
		iup.Fill(),
		iup.Button("1").SetAttribute("SIZE", "30x30"),
		iup.Button("2").SetAttribute("SIZE", "30x40"),
		iup.Button("3").SetAttribute("SIZE", "30x50"),
		iup.Fill(),
	).SetAttributes(`ALIGNMENT=ABOTTOM, SIZE=150`)
	fr3 := iup.Frame(h3).SetAttribute("TITLE", "ALIGNMENT=ABOTTOM, SIZE=150")

	dlg := iup.Dialog(
		iup.Hbox(
			fr1,
			fr2,
			fr3,
		),
	).SetAttribute("TITLE", "Hbox")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}
