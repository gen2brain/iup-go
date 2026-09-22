//go:build web

package iup

/*
#include "iup.h"
#include "iupweb.h"
*/
import "C"

// WebBrowserOpen must be called after Open, so that the control can be used.
//
// https://gen2brain.github.io/iup-go/ctrl/iup_web.html
func WebBrowserOpen() {
	C.IupWebBrowserOpen()
}

// WebBrowser creates a web browser control.
//
// https://gen2brain.github.io/iup-go/ctrl/iup_web.html
func WebBrowser() Ihandle {
	h := mkih(C.IupWebBrowser())
	return h
}
