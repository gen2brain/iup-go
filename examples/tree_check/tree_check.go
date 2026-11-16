package main

import (
	"fmt"
	"os"
	"strings"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

type step struct {
	run func() bool
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

	selections []string
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

func attrs(name string, ih *iup.Ihandle, pairs ...string) {
	add(func() {
		var bad []string
		for _, p := range pairs {
			k, want, _ := strings.Cut(p, "=")
			if got := (*ih).GetAttribute(k); got != want {
				bad = append(bad, fmt.Sprintf("%s=%q, want %q", k, got, want))
			}
		}
		check(name, len(bad) == 0, strings.Join(bad, ", "))
	})
}

func buildSteps() {
	steps = nil
	var dlg, tree, multi iup.Ihandle

	add(func() {
		selections = nil
		tree = iup.Tree().SetAttributes(`VISIBLELINES=10, VISIBLECOLUMNS=20`)
		tree.SetCallback("SELECTION_CB", iup.SelectionFunc(func(ih iup.Ihandle, id, status int) int {
			selections = append(selections, fmt.Sprintf("SELECTION_CB %d %d", id, status))
			return iup.DEFAULT
		}))
		multi = iup.Tree().SetAttributes(`MARKMODE=MULTIPLE, VISIBLELINES=6, VISIBLECOLUMNS=20`)
		dlg = iup.Dialog(iup.Hbox(tree, multi).SetAttributes(`NMARGIN=10x10, NGAP=6`)).SetAttribute("TITLE", "tree_check")
		iup.Show(dlg)
	})

	attrs("the root is a branch at depth 0", &tree, "COUNT=1", "KIND0=BRANCH", "DEPTH0=0")

	add(func() { tree.SetAttribute("TITLE0", "Root") })
	attrs("TITLE sets the node text", &tree, "TITLE0=Root")

	add(func() { tree.SetAttribute("ADDLEAF0", "Leaf A") })
	attrs("ADDLEAF on a branch adds its first child", &tree, "COUNT=2", "LASTADDNODE=1", "KIND1=LEAF", "DEPTH1=1", "PARENT1=0", "TITLE1=Leaf A")

	add(func() { tree.SetAttribute("ADDBRANCH1", "Branch B") })
	attrs("ADDBRANCH on a leaf adds a brother after it", &tree, "COUNT=3", "LASTADDNODE=2", "KIND2=BRANCH", "DEPTH2=1", "PARENT2=0")

	add(func() { tree.SetAttribute("ADDLEAF2", "Leaf C") })
	attrs("ADDLEAF on a branch adds a child", &tree, "COUNT=4", "LASTADDNODE=3", "DEPTH3=2", "PARENT3=2", "CHILDCOUNT2=1", "TOTALCHILDCOUNT0=3")

	attrs("NEXT, PREVIOUS, FIRST and LAST follow the brothers", &tree, "NEXT1=2", "PREVIOUS2=1", "FIRST2=1", "LAST1=2", "NEXT2=", "PREVIOUS1=")

	attrs("STATE of a new branch with children is EXPANDED", &tree, "STATE2=EXPANDED", "STATE1=")
	add(func() { tree.SetAttribute("STATE2", "COLLAPSED") })
	attrs("STATE COLLAPSED collapses a branch", &tree, "STATE2=COLLAPSED")
	add(func() { tree.SetAttribute("STATE2", "EXPANDED") })
	attrs("STATE EXPANDED expands a branch", &tree, "STATE2=EXPANDED")

	add(func() { tree.SetAttribute("VALUE", "3") })
	attrs("VALUE moves the focus to a node", &tree, "VALUE=3")

	add(func() { tree.SetAttribute("INSERTLEAF2", "Leaf D") })
	attrs("INSERTLEAF on a branch adds a brother after its children", &tree, "COUNT=5", "LASTADDNODE=4", "DEPTH4=1", "TITLE4=Leaf D", "PARENT4=0")

	add(func() { tree.SetAttribute("COPYNODE1", "4") })
	attrs("COPYNODE copies a node after a leaf", &tree, "COUNT=6", "TITLE5=Leaf A", "DEPTH5=1", "TITLE1=Leaf A")

	add(func() { tree.SetAttribute("MOVENODE5", "2") })
	attrs("MOVENODE moves a node into an expanded branch", &tree, "COUNT=6", "TITLE3=Leaf A", "PARENT3=2", "CHILDCOUNT2=2")

	add(func() { tree.SetAttribute("DELNODE3", "SELECTED") })
	attrs("DELNODE SELECTED removes a node", &tree, "COUNT=5", "CHILDCOUNT2=1")

	add(func() { tree.SetAttribute("DELNODE0", "CHILDREN") })
	attrs("DELNODE CHILDREN removes only the children", &tree, "COUNT=1", "TITLE0=Root")

	add(func() {
		multi.SetAttribute("ADDLEAF0", "a")
		multi.SetAttribute("ADDLEAF1", "b")
		multi.SetAttribute("ADDLEAF2", "c")
	})
	add(func() { multi.SetAttribute("MARKEDNODES", "-+-+") })
	attrs("MARKEDNODES sets the selection of a MULTIPLE tree", &multi, "MARKEDNODES=-+-+", "MARKED1=YES", "MARKED2=NO")
	add(func() { multi.SetAttribute("MARK", "CLEARALL") })
	attrs("MARK CLEARALL clears the selection", &multi, "MARKEDNODES=----")

	add(func() {
		check("setting attributes does not call SELECTION_CB", len(selections) == 0, strings.Join(selections, ", "))
	})

	add(func() { tree.SetAttribute("DELNODE", "ALL") })
	attrs("DELNODE ALL removes every node", &tree, "COUNT=0", "VALUE=-1")

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
	).SetAttributes(`NMARGIN=10x10, NGAP=8`)).SetAttribute("TITLE", "Tree checks")

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
