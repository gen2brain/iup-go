package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

var log iup.Ihandle

func logf(format string, a ...interface{}) {
	msg := fmt.Sprintf(format, a...)
	log.SetAttribute("APPEND", msg)
	fmt.Println(msg)
}

// wire tags a control with a name and logs its focus in/out.
func wire(h iup.Ihandle, name string) iup.Ihandle {
	h.SetAttribute("NAME", name)
	h.SetCallback("GETFOCUS_CB", iup.GetFocusFunc(func(ih iup.Ihandle) int {
		logf("GETFOCUS  %s", ih.GetAttribute("NAME"))
		return iup.DEFAULT
	}))
	h.SetCallback("KILLFOCUS_CB", iup.KillFocusFunc(func(ih iup.Ihandle) int {
		logf("KILLFOCUS %s", ih.GetAttribute("NAME"))
		return iup.DEFAULT
	}))
	return h
}

func main() {
	iup.Open()
	defer iup.Close()

	log = iup.Text().SetAttributes(map[string]string{
		"MULTILINE":      "YES",
		"READONLY":       "YES",
		"EXPAND":         "YES",
		"VISIBLELINES":   "20",
		"VISIBLECOLUMNS": "28",
	})

	text := wire(iup.Text(), "text").SetAttributes(map[string]string{
		"EXPAND": "HORIZONTAL", "VALUE": "single line",
	})

	multi := wire(iup.Text(), "multiline").SetAttributes(map[string]string{
		"MULTILINE": "YES", "EXPAND": "HORIZONTAL", "VISIBLELINES": "3",
		"VALUE": "multi\nline\nedit",
	})

	spin := wire(iup.Text(), "spin").SetAttributes(map[string]string{
		"SPIN": "YES", "SPINMIN": "0", "SPINMAX": "10", "VALUE": "5",
	})

	button := wire(iup.Button("Button"), "button")

	toggle := wire(iup.Toggle("Toggle"), "toggle")

	radioA := wire(iup.Toggle("Radio A"), "radioA")
	radioB := wire(iup.Toggle("Radio B"), "radioB")
	radio := iup.Radio(iup.Vbox(radioA, radioB))

	dropdown := wire(iup.List(), "dropdown").SetAttributes(map[string]string{
		"DROPDOWN": "YES", "EDITBOX": "YES", "1": "one", "2": "two", "3": "three",
	})

	listbox := wire(iup.List(), "listbox").SetAttributes(map[string]string{
		"1": "alpha", "2": "beta", "3": "gamma", "VISIBLELINES": "3",
	})

	slider := wire(iup.Val("HORIZONTAL"), "slider").SetAttribute("EXPAND", "HORIZONTAL")

	tree := wire(iup.Tree(), "tree").SetAttribute("SIZE", "x60")

	table := wire(iup.Table(), "table").SetAttributes(map[string]string{
		"NUMCOL": "2", "NUMLIN": "4", "SIZE": "x50",
		"TITLE1": "A", "TITLE2": "B", "WIDTH1": "80", "WIDTH2": "80",
		"SELECTIONMODE": "SINGLE",
	})

	tabText1 := wire(iup.Text(), "tab1text").SetAttribute("EXPAND", "HORIZONTAL")
	tabText2 := wire(iup.Text(), "tab2text").SetAttribute("EXPAND", "HORIZONTAL")
	tabs := iup.Tabs(iup.Vbox(tabText1), iup.Vbox(tabText2))
	tabs.SetAttributes(map[string]string{"TABTITLE0": "Tab 1", "TABTITLE1": "Tab 2"})

	left := iup.Vbox(
		iup.Label("Tab through these; watch the log:"),
		text, spin, button, toggle, radio,
		dropdown, listbox, slider, tree, table, tabs,
		multi,
	)
	left.SetAttribute("GAP", "4")
	left.SetAttribute("MARGIN", "6x6")

	dlg := iup.Dialog(iup.Hbox(left, iup.Frame(log)))
	dlg.SetAttributes(map[string]string{
		"TITLE": "Tab + Focus test", "MARGIN": "6x6",
	})

	iup.Map(dlg)

	for i := 1; i <= 4; i++ {
		table.SetAttribute(fmt.Sprintf("%d:1", i), fmt.Sprintf("r%dA", i))
		table.SetAttribute(fmt.Sprintf("%d:2", i), fmt.Sprintf("r%dB", i))
	}

	tree.SetAttribute("TITLE0", "Root")
	tree.SetAttribute("ADDLEAF0", "Leaf 3")
	tree.SetAttribute("ADDLEAF0", "Leaf 2")
	tree.SetAttribute("ADDBRANCH0", "Branch")
	tree.SetAttribute("ADDLEAF1", "Branch leaf")

	iup.Show(dlg)
	iup.MainLoop()
}
