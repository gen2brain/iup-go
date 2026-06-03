package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	cv := iup.Canvas()
	cv.SetAttributes("EXPAND=YES, RASTERSIZE=320x240")
	cv.SetCallback("ACTION", iup.ActionFunc(drawCb))

	cv.SetCallback("TOUCH_CB", iup.TouchFunc(func(ih iup.Ihandle, id, x, y int, state string) int {
		appendLog(fmt.Sprintf("TOUCH_CB       id=%d  x=%d y=%d  state=%s", id, x, y, state))
		return iup.DEFAULT
	}))

	cv.SetCallback("MULTITOUCH_CB", iup.MultiTouchFunc(func(ih iup.Ihandle, count int, pid, px, py, pstate []int) int {
		msg := fmt.Sprintf("MULTITOUCH_CB  count=%d", count)
		for i := 0; i < count; i++ {
			msg += fmt.Sprintf("  [id=%d %d,%d %c]", pid[i], px[i], py[i], rune(pstate[i]))
		}
		appendLog(msg)
		return iup.DEFAULT
	}))

	txtLog := iup.Text()
	txtLog.SetAttributes("MULTILINE=YES, EXPAND=HORIZONTAL, READONLY=YES, VISIBLELINES=10")
	txtLog.SetAttribute("VALUE", "Touch the canvas to see TOUCH_CB / MULTITOUCH_CB.\n"+
		"state: DOWN / MOVE / UP (-PRIMARY for the primary point); the MULTITOUCH state char is D/M/U.\n"+
		"Reported on Windows, Qt, WebAssembly, iOS and Android only.\n"+
		"---\n")
	iup.SetHandle("log", txtLog)

	dlg := iup.Dialog(
		iup.Vbox(
			cv,
			iup.Label("Event Log:"),
			txtLog,
		).SetAttributes("MARGIN=10x10, GAP=5"),
	)
	dlg.SetAttribute("TITLE", "Canvas Touch")

	iup.Show(dlg)
	iup.MainLoop()
}

func drawCb(ih iup.Ihandle) int {
	iup.DrawBegin(ih)
	defer iup.DrawEnd(ih)

	w, h := iup.DrawGetSize(ih)
	ih.SetAttributes(`DRAWCOLOR="225 230 245", DRAWSTYLE=FILL`)
	iup.DrawRectangle(ih, 0, 0, w, h)
	ih.SetAttribute("DRAWCOLOR", "60 60 80")
	iup.DrawText(ih, "Touch / multi-touch here", 10, 10, w, h)
	return iup.DEFAULT
}

func appendLog(msg string) {
	log := iup.GetHandle("log")
	log.SetAttribute("APPEND", msg)
}
