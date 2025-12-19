package main

import (
	"fmt"
	"time"

	"github.com/gen2brain/iup-go/iup"
)

func logMsg(msg string) {
	log := iup.GetHandle("log")
	if log != 0 {
		timestamp := time.Now().Format("15:04:05")
		log.SetAttribute("APPEND", fmt.Sprintf("[%s] %s\n", timestamp, msg))
	}
	fmt.Println(msg)
}

func main() {
	iup.Open()
	defer iup.Close()

	driver := iup.GetGlobal("DRIVER")

	// Log widget
	txtLog := iup.Text()
	txtLog.SetAttributes("MULTILINE=YES, EXPAND=HORIZONTAL, READONLY=YES, VISIBLELINES=5")
	txtLog.SetAttribute("VALUE", "=== IUP Tabs ===\n"+
		"Driver: "+driver+"\n"+
		"Callbacks logged below:\n"+
		"---\n")
	iup.SetHandle("log", txtLog)

	// Create tabs 1 (top)
	vbox1 := iup.Vbox(
		iup.Label("Inside Tab A"),
		iup.Button("Button A"),
		iup.Text().SetAttributes("EXPAND=HORIZONTAL"),
	).SetAttributes("MARGIN=10x10, GAP=5")
	vbox1.SetAttribute("TABTITLE", "Tab A")

	vbox2 := iup.Vbox(
		iup.Label("Inside Tab B"),
		iup.Button("Button B"),
		iup.Toggle("Toggle B"),
	).SetAttributes("MARGIN=10x10, GAP=5")
	vbox2.SetAttribute("TABTITLE", "Tab B")

	vbox3 := iup.Vbox(
		iup.Label("Inside Tab C"),
		iup.List().SetAttributes("1=Item 1, 2=Item 2, 3=Item 3"),
	).SetAttributes("MARGIN=10x10, GAP=5")
	vbox3.SetAttribute("TABTITLE", "Tab C")

	tabs1 := iup.Tabs(vbox1, vbox2, vbox3)
	tabs1.SetAttributes("SHOWCLOSE=YES")

	// Create tabs 2 (left side)
	vboxD := iup.Vbox(
		iup.Label("Inside Tab D"),
		iup.Button("Button D"),
	).SetAttributes("MARGIN=10x10, GAP=5")
	vboxD.SetAttribute("TABTITLE", "Tab D")

	vboxE := iup.Vbox(
		iup.Label("Inside Tab E"),
		iup.Button("Button E"),
	).SetAttributes("MARGIN=10x10, GAP=5")
	vboxE.SetAttribute("TABTITLE", "Tab E")

	tabs2 := iup.Tabs(vboxD, vboxE)
	tabs2.SetAttributes("TABTYPE=LEFT, ALLOWREORDER=YES")

	// Create tabs 3 (left side with VERTICAL text orientation)
	vboxF := iup.Vbox(
		iup.Label("Inside Tab F"),
		iup.Button("Button F"),
	).SetAttributes("MARGIN=10x10, GAP=5")
	vboxF.SetAttribute("TABTITLE", "Tab F")

	vboxG := iup.Vbox(
		iup.Label("Inside Tab G"),
		iup.Button("Button G"),
	).SetAttributes("MARGIN=10x10, GAP=5")
	vboxG.SetAttribute("TABTITLE", "Tab G")

	tabs3 := iup.Tabs(vboxF, vboxG)
	tabs3.SetAttributes("TABTYPE=LEFT, TABORIENTATION=VERTICAL")

	iup.SetHandle("tabs1", tabs1)
	iup.SetHandle("tabs2", tabs2)
	iup.SetHandle("tabs3", tabs3)

	// TABCHANGE_CB - Called when the user changes the active tab
	tabChangeCB := func(ih iup.Ihandle, newChild, oldChild iup.Ihandle) int {
		newTitle := ""
		oldTitle := ""
		if newChild != 0 {
			newTitle = newChild.GetAttribute("TABTITLE")
		}
		if oldChild != 0 {
			oldTitle = oldChild.GetAttribute("TABTITLE")
		}

		tabsName := "tabs1"
		if ih == iup.GetHandle("tabs2") {
			tabsName = "tabs2"
		} else if ih == iup.GetHandle("tabs3") {
			tabsName = "tabs3"
		}

		logMsg(fmt.Sprintf("TABCHANGE_CB (%s): '%s' -> '%s'", tabsName, oldTitle, newTitle))
		return iup.DEFAULT
	}

	// TABCHANGEPOS_CB - Called when the user changes the active tab (with positions)
	tabChangePosCB := func(ih iup.Ihandle, newPos, oldPos int) int {
		tabsName := "tabs1"
		if ih == iup.GetHandle("tabs2") {
			tabsName = "tabs2"
		} else if ih == iup.GetHandle("tabs3") {
			tabsName = "tabs3"
		}

		logMsg(fmt.Sprintf("TABCHANGEPOS_CB (%s): position %d -> %d", tabsName, oldPos, newPos))
		return iup.DEFAULT
	}

	// TABCLOSE_CB - Called when a user clicks the close button on a tab
	tabCloseCB := func(ih iup.Ihandle, pos int) int {
		tabsName := "tabs1"
		if ih == iup.GetHandle("tabs2") {
			tabsName = "tabs2"
		} else if ih == iup.GetHandle("tabs3") {
			tabsName = "tabs3"
		}

		// Get the child at position
		child := iup.GetChild(ih, pos)

		title := ""
		if child != 0 {
			title = child.GetAttribute("TABTITLE")
		}

		logMsg(fmt.Sprintf("TABCLOSE_CB (%s): closing tab %d ('%s')", tabsName, pos, title))

		// Return DEFAULT to allow closing, or IGNORE to prevent
		return iup.IGNORE
	}

	// RIGHTCLICK_CB - Called when user right-clicks on a tab
	rightClickCB := func(ih iup.Ihandle, pos int) int {
		tabsName := "tabs1"
		if ih == iup.GetHandle("tabs2") {
			tabsName = "tabs2"
		} else if ih == iup.GetHandle("tabs3") {
			tabsName = "tabs3"
		}

		// Get the child at position
		child := iup.GetChild(ih, pos)

		title := ""
		if child != 0 {
			title = child.GetAttribute("TABTITLE")
		}

		logMsg(fmt.Sprintf("RIGHTCLICK_CB (%s): right-clicked tab %d ('%s')", tabsName, pos, title))
		return iup.DEFAULT
	}

	// Set callbacks for tabs1
	iup.SetCallback(tabs1, "TABCHANGE_CB", iup.TabChangeFunc(tabChangeCB))
	iup.SetCallback(tabs1, "TABCHANGEPOS_CB", iup.TabChangePosFunc(tabChangePosCB))
	iup.SetCallback(tabs1, "TABCLOSE_CB", iup.TabCloseFunc(tabCloseCB))
	iup.SetCallback(tabs1, "RIGHTCLICK_CB", iup.RightClickFunc(rightClickCB))

	// Set callbacks for tabs2
	iup.SetCallback(tabs2, "TABCHANGE_CB", iup.TabChangeFunc(tabChangeCB))
	iup.SetCallback(tabs2, "TABCHANGEPOS_CB", iup.TabChangePosFunc(tabChangePosCB))
	iup.SetCallback(tabs2, "TABCLOSE_CB", iup.TabCloseFunc(tabCloseCB))
	iup.SetCallback(tabs2, "RIGHTCLICK_CB", iup.RightClickFunc(rightClickCB))

	// Set callbacks for tabs3
	iup.SetCallback(tabs3, "TABCHANGE_CB", iup.TabChangeFunc(tabChangeCB))
	iup.SetCallback(tabs3, "TABCHANGEPOS_CB", iup.TabChangePosFunc(tabChangePosCB))
	iup.SetCallback(tabs3, "TABCLOSE_CB", iup.TabCloseFunc(tabCloseCB))
	iup.SetCallback(tabs3, "RIGHTCLICK_CB", iup.RightClickFunc(rightClickCB))

	// Layout
	vboxMain := iup.Vbox(
		iup.Label("IUP Tabs").SetAttributes(`FONT="Sans, Bold 12"`),
		iup.Hbox(tabs1, tabs2, tabs3).SetAttribute("GAP", "10"),
		iup.Frame(txtLog).SetAttributes(`TITLE="Event Log", MARGIN=5x5`),
	).SetAttributes("MARGIN=10x10, GAP=10")

	dlg := iup.Dialog(vboxMain).SetAttributes(`TITLE="IUP Tabs Demo"`)

	iup.Show(dlg)

	logMsg(fmt.Sprintf("Driver: %s", driver))
	logMsg("tabs2: LEFT tabs with HORIZONTAL text (default)")
	logMsg("tabs3: LEFT tabs with VERTICAL text (rotated 90°)")
	logMsg("Note: tabs2 has ALLOWREORDER=YES, try dragging tabs!")

	iup.MainLoop()
}
