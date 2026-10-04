//go:build ctrl

package main

import (
	"github.com/gen2brain/iup-go/iup"
)

const size = 16

func icon(name string, inside func(x, y int) bool) {
	pix := make([]byte, size*size*4)
	for y := 0; y < size; y++ {
		for x := 0; x < size; x++ {
			if inside(x, y) {
				pix[(y*size+x)*4+3] = 255
			}
		}
	}
	iup.SetHandle(name, iup.ImageRGBA(size, size, pix))
}

func dist(x, y int) int {
	dx, dy := 2*x-size+1, 2*y-size+1
	return dx*dx + dy*dy
}

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	iup.ControlsOpen()

	icon("tint_disc", func(x, y int) bool { return dist(x, y) < 200 })
	icon("tint_ring", func(x, y int) bool { d := dist(x, y); return d < 220 && d > 70 })
	icon("tint_plus", func(x, y int) bool {
		return (x > 5 && x < 10 && y > 1 && y < 14) || (y > 5 && y < 10 && x > 1 && x < 14)
	})
	icon("tint_square", func(x, y int) bool { return x > 2 && x < 13 && y > 2 && y < 13 })

	tool := func(title, image string) iup.Ihandle {
		return iup.FlatButton(title).SetAttributes(map[string]string{
			"IMAGE": image, "IMAGETINT": "YES", "PADDING": "8x6",
			"FGCOLOR": "70 70 70", "TEXTHLCOLOR": "60 120 200", "TEXTPSCOLOR": "200 60 60",
		})
	}

	disabled := tool("Disabled", "tint_square").SetAttribute("ACTIVE", "NO")
	plain := iup.FlatButton("No tint").SetAttributes("IMAGE=tint_ring, PADDING=8x6")

	labels := iup.Hbox(
		iup.FlatLabel("Red").SetAttributes(`IMAGE=tint_disc, IMAGETINT=YES, FGCOLOR="200 60 60"`),
		iup.FlatLabel("Green").SetAttributes(`IMAGE=tint_disc, IMAGETINT=YES, FGCOLOR="60 160 90"`),
		iup.FlatLabel("Blue").SetAttributes(`IMAGE=tint_disc, IMAGETINT=YES, FGCOLOR="60 120 200"`),
	).SetAttributes("GAP=16")

	tree := iup.FlatTree().SetAttributes(map[string]string{
		"BUTTONSTYLE": "CHEVRON", "HIDELINES": "YES", "IMAGETINT": "YES", "ITEMCORNERRADIUS": "6",
		"IMAGELEAF": "tint_disc", "IMAGEBRANCHCOLLAPSED": "tint_square", "IMAGEBRANCHEXPANDED": "tint_ring",
		"FGCOLOR": "60 120 200", "PSCOLOR": "60 120 200", "TEXTPSCOLOR": "255 255 255", "HLCOLORALPHA": "0",
		"VISIBLELINES": "7", "VISIBLECOLUMNS": "16", "EXPAND": "HORIZONTAL",
	})
	iup.SetAttributeId(tree, "ADDBRANCH", -1, "Project")
	iup.SetAttributeId(tree, "ADDBRANCH", 0, "Docs")
	iup.SetAttributeId(tree, "ADDLEAF", 1, "Readme")
	iup.SetAttributeId(tree, "ADDBRANCH", 0, "Sources")
	iup.SetAttributeId(tree, "ADDLEAF", 1, "Draw")
	iup.SetAttributeId(tree, "ADDLEAF", 1, "Main")

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Hbox(tool("Record", "tint_disc"), tool("Target", "tint_ring"), tool("Add", "tint_plus")).SetAttributes("GAP=4"),
			iup.Hbox(disabled, plain).SetAttributes("GAP=4"),
			labels,
			tree,
		).SetAttributes("GAP=12, MARGIN=12x12"),
	).SetAttribute("TITLE", "Flat tint")

	iup.Show(dlg)
	iup.MainLoop()
}
