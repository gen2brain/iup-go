package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	if ret, color := iup.GetColor(100, 100); ret != 0 {
		iup.Message("Color", fmt.Sprintf("RGBA = %v %v %v %v", color.R, color.G, color.B, color.A))
	}
}
