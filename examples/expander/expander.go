package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	bt := iup.Button("Button ONE")
	bt.SetAttributes(`EXPAND=HORIZONTAL, MINSIZE=30x, RASTERSIZE=30x`)

	exp := iup.Expander(bt)
	exp.SetAttribute("TITLE", "Expander Title")
	//exp.SetAttributes(`BARPOSITION=TOP, FGCOLOR="0 0 255", AUTOSHOW=YES, STATE=OPEN`) // try for BARPOSITION: LEFT, BOTTOM, RIGHT

	status := iup.Label("Expander is open").SetHandle("status")

	exp.SetCallback("OPENCLOSE_CB", iup.OpenCloseFunc(func(_ iup.Ihandle, state int) int {
		action := "closing"
		if state != 0 {
			action = "opening"
		}
		iup.GetHandle("status").SetAttribute("TITLE", "Expander "+action)
		return iup.DEFAULT
	}))

	bt2 := iup.Button("Button TWO")

	vbox := iup.Vbox(exp, bt2, status).SetAttributes(`MARGIN=10x10, GAP=10`)

	dlg := iup.Dialog(vbox).SetAttributes(`TITLE="Expander"`)

	iup.Show(dlg)
	iup.MainLoop()
}
