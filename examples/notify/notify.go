package main

import (
	"fmt"
	"strings"
	"time"

	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	iup.SetGlobal("APPNAME", "IUP Notify")

	iconImage := iup.ImageRGBA(16, 16, notifyIcon)
	iup.SetHandle("NOTIFY_ICON", iconImage)

	var notify iup.Ihandle

	// macOS requires explicit permission for notifications
	if strings.HasPrefix(iup.GetGlobal("SYSTEM"), "macOS") {
		permNotify := iup.Notify()
		permission := permNotify.GetAttribute("PERMISSION")
		if permission == "NOTDETERMINED" {
			permNotify.SetAttribute("REQUESTPERMISSION", "YES")
			fmt.Println("Requested notification permission from user")
		} else {
			fmt.Printf("Notification permission: %s\n", permission)
		}
		iup.Destroy(permNotify)
	}

	txtLog := iup.Text()
	txtLog.SetAttributes("MULTILINE=YES, EXPAND=YES, READONLY=YES, VISIBLELINES=10")
	txtLog.SetAttribute("VALUE", "Desktop Notifications Example\n"+
		"=====================================\n"+
		"macOS: Requires a signed .app bundle.\n"+
		"  codesign --force --deep --sign - MyApp.app\n"+
		"Windows: Action buttons are not supported.\n"+
		"---\n")
	iup.SetHandle("log", txtLog)

	txtTitle := iup.Text()
	txtTitle.SetAttributes(`EXPAND=HORIZONTAL, VALUE="Hello from IUP!"`)
	iup.SetHandle("title", txtTitle)

	txtBody := iup.Text()
	txtBody.SetAttributes(`EXPAND=HORIZONTAL, VALUE="This is a test notification."`)
	iup.SetHandle("body", txtBody)

	btnSimple := iup.Button("Send Simple Notification")
	btnSimple.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		title := iup.GetHandle("title").GetAttribute("VALUE")
		body := iup.GetHandle("body").GetAttribute("VALUE")

		notify = iup.Notify()
		notify.SetAttribute("TITLE", title)
		notify.SetAttribute("BODY", body)

		notify.SetCallback("NOTIFY_CB", iup.NotifyFunc(func(ih iup.Ihandle, actionId int) int {
			appendLog(fmt.Sprintf("Notification clicked! ActionId: %d", actionId))
			return iup.DEFAULT
		}))

		notify.SetCallback("CLOSE_CB", iup.NotifyCloseFunc(func(ih iup.Ihandle, reason int) int {
			reasonStr := "unknown"
			switch reason {
			case 1:
				reasonStr = "expired"
			case 2:
				reasonStr = "user dismissed"
			case 3:
				reasonStr = "closed programmatically"
			}
			appendLog(fmt.Sprintf("Notification closed. Reason: %s (%d)", reasonStr, reason))
			return iup.DEFAULT
		}))

		notify.SetCallback("ERROR_CB", iup.ErrorFunc(func(ih iup.Ihandle, errMsg string) int {
			appendLog(fmt.Sprintf("Notification error: %s", errMsg))
			return iup.DEFAULT
		}))

		notify.SetAttribute("SHOW", "YES")
		appendLog("Sent simple notification")
		return iup.DEFAULT
	}))

	btnActions := iup.Button("Send Notification with Actions")
	btnActions.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		notify = iup.Notify()
		notify.SetAttribute("TITLE", "Download Complete")
		notify.SetAttribute("BODY", "Your file has finished downloading.")
		notify.SetAttribute("ACTION1", "Open")
		notify.SetAttribute("ACTION2", "Show")
		notify.SetAttribute("ACTION3", "Delete")

		notify.SetCallback("NOTIFY_CB", iup.NotifyFunc(func(ih iup.Ihandle, actionId int) int {
			action := "body clicked"
			switch actionId {
			case 1:
				action = "Open"
			case 2:
				action = "Show"
			case 3:
				action = "Delete"
			}
			appendLog(fmt.Sprintf("Action clicked: %s (id=%d)", action, actionId))
			return iup.DEFAULT
		}))

		notify.SetCallback("CLOSE_CB", iup.NotifyCloseFunc(func(ih iup.Ihandle, reason int) int {
			appendLog(fmt.Sprintf("Notification with actions closed. Reason: %d", reason))
			return iup.DEFAULT
		}))

		notify.SetAttribute("SHOW", "YES")
		appendLog("Sent notification with actions")
		return iup.DEFAULT
	}))

	btnSilent := iup.Button("Send Silent Notification")
	btnSilent.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		notify = iup.Notify()
		notify.SetAttribute("TITLE", "Silent Update")
		notify.SetAttribute("BODY", "This notification doesn't make a sound.")
		notify.SetAttribute("SILENT", "YES")
		notify.SetAttribute("SHOW", "YES")
		appendLog("Sent silent notification")
		return iup.DEFAULT
	}))

	btnUrgent := iup.Button("Send Urgent Notification (Linux)")
	btnUrgent.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		notify = iup.Notify()
		notify.SetAttribute("TITLE", "Critical Alert!")
		notify.SetAttribute("BODY", "This is an urgent notification that won't auto-dismiss.")
		notify.SetAttribute("URGENCY", "2")
		notify.SetAttribute("TIMEOUT", "0")
		notify.SetAttribute("SHOW", "YES")
		appendLog("Sent urgent notification (Linux only)")
		return iup.DEFAULT
	}))

	btnWithIcon := iup.Button("Send Notification with Icon")
	btnWithIcon.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		notify = iup.Notify()
		notify.SetAttribute("TITLE", "Update Available")
		notify.SetAttribute("BODY", "A new version is ready to install.")
		notify.SetAttribute("ICON", "NOTIFY_ICON")

		// macOS-specific attributes (ignored on other platforms)
		notify.SetAttribute("SUBTITLE", "Version 2.0")
		notify.SetAttribute("THREADID", "updates")

		notify.SetCallback("NOTIFY_CB", iup.NotifyFunc(func(ih iup.Ihandle, actionId int) int {
			appendLog(fmt.Sprintf("Icon notification clicked! ActionId: %d", actionId))
			return iup.DEFAULT
		}))

		notify.SetCallback("CLOSE_CB", iup.NotifyCloseFunc(func(ih iup.Ihandle, reason int) int {
			appendLog(fmt.Sprintf("Icon notification closed. Reason: %d", reason))
			return iup.DEFAULT
		}))

		notify.SetAttribute("SHOW", "YES")
		appendLog("Sent notification with icon")
		return iup.DEFAULT
	}))

	btnClose := iup.Button("Close Last Notification")
	btnClose.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		if notify != 0 {
			notify.SetAttribute("CLOSE", "YES")
			appendLog("Closed notification programmatically")
		} else {
			appendLog("No active notification to close")
		}
		return iup.DEFAULT
	}))

	titleFrame := iup.Frame(
		iup.Vbox(
			iup.Label("Title:"),
			txtTitle,
			iup.Label("Body:"),
			txtBody,
		),
	).SetAttribute("TITLE", "Notification Content")

	buttonsFrame := iup.Frame(
		iup.Vbox(
			btnSimple,
			btnActions,
			btnWithIcon,
			btnSilent,
			btnUrgent,
			btnClose,
		).SetAttributes("GAP=5"),
	).SetAttribute("TITLE", "Actions")

	logFrame := iup.Frame(txtLog).SetAttribute("TITLE", "Log")

	dlg := iup.Dialog(
		iup.Vbox(
			titleFrame,
			iup.Hbox(
				buttonsFrame,
				logFrame.SetAttribute("EXPAND", "YES"),
			).SetAttributes("GAP=10"),
		).SetAttributes("GAP=10, MARGIN=10x10"),
	)
	dlg.SetAttributes("TITLE=IupNotify Example, SIZE=500x400")

	iup.Show(dlg)
	iup.MainLoop()

	if notify != 0 {
		iup.Destroy(notify)
	}
}

