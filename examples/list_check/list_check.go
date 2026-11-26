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

func sizeOf(v string) (int, int) {
	var w, h int
	fmt.Sscanf(strings.Replace(v, "x", " ", 1), "%d %d", &w, &h)
	return w, h
}

func counted(name string, l iup.Ihandle) iup.Ihandle {
	l.SetCallback("ACTION", iup.ListActionFunc(func(ih iup.Ihandle, text string, item, state int) int {
		actions++
		fired = append(fired, "ACTION on "+name)
		return iup.DEFAULT
	}))
	l.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		changes++
		fired = append(fired, "VALUECHANGED_CB on "+name)
		return iup.DEFAULT
	}))
	return l
}

func buildSteps() {
	steps = nil
	var dlg, plain, drop, multi, edit, sorted, virtual iup.Ihandle

	add(func() {
		actions, changes, fired = 0, 0, nil
		plain = counted("plain", iup.List().SetAttributes(`1=Alpha, 2=Beta, 3=Gamma, VISIBLELINES=4`))
		drop = counted("dropdown", iup.List().SetAttributes(`DROPDOWN=YES, 1=One, 2=Two, 3=Three`))
		multi = counted("multiple", iup.List().SetAttributes(`MULTIPLE=YES, 1=A, 2=B, 3=C, VISIBLELINES=3`))
		edit = counted("editbox", iup.List().SetAttributes(`EDITBOX=YES, DROPDOWN=YES, 1=First, 2=Second`))
		sorted = counted("sorted", iup.List().SetAttributes(`SORT=YES, 1=c, 2=a, 3=b, VISIBLELINES=3`))
		virtual = counted("virtual", iup.List().SetAttributes(`VIRTUALMODE=YES, VISIBLELINES=4`))
		virtual.SetCallback("VALUE_CB", iup.ListValueFunc(func(ih iup.Ihandle, pos int) string {
			return fmt.Sprintf("Item %d", pos)
		}))
		dlg = iup.Dialog(iup.Vbox(plain, drop, multi, edit, sorted, virtual).SetAttributes(`NMARGIN=10x10, NGAP=6`)).SetAttribute("TITLE", "list_check")
		iup.Show(dlg)
		virtual.SetAttribute("ITEMCOUNT", "1000")
	})

	equal("COUNT counts the items set before map", &plain, "COUNT", "3")
	equal("item text is returned by its id", &plain, "2", "Beta")

	add(func() { plain.SetAttribute("APPENDITEM", "Delta") })
	add(func() {
		check("APPENDITEM adds after the last item", plain.GetAttribute("COUNT") == "4" && plain.GetAttribute("4") == "Delta",
			fmt.Sprintf("COUNT=%s 4=%s", plain.GetAttribute("COUNT"), plain.GetAttribute("4")))
	})

	add(func() { plain.SetAttribute("INSERTITEM1", "Zero") })
	add(func() {
		check("INSERTITEM1 inserts before the first item", plain.GetAttribute("COUNT") == "5" && plain.GetAttribute("1") == "Zero" && plain.GetAttribute("2") == "Alpha",
			fmt.Sprintf("COUNT=%s 1=%s 2=%s", plain.GetAttribute("COUNT"), plain.GetAttribute("1"), plain.GetAttribute("2")))
	})

	add(func() { plain.SetAttribute("REMOVEITEM", "1") })
	add(func() {
		check("REMOVEITEM removes the given item", plain.GetAttribute("COUNT") == "4" && plain.GetAttribute("1") == "Alpha",
			fmt.Sprintf("COUNT=%s 1=%s", plain.GetAttribute("COUNT"), plain.GetAttribute("1")))
	})

	add(func() { plain.SetAttribute("VALUE", "2") })
	equal("VALUE selects an item", &plain, "VALUE", "2")

	add(func() { plain.SetAttribute("VALUESTRING", "Gamma") })
	add(func() {
		check("VALUESTRING selects the item with that text", plain.GetAttribute("VALUE") == "3" && plain.GetAttribute("VALUESTRING") == "Gamma",
			fmt.Sprintf("VALUE=%s VALUESTRING=%s", plain.GetAttribute("VALUE"), plain.GetAttribute("VALUESTRING")))
	})

	add(func() { drop.SetAttribute("VALUE", "3") })
	equal("VALUE selects an item in a dropdown", &drop, "VALUE", "3")

	add(func() { multi.SetAttribute("VALUE", "+-+") })
	equal("VALUE sets the selection of a multiple list", &multi, "VALUE", "+-+")

	add(func() { edit.SetAttribute("VALUE", "hello") })
	equal("VALUE sets the text of an editbox", &edit, "VALUE", "hello")

	add(func() {
		got := sorted.GetAttribute("1") + sorted.GetAttribute("2") + sorted.GetAttribute("3")
		check("SORT orders the items", got == "abc", "items "+got)
	})

	equal("COUNT returns ITEMCOUNT in virtual mode", &virtual, "COUNT", "1000")
	equal("item text comes from VALUE_CB in virtual mode", &virtual, "5", "Item 5")
	add(func() {
		var natural [2]string
		for i, count := range []string{"1000", "5"} {
			l := iup.List().SetAttributes("VIRTUALMODE=YES, ITEMCOUNT=" + count)
			d := iup.Dialog(l)
			iup.Map(d)
			natural[i] = l.GetAttribute("NATURALSIZE")
			d.Destroy()
		}
		_, h0 := sizeOf(natural[0])
		_, h5 := sizeOf(natural[1])
		check("a virtual list without VISIBLELINES is 5 lines high", h0 == h5 && h5 > 0,
			fmt.Sprintf("NATURALSIZE with 1000 items %s, with 5 items %s", natural[0], natural[1]))
	})
	add(func() {
		virtual.SetAttribute("APPENDITEM", "x")
		virtual.SetAttribute("INSERTITEM2", "x")
		virtual.SetAttribute("REMOVEITEM", "3")
		virtual.SetAttribute("REMOVEITEM", "ALL")
	})
	add(func() {
		check("APPENDITEM, INSERTITEM and REMOVEITEM are ignored in virtual mode",
			virtual.GetAttribute("COUNT") == "1000" && virtual.GetAttribute("3") == "Item 3",
			fmt.Sprintf("COUNT=%s, 3=%s", virtual.GetAttribute("COUNT"), virtual.GetAttribute("3")))
	})
	add(func() { virtual.SetAttribute("VALUE", "500") })
	equal("VALUE selects an item in virtual mode", &virtual, "VALUE", "500")

	add(func() {
		check("setting VALUE does not call ACTION or VALUECHANGED_CB", actions == 0 && changes == 0,
			strings.Join(fired, ", "))
	})

	add(func() { plain.SetAttribute("REMOVEITEM", "ALL"); drop.SetAttribute("REMOVEITEM", "ALL") })
	equal("REMOVEITEM ALL removes every item", &plain, "COUNT", "0")
	equal("VALUE of an empty dropdown is 0", &drop, "VALUE", "0")

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
	).SetAttributes(`NMARGIN=10x10, NGAP=8`)).SetAttribute("TITLE", "List checks")

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
