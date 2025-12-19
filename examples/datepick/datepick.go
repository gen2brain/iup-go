package main

import (
	"fmt"
	"time"

	"github.com/gen2brain/iup-go/iup"
)

func appendLog(msg string) {
	log := iup.GetHandle("log")
	timestamp := time.Now().Format("15:04:05.000")
	log.SetAttribute("APPEND", fmt.Sprintf("[%s] %s\n", timestamp, msg))
	fmt.Printf("[%s] %s\n", timestamp, msg)
}

func main() {
	iup.Open()
	defer iup.Close()

	// Create DatePick variants to test different attributes
	dpDefault := iup.DatePick()
	dpMDY := iup.DatePick().SetAttributes("ORDER=MDY")
	dpYMD := iup.DatePick().SetAttributes(`ORDER=YMD, SEPARATOR="-"`)
	dpZero := iup.DatePick().SetAttributes("ZEROPRECED=YES")
	dpWeek := iup.DatePick().SetAttributes("CALENDARWEEKNUMBERS=YES")

	// Store handles for later access
	iup.SetHandle("dp_default", dpDefault)
	iup.SetHandle("dp_mdy", dpMDY)
	iup.SetHandle("dp_ymd", dpYMD)
	iup.SetHandle("dp_zero", dpZero)
	iup.SetHandle("dp_week", dpWeek)

	// Log widget
	txtLog := iup.Text()
	txtLog.SetAttributes("MULTILINE=YES, EXPAND=YES, READONLY=YES, VISIBLELINES=10")
	txtLog.SetAttribute("VALUE", "IupDatePick Test\n================\n")
	iup.SetHandle("log", txtLog)

	// VALUECHANGED_CB - the only DatePick-specific callback
	valueChangedCb := iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		name := iup.GetName(ih)
		value := iup.GetAttribute(ih, "VALUE")
		appendLog(fmt.Sprintf("%s: VALUECHANGED_CB value='%s'", name, value))
		return iup.DEFAULT
	})

	dpDefault.SetCallback("VALUECHANGED_CB", valueChangedCb)
	dpMDY.SetCallback("VALUECHANGED_CB", valueChangedCb)
	dpYMD.SetCallback("VALUECHANGED_CB", valueChangedCb)
	dpZero.SetCallback("VALUECHANGED_CB", valueChangedCb)
	dpWeek.SetCallback("VALUECHANGED_CB", valueChangedCb)

	// Action buttons
	btnGetToday := iup.Button("Get TODAY").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		today := iup.GetAttribute(dpDefault, "TODAY")
		appendLog(fmt.Sprintf("TODAY: %s", today))
		return iup.DEFAULT
	}))

	btnGetValue := iup.Button("Get VALUE").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		value := iup.GetAttribute(dpDefault, "VALUE")
		appendLog(fmt.Sprintf("VALUE: %s", value))
		return iup.DEFAULT
	}))

	btnSetToday := iup.Button("Set VALUE=TODAY").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		iup.SetAttribute(dpDefault, "VALUE", "TODAY")
		appendLog("Set VALUE=TODAY")
		return iup.DEFAULT
	}))

	btnSetDate := iup.Button("Set VALUE=2025/6/15").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		iup.SetAttribute(dpDefault, "VALUE", "2025/6/15")
		appendLog("Set VALUE=2025/6/15")
		return iup.DEFAULT
	}))

	btnCompare := iup.Button("Compare All").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		appendLog("--- All VALUES ---")
		appendLog(fmt.Sprintf("  default: %s", iup.GetAttribute(dpDefault, "VALUE")))
		appendLog(fmt.Sprintf("  MDY:     %s", iup.GetAttribute(dpMDY, "VALUE")))
		appendLog(fmt.Sprintf("  YMD:     %s", iup.GetAttribute(dpYMD, "VALUE")))
		appendLog(fmt.Sprintf("  zero:    %s", iup.GetAttribute(dpZero, "VALUE")))
		appendLog(fmt.Sprintf("  week:    %s", iup.GetAttribute(dpWeek, "VALUE")))
		return iup.DEFAULT
	}))

	btnClearLog := iup.Button("Clear Log").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		iup.GetHandle("log").SetAttribute("VALUE", "Log cleared.\n")
		return iup.DEFAULT
	}))

	// Layout
	frameVariants := iup.Frame(
		iup.Vbox(
			iup.Hbox(iup.Label("Default (DMY):"), dpDefault).SetAttribute("ALIGNMENT", "ACENTER"),
			iup.Hbox(iup.Label("ORDER=MDY:"), dpMDY).SetAttribute("ALIGNMENT", "ACENTER"),
			iup.Hbox(iup.Label("ORDER=YMD, SEP=-:"), dpYMD).SetAttribute("ALIGNMENT", "ACENTER"),
			iup.Hbox(iup.Label("ZEROPRECED=YES:"), dpZero).SetAttribute("ALIGNMENT", "ACENTER"),
			iup.Hbox(iup.Label("CALENDARWEEKNUMBERS:"), dpWeek).SetAttribute("ALIGNMENT", "ACENTER"),
		).SetAttributes("GAP=5, MARGIN=10x10"),
	).SetAttribute("TITLE", "DatePick Variants")

	frameActions := iup.Frame(
		iup.Vbox(
			iup.Hbox(btnGetToday, btnGetValue, btnSetToday).SetAttribute("GAP", "5"),
			iup.Hbox(btnSetDate, btnCompare, btnClearLog).SetAttribute("GAP", "5"),
		).SetAttributes("GAP=5, MARGIN=10x10"),
	).SetAttribute("TITLE", "Actions")

	frameLog := iup.Frame(txtLog).SetAttribute("TITLE", "Event Log")

	dlg := iup.Dialog(
		iup.Vbox(
			frameVariants,
			frameActions,
			frameLog,
		).SetAttributes("GAP=10, MARGIN=10x10"),
	).SetAttributes(`TITLE="DatePick Test"`)

	iup.Show(dlg)
	iup.MainLoop()
}
