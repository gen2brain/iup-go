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

	hint := iup.Label("Drag any item up or down to reorder. Hold Ctrl to copy.").
		SetAttribute("EXPAND", "HORIZONTAL")

	list := iup.FlatList().SetAttributes(map[string]string{
		"1":            "Alpha",
		"2":            "Bravo",
		"3":            "Charlie",
		"4":            "Delta",
		"5":            "Echo",
		"6":            "Foxtrot",
		"7":            "Golf",
		"8":            "Hotel",
		"SHOWDRAGDROP": "YES",
		"EXPAND":       "YES",
		"VISIBLELINES": "8",
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
