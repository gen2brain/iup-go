//go:build !cgo && !js && windows

package iup

import (
	"os"
	"syscall"
)

func dlopenRaw(nameOrPath string) uintptr {
	h, err := syscall.LoadLibrary(nameOrPath)
	if err != nil {
		return 0
	}
	return uintptr(h)
}

func sysLibNames(base string) []string { return []string{base + ".dll", "lib" + base + ".dll"} }

// canonicalLibName matches the DLL import name recorded in dependent libs, so
// loading iup first lets iupctrl/iupplot resolve it by base filename.
func canonicalLibName(base string) string { return "lib" + base + ".dll" }

// afterOpen is a no-op: a loaded DLL cannot be deleted while mapped; removal is
// deferred to unloadTempLib on Close.
func afterOpen(path string) {}

func unloadTempLib(h uintptr, path string) {
	syscall.FreeLibrary(syscall.Handle(h))
	os.Remove(path)
}
