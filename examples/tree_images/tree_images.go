package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func tree() iup.Ihandle { return iup.GetHandle("tree") }

func setStatus(format string, a ...any) {
	iup.GetHandle("status").SetAttribute("TITLE", fmt.Sprintf(format, a...))
}

func icon(color string) iup.Ihandle {
	pix := make([]byte, 16*16)
	for y := 0; y < 16; y++ {
		for x := 0; x < 16; x++ {
			if x >= 2 && x < 14 && y >= 2 && y < 14 {
				pix[y*16+x] = 1
			}
		}
	}
	return iup.Image(16, 16, pix).SetAttributes(fmt.Sprintf("0=BGCOLOR, 1=%q", color))
}

func build() {
	t := tree()
	t.SetAttribute("TITLE0", "Root")
	t.SetAttribute("ADDBRANCH0", "Vegetables")
	t.SetAttribute("ADDBRANCH0", "Fruits")
	t.SetAttribute("ADDLEAF1", "Apple")
	t.SetAttribute("ADDLEAF2", "Banana")
	t.SetAttribute("ADDLEAF4", "Carrot")
	t.SetAttribute("STATE1", "EXPANDED")
	t.SetAttribute("STATE4", "EXPANDED")

	// Per-node override: give Apple (id 2) a distinct marker image.
	iup.SetAttributeId(t, "IMAGE", 2, "imgMarker")

	updateInfo()
}

func updateInfo() int {
	t := tree()
	info := fmt.Sprintf(
		"HIDELINES=%s (creation-only)\nHIDEBUTTONS=%s (creation-only)\nINDENTATION=%s\nSPACING=%s\nglobal IMAGELEAF/BRANCH set; Apple has per-node IMAGE",
		t.GetAttribute("HIDELINES"), t.GetAttribute("HIDEBUTTONS"),
		t.GetAttribute("INDENTATION"), t.GetAttribute("SPACING"))
	iup.GetHandle("info").SetAttribute("VALUE", info)
	return iup.DEFAULT
}

func bump(name string, delta int) int {
	t := tree()
	v := iup.GetInt(t, name) + delta
	if v < 0 {
		v = 0
	}
	t.SetAttribute(name, fmt.Sprintf("%d", v))
	setStatus("%s=%d", name, v)
	return updateInfo()
}

func btn(label string, cb func() int) iup.Ihandle {
	return iup.Button(label).SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { return cb() }))
}

func main() {
	iup.Open()
	defer iup.Close()

	iup.SetHandle("imgLeaf", icon("40 160 40"))
	iup.SetHandle("imgCollapsed", icon("40 90 200"))
	iup.SetHandle("imgExpanded", icon("230 150 40"))
	iup.SetHandle("imgMarker", icon("210 40 40"))

	// HIDELINES/HIDEBUTTONS are creation-only.
	t := iup.Tree().SetAttributes(`VISIBLELINES=13, VISIBLECOLUMNS=14, EXPAND=YES, HIDELINES=YES,
		IMAGELEAF=imgLeaf, IMAGEBRANCHCOLLAPSED=imgCollapsed, IMAGEBRANCHEXPANDED=imgExpanded`)
	t.SetHandle("tree")

	metrics := iup.Frame(iup.Vbox(
		btn("Indent +", func() int { return bump("INDENTATION", 4) }),
		btn("Indent -", func() int { return bump("INDENTATION", -4) }),
		btn("Spacing +", func() int { return bump("SPACING", 1) }),
		btn("Spacing -", func() int { return bump("SPACING", -1) }),
	).SetAttributes(`NMARGIN=6x6, NGAP=4`)).SetAttribute("TITLE", "Metrics")

	info := iup.Frame(
		iup.Text().SetAttributes(`MULTILINE=YES, READONLY=YES, VISIBLELINES=5, VISIBLECOLUMNS=30`).SetHandle("info"),
	).SetAttribute("TITLE", "Info")

	status := iup.Label("Custom images for leaves/branches; Apple has a per-node image.").
		SetAttribute("EXPAND", "HORIZONTAL").SetHandle("status")

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Hbox(t, metrics, info).SetAttributes(`NGAP=10`),
			status,
		).SetAttributes(`NMARGIN=10x10, NGAP=8`),
	).SetAttribute("TITLE", "Tree images")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	build()
	iup.MainLoop()
}
