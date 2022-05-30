package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	switch iup.Alarm("Alarm Example", "File not saved! Save it now?", "Yes", "No", "Cancel") {
	case 1:
		iup.Message("Save File", "File saved successfully - leaving program")
	case 2:
		iup.Message("Save File", "File not saved - leaving program anyway")
	case 3:
		iup.Message("Save File", "Operation canceled")
	}
}
