//go:build ctl

package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func imageValueChangedCb(ih iup.Ihandle, item, state int) int {
	fmt.Printf("imageValueChangedCb(item=%d, state=%d)\n", item, state)
	return iup.DEFAULT
}

func listClickCb(ih iup.Ihandle, lin, col int, status string) int {
	value := iup.GetAttributeId(ih, "", lin)
	if value == "" {
		value = "NULL"
	}
	fmt.Printf("listclick_cb(%d, %d)\n", lin, col)
	fmt.Printf("  VALUE%d:%d = %s\n", lin, col, value)
	return iup.DEFAULT
}

func listActionCb(ih iup.Ihandle, item, state int) int {
	fmt.Printf("listActionCb(item=%d, state=%d)\n", item, state)
	return iup.DEFAULT
}

func listEditionCb(ih iup.Ihandle, lin, col, mode, update int) int {
	// Prevent editing column 1 (the COLOR column)
	if col == 1 {
		return iup.IGNORE
	}
	return iup.DEFAULT
}

func main() {
	iup.Open()
	iup.ControlsOpen()
	defer iup.Close()

	mlist := iup.MatrixList()
	mlist.SetAttribute("COUNT", "10")
	mlist.SetAttribute("VISIBLELINES", "9")
	mlist.SetAttribute("COLUMNORDER", "LABEL:COLOR:IMAGE")
	// NOTE: SHOWDELETE="No" (default) - IMAGE column shows checkboxes (IMAGECHECK/IMAGEUNCHECK)
	//       SHOWDELETE="Yes" - IMAGE column shows delete buttons (IMAGEDEL) instead
	// mlist.SetAttribute("SHOWDELETE", "Yes")
	mlist.SetAttribute("EDITABLE", "Yes")

	mlist.SetCallback("LISTCLICK_CB", iup.ClickFunc(listClickCb))
	mlist.SetCallback("ACTION_CB", iup.MatrixListActionFunc(listActionCb))
	mlist.SetCallback("IMAGEVALUECHANGED_CB", iup.MatrixListActionFunc(imageValueChangedCb))
	// mlist.SetCallback("LISTEDITION_CB", iup.MatrixListEditionFunc(listEditionCb)) // Prevents editing COLOR column

	// Column Layout Options:
	// mlist.SetAttribute("COLUMNORDER", "LABEL:COLOR")    // Only LABEL and COLOR columns
	// mlist.SetAttribute("COLUMNORDER", "LABEL")          // Only LABEL column
	//
	// State and Behavior:
	// mlist.SetAttribute("ACTIVE", "NO")                  // Disable entire list
	// mlist.SetAttribute("FOCUSCOLOR", "BGCOLOR")         // Use background color for focus
	// mlist.SetAttribute("READONLY", "Yes")               // Prevent editing
	//
	// Layout Options:
	// mlist.SetAttribute("EXPAND", "Yes")                 // Expand in all directions
	// mlist.SetAttribute("EXPAND", "VERTICAL")            // Expand only vertically
	// mlist.SetAttribute("FLATSCROLLBAR", "VERTICAL")     // Flat vertical scrollbar
	// mlist.SetAttribute("SHOWFLOATING", "Yes")           // Show floating elements

	// Bluish style
	mlist.SetAttribute("TITLE", "Test")
	mlist.SetAttribute("BGCOLOR", "220 230 240")
	mlist.SetAttribute("FRAMECOLOR", "120 140 160")
	mlist.SetAttribute("ITEMBGCOLOR0", "120 140 160")
	mlist.SetAttribute("ITEMFGCOLOR0", "255 255 255")

	// Set list items
	mlist.SetAttribute("1", "AAA")
	mlist.SetAttribute("2", "BBB")
	mlist.SetAttribute("3", "CCC")
	mlist.SetAttribute("4", "DDD")
	mlist.SetAttribute("5", "EEE")
	mlist.SetAttribute("6", "FFF")
	mlist.SetAttribute("7", "GGG")
	mlist.SetAttribute("8", "HHH")
	mlist.SetAttribute("9", "III")
	mlist.SetAttribute("10", "JJJ")

	// Set colors
	mlist.SetAttribute("COLOR1", "255 0 0")
	mlist.SetAttribute("COLOR2", "255 255 0")
	mlist.SetAttribute("COLOR4", "0 255 255")
	mlist.SetAttribute("COLOR5", "0 0 255")
	mlist.SetAttribute("COLOR6", "255 0 255")
	mlist.SetAttribute("COLOR7", "255 128 0")
	mlist.SetAttribute("COLOR8", "255 128 128")
	mlist.SetAttribute("COLOR9", "0 255 128")
	mlist.SetAttribute("COLOR10", "128 255 128")

	// Set item states
	mlist.SetAttribute("ITEMACTIVE3", "NO")
	mlist.SetAttribute("ITEMACTIVE7", "NO")
	mlist.SetAttribute("ITEMACTIVE8", "NO")

	mlist.SetAttribute("IMAGEACTIVE9", "No")

	// Set image values (checkmarks)
	mlist.SetAttribute("IMAGEVALUE1", "ON")
	mlist.SetAttribute("IMAGEVALUE2", "ON")
	mlist.SetAttribute("IMAGEVALUE3", "ON")

	dlg := iup.Dialog(iup.Vbox(mlist))
	dlg.SetAttribute("TITLE", "IupMatrixList")
	dlg.SetAttribute("MARGIN", "10x10")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)

	// Append item after dialog is shown
	mlist.SetAttribute("APPENDITEM", "KKK")

	iup.MainLoop()
}
