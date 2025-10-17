package main

import (
	"fmt"
	"github.com/gen2brain/iup-go/iup"
)

// Callback called when a leaf is added by the menu
func addLeaf(ih iup.Ihandle) int {
	fmt.Println("addLeaf")
	tree := iup.GetHandle("tree")
	id := iup.GetInt(tree, "VALUE")
	attr := fmt.Sprintf("ADDLEAF%d", id)
	iup.SetAttribute(tree, attr, "")
	return iup.DEFAULT
}

// Callback called when a branch is added by the menu
func addBranch(ih iup.Ihandle) int {
	fmt.Println("addBranch")
	tree := iup.GetHandle("tree")
	id := iup.GetInt(tree, "VALUE")
	attr := fmt.Sprintf("ADDBRANCH%d", id)
	iup.SetAttribute(tree, attr, "")
	return iup.DEFAULT
}

// Callback called when a node is removed by the menu
func removeNode(ih iup.Ihandle) int {
	fmt.Println("removeNode")
	tree := iup.GetHandle("tree")
	iup.SetAttribute(tree, "DELNODE", "MARKED")
	return iup.DEFAULT
}

// Callback called when a node is renamed from the menu
func renameNode(ih iup.Ihandle) int {
	tree := iup.GetHandle("tree")
	fmt.Printf("renameNode (%d)\n", iup.GetInt(tree, "VALUE"))
	iup.SetAttribute(tree, "RENAME", "YES")
	return iup.DEFAULT
}

func executeLeafCb(ih iup.Ihandle, id int) int {
	fmt.Printf("executeLeafCb (%d)\n", id)
	return iup.DEFAULT
}

func renameCb(ih iup.Ihandle, id int, name string) int {
	fmt.Printf("renameCb (%d=%s)\n", id, name)
	if name == "fool" {
		return iup.IGNORE
	}
	return iup.DEFAULT
}

func branchOpenCb(ih iup.Ihandle, id int) int {
	fmt.Printf("branchOpenCb (%d)\n", id)
	return iup.DEFAULT
}

func branchCloseCb(ih iup.Ihandle, id int) int {
	fmt.Printf("branchCloseCb (%d)\n", id)
	return iup.DEFAULT
}

func dragDropCb(ih iup.Ihandle, dragId, dropId, isShift, isControl int) int {
	fmt.Printf("dragDropCb (%d)->(%d)\n", dragId, dropId)
	return iup.DEFAULT
}

func kAnyCb(ih iup.Ihandle, c int) int {
	fmt.Printf("kAnyCb (%d)\n", c)
	if c == iup.K_DEL {
		iup.SetAttribute(ih, "DELNODE", "MARKED")
	}
	return iup.DEFAULT
}

func rightClickCb(ih iup.Ihandle, id int) int {
	popupMenu := iup.Menu(
		iup.Item("Add Leaf").SetCallback("ACTION", iup.ActionFunc(addLeaf)),
		iup.Item("Add Branch").SetCallback("ACTION", iup.ActionFunc(addBranch)),
		iup.Item("Rename Node").SetCallback("ACTION", iup.ActionFunc(renameNode)),
		iup.Item("Remove Node").SetCallback("ACTION", iup.ActionFunc(removeNode)),
	)

	iup.SetAttribute(ih, "VALUE", fmt.Sprintf("%d", id))
	iup.Popup(popupMenu, iup.MOUSEPOS, iup.MOUSEPOS)

	iup.Destroy(popupMenu)

	return iup.DEFAULT
}

func initTree() {
	tree := iup.Tree()

	iup.SetCallback(tree, "EXECUTELEAF_CB", iup.ExecuteLeafFunc(executeLeafCb))
	iup.SetCallback(tree, "RENAME_CB", iup.RenameFunc(renameCb))
	iup.SetCallback(tree, "BRANCHCLOSE_CB", iup.BranchCloseFunc(branchCloseCb))
	iup.SetCallback(tree, "BRANCHOPEN_CB", iup.BranchOpenFunc(branchOpenCb))
	iup.SetCallback(tree, "DRAGDROP_CB", iup.DragDropFunc(dragDropCb))
	iup.SetCallback(tree, "RIGHTCLICK_CB", iup.RightClickFunc(rightClickCb))
	iup.SetCallback(tree, "K_ANY", iup.KAnyFunc(kAnyCb))

	// iup.SetAttribute(tree, "FONT", "COURIER_NORMAL")
	// iup.SetAttribute(tree, "CTRL", "YES")
	// iup.SetAttribute(tree, "SHIFT", "YES")
	// iup.SetAttribute(tree, "ADDEXPANDED", "NO")
	// iup.SetAttribute(tree, "SHOWDRAGDROP", "YES")
	iup.SetAttribute(tree, "SHOWRENAME", "YES")

	iup.SetHandle("tree", tree)
}

func initDlg() {
	tree := iup.GetHandle("tree")
	box := iup.Vbox(
		iup.Hbox(
			tree,
		),
	)

	dlg := iup.Dialog(box)
	iup.SetAttribute(dlg, "TITLE", "Tree")
	iup.SetAttribute(box, "MARGIN", "20x20")
	iup.SetHandle("dlg", dlg)
}

func initTreeAttributes() {
	tree := iup.GetHandle("tree")

	iup.SetAttribute(tree, "TITLE", "Figures")
	iup.SetAttribute(tree, "CONTEXTMENU", nil)
	iup.SetAttribute(tree, "ADDBRANCH", "3D")
	iup.SetAttribute(tree, "ADDBRANCH", "2D")
	iup.SetAttribute(tree, "ADDLEAF", "test")
	iup.SetAttribute(tree, "ADDBRANCH1", "parallelogram")
	iup.SetAttribute(tree, "ADDLEAF2", "diamond")
	iup.SetAttribute(tree, "ADDLEAF2", "square")
	iup.SetAttribute(tree, "ADDBRANCH1", "triangle")
	iup.SetAttribute(tree, "ADDLEAF2", "scalenus")
	iup.SetAttribute(tree, "ADDLEAF2", "isoceles")
	iup.SetAttribute(tree, "ADDLEAF2", "equilateral")
	iup.SetAttribute(tree, "VALUE", "6")
}

func main() {
	iup.Open()
	defer iup.Close()

	initTree()
	initDlg()

	dlg := iup.GetHandle("dlg")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)

	initTreeAttributes()

	iup.MainLoop()
}
