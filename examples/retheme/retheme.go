// Runtime re-theming of native controls through the IUP global color palette.
package main

import (
	"fmt"
	"strconv"
	"strings"

	"github.com/gen2brain/iup-go/iup"
)

type palette struct {
	name                                                     string
	dlgBg, dlgFg, txtBg, txtFg, menuBg, menuFg, link, accent string
}

var palettes = []palette{
	{"Light", "240 240 240", "30 30 30", "255 255 255", "20 20 20", "240 240 240", "30 30 30", "0 90 200", "0 120 215"},
	{"Dark", "37 37 38", "220 220 220", "30 30 30", "230 230 230", "45 45 48", "220 220 220", "90 170 255", "0 122 204"},
	{"Solarized", "0 43 54", "131 148 150", "7 54 66", "147 161 161", "0 43 54", "131 148 150", "38 139 210", "42 161 152"},
	{"Nord", "46 52 64", "216 222 233", "59 66 82", "229 233 240", "46 52 64", "216 222 233", "136 192 208", "143 188 187"},
	{"Gruvbox", "40 40 40", "235 219 178", "50 48 47", "235 219 178", "40 40 40", "235 219 178", "131 165 152", "250 189 47"},
}

var (
	dlg          iup.Ihandle
	swatchLabels []iup.Ihandle
	currentLabel iup.Ihandle
)

// contrast returns black or white, whichever reads better on the given "r g b".
func contrast(rgb string) string {
	f := strings.Fields(rgb)
	if len(f) < 3 {
		return "0 0 0"
	}
	r, _ := strconv.Atoi(f[0])
	g, _ := strconv.Atoi(f[1])
	b, _ := strconv.Atoi(f[2])
	if (299*r+587*g+114*b)/1000 < 140 {
		return "255 255 255"
	}
	return "0 0 0"
}

func setGlobals(p palette) {
	iup.SetGlobal("DLGBGCOLOR", p.dlgBg)
	iup.SetGlobal("DLGFGCOLOR", p.dlgFg)
	iup.SetGlobal("TXTBGCOLOR", p.txtBg)
	iup.SetGlobal("TXTFGCOLOR", p.txtFg)
	iup.SetGlobal("MENUBGCOLOR", p.menuBg)
	iup.SetGlobal("MENUFGCOLOR", p.menuFg)
	iup.SetGlobal("LINKFGCOLOR", p.link)
	iup.SetGlobal("ACCENTCOLOR", p.accent)
}

func retheme(ih iup.Ihandle, p palette) {
	if ih == 0 || ih.GetAttribute("NORETHEME") == "YES" {
		return
	}
	switch iup.GetClassName(ih) {
	case "text", "list":
		ih.SetAttribute("BGCOLOR", p.txtBg)
		ih.SetAttribute("FGCOLOR", p.txtFg)
	case "link":
		ih.SetAttribute("FGCOLOR", p.link)
	default:
		ih.SetAttribute("BGCOLOR", p.dlgBg)
		ih.SetAttribute("FGCOLOR", p.dlgFg)
	}
	for i, n := 0, iup.GetChildCount(ih); i < n; i++ {
		retheme(iup.GetChild(ih, i), p)
	}
}

func applyTheme(p palette) {
	setGlobals(p)
	retheme(dlg, p)

	entries := []struct{ name, rgb string }{
		{"DLG bg", p.dlgBg}, {"DLG fg", p.dlgFg},
		{"TXT bg", p.txtBg}, {"TXT fg", p.txtFg},
		{"MENU bg", p.menuBg}, {"MENU fg", p.menuFg},
		{"LINK", p.link}, {"ACCENT", p.accent},
	}
	for i, e := range entries {
		swatchLabels[i].SetAttribute("TITLE", fmt.Sprintf(" %-8s %s ", e.name, e.rgb))
		swatchLabels[i].SetAttribute("BGCOLOR", e.rgb)
		swatchLabels[i].SetAttribute("FGCOLOR", contrast(e.rgb))
	}

	currentLabel.SetAttribute("TITLE", "Active theme: "+p.name)
	iup.Refresh(dlg)
	iup.Redraw(dlg, 1)
}

func group(title string, child iup.Ihandle) iup.Ihandle {
	return iup.Frame(iup.Vbox(
		iup.Label(title).SetAttributes(`FONT="Sans, Bold 10"`),
		child,
	).SetAttributes("GAP=8, MARGIN=8x8"))
}

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	themeList := iup.List().SetAttributes("EXPAND=HORIZONTAL")
	for i, p := range palettes {
		themeList.SetAttribute(strconv.Itoa(i+1), p.name)
	}
	themeList.SetAttributes(fmt.Sprintf("VISIBLELINES=%d, VALUE=1", len(palettes)))
	themeList.SetCallback("ACTION", iup.ListActionFunc(func(ih iup.Ihandle, text string, item, state int) int {
		if state == 1 {
			applyTheme(palettes[item-1])
		}
		return iup.DEFAULT
	}))
	picker := group("Themes", themeList)

	for i := 0; i < 8; i++ {
		lbl := iup.Label("").SetAttributes("EXPAND=HORIZONTAL")
		lbl.SetAttribute("NORETHEME", "YES")
		swatchLabels = append(swatchLabels, lbl)
	}
	swatchFrame := group("Palette", iup.Vbox(swatchLabels...).SetAttribute("GAP", "2"))

	nameText := iup.Text().SetAttributes("VISIBLECOLUMNS=22")
	nameText.SetAttribute("VALUE", "Ada Lovelace")

	notes := iup.Text().SetAttributes("MULTILINE=YES, VISIBLELINES=3, VISIBLECOLUMNS=22, WORDWRAP=YES")
	notes.SetAttribute("VALUE", "Notes re-theme through TXTBGCOLOR / TXTFGCOLOR.")

	list := iup.List().SetAttributes("VISIBLELINES=3, EXPAND=HORIZONTAL")
	list.SetAttributes(map[string]string{"1": "Automatic", "2": "Manual", "3": "Scheduled", "VALUE": "1"})

	preview := group("Preview", iup.Vbox(
		iup.Label("Native controls follow the palette."),
		iup.Label("Name:"),
		nameText,
		iup.Label("Notes:"),
		notes,
		iup.Label("Sync mode:"),
		list,
		iup.Hbox(
			iup.Toggle("Enable sync").SetAttribute("VALUE", "ON"),
			iup.Toggle("Notify"),
		).SetAttribute("GAP", "12"),
		iup.Link("https://iup-go", "Learn more about theming"),
	).SetAttributes("GAP=6"))

	currentLabel = iup.Label("Active theme: Light").SetAttributes(`FONT="Sans, Bold 10"`)

	dlg = iup.Dialog(iup.Vbox(
		iup.Hbox(
			iup.Vbox(picker, swatchFrame).SetAttributes("GAP=10"),
			preview,
		).SetAttributes("GAP=10, ALIGNMENT=ATOP"),
		currentLabel,
	).SetAttributes("MARGIN=12x12, GAP=10")).SetAttribute("TITLE", "Global Palette Re-theme")

	applyTheme(palettes[0])
	iup.Show(dlg)
	iup.MainLoop()
}
