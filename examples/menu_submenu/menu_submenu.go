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
				iup.MenuItem("Exit").SetCallback("ACTION", iup.ActionFunc(itemExitCb)),
			),
		),
		iup.Submenu("Edit",
			iup.Menu(
				iup.MenuItem("Copy").SetCallback("ACTION", iup.ActionFunc(itemCb)),
				iup.MenuItem("Paste").SetCallback("ACTION", iup.ActionFunc(itemCb)),
				iup.MenuSeparator(),
				iup.Submenu("Create",
					iup.Menu(
						iup.MenuItem("Line").SetCallback("ACTION", iup.ActionFunc(itemCb)),
						iup.MenuItem("Circle").SetCallback("ACTION", iup.ActionFunc(itemCb)),
						iup.Submenu("Triangle",
							iup.Menu(
								iup.MenuItem("Equilateral").SetCallback("ACTION", iup.ActionFunc(itemCb)),
								iup.MenuItem("Isosceles").SetCallback("ACTION", iup.ActionFunc(itemCb)),
								iup.MenuItem("Scalene").SetCallback("ACTION", iup.ActionFunc(itemCb)),
							),
						),
					),
				),
			),
		),
		iup.Submenu("Help",
			iup.Menu(
				iup.MenuItem("Help").SetCallback("ACTION", iup.ActionFunc(itemHelpCb)),
			),
		),
	).SetHandle("menu")

	dlg := iup.Dialog(
		iup.Text().SetAttributes(`VALUE="This text is here only to compose", EXPAND=YES`),
	).SetAttributes(map[string]string{
		"TITLE": "Submenu",
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

func itemHelpCb(ih iup.Ihandle) int {
	iup.Message("Help", "Only Help and Exit items perform an operation")
	return iup.DEFAULT
}

func itemExitCb(ih iup.Ihandle) int {
	return iup.CLOSE
}
