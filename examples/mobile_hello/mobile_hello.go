package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

var (
	bgIndex          int
	orientationIndex int
	bgPalette        = []struct {
		name, rgb string
	}{
		{"Default", ""},
		{"Light blue", "200 225 255"},
		{"Pale green", "210 245 210"},
		{"Sandstone", "245 230 200"},
	}
	orientationCycle = []string{"UNSPECIFIED", "PORTRAIT", "LANDSCAPE", "SENSOR"}
)

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	statusLabel := iup.Label("Ready").
		SetHandle("statusLabel").
		SetAttributes(`EXPAND=HORIZONTAL, ALIGNMENT=ACENTER`)

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Label("Hello from IUP-Go").SetAttributes(`EXPAND=HORIZONTAL, ALIGNMENT=ACENTER`),
			statusLabel,
			iup.Frame(
				iup.Vbox(
					iup.Hbox(
						iup.Button("Fullscreen").
							SetAttribute("PADDING", "DEFAULTBUTTONPADDING").
							SetCallback("ACTION", iup.ActionFunc(toggleFullscreen)),
						iup.Button("No Title Bar").
							SetAttribute("PADDING", "DEFAULTBUTTONPADDING").
							SetCallback("ACTION", iup.ActionFunc(toggleTitleBar)),
					).SetAttributes(`GAP=10`),
					iup.Hbox(
						iup.Button("Cycle BG").
							SetAttribute("PADDING", "DEFAULTBUTTONPADDING").
							SetCallback("ACTION", iup.ActionFunc(cycleBg)),
						iup.Button("Orientation").
							SetAttribute("PADDING", "DEFAULTBUTTONPADDING").
							SetCallback("ACTION", iup.ActionFunc(cycleOrientation)),
					).SetAttributes(`GAP=10`),
				).SetAttributes(`ALIGNMENT=ACENTER, MARGIN=10x10, GAP=10`),
			).SetAttributes(`TITLE=Actions`),
			iup.Button("Quit").
				SetAttribute("PADDING", "DEFAULTBUTTONPADDING").
				SetCallback("ACTION", iup.ActionFunc(quit)),
		).SetAttributes(`ALIGNMENT=ACENTER, MARGIN=10x10, GAP=10`),
	).SetAttributes(`TITLE="IUP-Go Demo", TITLECENTERED=YES, TITLEBARSTYLE=LIFTED`)

	dlg.SetCallback("SHOW_CB", iup.ShowFunc(onShow))
	dlg.SetCallback("FOCUS_CB", iup.FocusFunc(onFocus))
	dlg.SetCallback("RESIZE_CB", iup.ResizeFunc(onResize))

	iup.Show(dlg)
	iup.MainLoop()
}

func toggleFullscreen(ih iup.Ihandle) int {
	dlg := iup.GetDialog(ih)
	if dlg.GetAttribute("FULLSCREEN") == "YES" {
		dlg.SetAttribute("FULLSCREEN", "NO")
	} else {
		dlg.SetAttribute("FULLSCREEN", "YES")
	}
	return iup.DEFAULT
}

func toggleTitleBar(ih iup.Ihandle) int {
	dlg := iup.GetDialog(ih)
	if dlg.GetAttribute("HIDETITLEBAR") == "YES" {
		dlg.SetAttribute("HIDETITLEBAR", "NO")
	} else {
		dlg.SetAttribute("HIDETITLEBAR", "YES")
	}
	return iup.DEFAULT
}

func cycleBg(ih iup.Ihandle) int {
	bgIndex = (bgIndex + 1) % len(bgPalette)
	next := bgPalette[bgIndex]
	dlg := iup.GetDialog(ih)
	if next.rgb == "" {
		dlg.SetAttribute("BGCOLOR", nil)
	} else {
		dlg.SetAttribute("BGCOLOR", next.rgb)
	}
	iup.GetHandle("statusLabel").SetAttribute("TITLE", "BGCOLOR: "+next.name)
	return iup.DEFAULT
}

func cycleOrientation(ih iup.Ihandle) int {
	orientationIndex = (orientationIndex + 1) % len(orientationCycle)
	next := orientationCycle[orientationIndex]
	iup.GetDialog(ih).SetAttribute("ORIENTATION", next)
	iup.GetHandle("statusLabel").SetAttribute("TITLE", "ORIENTATION: "+next)
	return iup.DEFAULT
}

func onShow(ih iup.Ihandle, state int) int {
	names := []string{"SHOW", "RESTORE", "MINIMIZE", "MAXIMIZE", "HIDE"}
	label := "UNKNOWN"
	if state >= 0 && state < len(names) {
		label = names[state]
	}
	iup.GetHandle("statusLabel").SetAttribute("TITLE", "SHOW_CB: "+label)
	return iup.DEFAULT
}

func onFocus(ih iup.Ihandle, focus int) int {
	label := "focus lost"
	if focus != 0 {
		label = "focus gained"
	}
	iup.GetHandle("statusLabel").SetAttribute("TITLE", "FOCUS_CB: "+label)
	return iup.DEFAULT
}

func onResize(ih iup.Ihandle, width, height int) int {
	iup.GetHandle("statusLabel").SetAttribute("TITLE", fmt.Sprintf("RESIZE_CB: %dx%d", width, height))
	return iup.DEFAULT
}

func quit(ih iup.Ihandle) int {
	return iup.CLOSE
}
