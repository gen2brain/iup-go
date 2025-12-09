package main

import (
	"fmt"
	"github.com/gen2brain/iup-go/iup"
	"time"
)

func appendLog(msg string) {
	log := iup.GetHandle("log")
	timestamp := time.Now().Format("15:04:05.000")
	log.SetAttribute("APPEND", fmt.Sprintf("[%s] %s\n", timestamp, msg))
	fmt.Printf("[%s] %s\n", timestamp, msg)
}

func main() {
	iup.Open()
	defer iup.Close()

	// Create three different list types
	listSimple := iup.List()
	listMulti := iup.List()
	listDropdown := iup.List()

	// Configure lists
	listSimple.SetAttributes("1=Apple, 2=Banana, 3=Cherry, 4=Date, 5=Elderberry")
	listSimple.SetAttributes("EXPAND=YES, VISIBLELINES=6")
	listSimple.SetAttribute("SHOWIMAGE", "YES")
	listSimple.SetAttribute("SHOWDRAGDROP", "YES")

	listMulti.SetAttributes("1=Red, 2=Green, 3=Blue, 4=Yellow, 5=Orange, 6=Purple")
	listMulti.SetAttributes("EXPAND=YES, VISIBLELINES=6, MULTIPLE=YES")
	listMulti.SetAttribute("SHOWIMAGE", "YES")
	listMulti.SetAttribute("SHOWDRAGDROP", "YES")

	listDropdown.SetAttributes("1=Monday, 2=Tuesday, 3=Wednesday, 4=Thursday, 5=Friday")
	listDropdown.SetAttributes("EXPAND=HORIZONTAL, DROPDOWN=YES")
	listDropdown.SetAttribute("VALUE", "1")

	// Store handles
	iup.SetHandle("list_simple", listSimple)
	iup.SetHandle("list_multi", listMulti)
	iup.SetHandle("list_dropdown", listDropdown)

	// Log widget
	txtLog := iup.Text()
	txtLog.SetAttributes("MULTILINE=YES, EXPAND=YES, READONLY=YES, VISIBLELINES=10")
	txtLog.SetAttribute("VALUE", "IupList Callback Demo - All Callbacks\n"+
		"=========================================\n"+
		"• Simple List: Single selection, drag-drop enabled\n"+
		"• Multi List: Multiple selection, drag-drop enabled\n"+
		"• Dropdown: Editbox enabled\n"+
		"---\n")

	iup.SetHandle("log", txtLog)

	// ACTION callback - Called when selection changes
	actionCb := iup.ListActionFunc(func(ih iup.Ihandle, text string, item, state int) int {
		name := iup.GetName(ih)
		stateStr := "deselected"
		if state == 1 {
			stateStr = "selected"
		}
		appendLog(fmt.Sprintf("%s: ACTION - item=%d text='%s' state=%s", name, item, text, stateStr))
		return iup.DEFAULT
	})

	iup.SetCallback(listSimple, "ACTION", actionCb)
	iup.SetCallback(listMulti, "ACTION", actionCb)
	iup.SetCallback(listDropdown, "ACTION", actionCb)

	// MULTISELECT_CB - Called for multiple selection changes
	// value string: '+' = newly selected, '-' = newly deselected, 'x' = unchanged
	multiCb := iup.MultiselectFunc(func(ih iup.Ihandle, value string) int {
		name := iup.GetName(ih)

		// Get actual current selection from VALUE attribute
		currentValue := ih.GetAttribute("VALUE")
		selectedIDs := []int{}
		for i, c := range currentValue {
			if c == '+' {
				selectedIDs = append(selectedIDs, i+1)
			}
		}

		count := len(selectedIDs)
		idsStr := fmt.Sprintf("%v", selectedIDs)
		if count == 0 {
			idsStr = "none"
		}
		appendLog(fmt.Sprintf("%s: MULTISELECT_CB - total=%d, selected IDs=%s", name, count, idsStr))
		return iup.DEFAULT
	})

	iup.SetCallback(listMulti, "MULTISELECT_CB", multiCb)

	// DBLCLICK_CB - Called on double-click
	dblclickCb := iup.DblclickFunc(func(ih iup.Ihandle, item int, text string) int {
		name := iup.GetName(ih)
		appendLog(fmt.Sprintf("%s: DBLCLICK_CB - item=%d text='%s'", name, item, text))
		return iup.DEFAULT
	})

	iup.SetCallback(listSimple, "DBLCLICK_CB", dblclickCb)
	iup.SetCallback(listMulti, "DBLCLICK_CB", dblclickCb)
	iup.SetCallback(listDropdown, "DBLCLICK_CB", dblclickCb)

	// VALUECHANGED_CB - Called when value changes
	valueChangedCb := iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		name := iup.GetName(ih)
		value := ih.GetAttribute("VALUE")
		appendLog(fmt.Sprintf("%s: VALUECHANGED_CB - value='%s'", name, value))
		return iup.DEFAULT
	})

	iup.SetCallback(listSimple, "VALUECHANGED_CB", valueChangedCb)
	iup.SetCallback(listMulti, "VALUECHANGED_CB", valueChangedCb)
	iup.SetCallback(listDropdown, "VALUECHANGED_CB", valueChangedCb)

	// DROPDOWN_CB - Called when dropdown opens/closes
	iup.SetCallback(listDropdown, "DROPDOWN_CB", iup.DropDownFunc(func(ih iup.Ihandle, state int) int {
		stateStr := "closed"
		if state == 1 {
			stateStr = "opened"
		}
		appendLog(fmt.Sprintf("list_dropdown: DROPDOWN_CB - state=%s", stateStr))
		return iup.DEFAULT
	}))

	// EDIT_CB - Called when editbox content changes
	iup.SetCallback(listDropdown, "EDIT_CB", iup.EditFunc(func(ih iup.Ihandle, c int, newValue string) int {
		char := fmt.Sprintf("%c", c)
		if c < 32 || c > 126 {
			char = fmt.Sprintf("\\x%02x", c)
		}
		appendLog(fmt.Sprintf("list_dropdown: EDIT_CB - char=%s newValue='%s'", char, newValue))
		return iup.DEFAULT
	}))

	// CARET_CB - Called when caret position changes in editbox
	iup.SetCallback(listDropdown, "CARET_CB", iup.CaretFunc(func(ih iup.Ihandle, lin, col, pos int) int {
		appendLog(fmt.Sprintf("list_dropdown: CARET_CB - lin=%d col=%d pos=%d", lin, col, pos))
		return iup.DEFAULT
	}))

	// Common callbacks - GETFOCUS_CB, KILLFOCUS_CB
	focusCb := iup.GetFocusFunc(func(ih iup.Ihandle) int {
		name := iup.GetName(ih)
		appendLog(fmt.Sprintf("%s: GETFOCUS_CB", name))
		return iup.DEFAULT
	})

	killfocusCb := iup.KillFocusFunc(func(ih iup.Ihandle) int {
		name := iup.GetName(ih)
		appendLog(fmt.Sprintf("%s: KILLFOCUS_CB", name))
		return iup.DEFAULT
	})

	iup.SetCallback(listSimple, "GETFOCUS_CB", focusCb)
	iup.SetCallback(listMulti, "GETFOCUS_CB", focusCb)
	iup.SetCallback(listDropdown, "GETFOCUS_CB", focusCb)

	iup.SetCallback(listSimple, "KILLFOCUS_CB", killfocusCb)
	iup.SetCallback(listMulti, "KILLFOCUS_CB", killfocusCb)
	iup.SetCallback(listDropdown, "KILLFOCUS_CB", killfocusCb)

	// Enable drag-drop between lists
	listSimple.SetAttribute("DRAGDROPLIST", "YES")
	listSimple.SetAttribute("DRAGSOURCE", "YES")
	listSimple.SetAttribute("DRAGTYPES", "ITEMLIST")
	listSimple.SetAttribute("DROPTARGET", "YES")
	listSimple.SetAttribute("DROPTYPES", "ITEMLIST")

	listMulti.SetAttribute("DRAGDROPLIST", "YES")
	listMulti.SetAttribute("DRAGSOURCE", "YES")
	listMulti.SetAttribute("DRAGTYPES", "ITEMLIST")
	listMulti.SetAttribute("DROPTARGET", "YES")
	listMulti.SetAttribute("DROPTYPES", "ITEMLIST")

	// BUTTON_CB - Called on mouse button events
	buttonCb := iup.ButtonFunc(func(ih iup.Ihandle, button, pressed, x, y int, status string) int {
		name := iup.GetName(ih)
		action := "released"
		if pressed == 1 {
			action = "pressed"
		}
		appendLog(fmt.Sprintf("%s: BUTTON_CB - button=%d %s x=%d y=%d status='%s'", name, button, action, x, y, status))
		return iup.DEFAULT
	})

	iup.SetCallback(listSimple, "BUTTON_CB", buttonCb)
	iup.SetCallback(listMulti, "BUTTON_CB", buttonCb)
	iup.SetCallback(listDropdown, "BUTTON_CB", buttonCb)

	// Buttons for testing
	btnAddItem := iup.Button("Add Item")
	btnRemoveItem := iup.Button("Remove Item")
	btnClearLog := iup.Button("Clear Log")
	btnSetValue := iup.Button("Set Value=3")
	btnGetValue := iup.Button("Get Value")

	iup.SetCallback(btnAddItem, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		list := iup.GetHandle("list_simple")
		count := iup.GetInt(list, "COUNT")
		newItem := fmt.Sprintf("Item %d", count+1)
		list.SetAttribute("APPENDITEM", newItem)
		appendLog(fmt.Sprintf("Added item: '%s' (total: %d)", newItem, count+1))
		return iup.DEFAULT
	}))

	iup.SetCallback(btnRemoveItem, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		list := iup.GetHandle("list_simple")
		value := list.GetAttribute("VALUE")
		if value != "" && value != "0" {
			list.SetAttribute("REMOVEITEM", value)
			appendLog(fmt.Sprintf("Removed item at position: %s", value))
		} else {
			appendLog("No item selected to remove")
		}
		return iup.DEFAULT
	}))

	iup.SetCallback(btnClearLog, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		log := iup.GetHandle("log")
		log.SetAttribute("VALUE", "Log cleared.\n")
		return iup.DEFAULT
	}))

	iup.SetCallback(btnSetValue, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		list := iup.GetHandle("list_simple")
		list.SetAttribute("VALUE", "3")
		appendLog("Programmatically set VALUE=3 on list_simple")
		return iup.DEFAULT
	}))

	iup.SetCallback(btnGetValue, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		simple := iup.GetHandle("list_simple")
		multi := iup.GetHandle("list_multi")
		dropdown := iup.GetHandle("list_dropdown")

		simpleVal := simple.GetAttribute("VALUE")
		multiVal := multi.GetAttribute("VALUE")
		dropdownVal := dropdown.GetAttribute("VALUE")

		appendLog(fmt.Sprintf("VALUES: simple=%s multi=%s dropdown=%s", simpleVal, multiVal, dropdownVal))
		return iup.DEFAULT
	}))

	// Create frames for each list
	frameSimple := iup.Frame(
		iup.Vbox(
			iup.Label("Single Selection List"),
			listSimple,
		).SetAttributes("MARGIN=5x5, GAP=5"),
	).SetAttribute("TITLE", "Simple List")

	frameMulti := iup.Frame(
		iup.Vbox(
			iup.Label("Multiple Selection List"),
			listMulti,
		).SetAttributes("MARGIN=5x5, GAP=5"),
	).SetAttribute("TITLE", "Multiple Selection")

	frameDropdown := iup.Frame(
		iup.Vbox(
			iup.Label("Dropdown with Editbox"),
			listDropdown,
		).SetAttributes("MARGIN=5x5, GAP=5"),
	).SetAttribute("TITLE", "Dropdown")

	// Create dialog
	dlg := iup.Dialog(
		iup.Vbox(
			iup.Label("IupList Callbacks").SetAttributes(`FONT="Sans, Bold 12"`),
			iup.Hbox(
				frameSimple,
				frameMulti,
				frameDropdown,
			).SetAttributes("MARGIN=5x5, GAP=5"),
			iup.Hbox(
				btnAddItem,
				btnRemoveItem,
				btnSetValue,
				btnGetValue,
				btnClearLog,
			).SetAttributes("MARGIN=5x5, GAP=5"),
			iup.Label("Event Log (with timestamps):"),
			txtLog,
		).SetAttributes("MARGIN=10x10, GAP=10"),
	)

	dlg.SetAttributes(`TITLE="IupList Callbacks"`)

	iup.Show(dlg)
	iup.MainLoop()
}
