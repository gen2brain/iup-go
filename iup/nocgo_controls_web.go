//go:build !cgo && !js && web

package iup

import "github.com/ebitengine/purego"

var (
	iupWebBrowserOpen func()
	iupWebBrowser     func() uintptr
)

func init() {
	ensureBase()
	lib := openLib("iupweb")
	if lib == 0 {
		panic("iup: cannot load libiupweb")
	}
	purego.RegisterLibFunc(&iupWebBrowserOpen, lib, "IupWebBrowserOpen")
	purego.RegisterLibFunc(&iupWebBrowser, lib, "IupWebBrowser")
}

func WebBrowserOpen() { iupWebBrowserOpen() }

func WebBrowser() Ihandle { return mkih(iupWebBrowser()) }
