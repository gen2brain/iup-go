package main

import (
	"fmt"
	"strings"
	"time"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

type file struct {
	name string
	body string
}

type folder struct {
	name  string
	files []file
}

type tab struct {
	path  string
	text  iup.Ihandle
	dirty bool
}

var (
	project = []folder{
		{"cmd", []file{
			{"main.go", "package main\n\nimport \"fmt\"\n\nfunc main() {\n\tfmt.Println(greet(\"world\"))\n}\n"},
			{"greet.go", "package main\n\nfunc greet(who string) string {\n\treturn \"hello, \" + who\n}\n"},
		}},
		{"internal", []file{
			{"store.go", "package internal\n\ntype Store struct {\n\titems map[string]int\n}\n\nfunc New() *Store {\n\treturn &Store{items: map[string]int{}}\n}\n"},
			{"store_test.go", "package internal\n\nimport \"testing\"\n\nfunc TestNew(t *testing.T) {\n\tif New() == nil {\n\t\tt.Fatal(\"nil store\")\n\t}\n}\n"},
		}},
		{"docs", []file{
			{"README.md", "# Sample project\n\nA tree, a few editors and a build log.\n"},
			{"NOTES.md", "Ideas\n- keep the terminal pane collapsible\n- remember the split position\n"},
		}},
	}

	open []*tab
)

func main() {
	iup.Open()
	defer iup.Close()
	iup.SetGlobal("UTF8MODE", "YES")

	tree := iup.Tree().SetAttributes("ADDROOT=NO, EXPAND=YES").SetHandle("ws_tree")
	tree.SetCallback("EXECUTELEAF_CB", iup.ExecuteLeafFunc(openSelected))
	tree.SetCallback("RIGHTCLICK_CB", iup.RightClickFunc(treeMenu))

	tabs := iup.Tabs().SetAttributes("EXPAND=YES, SHOWCLOSE=YES, ALLOWREORDER=YES").SetHandle("ws_tabs")
	tabs.SetCallback("TABCLOSE_CB", iup.TabCloseFunc(closeTab))
	tabs.SetCallback("REORDER_CB", iup.ReorderFunc(reordered))
	tabs.SetCallback("TABCHANGE_CB", iup.TabChangeFunc(func(iup.Ihandle, iup.Ihandle, iup.Ihandle) int {
		updateStatus()
		return iup.DEFAULT
	}))

	term := iup.Terminal().SetAttributes(`EXPAND=YES, SCROLLBACKLINES=500, SCROLLONOUTPUT=YES,
		TERMNAME=xterm-256color, VISIBLELINES=8, VISIBLECOLUMNS=60`).SetHandle("ws_term")

	editors := iup.Split(tabs, term).SetAttributes("ORIENTATION=HORIZONTAL, VALUE=700, AUTOHIDE=YES").
		SetHandle("ws_vsplit")

	panes := iup.Split(iup.Vbox(iup.Label("Project").SetAttributes("PADDING=6x4, FONTSTYLE=Bold"), tree),
		editors).SetAttributes("ORIENTATION=VERTICAL, VALUE=260").SetHandle("ws_hsplit")

	toolbar := iup.Hbox(
		button("Build", build),
		button("Clear log", func() { iup.GetHandle("ws_term").SetAttribute("CLEARSCREEN", "YES") }),
		iup.Label("").SetAttribute("SEPARATOR", "VERTICAL"),
		logToggle(),
		iup.Label("").SetAttributes("EXPAND=HORIZONTAL"),
		button("Close all", closeAll),
	).SetAttributes("NMARGIN=8x6, NGAP=6, ALIGNMENT=ACENTER")

	status := iup.Label("").SetAttributes("EXPAND=HORIZONTAL, PADDING=8x4").SetHandle("ws_status")

	dlg := iup.Dialog(iup.Vbox(toolbar, panes, status).SetAttributes("NGAP=2"))
	dlg.SetAttribute("TITLE", "Workspace")

	openFile("docs", "README.md")

	iup.Show(dlg)
	fill(tree)

	write("\033[1;36mworkspace\033[0m ready, pick a file on the left\r\n")
	updateStatus()

	iup.MainLoop()
}

func fill(tree iup.Ihandle) {
	branch := -1
	for _, f := range project {
		if branch < 0 {
			tree.SetAttribute("ADDBRANCH-1", f.name)
		} else {
			tree.SetAttribute(fmt.Sprintf("INSERTBRANCH%d", branch), f.name)
		}
		branch = tree.GetInt("LASTADDNODE")

		leaf := -1
		for _, file := range f.files {
			if leaf < 0 {
				tree.SetAttribute(fmt.Sprintf("ADDLEAF%d", branch), file.name)
			} else {
				tree.SetAttribute(fmt.Sprintf("INSERTLEAF%d", leaf), file.name)
			}
			leaf = tree.GetInt("LASTADDNODE")
		}
	}
}

func openSelected(tree iup.Ihandle, id int) int {
	name := iup.GetAttributeId(tree, "TITLE", id)
	parent := iup.GetAttributeId(tree, "TITLE", iup.GetIntId(tree, "PARENT", id))
	openFile(parent, name)
	return iup.DEFAULT
}

func openFile(folderName, fileName string) {
	tabs := iup.GetHandle("ws_tabs")
	path := folderName + "/" + fileName

	for i, t := range open {
		if t.path == path {
			tabs.SetAttribute("VALUEPOS", i)
			updateStatus()
			return
		}
	}

	text := iup.Text().SetAttributes(`MULTILINE=YES, EXPAND=YES, FONT=Courier, WORDWRAP=NO,
		TABSIZE=4, VISIBLELINES=14, VISIBLECOLUMNS=48`).SetAttribute("VALUE", body(folderName, fileName))
	text.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(edited))
	text.SetCallback("CARET_CB", iup.CaretFunc(func(iup.Ihandle, int, int, int) int {
		updateStatus()
		return iup.DEFAULT
	}))

	pos := len(open)
	open = append(open, &tab{path: path, text: text})

	iup.Append(tabs, text)
	if iup.GetAttribute(tabs, "WID") != "" {
		iup.Map(text)
		iup.Refresh(iup.GetDialog(tabs))
	}

	iup.SetAttributeId(tabs, "TABTITLE", pos, fileName)
	tabs.SetAttribute("VALUEPOS", pos)

	updateStatus()
}

