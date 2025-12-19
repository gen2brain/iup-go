package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	// Create popup menus
	menu1 := iup.Menu(
		iup.Item("New").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
			fmt.Println("New clicked")
			return iup.DEFAULT
		})),
		iup.Item("Open").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
			fmt.Println("Open clicked")
			return iup.DEFAULT
		})),
		iup.Item("Save").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
			fmt.Println("Save clicked")
			return iup.DEFAULT
		})),
		iup.Separator(),
		iup.Item("Exit").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
			return iup.CLOSE
		})),
	)

	menu2 := iup.Menu(
		iup.Item("Cut").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
			fmt.Println("Cut clicked")
			return iup.DEFAULT
		})),
		iup.Item("Copy").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
			fmt.Println("Copy clicked")
			return iup.DEFAULT
		})),
		iup.Item("Paste").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
			fmt.Println("Paste clicked")
			return iup.DEFAULT
		})),
	)

	menu3 := iup.Menu(
		iup.Submenu("Colors",
			iup.Menu(
				iup.Item("Red").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
					fmt.Println("Red selected")
					return iup.DEFAULT
				})),
				iup.Item("Green").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
					fmt.Println("Green selected")
					return iup.DEFAULT
				})),
				iup.Item("Blue").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
					fmt.Println("Blue selected")
					return iup.DEFAULT
				})),
			),
		),
		iup.Submenu("Sizes",
			iup.Menu(
				iup.Item("Small").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
					fmt.Println("Small selected")
					return iup.DEFAULT
				})),
				iup.Item("Medium").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
					fmt.Println("Medium selected")
					return iup.DEFAULT
				})),
				iup.Item("Large").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
					fmt.Println("Large selected")
					return iup.DEFAULT
				})),
			),
		),
		iup.Separator(),
		iup.Item("About").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
			iup.Message("About", "Popup Menu Example")
			return iup.DEFAULT
		})),
	)

	// Checkable menu
	opt1 := iup.Item("Option 1").SetAttributes("VALUE=ON, AUTOTOGGLE=YES")
	opt2 := iup.Item("Option 2").SetAttributes("VALUE=OFF, AUTOTOGGLE=YES")
	opt3 := iup.Item("Option 3").SetAttributes("VALUE=ON, AUTOTOGGLE=YES")

	menu4 := iup.Menu(
		opt1, opt2, opt3,
		iup.Separator(),
		iup.Item("Show Status").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
			fmt.Printf("Option 1: %s\n", opt1.GetAttribute("VALUE"))
			fmt.Printf("Option 2: %s\n", opt2.GetAttribute("VALUE"))
			fmt.Printf("Option 3: %s\n", opt3.GetAttribute("VALUE"))
			return iup.DEFAULT
		})),
	)

	// Button that shows popup at current mouse position
	btn1 := iup.Button("File Menu (at cursor)")
	btn1.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		iup.Popup(menu1, iup.MOUSEPOS, iup.MOUSEPOS)
		return iup.DEFAULT
	}))

	// Button that shows popup at button position
	btn2 := iup.Button("Edit Menu (at button)")
	btn2.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		_, x, y := iup.GetInt2(ih, "SCREENPOSITION")
		_, _, h := iup.GetInt2(ih, "RASTERSIZE")
		iup.Popup(menu2, x, y+h)
		return iup.DEFAULT
	}))

	// Button that shows popup with submenus
	btn3 := iup.Button("Options Menu (submenus)")
	btn3.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		iup.Popup(menu3, iup.MOUSEPOS, iup.MOUSEPOS)
		return iup.DEFAULT
	}))

	// Button that shows checkable menu
	btn4 := iup.Button("Toggle Menu (checkable)")
	btn4.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		iup.Popup(menu4, iup.MOUSEPOS, iup.MOUSEPOS)
		return iup.DEFAULT
	}))

	// Canvas for right-click popup
	canvasMenu := iup.Menu(
		iup.Item("Canvas Cut").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
			fmt.Println("Canvas Cut clicked")
			return iup.DEFAULT
		})),
		iup.Item("Canvas Copy").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
			fmt.Println("Canvas Copy clicked")
			return iup.DEFAULT
		})),
		iup.Item("Canvas Paste").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
			fmt.Println("Canvas Paste clicked")
			return iup.DEFAULT
		})),
	)

	canvas := iup.Canvas().SetAttributes("RASTERSIZE=300x150, BORDER=YES")
	canvas.SetCallback("BUTTON_CB", iup.ButtonFunc(func(ih iup.Ihandle, button, pressed, x, y int, status string) int {
		if button == iup.BUTTON3 && pressed == 1 {
			// MOUSEPOS for simplicity
			iup.Popup(canvasMenu, iup.MOUSEPOS, iup.MOUSEPOS)
		}
		return iup.DEFAULT
	}))

	canvasLabel := iup.Label("Right-click on the canvas below:")

	// Create the main dialog
	dlg := iup.Dialog(
		iup.Vbox(
			iup.Label("Click buttons to show different popup menus:"),
			iup.Hbox(btn1, btn2, btn3, btn4).SetAttributes("GAP=10"),
			iup.Label(""),
			canvasLabel,
			canvas,
			iup.Fill(),
		).SetAttributes("MARGIN=20x20, GAP=10"),
	).SetAttributes("TITLE=Popup Menu Examples, SIZE=500x300")

	iup.Show(dlg)
	iup.MainLoop()
}
