package main

import (
	"fmt"
	"strings"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

var (
	form     iup.Ihandle
	selected iup.Ihandle
	preview  iup.Ihandle

	nodes   []iup.Ihandle
	labels  = map[iup.Ihandle]string{}
	counts  = map[string]int{}
	syncing bool

	kinds = []string{"Vbox", "Hbox", "Frame", "Button", "Label", "Text", "Toggle", "List", "Val", "ProgressBar"}
)

func main() {
	iup.Open()
	defer iup.Close()
	iup.SetGlobal("UTF8MODE", "YES")

	palette := iup.List().SetAttributes("VISIBLELINES=8, VISIBLECOLUMNS=10, EXPAND=HORIZONTAL").
		SetHandle("palette")
	if phone() {
		palette.SetAttribute("VISIBLELINES", 3)
	}
	for i, kind := range kinds {
		iup.SetAttributeId(palette, "", i+1, kind)
	}
	palette.SetAttribute("VALUE", "4")
	palette.SetCallback("DBLCLICK_CB", iup.DblclickFunc(func(iup.Ihandle, int, string) int {
		add()
		return iup.DEFAULT
	}))

	outline := iup.Tree().SetAttributes("ADDROOT=NO, SHOWDRAGDROP=YES, VISIBLELINES=10, VISIBLECOLUMNS=14").
		SetHandle("outline")
	if phone() {
		outline.SetAttribute("VISIBLELINES", 3)
	}
	outline.SetCallback("SELECTION_CB", iup.SelectionFunc(picked))
	outline.SetCallback("DRAGDROP_CB", iup.DragDropFunc(dropped))

	props := iup.Vbox().SetAttributes("NMARGIN=6x6, NGAP=6, EXPAND=YES").SetHandle("props")

	form = box("Vbox")
	iup.Append(form, label("Label", "Name"))
	iup.Append(form, label("Text", ""))
	row := box("Hbox")
	iup.Append(row, label("Button", "OK"))
	iup.Append(row, label("Button", "Cancel"))
	iup.Append(form, row)

	holder := iup.Vbox(form).SetAttributes("NMARGIN=10x10, EXPAND=YES").SetHandle("holder")

	status := iup.Label("").SetAttributes("EXPAND=HORIZONTAL, PADDING=8x4").SetHandle("status")

	left := iup.Vbox(
		heading("Palette"), palette,
		iup.Hbox(button("Add", add), button("Delete", remove)).SetAttributes("NGAP=6"),
		heading("Outline"), outline,
	).SetAttributes("NGAP=6")

	right := iup.Vbox(heading("Properties"), props, button("Preview", show)).
		SetAttributes("NGAP=6")

	frame := iup.Frame(holder).SetAttribute("TITLE", "Form")

	var content iup.Ihandle
	if phone() {
		content = iup.Vbox(left, frame, right)
	} else {
		content = iup.Hbox(left, frame, right)
	}
	content.SetAttributes("NGAP=8, NMARGIN=8x8")

	dlg := iup.Dialog(iup.Vbox(content, status)).SetHandle("designer")
	dlg.SetAttribute("TITLE", "Designer")

	iup.Show(dlg)

	rebuild()
	choose(form)
	relayout()
	setStatus("Double click a palette entry to add it, drag in the outline to reparent")

	iup.MainLoop()
}

func phone() bool {
	switch iup.GetGlobal("SYSTEM") {
	case "Android", "iOS":
		return true
	}
	return false
}

func create(kind string) iup.Ihandle {
	switch kind {
	case "Vbox", "Hbox", "Frame":
		return box(kind)
	}
	return label(kind, kind)
}

func box(kind string) iup.Ihandle {
	var ih iup.Ihandle
	switch kind {
	case "Hbox":
		ih = iup.Hbox().SetAttributes("NGAP=6, ALIGNMENT=ACENTER")
	case "Frame":
		ih = iup.Frame(iup.Vbox().SetAttributes("NGAP=6, NMARGIN=6x6"))
	default:
		ih = iup.Vbox().SetAttributes("NGAP=6")
	}
	return named(ih, kind)
}

func label(kind, title string) iup.Ihandle {
	var ih iup.Ihandle
	switch kind {
	case "Label":
		ih = iup.Label(title)
	case "Text":
		ih = iup.Text().SetAttributes("VISIBLECOLUMNS=12, EXPAND=HORIZONTAL")
	case "Toggle":
		ih = iup.Toggle(title)
	case "List":
		ih = iup.List().SetAttributes("DROPDOWN=YES, 1=One, 2=Two, 3=Three, VALUE=1")
	case "Val":
		ih = iup.Val("HORIZONTAL").SetAttributes("EXPAND=HORIZONTAL")
	case "ProgressBar":
		ih = iup.ProgressBar().SetAttributes("VALUE=0.4, EXPAND=HORIZONTAL")
	default:
		ih = iup.Button(title)
	}
	return named(ih, kind)
}

func named(ih iup.Ihandle, kind string) iup.Ihandle {
	counts[kind]++
	labels[ih] = fmt.Sprintf("%s %d", kind, counts[kind])
	return ih
}

func add() {
	kind := kinds[iup.GetHandle("palette").GetInt("VALUE")-1]
	parent := container(selected)
	child := create(kind)

	iup.Append(parent, child)
	iup.Map(child)

	rebuild()
	choose(child)
	relayout()
	setStatus(labels[child] + " added to " + labels[owner(child)])
}

func remove() {
	if selected == form {
		setStatus("The form itself cannot be removed")
		return
	}

	gone := labels[selected]
	parent := owner(selected)
	iup.Destroy(selected)
	delete(labels, selected)

	rebuild()
	choose(parent)
	relayout()
	setStatus(gone + " removed")
}

func dropped(ih iup.Ihandle, drag, drop, shift, control int) int {
	if drag >= len(nodes) || drop < 0 || drop >= len(nodes) {
		return iup.DEFAULT
	}

	child, target := nodes[drag], nodes[drop]
	if child == form || target == child || inside(target, child) {
		setStatus("A control cannot be moved inside itself")
		return iup.DEFAULT
	}

	var ref iup.Ihandle
	parent := target
	if holds(target) {
		parent = container(target)
	} else {
		parent, ref = container(target), target
	}
	if iup.Reparent(child, parent, ref) != 0 {
		setStatus("Reparent refused " + labels[child])
		return iup.DEFAULT
	}

	rebuild()
	choose(child)
	relayout()
	setStatus(labels[child] + " moved into " + labels[owner(child)])
	return iup.DEFAULT
}

func picked(ih iup.Ihandle, id, state int) int {
	if state != 1 || syncing || id >= len(nodes) || nodes[id] == selected {
		return iup.DEFAULT
	}

	selected = nodes[id]
	properties()
	return iup.DEFAULT
}

func show() {
	if preview == 0 {
		slot := iup.Vbox().SetAttributes("NMARGIN=10x10, EXPAND=YES").SetHandle("slot")
		preview = iup.Dialog(slot).SetAttributes("TITLE=Preview, PARENTDIALOG=designer")
		preview.SetCallback("CLOSE_CB", iup.CloseFunc(func(iup.Ihandle) int {
			iup.Reparent(form, iup.GetHandle("holder"), 0)
			relayout()
			setStatus("Back in the designer")
			return iup.DEFAULT
		}))
	}

	iup.Map(preview)
	iup.Reparent(form, iup.GetHandle("slot"), 0)
	relayout()

	preview.SetAttribute("SIZE", "")
	iup.Refresh(preview)
	iup.ShowXY(preview, iup.CENTERPARENT, iup.CENTERPARENT)
	setStatus("The form is live in the preview, close it to bring it back")
}

func container(ih iup.Ihandle) iup.Ihandle {
	if iup.GetClassName(ih) == "frame" {
		return iup.GetChild(ih, 0)
	}
	if holds(ih) {
		return ih
	}
	return owner(ih)
}

func owner(ih iup.Ihandle) iup.Ihandle {
	parent := iup.GetParent(ih)
	if grand := iup.GetParent(parent); grand != 0 && iup.GetClassName(grand) == "frame" {
		return grand
	}
	return parent
}

func holds(ih iup.Ihandle) bool {
	switch iup.GetClassName(ih) {
	case "vbox", "hbox":
		return true
	case "frame":
		return true
	}
	return false
}

func inside(ih, ancestor iup.Ihandle) bool {
	for p := iup.GetParent(ih); p != 0; p = iup.GetParent(p) {
		if p == ancestor {
			return true
		}
	}
	return false
}

func rebuild() {
	tree := iup.GetHandle("outline")
	syncing = true
	tree.SetAttribute("DELNODE0", "ALL")
	nodes = nodes[:0]
	fill(tree, form, -1, -1)
	syncing = false
}

func fill(tree, ih iup.Ihandle, parent, prev int) int {
	attr := "ADDLEAF"
	ref := parent
	if prev >= 0 {
		attr = "INSERTLEAF"
		ref = prev
	}
	if holds(ih) {
		attr = strings.Replace(attr, "LEAF", "BRANCH", 1)
	}

	iup.SetAttributeId(tree, attr, ref, labels[ih])
	id := tree.GetInt("LASTADDNODE")
	for len(nodes) <= id {
		nodes = append(nodes, 0)
	}
	nodes[id] = ih

	if holds(ih) {
		child := -1
		inner := ih
		if iup.GetClassName(ih) == "frame" {
			inner = iup.GetChild(ih, 0)
		}
		for i := 0; i < iup.GetChildCount(inner); i++ {
			child = fill(tree, iup.GetChild(inner, i), id, child)
		}
	}

	return id
}

func choose(ih iup.Ihandle) {
	selected = ih
	syncing = true
	for id, node := range nodes {
		if node == ih {
			iup.GetHandle("outline").SetAttribute("VALUE", id)
			break
		}
	}
	syncing = false
	properties()
}

func properties() {
	panel := iup.GetHandle("props")
	if old := iup.GetChild(panel, 0); old != 0 {
		iup.Destroy(old)
	}

	rows := iup.GridBox().SetAttributes(`ORIENTATION=HORIZONTAL, NUMDIV=2, SIZECOL=-1, SIZELIN=-1,
		ALIGNMENTLIN=ACENTER, NGAPLIN=6, NGAPCOL=8`)

	iup.Append(rows, iup.Label("Element").SetAttribute("ALIGNMENT", "ARIGHT"))
	iup.Append(rows, iup.Label(labels[selected]).SetAttribute("FONTSTYLE", "Bold"))

	if title, ok := titled(); ok {
		iup.Append(rows, iup.Label("Title").SetAttribute("ALIGNMENT", "ARIGHT"))
		iup.Append(rows, field("TITLE", title))
	}

	iup.Append(rows, iup.Label("Expand").SetAttribute("ALIGNMENT", "ARIGHT"))
	iup.Append(rows, choice("EXPAND", []string{"NO", "HORIZONTAL", "VERTICAL", "YES"}))

	iup.Append(rows, iup.Label("Active").SetAttribute("ALIGNMENT", "ARIGHT"))
	iup.Append(rows, switchOf("ACTIVE"))

	if holds(selected) && iup.GetClassName(selected) != "frame" {
		iup.Append(rows, iup.Label("Gap").SetAttribute("ALIGNMENT", "ARIGHT"))
		iup.Append(rows, field("NGAP", selected.GetAttribute("NGAP")))
		iup.Append(rows, iup.Label("Margin").SetAttribute("ALIGNMENT", "ARIGHT"))
		iup.Append(rows, field("NMARGIN", selected.GetAttribute("NMARGIN")))
	}

	iup.Append(panel, rows)
	iup.Map(rows)
	iup.Refresh(iup.GetHandle("designer"))
}

func titled() (string, bool) {
	switch iup.GetClassName(selected) {
	case "button", "label", "toggle", "frame":
		return selected.GetAttribute("TITLE"), true
	}
	return "", false
}

func field(name, value string) iup.Ihandle {
	target := selected
	text := iup.Text().SetAttributes("VISIBLECOLUMNS=10, EXPAND=HORIZONTAL")
	text.SetAttribute("VALUE", value)
	text.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		apply(target, name, ih.GetAttribute("VALUE"))
		return iup.DEFAULT
	}))
	return text
}

