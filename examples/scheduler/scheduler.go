package main

import (
	"fmt"
	"sort"
	"time"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

const (
	firstHour = 8
	lastHour  = 19
	headerPx  = 28
	gutterPx  = 52
)

type event struct {
	day   int
	hour  int
	span  int
	title string
	kind  int
}

var (
	kinds = []struct {
		fill   string
		stroke string
		name   string
	}{
		{"170 205 240", "60 120 180", "Meeting"},
		{"200 230 195", "90 150 80", "Focus"},
		{"245 215 175", "190 130 60", "Errand"},
	}

	events  []*event
	week    time.Time
	picked  *event
	dragged *event
	dragDay int
	dragHr  int
	remind  = true
	fired   = map[*event]bool{}
)

func main() {
	iup.Open()
	defer iup.Close()
	iup.SetGlobal("UTF8MODE", "YES")

	week = monday(time.Now())
	seed()

	canvas := iup.Canvas().SetAttributes(`EXPAND=YES, BORDER=NO, BGCOLOR="255 255 255",
		SCROLLBAR=NO, RASTERSIZE=680x400`).SetHandle("sc_canvas")
	canvas.SetCallback("ACTION", iup.ActionFunc(redraw))
	canvas.SetCallback("BUTTON_CB", iup.ButtonFunc(pressed))
	canvas.SetCallback("MOTION_CB", iup.MotionFunc(moved))
	canvas.SetCallback("RESIZE_CB", iup.ResizeFunc(func(iup.Ihandle, int, int) int {
		return iup.DEFAULT
	}))

	cal := iup.Calendar().SetAttributes("VALUE=TODAY").SetHandle("sc_calendar")
	cal.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		if d, err := time.Parse("2006/1/2", iup.GetAttribute(ih, "VALUE")); err == nil {
			week = monday(d)
			refresh()
		}
		return iup.DEFAULT
	}))

	remindToggle := iup.Toggle("Remind me").SetAttributes("VALUE=ON")
	remindToggle.SetCallback("ACTION", iup.ToggleActionFunc(func(_ iup.Ihandle, state int) int {
		remind = state == 1
		return iup.DEFAULT
	}))

	side := iup.Vbox(
		cal,
		iup.Label("").SetAttribute("SEPARATOR", "HORIZONTAL"),
		legend(),
		iup.Label("").SetAttribute("SEPARATOR", "HORIZONTAL"),
		remindToggle,
		iup.Fill(),
	).SetAttributes("NMARGIN=8x8, NGAP=8")

	title := iup.Text().SetAttributes("EXPAND=HORIZONTAL, CUEBANNER=Title").SetHandle("sc_title")
	title.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		if picked != nil {
			picked.title = ih.GetAttribute("VALUE")
			refresh()
		}
		return iup.DEFAULT
	}))

	editor := iup.Popover(iup.Vbox(
		iup.Label("Appointment").SetAttributes("FONTSTYLE=Bold"),
		title,
		iup.Hbox(kindList(), button("Delete", remove)).SetAttributes("NGAP=6, ALIGNMENT=ACENTER"),
	).SetAttributes("NMARGIN=10x8, NGAP=6")).SetHandle("sc_editor")
	editor.SetAttributes("POSITION=BOTTOMLEFT, AUTOHIDE=YES, ARROW=YES")
	editor.SetAttributeHandle("ANCHOR", canvas)

	head := iup.Hbox(
		button("<", func() { week = week.AddDate(0, 0, -7); refresh() }),
		button("Today", func() { week = monday(time.Now()); refresh() }),
		button(">", func() { week = week.AddDate(0, 0, 7); refresh() }),
		iup.Label(rangeText()).SetHandle("sc_range").SetAttributes("PADDING=10x0, FONTSTYLE=Bold"),
		iup.Fill(),
		button("New", add),
	).SetAttributes("NMARGIN=8x6, NGAP=6, ALIGNMENT=ACENTER")

	status := iup.Label("").SetAttributes("EXPAND=HORIZONTAL, PADDING=8x4").SetHandle("sc_status")

	dlg := iup.Dialog(iup.Vbox(head,
		iup.Split(side, canvas).SetAttributes("ORIENTATION=VERTICAL"),
		status).SetAttributes("NGAP=2"))
	dlg.SetAttribute("TITLE", "Scheduler")

	iup.Show(dlg)
	refresh()

	timer := iup.Timer().SetAttributes("TIME=1000, RUN=YES")
	timer.SetCallback("ACTION_CB", iup.TimerActionFunc(tick))

	iup.MainLoop()
}

