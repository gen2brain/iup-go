package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	var dlg iup.Ihandle

	caption := iup.FlatLabel("Custom Frame Window").SetAttributes("EXPAND=HORIZONTAL, CANFOCUS=NO, SIZE=x12")
	caption.SetAttribute("NAME", "CUSTOMFRAMECAPTION")
	caption.SetAttribute("FGCOLOR", "255 255 255")
	caption.SetAttribute("ALIGNMENT", "ACENTER:ACENTER")

	closeBtn := iup.FlatButton("X").SetAttributes("SIZE=12x12, PADDING=0x0, CANFOCUS=NO")
	closeBtn.SetAttribute("BGCOLOR", "180 60 60")
	closeBtn.SetAttribute("HLCOLOR", "220 80 80")
	closeBtn.SetAttribute("PSCOLOR", "140 40 40")
	closeBtn.SetAttribute("FGCOLOR", "255 255 255")

	closeBtn.SetCallback("FLAT_ACTION", iup.FlatActionFunc(func(ih iup.Ihandle) int {
		return iup.CLOSE
	}))

	maxBtn := iup.FlatButton("[ ]").SetAttributes("SIZE=12x12, PADDING=0x0, CANFOCUS=NO")
	maxBtn.SetAttribute("BGCOLOR", "80 80 100")
	maxBtn.SetAttribute("HLCOLOR", "100 100 130")
	maxBtn.SetAttribute("PSCOLOR", "60 60 80")
	maxBtn.SetAttribute("FGCOLOR", "255 255 255")

	maxBtn.SetCallback("FLAT_ACTION", iup.FlatActionFunc(func(ih iup.Ihandle) int {
		if dlg.GetInt("MAXIMIZED") != 0 {
			dlg.SetAttribute("PLACEMENT", "")
		} else {
			dlg.SetAttribute("PLACEMENT", "MAXIMIZED")
		}
		iup.Show(dlg)
		return iup.DEFAULT
	}))

	minBtn := iup.FlatButton("—").SetAttributes("SIZE=12x12, PADDING=0x0, CANFOCUS=NO")
	minBtn.SetAttribute("BGCOLOR", "80 80 100")
	minBtn.SetAttribute("HLCOLOR", "100 100 130")
	minBtn.SetAttribute("PSCOLOR", "60 60 80")
	minBtn.SetAttribute("FGCOLOR", "255 255 255")

	minBtn.SetCallback("FLAT_ACTION", iup.FlatActionFunc(func(ih iup.Ihandle) int {
		dlg.SetAttribute("PLACEMENT", "MINIMIZED")
		iup.Show(dlg)
		return iup.DEFAULT
	}))

	buttonBox := iup.Hbox(minBtn, maxBtn, closeBtn).SetAttributes("MARGIN=0x0")

	titleBar := iup.BackgroundBox(
		iup.Hbox(
			caption,
			buttonBox,
		).SetAttributes("ALIGNMENT=ACENTER"),
	).SetAttributes("EXPAND=HORIZONTAL")
	titleBar.SetAttribute("BGCOLOR", "50 50 70")

	content := iup.Vbox(
		iup.Label("This is a custom frame window."),
		iup.Label("The title bar is drawn using IUP controls."),
		iup.Space().SetAttribute("SIZE", "x20"),
		iup.Label("Features:"),
		iup.Label("- Custom close, minimize, maximize buttons"),
		iup.Label("- Draggable title bar (NAME=CUSTOMFRAMECAPTION)"),
		iup.Label("- No native window decorations"),
		iup.Space().SetAttribute("SIZE", "x20"),
		iup.Hbox(
			iup.Button("Sample Button 1"),
			iup.Button("Sample Button 2"),
		).SetAttribute("GAP", "10"),
		iup.Text().SetAttributes("EXPAND=HORIZONTAL"),
	).SetAttributes("MARGIN=20x20, GAP=5, EXPAND=YES")

	dlg = iup.Dialog(
		iup.Vbox(
			titleBar,
			content,
		).SetAttribute("MARGIN", "0x0"),
	).SetAttribute("CUSTOMFRAME", "YES")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}