func choice(name string, values []string) iup.Ihandle {
	target := selected
	list := iup.List().SetAttributes("DROPDOWN=YES, VISIBLEITEMS=5")
	current := target.GetAttribute(name)
	for i, v := range values {
		iup.SetAttributeId(list, "", i+1, v)
		if v == current {
			list.SetAttribute("VALUE", i+1)
		}
	}
	list.SetCallback("ACTION", iup.ListActionFunc(func(ih iup.Ihandle, text string, item, state int) int {
		if state == 1 {
			apply(target, name, text)
		}
		return iup.DEFAULT
	}))
	return list
}

func switchOf(name string) iup.Ihandle {
	target := selected
	toggle := iup.Toggle("")
	if target.GetInt(name) != 0 {
		toggle.SetAttribute("VALUE", "ON")
	} else {
		toggle.SetAttribute("VALUE", "OFF")
	}
	toggle.SetCallback("ACTION", iup.ToggleActionFunc(func(ih iup.Ihandle, state int) int {
		if state == 1 {
			apply(target, name, "YES")
		} else {
			apply(target, name, "NO")
		}
		return iup.DEFAULT
	}))
	return toggle
}

func apply(target iup.Ihandle, name, value string) {
	target.SetAttribute(name, value)
	iup.Refresh(iup.GetHandle("designer"))
	setStatus(labels[target] + " " + name + " = " + value)
}

func relayout() {
	dlg := iup.GetHandle("designer")
	dlg.SetAttribute("SIZE", "")
	iup.Refresh(dlg)
}

func heading(title string) iup.Ihandle {
	return iup.Label(title).SetAttributes("FONTSTYLE=Bold, PADDING=2x2")
}

func button(title string, action func()) iup.Ihandle {
	return iup.Button(title).SetAttributes("PADDING=10x4").
		SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
			action()
			return iup.DEFAULT
		}))
}

func setStatus(s string) {
	iup.GetHandle("status").SetAttribute("TITLE", s)
}
