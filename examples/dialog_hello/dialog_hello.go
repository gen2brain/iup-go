package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	fn := func(ih iup.Ihandle) int {
		return iup.CLOSE
	}

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Label("Very Long Text Label").SetAttributes(`EXPAND=YES, ALIGNMENT=ACENTER`),
			iup.Button("Quit").SetHandle("quitBtName").SetCallback("ACTION", iup.ActionFunc(fn)),
		).SetAttributes(`ALIGNMENT=ACENTER, MARGIN=10x10, GAP=10`),
	).SetAttributes(`TITLE="Dialog", DEFAULTESC=quitBtName`)

	iup.Show(dlg)
	iup.MainLoop()
}
