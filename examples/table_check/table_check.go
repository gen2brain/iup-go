package main

import (
	"fmt"
	"os"
	"strings"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

type step struct {
	run   func() bool
	tries int
}

var (
	logText  iup.Ihandle
	table    iup.Ihandle
	steps    []*step
	current  int
	pass     int
	fail     int
	failed   []string
	skipped  int
	runTimer iup.Ihandle
	auto     bool
	input    bool

	scrollCalls int
	scrollLast  string
	maxBgLin    int
	maxFgLin    int
)

func logf(format string, a ...any) {
	line := fmt.Sprintf(format, a...)
	logText.SetAttribute("APPEND", line)
	fmt.Fprintln(os.Stderr, line)
}

func check(name string, ok bool, detail string) {
	if ok {
		pass++
		logf("PASS  %s", name)
	} else {
		fail++
		failed = append(failed, name)
		logf("FAIL  %s: %s", name, detail)
	}
}

func skip(name, reason string) {
	skipped++
	logf("SKIP  %s (%s)", name, reason)
}

func add(f func()) {
	steps = append(steps, &step{run: func() bool { f(); return true }})
}

func poll(name string, tries int, cond func() (bool, string)) {
	steps = append(steps, &step{tries: tries, run: func() bool {
		ok, detail := cond()
		s := steps[current-1]
		s.tries--
		if ok || s.tries <= 0 {
			check(name, ok, detail)
			return true
		}
		return false
	}})
}

func pair(ih iup.Ihandle, name string, sep string) (int, int) {
	var a, b int
	fmt.Sscanf(strings.Replace(ih.GetAttribute(name), sep, " ", 1), "%d %d", &a, &b)
	return a, b
}

func origin(ih iup.Ihandle) (int, int) { return pair(ih, "ORIGIN", ":") }

func newTable(virtual bool) iup.Ihandle {
	lines := 1000
	if !virtual {
		lines = 100
	}
	t := iup.Table().SetAttributes(fmt.Sprintf("NUMCOL=12, NUMLIN=%d, VISIBLELINES=10, VISIBLECOLUMNS=4", lines))
	for c := 1; c <= 12; c++ {
		t.SetAttributeId("TITLE", c, fmt.Sprintf("Column %d", c))
		t.SetAttributeId("RASTERWIDTH", c, "90")
	}
	if virtual {
		t.SetAttribute("VIRTUALMODE", "YES")
		t.SetCallback("VALUE_CB", iup.TableValueFunc(func(ih iup.Ihandle, lin, col int) string {
			return fmt.Sprintf("r%d c%d", lin, col)
		}))
	} else {
		for l := 1; l <= lines; l++ {
			t.SetAttributeId2("", l, 1, fmt.Sprintf("r%d", l))
		}
	}
	return t
}

func buildSteps() {
	steps = nil
	var dlg, deferred iup.Ihandle

	add(func() {
		table = newTable(true)
		table.SetCallback("SCROLLTOP_CB", iup.ScrollTopFunc(func(ih iup.Ihandle, lin, col int) int {
			scrollCalls++
			scrollLast = fmt.Sprintf("%d:%d", lin, col)
			return iup.DEFAULT
		}))
		table.SetCallback("BGCOLOR_CB", iup.BgColorFunc(func(ih iup.Ihandle, lin, col int) (int, int, int, int) {
			maxBgLin = max(maxBgLin, lin)
			return 0, 0, 0, iup.IGNORE
		}))
		table.SetCallback("FGCOLOR_CB", iup.FgColorFunc(func(ih iup.Ihandle, lin, col int) (int, int, int, int) {
			maxFgLin = max(maxFgLin, lin)
			return 0, 0, 0, iup.IGNORE
		}))
		dlg = iup.Dialog(table).SetAttribute("TITLE", "table_check")
		iup.Show(dlg)
	})

	poll("ORIGIN starts at 1:1", 10, func() (bool, string) {
		o := table.GetAttribute("ORIGIN")
		return o == "1:1", "ORIGIN=" + o
	})

	add(func() { scrollCalls = 0; table.SetAttribute("ORIGIN", "500:*") })
	poll("ORIGIN 500:* keeps the column", 10, func() (bool, string) {
		o := table.GetAttribute("ORIGIN")
		return o == "500:1", "ORIGIN=" + o
	})

	add(func() { table.SetAttribute("ORIGIN", "200:6") })
	poll("ORIGIN 200:6 scrolls both ways", 10, func() (bool, string) {
		o := table.GetAttribute("ORIGIN")
		return o == "200:6", "ORIGIN=" + o
	})

	add(func() { table.SetAttribute("ORIGIN", "*:1") })
	poll("ORIGIN *:1 keeps the line", 10, func() (bool, string) {
		o := table.GetAttribute("ORIGIN")
		return o == "200:1", "ORIGIN=" + o
	})

	add(func() { maxBgLin, maxFgLin = 0, 0; table.SetAttribute("SHOW", "800:1") })
	poll("SHOW 800:1 makes line 800 visible", 10, func() (bool, string) {
		lin, _ := origin(table)
		return lin <= 800 && lin > 800-20, fmt.Sprintf("ORIGIN=%s", table.GetAttribute("ORIGIN"))
	})
	add(func() {
		check("ORIGIN and SHOW do not call SCROLLTOP_CB", scrollCalls == 0, fmt.Sprintf("%d calls, last %s", scrollCalls, scrollLast))
	})

	poll("BGCOLOR_CB and FGCOLOR_CB are called for the displayed lines", 10, func() (bool, string) {
		return maxBgLin >= 790 && maxFgLin >= 790, fmt.Sprintf("highest BGCOLOR_CB line %d, FGCOLOR_CB line %d", maxBgLin, maxFgLin)
	})

	add(func() { scrollCalls = 0; table.SetAttribute("FOCUSCELL", "100:1") })
	poll("FOCUSCELL scrolling calls SCROLLTOP_CB with the new ORIGIN", 10, func() (bool, string) {
		o := table.GetAttribute("ORIGIN")
		lin, _ := origin(table)
		ok := scrollCalls > 0 && scrollLast == o && lin <= 100 && lin > 100-20
		return ok, fmt.Sprintf("%d calls, last %s, ORIGIN=%s", scrollCalls, scrollLast, o)
	})

	add(func() {
		tx, ty := pair(table, "SCREENPOSITION", ",")
		dx, dy := pair(dlg, "SCREENPOSITION", ",")
		dw, dh := pair(dlg, "RASTERSIZE", "x")
		ok := tx >= dx && ty >= dy && tx < dx+dw && ty < dy+dh
		check("SCREENPOSITION of a scrolled table is inside its dialog", ok,
			fmt.Sprintf("table %d,%d, dialog %d,%d %dx%d", tx, ty, dx, dy, dw, dh))
	})

	add(func() {
		deferred = newTable(false)
		deferred.SetAttribute("ORIGIN", "40:3")
		d := iup.Dialog(deferred).SetAttribute("TITLE", "table_check deferred")
		iup.Show(d)
	})
	poll("ORIGIN set before map is applied after layout", 10, func() (bool, string) {
		o := deferred.GetAttribute("ORIGIN")
		return o == "40:3", "ORIGIN=" + o
	})
	add(func() {
		deferred.SetAttributeId2("BGCOLOR", 10, 2, "255 0 0")
		deferred.SetAttributeId2("BGCOLOR", 60, 2, "0 255 0")
		deferred.SetAttributeId2("FGCOLOR", 90, 0, "0 0 255")
		deferred.SetAttributeId2("FONT", 100, 1, "Helvetica, Bold 12")
		deferred.SetAttribute("NUMLIN", "99")
		deferred.SetAttribute("NUMLIN", "50")
		deferred.SetAttribute("NUMLIN", "100")
		got := fmt.Sprintf("10:2=%q 60:2=%q 90:0=%q 100:1=%q", deferred.GetAttribute("BGCOLOR", 10, 2), deferred.GetAttribute("BGCOLOR", 60, 2),
			deferred.GetAttribute("FGCOLOR", 90, 0), deferred.GetAttribute("FONT", 100, 1))
		check("NUMLIN shrink drops the attributes of the removed lines only", got == `10:2="255 0 0" 60:2="" 90:0="" 100:1=""`, got)
	})
	add(func() {
		deferred.SetAttributeId2("BGCOLOR", 0, 3, "0 255 0")
		deferred.SetAttributeId2("BGCOLOR", 0, 5, "1 2 3")
		deferred.SetAttributeId2("FGCOLOR", 20, 0, "0 0 255")
		deferred.SetAttribute("ADDLIN", "5")
		deferred.SetAttribute("ADDLIN", "5")
		deferred.SetAttribute("DELLIN", "15")
		deferred.SetAttribute("ADDCOL", "1")
		deferred.SetAttribute("DELCOL", "4")
		get := func(name string, lin, col int) string {
			return fmt.Sprintf("%s%d:%d=%q", name, lin, col, deferred.GetAttribute(name, lin, col))
		}
		got := strings.Join([]string{get("BGCOLOR", 12, 3), get("BGCOLOR", 10, 2), get("FGCOLOR", 21, 0), get("FGCOLOR", 20, 0),
			get("BGCOLOR", 0, 5), get("BGCOLOR", 0, 4), get("BGCOLOR", 0, 3)}, " ")
		check("ADDLIN, DELLIN, ADDCOL and DELCOL move the line and column attributes",
			got == `BGCOLOR12:3="255 0 0" BGCOLOR10:2="" FGCOLOR21:0="0 0 255" FGCOLOR20:0="" BGCOLOR0:5="1 2 3" BGCOLOR0:4="" BGCOLOR0:3=""`, got)
	})
	add(func() { iup.GetDialog(deferred).Destroy() })

	if input {
		add(func() {
			scrollCalls = 0
			iup.SetFocus(table)
			table.SetAttribute("FOCUSCELL", table.GetAttribute("ORIGIN"))
			scrollCalls = 0
			logf("READY input %s %s", table.GetAttribute("SCREENPOSITION"), table.GetAttribute("RASTERSIZE"))
		})
		poll("user scrolling calls SCROLLTOP_CB with the new ORIGIN", 40, func() (bool, string) {
			o := table.GetAttribute("ORIGIN")
			return scrollCalls > 0 && scrollLast == o, fmt.Sprintf("%d calls, last %s, ORIGIN=%s", scrollCalls, scrollLast, o)
		})
	} else {
		add(func() { skip("user scrolling calls SCROLLTOP_CB", "run with -input and scroll the table") })
	}

	add(func() {
		dlg.Destroy()
		logf("Done: %d passed, %d failed, %d skipped", pass, fail, skipped)
		if fail > 0 {
			logf("Failed: %s", strings.Join(failed, "; "))
		}
		runTimer.SetAttribute("RUN", "NO")
		if auto {
			iup.ExitLoop()
		}
	})
}

func run() {
	if runTimer.GetBool("RUN") {
		return
	}
	pass, fail, skipped, current = 0, 0, 0, 0
	failed = nil
	logText.SetAttribute("VALUE", "")
	logf("Driver %s, windowing %s", iup.GetGlobal("DRIVER"), iup.GetGlobal("WINDOWING"))
	buildSteps()
	runTimer.SetAttribute("RUN", "YES")
}

func main() {
	iup.Open()
	defer iup.Close()

	for _, a := range os.Args[1:] {
		switch a {
		case "-run":
			auto = true
		case "-input":
			input = true
		}
	}

	logText = iup.Text().SetAttributes(`MULTILINE=YES, READONLY=YES, WORDWRAP=YES, VISIBLECOLUMNS=80, VISIBLELINES=20, EXPAND=YES`)

	runBtn := iup.Button("Run checks").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { run(); return iup.DEFAULT }))
	copyBtn := iup.Button("Copy log").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		clip := iup.Clipboard()
		clip.SetAttribute("TEXT", logText.GetAttribute("VALUE"))
		clip.Destroy()
		return iup.DEFAULT
	}))

	dlg := iup.Dialog(iup.Vbox(
		iup.Hbox(runBtn, copyBtn).SetAttributes(`NGAP=6`),
		logText,
	).SetAttributes(`NMARGIN=10x10, NGAP=8`)).SetAttribute("TITLE", "Table checks")

	runTimer = iup.Timer().SetAttribute("TIME", "500")
	busy := false
	runTimer.SetCallback("ACTION_CB", iup.TimerActionFunc(func(ih iup.Ihandle) int {
		if busy || current >= len(steps) {
			return iup.DEFAULT
		}
		busy = true
		s := steps[current]
		current++
		if !s.run() {
			current--
		}
		busy = false
		return iup.DEFAULT
	}))

	iup.Show(dlg)
	if auto {
		run()
	}
	iup.MainLoop()

	if d := iup.GetGlobal("DRIVER"); d != "Android" && d != "CocoaTouch" {
		runTimer.Destroy()
	}

	if auto && fail > 0 {
		iup.Close()
		os.Exit(1)
	}
}
