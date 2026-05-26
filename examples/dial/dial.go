package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	lblH := iup.Label("0").SetHandle("lblH").SetAttributes("ALIGNMENT=ACENTER, SIZE=100x10")
	lblV := iup.Label("0").SetHandle("lblV").SetAttributes("ALIGNMENT=ACENTER, SIZE=100x10")

	dialV := iup.Dial("VERTICAL").SetAttributes("RASTERSIZE=100x100")
	dialH := iup.Dial("HORIZONTAL").SetAttributes("DENSITY=0.3")

	dialV.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(dialVCb))
	dialH.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(dialHCb))

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Vbox(
				dialV,
				lblV,
			),
			iup.Vbox(
				dialH,
				lblH,
			),
		).SetAttributes("MARGIN=10x10, GAP=5"),
	).SetAttributes(`TITLE="Dial"`)

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}

func dialVCb(ih iup.Ihandle) int {
	iup.GetHandle("lblV").SetAttribute("TITLE", ih.GetAttribute("VALUE"))
	return iup.DEFAULT
}

func dialHCb(ih iup.Ihandle) int {
	iup.GetHandle("lblH").SetAttribute("TITLE", ih.GetAttribute("VALUE"))
	return iup.DEFAULT
}
