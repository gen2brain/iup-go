package main

import (
	"fmt"
	"math"
	"sort"
	"strings"
	"time"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

type mail struct {
	from    string
	address string
	subject string
	date    time.Time
	body    string
	unread  bool
	flagged bool
	files   []string
}

type folder struct {
	name     string
	icon     string
	messages []*mail
	id       int
}

var (
	folders                                       []*folder
	current                                       *folder
	shown                                         []*mail
	selected                                      *mail
	sortColumn                                    = 4
	sortAscending                                 bool
	iconColor, accentColor, dimColor, unreadColor string
	iconGeneration                                int
)

const dateFormat = "2006-01-02 15:04"

func main() {
	iup.Open()
	defer iup.Close()

	iup.SetGlobal("UTF8MODE", "YES")
	makeFolders()
	setColors()
	makeIcons()

	tree := iup.Tree().SetAttributes(`ADDROOT=NO, EXPAND=YES, HIDELINES=YES, VISIBLECOLUMNS=14`).SetHandle("mail_tree")
	tree.SetCallback("SELECTION_CB", iup.SelectionFunc(folderSelected))

	table := iup.Table().SetAttributes(`EXPAND=YES, NUMCOL=5, SHOWIMAGE=YES, SORTABLE=YES, USERRESIZE=YES,
		STRETCHLAST=YES, ALTERNATECOLOR=YES, FOCUSRECT=NO, VISIBLELINES=8`).SetHandle("mail_table")
	table.SetAttributes(map[string]string{
		"TITLE1": " ", "TITLE2": "From", "TITLE3": "Subject", "TITLE4": "Date", "TITLE5": "Size",
		"ALIGNMENT5": "ARIGHT",
	})
	table.SetCallback("ENTERITEM_CB", iup.EnterItemFunc(rowSelected))
	table.SetCallback("CLICK_CB", iup.ClickFunc(func(ih iup.Ihandle, lin, col int, status string) int {
		if iup.IsDouble(status) && lin >= 1 && lin <= len(shown) {
			forward()
		}
		return iup.DEFAULT
	}))
	table.SetCallback("SORT_CB", iup.TableSortFunc(sortRequested))

	preview := iup.Text().SetAttributes(`MULTILINE=YES, FORMATTING=YES, READONLY=YES, EXPAND=YES,
		WORDWRAP=YES, BORDER=NO, VISIBLELINES=10`).SetHandle("mail_preview")

	search := iup.Text().SetAttributes(`VISIBLECOLUMNS=16, CUEBANNER="Search mail"`).SetHandle("mail_search")
	search.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(iup.Ihandle) int { fillTable(); return iup.DEFAULT }))

	toolbar := iup.Hbox(
		toolButton("refresh", "Get new mail", getMail),
		toolButton("compose", "Write a new message", compose),
		iup.Label("").SetAttribute("SEPARATOR", "VERTICAL"),
		toolButton("reply", "Reply", func() { reply(false) }),
		toolButton("replyall", "Reply to all", func() { reply(true) }),
		toolButton("forward", "Forward", forward),
		iup.Label("").SetAttribute("SEPARATOR", "VERTICAL"),
		toolButton("flag", "Flag or unflag", toggleFlag),
		toolButton("unread", "Mark read or unread", toggleRead),
		toolButton("trash", "Delete", deleteMessage),
		iup.Fill(),
		search,
	).SetAttributes("NGAP=4, NMARGIN=4x4, ALIGNMENT=ACENTER")

	right := iup.Split(table, preview).SetAttributes("ORIENTATION=HORIZONTAL, VALUE=520, SHOWGRIP=YES")
	main := iup.Split(tree, right).SetAttributes("ORIENTATION=VERTICAL, VALUE=230, MINMAX=120:400")

	statusFolder := iup.Label("").SetAttribute("EXPAND", "HORIZONTAL").SetHandle("mail_folder")
	statusCount := iup.Label("").SetHandle("mail_count")
	statusSelected := iup.Label("").SetHandle("mail_selected")
	status := iup.Hbox(statusFolder,
		iup.Label("").SetAttribute("SEPARATOR", "VERTICAL"), statusCount,
		iup.Label("").SetAttribute("SEPARATOR", "VERTICAL"), statusSelected,
	).SetAttributes("NGAP=8, NMARGIN=6x3, ALIGNMENT=ACENTER")

	dlg := iup.Dialog(iup.Vbox(toolbar, main, status)).SetHandle("mail_dlg").SetAttributes(map[string]string{
		"TITLE":     "Mail",
		"MENU":      "mail_menu",
		"PLACEMENT": "MAXIMIZED",
	})
	dlg.SetCallback("THEMECHANGED_CB", iup.ThemeChangedFunc(func(iup.Ihandle, int) int { retheme(); return iup.DEFAULT }))
	buildMenu()

	iup.Show(dlg)
	fillTree()
	selectFolder(folders[0])

	iup.MainLoop()
}

