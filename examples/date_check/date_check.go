package main

import (
	"fmt"
	"os"
	"strings"
	"time"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

type step struct {
	run   func() bool
	tries int
}

var (
	logText  iup.Ihandle
	steps    []*step
	current  int
	pass     int
	fail     int
	failed   []string
	skipped  int
	runTimer iup.Ihandle
	auto     bool

	actions int
	changes int
	fired   []string
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

func equal(name string, ih *iup.Ihandle, attr, want string) {
	add(func() {
		got := (*ih).GetAttribute(attr)
		check(name, got == want, fmt.Sprintf("%s=%q, want %q", attr, got, want))
	})
}

func today() string {
	t := time.Now()
	return fmt.Sprintf("%d/%d/%d", t.Year(), int(t.Month()), t.Day())
}

func dateChecks(kind string, ih *iup.Ihandle) {
	add(func() {
		check(kind+" TODAY is today as year/month/day", (*ih).GetAttribute("TODAY") == today(),
			fmt.Sprintf("TODAY=%q, want %q", (*ih).GetAttribute("TODAY"), today()))
	})
	add(func() {
		check(kind+" VALUE is today by default", (*ih).GetAttribute("VALUE") == today(),
			fmt.Sprintf("VALUE=%q, want %q", (*ih).GetAttribute("VALUE"), today()))
	})
	add(func() { (*ih).SetAttribute("VALUE", "2024/2/29") })
	equal(kind+" VALUE round trips without leading zeros", ih, "VALUE", "2024/2/29")
	add(func() { (*ih).SetAttribute("VALUE", "2023/12/31") })
	equal(kind+" VALUE crosses a year", ih, "VALUE", "2023/12/31")
	add(func() { (*ih).SetAttribute("VALUE", "TODAY") })
	add(func() {
		check(kind+" VALUE TODAY selects today", (*ih).GetAttribute("VALUE") == today(),
			fmt.Sprintf("VALUE=%q, want %q", (*ih).GetAttribute("VALUE"), today()))
	})

	add(func() {
		(*ih).SetAttribute("MINDATE", "2024/3/10")
		(*ih).SetAttribute("MAXDATE", "2024/3/20")
	})
	equal(kind+" MAXDATE moves a later current date to MAXDATE", ih, "VALUE", "2024/3/20")
	add(func() { (*ih).SetAttribute("VALUE", "2024/3/15") })
	equal(kind+" VALUE inside MINDATE and MAXDATE", ih, "VALUE", "2024/3/15")
	add(func() { (*ih).SetAttribute("VALUE", "2024/3/5") })
	equal(kind+" VALUE before MINDATE is MINDATE", ih, "VALUE", "2024/3/10")
	add(func() { (*ih).SetAttribute("VALUE", "2024/4/1") })
	equal(kind+" VALUE after MAXDATE is MAXDATE", ih, "VALUE", "2024/3/20")
	add(func() { (*ih).SetAttribute("VALUE", "2024/3/12"); (*ih).SetAttribute("MINDATE", "2024/3/14") })
	equal(kind+" MINDATE after the current date moves it to MINDATE", ih, "VALUE", "2024/3/14")
	equal(kind+" MINDATE returns the value set", ih, "MINDATE", "2024/3/14")
	add(func() {
		(*ih).SetAttribute("MINDATE", nil)
		(*ih).SetAttribute("MAXDATE", nil)
		(*ih).SetAttribute("VALUE", "2020/1/1")
	})
	equal(kind+" clearing MINDATE and MAXDATE removes the limits", ih, "VALUE", "2020/1/1")
}

func buildSteps() {
	steps = nil
	var dlg, cal, pick iup.Ihandle

	add(func() {
		changes, fired = 0, nil
		cal = iup.Calendar()
		pick = iup.DatePick()
		for _, d := range []iup.Ihandle{cal, pick} {
			d.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
				changes++
				fired = append(fired, "VALUECHANGED_CB on "+iup.GetClassName(ih))
				return iup.DEFAULT
			}))
		}
		dlg = iup.Dialog(iup.Vbox(cal, pick).SetAttributes(`NMARGIN=10x10, NGAP=6`)).SetAttribute("TITLE", "date_check")
		iup.Show(dlg)
	})

	dateChecks("Calendar", &cal)
	dateChecks("DatePick", &pick)

	add(func() {
		check("setting VALUE, MINDATE or MAXDATE does not call VALUECHANGED_CB", changes == 0, strings.Join(fired, ", "))
	})

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

	auto = len(os.Args) > 1 && os.Args[1] == "-run"

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
	).SetAttributes(`NMARGIN=10x10, NGAP=8`)).SetAttribute("TITLE", "Date checks")

	runTimer = iup.Timer().SetAttribute("TIME", "300")
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
