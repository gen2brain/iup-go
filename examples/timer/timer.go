package main

import (
	"github.com/gen2brain/iup-go/iup"
)

var timer1, timer2 iup.Ihandle

func main() {
	iup.Open()
	defer iup.Close()

	text := iup.Label("Timer example")
	iup.SetHandle("quit", text)

	dlg := iup.Dialog(iup.Vbox(text)).SetAttributes(`TITLE="Timer", SIZE=200x200`)
	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)

	timer1 = iup.Timer().SetAttributes("TIME=1000, RUN=YES").SetCallback("ACTION_CB", iup.TimerActionFunc(timerCb))
	timer2 = iup.Timer().SetAttributes("TIME=4000, RUN=YES").SetCallback("ACTION_CB", iup.TimerActionFunc(timerCb))

	iup.MainLoop()

	// Timers are NOT automatically destroyed, must be manually done
	iup.Destroy(timer1)
	iup.Destroy(timer2)
}

func timerCb(ih iup.Ihandle) int {
	if ih == timer1 {
		println("timer1 called")
	} else if ih == timer2 {
		println("timer2 called")
		return iup.CLOSE
	}

	return iup.DEFAULT
}
