package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	bt := iup.Button("Button").SetAttribute("EXPAND", "YES")

	ml := iup.MultiLine().SetAttributes(map[string]string{
		"EXPAND":         "YES",
		"VISIBLELINES":   "5",
		"VISIBLECOLUMNS": "10",
	})

	split := iup.Split(bt, ml).SetAttributes(map[string]string{
		"ORIENTATION": "HORIZONTAL",
		"COLOR":       "127 127 255",
	})

	vbox := iup.Vbox(split).SetAttributes(map[string]string{
		"MARGIN": "10x10",
		"GAP":    "10",
	})

	dlg := iup.Dialog(vbox).SetAttribute("TITLE", "Split")

	iup.Show(dlg)
	iup.MainLoop()
}
