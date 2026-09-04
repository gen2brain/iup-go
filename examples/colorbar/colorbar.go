package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

var primaryLabel, secondaryLabel iup.Ihandle

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	primaryLabel = iup.Label("Primary: (none)").SetAttribute("EXPAND", "HORIZONTAL")
	secondaryLabel = iup.Label("Secondary: (none)").SetAttribute("EXPAND", "HORIZONTAL")
	status := iup.Label("Flat bar: FLAT, FLATCOLOR, SHOW_PREVIEW=NO, FOCUSSELECT. EXTENDED_CB: Shift+right-click a cell; SWITCH_CB: double-click the preview outside the cells").SetAttribute("EXPAND", "HORIZONTAL")

	colorbar := iup.ColorBar()
	colorbar.SetAttribute("NUM_CELLS", "16")
	colorbar.SetAttribute("NUM_PARTS", "1")
	colorbar.SetAttribute("ORIENTATION", "HORIZONTAL")
	colorbar.SetAttribute("SHOW_SECONDARY", "YES")
	colorbar.SetAttributes(`RASTERSIZE=400x40, EXPAND=HORIZONTAL, PREVIEW_SIZE=40, SQUARED=NO, SHADOWED=YES, PRIMARY_CELL=3, SECONDARY_CELL=5, TRANSPARENCY="255 255 255"`)

	colorbar.SetCallback("EXTENDED_CB", iup.ExtendedFunc(func(ih iup.Ihandle, cell int) int {
		status.SetAttribute("TITLE", fmt.Sprintf("EXTENDED_CB on cell %d", cell))
		return iup.DEFAULT
	}))
	colorbar.SetCallback("SWITCH_CB", iup.SwitchFunc(func(ih iup.Ihandle, primCell, secCell int) int {
		status.SetAttribute("TITLE", fmt.Sprintf("SWITCH_CB: primary %d and secondary %d swapped, PRIMARY_CELL=%s SECONDARY_CELL=%s",
			primCell, secCell, ih.GetAttribute("PRIMARY_CELL"), ih.GetAttribute("SECONDARY_CELL")))
		return iup.DEFAULT
	}))

	flat := iup.ColorBar()
	flat.SetAttributes(`RASTERSIZE=60x120, EXPAND=NO, NUM_CELLS=8, NUM_PARTS=2, ORIENTATION=VERTICAL, SHOW_PREVIEW=NO, FLAT=YES, FLATCOLOR="200 40 40", FOCUSSELECT=YES, SQUARED=YES`)
	flat.SetCallback("SELECT_CB", iup.SelectFunc(func(ih iup.Ihandle, cell, typ int) int {
		status.SetAttribute("TITLE", fmt.Sprintf("Flat bar: cell %d selected (FOCUSSELECT follows the keyboard focus)", cell))
		return iup.DEFAULT
	}))

	colorbar.SetCallback("SELECT_CB", iup.SelectFunc(func(ih iup.Ihandle, cell, typ int) int {
		color := ih.GetAttribute(fmt.Sprintf("CELL%d", cell))
		if typ == iup.PRIMARY {
			primaryLabel.SetAttribute("TITLE", fmt.Sprintf("Primary: Cell %d (%s)", cell, color))
		} else {
			secondaryLabel.SetAttribute("TITLE", fmt.Sprintf("Secondary: Cell %d (%s)", cell, color))
		}
		return iup.DEFAULT
	}))

	colorbar.SetCallback("CELL_CB", iup.CellFunc(func(ih iup.Ihandle, cell int) string {
		if cell%2 == 0 {
			return ""
		}
		return "255 0 255"
	}))

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Label("ColorBar - Click to select colors"),
			iup.Label("Left-click: primary color, Right-click: secondary color"),
			iup.Label("Double-click an odd cell to recolor it, even cells ignore the change"),
			iup.Frame(colorbar).SetAttributes("TITLE=Color Palette, EXPAND=HORIZONTAL"),
			iup.Hbox(
				iup.Frame(flat).SetAttribute("TITLE", "Flat"),
				iup.Vbox(primaryLabel, secondaryLabel, status).SetAttributes("NGAP=5, EXPAND=YES"),
			).SetAttributes("NGAP=10, EXPAND=YES"),
		).SetAttributes("NMARGIN=10x10, NGAP=5"),
	).SetAttribute("TITLE", "ColorBar Example")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}
