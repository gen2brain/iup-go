//go:build !cgo && !js && !extlib

package iup

import (
	"bytes"
	"compress/gzip"
	"io"
	"os"
	"path/filepath"
	"sync"
)

// embeddedLibs maps a library base name (iup, iupctrl, ...) to its gzipped
// shared object, populated by the per-platform nocgo_embed_*.go files.
var embeddedLibs = map[string][]byte{}

// var initializer (runs before any init()) so the map is ready before tagged loaders' init().
func regEmbed(name string, data []byte) struct{} {
	embeddedLibs[name] = data
	return struct{}{}
}

var (
	libDir     string
	libDirOnce sync.Once
	loadedLibs []loadedLib
)

type loadedLib struct {
	h    uintptr
	path string
}

func ensureLibDir() string {
	libDirOnce.Do(func() {
		if d, err := os.MkdirTemp("", "iupgo-"); err == nil {
			libDir = d
		}
	})
	return libDir
}

// openLib unpacks the embedded gzipped library into a shared temp dir under its
// canonical filename, then loads it. Canonical naming lets dependent libs
// (iupctrl/iupplot/...) resolve their reference to iup once it is loaded first.
func openLib(base string) uintptr {
	gz, ok := embeddedLibs[base]
	if !ok {
		return 0
	}
	dir := ensureLibDir()
	if dir == "" {
		return 0
	}
	zr, err := gzip.NewReader(bytes.NewReader(gz))
	if err != nil {
		return 0
	}
	path := filepath.Join(dir, canonicalLibName(base))
	f, err := os.OpenFile(path, os.O_CREATE|os.O_WRONLY|os.O_TRUNC, 0o700)
	if err != nil {
		zr.Close()
		return 0
	}
	_, err = io.Copy(f, zr)
	f.Close()
	zr.Close()
	if err != nil {
		os.Remove(path)
		return 0
	}
	h := dlopenRaw(path)
	if h == 0 {
		os.Remove(path)
		return 0
	}
	loadedLibs = append(loadedLibs, loadedLib{h, path})
	afterOpen(path)
	return h
}

// closeEmbeddedLibs is called from Close; unloads and removes the unpacked libs
// (a no-op on unix where afterOpen already unlinked them).
func closeEmbeddedLibs() {
	for i := len(loadedLibs) - 1; i >= 0; i-- {
		unloadTempLib(loadedLibs[i].h, loadedLibs[i].path)
	}
	loadedLibs = nil
	if libDir != "" {
		os.RemoveAll(libDir)
	}
}
