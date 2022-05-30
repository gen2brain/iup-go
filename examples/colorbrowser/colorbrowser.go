package main

import (
	"github.com/gen2brain/iup-go/iup"
)

var textRed, textGreen, textBlue iup.Ihandle

func main() {
	iup.Open()
	defer iup.Close()

	textRed = iup.Text()
	textGreen = iup.Text()
	textBlue = iup.Text()

	cbrowser := iup.ColorBrowser()
	cbrowser.SetCallback("DRAG_CB", iup.DragFunc(dragCb))
	cbrowser.SetCallback("CHANGE_CB", iup.ChangeFunc(func(ih iup.Ihandle, r, g, b uint8) int {
		dragCb(ih, r, g, b)
		return iup.DEFAULT
	}))

	vbox := iup.Hbox(
		iup.Fill(),
		textRed,
		iup.Fill(),
		textGreen,
		iup.Fill(),
		textBlue,
		iup.Fill(),
	)

	hbox := iup.Hbox(cbrowser, iup.Fill(), vbox)

	dlg := iup.Dialog(hbox)
	iup.SetAttribute(dlg, "TITLE", "ColorBrowser")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}

func dragCb(ih iup.Ihandle, r, g, b uint8) int {
	textRed.SetAttribute("VALUE", int(r))
	textGreen.SetAttribute("VALUE", int(g))
	textBlue.SetAttribute("VALUE", int(b))

	// Gives IUP a time to call the OS and update the text field
	iup.LoopStep()

	return iup.DEFAULT
}
