package main

import (
	"fmt"
	"path/filepath"
	"strings"
	"unsafe"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

const dragType = "TEXT"

var (
	lines     = "VISIBLELINES=6"
	boardSize = "RASTERSIZE=220x120"
)

type chip struct {
	text string
	x, y int
}

var (
	parts   = []string{"Bolt", "Nut", "Washer", "Spring"}
	nodes   = []string{"Gearbox", "Axle"}
	rows    = []string{"Piston", "Valve"}
	chips   []chip
	carried string
)

func main() {
	iup.Open()
	iup.SetGlobal("UTF8MODE", "YES")
	defer iup.Close()

	if phone() {
		lines = "VISIBLELINES=3"
		boardSize = "RASTERSIZE=220x90"
	}

	list := partList()
	tree := assemblyTree()
	table := inventory()
	canvas := board()

	hint := "Drag an entry from one control onto another. Hold Ctrl to copy."
	if phone() {
		hint = "Long press an entry, then drag it."
	}
	status := iup.Label(hint).SetAttributes("EXPAND=HORIZONTAL, PADDING=6x4").SetHandle("status")

	frames := []iup.Ihandle{
		iup.Frame(list).SetAttribute("TITLE", "List"),
		iup.Frame(tree).SetAttribute("TITLE", "Tree"),
		iup.Frame(table).SetAttribute("TITLE", "Table"),
		iup.Frame(canvas).SetAttributes("TITLE=Canvas, EXPAND=YES"),
	}

	var body iup.Ihandle
	if phone() {
		body = iup.Vbox(frames...).SetAttributes("NGAP=6, NMARGIN=6x6")
	} else {
		body = iup.Vbox(
			iup.Hbox(frames[0], frames[1]).SetAttributes("NGAP=6, NMARGIN=6x6"),
			iup.Hbox(frames[2], frames[3]).SetAttributes("NGAP=6, NMARGIN=6x6"),
		)
	}

	dlg := iup.Dialog(iup.Vbox(body, status).SetAttribute("NGAP", "4"))

	dlg.SetAttribute("TITLE", "Drag and drop between controls")

	iup.Show(dlg)
	iup.MainLoop()
}

func partList() iup.Ihandle {
	list := iup.List().SetAttributes(lines + ", VISIBLECOLUMNS=12, EXPAND=YES").SetHandle("list")
	fillList(list)

	source(list, "list", func(int, int) string {
		return iup.GetAttributeId(list, "", list.GetInt("VALUE"))
	}, func(text string) {
		parts = remove(parts, text)
		fillList(list)
	})

	target(list, "list", func(text string, _, _ int) {
		parts = append(parts, text)
		fillList(list)
	})

	return list
}

func fillList(list iup.Ihandle) {
	list.SetAttribute("REMOVEITEM", "ALL")
	for i, p := range parts {
		iup.SetAttributeId(list, "", i+1, p)
	}
	if len(parts) > 0 {
		list.SetAttribute("VALUE", "1")
	}
}

func assemblyTree() iup.Ihandle {
	tree := iup.Tree().SetAttributes("ADDROOT=NO, " + lines + ", VISIBLECOLUMNS=12, EXPAND=YES").SetHandle("tree")
	tree.SetCallback("MAP_CB", iup.MapFunc(func(ih iup.Ihandle) int {
		fillTree(ih)
		return iup.DEFAULT
	}))

	source(tree, "tree", func(int, int) string {
		return iup.GetAttributeId(tree, "TITLE", tree.GetInt("VALUE"))
	}, func(text string) {
		nodes = remove(nodes, text)
		fillTree(tree)
	})

	target(tree, "tree", func(text string, _, _ int) {
		nodes = append(nodes, text)
		fillTree(tree)
	})

	return tree
}

func fillTree(tree iup.Ihandle) {
	tree.SetAttribute("DELNODE0", "ALL")
	for i, n := range nodes {
		iup.SetAttributeId(tree, "ADDLEAF", i-1, n)
	}
}

func inventory() iup.Ihandle {
	table := iup.Table().SetAttributes("NUMCOL=1, " + lines + ", EXPAND=YES").SetHandle("table")
	iup.SetAttributeId(table, "TITLE", 1, "Part")
	fillTable(table)

	source(table, "table", func(int, int) string {
		lin, _, _ := strings.Cut(table.GetAttribute("FOCUSCELL"), ":")
		return iup.GetAttributeId2(table, "", atoi(lin), 1)
	}, func(text string) {
		rows = remove(rows, text)
		fillTable(table)
	})

	target(table, "table", func(text string, _, _ int) {
		rows = append(rows, text)
		fillTable(table)
	})

	return table
}

func fillTable(table iup.Ihandle) {
	table.SetAttribute("NUMLIN", fmt.Sprint(len(rows)))
	for i, r := range rows {
		iup.SetAttributeId2(table, "", i+1, 1, r)
	}
}

func board() iup.Ihandle {
	canvas := iup.Canvas().SetAttributes(boardSize + ", EXPAND=YES")
	canvas.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		iup.DrawBegin(ih)
		w, h := iup.DrawGetSize(ih)
		ih.SetAttributes("DRAWSTYLE=FILL, DRAWCOLOR=\"250 250 250\"")
		iup.DrawRectangle(ih, 0, 0, w, h)
		for _, c := range chips {
			ih.SetAttributes("DRAWSTYLE=FILL, DRAWCOLOR=\"210 225 245\"")
			iup.DrawRectangle(ih, c.x-34, c.y-10, c.x+34, c.y+10)
			ih.SetAttribute("DRAWCOLOR", "20 20 20")
			iup.DrawText(ih, c.text, c.x-30, c.y-7, 0, 0)
		}
		iup.DrawEnd(ih)
		return iup.DEFAULT
	}))

	source(canvas, "canvas", func(x, y int) string {
		if c := chipAt(x, y); c >= 0 {
			return chips[c].text
		}
		return ""
	}, func(text string) {
		for i, c := range chips {
			if c.text == text {
				chips = append(chips[:i], chips[i+1:]...)
				break
			}
		}
		iup.Update(canvas)
	})

	target(canvas, "canvas", func(text string, x, y int) {
		chips = append(chips, chip{text, x, y})
		iup.Update(canvas)
	})

	return canvas
}

