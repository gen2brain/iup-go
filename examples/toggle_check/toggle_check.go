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

func skip(name, reason string) {
	skipped++
	logf("SKIP  %s (%s)", name, reason)
}

func counted(name string, t iup.Ihandle) iup.Ihandle {
	t.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		changes++
		fired = append(fired, "VALUECHANGED_CB on "+name)
		return iup.DEFAULT
	}))
	return t
}

func values(ts ...iup.Ihandle) string {
	var s []string
	for _, t := range ts {
		s = append(s, t.GetAttribute("VALUE"))
	}
	return strings.Join(s, " ")
}

func buildSteps() {
	steps = nil
	var dlg, plain, three, radio, r1, r2, r3, free iup.Ihandle
	driver := iup.GetGlobal("DRIVER")
	no3state := driver == "FLTK" || driver == "EFL"

	add(func() {
		changes, fired = 0, nil
		plain = counted("plain", iup.Toggle("Plain"))
		if !no3state {
			three = counted("3state", iup.Toggle("Three state").SetAttribute("3STATE", "YES"))
		} else {
			three = iup.Toggle("Three state")
		}
		r1 = counted("r1", iup.Toggle("One"))
		r2 = counted("r2", iup.Toggle("Two"))
		r3 = counted("r3", iup.Toggle("Three"))
		free = counted("ignoreradio", iup.Toggle("Free").SetAttribute("IGNORERADIO", "YES"))
		radio = iup.Radio(iup.Vbox(r1, r2, r3, free))
		dlg = iup.Dialog(iup.Vbox(plain, three, radio).SetAttributes(`NMARGIN=10x10, NGAP=6`)).SetAttribute("TITLE", "toggle_check")
		iup.Show(dlg)
	})

	equal("VALUE is OFF by default", &plain, "VALUE", "OFF")
	add(func() { plain.SetAttribute("VALUE", "ON") })
	equal("VALUE ON checks the toggle", &plain, "VALUE", "ON")
	add(func() { plain.SetAttribute("VALUE", "TOGGLE") })
	equal("VALUE TOGGLE inverts ON", &plain, "VALUE", "OFF")
	add(func() { plain.SetAttribute("VALUE", "TOGGLE") })
	equal("VALUE TOGGLE inverts OFF", &plain, "VALUE", "ON")
	add(func() { plain.SetAttribute("VALUE", "OFF") })
	equal("VALUE OFF unchecks the toggle", &plain, "VALUE", "OFF")

	if no3state {
		add(func() { skip("VALUE NOTDEF on a 3STATE toggle", "3STATE not supported in "+driver) })
	} else {
		add(func() { three.SetAttribute("VALUE", "NOTDEF") })
		equal("VALUE NOTDEF on a 3STATE toggle", &three, "VALUE", "NOTDEF")
		add(func() { three.SetAttribute("VALUE", "ON") })
		equal("VALUE ON on a 3STATE toggle", &three, "VALUE", "ON")
	}

	equal("RADIO is NO outside a radio", &plain, "RADIO", "NO")
	equal("RADIO is YES inside a radio", &r2, "RADIO", "YES")

	add(func() {
		check("the first toggle of a radio is ON after map", values(r1, r2, r3) == "ON OFF OFF", values(r1, r2, r3))
	})
	add(func() {
		name := iup.GetName(r1)
		check("radio VALUE is the name of the ON toggle", name != "" && radio.GetAttribute("VALUE") == name,
			fmt.Sprintf("VALUE=%q, name %q", radio.GetAttribute("VALUE"), name))
	})

	add(func() { r2.SetAttribute("VALUE", "ON") })
	add(func() {
		check("VALUE ON on a radio toggle turns the previous one OFF", values(r1, r2, r3) == "OFF ON OFF", values(r1, r2, r3))
	})
	add(func() {
		check("radio VALUE follows the toggle set ON", radio.GetAttribute("VALUE") == iup.GetName(r2),
			fmt.Sprintf("VALUE=%q, want %q", radio.GetAttribute("VALUE"), iup.GetName(r2)))
	})

	add(func() { radio.SetAttribute("VALUE_HANDLE", r3) })
	add(func() {
		check("radio VALUE_HANDLE selects the toggle", values(r1, r2, r3) == "OFF OFF ON", values(r1, r2, r3))
	})
	add(func() { radio.SetAttribute("VALUE", iup.GetName(r1)) })
	add(func() {
		check("radio VALUE selects the toggle by name", values(r1, r2, r3) == "ON OFF OFF", values(r1, r2, r3))
	})

	add(func() { r1.SetAttribute("VALUE", "OFF") })
	add(func() {
		check("a radio toggle cannot be set OFF", values(r1, r2, r3) == "ON OFF OFF", values(r1, r2, r3))
	})

	equal("IGNORERADIO toggle is not ON after map", &free, "VALUE", "OFF")
	add(func() { free.SetAttribute("VALUE", "ON") })
	add(func() {
		check("IGNORERADIO toggle turns ON without changing the radio", free.GetAttribute("VALUE") == "ON" && values(r1, r2, r3) == "ON OFF OFF",
			fmt.Sprintf("free=%s radio=%s", free.GetAttribute("VALUE"), values(r1, r2, r3)))
	})
	add(func() { free.SetAttribute("VALUE", "OFF") })
	equal("IGNORERADIO toggle can be set OFF", &free, "VALUE", "OFF")

	add(func() {
		check("setting VALUE does not call VALUECHANGED_CB", changes == 0, strings.Join(fired, ", "))
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
	).SetAttributes(`NMARGIN=10x10, NGAP=8`)).SetAttribute("TITLE", "Toggle checks")

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
