//go:build !cgo && !js && windows

package iup

import (
	"math"
	"os"
	"reflect"
	"runtime"
	"syscall"
)

var getCurrentThreadID = syscall.NewLazyDLL("kernel32.dll").NewProc("GetCurrentThreadId")

func registerThreadFunc(lib uintptr) {}

func currentThreadID() uint64 {
	id, _, _ := getCurrentThreadID.Call()

	return uint64(id)
}

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

const maxFloatCallbacks = 24

var (
	floatStubs     [maxFloatCallbacks]uintptr
	floatTargets   [maxFloatCallbacks]uintptr
	floatScratch   [8]uint64
	floatCallbacks int
)

// newFloatCallback is purego.NewCallback for float arguments, which syscall.NewCallback rejects: a stub saves the
// float registers in floatScratch first. One scratch is enough, IUP raises these callbacks on the main thread only.
func newFloatCallback(fn any) uintptr {
	if floatStubs[0] == 0 {
		panic("iup: callbacks with float arguments are not supported on windows/" + runtime.GOARCH)
	}
	v := reflect.ValueOf(fn)
	t := v.Type()
	positional := runtime.GOARCH == "amd64"
	regs := 8
	if positional {
		regs = 4
	}

	var in []reflect.Type
	floats := 0
	for i := 0; i < t.NumIn(); i++ {
		switch t.In(i).Kind() {
		case reflect.Float32, reflect.Float64:
			if positional {
				in = append(in, reflect.TypeOf(uintptr(0)))
			} else if floats++; floats > regs {
				panic("iup: too many float arguments in a callback")
			}
		default:
			in = append(in, t.In(i))
		}
	}

	wrapper := reflect.MakeFunc(reflect.FuncOf(in, []reflect.Type{t.Out(0)}, false), func(args []reflect.Value) []reflect.Value {
		saved := floatScratch
		full := make([]reflect.Value, t.NumIn())
		next, float := 0, 0
		for i := range full {
			kind := t.In(i).Kind()
			if kind != reflect.Float32 && kind != reflect.Float64 {
				full[i] = args[next]
				next++
				continue
			}
			var bits uint64
			switch {
			case !positional:
				bits = saved[float]
				float++
			case i < regs:
				bits = saved[i]
				next++
			default:
				bits = args[next].Uint()
				next++
			}
			if kind == reflect.Float32 {
				full[i] = reflect.ValueOf(math.Float32frombits(uint32(bits)))
			} else {
				full[i] = reflect.ValueOf(math.Float64frombits(bits))
			}
		}
		return v.Call(full)
	})

	if floatCallbacks == maxFloatCallbacks {
		panic("iup: too many float callbacks")
	}
	slot := floatCallbacks
	floatCallbacks++
	floatTargets[slot] = syscall.NewCallback(wrapper.Interface())
	return floatStubs[slot]
}
