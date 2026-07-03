//go:build !cgo && !js && linux

package iup

import (
	"os"

	"github.com/ebitengine/purego"
)

func dlopenRaw(nameOrPath string) uintptr {
	h, err := purego.Dlopen(nameOrPath, purego.RTLD_NOW|purego.RTLD_GLOBAL)
	if err != nil {
		return 0
	}
	return h
}

func sysLibNames(base string) []string { return []string{"lib" + base + ".so", "lib" + base + ".so.4"} }

func canonicalLibName(base string) string { return "lib" + base + ".so" }

// afterOpen unlinks the unpacked file; the dlopen mapping keeps it valid, so
// nothing lingers on disk even on crash. Deps resolve iup by soname, not path.
func afterOpen(path string) { os.Remove(path) }

func unloadTempLib(h uintptr, path string) {}
