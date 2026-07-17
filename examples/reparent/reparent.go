package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func setStatus(format string, a ...any) {
	iup.GetHandle("status").SetAttribute("TITLE", fmt.Sprintf(format, a...))
}

func counts() string {
	return fmt.Sprintf("A=%d  B=%d",
		iup.GetChildCount(iup.GetHandle("colA")),
		iup.GetChildCount(iup.GetHandle("colB")))
}

// hop moves an item to the opposite column with a single Reparent, then relays out.
func hop(item iup.Ihandle) int {
	from := iup.GetParent(item)
	to, name := iup.GetHandle("colB"), "Column B"
	if from == to {
		to, name = iup.GetHandle("colA"), "Column A"
	}
	iup.Reparent(item, to, 0)
	iup.Refresh(to)
	setStatus("moved %q to %s   (%s)", item.GetAttribute("TITLE"), name, counts())
	return iup.DEFAULT
}

func item(title string) iup.Ihandle {
	return iup.Button(title).SetAttribute("EXPAND", "HORIZONTAL").
		SetCallback("ACTION", iup.ActionFunc(hop))
}

func column(handle, title string, items ...iup.Ihandle) iup.Ihandle {
	box := iup.Vbox(items...).SetAttributes(`NMARGIN=6x6, NGAP=4`).SetHandle(handle)
	return iup.Frame(box).SetAttributes(fmt.Sprintf(`TITLE="%s", EXPAND=YES`, title))
}

// rotate does the manual equivalent of a move: detach the first child, append it, remap it.
func rotate(handle string) int {
	box := iup.GetHandle(handle)
	if iup.GetChildCount(box) < 2 {
		return iup.DEFAULT
	}
	first := iup.GetChild(box, 0)
	iup.Detach(first)
	iup.Append(box, first)
	iup.Map(first)
	iup.Refresh(box)
	setStatus("rotated %s (Detach+Append+Map)   (%s)", handle, counts())
	return iup.DEFAULT
}

// sendTop reparents the last child before the first, exercising the ref_child insert path.
func sendTop(handle string) int {
	box := iup.GetHandle(handle)
	n := iup.GetChildCount(box)
	if n < 2 {
		return iup.DEFAULT
	}
	iup.Reparent(iup.GetChild(box, n-1), box, iup.GetChild(box, 0))
	iup.Refresh(box)
	setStatus("sent last of %s to top   (%s)", handle, counts())
	return iup.DEFAULT
}

func focusPanel() iup.Ihandle {
	var fields []iup.Ihandle
	for i := 1; i <= 4; i++ {
		t := iup.Text().SetAttributes(fmt.Sprintf(`VISIBLECOLUMNS=8, VALUE="field %d"`, i)).
			SetHandle(fmt.Sprintf("f%d", i))
		fields = append(fields, t)
	}
	label := func(ih iup.Ihandle) string {
		if n := iup.GetName(ih); n != "" {
			return n
		}
		return "(other control)"
	}
	next := iup.Button("Focus next").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		cur := iup.GetFocus()
		if cur == 0 {
			cur = iup.GetHandle("f1")
		}
		setStatus("NextField -> %s", label(iup.NextField(cur)))
		return iup.DEFAULT
	}))
	prev := iup.Button("Focus previous").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		cur := iup.GetFocus()
		if cur == 0 {
			cur = iup.GetHandle("f1")
		}
		setStatus("PreviousField -> %s", label(iup.PreviousField(cur)))
		return iup.DEFAULT
	}))
	body := iup.Vbox(
		iup.Hbox(fields...).SetAttributes(`NGAP=6`),
		iup.Hbox(next, prev).SetAttributes(`NGAP=6`),
	).SetAttributes(`NMARGIN=6x6, NGAP=6`)
	return iup.Frame(body).SetAttribute("TITLE", "Tab order / focus")
}

func main() {
	iup.Open()
	defer iup.Close()

	colA := column("colA", "Column A",
		item("Item 1"), item("Item 2"), item("Item 3"))
	colB := column("colB", "Column B",
		item("Item 4"), item("Item 5"))

	ops := iup.Hbox(
		iup.Button("Rotate A").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { return rotate("colA") })),
		iup.Button("A: last to top").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { return sendTop("colA") })),
		iup.Button("Rotate B").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { return rotate("colB") })),
		iup.Button("B: last to top").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { return sendTop("colB") })),
	).SetAttributes(`NGAP=6`)

	status := iup.Label("Click an item to move it between columns.").
		SetAttribute("EXPAND", "HORIZONTAL").SetHandle("status")

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Hbox(colA, colB).SetAttributes(`NGAP=8`),
			ops,
			focusPanel(),
			status,
		).SetAttributes(`NMARGIN=10x10, NGAP=8`),
	).SetAttributes(`TITLE="Reparent", SIZE=HALFxHALF`)

	iup.Show(dlg)
	iup.MainLoop()
}
