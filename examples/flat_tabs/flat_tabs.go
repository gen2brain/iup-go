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

func init() { iup.EntryPoint(main) }

func page(title, rgb string) iup.Ihandle {
	vbox := iup.Vbox(
		iup.Label("Content of the \""+title+"\" tab.").SetAttributes("NMARGIN=15x15"),
		iup.Button("Button "+title),
	).SetAttributes("NMARGIN=15x15, GAP=8")
	vbox.SetAttribute("TABTITLE", title)
	if rgb != "" {
		vbox.SetAttribute("TABBACKCOLOR", rgb)
	}
	return vbox
}

func main() {
	iup.Open()
	defer iup.Close()

	iup.ControlsOpen()

	driver := iup.GetGlobal("DRIVER")

	txtLog := iup.Text()
	txtLog.SetAttributes("MULTILINE=YES, EXPAND=HORIZONTAL, READONLY=YES, VISIBLELINES=6")
	txtLog.SetAttribute("VALUE", "=== IUP FlatTabs reorder ===\n"+
		"Driver: "+driver+"\n"+
		"Drag a tab title onto another to reorder it.\n"+
		"An insertion line shows where it will land; colors follow the tab.\n---\n")
	iup.SetHandle("log", txtLog)

	reorderCB := func(ih iup.Ihandle, oldPos, newPos int) int {
		logMsg(fmt.Sprintf("REORDER_CB: tab moved from %d to %d", oldPos, newPos))
		return iup.DEFAULT
	}

	tabChangeCB := func(ih iup.Ihandle, newPos, oldPos int) int {
		logMsg(fmt.Sprintf("TABCHANGEPOS_CB: position %d -> %d", oldPos, newPos))
		return iup.DEFAULT
	}

	tabs := iup.FlatTabs(
		page("Red", "230 120 120"),
		page("Green", "120 200 120"),
		page("Blue", "120 150 230"),
		page("Amber", "235 190 100"),
	)
	tabs.SetAttributes("ALLOWREORDER=YES, EXPAND=YES")
	iup.SetCallback(tabs, "REORDER_CB", iup.ReorderFunc(reorderCB))
	iup.SetCallback(tabs, "TABCHANGEPOS_CB", iup.TabChangePosFunc(tabChangeCB))

	vboxMain := iup.Vbox(
		iup.Label("IUP FlatTabs - drag tab titles to reorder").SetAttributes(`FONT="Sans, Bold 12"`),
		tabs,
		iup.Frame(txtLog).SetAttributes(`TITLE="Event Log", NMARGIN=5x5`),
	).SetAttributes("NMARGIN=10x10, GAP=10")

	dlg := iup.Dialog(vboxMain).SetAttributes(`TITLE="IUP FlatTabs Reorder", RASTERSIZE=560x400`)

	iup.Show(dlg)
	iup.MainLoop()
}
