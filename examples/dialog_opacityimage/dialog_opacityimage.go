package main

import (
	"fmt"
	"math"

	"github.com/gen2brain/iup-go/iup"
)

const opacitySize = 360

// Splash-style image: a soft-edged colored disc on a transparent background.
// With OPACITYIMAGE the dialog cannot have children; the image IS the visual.
func makeOpacityImage() iup.Ihandle {
	pix := make([]byte, opacitySize*opacitySize*4)
	outer := float64(opacitySize)/2 - 2
	inner := outer - 18
	for y := 0; y < opacitySize; y++ {
		for x := 0; x < opacitySize; x++ {
			i := (y*opacitySize + x) * 4
			dx := float64(x - opacitySize/2)
			dy := float64(y - opacitySize/2)
			dist := math.Sqrt(dx*dx + dy*dy)

			t := 1.0 - dist/outer
			if t < 0 {
				t = 0
			}
			pix[i+0] = byte(0x20 + int(0x80*t))
			pix[i+1] = byte(0x40 + int(0xa0*t))
			pix[i+2] = byte(0x80 + int(0x60*t))

			switch {
			case dist <= inner:
				pix[i+3] = 0xff
			case dist <= outer:
				pix[i+3] = byte(0xff * (1.0 - (dist-inner)/(outer-inner)))
			default:
				pix[i+3] = 0
			}
		}
	}
	return iup.ImageRGBA(opacitySize, opacitySize, pix).SetHandle("opacity_disc")
}

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	makeOpacityImage()

	dlg := iup.Dialog(iup.Vbox()).
		SetAttributes("TITLE=OpacityImage, HIDETITLEBAR=YES, BORDER=NO, RESIZE=NO, MENUBOX=NO, MAXBOX=NO, MINBOX=NO, OPACITYIMAGE=opacity_disc")
	dlg.SetAttribute("RASTERSIZE", fmt.Sprintf("%dx%d", opacitySize, opacitySize))

	iup.Timer().
		SetAttribute("TIME", "3000").
		SetCallback("ACTION_CB", iup.TimerActionFunc(func(iup.Ihandle) int { return iup.CLOSE })).
		SetAttribute("RUN", "YES")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}
