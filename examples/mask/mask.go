package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	text := iup.Text()
	text.SetAttribute("MASK", "/d*")
	text.SetAttribute("SIZE", "100x")
	text.SetAttribute("EXPAND", "HORIZONTAL")

	dlg := iup.Dialog(text).SetAttribute("TITLE", "Text MASK")

	iup.Show(dlg)
	iup.MainLoop()
}
