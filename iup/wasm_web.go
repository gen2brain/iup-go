//go:build js && wasm && web

package iup

// WebBrowserOpen must be called after Open, so that the control can be used.
//
// https://gen2brain.github.io/iup-go/ctrl/iup_web.html
func WebBrowserOpen() {
	ccall("IupWebBrowserOpen", "number", nil, nil)
}

// WebBrowser creates a web browser control. Under WebAssembly it is an HTML <iframe>.
//
// https://gen2brain.github.io/iup-go/ctrl/iup_web.html
func WebBrowser() Ihandle {
	return ccallHandle("IupWebBrowser", nil, nil)
}
