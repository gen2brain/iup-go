package main

import (
	"fmt"
	"strings"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

type dataset struct {
	name string
	cols []string
	rows [][]string
}

type dialog struct {
	ih    iup.Ihandle
	table iup.Ihandle
	set   *dataset
}

const (
	closeOne = iota + 1
	closeAll
	quitAll
)

var (
	sets   []*dataset
	opened []*dialog
	hub    iup.Ihandle
)

func main() {
	iup.Open()
	defer iup.Close()

	sets = datasets()

	list := iup.List().SetAttributes("VISIBLELINES=5, VISIBLECOLUMNS=20, EXPAND=YES").SetHandle("sets")
	for i, set := range sets {
		iup.SetAttributeId(list, "", i+1, set.name)
	}
	list.SetAttribute("VALUE", "1")
	list.SetCallback("DBLCLICK_CB", iup.DblclickFunc(func(ih iup.Ihandle, pos int, text string) int {
		open(sets[pos-1])
		return iup.DEFAULT
	}))

	intro := "Every dataset opens in its own dialog, each with its own menu bar."
	if phone() {
		intro = "Each dataset opens in its own dialog."
	}

	hub = iup.Dialog(iup.Vbox(
		iup.Label(intro),
		list,
		iup.Button("Open").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
			open(sets[list.GetInt("VALUE")-1])
			return iup.DEFAULT
		})),
		iup.Label("No dialog open").SetAttribute("EXPAND", "HORIZONTAL").SetHandle("hubstatus"),
	).SetAttributes("MARGIN=10x10, NGAP=6"))

	hub.SetAttribute("TITLE", "Multi Dialog")
	hub.SetCallback("POSTMESSAGE_CB", iup.PostMessageFunc(deferred))
	hub.SetCallback("CLOSE_CB", iup.CloseFunc(func(iup.Ihandle) int {
		destroyAll()
		return iup.CLOSE
	}))
	setMenu(hub, menuFor(nil))

	iup.Show(hub)
	iup.MainLoop()
}

func open(set *dataset) {
	if d := dialogOf(set); d != nil {
		raise(d.ih)
		return
	}

	table := iup.Table().SetAttributes(fmt.Sprintf("NUMCOL=%d, NUMLIN=%d, VISIBLELINES=%d, ALTERNATECOLOR=YES, EXPAND=YES",
		len(set.cols), len(set.rows), len(set.rows)))
	for i, title := range set.cols {
		iup.SetAttributeId(table, "TITLE", i+1, title)
	}
	for lin, row := range set.rows {
		for col, value := range row {
			iup.SetAttributeId2(table, "", lin+1, col+1, value)
		}
	}

	d := &dialog{set: set, table: table}
	d.ih = iup.Dialog(iup.Vbox(
		table,
		iup.Label(fmt.Sprintf("%d rows", len(set.rows))).SetAttribute("EXPAND", "HORIZONTAL"),
	).SetAttributes("MARGIN=6x6, NGAP=4"))
	d.ih.SetAttribute("TITLE", set.name)
	d.ih.SetCallback("CLOSE_CB", iup.CloseFunc(func(ih iup.Ihandle) int {
		iup.PostMessage(hub, set.name, closeOne, nil)
		return iup.IGNORE
	}))

	opened = append(opened, d)
	refresh()

	x, y := cascade(len(opened) - 1)
	iup.ShowXY(d.ih, x, y)
}

func deferred(ih iup.Ihandle, s string, i int, _ any) int {
	switch i {
	case closeOne:
		if d := dialogNamed(s); d != nil {
			destroy(d)
			refresh()
		}
	case closeAll:
		destroyAll()
		refresh()
	case quitAll:
		destroyAll()
		return iup.CLOSE
	}
	return iup.DEFAULT
}

func destroy(d *dialog) {
	for i, other := range opened {
		if other == d {
			opened = append(opened[:i], opened[i+1:]...)
			break
		}
	}
	iup.Destroy(d.ih)
}

func destroyAll() {
	for _, d := range opened {
		iup.Destroy(d.ih)
	}
	opened = nil
}

func refresh() {
	setMenu(hub, menuFor(nil))
	for _, d := range opened {
		setMenu(d.ih, menuFor(d))
	}

	status := "No dialog open"
	if len(opened) > 0 {
		names := make([]string, 0, len(opened))
		for _, d := range opened {
			names = append(names, d.set.name)
		}
		status = fmt.Sprintf("%d open: %s", len(opened), strings.Join(names, ", "))
	}
	iup.GetHandle("hubstatus").SetAttribute("TITLE", status)
}

func setMenu(dlg iup.Ihandle, menu iup.Ihandle) {
	old := iup.GetAttributeHandle(dlg, "MENU")
	iup.SetAttributeHandle(dlg, "MENU", menu)
	if old != 0 {
		iup.Destroy(old)
	}
}

