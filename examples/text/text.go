package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

var (
	password []byte
)

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	text := iup.Text()
	text.SetAttribute("EXPAND", "HORIZONTAL")
	text.SetCallback("ACTION", iup.TextActionFunc(action))
	text.SetCallback("K_ANY", iup.KAnyFunc(kAny))

	pwd := iup.Text().SetHandle("pwd")
	pwd.SetAttribute("READONLY", "YES")
	pwd.SetAttribute("EXPAND", "HORIZONTAL")

	multi := iup.Text().SetAttributes("MULTILINE=YES, EXPAND=HORIZONTAL, VISIBLELINES=5").SetAttribute("VALUE", "first line\nSecond Line\nthird line")
	lineValue := iup.Label("LINEVALUE: (move the caret)").SetAttribute("EXPAND", "HORIZONTAL")
	multi.SetCallback("CARET_CB", iup.CaretFunc(func(ih iup.Ihandle, lin, col, pos int) int {
		lineValue.SetAttribute("TITLE", fmt.Sprintf("LINEVALUE at %d,%d: %q", lin, col, ih.GetAttribute("LINEVALUE")))
		return iup.DEFAULT
	}))

	caseButtons := iup.Hbox()
	for _, mode := range []string{"UPPER", "LOWER", "TOGGLE", "TITLE"} {
		mode := mode
		b := iup.Button("CHANGECASE=" + mode)
		b.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
			multi.SetAttribute("CHANGECASE", mode)
			return iup.DEFAULT
		}))
		iup.Append(caseButtons, b)
	}
	caseButtons.SetAttribute("NGAP", "4")

	newline := iup.Toggle("APPENDNEWLINE").SetAttribute("VALUE", "ON")
	scroll := iup.Toggle("APPENDSCROLL").SetAttribute("VALUE", "ON")
	appendBtn := iup.Button("APPEND a line")
	appendBtn.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		multi.SetAttribute("APPENDNEWLINE", newline.GetAttribute("VALUE"))
		multi.SetAttribute("APPENDSCROLL", scroll.GetAttribute("VALUE"))
		multi.SetAttribute("APPEND", "appended line")
		return iup.DEFAULT
	}))

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Frame(iup.Vbox(text, pwd).SetAttribute("NGAP", "4")).SetAttribute("TITLE", "Password through ACTION and K_ANY"),
			iup.Frame(iup.Vbox(
				multi,
				lineValue,
				caseButtons,
				iup.Hbox(appendBtn, newline, scroll).SetAttributes("NGAP=6, ALIGNMENT=ACENTER"),
			).SetAttribute("NGAP", "4")).SetAttribute("TITLE", "Multiline"),
		).SetAttributes("NMARGIN=10x10, NGAP=8"),
	).SetAttribute("TITLE", "Text")

	iup.Show(dlg)
	iup.MainLoop()
}

func action(ih iup.Ihandle, ch int, newValue string) int {
	if ch > 0 {
		password = append(password, byte(ch))
		iup.GetHandle("pwd").SetAttribute("VALUE", string(password))
	}

	return iup.K_asterisk
}

func kAny(ih iup.Ihandle, c int) int {
	switch c {
	case iup.K_BS:
		if len(password) == 0 {
			return iup.IGNORE
		}
		password = password[:len(password)-1]
		iup.GetHandle("pwd").SetAttribute("VALUE", string(password))
	case iup.K_CR, iup.K_SP, iup.K_ESC, iup.K_INS, iup.K_DEL, iup.K_TAB, iup.K_HOME, iup.K_UP,
		iup.K_PGUP, iup.K_LEFT, iup.K_MIDDLE, iup.K_RIGHT, iup.K_END, iup.K_DOWN, iup.K_PGDN:
		return iup.IGNORE
	}

	return iup.DEFAULT
}
