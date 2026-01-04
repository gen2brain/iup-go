package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	btn1 := iup.Button("Button 1")
	btn1.SetAttribute("CX", "10")
	btn1.SetAttribute("CY", "10")

	btn2 := iup.Button("Button 2")
	btn2.SetAttribute("CX", "100")
	btn2.SetAttribute("CY", "50")

	btn3 := iup.Button("Button 3")
	btn3.SetAttribute("CX", "50")
	btn3.SetAttribute("CY", "100")

	btn4 := iup.Button("Overlapping")
	btn4.SetAttribute("CX", "80")
	btn4.SetAttribute("CY", "80")
	btn4.SetAttribute("BGCOLOR", "255 200 200")

	lbl1 := iup.Label("X=10, Y=10")
	lbl1.SetAttribute("CX", "10")
	lbl1.SetAttribute("CY", "35")

	lbl2 := iup.Label("X=100, Y=50")
	lbl2.SetAttribute("CX", "100")
	lbl2.SetAttribute("CY", "75")

	lbl3 := iup.Label("X=50, Y=100")
	lbl3.SetAttribute("CX", "50")
	lbl3.SetAttribute("CY", "125")

	cbox := iup.Cbox(btn1, lbl1, btn2, lbl2, btn3, lbl3, btn4).SetAttribute("SIZE", "250x180")

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Label("Cbox allows absolute positioning using CX and CY attributes."),
			iup.Label("Notice Button 4 overlaps Button 2 and Button 3."),
			iup.Space().SetAttribute("SIZE", "x10"),
			iup.Frame(cbox).SetAttribute("TITLE", "Cbox Container"),
		).SetAttributes("MARGIN=10x10, GAP=5"),
	).SetAttribute("TITLE", "Cbox Example")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}
