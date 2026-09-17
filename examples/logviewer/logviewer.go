package main

import (
	"fmt"
	"math/rand"
	"strings"
	"time"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

const total = 200000

type record struct {
	when    time.Time
	level   string
	source  string
	message string
}

var (
	lines    []record
	matches  []int
	query    string
	level    string
	follow   = true
	levels   = []string{"DEBUG", "INFO", "WARN", "ERROR"}
	sources  = []string{"auth", "cache", "db", "http", "scheduler", "worker"}
	messages = []string{
		"connection established", "request completed in %dms", "cache miss for key %d",
		"retrying after timeout", "user session expired", "queue depth %d",
		"checksum mismatch on block %d", "reloaded configuration", "rate limit reached",
		"dropped %d stale entries",
	}
)

func main() {
	iup.Open()
	defer iup.Close()
	iup.SetGlobal("UTF8MODE", "YES")

	generate()

	table := iup.Table().SetAttributes(`EXPAND=YES, NUMCOL=5, VIRTUALMODE=YES, USERRESIZE=YES,
		STRETCHLAST=YES, ALTERNATECOLOR=YES, SHOWGRID=NO, FOCUSRECT=NO`).SetHandle("log_table")
	table.SetAttributes(map[string]string{
		"TITLE1": "#", "TITLE2": "Time", "TITLE3": "Level", "TITLE4": "Source", "TITLE5": "Message",
		"ALIGNMENT1": "ARIGHT", "RASTERWIDTH1": "70", "RASTERWIDTH2": "180",
		"RASTERWIDTH3": "70", "RASTERWIDTH4": "90",
	})
	table.SetCallback("VALUE_CB", iup.TableValueFunc(cellValue))
	table.SetCallback("CLICK_CB", iup.ClickFunc(clicked))

	search := iup.Text().SetAttributes("EXPAND=HORIZONTAL, CUEBANNER=Filter").SetHandle("log_search")
	search.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		query = strings.ToLower(ih.GetAttribute("VALUE"))
		apply()
		return iup.DEFAULT
	}))

	levelList := iup.List().SetAttributes("DROPDOWN=YES, VISIBLEITEMS=5").SetHandle("log_level")
	for i, l := range append([]string{"All levels"}, levels...) {
		iup.SetAttributeId(levelList, "", i+1, l)
	}
	levelList.SetAttribute("VALUE", "1")
	levelList.SetCallback("ACTION", iup.ListActionFunc(func(_ iup.Ihandle, text string, _, selected int) int {
		if selected == 1 {
			level = ""
			if text != "All levels" {
				level = text
			}
			apply()
		}
		return iup.DEFAULT
	}))

	followToggle := iup.Toggle("Follow tail").SetAttributes("VALUE=ON").SetHandle("log_follow")
	followToggle.SetCallback("ACTION", iup.ToggleActionFunc(func(_ iup.Ihandle, state int) int {
		follow = state == 1
		if follow {
			tail()
		}
		return iup.DEFAULT
	}))

	detail := iup.Text().SetAttributes(`MULTILINE=YES, EXPAND=HORIZONTAL, READONLY=YES,
		VISIBLELINES=3, WORDWRAP=YES`).SetHandle("log_detail")

	dlg := iup.Dialog(iup.Vbox(
		iup.Hbox(search, levelList, followToggle, button("Clear", clear)).
			SetAttributes("NMARGIN=8x6, NGAP=6, ALIGNMENT=ACENTER"),
		table,
		detail,
		iup.Hbox(
			iup.Label("").SetAttributes("EXPAND=HORIZONTAL").SetHandle("log_status"),
			iup.Label("").SetHandle("log_rate"),
		).SetAttributes("NMARGIN=8x4, NGAP=8"),
	).SetAttributes("NGAP=4")).SetHandle("log_dlg")
	dlg.SetAttribute("TITLE", "Log viewer")

	apply()
	iup.Show(dlg)

	timer := iup.Timer().SetAttributes("TIME=600, RUN=YES")
	timer.SetCallback("ACTION_CB", iup.TimerActionFunc(arrive))
	iup.MainLoop()
}

