//go:build !cgo && !js && darwin

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

func sysLibNames(base string) []string {
	return []string{"lib" + base + ".dylib", "lib" + base + ".4.dylib"}
}

func canonicalLibName(base string) string { return "lib" + base + ".dylib" }

// afterOpen unlinks the unpacked file; the mapping keeps it valid and deps
// resolve iup by its install name, not path.
func afterOpen(path string) { os.Remove(path) }

func unloadTempLib(h uintptr, path string) {}
