package main

import (
	"fmt"
	"math"
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

func value(name string, ih *iup.Ihandle, want float64) {
	add(func() {
		got := (*ih).GetDouble("VALUE")
		check(name, math.Abs(got-want) < 0.001*math.Max(1, math.Abs(want)), fmt.Sprintf("VALUE=%s, want %g", (*ih).GetAttribute("VALUE"), want))
	})
}

func number(name string, ih *iup.Ihandle, attr string, want float64) {
	add(func() {
		got := (*ih).GetDouble(attr)
		check(name, got == want, fmt.Sprintf("%s=%s, want %g", attr, (*ih).GetAttribute(attr), want))
	})
}

func buildSteps() {
	steps = nil
	var dlg, hval, vval, bar iup.Ihandle

	add(func() {
		changes, fired = 0, nil
		hval = iup.Val("HORIZONTAL")
		vval = iup.Val("VERTICAL")
		for _, v := range []iup.Ihandle{hval, vval} {
			v.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
				changes++
				fired = append(fired, "VALUECHANGED_CB on "+ih.GetAttribute("ORIENTATION"))
				return iup.DEFAULT
			}))
		}
		bar = iup.ProgressBar()
		dlg = iup.Dialog(iup.Vbox(hval, vval, bar).SetAttributes(`NMARGIN=10x10, NGAP=6`)).SetAttribute("TITLE", "val_check")
		iup.Show(dlg)
	})

	number("Val MIN is 0 by default", &hval, "MIN", 0)
	number("Val MAX is 1 by default", &hval, "MAX", 1)
	value("Val VALUE is 0 by default", &hval, 0)
	add(func() { hval.SetAttribute("VALUE", "0.25") })
	value("Val VALUE keeps a fraction", &hval, 0.25)

	add(func() {
		hval.SetAttribute("MIN", "10")
		hval.SetAttribute("MAX", "20")
		hval.SetAttribute("VALUE", "15")
	})
	number("Val MIN after set", &hval, "MIN", 10)
	number("Val MAX after set", &hval, "MAX", 20)
	value("Val VALUE inside MIN and MAX", &hval, 15)
	add(func() { hval.SetAttribute("VALUE", "25") })
	value("Val VALUE above MAX is MAX", &hval, 20)
	add(func() { hval.SetAttribute("VALUE", "5") })
	value("Val VALUE below MIN is MIN", &hval, 10)
	add(func() { hval.SetAttribute("VALUE", "18"); hval.SetAttribute("MAX", "16") })
	value("Val MAX below VALUE moves VALUE to MAX", &hval, 16)
	add(func() { hval.SetAttribute("VALUE", "12"); hval.SetAttribute("MIN", "14") })
	value("Val MIN above VALUE moves VALUE to MIN", &hval, 14)

	add(func() {
		vval.SetAttribute("MIN", "-50")
		vval.SetAttribute("MAX", "50")
		vval.SetAttribute("VALUE", "-20")
	})
	value("vertical Val VALUE with a negative range", &vval, -20)

	add(func() {
		check("setting VALUE, MIN or MAX does not call VALUECHANGED_CB", changes == 0, strings.Join(fired, ", "))
	})

	number("ProgressBar MIN is 0 by default", &bar, "MIN", 0)
	number("ProgressBar MAX is 1 by default", &bar, "MAX", 1)
	value("ProgressBar VALUE is 0 by default", &bar, 0)
	add(func() { bar.SetAttribute("MAX", "100"); bar.SetAttribute("VALUE", "42") })
	value("ProgressBar VALUE inside MIN and MAX", &bar, 42)
	add(func() { bar.SetAttribute("VALUE", "150") })
	value("ProgressBar VALUE above MAX is MAX", &bar, 100)
	add(func() { bar.SetAttribute("VALUE", "-5") })
	value("ProgressBar VALUE below MIN is MIN", &bar, 0)
	add(func() { bar.SetAttribute("VALUE", "80"); bar.SetAttribute("MAX", "60") })
	value("ProgressBar MAX below VALUE moves VALUE to MAX", &bar, 60)
	add(func() { bar.SetAttribute("VALUE", "20"); bar.SetAttribute("MIN", "30") })
	value("ProgressBar MIN above VALUE moves VALUE to MIN", &bar, 30)

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
	).SetAttributes(`NMARGIN=10x10, NGAP=8`)).SetAttribute("TITLE", "Val checks")

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
