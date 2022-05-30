package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	bt := iup.Button("Detach Me!").SetHandle("detach").SetCallback("ACTION", iup.ActionFunc(btnDetachCb))
	ml := iup.MultiLine().SetAttributes(`EXPAND=YES, VISIBLELINES=5`)
	hbox := iup.Hbox(bt, ml).SetAttributes(`MARGIN=10x0`)

	dbox := iup.DetachBox(hbox).SetAttributes(map[string]string{
		"ORIENTATION": "VERTICAL",
		//"SHOWGRIP":    "NO",
		//"BARSIZE":     "0",
		//"COLOR":       "255 0 0",
	}).SetHandle("dbox")
	dbox.SetCallback("DETACHED_CB", iup.DetachedFunc(detachedCb))

	lbl := iup.Label("Label").SetAttributes(`EXPAND=VERTICAL`)

	bt2 := iup.Button("Restore Me!").SetAttributes(`EXPAND=YES, ACTIVE=NO`).SetHandle("restore")
	bt2.SetCallback("ACTION", iup.ActionFunc(btnRestoreCb))

	txt := iup.Text().SetAttributes(`EXPAND=HORIZONTAL`)

	dlg := iup.Dialog(iup.Vbox(
		dbox, lbl, bt2, txt,
	)).SetAttributes(map[string]string{
		"TITLE":      "DetachBox",
		"MARGIN":     "10x10",
		"GAP":        "10",
		"RASTERSIZE": "300x300",
	})

	iup.Show(dlg)
	iup.MainLoop()
}

func detachedCb(ih, newParent iup.Ihandle, x, y int) int {
	newParent.SetAttribute("TITLE", "New Dialog")

	iup.GetHandle("restore").SetAttribute("ACTIVE", "YES")
	iup.GetHandle("detach").SetAttribute("ACTIVE", "NO")

	return iup.DEFAULT
}

func btnRestoreCb(bt iup.Ihandle) int {
	iup.GetHandle("dbox").SetAttribute("RESTORE", nil)

	bt.SetAttribute("ACTIVE", "NO")
	iup.GetHandle("detach").SetAttribute("ACTIVE", "YES")

	return iup.DEFAULT
}

func btnDetachCb(bt iup.Ihandle) int {
	iup.GetHandle("dbox").SetAttribute("DETACH", nil)

	bt.SetAttribute("ACTIVE", "NO")
	iup.GetHandle("restore").SetAttribute("ACTIVE", "YES")

	return iup.DEFAULT
}
