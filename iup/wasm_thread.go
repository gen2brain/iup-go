//go:build js && wasm

package iup

import (
	"runtime"
	"sync"
)

type wasmThread struct {
	lock    sync.Mutex
	done    chan struct{}
	running bool
}

var (
	wasmThreadsMu sync.Mutex
	wasmThreads   = map[Ihandle]*wasmThread{}
)

func wasmRegisterThread(ih Ihandle) {
	wasmThreadsMu.Lock()
	wasmThreads[ih] = &wasmThread{}
	wasmThreadsMu.Unlock()
}

func wasmAttrBool(value interface{}) bool {
	switch v := value.(type) {
	case bool:
		return v
	case int:
		return v != 0
	case string:
		return v == "YES" || v == "yes" || v == "1" || v == "ON" || v == "on"
	}
	return false
}

func wasmThreadSetAttr(ih Ihandle, name string, value interface{}) bool {
	wasmThreadsMu.Lock()
	t := wasmThreads[ih]
	wasmThreadsMu.Unlock()
	if t == nil {
		return false
	}

	switch name {
	case "START":
		if t.running || !wasmAttrBool(value) {
			return true
		}
		cb, ok := callbacks[cbKey{ih, "THREAD_CB"}].(ThreadFunc)
		if !ok || cb == nil {
			return true
		}
		t.running = true
		t.done = make(chan struct{})
		go func() {
			cb(ih)
			wasmThreadsMu.Lock()
			t.running = false
			close(t.done)
			wasmThreadsMu.Unlock()
		}()
		return true
	case "LOCK":
		if wasmAttrBool(value) {
			t.lock.Lock()
		} else {
			t.lock.Unlock()
		}
		return true
	case "JOIN":
		wasmThreadsMu.Lock()
		done := t.done
		wasmThreadsMu.Unlock()
		if done != nil {
			<-done
		}
		return true
	case "YIELD":
		runtime.Gosched()
		return true
	case "EXIT":
		return true
	}
	return false
}