func seed() {
	events = []*event{
		{0, 9, 1, "Standup", 0},
		{0, 14, 2, "Design review", 0},
		{1, 10, 3, "Driver sweep", 1},
		{2, 9, 2, "Écrire les notes", 1},
		{2, 16, 1, "1:1", 0},
		{3, 11, 1, "Καφές", 2},
		{4, 13, 2, "Release notes", 1},
	}
}

func monday(t time.Time) time.Time {
	day := (int(t.Weekday()) + 6) % 7
	return time.Date(t.Year(), t.Month(), t.Day()-day, 0, 0, 0, 0, t.Location())
}

func legend() iup.Ihandle {
	box := iup.Vbox().SetAttributes("NGAP=4")
	for _, k := range kinds {
		iup.Append(box, iup.Hbox(
			iup.Label(" ").SetAttributes("RASTERSIZE=14x14").SetAttribute("BGCOLOR", k.fill),
			iup.Label(k.name),
		).SetAttributes("NGAP=6, ALIGNMENT=ACENTER"))
	}
	return box
}

func kindList() iup.Ihandle {
	list := iup.List().SetAttributes("DROPDOWN=YES, VISIBLEITEMS=3").SetHandle("sc_kind")
	for i, k := range kinds {
		iup.SetAttributeId(list, "", i+1, k.name)
	}
	list.SetAttribute("VALUE", "1")
	list.SetCallback("ACTION", iup.ListActionFunc(func(_ iup.Ihandle, _ string, item, state int) int {
		if state == 1 && picked != nil {
			picked.kind = item - 1
			refresh()
		}
		return iup.DEFAULT
	}))
	return list
}

func size(canvas iup.Ihandle) (int, int) {
	var w, h int
	fmt.Sscanf(canvas.GetAttribute("DRAWSIZE"), "%dx%d", &w, &h)
	return w, h
}

func slot(canvas iup.Ihandle, x, y int) (int, int, bool) {
	w, h := size(canvas)
	colw := (w - gutterPx) / 7
	rowh := (h - headerPx) / (lastHour - firstHour)
	if colw <= 0 || rowh <= 0 || x < gutterPx || y < headerPx {
		return 0, 0, false
	}
	day := (x - gutterPx) / colw
	hour := firstHour + (y-headerPx)/rowh
	if day > 6 || hour >= lastHour {
		return 0, 0, false
	}
	return day, hour, true
}

func at(day, hour int) *event {
	for _, e := range events {
		if e.day == day && hour >= e.hour && hour < e.hour+e.span {
			return e
		}
	}
	return nil
}

func pressed(ih iup.Ihandle, button, press, x, y int, status string) int {
	day, hour, ok := slot(ih, x, y)
	if !ok {
		return iup.DEFAULT
	}

	if button == iup.BUTTON3 {
		if press == 1 {
			picked = at(day, hour)
			refresh()
			context(day, hour)
		}
		return iup.DEFAULT
	}

	if button != iup.BUTTON1 {
		return iup.DEFAULT
	}

	if press == 1 {
		picked = at(day, hour)
		dragged = picked
		dragDay, dragHr = day, hour
		iup.GetHandle("sc_editor").SetAttribute("VISIBLE", "NO")

		/* the editor takes a pointer grab, so it waits for the double click and leaves dragging alone */
		if iup.IsDouble(status) {
			if picked == nil {
				picked = create(day, hour)
			}
			edit()
		}

		refresh()
		return iup.DEFAULT
	}

	dragged = nil
	refresh()
	return iup.DEFAULT
}

