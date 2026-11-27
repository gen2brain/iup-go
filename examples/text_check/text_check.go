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

	fired []string
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

func counted(name string, t iup.Ihandle) iup.Ihandle {
	t.SetCallback("ACTION", iup.TextActionFunc(func(ih iup.Ihandle, c int, newValue string) int {
		fired = append(fired, "ACTION on "+name)
		return iup.DEFAULT
	}))
	t.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		fired = append(fired, "VALUECHANGED_CB on "+name)
		return iup.DEFAULT
	}))
	t.SetCallback("CARET_CB", iup.CaretFunc(func(ih iup.Ihandle, lin, col, pos int) int {
		fired = append(fired, fmt.Sprintf("CARET_CB %d,%d on %s", lin, col, name))
		return iup.DEFAULT
	}))
	return t
}

func buildSteps() {
	steps = nil
	var dlg, line, multi, masked, password, spin, limited iup.Ihandle

	add(func() {
		fired = nil
		line = counted("line", iup.Text().SetAttributes(`VISIBLECOLUMNS=20`))
		multi = counted("multiline", iup.Text().SetAttributes(`MULTILINE=YES, VISIBLECOLUMNS=20, VISIBLELINES=4`))
		masked = counted("masked", iup.Text().SetAttributes(`VISIBLECOLUMNS=20`))
		password = counted("password", iup.Text().SetAttributes(`PASSWORD=YES, VISIBLECOLUMNS=20`))
		spin = counted("spin", iup.Text().SetAttributes(`SPIN=YES, SPINMAX=10, VISIBLECOLUMNS=8`))
		limited = counted("limited", iup.Text().SetAttributes(`NC=3, VISIBLECOLUMNS=8`))
		dlg = iup.Dialog(iup.Vbox(line, multi, masked, password, spin, limited).SetAttributes(`NMARGIN=10x10, NGAP=6`)).SetAttribute("TITLE", "text_check")
		iup.Show(dlg)
	})

	attrs("VALUE of an empty text is the empty string", &line, "VALUE=", "COUNT=0", "LINECOUNT=1")

	add(func() { line.SetAttribute("VALUE", "hello") })
	attrs("VALUE sets ASCII text", &line, "VALUE=hello", "COUNT=5")

	add(func() { line.SetAttribute("VALUE", "čćžšđ €") })
	attrs("VALUE sets UTF-8 text and COUNT counts characters", &line, "VALUE=čćžšđ €", "COUNT=7")

	add(func() { line.SetAttribute("VALUE", "hello"); line.SetAttribute("CARETPOS", "2") })
	attrs("CARETPOS moves the caret", &line, "CARETPOS=2", "CARET=3")

	add(func() { line.SetAttribute("INSERT", "XY") })
	attrs("INSERT inserts at the caret", &line, "VALUE=heXYllo")

	add(func() { line.SetAttribute("SELECTIONPOS", "1:4") })
	attrs("SELECTIONPOS selects characters", &line, "SELECTIONPOS=1:4", "SELECTION=2:5", "SELECTEDTEXT=eXY")

	add(func() { line.SetAttribute("SELECTEDTEXT", "E") })
	attrs("SELECTEDTEXT replaces the selection", &line, "VALUE=hEllo")

	add(func() { line.SetAttribute("SELECTION", "ALL") })
	attrs("SELECTION ALL selects everything", &line, "SELECTIONPOS=0:5", "SELECTEDTEXT=hEllo")

	add(func() { line.SetAttribute("SELECTION", "NONE") })
	attrs("SELECTION NONE clears the selection", &line, "SELECTION=", "SELECTEDTEXT=")

	add(func() { multi.SetAttribute("VALUE", "a\nbc\ndef") })
	attrs("multiline VALUE keeps the line breaks", &multi, "VALUE=a\nbc\ndef", "COUNT=8", "LINECOUNT=3")

	add(func() { multi.SetAttribute("APPEND", "gh") })
	attrs("APPEND adds a line break before the text", &multi, "VALUE=a\nbc\ndef\ngh", "LINECOUNT=4")

	add(func() { multi.SetAttribute("CARET", "2,2") })
	attrs("CARET lin,col moves the caret in a multiline text", &multi, "CARET=2,2", "CARETPOS=3", "LINEVALUE=bc")

	add(func() { multi.SetAttribute("SELECTION", "2,1:3,2") })
	attrs("SELECTION lin,col selects across lines", &multi, "SELECTION=2,1:3,2", "SELECTIONPOS=2:6", "SELECTEDTEXT=bc\nd")

	add(func() { multi.SetAttribute("CARET", "1,1"); multi.SetAttribute("SCROLLTO", "3,2") })
	attrs("SCROLLTO keeps the caret of a multiline text", &multi, "CARET=1,1")
	add(func() { multi.SetAttribute("SCROLLTOPOS", "6") })
	attrs("SCROLLTOPOS keeps the caret of a multiline text", &multi, "CARET=1,1")

	add(func() { multi.SetAttribute("VALUE", "č😀x\nab😀cd"); multi.SetAttribute("CARET", "2,4") })
	attrs("CARET lin,col counts characters on a line with non-ASCII text", &multi, "CARET=2,4", "CARETPOS=7", "LINEVALUE=ab😀cd")
	add(func() { multi.SetAttribute("SELECTION", "1,2:2,4") })
	attrs("SELECTION lin,col counts characters on lines with non-ASCII text", &multi, "SELECTION=1,2:2,4", "SELECTIONPOS=1:7", "SELECTEDTEXT=😀x\nab😀")

	add(func() { line.SetAttribute("CARETPOS", "0"); line.SetAttribute("SCROLLTOPOS", "3") })
	attrs("SCROLLTOPOS moves the caret of a single line text", &line, "CARETPOS=3")
	add(func() { line.SetAttribute("SCROLLTO", "2") })
	attrs("SCROLLTO moves the caret of a single line text", &line, "CARETPOS=1")

	add(func() { masked.SetAttribute("MASK", iup.MASK_UINT); masked.SetAttribute("VALUEMASKED", "123") })
	attrs("VALUEMASKED sets a value the MASK accepts", &masked, "VALUE=123")
	add(func() { masked.SetAttribute("VALUEMASKED", "12a") })
	attrs("VALUEMASKED ignores a value the MASK refuses", &masked, "VALUE=123")
	add(func() { masked.SetAttribute("VALUE", "abc") })
	attrs("VALUE ignores the MASK", &masked, "VALUE=abc")

	add(func() { password.SetAttribute("VALUE", "secret") })
	attrs("VALUE of a PASSWORD text is the real text", &password, "VALUE=secret", "COUNT=6")

	add(func() { masked.SetAttribute("READONLY", "YES"); masked.SetAttribute("VALUE", "ro") })
	attrs("VALUE changes a READONLY text", &masked, "READONLY=YES", "VALUE=ro")

	add(func() { limited.SetAttribute("VALUE", "abcdef") })
	attrs("VALUE can be longer than NC", &limited, "VALUE=abcdef")

	add(func() { spin.SetAttribute("SPINVALUE", "7") })
	attrs("SPINVALUE sets the spin and the text", &spin, "SPINVALUE=7", "VALUE=7")
	add(func() { spin.SetAttribute("SPINVALUE", "50") })
	attrs("SPINVALUE is limited to SPINMAX", &spin, "SPINVALUE=10")
	add(func() { spin.SetAttribute("SPINVALUE", "-5") })
	attrs("SPINVALUE is limited to SPINMIN", &spin, "SPINVALUE=0")

	add(func() {
		check("setting attributes does not call ACTION, VALUECHANGED_CB or CARET_CB", len(fired) == 0, strings.Join(fired, ", "))
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
	).SetAttributes(`NMARGIN=10x10, NGAP=8`)).SetAttribute("TITLE", "Text checks")

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
