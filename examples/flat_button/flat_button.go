//go:build ctrl

package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()
	iup.ControlsOpen()

	status := iup.Label("Click a button to show its name here")
	status.SetAttribute("EXPAND", "HORIZONTAL")

	action := func(name string) iup.FlatActionFunc {
		return func(iup.Ihandle) int {
			status.SetAttribute("TITLE", "Clicked: "+name)
			return iup.DEFAULT
		}
	}

	button := func(title, grad string) iup.Ihandle {
		b := iup.FlatButton(title)
		b.SetAttributes(`EXPAND=HORIZONTAL, PADDING=12x8, CORNERRADIUS=6, FGCOLOR="255 255 255", GRADIENTANGLE=90`)
		if grad != "" {
			b.SetAttribute("GRADIENT", grad)
		}
		iup.SetCallback(b, "FLAT_ACTION", action(title))
		return b
	}

	solid := iup.FlatButton("Solid color")
	solid.SetAttributes(`EXPAND=HORIZONTAL, PADDING=12x8, CORNERRADIUS=6, BGCOLOR="40 120 210", FGCOLOR="255 255 255"`)
	iup.SetCallback(solid, "FLAT_ACTION", action("Solid color"))

	vbox := iup.Vbox(
		solid,
		button("Two-stop gradient", "60 130 220:20 60 130"),
		button("Three-stop gradient", "200 40 40:230 200 40:40 160 60"),
		button("Five-stop gradient", "200 0 0:230 130 0:230 230 0:0 170 0:0 0 210"),
		button("Sunset (four stops)", "40 20 80:180 60 90:240 150 70:250 220 120"),
		status,
	)
	vbox.SetAttributes("NMARGIN=10x10, GAP=8")

	dlg := iup.Dialog(vbox)
	dlg.SetAttribute("TITLE", "IupFlatButton")

	iup.Show(dlg)
	iup.MainLoop()
}
