package main

import (
	"github.com/gen2brain/iup-go/iup"
)

var (
	dlg2 iup.Ihandle
)

func main() {
	iup.Open()
	defer iup.Close()

	list := iup.List().SetHandle("list").SetAttribute("DROPDOWN", "YES")

	iup.SetAttribute(list, "1", "Helvetica, 8")
	iup.SetAttribute(list, "2", "Courier, 8")
	iup.SetAttribute(list, "3", "Times, 8")
	iup.SetAttribute(list, "4", "Helvetica, Italic 8")
	iup.SetAttribute(list, "5", "Courier, Italic 8")
	iup.SetAttribute(list, "6", "Times, Italic 8")
	iup.SetAttribute(list, "7", "Helvetica, Bold 8")
	iup.SetAttribute(list, "8", "Courier, Bold 8")
	iup.SetAttribute(list, "9", "Times, Bold 8")
	iup.SetAttribute(list, "10", "Helvetica, 10")
	iup.SetAttribute(list, "11", "Courier, 10")
	iup.SetAttribute(list, "12", "Times, 10")
	iup.SetAttribute(list, "13", "Helvetica, Italic 10")
	iup.SetAttribute(list, "14", "Courier, Italic 10")
	iup.SetAttribute(list, "15", "Times, Italic 10")
	iup.SetAttribute(list, "16", "Helvetica, Bold 10")
	iup.SetAttribute(list, "17", "Courier, Bold 10")
	iup.SetAttribute(list, "18", "Times, Bold 10")
	iup.SetAttribute(list, "19", "Helvetica, 12")
	iup.SetAttribute(list, "20", "Courier, 12")
	iup.SetAttribute(list, "21", "Times, 12")
	iup.SetAttribute(list, "22", "Helvetica, Italic 12")
	iup.SetAttribute(list, "23", "Courier, Italic 12")
	iup.SetAttribute(list, "24", "Times, Italic 12")
	iup.SetAttribute(list, "25", "Helvetica, Bold 12")
	iup.SetAttribute(list, "26", "Courier, Bold 12")
	iup.SetAttribute(list, "27", "Times, Bold 12")
	iup.SetAttribute(list, "28", "Helvetica, 14")
	iup.SetAttribute(list, "29", "Courier, 14")
	iup.SetAttribute(list, "30", "Times, 14")
	iup.SetAttribute(list, "31", "Helvetica, Italic 14")
	iup.SetAttribute(list, "32", "Courier, Italic 14")
	iup.SetAttribute(list, "33", "Times, Italic 14")
	iup.SetAttribute(list, "34", "Helvetica, Bold 14")
	iup.SetAttribute(list, "35", "Courier, Bold 14")
	iup.SetAttribute(list, "36", "Times, Bold 14")

	dlg := iup.Dialog(
		list,
	).SetAttribute("TITLE", "Fonts")

	list.SetCallback("ACTION", iup.ListActionFunc(actionCb))

	iup.Show(dlg)
	iup.MainLoop()
}

func actionCb(ih iup.Ihandle, text string, item, state int) int {
	if dlg2 != 0 {
		iup.Hide(dlg2)
	}

	if state == 1 {
		ml := iup.MultiLine().SetAttributes(`SIZE=200x200, VALUE="1234\nmmmmm\niiiii"`)
		ml.SetAttribute("FONT", text)

		dlg2 = iup.Dialog(
			ml,
		).SetAttribute("TITLE", text)

		iup.Show(dlg2)
		iup.SetFocus(iup.GetHandle("list"))
	}

	return iup.DEFAULT
}
