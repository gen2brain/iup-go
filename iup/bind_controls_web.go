//go:build web

package iup

/*
#include "iup.h"
#include "iupweb.h"
*/
import "C"

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
	return h
}
