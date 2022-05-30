package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	frame := iup.Frame(
		iup.Hbox(
			iup.Fill(),
			iup.Label("Frame Attributes:\nFGCOLOR = \"255 0 0\"\nSIZE = \"EIGHTHxEIGHTH\"\nTITLE = \"This is the frame\"\nMARGIN = \"10x10\""),
			iup.Fill(),
		),
	).SetAttributes(`FGCOLOR="255 0 0", SIZE=EIGHTHxEIGHTH, TITLE="This is the frame", MARGIN=10x10`)

	dlg := iup.Dialog(frame).SetAttribute("TITLE", "Frame")
	iup.Show(dlg)
	iup.MainLoop()
}
