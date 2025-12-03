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

	table.SetAttribute("TITLE1", "ID")
	table.SetAttribute("TITLE2", "Name")
	table.SetAttribute("TITLE3", "Age")
	table.SetAttribute("TITLE4", "City")

	// Make Name and Age columns editable
	table.SetAttribute("EDITABLE2", "YES")
	table.SetAttribute("EDITABLE3", "YES")

	// Add 10 rows of data
	table.SetAttribute("NUMLIN", "10")

	// Focus/select row 6, column 4
	table.SetAttribute("FOCUSCELL", "6:4")

	// Disable cell/column dashed rectangle.
	table.SetAttribute("FOCUSRECT", "NO")

	// Populate with sample data
	sampleData := [][]string{
		{"1", "Alice Johnson", "25", "New York"},
		{"2", "Bob Smith", "30", "London"},
		{"3", "Charlie Brown", "35", "Tokyo"},
		{"4", "Diana Prince", "28", "Paris"},
		{"5", "Eve Adams", "32", "Berlin"},
		{"6", "Frank Miller", "29", "Sydney"},
		{"7", "Grace Lee", "27", "Seoul"},
		{"8", "Henry Ford", "31", "Toronto"},
		{"9", "Ivy Chen", "26", "Beijing"},
		{"10", "Jack Wilson", "33", "Mumbai"},
	}

	for lin, row := range sampleData {
		for col, value := range row {
			iup.SetAttributeId2(table, "", lin+1, col+1, value)
		}
	}

	// Enable alternating row colors by default
	table.SetAttribute("ALTERNATECOLOR", "YES")
	table.SetAttribute("EVENROWCOLOR", "#F0F0F0") // Light gray
	table.SetAttribute("ODDROWCOLOR", "#FFFFFF")  // White

	// Buttons for testing
	btnAddRow := iup.Button("Add Row").SetAttribute("PADDING", "5x0")
	btnDelRow := iup.Button("Delete Row").SetAttribute("PADDING", "5x0")
	btnGetValue := iup.Button("Get Cell 1:2").SetAttribute("PADDING", "5x0")
	btnSetValue := iup.Button("Set Cell 1:2").SetAttribute("PADDING", "5x0")
	btnToggleColors := iup.Button("Toggle Alternating Colors").SetAttribute("PADDING", "5x0")
	btnToggleGrid := iup.Button("Toggle Grid").SetAttribute("PADDING", "5x0")

	txtLog := iup.Text()
	txtLog.SetAttributes("MULTILINE=YES, EXPAND=HORIZONTAL, READONLY=YES, VISIBLELINES=3")

	iup.SetHandle("table", table)
	iup.SetHandle("log", txtLog)

	// Set callbacks
	iup.SetCallback(btnAddRow, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		t := iup.GetHandle("table")
		numlin := t.GetInt("NUMLIN")
		t.SetAttribute("ADDLIN", "0") // Append
		log := iup.GetHandle("log")
		log.SetAttribute("APPEND", fmt.Sprintf("Added row %d\n", numlin+1))
		return iup.DEFAULT
	}))

	iup.SetCallback(btnDelRow, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		t := iup.GetHandle("table")
		numlin := t.GetInt("NUMLIN")
		if numlin > 0 {
			t.SetAttribute("DELLIN", fmt.Sprintf("%d", numlin)) // Delete last
			log := iup.GetHandle("log")
			log.SetAttribute("APPEND", fmt.Sprintf("Deleted row %d\n", numlin))
		}
		return iup.DEFAULT
	}))

	iup.SetCallback(btnGetValue, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		t := iup.GetHandle("table")
		value := iup.GetAttributeId2(t, "", 1, 2)
		log := iup.GetHandle("log")
		log.SetAttribute("APPEND", fmt.Sprintf("Cell 1:2 = %s\n", value))
		return iup.DEFAULT
	}))

	iup.SetCallback(btnSetValue, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		t := iup.GetHandle("table")
		iup.SetAttributeId2(t, "", 1, 2, "MODIFIED!")
		log := iup.GetHandle("log")
		log.SetAttribute("APPEND", "Set cell 1:2 to 'MODIFIED!'\n")
		return iup.DEFAULT
	}))

	iup.SetCallback(btnToggleColors, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		t := iup.GetHandle("table")
		currentValue := t.GetAttribute("ALTERNATECOLOR")
		if currentValue == "YES" {
			t.SetAttribute("ALTERNATECOLOR", "NO")
			ih.SetAttribute("TITLE", "Enable Alternating Colors")
		} else {
			t.SetAttribute("ALTERNATECOLOR", "YES")
			ih.SetAttribute("TITLE", "Disable Alternating Colors")
		}
		t.SetAttribute("REDRAW", "YES")
		return iup.DEFAULT
	}))

	iup.SetCallback(btnToggleGrid, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		t := iup.GetHandle("table")
		currentValue := t.GetAttribute("SHOWGRID")
		log := iup.GetHandle("log")
		if currentValue == "YES" {
			t.SetAttribute("SHOWGRID", "NO")
			ih.SetAttribute("TITLE", "Enable Grid")
			log.SetAttribute("APPEND", "Grid disabled\n")
		} else {
			t.SetAttribute("SHOWGRID", "YES")
			ih.SetAttribute("TITLE", "Disable Grid")
			log.SetAttribute("APPEND", "Grid enabled\n")
		}
		return iup.DEFAULT
	}))

	// Table callbacks
	iup.SetCallback(table, "CLICK_CB", iup.ClickFunc(func(ih iup.Ihandle, lin, col int, status string) int {
		log := iup.GetHandle("log")
		value := iup.GetAttributeId2(ih, "", lin, col)
		log.SetAttribute("APPEND", fmt.Sprintf("CLICK_CB: Clicked cell [%d,%d] = %s (status: %s)\n", lin, col, value, status))
		return iup.DEFAULT
	}))

	iup.SetCallback(table, "ENTERITEM_CB", iup.EnterItemFunc(func(ih iup.Ihandle, lin, col int) int {
		log := iup.GetHandle("log")
		if lin > 0 {
			value := iup.GetAttributeId2(ih, "", lin, col)
			msg := fmt.Sprintf("ENTERITEM_CB: Selected cell [%d,%d] = %s\n", lin, col, value)
			fmt.Print(msg)
			log.SetAttribute("APPEND", msg)
		} else {
			msg := "ENTERITEM_CB: Selection cleared\n"
			fmt.Print(msg)
			log.SetAttribute("APPEND", msg)
		}
		return iup.DEFAULT
	}))

	// Edition callback (for editable columns)
	iup.SetCallback(table, "EDITION_CB", iup.TableEditionFunc(func(ih iup.Ihandle, lin, col int, update string) int {
		log := iup.GetHandle("log")
		log.SetAttribute("APPEND", fmt.Sprintf("EDITION_CB: Cell [%d,%d] edited to '%s'\n", lin, col, update))
		return iup.DEFAULT
	}))

	// Create dialog
	dlg := iup.Dialog(
		iup.Vbox(
			iup.Hbox(
				btnAddRow,
				btnDelRow,
				btnGetValue,
				btnSetValue,
			).SetAttributes("MARGIN=5x5, GAP=10"),
			iup.Hbox(
				btnToggleColors,
				btnToggleGrid,
			).SetAttributes("MARGIN=5x5, GAP=10"),
			table,
			iup.Label("Log:"),
			txtLog,
		).SetAttributes("MARGIN=10x10, GAP=10"),
	)

	dlg.SetAttributes(`TITLE="IupTable Test - Editable cells and alternating colors"`)

	iup.Show(dlg)
	iup.MainLoop()
}
