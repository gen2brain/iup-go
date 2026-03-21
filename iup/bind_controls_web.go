//go:build web

package iup

/*
#include <stdlib.h>
#include "iup.h"
#include "iupweb.h"
*/
import "C"

import (
	"github.com/google/uuid"
)

// WebBrowserOpen must be called after Open, so that the control can be used.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_web.md
func WebBrowserOpen() {
	C.IupWebBrowserOpen()
}

// WebBrowser creates a web browser control.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_web.md
func WebBrowser() Ihandle {
	h := mkih(C.IupWebBrowser())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}
