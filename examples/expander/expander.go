package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	bt := iup.Button("Button ONE")
	bt.SetAttributes(`EXPAND=YES, MINSIZE=30x, RASTERSIZE=30x`)

	exp := iup.Expander(bt)
	exp.SetAttribute("TITLE", "Expander Title")
	//exp.SetAttributes(`BARPOSITION=TOP, FGCOLOR="0 0 255", AUTOSHOW=YES, STATE=OPEN`) // try for BARPOSITION: LEFT, BOTTOM, RIGHT

	bt2 := iup.Button("Button TWO")

	vbox := iup.Vbox(exp, bt2).SetAttributes(`MARGIN=10x10, GAP=10`)

	dlg := iup.Dialog(vbox).SetAttributes(`TITLE="Expander"`)

	iup.Show(dlg)
	iup.MainLoop()
}