func buildMenu() {
	darkItem := item("Dar&k Mode", toggleAppearance).SetHandle("mail_dark")
	showAppearance()

	iup.Menu(
		iup.Submenu("&File", iup.Menu(
			item("&Get New Mail\tF5", getMail),
			item("&Write\tCtrl+N", compose),
			iup.MenuSeparator(),
			item("E&xit", func() { iup.ExitLoop() }),
		)),
		iup.Submenu("&Message", iup.Menu(
			item("&Reply\tCtrl+R", func() { reply(false) }),
			item("Reply to &All\tCtrl+Shift+R", func() { reply(true) }),
			item("&Forward\tCtrl+L", forward),
			iup.MenuSeparator(),
			item("Mark Read or &Unread\tCtrl+U", toggleRead),
			item("Fla&g\tCtrl+G", toggleFlag),
			item("&Delete\tDel", deleteMessage),
		)),
		iup.Submenu("&View", iup.Menu(
			item("Sort by &Date", func() { sortBy(4) }),
			item("Sort by &Sender", func() { sortBy(2) }),
			item("Sort by Su&bject", func() { sortBy(3) }),
			iup.MenuSeparator(),
			darkItem,
		)),
	).SetHandle("mail_menu")
}

func item(title string, action func()) iup.Ihandle {
	return iup.MenuItem(title).SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		action()
		return iup.DEFAULT
	}))
}

func toolButton(icon, tip string, action func()) iup.Ihandle {
	b := iup.Button("").SetAttributes(map[string]string{
		"IMAGE": iconName(icon), "TIP": tip, "FLAT": "YES", "PADDING": "6x6", "CANFOCUS": "NO",
	})
	b.SetAttribute("_ICON", icon)
	b.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		action()
		return iup.DEFAULT
	}))
	return b
}

func fillTree() {
	tree := iup.GetHandle("mail_tree")

	for i, f := range folders {
		op := "ADDBRANCH-1"
		if i > 0 {
			op = fmt.Sprintf("INSERTBRANCH%d", folders[i-1].id)
		}
		tree.SetAttribute(op, f.name)
		f.id = tree.GetInt("LASTADDNODE")
		iup.SetAttributeId(tree, "IMAGE", f.id, iconName(f.icon))
		iup.SetAttributeId(tree, "IMAGEEXPANDED", f.id, iconName(f.icon))
		iup.TreeSetUserId(tree, f.id, uintptr(i+1))
	}
	updateTree()
}

func updateTree() {
	tree := iup.GetHandle("mail_tree")

	for _, f := range folders {
		title := f.name
		if n := unreadCount(f); n > 0 {
			title = fmt.Sprintf("%s (%d)", f.name, n)
			iup.SetAttributeId(tree, "TITLEFONTSTYLE", f.id, "Bold")
		} else {
			iup.SetAttributeId(tree, "TITLEFONTSTYLE", f.id, "Normal")
		}
		iup.SetAttributeId(tree, "TITLE", f.id, title)
	}
}

