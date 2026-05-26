package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	valV := iup.Val("VERTICAL").SetAttribute("SHOWTICKS", "5")
	valH := iup.Val("HORIZONTAL")
	lblV := iup.Label("VALUE=").SetHandle("lblV").SetAttribute("SIZE", "50x")
	lblH := iup.Label("VALUE=").SetHandle("lblH").SetAttribute("SIZE", "50x")

	dlg := iup.Dialog(
		iup.Hbox(
			iup.Hbox(valV, lblV).SetAttribute("ALIGNMENT", "ACENTER"),
			iup.Vbox(valH, lblH).SetAttribute("ALIGNMENT", "ACENTER"),
		),
	).SetAttributes("TITLE=Val, MARGIN=10x10")

	iup.SetCallback(valV, "VALUECHANGED_CB", iup.ValueChangedFunc(valueChanged))
	iup.SetCallback(valH, "VALUECHANGED_CB", iup.ValueChangedFunc(valueChanged))

	iup.Show(dlg)
	iup.MainLoop()
}

func valueChanged(ih iup.Ihandle) int {
	buffer := fmt.Sprintf("VALUE=%.2g", ih.GetFloat("VALUE"))

	t := ih.GetAttribute("ORIENTATION")
	switch string(t[0]) {
	case "V":
		iup.GetHandle("lblV").SetAttribute("TITLE", buffer)
	case "H":
		iup.GetHandle("lblH").SetAttribute("TITLE", buffer)
	}
	return iup.DEFAULT
}