func context(day, hour int) {
	var menu iup.Ihandle
	if picked != nil {
		menu = iup.Menu(
			item("Edit "+picked.title, edit),
			iup.Submenu("Kind", kindMenu()),
			item("Duplicate", duplicate),
			iup.Separator(),
			item("Delete", remove),
		)
	} else {
		menu = iup.Menu(
			item(fmt.Sprintf("New at %s %02d:00", week.AddDate(0, 0, day).Format("Mon"), hour), func() {
				picked = create(day, hour)
				refresh()
				edit()
			}),
		)
	}

	iup.Popup(menu, iup.MOUSEPOS, iup.MOUSEPOS)
	iup.Destroy(menu)
}

func kindMenu() iup.Ihandle {
	menu := iup.Menu()
	for i, k := range kinds {
		kind := i
		iup.Append(menu, item(k.name, func() {
			if picked != nil {
				picked.kind = kind
				refresh()
			}
		}))
	}
	return menu
}

func create(day, hour int) *event {
	e := &event{day: day, hour: hour, span: 1, title: "New appointment"}
	clamp(e)
	events = append(events, e)
	return e
}

func duplicate() {
	if picked == nil {
		return
	}
	e := *picked
	e.hour += e.span
	clamp(&e)
	events = append(events, &e)
	picked = &e
	refresh()
}

func item(title string, action func()) iup.Ihandle {
	it := iup.MenuItem(title)
	it.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		action()
		return iup.DEFAULT
	}))
	return it
}

func moved(ih iup.Ihandle, x, y int, status string) int {
	if dragged == nil || !iup.IsButton1(status) {
		return iup.DEFAULT
	}

	day, hour, ok := slot(ih, x, y)
	if !ok || (day == dragDay && hour == dragHr) {
		return iup.DEFAULT
	}

	dragged.day += day - dragDay
	dragged.hour += hour - dragHr
	clamp(dragged)
	dragDay, dragHr = day, hour

	refresh()
	return iup.DEFAULT
}

func clamp(e *event) {
	if e.day < 0 {
		e.day = 0
	}
	if e.day > 6 {
		e.day = 6
	}
	if e.hour < firstHour {
		e.hour = firstHour
	}
	if e.hour+e.span > lastHour {
		e.hour = lastHour - e.span
	}
}

func add() {
	picked = create((int(time.Now().Weekday())+6)%7, 12)
	refresh()
	edit()
}

func remove() {
	if picked == nil {
		return
	}
	for i, e := range events {
		if e == picked {
			events = append(events[:i], events[i+1:]...)
			break
		}
	}
	picked = nil
	iup.GetHandle("sc_editor").SetAttribute("VISIBLE", "NO")
	refresh()
}

func edit() {
	if picked == nil {
		return
	}

	editor := iup.GetHandle("sc_editor")
	canvas := iup.GetHandle("sc_canvas")
	w, h := size(canvas)
	colw := (w - gutterPx) / 7
	rowh := (h - headerPx) / (lastHour - firstHour)

	iup.GetHandle("sc_title").SetAttribute("VALUE", picked.title)
	iup.GetHandle("sc_kind").SetAttribute("VALUE", picked.kind+1)
	editor.SetAttribute("OFFSETX", gutterPx+picked.day*colw)
	editor.SetAttribute("OFFSETY", headerPx+(picked.hour-firstHour+picked.span)*rowh-h)
	editor.SetAttribute("VISIBLE", "YES")
}

func tick(iup.Ihandle) int {
	if !remind {
		return iup.DEFAULT
	}

	now := time.Now()
	if !monday(now).Equal(week) {
		return iup.DEFAULT
	}

	day := (int(now.Weekday()) + 6) % 7
	for _, e := range events {
		if e.day == day && e.hour == now.Hour() && !fired[e] {
			fired[e] = true
			notify(e)
		}
	}
	return iup.DEFAULT
}

func notify(e *event) {
	n := iup.Notify()
	n.SetAttribute("TITLE", e.title)
	n.SetAttribute("BODY", fmt.Sprintf("%s at %02d:00", kinds[e.kind].name, e.hour))
	n.SetAttribute("SHOW", "YES")
}

func rangeText() string {
	return week.Format("2 Jan") + " - " + week.AddDate(0, 0, 6).Format("2 Jan 2006")
}

