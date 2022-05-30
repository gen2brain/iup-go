package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	file, ret := iup.GetFile("./*.txt")

	switch ret {
	case 1:
		iup.Message("New file", file)
	case 0:
		iup.Message("File already exists", file)
	case -1:
		iup.Message("IupFileDlg", "Operation canceled")
	case -2:
		iup.Message("IupFileDlg", "Allocation error")
	case -3:
		iup.Message("IupFileDlg", "Invalid parameter")
	}
}
