package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	sbH := iup.Scrollbar("HORIZONTAL").
		SetAttribute("PAGESIZE", "0.2").
		SetAttribute("LINESTEP", "0.02").
		SetAttribute("PAGESTEP", "0.2").
		SetAttribute("EXPAND", "HORIZONTAL")

	sbV := iup.Scrollbar("VERTICAL").
		SetAttribute("PAGESIZE", "0.3").
		SetAttribute("LINESTEP", "0.01").
		SetAttribute("PAGESTEP", "0.1").
		SetAttribute("EXPAND", "VERTICAL")

	lblH := iup.Label("H: 0.00").SetHandle("lblH").SetAttribute("SIZE", "80x")
	lblV := iup.Label("V: 0.00").SetHandle("lblV").SetAttribute("SIZE", "80x")
	lblOp := iup.Label("Op: -").SetHandle("lblOp").SetAttribute("SIZE", "120x")

	iup.SetCallback(sbH, "VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		val := ih.GetAttribute("VALUE")
		iup.GetHandle("lblH").SetAttribute("TITLE", fmt.Sprintf("H: %s", val))
		return iup.DEFAULT
	}))

	iup.SetCallback(sbV, "VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		val := ih.GetAttribute("VALUE")
		iup.GetHandle("lblV").SetAttribute("TITLE", fmt.Sprintf("V: %s", val))
		return iup.DEFAULT
	}))

	iup.SetCallback(sbH, "SCROLL_CB", iup.ScrollFunc(func(ih iup.Ihandle, op int, posx, posy float64) int {
		iup.GetHandle("lblOp").SetAttribute("TITLE", fmt.Sprintf("Op: %s (%.2f)", opName(op), posx))
		return iup.DEFAULT
	}))

	iup.SetCallback(sbV, "SCROLL_CB", iup.ScrollFunc(func(ih iup.Ihandle, op int, posx, posy float64) int {
		iup.GetHandle("lblOp").SetAttribute("TITLE", fmt.Sprintf("Op: %s (%.2f)", opName(op), posy))
		return iup.DEFAULT
	}))

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Hbox(
				iup.Vbox(sbV).SetAttribute("EXPAND", "VERTICAL"),
				iup.Vbox(lblH, lblV, lblOp).SetAttribute("GAP", "4"),
			).SetAttribute("GAP", "10"),
			sbH,
		).SetAttributes("MARGIN=10x10, GAP=10"),
	).SetAttributes("TITLE=Scrollbar, SIZE=300x200")

	iup.Show(dlg)
	iup.MainLoop()
}

func opName(op int) string {
	switch op {
	case iup.SBUP:
		return "SBUP"
	case iup.SBDN:
		return "SBDN"
	case iup.SBPGUP:
		return "SBPGUP"
	case iup.SBPGDN:
		return "SBPGDN"
	case iup.SBPOSV:
		return "SBPOSV"
	case iup.SBDRAGV:
		return "SBDRAGV"
	case iup.SBLEFT:
		return "SBLEFT"
	case iup.SBRIGHT:
		return "SBRIGHT"
	case iup.SBPGLEFT:
		return "SBPGLEFT"
	case iup.SBPGRIGHT:
		return "SBPGRIGHT"
	case iup.SBPOSH:
		return "SBPOSH"
	case iup.SBDRAGH:
		return "SBDRAGH"
	default:
		return fmt.Sprintf("UNKNOWN(%d)", op)
	}
}
