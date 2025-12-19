package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	// Simple text content
	btn1 := iup.Button("Info Popover")
	popover1 := iup.Popover(
		iup.Vbox(
			iup.Label("This is an info popover"),
			iup.Label("It contains simple text content."),
		).SetAttributes("MARGIN=10x10, GAP=5"),
	)
	popover1.SetAttribute("POSITION", "BOTTOM")
	iup.SetAttributeHandle(popover1, "ANCHOR", btn1)

	btn1.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		visible := popover1.GetAttribute("VISIBLE")
		if visible == "YES" {
			popover1.SetAttribute("VISIBLE", "NO")
		} else {
			popover1.SetAttribute("VISIBLE", "YES")
		}
		return iup.DEFAULT
	}))

	// Form with input fields
	btn2 := iup.Button("Form Popover")
	nameText := iup.Text().SetAttributes("VISIBLECOLUMNS=15")
	emailText := iup.Text().SetAttributes("VISIBLECOLUMNS=15")
	var popover2 iup.Ihandle

	submitBtn := iup.Button("Submit")
	submitBtn.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		fmt.Printf("Name: %s, Email: %s\n", nameText.GetAttribute("VALUE"), emailText.GetAttribute("VALUE"))
		popover2.SetAttribute("VISIBLE", "NO")
		return iup.DEFAULT
	}))

	popover2 = iup.Popover(
		iup.Vbox(
			iup.Label("Quick Form"),
			iup.Hbox(iup.Label("Name:"), nameText).SetAttributes("GAP=5"),
			iup.Hbox(iup.Label("Email:"), emailText).SetAttributes("GAP=5"),
			submitBtn,
		).SetAttributes("MARGIN=10x10, GAP=8"),
	)
	popover2.SetAttribute("POSITION", "BOTTOM")
	iup.SetAttributeHandle(popover2, "ANCHOR", btn2)

	btn2.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		visible := popover2.GetAttribute("VISIBLE")
		if visible == "YES" {
			popover2.SetAttribute("VISIBLE", "NO")
		} else {
			popover2.SetAttribute("VISIBLE", "YES")
		}
		return iup.DEFAULT
	}))

	// Color selection with toggle buttons
	btn3 := iup.Button("Color Picker")
	selectedColor := iup.Label("Selected: None")

	createColorBtn := func(color, name string) iup.Ihandle {
		btn := iup.Button("").SetAttribute("BGCOLOR", color)
		btn.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
			selectedColor.SetAttribute("TITLE", fmt.Sprintf("Selected: %s", name))
			return iup.DEFAULT
		}))
		return btn
	}

	popover3 := iup.Popover(
		iup.Vbox(
			iup.Label("Pick a Color"),
			iup.Hbox(
				createColorBtn("255 0 0", "Red"),
				createColorBtn("0 255 0", "Green"),
				createColorBtn("0 0 255", "Blue"),
				createColorBtn("255 255 0", "Yellow"),
			).SetAttributes("GAP=5"),
			iup.Hbox(
				createColorBtn("255 128 0", "Orange"),
				createColorBtn("128 0 255", "Purple"),
				createColorBtn("0 255 255", "Cyan"),
				createColorBtn("255 0 255", "Magenta"),
			).SetAttributes("GAP=5"),
			selectedColor.SetAttribute("EXPAND", "HORIZONTAL"),
		).SetAttributes("MARGIN=10x10, GAP=8"),
	)
	popover3.SetAttribute("POSITION", "RIGHT")
	popover3.SetAttribute("AUTOHIDE", "NO")
	iup.SetAttributeHandle(popover3, "ANCHOR", btn3)

	btn3.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		visible := popover3.GetAttribute("VISIBLE")
		if visible == "YES" {
			popover3.SetAttribute("VISIBLE", "NO")
		} else {
			popover3.SetAttribute("VISIBLE", "YES")
		}
		return iup.DEFAULT
	}))

	// Menu-like list
	btn4 := iup.Button("Menu Popover")
	var popover4 iup.Ihandle

	createMenuItem := func(name string) iup.Ihandle {
		btn := iup.Button(name).SetAttributes("FLAT=YES, ALIGNMENT=ALEFT")
		btn.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
			fmt.Printf("Menu item clicked: %s\n", name)
			popover4.SetAttribute("VISIBLE", "NO")
			return iup.DEFAULT
		}))
		return btn
	}

	popover4 = iup.Popover(
		iup.Vbox(
			createMenuItem("New File"),
			createMenuItem("Open File"),
			createMenuItem("Save File"),
			iup.Label("").SetAttributes("SEPARATOR=HORIZONTAL"),
			createMenuItem("Exit"),
		).SetAttributes("MARGIN=5x5, GAP=2"),
	)
	popover4.SetAttribute("POSITION", "BOTTOM")
	iup.SetAttributeHandle(popover4, "ANCHOR", btn4)

	btn4.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		visible := popover4.GetAttribute("VISIBLE")
		if visible == "YES" {
			popover4.SetAttribute("VISIBLE", "NO")
		} else {
			popover4.SetAttribute("VISIBLE", "YES")
		}
		return iup.DEFAULT
	}))

	// Create the main dialog
	dlg := iup.Dialog(
		iup.Vbox(
			iup.Label("Click buttons to show different popover types:"),
			iup.Hbox(btn1, btn2, btn3, btn4).SetAttributes("GAP=10"),
			iup.Fill(),
		).SetAttributes("MARGIN=20x20, GAP=15"),
	).SetAttributes("TITLE=Popover Examples, SIZE=450x150")

	iup.Show(dlg)
	iup.MainLoop()
}
