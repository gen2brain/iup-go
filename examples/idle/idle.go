package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

var (
	step int
	fact float64
)

func main() {
	iup.Open()
	defer iup.Close()

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Text().SetHandle("mens").SetAttributes(`SIZE=300x`).SetCallback("K_ANY", iup.KAnyFunc(kCrCb)),
			iup.Button("Calculate").SetCallback("ACTION", iup.ActionFunc(actionCb)),
		),
	).SetAttribute("TITLE", "Idle")

	iup.Show(dlg)
	iup.MainLoop()
}

func kCrCb(ih iup.Ihandle, c int) int {
	if c == iup.K_CR {
		return iup.CLOSE
	}
	return iup.DEFAULT
}

func actionCb(ih iup.Ihandle) int {
	step, fact = 0, 1.0

	iup.SetFunction("IDLE_ACTION", iup.IdleFunc(idleFunction))
	iup.SetAttribute(iup.GetHandle("mens"), "VALUE", "Computing...")

	return iup.DEFAULT
}

func idleFunction() int {
	step++
	fact *= float64(step)

	iup.SetAttribute(iup.GetHandle("mens"), "VALUE", fmt.Sprintf("%d -> %10.4f", step, fact))

	if step == 100 {
		iup.SetFunction("IDLE_ACTION", nil)
	}

	return iup.DEFAULT
}
