package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func gradientImage(w, h int) iup.Ihandle {
	pix := make([]byte, w*h*3)
	for y := 0; y < h; y++ {
		for x := 0; x < w; x++ {
			i := (y*w + x) * 3
			pix[i] = byte(40 + x*120/w)
			pix[i+1] = byte(60 + y*110/h)
			pix[i+2] = byte(150 - (x+y)*80/(w+h))
		}
	}
	return iup.ImageRGB(w, h, pix)
}

func main() {
	iup.Open()
	defer iup.Close()

	iup.SetHandle("gradient", gradientImage(256, 256))

	header := iup.BackgroundBox(
		iup.Vbox(
			iup.Label("IupBackgroundBox").SetAttributes(`FONT="Helvetica, Bold 16", FGCOLOR="255 255 255"`),
			iup.Label("A native container with a solid color or image background.").
				SetAttributes(`FGCOLOR="235 235 245"`),
			iup.Hbox(
				iup.Toggle("Zoom image").SetAttributes(`FGCOLOR="255 255 255"`).SetCallback("ACTION",
					iup.ToggleActionFunc(func(_ iup.Ihandle, state int) int {
						h := iup.GetHandle("header")
						h.SetAttribute("BACKIMAGEZOOM", state == 1)
						iup.Redraw(h, 0)
						return iup.DEFAULT
					})),
				iup.Toggle("Show image").SetAttributes(`VALUE=ON, FGCOLOR="255 255 255"`).SetCallback("ACTION",
					iup.ToggleActionFunc(func(_ iup.Ihandle, state int) int {
						h := iup.GetHandle("header")
						if state == 1 {
							h.SetAttribute("BACKIMAGE", "gradient")
						} else {
							h.SetAttribute("BACKIMAGE", "")
						}
						iup.Redraw(h, 0)
						return iup.DEFAULT
					})),
			).SetAttributes(`NGAP=16`),
		).SetAttributes(`NMARGIN=18x16, NGAP=8`),
	)
	header.SetHandle("header")
	header.SetAttributes(`BACKIMAGE=gradient, BACKIMAGEZOOM=YES, BACKCOLOR="40 44 82", BORDER=YES`)

	cards := iup.Hbox(
		card("Blue panel", "38 78 120"),
		card("Green panel", "40 96 72"),
	).SetAttributes(`NGAP=12`)

	dlg := iup.Dialog(
		iup.Vbox(header, cards).SetAttributes(`NMARGIN=14x14, NGAP=14`),
	).SetAttributes(`TITLE="IupBackgroundBox", RASTERSIZE=520x420`)

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}

func card(title, bgcolor string) iup.Ihandle {
	box := iup.BackgroundBox(
		iup.Vbox(
			iup.Label(title).SetAttributes(`FONT="Helvetica, Bold 11", FGCOLOR="255 255 255"`),
			iup.Toggle("Enabled").SetAttributes(`VALUE=ON, FGCOLOR="235 235 245"`),
			iup.Frame(
				iup.Vbox(
					iup.Label("Nested in a frame").SetAttributes(`FGCOLOR="235 235 245"`),
					iup.Button("Action"),
				).SetAttributes(`NMARGIN=8x8, NGAP=6`),
			).SetAttributes(`TITLE="Group", FGCOLOR="235 235 245"`),
		).SetAttributes(`NMARGIN=14x14, NGAP=10`),
	)
	box.SetAttributes(fmt.Sprintf(`BGCOLOR="%s", BORDER=YES, EXPAND=YES`, bgcolor))
	return box
}
