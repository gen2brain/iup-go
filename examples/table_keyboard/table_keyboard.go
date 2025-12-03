package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	iup.SetGlobal("UTF8MODE", "YES")

	// Create table
	table := iup.Table()

	// Set up 4 columns
	table.SetAttribute("NUMCOL", "4")
	table.SetAttribute("TITLE1", "Name")
	table.SetAttribute("TITLE2", "Email")
	table.SetAttribute("TITLE3", "Phone")
	table.SetAttribute("TITLE4", "Notes")

	// Set column widths
	table.SetAttribute("RASTERWIDTH1", "120")
	table.SetAttribute("RASTERWIDTH2", "180")
	table.SetAttribute("RASTERWIDTH3", "120")
	table.SetAttribute("RASTERWIDTH4", "200")

	// Make all columns editable (required for paste to work)
	table.SetAttribute("EDITABLE", "YES")

	// Add 6 rows
	table.SetAttribute("NUMLIN", "6")

	// Populate with sample data
	sampleData := [][]string{
		{"John Doe", "john@example.com", "555-0100", "Sales team"},
		{"Jane Smith", "jane@example.com", "555-0101", "Engineering"},
		{"Bob Johnson", "bob@example.com", "555-0102", "Marketing"},
		{"Alice Brown", "alice@example.com", "555-0103", "HR"},
		{"Charlie Davis", "charlie@example.com", "555-0104", "Finance"},
		{"Eve Wilson", "eve@example.com", "555-0105", "Support"},
	}

	for lin, row := range sampleData {
		for col, value := range row {
			iup.SetAttributeId2(table, "", lin+1, col+1, value)
		}
	}

	// Log widget
	txtLog := iup.Text()
	txtLog.SetAttributes("MULTILINE=YES, EXPAND=HORIZONTAL, READONLY=YES, VISIBLELINES=6")
	txtLog.SetAttribute("VALUE", "Keyboard Navigation & Copy/Paste:\n"+
		"• Arrow keys: Navigate between cells\n"+
		"• Double-click or Enter: Start editing cell\n"+
		"• Enter: Accept edit | Escape: Cancel edit\n"+
		"• Ctrl+C: Copy cell value\n"+
		"• Ctrl+V: Paste to cell\n"+
		"• Watch for EDITBEGIN/EDITEND callbacks below\n"+
		"---\n")

	iup.SetHandle("table", table)
	iup.SetHandle("log", txtLog)

	// Track current cell
	iup.SetCallback(table, "ENTERITEM_CB", iup.EnterItemFunc(func(ih iup.Ihandle, lin, col int) int {
		log := iup.GetHandle("log")
		value := iup.GetAttributeId2(ih, "", lin, col)
		msg := fmt.Sprintf("Current cell: [%d,%d] = '%s'\n", lin, col, value)
		log.SetAttribute("APPEND", msg)
		return iup.DEFAULT
	}))

	// Track paste operations and edits
	iup.SetCallback(table, "VALUECHANGED_CB", iup.TableValueChangedFunc(func(ih iup.Ihandle, lin, col int) int {
		log := iup.GetHandle("log")
		value := iup.GetAttributeId2(ih, "", lin, col)
		msg := fmt.Sprintf("VALUECHANGED_CB: [%d,%d] changed to '%s'\n", lin, col, value)
		log.SetAttribute("APPEND", msg)
		fmt.Printf(msg)
		return iup.DEFAULT
	}))

	// Track when editing begins
	iup.SetCallback(table, "EDITBEGIN_CB", iup.EditBeginFunc(func(ih iup.Ihandle, lin, col int) int {
		log := iup.GetHandle("log")
		value := iup.GetAttributeId2(ih, "", lin, col)
		msg := fmt.Sprintf("EDITBEGIN: [%d,%d] = '%s'\n", lin, col, value)
		log.SetAttribute("APPEND", msg)
		fmt.Printf(msg)
		return iup.DEFAULT // Allow editing
	}))

	// Track when editing ends
	iup.SetCallback(table, "EDITEND_CB", iup.EditEndFunc(func(ih iup.Ihandle, lin, col int, newValue string, apply int) int {
		log := iup.GetHandle("log")
		oldValue := iup.GetAttributeId2(ih, "", lin, col)
		applyStr := "accepted"
		if apply == 0 {
			applyStr = "cancelled"
		}
		msg := fmt.Sprintf("EDITEND: [%d,%d] '%s' → '%s' (%s)\n", lin, col, oldValue, newValue, applyStr)
		log.SetAttribute("APPEND", msg)
		fmt.Printf(msg)
		return iup.DEFAULT // Accept edit
	}))

	// K_ANY callback to show keyboard events
	iup.SetCallback(table, "K_ANY", iup.KAnyFunc(func(ih iup.Ihandle, c int) int {
		log := iup.GetHandle("log")
		keyStr := ""
		switch c {
		case iup.K_UP:
			keyStr = "↑ (Up)"
		case iup.K_DOWN:
			keyStr = "↓ (Down)"
		case iup.K_LEFT:
			keyStr = "← (Left)"
		case iup.K_RIGHT:
			keyStr = "→ (Right)"
		case iup.K_CR:
			keyStr = "↵ (Enter)"
		case iup.K_ESC:
			keyStr = "Esc"
		case 3: // Ctrl+C
			keyStr = "Ctrl+C (Copy)"
		case 22: // Ctrl+V
			keyStr = "Ctrl+V (Paste)"
		default:
			if c >= 32 && c < 127 {
				keyStr = fmt.Sprintf("'%c'", c)
			} else {
				return iup.DEFAULT // Don't log other keys
			}
		}
		msg := fmt.Sprintf("Key pressed: %s\n", keyStr)
		log.SetAttribute("APPEND", msg)
		return iup.DEFAULT
	}))

	// Buttons for testing copy/paste
	btnCopyFirst := iup.Button("Copy First Cell")
	btnPasteToLast := iup.Button("Paste to Last Cell")
	btnClear := iup.Button("Clear Log")

	iup.SetCallback(btnCopyFirst, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		t := iup.GetHandle("table")
		value := iup.GetAttributeId2(t, "", 1, 1)
		iup.SetGlobal("CLIPBOARD", value)
		log := iup.GetHandle("log")
		msg := fmt.Sprintf("Copied [1,1] to clipboard: '%s'\n", value)
		log.SetAttribute("APPEND", msg)
		return iup.DEFAULT
	}))

	iup.SetCallback(btnPasteToLast, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		clipboard := iup.GetGlobal("CLIPBOARD")
		t := iup.GetHandle("table")
		iup.SetAttributeId2(t, "", 6, 4, clipboard)
		log := iup.GetHandle("log")
		msg := fmt.Sprintf("Pasted to [6,4]: '%s'\n", clipboard)
		log.SetAttribute("APPEND", msg)
		return iup.DEFAULT
	}))

	iup.SetCallback(btnClear, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		log := iup.GetHandle("log")
		log.SetAttribute("VALUE", "Log cleared.\n")
		return iup.DEFAULT
	}))

	// Create dialog
	dlg := iup.Dialog(
		iup.Vbox(
			iup.Label("IupTable: Keyboard Navigation & Copy/Paste Demo").SetAttributes(`FONT="Sans, Bold 12"`),
			iup.Label("Navigate with arrows, copy with Ctrl+C, paste with Ctrl+V"),
			iup.Hbox(
				btnCopyFirst,
				btnPasteToLast,
				btnClear,
			).SetAttributes("MARGIN=5x5, GAP=5"),
			table,
			iup.Label("Event Log:"),
			txtLog,
		).SetAttributes("MARGIN=10x10, GAP=10"),
	)

	dlg.SetAttributes(`TITLE="IupTable Keyboard & Copy/Paste Demo"`)

	iup.Show(dlg)
	iup.MainLoop()
}
