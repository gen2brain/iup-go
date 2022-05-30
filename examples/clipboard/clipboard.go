package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	clipboard := iup.Clipboard()
	defer clipboard.Destroy()

	text := iup.Text().SetAttribute("EXPAND", "HORIZONTAL")

	fn := func(ih iup.Ihandle) int {
		text.SetAttribute("VALUE", clipboard.GetAttribute("TEXT"))
		return iup.DEFAULT
	}

	dlg := iup.Dialog(
		iup.Vbox(
			text,
			iup.Button("Paste from clipboard").SetCallback("ACTION", iup.ActionFunc(fn)),
		).SetAttributes(`ALIGNMENT=ACENTER, MARGIN=10x10, GAP=10`),
	).SetAttributes(`TITLE="Clipboard"`)

	iup.Show(dlg)
	iup.MainLoop()
}
