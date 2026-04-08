//go:build ctrl

package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

type theme struct {
	name  string
	attrs map[string]string
}

var themes = []theme{
	{
		name: "Default",
		attrs: map[string]string{
			"SHOWBORDER":  "YES",
			"BORDERWIDTH": "1",
			"BORDERCOLOR": "160 160 160",
			"PADDING":     "10x5",
		},
	},
	{
		name: "Dark",
		attrs: map[string]string{
			"BGCOLOR":     "45 45 48",
			"FGCOLOR":     "220 220 220",
			"HLCOLOR":     "62 62 66",
			"PSCOLOR":     "80 80 85",
			"SHOWBORDER":  "YES",
			"BORDERWIDTH": "1",
			"BORDERCOLOR": "100 100 105",
			"PADDING":     "10x5",
			"FRAMECOLOR":  "100 100 105",
			"TITLECOLOR":  "180 180 180",
			"SLIDERCOLOR": "62 62 66",
		},
	},
	{
		name: "Ocean",
		attrs: map[string]string{
			"BGCOLOR":     "230 244 255",
			"FGCOLOR":     "20 60 100",
			"HLCOLOR":     "180 220 255",
			"PSCOLOR":     "130 190 240",
			"SHOWBORDER":  "YES",
			"BORDERWIDTH": "1",
			"BORDERCOLOR": "60 140 200",
			"PADDING":     "10x5",
			"FRAMECOLOR":  "60 140 200",
			"TITLECOLOR":  "30 90 160",
			"SLIDERCOLOR": "160 210 250",
		},
	},
	{
		name: "Forest",
		attrs: map[string]string{
			"BGCOLOR":     "235 245 230",
			"FGCOLOR":     "30 70 30",
			"HLCOLOR":     "190 225 180",
			"PSCOLOR":     "150 200 140",
			"SHOWBORDER":  "YES",
			"BORDERWIDTH": "1",
			"BORDERCOLOR": "80 150 70",
			"PADDING":     "10x5",
			"FRAMECOLOR":  "80 150 70",
			"TITLECOLOR":  "40 100 40",
			"SLIDERCOLOR": "180 215 170",
		},
	},
	{
		name: "Warm",
		attrs: map[string]string{
			"BGCOLOR":     "255 240 225",
			"FGCOLOR":     "100 50 20",
			"HLCOLOR":     "255 215 180",
			"PSCOLOR":     "240 190 150",
			"SHOWBORDER":  "YES",
			"BORDERWIDTH": "1",
			"BORDERCOLOR": "200 130 70",
			"PADDING":     "10x5",
			"FRAMECOLOR":  "200 130 70",
			"TITLECOLOR":  "160 80 30",
			"SLIDERCOLOR": "245 210 175",
		},
	},
}

var (
	dlg          iup.Ihandle
	contentBox   iup.Ihandle
	currentTheme int
)

func applyTheme(idx int) {
	currentTheme = idx
	t := themes[idx]

	th := iup.User()
	for k, v := range t.attrs {
		th.SetAttribute(k, v)
	}
	iup.SetHandle("_FlatTheme", th)
	iup.SetGlobal("DEFAULTTHEME", "_FlatTheme")
}

