package main

import (
	"fmt"
	"os"
	"strings"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

var (
	logText  iup.Ihandle
	subject  iup.Ihandle
	steps    []func()
	current  int
	busy     bool
	pass     int
	fail     int
	failed   []string
	skipped  int
	driver   string
	winsys   string
	normal   [4]int
	runTimer iup.Ihandle
	auto     bool
)

func logf(format string, a ...any) {
	line := fmt.Sprintf(format, a...)
	logText.SetAttribute("APPEND", line)
	if auto {
		fmt.Fprintln(os.Stderr, line)
	}
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

func pair(ih iup.Ihandle, name string) (int, int) {
	var a, b int
	fmt.Sscanf(strings.Replace(ih.GetAttribute(name), ",", "x", 1), "%dx%d", &a, &b)
	return a, b
}

func near(a, b int) bool { return a-b <= 2 && b-a <= 2 }

func mobile() bool { return winsys == "ANDROID" || driver == "CocoaTouch" }

func noPosition() bool { return winsys == "WAYLAND" || mobile() }

func state(ih iup.Ihandle) string {
	return fmt.Sprintf("RASTERSIZE=%s CLIENTSIZE=%s SCREENPOSITION=%s MAXIMIZED=%s MINIMIZED=%s ACTIVE=%s",
		ih.GetAttribute("RASTERSIZE"), ih.GetAttribute("CLIENTSIZE"), ih.GetAttribute("SCREENPOSITION"),
		ih.GetAttribute("MAXIMIZED"), ih.GetAttribute("MINIMIZED"), ih.GetAttribute("ACTIVE"))
}

func newDialog(title, attrs string, body iup.Ihandle) iup.Ihandle {
	d := iup.Dialog(body).SetAttribute("TITLE", title)
	if attrs != "" {
		d.SetAttributes(attrs)
	}
	iup.SetAttributeHandle(d, "PARENTDIALOG", subject)
	return d
}

func label(s string) iup.Ihandle {
	return iup.Vbox(iup.Label(s)).SetAttributes(`NMARGIN=12x12`)
}

func childAt(name string, x, y int, verify func(c iup.Ihandle)) []func() {
	var c iup.Ihandle
	return []func(){
		func() {
			c = newDialog(name, "", label(name))
			iup.ShowXY(c, x, y)
		},
		func() {
			logf("      %s", state(c))
			if noPosition() {
				skip(name, "the windowing system places dialogs")
			} else {
				verify(c)
			}
			c.Destroy()
		},
	}
}

func buildSteps() {
	steps = nil
	add := func(f ...func()) { steps = append(steps, f...) }

	add(func() {
		subject = iup.Dialog(iup.Vbox(
			iup.Label("Dialog under test"),
			iup.Text().SetAttributes(`VISIBLECOLUMNS=12`),
			iup.Button("Button"),
		).SetAttributes(`NMARGIN=10x10, NGAP=6`)).SetAttribute("TITLE", "Dialog under test")
		iup.ShowXY(subject, iup.CENTER, iup.CENTER)
	})
	add(func() {
		logf("      %s", state(subject))
		subject.SetAttribute("RASTERSIZE", "420x300")
		iup.Show(subject)
	})
	add(func() {
		w, h := pair(subject, "RASTERSIZE")
		cw, _ := pair(subject, "CLIENTSIZE")
		if mobile() {
			skip("RASTERSIZE", "the dialog fills the screen")
		} else {
			check("RASTERSIZE 420x300", w == 420 && h == 300, state(subject))
		}
		border := subject.GetInt("BORDERSIZE")
		check("CLIENTSIZE width is RASTERSIZE minus the borders", cw == w-2*border, state(subject))
		x, y := pair(subject, "SCREENPOSITION")
		normal = [4]int{x, y, w, h}
	})

	add(func() { subject.SetAttribute("PLACEMENT", "MAXIMIZED"); iup.Show(subject) })
	add(func() {
		logf("      %s", state(subject))
		if mobile() {
			skip("MAXIMIZED", "not supported")
		} else {
			check("MAXIMIZED reads YES", subject.GetAttribute("MAXIMIZED") == "YES", state(subject))
		}
		subject.SetAttribute("PLACEMENT", "NORMAL")
		iup.Show(subject)
	})
	add(func() {
		x, y := pair(subject, "SCREENPOSITION")
		w, h := pair(subject, "RASTERSIZE")
		if mobile() {
			skip("restore from maximized", "not supported")
			return
		}
		check("restore from maximized: MAXIMIZED reads NO", subject.GetAttribute("MAXIMIZED") == "NO", state(subject))
		check("restore from maximized: size", w == normal[2] && h == normal[3], state(subject))
		if noPosition() {
			skip("restore from maximized: position", "the windowing system places dialogs")
		} else {
			check("restore from maximized: position", near(x, normal[0]) && near(y, normal[1]), fmt.Sprintf("%d,%d, was %d,%d", x, y, normal[0], normal[1]))
		}
	})

	add(func() {
		if mobile() || winsys == "WEB" || winsys == "WAYLAND" {
			skip("MINIMIZED", "not supported")
			return
		}
		subject.SetAttribute("PLACEMENT", "MINIMIZED")
		iup.Show(subject)
	})
	add(func() {
		if mobile() || winsys == "WEB" || winsys == "WAYLAND" {
			return
		}
		check("MINIMIZED reads YES", subject.GetAttribute("MINIMIZED") == "YES", state(subject))
		subject.SetAttribute("PLACEMENT", "NORMAL")
		iup.Show(subject)
	})
	add(func() {
		if mobile() || winsys == "WEB" || winsys == "WAYLAND" {
			return
		}
		check("restore from minimized: MINIMIZED reads NO", subject.GetAttribute("MINIMIZED") == "NO", state(subject))
	})

	add(func() { subject.SetAttribute("FULLSCREEN", "YES") })
	add(func() {
		logf("      %s", state(subject))
		subject.SetAttribute("FULLSCREEN", "NO")
	})
	add(func() {
		w, h := pair(subject, "RASTERSIZE")
		check("FULLSCREEN=NO keeps TITLE", subject.GetAttribute("TITLE") == "Dialog under test", subject.GetAttribute("TITLE"))
		if mobile() {
			skip("FULLSCREEN=NO size", "the dialog fills the screen")
		} else {
			check("FULLSCREEN=NO restores the size", w == normal[2] && h == normal[3], state(subject))
		}
	})

	add(childAt("ShowXY 200,150", 200, 150, func(c iup.Ihandle) {
		x, y := pair(c, "SCREENPOSITION")
		check("ShowXY 200,150", x == 200 && y == 150, c.GetAttribute("SCREENPOSITION"))
	})...)
	add(childAt("ShowXY LEFT,TOP", iup.LEFT, iup.TOP, func(c iup.Ihandle) {
		x, y := pair(c, "SCREENPOSITION")
		if driver == "EFL" && winsys == "X11" {
			skip("ShowXY LEFT,TOP", "EFL leaves a new dialog at 0,0 to the window manager")
			return
		}
		check("ShowXY LEFT,TOP", x == 0 && y == 0, c.GetAttribute("SCREENPOSITION"))
	})...)
	add(childAt("ShowXY RIGHT,BOTTOM", iup.RIGHT, iup.BOTTOM, func(c iup.Ihandle) {
		x, y := pair(c, "SCREENPOSITION")
		w, h := pair(c, "RASTERSIZE")
		var sw, sh int
		fmt.Sscanf(iup.GetGlobal("SCREENSIZE"), "%dx%d", &sw, &sh)
		check("ShowXY RIGHT,BOTTOM", near(x+w, sw) && near(y+h, sh), fmt.Sprintf("right,bottom %d,%d, screen %dx%d", x+w, y+h, sw, sh))
	})...)
	add(childAt("ShowXY CENTERPARENT", iup.CENTERPARENT, iup.CENTERPARENT, func(c iup.Ihandle) {
		x, y := pair(c, "SCREENPOSITION")
		w, h := pair(c, "RASTERSIZE")
		px, py := pair(subject, "SCREENPOSITION")
		pw, ph := pair(subject, "RASTERSIZE")
		check("ShowXY CENTERPARENT", near(2*x+w, 2*px+pw) && near(2*y+h, 2*py+ph), fmt.Sprintf("child %d,%d %dx%d, parent %d,%d %dx%d", x, y, w, h, px, py, pw, ph))
	})...)

	add(func() {
		var outer, inner iup.Ihandle
		outer = newDialog("Modal outer", "", label("modal"))
		inner = newDialog("Modal inner", "", label("nested"))
		tinner := iup.Timer().SetAttributes("TIME=600")
		tinner.SetCallback("ACTION_CB", iup.TimerActionFunc(func(ih iup.Ihandle) int {
			ih.SetAttribute("RUN", "NO")
			check("nested Popup: MODAL reads YES", inner.GetAttribute("MODAL") == "YES", inner.GetAttribute("MODAL"))
			check("nested Popup: outer dialog inactive", outer.GetAttribute("ACTIVE") == "NO", state(outer))
			iup.Hide(inner)
			return iup.DEFAULT
		}))
		touter := iup.Timer().SetAttributes("TIME=600")
		touter.SetCallback("ACTION_CB", iup.TimerActionFunc(func(ih iup.Ihandle) int {
			ih.SetAttribute("RUN", "NO")
			check("Popup: MODAL reads YES", outer.GetAttribute("MODAL") == "YES", outer.GetAttribute("MODAL"))
			check("Popup: parent inactive", subject.GetAttribute("ACTIVE") == "NO", state(subject))
			tinner.SetAttribute("RUN", "YES")
			iup.Popup(inner, iup.CENTER, iup.CENTER)
			iup.Hide(outer)
			return iup.DEFAULT
		}))
		touter.SetAttribute("RUN", "YES")
		iup.Popup(outer, iup.CENTER, iup.CENTER)
		check("after Popup: parent active again", subject.GetAttribute("ACTIVE") == "YES", state(subject))
		touter.Destroy()
		tinner.Destroy()
		inner.Destroy()
		outer.Destroy()
	})

	var child iup.Ihandle
	add(func() {
		child = newDialog("Simulated modal", "", label("simulated modal"))
		iup.Show(child)
		child.SetAttribute("SIMULATEMODAL", "YES")
	})
	add(func() {
		if mobile() {
			skip("SIMULATEMODAL", "the child covers the parent")
		} else {
			check("SIMULATEMODAL=YES: parent inactive", subject.GetAttribute("ACTIVE") == "NO", state(subject))
		}
		child.SetAttribute("SIMULATEMODAL", "NO")
		check("SIMULATEMODAL=NO: parent active", subject.GetAttribute("ACTIVE") == "YES", state(subject))
		child.Destroy()
	})

	add(func() {
		subject.SetAttribute("NACTIVE", "NO")
		check("NACTIVE=NO reads back", subject.GetAttribute("NACTIVE") == "NO", subject.GetAttribute("NACTIVE"))
		subject.SetAttribute("NACTIVE", "YES")
		check("NACTIVE=YES reads back", subject.GetAttribute("NACTIVE") == "YES", subject.GetAttribute("NACTIVE"))
	})

	add(func() {
		second := iup.Text().SetAttributes(`VISIBLECOLUMNS=12`).SetHandle("dialogCheckSecond")
		child = newDialog("STARTFOCUS", `STARTFOCUS=dialogCheckSecond`, iup.Vbox(iup.Text(), second).SetAttributes(`NMARGIN=12x12`))
		iup.Show(child)
	})
	add(func() {
		if mobile() {
			skip("STARTFOCUS", "SHOWNOFOCUS defaults to YES")
		} else {
			check("STARTFOCUS focuses the named field", iup.GetFocus() == iup.GetHandle("dialogCheckSecond"), "focus elsewhere")
		}
		child.Destroy()
	})

	parentActive := false
	add(func() { subject.SetAttribute("BRINGFRONT", "YES") })
	add(func() {
		parentActive = subject.GetAttribute("ACTIVEWINDOW") == "YES"
		child = newDialog("SHOWNOACTIVATE", `SHOWNOACTIVATE=YES`, iup.Vbox(iup.Text()).SetAttributes(`NMARGIN=12x12`))
		iup.ShowXY(child, iup.RIGHT, iup.TOP)
	})
	add(func() {
		supported := driver == "Win32" || driver == "WinUI" || driver == "Qt" || driver == "QML" || (driver == "Cocoa" && winsys == "QUARTZ")
		if !supported || winsys == "WAYLAND" {
			skip("SHOWNOACTIVATE", "not supported")
		} else if !parentActive {
			skip("SHOWNOACTIVATE", "the parent was not the active window")
		} else {
			check("SHOWNOACTIVATE: parent stays active", subject.GetAttribute("ACTIVEWINDOW") == "YES", "child ACTIVEWINDOW="+child.GetAttribute("ACTIVEWINDOW"))
		}
		child.Destroy()
	})

	add(func() {
		child = newDialog("Destroyed by its own timer", "", label("closes itself"))
		iup.Show(child)
		t := iup.Timer().SetAttributes("TIME=200, RUN=YES")
		t.SetCallback("ACTION_CB", iup.TimerActionFunc(func(ih iup.Ihandle) int {
			ih.SetAttribute("RUN", "NO")
			child.Destroy()
			child = 0
			ih.Destroy()
			return iup.DEFAULT
		}))
	})
	add(func() {
		check("dialog destroyed from a timer callback", child == 0, "the timer did not finish")
	})

	add(func() {
		subject.Destroy()
		subject = 0
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
	driver = iup.GetGlobal("DRIVER")
	winsys = iup.GetGlobal("WINDOWING")
	logText.SetAttribute("VALUE", "")
	logf("Driver %s, windowing %s, screen %s", driver, winsys, iup.GetGlobal("SCREENSIZE"))
	buildSteps()
	runTimer.SetAttribute("RUN", "YES")
}

func main() {
	iup.Open()
	defer iup.Close()

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
	).SetAttributes(`NMARGIN=10x10, NGAP=8`)).SetAttribute("TITLE", "Dialog checks")

	runTimer = iup.Timer().SetAttribute("TIME", "700")
	runTimer.SetCallback("ACTION_CB", iup.TimerActionFunc(func(ih iup.Ihandle) int {
		if busy || current >= len(steps) {
			return iup.DEFAULT
		}
		busy = true
		step := steps[current]
		current++
		step()
		busy = false
		return iup.DEFAULT
	}))

	auto = len(os.Args) > 1 && os.Args[1] == "-run"

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
