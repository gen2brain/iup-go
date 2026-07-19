package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func setStatus(s string) { iup.GetHandle("status").SetAttribute("TITLE", s) }

func mainDlg() iup.Ihandle { return iup.GetHandle("dlg") }

func btn(title string, fn func()) iup.Ihandle {
	b := iup.Button(title).SetAttribute("EXPAND", "HORIZONTAL")
	b.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { fn(); return iup.DEFAULT }))
	return b
}

// newChild builds a closable child dialog parented to the main dialog (not shown).
func newChild(title, attrs string, body iup.Ihandle) iup.Ihandle {
	child := iup.Dialog(body).SetAttribute("TITLE", title)
	if attrs != "" {
		child.SetAttributes(attrs)
	}
	iup.SetAttributeHandle(child, "PARENTDIALOG", mainDlg())
	child.SetCallback("CLOSE_CB", iup.CloseFunc(func(iup.Ihandle) int { return iup.CLOSE }))
	return child
}

// focusless: RASTERSIZE + no focusable child - the WinUI-crash regression guard.
func focusless() {
	child := newChild("Focus-less RASTERSIZE", `RASTERSIZE=320x130`,
		iup.Vbox(
			iup.Label("RASTERSIZE with no focusable child."),
			iup.Label("This crashed the WinUI driver."),
		).SetAttributes(`NMARGIN=16x16, NGAP=8`))
	iup.Show(child)
	setStatus("opened focus-less RASTERSIZE dialog")
}

// showChildAt shows a modeless child at a named position (LEFT=top, RIGHT=bottom for y).
func showChildAt(desc string, x, y int) {
	child := newChild("Positioned child", "",
		iup.Vbox(
			iup.Label(desc),
		).SetAttributes(`NMARGIN=16x16, NGAP=8`))
	iup.ShowXY(child, x, y)
	setStatus("child shown " + desc)
}

// modalDemo pops a modal child that can stack a second modal on top.
func modalDemo() {
	var child iup.Ihandle
	body := iup.Vbox(
		iup.Label("Modal (Popup) - blocks the parent until closed."),
		btn("Open nested modal", func() {
			var inner iup.Ihandle
			inner = newChild("Nested modal", "",
				iup.Vbox(
					iup.Label("A second modal, stacked on the first."),
					btn("Close", func() { iup.Hide(inner) }),
				).SetAttributes(`NMARGIN=16x16, NGAP=8`))
			iup.Popup(inner, iup.CENTER, iup.CENTER)
			iup.Destroy(inner)
		}),
		btn("Close", func() { iup.Hide(child) }),
	).SetAttributes(`NMARGIN=16x16, NGAP=8`)
	child = newChild("Modal dialog", "", body)
	iup.Popup(child, iup.CENTER, iup.CENTER)
	iup.Destroy(child)
	setStatus("modal closed")
}

// startFocusDemo names the second field as STARTFOCUS, so it focuses on show.
func startFocusDemo() {
	first := iup.Text().SetAttributes(`VISIBLECOLUMNS=16`)
	second := iup.Text().SetAttributes(`VISIBLECOLUMNS=16`).SetHandle("focusField")
	child := newChild("STARTFOCUS", `STARTFOCUS=focusField`,
		iup.Vbox(
			iup.Label("The second field starts focused."),
			iup.Label("First:"), first,
			iup.Label("Second (STARTFOCUS):"), second,
		).SetAttributes(`NMARGIN=16x16, NGAP=6`))
	iup.Show(child)
	setStatus("child shown with STARTFOCUS on the second field")
}

// showNoFocus shows a child that does not steal focus from the main dialog.
func showNoFocus() {
	child := newChild("SHOWNOFOCUS", `SHOWNOFOCUS=YES`,
		iup.Vbox(iup.Label("Shown without taking focus - the main dialog keeps it.")).SetAttributes(`NMARGIN=16x16`))
	iup.ShowXY(child, iup.RIGHT, iup.LEFT)
	setStatus("SHOWNOFOCUS child shown - main dialog keeps focus")
}

// showNoActivate shows a child without activating it (Windows/macOS; no-op elsewhere).
func showNoActivate() {
	child := newChild("SHOWNOACTIVATE", `SHOWNOACTIVATE=YES`,
		iup.Vbox(iup.Label("Shown without activating (Windows/macOS).")).SetAttributes(`NMARGIN=16x16`))
	iup.ShowXY(child, iup.RIGHT, iup.RIGHT)
	setStatus("SHOWNOACTIVATE child shown")
}

// simulateModal disables the other dialogs without a nested loop; a button releases it.
func simulateModal() {
	var child iup.Ihandle
	child = newChild("SIMULATEMODAL", "",
		iup.Vbox(
			iup.Label("The main dialog is disabled (manual modal)."),
			btn("Release and close", func() {
				child.SetAttribute("SIMULATEMODAL", "NO")
				iup.Hide(child)
			}),
		).SetAttributes(`NMARGIN=16x16, NGAP=8`))
	iup.Show(child)
	child.SetAttribute("SIMULATEMODAL", "YES")
	setStatus("SIMULATEMODAL on - the main dialog is disabled until released")
}

