package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

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

	iup.SetCallback(valV, "BUTTON_PRESS_CB", iup.ButtonPressFunc(buttonPress))
	iup.SetCallback(valV, "BUTTON_RELEASE_CB", iup.ButtonReleaseFunc(buttonRelease))
	iup.SetCallback(valV, "MOUSEMOVE_CB", iup.MouseMoveFunc(mouseMove))

	iup.SetCallback(valH, "BUTTON_PRESS_CB", iup.ButtonPressFunc(buttonPress))
	iup.SetCallback(valH, "BUTTON_RELEASE_CB", iup.ButtonReleaseFunc(buttonRelease))
	iup.SetCallback(valH, "MOUSEMOVE_CB", iup.MouseMoveFunc(mouseMove))

	iup.Show(dlg)
	iup.MainLoop()
}

func buttonPress(ih iup.Ihandle, a float64) int {
	t := ih.GetAttribute("ORIENTATION")
	switch string(t[0]) {
	case "V":
		iup.GetHandle("lblV").SetAttribute("FGCOLOR", "255 0 0")
	case "H":
		iup.GetHandle("lblH").SetAttribute("FGCOLOR", "255 0 0")
	}

	mouseMove(ih, a)

	return iup.DEFAULT
}

func buttonRelease(ih iup.Ihandle, a float64) int {
	t := ih.GetAttribute("ORIENTATION")
	switch string(t[0]) {
	case "V":
		iup.GetHandle("lblV").SetAttribute("FGCOLOR", "0 0 0")
	case "H":
		iup.GetHandle("lblH").SetAttribute("FGCOLOR", "0 0 0")
	}

	mouseMove(ih, a)

	return iup.DEFAULT
}

func mouseMove(ih iup.Ihandle, a float64) int {
	buffer := fmt.Sprintf("VALUE=%.2g", a)

	t := ih.GetAttribute("ORIENTATION")
	switch string(t[0]) {
	case "V":
		iup.GetHandle("lblV").SetAttribute("TITLE", buffer)
	case "H":
		iup.GetHandle("lblH").SetAttribute("TITLE", buffer)
	}
	return iup.DEFAULT
}
