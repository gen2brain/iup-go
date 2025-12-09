package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	table := iup.Table()

	// Set up 4 columns
	table.SetAttribute("NUMCOL", "4")
	table.SetAttribute("TITLE1", "Product")
	table.SetAttribute("TITLE2", "Status")
	table.SetAttribute("TITLE3", "Price")
	table.SetAttribute("TITLE4", "Notes")

	// Set column alignments
	table.SetAttribute("ALIGNMENT1", "ALEFT")
	table.SetAttribute("ALIGNMENT2", "ACENTER")
	table.SetAttribute("ALIGNMENT3", "ARIGHT")
	table.SetAttribute("ALIGNMENT4", "ALEFT")

	// Add 8 rows
	table.SetAttribute("NUMLIN", "8")

	// Populate with sample data
	sampleData := [][]string{
		{"Laptop", "Available", "$999", "High performance"},
		{"Mouse", "Low Stock", "$29", "Wireless"},
		{"Keyboard", "Available", "$79", "Mechanical"},
		{"Monitor", "Out of Stock", "$299", "4K display"},
		{"USB Cable", "Available", "$9", "USB-C"},
		{"Headphones", "Available", "$149", "Noise canceling"},
		{"Webcam", "Discontinued", "$89", "1080p"},
		{"SSD Drive", "Available", "$199", "1TB NVMe"},
	}

	for lin, row := range sampleData {
		for col, value := range row {
			iup.SetAttributeId2(table, "", lin+1, col+1, value)
		}
	}

	// Cell [1,1] - Light blue background
	iup.SetAttributeId2(table, "BGCOLOR", 1, 1, "#E3F2FD")

	// Cell [2,2] - Yellow background for "Low Stock"
	iup.SetAttributeId2(table, "BGCOLOR", 2, 2, "#FFF9C4")
	iup.SetAttributeId2(table, "FGCOLOR", 2, 2, "#F57F17")

	// Cell [4,2] - Red background for "Out of Stock"
	iup.SetAttributeId2(table, "BGCOLOR", 4, 2, "#FFCDD2")
	iup.SetAttributeId2(table, "FGCOLOR", 4, 2, "#C62828")

	// Cell [7,2] - Gray for "Discontinued"
	iup.SetAttributeId2(table, "BGCOLOR", 7, 2, "#E0E0E0")
	iup.SetAttributeId2(table, "FGCOLOR", 7, 2, "#616161")

	// Per-column color: Column 3 (Price) - light green background
	iup.SetAttributeId2(table, "BGCOLOR", 0, 3, "#C8E6C9")

	// Per-row color: Row 6 - light purple background (highlight entire row)
	iup.SetAttributeId2(table, "BGCOLOR", 6, 0, "#E1BEE7")

	// Per-cell fonts: Make specific cells bold/larger
	iup.SetAttributeId2(table, "FONT", 1, 1, "Sans, Bold 12")

	// Per-column font: Column 2 (Status) - Bold
	iup.SetAttributeId2(table, "FONT", 0, 2, "Sans, Bold 10")

	// Per-column font: Column 3 (Price) - Monospace for numbers
	iup.SetAttributeId2(table, "FONT", 0, 3, "Monospace 10")

	// Specific cell with larger font for emphasis
	iup.SetAttributeId2(table, "FONT", 4, 2, "Sans, Bold 11")

	// Buttons for dynamic color changes
	btnHighlightRow := iup.Button("Highlight Row 3").SetAttribute("PADDING", "5x0")
	btnClearColors := iup.Button("Clear Row 3 Colors").SetAttribute("PADDING", "5x0")
	btnSetColumnColor := iup.Button("Set Column 1 Blue").SetAttribute("PADDING", "5x0")

	txtLog := iup.Text()
	txtLog.SetAttributes("MULTILINE=YES, EXPAND=HORIZONTAL, READONLY=YES, VISIBLELINES=3")

	iup.SetHandle("table", table)
	iup.SetHandle("log", txtLog)

	// Button callbacks
	iup.SetCallback(btnHighlightRow, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		t := iup.GetHandle("table")
		// Set per-row color for row 3
		iup.SetAttributeId2(t, "BGCOLOR", 3, 0, "#FFECB3")
		iup.SetAttributeId2(t, "FGCOLOR", 3, 0, "#E65100")
		t.SetAttribute("REDRAW", "YES")
		log := iup.GetHandle("log")
		log.SetAttribute("APPEND", "Set row 3 background to orange\n")
		return iup.DEFAULT
	}))

	iup.SetCallback(btnClearColors, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		t := iup.GetHandle("table")
		// Clear per-row color for row 3
		iup.SetAttributeId2(t, "BGCOLOR", 3, 0, "")
		iup.SetAttributeId2(t, "FGCOLOR", 3, 0, "")
		t.SetAttribute("REDRAW", "YES")
		log := iup.GetHandle("log")
		log.SetAttribute("APPEND", "Cleared row 3 colors\n")
		return iup.DEFAULT
	}))

	iup.SetCallback(btnSetColumnColor, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		t := iup.GetHandle("table")
		// Set per-column color for column 1
		iup.SetAttributeId2(t, "BGCOLOR", 0, 1, "#BBDEFB")
		t.SetAttribute("REDRAW", "YES")
		log := iup.GetHandle("log")
		log.SetAttribute("APPEND", "Set column 1 background to blue\n")
		return iup.DEFAULT
	}))

	// Create dialog
	dlg := iup.Dialog(
		iup.Vbox(
			iup.Label("IupTable: Colors, Fonts & Alignment").SetAttributes("FONT=\"Sans, Bold 12\""),
			iup.Label("Colors: Per-cell > Per-column > Per-row | Fonts: Status=Bold, Price=Monospace"),
			iup.Hbox(
				btnHighlightRow,
				btnClearColors,
				btnSetColumnColor,
			).SetAttributes("MARGIN=5x5, GAP=5"),
			table,
			iup.Label("Log:"),
			txtLog,
		).SetAttributes("MARGIN=10x10, GAP=10"),
	)

	dlg.SetAttributes("TITLE=\"IupTable Cell Colors Example\"")

	iup.Show(dlg)
	iup.MainLoop()
}
