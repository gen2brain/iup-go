package main

import (
	"fmt"
	"time"

	"github.com/gen2brain/iup-go/iup"
)

func logMsg(msg string) {
	log := iup.GetHandle("log")
	if log != 0 {
		log.SetAttribute("APPEND", fmt.Sprintf("[%s] %s", time.Now().Format("15:04:05"), msg))
	}
	fmt.Println(msg)
}

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	table := iup.Table()
	table.SetAttributes(map[string]string{
		"NUMCOL":       "3",
		"NUMLIN":       "6",
		"TITLE1":       "Name",
		"TITLE2":       "Role",
		"TITLE3":       "City",
		"SHOWDRAGDROP": "YES",
		"EXPAND":       "YES",
	})

	rows := [][]string{
		{"Alice", "Engineer", "New York"},
		{"Bob", "Designer", "London"},
		{"Charlie", "Manager", "Tokyo"},
		{"Diana", "Analyst", "Paris"},
		{"Eve", "Writer", "Berlin"},
		{"Frank", "Support", "Sydney"},
	}
	for l, row := range rows {
		for c, v := range row {
			iup.SetAttributeId2(table, "", l+1, c+1, v)
		}
	}

	table.SetCallback("DRAGDROP_CB", iup.DragDropFunc(func(ih iup.Ihandle, dragId, dropId, isShift, isControl int) int {
		logMsg(fmt.Sprintf("DRAGDROP_CB: row %d dropped at %d", dragId, dropId))
		return iup.CONTINUE
	}))

	txtLog := iup.Text()
	txtLog.SetAttributes("MULTILINE=YES, EXPAND=HORIZONTAL, READONLY=YES, VISIBLELINES=5")
	txtLog.SetAttribute("VALUE", "Drag a row onto another to reorder it.\n")
	iup.SetHandle("log", txtLog)

	box := iup.Vbox(
		iup.Label("IupTable - drag a row to reorder").SetAttributes(`FONT="Sans, Bold 12"`),
		table,
		iup.Frame(txtLog).SetAttributes(`TITLE="Event Log", NMARGIN=5x5`),
	).SetAttributes("NMARGIN=10x10, GAP=10")

	dlg := iup.Dialog(box).SetAttributes(`TITLE="IupTable Reorder", RASTERSIZE=460x420`)

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}
