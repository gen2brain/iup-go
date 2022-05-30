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
	).SetAttributes(map[string]string{
		"TITLE":     "Tray",
		"TRAY":      "YES",
		"TRAYTIP":   "This is a tip at tray",
		"TRAYIMAGE": "myicon",
		"ICON":      "myicon",
		"SIZE":      "200x200",
	}).SetCallback("TRAYCLICK_CB", iup.TrayClickFunc(trayClickCb)).SetHandle("dlg")

	iup.Show(dlg)
	iup.MainLoop()
}

func trayClickCb(ih iup.Ihandle, but, pressed, dclick int) int {
	if but == 3 && pressed > 0 {
		itemShow := iup.Item("Show").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
			iup.Show(iup.GetHandle("dlg"))
			return iup.DEFAULT
		}))

		itemHide := iup.Item("Hide").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
			iup.Hide(iup.GetHandle("dlg"))
			return iup.DEFAULT
		}))

		itemExit := iup.Item("Exit").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
			iup.GetHandle("dlg").SetAttribute("TRAY", "NO")
			iup.Hide(iup.GetHandle("dlg"))
			return iup.CLOSE
		}))

		menu := iup.Menu(itemShow, itemHide, iup.Separator(), itemExit)
		iup.Popup(menu, iup.MOUSEPOS, iup.MOUSEPOS)
	}
	return iup.DEFAULT
}