func menuFor(d *dialog) iup.Ihandle {
	file := iup.Menu(
		item("&Open dataset...\tCtrl+O", func() { raise(hub) }),
		iup.MenuSeparator(),
		item("&Close\tCtrl+W", func() {
			if d != nil {
				iup.PostMessage(hub, d.set.name, closeOne, nil)
			}
		}).SetAttribute("ACTIVE", boolean(d != nil)),
		item("Close &all dialogs", func() { iup.PostMessage(hub, "", closeAll, nil) }).
			SetAttribute("ACTIVE", boolean(len(opened) > 0)),
		iup.MenuSeparator(),
		item("E&xit\tCtrl+Q", func() { iup.PostMessage(hub, "", quitAll, nil) }),
	)

	items := []iup.Ihandle{
		item("&Cascade", cascadeAll).SetAttribute("ACTIVE", boolean(len(opened) > 0)),
		iup.MenuSeparator(),
	}
	for _, other := range opened {
		entry := item(other.set.name, func() { raise(other.ih) })
		if other == d {
			entry.SetAttribute("VALUE", "ON")
		}
		items = append(items, entry)
	}
	if len(opened) == 0 {
		items = append(items, item("(none)", func() {}).SetAttribute("ACTIVE", "NO"))
	}

	bar := []iup.Ihandle{iup.Submenu("&File", file)}
	if d != nil {
		bar = append(bar, iup.Submenu("&Edit", iup.Menu(
			item("&Find...\tCtrl+F", func() { find(d) }),
		)))
	}
	bar = append(bar, iup.Submenu("&Dialog", iup.Menu(items...)))

	return iup.Menu(bar...)
}

func find(d *dialog) {
	text := iup.Text().SetAttributes("VISIBLECOLUMNS=18, EXPAND=HORIZONTAL")
	status := iup.Label("Type a value and press Enter").SetAttribute("EXPAND", "HORIZONTAL")

	search := iup.Button("Find")
	dismiss := iup.Button("Close").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		return iup.CLOSE
	}))

	dlg := iup.Dialog(iup.Vbox(
		iup.Label("Find in "+d.set.name),
		text,
		iup.Hbox(iup.Fill(), search, dismiss).SetAttribute("NGAP", "4"),
		status,
	).SetAttributes("MARGIN=10x10, NGAP=6"))

	search.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		status.SetAttribute("TITLE", locate(d, text.GetAttribute("VALUE")))
		return iup.DEFAULT
	}))

	dlg.SetAttributes(`TITLE="Find", DIALOGFRAME=YES`)
	dlg.SetAttributeHandle("PARENTDIALOG", d.ih)
	dlg.SetAttributeHandle("DEFAULTENTER", search)
	dlg.SetAttributeHandle("DEFAULTESC", dismiss)

	iup.Popup(dlg, iup.CENTERPARENT, iup.CENTERPARENT)
	iup.Destroy(dlg)
}

func locate(d *dialog, value string) string {
	if value == "" {
		return "Type a value and press Enter"
	}

	for lin, row := range d.set.rows {
		for col, cell := range row {
			if strings.Contains(strings.ToLower(cell), strings.ToLower(value)) {
				d.table.SetAttribute("FOCUSCELL", fmt.Sprintf("%d:%d", lin+1, col+1))
				raise(d.ih)
				return fmt.Sprintf("Found in row %d, %s", lin+1, d.set.cols[col])
			}
		}
	}

	return "Not found in " + d.set.name
}

func cascadeAll() {
	for i, d := range opened {
		x, y := cascade(i)
		iup.ShowXY(d.ih, x, y)
	}
}

func cascade(index int) (int, int) {
	return 60 + index*40, 60 + index*40
}

func raise(ih iup.Ihandle) {
	ih.SetAttribute("BRINGFRONT", "YES")
}

func phone() bool {
	switch iup.GetGlobal("SYSTEM") {
	case "Android", "iOS":
		return true
	}
	return false
}

func dialogOf(set *dataset) *dialog {
	for _, d := range opened {
		if d.set == set {
			return d
		}
	}
	return nil
}

func dialogNamed(name string) *dialog {
	for _, d := range opened {
		if d.set.name == name {
			return d
		}
	}
	return nil
}

func item(title string, action func()) iup.Ihandle {
	return iup.MenuItem(title).SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		action()
		return iup.DEFAULT
	}))
}

func boolean(b bool) string {
	if b {
		return "YES"
	}
	return "NO"
}

func datasets() []*dataset {
	return []*dataset{
		{
			name: "Servers",
			cols: []string{"Host", "Role", "Load"},
			rows: [][]string{
				{"alpha", "web", "0.42"},
				{"beta", "web", "0.71"},
				{"gamma", "database", "1.20"},
				{"delta", "cache", "0.08"},
				{"epsilon", "queue", "0.55"},
			},
		},
		{
			name: "Releases",
			cols: []string{"Version", "Date", "Notes"},
			rows: [][]string{
				{"0.9.0", "2026-01-14", "First public build"},
				{"0.9.4", "2026-03-02", "Scroll fixes"},
				{"1.0.0", "2026-05-19", "Stable"},
				{"1.0.1", "2026-06-30", "Menu shortcuts"},
			},
		},
		{
			name: "Tickets",
			cols: []string{"Id", "Reporter", "State", "Summary"},
			rows: [][]string{
				{"114", "ana", "open", "Dropdown measures zero when hidden"},
				{"118", "boris", "closed", "Scrollbar page step ignored"},
				{"121", "chen", "open", "Tree column clipped after resize"},
				{"126", "dmitri", "open", "Menu bar leaks on close"},
				{"130", "elena", "closed", "Text column too narrow"},
				{"133", "farid", "open", "Canvas reports only POS"},
			},
		},
	}
}
