package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	theme := iup.User()
	theme.SetAttribute("BGCOLOR", "220 230 240")
	theme.SetAttribute("FONT", "Sans, 11")

	iup.SetHandle("MyTheme", theme)
	iup.SetGlobal("DEFAULTTHEME", "MyTheme")

	btn1 := iup.FlatButton("Themed FlatButton 1")
	btn2 := iup.FlatButton("Themed FlatButton 2")
	btn3 := iup.FlatButton("Themed FlatButton 3")

	text1 := iup.Text().SetAttribute("VALUE", "Themed text field")
	text2 := iup.MultiLine()
	text2.SetAttribute("VALUE", "Themed multiline")
	text2.SetAttribute("VISIBLELINES", "3")

	label1 := iup.FlatLabel("This FlatLabel inherits theme BGCOLOR")
	label2 := iup.FlatLabel("Background color: 220 230 240")

	noThemeBtn := iup.FlatButton("FlatButton without theme")
	noThemeBtn.SetAttribute("BGCOLOR", "255 255 255")

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Frame(
				iup.Vbox(
					iup.Label("DEFAULTTHEME applies attributes to all new widgets."),
					iup.Label("Theme sets: BGCOLOR='220 230 240', FONT='Sans, 11'"),
				).SetAttribute("GAP", "5"),
			).SetAttribute("TITLE", "About Themes"),

			iup.Frame(
				iup.Vbox(
					btn1, btn2, btn3,
					iup.Space().SetAttribute("SIZE", "x5"),
					text1,
					text2,
					iup.Space().SetAttribute("SIZE", "x5"),
					label1,
					label2,
				).SetAttribute("GAP", "5"),
			).SetAttribute("TITLE", "Themed Controls"),

			iup.Frame(
				iup.Vbox(
					noThemeBtn,
					iup.Label("Override theme by setting attributes directly."),
				).SetAttribute("GAP", "5"),
			).SetAttribute("TITLE", "Overriding Theme"),
		).SetAttributes("MARGIN=10x10, GAP=10"),
	).SetAttribute("TITLE", "Theme Example")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}
