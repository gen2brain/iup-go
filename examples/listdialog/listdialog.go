package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	list := []string{
		"Blue",
		"Red",
		"Green",
		"Yellow",
		"Black",
		"White",
		"Gray",
		"Brown",
	}

	marks := []bool{false, false, false, false, true, true, false, false}

	if iup.ListDialog(2, "Color Selection", list, 0, 16, 5, &marks) == -1 {
		iup.Message("ListDialog Example", "Operation canceled")
	} else {
		selection := ""
		for i, mark := range marks {
			if mark {
				selection += list[i] + " "
			}
		}

		if len(selection) == 0 {
			iup.Message("ListDialog Example", "No option selected")
		} else {
			iup.Message("ListDialog Example", "Selected colors: "+selection)
		}
	}
}
