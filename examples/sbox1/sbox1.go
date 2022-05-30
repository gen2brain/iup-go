package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	btn := iup.Button("Button").SetAttribute("EXPAND", "YES")

	box := iup.Sbox(btn).SetAttribute("DIRECTION", "SOUTH")

	ml := iup.MultiLine().SetAttributes("EXPAND=YES, VISIBLELINES=5")

	vbox := iup.Vbox(box, ml)

	lbl := iup.Label("Label").SetAttribute("EXPAND", "VERTICAL")

	dlg := iup.Dialog(iup.Hbox(vbox, lbl)).SetAttributes(`TITLE="Sbox", MARGIN=10x10, GAP=10`)

	iup.Show(dlg)
	iup.MainLoop()
}
