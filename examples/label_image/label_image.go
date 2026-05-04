package main

import (
	"bytes"
	_ "embed"
	"image/png"
	"log"

	"github.com/gen2brain/iup-go/iup"
)

//go:embed gopher.png
var gopherPNG []byte

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	image, err := png.Decode(bytes.NewReader(gopherPNG))
	if err != nil {
		log.Fatalln(err)
	}

	iup.ImageFromImage(image).SetHandle("myimage")

	lbl := iup.Label("")
	lbl.SetAttribute("IMAGE", "myimage")

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Fill(),
			iup.Hbox(
				iup.Fill(),
				lbl,
				iup.Fill(),
			),
			iup.Fill(),
		),
	).SetAttribute("TITLE", "Image")

	iup.Show(dlg)
	iup.MainLoop()
}
