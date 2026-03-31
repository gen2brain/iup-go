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
				iup.MenuItem("New").SetCallback("ACTION", iup.ActionFunc(itemCb)),
				iup.MenuItem("Open").SetCallback("ACTION", iup.ActionFunc(itemCb)),
				iup.MenuItem("Close").SetCallback("ACTION", iup.ActionFunc(itemCb)),
				iup.MenuSeparator(),
				iup.MenuItem("Page Setup").SetCallback("ACTION", iup.ActionFunc(itemCb)),
				iup.MenuItem("Print").SetCallback("ACTION", iup.ActionFunc(itemCb)),
				iup.MenuSeparator(),
				iup.MenuItem("Exit").SetCallback("ACTION", iup.ActionFunc(itemExitCb)),
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
