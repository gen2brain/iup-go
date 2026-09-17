package main

import (
	"fmt"
	"os"
	"path/filepath"
	"sort"
	"strings"
	"time"
	"unsafe"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

const dragType = "org.iup-go.paths"

type entry struct {
	name  string
	dir   bool
	size  int64
	mtime time.Time
}

var (
	root          string
	current       string
	shown         []entry
	sortColumn    = 1
	sortAscending = true
	branches      = map[int]string{}
	dragged       []string
)

func main() {
	iup.Open()
	defer iup.Close()
	iup.SetGlobal("UTF8MODE", "YES")

	root = sandbox()
	defer os.RemoveAll(root)
	current = root

	makeIcons()

	tree := iup.Tree().SetAttributes(`EXPAND=YES, ADDROOT=NO, VISIBLECOLUMNS=16,
		DROPTARGET=YES, DROPTYPES=` + dragType).SetHandle("fm_tree")
	tree.SetCallback("SELECTION_CB", iup.SelectionFunc(branchSelected))
	tree.SetCallback("DROPDATA_CB", iup.DropDataFunc(dropped))

	table := iup.Table().SetAttributes(`EXPAND=YES, NUMCOL=3, SORTABLE=YES, USERRESIZE=YES,
		STRETCHLAST=YES, ALTERNATECOLOR=YES, SHOWIMAGE=YES, FOCUSRECT=NO,
		SELECTIONMODE=MULTIPLE, DRAGSOURCE=YES, DRAGTYPES=` + dragType).SetHandle("fm_table")
	table.SetAttributes(map[string]string{
		"TITLE1": "Name", "TITLE2": "Size", "TITLE3": "Modified",
		"ALIGNMENT2": "ARIGHT",
	})
	table.SetCallback("CLICK_CB", iup.ClickFunc(clicked))
	table.SetCallback("SORT_CB", iup.TableSortFunc(sortRequested))
	table.SetCallback("DRAGBEGIN_CB", iup.DragBeginFunc(dragBegin))
	table.SetCallback("DRAGDATASIZE_CB", iup.DragDataSizeFunc(func(iup.Ihandle, string) int {
		return len(strings.Join(dragged, "\n")) + 1
	}))
	table.SetCallback("DRAGDATA_CB", iup.DragDataFunc(dragData))

	path := iup.Label("").SetAttributes("EXPAND=HORIZONTAL").SetHandle("fm_path")

	dlg := iup.Dialog(iup.Vbox(
		toolbar(),
		iup.Split(tree, iup.Vbox(path, table).SetAttributes("NMARGIN=6x6, NGAP=4")).
			SetAttributes("ORIENTATION=VERTICAL, VALUE=260, SHOWGRIP=YES"),
		iup.Hbox(
			iup.Label("").SetAttributes("EXPAND=HORIZONTAL").SetHandle("fm_status"),
			iup.Label("Drag a row onto a folder, or right click for Move to"),
		).SetAttributes("NMARGIN=8x4, NGAP=8"),
	)).SetHandle("fm_dlg")
	dlg.SetAttributes(map[string]string{"TITLE": "File manager", "MENU": "fm_menu"})

	buildMenu()
	iup.Show(dlg)
	fillTree()
	fillTable()
	iup.MainLoop()
}

func toolbar() iup.Ihandle {
	return iup.Hbox(
		button("Up", goUp),
		button("New folder", newFolder),
		button("Rename", renameSelected),
		button("Delete", deleteSelected),
		button("Refresh", refresh),
		iup.Fill(),
	).SetAttributes("NMARGIN=8x6, NGAP=6")
}

func buildMenu() {
	iup.SetHandle("fm_menu", iup.Menu(
		iup.Submenu("File", iup.Menu(
			item("New folder", newFolder),
			item("Rename", renameSelected),
			item("Delete", deleteSelected),
			iup.Separator(),
			item("Quit", func() { iup.ExitLoop() }),
		)),
		iup.Submenu("View", iup.Menu(
			item("Up one level", goUp),
			item("Refresh", refresh),
		)),
	))
}

func sandbox() string {
	dir, err := os.MkdirTemp("", "iup-filemanager-")
	if err != nil {
		panic(err)
	}
	tree := map[string][]string{
		"Documents":          {"notes.txt:1200", "report final.pdf:284000", "budget.csv:8400"},
		"Documents/Archive":  {"2024 summary.txt:4300", "old notes.txt:900"},
		"Pictures":           {"beach.jpg:1840000", "mountain view.png:920000", "screenshot.png:140000"},
		"Pictures/Holiday":   {"día 1.jpg:1200000", "día 2.jpg:1500000"},
		"Projects":           {"README.md:2400", "main.go:15800"},
		"Projects/prototype": {"sketch.svg:33000", "ideas.md:1100"},
		"Μουσική":            {"τραγούδι.mp3:4200000"},
	}
	for sub, files := range tree {
		full := filepath.Join(dir, filepath.FromSlash(sub))
		if err := os.MkdirAll(full, 0o755); err != nil {
			panic(err)
		}
		for _, f := range files {
			name, size, _ := strings.Cut(f, ":")
			var n int64
			fmt.Sscanf(size, "%d", &n)
			if err := os.WriteFile(filepath.Join(full, name), make([]byte, n), 0o644); err != nil {
				panic(err)
			}
		}
	}
	return dir
}

func fillTree() {
	tree := iup.GetHandle("fm_tree")
	if tree.GetInt("COUNT") > 0 {
		tree.SetAttribute("DELNODE", "ALL")
	}
	branches = map[int]string{}
	addBranches(tree, -1, root)
}

func addBranches(tree iup.Ihandle, parent int, dir string) {
	prev := -1
	for _, name := range subdirs(dir) {
		switch {
		case prev >= 0:
			tree.SetAttribute(fmt.Sprintf("INSERTBRANCH%d", prev), name)
		case parent >= 0:
			tree.SetAttribute(fmt.Sprintf("ADDBRANCH%d", parent), name)
		default:
			tree.SetAttribute("ADDBRANCH-1", name)
		}
		prev = tree.GetInt("LASTADDNODE")
		path := filepath.Join(dir, name)
		branches[prev] = path
		iup.SetAttributeId(tree, "IMAGE", prev, "fm_folder")
		iup.SetAttributeId(tree, "IMAGEEXPANDED", prev, "fm_folder")
		addBranches(tree, prev, path)
	}
}

func branchSelected(ih iup.Ihandle, id, state int) int {
	if state != 1 {
		return iup.DEFAULT
	}
	if path, ok := branches[id]; ok {
		current = path
		fillTable()
	}
	return iup.DEFAULT
}

func subdirs(dir string) []string {
	items, err := os.ReadDir(dir)
	if err != nil {
		return nil
	}
	var out []string
	for _, it := range items {
		if it.IsDir() {
			out = append(out, it.Name())
		}
	}
	sort.Strings(out)
	return out
}

func fillTable() {
	table := iup.GetHandle("fm_table")
	shown = nil
	items, _ := os.ReadDir(current)
	for _, it := range items {
		info, err := it.Info()
		if err != nil {
			continue
		}
		shown = append(shown, entry{it.Name(), it.IsDir(), info.Size(), info.ModTime()})
	}
	sortEntries()

	table.SetAttribute("NUMLIN", len(shown))
	for i, e := range shown {
		lin := i + 1
		iup.SetAttributeId2(table, "", lin, 1, e.name)
		iup.SetAttributeId2(table, "", lin, 2, humanSize(e))
		iup.SetAttributeId2(table, "", lin, 3, e.mtime.Format("2006-01-02 15:04"))
		iup.SetAttributeId2(table, "IMAGE", lin, 1, icon(e))
	}
	sign := "UP"
	if !sortAscending {
		sign = "DOWN"
	}
	iup.SetAttributeId(table, "SORTSIGN", sortColumn, sign)

	iup.GetHandle("fm_path").SetAttribute("TITLE", display(current))
	updateStatus()
}

func sortEntries() {
	sort.SliceStable(shown, func(i, j int) bool {
		a, b := shown[i], shown[j]
		if a.dir != b.dir {
			return a.dir
		}
		less := false
		switch sortColumn {
		case 1:
			less = strings.ToLower(a.name) < strings.ToLower(b.name)
		case 2:
			less = a.size < b.size
		case 3:
			less = a.mtime.Before(b.mtime)
		}
		if sortAscending {
			return less
		}
		return !less
	})
}

func sortRequested(ih iup.Ihandle, col int) int {
	if col == sortColumn {
		sortAscending = !sortAscending
	} else {
		sortColumn, sortAscending = col, true
	}
	fillTable()
	return iup.IGNORE
}

func clicked(ih iup.Ihandle, lin, col int, status string) int {
	if lin < 1 || lin > len(shown) {
		return iup.DEFAULT
	}
	e := shown[lin-1]
	switch {
	case iup.IsDouble(status) && e.dir:
		current = filepath.Join(current, e.name)
		fillTable()
	case iup.IsButton3(status):
		menu := iup.Menu(
			item("Rename "+e.name, renameSelected),
			item("Delete "+e.name, deleteSelected),
			iup.Submenu("Move to", moveMenu(e)),
			iup.Separator(),
			item("Copy path", func() { copyPath(filepath.Join(current, e.name)) }),
		)
		iup.Popup(menu, iup.MOUSEPOS, iup.MOUSEPOS)
		iup.Destroy(menu)
	}
	return iup.DEFAULT
}

func moveMenu(e entry) iup.Ihandle {
	source := filepath.Join(current, e.name)
	targets := []iup.Ihandle{}
	for _, dir := range sorted(branches) {
		if dir == current || dir == source {
			continue
		}
		targets = append(targets, item(display(dir), func() { move([]string{source}, dir) }))
	}
	if len(targets) == 0 {
		targets = append(targets, item("No other folder", func() {}))
	}
	return iup.Menu(targets...)
}

func sorted(m map[int]string) []string {
	out := make([]string, 0, len(m))
	for _, v := range m {
		out = append(out, v)
	}
	sort.Strings(out)
	return out
}

func move(paths []string, target string) {
	moved := 0
	for _, p := range paths {
		if err := os.Rename(p, filepath.Join(target, filepath.Base(p))); err == nil {
			moved++
		}
	}
	refresh()
	setStatus(fmt.Sprintf("Moved %d of %d into %s", moved, len(paths), display(target)))
}

func dragBegin(ih iup.Ihandle, x, y int) int {
	dragged = nil
	for _, e := range selection() {
		dragged = append(dragged, filepath.Join(current, e.name))
	}
	if len(dragged) == 0 {
		return iup.IGNORE
	}
	return iup.DEFAULT
}

func dragData(_ iup.Ihandle, _ string, data unsafe.Pointer, size int) int {
	buf := unsafe.Slice((*byte)(data), size)
	if n := copy(buf, strings.Join(dragged, "\n")); n < size {
		buf[n] = 0
	}
	return iup.DEFAULT
}

func dropped(ih iup.Ihandle, _ string, data unsafe.Pointer, size, x, y int) int {
	payload := string(unsafe.Slice((*byte)(data), size))
	if i := strings.IndexByte(payload, 0); i >= 0 {
		payload = payload[:i]
	}
	id := iup.ConvertXYToPos(ih, x, y)
	target, ok := branches[id]
	if !ok || payload == "" {
		return iup.DEFAULT
	}

	move(strings.Split(payload, "\n"), target)
	return iup.DEFAULT
}

func selection() []entry {
	table := iup.GetHandle("fm_table")
	var sel []entry
	for i, state := range table.GetAttribute("SELECTEDLINES") {
		if state == '+' && i < len(shown) {
			sel = append(sel, shown[i])
		}
	}
	return sel
}

func goUp() {
	if current == root {
		return
	}
	current = filepath.Dir(current)
	fillTable()
}

func refresh() {
	fillTree()
	fillTable()
}

func newFolder() {
	name := ask("New folder", "Name:", "untitled")
	if name == "" {
		return
	}
	if err := os.Mkdir(filepath.Join(current, name), 0o755); err != nil {
		iup.Message("New folder", err.Error())
		return
	}
	refresh()
}

func renameSelected() {
	sel := selection()
	if len(sel) != 1 {
		setStatus("Select a single item to rename")
		return
	}
	name := ask("Rename", "New name:", sel[0].name)
	if name == "" || name == sel[0].name {
		return
	}
	old := filepath.Join(current, sel[0].name)
	if err := os.Rename(old, filepath.Join(current, name)); err != nil {
		iup.Message("Rename", err.Error())
		return
	}
	refresh()
}

func deleteSelected() {
	sel := selection()
	if len(sel) == 0 {
		return
	}
	if iup.Alarm("Delete", fmt.Sprintf("Delete %d item(s)?", len(sel)), "Delete", "Cancel", "") != 1 {
		return
	}
	for _, e := range sel {
		os.RemoveAll(filepath.Join(current, e.name))
	}
	refresh()
	setStatus(fmt.Sprintf("Deleted %d item(s)", len(sel)))
}

func ask(title, label, value string) string {
	text := iup.Text().SetAttributes("EXPAND=HORIZONTAL, VISIBLECOLUMNS=24").SetAttribute("VALUE", value)
	result := ""
	ok := iup.Button("OK")
	ok.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		result = text.GetAttribute("VALUE")
		return iup.CLOSE
	}))
	cancel := iup.Button("Cancel")
	cancel.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { return iup.CLOSE }))

	dlg := iup.Dialog(iup.Vbox(
		iup.Hbox(iup.Label(label), text).SetAttributes("NGAP=6, ALIGNMENT=ACENTER"),
		iup.Hbox(iup.Fill(), cancel, ok).SetAttributes("NGAP=6"),
	).SetAttributes("NMARGIN=10x10, NGAP=8"))
	dlg.SetAttributes(map[string]string{
		"TITLE":        title,
		"PARENTDIALOG": "fm_dlg",
		"DIALOGFRAME":  "YES",
	})
	iup.Popup(dlg, iup.CENTERPARENT, iup.CENTERPARENT)
	iup.Destroy(dlg)
	return strings.TrimSpace(result)
}

