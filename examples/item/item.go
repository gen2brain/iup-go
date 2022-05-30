package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	iup.Menu(
		iup.Submenu("File",
			iup.Menu(
				iup.Item("Save\tCtrl+S").SetCallback("ACTION", iup.ActionFunc(itemSaveCb)),
				iup.Item("&Auto Save").SetHandle("itemAutosave").SetAttributes(`VALUE=ON`).SetCallback("ACTION", iup.ActionFunc(itemAutosaveCb)),
				iup.Item("Exit").SetAttributes(`KEY="x"`).SetCallback("ACTION", iup.ActionFunc(itemExitCb)),
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

	dlg.SetCallback("K_ANY", iup.KAnyFunc(kAny))

	iup.Show(dlg)
	iup.MainLoop()
}

func kAny(ih iup.Ihandle, c int) int {
	if iup.IsCtrlXKey(c) {
		if iup.XKeyCtrl(iup.K_S) == c {
			itemSaveCb(ih)
			return iup.DEFAULT
		} else if iup.XKeyCtrl(iup.K_A) == c {
			itemAutosaveCb(ih)
			return iup.DEFAULT
		} else if iup.XKeyCtrl(iup.K_X) == c {
			return iup.CLOSE
		}
	}
	return iup.DEFAULT
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
