package main

import (
	"fmt"
	"image/png"
	"log"
	"os"
	"time"

	"github.com/gen2brain/iup-go/iup"
)

func appendLog(msg string) {
	logText := iup.GetHandle("log")
	timestamp := time.Now().Format("15:04:05.000")
	logText.SetAttribute("APPEND", fmt.Sprintf("[%s] %s\n", timestamp, msg))
	fmt.Printf("[%s] %s\n", timestamp, msg)
}

func main() {
	iup.Open()
	defer iup.Close()

	file, err := os.Open("icon.png")
	if err != nil {
		log.Fatalln(err)
	}

	image, err := png.Decode(file)
	if err != nil {
		log.Fatalln(err)
	}

	iup.ImageFromImage(image).SetHandle("myicon")

	// Log widget
	txtLog := iup.Text()
	txtLog.SetAttributes("MULTILINE=YES, EXPAND=YES, READONLY=YES, VISIBLELINES=12")
	txtLog.SetAttribute("VALUE", "=====================================\n"+
		"• Left-click: Show dialog\n"+
		"• Right-click: Show menu\n"+
		"• Double-click: Log event\n"+
		"---\n")
	iup.SetHandle("log", txtLog)

	// Status label
	statusLabel := iup.Label("Status: Tray visible")
	statusLabel.SetAttributes("EXPAND=HORIZONTAL")
	iup.SetHandle("status", statusLabel)

	// Buttons
	btnClearLog := iup.Button("Clear Log")
	btnClearLog.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		logText := iup.GetHandle("log")
		logText.SetAttribute("VALUE", "Log cleared.\n")
		appendLog("Log cleared")
		return iup.DEFAULT
	}))

	btnChangeTip := iup.Button("Change Tip")
	btnChangeTip.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		tray := iup.GetHandle("tray")
		newTip := fmt.Sprintf("Updated at %s", time.Now().Format("15:04:05"))
		tray.SetAttribute("TIP", newTip)
		appendLog(fmt.Sprintf("Changed tip to: %s", newTip))
		return iup.DEFAULT
	}))

	btnHideTray := iup.Button("Hide Tray")
	btnHideTray.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		tray := iup.GetHandle("tray")
		tray.SetAttribute("VISIBLE", "NO")
		appendLog("Tray hidden")
		statusLabel.SetAttribute("TITLE", "Status: Tray hidden")
		return iup.DEFAULT
	}))

	btnShowTray := iup.Button("Show Tray")
	btnShowTray.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		tray := iup.GetHandle("tray")
		tray.SetAttribute("VISIBLE", "YES")
		appendLog("Tray shown")
		statusLabel.SetAttribute("TITLE", "Status: Tray visible")
		return iup.DEFAULT
	}))

	// Dialog
	dlg := iup.Dialog(
		iup.Vbox(
			iup.Label("IupTray").SetAttributes(`FONT="Sans, Bold 12"`),
			statusLabel,
			iup.Hbox(
				btnShowTray,
				btnHideTray,
				btnChangeTip,
				btnClearLog,
			).SetAttributes("GAP=5"),
			iup.Label("Event Log:"),
			txtLog,
		).SetAttributes("MARGIN=10x10, GAP=8"),
	).SetHandle("dlg")

	// Menu items
	itemShow := iup.Item("Show Dialog").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		appendLog("Menu: Show Dialog clicked")
		dlg.SetAttribute("HIDETASKBAR", "NO")
		return iup.DEFAULT
	}))

	itemHide := iup.Item("Hide Dialog").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		appendLog("Menu: Hide Dialog clicked")
		dlg.SetAttribute("HIDETASKBAR", "YES")
		return iup.DEFAULT
	}))

	itemExit := iup.Item("Exit").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		appendLog("Menu: Exit clicked")
		tray := iup.GetHandle("tray")
		tray.SetAttribute("VISIBLE", "NO")
		tray.Destroy()
		dlg.Destroy()
		return iup.DEFAULT
	}))

	trayMenu := iup.Menu(itemShow, itemHide, iup.Separator(), itemExit)
	trayMenu.SetHandle("traymenu")

	// Create tray
	tray := iup.Tray()
	tray.SetHandle("tray")
	tray.SetAttributes(map[string]string{
		"IMAGE": "myicon",
		"TIP":   "IupTray - Right-click for menu",
		"MENU":  "traymenu",
	})

	tray.SetAttribute("VISIBLE", "YES")

	tray.SetCallback("TRAYCLICK_CB", iup.TrayClickFunc(func(ih iup.Ihandle, but, pressed, dclick int) int {
		buttonName := map[int]string{1: "Left", 2: "Middle", 3: "Right"}[but]
		pressedStr := "released"
		if pressed == 1 {
			pressedStr = "pressed"
		}
		dclickStr := ""
		if dclick == 1 {
			dclickStr = " (double-click)"
		}

		appendLog(fmt.Sprintf("TRAYCLICK_CB: button=%s %s%s", buttonName, pressedStr, dclickStr))

		// Show dialog on left-click press
		if but == 1 && pressed == 1 {
			dlg.SetAttribute("HIDETASKBAR", "NO")
		}

		return iup.DEFAULT
	}))

	dlg.SetAttributes(map[string]string{
		"TITLE": "IupTray",
		"ICON":  "myicon",
	})

	dlg.SetCallback("CLOSE_CB", iup.CloseFunc(func(ih iup.Ihandle) int {
		appendLog("Dialog CLOSE_CB: Hiding to tray instead of closing")
		dlg.SetAttribute("HIDETASKBAR", "YES")
		return iup.IGNORE
	}))

	iup.Map(trayMenu)
	iup.Show(dlg)

	iup.MainLoop()
}
