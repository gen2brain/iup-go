package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	lbl := iup.Label("Opacity: 255").SetHandle("lblOpacity").SetAttribute("SIZE", "80x")

	val := iup.Val("HORIZONTAL").SetAttributes("MIN=0, MAX=255, VALUE=255, EXPAND=HORIZONTAL")

	btn50 := iup.Button("50%").SetAttribute("SIZE", "40x")
	btn100 := iup.Button("100%").SetAttribute("SIZE", "40x")

	dlg := iup.Dialog(
		iup.Vbox(
			lbl,
			val,
			iup.Hbox(btn50, btn100),
		),
	).SetAttributes("TITLE=Dialog Opacity, SIZE=300x, MARGIN=10x10, GAP=5, OPACITY=255")

	iup.SetCallback(val, "VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		v := iup.GetInt(ih, "VALUE")
		iup.SetAttribute(dlg, "OPACITY", v)
		iup.GetHandle("lblOpacity").SetAttribute("TITLE", fmt.Sprintf("Opacity: %d", v))
		return iup.DEFAULT
	}))

	iup.SetCallback(btn50, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		iup.SetAttribute(dlg, "OPACITY", 128)
		iup.SetAttribute(val, "VALUE", 128)
		iup.GetHandle("lblOpacity").SetAttribute("TITLE", "Opacity: 128")
		return iup.DEFAULT
	}))

	iup.SetCallback(btn100, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		iup.SetAttribute(dlg, "OPACITY", 255)
		iup.SetAttribute(val, "VALUE", 255)
		iup.GetHandle("lblOpacity").SetAttribute("TITLE", "Opacity: 255")
		return iup.DEFAULT
	}))

	iup.Show(dlg)
	iup.MainLoop()
}
