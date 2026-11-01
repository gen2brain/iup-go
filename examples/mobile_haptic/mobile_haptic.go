package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	status := iup.Label("").SetAttributes("EXPAND=HORIZONTAL")

	buttons := iup.Vbox().SetAttributes("GAP=6, NORMALIZESIZE=HORIZONTAL")
	for _, value := range []string{"LIGHT", "MEDIUM", "HEAVY", "SELECTION"} {
		btn := iup.Button(value).SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
			iup.SetGlobal("HAPTIC", value)
			status.SetAttribute("TITLE", "HAPTIC="+value)
			return iup.DEFAULT
		}))
		iup.Append(buttons, btn)
	}

	dlg := iup.Dialog(
		iup.Vbox(buttons, status).SetAttributes("MARGIN=10x10, GAP=10"),
	).SetAttribute("TITLE", "Mobile Haptic")

	iup.Show(dlg)
	iup.MainLoop()
}
