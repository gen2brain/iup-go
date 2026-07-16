package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

// darkScroll wraps a ScrollBox in a BackgroundBox with a forced dark background.
func darkScroll() iup.Ihandle {
	var items []iup.Ihandle
	for i := 1; i <= 30; i++ {
		items = append(items, iup.Label(fmt.Sprintf("Item %d", i)).SetAttribute("FGCOLOR", "220 220 220"))
	}
	scroll := iup.ScrollBox(iup.Vbox(items...).SetAttributes(`NMARGIN=14x14, NGAP=10`)).SetAttribute("EXPAND", "YES")
	bg := iup.BackgroundBox(scroll)
	bg.SetAttributes(`BGCOLOR="36 36 36", EXPAND=YES`)
	bg.SetAttribute("TABTITLE", "BgBox>Scroll")
	return bg
}

// scrollGrid puts a real GridBox of controls inside a ScrollBox viewport.
func scrollGrid() iup.Ihandle {
	var cells []iup.Ihandle
	for i := 1; i <= 48; i++ {
		cells = append(cells, iup.Button(fmt.Sprintf("Cell %d", i)))
	}
	grid := iup.GridBox(cells...).SetAttributes(`ORIENTATION=HORIZONTAL, NUMDIV=4, SIZELIN=-1, SIZECOL=-1, NMARGIN=10x10, NGAP=6`)
	sb := iup.ScrollBox(grid).SetAttribute("EXPAND", "YES")
	sb.SetAttribute("TABTITLE", "Scroll>Grid")
	return sb
}

// nestedSplit builds a Split whose first pane is another Split (nested, both orientations).
func nestedSplit() iup.Ihandle {
	inner := iup.Split(
		iup.MultiLine().SetAttributes(`EXPAND=YES, VALUE="left pane"`),
		iup.MultiLine().SetAttributes(`EXPAND=YES, VALUE="right pane"`),
	).SetAttribute("ORIENTATION", "VERTICAL")
	outer := iup.Split(
		inner,
		iup.MultiLine().SetAttributes(`EXPAND=YES, VALUE="bottom pane"`),
	).SetAttribute("ORIENTATION", "HORIZONTAL")
	outer.SetAttribute("TABTITLE", "Nested Split")
	return outer
}

// expanderScroll nests a ScrollBox inside each Expander body.
func expanderScroll() iup.Ihandle {
	mkExp := func(title string, open bool) iup.Ihandle {
		var items []iup.Ihandle
		for i := 1; i <= 12; i++ {
			items = append(items, iup.Label(fmt.Sprintf("%s row %d", title, i)))
		}
		body := iup.ScrollBox(iup.Vbox(items...).SetAttributes(`NMARGIN=8x8, NGAP=6`)).SetAttributes(`EXPAND=YES, RASTERSIZE=x120`)
		exp := iup.Expander(body)
		exp.SetAttributes(fmt.Sprintf(`TITLE="%s", EXPAND=HORIZONTAL`, title))
		if open {
			exp.SetAttribute("STATE", "OPEN")
		}
		return exp
	}
	vbox := iup.Vbox(
		mkExp("Alpha", true),
		mkExp("Beta", false),
		mkExp("Gamma", false),
	).SetAttributes(`NMARGIN=6x6, NGAP=6`)
	vbox.SetAttribute("TABTITLE", "Expander>Scroll")
	return vbox
}

func main() {
	iup.Open()
	defer iup.Close()

	tabs := iup.Tabs(
		darkScroll(),
		scrollGrid(),
		nestedSplit(),
		expanderScroll(),
	)

	dlg := iup.Dialog(tabs).SetAttributes(`TITLE="Nested containers", SIZE=HALFxHALF`)

	iup.Show(dlg)
	iup.MainLoop()
}
