package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

var weekdays = []string{"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"}

func main() {
	iup.Open()
	defer iup.Close()

	counter := iup.Label("0").SetAttribute("EXPAND", "HORIZONTAL")
	count := 0
	bareSpin := iup.Spin()
	bareSpin.SetCallback("SPIN_CB", iup.SpinFunc(func(ih iup.Ihandle, inc int) int {
		count += inc
		counter.SetAttribute("TITLE", fmt.Sprintf("%d", count))
		return iup.DEFAULT
	}))

	dayLabel := iup.Label(weekdays[0]).SetAttribute("EXPAND", "HORIZONTAL")
	dayIdx := 0
	dayBox := iup.SpinBox(dayLabel)
	dayBox.SetCallback("SPIN_CB", iup.SpinFunc(func(ih iup.Ihandle, inc int) int {
		dayIdx = (dayIdx + inc + len(weekdays)*1000) % len(weekdays)
		dayLabel.SetAttribute("TITLE", weekdays[dayIdx])
		return iup.DEFAULT
	}))

	numText := iup.Text().SetAttributes("VALUE=0, EXPAND=HORIZONTAL")
	numBox := iup.SpinBox(numText)
	numBox.SetCallback("SPIN_CB", iup.SpinFunc(func(ih iup.Ihandle, inc int) int {
		v := iup.GetInt(numText, "VALUE")
		v += inc
		numText.SetAttribute("VALUE", fmt.Sprintf("%d", v))
		return iup.DEFAULT
	}))

	hueLabel := iup.Label("hue 0").SetAttributes("EXPAND=HORIZONTAL, ALIGNMENT=ACENTER")
	hue := 0
	applyHue := func() {
		hueLabel.SetAttribute("BGCOLOR", hsvRGB(hue))
		hueLabel.SetAttribute("TITLE", fmt.Sprintf("hue %d", hue))
	}
	applyHue()
	hueSpin := iup.Spin()
	hueSpin.SetCallback("SPIN_CB", iup.SpinFunc(func(ih iup.Ihandle, inc int) int {
		hue = (hue + inc*10 + 3600) % 360
		applyHue()
		return iup.DEFAULT
	}))

	dlg := iup.Dialog(iup.Vbox(
		iup.Frame(iup.Hbox(counter, bareSpin).SetAttributes("NGAP=8, ALIGNMENT=ACENTER, NMARGIN=8x8")).
			SetAttribute("TITLE", "Counter"),
		iup.Frame(iup.Vbox(dayBox).SetAttributes("NMARGIN=8x8")).
			SetAttribute("TITLE", "Weekdays"),
		iup.Frame(iup.Vbox(numBox).SetAttributes("NMARGIN=8x8")).
			SetAttribute("TITLE", "Numeric"),
		iup.Frame(iup.Hbox(hueLabel, hueSpin).SetAttributes("NGAP=8, ALIGNMENT=ACENTER, NMARGIN=8x8")).
			SetAttribute("TITLE", "Hue"),
	).SetAttributes("NMARGIN=10x10, NGAP=8")).SetAttribute("TITLE", "Spin / SpinBox")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}

func hsvRGB(h int) string {
	s, v := 0.7, 0.95
	hh := float64(h%360) / 60.0
	c := v * s
	x := c * (1 - abs(mod(hh, 2)-1))
	m := v - c
	var r, g, b float64
	switch int(hh) % 6 {
	case 0:
		r, g, b = c, x, 0
	case 1:
		r, g, b = x, c, 0
	case 2:
		r, g, b = 0, c, x
	case 3:
		r, g, b = 0, x, c
	case 4:
		r, g, b = x, 0, c
	case 5:
		r, g, b = c, 0, x
	}
	return fmt.Sprintf("%d %d %d", int((r+m)*255), int((g+m)*255), int((b+m)*255))
}

func abs(f float64) float64 {
	if f < 0 {
		return -f
	}
	return f
}

func mod(a, b float64) float64 {
	r := a - float64(int(a/b))*b
	if r < 0 {
		r += b
	}
	return r
}