func chipAt(x, y int) int {
	for i, c := range chips {
		if x >= c.x-34 && x <= c.x+34 && y >= c.y-10 && y <= c.y+10 {
			return i
		}
	}
	return -1
}

func source(ih iup.Ihandle, name string, pick func(x, y int) string, take func(text string)) {
	ih.SetAttributes("DRAGSOURCE=YES, DRAGSOURCEMOVE=YES, DRAGTYPES=" + dragType)

	ih.SetCallback("DRAGBEGIN_CB", iup.DragBeginFunc(func(_ iup.Ihandle, x, y int) int {
		carried = pick(x, y)
		if carried == "" {
			return iup.IGNORE
		}
		setStatus(fmt.Sprintf("%s: dragging %q", name, carried))
		return iup.DEFAULT
	}))

	ih.SetCallback("DRAGDATASIZE_CB", iup.DragDataSizeFunc(func(iup.Ihandle, string) int {
		return len(carried) + 1
	}))

	ih.SetCallback("DRAGDATA_CB", iup.DragDataFunc(func(_ iup.Ihandle, _ string, data unsafe.Pointer, size int) int {
		buf := unsafe.Slice((*byte)(data), size)
		if n := copy(buf, carried); n < size {
			buf[n] = 0
		}
		return iup.DEFAULT
	}))

	ih.SetCallback("DRAGEND_CB", iup.DragEndFunc(func(_ iup.Ihandle, action int) int {
		if action == 1 {
			take(carried)
		}
		setStatus(fmt.Sprintf("%s: %s %q", name, actionName(action), carried))
		return iup.DEFAULT
	}))
}

func target(ih iup.Ihandle, name string, add func(text string, x, y int)) {
	ih.SetAttributes("DROPTARGET=YES, DROPFILESTARGET=YES, DROPTYPES=" + dragType)

	ih.SetCallback("DROPDATA_CB", iup.DropDataFunc(func(_ iup.Ihandle, _ string, data unsafe.Pointer, size, x, y int) int {
		value := decode(data, size)
		if value == "" {
			return iup.DEFAULT
		}
		add(value, x, y)
		setStatus(fmt.Sprintf("%s: dropped %q at %d,%d", name, value, x, y))
		return iup.DEFAULT
	}))

	ih.SetCallback("DROPMOTION_CB", iup.DropMotionFunc(func(iup.Ihandle, int, int, string) int {
		return iup.DEFAULT
	}))

	ih.SetCallback("DROPFILES_CB", iup.DropFilesFunc(func(_ iup.Ihandle, file string, _, x, y int) int {
		add(filepath.Base(file), x, y)
		setStatus(fmt.Sprintf("%s: dropped file %q", name, filepath.Base(file)))
		return iup.DEFAULT
	}))
}

func decode(data unsafe.Pointer, size int) string {
	s := string(unsafe.Slice((*byte)(data), size))
	if i := strings.IndexByte(s, 0); i >= 0 {
		s = s[:i]
	}
	return strings.TrimSpace(s)
}

func remove(items []string, text string) []string {
	for i, s := range items {
		if s == text {
			return append(items[:i], items[i+1:]...)
		}
	}
	return items
}

func actionName(action int) string {
	switch action {
	case 1:
		return "moved"
	case 0:
		return "copied"
	}
	return "cancelled"
}

func atoi(s string) int {
	n := 0
	for _, r := range s {
		if r < '0' || r > '9' {
			return n
		}
		n = n*10 + int(r-'0')
	}
	return n
}

func setStatus(s string) { iup.GetHandle("status").SetAttribute("TITLE", s) }

func phone() bool {
	switch iup.GetGlobal("SYSTEM") {
	case "Android", "iOS":
		return true
	}
	return false
}
