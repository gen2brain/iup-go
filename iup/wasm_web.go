//go:build js && wasm && web

package iup

// WebBrowserOpen must be called after Open, so that the control can be used.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_web.md
func WebBrowserOpen() {
	ccall("IupWebBrowserOpen", "number", nil, nil)
}

// WebBrowser creates a web browser control. Under WebAssembly it is an HTML <iframe>.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_web.md
func WebBrowser() Ihandle {
	return ccallHandle("IupWebBrowser", nil, nil)
}
