package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	// Toggle popover visibility on button click
	togglePopover := func(p iup.Ihandle) iup.ActionFunc {
		return func(ih iup.Ihandle) int {
			if p.GetAttribute("VISIBLE") == "YES" {
				p.SetAttribute("VISIBLE", "NO")
			} else {
				p.SetAttribute("VISIBLE", "YES")
			}
			return iup.DEFAULT
		}
	}

	// Create a popover anchored to a button with given position
	makePositionDemo := func(position string) iup.Ihandle {
		btn := iup.Button(position).SetAttributes("EXPAND=HORIZONTAL, PADDING=8x4")
		popover := iup.Popover(
			iup.Vbox(
				iup.Label(fmt.Sprintf("Position: %s", position)).SetAttributes("FONTSTYLE=Bold"),
				iup.Label("Popover content here"),
			).SetAttributes("MARGIN=10x8, GAP=4"),
		)
		popover.SetAttribute("POSITION", position)
		iup.SetAttributeHandle(popover, "ANCHOR", btn)
		btn.SetCallback("ACTION", togglePopover(popover))
		return btn
	}

	// Left/Right edge positions
	leftFrame := iup.Frame(
		iup.Vbox(
			iup.Hbox(makePositionDemo("LEFT"), makePositionDemo("LEFTTOP"), makePositionDemo("LEFTBOTTOM")).SetAttributes("GAP=8"),
		).SetAttributes("MARGIN=10x10"),
	).SetAttributes("TITLE=\"Left Edge\"")

	rightFrame := iup.Frame(
		iup.Vbox(
			iup.Hbox(makePositionDemo("RIGHT"), makePositionDemo("RIGHTTOP"), makePositionDemo("RIGHTBOTTOM")).SetAttributes("GAP=8"),
		).SetAttributes("MARGIN=10x10"),
	).SetAttributes("TITLE=\"Right Edge\"")

	// Centered positions
	centeredFrame := iup.Frame(
		iup.Vbox(
			iup.Hbox(makePositionDemo("TOP"), makePositionDemo("BOTTOM"), makePositionDemo("RIGHT"), makePositionDemo("LEFT")).SetAttributes("GAP=8"),
		).SetAttributes("MARGIN=10x10"),
	).SetAttributes("TITLE=Centered")

	// Bottom/Top edge variants
	bottomFrame := iup.Frame(
		iup.Vbox(
			iup.Hbox(makePositionDemo("BOTTOM"), makePositionDemo("BOTTOMLEFT"), makePositionDemo("BOTTOMRIGHT")).SetAttributes("GAP=8"),
		).SetAttributes("MARGIN=10x10"),
	).SetAttributes("TITLE=\"Bottom Edge\"")

	topFrame := iup.Frame(
		iup.Vbox(
			iup.Hbox(makePositionDemo("TOP"), makePositionDemo("TOPLEFT"), makePositionDemo("TOPRIGHT")).SetAttributes("GAP=8"),
		).SetAttributes("MARGIN=10x10"),
	).SetAttributes("TITLE=\"Top Edge\"")

	// Practical: Dropdown menu (BOTTOMLEFT)
	menuBtn := iup.Button("File").SetAttributes("PADDING=12x4")
	var menuPopover iup.Ihandle

	createMenuItem := func(name string) iup.Ihandle {
		btn := iup.Button(name).SetAttributes("FLAT=YES, ALIGNMENT=ALEFT, EXPAND=HORIZONTAL")
		btn.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
			fmt.Printf("Clicked: %s\n", name)
			menuPopover.SetAttribute("VISIBLE", "NO")
			return iup.DEFAULT
		}))
		return btn
	}

	menuPopover = iup.Popover(
		iup.Vbox(
			createMenuItem("New File"),
			createMenuItem("Open File"),
			createMenuItem("Save"),
			iup.Label("").SetAttributes("SEPARATOR=HORIZONTAL"),
			createMenuItem("Exit"),
		).SetAttributes("MARGIN=4x4, GAP=1"),
	)
	menuPopover.SetAttribute("POSITION", "BOTTOMLEFT")
	iup.SetAttributeHandle(menuPopover, "ANCHOR", menuBtn)
	menuBtn.SetCallback("ACTION", togglePopover(menuPopover))

	// Practical: Form popover (BOTTOMRIGHT)
	formBtn := iup.Button("Quick Form").SetAttributes("PADDING=12x4")
	nameText := iup.Text().SetAttributes("VISIBLECOLUMNS=15")
	emailText := iup.Text().SetAttributes("VISIBLECOLUMNS=15")
	var formPopover iup.Ihandle

	submitBtn := iup.Button("Submit")
	submitBtn.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		fmt.Printf("Name: %s, Email: %s\n", nameText.GetAttribute("VALUE"), emailText.GetAttribute("VALUE"))
		formPopover.SetAttribute("VISIBLE", "NO")
		return iup.DEFAULT
	}))

	formPopover = iup.Popover(
		iup.Vbox(
			iup.Label("Quick Form"),
			iup.Hbox(iup.Label("Name:"), nameText).SetAttributes("GAP=5"),
			iup.Hbox(iup.Label("Email:"), emailText).SetAttributes("GAP=5"),
			submitBtn,
		).SetAttributes("MARGIN=10x10, GAP=8"),
	)
	formPopover.SetAttribute("POSITION", "BOTTOMRIGHT")
	iup.SetAttributeHandle(formPopover, "ANCHOR", formBtn)
	formBtn.SetCallback("ACTION", togglePopover(formPopover))

	// Practical: Color picker (RIGHTTOP, no autohide)
	colorBtn := iup.Button("Color Picker").SetAttributes("PADDING=12x4")
	selectedColor := iup.Label("Selected: None")

	createColorBtn := func(color, name string) iup.Ihandle {
		btn := iup.Button("").SetAttribute("BGCOLOR", color)
		btn.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
			selectedColor.SetAttribute("TITLE", fmt.Sprintf("Selected: %s", name))
			return iup.DEFAULT
		}))
		return btn
	}

	colorPopover := iup.Popover(
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
	colorPopover.SetAttribute("POSITION", "RIGHTTOP")
	colorPopover.SetAttribute("AUTOHIDE", "NO")
	iup.SetAttributeHandle(colorPopover, "ANCHOR", colorBtn)
	colorBtn.SetCallback("ACTION", togglePopover(colorPopover))

	// Practical: Help tooltip (TOPLEFT)
	infoBtn := iup.Button("?").SetAttributes("PADDING=4x4, FONTSTYLE=Bold")
	infoPopover := iup.Popover(
		iup.Vbox(
			iup.Label("Help Information").SetAttributes("FONTSTYLE=Bold"),
			iup.Label("This popover uses TOPLEFT position,"),
			iup.Label("appearing above with left edges aligned."),
		).SetAttributes("MARGIN=10x8, GAP=4"),
	)
	infoPopover.SetAttribute("POSITION", "TOPLEFT")
	iup.SetAttributeHandle(infoPopover, "ANCHOR", infoBtn)
	infoBtn.SetCallback("ACTION", togglePopover(infoPopover))

	// Practical: Offset demo (BOTTOM with gap)
	offsetBtn := iup.Button("Offset Demo").SetAttributes("PADDING=12x4")
	offsetPopover := iup.Popover(
		iup.Vbox(
			iup.Label("BOTTOM + OFFSETY=10").SetAttributes("FONTSTYLE=Bold"),
			iup.Label("10px gap below anchor"),
		).SetAttributes("MARGIN=10x8, GAP=4"),
	)
	offsetPopover.SetAttribute("POSITION", "BOTTOM")
	offsetPopover.SetAttribute("OFFSETY", "10")
	iup.SetAttributeHandle(offsetPopover, "ANCHOR", offsetBtn)
	offsetBtn.SetCallback("ACTION", togglePopover(offsetPopover))

	useCasesFrame := iup.Frame(
		iup.Vbox(
			iup.Hbox(
				iup.Label("Menu (BOTTOMLEFT):"), menuBtn,
				iup.Fill(),
				iup.Label("Form (BOTTOMRIGHT):"), formBtn,
			).SetAttributes("GAP=5, ALIGNMENT=ACENTER"),
			iup.Hbox(
				iup.Label("Colors (RIGHTTOP):"), colorBtn,
				iup.Fill(),
				iup.Label("Help (TOPLEFT):"), infoBtn,
				iup.Fill(),
				iup.Label("Offset:"), offsetBtn,
			).SetAttributes("GAP=5, ALIGNMENT=ACENTER"),
		).SetAttributes("MARGIN=10x10, GAP=8"),
	).SetAttributes("TITLE=\"Practical Examples\"")

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Hbox(leftFrame, rightFrame).SetAttributes("GAP=8"),
			centeredFrame,
			iup.Hbox(bottomFrame, topFrame).SetAttributes("GAP=8"),
			useCasesFrame.SetAttribute("EXPAND", "HORIZONTAL"),
		).SetAttributes("MARGIN=15x15, GAP=8"),
	).SetAttributes("TITLE=\"Popover Positions\", SIZE=640x350")

	iup.Show(dlg)
	iup.MainLoop()
}
