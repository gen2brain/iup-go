package main

import (
	"fmt"
	"strings"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

type property struct {
	format string
	value  string
}

type category struct {
	title string
	props []property
}

type object struct {
	name       string
	categories []category
}

var objects = []object{
	{"Button", []category{
		{"Appearance", []property{
			{"Title%s\n", "OK"},
			{"Font%n\n", "Sans, 10"},
			{"FgColor%c\n", "0 0 0"},
			{"BgColor%c\n", "224 224 224"},
			{"Image%f[OPEN|*.png;*.ico]\n", ""},
		}},
		{"Layout", []property{
			{"Expand%l|NO|HORIZONTAL|VERTICAL|YES|\n", "0"},
			{"Padding%i\n", "8"},
			{"Alignment%o|ALEFT|ACENTER|ARIGHT|\n", "1"},
		}},
		{"Behavior", []property{
			{"Active%b[No,Yes]\n", "1"},
			{"Visible%b[No,Yes]\n", "1"},
			{"Tip%s{Text shown when the mouse rests over the button}\n", ""},
		}},
	}},
	{"Text", []category{
		{"Appearance", []property{
			{"Value%s\n", "sample text"},
			{"Font%n\n", "Sans, 10"},
			{"FgColor%c\n", "0 0 0"},
		}},
		{"Layout", []property{
			{"VisibleColumns%i\n", "20"},
			{"VisibleLines%i\n", "1"},
			{"Expand%l|NO|HORIZONTAL|VERTICAL|YES|\n", "1"},
		}},
		{"Behavior", []property{
			{"Active%b[No,Yes]\n", "1"},
			{"ReadOnly%b[No,Yes]\n", "0"},
			{"Multiline%b[No,Yes]\n", "0"},
			{"Mask%s\n", ""},
		}},
	}},
	{"Dialog", []category{
		{"Appearance", []property{
			{"Title%s\n", "My Dialog"},
			{"BgColor%c\n", "240 240 240"},
			{"Icon%f[OPEN|*.png;*.ico]\n", ""},
		}},
		{"Layout", []property{
			{"Size%s\n", "300x200"},
			{"Margin%i\n", "10"},
			{"Gap%i\n", "5"},
		}},
		{"Behavior", []property{
			{"Resize%b[No,Yes]\n", "1"},
			{"MaxBox%b[No,Yes]\n", "1"},
			{"MinBox%b[No,Yes]\n", "1"},
			{"FullScreen%b[No,Yes]\n", "0"},
		}},
	}},
}

var expanded = map[string]bool{}

func paramName(title string, index int) string {
	return fmt.Sprintf("PARAM_%s_%d", title, index)
}

func stateKey(name, title string) string {
	return name + "." + title
}

func currentObject() object {
	return objects[iup.GetInt(iup.GetHandle("OBJECTS"), "VALUE")-1]
}

func values(box iup.Ihandle) string {
	var sb strings.Builder
	title := box.GetAttribute("CATEGORY")

	for i := 0; i < iup.GetInt(box, "PARAMCOUNT"); i++ {
		param := iup.GetHandle(paramName(title, i))
		sb.WriteString(fmt.Sprintf("%s = %s\n", param.GetAttribute("TITLE"), param.GetAttribute("VALUE")))
	}

	return sb.String()
}

func paramAction(box iup.Ihandle, index int) int {
	switch index {
	case iup.GETPARAM_BUTTON1:
		iup.Message(box.GetAttribute("CATEGORY"), values(box))
	case iup.GETPARAM_BUTTON2:
		iup.PostMessage(iup.GetDialog(box), "RESET", 0, nil)
	default:
		return 1
	}

	return 0
}

func newCategory(c category, open bool) iup.Ihandle {
	params := []iup.Ihandle{iup.Param("%x[NOFRAME=YES]\n")}

	for i, p := range c.props {
		param := iup.Param(p.format).SetHandle(paramName(c.title, i))
		if p.value != "" {
			param.SetAttribute("VALUE", p.value)
		}
		params = append(params, param)
	}

	box := iup.ParamBox(params...)
	box.SetAttribute("CATEGORY", c.title)
	box.SetCallback("PARAM_CB", iup.ParamFunc(paramAction))

	exp := iup.Expander(box).SetAttributes(`EXPAND=HORIZONTAL, TITLEEXPAND=YES`)
	exp.SetAttribute("TITLE", c.title)
	if !open {
		exp.SetAttribute("STATE", "CLOSE")
	}

	return exp
}

func showProperties(obj object) {
	pane := iup.GetHandle("PANE")
	mapped := iup.GetDialog(pane).GetAttribute("WID") != ""
	previous := pane.GetAttribute("OBJECT")

	for iup.GetChildCount(pane) > 0 {
		exp := iup.GetChild(pane, 0)
		expanded[stateKey(previous, exp.GetAttribute("TITLE"))] = exp.GetAttribute("STATE") == "OPEN"
		iup.Destroy(exp)
	}

	pane.SetAttribute("OBJECT", obj.name)

	for i, c := range obj.categories {
		open, seen := expanded[stateKey(obj.name, c.title)]
		if !seen {
			open = i == 0
		}

		exp := newCategory(c, open)
		iup.Append(pane, exp)
		if mapped {
			iup.Map(exp)
		}
	}

	if mapped {
		iup.Refresh(pane)
	}
}

func main() {
	iup.Open()
	defer iup.Close()

	pane := iup.Vbox().SetAttributes(`NMARGIN=5x5, NGAP=5`).SetHandle("PANE")
	scroll := iup.ScrollBox(pane).SetAttributes(`EXPAND=YES`)

	list := iup.List().SetAttributes(`EXPAND=VERTICAL, VISIBLECOLUMNS=8`).SetHandle("OBJECTS")
	for i, o := range objects {
		iup.SetAttributeId(list, "", i+1, o.name)
	}
	list.SetAttribute("VALUE", "1")

	list.SetCallback("ACTION", iup.ListActionFunc(func(_ iup.Ihandle, _ string, item, state int) int {
		if state == 1 {
			showProperties(objects[item-1])
		}
		return iup.DEFAULT
	}))

	dlg := iup.Dialog(
		iup.Hbox(list, scroll).SetAttributes(`NMARGIN=10x10, NGAP=10`),
	).SetAttributes(`TITLE="Property Grid"`)

	dlg.SetCallback("POSTMESSAGE_CB", iup.PostMessageFunc(func(iup.Ihandle, string, int, any) int {
		showProperties(currentObject())
		return iup.DEFAULT
	}))

	showProperties(objects[0])

	iup.Show(dlg)
	iup.MainLoop()
}