func refresh() {
	iup.GetHandle("sc_range").SetAttribute("TITLE", rangeText())

	count := 0
	for _, e := range events {
		count += e.span
	}
	state := "nothing selected"
	if picked != nil {
		state = fmt.Sprintf("%s, %s %02d:00", picked.title,
			week.AddDate(0, 0, picked.day).Format("Mon"), picked.hour)
	}
	iup.GetHandle("sc_status").SetAttribute("TITLE",
		fmt.Sprintf("%d appointments, %d hours  |  %s", len(events), count, state))

	iup.Redraw(iup.GetHandle("sc_canvas"), 0)
}

func redraw(ih iup.Ihandle) int {
	iup.DrawBegin(ih)
	defer iup.DrawEnd(ih)

	w, h := iup.DrawGetSize(ih)
	colw := (w - gutterPx) / 7
	rowh := (h - headerPx) / (lastHour - firstHour)
	if colw <= 0 || rowh <= 0 {
		return iup.DEFAULT
	}

	ih.SetAttribute("DRAWSTYLE", "FILL")
	ih.SetAttribute("DRAWCOLOR", "255 255 255")
	iup.DrawRectangle(ih, 0, 0, w-1, h-1)

	today := -1
	if monday(time.Now()).Equal(week) {
		today = (int(time.Now().Weekday()) + 6) % 7
	}

	for d := 0; d < 7; d++ {
		x := gutterPx + d*colw
		if d == today {
			ih.SetAttribute("DRAWCOLOR", "236 242 250")
			iup.DrawRectangle(ih, x, headerPx, x+colw-1, h-1)
		}
		ih.SetAttribute("DRAWCOLOR", "70 70 70")
		iup.DrawText(ih, week.AddDate(0, 0, d).Format("Mon 2"), x+6, 7, 0, 0)
	}

	ih.SetAttribute("DRAWSTYLE", "STROKE")
	ih.SetAttribute("DRAWCOLOR", "215 215 215")
	for hour := firstHour; hour <= lastHour; hour++ {
		y := headerPx + (hour-firstHour)*rowh
		iup.DrawLine(ih, 0, y, w-1, y)
		if hour < lastHour {
			ih.SetAttribute("DRAWCOLOR", "130 130 130")
			iup.DrawText(ih, fmt.Sprintf("%02d:00", hour), 6, y+4, 0, 0)
			ih.SetAttribute("DRAWCOLOR", "215 215 215")
		}
	}
	for d := 0; d <= 7; d++ {
		x := gutterPx + d*colw
		iup.DrawLine(ih, x, headerPx, x, h-1)
	}

	sort.SliceStable(events, func(i, j int) bool { return events[i].hour < events[j].hour })
	for _, e := range events {
		drawEvent(ih, e, colw, rowh)
	}

	return iup.DEFAULT
}

func drawEvent(ih iup.Ihandle, e *event, colw, rowh int) {
	x := gutterPx + e.day*colw + 3
	y := headerPx + (e.hour-firstHour)*rowh + 2
	x2 := x + colw - 7
	y2 := y + e.span*rowh - 5

	k := kinds[e.kind]
	ih.SetAttribute("DRAWSTYLE", "FILL")
	ih.SetAttribute("DRAWCOLOR", k.fill)
	iup.DrawRectangle(ih, x, y, x2, y2)

	ih.SetAttribute("DRAWSTYLE", "STROKE")
	ih.SetAttribute("DRAWCOLOR", k.stroke)
	iup.DrawRectangle(ih, x, y, x2, y2)

	if e == picked {
		iup.DrawFocusRect(ih, x+2, y+2, x2-2, y2-2)
	}

	iup.DrawSetClipRect(ih, x, y, x2, y2)
	ih.SetAttribute("DRAWCOLOR", "40 40 40")
	iup.DrawText(ih, e.title, x+5, y+3, 0, 0)
	if e.span > 1 {
		iup.DrawText(ih, fmt.Sprintf("%02d:00 - %02d:00", e.hour, e.hour+e.span), x+5, y+3+rowh/2, 0, 0)
	}
	iup.DrawResetClip(ih)
}

func button(title string, action func()) iup.Ihandle {
	b := iup.Button(title).SetAttributes("PADDING=8x3, CANFOCUS=NO")
	b.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		action()
		return iup.DEFAULT
	}))
	return b
}
