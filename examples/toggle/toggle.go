package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	img1 := iup.Image(16, 16, imgPixmap1)
	img2 := iup.Image(16, 16, imgPixmap2)
	img1.SetHandle("img1").SetAttributes(`1="255 255 255", 2="0 192 0"`)
	img2.SetHandle("img2").SetAttributes(`1="255 255 255", 2="0 192 0"`)

	toggle1 := iup.Toggle("toggle with image")
	toggle2 := iup.Toggle("deactivated toggle with image")
	toggle3 := iup.Toggle("regular toggle")
	toggle4 := iup.Toggle("blue foreground color")
	toggle5 := iup.Toggle("deactivated toggle")
	toggle6 := iup.Toggle("toggle with Courier 14 Bold font")

	iup.SetCallback(toggle1, "ACTION", iup.ToggleActionFunc(toggleCb))
	iup.SetCallback(toggle3, "ACTION", iup.ToggleActionFunc(toggleCb))
	iup.SetCallback(toggle4, "ACTION", iup.ToggleActionFunc(toggleCb))
	iup.SetCallback(toggle6, "ACTION", iup.ToggleActionFunc(toggleCb))

	iup.SetAttribute(toggle1, "IMAGE", "img1")            // Toggle 1 uses image
	iup.SetAttribute(toggle2, "IMAGE", "img2")            // Toggle 2 uses image
	iup.SetAttribute(toggle2, "ACTIVE", "NO")             // Toggle 2 inactive
	iup.SetAttribute(toggle4, "FGCOLOR", "0 0 255")       // Toggle 4 has blue foreground color
	iup.SetAttribute(toggle5, "ACTIVE", "NO")             // Toggle 6 inactive
	iup.SetAttribute(toggle6, "FONT", "Courier, Bold 14") // Toggle 8 has Courier 14 Bold font

	box := iup.Vbox(
		toggle1,
		toggle2,
		toggle3,
		toggle4,
		toggle5,
		toggle6,
	)

	toggles := iup.Radio(box)
	dlg := iup.Dialog(toggles).SetAttributes("TITLE=Toggle, MARGIN=5x5, GAP=5, RESIZE=NO")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}

func toggleCb(ih iup.Ihandle, state int) int {
	pos := iup.GetChildPos(iup.GetParent(ih), ih)
	fmt.Printf("toggle%d_cb(state=%d)\n", pos+1, state)
	return iup.DEFAULT
}

var imgPixmap1 = []byte{
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
}

var imgPixmap2 = []byte{
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
}