func copyPath(path string) {
	clip := iup.Clipboard()
	clip.SetAttribute("TEXT", path)
	iup.Destroy(clip)
	setStatus("Copied " + path)
}

func updateStatus() {
	var total int64
	dirs := 0
	for _, e := range shown {
		if e.dir {
			dirs++
			continue
		}
		total += e.size
	}
	setStatus(fmt.Sprintf("%d folders, %d files, %s", dirs, len(shown)-dirs, bytes(total)))
}

func setStatus(s string) { iup.GetHandle("fm_status").SetAttribute("TITLE", s) }

func display(path string) string {
	rel, err := filepath.Rel(root, path)
	if err != nil || rel == "." {
		return "/"
	}
	return "/" + filepath.ToSlash(rel)
}

func humanSize(e entry) string {
	if e.dir {
		return ""
	}
	return bytes(e.size)
}

func bytes(n int64) string {
	switch {
	case n >= 1<<20:
		return fmt.Sprintf("%.1f MB", float64(n)/(1<<20))
	case n >= 1<<10:
		return fmt.Sprintf("%.1f KB", float64(n)/(1<<10))
	}
	return fmt.Sprintf("%d B", n)
}

func icon(e entry) string {
	if e.dir {
		return "fm_folder"
	}
	switch strings.ToLower(filepath.Ext(e.name)) {
	case ".jpg", ".png", ".svg":
		return "fm_image"
	case ".mp3":
		return "fm_audio"
	}
	return "fm_file"
}

