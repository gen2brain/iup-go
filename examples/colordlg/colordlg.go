package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	dlg := iup.ColorDlg().SetAttributes(map[string]interface{}{
		"TITLE":          "ColorDlg",
		"VALUE":          "128 0 255",
		"ALPHA":          "142",
		"SHOWHEX":        "YES",
		"SHOWCOLORTABLE": "YES",
		"SHOWHELP":       "YES",
	})
	defer dlg.Destroy()

	dlg.SetCallback("COLORUPDATE_CB", iup.ColorUpdateFunc(func(ih iup.Ihandle) int {
		fmt.Println("COLORUPDATE_CB", ih.GetAttribute("VALUE"), "HSI", ih.GetAttribute("VALUEHSI"))
		return iup.DEFAULT
	}))
	dlg.SetCallback("HELP_CB", iup.HelpFunc(func(ih iup.Ihandle) int {
		fmt.Println("HELP_CB")
		return iup.DEFAULT
	}))

	iup.Popup(dlg, iup.CENTER, iup.CENTER)

	if iup.GetInt(dlg, "STATUS") == 1 {
		fmt.Println("VALUE", iup.GetAttribute(dlg, "VALUE"))
		fmt.Println("VALUEHEX", iup.GetAttribute(dlg, "VALUEHEX"))
		fmt.Println("VALUEHSI", iup.GetAttribute(dlg, "VALUEHSI"))
		fmt.Println("COLORTABLE", iup.GetAttribute(dlg, "COLORTABLE"))
	}
}
