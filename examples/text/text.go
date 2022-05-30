package main

import (
	"github.com/gen2brain/iup-go/iup"
)

var (
	password []byte
)

func main() {
	iup.Open()
	defer iup.Close()

	text := iup.Text()
	text.SetAttribute("SIZE", "200x")
	text.SetCallback("ACTION", iup.TextActionFunc(action))
	text.SetCallback("K_ANY", iup.KAnyFunc(kAny))

	pwd := iup.Text().SetHandle("pwd")
	pwd.SetAttribute("READONLY", "YES")
	pwd.SetAttribute("SIZE", "200x")

	dlg := iup.Dialog(
		iup.Vbox(
			text,
			pwd,
		),
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
