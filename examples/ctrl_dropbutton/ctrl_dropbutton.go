//go:build ctrl

package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

var status iup.Ihandle

func init() { iup.EntryPoint(main) }

func report(msg string) {
	fmt.Println(msg)
	status.SetAttribute("TITLE", msg)
}

func main() {
	iup.Open()
	defer iup.Close()
	iup.ControlsOpen()

	status = iup.Label("Selected: (none)").SetAttribute("EXPAND", "HORIZONTAL")

	iup.SetHandle("arrow", makeImage(40, 120, 210))
	iup.SetHandle("arrow_high", makeImage(120, 190, 255))
	iup.SetHandle("arrow_press", makeImage(20, 70, 140))
	iup.SetHandle("arrow_inactive", makeImage(170, 170, 170))
	iup.SetHandle("icon", makeImage(200, 60, 60))
	iup.SetHandle("icon_high", makeImage(255, 120, 120))
	iup.SetHandle("icon_press", makeImage(140, 20, 20))
	iup.SetHandle("icon_inactive", makeImage(190, 190, 190))

	optionList := iup.Vbox(
		createDropItem("Option A"),
		createDropItem("Option B"),
		createDropItem("Option C"),
		createDropItem("Option D"),
	).SetAttributes("NMARGIN=5x5, NGAP=2")

	options := iup.DropButton(optionList).SetAttributes(`TITLE="Select an option", EXPAND=HORIZONTAL, DROPONARROW=NO`)
	options.SetCallback("DROPDOWN_CB", iup.DropDownFunc(func(ih iup.Ihandle, state int) int {
		report(fmt.Sprintf("DROPDOWN_CB state=%d", state))
		return iup.DEFAULT
	}))
	options.SetCallback("DROPSHOW_CB", iup.DropShowFunc(func(ih iup.Ihandle, state int) int {
		fmt.Printf("DROPSHOW_CB state=%d\n", state)
		return iup.DEFAULT
	}))

	colorList := iup.Vbox(
		createColorItem("Red", "255 0 0"),
		createColorItem("Green", "0 255 0"),
		createColorItem("Blue", "0 0 255"),
		createColorItem("Yellow", "255 255 0"),
	).SetAttributes("NMARGIN=5x5, NGAP=2")
	iup.SetHandle("color_list", colorList)

	colors := iup.DropButton(colorList).SetAttributes(`TITLE="Pick a color", EXPAND=HORIZONTAL`)
	colors.SetAttributes(`IMAGE=icon, IMAGEHIGHLIGHT=icon_high, IMAGEPRESS=icon_press, IMAGEINACTIVE=icon_inactive, IMAGEPOSITION=LEFT`)
	colors.SetAttributes(`ARROWSIZE=28, ARROWPADDING=6, ARROWALIGN=TOP, ARROWCOLOR="200 60 60", BORDERWIDTH=1`)
	colors.SetAttributes(`BORDERHLCOLOR="200 60 60", BORDERPSCOLOR="140 20 20", TEXTHLCOLOR="200 60 60", TEXTPSCOLOR="140 20 20", FOCUSFEEDBACK=NO`)
	colors.SetCallback("FLAT_ACTION", iup.FlatActionFunc(func(ih iup.Ihandle) int {
		report("FLAT_ACTION on the button body, DROPONARROW keeps the list closed")
		return iup.DEFAULT
	}))

	images := iup.DropButton(iup.Vbox(
		createDropItem("First"),
		createDropItem("Second"),
	).SetAttributes("NMARGIN=5x5, NGAP=2"))
	images.SetAttributes(`TITLE="Arrow images", ARROWIMAGES=YES, ARROWIMAGE=arrow, ARROWIMAGEHIGHLIGHT=arrow_high, ARROWIMAGEPRESS=arrow_press, ARROWIMAGEINACTIVE=arrow_inactive`)
	images.SetAttributes(`TEXTALIGNMENT=ACENTER, CPADDING=2x1, CSPACING=1, EXPAND=HORIZONTAL`)
	images.SetCallback("FLAT_BUTTON_CB", iup.ButtonFunc(func(ih iup.Ihandle, button, pressed, x, y int, st string) int {
		fmt.Printf("FLAT_BUTTON_CB button=%d pressed=%d at %d,%d [%s] PRESSED=%s\n", button, pressed, x, y, st, ih.GetAttribute("PRESSED"))
		return iup.DEFAULT
	}))
	images.SetCallback("FLAT_MOTION_CB", iup.MotionFunc(func(ih iup.Ihandle, x, y int, st string) int {
		fmt.Printf("FLAT_MOTION_CB %d,%d HIGHLIGHTED=%s\n", x, y, ih.GetAttribute("HIGHLIGHTED"))
		return iup.DEFAULT
	}))
	images.SetCallback("FLAT_ENTERWINDOW_CB", iup.EnterWindowFunc(func(ih iup.Ihandle) int {
		fmt.Println("FLAT_ENTERWINDOW_CB")
		return iup.DEFAULT
	}))
	images.SetCallback("FLAT_LEAVEWINDOW_CB", iup.LeaveWindowFunc(func(ih iup.Ihandle) int {
		fmt.Println("FLAT_LEAVEWINDOW_CB")
		return iup.DEFAULT
	}))
	images.SetCallback("FLAT_FOCUS_CB", iup.FocusFunc(func(ih iup.Ihandle, focus int) int {
		fmt.Printf("FLAT_FOCUS_CB focus=%d HASFOCUS=%s\n", focus, ih.GetAttribute("HASFOCUS"))
		return iup.DEFAULT
	}))

	position := iup.List()
	position.SetAttributes(`DROPDOWN=YES, 1=BOTTOMLEFT, 2=TOPLEFT, 3=BOTTOMRIGHT, 4=TOPRIGHT, VALUE=1`)
	position.SetCallback("ACTION", iup.ListActionFunc(func(ih iup.Ihandle, text string, item, state int) int {
		if state == 1 {
			for _, b := range []iup.Ihandle{options, colors, images} {
				b.SetAttribute("DROPPOSITION", text)
			}
			report("DROPPOSITION=" + text)
		}
		return iup.DEFAULT
	}))

	swapChild := iup.Button("Swap the drop child of the first button")
	swapChild.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		if iup.GetAttributeHandle(options, "DROPCHILD_HANDLE") == colorList {
			iup.SetAttributeHandle(options, "DROPCHILD_HANDLE", optionList)
		} else {
			options.SetAttribute("DROPCHILD", "color_list")
		}
		report("DROPCHILD=" + options.GetAttribute("DROPCHILD"))
		return iup.DEFAULT
	}))

	arrowActive := iup.Toggle("ARROWACTIVE")
	arrowActive.SetAttribute("VALUE", "ON")
	arrowActive.SetCallback("ACTION", iup.ToggleActionFunc(func(_ iup.Ihandle, state int) int {
		value := "NO"
		if state != 0 {
			value = "YES"
		}
		for _, b := range []iup.Ihandle{options, colors, images} {
			b.SetAttribute("ARROWACTIVE", value)
		}
		return iup.DEFAULT
	}))

	active := iup.Toggle("ACTIVE")
	active.SetAttribute("VALUE", "ON")
	active.SetCallback("ACTION", iup.ToggleActionFunc(func(_ iup.Ihandle, state int) int {
		value := "NO"
		if state != 0 {
			value = "YES"
		}
		for _, b := range []iup.Ihandle{options, colors, images} {
			b.SetAttribute("ACTIVE", value)
		}
		return iup.DEFAULT
	}))

	open := iup.Button("Open the first button")
	open.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		options.SetAttribute("SHOWDROPDOWN", "YES")
		return iup.DEFAULT
	}))

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Label("Click the arrow to open, select an item."),
			iup.Frame(
				iup.Vbox(options, colors, images).SetAttribute("NGAP", "5"),
			).SetAttribute("TITLE", "DropButtons"),
			iup.Hbox(iup.Label("DROPPOSITION"), position, arrowActive, active).SetAttributes("NGAP=5, ALIGNMENT=ACENTER"),
			iup.Hbox(swapChild, open).SetAttribute("NGAP", "5"),
			status,
		).SetAttributes("NMARGIN=10x10, NGAP=8"),
	).SetAttribute("TITLE", "IupDropButton")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}

