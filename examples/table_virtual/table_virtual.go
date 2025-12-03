package main

import (
	"fmt"
	"sort"
	"strconv"

	"github.com/gen2brain/iup-go/iup"
)

// Simulated large dataset
var dataset [][]string

// Sort state tracking
var sortColumn int = 0        // 0 = unsorted, 1-4 = column number
var sortAscending bool = true // true = ascending, false = descending

func main() {
	iup.Open()
	defer iup.Close()

	iup.SetGlobal("UTF8MODE", "YES")

	// Simulate a large dataset
	dataset = make([][]string, 100000)
	for i := 0; i < 100000; i++ {
		dataset[i] = []string{
			fmt.Sprintf("%d", i+1),
			fmt.Sprintf("Product %d", i+1),
			fmt.Sprintf("Category %d", (i%10)+1),
			fmt.Sprintf("$%d.99", (i%1000)+1),
		}
	}

	// Create table in virtual mode
	table := iup.Table()

	// Enable virtual mode BEFORE setting the number of rows
	table.SetAttribute("VIRTUALMODE", "YES")

	// Enable sorting (application handles sorting via SORT_CB)
	table.SetAttribute("SORTABLE", "YES")

	// Set up 4 columns
	table.SetAttribute("NUMCOL", "4")
	table.SetAttribute("TITLE1", "ID")
	table.SetAttribute("TITLE2", "Product")
	table.SetAttribute("TITLE3", "Category")
	table.SetAttribute("TITLE4", "Price")

	// Set column widths
	table.SetAttribute("RASTERWIDTH1", "60")
	table.SetAttribute("RASTERWIDTH2", "120")
	table.SetAttribute("RASTERWIDTH3", "120")
	table.SetAttribute("RASTERWIDTH4", "100")

	// Set column alignments
	table.SetAttribute("ALIGNMENT1", "ACENTER")
	table.SetAttribute("ALIGNMENT2", "ALEFT")
	table.SetAttribute("ALIGNMENT3", "ALEFT")
	table.SetAttribute("ALIGNMENT4", "ARIGHT")

	// Add 100,000 rows (in virtual mode, this doesn't allocate storage)
	table.SetAttribute("NUMLIN", "100000")

	// Disable cell/column dashed rectangle.
	table.SetAttribute("FOCUSRECT", "NO")

	// Set VALUE_CB callback to provide data on demand
	iup.SetCallback(table, "VALUE_CB", iup.TableValueFunc(func(ih iup.Ihandle, lin, col int) string {
		// lin and col are 1-based
		if lin <= 0 || lin > len(dataset) || col <= 0 || col > 4 {
			return ""
		}
		return dataset[lin-1][col-1]
	}))

	// Set SORT_CB callback for virtual mode sorting
	// In virtual mode, the application is responsible for sorting the data
	iup.SetCallback(table, "SORT_CB", iup.TableSortFunc(func(ih iup.Ihandle, col int) int {
		status := iup.GetHandle("status")
		columnTitle := ih.GetAttribute(fmt.Sprintf("TITLE%d", col))

		// If clicking the same column, toggle a sort direction
		if sortColumn == col {
			sortAscending = !sortAscending
		} else {
			sortColumn = col
			sortAscending = true
		}

		// Sort the dataset array based on column and direction
		sort.SliceStable(dataset, func(i, j int) bool {
			valI := dataset[i][col-1]
			valJ := dataset[j][col-1]

			// For numeric columns (ID and Price), parse as numbers
			if col == 1 || col == 4 {
				numI, _ := strconv.ParseFloat(valI[1:], 64) // Skip $ for price
				numJ, _ := strconv.ParseFloat(valJ[1:], 64)
				if col == 1 {
					numI, _ = strconv.ParseFloat(valI, 64)
					numJ, _ = strconv.ParseFloat(valJ, 64)
				}
				if sortAscending {
					return numI < numJ
				}
				return numI > numJ
			}

			// For string columns, use lexicographic comparison
			if sortAscending {
				return valI < valJ
			}
			return valI > valJ
		})

		// Update status
		direction := "ascending"
		if !sortAscending {
			direction = "descending"
		}
		status.SetAttribute("TITLE", fmt.Sprintf("Sorted by %s (%s)", columnTitle, direction))

		// Redraw table to reflect sorted data
		ih.SetAttribute("REDRAW", "YES")

		return iup.DEFAULT
	}))

	// Enable alternating colors for better readability
	table.SetAttribute("ALTERNATECOLOR", "YES")
	table.SetAttribute("EVENROWCOLOR", "#F5F5F5")
	table.SetAttribute("ODDROWCOLOR", "#FFFFFF")

	// Info label
	lblInfo := iup.Label(fmt.Sprintf("Virtual mode enabled: 100,000 rows loaded instantly!\n" +
		"Memory usage is minimal, data is fetched on demand via VALUE_CB.\n" +
		"Click column headers to sort (application handles sorting via SORT_CB)."))

	// Status label to show current position
	lblStatus := iup.Label("Click any column header to sort the table")
	iup.SetHandle("status", lblStatus)

	// ENTERITEM_CB to show current position
	iup.SetCallback(table, "ENTERITEM_CB", iup.EnterItemFunc(func(ih iup.Ihandle, lin, col int) int {
		status := iup.GetHandle("status")
		if sortColumn > 0 {
			columnTitle := ih.GetAttribute(fmt.Sprintf("TITLE%d", sortColumn))
			direction := "ascending"
			if !sortAscending {
				direction = "descending"
			}
			status.SetAttribute("TITLE", fmt.Sprintf("Row %d, Col %d - Sorted by %s (%s)", lin, col, columnTitle, direction))
		} else {
			status.SetAttribute("TITLE", fmt.Sprintf("Current position: Row %d, Column %d", lin, col))
		}
		return iup.DEFAULT
	}))

	// Create dialog
	dlg := iup.Dialog(
		iup.Vbox(
			iup.Label("IupTable: Virtual Mode Demo").SetAttributes(`FONT="Sans, Bold 12"`),
			lblInfo,
			table,
			lblStatus,
		).SetAttributes("MARGIN=10x10, GAP=10"),
	)

	dlg.SetAttributes(`TITLE="IupTable Virtual Mode"`)

	iup.Show(dlg)
	iup.MainLoop()
}
