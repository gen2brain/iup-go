package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

var selectionLabel iup.Ihandle

func main() {
	iup.Open()
	defer iup.Close()

	selectionLabel = iup.Label("Selected: (none)").SetAttribute("SIZE", "200x")

	dropList := iup.Vbox(
		createDropItem("Option A"),
		createDropItem("Option B"),
		createDropItem("Option C"),
		createDropItem("Option D"),
	).SetAttributes("MARGIN=5x5, GAP=2")

	dropButton1 := iup.DropButton(dropList).SetAttributes(`TITLE="Select an option", EXPAND=HORIZONTAL`)

	dropButton1.SetCallback("DROPDOWN_CB", iup.DropDownFunc(func(ih iup.Ihandle, state int) int {
		if state == 1 {
			fmt.Println("Dropdown opened")
		} else {
			fmt.Println("Dropdown closed")
		}
		return iup.DEFAULT
	}))

	colorList := iup.Vbox(
		createColorItem("Red", "255 0 0"),
		createColorItem("Green", "0 255 0"),
		createColorItem("Blue", "0 0 255"),
		createColorItem("Yellow", "255 255 0"),
	).SetAttributes("MARGIN=5x5, GAP=2")

	dropButton2 := iup.DropButton(colorList).SetAttributes(`TITLE="Pick a color", EXPAND=HORIZONTAL`)

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Label("DropButton creates a button with a dropdown panel."),
			iup.Label("Click the arrow to open, select an item."),
			iup.Space().SetAttribute("SIZE", "x10"),
			iup.Frame(
				iup.Vbox(
					dropButton1,
					iup.Space().SetAttribute("SIZE", "x5"),
					dropButton2,
				).SetAttribute("GAP", "5"),
			).SetAttribute("TITLE", "DropButtons"),
			iup.Space().SetAttribute("SIZE", "x10"),
			selectionLabel,
		).SetAttributes("MARGIN=20x20, GAP=5"),
	).SetAttribute("TITLE", "DropButton Example")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}

func createDropItem(text string) iup.Ihandle {
	btn := iup.FlatButton(text).SetAttributes("EXPAND=HORIZONTAL, ALIGNMENT=ALEFT")
	btn.SetCallback("FLAT_ACTION", iup.FlatActionFunc(func(ih iup.Ihandle) int {
		dlg := iup.GetDialog(ih)
		dropBtn := iup.GetAttributeHandle(dlg, "DROPBUTTON")
		if dropBtn != 0 {
			dropBtn.SetAttribute("SHOWDROPDOWN", "NO")
			dropBtn.SetAttribute("TITLE", text)
		}
		selectionLabel.SetAttribute("TITLE", "Selected: "+text)
		return iup.DEFAULT
	}))
	return btn
}

func createColorItem(text, color string) iup.Ihandle {
	btn := iup.FlatButton(text).SetAttributes("EXPAND=HORIZONTAL")
	btn.SetAttribute("BGCOLOR", color)
	btn.SetCallback("FLAT_ACTION", iup.FlatActionFunc(func(ih iup.Ihandle) int {
		dlg := iup.GetDialog(ih)
		dropBtn := iup.GetAttributeHandle(dlg, "DROPBUTTON")
		if dropBtn != 0 {
			dropBtn.SetAttribute("SHOWDROPDOWN", "NO")
			dropBtn.SetAttribute("TITLE", text)
		}
		selectionLabel.SetAttribute("TITLE", "Color: "+text+" ("+color+")")
		return iup.DEFAULT
	}))
	return btn
}
