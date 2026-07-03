//go:build !cgo && !js && extlib

package iup

// openLib loads the system-installed library (not bundled). Set the platform
// library search path (LD_LIBRARY_PATH / DYLD_LIBRARY_PATH / PATH) if needed.
func openLib(base string) uintptr {
	for _, name := range sysLibNames(base) {
		if h := dlopenRaw(name); h != 0 {
			return h
		}
	}
	return 0
}

// closeEmbeddedLibs is a no-op with system libraries (nothing was unpacked).
func closeEmbeddedLibs() {}
