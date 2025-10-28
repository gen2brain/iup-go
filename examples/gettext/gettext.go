package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	if text, ret := iup.GetText("Title", "Text", 1024); ret != 0 {
		fmt.Println("Text:", text)
	}
}
