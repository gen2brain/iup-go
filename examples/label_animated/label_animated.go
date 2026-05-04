package main

import (
	"bytes"
	_ "embed"
	"fmt"
	"image/gif"
	"log"

	"github.com/gen2brain/iup-go/iup"
)

//go:embed loading.gif
var loadingGIF []byte

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	img, err := gif.DecodeAll(bytes.NewReader(loadingGIF))
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

	dlg := iup.Dialog(lbl)

	dlg.SetAttribute("TITLE", "Animated Label")

	iup.Show(dlg)
	iup.MainLoop()
}
