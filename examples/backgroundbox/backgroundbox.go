package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	btn := iup.BackgroundBox(
		iup.Frame(
			iup.Vbox(
				iup.Button("This button does nothing"),
				iup.Text(),
			).SetAttributes(`MARGIN=0x0`),
		),
	)

	//btn.SetAttribute("BGCOLOR", "0 128 0")
	btn.SetAttribute("BORDER", "Yes")

	dlg := iup.Dialog(iup.Vbox(btn)).SetAttributes(`MARGIN=10x10, GAP=10, TITLE="BackgroundBox"`)

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}
