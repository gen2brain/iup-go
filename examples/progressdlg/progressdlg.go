package main

import (
	"math"

	"github.com/gen2brain/iup-go/iup"
)

var counterDlg iup.Ihandle

func cancelCb(ih iup.Ihandle) int {
	ret := iup.Alarm("Warning!", "Interrupt Processing?", "Yes", "No", "")
	if ret == 1 { // interrupted
		iup.SetAttribute(ih, "SIMULATEMODE", "No")
		return iup.DEFAULT
	}

	return iup.CONTINUE
}

func counterStart(totalCount int) {
	counterDlg.SetAttribute("TOTALCOUNT", totalCount)
	counterDlg.SetAttribute("STATE", nil)
	counterDlg.SetCallback("CANCEL_CB", iup.CancelFunc(cancelCb))

	iup.Show(counterDlg)
	iup.SetAttribute(counterDlg, "SIMULATEMODAL", "No")
}

func counterInc() bool {
	iup.SetAttribute(counterDlg, "INC", nil)

	if iup.GetAttribute(counterDlg, "STATE") == "ABORTED" {
		return false
	}

	iup.LoopStep()

	return true
}

func counterStop() {
	iup.SetAttribute(counterDlg, "COUNT", iup.GetInt(counterDlg, "TOTALCOUNT"))
	iup.SetAttribute(counterDlg, "SIMULATEMODAL", "No")
	iup.Hide(counterDlg)
}

func longProcessing() {
	counterStart(10000)

	for i := 0; i < 10000; i++ {
		for j := 0; j < 10000; j++ {
			x := math.Abs(math.Sin(float64(j)*100) + math.Cos(float64(j)*100))
			x = math.Sqrt(x * x)
		}

		if !counterInc() {
			break
		}
	}

	counterStop()
}

func main() {
	iup.Open()
	defer iup.Close()

	counterDlg = iup.ProgressDlg().SetAttributes(map[string]string{
		"TITLE":       "Long Processing Test",
		"DESCRIPTION": "Description first line\nSecond Line",
		"RESIZE":      "YES",
	})
	defer iup.Destroy(counterDlg)

	longProcessing()
}
