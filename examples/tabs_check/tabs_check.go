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

func pos(name string, t *iup.Ihandle, want int) {
	add(func() {
		got := (*t).GetInt("VALUEPOS")
		check(name, got == want, fmt.Sprintf("VALUEPOS=%d, want %d", got, want))
	})
}

func isCurrent(name string, t *iup.Ihandle, want *iup.Ihandle) {
	add(func() {
		got := iup.Ihandle((*t).GetPtr("VALUE_HANDLE"))
		check(name, got == *want, fmt.Sprintf("VALUE_HANDLE is %q, want %q", iup.GetName(got), iup.GetName(*want)))
	})
}

func page(text string) iup.Ihandle {
	return iup.Vbox(iup.Label(text)).SetAttributes(`NMARGIN=10x10`)
}

func buildSteps() {
	steps = nil
	var dlg, tabs, other, c0, c1, c2, c3, c4 iup.Ihandle

	add(func() {
		changes, fired = 0, nil
		c0 = page("Page one")
		c0.SetAttribute("TABTITLE", "First")
		c1 = page("Page two")
		c2 = page("Page three")
		tabs = iup.Tabs(c0, c1, c2)
		tabs.SetAttribute("TABTITLE1", "Second")
		tabs.SetAttribute("TABTITLE2", "Third")
		tabs.SetCallback("TABCHANGE_CB", iup.TabChangeFunc(func(ih, n, o iup.Ihandle) int {
			changes++
			fired = append(fired, "TABCHANGE_CB")
			return iup.DEFAULT
		}))
		other = iup.Tabs(page("A"), page("B"), page("C"))
		other.SetCallback("TABCHANGEPOS_CB", iup.TabChangePosFunc(func(ih iup.Ihandle, n, o int) int {
			changes++
			fired = append(fired, "TABCHANGEPOS_CB")
			return iup.DEFAULT
		}))
		dlg = iup.Dialog(iup.Vbox(tabs, other).SetAttributes(`NMARGIN=10x10, NGAP=6`)).SetAttribute("TITLE", "tabs_check")
		iup.Show(dlg)
	})

	equal("COUNT counts the tabs", &tabs, "COUNT", "3")
	pos("the first tab is current after map", &tabs, 0)
	add(func() {
		name := iup.GetName(c0)
		check("VALUE returns the name of the current child", name != "" && tabs.GetAttribute("VALUE") == name,
			fmt.Sprintf("VALUE=%q, name %q", tabs.GetAttribute("VALUE"), name))
	})
	equal("TABTITLE set at the child before creation is the tab title", &tabs, "TABTITLE0", "First")
	equal("TABTITLEid set before map is the tab title", &tabs, "TABTITLE1", "Second")

	add(func() { tabs.SetAttribute("VALUEPOS", "2") })
	pos("VALUEPOS changes the current tab", &tabs, 2)
	isCurrent("VALUE_HANDLE follows VALUEPOS", &tabs, &c2)

	add(func() { tabs.SetAttribute("VALUE", iup.GetName(c1)) })
	pos("VALUE changes the current tab by name", &tabs, 1)

	add(func() { tabs.SetAttribute("VALUE_HANDLE", c0) })
	pos("VALUE_HANDLE changes the current tab by handle", &tabs, 0)

	add(func() { tabs.SetAttribute("TABTITLE1", "Changed") })
	add(func() {
		check("TABTITLEid after map changes the title and the child TABTITLE",
			tabs.GetAttribute("TABTITLE1") == "Changed" && c1.GetAttribute("TABTITLE") == "Changed",
			fmt.Sprintf("TABTITLE1=%q child TABTITLE=%q", tabs.GetAttribute("TABTITLE1"), c1.GetAttribute("TABTITLE")))
	})

	add(func() { tabs.SetAttribute("VALUEPOS", "1") })
	add(func() { tabs.SetAttribute("TABVISIBLE1", "NO") })
	pos("hiding the current tab selects the nearest visible tab to its right", &tabs, 2)
	equal("TABVISIBLEid returns the hidden state", &tabs, "TABVISIBLE1", "NO")
	equal("hiding a tab keeps COUNT", &tabs, "COUNT", "3")
	add(func() { tabs.SetAttribute("TABVISIBLE2", "NO") })
	pos("hiding the last visible tab on the right selects the nearest visible on the left", &tabs, 0)
	add(func() { tabs.SetAttribute("TABVISIBLE1", "YES"); tabs.SetAttribute("TABVISIBLE2", "YES") })
	pos("showing tabs again keeps the current tab", &tabs, 0)

	add(func() {
		c3 = page("Page four")
		c3.SetAttribute("TABTITLE", "Fourth")
		iup.Append(tabs, c3)
		iup.Map(c3)
		iup.Refresh(tabs)
	})
	add(func() {
		check("Append adds a tab with the child TABTITLE", tabs.GetAttribute("COUNT") == "4" && tabs.GetAttribute("TABTITLE3") == "Fourth",
			fmt.Sprintf("COUNT=%s TABTITLE3=%q", tabs.GetAttribute("COUNT"), tabs.GetAttribute("TABTITLE3")))
	})
	pos("Append keeps the current tab", &tabs, 0)

	add(func() {
		c4 = page("Page zero")
		c4.SetAttribute("TABTITLE", "Zero")
		iup.Insert(tabs, c0, c4)
		iup.Map(c4)
		iup.Refresh(tabs)
	})
	add(func() {
		check("Insert adds a tab before the reference child", tabs.GetAttribute("COUNT") == "5" && tabs.GetAttribute("TABTITLE0") == "Zero" && tabs.GetAttribute("TABTITLE1") == "First",
			fmt.Sprintf("COUNT=%s TABTITLE0=%q TABTITLE1=%q", tabs.GetAttribute("COUNT"), tabs.GetAttribute("TABTITLE0"), tabs.GetAttribute("TABTITLE1")))
	})
	isCurrent("Insert keeps the current child", &tabs, &c0)
	pos("Insert before the current tab shifts VALUEPOS", &tabs, 1)

	add(func() { tabs.SetAttribute("VALUE_HANDLE", c1) })
	add(func() { c1.Destroy(); iup.Refresh(tabs) })
	isCurrent("removing the current tab selects the tab to its right", &tabs, &c2)
	equal("removing a tab decrements COUNT", &tabs, "COUNT", "4")
	add(func() { tabs.SetAttribute("VALUE_HANDLE", c3) })
	add(func() { c3.Destroy(); iup.Refresh(tabs) })
	isCurrent("removing the current last tab selects the tab to its left", &tabs, &c2)

	add(func() {
		other.SetAttribute("VALUEPOS", "2")
		other.SetAttribute("VALUEPOS", "1")
		other.SetAttribute("TABVISIBLE1", "NO")
		iup.GetChild(other, 2).Destroy()
	})
	pos("TABCHANGEPOS_CB tabs follow the same rules", &other, 0)
	add(func() {
		check("changing, hiding or removing tabs programmatically calls no TABCHANGE callback", changes == 0,
			strings.Join(fired, ", "))
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
	).SetAttributes(`NMARGIN=10x10, NGAP=8`)).SetAttribute("TITLE", "Tabs checks")

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