// nativeParentDemo parents a child through the raw native window handle (WID) instead of PARENTDIALOG.
func nativeParentDemo() {
	child := iup.Dialog(
		iup.Vbox(iup.Label("Parented via NATIVEPARENT (raw native handle).")).SetAttributes(`NMARGIN=16x16`)).
		SetAttribute("TITLE", "NATIVEPARENT")
	child.SetAttribute("NATIVEPARENT", iup.GetPtr(mainDlg(), "WID"))
	child.SetCallback("CLOSE_CB", iup.CloseFunc(func(iup.Ihandle) int { return iup.CLOSE }))
	iup.ShowXY(child, iup.CENTER, iup.CENTER)
	setStatus("child shown via NATIVEPARENT (raw native handle)")
}

func main() {
	iup.Open()
	defer iup.Close()

	sizing := iup.Frame(iup.Vbox(
		btn("RASTERSIZE 640x520 (live)", func() {
			mainDlg().SetAttribute("RASTERSIZE", "640x520")
			iup.Show(mainDlg())
			setStatus("RASTERSIZE set to 640x520 on the live dialog")
		}),
		btn("MINSIZE 300x220 / MAXSIZE 800x600", func() {
			mainDlg().SetAttributes(`MINSIZE=300x220, MAXSIZE=800x600`)
			setStatus("MINSIZE/MAXSIZE set - drag-resize to test the clamp")
		}),
	).SetAttributes(`NMARGIN=6x6, NGAP=4`)).SetAttribute("TITLE", "Sizing")

	placement := iup.Frame(iup.Vbox(
		btn("Maximize", func() {
			mainDlg().SetAttribute("PLACEMENT", "MAXIMIZED")
			iup.Show(mainDlg())
			setStatus("PLACEMENT=MAXIMIZED")
		}),
		btn("Full (client fills screen)", func() {
			mainDlg().SetAttribute("PLACEMENT", "FULL")
			iup.Show(mainDlg())
			setStatus("PLACEMENT=FULL")
		}),
		btn("Minimize (restore from taskbar)", func() {
			mainDlg().SetAttribute("PLACEMENT", "MINIMIZED")
			iup.Show(mainDlg())
			setStatus("PLACEMENT=MINIMIZED")
		}),
		btn("Restore", func() {
			mainDlg().SetAttribute("PLACEMENT", "NORMAL")
			iup.Show(mainDlg())
			setStatus("restored to normal")
		}),
		btn("Toggle FULLSCREEN", func() {
			d := mainDlg()
			if d.GetAttribute("FULLSCREEN") == "YES" {
				d.SetAttribute("FULLSCREEN", "NO")
				setStatus("FULLSCREEN off")
			} else {
				d.SetAttribute("FULLSCREEN", "YES")
				setStatus("FULLSCREEN on")
			}
		}),
	).SetAttributes(`NMARGIN=6x6, NGAP=4`)).SetAttribute("TITLE", "Placement")

	windows := iup.Frame(iup.Vbox(
		btn("Child top-left", func() { showChildAt("top-left", iup.LEFT, iup.LEFT) }),
		btn("Child bottom-right", func() { showChildAt("bottom-right", iup.RIGHT, iup.RIGHT) }),
		btn("Child at mouse", func() { showChildAt("at mouse", iup.MOUSEPOS, iup.MOUSEPOS) }),
		btn("Child centered on parent", func() { showChildAt("centered on parent", iup.CENTERPARENT, iup.CENTERPARENT) }),
		btn("Modal (+ nested)", modalDemo),
		btn("TOPMOST child", func() {
			child := newChild("TOPMOST", `TOPMOST=YES`,
				iup.Vbox(iup.Label("Always in front of other dialogs.")).SetAttributes(`NMARGIN=16x16`))
			iup.ShowXY(child, iup.RIGHT, iup.LEFT)
			setStatus("TOPMOST child shown")
		}),
		btn("TOOLBOX child", func() {
			child := newChild("Toolbox", `TOOLBOX=YES, MAXBOX=NO, MINBOX=NO`,
				iup.Vbox(iup.Label("Small title bar, no taskbar entry.")).SetAttributes(`NMARGIN=16x16`))
			iup.ShowXY(child, iup.LEFT, iup.RIGHT)
			setStatus("TOOLBOX child shown")
		}),
		btn("STARTFOCUS child", startFocusDemo),
		btn("Focus-less RASTERSIZE", focusless),
	).SetAttributes(`NMARGIN=6x6, NGAP=4`)).SetAttribute("TITLE", "Child windows")

	modes := iup.Frame(iup.Vbox(
		btn("SHOWNOFOCUS child", showNoFocus),
		btn("SHOWNOACTIVATE child", showNoActivate),
		btn("SIMULATEMODAL child", simulateModal),
		btn("NATIVEPARENT child", nativeParentDemo),
	).SetAttributes(`NMARGIN=6x6, NGAP=4`)).SetAttribute("TITLE", "Modal & focus")

	status := iup.Label("Drive dialog sizing, placement and child windows.").
		SetAttribute("EXPAND", "HORIZONTAL")
	iup.SetHandle("status", status)

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Hbox(sizing, placement, windows, modes).SetAttributes(`NGAP=10`),
			status,
		).SetAttributes(`NMARGIN=12x12, NGAP=8`),
	).SetAttributes(`TITLE="Dialog sizing and placement"`)
	iup.SetHandle("dlg", dlg)

	iup.Show(dlg)
	iup.MainLoop()
}
