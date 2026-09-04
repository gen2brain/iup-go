package main

import (
	"bytes"
	_ "embed"
	"fmt"
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

	img := iup.GetHandle("myimage")
	info := iup.Label(fmt.Sprintf("%sx%s BPP=%s CHANNELS=%s SCALED=%s ORIGINALSCALE=%s",
		img.GetAttribute("WIDTH"), img.GetAttribute("HEIGHT"), img.GetAttribute("BPP"), img.GetAttribute("CHANNELS"),
		img.GetAttribute("SCALED"), img.GetAttribute("ORIGINALSCALE")))

	cursor := makeCursor()
	cursor.SetAttribute("HOTSPOT", "15:15")
	iup.SetHandle("cross", cursor)
	lbl.SetAttribute("CURSOR", "cross")

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Fill(),
			iup.Hbox(
				iup.Fill(),
				lbl,
				iup.Fill(),
			),
			iup.Fill(),
			info,
			iup.Label("The cursor over the image is a 32x32 cross with HOTSPOT=15:15"),
		).SetAttributes("NMARGIN=10x10, NGAP=5"),
	).SetAttribute("TITLE", "Image")

	iup.Show(dlg)
	iup.MainLoop()
}

func makeCursor() iup.Ihandle {
	const n = 32
	pixels := make([]byte, n*n)
	for i := 0; i < n; i++ {
		pixels[15*n+i] = 1
		pixels[i*n+15] = 1
	}
	return iup.Image(n, n, pixels).SetAttributes(`0="BGCOLOR", 1="0 0 0"`)
}
