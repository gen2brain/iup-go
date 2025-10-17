package main

import (
	"image/png"
	"log"
	"os"

	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	file, err := os.Open("gopher.png")
	if err != nil {
		log.Fatalln(err)
	}

	image, err := png.Decode(file)
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
