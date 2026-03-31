package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	itemOpen := iup.MenuItem("Open").SetAttribute("KEY", "O")
	itemSave := iup.MenuItem("Save").SetAttribute("KEY", "S")
	itemUndo := iup.MenuItem("Undo").SetAttributes("KEY=U, ACTIVE=NO")
	itemExit := iup.MenuItem("Exit").SetAttribute("KEY", "x")

	itemExit.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int { return iup.CLOSE }))

	fileMenu := iup.Menu(itemOpen, itemSave, iup.MenuSeparator(), itemUndo, itemExit)
	subMenu := iup.Submenu("File", fileMenu)

	menu := iup.Menu(subMenu)
	menu.SetHandle("mymenu")

	dlg := iup.Dialog(iup.Canvas()).SetAttributes(map[string]string{
		"MENU":  "mymenu",
		"TITLE": "Menu",
		"SIZE":  "QUARTERxQUARTER",
	})

	iup.Show(dlg)
	iup.MainLoop()
}
