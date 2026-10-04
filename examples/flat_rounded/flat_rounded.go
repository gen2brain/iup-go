//go:build ctrl

package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

const (
	accent = "60 120 200"
	soft   = "230 240 255"
)

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	iup.ControlsOpen()

	button := iup.FlatButton("Button").SetAttributes(map[string]string{
		"SHOWBORDER": "YES", "BORDERCOLOR": accent, "BGCOLOR": soft, "PADDING": "10x6",
	})
	gradient := iup.FlatButton("Gradient").SetAttributes(map[string]string{
		"SHOWBORDER": "YES", "BORDERCOLOR": accent, "GRADIENT": "255 255 255:170 200 245", "PADDING": "10x6",
	})
	drop := iup.DropButton(iup.Vbox(iup.Label("Drop child")).SetAttributes("MARGIN=10x10")).SetAttributes(map[string]string{
		"TITLE": "Drop", "SHOWBORDER": "YES", "DROPONARROW": "YES", "BORDERCOLOR": accent, "BGCOLOR": soft, "PADDING": "10x6",
	})
	toggle := iup.FlatToggle("Toggle").SetAttributes(map[string]string{
		"VALUE": "ON", "CHECKSIZE": "0", "SHOWBORDER": "YES", "BORDERCOLOR": accent, "BGCOLOR": soft, "PADDING": "10x6",
	})
	check := iup.FlatToggle("Check box").SetAttributes(map[string]string{
		"VALUE": "ON", "CHECKFGCOLOR": accent,
	})

	val := iup.FlatVal("HORIZONTAL").SetAttributes(map[string]string{
		"VALUE": "0.6", "EXPAND": "HORIZONTAL", "SLIDERFILLCOLOR": accent, "BORDERCOLOR": accent, "FGCOLOR": "255 255 255",
	})
	gauge := iup.Gauge().SetAttributes(map[string]string{
		"VALUE": "0.6", "EXPAND": "HORIZONTAL", "FGCOLOR": accent,
	})

	list := iup.FlatList().SetAttributes(map[string]string{
		"1": "Inbox", "2": "Drafts", "3": "Sent", "4": "Archive", "5": "Spam", "6": "Trash",
		"VALUE": "2", "VISIBLELINES": "4", "VISIBLECOLUMNS": "10", "PADDING": "8x4", "EXPAND": "HORIZONTAL",
		"FLATSCROLLBAR": "YES", "PSCOLOR": accent, "TEXTPSCOLOR": "255 255 255", "HLCOLORALPHA": "0",
	})

	tabs := iup.FlatTabs(
		iup.Vbox(iup.Label("First page").SetAttribute("EXPAND", "HORIZONTAL")).SetAttributes("MARGIN=10x10"),
		iup.Vbox(iup.Label("Second page").SetAttribute("EXPAND", "HORIZONTAL")).SetAttributes("MARGIN=10x10"),
		iup.Vbox(iup.Label("Third page").SetAttribute("EXPAND", "HORIZONTAL")).SetAttributes("MARGIN=10x10"),
	).SetAttributes(map[string]string{
		"TABTITLE0": "First", "TABTITLE1": "Second", "TABTITLE2": "Third",
		"TABSBACKCOLOR": "235 235 235", "TABSPADDING": "10x6",
	})

	frame := iup.FlatFrame(
		iup.Vbox(
			iup.Hbox(button, gradient, drop).SetAttributes("GAP=8, ALIGNMENT=ACENTER"),
			iup.Hbox(toggle, check).SetAttributes("GAP=8, ALIGNMENT=ACENTER"),
			val, gauge, list, tabs,
		).SetAttributes("GAP=10, MARGIN=12x12"),
	).SetAttributes(map[string]string{
		"TITLE": "Rounded", "FRAMECOLOR": accent, "TITLEBGCOLOR": soft, "BGCOLOR": "255 255 255", "TITLEPADDING": "8x4",
	})

	apply := func(radius int) {
		for _, ih := range []iup.Ihandle{button, gradient, drop, toggle, val, gauge, frame} {
			ih.SetAttribute("CORNERRADIUS", radius)
		}
		check.SetAttribute("CHECKCORNERRADIUS", radius/3)
		list.SetAttribute("ITEMCORNERRADIUS", radius)
		list.SetAttribute("SB_CORNERRADIUS", radius)
		tabs.SetAttribute("TABSCORNERRADIUS", radius)
	}

	label := iup.Label("Radius: 8")
	radius := iup.Val("HORIZONTAL").SetAttributes("MIN=0, MAX=20, VALUE=8, EXPAND=HORIZONTAL")
	radius.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		r := int(ih.GetFloat("VALUE") + 0.5)
		label.SetAttribute("TITLE", fmt.Sprintf("Radius: %d", r))
		iup.Refresh(label)
		apply(r)
		return iup.DEFAULT
	}))
	apply(8)

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Hbox(label, radius).SetAttributes("GAP=10, ALIGNMENT=ACENTER"),
			frame,
		).SetAttributes("GAP=10, MARGIN=10x10"),
	).SetAttribute("TITLE", "Flat rounded")

	iup.Show(dlg)
	iup.MainLoop()
}
