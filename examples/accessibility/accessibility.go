//go:build ctrl

package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

// icon builds a 16x16 image where set(r,c) marks the glyph pixels.
// Color index 0 is transparent, 1 is the glyph color.
func icon(set func(r, c int) bool) iup.Ihandle {
	pixels := make([]byte, 16*16)
	for r := 0; r < 16; r++ {
		for c := 0; c < 16; c++ {
			if set(r, c) {
				pixels[r*16+c] = 1
			}
		}
	}
	img := iup.Image(16, 16, pixels)
	img.SetAttribute("0", "BGCOLOR")
	img.SetAttribute("1", "60 60 60")
	return img
}

func abs(v int) int {
	if v < 0 {
		return -v
	}
	return v
}

// action is the toolbar button without a visible label. Only the image is
// shown, so a screen reader has nothing to announce unless ACCESSIBLETITLE is
// set. ACCESSIBLEDESCRIPTION adds the longer spoken hint, and TIP shows the
// same idea to sighted users.
func toolButton(image iup.Ihandle, name, title, description string) iup.Ihandle {
	iup.SetHandle(name, image)
	btn := iup.FlatButton("")
	btn.SetAttribute("IMAGE", name)
	btn.SetAttribute("ACCESSIBLETITLE", title)
	btn.SetAttribute("ACCESSIBLEDESCRIPTION", description)
	btn.SetAttribute("TIP", title)
	iup.SetCallback(btn, "FLAT_ACTION", iup.FlatActionFunc(func(ih iup.Ihandle) int {
		fmt.Printf("%s pressed\n", title)
		return iup.DEFAULT
	}))
	return btn
}

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()
	iup.ControlsOpen()

	newImg := icon(func(r, c int) bool {
		return (r >= 3 && r <= 12 && (c == 7 || c == 8)) || (c >= 3 && c <= 12 && (r == 7 || r == 8))
	})
	openImg := icon(func(r, c int) bool {
		if r == 3 && c >= 2 && c <= 6 {
			return true
		}
		if r == 4 && c >= 2 && c <= 6 {
			return true
		}
		if (r == 5 || r == 12) && c >= 1 && c <= 14 {
			return true
		}
		return r >= 5 && r <= 12 && (c == 1 || c == 14)
	})
	saveImg := icon(func(r, c int) bool {
		if r >= 2 && r <= 8 && (c == 7 || c == 8) {
			return true
		}
		if r >= 8 && r <= 11 && c >= 7-(11-r) && c <= 8+(11-r) {
			return true
		}
		return r == 14 && c >= 3 && c <= 12
	})
	deleteImg := icon(func(r, c int) bool {
		return r >= 2 && r <= 13 && (abs(r-c) <= 1 || abs(r-(15-c)) <= 1)
	})

	toolbar := iup.Hbox(
		toolButton(newImg, "imgNew", "New", "Create a new document"),
		toolButton(openImg, "imgOpen", "Open", "Open an existing document"),
		toolButton(saveImg, "imgSave", "Save", "Save the current document"),
		toolButton(deleteImg, "imgDelete", "Delete", "Delete the current document"),
	).SetAttributes("GAP=4")

	// A labeled button gets its accessible name from TITLE automatically.
	done := iup.FlatButton("Done")
	iup.SetCallback(done, "FLAT_ACTION", iup.FlatActionFunc(func(ih iup.Ihandle) int {
		return iup.CLOSE
	}))

	vbox := iup.Vbox(
		iup.FlatLabel("Image-only buttons labeled for screen readers").SetAttributes("FONTBOLD=YES"),
		toolbar,
		iup.Separator(),
		done,
	).SetAttributes("MARGIN=10x10, GAP=8")

	dlg := iup.Dialog(vbox)
	dlg.SetAttributes(`TITLE="Accessibility", RESIZE=NO`)

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}
