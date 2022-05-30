package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	frame := iup.Frame(
		iup.List().SetAttributes(`DROPDOWN=YES, 1=Test, 2=XXX, VALUE=1`),
	).SetAttribute("TITLE", "List")
	text := iup.Text().SetAttributes(`EXPAND=YES, VALUE="Enter your text here"`)
	lbl := iup.Label("This element is a label")
	btn := iup.Button("This button does nothing")

	frame.SetHandle("frame")
	text.SetHandle("text")
	lbl.SetHandle("lbl")
	btn.SetHandle("btn")

	zbox := iup.Zbox(frame, text, lbl, btn).SetAttributes(`ALIGNMENT=ACENTER, VALUE=text`)
	zbox.SetHandle("zbox")

	list := iup.List().SetAttributes(`1=frame, 2=text, 3=lbl, 4=btn, VALUE=2`)
	frm := iup.Frame(
		iup.Hbox(
			list,
		),
	).SetAttribute("TITLE", "Select an element")

	dlg := iup.Dialog(
		iup.Vbox(
			frm,
			zbox,
		),
	).SetAttributes(`TITLE="Zbox", MARGIN=10x10, GAP=10`)

	list.SetCallback("ACTION", iup.ListActionFunc(listCb))

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}

func listCb(ih iup.Ihandle, text string, item, state int) int {
	if state == 1 {
		iup.GetHandle("zbox").SetAttribute("VALUE", text)
	}
	return iup.DEFAULT
}