func unreadCount(f *folder) int {
	n := 0
	for _, m := range f.messages {
		if m.unread {
			n++
		}
	}
	return n
}

func folderSelected(ih iup.Ihandle, id, state int) int {
	if state != 1 {
		return iup.DEFAULT
	}
	if i := int(iup.TreeGetUserId(ih, id)); i > 0 && i <= len(folders) {
		selectFolder(folders[i-1])
	}
	return iup.DEFAULT
}

func selectFolder(f *folder) {
	current = f
	iup.SetAttributeId(iup.GetHandle("mail_tree"), "VALUE", f.id, "")
	fillTable()
}

func fillTable() {
	table := iup.GetHandle("mail_table")
	query := strings.ToLower(iup.GetHandle("mail_search").GetAttribute("VALUE"))
	shown = nil
	for _, m := range current.messages {
		if query == "" || strings.Contains(strings.ToLower(m.from+" "+m.subject+" "+m.body), query) {
			shown = append(shown, m)
		}
	}
	sortMessages()

	table.SetAttribute("NUMLIN", len(shown))
	for i, m := range shown {
		lin := i + 1
		iup.SetAttributeId2(table, "", lin, 1, "")
		iup.SetAttributeId2(table, "", lin, 2, m.from)
		iup.SetAttributeId2(table, "", lin, 3, m.subject)
		iup.SetAttributeId2(table, "", lin, 4, m.date.Format(dateFormat))
		iup.SetAttributeId2(table, "", lin, 5, size(m))
		iup.SetAttributeId2(table, "IMAGE", lin, 1, rowIcon(m))
	}
	sign := "DOWN"
	if sortAscending {
		sign = "UP"
	}
	iup.SetAttributeId(table, "SORTSIGN", sortColumn, sign)

	selected = nil
	iup.GetHandle("mail_preview").SetAttribute("VALUE", "")
	if len(shown) > 0 {
		table.SetAttribute("FOCUSCELL", "1:2")
		showMessage(shown[0])
	}
	updateStatus()
}

func rowIcon(m *mail) string {
	switch {
	case m.flagged:
		return iconName("flag")
	case m.unread:
		return iconName("unread")
	case len(m.files) > 0:
		return iconName("clip")
	}
	return iconName("read")
}

func size(m *mail) string {
	n := len(m.body) + len(m.subject)
	for _, f := range m.files {
		n += 40000 + len(f)*1000
	}
	if n >= 1024 {
		return fmt.Sprintf("%.1f KB", float64(n)/1024)
	}
	return fmt.Sprintf("%d B", n)
}

func sortRequested(ih iup.Ihandle, col int) int {
	sortBy(col)
	return iup.IGNORE
}

func sortBy(col int) {
	if col < 2 {
		return
	}
	if col == sortColumn {
		sortAscending = !sortAscending
	} else {
		sortColumn, sortAscending = col, true
	}
	fillTable()
}

func sortMessages() {
	sort.SliceStable(shown, func(i, j int) bool {
		a, b := shown[i], shown[j]
		less := a.date.Before(b.date)
		switch sortColumn {
		case 2:
			less = strings.ToLower(a.from) < strings.ToLower(b.from)
		case 3:
			less = strings.ToLower(a.subject) < strings.ToLower(b.subject)
		case 5:
			less = len(a.body) < len(b.body)
		}
		if sortAscending {
			return less
		}
		return !less
	})
}

func rowSelected(ih iup.Ihandle, lin, col int) int {
	if lin >= 1 && lin <= len(shown) {
		showMessage(shown[lin-1])
	}
	return iup.DEFAULT
}

