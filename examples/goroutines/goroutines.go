package main

import (
	"fmt"
	"sync/atomic"

	"github.com/gen2brain/iup-go/iup"
)

var totalMessages atomic.Int64

func main() {
	iup.Open()
	defer iup.Close()

	label := iup.Label("Press the button to start Goroutines stress test").SetAttributes(`EXPAND=HORIZONTAL, ALIGNMENT=ACENTER`)
	counter := iup.Label("Messages: 0").SetAttributes(`EXPAND=HORIZONTAL, ALIGNMENT=ACENTER`)
	status := iup.Label("Idle").SetAttributes(`EXPAND=HORIZONTAL, ALIGNMENT=ACENTER`)
	btn := iup.Button("Start Test").SetAttribute("PADDING", "DEFAULTBUTTONPADDING")

	iup.SetHandle("status", status)
	iup.SetHandle("counter", counter)
	iup.SetHandle("btn", btn)

	counter.SetCallback("POSTMESSAGE_CB", iup.PostMessageFunc(func(ih iup.Ihandle, s string, i int, p any) int {
		if i < 0 {
			iup.GetHandle("status").SetAttribute("TITLE", s)
			iup.GetHandle("btn").SetAttribute("ACTIVE", "YES")
			return iup.DEFAULT
		}

		ih.SetAttribute("TITLE", fmt.Sprintf("Messages: %d", totalMessages.Add(1)))
		return iup.DEFAULT
	}))

	btn.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		ih.SetAttribute("ACTIVE", "NO")
		goroutines := 100
		perGoroutine := 1000

		status.SetAttribute("TITLE", fmt.Sprintf("Running %d Goroutines, %d messages each...", goroutines, perGoroutine))
		totalMessages.Store(0)
		counter.SetAttribute("TITLE", "Messages: 0")

		target := iup.GetHandle("counter")
		var done atomic.Int32

		for i := 0; i < goroutines; i++ {
			go func(id int) {
				for j := 0; j < perGoroutine; j++ {
					iup.PostMessage(target, "", id, nil)
				}

				if int(done.Add(1)) == goroutines {
					iup.PostMessage(target, "Done - all Goroutines finished", -1, nil)
				}
			}(i)
		}

		return iup.DEFAULT
	}))

	dlg := iup.Dialog(iup.Vbox(
		label, counter, status,
		iup.Hbox(iup.Fill(), btn, iup.Fill()),
	).SetAttributes(map[string]string{
		"GAP":    "8",
		"MARGIN": "12x12",
	})).SetAttributes(`TITLE="Goroutine Stress Test", SIZE=350x`)

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}