func treeMenu(tree iup.Ihandle, id int) int {
	tree.SetAttribute("VALUE", id)

	var menu iup.Ihandle
	if iup.GetAttributeId(tree, "KIND", id) == "LEAF" {
		name := iup.GetAttributeId(tree, "TITLE", id)
		folderName := iup.GetAttributeId(tree, "TITLE", iup.GetIntId(tree, "PARENT", id))
		menu = iup.Menu(
			item("Open "+name, func() { openFile(folderName, name) }),
			iup.Separator(),
			item("Copy path", func() { copyPath(folderName + "/" + name) }),
		)
	} else {
		menu = iup.Menu(
			item("Open all in "+iup.GetAttributeId(tree, "TITLE", id), func() { openFolder(tree, id) }),
			iup.Separator(),
			item("Collapse", func() { iup.SetAttributeId(tree, "STATE", id, "COLLAPSED") }),
			item("Expand", func() { iup.SetAttributeId(tree, "STATE", id, "EXPANDED") }),
		)
	}

	iup.Popup(menu, iup.MOUSEPOS, iup.MOUSEPOS)
	iup.Destroy(menu)
	return iup.DEFAULT
}

func openFolder(tree iup.Ihandle, id int) {
	folderName := iup.GetAttributeId(tree, "TITLE", id)
	for _, f := range project {
		if f.name != folderName {
			continue
		}
		for _, file := range f.files {
			openFile(folderName, file.name)
		}
	}
}

func copyPath(path string) {
	clip := iup.Clipboard()
	clip.SetAttribute("TEXT", path)
	iup.Destroy(clip)
	iup.GetHandle("ws_status").SetAttribute("TITLE", "Copied "+path)
}

func item(title string, action func()) iup.Ihandle {
	it := iup.MenuItem(title)
	it.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		action()
		return iup.DEFAULT
	}))
	return it
}