func showMessage(m *mail) {
	selected = m
	if m.unread {
		m.unread = false
		updateRow(m)
		updateTree()
	}

	var b strings.Builder
	fmt.Fprintf(&b, "## %s\n\n", m.subject)
	fmt.Fprintf(&b, "**%s** <%s>  \n", m.from, m.address)
	fmt.Fprintf(&b, "*%s*\n\n", m.date.Format("Monday, 2 January 2006 at 15:04"))
	if len(m.files) > 0 {
		fmt.Fprintf(&b, "> Attachments: %s\n\n", strings.Join(m.files, ", "))
	}
	b.WriteString("---\n\n")
	b.WriteString(m.body)

	/* the markdown attribute is ignored where it is not supported, the plain text stays */
	preview := iup.GetHandle("mail_preview")
	preview.SetAttribute("VALUE", b.String())
	preview.SetAttribute("MARKDOWNVALUE", b.String())
	updateStatus()
}

func updateRow(m *mail) {
	for i, s := range shown {
		if s == m {
			table := iup.GetHandle("mail_table")
			iup.SetAttributeId2(table, "IMAGE", i+1, 1, rowIcon(m))
			table.SetAttribute("REDRAW", "ALL")
			return
		}
	}
}

func updateStatus() {
	statusFolder := iup.GetHandle("mail_folder")

	statusFolder.SetAttribute("TITLE", current.name)
	iup.GetHandle("mail_count").SetAttribute("TITLE", fmt.Sprintf("%d messages, %d unread", len(current.messages), unreadCount(current)))
	if selected != nil {
		iup.GetHandle("mail_selected").SetAttribute("TITLE", selected.from+", "+size(selected))
	} else {
		iup.GetHandle("mail_selected").SetAttribute("TITLE", "No message selected")
	}
	iup.Refresh(statusFolder)
}

func getMail() {
	inbox := folders[0]
	m := &mail{
		from: "Continuous Integration", address: "ci@example.org",
		subject: "Build #" + fmt.Sprint(1200+len(inbox.messages)) + " passed",
		date:    time.Now(), unread: true,
		body: "All checks are green.\n\n| Platform | Tests | Time |\n|---|---:|---:|\n| linux | 412 | 1m02s |\n| windows | 412 | 1m31s |\n| macos | 412 | 1m11s |\n\nNothing to do.\n",
	}
	inbox.messages = append(inbox.messages, m)
	updateTree()
	if current == inbox {
		fillTable()
	}
	iup.GetHandle("mail_folder").SetAttribute("TITLE", "One new message")
}

func compose() { composeDialog("", "", "") }

func reply(all bool) {
	if selected == nil {
		return
	}
	to := selected.address
	if all {
		to += ", team@example.org"
	}
	composeDialog(to, "Re: "+strings.TrimPrefix(selected.subject, "Re: "), quote(selected))
}

func forward() {
	if selected == nil {
		return
	}
	composeDialog("", "Fwd: "+selected.subject, quote(selected))
}

func quote(m *mail) string {
	var b strings.Builder
	fmt.Fprintf(&b, "\n\nOn %s, %s wrote:\n", m.date.Format(dateFormat), m.from)
	for line := range strings.SplitSeq(strings.TrimSpace(m.body), "\n") {
		fmt.Fprintf(&b, "> %s\n", line)
	}
	return b.String()
}

