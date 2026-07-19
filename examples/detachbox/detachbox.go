package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	bt := iup.Button("Detach Me!").SetHandle("detach").SetCallback("ACTION", iup.ActionFunc(btnDetachCb))
	ml := iup.MultiLine().SetAttributes(`EXPAND=YES, VISIBLELINES=5, VISIBLECOLUMNS=20`)
	hbox := iup.Hbox(bt, ml).SetAttributes(`MARGIN=10x0`)

	dbox := iup.DetachBox(hbox).SetAttributes(map[string]string{
		"ORIENTATION":       "VERTICAL",
		"RESTOREWHENCLOSED": "YES",
		//"SHOWGRIP":    "NO",
		//"BARSIZE":     "0",
		//"COLOR":       "255 0 0",
	}).SetHandle("dbox")
	dbox.SetCallback("DETACHED_CB", iup.DetachedFunc(detachedCb))
	dbox.SetCallback("RESTORED_CB", iup.RestoredFunc(restoredCb))

	status := iup.Label("Detach and restore the box.").
		SetHandle("status").SetAttributes(`EXPAND=HORIZONTAL`)

	bt2 := iup.Button("Restore Me!").SetAttributes(`EXPAND=YES, ACTIVE=NO`).SetHandle("restore")
	bt2.SetCallback("ACTION", iup.ActionFunc(btnRestoreCb))

	txt := iup.Text().SetAttributes(`EXPAND=HORIZONTAL`)

	dlg := iup.Dialog(iup.Vbox(
		dbox, status, bt2, txt,
	)).SetAttributes(map[string]string{
		"TITLE":  "DetachBox",
		"MARGIN": "10x10",
		"GAP":    "10",
	})

	iup.Show(dlg)
	iup.MainLoop()
}

func detachedCb(ih, newParent iup.Ihandle, x, y int) int {
	newParent.SetAttribute("TITLE", "Close me to restore (RESTORED_CB)")

	iup.GetHandle("status").SetAttribute("TITLE", "DETACHED_CB fired - box detached.")
	iup.GetHandle("restore").SetAttribute("ACTIVE", "YES")
	iup.GetHandle("detach").SetAttribute("ACTIVE", "NO")

	return iup.DEFAULT
}

func restoredCb(ih, oldParent iup.Ihandle, x, y int) int {
	iup.GetHandle("status").SetAttribute("TITLE", "RESTORED_CB fired - box restored.")
	iup.GetHandle("restore").SetAttribute("ACTIVE", "NO")
	iup.GetHandle("detach").SetAttribute("ACTIVE", "YES")

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
