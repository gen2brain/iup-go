package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	dlg := iup.ColorDlg().SetAttributes(map[string]interface{}{
		"TITLE":          "ColorDlg",
		"VALUE":          "128 0 255",
		"ALPHA":          "142",
		"SHOWHEX":        "YES",
		"SHOWCOLORTABLE": "YES",
	})
	defer dlg.Destroy()

	iup.Popup(dlg, iup.CENTER, iup.CENTER)

	if iup.GetInt(dlg, "STATUS") == 1 {
		fmt.Println("VALUE", iup.GetAttribute(dlg, "VALUE"))
		fmt.Println("VALUEHEX", iup.GetAttribute(dlg, "VALUEHEX"))
		fmt.Println("COLORTABLE", iup.GetAttribute(dlg, "COLORTABLE"))
	}
}
