//go:build !cgo && !js

package iup

import (
	"os"
	"runtime"
	"unsafe"
)

func init() {
	runtime.LockOSThread()
}

func goPtr(p uintptr) unsafe.Pointer {
	return *(*unsafe.Pointer)(unsafe.Pointer(&p))
}

func goString(p uintptr) string {
	if p == 0 {
		return ""
	}
	var n int
	for *(*byte)(goPtr(p + uintptr(n))) != 0 {
		n++
	}
	return string(unsafe.Slice((*byte)(goPtr(p)), n))
}

func optCStr(s string) *byte {
	if s == "" {
		return nil
	}
	b := append([]byte(s), 0)
	return &b[0]
}

func openShared() int {
	ret := int(iupOpen(0, 0))

	SetGlobal("UTF8MODE", "YES")
	SetGlobal("UTF8MODE_FILE", "YES")
	if len(os.Args) > 0 {
		SetGlobal("ARGV0", os.Args[0])
	}
	return ret
}

func Open() int {
	return openShared()
}

func Close() {
	iupClose()
	closeEmbeddedLibs()
}

func EntryPoint(entry func()) {}

func Version() string {
	return iupVersion()
}

func VersionDate() string {
	return iupVersionDate()
}

func VersionNumber() int {
	return int(iupVersionNumber())
}

func bool2int(v bool) int {
	if v {
		return 1
	}
	return 0
}
