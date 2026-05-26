package main

import (
	"github.com/gen2brain/iup-go/iup"
)

const longText = "This is a long line of text that does not fit in the label width, so it can be cut off with an ellipsis or wrapped onto multiple lines."

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	ellipsisLabel := iup.Label(longText).SetAttributes(`ELLIPSIS=YES, EXPAND=HORIZONTAL`)
	wrapLabel := iup.Label(longText).SetAttributes(`WORDWRAP=YES, EXPAND=HORIZONTAL, RASTERSIZE=x90`)
	selectableLabel := iup.Label("Select this text and copy it (long-press on mobile).").SetAttributes(`SELECTABLE=YES, EXPAND=HORIZONTAL`)

	ellipsisToggle := iup.Toggle("ELLIPSIS")
	ellipsisToggle.SetAttribute("VALUE", "ON")
	ellipsisToggle.SetCallback("ACTION", iup.ToggleActionFunc(func(ih iup.Ihandle, state int) int {
		ellipsisLabel.SetAttribute("ELLIPSIS", yesNo(state))
		iup.Refresh(ellipsisLabel)
		return iup.DEFAULT
	}))

	wrapToggle := iup.Toggle("WORDWRAP")
	wrapToggle.SetAttribute("VALUE", "ON")
	wrapToggle.SetCallback("ACTION", iup.ToggleActionFunc(func(ih iup.Ihandle, state int) int {
		wrapLabel.SetAttribute("WORDWRAP", yesNo(state))
		iup.Refresh(wrapLabel)
		return iup.DEFAULT
	}))

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Frame(ellipsisLabel).SetAttribute("TITLE", "ELLIPSIS"),
			ellipsisToggle,
			iup.Frame(wrapLabel).SetAttribute("TITLE", "WORDWRAP"),
			wrapToggle,
			iup.Frame(selectableLabel).SetAttribute("TITLE", "SELECTABLE"),
		).SetAttributes(`MARGIN=10x10, GAP=5`),
	)
	dlg.SetAttributes(`TITLE="Label Text", RASTERSIZE=360x, SHRINK=YES`)

	iup.Show(dlg)
	iup.MainLoop()
}

func yesNo(state int) string {
	if state == 1 {
		return "YES"
	}
	return "NO"
}