func buildContent() iup.Ihandle {
	btn1 := iup.FlatButton("FlatButton")
	btn2 := iup.FlatButton("Rounded")
	btn2.SetAttribute("CORNERRADIUS", "8")
	btn3 := iup.FlatButton("Pill Shape")
	btn3.SetAttribute("CORNERRADIUS", "20")

	toggle1 := iup.FlatToggle("Option A")
	toggle2 := iup.FlatToggle("Option B")
	toggle2.SetAttribute("VALUE", "ON")
	toggle3 := iup.FlatToggle("Option C")

	label1 := iup.FlatLabel("FlatLabel with padding")
	label1.SetAttribute("PADDING", "8x4")
	label2 := iup.FlatLabel("Another label")
	label2.SetAttribute("PADDING", "8x4")

	slider := iup.FlatVal("HORIZONTAL")
	slider.SetAttributes("EXPAND=HORIZONTAL, VALUE=0.5")

	list := iup.FlatList()
	list.SetAttributes(`1="Mercury", 2="Venus", 3="Earth", 4="Mars", 5="Jupiter"`)
	list.SetAttributes("EXPAND=HORIZONTAL, VISIBLELINES=5, VALUE=3")

	frameContent := iup.Vbox(
		iup.FlatLabel("Content inside a FlatFrame"),
		iup.FlatButton("Framed Button"),
	).SetAttributes("MARGIN=8x8, GAP=5")
	frame := iup.FlatFrame(frameContent)
	frame.SetAttributes("TITLE=FlatFrame, EXPAND=HORIZONTAL")

	leftCol := iup.Vbox(
		iup.FlatLabel("Buttons").SetAttribute("FONTSTYLE", "Bold"),
		iup.Hbox(btn1, btn2, btn3).SetAttribute("GAP", "8"),
		iup.Space().SetAttribute("SIZE", "x8"),
		iup.FlatLabel("Toggles").SetAttribute("FONTSTYLE", "Bold"),
		iup.Hbox(toggle1, toggle2, toggle3).SetAttribute("GAP", "8"),
		iup.Space().SetAttribute("SIZE", "x8"),
		iup.FlatLabel("Labels").SetAttribute("FONTSTYLE", "Bold"),
		label1,
		label2,
	).SetAttributes("GAP=4, EXPAND=HORIZONTAL")

	rightCol := iup.Vbox(
		iup.FlatLabel("Slider").SetAttribute("FONTSTYLE", "Bold"),
		slider,
		iup.Space().SetAttribute("SIZE", "x8"),
		iup.FlatLabel("List").SetAttribute("FONTSTYLE", "Bold"),
		list,
		iup.Space().SetAttribute("SIZE", "x8"),
		frame,
	).SetAttributes("GAP=4, EXPAND=YES")

	return iup.Hbox(leftCol, rightCol).SetAttributes("GAP=20, EXPAND=YES")
}

func switchTheme(idx int) {
	child := iup.GetChild(contentBox, 0)
	if child != 0 {
		iup.Detach(child)
		child.Destroy()
	}

	applyTheme(idx)
	content := buildContent()
	iup.Append(contentBox, content)
	iup.Map(content)
	iup.Refresh(dlg)

	t := themes[idx]
	if bg, ok := t.attrs["BGCOLOR"]; ok {
		dlg.SetAttribute("BGCOLOR", bg)
	} else {
		dlg.SetAttribute("BGCOLOR", "")
	}

	dlg.SetAttribute("TITLE", fmt.Sprintf("Flat Theme - %s", t.name))
}

func main() {
	iup.Open()
	defer iup.Close()
	iup.ControlsOpen()

	var themeButtons []iup.Ihandle
	for i, t := range themes {
		idx := i
		btn := iup.Button(t.name)
		btn.SetAttribute("PADDING", "10x4")
		iup.SetCallback(btn, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
			switchTheme(idx)
			return iup.DEFAULT
		}))
		themeButtons = append(themeButtons, btn)
	}

	selectorBox := iup.Hbox(themeButtons...).SetAttribute("GAP", "5")
	selectorFrame := iup.Frame(
		iup.Vbox(
			iup.Label("Select a theme to see flat controls update:"),
			selectorBox,
		).SetAttributes("MARGIN=5x5, GAP=5"),
	).SetAttributes("TITLE=Theme Selector, EXPAND=HORIZONTAL")

	contentBox = iup.Vbox().SetAttribute("EXPAND", "YES")

	dlg = iup.Dialog(
		iup.Vbox(
			selectorFrame,
			contentBox,
		).SetAttributes("MARGIN=10x10, GAP=10"),
	)
	dlg.SetAttributes("TITLE=Flat Theme, SIZE=500x400")

	switchTheme(0)

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}
