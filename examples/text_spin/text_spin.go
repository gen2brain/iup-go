package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	valueLabel := iup.Label("Value: 0").SetAttribute("EXPAND", "HORIZONTAL")

	text := iup.Text()
	text.SetAttribute("VALUE", "0")
	text.SetAttribute("SPIN", "YES")
	text.SetAttribute("SPINMIN", "-10")
	text.SetAttribute("SPINMAX", "10")
	text.SetCallback("SPIN_CB", iup.SpinFunc(func(ih iup.Ihandle, pos int) int {
		valueLabel.SetAttribute("TITLE", fmt.Sprintf("Value: %d", pos))
		return iup.DEFAULT
	}))

	text2 := iup.Text()
	text2.SetAttribute("VALUE", "50")
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
	text3.SetAttribute("SPIN", "YES")
	text3.SetAttribute("SPINWRAP", "YES")
	text3.SetAttribute("SPINMIN", "0")
	text3.SetAttribute("SPINMAX", "359")

	text4 := iup.Text()
	text4.SetAttributes("VALUE=0, SPIN=YES, SPINALIGN=LEFT, SPINMIN=0, SPINMAX=20")

	text5 := iup.Text()
	text5.SetAttributes("VALUE=\"level 0\", SPIN=YES, SPINAUTO=NO, SPINMIN=0, SPINMAX=9")
	text5.SetCallback("SPIN_CB", iup.SpinFunc(func(ih iup.Ihandle, pos int) int {
		ih.SetAttribute("VALUE", fmt.Sprintf("level %d", pos))
		return iup.DEFAULT
	}))

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

			iup.Frame(
				iup.Hbox(
					iup.Label("SPINALIGN=LEFT:"),
					text4,
					iup.Label("SPINAUTO=NO, SPIN_CB sets the text:"),
					text5,
				).SetAttribute("GAP", "10"),
			).SetAttribute("TITLE", "Spin position and manual value"),
		).SetAttributes("MARGIN=10x10, GAP=10"),
	).SetAttribute("TITLE", "Spin Example")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}
