//go:build web

package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	iup.WebBrowserOpen()

	web := iup.WebBrowser()

	txtScript := iup.Text()
	txtScript.SetAttribute("EXPAND", "HORIZONTAL")
	txtScript.SetAttribute("VALUE", "document.title")

	txtResult := iup.Text()
	txtResult.SetAttribute("EXPAND", "HORIZONTAL")
	txtResult.SetAttribute("READONLY", "YES")

	btRun := iup.Button("Run")
	btRun.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		script := txtScript.GetAttribute("VALUE")
		web.SetAttribute("JAVASCRIPT", script)
		result := web.GetAttribute("JAVASCRIPT")
		txtResult.SetAttribute("VALUE", result)
		fmt.Printf("JS: %s => %s\n", script, result)
		return iup.DEFAULT
	}))

	btTitle := iup.Button("Get Title")
	btTitle.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		web.SetAttribute("JAVASCRIPT", "document.title")
		txtResult.SetAttribute("VALUE", web.GetAttribute("JAVASCRIPT"))
		return iup.DEFAULT
	}))

	btCalc := iup.Button("1 + 2")
	btCalc.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		web.SetAttribute("JAVASCRIPT", "1 + 2")
		txtResult.SetAttribute("VALUE", web.GetAttribute("JAVASCRIPT"))
		return iup.DEFAULT
	}))

	btHeading := iup.Button("Get Heading")
	btHeading.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		web.SetAttribute("JAVASCRIPT", "document.getElementById('heading').innerText")
		txtResult.SetAttribute("VALUE", web.GetAttribute("JAVASCRIPT"))
		return iup.DEFAULT
	}))

	btModify := iup.Button("Modify Page")
	btModify.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		web.SetAttribute("JAVASCRIPT", "document.getElementById('info').innerText = 'Modified by JAVASCRIPT attribute!'; 'done'")
		txtResult.SetAttribute("VALUE", web.GetAttribute("JAVASCRIPT"))
		return iup.DEFAULT
	}))

	toolbar := iup.Hbox(
		iup.Label("Script:"), txtScript, btRun,
	)
	toolbar.SetAttribute("ALIGNMENT", "ACENTER")
	toolbar.SetAttribute("GAP", "4")

	buttons := iup.Hbox(btTitle, btCalc, btHeading, btModify)
	buttons.SetAttribute("GAP", "4")

	resultBox := iup.Hbox(iup.Label("Result:"), txtResult)
	resultBox.SetAttribute("ALIGNMENT", "ACENTER")
	resultBox.SetAttribute("GAP", "4")

	vbox := iup.Vbox(toolbar, buttons, resultBox, web)
	vbox.SetAttribute("MARGIN", "8x8")
	vbox.SetAttribute("GAP", "6")

	dlg := iup.Dialog(vbox)
	dlg.SetAttribute("TITLE", "JAVASCRIPT Attribute Demo")
	dlg.SetAttribute("RASTERSIZE", "800x600")

	iup.Show(dlg)

	web.SetAttribute("HTML", `<html>
<head><title>JAVASCRIPT Attribute Test</title></head>
<body>
<h1 id="heading">Hello from IUP</h1>
<p id="info">This page tests the JAVASCRIPT attribute.</p>
</body>
</html>`)

	iup.MainLoop()
}
