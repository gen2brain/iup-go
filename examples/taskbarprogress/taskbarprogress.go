package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

var (
	value     float32 = 0
	increment float32 = 0.01
)

func updateLabel(dlg iup.Ihandle, state string) {
	iup.GetHandle("info").SetAttribute("TITLE",
		fmt.Sprintf("State: %s   Value: %d%%", state, int(value*100)))
}

func setProgress(dlg iup.Ihandle, v float32) {
	if v < 0 {
		v = 0
	}
	if v > 1 {
		v = 1
	}
	value = v
	iup.GetHandle("bar").SetAttribute("VALUE", fmt.Sprintf("%g", v))
	dlg.SetAttribute("TASKBARPROGRESSVALUE", fmt.Sprintf("%d", int(v*100)))
}

func timeCb(ih iup.Ihandle) int {
	dlg := iup.GetHandle("dlg")
	setProgress(dlg, value+increment)
	updateLabel(dlg, iup.GetHandle("stateLabel").GetAttribute("TITLE"))
	if value >= 1 {
		iup.GetHandle("timer").SetAttribute("RUN", "NO")
	}
	return iup.DEFAULT
}

func btnStartCb(ih iup.Ihandle) int {
	dlg := iup.GetHandle("dlg")
	dlg.SetAttribute("TASKBARPROGRESS", "YES")
	dlg.SetAttribute("TASKBARPROGRESSSTATE", "NORMAL")
	setProgress(dlg, 0)
	iup.GetHandle("stateLabel").SetAttribute("TITLE", "NORMAL")
	updateLabel(dlg, "NORMAL")
	iup.GetHandle("timer").SetAttribute("RUN", "YES")
	return iup.DEFAULT
}

func setStateCb(state string) iup.ActionFunc {
	return func(ih iup.Ihandle) int {
		dlg := iup.GetHandle("dlg")
		dlg.SetAttribute("TASKBARPROGRESSSTATE", state)
		iup.GetHandle("stateLabel").SetAttribute("TITLE", state)
		updateLabel(dlg, state)
		if state == "INDETERMINATE" || state == "PAUSED" {
			iup.GetHandle("timer").SetAttribute("RUN", "NO")
		}
		return iup.DEFAULT
	}
}

func btnResumeCb(ih iup.Ihandle) int {
	dlg := iup.GetHandle("dlg")
	dlg.SetAttribute("TASKBARPROGRESSSTATE", "NORMAL")
	iup.GetHandle("stateLabel").SetAttribute("TITLE", "NORMAL")
	updateLabel(dlg, "NORMAL")
	iup.GetHandle("timer").SetAttribute("RUN", "YES")
	return iup.DEFAULT
}

func btnStopCb(ih iup.Ihandle) int {
	dlg := iup.GetHandle("dlg")
	iup.GetHandle("timer").SetAttribute("RUN", "NO")
	dlg.SetAttribute("TASKBARPROGRESSSTATE", "NOPROGRESS")
	dlg.SetAttribute("TASKBARPROGRESS", "NO")
	setProgress(dlg, 0)
	iup.GetHandle("stateLabel").SetAttribute("TITLE", "STOPPED")
	updateLabel(dlg, "STOPPED")
	return iup.DEFAULT
}

func btnFasterCb(ih iup.Ihandle) int { increment *= 2; return iup.DEFAULT }
func btnSlowerCb(ih iup.Ihandle) int { increment /= 2; return iup.DEFAULT }

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	iup.Timer().
		SetHandle("timer").
		SetAttribute("TIME", "100").
		SetCallback("ACTION_CB", iup.TimerActionFunc(timeCb))

	iup.ProgressBar().
		SetHandle("bar").
		SetAttribute("EXPAND", "HORIZONTAL")

	iup.Label("State: STOPPED   Value: 0%").
		SetHandle("info").
		SetAttribute("EXPAND", "HORIZONTAL")

	iup.Label("STOPPED").SetHandle("stateLabel")

	btn := func(title, name string, cb iup.ActionFunc) iup.Ihandle {
		return iup.Button(title).
			SetHandle(name).
			SetCallback("ACTION", cb).
			SetAttribute("EXPAND", "HORIZONTAL")
	}

	row1 := iup.Hbox(
		btn("Start", "btnStart", iup.ActionFunc(btnStartCb)),
		btn("Resume", "btnResume", iup.ActionFunc(btnResumeCb)),
		btn("Stop", "btnStop", iup.ActionFunc(btnStopCb)),
	).SetAttributes("NGAP=8")

	row2 := iup.Hbox(
		btn("Pause", "btnPause", setStateCb("PAUSED")),
		btn("Error", "btnError", setStateCb("ERROR")),
		btn("Indeterminate", "btnInd", setStateCb("INDETERMINATE")),
	).SetAttributes("NGAP=8")

	row3 := iup.Hbox(
		btn("Slower", "btnSlower", iup.ActionFunc(btnSlowerCb)),
		btn("Faster", "btnFaster", iup.ActionFunc(btnFasterCb)),
	).SetAttributes("NGAP=8")

	dlg := iup.Dialog(
		iup.Vbox(
			iup.GetHandle("bar"),
			iup.GetHandle("info"),
			row1,
			row2,
			row3,
		).SetAttributes("NMARGIN=12x12, NGAP=10"),
	)
	dlg.SetHandle("dlg")
	dlg.SetAttributes("TITLE=Taskbar Progress, RASTERSIZE=420x")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}
