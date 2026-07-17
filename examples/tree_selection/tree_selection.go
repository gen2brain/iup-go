package main

import (
	"fmt"
	"strings"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func tree() iup.Ihandle { return iup.GetHandle("tree") }

func setStatus(format string, a ...any) {
	iup.GetHandle("status").SetAttribute("TITLE", fmt.Sprintf(format, a...))
}

// build populates the tree; must run after the tree is mapped.
func build() {
	t := tree()
	t.SetAttribute("TITLE0", "Root")
	t.SetAttribute("ADDBRANCH0", "Vegetables")
	t.SetAttribute("ADDBRANCH0", "Fruits")
	t.SetAttribute("ADDLEAF1", "Apple")
	t.SetAttribute("ADDLEAF2", "Banana")
	t.SetAttribute("ADDLEAF3", "Cherry")
	t.SetAttribute("ADDLEAF5", "Carrot")
	t.SetAttribute("ADDLEAF6", "Potato")
	t.SetAttribute("ADDLEAF7", "Onion")
	t.SetAttribute("STATE1", "EXPANDED")
	t.SetAttribute("STATE5", "EXPANDED")
	updateInfo()
}

func attr(name string, id int) string {
	return tree().GetAttribute(fmt.Sprintf("%s%d", name, id))
}

func updateInfo() int {
	t := tree()
	id := iup.GetInt(t, "VALUE")
	marked := t.GetAttribute("MARKEDNODES")
	info := fmt.Sprintf(
		"focus id=%d  %q\nKIND=%s  DEPTH=%s  PARENT=%s\nCOUNT=%d  ROOTCOUNT=%s\nCHILDCOUNT=%s  TOTALCHILDCOUNT=%s\nmarked=%d  [%s]",
		id, attr("TITLE", id), attr("KIND", id), attr("DEPTH", id), attr("PARENT", id),
		iup.GetInt(t, "COUNT"), t.GetAttribute("ROOTCOUNT"),
		attr("CHILDCOUNT", id), attr("TOTALCHILDCOUNT", id),
		strings.Count(marked, "+"), marked)
	iup.GetHandle("info").SetAttribute("VALUE", info)
	return iup.DEFAULT
}

func mark(value string) int {
	tree().SetAttribute("MARK", value)
	setStatus("MARK=%s", value)
	return updateInfo()
}

func nav(value string) int {
	tree().SetAttribute("VALUE", value)
	setStatus("VALUE=%s -> focus %d", value, iup.GetInt(tree(), "VALUE"))
	return updateInfo()
}

func btn(label string, cb func() int) iup.Ihandle {
	return iup.Button(label).SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { return cb() }))
}

func main() {
	iup.Open()
	defer iup.Close()

	t := iup.Tree().SetAttributes(`MARKMODE=MULTIPLE, VISIBLELINES=11, VISIBLECOLUMNS=14, EXPAND=YES`)
	t.SetHandle("tree")
	t.SetCallback("SELECTION_CB", iup.SelectionFunc(func(_ iup.Ihandle, id, status int) int {
		setStatus("SELECTION_CB: node %d %s", id, map[int]string{1: "selected", 0: "unselected"}[status])
		return updateInfo()
	}))
	t.SetCallback("MULTISELECTION_CB", iup.MultiSelectionFunc(func(_ iup.Ihandle, ids []int, n int) int {
		setStatus("MULTISELECTION_CB: %d nodes %v", n, ids)
		return updateInfo()
	}))
	t.SetCallback("MULTIUNSELECTION_CB", iup.MultiUnselectionFunc(func(_ iup.Ihandle, ids []int, n int) int {
		setStatus("MULTIUNSELECTION_CB: %d nodes %v", n, ids)
		return updateInfo()
	}))

	marks := iup.Frame(iup.Vbox(
		btn("Mark all", func() int { return mark("MARKALL") }),
		btn("Clear all", func() int { return mark("CLEARALL") }),
		btn("Invert all", func() int { return mark("INVERTALL") }),
		btn("Mark 2-4", func() int { return mark("2-4") }),
	).SetAttributes(`NMARGIN=6x6, NGAP=4`)).SetAttribute("TITLE", "Marks")

	focus := iup.Frame(iup.Vbox(
		btn("Root", func() int { return nav("ROOT") }),
		btn("Last", func() int { return nav("LAST") }),
		btn("Next", func() int { return nav("NEXT") }),
		btn("Previous", func() int { return nav("PREVIOUS") }),
	).SetAttributes(`NMARGIN=6x6, NGAP=4`)).SetAttribute("TITLE", "Focus")

	info := iup.Frame(
		iup.Text().SetAttributes(`MULTILINE=YES, READONLY=YES, VISIBLELINES=5, VISIBLECOLUMNS=30`).SetHandle("info"),
	).SetAttribute("TITLE", "Info")

	status := iup.Label("Ctrl/Shift-click to multi-select, or use the buttons.").
		SetAttribute("EXPAND", "HORIZONTAL").SetHandle("status")

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Hbox(t, iup.Vbox(marks, focus).SetAttributes(`NGAP=8`), info).SetAttributes(`NGAP=10`),
			status,
		).SetAttributes(`NMARGIN=10x10, NGAP=8`),
	).SetAttribute("TITLE", "Tree selection")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	build()
	iup.MainLoop()
}
