package main

import (
	"fmt"
	"time"

	"github.com/gen2brain/iup-go/iup"
)

var (
	counter      int
	idleRunning  bool
	lastUpdate   time.Time
	updateCount  int
	statusLabel  iup.Ihandle
	counterLabel iup.Ihandle
	rateLabel    iup.Ihandle
	startBtn     iup.Ihandle
	stopBtn      iup.Ihandle
	testBtn      iup.Ihandle
)

func main() {
	iup.Open()
	defer iup.Close()

	statusLabel = iup.Label("Status: Idle stopped").SetAttribute("EXPAND", "HORIZONTAL")
	counterLabel = iup.Label("Counter: 0").SetAttribute("EXPAND", "HORIZONTAL")
	rateLabel = iup.Label("Rate: 0 updates/sec").SetAttribute("EXPAND", "HORIZONTAL")

	startBtn = iup.Button("Start Idle").SetCallback("ACTION", iup.ActionFunc(startIdleCb))
	stopBtn = iup.Button("Stop Idle").SetCallback("ACTION", iup.ActionFunc(stopIdleCb))
	testBtn = iup.Button("Test Event").SetCallback("ACTION", iup.ActionFunc(testEventCb))

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Frame(
				iup.Vbox(
					statusLabel,
					counterLabel,
					rateLabel,
				),
			).SetAttribute("TITLE", "Idle Status"),
			iup.Hbox(
				startBtn,
				stopBtn,
				testBtn,
			).SetAttribute("GAP", "5"),
			iup.Label("Close window to test if close works during idle"),
		).SetAttributes(`MARGIN=10x10, GAP=5`),
	).SetAttributes(`TITLE="Idle Test", SIZE=350x`)

	dlg.SetCallback("CLOSE_CB", iup.CloseFunc(closeCb))

	iup.Show(dlg)
	iup.MainLoop()

	fmt.Println("MainLoop exited normally")
}

func startIdleCb(ih iup.Ihandle) int {
	if idleRunning {
		return iup.DEFAULT
	}

	counter = 0
	updateCount = 0
	lastUpdate = time.Now()
	idleRunning = true

	iup.SetAttribute(statusLabel, "TITLE", "Status: Idle RUNNING")
	iup.SetFunction("IDLE_ACTION", iup.IdleFunc(idleFunction))

	fmt.Println("Idle started")
	return iup.DEFAULT
}

func stopIdleCb(ih iup.Ihandle) int {
	if !idleRunning {
		return iup.DEFAULT
	}

	iup.SetFunction("IDLE_ACTION", nil)
	idleRunning = false
	iup.SetAttribute(statusLabel, "TITLE", "Status: Idle stopped")

	fmt.Println("Idle stopped")
	return iup.DEFAULT
}

func testEventCb(ih iup.Ihandle) int {
	msg := fmt.Sprintf("Event received! Counter is at %d", counter)
	fmt.Println(msg)
	iup.Message("Test", msg)
	return iup.DEFAULT
}

func closeCb(ih iup.Ihandle) int {
	fmt.Println("CLOSE_CB called, stopping idle...")
	iup.SetFunction("IDLE_ACTION", nil)
	idleRunning = false
	fmt.Println("Idle stopped, returning DEFAULT to close")
	return iup.DEFAULT
}

func idleFunction() int {
	counter++
	updateCount++

	if counter%1000 == 0 {
		iup.SetAttribute(counterLabel, "TITLE", fmt.Sprintf("Counter: %d", counter))

		elapsed := time.Since(lastUpdate).Seconds()
		if elapsed > 0 {
			rate := float64(updateCount) / elapsed
			iup.SetAttribute(rateLabel, "TITLE", fmt.Sprintf("Rate: %.0f updates/sec", rate))
		}

		if elapsed >= 1.0 {
			lastUpdate = time.Now()
			updateCount = 0
		}
	}

	return iup.DEFAULT
}
