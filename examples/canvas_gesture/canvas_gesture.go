package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

var swipeNames = []string{"RIGHT", "LEFT", "UP", "DOWN"}

// Committed transform plus the live delta of the gesture in progress.
var (
	scale, angle, panX, panY = 1.0, 0.0, 0.0, 0.0
	liveScale, liveAngle     = 1.0, 0.0
	livePanX, livePanY       = 0.0, 0.0
)

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	cv := iup.Canvas()
	cv.SetAttributes("EXPAND=YES, RASTERSIZE=320x320")
	cv.SetCallback("ACTION", iup.ActionFunc(drawCb))
	cv.SetCallback("GESTURE_CB", iup.GestureFunc(gestureCb))

	txtLog := iup.Text()
	txtLog.SetAttributes("MULTILINE=YES, EXPAND=HORIZONTAL, READONLY=YES, VISIBLELINES=8")
	txtLog.SetAttribute("VALUE", "Pinch to zoom, rotate or pan with two fingers.\n"+
		"Swipe, tap, double-tap and long-press are logged below.\n---\n")
	iup.SetHandle("log", txtLog)

	dlg := iup.Dialog(
		iup.Vbox(
			cv,
			iup.Label("Event Log:"),
			txtLog,
		).SetAttributes("MARGIN=10x10, GAP=5"),
	)
	dlg.SetAttribute("TITLE", "Canvas Gesture")

	iup.Show(dlg)
	iup.MainLoop()
}

func gestureCb(ih iup.Ihandle, gesture, state, x, y int, v1, v2 float64) int {
	switch gesture {
	case iup.GESTURE_PINCH:
		switch state {
		case iup.GESTURE_BEGIN:
			liveScale = 1.0
		case iup.GESTURE_CHANGED:
			liveScale = v1
		default:
			scale *= v1
			liveScale = 1.0
		}
		iup.Update(ih)
	case iup.GESTURE_ROTATE:
		switch state {
		case iup.GESTURE_BEGIN:
			liveAngle = 0
		case iup.GESTURE_CHANGED:
			liveAngle = v1
		default:
			angle += v1
			liveAngle = 0
		}
		iup.Update(ih)
	case iup.GESTURE_PAN:
		switch state {
		case iup.GESTURE_BEGIN:
			livePanX, livePanY = 0, 0
		case iup.GESTURE_CHANGED:
			livePanX, livePanY = v1, v2
		default:
			panX += v1
			panY += v2
			livePanX, livePanY = 0, 0
		}
		iup.Update(ih)
	case iup.GESTURE_SWIPE:
		appendLog(fmt.Sprintf("SWIPE      %s at %d,%d", swipeNames[int(v1)], x, y))
	case iup.GESTURE_TAP:
		appendLog(fmt.Sprintf("TAP        count=%d at %d,%d", int(v1), x, y))
	case iup.GESTURE_LONGPRESS:
		appendLog(fmt.Sprintf("LONGPRESS  at %d,%d", x, y))
	}
	return iup.DEFAULT
}

func drawCb(ih iup.Ihandle) int {
	iup.DrawBegin(ih)
	defer iup.DrawEnd(ih)

	w, h := iup.DrawGetSize(ih)
	ih.SetAttributes(`DRAWCOLOR="225 230 245", DRAWSTYLE=FILL`)
	iup.DrawRectangle(ih, 0, 0, w, h)

	iup.DrawSave(ih)
	iup.DrawTranslate(ih, float64(w)/2+panX+livePanX, float64(h)/2+panY+livePanY)
	iup.DrawRotate(ih, -(angle + liveAngle))
	iup.DrawScale(ih, scale*liveScale, scale*liveScale)
	ih.SetAttributes(`DRAWCOLOR="70 110 200", DRAWSTYLE=FILL`)
	iup.DrawRectangle(ih, -60, -60, 59, 59)
	ih.SetAttributes(`DRAWCOLOR="255 255 255", DRAWFONT="Helvetica, Bold 16"`)
	tw, th := iup.DrawGetTextSize(ih, "IUP")
	iup.DrawText(ih, "IUP", -tw/2, -th/2, -1, -1)
	iup.DrawRestore(ih)

	ih.SetAttribute("DRAWCOLOR", "60 60 80")
	iup.DrawText(ih, fmt.Sprintf("scale %.2f  angle %.0f", scale*liveScale, angle+liveAngle), 10, 10, w, h)
	return iup.DEFAULT
}

func appendLog(msg string) {
	iup.GetHandle("log").SetAttribute("APPEND", msg)
}
