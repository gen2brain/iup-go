package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	gauge := iup.Gauge().SetHandle("gauge").SetAttribute("SHOWTEXT", "YES")

	dlg := iup.Dialog(
		gauge,
	).SetAttribute("TITLE", "Gauge")

	iup.SetFunction("IDLE_ACTION", iup.IdleFunc(idleFunction))

	iup.Show(dlg)
	iup.MainLoop()
}

func idleFunction() int {
	gauge := iup.GetHandle("gauge")
	value := gauge.GetFloat("VALUE")
	value = value + 0.00001
	if value > 1.0 {
		value = 0
	}
	gauge.SetAttribute("VALUE", value)
	return iup.DEFAULT
}