func makeIcons() {
	fg := global("TXTFGCOLOR", "60 60 60")
	iup.SetHandle("fm_folder", pixmap(folderMask, "222 176 80"))
	iup.SetHandle("fm_file", pixmap(fileMask, fg))
	iup.SetHandle("fm_image", pixmap(fileMask, "90 150 220"))
	iup.SetHandle("fm_audio", pixmap(fileMask, "150 110 200"))
}

func pixmap(mask []string, color string) iup.Ihandle {
	pix := make([]byte, 16*16)
	for y, row := range mask {
		for x, c := range row {
			if c == 'x' {
				pix[y*16+x] = 1
			}
		}
	}
	return iup.Image(16, 16, pix).SetAttributes(fmt.Sprintf("0=BGCOLOR, 1=%q", color))
}

func global(name, fallback string) string {
	if v := iup.GetGlobal(name); v != "" {
		return v
	}
	return fallback
}

var folderMask = []string{
	"                ",
	"                ",
	"                ",
	"  xxxx          ",
	" xxxxxx         ",
	" xxxxxxxxxxxxx  ",
	" xxxxxxxxxxxxx  ",
	" xxxxxxxxxxxxx  ",
	" xxxxxxxxxxxxx  ",
	" xxxxxxxxxxxxx  ",
	" xxxxxxxxxxxxx  ",
	" xxxxxxxxxxxxx  ",
	" xxxxxxxxxxxxx  ",
	"                ",
	"                ",
	"                ",
}

var fileMask = []string{
	"                ",
	"   xxxxxxxx     ",
	"   xxxxxxxxx    ",
	"   xxxxxxxxxx   ",
	"   xxxx    xx   ",
	"   xxxxxxxxxx   ",
	"   xxxxxxxxxx   ",
	"   xxxxxxxxxx   ",
	"   xxxxxxxxxx   ",
	"   xxxxxxxxxx   ",
	"   xxxxxxxxxx   ",
	"   xxxxxxxxxx   ",
	"   xxxxxxxxxx   ",
	"   xxxxxxxxxx   ",
	"                ",
	"                ",
}

func button(title string, action func()) iup.Ihandle {
	b := iup.Button(title)
	b.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		action()
		return iup.DEFAULT
	}))
	return b
}

func item(title string, action func()) iup.Ihandle {
	it := iup.MenuItem(title)
	it.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		action()
		return iup.DEFAULT
	}))
	return it
}
