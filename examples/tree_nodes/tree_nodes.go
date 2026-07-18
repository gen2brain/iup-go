package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func tree() iup.Ihandle { return iup.GetHandle("tree") }

func setStatus(format string, a ...any) {
	iup.GetHandle("status").SetAttribute("TITLE", fmt.Sprintf(format, a...))
}

var (
	nextUID = uintptr(1000)
	uidName = map[uintptr]string{}
	srcUID  = uintptr(0)
)

// ensureUID gives a node a stable user id the first time it is seen, so the
// USERDATA <-> position mapping can be demonstrated across id shifts.
func ensureUID(id int) uintptr {
	t := tree()
	uid := iup.TreeGetUserId(t, id)
	if uid == 0 {
		nextUID++
		uid = nextUID
		iup.TreeSetUserId(t, id, uid)
		uidName[uid] = iup.GetAttributeId(t, "TITLE", id)
	}
	return uid
}

func build() {
	t := tree()
	t.SetAttribute("TITLE0", "Root")
	t.SetAttribute("ADDBRANCH0", "Vegetables")
	t.SetAttribute("ADDBRANCH0", "Fruits")
	t.SetAttribute("ADDLEAF1", "Apple")
	t.SetAttribute("ADDLEAF2", "Banana")
	t.SetAttribute("ADDLEAF4", "Carrot")
	t.SetAttribute("STATE1", "EXPANDED")
	t.SetAttribute("STATE4", "EXPANDED")

	for id := 0; id < iup.GetInt(t, "COUNT"); id++ {
		ensureUID(id)
	}
	updateInfo()
}

func focusID() int { return iup.GetInt(tree(), "VALUE") }

func updateInfo() int {
	t := tree()
	id := focusID()
	uid := ensureUID(id)
	info := fmt.Sprintf(
		"focus id=%d  %q\nKIND=%s  DEPTH=%s  PARENT=%s\nuserid=%d  ->  GetId(userid)=%d\nCOUNT=%d  CHILDCOUNT=%s  src=%d",
		id, iup.GetAttributeId(t, "TITLE", id),
		iup.GetAttributeId(t, "KIND", id), iup.GetAttributeId(t, "DEPTH", id),
		iup.GetAttributeId(t, "PARENT", id),
		uid, iup.TreeGetId(t, uid),
		iup.GetInt(t, "COUNT"), iup.GetAttributeId(t, "CHILDCOUNT", id), srcUID)
	iup.GetHandle("info").SetAttribute("VALUE", info)
	return iup.DEFAULT
}

func add(kind string) int {
	t := tree()
	id := focusID()
	iup.SetAttributeId(t, kind, id, "New "+kind)
	ensureUID(focusID())
	setStatus("%s at %d", kind, id)
	return updateInfo()
}

func setSrc() int {
	srcUID = ensureUID(focusID())
	setStatus("src = node %d (userid %d)", focusID(), srcUID)
	return updateInfo()
}

func moveCopy(name string) int {
	t := tree()
	if srcUID == 0 {
		setStatus("%s: set a source first", name)
		return iup.DEFAULT
	}
	src := iup.TreeGetId(t, srcUID)
	dst := focusID()
	if src < 0 || src == dst {
		setStatus("%s: invalid src/dst", name)
		return iup.DEFAULT
	}
	iup.SetAttributeId(t, name, src, fmt.Sprintf("%d", dst))
	setStatus("%s src %d -> dst %d", name, src, dst)
	return updateInfo()
}

func del(mode string) int {
	setStatus("DELNODE=%s", mode)
	iup.SetAttributeId(tree(), "DELNODE", focusID(), mode)
	return updateInfo()
}

func state(value string) int {
	tree().SetAttribute("EXPANDALL", value)
	setStatus("EXPANDALL=%s", value)
	return updateInfo()
}

func style(name, value string) int {
	iup.SetAttributeId(tree(), name, focusID(), value)
	setStatus("%s%d=%s", name, focusID(), value)
	return updateInfo()
}

func btn(label string, cb func() int) iup.Ihandle {
	return iup.Button(label).SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { return cb() }))
}

func frame(title string, children ...iup.Ihandle) iup.Ihandle {
	return iup.Frame(iup.Vbox(children...).SetAttributes(`NMARGIN=6x6, NGAP=4`)).SetAttribute("TITLE", title)
}

func main() {
	iup.Open()
	defer iup.Close()

	t := iup.Tree().SetAttributes(`MARKMODE=MULTIPLE, VISIBLELINES=13, VISIBLECOLUMNS=13, EXPAND=YES, ADDEXPANDED=YES`)
	t.SetHandle("tree")
	t.SetCallback("SELECTION_CB", iup.SelectionFunc(func(_ iup.Ihandle, id, status int) int { return updateInfo() }))
	t.SetCallback("NODEREMOVED_CB", iup.NodeRemovedFunc(func(_ iup.Ihandle, userId uintptr) int {
		setStatus("NODEREMOVED_CB: userid %d %q", userId, uidName[userId])
		delete(uidName, userId)
		return iup.DEFAULT
	}))

	addFrame := frame("Add",
		btn("Add leaf", func() int { return add("ADDLEAF") }),
		btn("Add branch", func() int { return add("ADDBRANCH") }),
		btn("Insert leaf", func() int { return add("INSERTLEAF") }),
	)
	moveFrame := frame("Move / Copy",
		btn("Set source", setSrc),
		btn("Move here", func() int { return moveCopy("MOVENODE") }),
		btn("Copy here", func() int { return moveCopy("COPYNODE") }),
	)
	delFrame := frame("Delete",
		btn("Node", func() int { return del("SELECTED") }),
		btn("Children", func() int { return del("CHILDREN") }),
		btn("Marked", func() int { return del("MARKED") }),
		btn("All", func() int { return del("ALL") }),
	)
	stateFrame := frame("State / Style",
		btn("Expand all", func() int { return state("YES") }),
		btn("Collapse all", func() int { return state("NO") }),
		btn("Color red", func() int { return style("COLOR", "255 0 0") }),
		btn("Bold title", func() int { return style("TITLEFONT", "Bold") }),
	)

	info := iup.Frame(
		iup.Text().SetAttributes(`MULTILINE=YES, READONLY=YES, VISIBLELINES=5, VISIBLECOLUMNS=30`).SetHandle("info"),
	).SetAttribute("TITLE", "Info")

	status := iup.Label("Focus a node, then act. Ctrl/Shift-click to mark for DELNODE=MARKED.").
		SetAttribute("EXPAND", "HORIZONTAL").SetHandle("status")

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Hbox(
				t,
				iup.Vbox(addFrame, moveFrame).SetAttributes(`NGAP=8`),
				iup.Vbox(delFrame, stateFrame).SetAttributes(`NGAP=8`),
				info,
			).SetAttributes(`NGAP=10`),
			status,
		).SetAttributes(`NMARGIN=10x10, NGAP=8`),
	).SetAttribute("TITLE", "Tree nodes")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	build()
	iup.MainLoop()
}
