package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Frame(
				iup.Hbox(
					iup.Button("A"),
					iup.Space().SetAttribute("SIZE", "20x"),
					iup.Button("B"),
					iup.Space().SetAttribute("SIZE", "20x"),
					iup.Button("C"),
				),
			).SetAttribute("TITLE", "Space (fixed 20px gaps)"),

			iup.Frame(
				iup.Hbox(
					iup.Button("A"),
					iup.Fill(),
					iup.Button("B"),
					iup.Fill(),
					iup.Button("C"),
				),
			).SetAttribute("TITLE", "Fill (expands to fill available space)"),

			iup.Frame(
				iup.Hbox(
					iup.Button("A"),
					iup.Space().SetAttribute("SIZE", "50x"),
					iup.Button("B"),
					iup.Fill(),
					iup.Button("C"),
				),
			).SetAttribute("TITLE", "Mixed (50px Space + Fill)"),

			iup.Label("Resize the window to see the difference:"),
			iup.Label("- Space keeps fixed size"),
			iup.Label("- Fill expands/contracts"),
		).SetAttributes("MARGIN=10x10, GAP=10"),
	).SetAttribute("TITLE", "Space vs Fill")

	iup.Show(dlg)
	iup.MainLoop()
}
