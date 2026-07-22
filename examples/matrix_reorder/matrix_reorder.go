//go:build ctrl

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
	iup.ControlsOpen()
	defer iup.Close()

	mat := iup.Matrix()
	mat.SetAttributes(map[string]string{
		"NUMCOL":         "4",
		"NUMLIN":         "5",
		"NUMCOL_VISIBLE": "4",
		"NUMLIN_VISIBLE": "5",
		"RESIZEMATRIX":   "YES",
		"ALLOWREORDER":   "YES",
	})

	titles := []string{"Name", "Quantity", "Price", "Total"}
	for c, t := range titles {
		mat.SetAttribute(fmt.Sprintf("0:%d", c+1), t)
	}
	data := [][]string{
		{"Apples", "12", "0.50", "6.00"},
		{"Bread", "3", "1.20", "3.60"},
		{"Coffee", "2", "4.75", "9.50"},
		{"Eggs", "24", "0.20", "4.80"},
		{"Flour", "5", "0.90", "4.50"},
	}
	for l, row := range data {
		mat.SetAttribute(fmt.Sprintf("%d:0", l+1), fmt.Sprintf("%d", l+1))
		for c, v := range row {
			mat.SetAttribute(fmt.Sprintf("%d:%d", l+1, c+1), v)
		}
	}

	mat.SetCallback("REORDER_CB", iup.ReorderFunc(func(ih iup.Ihandle, oldPos, newPos int) int {
		logMsg(fmt.Sprintf("REORDER_CB: column moved from %d to %d", oldPos, newPos))
		return iup.DEFAULT
	}))

	txtLog := iup.Text()
	txtLog.SetAttributes("MULTILINE=YES, EXPAND=HORIZONTAL, READONLY=YES, VISIBLELINES=5")
	txtLog.SetAttribute("VALUE", "Drag a column title left or right to reorder the column.\n"+
		"The whole column (title, values, width) moves with it.\n")
	iup.SetHandle("log", txtLog)

	box := iup.Vbox(
		iup.Label("IupMatrix - drag a column title to reorder").SetAttributes(`FONT="Sans, Bold 12"`),
		mat,
		iup.Frame(txtLog).SetAttributes(`TITLE="Event Log", NMARGIN=5x5`),
	).SetAttributes("NMARGIN=10x10, GAP=10")

	dlg := iup.Dialog(box).SetAttribute("TITLE", "IupMatrix Reorder")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}
