package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	// Create table with sorting, reordering, and selection
	table := iup.Table()

	// Set up 5 columns
	table.SetAttribute("NUMCOL", "5")
	table.SetAttribute("TITLE1", "ID")
	table.SetAttribute("TITLE2", "Name")
	table.SetAttribute("TITLE3", "Department")
	table.SetAttribute("TITLE4", "Score")
	table.SetAttribute("TITLE5", "Grade")

	// Set column alignments
	table.SetAttribute("ALIGNMENT1", "ACENTER")
	table.SetAttribute("ALIGNMENT2", "ALEFT")
	table.SetAttribute("ALIGNMENT3", "ALEFT")
	table.SetAttribute("ALIGNMENT4", "ARIGHT")
	table.SetAttribute("ALIGNMENT5", "ACENTER")

	// Enable advanced features
	table.SetAttribute("SORTABLE", "YES")           // Click headers to sort
	table.SetAttribute("ALLOWREORDER", "YES")       // Drag headers to reorder
	table.SetAttribute("SELECTIONMODE", "MULTIPLE") // Ctrl+Click, Shift+Click
	table.SetAttribute("STRETCHLAST", "NO")         // Disable stretching the last column

	// Add 15 rows
	table.SetAttribute("NUMLIN", "15")

	// Populate with sample data
	sampleData := [][]string{
		{"1", "Alice Johnson", "Engineering", "92.5", "A"},
		{"2", "Bob Smith", "Sales", "78.3", "C"},
		{"3", "Charlie Brown", "Marketing", "88.7", "B"},
		{"4", "Diana Prince", "Engineering", "95.2", "A"},
		{"5", "Eve Adams", "Finance", "71.4", "C"},
		{"6", "Frank Miller", "Engineering", "84.6", "B"},
		{"7", "Grace Lee", "Sales", "91.8", "A"},
		{"8", "Henry Ford", "Marketing", "76.9", "C"},
		{"9", "Ivy Chen", "Engineering", "89.3", "B"},
		{"10", "Jack Wilson", "Finance", "68.5", "D"},
		{"11", "Kate Morgan", "Engineering", "94.7", "A"},
		{"12", "Leo Martinez", "Sales", "82.1", "B"},
		{"13", "Maya Patel", "Marketing", "87.9", "B"},
		{"14", "Noah Davis", "Finance", "90.4", "A"},
		{"15", "Olivia Brown", "Engineering", "73.2", "C"},
	}

	for lin, row := range sampleData {
		for col, value := range row {
			iup.SetAttributeId2(table, "", lin+1, col+1, value)
		}
	}

	// Color code by grade
	for lin := 1; lin <= 15; lin++ {
		grade := iup.GetAttributeId2(table, "", lin, 5)
		switch grade {
		case "A":
			iup.SetAttributeId2(table, "BGCOLOR", lin, 5, "#C8E6C9") // Green
		case "B":
			iup.SetAttributeId2(table, "BGCOLOR", lin, 5, "#FFF9C4") // Yellow
		case "C":
			iup.SetAttributeId2(table, "BGCOLOR", lin, 5, "#FFE0B2") // Orange
		case "D":
			iup.SetAttributeId2(table, "BGCOLOR", lin, 5, "#FFCDD2") // Red
		}
	}

	// Buttons to toggle features
	btnToggleSort := iup.Button("Disable Sorting").SetAttribute("PADDING", "5x0")
	btnToggleReorder := iup.Button("Disable Reordering").SetAttribute("PADDING", "5x0")
	btnToggleUserResize := iup.Button("Enable User Resize").SetAttribute("PADDING", "5x0")

	txtLog := iup.Text()
	txtLog.SetAttributes("MULTILINE=YES, EXPAND=HORIZONTAL, READONLY=YES, VISIBLELINES=5")
	txtLog.SetAttribute("VALUE", "Interactive Table Features:\n"+
		"SORTING: Click column headers to sort (ascending/descending)\n"+
		"REORDERING: Drag column headers to rearrange columns\n"+
		"SELECTION: Ctrl+Click for multiple, Shift+Click for range\n"+
		"USER RESIZE: Manually resize columns by dragging column borders\n"+
		"---\n")

	iup.SetHandle("table", table)
	iup.SetHandle("log", txtLog)
	iup.SetHandle("btnToggleSort", btnToggleSort)
	iup.SetHandle("btnToggleReorder", btnToggleReorder)
	iup.SetHandle("btnToggleUserResize", btnToggleUserResize)

	// Toggle sorting
	iup.SetCallback(btnToggleSort, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		t := iup.GetHandle("table")
		currentValue := t.GetAttribute("SORTABLE")
		log := iup.GetHandle("log")

		if currentValue == "YES" {
			t.SetAttribute("SORTABLE", "NO")
			ih.SetAttribute("TITLE", "Enable Sorting")
			log.SetAttribute("APPEND", "Sorting disabled\n")
		} else {
			t.SetAttribute("SORTABLE", "YES")
			ih.SetAttribute("TITLE", "Disable Sorting")
			log.SetAttribute("APPEND", "Sorting enabled\n")
		}
		return iup.DEFAULT
	}))

	// Toggle reordering
	iup.SetCallback(btnToggleReorder, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		t := iup.GetHandle("table")
		currentValue := t.GetAttribute("ALLOWREORDER")
		log := iup.GetHandle("log")

		if currentValue == "YES" {
			t.SetAttribute("ALLOWREORDER", "NO")
			ih.SetAttribute("TITLE", "Enable Reordering")
			log.SetAttribute("APPEND", "Column reordering disabled\n")
		} else {
			t.SetAttribute("ALLOWREORDER", "YES")
			ih.SetAttribute("TITLE", "Disable Reordering")
			log.SetAttribute("APPEND", "Column reordering enabled\n")
		}
		return iup.DEFAULT
	}))

	// Toggle user resize
	iup.SetCallback(btnToggleUserResize, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		t := iup.GetHandle("table")
		currentValue := t.GetAttribute("USERRESIZE")
		log := iup.GetHandle("log")

		if currentValue == "YES" {
			t.SetAttribute("USERRESIZE", "NO")
			ih.SetAttribute("TITLE", "Enable User Resize")
			log.SetAttribute("APPEND", "User column resizing disabled\n")
		} else {
			t.SetAttribute("USERRESIZE", "YES")
			ih.SetAttribute("TITLE", "Disable User Resize")
			log.SetAttribute("APPEND", "User column resizing enabled - try dragging column borders!\n")
		}
		return iup.DEFAULT
	}))

	// Selection callback
	iup.SetCallback(table, "ENTERITEM_CB", iup.EnterItemFunc(func(ih iup.Ihandle, lin, col int) int {
		log := iup.GetHandle("log")
		if lin > 0 {
			name := iup.GetAttributeId2(ih, "", lin, 2)
			dept := iup.GetAttributeId2(ih, "", lin, 3)
			log.SetAttribute("APPEND", fmt.Sprintf("Selected: [%d,%d] - %s (%s)\n", lin, col, name, dept))
		}
		return iup.DEFAULT
	}))

	// Sort callback
	iup.SetCallback(table, "SORT_CB", iup.TableSortFunc(func(ih iup.Ihandle, col int) int {
		log := iup.GetHandle("log")
		columnTitle := ih.GetAttribute(fmt.Sprintf("TITLE%d", col))
		log.SetAttribute("APPEND", fmt.Sprintf("Sorting by column %d (%s)\n", col, columnTitle))
		return iup.DEFAULT
	}))

	// Create dialog
	dlg := iup.Dialog(
		iup.Vbox(
			iup.Label("IupTable: Interactive Features Demo").SetAttributes(`FONT="Sans, Bold 12"`),
			iup.Label("Sorting, Column Reordering, User Resizing, and Multi-Selection"),
			iup.Hbox(
				btnToggleSort,
				btnToggleReorder,
				btnToggleUserResize,
			).SetAttributes("MARGIN=5x5, GAP=5"),
			table,
			iup.Label("Event Log:"),
			txtLog,
		).SetAttributes("MARGIN=10x10, GAP=10"),
	)

	dlg.SetAttributes(`TITLE="IupTable Advanced Features"`)

	iup.Show(dlg)
	iup.MainLoop()
}
