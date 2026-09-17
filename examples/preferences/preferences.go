package main

import (
	"fmt"
	"strconv"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

type setting struct {
	group string
	key   string
	field iup.Ihandle
	value func(iup.Ihandle) string
	apply func(iup.Ihandle, string)
}

var (
	settings []*setting
	config   iup.Ihandle
	dirty    bool

	pages = []string{"General", "Appearance", "Editor", "Network"}
)

func main() {
	iup.Open()
	defer iup.Close()
	iup.SetGlobal("UTF8MODE", "YES")

	config = iup.Config()
	config.SetAttribute("APP_NAME", "iup_preferences")
	iup.ConfigLoad(config)

	tree := iup.Tree().SetAttributes("ADDROOT=NO, VISIBLELINES=8, VISIBLECOLUMNS=12").
		SetHandle("pf_tree")
	if phone() {
		tree.SetAttributes(fmt.Sprintf("VISIBLELINES=%d, EXPAND=HORIZONTAL", len(pages)))
	}
	tree.SetCallback("SELECTION_CB", iup.SelectionFunc(selected))

	zbox := iup.Zbox(general(), appearance(), editor(), network()).
		SetAttributes("ALIGNMENT=NW, EXPAND=YES").SetHandle("pf_zbox")

	status := iup.Label("").SetAttributes("EXPAND=HORIZONTAL, PADDING=8x4").SetHandle("pf_status")

	ok := button("OK", func() { save(); iup.ExitLoop() })
	apply := button("Apply", save)
	reset := button("Reset", reload)
	iup.Normalizer(ok, apply, reset).SetAttribute("NORMALIZE", "HORIZONTAL")

	buttons := iup.Hbox(iup.Fill(), reset, apply, ok).SetAttributes("NGAP=6, NMARGIN=8x8")

	categories := iup.Vbox(iup.Label("Categories").SetAttributes("PADDING=6x4, FONTSTYLE=Bold"), tree)

	var content iup.Ihandle
	if phone() {
		content = iup.Vbox(categories, zbox)
	} else {
		content = iup.Hbox(categories, zbox)
	}
	content.SetAttributes("NGAP=8, NMARGIN=8x8")

	dlg := iup.Dialog(iup.Vbox(
		content,
		iup.Label("").SetAttribute("SEPARATOR", "HORIZONTAL"),
		buttons,
		status,
	))
	dlg.SetAttribute("TITLE", "Preferences")

	iup.Show(dlg)
	fill(tree)
	reload()

	iup.MainLoop()
}

func phone() bool {
	switch iup.GetGlobal("SYSTEM") {
	case "Android", "iOS":
		return true
	}
	return false
}

func fill(tree iup.Ihandle) {
	prev := -1
	for _, name := range pages {
		if prev < 0 {
			tree.SetAttribute("ADDLEAF-1", name)
		} else {
			tree.SetAttribute(fmt.Sprintf("INSERTLEAF%d", prev), name)
		}
		prev = tree.GetInt("LASTADDNODE")
	}
	tree.SetAttribute("VALUE", "0")
}

func selected(_ iup.Ihandle, id, state int) int {
	if state == 1 {
		iup.GetHandle("pf_zbox").SetAttribute("VALUEPOS", id)
		setStatus(pages[id] + " settings")
	}
	return iup.DEFAULT
}

func general() iup.Ihandle {
	return page(
		"Your name", text("general", "name", "Milan"),
		"Language", list("general", "language", []string{"English", "Français", "Ελληνικά"}),
		"Recent files", spin("general", "recent", 1, 20, 8),
		"", toggle("general", "minimized", "Start minimized", "OFF"),
		"", toggle("general", "updates", "Check for updates", "ON"),
	)
}

func appearance() iup.Ihandle {
	return page(
		"Theme", list("appearance", "theme", []string{"System", "Light", "Dark"}),
		"Accent", color("appearance", "accent", "70 130 200"),
		"Opacity", slider("appearance", "opacity", 40, 100, 100),
		"Font size", spin("appearance", "fontsize", 8, 24, 11),
		"", toggle("appearance", "animations", "Enable animations", "ON"),
	)
}

func editor() iup.Ihandle {
	return page(
		"Tab size", spin("editor", "tabsize", 2, 8, 4),
		"Ruler column", slider("editor", "ruler", 60, 140, 100),
		"", toggle("editor", "wrap", "Wrap long lines", "OFF"),
		"", toggle("editor", "numbers", "Show line numbers", "ON"),
		"", toggle("editor", "autosave", "Autosave on focus loss", "OFF"),
	)
}

func network() iup.Ihandle {
	return page(
		"Proxy host", text("network", "host", "proxy.example.org"),
		"Proxy port", spin("network", "port", 1, 9999, 8080),
		"Timeout", list("network", "timeout", []string{"5 s", "15 s", "30 s", "60 s"}),
		"", toggle("network", "proxy", "Use a proxy", "OFF"),
		"", toggle("network", "offline", "Work offline", "OFF"),
	)
}

func page(rows ...interface{}) iup.Ihandle {
	grid := iup.GridBox().SetAttributes(`ORIENTATION=HORIZONTAL, NUMDIV=2, SIZECOL=-1, SIZELIN=-1,
		ALIGNMENTLIN=ACENTER, NGAPLIN=8, NGAPCOL=10, NMARGIN=10x10`)

	for i := 0; i < len(rows); i += 2 {
		label := rows[i].(string)
		iup.Append(grid, iup.Label(label).SetAttribute("ALIGNMENT", "ARIGHT"))
		iup.Append(grid, rows[i+1].(iup.Ihandle))
	}

	return grid
}

func remember(s *setting) iup.Ihandle {
	settings = append(settings, s)
	return s.field
}

func text(group, key, def string) iup.Ihandle {
	t := iup.Text().SetAttributes("EXPAND=HORIZONTAL, VISIBLECOLUMNS=12")
	t.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(changed))
	return remember(&setting{group, key, t,
		func(ih iup.Ihandle) string { return ih.GetAttribute("VALUE") },
		func(ih iup.Ihandle, v string) {
			if v == "" {
				v = def
			}
			ih.SetAttribute("VALUE", v)
		}})
}