func composeDialog(to, subject, body string) {
	toText := iup.Text().SetAttributes(fmt.Sprintf("EXPAND=HORIZONTAL, VALUE=%q, CUEBANNER=\"someone@example.org\"", to))
	subjectText := iup.Text().SetAttributes(fmt.Sprintf("EXPAND=HORIZONTAL, VALUE=%q", subject))
	bodyText := iup.Text().SetAttributes(fmt.Sprintf("MULTILINE=YES, EXPAND=YES, WORDWRAP=YES, VISIBLELINES=14, VISIBLECOLUMNS=60, VALUE=%q", body))

	var win iup.Ihandle
	send := iup.Button("Send").SetAttribute("PADDING", "12x4")
	send.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		sent := folders[2]
		sent.messages = append(sent.messages, &mail{
			from: "Me", address: toText.GetAttribute("VALUE"),
			subject: subjectText.GetAttribute("VALUE"), date: time.Now(),
			body: bodyText.GetAttribute("VALUE"),
		})
		updateTree()
		if current == sent {
			fillTable()
		}
		iup.GetHandle("mail_folder").SetAttribute("TITLE", "Message sent")
		return iup.CLOSE
	}))
	cancel := iup.Button("Cancel").SetAttribute("PADDING", "12x4")
	cancel.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { return iup.CLOSE }))

	grid := iup.GridBox(
		iup.Label("To"), toText,
		iup.Label("Subject"), subjectText,
	).SetAttributes("ORIENTATION=HORIZONTAL, NUMDIV=2, SIZECOL=-1, SIZELIN=-1, NGAPCOL=6, NGAPLIN=4, ALIGNMENTLIN=ACENTER")

	win = iup.Dialog(iup.Vbox(grid, bodyText,
		iup.Hbox(iup.Fill(), cancel, send).SetAttributes("NGAP=6"),
	).SetAttributes("NMARGIN=10x10, NGAP=8")).SetAttributes(`TITLE="New Message"`)
	iup.SetAttributeHandle(win, "PARENTDIALOG", iup.GetHandle("mail_dlg"))
	iup.SetAttributeHandle(win, "DEFAULTENTER", send)
	iup.SetAttributeHandle(win, "DEFAULTESC", cancel)
	defer win.Destroy()
	iup.Popup(win, iup.CENTERPARENT, iup.CENTERPARENT)
}

func toggleRead() {
	if selected == nil {
		return
	}
	selected.unread = !selected.unread
	updateRow(selected)
	updateTree()
	updateStatus()
}

func toggleFlag() {
	if selected == nil {
		return
	}
	selected.flagged = !selected.flagged
	updateRow(selected)
}

func deleteMessage() {
	if selected == nil || current == folders[4] {
		return
	}
	for i, m := range current.messages {
		if m == selected {
			current.messages = append(current.messages[:i], current.messages[i+1:]...)
			break
		}
	}
	folders[4].messages = append(folders[4].messages, selected)
	updateTree()
	fillTable()
}

func toggleAppearance() {
	if iup.GetGlobal("APPEARANCE") == "DARK" {
		iup.SetGlobal("APPEARANCE", "LIGHT")
	} else {
		iup.SetGlobal("APPEARANCE", "DARK")
	}
	showAppearance()
	retheme()
}

func showAppearance() {
	value := "OFF"
	if iup.GetGlobal("APPEARANCE") == "DARK" {
		value = "ON"
	}
	iup.GetHandle("mail_dark").SetAttribute("VALUE", value)
}

func retheme() {
	setColors()
	makeIcons()
	updateTree()
	fillTable()
}

func setColors() {
	iconColor = global("TXTFGCOLOR", "0 0 0")
	accentColor = global("ACCENTCOLOR", global("TXTHLCOLOR", "60 120 220"))
	dimColor = mix(iconColor, global("DLGBGCOLOR", "240 240 240"), 0.45)
	unreadColor = accentColor
}

func global(name, fallback string) string {
	if v := iup.GetGlobal(name); v != "" {
		return v
	}
	return fallback
}

func rgb(c string) (float64, float64, float64) {
	var r, g, b int
	if strings.HasPrefix(c, "#") {
		fmt.Sscanf(c, "#%02x%02x%02x", &r, &g, &b)
	} else {
		fmt.Sscanf(c, "%d %d %d", &r, &g, &b)
	}
	return float64(r), float64(g), float64(b)
}

func mix(a, b string, t float64) string {
	ar, ag, ab := rgb(a)
	br, bg, bb := rgb(b)
	return fmt.Sprintf("%d %d %d", int(ar+(br-ar)*t), int(ag+(bg-ag)*t), int(ab+(bb-ab)*t))
}

type sdf func(x, y float64) float64

type layer struct {
	shape sdf
	color string
}

