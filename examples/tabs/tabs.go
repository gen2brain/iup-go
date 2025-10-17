package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	vbox1 := iup.Vbox(iup.Label("Inside Tab A"), iup.Button("Button A"))
	vbox2 := iup.Vbox(iup.Label("Inside Tab B"), iup.Button("Button B"))

	vbox1.SetAttribute("TABTITLE", "Tab A")
	vbox2.SetAttribute("TABTITLE", "Tab B")

	tabs1 := iup.Tabs(vbox1, vbox2)

	tabs1.SetAttribute("SIZE", "150x100")
	tabs1.SetAttribute("SHOWCLOSE", "YES")

	vbox1 = iup.Vbox(iup.Label("Inside Tab C"), iup.Button("Button C"))
	vbox2 = iup.Vbox(iup.Label("Inside Tab D"), iup.Button("Button D"))

	vbox1.SetAttribute("TABTITLE", "Tab C")
	vbox2.SetAttribute("TABTITLE", "Tab D")

	tabs2 := iup.Tabs(vbox1, vbox2)

	tabs2.SetAttribute("SIZE", "150x100")
	tabs2.SetAttribute("TABTYPE", "LEFT")

	box := iup.Hbox(tabs1, tabs2).SetAttributes("MARGIN=10x10, GAP=10")
	dlg := iup.Dialog(box).SetAttributes(`TITLE="Tabs", SIZE=350x150`)

	iup.Show(dlg)
	iup.MainLoop()
}