func appendLog(msg string) {
	logText := iup.GetHandle("log")
	timestamp := time.Now().Format("15:04:05.000")
	logText.SetAttribute("APPEND", fmt.Sprintf("[%s] %s\n", timestamp, msg))
	fmt.Printf("[%s] %s\n", timestamp, msg)
}

// Simple 16x16 notification bell icon (RGBA format)
var notifyIcon = []byte{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 65, 105, 225, 255, 65, 105, 225, 255, 65, 105, 225, 255, 65, 105, 225, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 65, 105, 225, 255, 65, 105, 225, 255, 65, 105, 225, 255, 65, 105, 225, 255, 65, 105, 225, 255, 65, 105, 225, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 65, 105, 225, 255, 65, 105, 225, 255, 65, 105, 225, 255, 65, 105, 225, 255, 65, 105, 225, 255, 65, 105, 225, 255, 65, 105, 225, 255, 65, 105, 225, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 65, 105, 225, 255, 65, 105, 225, 255, 65, 105, 225, 255, 65, 105, 225, 255, 65, 105, 225, 255, 65, 105, 225, 255, 65, 105, 225, 255, 65, 105, 225, 255, 65, 105, 225, 255, 65, 105, 225, 255, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 65, 105, 225, 255, 65, 105, 225, 255, 65, 105, 225, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 65, 105, 225, 255, 65, 105, 225, 255, 65, 105, 225, 255, 0, 0, 0, 0,
	0, 0, 0, 0, 65, 105, 225, 255, 65, 105, 225, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 65, 105, 225, 255, 65, 105, 225, 255, 0, 0, 0, 0,
	0, 0, 0, 0, 65, 105, 225, 255, 65, 105, 225, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 65, 105, 225, 255, 65, 105, 225, 255, 0, 0, 0, 0,
	0, 0, 0, 0, 65, 105, 225, 255, 65, 105, 225, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 65, 105, 225, 255, 65, 105, 225, 255, 0, 0, 0, 0,
	0, 0, 0, 0, 65, 105, 225, 255, 65, 105, 225, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 65, 105, 225, 255, 65, 105, 225, 255, 0, 0, 0, 0,
	0, 0, 0, 0, 65, 105, 225, 255, 65, 105, 225, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 65, 105, 225, 255, 65, 105, 225, 255, 0, 0, 0, 0,
	0, 0, 0, 0, 65, 105, 225, 255, 65, 105, 225, 255, 65, 105, 225, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 100, 149, 237, 255, 65, 105, 225, 255, 65, 105, 225, 255, 65, 105, 225, 255, 0, 0, 0, 0,
	65, 105, 225, 255, 65, 105, 225, 255, 65, 105, 225, 255, 65, 105, 225, 255, 65, 105, 225, 255, 65, 105, 225, 255, 65, 105, 225, 255, 65, 105, 225, 255, 65, 105, 225, 255, 65, 105, 225, 255, 65, 105, 225, 255, 65, 105, 225, 255, 65, 105, 225, 255, 65, 105, 225, 255, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 165, 0, 255, 255, 165, 0, 255, 255, 165, 0, 255, 255, 165, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 165, 0, 255, 255, 165, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
}
