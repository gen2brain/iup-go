package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func updateStatus() {
	iup.GetHandle("status").SetAttribute("TITLE", "APPEARANCE="+iup.GetGlobal("APPEARANCE")+
		"  DARKMODE="+iup.GetGlobal("DARKMODE")+
		"  DLGBGCOLOR="+iup.GetGlobal("DLGBGCOLOR"))
}

func setAppearance(value string) func(iup.Ihandle, int) int {
	return func(ih iup.Ihandle, state int) int {
		if state == 1 {
			iup.SetGlobal("APPEARANCE", value)
			updateStatus()
		}
		return iup.DEFAULT
	}
}

func main() {
	iup.Open()
	defer iup.Close()

	system := iup.Toggle("System").SetAttribute("VALUE", "ON")
	light := iup.Toggle("Light")
	dark := iup.Toggle("Dark")

	system.SetCallback("ACTION", iup.ToggleActionFunc(setAppearance("SYSTEM")))
	light.SetCallback("ACTION", iup.ToggleActionFunc(setAppearance("LIGHT")))
	dark.SetCallback("ACTION", iup.ToggleActionFunc(setAppearance("DARK")))

	status := iup.Label("")
	status.SetHandle("status")

	sample := iup.Vbox(
		iup.Label("Label"),
		iup.Text().SetAttributes(`VALUE="Text entry", EXPAND=HORIZONTAL`),
		iup.List().SetAttributes(`1=Alpha, 2=Beta, 3=Gamma, VALUE=1, VISIBLELINES=3, EXPAND=HORIZONTAL`),
		iup.Hbox(
			iup.Button("Button"),
			iup.Toggle("Check"),
		).SetAttributes(`GAP=5`),
	).SetAttributes(`GAP=5, MARGIN=5x5`)

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Radio(
				iup.Hbox(system, light, dark).SetAttributes(`GAP=10`),
			),
			iup.Frame(sample).SetAttribute("TITLE", "Preview"),
			status,
		).SetAttributes(`GAP=10, MARGIN=10x10`),
	).SetAttributes(`TITLE="Appearance"`)

	dlg.SetCallback("THEMECHANGED_CB", iup.ThemeChangedFunc(func(ih iup.Ihandle, darkMode int) int {
		updateStatus()
		return iup.DEFAULT
	}))

	updateStatus()

	iup.Show(dlg)
	iup.MainLoop()
}
