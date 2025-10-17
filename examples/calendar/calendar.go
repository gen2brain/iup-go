package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	cal := iup.Calendar().SetAttributes(`EXPAND=YES, VALUE="2024-06-15"`)

	dlg := iup.Dialog(
		iup.Frame(cal),
	).SetAttribute("TITLE", "Calendar")

	iup.Show(dlg)
	iup.MainLoop()
}