func union(fs ...sdf) sdf {
	return func(x, y float64) float64 {
		d := math.MaxFloat64
		for _, f := range fs {
			d = math.Min(d, f(x, y))
		}
		return d
	}
}

func cut(a, b sdf) sdf {
	return func(x, y float64) float64 { return math.Max(a(x, y), -b(x, y)) }
}

func box(x0, y0, x1, y1 float64) sdf {
	return func(x, y float64) float64 {
		return math.Max(math.Max(x0-x, x-x1), math.Max(y0-y, y-y1))
	}
}

func disc(cx, cy, r float64) sdf {
	return func(x, y float64) float64 { return math.Hypot(x-cx, y-cy) - r }
}

func ring(cx, cy, r, t float64) sdf {
	return func(x, y float64) float64 { return math.Abs(math.Hypot(x-cx, y-cy)-r) - t/2 }
}

func bar(x0, y0, x1, y1, t float64) sdf {
	return func(x, y float64) float64 {
		dx, dy := x1-x0, y1-y0
		l := dx*dx + dy*dy
		u := 0.0
		if l > 0 {
			u = math.Max(0, math.Min(1, ((x-x0)*dx+(y-y0)*dy)/l))
		}
		return math.Hypot(x-(x0+u*dx), y-(y0+u*dy)) - t/2
	}
}

func arc(cx, cy, r, t, a0, a1 float64) sdf {
	band := ring(cx, cy, r, t)
	return func(x, y float64) float64 {
		a := math.Atan2(y-cy, x-cx)
		for a < a0 {
			a += 2 * math.Pi
		}
		if a <= a1 {
			return band(x, y)
		}
		return math.Min(
			math.Hypot(x-(cx+r*math.Cos(a0)), y-(cy+r*math.Sin(a0))),
			math.Hypot(x-(cx+r*math.Cos(a1)), y-(cy+r*math.Sin(a1)))) - t/2
	}
}

func wedge(x0, y0, x1, y1, x2, y2 float64) sdf {
	side := func(ax, ay, bx, by, px, py float64) float64 {
		return (bx-ax)*(py-ay) - (by-ay)*(px-ax)
	}
	return func(x, y float64) float64 {
		a := side(x0, y0, x1, y1, x, y)
		b := side(x1, y1, x2, y2, x, y)
		c := side(x2, y2, x0, y0, x, y)
		if (a >= 0 && b >= 0 && c >= 0) || (a <= 0 && b <= 0 && c <= 0) {
			return -1
		}
		return 1
	}
}

func envelope(t float64) sdf {
	return union(cut(box(0.10, 0.24, 0.90, 0.76), box(0.10+t, 0.24+t, 0.90-t, 0.76-t)),
		bar(0.10, 0.24, 0.50, 0.54, t), bar(0.50, 0.54, 0.90, 0.24, t))
}

func iconName(base string) string {
	return fmt.Sprintf("mail_%s_%d", base, iconGeneration)
}

