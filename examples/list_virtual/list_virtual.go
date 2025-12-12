package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

// Simulated large dataset
var dataset []string

func main() {
	iup.Open()
	defer iup.Close()

	// Simulate a large dataset
	dataset = make([]string, 100000)
	for i := 0; i < 100000; i++ {
		dataset[i] = fmt.Sprintf("Item %d - This is the description for item number %d", i+1, i+1)
	}

	// Create list in virtual mode (plain list, no dropdown, no editbox)
	list := iup.List()

	// Enable virtual mode BEFORE setting item count
	list.SetAttribute("VIRTUALMODE", "YES")

	// Set the number of items
	list.SetAttribute("ITEMCOUNT", "100000")

	// Configure list appearance
	list.SetAttribute("VISIBLELINES", "15")
	list.SetAttribute("EXPAND", "YES")

	// Set VALUE_CB callback to provide data on demand
	iup.SetCallback(list, "VALUE_CB", iup.ListValueFunc(func(ih iup.Ihandle, pos int) string {
		// pos is 1-based
		if pos <= 0 || pos > len(dataset) {
			return ""
		}
		return dataset[pos-1]
	}))

	// Status label to show selection
	lblStatus := iup.Label("Select an item from the list")
	lblStatus.SetAttribute("EXPAND", "HORIZONTAL")
	iup.SetHandle("status", lblStatus)

	// Track selection changes
	iup.SetCallback(list, "ACTION", iup.ListActionFunc(func(ih iup.Ihandle, text string, item, state int) int {
		if state == 1 {
			status := iup.GetHandle("status")
			status.SetAttribute("TITLE", fmt.Sprintf("Selected item %d: %s", item, text))
		}
		return iup.DEFAULT
	}))

	// Button to change item count dynamically
	btnChangeCount := iup.Button("Change to 50,000 items")
	iup.SetCallback(btnChangeCount, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		currentCount := list.GetInt("ITEMCOUNT")
		if currentCount == 100000 {
			// Resize dataset
			dataset = dataset[:50000]
			list.SetAttribute("ITEMCOUNT", "50000")
			ih.SetAttribute("TITLE", "Change to 100,000 items")
		} else {
			// Restore dataset
			dataset = make([]string, 100000)
			for i := 0; i < 100000; i++ {
				dataset[i] = fmt.Sprintf("Item %d - This is the description for item number %d", i+1, i+1)
			}
			list.SetAttribute("ITEMCOUNT", "100000")
			ih.SetAttribute("TITLE", "Change to 50,000 items")
		}
		return iup.DEFAULT
	}))

	// Info label
	lblInfo := iup.Label(fmt.Sprintf("Virtual mode enabled: 100,000 items loaded instantly!\n" +
		"Memory usage is minimal, item text is fetched on demand via VALUE_CB.\n" +
		"Only plain lists support VIRTUALMODE (no dropdown, no editbox)."))
	lblInfo.SetAttribute("EXPAND", "HORIZONTAL")

	// Create dialog
	dlg := iup.Dialog(
		iup.Vbox(
			iup.Label("IupList Virtual Mode").SetAttributes(`FONT="Sans, Bold 12"`),
			lblInfo,
			list,
			iup.Hbox(
				btnChangeCount,
				iup.Fill(),
			).SetAttributes("GAP=5"),
			lblStatus,
		).SetAttributes("MARGIN=10x10, GAP=10"),
	)

	dlg.SetAttributes(`TITLE="IupList Virtual Mode"`)

	iup.Show(dlg)
	iup.MainLoop()
}
