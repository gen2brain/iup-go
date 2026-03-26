package main

import (
	"fmt"
	"time"

	"github.com/gen2brain/iup-go/iup"
)

var startTime time.Time

func main() {
	iup.Open()
	defer iup.Close()

	statusLabel := iup.Label("Ready").SetAttributes(`EXPAND=HORIZONTAL, ALIGNMENT=ACENTER`)
	progressLabel := iup.Label("0%").SetAttributes(`SIZE=30x, ALIGNMENT=ACENTER`)
	gauge := iup.ProgressBar().SetAttributes(`EXPAND=HORIZONTAL`)
	startBtn := iup.Button("Start Computation")
	cancelBtn := iup.Button("Cancel").SetAttribute("ACTIVE", "NO")

	thread := iup.Thread()
	iup.SetHandle("thread", thread)
	iup.SetHandle("gauge", gauge)
	iup.SetHandle("status", statusLabel)
	iup.SetHandle("progress", progressLabel)
	iup.SetHandle("startbtn", startBtn)
	iup.SetHandle("cancelbtn", cancelBtn)

	thread.SetCallback("THREAD_CB", iup.ThreadFunc(threadCb))
	statusLabel.SetCallback("POSTMESSAGE_CB", iup.PostMessageFunc(postMessageCb))
	startBtn.SetCallback("ACTION", iup.ActionFunc(startCb))
	cancelBtn.SetCallback("ACTION", iup.ActionFunc(cancelCb))

	timer := iup.Timer().SetAttribute("TIME", "100")
	timer.SetCallback("ACTION_CB", iup.TimerActionFunc(timerCb))

	vbox := iup.Vbox(
		statusLabel,
		iup.Hbox(gauge, progressLabel).SetAttribute("ALIGNMENT", "ACENTER"),
		iup.Hbox(iup.Fill(), startBtn, cancelBtn, iup.Fill()),
	).SetAttributes(map[string]string{
		"GAP":    "8",
		"MARGIN": "12x12",
	})

	dlg := iup.Dialog(vbox).SetAttributes(`TITLE="Thread", SIZE=300x`)

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	timer.SetAttribute("RUN", "YES")
	iup.MainLoop()

	iup.Destroy(timer)
	iup.Destroy(thread)
}

func threadCb(ih iup.Ihandle) int {
	status := iup.GetHandle("status")
	steps := 100

	for i := 0; i <= steps; i++ {
		ih.SetAttribute("LOCK", "YES")
		cancelled := ih.GetAttribute("_CANCELLED")
		ih.SetAttribute("LOCK", "NO")

		if cancelled == "YES" {
			iup.PostMessage(status, "Cancelled", -1, nil)
			return iup.DEFAULT
		}

		time.Sleep(50 * time.Millisecond)

		iup.PostMessage(status, "Computing...", i*100/steps, nil)
	}

	iup.PostMessage(status, "Done", 100, nil)
	return iup.DEFAULT
}

func postMessageCb(ih iup.Ihandle, s string, i int, p any) int {
	gauge := iup.GetHandle("gauge")
	progress := iup.GetHandle("progress")
	startBtn := iup.GetHandle("startbtn")
	cancelBtn := iup.GetHandle("cancelbtn")

	ih.SetAttribute("TITLE", s)

	if i < 0 {
		gauge.SetAttribute("VALUE", "0")
		progress.SetAttribute("TITLE", "0%")
		startBtn.SetAttribute("ACTIVE", "YES")
		cancelBtn.SetAttribute("ACTIVE", "NO")
		return iup.DEFAULT
	}

	gauge.SetAttribute("VALUE", fmt.Sprintf("%g", float64(i)/100.0))
	progress.SetAttribute("TITLE", fmt.Sprintf("%d%%", i))

	if i >= 100 {
		startBtn.SetAttribute("ACTIVE", "YES")
		cancelBtn.SetAttribute("ACTIVE", "NO")
	}

	return iup.DEFAULT
}

func startCb(ih iup.Ihandle) int {
	thread := iup.GetHandle("thread")

	thread.SetAttribute("LOCK", "YES")
	thread.SetAttribute("_CANCELLED", "NO")
	thread.SetAttribute("LOCK", "NO")

	ih.SetAttribute("ACTIVE", "NO")
	iup.GetHandle("cancelbtn").SetAttribute("ACTIVE", "YES")

	startTime = time.Now()
	thread.SetAttribute("START", "YES")
	return iup.DEFAULT
}

func cancelCb(ih iup.Ihandle) int {
	thread := iup.GetHandle("thread")

	thread.SetAttribute("LOCK", "YES")
	thread.SetAttribute("_CANCELLED", "YES")
	thread.SetAttribute("LOCK", "NO")

	ih.SetAttribute("ACTIVE", "NO")
	return iup.DEFAULT
}

func timerCb(ih iup.Ihandle) int {
	startBtn := iup.GetHandle("startbtn")
	if startBtn.GetAttribute("ACTIVE") == "NO" {
		elapsed := time.Since(startTime)
		dlg := iup.GetDialog(startBtn)
		dlg.SetAttribute("TITLE", fmt.Sprintf("Thread - %.1fs", elapsed.Seconds()))
	}
	return iup.DEFAULT
}
