package main

import (
	"github.com/gen2brain/iup-go/iup"
)

var img = []byte{
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2,
	1, 3, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 2,
	1, 3, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 2,
	1, 3, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 2,
	1, 3, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 2,
	1, 3, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 2,
	1, 3, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 2,
	1, 3, 5, 5, 6, 6, 6, 6, 6, 6, 5, 5, 6, 6, 5, 5, 6, 6, 5, 5, 6, 6, 6, 6, 6, 6, 6, 5, 5, 5, 4, 2,
	1, 3, 5, 5, 6, 6, 6, 6, 6, 6, 5, 5, 6, 6, 5, 5, 6, 6, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 5, 5, 4, 2,
	1, 3, 5, 5, 5, 5, 6, 6, 5, 5, 5, 5, 6, 6, 5, 5, 6, 6, 5, 5, 6, 6, 6, 5, 5, 6, 6, 6, 5, 5, 4, 2,
	1, 3, 5, 5, 5, 5, 6, 6, 5, 5, 5, 5, 6, 6, 5, 5, 6, 6, 5, 5, 6, 6, 5, 5, 5, 5, 6, 6, 5, 5, 4, 2,
	1, 3, 5, 5, 5, 5, 6, 6, 5, 5, 5, 5, 6, 6, 5, 5, 6, 6, 5, 5, 6, 6, 5, 5, 5, 5, 6, 6, 5, 5, 4, 2,
	1, 3, 5, 5, 5, 5, 6, 6, 5, 5, 5, 5, 6, 6, 5, 5, 6, 6, 5, 5, 6, 6, 5, 5, 5, 5, 6, 6, 5, 5, 4, 2,
	1, 3, 5, 5, 5, 5, 6, 6, 5, 5, 5, 5, 6, 6, 5, 5, 6, 6, 5, 5, 6, 6, 6, 5, 5, 6, 6, 6, 5, 5, 4, 2,
	1, 3, 5, 5, 5, 5, 6, 6, 5, 5, 5, 5, 6, 6, 5, 5, 6, 6, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 5, 5, 4, 2,
	1, 3, 5, 5, 5, 5, 6, 6, 5, 5, 5, 5, 6, 6, 5, 5, 6, 6, 5, 5, 6, 6, 6, 6, 6, 6, 6, 5, 5, 5, 4, 2,
	1, 3, 5, 5, 5, 5, 6, 6, 5, 5, 5, 5, 6, 6, 5, 5, 6, 6, 5, 5, 6, 6, 5, 5, 5, 5, 5, 5, 5, 5, 4, 2,
	1, 3, 5, 5, 5, 5, 6, 6, 5, 5, 5, 5, 6, 6, 5, 5, 6, 6, 5, 5, 6, 6, 5, 5, 5, 5, 5, 5, 5, 5, 4, 2,
	1, 3, 5, 5, 5, 5, 6, 6, 5, 5, 5, 5, 6, 6, 5, 5, 6, 6, 5, 5, 6, 6, 5, 5, 5, 5, 5, 5, 5, 5, 4, 2,
	1, 3, 5, 5, 5, 5, 6, 6, 5, 5, 5, 5, 6, 6, 5, 5, 6, 6, 5, 5, 6, 6, 5, 5, 5, 5, 5, 5, 5, 5, 4, 2,
	1, 3, 5, 5, 5, 5, 6, 6, 5, 5, 5, 5, 6, 6, 5, 5, 6, 6, 5, 5, 6, 6, 5, 5, 5, 5, 5, 5, 5, 5, 4, 2,
	1, 3, 5, 5, 6, 6, 6, 6, 6, 6, 5, 5, 6, 6, 6, 6, 6, 6, 5, 5, 6, 6, 5, 5, 5, 5, 5, 5, 5, 5, 4, 2,
	1, 3, 5, 5, 6, 6, 6, 6, 6, 6, 5, 5, 5, 6, 6, 6, 6, 5, 5, 5, 6, 6, 5, 5, 5, 5, 5, 5, 5, 5, 4, 2,
	1, 3, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 2,
	1, 3, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 2,
	1, 3, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 2,
	1, 3, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 2,
	1, 3, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 2,
	1, 3, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 2,
	1, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 2,
	1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
}

func main() {
	iup.Open()
	defer iup.Close()

	icon := iup.Image(32, 32, img).SetAttributes(map[string]string{
		"1": "255 255 255",
		"2": "000 000 000",
		"3": "226 226 226",
		"4": "128 128 128",
		"5": "192 192 192",
		"6": "000 000 255",
	})
	iup.SetHandle("icon", icon)

	quitBt := iup.Button("Quit").SetCallback("ACTION", iup.ActionFunc(quitCb))
	iup.SetHandle("quit", quitBt)

	options := iup.Menu(iup.Item("Quit").SetCallback("ACTION", iup.ActionFunc(quitCb)))
	submenu := iup.Submenu("File", options)
	menu := iup.Menu(submenu)
	iup.SetHandle("menu", menu)

	dialog := iup.Dialog(iup.Vbox(quitBt))
	iup.SetAttribute(dialog, "TITLE", "Dialog")
	iup.SetAttribute(dialog, "MENU", "menu")
	iup.SetAttribute(dialog, "CURSOR", "CROSS")
	iup.SetAttribute(dialog, "ICON", "icon")
	iup.SetAttribute(dialog, "DEFAULTESC", "quit")

	iup.Show(dialog)
	iup.MainLoop()
}

func quitCb(ih iup.Ihandle) int {
	return iup.CLOSE
}
