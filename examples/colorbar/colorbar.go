package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

var primaryLabel, secondaryLabel iup.Ihandle

func main() {
	iup.Open()
	defer iup.Close()

	primaryLabel = iup.Label("Primary: (none)").SetAttribute("SIZE", "150x")
	secondaryLabel = iup.Label("Secondary: (none)").SetAttribute("SIZE", "150x")

	colorbar := iup.ColorBar()
	colorbar.SetAttribute("NUM_CELLS", "16")
	colorbar.SetAttribute("NUM_PARTS", "1")
	colorbar.SetAttribute("SIZE", "200x30")
	colorbar.SetAttribute("ORIENTATION", "HORIZONTAL")
	colorbar.SetAttribute("SHOW_SECONDARY", "YES")

	colorbar.SetCallback("SELECT_CB", iup.SelectFunc(func(ih iup.Ihandle, cell, typ int) int {
		color := ih.GetAttribute(fmt.Sprintf("CELL%d", cell))
		if typ == iup.PRIMARY {
			primaryLabel.SetAttribute("TITLE", fmt.Sprintf("Primary: Cell %d (%s)", cell, color))
		} else {
			secondaryLabel.SetAttribute("TITLE", fmt.Sprintf("Secondary: Cell %d (%s)", cell, color))
		}
		return iup.DEFAULT
	}))

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Label("ColorBar - Click to select colors"),
			iup.Label("Left-click: primary color, Right-click: secondary color"),
			iup.Space().SetAttribute("SIZE", "x10"),
			iup.Frame(colorbar).SetAttribute("TITLE", "Color Palette"),
			iup.Space().SetAttribute("SIZE", "x10"),
			primaryLabel,
			secondaryLabel,
		).SetAttributes("MARGIN=20x20, GAP=5"),
	).SetAttribute("TITLE", "ColorBar Example")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}
