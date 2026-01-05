package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	var buttons []iup.Ihandle
	for i := 1; i <= 12; i++ {
		btn := iup.Button(fmt.Sprintf("Button %d", i))
		buttons = append(buttons, btn)
	}

	multibox := iup.MultiBox(buttons...)
	multibox.SetAttribute("ORIENTATION", "HORIZONTAL")
	multibox.SetAttribute("CHILDMINSPACE", "5")

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Label("MultiBox wraps children when space runs out."),
			iup.Label("Resize the window to see the wrapping behavior."),
			iup.Space().SetAttribute("SIZE", "x10"),
			iup.Frame(multibox).SetAttribute("TITLE", "MultiBox Container"),
		).SetAttributes("MARGIN=10x10, GAP=5"),
	).SetAttributes(`TITLE="MultiBox Example", SIZE=250x200`)

	iup.Show(dlg)
	iup.MainLoop()
}
