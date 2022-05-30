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
				iup.Hbox(iup.Button("OK"), iup.Fill()),
			).SetAttribute("TITLE", "Left aligned"),
			iup.Frame(
				iup.Hbox(iup.Fill(), iup.Button("OK"), iup.Fill()),
			).SetAttribute("TITLE", "Centered"),
			iup.Frame(
				iup.Hbox(iup.Fill(), iup.Button("OK")),
			).SetAttribute("TITLE", "Right aligned"),
		),
	).SetAttributes(`TITLE="Fill", SIZE=120`)

	iup.Show(dlg)
	iup.MainLoop()
}
