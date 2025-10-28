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
// https://www.tecgraf.puc-rio.br/iup/en/ctrl/iupweb.html
func WebBrowserOpen() {
	C.IupWebBrowserOpen()
}

// WebBrowser creates a web browser control.
//
// https://www.tecgraf.puc-rio.br/iup/en/ctrl/iupweb.html
func WebBrowser() Ihandle {
	h := mkih(C.IupWebBrowser())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}
