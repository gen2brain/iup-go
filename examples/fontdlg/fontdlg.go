package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	dlg := iup.FontDlg()
	defer dlg.Destroy()

	iup.Popup(dlg, iup.CENTER, iup.CENTER)

	fmt.Println(dlg.GetAttribute("VALUE"))
}
