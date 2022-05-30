package main

import (
	"fmt"
	"image/gif"
	"log"
	"os"

	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	file, err := os.Open("loading.gif")
	if err != nil {
		log.Fatalln(err)
	}

	img, err := gif.DecodeAll(file)
	if err != nil {
		log.Fatalln(err)
	}

	animation := iup.User()
	animation.SetAttribute("FRAMETIME", "125")

	for idx, i := range img.Image {
		name := fmt.Sprintf("loading%d", idx)
		iup.ImageFromImage(i).SetHandle(name)
		iup.Append(animation, iup.GetHandle(name))
	}

	lbl := iup.AnimatedLabel(animation)
	lbl.SetAttribute("START", "YES")

	dlg := iup.Dialog(
		iup.Vbox(
			lbl,
		),
	)

	dlg.SetAttribute("TITLE", "AnimatedLabel")

	iup.Show(dlg)
	iup.MainLoop()
}
