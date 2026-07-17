package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func setStatus(format string, a ...any) {
	iup.GetHandle("status").SetAttribute("TITLE", fmt.Sprintf(format, a...))
}

func stateName(state int) string {
	switch state {
	case 1:
		return "ON"
	case -1:
		return "NOTDEF"
	default:
		return "OFF"
	}
}

func tree() iup.Ihandle { return iup.GetHandle("tree") }

// build populates the tree; must run after the tree is mapped.
func build() {
	t := tree()
	t.SetAttribute("TITLE", "Options")
	t.SetAttribute("ADDLEAF", "Verbose logging")
	t.SetAttribute("ADDLEAF1", "Auto-update")
	t.SetAttribute("ADDLEAF2", "Telemetry")
	t.SetAttribute("ADDBRANCH3", "Advanced")
	t.SetAttribute("ADDLEAF4", "Experimental")
	t.SetAttribute("ADDLEAF5", "Beta features")
	t.SetAttribute("STATE4", "EXPANDED")

	t.SetAttribute("TOGGLEVALUE1", "ON")
	t.SetAttribute("TOGGLEVALUE2", "NOTDEF")
	updateCount()
}

func updateCount() {
	t := tree()
	n, on := iup.GetInt(t, "COUNT"), 0
	for i := 0; i < n; i++ {
		if t.GetAttribute(fmt.Sprintf("TOGGLEVALUE%d", i)) == "ON" {
			on++
		}
	}
	iup.GetHandle("count").SetAttribute("TITLE", fmt.Sprintf("%d of %d ON", on, n))
}

func setAll(value string) int {
	t := tree()
	for i, n := 0, iup.GetInt(t, "COUNT"); i < n; i++ {
		t.SetAttribute(fmt.Sprintf("TOGGLEVALUE%d", i), value)
	}
	updateCount()
	setStatus("set all toggles to %s", value)
	return iup.DEFAULT
}

// cycleFocus advances the focus node OFF -> ON -> NOTDEF -> OFF.
func cycleFocus() int {
	t := tree()
	id := iup.GetInt(t, "VALUE")
	attr := fmt.Sprintf("TOGGLEVALUE%d", id)
	next := map[string]string{"OFF": "ON", "ON": "NOTDEF", "NOTDEF": "OFF"}[t.GetAttribute(attr)]
	if next == "" {
		next = "ON"
	}
	t.SetAttribute(attr, next)
	updateCount()
	setStatus("node %d (%s) -> %s", id, t.GetAttribute(fmt.Sprintf("TITLE%d", id)), next)
	return iup.DEFAULT
}

func hideFocusToggle() int {
	t := tree()
	id := iup.GetInt(t, "VALUE")
	attr := fmt.Sprintf("TOGGLEVISIBLE%d", id)
	if t.GetAttribute(attr) == "NO" {
		t.SetAttribute(attr, "YES")
		setStatus("node %d toggle shown", id)
	} else {
		t.SetAttribute(attr, "NO")
		setStatus("node %d toggle hidden", id)
	}
	return iup.DEFAULT
}

func main() {
	iup.Open()
	defer iup.Close()

	t := iup.Tree()
	t.SetAttributes(`SHOWTOGGLE=3STATE, VISIBLELINES=9, VISIBLECOLUMNS=18`)
	t.SetHandle("tree")
	t.SetCallback("TOGGLEVALUE_CB", iup.ToggleValueFunc(func(_ iup.Ihandle, id, state int) int {
		updateCount()
		setStatus("TOGGLEVALUE_CB: node %d -> %s", id, stateName(state))
		return iup.DEFAULT
	}))

	markWhen := iup.Toggle("MARKWHENTOGGLE").SetCallback("ACTION", iup.ToggleActionFunc(func(_ iup.Ihandle, state int) int {
		on := "NO"
		if state == 1 {
			on = "YES"
		}
		tree().SetAttribute("MARKWHENTOGGLE", on)
		setStatus("MARKWHENTOGGLE = %s", on)
		return iup.DEFAULT
	}))

	buttons := iup.Vbox(
		iup.Button("All ON").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { return setAll("ON") })),
		iup.Button("All OFF").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { return setAll("OFF") })),
		iup.Button("Cycle focus (3-state)").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { return cycleFocus() })),
		iup.Button("Hide/show focus toggle").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { return hideFocusToggle() })),
		markWhen,
		iup.Label("0 of 0 ON").SetHandle("count"),
	).SetAttributes(`NGAP=6`)

	status := iup.Label("Click toggles, or use the buttons. Focus a node first for the focus actions.").
		SetAttribute("EXPAND", "HORIZONTAL").SetHandle("status")

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Hbox(t, buttons).SetAttributes(`NGAP=10`),
			status,
		).SetAttributes(`NMARGIN=10x10, NGAP=8`),
	).SetAttribute("TITLE", "Tree toggles")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	build()
	iup.MainLoop()
}
