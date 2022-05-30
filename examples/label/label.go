package main

import (
	"github.com/gen2brain/iup-go/iup"
)

var pixmap = []byte{
	1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 2, 2, 2, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 2, 2, 2, 1, 1, 1, 1, 1,
	1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1,
	1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1,
	1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1,
	1, 1, 1, 2, 2, 1, 1, 2, 2, 2, 1, 1, 1,
	1, 1, 2, 2, 1, 1, 1, 1, 1, 2, 2, 1, 1,
	1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1,
	2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2,
}

func main() {
	iup.Open()
	defer iup.Close()

	imgStar := iup.Image(13, 13, pixmap)
	imgStar.SetAttribute("1", "0 0 0")
	imgStar.SetAttribute("2", "0 198 0")
	iup.SetHandle("imgStar", imgStar)

	lbl := iup.Label("This label has the following attributes set:\nBGCOLOR = 255 255 0\nFGCOLOR = 0 0 255\nFONT = COURIER_NORMAL_14\nTITLE = All text contained here\nALIGNMENT = ACENTER")
	lbl.SetAttributes(`BGCOLOR="255 255 0",FGCOLOR="0 0 255",FONT=COURIER_NORMAL_14,ALIGNMENT=ACENTER`)

	lblExplain := iup.Label("The label on the right has the image of a star")

	lblStar := iup.Label("")
	lblStar.SetAttribute("IMAGE", "imgStar")

	dlg := iup.Dialog(
		iup.Vbox(
			lbl,
			iup.Hbox(lblExplain, lblStar),
		),
	)
	dlg.SetAttribute("TITLE", "Label")

	iup.Show(dlg)
	iup.MainLoop()
}
