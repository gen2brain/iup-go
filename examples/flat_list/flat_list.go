//go:build ctrl

package main

import (
	"fmt"
	"strings"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	iup.ControlsOpen()

	iup.SetHandle("arrow_left", makeImage(60, 140, 220))
	iup.SetHandle("arrow_right", makeImage(200, 60, 60))
	iup.SetHandle("arrow_high", makeImage(120, 190, 255))
	iup.SetHandle("arrow_press", makeImage(20, 70, 140))
	iup.SetHandle("arrow_gray", makeImage(170, 170, 170))

	hint := iup.Label("Drag any item up or down to reorder. Hold Ctrl to copy.").
		SetAttribute("EXPAND", "HORIZONTAL")

	list := iup.FlatList().SetAttributes(map[string]string{
		"1":              "Alpha",
		"2":              "Bravo",
		"3":              "Charlie",
		"4":              "Delta",
		"5":              "Echo",
		"6":              "Foxtrot",
		"7":              "Golf",
		"8":              "Hotel",
		"9":              "India, a much longer item that needs the horizontal scrollbar",
		"SHOWDRAGDROP":   "YES",
		"EXPAND":         "VERTICAL",
		"VISIBLELINES":   "8",
		"FLATSCROLLBAR":  "YES",
		"SHOWARROWS":     "YES",
		"VISIBLECOLUMNS": "20",
		"ITEMFONT1":      "Sans, Bold 10",
		"ARROWIMAGES":    "YES",
		"SB_IMAGELEFT":   "arrow_left", "SB_IMAGELEFTHIGHLIGHT": "arrow_high", "SB_IMAGELEFTPRESS": "arrow_press", "SB_IMAGELEFTINACTIVE": "arrow_gray",
		"SB_IMAGERIGHT": "arrow_right", "SB_IMAGERIGHTHIGHLIGHT": "arrow_high", "SB_IMAGERIGHTPRESS": "arrow_press", "SB_IMAGERIGHTINACTIVE": "arrow_gray",
		"SB_IMAGETOP": "arrow_left", "SB_IMAGEBOTTOM": "arrow_right",
	})
	iup.SetHandle("list", list)

	status := iup.Label("Last action: (none yet)").SetAttribute("EXPAND", "HORIZONTAL")
	order := iup.Label("Order: "+currentOrder(list)).SetAttribute("EXPAND", "HORIZONTAL")
	iup.SetHandle("status", status)
	iup.SetHandle("order", order)

	list.SetCallback("DRAGDROP_CB", iup.DragDropFunc(func(ih iup.Ihandle, dragID, dropID, isShift, isCtrl int) int {
		op := "moved"
		if isCtrl == 1 {
			op = "copied"
		}
		dest := fmt.Sprintf("%d", dropID)
		if dropID == -1 {
			dest = "blank area (end)"
		}
		iup.GetHandle("status").SetAttribute("TITLE",
			fmt.Sprintf("Last action: %s item %d -> %s", op, dragID, dest))
		iup.PostMessage(iup.GetHandle("order"), "refresh", 0, nil)
		return iup.CONTINUE
	}))

	order.SetCallback("POSTMESSAGE_CB", iup.PostMessageFunc(func(ih iup.Ihandle, s string, i int, p any) int {
		ih.SetAttribute("TITLE", "Order: "+currentOrder(iup.GetHandle("list")))
		return iup.DEFAULT
	}))

	dlg := iup.Dialog(iup.Vbox(
		hint,
		iup.Frame(iup.Vbox(list).SetAttributes("NMARGIN=4x4")).SetAttribute("TITLE", "Items"),
		status,
		order,
	).SetAttributes("NMARGIN=10x10, NGAP=8")).SetAttribute("TITLE", "FlatList Reorder")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}

func currentOrder(list iup.Ihandle) string {
	n := list.GetInt("COUNT")
	items := make([]string, 0, n)
	for i := 1; i <= n; i++ {
		items = append(items, list.GetAttribute(fmt.Sprintf("%d", i)))
	}
	return strings.Join(items, ", ")
}

func makeImage(r, g, b byte) iup.Ihandle {
	const n = 12
	pixels := make([]byte, n*n*4)
	for y := 0; y < n; y++ {
		for x := 0; x < n; x++ {
			i := (y*n + x) * 4
			dx, dy := float64(x-6)+0.5, float64(y-6)+0.5
			if dx*dx+dy*dy <= 25 {
				pixels[i+0], pixels[i+1], pixels[i+2], pixels[i+3] = r, g, b, 255
			}
		}
	}
	return iup.ImageRGBA(n, n, pixels)
}