func body(folderName, fileName string) string {
	for _, f := range project {
		if f.name != folderName {
			continue
		}
		for _, file := range f.files {
			if file.name == fileName {
				return file.body
			}
		}
	}
	return ""
}

func edited(ih iup.Ihandle) int {
	for i, t := range open {
		if t.text == ih && !t.dirty {
			t.dirty = true
			iup.SetAttributeId(iup.GetHandle("ws_tabs"), "TABTITLE", i, "*"+name(t.path))
			updateStatus()
		}
	}
	return iup.DEFAULT
}

func closeTab(_ iup.Ihandle, pos int) int {
	if pos < 0 || pos >= len(open) {
		return iup.IGNORE
	}
	if open[pos].dirty {
		if iup.Alarm("Close", name(open[pos].path)+" has unsaved changes.", "Discard", "Cancel", "") != 1 {
			return iup.IGNORE
		}
	}

	open = append(open[:pos], open[pos+1:]...)
	updateStatus()

	return iup.CONTINUE
}

func reordered(_ iup.Ihandle, oldPos, newPos int) int {
	if oldPos < 0 || oldPos >= len(open) || newPos < 0 || newPos >= len(open) {
		return iup.IGNORE
	}

	t := open[oldPos]
	open = append(open[:oldPos], open[oldPos+1:]...)
	open = append(open[:newPos], append([]*tab{t}, open[newPos:]...)...)

	updateStatus()
	return iup.DEFAULT
}

func closeAll() {
	tabs := iup.GetHandle("ws_tabs")
	for len(open) > 0 {
		last := len(open) - 1
		text := open[last].text
		open = open[:last]
		iup.Destroy(text)
	}
	iup.Refresh(iup.GetDialog(tabs))
	updateStatus()
}

func build() {
	write("\r\n\033[1m$ go build ./...\033[0m\r\n")

	for _, f := range project {
		for _, file := range f.files {
			if strings.HasSuffix(file.name, ".go") {
				write(fmt.Sprintf("  compiling %s/%s\r\n", f.name, file.name))
			}
		}
	}

	if dirty := count(); dirty > 0 {
		write(fmt.Sprintf("\033[33mwarning: %d unsaved file(s)\033[0m\r\n", dirty))
	}
	write("\033[32mok\033[0m  " + time.Now().Format("15:04:05") + "\r\n")
}

func logToggle() iup.Ihandle {
	toggle := iup.Toggle("Build log").SetAttributes("VALUE=ON, CANFOCUS=NO")
	toggle.SetCallback("ACTION", iup.ToggleActionFunc(func(_ iup.Ihandle, state int) int {
		split := iup.GetHandle("ws_vsplit")
		if state == 1 {
			split.SetAttribute("VALUE", "700")
		} else {
			split.SetAttribute("VALUE", "1000")
		}
		return iup.DEFAULT
	}))
	return toggle
}

func write(s string) {
	iup.GetHandle("ws_term").SetAttribute("WRITE", s)
}

func count() int {
	n := 0
	for _, t := range open {
		if t.dirty {
			n++
		}
	}
	return n
}

func name(path string) string {
	if i := strings.LastIndexByte(path, '/'); i >= 0 {
		return path[i+1:]
	}
	return path
}

func updateStatus() {
	pos := iup.GetInt(iup.GetHandle("ws_tabs"), "VALUEPOS")
	if pos < 0 || pos >= len(open) {
		iup.GetHandle("ws_status").SetAttribute("TITLE", fmt.Sprintf("%d open", len(open)))
		return
	}

	t := open[pos]
	lin, col := 1, 1
	fmt.Sscanf(iup.GetAttribute(t.text, "CARET"), "%d,%d", &lin, &col)

	state := ""
	if t.dirty {
		state = "  modified"
	}
	iup.GetHandle("ws_status").SetAttribute("TITLE",
		fmt.Sprintf("%s  %d:%d  %d open%s", t.path, lin, col, len(open), state))
}

func button(title string, action func()) iup.Ihandle {
	b := iup.Button(title).SetAttributes("PADDING=6x3, CANFOCUS=NO")
	b.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		action()
		return iup.DEFAULT
	}))
	return b
}
