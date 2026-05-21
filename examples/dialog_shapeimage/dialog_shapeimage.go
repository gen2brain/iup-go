package main

import (
	"fmt"
	"math"

	"github.com/gen2brain/iup-go/iup"
)

const shapeSize = 360

func makeShapeImage() iup.Ihandle {
	pix := make([]byte, shapeSize*shapeSize*4)
	r := shapeSize/2 - 4
	for y := 0; y < shapeSize; y++ {
		for x := 0; x < shapeSize; x++ {
			i := (y*shapeSize + x) * 4
			dx, dy := x-shapeSize/2, y-shapeSize/2
			dist := math.Sqrt(float64(dx*dx + dy*dy))
			pix[i+0] = 0x20
			pix[i+1] = 0x60
			pix[i+2] = 0xa0
			if dist <= float64(r) {
				pix[i+3] = 0xff
			} else {
				pix[i+3] = 0
			}
		}
	}
	return iup.ImageRGBA(shapeSize, shapeSize, pix).SetHandle("shape_circle")
}

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	makeShapeImage()

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Label("Shaped Dialog").SetAttributes("FONTSIZE=18, ALIGNMENT=ACENTER, EXPAND=HORIZONTAL"),
			iup.Label("The window only renders where the SHAPEIMAGE alpha is non-zero.").
				SetAttributes("ALIGNMENT=ACENTER, EXPAND=HORIZONTAL"),
			iup.Fill(),
			iup.Button("Close").
				SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { return iup.CLOSE })).
				SetAttributes("PADDING=20x10"),
			iup.Fill(),
		).SetAttributes("NMARGIN=40x40, NGAP=10, ALIGNMENT=ACENTER"),
	).SetAttributes("TITLE=ShapeImage, HIDETITLEBAR=YES, BORDER=NO, RESIZE=NO, MENUBOX=NO, MAXBOX=NO, MINBOX=NO, SHAPEIMAGE=shape_circle")
	dlg.SetAttribute("RASTERSIZE", fmt.Sprintf("%dx%d", shapeSize, shapeSize))

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}
