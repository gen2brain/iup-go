//go:build ctrl

package main

import "github.com/gen2brain/iup-go/iup"

func init() {
	extraOpens = append(extraOpens, func() { iup.ControlsOpen() })
}
