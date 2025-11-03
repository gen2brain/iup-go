package main

import (
	"image/png"
	"log"
	"os"

	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	file, err := os.Open("icon.png")
	if err != nil {
		log.Fatalln(err)
	}

	image, err := png.Decode(file)
	if err != nil {
		log.Fatalln(err)
	}

	iup.ImageFromImage(image).SetHandle("myicon")

	dlg := iup.Dialog(
		iup.Hbox(
			iup.Fill(),
			iup.Canvas(),
			iup.Fill(),
		),
	).SetHandle("dlg")

	itemShow := iup.Item("Show").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		dlg.SetAttribute("HIDETASKBAR", "NO")
		return iup.DEFAULT
	}))

	itemHide := iup.Item("Hide").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		dlg.SetAttribute("HIDETASKBAR", "YES")
		return iup.DEFAULT
	}))

	itemExit := iup.Item("Exit").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		dlg.SetAttribute("TRAY", "NO")
		dlg.Destroy()
		return iup.DEFAULT
	}))

	trayMenu := iup.Menu(itemShow, itemHide, iup.Separator(), itemExit)
	trayMenu.SetHandle("traymenu")

	// This example covers both the menu set via TRAYMENU for SNI and iup.Popup menu for other implementations.
	dlg.SetAttributes(map[string]string{
		"TITLE":     "Tray",
		"TRAY":      "YES",
		"TRAYMENU":  "traymenu",
		"TRAYIMAGE": "myicon",
		"TRAYTIP":   "This is a tip at tray",
		"ICON":      "myicon",
		"SIZE":      "100x100",
	})

	dlg.SetCallback("TRAYCLICK_CB", iup.TrayClickFunc(trayClickCb))

	dlg.SetCallback("CLOSE_CB", iup.CloseFunc(func(ih iup.Ihandle) int {
		dlg.SetAttribute("HIDETASKBAR", "YES")
		return iup.IGNORE
	}))

	iup.Map(trayMenu)

	iup.Show(dlg)

	iup.MainLoop()
}

func trayClickCb(ih iup.Ihandle, but, pressed, dclick int) int {
	dlg := iup.GetHandle("dlg")

	if but == 1 && pressed == 1 {
		dlg.SetAttribute("HIDETASKBAR", "NO")
		return iup.DEFAULT
	}

	if but == 3 && pressed == 1 {
		menu := iup.GetHandle("traymenu")
		iup.Popup(menu, iup.MOUSEPOS, iup.MOUSEPOS)
		return iup.DEFAULT
	}

	return iup.DEFAULT
}
