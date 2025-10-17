package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	dlg := iup.GlobalsDialog()
	defer dlg.Destroy()

	iup.Popup(dlg, iup.CENTER, iup.CENTER)
}