func createDropItem(text string) iup.Ihandle {
	btn := iup.Button(text).SetAttributes("EXPAND=HORIZONTAL, ALIGNMENT=ALEFT")
	btn.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		dropBtn := iup.GetAttributeHandle(iup.GetDialog(ih), "DROPBUTTON")
		if dropBtn != 0 {
			dropBtn.SetAttribute("SHOWDROPDOWN", "NO")
			dropBtn.SetAttribute("TITLE", text)
		}
		report("Selected: " + text)
		return iup.DEFAULT
	}))
	return btn
}

func createColorItem(text, color string) iup.Ihandle {
	btn := iup.Button("").SetAttribute("BGCOLOR", color).SetAttribute("EXPAND", "HORIZONTAL")
	btn.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		dropBtn := iup.GetAttributeHandle(iup.GetDialog(ih), "DROPBUTTON")
		if dropBtn != 0 {
			dropBtn.SetAttribute("SHOWDROPDOWN", "NO")
			dropBtn.SetAttribute("TITLE", text)
		}
		report("Color: " + text + " (" + color + ")")
		return iup.DEFAULT
	}))
	return btn
}

func makeImage(r, g, b byte) iup.Ihandle {
	const n = 16
	pixels := make([]byte, n*n*4)
	for y := 0; y < n; y++ {
		for x := 0; x < n; x++ {
			i := (y*n + x) * 4
			dx, dy := float64(x-8)+0.5, float64(y-8)+0.5
			if dx*dx+dy*dy <= 49 {
				pixels[i+0], pixels[i+1], pixels[i+2], pixels[i+3] = r, g, b, 255
			}
		}
	}
	return iup.ImageRGBA(n, n, pixels)
}
