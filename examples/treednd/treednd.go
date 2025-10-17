package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func initDlg() {
	tree1 := iup.GetHandle("tree1")
	tree2 := iup.GetHandle("tree2")
	box := iup.Vbox(
		iup.Hbox(tree1, tree2),
	)
	dlg := iup.Dialog(box)
	dlg.SetAttribute("TITLE", "Tree DND")
	box.SetAttribute("MARGIN", "20x20")
	iup.SetHandle("dlg", dlg)
}

func initTree2Attributes() {
	tree2 := iup.GetHandle("tree2")

	tree2.SetAttribute("TITLE", "Frutas")
	tree2.SetAttribute("CONTEXTMENU", nil)
	tree2.SetAttribute("ADDBRANCH", "Outras")
	tree2.SetAttribute("ADDLEAF", "morango")
	tree2.SetAttribute("ADDLEAF", "maçã")
	tree2.SetAttribute("ADDBRANCH1", "Cítricas")
	tree2.SetAttribute("ADDLEAF2", "limão")
	tree2.SetAttribute("ADDLEAF2", "laranja")
	tree2.SetAttribute("ADDLEAF2", "cajá")
	tree2.SetAttribute("VALUE", "3")
}

func initTree1Attributes() {
	tree1 := iup.GetHandle("tree1")

	tree1.SetAttribute("TITLE", "Figures")
	tree1.SetAttribute("CONTEXTMENU", nil)
	tree1.SetAttribute("ADDBRANCH", "3D")
	tree1.SetAttribute("ADDBRANCH", "2D")
	tree1.SetAttribute("ADDLEAF", "test")
	tree1.SetAttribute("ADDBRANCH1", "parallelogram")
	tree1.SetAttribute("ADDLEAF2", "diamond")
	tree1.SetAttribute("ADDLEAF2", "square")
	tree1.SetAttribute("ADDBRANCH1", "triangle")
	tree1.SetAttribute("ADDLEAF2", "scalenus")
	tree1.SetAttribute("ADDLEAF2", "isoceles")
	tree1.SetAttribute("ADDLEAF2", "equilateral")
	tree1.SetAttribute("ADDBRANCH2", "new branch")
	tree1.SetAttribute("ADDLEAF3", "child")
	tree1.SetAttribute("ADDBRANCH3", "nothing")
	tree1.SetAttribute("VALUE", "6")
	tree1.SetAttribute("COLOR5", "0 0 255")
	tree1.SetAttribute("TITLEFONT6", "Courier, 12")
}

func main() {
	iup.Open()
	defer iup.Close()

	iup.SetGlobal("UTF8MODE", "Yes")

	tree1 := iup.Tree()
	iup.SetHandle("tree1", tree1)

	tree2 := iup.Tree()
	iup.SetHandle("tree2", tree2)

	// Generic DND
	// tree1.SetAttribute("SHOWDRAGDROP", "YES")
	// tree1.SetAttribute("DRAGTYPES", "TEXT,STRING")
	// Tree2Tree DND
	tree1.SetAttribute("DRAGDROPTREE", "YES")
	// Common DND Attrib
	tree1.SetAttribute("DRAGSOURCE", "YES")
	tree1.SetAttribute("DRAGSOURCEMOVE", "YES")
	tree1.SetAttribute("DRAGTYPES", "TREEBRANCH")

	// Generic DND
	// tree2.SetAttribute("SHOWDRAGDROP", "YES")
	// tree2.SetAttribute("DROPTYPES", "TEXT,STRING")
	// Tree2Tree DND
	tree2.SetAttribute("DRAGDROPTREE", "YES")
	// Common DND Attrib
	tree2.SetAttribute("DROPTARGET", "YES")
	tree2.SetAttribute("DROPTYPES", "TREEBRANCH")

	initDlg()
	dlg := iup.GetHandle("dlg")
	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)

	initTree1Attributes() // Initializes attributes, can be done here or anywhere
	initTree2Attributes() // Initializes attributes, can be done here or anywhere

	iup.MainLoop() // Main loop
}