func generate() {
	r := rand.New(rand.NewSource(1))
	start := time.Now().Add(-time.Duration(total) * 200 * time.Millisecond)
	lines = make([]record, 0, total)
	for i := 0; i < total; i++ {
		lines = append(lines, entry(r, start.Add(time.Duration(i)*200*time.Millisecond)))
	}
}

func entry(r *rand.Rand, when time.Time) record {
	lvl := levels[weighted(r)]
	msg := messages[r.Intn(len(messages))]
	if strings.Contains(msg, "%d") {
		msg = fmt.Sprintf(msg, r.Intn(9000)+100)
	}
	return record{when, lvl, sources[r.Intn(len(sources))], msg}
}

func weighted(r *rand.Rand) int {
	switch n := r.Intn(100); {
	case n < 45:
		return 1
	case n < 70:
		return 0
	case n < 92:
		return 2
	}
	return 3
}

func apply() {
	matches = matches[:0]
	for i, l := range lines {
		if keep(l) {
			matches = append(matches, i)
		}
	}
	refresh()
}

func keep(l record) bool {
	if level != "" && l.level != level {
		return false
	}
	if query == "" {
		return true
	}
	return strings.Contains(strings.ToLower(l.source+" "+l.message+" "+l.level), query)
}

func refresh() {
	table := iup.GetHandle("log_table")
	table.SetAttribute("NUMLIN", len(matches))
	table.SetAttribute("REDRAW", "YES")
	if follow {
		tail()
	}
	updateStatus()
}

func cellValue(ih iup.Ihandle, lin, col int) string {
	if lin < 1 || lin > len(matches) {
		return ""
	}
	l := lines[matches[lin-1]]
	switch col {
	case 1:
		return fmt.Sprint(matches[lin-1] + 1)
	case 2:
		return l.when.Format("2006-01-02 15:04:05.000")
	case 3:
		return l.level
	case 4:
		return l.source
	}
	return l.message
}

func clicked(ih iup.Ihandle, lin, col int, status string) int {
	if lin < 1 || lin > len(matches) {
		return iup.DEFAULT
	}
	l := lines[matches[lin-1]]
	iup.GetHandle("log_detail").SetAttribute("VALUE", fmt.Sprintf("%s  %s  [%s]\n%s",
		l.when.Format("2006-01-02 15:04:05.000"), l.level, l.source, l.message))
	return iup.DEFAULT
}

func arrive(iup.Ihandle) int {
	r := rand.New(rand.NewSource(time.Now().UnixNano()))
	for i := 0; i < 5+r.Intn(20); i++ {
		l := entry(r, time.Now())
		lines = append(lines, l)
		if keep(l) {
			matches = append(matches, len(lines)-1)
		}
	}
	refresh()
	return iup.DEFAULT
}

func tail() {
	if len(matches) == 0 {
		return
	}
	iup.GetHandle("log_table").SetAttribute("SHOW", fmt.Sprintf("%d:1", len(matches)))
}

func clear() {
	iup.GetHandle("log_search").SetAttribute("VALUE", "")
	iup.GetHandle("log_level").SetAttribute("VALUE", "1")
	query, level = "", ""
	apply()
}

func updateStatus() {
	counts := map[string]int{}
	for _, i := range matches {
		counts[lines[i].level]++
	}
	iup.GetHandle("log_status").SetAttribute("TITLE", fmt.Sprintf(
		"%d of %d lines   DEBUG %d   INFO %d   WARN %d   ERROR %d",
		len(matches), len(lines), counts["DEBUG"], counts["INFO"], counts["WARN"], counts["ERROR"]))
	iup.GetHandle("log_rate").SetAttribute("TITLE", fmt.Sprintf("%d lines held", len(lines)))
}

func button(title string, action func()) iup.Ihandle {
	b := iup.Button(title)
	b.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		action()
		return iup.DEFAULT
	}))
	return b
}
