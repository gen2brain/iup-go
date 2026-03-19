package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func createCheckerImage(size, tileSize int) iup.Ihandle {
	pixels := make([]byte, size*size*4)
	for y := 0; y < size; y++ {
		for x := 0; x < size; x++ {
			idx := (y*size + x) * 4
			tx := (x / tileSize) % 2
			ty := (y / tileSize) % 2
			if tx == ty {
				pixels[idx+0] = 200
				pixels[idx+1] = 200
				pixels[idx+2] = 200
			} else {
				pixels[idx+0] = 240
				pixels[idx+1] = 240
				pixels[idx+2] = 240
			}
			pixels[idx+3] = 255
		}
	}
	return iup.ImageRGBA(size, size, pixels)
}

func createGradientImage(w, h int) iup.Ihandle {
	pixels := make([]byte, w*h*4)
	for y := 0; y < h; y++ {
		for x := 0; x < w; x++ {
			idx := (y*w + x) * 4
			pixels[idx+0] = byte(x * 255 / w)
			pixels[idx+1] = byte(100 + y*100/h)
			pixels[idx+2] = byte(200 - x*100/w)
			pixels[idx+3] = 255
		}
	}
	return iup.ImageRGBA(w, h, pixels)
}

func main() {
	iup.Open()
	defer iup.Close()

	iup.SetHandle("imgChecker", createCheckerImage(64, 8))
	iup.SetHandle("imgGradient", createGradientImage(128, 128))

	lbl := iup.Label("Background:").SetAttribute("SIZE", "60x")
	lblCurrent := iup.Label("Default").SetHandle("lblCurrent").SetAttribute("EXPAND", "HORIZONTAL")

	btnColor1 := iup.Button("Red").SetAttribute("SIZE", "50x")
	btnColor2 := iup.Button("Light Blue").SetAttribute("SIZE", "70x")
	btnColor3 := iup.Button("Custom...").SetAttribute("SIZE", "60x")

	btnImg1 := iup.Button("Checker").SetAttribute("SIZE", "50x")
	btnImg2 := iup.Button("Gradient").SetAttribute("SIZE", "60x")

	btnReset := iup.Button("Reset").SetAttribute("SIZE", "50x")

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Hbox(lbl, lblCurrent),
			iup.Frame(
				iup.Hbox(btnColor1, btnColor2, btnColor3),
			).SetAttribute("TITLE", "Solid Color"),
			iup.Frame(
				iup.Hbox(btnImg1, btnImg2),
			).SetAttribute("TITLE", "Image"),
			iup.Fill(),
			btnReset,
		),
	).SetAttributes("TITLE=Dialog Background, SIZE=320x200, MARGIN=10x10, GAP=5")

	setBackground := func(value string, label string) {
		iup.SetAttribute(dlg, "BACKGROUND", value)
		iup.GetHandle("lblCurrent").SetAttribute("TITLE", label)
	}

	iup.SetCallback(btnColor1, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		setBackground("220 60 60", "220 60 60")
		return iup.DEFAULT
	}))

	iup.SetCallback(btnColor2, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		setBackground("180 210 240", "180 210 240")
		return iup.DEFAULT
	}))

	iup.SetCallback(btnColor3, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		color, ret := iup.GetColor(iup.CENTERPARENT, iup.CENTERPARENT)
		if ret != 0 {
			rgb := fmt.Sprintf("%d %d %d", color.R, color.G, color.B)
			setBackground(rgb, rgb)
		}
		return iup.DEFAULT
	}))

	iup.SetCallback(btnImg1, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		iup.SetAttribute(dlg, "BACKIMAGEZOOM", "NO")
		setBackground("imgChecker", "Checker pattern (tiled)")
		return iup.DEFAULT
	}))

	iup.SetCallback(btnImg2, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		iup.SetAttribute(dlg, "BACKIMAGEZOOM", "YES")
		setBackground("imgGradient", "Gradient (stretched)")
		return iup.DEFAULT
	}))

	iup.SetCallback(btnReset, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		setBackground(iup.GetGlobal("DLGBGCOLOR"), "Default")
		return iup.DEFAULT
	}))

	iup.Show(dlg)
	iup.MainLoop()
}
