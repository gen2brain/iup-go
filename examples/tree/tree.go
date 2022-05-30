package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	tree := iup.Tree().SetHandle("tree").SetAttributes(map[string]string{
		"FONT":        "COURIER_NORMAL_10",
		"MARKMODE":    "MULTIPLE",
		"ADDEXPANDED": "NO",
		"SHOWRENAME":  "YES",
	})

	tree.SetCallback("SHOWRENAME_CB", iup.ShowRenameFunc(showRenameCb))
	tree.SetCallback("RENAME_CB", iup.RenameFunc(renameCb))
	tree.SetCallback("K_ANY", iup.KAnyFunc(kAny))

	dlg := iup.Dialog(tree).SetAttribute("TITLE", "Tree")

	iup.Show(dlg)

	tree.SetAttributes(`
		TITLE=Figures,
		ADDBRANCH=3D,
		ADDBRANCH=2D,
		ADDBRANCH1=parallelogram,
		ADDLEAF2=diamond,
		ADDLEAF2=square,
		ADDBRANCH1=triangle,
		ADDLEAF2=scalenus,
		ADDLEAF2=isoceles,
		VALUE=6,
	`)

	iup.MainLoop()
}

func showRenameCb(ih iup.Ihandle, id int) int {
	println("SHOWRENAME_CB")
	return iup.DEFAULT
}

func renameCb(ih iup.Ihandle, id int, title string) int {
	println("RENAME_CB")
	return iup.DEFAULT
}

func kAny(ih iup.Ihandle, c int) int {
	if c == iup.K_DEL {
		iup.GetHandle("tree").SetAttribute("DELNODE", "MARKED")
	}
	return iup.DEFAULT
}
