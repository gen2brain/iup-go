package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	tree := iup.Tree().SetAttributes(`
		SIZE=80x80,
		FONT=COURIER_NORMAL_10,
		TITLE=Figures,
		ADDBRANCH=3D,
		ADDBRANCH=2D,
		ADDLEAF1=trapeze,
		ADDBRANCH1=parallelogram,
		ADDLEAF2=diamond,
		ADDLEAF2=square,
		ADDBRANCH4=triangle,
		ADDLEAF5=scalenus,
		ADDLEAF5=isoceles,
		ADDLEAF5=equilateral,
		VALUE=6,
		ADDEXPANDED=NO,
		EXPAND=YES,
	`)

	sbox := iup.Sbox(tree).SetAttribute("DIRECTION", "EAST")

	cv := iup.Canvas().SetAttribute("EXPAND", "YES")
	ml := iup.MultiLine().SetAttribute("EXPAND", "YES")
	sbox2 := iup.Sbox(ml).SetAttribute("DIRECTION", "WEST")
	vbox := iup.Vbox(sbox, cv, sbox2)

	lbl := iup.Label("This is a label").SetAttribute("EXPAND", "NO")
	sbox3 := iup.Sbox(lbl).SetAttribute("DIRECTION", "NORTH")

	dlg := iup.Dialog(iup.Vbox(vbox, sbox3)).SetAttribute("TITLE", "Sbox")

	iup.Show(dlg)
	iup.MainLoop()
}
