package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	var labels []iup.Ihandle
	for i := 1; i <= 20; i++ {
		lbl := iup.Label(fmt.Sprintf("Label %d", i))
		if i == 10 {
			iup.SetHandle("LABEL10", lbl)
			lbl.SetAttribute("BGCOLOR", "255 0 0")
		}
		labels = append(labels, lbl)
	}

	vbox := iup.Vbox(labels...).SetAttributes(`MARGIN=10x10, GAP=5`)

	scrollbox := iup.ScrollBox(vbox).SetAttributes(`EXPAND=YES`)
	iup.SetHandle("SCROLLBOXTEST", scrollbox)

	btnTop := iup.Button("Top").SetAttributes(`PADDING=5x5`)
	btnBottom := iup.Button("Bottom").SetAttributes(`PADDING=5x5`)
	btnChild := iup.Button("Label 10").SetAttributes(`PADDING=5x5`)
	btnPos := iup.Button("0,100").SetAttributes(`PADDING=5x5`)

	iup.SetCallback(btnTop, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		iup.GetHandle("SCROLLBOXTEST").SetAttribute("SCROLLTO", "TOP")
		return iup.DEFAULT
	}))

	iup.SetCallback(btnBottom, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		iup.GetHandle("SCROLLBOXTEST").SetAttribute("SCROLLTO", "BOTTOM")
		return iup.DEFAULT
	}))

	iup.SetCallback(btnChild, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		iup.GetHandle("SCROLLBOXTEST").SetAttribute("SCROLLTOCHILD", "LABEL10")
		return iup.DEFAULT
	}))

	iup.SetCallback(btnPos, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		iup.GetHandle("SCROLLBOXTEST").SetAttribute("SCROLLTO", "0,100")
		return iup.DEFAULT
	}))

	buttons := iup.Hbox(btnTop, btnBottom, btnChild, btnPos).SetAttributes(`MARGIN=10x5, GAP=5`)

	dlg := iup.Dialog(
		iup.Vbox(buttons, scrollbox),
	).SetAttributes(`TITLE=ScrollBox, SIZE=QUARTERxQUARTER`)

	iup.Show(dlg)
	iup.MainLoop()
}
