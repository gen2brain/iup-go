package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	iup.Menu(
		iup.Submenu("File",
			iup.Menu(
				iup.Item("New").SetCallback("ACTION", iup.ActionFunc(itemCb)),
				iup.Item("Open").SetCallback("ACTION", iup.ActionFunc(itemCb)),
				iup.Item("Close").SetCallback("ACTION", iup.ActionFunc(itemCb)),
				iup.Separator(),
				iup.Item("Page Setup").SetCallback("ACTION", iup.ActionFunc(itemCb)),
				iup.Item("Print").SetCallback("ACTION", iup.ActionFunc(itemCb)),
				iup.Separator(),
				iup.Item("Exit").SetCallback("ACTION", iup.ActionFunc(itemExitCb)),
			),
		),
	).SetHandle("menu")

	dlg := iup.Dialog(
		iup.Text().SetAttributes(`VALUE="This text is here only to compose", EXPAND=HORIZONTAL`),
	).SetAttributes(map[string]string{
		"TITLE": "Separator",
		"SIZE":  "QUARTERxEIGHTH",
		"MENU":  "menu",
	})

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}

func itemCb(ih iup.Ihandle) int {
	fmt.Printf("Item %q selected\n", ih.GetAttribute("TITLE"))
	return iup.DEFAULT
}

func itemExitCb(ih iup.Ihandle) int {
	return iup.CLOSE
}
