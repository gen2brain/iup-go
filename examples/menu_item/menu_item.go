package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	iup.Menu(
		iup.Submenu("File",
			iup.Menu(
				iup.MenuItem("Save\tCtrl+S").SetCallback("ACTION", iup.ActionFunc(itemSaveCb)),
				iup.MenuItem("&Auto Save\tCtrl+A").SetHandle("itemAutosave").SetAttributes(`VALUE=ON`).SetCallback("ACTION", iup.ActionFunc(itemAutosaveCb)),
				iup.MenuItem("Exit\tCtrl+X").SetAttributes(`KEY="x"`).SetCallback("ACTION", iup.ActionFunc(itemExitCb)),
			),
		),
	).SetHandle("menu")

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Text().SetAttributes(`VALUE="This is an empty text", EXPAND=HORIZONTAL`),
			iup.Button("Test").SetAttributes(`EXPAND=HORIZONTAL`),
		),
	)

	dlg.SetAttributes(map[string]interface{}{
		"TITLE":  "Item",
		"SIZE":   "120x",
		"MARGIN": "10x10",
		"GAP":    "10",
		"MENU":   "menu",
	})

	iup.Show(dlg)
	iup.MainLoop()
}

func itemSaveCb(ih iup.Ihandle) int {
	iup.Message("Item", "Saved!")
	return iup.DEFAULT
}

func itemAutosaveCb(ih iup.Ihandle) int {
	itemAutosave := iup.GetHandle("itemAutosave")
	if itemAutosave.GetInt("VALUE") != 0 {
		itemAutosave.SetAttribute("VALUE", "OFF")
		iup.Message("Item", "AutoSave OFF")
	} else {
		itemAutosave.SetAttribute("VALUE", "ON")
		iup.Message("Item", "AutoSave ON")
	}
	return iup.DEFAULT
}

func itemExitCb(ih iup.Ihandle) int {
	return iup.CLOSE
}
