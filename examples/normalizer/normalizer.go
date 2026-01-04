package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	btn1a := iup.Button("OK")
	btn2a := iup.Button("Cancel")
	btn3a := iup.Button("Apply Changes")

	btn1b := iup.Button("OK")
	btn2b := iup.Button("Cancel")
	btn3b := iup.Button("Apply Changes")

	iup.Normalizer(btn1b, btn2b, btn3b).SetAttribute("NORMALIZE", "HORIZONTAL")

	btn1c := iup.Button("Short")
	btn2c := iup.Button("Medium Text")
	btn3c := iup.Button("Very Long Button Text")

	iup.Normalizer(btn1c, btn2c, btn3c).SetAttribute("NORMALIZE", "BOTH")

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Frame(
				iup.Hbox(btn1a, btn2a, btn3a).SetAttribute("GAP", "5"),
			).SetAttribute("TITLE", "Without Normalizer"),

			iup.Frame(
				iup.Hbox(btn1b, btn2b, btn3b).SetAttribute("GAP", "5"),
			).SetAttribute("TITLE", "With Normalizer (HORIZONTAL)"),

			iup.Frame(
				iup.Vbox(btn1c, btn2c, btn3c).SetAttribute("GAP", "5"),
			).SetAttribute("TITLE", "With Normalizer (BOTH)"),

			iup.Label("Normalizer makes all buttons the same size."),
			iup.Label("HORIZONTAL: same width"),
			iup.Label("VERTICAL: same height"),
			iup.Label("BOTH: same width and height"),
		).SetAttributes("MARGIN=10x10, GAP=10"),
	).SetAttribute("TITLE", "Normalizer Example")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}