func makeIcons() {
	iconGeneration++
	size := 2 * iup.GetGlobalInt("DEFAULTFONTSIZE")
	if size < 16 {
		size = 16
	}

	arrow := func(tip float64) sdf {
		return union(bar(tip, 0.50, 0.82, 0.50, 0.10), bar(tip, 0.50, tip+0.18, 0.32, 0.10),
			bar(tip, 0.50, tip+0.18, 0.68, 0.10), bar(0.82, 0.50, 0.82, 0.74, 0.10))
	}

	shapes := map[string]layer{
		"inbox":   {union(cut(box(0.14, 0.30, 0.86, 0.80), box(0.22, 0.38, 0.78, 0.60)), bar(0.14, 0.60, 0.34, 0.60, 0.09), bar(0.66, 0.60, 0.86, 0.60, 0.09)), accentColor},
		"drafts":  {union(bar(0.24, 0.78, 0.74, 0.28, 0.13), bar(0.18, 0.84, 0.28, 0.74, 0.10)), iconColor},
		"sent":    {wedge(0.12, 0.14, 0.88, 0.50, 0.12, 0.86), iconColor},
		"spam":    {union(ring(0.5, 0.5, 0.34, 0.10), bar(0.5, 0.30, 0.5, 0.56, 0.10), disc(0.5, 0.70, 0.055)), iconColor},
		"trash":   {union(cut(box(0.26, 0.28, 0.74, 0.84), box(0.34, 0.36, 0.66, 0.76)), bar(0.18, 0.26, 0.82, 0.26, 0.10), bar(0.40, 0.18, 0.60, 0.18, 0.10)), iconColor},
		"unread":  {envelope(0.09), unreadColor},
		"read":    {envelope(0.07), dimColor},
		"flag":    {union(bar(0.30, 0.16, 0.30, 0.86, 0.09), wedge(0.34, 0.20, 0.78, 0.36, 0.34, 0.54)), accentColor},
		"clip":    {cut(union(arc(0.50, 0.40, 0.22, 0.09, math.Pi, 2*math.Pi), bar(0.28, 0.40, 0.28, 0.66, 0.09), bar(0.72, 0.40, 0.72, 0.56, 0.09), arc(0.50, 0.66, 0.22, 0.09, 0, math.Pi)), box(0.60, 0.70, 1.0, 1.0)), dimColor},
		"refresh": {union(arc(0.5, 0.5, 0.30, 0.10, 0.35*math.Pi, 1.9*math.Pi), wedge(0.62, 0.10, 0.90, 0.26, 0.60, 0.40)), iconColor},
		"compose": {union(bar(0.26, 0.76, 0.76, 0.26, 0.13), bar(0.20, 0.82, 0.30, 0.72, 0.10), bar(0.14, 0.88, 0.86, 0.88, 0.09)), iconColor},
		"reply":   {arrow(0.18), iconColor},
		"forward": {union(bar(0.82, 0.50, 0.18, 0.50, 0.10), bar(0.82, 0.50, 0.64, 0.32, 0.10), bar(0.82, 0.50, 0.64, 0.68, 0.10), bar(0.18, 0.50, 0.18, 0.74, 0.10)), iconColor},
	}
	shapes["replyall"] = layer{union(arrow(0.30), bar(0.06, 0.50, 0.24, 0.32, 0.10), bar(0.06, 0.50, 0.24, 0.68, 0.10)), iconColor}

	for name, l := range shapes {
		iup.ImageRGBA(size, size, raster(size, l)).SetHandle(iconName(name))
	}
}

func raster(size int, l layer) []byte {
	pix := make([]byte, size*size*4)
	r, g, b := rgb(l.color)
	const ss = 3
	for y := range size {
		for x := range size {
			hits := 0
			for sy := range ss {
				for sx := range ss {
					u := (float64(x) + (float64(sx)+0.5)/ss) / float64(size)
					v := (float64(y) + (float64(sy)+0.5)/ss) / float64(size)
					if l.shape(u, v) <= 0 {
						hits++
					}
				}
			}
			if hits == 0 {
				continue
			}
			i := (y*size + x) * 4
			pix[i], pix[i+1], pix[i+2] = byte(r), byte(g), byte(b)
			pix[i+3] = byte(255 * hits / (ss * ss))
		}
	}
	return pix
}

