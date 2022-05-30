package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	iup.SetGlobal("UTF8MODE", "YES")

	text := iup.Text()

	text.SetAttribute("READONLY", "YES")
	text.SetAttribute("EXPAND", "HORIZONTAL")

	iup.SetHandle("text", text)

	imgRelease := iup.Image(16, 16, pixmapRelease)
	imgRelease.SetAttributes(map[string]string{
		"1": "215 215 215",
		"2": "40 40 40",
		"3": "30 50 210",
		"4": "240 0 0",
	})

	iup.SetHandle("imgRelease", imgRelease)

	imgPress := iup.Image(16, 16, pixmapPress)
	imgPress.SetAttributes(map[string]string{
		"1": "40 40 40",
		"2": "215 215 215",
		"3": "0 20 180",
		"4": "210 0 0",
	})

	iup.SetHandle("imgPress", imgPress)

	imgInactive := iup.Image(16, 16, pixmapInactive)

	imgInactive.SetAttributes(map[string]string{
		"1": "215 215 215",
		"2": "40 40 40",
		"3": "100 100 100",
		"4": "200 200 200",
	})

	iup.SetHandle("imgInactive", imgInactive)

	btnImage := iup.Button("Button with image")

	btnImage.SetAttributes(`IMAGE=imgRelease, IMPRESS=imgPress, IMINACTIVE=imgInactive`)

	btnBig := iup.Button("Big useless button").SetAttribute("SIZE", "EIGHTHxEIGHTH")
	btnExit := iup.Button("Exit")
	btnOnOff := iup.Button("on/off")

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Hbox(
				btnImage, btnOnOff, btnExit,
			).SetAttributes(`"MARGIN=10x10, GAP=10`),
			text,
			btnBig,
		).SetAttributes(`MARGIN=10x10, GAP=10`),
	)

	iup.SetAttributes(dlg, `EXPAND=YES, TITLE="Button", RESIZE=NO, MENUBOX=NO, MAXBOX=NO, MINBOX=NO`)

	iup.SetCallback(btnExit, "ACTION", iup.ActionFunc(btnExitCb))
	iup.SetCallback(btnOnOff, "ACTION", iup.ActionFunc(btnOnOffCb))
	iup.SetCallback(btnImage, "BUTTON_CB", iup.ButtonFunc(btnImageCb))
	iup.SetCallback(btnBig, "BUTTON_CB", iup.ButtonFunc(btnBigCb))

	iup.Show(dlg)
	iup.MainLoop()
}

func btnExitCb(ih iup.Ihandle) int {
	return iup.CLOSE
}

func btnOnOffCb(ih iup.Ihandle) int {
	btnImage := iup.GetHandle("btnImage")

	fmt.Println("ACTIVE", btnImage.GetInt("ACTIVE"))

	if btnImage.GetInt("ACTIVE") != 0 {
		btnImage.SetAttribute("ACTIVE", "NO")
	} else {
		iup.SetAttribute(btnImage, "ACTIVE", "YES")
	}

	return iup.DEFAULT
}

func btnImageCb(ih iup.Ihandle, button, pressed, x, y int, status string) int {
	if button == iup.BUTTON1 {
		text := iup.GetHandle("text")
		if pressed != 0 {
			text.SetAttribute("VALUE", "Red button pressed")
		} else {
			text.SetAttribute("VALUE", "Red button released")
		}
	}

	return iup.DEFAULT
}

func btnBigCb(ih iup.Ihandle, button, pressed, x, y int, status string) int {
	fmt.Printf("BUTTON_CB(button=%v,pressed=%v)\n", button, pressed)
	return iup.DEFAULT
}

var pixmapRelease = []byte{
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2,
	1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2,
	1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2,
	1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2,
	1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2,
	1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2,
	1, 1, 3, 3, 3, 3, 3, 3, 4, 4, 3, 3, 3, 3, 2, 2,
	1, 1, 3, 3, 3, 3, 3, 4, 4, 4, 4, 3, 3, 3, 2, 2,
	1, 1, 3, 3, 3, 3, 3, 4, 4, 4, 4, 3, 3, 3, 2, 2,
	1, 1, 3, 3, 3, 3, 3, 3, 4, 4, 3, 3, 3, 3, 2, 2,
	1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2,
	1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2,
	1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2,
	1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2,
	1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
}

var pixmapPress = []byte{
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2,
	1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2,
	1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2,
	1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2,
	1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2,
	1, 1, 3, 3, 3, 3, 3, 4, 4, 3, 3, 3, 3, 3, 2, 2,
	1, 1, 3, 3, 3, 3, 4, 4, 4, 4, 3, 3, 3, 3, 2, 2,
	1, 1, 3, 3, 3, 3, 4, 4, 4, 4, 3, 3, 3, 3, 2, 2,
	1, 1, 3, 3, 3, 3, 3, 4, 4, 3, 3, 3, 3, 3, 2, 2,
	1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2,
	1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2,
	1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2,
	1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2,
	1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2,
	1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
}

var pixmapInactive = []byte{
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2,
	1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2,
	1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2,
	1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2,
	1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2,
	1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2,
	1, 1, 3, 3, 3, 3, 3, 3, 4, 4, 3, 3, 3, 3, 2, 2,
	1, 1, 3, 3, 3, 3, 3, 4, 4, 4, 4, 3, 3, 3, 2, 2,
	1, 1, 3, 3, 3, 3, 3, 4, 4, 4, 4, 3, 3, 3, 2, 2,
	1, 1, 3, 3, 3, 3, 3, 3, 4, 4, 3, 3, 3, 3, 2, 2,
	1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2,
	1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2,
	1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2,
	1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2,
	1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
}
