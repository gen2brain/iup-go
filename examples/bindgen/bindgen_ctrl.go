//go:build ctrl

package main

import "github.com/gen2brain/iup-go/iup"

func init() {
	extraOpens = append(extraOpens, extraLib{"iupctrl", "IupControlsOpen", func() { iup.ControlsOpen() }})
}
