package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	iup.Show(
		iup.Dialog(
			iup.MultiLine().SetCallback("ACTION", iup.TextActionFunc(mlAction)).SetAttributes(map[string]string{
				"EXPAND": "YES",
				"BORDER": "YES",
				"VALUE":  "I ignore the \"g\" key!",
			}),
		).SetAttributes(`TITLE=Multiline, SIZE=QUARTERxQUARTER`),
	)

	iup.MainLoop()
}

func mlAction(self iup.Ihandle, c int, newValue string) int {
	if c == iup.K_g {
		return iup.IGNORE
	}
	return iup.DEFAULT
}