func makeFolders() {
	day := func(d, h, m int) time.Time {
		return time.Date(2026, time.September, d, h, m, 0, 0, time.Local)
	}

	inbox := &folder{name: "Inbox", icon: "inbox", messages: []*mail{
		{from: "Ada Lovelace", address: "ada@analytical.example", subject: "Notes on the Analytical Engine", date: day(15, 9, 12), unread: true,
			files: []string{"engine-notes.pdf"},
			body: "Good morning,\n\nI have finished the notes on the engine. The key point is in **note G**: the machine can act upon *other things besides number*.\n\n" +
				"A short sketch of the sequence:\n\n1. Load the initial values\n2. Run the recurring loop\n3. Emit the Bernoulli number\n\n" +
				"```\nfor n := 1; n <= 8; n++ {\n    b[n] = bernoulli(n)\n}\n```\n\nThe attachment has the full table.\n\nRegards,\nAda\n"},
		{from: "Grace Hopper", address: "grace@navy.example", subject: "Compiler progress and a moth", date: day(15, 8, 3), unread: true,
			body: "The compiler now translates the whole sample program.\n\n> A ship in port is safe, but that is not what ships are built for.\n\n" +
				"Also, we found the cause of the failure in relay 70. It was an actual moth. It is taped into the log book.\n"},
		{from: "Edsger Dijkstra", address: "edsger@ewd.example", subject: "Re: shortest path review", date: day(14, 17, 45),
			body: "Your proof holds, but the presentation can be tightened.\n\n" +
				"| Section | Verdict | Note |\n|---|---|---|\n| 1 | fine | keep |\n| 2 | rework | the invariant is stated twice |\n| 3 | fine | shorten the example |\n\n" +
				"Simplicity is a great virtue but it requires hard work to achieve it.\n"},
		{from: "Continuous Integration", address: "ci@example.org", subject: "Build #1199 passed", date: day(14, 11, 30),
			body: "All checks are green.\n\n- linux: 412 tests\n- windows: 412 tests\n- macos: 412 tests\n\nNothing to do.\n"},
		{from: "Margaret Hamilton", address: "margaret@apollo.example", subject: "Priority display routines", date: day(13, 20, 5),
			files: []string{"p01.lst", "restart.txt"},
			body: "The restart logic worked during the descent. The alarms were **1202** and **1201**, both meaning the computer was overloaded and shedding low priority work.\n\n" +
				"It did exactly what it was designed to do.\n"},
		{from: "Newsletter", address: "news@toolkit.example", subject: "This week in toolkits", date: day(13, 7, 0),
			body: "### Highlights\n\n- A new table control lands in the trunk\n- The markdown renderer now draws tables\n- Two crashes fixed in the image loaders\n\n[Read the full issue](https://example.org/weekly)\n"},
		{from: "Katherine Johnson", address: "katherine@langley.example", subject: "Trajectory numbers checked", date: day(12, 15, 22),
			body: "I ran the numbers by hand as requested. They agree with the machine to five places.\n\nThe launch window holds.\n"},
	}}

	drafts := &folder{name: "Drafts", icon: "drafts", messages: []*mail{
		{from: "Me", address: "team@example.org", subject: "Release notes, draft", date: day(15, 7, 40),
			body: "Still missing the section about the new drivers.\n\n- [x] image loading\n- [ ] canvas scrollbars\n- [ ] known issues\n"},
	}}

	sent := &folder{name: "Sent", icon: "sent", messages: []*mail{
		{from: "Me", address: "ada@analytical.example", subject: "Re: Notes on the Analytical Engine", date: day(15, 9, 40),
			body: "Thank you, the notes are clear. I will fold note G into the introduction.\n"},
		{from: "Me", address: "ci@example.org", subject: "Re: Build #1198 failed", date: day(13, 9, 15),
			body: "The failure was a stale cache. Rebuilt with a clean tree and it passes.\n"},
	}}

	spam := &folder{name: "Spam", icon: "spam", messages: []*mail{
		{from: "Prince of Somewhere", address: "prince@offer.example", subject: "URGENT business proposal", date: day(11, 3, 12), unread: true,
			body: "Dear friend, I have **10,000,000** waiting for you. Kindly reply with your bank details.\n"},
	}}

	trash := &folder{name: "Trash", icon: "trash", messages: []*mail{
		{from: "Old Mailing List", address: "list@defunct.example", subject: "Meeting moved to Thursday", date: day(9, 12, 0),
			body: "The meeting is moved. Again.\n"},
	}}

	folders = []*folder{inbox, drafts, sent, spam, trash}
}