func toggle(group, key, title, def string) iup.Ihandle {
	t := iup.Toggle(title)
	t.SetCallback("ACTION", iup.ToggleActionFunc(func(iup.Ihandle, int) int { changed(t); return iup.DEFAULT }))
	return remember(&setting{group, key, t,
		func(ih iup.Ihandle) string { return ih.GetAttribute("VALUE") },
		func(ih iup.Ihandle, v string) {
			if v == "" {
				v = def
			}
			ih.SetAttribute("VALUE", v)
		}})
}

func list(group, key string, items []string) iup.Ihandle {
	l := iup.List().SetAttributes("DROPDOWN=YES, VISIBLEITEMS=5")
	for i, item := range items {
		iup.SetAttributeId(l, "", i+1, item)
	}
	l.SetCallback("ACTION", iup.ListActionFunc(func(iup.Ihandle, string, int, int) int {
		changed(l)
		return iup.DEFAULT
	}))
	return remember(&setting{group, key, l,
		func(ih iup.Ihandle) string { return ih.GetAttribute("VALUE") },
		func(ih iup.Ihandle, v string) {
			if v == "" {
				v = "1"
			}
			ih.SetAttribute("VALUE", v)
		}})
}

func spin(group, key string, min, max, def int) iup.Ihandle {
	t := iup.Text().SetAttributes(fmt.Sprintf("SPIN=YES, SPINMIN=%d, SPINMAX=%d, VISIBLECOLUMNS=5", min, max))
	t.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(changed))
	return remember(&setting{group, key, t,
		func(ih iup.Ihandle) string { return ih.GetAttribute("VALUE") },
		func(ih iup.Ihandle, v string) {
			n, err := strconv.Atoi(v)
			if err != nil || n < min || n > max {
				n = def
			}
			ih.SetAttribute("SPINVALUE", n)
		}})
}

func slider(group, key string, min, max, def int) iup.Ihandle {
	val := iup.Val("HORIZONTAL").SetAttributes(fmt.Sprintf("MIN=%d, MAX=%d, EXPAND=HORIZONTAL", min, max))
	shown := iup.Label(strconv.Itoa(def)).SetAttributes("VISIBLECOLUMNS=4")

	val.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		shown.SetAttribute("TITLE", strconv.Itoa(int(ih.GetFloat("VALUE"))))
		changed(ih)
		return iup.DEFAULT
	}))

	remember(&setting{group, key, val,
		func(ih iup.Ihandle) string { return strconv.Itoa(int(ih.GetFloat("VALUE"))) },
		func(ih iup.Ihandle, v string) {
			n, err := strconv.Atoi(v)
			if err != nil || n < min || n > max {
				n = def
			}
			ih.SetAttribute("VALUE", n)
			shown.SetAttribute("TITLE", strconv.Itoa(n))
		}})

	return iup.Hbox(val, shown).SetAttributes("NGAP=6, ALIGNMENT=ACENTER")
}

func color(group, key, def string) iup.Ihandle {
	swatch := iup.Label(" ").SetAttributes("RASTERSIZE=48x18, EXPAND=NO")
	pick := iup.Button("Choose...")

	pick.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		if col, ret := iup.GetColor(iup.CENTERPARENT, iup.CENTERPARENT); ret != 0 {
			swatch.SetAttribute("BGCOLOR", fmt.Sprintf("%d %d %d", col.R, col.G, col.B))
			changed(swatch)
		}
		return iup.DEFAULT
	}))

	remember(&setting{group, key, swatch,
		func(ih iup.Ihandle) string { return ih.GetAttribute("BGCOLOR") },
		func(ih iup.Ihandle, v string) {
			if v == "" {
				v = def
			}
			ih.SetAttribute("BGCOLOR", v)
		}})

	return iup.Hbox(swatch, pick).SetAttributes("NGAP=6, ALIGNMENT=ACENTER")
}

func changed(iup.Ihandle) int {
	if !dirty {
		dirty = true
		setStatus("Modified, press Apply to save")
	}
	return iup.DEFAULT
}

func reload() {
	for _, s := range settings {
		s.apply(s.field, iup.ConfigGetVariableStr(config, s.group, s.key))
	}
	dirty = false
	setStatus("Loaded from " + config.GetAttribute("FILENAME"))
}

func save() {
	for _, s := range settings {
		iup.ConfigSetVariableStr(config, s.group, s.key, s.value(s.field))
	}
	iup.ConfigSave(config)
	dirty = false
	setStatus(fmt.Sprintf("Saved %d settings to %s", len(settings), config.GetAttribute("FILENAME")))
}

func setStatus(s string) {
	iup.GetHandle("pf_status").SetAttribute("TITLE", s)
}

func button(title string, action func()) iup.Ihandle {
	b := iup.Button(title).SetAttributes("PADDING=10x4")
	b.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		action()
		return iup.DEFAULT
	}))
	return b
}
