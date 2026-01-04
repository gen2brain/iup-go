package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	valueLabel := iup.Label("Value: 0").SetAttribute("SIZE", "100x")

	text := iup.Text()
	text.SetAttribute("VALUE", "0")
	text.SetAttribute("SIZE", "50x")
	text.SetAttribute("SPIN", "YES")
	text.SetAttribute("SPINMIN", "-10")
	text.SetAttribute("SPINMAX", "10")
	text.SetCallback("SPIN_CB", iup.SpinFunc(func(ih iup.Ihandle, pos int) int {
		valueLabel.SetAttribute("TITLE", fmt.Sprintf("Value: %d", pos))
		return iup.DEFAULT
	}))

	text2 := iup.Text()
	text2.SetAttribute("VALUE", "50")
	text2.SetAttribute("SIZE", "60x")
	text2.SetAttribute("SPIN", "YES")
	text2.SetAttribute("SPINMIN", "0")
	text2.SetAttribute("SPINMAX", "100")
	text2.SetAttribute("SPININC", "5")
	text2.SetCallback("SPIN_CB", iup.SpinFunc(func(ih iup.Ihandle, pos int) int {
		fmt.Printf("Spin value changed to: %d\n", pos)
		return iup.DEFAULT
	}))

	text3 := iup.Text()
	text3.SetAttribute("VALUE", "0")
	text3.SetAttribute("SIZE", "60x")
	text3.SetAttribute("SPIN", "YES")
	text3.SetAttribute("SPINWRAP", "YES")
	text3.SetAttribute("SPINMIN", "0")
	text3.SetAttribute("SPINMAX", "359")

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Frame(
				iup.Vbox(
					iup.Hbox(
						iup.Label("Range -10 to 10:"),
						text,
					).SetAttribute("GAP", "10"),
					valueLabel,
				).SetAttribute("GAP", "5"),
			).SetAttribute("TITLE", "Basic Spin"),

			iup.Frame(
				iup.Hbox(
					iup.Label("Step of 5 (0-100):"),
					text2,
				).SetAttribute("GAP", "10"),
			).SetAttribute("TITLE", "Custom Step"),

			iup.Frame(
				iup.Hbox(
					iup.Label("Wrap around (0-359):"),
					text3,
				).SetAttribute("GAP", "10"),
			).SetAttribute("TITLE", "Wrap Mode"),
		).SetAttributes("MARGIN=10x10, GAP=10"),
	).SetAttribute("TITLE", "Spin Example")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}
