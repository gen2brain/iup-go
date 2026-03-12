package main

import (
	"fmt"
	"time"

	"github.com/gen2brain/iup-go/iup"
)

var closeTimer iup.Ihandle

func main() {
	iup.Open()
	defer iup.Close()

	txtLog := iup.Text()
	txtLog.SetAttributes("MULTILINE=YES, EXPAND=YES, READONLY=YES, VISIBLELINES=10")
	txtLog.SetAttribute("VALUE", "IupDialog Callbacks\n"+
		"=========================================\n"+
		"• Resize the dialog to trigger RESIZE_CB\n"+
		"• Minimize/restore to trigger SHOW_CB\n"+
		"• Drop files onto the dialog for DROPFILES_CB\n"+
		"• Close the dialog for CLOSE_CB\n"+
		"---\n")

	iup.SetHandle("log", txtLog)

	btnClearLog := iup.Button("Clear Log")
	iup.SetCallback(btnClearLog, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		log := iup.GetHandle("log")
		log.SetAttribute("VALUE", "Log cleared.\n")
		return iup.DEFAULT
	}))

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Label("IupDialog Callbacks").SetAttributes(`FONT="Sans, Bold 12"`),
			iup.Label("Resize, minimize/restore, drop files, or close the dialog to see callbacks."),
			iup.Hbox(
				btnClearLog,
			).SetAttributes("GAP=5"),
			iup.Label("Event Log:"),
			txtLog,
		).SetAttributes("MARGIN=10x10, GAP=10"),
	)

	dlg.SetAttributes(`TITLE="IupDialog Callbacks", SIZE=400x300`)

	closeTimer = iup.Timer()
	closeTimer.SetAttribute("TIME", "2000")
	iup.SetCallback(closeTimer, "ACTION_CB", iup.TimerActionFunc(func(ih iup.Ihandle) int {
		ih.SetAttribute("RUN", "NO")
		return iup.CLOSE
	}))

	iup.SetCallback(dlg, "CLOSE_CB", iup.CloseFunc(func(ih iup.Ihandle) int {
		appendLog("CLOSE_CB - closing in 2 seconds...")
		closeTimer.SetAttribute("RUN", "YES")
		return iup.IGNORE
	}))

	iup.SetCallback(dlg, "DROPFILES_CB", iup.DropFilesFunc(func(ih iup.Ihandle, filename string, num, x, y int) int {
		appendLog(fmt.Sprintf("DROPFILES_CB - file='%s' num=%d x=%d y=%d", filename, num, x, y))
		return iup.DEFAULT
	}))

	iup.SetCallback(dlg, "RESIZE_CB", iup.ResizeFunc(func(ih iup.Ihandle, width, height int) int {
		appendLog(fmt.Sprintf("RESIZE_CB - width=%d height=%d", width, height))
		return iup.DEFAULT
	}))

	iup.SetCallback(dlg, "SHOW_CB", iup.ShowFunc(func(ih iup.Ihandle, state int) int {
		var stateStr string
		switch state {
		case iup.SHOW:
			stateStr = "SHOW"
		case iup.RESTORE:
			stateStr = "RESTORE"
		case iup.MINIMIZE:
			stateStr = "MINIMIZE"
		case iup.MAXIMIZE:
			stateStr = "MAXIMIZE"
		case iup.HIDE:
			stateStr = "HIDE"
		default:
			stateStr = fmt.Sprintf("UNKNOWN(%d)", state)
		}
		appendLog(fmt.Sprintf("SHOW_CB - state=%s", stateStr))
		return iup.DEFAULT
	}))

	iup.Show(dlg)
	iup.MainLoop()

	iup.Destroy(closeTimer)
}

func appendLog(msg string) {
	log := iup.GetHandle("log")
	timestamp := time.Now().Format("15:04:05.000")
	log.SetAttribute("APPEND", fmt.Sprintf("[%s] %s\n", timestamp, msg))
	fmt.Printf("[%s] %s\n", timestamp, msg)
}
